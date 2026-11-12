#include "Combat/VortexClassSkills.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimSequence.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexHitFx.h"
#include "Combat/VortexProjectile.h"
#include "Combat/VortexTelegraph.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Dom/JsonObject.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Net/UnrealNetwork.h"
#include "Combat/VortexPowerFx.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/PackageName.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Rpg/VortexHeroSheet.h"
#include "TimerManager.h"
#include "World/VortexCaldera.h"
#include "World/VortexTerrainStreamer.h"

namespace
{
    const TCHAR* GlowMaterial = TEXT("/Game/Vortex/Combat/M_SpectralGlow.M_SpectralGlow");

    /** A onda de choque colorida do Vefects mais perto da cor do elemento. */
    FString ShockwaveFor(FName Element, bool bVertical = false)
    {
        const FString E = Element.ToString();
        const TCHAR* Color = TEXT("White");
        if (E == TEXT("fire") || E == TEXT("earth")) Color = TEXT("Orange");
        else if (E == TEXT("blood")) Color = TEXT("Red");
        else if (E == TEXT("holy") || E == TEXT("star") || E == TEXT("lightning")) Color = TEXT("Yellow");
        else if (E == TEXT("nature") || E == TEXT("jade") || E == TEXT("poison")) Color = TEXT("Green");
        else if (E == TEXT("arcane") || E == TEXT("shadow")) Color = TEXT("Purple");
        else if (E == TEXT("moon") || E == TEXT("time")) Color = TEXT("Lavender");
        else if (E == TEXT("ice") || E == TEXT("water") || E == TEXT("spirit")) Color = TEXT("Teal");
        const FString Kind = bVertical ? TEXT("Vertical_01") : TEXT("01");
        if (FCString::Strcmp(Color, TEXT("White")) == 0)
            return FString::Printf(TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/%sVFX_Shockwave_%s_White_1s.VFX_Shockwave_%s_White_1s"),
                bVertical ? TEXT("Vertical/") : TEXT(""), *Kind, *Kind);
        return FString::Printf(TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_%s_%s_1s.VFX_Shockwave_%s_%s_1s"),
            *Kind, Color, *Kind, Color);
    }

    /** Aura/bênção do Free_Spells mais perto do elemento. */
    FString SpellFor(FName Element, bool bAura)
    {
        const FString E = Element.ToString();
        const TCHAR* Name = TEXT("Mana");
        if (E == TEXT("fire") || E == TEXT("blood")) Name = TEXT("Fire");
        else if (E == TEXT("lightning") || E == TEXT("star") || E == TEXT("time")) Name = TEXT("Lightning");
        else if (E == TEXT("holy") || E == TEXT("nature") || E == TEXT("jade")) Name = TEXT("Healing");
        else if (E == TEXT("shadow") || E == TEXT("bone") || E == TEXT("poison")) Name = TEXT("Soul");
        else if (E == TEXT("wind") || E == TEXT("steel") || E == TEXT("earth")) Name = TEXT("Air");
        const TCHAR* Kind = bAura ? TEXT("Aura") : TEXT("Buff");
        return FString::Printf(TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_%s_%s.NS_Free_Spells_%s_%s"), Kind, Name, Kind, Name);
    }

    void Spawn(UWorld* World, const FString& Path, const FVector& At, const FRotator& Rotation, float Scale)
    {
        if (UNiagaraSystem* System = VortexHitFx::System(*Path)) UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, At, Rotation, FVector(Scale));
    }

    float GroundAt(const FVector& At)
    {
        if (const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData())
        {
            const float Z = Terrain->GroundAt(At.X, At.Y);
            if (Z > -299.f) return Z;
        }
        return At.Z - 90.f;
    }

    float RadiusOf(const AActor* Actor)
    {
        const UCapsuleComponent* Capsule = Actor ? Cast<UCapsuleComponent>(Actor->GetRootComponent()) : nullptr;
        return Capsule ? Capsule->GetScaledCapsuleRadius() : 40.f;
    }
}

UVortexClassSkills::UVortexClassSkills()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    ReadyAt.Init(0.f, 4);
}

void UVortexClassSkills::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UVortexClassSkills, ReadyAt, COND_OwnerOnly);
    DOREPLIFETIME(UVortexClassSkills, Buffs);
    DOREPLIFETIME(UVortexClassSkills, CastSlot);
    DOREPLIFETIME(UVortexClassSkills, CastServerTime);
}

AVortexCharacter* UVortexClassSkills::Hero() const { return Cast<AVortexCharacter>(GetOwner()); }

double UVortexClassSkills::ServerNow() const
{
    const UWorld* World = GetWorld();
    const AGameStateBase* State = World ? World->GetGameState() : nullptr;
    return State ? State->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
}

const FVortexClassSkill* UVortexClassSkills::Skill(int32 Slot) const
{
    const AVortexCharacter* Owner = Hero();
    const UVortexHeroSheet* Sheet = Owner ? Owner->GetSheet() : nullptr;
    if (!Sheet || Sheet->GetClassId().IsNone()) return nullptr;
    const TArray<FVortexClassSkill>* List = FVortexRpgCatalog::Get().SkillsOf(Sheet->GetClassId());
    return List && List->IsValidIndex(Slot) ? &(*List)[Slot] : nullptr;
}

float UVortexClassSkills::CooldownTotal(int32 Slot) const
{
    const FVortexClassSkill* S = Skill(Slot);
    const AVortexCharacter* Owner = Hero();
    const float Scale = Owner && Owner->GetSheet() ? Owner->GetSheet()->ComputeStats().CooldownScale : 1.f;
    return S ? FMath::Max(.5f, S->Cooldown * Scale / (1.f + HasteBonus())) : 1.f;
}

float UVortexClassSkills::CooldownLeft(int32 Slot) const
{
    return ReadyAt.IsValidIndex(Slot) ? FMath::Max(0.f, float(ReadyAt[Slot] - ServerNow())) : 0.f;
}

bool UVortexClassSkills::IsUnlocked(int32 Slot) const
{
    const AVortexCharacter* Owner = Hero();
    return Owner && Owner->GetSheet() && Owner->GetSheet()->GetLevel() >= UnlockLevel(Slot);
}

bool UVortexClassSkills::CanCast(int32 Slot) const
{
    if (!IsUnlocked(Slot)) return false;
    const FVortexClassSkill* S = Skill(Slot);
    const AVortexCharacter* Owner = Hero();
    const UAbilitySystemComponent* ASC = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
    return S && ASC && CooldownLeft(Slot) <= 0.f && ASC->GetNumericAttribute(UVortexAttributes::GetManaAttribute()) >= S->Mana &&
        ASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) > 0.f;
}

