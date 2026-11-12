#include "Creatures/VortexVolcanicBoss.h"

#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexLobbed.h"
#include "Combat/VortexTelegraph.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Creatures/VortexMonster.h"
#include "Creatures/VortexMonsterSpecies.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

namespace
{
    const FLinearColor Magma(1.f, .38f, .06f);
    const TCHAR* FxFolder = TEXT("/Game/Vortex/Bosses/Azhur/FX/");

    bool IsPhysical(FName Id)
    {
        return Id == TEXT("hook") || Id == TEXT("hammer") || Id == TEXT("stomp") || Id == TEXT("sweep") || Id == TEXT("combo");
    }
}

UVortexVolcanicBoss::UVortexVolcanicBoss()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UVortexVolcanicBoss::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UVortexVolcanicBoss, State);
}

void UVortexVolcanicBoss::BeginPlay()
{
    Super::BeginPlay();
    Load();
}

AVortexMonster* UVortexVolcanicBoss::Body() const { return Cast<AVortexMonster>(GetOwner()); }

AVortexCharacter* UVortexVolcanicBoss::Target() const
{
    const AVortexMonster* M = Body();
    return M ? M->Target.Get() : nullptr;
}

// ---- dados -------------------------------------------------------------------------------------------

void UVortexVolcanicBoss::Load()
{
    FString Text;
    TSharedPtr<FJsonObject> Root;
    if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Creatures/azhur.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root)
    {
        UE_LOG(LogTemp, Error, TEXT("VORTEX_AZHUR sem Data/Creatures/azhur.json"));
        return;
    }
    auto Numbers = [](const TSharedPtr<FJsonObject>& O, TMap<FName, float>& Into)
    {
        for (const TPair<FString, TSharedPtr<FJsonValue>> Field : O->Values)
            if (Field.Value->Type == EJson::Number) Into.Add(FName(*Field.Key), float(Field.Value->AsNumber()));
    };
    Numbers(Root, Stats);
    const TSharedPtr<FJsonObject>* Obj = nullptr;
    if (Root->TryGetObjectField(TEXT("stagger"), Obj))
        for (const TPair<FString, TSharedPtr<FJsonValue>> Field : (*Obj)->Values)
            if (Field.Value->Type == EJson::Number) Stats.Add(FName(*(TEXT("stagger.") + Field.Key)), float(Field.Value->AsNumber()));
    if (Root->TryGetObjectField(TEXT("phases"), Obj))
        for (const TPair<FString, TSharedPtr<FJsonValue>> Field : (*Obj)->Values)
            if (Field.Value->Type == EJson::Object) Numbers(Field.Value->AsObject(), PhaseStats.FindOrAdd(FCString::Atoi(*Field.Key)));
    if (Root->TryGetObjectField(TEXT("combos"), Obj))
        for (const TPair<FString, TSharedPtr<FJsonValue>> Field : (*Obj)->Values)
            if (Field.Value->Type == EJson::String) Combos.Add(FName(*Field.Key), FName(*Field.Value->AsString()));
    if (Root->TryGetObjectField(TEXT("abilities"), Obj))
        for (const TPair<FString, TSharedPtr<FJsonValue>> Field : (*Obj)->Values)
        {
            const TSharedPtr<FJsonObject> O = Field.Value->AsObject();
            if (!O) continue;
            FAbility A;
            A.Id = FName(*Field.Key);
            A.Clip = FName(*O->GetStringField(TEXT("clip")));
            A.Label = O->GetStringField(TEXT("label"));
            O->TryGetBoolField(TEXT("magic"), A.bMagic);
            Numbers(O, A.Num);
            const TArray<TSharedPtr<FJsonValue>>* Hits = nullptr;
            if (O->TryGetArrayField(TEXT("hits"), Hits)) for (const TSharedPtr<FJsonValue>& H : *Hits) A.Hits.Add(float(H->AsNumber()));
            Abilities.Add(A);
        }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_AZHUR %d habilidades, fase 2 em %.0f%%"), Abilities.Num(), Stat(TEXT("phase2At"), .4f) * 100.f);
}

float UVortexVolcanicBoss::PhaseStat(const TCHAR* Key, float Default) const
{
    const AVortexMonster* M = Body();
    const TMap<FName, float>* P = PhaseStats.Find(M ? M->Phase : 1);
    const float* V = P ? P->Find(Key) : nullptr;
    return V ? *V : Default;
}

TArray<FName> UVortexVolcanicBoss::AbilityNames() const
{
    TArray<FName> Out;
    for (const FAbility& A : Abilities) Out.Add(A.Id);
    return Out;
}

float UVortexVolcanicBoss::AbilityValue(FName Ability, const TCHAR* Key, float Default) const
{
    const FAbility* A = Abilities.FindByPredicate([&](const FAbility& F) { return F.Id == Ability; });
    if (!A) return Default;
    if (FCString::Strcmp(Key, TEXT("impact")) == 0 && A->Hits.Num()) return A->Hits[0];
    return A->Get(Key, Default);
}

float UVortexVolcanicBoss::IncomingScale() const
{
    return State == EVolcanicState::Stunned ? Stat(TEXT("stagger.damageTaken"), 1.3f) : 1.f;
}

// ---- a máquina de estados (servidor) -----------------------------------------------------------------

void UVortexVolcanicBoss::Enter(EVolcanicState NewState, double Now, float Seconds, const FString& Label)
{
    State = NewState;
    StateUntil = Now + Seconds;
    if (AVortexMonster* M = Body(); M && !Label.IsEmpty()) M->ActionLabel = Label;
}

void UVortexVolcanicBoss::Reset()
{
    ClearPending();
    State = EVolcanicState::Idle;
    Current = INDEX_NONE;
    Recent.Reset();
    ReadyAt.Reset();
    bUltimateUsed = false;
    StaggerFrom = -1.f;
}

