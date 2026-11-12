#include "Rpg/VortexArmor.h"

#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexSwords.h"
#include "Creatures/VortexCreature.h"
#include "Dom/JsonObject.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NiagaraFunctionLibrary.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexRpg.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "World/VortexDayNightSubsystem.h"

namespace
{
    struct FPassive
    {
        FName Kind;
        TMap<FName, float> Values;
        float Value(FName Key) const { return Values.FindRef(Key); }
    };

    struct FTable
    {
        TMap<FName, VortexArmor::FPiece> Pieces;
        TMap<FName, FPassive> PiecePassive;   // item -> a passiva da peça
        TMap<FName, FPassive> SetPassive;     // conjunto -> a passiva das 4 peças
        TMap<FName, FName> Visual;            // armadura antiga -> a peça nova com o visual dela
    };

    FPassive ReadPassive(const TSharedPtr<FJsonObject>& O)
    {
        FPassive P;
        if (!O) return P;
        P.Kind = FName(*O->GetStringField(TEXT("kind")));
        const TSharedPtr<FJsonObject>* Values = nullptr;
        if (O->TryGetObjectField(TEXT("values"), Values))
            for (const auto& V : (*Values)->Values) P.Values.Add(FName(*V.Key), float(V.Value->AsNumber()));
        return P;
    }

    const FTable& Table()
    {
        static FTable T = []()
        {
            FTable Out;
            FString Text;
            TSharedPtr<FJsonObject> Root;
            if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Rpg/armors.json"))) ||
                !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root) return Out;
            for (const auto& Pair : Root->GetObjectField(TEXT("pieces"))->Values)
            {
                const TSharedPtr<FJsonObject> O = Pair.Value->AsObject();
                VortexArmor::FPiece Piece;
                Piece.Set = FName(*O->GetStringField(TEXT("set")));
                Piece.Slot = FName(*O->GetStringField(TEXT("slot")));
                Piece.Mesh = O->GetStringField(TEXT("mesh"));
                FString Hair;
                Piece.bFringe = O->TryGetStringField(TEXT("hair"), Hair) && Hair == TEXT("fringe");
                Out.Pieces.Add(FName(*Pair.Key), Piece);
                Out.PiecePassive.Add(FName(*Pair.Key), ReadPassive(O->GetObjectField(TEXT("passive"))));
            }
            for (const auto& Pair : Root->GetObjectField(TEXT("sets"))->Values)
                Out.SetPassive.Add(FName(*Pair.Key), ReadPassive(Pair.Value->AsObject()->GetObjectField(TEXT("passive4"))));
            const TSharedPtr<FJsonObject>* Visual = nullptr;
            if (Root->TryGetObjectField(TEXT("visual"), Visual))
                for (const auto& Pair : (*Visual)->Values) Out.Visual.Add(FName(*Pair.Key), FName(*Pair.Value->AsString()));
            UE_LOG(LogTemp, Display, TEXT("VORTEX_ARMOR %d peças, %d conjuntos, %d antigas com visual"), Out.Pieces.Num(), Out.SetPassive.Num(), Out.Visual.Num());
            return Out;
        }();
        return T;
    }

    /** As passivas ativas: as das peças vestidas e a do conjunto completo. */
    void Active(const UVortexHeroSheet* Sheet, TArray<const FPassive*>& Out)
    {
        if (!Sheet) return;
        const FTable& T = Table();
        TMap<FName, int32> Count;
        for (int32 Slot : { int32(VortexRpg::Head), int32(VortexRpg::Chest), int32(VortexRpg::Legs), int32(VortexRpg::Boots) })
        {
            const FName Item = FVortexRpgCatalog::BaseOf(Sheet->GetEquipped(Slot));   // a passiva é a mesma em qualquer nível
            if (const FPassive* P = T.PiecePassive.Find(Item)) Out.Add(P);
            if (const VortexArmor::FPiece* Piece = T.Pieces.Find(Item)) Count.FindOrAdd(Piece->Set)++;
        }
        for (const TPair<FName, int32>& C : Count)
            if (C.Value >= 4)
                if (const FPassive* P = T.SetPassive.Find(C.Key)) Out.Add(P);
    }

    /** O menor valor (os tempos de espera), 0 sem a passiva. */
    float Least(const UVortexHeroSheet* Sheet, FName Kind, FName Key)
    {
        TArray<const FPassive*> List;
        Active(Sheet, List);
        float Best = 0.f;
        for (const FPassive* P : List)
            if (P->Kind == Kind && (Best <= 0.f || P->Value(Key) < Best)) Best = P->Value(Key);
        return Best;
    }

    /** O estado de cada herói (as esperas e os relógios), só no servidor. */
    struct FState
    {
        double FirstHit = -1.e9, Revive = -1.e9, Dodged = -1.e9;
        float Regen = 0.f, Aura = 0.f, Star = 0.f;
    };
    FState& StateOf(const AActor* Hero)
    {
        static TMap<TWeakObjectPtr<const AActor>, FState> All;
        if (All.Num() > 64) for (auto It = All.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
        return All.FindOrAdd(Hero);
    }

    UAbilitySystemComponent* Abilities(const AActor* Actor)
    {
        const AVortexCharacter* Hero = Cast<AVortexCharacter>(Actor);
        return Hero ? Hero->GetAbilitySystemComponent() : nullptr;
    }

    float Health(const AActor* Actor)
    {
        const UAbilitySystemComponent* A = Abilities(Actor);
        return A ? A->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) : 0.f;
    }

    void Burst(const AActor* At, FName Element, float Scale = 1.f)
    {
        if (!At || At->GetNetMode() == NM_DedicatedServer) return;
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(At->GetWorld(), VortexSwords::ElementBurst(Element), At->GetActorLocation() - FVector(0.f, 0.f, 60.f),
            FRotator::ZeroRotator, FVector(Scale));
    }

    bool IsBeast(const AActor* Target)
    {
        const IVortexCreature* Creature = Cast<IVortexCreature>(Target);
        if (!Creature) return false;
        const FString Species = Creature->GetSpecies().ToString().ToLower();
        for (const TCHAR* Beast : { TEXT("wolf"), TEXT("boar"), TEXT("bear"), TEXT("bat"), TEXT("spider"), TEXT("frog"), TEXT("deer"), TEXT("rabbit"),
                 TEXT("scorpion"), TEXT("snake"), TEXT("fox"), TEXT("lion"), TEXT("croc") })
            if (Species.Contains(Beast)) return true;
        return false;
    }
}