void UVortexClassSkills::Press(int32 Slot)
{
    AVortexCharacter* Owner = Hero();
    if (!Owner || !Owner->IsLocallyControlled()) return;
    const FVortexClassSkill* S = Skill(Slot);
    if (!S)
    {
        if (UVortexHeroSheet* Sheet = Owner->GetSheet()) Sheet->PushNotice(Sheet->CanAwaken() ? TEXT("Desperte uma classe para usar as habilidades (I)") : TEXT("As habilidades de classe chegam no nível 10"));
        return;
    }
    if (!IsUnlocked(Slot))
    {
        if (UVortexHeroSheet* Sheet = Owner->GetSheet()) Sheet->PushNotice(FString::Printf(TEXT("%s: libera no nível %d"), *S->Name, UnlockLevel(Slot)));
        return;
    }
    if (!CanCast(Slot))
    {
        if (UVortexHeroSheet* Sheet = Owner->GetSheet())
            Sheet->PushNotice(CooldownLeft(Slot) > 0.f ? FString::Printf(TEXT("%s: recarregando"), *S->Name) : FString::Printf(TEXT("%s: mana insuficiente"), *S->Name));
        return;
    }
    // mira: o ponto sob a cruz da tela (o herói vira para lá)
    FHitResult AimHit;
    const float Reach = FMath::Max(S->Reach(), 6.f) * 100.f;
    const bool bAimed = Owner->TraceAim(AimHit, Reach + 1500.f);
    const FVector Aim = bAimed ? AimHit.ImpactPoint : AimHit.TraceEnd;
    const FVector Flat = (Aim - Owner->GetActorLocation()).GetSafeNormal2D();
    if (!Flat.IsNearlyZero()) Owner->SetActorRotation(Flat.Rotation());
    // investida: o próprio herói sai já aqui (o servidor faz o mesmo), sem esperar a ida e volta
    if (S->Kind == TEXT("dash") && !Owner->HasAuthority())
    {
        const FVector Dir = (S->Range < 0.f ? -1.f : 1.f) * (Flat.IsNearlyZero() ? Owner->GetActorForwardVector() : Flat);
        Owner->LaunchCharacter(Dir * FMath::Abs(S->Range) * 100.f / .28f + FVector(0.f, 0.f, 120.f), true, true);
    }
    ServerCast(uint8(Slot), Aim);
}

float UVortexClassSkills::ScaleOf(const FVortexClassSkill& S) const
{
    const FVortexClassDef* Owner = FVortexRpgCatalog::Get().Class(S.Owner);
    const float* Rarity = Owner ? FVortexRpgCatalog::Get().RarityPower.Find(Owner->Rarity) : nullptr;
    return Rarity ? *Rarity : 1.f;
}

float UVortexClassSkills::PowerOf(const FVortexClassSkill& S) const
{
    // o poder do Godot cresce com o nível (metade no 1, cheio no 10, o dobro perto do 30) e a raridade
    // da classe; o atributo (Força/Poder de magia) entra depois, no dano do golpe (OutgoingDamage)
    const AVortexCharacter* Owner = Hero();
    const int32 Level = Owner && Owner->GetSheet() ? Owner->GetSheet()->GetLevel() : 10;
    return S.Power * (.45f + .055f * FMath::Max(1, Level)) * ScaleOf(S);
}

void UVortexClassSkills::ServerCast_Implementation(uint8 Slot, FVector_NetQuantize Aim)
{
    AVortexCharacter* Owner = Hero();
    const FVortexClassSkill* S = Skill(Slot);
    UAbilitySystemComponent* ASC = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
    if (!S || !ASC || !CanCast(Slot)) return;
    ASC->ApplyModToAttribute(UVortexAttributes::GetManaAttribute(), EGameplayModOp::Additive, -float(S->Mana));
    ReadyAt[Slot] = float(ServerNow() + CooldownTotal(Slot));
    CastSlot = Slot;
    CastServerTime = float(ServerNow());
    const FVector Flat = (FVector(Aim) - Owner->GetActorLocation()).GetSafeNormal2D();
    if (!Flat.IsNearlyZero()) Owner->SetActorRotation(Flat.Rotation());
    Owner->ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("VORTEX_CLASS_SKILL %s %s (%s)"), *GetNameSafe(Owner), *S->Id.ToString(), *S->Kind.ToString());
    // 07/11: o poder sai no instante do golpe da animação dele (Data/Rpg/power_fx.json "hit", da Tools/Blender/class_anims_hand.py)
    const float Hit = AVortexPowerFx::HitTime(FName(*(TEXT("classe_") + S->Id.ToString())));
    if (Hit <= .01f) { Execute(Slot, *S, Aim); return; }
    const FVortexClassSkill Copy = *S;
    const FVector AimAt = Aim;
    Later(Hit, [this, Slot, Copy, AimAt]() { if (Hero()) Execute(Slot, Copy, AimAt); });
}

// ---- execução no servidor -------------------------------------------------------------------------

TArray<AActor*> UVortexClassSkills::Enemies(const FVector& Center, float RadiusCm) const
{
    TArray<AActor*> Out;
    const AVortexCharacter* Owner = Hero();
    const UVortexCombatComponent* Combat = Owner ? Owner->GetCombat() : nullptr;
    if (!Combat) return Out;
    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexClassSkill), false, Owner);
    GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, Objects, FCollisionShape::MakeSphere(RadiusCm + 300.f), Query);
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Target = Overlap.GetActor();
        if (!Target || Out.Contains(Target) || !Combat->CanHarm(Target)) continue;
        if (FVector::Dist2D(Target->GetActorLocation(), Center) > RadiusCm + RadiusOf(Target)) continue;
        if (FMath::Abs(Target->GetActorLocation().Z - Center.Z) > 320.f) continue;
        Out.Add(Target);
    }
    return Out;
}

void UVortexClassSkills::Hit(const FVortexClassSkill& S, AActor* Target, float Power)
{
    AVortexCharacter* Owner = Hero();
    UVortexCombatComponent* Combat = Owner ? Owner->GetCombat() : nullptr;
    if (!Combat || !Target) return;
    // estados do Compêndio: lentidão/prisão, enfraquecer, expor, dano contínuo, empurrão
    if (UVortexCombatComponent* Other = Target->FindComponentByClass<UVortexCombatComponent>())
    {
        if (S.SlowTime > 0.f) Other->ApplySlow(S.SlowFactor, S.SlowTime);
        if (S.WeakenTime > 0.f) Other->ApplyWeaken(S.WeakenFactor, S.WeakenTime);
        if (S.ExposeTime > 0.f) Other->ApplyExpose(S.ExposeFactor, S.ExposeTime);
        if (S.DotTime > 0.f) Other->ApplyDot(S.DotDps * ScaleOf(S), S.DotTime, Owner);
        if (S.Taunt > 0.f) Other->LastAttacker = Owner;
        if (S.Knock > 0.f && !Other->bStunImmune)
        {
            if (ACharacter* Body = Cast<ACharacter>(Target))
                Body->LaunchCharacter((Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D() * S.Knock * 260.f + FVector(0, 0, 220.f), true, true);
        }
    }
    if (Power <= 0.f) return;
    float Mult = 1.f;
    if (S.Execute > 0.f)
    {
        // execução: até +Execute de dano conforme a vida que falta
        const IAbilitySystemInterface* Abilities = Cast<IAbilitySystemInterface>(Target);
        const UAbilitySystemComponent* TargetASC = Abilities ? Abilities->GetAbilitySystemComponent() : nullptr;
        if (TargetASC)
        {
            const float Max = FMath::Max(1.f, TargetASC->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute()));
            Mult *= 1.f + S.Execute * (1.f - TargetASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) / Max);
        }
    }
    if (S.Backstab > 0.f && FVector::DotProduct(Target->GetActorForwardVector().GetSafeNormal2D(), (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D()) > .3f)
        Mult *= 1.f + S.Backstab;
    const bool bHit = Combat->ApplyHit(Target, FMath::Max(1.f, FMath::RoundToFloat(Power * Mult)), S.IsPhysical() ? EVortexDamageKind::Melee : EVortexDamageKind::Magic, S.Stun);
    if (bHit && S.Lifesteal > 0.f)
    {
        if (UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent())
        {
            const float Health = ASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
            ASC->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), FMath::Min(ASC->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute()), Health + Power * S.Lifesteal));
        }
    }
}