void UVortexVolcanicBoss::Think(double Now)
{
    AVortexMonster* M = Body();
    if (!M || !M->HasAuthority()) return;
    LastNow = Now;
    UAbilitySystemComponent* ASC = M->AbilitySystem;
    if (!bHealthSet && ASC)
    {
        // a vida do arquivo (o chefe da Caldeira: igual para 1 a 8 jogadores; o grupo já endurece cada golpe)
        bHealthSet = true;
        const float Health = Stat(TEXT("health"), M->MaxHealthValue);
        M->MaxHealthValue = Health;
        ASC->SetNumericAttributeBase(UVortexAttributes::GetMaxHealthAttribute(), Health);
        ASC->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Health);
    }
    if (M->bDead)
    {
        if (State != EVolcanicState::Death)
        {
            ClearPending();
            Enter(EVolcanicState::Death, Now, 999.f, TEXT("O núcleo se apaga"));
            Fx(TEXT("NS_Volcanic_Death"), M->GetActorLocation(), 3.f, 5.f);
            Sfx(TEXT("azhur_death"), M->GetActorLocation(), 1.f);
        }
        return;
    }
    // quem está na arena (perto de casa, vivo): o mais perto do chefe é o alvo
    AVortexCharacter* Hero = nullptr;
    float Best = Stat(TEXT("detectRange"), 4500.f);
    for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
    {
        const UAbilitySystemComponent* HeroASC = It->GetAbilitySystemComponent();
        if (!HeroASC || HeroASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) <= 0.f) continue;
        if (FVector::Dist2D(It->GetActorLocation(), M->Home) > Stat(TEXT("detectRange"), 4500.f)) continue;
        const float D = FVector::Dist2D(It->GetActorLocation(), M->GetActorLocation());
        if (D < Best) { Hero = *It; Best = D; }
    }
    if (!Hero)
    {
        // todos saíram da arena: depois de 5 s ele volta ao começo
        if (State != EVolcanicState::Idle)
        {
            if (M->AbsentSince < 0.0) M->AbsentSince = Now;
            if (Now - M->AbsentSince > 5.0) { M->ResetEncounter(); Reset(); }
        }
        M->StopMoving();
        return;
    }
    M->AbsentSince = -1.0;
    M->Target = Hero;
    const float Fraction = M->GetHealthFraction();
    const float Health = ASC ? ASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) : 0.f;
    if (StaggerFrom < 0.f) StaggerFrom = Health;

    switch (State)
    {
    case EVolcanicState::Idle:
        // Detection: ajoelhado, ele levanta e ruge (o Awaken), e só então persegue
        M->bAwake = true;
        M->bEngaged = true;
        M->Act(TEXT("Awaken"));
        Sfx(TEXT("azhur_step"), M->GetActorLocation(), .9f, .4f);
        Sfx(TEXT("azhur_roar"), M->GetActorLocation(), 1.f, 1.6f);
        Fx(TEXT("NS_MagmaCore_Charge"), M->GetActorLocation(), 2.f, Stat(TEXT("detectSeconds"), 3.f), FRotator::ZeroRotator, true);
        Enter(EVolcanicState::Detection, Now, Stat(TEXT("detectSeconds"), 3.f), TEXT("O guardião desperta"));
        return;
    case EVolcanicState::Death:
        return;
    default:
        break;
    }
    // a troca de fase (40%): uma vez, com a transição própria
    if (M->Phase == 1 && Fraction <= Stat(TEXT("phase2At"), .4f) && State != EVolcanicState::Detection)
    {
        ClearPending();
        M->Phase = 2;
        M->Act(TEXT("Awaken"));
        M->StopMoving();
        Sfx(TEXT("azhur_explode"), M->GetActorLocation(), .9f);
        Sfx(TEXT("azhur_roar"), M->GetActorLocation(), 1.f, 1.6f);
        Fx(TEXT("NS_MagmaCore_Explosion"), M->GetActorLocation(), 3.5f, 2.5f);
        // a Caldeira responde: poças de lava em volta dele (sem encher a arena)
        for (int32 k = 0; k < 5; ++k)
        {
            const float Angle = 2.f * UE_PI * k / 5.f + .4f;
            M->MulticastLavaPool(Ground(M->GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 900.f), 220.f, 20.f);
        }
        Enter(EVolcanicState::PhaseTransition, Now, Stat(TEXT("phaseTransitionSeconds"), 3.f), TEXT("O NÚCLEO DESPERTA · fase 2"));
        StaggerFrom = Health;
        UE_LOG(LogTemp, Display, TEXT("VORTEX_AZHUR fase 2 (%.0f%%)"), Fraction * 100.f);
        return;
    }
    // atordoamento: dano acumulado demais quebra a guarda dele (nunca na final nem na troca de fase)
    if (State != EVolcanicState::Stunned && State != EVolcanicState::PhaseTransition && State != EVolcanicState::Detection && Now >= StaggerReadyAt &&
        !(Abilities.IsValidIndex(Current) && Abilities[Current].Id == TEXT("ultimate") && State == EVolcanicState::Ability) &&
        StaggerFrom - Health >= Stat(TEXT("stagger.health"), .09f) * M->MaxHealthValue)
    {
        ClearPending();
        M->Act(TEXT("Stun"));
        M->StopMoving();
        Sfx(TEXT("azhur_stun"), M->GetActorLocation(), 1.f);
        Fx(TEXT("NS_Rock_Debris"), Ground(M->GetActorLocation()), 2.f, 2.f);
        Enter(EVolcanicState::Stunned, Now, Stat(TEXT("stagger.stunSeconds"), 4.f), TEXT("ATORDOADO · ataque agora!"));
        StaggerReadyAt = Now + Stat(TEXT("stagger.cooldown"), 14.f);
        StaggerFrom = Health;
        return;
    }
    if (Now < StateUntil)
    {
        if (State == EVolcanicState::Attack || State == EVolcanicState::Ability || State == EVolcanicState::Stunned ||
            State == EVolcanicState::PhaseTransition || State == EVolcanicState::Detection || State == EVolcanicState::Recovery) M->StopMoving();
        return;
    }
    // acabou o estado
    if (State == EVolcanicState::Attack || State == EVolcanicState::Ability)
    {
        // fase 2: às vezes emenda outra habilidade quase sem pausa
        const FName Was = Abilities.IsValidIndex(Current) ? Abilities[Current].Id : NAME_None;
        if (const FName* Next = Combos.Find(Was); Next && !bComboNext && FMath::FRand() < PhaseStat(TEXT("comboChance"), 0.f))
        {
            const int32 Index = Abilities.IndexOfByPredicate([&](const FAbility& A) { return A.Id == *Next; });
            if (Abilities.IsValidIndex(Index))
            {
                bComboNext = true;
                Use(Index, Now);
                return;
            }
        }
        bComboNext = false;
        const float Recovery = (Abilities.IsValidIndex(Current) ? Abilities[Current].Get(TEXT("recovery"), 1.f) : 1.f) * PhaseStat(TEXT("recovery"), 1.f);
        M->Act(NAME_None);
        Enter(EVolcanicState::Recovery, Now, Recovery, TEXT("Brecha após o ataque"));
        return;
    }
    if (State == EVolcanicState::Stunned || State == EVolcanicState::PhaseTransition || State == EVolcanicState::Detection)
    {
        M->Act(NAME_None);
        Enter(EVolcanicState::Recovery, Now, PhaseStat(TEXT("gap"), .8f), TEXT(""));
        return;
    }
    // Chase: escolhe um golpe que caiba na distância; sem nenhum, chega mais perto
    const float Distance = FVector::Dist2D(Hero->GetActorLocation(), M->GetActorLocation());
    const int32 Pick = Choose(Now, Distance);
    if (Pick != INDEX_NONE) { Use(Pick, Now); return; }
    State = EVolcanicState::Chase;
    if (Distance > Stat(TEXT("chaseUntil"), 520.f) + M->Def()->Radius)
        M->MoveTo(Hero->GetActorLocation(), M->Phase >= 2 ? Stat(TEXT("speedPhase2"), 235.f) : Stat(TEXT("speed"), 190.f));
    else M->StopMoving();
}