const VortexArmor::FPiece* VortexArmor::Piece(FName Item)
{
    return Table().Pieces.Find(FVortexRpgCatalog::BaseOf(Item));   // 07/11: "<id>@<nível>" é a mesma peça
}

const VortexArmor::FPiece* VortexArmor::VisualPiece(FName Item)
{
    const FTable& T = Table();
    Item = FVortexRpgCatalog::BaseOf(Item);
    if (const FPiece* Own = T.Pieces.Find(Item)) return Own;
    const FName* Alias = T.Visual.Find(Item);
    return Alias ? T.Pieces.Find(*Alias) : nullptr;
}

float VortexArmor::Sum(const UVortexHeroSheet* Sheet, FName Kind, FName Key)
{
    TArray<const FPassive*> List;
    Active(Sheet, List);
    float Total = 0.f;
    for (const FPassive* P : List) if (P->Kind == Kind) Total += P->Value(Key);
    return Total;
}

bool VortexArmor::Has(const UVortexHeroSheet* Sheet, FName Kind)
{
    TArray<const FPassive*> List;
    Active(Sheet, List);
    return List.ContainsByPredicate([Kind](const FPassive* P) { return P->Kind == Kind; });
}

float VortexArmor::Incoming(UVortexHeroSheet* Target, AActor* Source, float Damage)
{
    AActor* Hero = Target ? Target->GetOwner() : nullptr;
    if (!Hero || !Hero->HasAuthority() || Damage <= 0.f) return Damage;
    TArray<const FPassive*> List;
    Active(Target, List);
    if (List.IsEmpty()) return Damage;
    FState& St = StateOf(Hero);
    const double Now = Hero->GetWorld()->GetTimeSeconds();
    // bloqueio: anula o golpe
    if (FMath::FRand() < FMath::Min(.6f, Sum(Target, TEXT("block"), TEXT("chance"))))
    {
        Target->PushNotice(TEXT("Bloqueado!"));
        VortexAudio::Play(Hero, TEXT("skill_barrier_hit"), Hero->GetActorLocation(), .5f);
        return 0.f;
    }
    float D = Damage;
    // guarda alta: o primeiro golpe depois da espera
    if (const float Every = Least(Target, TEXT("first_hit"), TEXT("every")); Every > 0.f && Now - St.FirstHit >= Every)
    {
        St.FirstHit = Now;
        D *= 1.f - FMath::Min(.8f, Sum(Target, TEXT("first_hit"), TEXT("amount")));
    }
    const float Max = FMath::Max(1.f, Target->ComputeStats().MaxHealth);
    const float Life = Health(Hero);
    if (Life < Max * .3f) D *= 1.f - FMath::Min(.6f, Sum(Target, TEXT("last_stand"), TEXT("amount")));
    UVortexCombatComponent* Mine = Hero->FindComponentByClass<UVortexCombatComponent>();
    UVortexCombatComponent* Their = Source ? Source->FindComponentByClass<UVortexCombatComponent>() : nullptr;
    if (Their)
    {
        // espinhos: o dano volta direto (sem os atributos do herói por cima)
        if (const float Thorns = Sum(Target, TEXT("thorns"), TEXT("amount")); Thorns > 0.f) Their->ApplyDot(FMath::Max(1.f, D * Thorns), 1.f, Hero);   // o veneno passa a cada 0,5 s: em 1 s, o dano todo
        if (FMath::FRand() < Sum(Target, TEXT("chill"), TEXT("chance"))) Their->ApplySlow(.6f, 2.f);
        if (FMath::FRand() < Sum(Target, TEXT("freeze"), TEXT("chance")))
        {
            Their->Stun(FMath::Max(.5f, Least(Target, TEXT("freeze"), TEXT("time"))));
            Burst(Source, TEXT("ice"), .6f);
        }
        if (FMath::FRand() < Sum(Target, TEXT("burn_attacker"), TEXT("chance"))) Their->ApplyDot(FMath::Max(1.f, Damage * .3f), 3.f, Hero);
    }
    // a lava: explode em volta
    if (Mine && FMath::FRand() < Sum(Target, TEXT("magma_burst"), TEXT("chance")))
    {
        const float Radius = FMath::Max(200.f, Least(Target, TEXT("magma_burst"), TEXT("radius")));
        const float Scale = Sum(Target, TEXT("magma_burst"), TEXT("scale")) / FMath::Max(1, List.FilterByPredicate([](const FPassive* P) { return P->Kind == TEXT("magma_burst"); }).Num());
        TArray<FOverlapResult> Overlaps;
        FCollisionObjectQueryParams Pawns;
        Pawns.AddObjectTypesToQuery(ECC_Pawn);
        Hero->GetWorld()->OverlapMultiByObjectType(Overlaps, Hero->GetActorLocation(), FQuat::Identity, Pawns, FCollisionShape::MakeSphere(Radius));
        TSet<AActor*> Done;
        for (const FOverlapResult& O : Overlaps)
            if (AActor* Other = O.GetActor(); Other && Other != Hero && !Done.Contains(Other) && Mine->CanHarm(Other))
            {
                Done.Add(Other);
                Mine->ApplyHit(Other, Scale, EVortexDamageKind::Magic, 0.f);
            }
        Burst(Hero, TEXT("fire"), Radius / 300.f);
        VortexAudio::Play(Hero, TEXT("boss_impact_core"), Hero->GetActorLocation(), .6f);
    }
    // renascer: o golpe que mataria deixa o herói com parte da vida
    if (D >= Life)
        if (const float Every = Least(Target, TEXT("revive"), TEXT("every")); Every > 0.f && Now - St.Revive >= Every)
        {
            St.Revive = Now;
            Target->HealBy(FMath::Max(0.f, Max * FMath::Min(1.f, Sum(Target, TEXT("revive"), TEXT("amount"))) - Life));
            Target->PushNotice(TEXT("A Aurora te ergueu!"), 5);
            Burst(Hero, TEXT("holy"), 1.4f);
            VortexAudio::Play(Hero, TEXT("level_up"), Hero->GetActorLocation(), .8f);
            return 0.f;
        }
    return D;
}