void UVortexClassSkills::Area(const FVortexClassSkill& S, const FVector& Center, float RadiusCm, float Power, TSet<TWeakObjectPtr<AActor>>* Skip)
{
    for (AActor* Target : Enemies(Center, RadiusCm))
    {
        if (Skip && Skip->Contains(Target)) continue;
        if (Skip) Skip->Add(Target);
        Hit(S, Target, Power);
    }
}

void UVortexClassSkills::Later(float Seconds, TFunction<void()> Run)
{
    if (Seconds <= 0.f) { Run(); return; }
    FTimerHandle Handle;
    GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, MoveTemp(Run)), Seconds, false);
}

void UVortexClassSkills::Execute(int32 Slot, const FVortexClassSkill& S, const FVector& AimIn)
{
    AVortexCharacter* Owner = Hero();
    const FVector Origin = Owner->GetActorLocation();
    FVector Forward = (AimIn - Origin).GetSafeNormal2D();
    if (Forward.IsNearlyZero()) Forward = Owner->GetActorForwardVector().GetSafeNormal2D();
    const float Reach = S.Reach() * 100.f;
    // ponto mirado no chão, dentro do alcance
    FVector Aim = AimIn;
    if (FVector::Dist2D(Aim, Origin) > Reach) Aim = Origin + Forward * Reach;
    Aim.Z = GroundAt(Aim);
    const float Power = PowerOf(S);
    const FString Kind = S.Kind.ToString();
    TArray<FVector_NetQuantize> Points;
    FVector Target = Aim;

    if (Kind == TEXT("projectile") || Kind == TEXT("wave"))
    {
        const bool bWave = Kind == TEXT("wave");
        const int32 N = FMath::Max(1, S.Count);
        for (int32 k = 0; k < N; ++k)
        {
            const FVector Dir = ((AimIn - (Origin + FVector(0, 0, 40.f))).GetSafeNormal()).RotateAngleAxis((k - (N - 1) * .5f) * FMath::Max(S.Spread, N > 1 ? 9.f : 0.f), FVector::UpVector);
            const FVector From = Origin + FVector(0.f, 0.f, 40.f) + Forward * 70.f;
            const FTransform Where((S.bLob ? Forward : Dir).Rotation(), From);
            AVortexProjectile* Shot = GetWorld()->SpawnActorDeferred<AVortexProjectile>(AVortexProjectile::StaticClass(), Where, Owner, Owner,
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if (!Shot) continue;
            Shot->Mode = S.Explode > 0.f || bWave ? TEXT("orb") : TEXT("bolt");
            Shot->Element = S.Element;
            Shot->Damage = Power;
            Shot->Speed = FMath::Max(S.Speed, bWave ? 11.f : 22.f) * 100.f;
            Shot->Range = Reach;
            Shot->Radius = S.Explode > 0.f ? S.Explode * 100.f : bWave ? S.Radius * 100.f : 0.f;
            Shot->Size = bWave ? 1.6f + S.Radius : 1.2f + .3f * S.Explode;
            Shot->bPierce = S.bPierce || bWave;
            Shot->Stun = S.Stun;
            Shot->FinishSpawning(Where);
        }
    }
    else if (Kind == TEXT("cleave"))
    {
        // golpes seguidos (pulsos): cada um acerta quem está no cone à frente
        for (int32 p = 0; p < FMath::Max(1, S.Pulses); ++p)
        {
            Later(p * .16f, [this, S, Power]()
            {
                AVortexCharacter* H = Hero();
                if (!H) return;
                const FVector At = H->GetActorLocation();
                const FVector F = H->GetActorForwardVector().GetSafeNormal2D();
                for (AActor* T : Enemies(At, S.Radius * 100.f))
                {
                    const FVector To = (T->GetActorLocation() - At).GetSafeNormal2D();
                    if (FVector::DotProduct(F, To) >= .35f || FVector::Dist2D(T->GetActorLocation(), At) < RadiusOf(T) + 80.f) Hit(S, T, Power);
                }
            });
        }
        Target = Origin + Forward * S.Radius * 60.f;
    }
    else if (Kind == TEXT("nova"))
    {
        for (int32 p = 0; p < FMath::Max(1, S.Pulses); ++p)
        {
            Later(p * .32f, [this, S, Power]() { if (AVortexCharacter* H = Hero()) Area(S, H->GetActorLocation(), S.Radius * 100.f, Power); });
        }
        Target = Origin;
    }
    else if (Kind == TEXT("strike"))
    {
        // golpe do céu: marcas no chão que explodem depois do atraso (a primeira no ponto mirado)
        const int32 N = FMath::Max(1, S.Count);
        // as marcas caem dentro da área escrita (o raio), não além dela
        const float Spread = N == 1 ? 0.f : FMath::Max(150.f, S.Radius * 100.f - 150.f);
        FRandomStream Rng(int32(GetWorld()->GetTimeSeconds() * 1000.f));
        for (int32 k = 0; k < N; ++k)
        {
            const float A = Rng.FRandRange(0.f, 2.f * UE_PI), R = FMath::Sqrt(Rng.FRand()) * Spread;
            FVector P = k == 0 ? Aim : Aim + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f);
            P.Z = GroundAt(P);
            Points.Add(P);
            AVortexTelegraph::Spawn(Owner, P, S.Radius * 100.f, S.Delay + k * .07f, Power, false, ElementColor(S.Element), !S.IsPhysical(), S.Stun);
            if (S.SlowTime > 0.f || S.WeakenTime > 0.f || S.ExposeTime > 0.f || S.DotTime > 0.f || S.Knock > 0.f)
                Later(S.Delay + k * .07f, [this, S, P]() { Area(S, P, S.Radius * 100.f, 0.f); });   // os estados (o dano vem da marca)
        }
    }
    else if (Kind == TEXT("zone"))
    {
        FZone Z;
        Z.Skill = S;
        // com alcance, a zona cai no ponto mirado (até o alcance); sem, em volta do herói
        Z.Center = S.Range > 0.f ? Aim : Origin;
        Z.Center.Z = GroundAt(Z.Center);
        Z.Until = GetWorld()->GetTimeSeconds() + FMath::Max(1.f, S.Duration);
        Z.NextTick = GetWorld()->GetTimeSeconds() + .15;
        Z.Power = Power;
        Z.Scale = ScaleOf(S);
        Zones.Add(Z);
        Target = Z.Center;
    }
    else if (Kind == TEXT("dash"))
    {
        const FVector Start = Origin;
        const FVector Dir = S.Range < 0.f ? -Forward : Forward;
        Owner->LaunchCharacter(Dir * FMath::Abs(S.Range) * 100.f / .28f + FVector(0.f, 0.f, 120.f), true, true);
        Target = Start + Dir * FMath::Abs(S.Range) * 100.f;
        Later(.3f, [this, S, Power, Start]()
        {
            AVortexCharacter* H = Hero();
            if (!H) return;
            const FVector End = H->GetActorLocation();
            if (S.Range < 0.f && Power > 0.f) Area(S, Start, S.Radius * 100.f, Power);   // salto de retirada: explode onde estava
            else if (Power > 0.f)
            {
                for (AActor* T : Enemies((Start + End) * .5f, FVector::Dist2D(Start, End) * .5f + S.Radius * 100.f))
                    if (FMath::PointDistToSegment(T->GetActorLocation(), Start, End) < S.Radius * 100.f + RadiusOf(T)) Hit(S, T, Power);
            }
            if (S.Land > 0.f) Area(S, End, S.Land * 100.f, Power);
        });
    }
    else if (Kind == TEXT("buff") || Kind == TEXT("heal") || Kind == TEXT("shield"))
    {
        // a bênção/cura/escudo com raio vale para o herói e os companheiros perto (metade da cura para eles);
        // sem raio, só para o herói
        const float Scale = ScaleOf(S);
        const float Radius = S.Radius * 100.f;
        for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
        {
            const bool bSelf = *It == Owner;
            if (!bSelf && (Radius <= 0.f || FVector::Dist(It->GetActorLocation(), Origin) > Radius)) continue;
            UAbilitySystemComponent* ASC = It->GetAbilitySystemComponent();
            if (S.Heal > 0.f && ASC)
            {
                const float Bonus = 1.f + (Owner->GetSheet() ? Owner->GetSheet()->ComputeStats().SkillPower * .01f : 0.f);
                const float Amount = S.Heal * Scale * Bonus * (bSelf ? 1.f : .5f);
                ASC->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(),
                    FMath::Min(ASC->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute()), ASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) + Amount));
            }
            if (UVortexClassSkills* Other = It->FindComponentByClass<UVortexClassSkills>()) Other->GiveBuff(S, bSelf ? Scale : Scale * .75f);
            if (!bSelf) Points.Add(It->GetActorLocation());
        }
        Target = Origin;
    }
    else if (Kind == TEXT("chain"))
    {
        // o inimigo mais perto à frente e, dele, o mais perto ainda não atingido (até 5,5 m)
        TArray<AActor*> Chosen;
        FVector From = Origin;
        for (int32 k = 0; k < FMath::Max(1, S.Count); ++k)
        {
            AActor* Best = nullptr;
            float BestDistance = k == 0 ? Reach : 550.f;
            for (AActor* T : Enemies(From, BestDistance))
            {
                if (Chosen.Contains(T)) continue;
                const FVector To = T->GetActorLocation() - From;
                const float D = To.Size2D();
                if (k == 0 && D > 50.f && FVector::DotProduct(Forward, To.GetSafeNormal2D()) < .2f) continue;
                if (D < BestDistance) { Best = T; BestDistance = D; }
            }
            if (!Best) break;
            Chosen.Add(Best);
            From = Best->GetActorLocation();
        }
        for (int32 k = 0; k < Chosen.Num(); ++k)
        {
            Hit(S, Chosen[k], Power * FMath::Pow(.9f, float(k)));
            Points.Add(Chosen[k]->GetActorLocation());
        }
        if (Chosen.IsEmpty()) Points.Add(Aim);
    }
    else if (Kind == TEXT("beam"))
    {
        Target = Origin + Forward * Reach;
        for (int32 p = 0; p < FMath::Max(1, S.Pulses); ++p)
        {
            Later(p * .25f, [this, S, Power, Reach]()
            {
                AVortexCharacter* H = Hero();
                if (!H) return;
                const FVector From = H->GetActorLocation();
                const FVector To = From + H->GetActorForwardVector().GetSafeNormal2D() * Reach;
                for (AActor* T : Enemies((From + To) * .5f, Reach * .5f + S.Radius * 100.f))
                    if (FMath::PointDistToSegment(T->GetActorLocation(), From, To) < S.Radius * 120.f + RadiusOf(T)) Hit(S, T, Power);
            });
        }
    }
    else if (Kind == TEXT("summon"))
    {
        // espíritos em volta do herói: a cada 1 s cada um morde um inimigo perto
        Summons.RemoveAll([&S](const FSummon& M) { return M.Skill.Id == S.Id; });
        FSummon M;
        M.Skill = S;
        M.Until = GetWorld()->GetTimeSeconds() + FMath::Max(2.f, S.Duration);
        M.NextTick = GetWorld()->GetTimeSeconds() + .6;
        M.Power = Power;
        M.Scale = ScaleOf(S);
        Summons.Add(M);
    }
    Extras(S);
    VortexAudio::Play(this, FName(*FString::Printf(TEXT("class_%s_cast"), *S.Element.ToString())), Origin, .8f);
    MulticastFx(uint8(Slot), Origin, Target, Points);
}