int32 UVortexVolcanicBoss::Choose(double Now, float Distance) const
{
    const AVortexMonster* M = Body();
    const int32 Phase = M ? M->Phase : 1;
    const float Fraction = M ? M->GetHealthFraction() : 1.f;
    // a final: a primeira vez quando a vida passa de firstAt; na fase 2 entra no sorteio quando recarrega
    for (int32 i = 0; i < Abilities.Num(); ++i)
    {
        const FAbility& A = Abilities[i];
        if (A.Id != TEXT("ultimate")) continue;
        const double* Ready = ReadyAt.Find(A.Id);
        if (!bUltimateUsed && Fraction <= A.Get(TEXT("firstAt"), .75f)) return i;
        if (Phase >= 2 && bUltimateUsed && (!Ready || Now >= *Ready) && FMath::FRand() < .35f) return i;
    }
    float Total = 0.f;
    TArray<TPair<int32, float>> Options;
    for (int32 i = 0; i < Abilities.Num(); ++i)
    {
        const FAbility& A = Abilities[i];
        const float Weight = A.Get(TEXT("weight"), 1.f);
        if (Weight <= 0.f || A.Get(TEXT("phase"), 1.f) > Phase) continue;
        if (Distance < A.Get(TEXT("min"), 0.f) || Distance > A.Get(TEXT("max"), 99999.f)) continue;
        if (const double* Ready = ReadyAt.Find(A.Id); Ready && Now < *Ready) continue;
        // sem repetir: o último não volta, o penúltimo vale a metade
        if (Recent.Num() && Recent.Last() == A.Id) continue;
        const float W = Weight * (Recent.Num() > 1 && Recent[Recent.Num() - 2] == A.Id ? .5f : 1.f);
        Options.Add({ i, W });
        Total += W;
    }
    if (Options.IsEmpty()) return INDEX_NONE;
    float Roll = FMath::FRand() * Total;
    for (const TPair<int32, float>& O : Options) { Roll -= O.Value; if (Roll <= 0.f) return O.Key; }
    return Options.Last().Key;
}

bool UVortexVolcanicBoss::DebugUse(FName Ability)
{
    const int32 Index = Abilities.IndexOfByPredicate([&](const FAbility& A) { return A.Id == Ability; });
    if (!Abilities.IsValidIndex(Index) || !Target()) return false;
    Use(Index, GetWorld()->GetTimeSeconds());
    return true;
}

void UVortexVolcanicBoss::Use(int32 Index, double Now)
{
    AVortexMonster* M = Body();
    AVortexCharacter* Hero = Target();
    if (!M || !Hero || !Abilities.IsValidIndex(Index)) return;
    const FAbility& A = Abilities[Index];
    Current = Index;
    Recent.Add(A.Id);
    if (Recent.Num() > 3) Recent.RemoveAt(0);
    ReadyAt.Add(A.Id, Now + A.Get(TEXT("cooldown"), 6.f) * PhaseStat(TEXT("cooldown"), 1.f));
    if (A.Id == TEXT("ultimate")) bUltimateUsed = true;
    M->StopMoving();
    const FVector Toward = (Hero->GetActorLocation() - M->GetActorLocation()).GetSafeNormal2D();
    if (!Toward.IsNearlyZero()) M->SetActorRotation(FRotator(0.f, Toward.Rotation().Yaw, 0.f));
    M->Act(A.Clip);
    Enter(IsPhysical(A.Id) ? EVolcanicState::Attack : EVolcanicState::Ability, Now, A.Get(TEXT("duration"), 2.f), A.Label);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_AZHUR %s (%s, fase %d, distância %.0f)"), *A.Id.ToString(), *A.Clip.ToString(), int32(M->Phase),
        FVector::Dist2D(Hero->GetActorLocation(), M->GetActorLocation()));
    if (A.Id == TEXT("hook")) DoHook(A);
    else if (A.Id == TEXT("hammer")) DoHammer(A);
    else if (A.Id == TEXT("stomp")) DoStomp(A);
    else if (A.Id == TEXT("sweep")) DoSweep(A);
    else if (A.Id == TEXT("combo")) DoCombo(A);
    else if (A.Id == TEXT("coreBurst")) DoCoreBurst(A);
    else if (A.Id == TEXT("fissure"))
        DoFissure(A, M->GetActorLocation() + Toward * (M->Def()->Radius + 300.f), Toward, A.Get(TEXT("impact"), .8f), int32(A.Get(TEXT("segments"), 9.f)));
    else if (A.Id == TEXT("meteor")) DoMeteor(A);
    else if (A.Id == TEXT("beam")) DoBeam(A);
    else if (A.Id == TEXT("eruption")) DoEruption(A);
    else if (A.Id == TEXT("shed")) DoShed(A);
    else if (A.Id == TEXT("ultimate")) DoUltimate(A);
}