float VortexArmor::OutgoingScale(const UVortexHeroSheet* Source, const AActor* Target)
{
    if (!Source) return 1.f;
    float Scale = 1.f;
    if (IsBeast(Target)) Scale += Sum(Source, TEXT("beast"), TEXT("amount"));
    if (const float Full = Sum(Source, TEXT("full_power"), TEXT("amount")); Full > 0.f && Health(Source->GetOwner()) >= Source->ComputeStats().MaxHealth * .95f)
        Scale += Full;
    return Scale;
}

bool VortexArmor::ConsumeCrit(AActor* Hero)
{
    const UVortexHeroSheet* Sheet = Hero ? Hero->FindComponentByClass<UVortexHeroSheet>() : nullptr;
    const float Window = Sheet ? Sum(Sheet, TEXT("dodge_crit"), TEXT("window")) : 0.f;
    if (Window <= 0.f) return false;
    FState& St = StateOf(Hero);
    if (Hero->GetWorld()->GetTimeSeconds() - St.Dodged > Window) return false;
    St.Dodged = -1.e9;
    return true;
}

void VortexArmor::OnDealt(AActor* Hero, AActor* Target, float Damage, bool bKilled)
{
    UVortexHeroSheet* Sheet = Hero ? Hero->FindComponentByClass<UVortexHeroSheet>() : nullptr;
    if (!Sheet || !Hero->HasAuthority() || Damage <= 0.f) return;
    UVortexCombatComponent* Their = Target ? Target->FindComponentByClass<UVortexCombatComponent>() : nullptr;
    if (Their && !bKilled)
    {
        if (FMath::FRand() < Sum(Sheet, TEXT("proc_poison"), TEXT("chance"))) Their->ApplyDot(FMath::Max(1.f, Damage * .22f), 4.f, Hero);   // como a arma de veneno
        if (FMath::FRand() < Sum(Sheet, TEXT("proc_burn"), TEXT("chance"))) Their->ApplyDot(FMath::Max(1.f, Damage * .3f), 3.f, Hero);   // como a arma de fogo
    }
    // fome da noite: rouba mais à noite
    const float Day = Sum(Sheet, TEXT("leech"), TEXT("day"));
    if (Day > 0.f)
    {
        const UVortexDayNightSubsystem* Sky = Hero->GetWorld()->GetSubsystem<UVortexDayNightSubsystem>();
        const bool bNight = Sky && Sky->IsActive() && Sky->GetDaylight() < .25f;
        Sheet->HealBy(Damage * (bNight ? Sum(Sheet, TEXT("leech"), TEXT("night")) : Day));
    }
    if (bKilled)
        if (const float Heal = Sum(Sheet, TEXT("kill_heal"), TEXT("amount")); Heal > 0.f) Sheet->HealBy(Sheet->ComputeStats().MaxHealth * Heal);
}