void UVortexClassSkills::Extras(const FVortexClassSkill& S)
{
    AVortexCharacter* Owner = Hero();
    UAbilitySystemComponent* ASC = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
    if (!ASC) return;
    const double Now = ServerNow();
    if (S.ManaGain > 0.f)
        ASC->SetNumericAttributeBase(UVortexAttributes::GetManaAttribute(),
            FMath::Min(ASC->GetNumericAttribute(UVortexAttributes::GetMaxManaAttribute()), ASC->GetNumericAttribute(UVortexAttributes::GetManaAttribute()) + S.ManaGain));
    if (S.Deathward > 0.f) Buffs.DeathwardUntil = float(Now + S.Deathward);
    if (S.Haste > 0.f) { Buffs.Haste = S.Haste; Buffs.HasteUntil = float(Now + (S.Duration > 0.f ? S.Duration : 8.f)); }
    Owner->ForceNetUpdate();
}

void UVortexClassSkills::GiveBuff(const FVortexClassSkill& S, float Scale)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    const double Now = ServerNow();
    const float For = S.BuffDuration > 0.f ? S.BuffDuration : (S.Duration > 0.f ? S.Duration : 6.f);
    if (S.BuffDamage > 0.f) { Buffs.Damage = S.BuffDamage; Buffs.DamageUntil = float(Now + For); }
    if (S.BuffArmor > 0.f) { Buffs.Armor = S.BuffArmor; Buffs.ArmorUntil = float(Now + For); }
    if (S.BuffSpeed > 0.f) { Buffs.Speed = S.BuffSpeed; Buffs.SpeedUntil = float(Now + For); }
    if (S.BuffRegen > 0.f) { Buffs.Regen = S.BuffRegen * Scale; Buffs.RegenUntil = float(Now + For); }
    if (S.Shield > 0.f) { Buffs.Shield = S.Shield * Scale; Buffs.ShieldUntil = float(Now + (S.Duration > 0.f ? S.Duration : 6.f)); }
    Buffs.Element = S.Element;
    GetOwner()->ForceNetUpdate();
}