// ---- ajudas -------------------------------------------------------------------------------------------

void UVortexVolcanicBoss::ClearPending()
{
    for (FTimerHandle& H : Pending) GetWorld()->GetTimerManager().ClearTimer(H);
    Pending.Reset();
}

void UVortexVolcanicBoss::Later(float Seconds, TFunction<void()> Do)
{
    FTimerHandle H;
    if (Seconds <= 0.f) { Do(); return; }
    GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [Do]() { Do(); }), Seconds, false);
    Pending.Add(H);
}

float UVortexVolcanicBoss::Hit(const FAbility& A, const TCHAR* Key, float Default) const
{
    const AVortexMonster* M = Body();
    return FMath::Max(1.f, FMath::RoundToFloat((M ? M->AttackDamage : 40.f) * A.Get(Key, Default) * PhaseStat(TEXT("damage"), 1.f)));
}

FVector UVortexVolcanicBoss::Ground(const FVector& At) const
{
    const AVortexMonster* M = Body();
    float Z = M ? M->GetActorLocation().Z - M->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : At.Z;
    if (const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData())
    {
        const float G = Terrain->GroundAt(At.X, At.Y);
        if (G > -299.f) Z = G;
    }
    return FVector(At.X, At.Y, Z);
}

void UVortexVolcanicBoss::Mark(const FVector& At, float Radius, float Delay, float Damage, bool bRing, bool bMagic, float Stun)
{
    if (AVortexMonster* M = Body())
        AVortexTelegraph::Spawn(M, Ground(At), Radius, FMath::Max(.15f, Delay), Damage, bRing, Magma, bMagic, Stun);
}

void UVortexVolcanicBoss::Fx(FName System, const FVector& At, float Scale, float Life, const FRotator& Rotation, bool bFollow)
{
    // fxScale (azhur.json): todos os efeitos dele juntos, para não tapar a leitura dos golpes
    MulticastFx(System, At, Rotation, Scale * Stat(TEXT("fxScale"), .6f), Life, bFollow);
}

// ---- os golpes físicos ------------------------------------------------------------------------------

void UVortexVolcanicBoss::DoHook(const FAbility& A)
{
    // Punho Devastador: um arco de marcas à frente (o soco horizontal varre da direita para a esquerda)
    AVortexMonster* M = Body();
    const FVector Here = M->GetActorLocation(), Forward = M->GetActorForwardVector();
    const float Arc = A.Get(TEXT("arc"), 150.f), Reach = M->Def()->Radius + A.Get(TEXT("reach"), 650.f);
    for (int32 i = 0; i < 5; ++i)
    {
        const float Angle = -Arc * .5f + Arc * i / 4.f;
        Mark(Here + Forward.RotateAngleAxis(Angle, FVector::UpVector) * Reach * .75f, A.Get(TEXT("radius"), 300.f), A.Get(TEXT("impact"), 1.f), Hit(A), false, false, A.Get(TEXT("stun")));
    }
    Later(A.Get(TEXT("impact"), 1.f), [this, Here, Forward, Reach]() { Fx(TEXT("NS_Stone_Impact"), Ground(Here + Forward * Reach * .7f), 1.4f, 1.2f); });
    Sfx(TEXT("azhur_whoosh"), Here, 1.f, A.Get(TEXT("impact"), 1.f) - .35f);
    Sfx(TEXT("azhur_slam"), Here + Forward * Reach * .7f, .6f, A.Get(TEXT("impact"), 1.f));
}

void UVortexVolcanicBoss::DoHammer(const FAbility& A)
{
    // Martelo Colossal: os dois punhos à frente; na fase 2 a onda de choque em anel logo depois
    AVortexMonster* M = Body();
    const FVector At = M->GetActorLocation() + M->GetActorForwardVector() * (M->Def()->Radius + A.Get(TEXT("reach"), 520.f));
    const float Impact = A.Get(TEXT("impact"), 1.46f);
    Mark(At, A.Get(TEXT("radius"), 430.f), Impact, Hit(A), false, false, .4f);
    if (M->Phase >= 2) Mark(At, A.Get(TEXT("ring"), 1050.f), Impact + .45f, Hit(A) * .6f, true, false);
    Later(Impact, [this, At]() { Fx(TEXT("NS_Stone_Impact"), Ground(At), 2.f, 1.4f); Fx(TEXT("NS_Rock_Debris"), Ground(At), 1.4f, 1.6f); });
    Sfx(TEXT("azhur_whoosh"), At, .8f, Impact - .3f);
    Sfx(TEXT("azhur_slam"), At, 1.f, Impact);
}