void VortexArmor::OnDodge(AActor* Hero)
{
    if (Hero && Hero->HasAuthority()) StateOf(Hero).Dodged = Hero->GetWorld()->GetTimeSeconds();
}

void VortexArmor::Tick(UVortexHeroSheet* Sheet, float DeltaSeconds)
{
    AActor* Hero = Sheet ? Sheet->GetOwner() : nullptr;
    if (!Hero || !Hero->HasAuthority()) return;
    TArray<const FPassive*> List;
    Active(Sheet, List);
    if (List.IsEmpty() || Health(Hero) <= 0.f) return;
    FState& St = StateOf(Hero);
    // a cada 5 s: a vida e a mana
    if ((St.Regen += DeltaSeconds) >= 5.f)
    {
        St.Regen = 0.f;
        const FVortexHeroStats S = Sheet->ComputeStats();
        if (const float Regen = Sum(Sheet, TEXT("regen"), TEXT("amount")); Regen > 0.f) Sheet->HealBy(S.MaxHealth * Regen);
        if (const float Mana = Sum(Sheet, TEXT("mana_regen"), TEXT("amount")); Mana > 0.f)
            if (UAbilitySystemComponent* A = Abilities(Hero))
            {
                const float Now = A->GetNumericAttribute(UVortexAttributes::GetManaAttribute());
                A->SetNumericAttributeBase(UVortexAttributes::GetManaAttribute(), FMath::Min(S.MaxMana, Now + S.MaxMana * Mana));
            }
    }
    // o abrigo: cura o herói e os aliados em volta, a cada segundo
    if (const float Aura = Sum(Sheet, TEXT("ally_heal"), TEXT("amount")); Aura > 0.f && (St.Aura += DeltaSeconds) >= 1.f)
    {
        St.Aura = 0.f;
        const float Radius = Least(Sheet, TEXT("ally_heal"), TEXT("radius"));
        for (TActorIterator<AVortexCharacter> It(Hero->GetWorld()); It; ++It)
            if (FVector::Dist(It->GetActorLocation(), Hero->GetActorLocation()) <= FMath::Max(Radius, 300.f) && Health(*It) > 0.f)
                if (UVortexHeroSheet* Other = It->FindComponentByClass<UVortexHeroSheet>()) Other->HealBy(Other->ComputeStats().MaxHealth * Aura);
    }
    // a chuva de estrelas: uma estrela no inimigo mais perto
    if (const float Every = Least(Sheet, TEXT("starfall"), TEXT("every")); Every > 0.f && (St.Star += DeltaSeconds) >= Every)
    {
        UVortexCombatComponent* Mine = Hero->FindComponentByClass<UVortexCombatComponent>();
        const float Radius = Least(Sheet, TEXT("starfall"), TEXT("radius"));
        AActor* Best = nullptr;
        float BestDist = Radius;
        for (TActorIterator<APawn> It(Hero->GetWorld()); It; ++It)
        {
            const IVortexCreature* Creature = Cast<IVortexCreature>(*It);
            if (!Mine || !Creature || Creature->IsCreatureDead() || !Mine->CanHarm(*It)) continue;
            const float Dist = FVector::Dist(It->GetActorLocation(), Hero->GetActorLocation());
            if (Dist < BestDist) { BestDist = Dist; Best = *It; }
        }
        if (Best)
        {
            St.Star = 0.f;
            Mine->ApplyHit(Best, Sum(Sheet, TEXT("starfall"), TEXT("damage")), EVortexDamageKind::Magic, .3f);
            Burst(Best, TEXT("holy"), .9f);
            VortexAudio::Play(Best, TEXT("holy_cast"), Best->GetActorLocation(), .7f);
        }
    }
}

float VortexArmor::MoveScale(const UVortexHeroSheet* Sheet)
{
    return 1.f + FMath::Min(.35f, Sum(Sheet, TEXT("move"), TEXT("amount")));
}

float VortexArmor::DodgeCooldownScale(const AActor* Hero)
{
    const UVortexHeroSheet* Sheet = Hero ? Hero->FindComponentByClass<UVortexHeroSheet>() : nullptr;
    return 1.f - FMath::Min(.5f, Sum(Sheet, TEXT("dodge_cd"), TEXT("amount")));
}