float UVortexClassSkills::DamageBonus() const { return ServerNow() < Buffs.DamageUntil ? Buffs.Damage : 0.f; }
float UVortexClassSkills::ArmorBonus() const { return ServerNow() < Buffs.ArmorUntil ? FMath::Min(Buffs.Armor, .8f) : 0.f; }
float UVortexClassSkills::SpeedBonus() const { return ServerNow() < Buffs.SpeedUntil ? Buffs.Speed : 0.f; }
float UVortexClassSkills::ShieldPoints() const { return ServerNow() < Buffs.ShieldUntil ? Buffs.Shield : 0.f; }
bool UVortexClassSkills::HasDeathward() const { return ServerNow() < Buffs.DeathwardUntil; }
float UVortexClassSkills::HasteBonus() const { return ServerNow() < Buffs.HasteUntil ? Buffs.Haste : 0.f; }

TArray<UVortexClassSkills::FActive> UVortexClassSkills::ActiveBuffs() const
{
    TArray<FActive> Out;
    const float Now = float(ServerNow());
    const FLinearColor C = ElementColor(Buffs.Element);
    if (Now < Buffs.DamageUntil) Out.Add({ FString::Printf(TEXT("Dano +%d%%"), FMath::RoundToInt(Buffs.Damage * 100.f)), C, Buffs.DamageUntil - Now });
    if (Now < Buffs.ArmorUntil) Out.Add({ FString::Printf(TEXT("Proteção %d%%"), FMath::RoundToInt(Buffs.Armor * 100.f)), FLinearColor(.75f, .85f, 1.f), Buffs.ArmorUntil - Now });
    if (Now < Buffs.SpeedUntil) Out.Add({ FString::Printf(TEXT("Velocidade +%d%%"), FMath::RoundToInt(Buffs.Speed * 100.f)), FLinearColor(.6f, 1.f, .8f), Buffs.SpeedUntil - Now });
    if (Now < Buffs.RegenUntil) Out.Add({ TEXT("Vida voltando"), FLinearColor(.45f, 1.f, .5f), Buffs.RegenUntil - Now });
    if (Now < Buffs.ShieldUntil && Buffs.Shield > 0.f) Out.Add({ FString::Printf(TEXT("Escudo %d"), FMath::CeilToInt(Buffs.Shield)), FLinearColor(.6f, .9f, 1.f), Buffs.ShieldUntil - Now });
    if (Now < Buffs.DeathwardUntil) Out.Add({ TEXT("Último Fôlego"), FLinearColor(1.f, .4f, .4f), Buffs.DeathwardUntil - Now });
    if (Now < Buffs.HasteUntil) Out.Add({ FString::Printf(TEXT("Pressa +%d%%"), FMath::RoundToInt(Buffs.Haste * 100.f)), FLinearColor(1.f, .9f, .5f), Buffs.HasteUntil - Now });
    return Out;
}

float UVortexClassSkills::AbsorbIncoming(float Damage, float CurrentHealth)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Damage <= 0.f) return Damage;
    Damage *= 1.f - ArmorBonus();
    if (ShieldPoints() > 0.f)
    {
        const float Taken = FMath::Min(Buffs.Shield, Damage);
        Buffs.Shield -= Taken;
        Damage -= Taken;
        GetOwner()->ForceNetUpdate();
    }
    if (HasDeathward() && Damage >= CurrentHealth) Damage = FMath::Max(0.f, CurrentHealth - 1.f);
    return Damage;
}

void UVortexClassSkills::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    AVortexCharacter* Owner = Hero();
    if (!Owner || !Owner->HasAuthority()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    // zonas: a cada meio segundo ferem/prendem quem está dentro e curam o grupo
    for (int32 i = Zones.Num() - 1; i >= 0; --i)
    {
        FZone& Z = Zones[i];
        if (Now >= Z.Until) { Zones.RemoveAt(i); continue; }
        if (Z.Skill.bFollow) Z.Center = Owner->GetActorLocation();
        if (Now < Z.NextTick) continue;
        Z.NextTick = Now + .5;
        if (Z.Power > 0.f || Z.Skill.SlowTime > 0.f || Z.Skill.WeakenTime > 0.f || Z.Skill.Taunt > 0.f) Area(Z.Skill, Z.Center, Z.Skill.Radius * 100.f, Z.Power);
        if (Z.Skill.Heal > 0.f || Z.Skill.BuffDamage > 0.f || Z.Skill.BuffArmor > 0.f || Z.Skill.BuffSpeed > 0.f)
        {
            for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
            {
                if (FVector::Dist2D(It->GetActorLocation(), Z.Center) > Z.Skill.Radius * 100.f) continue;
                if (UAbilitySystemComponent* ASC = It->GetAbilitySystemComponent(); ASC && Z.Skill.Heal > 0.f)
                    ASC->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(),
                        FMath::Min(ASC->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute()), ASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) + Z.Skill.Heal * .5f * Z.Scale));
                if (UVortexClassSkills* Other = It->FindComponentByClass<UVortexClassSkills>())
                {
                    FVortexClassSkill Brief = Z.Skill;
                    Brief.Heal = 0.f; Brief.Shield = 0.f; Brief.BuffRegen = 0.f; Brief.BuffDuration = .7f;
                    Other->GiveBuff(Brief, Z.Scale);
                }
            }
        }
    }
    // espíritos invocados: mordem os inimigos mais perto
    for (int32 i = Summons.Num() - 1; i >= 0; --i)
    {
        FSummon& M = Summons[i];
        if (Now >= M.Until) { Summons.RemoveAt(i); continue; }
        if (Now < M.NextTick) continue;
        M.NextTick = Now + 1.0;
        TArray<AActor*> Near = Enemies(Owner->GetActorLocation(), FMath::Max(M.Skill.Radius, 16.f) * 100.f);
        Near.Sort([Owner](const AActor& A, const AActor& B) { return FVector::DistSquared(A.GetActorLocation(), Owner->GetActorLocation()) < FVector::DistSquared(B.GetActorLocation(), Owner->GetActorLocation()); });
        for (int32 k = 0; k < Near.Num() && k < FMath::Max(1, M.Skill.Count); ++k) Hit(M.Skill, Near[k], M.Power);
        if (M.Skill.Heal > 0.f)
        {
            if (UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent())
                ASC->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(),
                    FMath::Min(ASC->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute()), ASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) + M.Skill.Heal * M.Scale));
        }
    }
    // bênção de vida que volta
    if (ServerNow() < Buffs.RegenUntil && (RegenTick += DeltaTime) >= 1.f)
    {
        RegenTick = 0.f;
        if (UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent())
            ASC->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(),
                FMath::Min(ASC->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute()), ASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) + Buffs.Regen));
    }
}