void UVortexVolcanicBoss::DoStomp(const FAbility& A)
{
    AVortexMonster* M = Body();
    const FVector Here = M->GetActorLocation();
    const float Impact = A.Get(TEXT("impact"), 1.07f);
    Mark(Here, A.Get(TEXT("radius"), 650.f), Impact, Hit(A), false, false, A.Get(TEXT("stun")));
    if (M->Phase >= 2) Mark(Here, A.Get(TEXT("ring2"), 1250.f), Impact + .5f, Hit(A) * .7f, true, false);
    Later(Impact, [this, Here]() { Fx(TEXT("NS_Stone_Impact"), Ground(Here), 2.4f, 1.4f); });
    Sfx(TEXT("azhur_slam"), Here, 1.f, Impact);
    if (M->Phase >= 2) Sfx(TEXT("azhur_slam"), Here, .6f, Impact + .5f);
}

void UVortexVolcanicBoss::DoSweep(const FAbility& A)
{
    // Varredura: meio círculo de marcas que estouram em sequência, da direita para a esquerda, junto com o braço
    AVortexMonster* M = Body();
    const FVector Here = M->GetActorLocation(), Forward = M->GetActorForwardVector();
    const float Arc = A.Get(TEXT("arc"), 190.f), Reach = M->Def()->Radius + A.Get(TEXT("reach"), 760.f) * .7f;
    const float From = A.Get(TEXT("impact"), .95f), To = A.Get(TEXT("impactEnd"), 1.38f);
    const int32 Count = 7;
    Sfx(TEXT("azhur_whoosh"), Here, 1.f, From - .25f);
    Sfx(TEXT("azhur_slam"), Here + Forward * Reach, .5f, (From + To) * .5f);
    for (int32 i = 0; i < Count; ++i)
    {
        const float U = float(i) / (Count - 1);
        const FVector At = Here + Forward.RotateAngleAxis(-Arc * .5f + Arc * U, FVector::UpVector) * Reach;
        const float Delay = FMath::Lerp(From, To, U);
        Mark(At, A.Get(TEXT("radius"), 320.f), Delay, Hit(A), false, false, .3f);
        Later(Delay, [this, At]() { Fx(TEXT("NS_Rock_Debris"), Ground(At), 1.f, 1.2f); });
    }
}

void UVortexVolcanicBoss::DoCombo(const FAbility& A)
{
    // Combo Sísmico: direita, esquerda e os dois punhos; cada marca aparece 0,75 s antes do seu soco (dá para ler um
    // de cada vez)
    AVortexMonster* M = Body();
    const FVector Here = M->GetActorLocation(), Forward = M->GetActorForwardVector(), Right = M->GetActorRightVector();
    const float Reach = M->Def()->Radius + A.Get(TEXT("reach"), 480.f);
    const TArray<float> Hits = A.Hits.Num() >= 3 ? A.Hits : TArray<float>{ .78f, 1.68f, 2.95f };
    const float Lead = .75f;
    for (int32 i = 0; i < 3; ++i)
    {
        const FVector At = Here + Forward * Reach + Right * (i == 0 ? 140.f : i == 1 ? -140.f : 0.f);
        const float Radius = A.Get(TEXT("radius"), 380.f) * (i == 2 ? 1.35f : 1.f);
        const float Damage = Hit(A) * (i == 2 ? A.Get(TEXT("finisher"), 1.5f) : 1.f);
        Later(FMath::Max(0.f, Hits[i] - Lead), [this, At, Radius, Damage, Lead, Hits, i]()
        {
            Mark(At, Radius, FMath::Min(Lead, Hits[i]), Damage, false, false, i == 2 ? .5f : .2f);
        });
        Later(Hits[i], [this, At, i]() { Fx(TEXT("NS_Stone_Impact"), Ground(At), i == 2 ? 2.f : 1.2f, 1.2f); });
        Sfx(TEXT("azhur_whoosh"), At, .7f, Hits[i] - .3f);
        Sfx(TEXT("azhur_slam"), At, i == 2 ? 1.f : .7f, Hits[i]);
    }
}

// ---- as habilidades vulcânicas -------------------------------------------------------------------------

void UVortexVolcanicBoss::DoCoreBurst(const FAbility& A)
{
    AVortexMonster* M = Body();
    const FVector Here = M->GetActorLocation();
    const float Impact = A.Get(TEXT("impact"), 1.75f);
    Fx(TEXT("NS_MagmaCore_Charge"), Here, 2.2f, Impact, FRotator::ZeroRotator, true);
    Mark(Here, A.Get(TEXT("radius"), 1000.f), Impact, Hit(A), false, true);
    Later(Impact, [this, Here]() { Fx(TEXT("NS_MagmaCore_Explosion"), Here, 3.f, 1.8f); });
    Sfx(TEXT("azhur_charge"), Here, 1.f, FMath::Max(0.f, Impact - 1.5f));
    Sfx(TEXT("azhur_explode"), Here, 1.f, Impact);
}

void UVortexVolcanicBoss::DoFissure(const FAbility& A, const FVector& From, const FVector& Direction, float Delay, int32 Segments)
{
    // a fenda de lava avança pelo chão, um pedaço de cada vez (a marca de cada um aparece junto com a da primeira)
    AVortexMonster* M = Body();
    const float Step = A.Get(TEXT("step"), 290.f), Travel = A.Get(TEXT("travel"), .11f);
    const float Lava = M->Phase >= 2 ? A.Get(TEXT("lavaLifePhase2"), 6.f) : A.Get(TEXT("lavaLife"), 0.f);
    const float Damage = Hit(A, TEXT("damage"), 1.f);
    for (int32 i = 0; i < Segments; ++i)
    {
        const FVector At = From + Direction * Step * i;
        const float When = Delay + Travel * i;
        Mark(At, A.Get(TEXT("radius"), 230.f), When, Damage, false, true);
        if (i % 3 == 0) Sfx(TEXT("azhur_fissure"), At, .8f, When);
        Later(When, [this, At, Direction, Lava, M]()
        {
            Fx(TEXT("NS_Volcanic_Fissure"), Ground(At), 1.f, 1.8f, Direction.Rotation());
            if (Lava > 0.f && IsValid(M)) M->MulticastLavaPool(Ground(At), 150.f, Lava);
        });
    }
}