// ---- desenho (todas as máquinas) ------------------------------------------------------------------

FLinearColor UVortexClassSkills::ElementColor(FName Element)
{
    const FString E = Element.ToString();
    if (E == TEXT("steel")) return FLinearColor(.85f, .9f, 1.f);
    if (E == TEXT("earth")) return FLinearColor(.85f, .55f, .25f);
    if (E == TEXT("wind")) return FLinearColor(.7f, 1.f, .85f);
    if (E == TEXT("blood")) return FLinearColor(1.f, .12f, .15f);
    if (E == TEXT("star")) return FLinearColor(1.f, .95f, .6f);
    if (E == TEXT("time")) return FLinearColor(.4f, 1.f, .9f);
    if (E == TEXT("moon")) return FLinearColor(.75f, .8f, 1.f);
    if (E == TEXT("spirit")) return FLinearColor(.5f, .95f, 1.f);
    if (E == TEXT("lightning")) return FLinearColor(1.f, 1.f, .55f);
    if (E == TEXT("bone")) return FLinearColor(.95f, .92f, .8f);
    return AVortexProjectile::ElementColor(Element);
}

void UVortexClassSkills::MulticastFx_Implementation(uint8 Slot, FVector_NetQuantize Origin, FVector_NetQuantize Target, const TArray<FVector_NetQuantize>& Points)
{
    AVortexCharacter* Owner = Hero();
    const FVortexClassSkill* S = Skill(Slot);
    UWorld* World = GetWorld();
    if (!S || !World || GetNetMode() == NM_DedicatedServer) return;
    const FString Kind = S->Kind.ToString();
    const FLinearColor Color = ElementColor(S->Element);
    const FVector Feet = FVector(Origin.X, Origin.Y, GroundAt(Origin) + 6.f);
    // 07/11: com efeito próprio (AVortexPowerFx, "classe_<id>", que nasce com a animação), os Niagara prontos saem; ficam
    // só os desenhos que mostram o estado (a zona, o escudo, os espíritos, a corrente, o aviso e o meteoro)
    const bool bOwn = AVortexPowerFx::Has(FName(*(TEXT("classe_") + S->Id.ToString())));
    auto Spawn = [bOwn](UWorld* InWorld, const FString& Path, const FVector& At, const FRotator& Rotation, float Scale)
    {
        if (!bOwn) ::Spawn(InWorld, Path, At, Rotation, Scale);
    };
    const FVector Forward = (FVector(Target) - FVector(Origin)).GetSafeNormal2D().IsNearlyZero() ? (Owner ? Owner->GetActorForwardVector() : FVector::ForwardVector)
        : (FVector(Target) - FVector(Origin)).GetSafeNormal2D();
    auto Fx = [&](EVortexSkillFxKind FxKind, float Life, const TArray<FVector>& P, AActor* Follow, float Size, int32 Count)
    {
        FActorSpawnParameters Params;
        Params.ObjectFlags |= RF_Transient;
        if (AVortexSkillFx* Actor = World->SpawnActor<AVortexSkillFx>(Origin, FRotator::ZeroRotator, Params)) Actor->Start(FxKind, Color, Life, P, Follow, Size, Count);
    };
    // a assinatura de todo lançamento: um anel no chão na cor do elemento
    Spawn(World, ShockwaveFor(S->Element), Feet, FRotator::ZeroRotator, .16f);
    if (Kind == TEXT("projectile") || Kind == TEXT("wave"))
    {
        Spawn(World, ShockwaveFor(S->Element, true), FVector(Origin) + Forward * 90.f + FVector(0, 0, 40.f), Forward.Rotation(), .25f);
    }
    else if (Kind == TEXT("cleave"))
    {
        for (int32 p = 0; p < FMath::Max(1, S->Pulses); ++p)
            Spawn(World, ShockwaveFor(S->Element, true), FVector(Origin) + Forward * S->Radius * 55.f + FVector(0, 0, 40.f), Forward.Rotation(), .32f + .08f * p);
        if (!bOwn) VortexHitFx::Splash(World, FVector(Origin) + Forward * S->Radius * 60.f, Color, 10);
    }
    else if (Kind == TEXT("nova"))
    {
        Spawn(World, ShockwaveFor(S->Element), Feet, FRotator::ZeroRotator, FMath::Max(.25f, S->Radius / 8.f));
        Spawn(World, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Explosion.NS_Free_Spells_Explosion"), FVector(Origin), FRotator::ZeroRotator, FMath::Max(.4f, S->Radius / 6.f));
    }
    else if (Kind == TEXT("strike"))
    {
        // fogo, terra e estrelas caem do céu como meteoros; os outros, uma coluna de luz na marca
        const bool bMeteor = S->Element == TEXT("fire") || S->Element == TEXT("earth") || S->Element == TEXT("star");
        for (int32 k = 0; k < Points.Num(); ++k)
        {
            const FVector P = Points[k];
            if (bMeteor)
            {
                FActorSpawnParameters Params;
                Params.ObjectFlags |= RF_Transient;
                if (AVortexMeteor* Rock = World->SpawnActor<AVortexMeteor>(P, FRotator::ZeroRotator, Params)) Rock->Start(P, S->Delay + k * .07f);
            }
            else Fx(EVortexSkillFxKind::Beam, S->Delay + k * .07f + .3f, { P + FVector(0, 0, 2400.f), P }, nullptr, 40.f, 1);
        }
    }
    else if (Kind == TEXT("zone"))
    {
        Fx(EVortexSkillFxKind::Zone, FMath::Max(1.f, S->Duration), { FVector(Target) }, S->bFollow ? Owner : nullptr, S->Radius * 100.f, 1);
        Spawn(World, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Circle.NS_Free_Spells_Circle"), FVector(Target), FRotator::ZeroRotator, FMath::Max(.5f, S->Radius / 3.f));
    }
    else if (Kind == TEXT("dash"))
    {
        Spawn(World, ShockwaveFor(S->Element, true), FVector(Origin) + FVector(0, 0, 40.f), Forward.Rotation(), .3f);
        if (!bOwn) VortexHitFx::Splash(World, FVector(Origin), Color, 12);
        if (S->Land > 0.f) Spawn(World, ShockwaveFor(S->Element), FVector(Target), FRotator::ZeroRotator, FMath::Max(.25f, S->Land / 8.f));
    }
    else if (Kind == TEXT("buff") || Kind == TEXT("heal"))
    {
        if (Owner && !bOwn)
        {
            if (UNiagaraSystem* Buff = VortexHitFx::System(*SpellFor(S->Element, false)))
                UNiagaraFunctionLibrary::SpawnSystemAttached(Buff, Owner->GetRootComponent(), NAME_None, FVector(0, 0, -80.f), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
            if (Kind == TEXT("buff"))
                if (UNiagaraSystem* Aura = VortexHitFx::System(*SpellFor(S->Element, true)))
                {
                    if (UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAttached(Aura, Owner->GetRootComponent(), NAME_None, FVector(0, 0, -80.f), FRotator::ZeroRotator,
                        EAttachLocation::KeepRelativeOffset, false, true, ENCPoolMethod::None))
                    {
                        TWeakObjectPtr<UNiagaraComponent> Weak(Comp);
                        FTimerHandle Handle;
                        World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([Weak]() { if (Weak.IsValid()) Weak->DestroyComponent(); }),
                            FMath::Max(1.f, S->BuffDuration > 0.f ? S->BuffDuration : S->Duration), false);
                    }
                }
        }
        for (const FVector_NetQuantize& Ally : Points) VortexHitFx::Splash(World, Ally, Color, 8);
    }
    else if (Kind == TEXT("shield"))
    {
        Fx(EVortexSkillFxKind::Shield, FMath::Max(1.f, S->Duration), { FVector(Origin) }, Owner, 115.f, 1);
    }
    else if (Kind == TEXT("chain"))
    {
        TArray<FVector> Line = { FVector(Origin) + FVector(0, 0, 40.f) };
        for (const FVector_NetQuantize& P : Points) Line.Add(FVector(P) + FVector(0, 0, 40.f));
        Fx(EVortexSkillFxKind::Chain, .45f, Line, nullptr, 18.f, 1);
        for (const FVector_NetQuantize& P : Points) VortexHitFx::Splash(World, P, Color, 6);
    }
    else if (Kind == TEXT("beam"))
    {
        Fx(EVortexSkillFxKind::Beam, .3f + .25f * FMath::Max(1, S->Pulses), { FVector(Origin) + FVector(0, 0, 40.f), FVector(Target) + FVector(0, 0, 40.f) }, nullptr, S->Radius * 60.f, 1);
    }
    else if (Kind == TEXT("summon"))
    {
        Fx(EVortexSkillFxKind::Spirits, FMath::Max(2.f, S->Duration), { FVector(Origin) }, Owner, S->Radius * 40.f, FMath::Clamp(S->Count, 1, 4));
    }
    VortexAudio::Play(this, FName(*FString::Printf(TEXT("class_%s_impact"), *Kind)), FVector(Target), .7f);
}

UAnimSequence* UVortexClassSkills::GestureClip(const FString& Gesture, FName WeaponType)
{
    // a família de cada tipo de arma: a postura dela em Data/Rpg/weapons.json ("Staff_Idle" -> "Staff")
    static TMap<FName, FString> Family;
    static bool bRead = false;
    if (!bRead)
    {
        bRead = true;
        FString Text;
        TSharedPtr<FJsonObject> Root;
        if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Rpg/weapons.json"))) &&
            FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
        {
            for (const TSharedPtr<FJsonValue>& Value : Root->GetArrayField(TEXT("types")))
            {
                const TSharedPtr<FJsonObject> Type = Value->AsObject();
                const TSharedPtr<FJsonObject>* Stance = nullptr;
                FString Idle;
                if (Type->TryGetObjectField(TEXT("stance"), Stance) && (*Stance)->TryGetStringField(TEXT("Idle"), Idle) && Idle.EndsWith(TEXT("_Idle")))
                    Family.Add(FName(*Type->GetStringField(TEXT("id"))), Idle.LeftChop(5));
            }
        }
    }
    static TMap<FString, TObjectPtr<UAnimSequence>> Cache;
    auto Load = [](const FString& Name) -> UAnimSequence*
    {
        if (const TObjectPtr<UAnimSequence>* Found = Cache.Find(Name)) return Found->Get();
        const FString Asset = TEXT("Adventurer_SkillClips") + Name;
        UAnimSequence* Clip = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Weapons/SkillClips/Adventurer_SkillClips/SkeletalMeshes/%s.%s"), *Asset, *Asset));
        if (Clip) Clip->AddToRoot();   // poucos e pequenos: ficam carregados
        Cache.Add(Name, Clip);
        return Clip;
    };
    if (Gesture.IsEmpty()) return nullptr;
    if (const FString* Prefix = Family.Find(WeaponType))
        if (UAnimSequence* Clip = Load(FString::Printf(TEXT("Skill_%s_%s"), *Gesture, **Prefix))) return Clip;
    return Load(TEXT("Skill_") + Gesture);
}

UAnimSequence* UVortexClassSkills::CastAnimation(float& OutTime) const
{
    OutTime = float(ServerNow()) - CastServerTime;
    const FVortexClassSkill* S = Skill(CastSlot);
    const AVortexCharacter* Owner = Hero();
    // 07/11: a animação feita para o poder (Tools/Blender/class_anims_hand.py -> MX_<raça>_Poder_classe_<id>_Full) e o
    // efeito dele (AVortexPowerFx "classe_<id>"), que nasce uma vez por lançamento em cada máquina (como as espadas)
    if (S && Owner && OutTime >= 0.f)
    {
        const USkeletalMesh* Mesh = Owner->GetMesh() ? Owner->GetMesh()->GetSkeletalMeshAsset() : nullptr;
        const FString Race = Mesh && Mesh->GetName().StartsWith(TEXT("Hero_")) ? Mesh->GetName().RightChop(5).Replace(TEXT("_FP"), TEXT("")) : FString();
        UAnimSequence* Own = nullptr;
        if (!Race.IsEmpty())
        {
            static TMap<FString, UAnimSequence*> Clips;
            const FString Name = FString::Printf(TEXT("MX_%s_Poder_classe_%s_Full"), *Race, *S->Id.ToString());
            if (UAnimSequence** Found = Clips.Find(Name)) Own = *Found;
            else
            {
                const FString Package = FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/Anims/%s/%s"), *Race, *Name);
                Own = FPackageName::DoesPackageExist(Package) ? LoadObject<UAnimSequence>(nullptr, *(Package + TEXT(".") + Name)) : nullptr;
                if (Own) Own->AddToRoot();
                Clips.Add(Name, Own);
            }
        }
        static TMap<TWeakObjectPtr<const UVortexClassSkills>, float> Shown;
        if (OutTime < .4f && Shown.FindRef(this) != CastServerTime)
        {
            Shown.Add(this, CastServerTime);
            AVortexPowerFx::Spawn(const_cast<AVortexCharacter*>(Owner), FName(*(TEXT("classe_") + S->Id.ToString())));
        }
        if (Own) return OutTime <= Own->GetPlayLength() ? Own : nullptr;
    }
    const UVortexWeaponDefinition* Weapon = Owner && Owner->GetWeapon() ? Owner->GetWeapon()->GetDefinition() : nullptr;
    if (!S || !Weapon || Weapon->Swings.IsEmpty() || OutTime < 0.f || OutTime > .75f) return nullptr;
    // o golpe da arma na mão (finalização, combo ou estocada)
    const FString Anim = S->Anim.ToString();
    if (Anim == TEXT("WeaponCombo")) return Weapon->Swings[0].Animation.Get();
    if (Anim == TEXT("WeaponThrust")) return Weapon->Swings[FMath::Min(1, Weapon->Swings.Num() - 1)].Animation.Get();
    if (Anim == TEXT("WeaponFinisher") || Anim == TEXT("Dash")) return Weapon->Swings.Last().Animation.Get();
    // os outros são os gestos do Godot (Tools/ImportSkillClips.py): Skill_<gesto>_<família> com o braço na guarda da
    // arma (cajado, orbe, besta...) ou Skill_<gesto> de mãos livres (espada, martelo, arco)
    if (UAnimSequence* Gesture = GestureClip(Anim, Owner->GetSheet() ? Owner->GetSheet()->GetWeaponType() : NAME_None)) return Gesture;
    if (Weapon->SecondaryHoldClip) return Weapon->SecondaryHoldClip.Get();
    if (Weapon->GuardAnimation) return Weapon->GuardAnimation.Get();
    return Weapon->Swings.Last().Animation.Get();
}

// ---- o ator do desenho ---------------------------------------------------------------------------

AVortexSkillFx::AVortexSkillFx()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

UStaticMeshComponent* AVortexSkillFx::AddPart(const TCHAR* Shape)
{
    UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
    Part->SetupAttachment(RootComponent);
    Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Shape, Shape)));
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetCastShadow(false);
    Part->SetUsingAbsoluteLocation(true);
    Part->SetUsingAbsoluteRotation(true);
    Part->SetUsingAbsoluteScale(true);
    if (Glow) Part->SetMaterial(0, Glow);
    Part->RegisterComponent();
    Parts.Add(Part);
    return Part;
}