void UVortexVolcanicBoss::DoMeteor(const FAbility& A)
{
    AVortexMonster* M = Body();
    AVortexCharacter* Hero = Target();
    const int32 Count = int32(M->Phase >= 2 ? A.Get(TEXT("countPhase2"), 3.f) : A.Get(TEXT("count"), 1.f));
    const float Release = A.Get(TEXT("impact"), 1.52f), Flight = A.Get(TEXT("flight"), .9f);
    const FVector Aim = Hero->GetActorLocation();
    for (int32 i = 0; i < Count; ++i)
    {
        // o primeiro no herói; os outros em volta (nunca um em cima do outro)
        const FVector At = i == 0 ? Aim : Aim + FVector(1.f, 0.f, 0.f).RotateAngleAxis(120.f * i + FMath::FRandRange(-20.f, 20.f), FVector::UpVector) * A.Get(TEXT("spread"), 520.f);
        const FVector Land = Ground(At);
        Mark(Land, A.Get(TEXT("radius"), 380.f), Release + Flight + .1f * i, Hit(A), false, true);
        Sfx(TEXT("azhur_meteor"), M->GetActorLocation(), .9f, Release + .1f * i);
        Sfx(TEXT("azhur_meteor_land"), Land, 1.f, Release + Flight + .1f * i);
        Later(Release + .1f * i, [this, M, Land, Flight]()
        {
            if (!IsValid(M)) return;
            M->MulticastMeteor(Land, Flight);
            Fx(TEXT("NS_Molten_Meteor"), M->GetActorLocation() + FVector(0.f, 0.f, 500.f), 2.f, Flight, (Land - M->GetActorLocation()).Rotation());
        });
        Later(Release + Flight + .1f * i, [this, M, Land, Life = A.Get(TEXT("lavaLife"), 8.f)]()
        {
            Fx(TEXT("NS_Stone_Impact"), Land, 1.5f, 1.2f);
            if (IsValid(M) && Life > 0.f) M->MulticastLavaPool(Land, 230.f, Life);
        });
    }
}

void UVortexVolcanicBoss::DoBeam(const FAbility& A)
{
    // Raio do Núcleo: o núcleo carrega, depois o feixe sai do peito e varre um leque à frente; o servidor fere quem
    // estiver na linha a cada tique (atrás dele é seguro)
    AVortexMonster* M = Body();
    const float Start = A.Get(TEXT("impact"), 1.f), End = A.Get(TEXT("impactEnd"), 3.4f), Tick = A.Get(TEXT("tick"), .2f);
    const float Sweep = A.Get(TEXT("sweep"), 70.f), Length = A.Get(TEXT("length"), 2400.f), Width = A.Get(TEXT("width"), 170.f);
    const float BaseYaw = M->GetActorRotation().Yaw;
    const float Damage = Hit(A, TEXT("damage"), .22f);
    Fx(TEXT("NS_MagmaCore_Charge"), M->GetActorLocation(), 2.f, Start, FRotator::ZeroRotator, true);
    Later(Start, [this, End, Start, Length]() { MulticastBeam(End - Start, Length); });
    Sfx(TEXT("azhur_charge"), M->GetActorLocation(), .8f);
    Sfx(TEXT("azhur_beam"), M->GetActorLocation(), 1.f, Start);
    const int32 Ticks = FMath::Max(1, FMath::RoundToInt((End - Start) / Tick));
    for (int32 i = 0; i <= Ticks; ++i)
    {
        const float U = float(i) / Ticks;
        Later(Start + Tick * i, [this, M, U, BaseYaw, Sweep, Length, Width, Damage]()
        {
            if (!IsValid(M) || M->bDead) return;
            M->SetActorRotation(FRotator(0.f, BaseYaw + FMath::Lerp(-Sweep * .5f, Sweep * .5f, U), 0.f));
            const FVector From = M->GetActorLocation() + M->GetActorForwardVector() * M->Def()->Radius;
            const FVector To = From + M->GetActorForwardVector() * Length;
            for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
            {
                const FVector P = It->GetActorLocation();
                const FVector Closest = FMath::ClosestPointOnSegment(FVector(P.X, P.Y, From.Z), FVector(From.X, From.Y, From.Z), FVector(To.X, To.Y, From.Z));
                if (FVector::Dist2D(Closest, P) <= Width && FMath::Abs(P.Z - M->GetActorLocation().Z) < 500.f && M->Combat->CanHarm(*It))
                    M->Combat->ApplyHit(*It, Damage, EVortexDamageKind::Magic);
            }
        });
    }
}

void UVortexVolcanicBoss::DoEruption(const FAbility& A)
{
    // colunas de lava pela arena: algumas em volta de cada herói, o resto espalhado perto do chefe, em sequência
    AVortexMonster* M = Body();
    const int32 Count = int32(M->Phase >= 2 ? A.Get(TEXT("countPhase2"), 10.f) : A.Get(TEXT("count"), 6.f));
    const float From = A.Get(TEXT("impact"), 1.f), To = A.Get(TEXT("impactEnd"), 2.f), Area = A.Get(TEXT("area"), 1700.f);
    TArray<FVector> Spots;
    const int32 PerHero = int32(A.Get(TEXT("aroundHeroes"), 2.f));
    for (TActorIterator<AVortexCharacter> It(GetWorld()); It && Spots.Num() < Count; ++It)
        if (FVector::Dist2D(It->GetActorLocation(), M->Home) < Stat(TEXT("detectRange"), 4500.f))
            for (int32 k = 0; k < PerHero && Spots.Num() < Count; ++k)
                Spots.Add(It->GetActorLocation() + FVector(FMath::FRandRange(-260.f, 260.f), FMath::FRandRange(-260.f, 260.f), 0.f));
    while (Spots.Num() < Count)
    {
        const FVector2D R = FMath::RandPointInCircle(Area);
        Spots.Add(M->GetActorLocation() + FVector(R.X, R.Y, 0.f));
    }
    for (int32 i = 0; i < Spots.Num(); ++i)
    {
        const float Delay = FMath::Lerp(From, To, Spots.Num() > 1 ? float(i) / (Spots.Num() - 1) : 0.f);
        const FVector At = Spots[i];
        Mark(At, A.Get(TEXT("radius"), 260.f), Delay, Hit(A), false, true);
        Later(Delay, [this, At]() { Fx(TEXT("NS_Lava_Pillar"), Ground(At), 1.2f, 1.6f); });
        Sfx(TEXT("azhur_eruption"), At, .8f, Delay);
    }
}