void AVortexSkillFx::Start(EVortexSkillFxKind InKind, const FLinearColor& InColor, float InLife, const TArray<FVector>& InPoints, AActor* InFollow, float InSize, int32 InCount)
{
    Kind = InKind;
    Color = InColor;
    Life = FMath::Max(.1f, InLife);
    Points = InPoints;
    Follow = InFollow;
    Size = InSize;
    Count = InCount;
    if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, GlowMaterial))
    {
        Glow = UMaterialInstanceDynamic::Create(Base, this);
        Glow->SetVectorParameterValue(TEXT("Color"), Color);
        Glow->SetScalarParameterValue(TEXT("Opacity"), 1.f);
    }
    switch (Kind)
    {
    case EVortexSkillFxKind::Beam:
    case EVortexSkillFxKind::Chain:
        for (int32 i = 0; i + 1 < Points.Num(); ++i) AddPart(TEXT("Cylinder"));
        break;
    case EVortexSkillFxKind::Shield: AddPart(TEXT("Sphere")); break;
    case EVortexSkillFxKind::Zone: AddPart(TEXT("Cylinder")); AddPart(TEXT("Cylinder")); break;
    case EVortexSkillFxKind::Spirits: for (int32 i = 0; i < Count; ++i) AddPart(TEXT("Cube")); break;
    }
    Tick(0.f);
}