void UVortexVolcanicBoss::DoShed(const FAbility& A)
{
    // Armadura Derretida: a casca explode em volta e os fragmentos em brasa voam para longe
    AVortexMonster* M = Body();
    const FVector Here = M->GetActorLocation();
    const float Impact = A.Get(TEXT("impact"), 1.38f);
    Mark(Here, A.Get(TEXT("radius"), 700.f), Impact, Hit(A), false, false, .5f);
    const int32 Count = int32(A.Get(TEXT("fragments"), 8.f));
    const float Reach = A.Get(TEXT("fragmentReach"), 1400.f), Radius = A.Get(TEXT("fragmentRadius"), 200.f);
    const float Damage = Hit(A, TEXT("fragmentDamage"), .6f);
    Sfx(TEXT("azhur_shed"), Here, 1.f, Impact);
    Later(Impact, [this, M, Here, Count, Reach, Radius, Damage]()
    {
        if (!IsValid(M)) return;
        Fx(TEXT("NS_Rock_Debris"), Ground(Here), 2.4f, 2.f);
        const FVector From = Here + FVector(0.f, 0.f, M->Def()->Height * .2f);
        for (int32 i = 0; i < Count; ++i)
        {
            const float Angle = 360.f * i / Count + FMath::FRandRange(-15.f, 15.f);
            const FVector Land = Ground(Here + FVector(1.f, 0.f, 0.f).RotateAngleAxis(Angle, FVector::UpVector) * FMath::FRandRange(Reach * .45f, Reach));
            AVortexLobbed::Throw(M, From, Land, .95f, Damage, Radius, true);
        }
    });
}

void UVortexVolcanicBoss::DoUltimate(const FAbility& A)
{
    // Despertar do Núcleo: ajoelha e carrega (o chão avisa com um anel enorme: o miolo, perto dele, é seguro), levanta
    // num rugido e bate; saem as fendas em estrela e, na fase 2, chovem pedras de lava
    AVortexMonster* M = Body();
    const FVector Here = M->GetActorLocation();
    const float Impact = A.Get(TEXT("impact"), 4.45f);
    Fx(TEXT("NS_MagmaCore_Charge"), Here, 3.f, Impact, FRotator::ZeroRotator, true);
    Later(FMath::Max(0.f, Impact - 2.2f), [this, Here, Impact, Radius = A.Get(TEXT("radius"), 1800.f), Damage = Hit(A)]()
    {
        Mark(Here, Radius, 2.2f, Damage, true, true, .6f);
    });
    Later(Impact, [this, Here]() { Fx(TEXT("NS_MagmaCore_Explosion"), Here, 5.f, 2.2f); });
    Sfx(TEXT("azhur_charge"), Here, 1.f, .6f);
    Sfx(TEXT("azhur_charge"), Here, 1.f, 2.4f);
    Sfx(TEXT("azhur_roar"), Here, 1.f, 3.4f);
    Sfx(TEXT("azhur_explode"), Here, 1.f, Impact);
    const int32 Lines = int32(M->Phase >= 2 ? A.Get(TEXT("fissuresPhase2"), 10.f) : A.Get(TEXT("fissures"), 6.f));
    const int32 Fissure = Abilities.IndexOfByPredicate([](const FAbility& F) { return F.Id == TEXT("fissure"); });
    if (Abilities.IsValidIndex(Fissure))
    {
        const FAbility& F = Abilities[Fissure];
        for (int32 i = 0; i < Lines; ++i)
        {
            const FVector Dir = M->GetActorForwardVector().RotateAngleAxis(360.f * i / Lines, FVector::UpVector);
            DoFissure(F, Here + Dir * (M->Def()->Radius + 200.f), Dir, Impact + .15f, 6);
        }
    }
    if (M->Phase >= 2)
    {
        const int32 Count = int32(A.Get(TEXT("meteorsPhase2"), 6.f));
        for (int32 i = 0; i < Count; ++i)
        {
            const FVector2D R = FMath::RandPointInCircle(1600.f);
            const FVector Land = Ground(Here + FVector(R.X, R.Y, 0.f));
            Mark(Land, 300.f, Impact + .9f + .15f * i, Hit(A, TEXT("damage"), 2.2f) * .45f, false, true);
            Later(Impact + .15f * i, [this, M, Land]() { if (IsValid(M)) M->MulticastMeteor(Land, .9f); });
        }
    }
}

// ---- efeitos, feixe e calor (todas as máquinas) ---------------------------------------------------------