void AVortexSkillFx::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    const float Fade = FMath::Clamp(FMath::Min(Age / .12f, (Life - Age) / .25f), 0.f, 1.f);
    // superfícies grandes (escudo, zona) bem fracas: o material soma luz e estourava em branco
    if (Glow) Glow->SetScalarParameterValue(TEXT("Opacity"), Fade * (Kind == EVortexSkillFxKind::Shield ? .09f : Kind == EVortexSkillFxKind::Zone ? .18f
        : Kind == EVortexSkillFxKind::Spirits ? .45f : 1.f));
    const FVector Anchor = Follow.IsValid() ? Follow->GetActorLocation() : (Points.Num() ? Points[0] : GetActorLocation());
    switch (Kind)
    {
    case EVortexSkillFxKind::Beam:
    case EVortexSkillFxKind::Chain:
        for (int32 i = 0; i + 1 < Points.Num() && i < Parts.Num(); ++i)
        {
            // cilindro esticado entre os pontos (a corrente treme)
            FVector A = Points[i], B = Points[i + 1];
            if (Kind == EVortexSkillFxKind::Chain) { A += FMath::VRand() * 12.f; B += FMath::VRand() * 12.f; }
            const FVector D = B - A;
            const float Thick = Size / 50.f * (Kind == EVortexSkillFxKind::Chain ? 1.f : (.6f + .4f * FMath::Sin(Age * 40.f)));
            Parts[i]->SetWorldLocationAndRotation((A + B) * .5f, FRotationMatrix::MakeFromZ(D.GetSafeNormal()).Rotator());
            Parts[i]->SetWorldScale3D(FVector(Thick, Thick, D.Size() / 100.f));
        }
        break;
    case EVortexSkillFxKind::Shield:
        if (Parts.Num())
        {
            const float S = Size / 50.f * (1.f + .04f * FMath::Sin(Age * 6.f));
            Parts[0]->SetWorldLocation(Anchor + FVector(0, 0, 10.f));
            Parts[0]->SetWorldScale3D(FVector(S));
        }
        break;
    case EVortexSkillFxKind::Zone:
        if (Parts.Num() >= 2)
        {
            const float Ground = GroundAt(Anchor);
            const float R = Size / 50.f * FMath::Min(1.f, Age / .25f);
            Parts[0]->SetWorldLocation(FVector(Anchor.X, Anchor.Y, Ground + 4.f));
            Parts[0]->SetWorldScale3D(FVector(R, R, .02f));
            // o anel interno gira e pulsa
            const float Pulse = R * (.55f + .35f * FMath::Frac(Age * .8f));
            Parts[1]->SetWorldLocation(FVector(Anchor.X, Anchor.Y, Ground + 8.f));
            Parts[1]->SetWorldScale3D(FVector(Pulse, Pulse, .03f));
        }
        break;
    case EVortexSkillFxKind::Spirits:
        for (int32 i = 0; i < Parts.Num(); ++i)
        {
            const float A = Age * 2.4f + 2.f * UE_PI * i / FMath::Max(1, Parts.Num());
            const float R = 140.f + 30.f * FMath::Sin(Age * 3.f + i);
            Parts[i]->SetWorldLocation(Anchor + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 60.f + 25.f * FMath::Sin(Age * 4.f + i * 2.f)));
            Parts[i]->SetWorldRotation(FRotator(Age * 120.f, Age * 90.f, 0.f));
            Parts[i]->SetWorldScale3D(FVector(.18f));
        }
        break;
    }
    if (Age >= Life) Destroy();
}