void UVortexVolcanicBoss::MulticastFx_Implementation(FName System, FVector_NetQuantize At, FRotator Rotation, float Scale, float Life, bool bFollow)
{
    if (GetNetMode() == NM_DedicatedServer) return;
    UNiagaraSystem* Asset = LoadObject<UNiagaraSystem>(nullptr, *FString::Printf(TEXT("%s%s.%s"), FxFolder, *System.ToString(), *System.ToString()));
    if (!Asset) return;
    UNiagaraComponent* C = bFollow && GetOwner()
        ? UNiagaraFunctionLibrary::SpawnSystemAttached(Asset, GetOwner()->GetRootComponent(), NAME_None, FVector(0.f, 0.f, 120.f), Rotation, EAttachLocation::KeepRelativeOffset, true)
        : UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Asset, At, Rotation, FVector(Scale), true);
    if (!C) return;
    C->SetWorldScale3D(FVector(Scale));
    C->SetVariableLinearColor(TEXT("User.Color"), Magma);
    if (Life > 0.f)
    {
        FTimerHandle H;
        TWeakObjectPtr<UNiagaraComponent> Weak(C);
        GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([Weak]() { if (Weak.IsValid()) Weak->Deactivate(); }), Life, false);
    }
}

void UVortexVolcanicBoss::Sfx(FName Sound, const FVector& At, float Volume, float Delay)
{
    if (Delay <= .01f) { MulticastSfx(Sound, At, Volume); return; }
    Later(Delay, [this, Sound, At, Volume]() { MulticastSfx(Sound, At, Volume); });
}

void UVortexVolcanicBoss::MulticastSfx_Implementation(FName Sound, FVector_NetQuantize At, float Volume)
{
    if (GetNetMode() != NM_DedicatedServer) VortexAudio::Play(GetOwner(), Sound, At, Volume, .05f);
}

void UVortexVolcanicBoss::MulticastBeam_Implementation(float Seconds, float Length)
{
    if (GetNetMode() == NM_DedicatedServer || !GetOwner()) return;
    BeamUntil = GetWorld()->GetTimeSeconds() + Seconds;
    BeamLength = Length;
    if (!BeamMesh)
    {
        BeamMesh = NewObject<UStaticMeshComponent>(GetOwner());
        BeamMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
        BeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BeamMesh->SetCastShadow(false);
        BeamMesh->SetUsingAbsoluteLocation(true);
        BeamMesh->SetUsingAbsoluteRotation(true);
        BeamMesh->SetUsingAbsoluteScale(true);
        BeamMesh->SetupAttachment(GetOwner()->GetRootComponent());
        BeamMesh->RegisterComponent();
        if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/World/Portal/M_PortalGlow.M_PortalGlow")))
        {
            BeamMaterial = UMaterialInstanceDynamic::Create(Glow, this);
            BeamMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, .32f, .04f));
            BeamMaterial->SetScalarParameterValue(TEXT("Intensity"), 4.f);   // 14 estourava em branco
            BeamMesh->SetMaterial(0, BeamMaterial);
        }
    }
    if (!BeamFire)
        if (UNiagaraSystem* Fire = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Vortex/Bosses/Azhur/FX/NS_Core_Beam.NS_Core_Beam")))
            BeamFire = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Fire, GetOwner()->GetActorLocation(), FRotator::ZeroRotator, FVector(1.4f), false);
    BeamMesh->SetVisibility(true);
    if (BeamFire) BeamFire->Activate(true);
}

void UVortexVolcanicBoss::UpdateBeam()
{
    if (!BeamMesh) return;
    const bool bOn = GetWorld()->GetTimeSeconds() < BeamUntil;
    if (!bOn)
    {
        if (BeamMesh->IsVisible()) BeamMesh->SetVisibility(false);
        if (BeamFire && BeamFire->IsActive()) BeamFire->Deactivate();
        return;
    }
    const AVortexMonster* M = Body();
    if (!M || !M->Def()) return;
    // do núcleo (no peito) até o chão à frente: o cilindro da engine tem 100 cm de altura, deitado na direção do feixe
    const FVector Core = M->GetActorLocation() + FVector(0.f, 0.f, M->Def()->Height * .15f) + M->GetActorForwardVector() * M->Def()->Radius * .6f;
    FVector End = M->GetActorLocation() + M->GetActorForwardVector() * BeamLength;
    End = Ground(End) + FVector(0.f, 0.f, 40.f);
    const FVector Dir = End - Core;
    const float Flicker = 1.f + .15f * FMath::Sin(GetWorld()->GetTimeSeconds() * 40.f);
    BeamMesh->SetWorldLocation((Core + End) * .5f);
    BeamMesh->SetWorldRotation(FRotationMatrix::MakeFromZ(Dir.GetSafeNormal()).Rotator());
    BeamMesh->SetWorldScale3D(FVector(.55f * Flicker, .55f * Flicker, Dir.Size() / 100.f));
    if (BeamFire) BeamFire->SetWorldLocation(End);
}

void UVortexVolcanicBoss::UpdateHeat(float DeltaTime)
{
    AVortexMonster* M = Body();
    if (!M) return;
    // o núcleo esquenta na fase 2 e apaga na morte (o material M_AzhurMeshy: CoreHeat)
    if (Heat.IsEmpty() && M->GetMesh())
        for (int32 i = 0; i < M->GetMesh()->GetNumMaterials(); ++i)
            if (UMaterialInstanceDynamic* D = M->GetMesh()->CreateAndSetMaterialInstanceDynamic(i)) Heat.Add(D);
    const float Want = M->bDead ? .1f : M->Phase >= 2 ? PhaseStat(TEXT("coreGlow"), 2.6f) : 1.f;
    const float Pulse = M->Phase >= 2 && !M->bDead ? 1.f + .18f * FMath::Sin(GetWorld()->GetTimeSeconds() * 4.f) : 1.f;
    HeatNow = FMath::FInterpTo(HeatNow, Want, DeltaTime, M->bDead ? .6f : 1.5f);
    for (const TWeakObjectPtr<UMaterialInstanceDynamic>& D : Heat)
        if (D.IsValid()) D->SetScalarParameterValue(TEXT("CoreHeat"), HeatNow * Pulse);
}

void UVortexVolcanicBoss::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (GetNetMode() == NM_DedicatedServer) return;
    UpdateBeam();
    UpdateHeat(DeltaTime);
}
