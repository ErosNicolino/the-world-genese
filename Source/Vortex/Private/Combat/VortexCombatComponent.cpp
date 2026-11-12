#include "Combat/VortexCombatComponent.h"
#include "World/VortexAwakeningStadium.h"
#include "Rpg/VortexArmor.h"
#include "Combat/VortexWeaponKit.h"
#include "Creatures/VortexCreature.h"
#include "EngineUtils.h"
#include "Rpg/VortexDuelDome.h"
#include "Creatures/VortexVolcanicBoss.h"
#include "World/VortexTown.h"
#include "World/VortexBreakable.h"
#include "Rpg/VortexRpg.h"
#include "Rpg/VortexSocial.h"
#include "Combat/VortexHitFx.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Combat/VortexWeaponComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexDamageEffect.h"
#include "Rpg/VortexHeroSheet.h"
#include "Combat/VortexClassSkills.h"
#include "Combat/VortexWeaponComponent.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacterMovementComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "NativeGameplayTags.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Event_Combat_Hit, "Event.Combat.Hit");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Event_Combat_Death, "Event.Combat.Death");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Event_Combat_Parry, "Event.Combat.Parry");

UVortexCombatComponent::UVortexCombatComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
}

bool UVortexCombatComponent::TryMeleeHit(AActor* Target, float Damage, float ReachCm, float MinForwardDot, float StunSeconds, bool bMagic, float DamageScale)
{
    AActor* Source = GetOwner();
    if (!Source || !Source->HasAuthority() || !IsValid(Target) || Target == Source ||
        !FMath::IsFinite(Damage) || Damage <= 0.f || Damage > 10000.f ||
        !FMath::IsFinite(ReachCm) || ReachCm <= 0.f || ReachCm > 1000.f ||
        !FMath::IsFinite(MinForwardDot) || MinForwardDot < -1.f || MinForwardDot > 1.f) return false;

    if (!CanHarm(Target)) return false;

    const FVector From = Source->GetActorLocation() + FVector(0, 0, 60.f);
    const FVector To = Target->GetActorLocation() + FVector(0, 0, 60.f);
    const FVector Delta = To - From;
    const FVector Flat(Delta.X, Delta.Y, 0.f);
    FVector AttackForward = Source->GetActorForwardVector().GetSafeNormal2D();
    // o alcance conta até a borda do corpo do alvo (um golem largo é acertado de onde se acerta um coelho)
    float TargetRadius = 0.f, TargetHalfHeight = 90.f;
    if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Target->GetRootComponent()))
    {
        TargetRadius = Capsule->GetScaledCapsuleRadius();
        TargetHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    }
    const float FlatDistance = Flat.Size();
    const float Surface = FMath::Max(0.f, FlatDistance - TargetRadius);
    // alvo grande cobre um ângulo maior: o cone abre o quanto o corpo ocupa visto de quem bate
    const float Spread = FlatDistance > 1.f ? FMath::Atan(TargetRadius / FlatDistance) : UE_HALF_PI;
    const float ConeDot = FMath::Cos(FMath::Min(UE_PI, FMath::Acos(FMath::Clamp(MinForwardDot, -1.f, 1.f)) + Spread));
    bool bPlayerMelee = false;
    bool bFlexibleMelee = false;
    if (const AVortexCharacter* Attacker = Cast<AVortexCharacter>(Source))
    {
        const UVortexWeaponComponent* Gear = Attacker->GetWeapon();
        bPlayerMelee = Attacker->IsPlayerControlled() && Gear && Gear->GetDefinition() && Gear->GetDefinition()->IsMelee() && !Attacker->IsAimHeld();
        if (bPlayerMelee)
        {
            const VortexWeaponKit::FKit* Kit = VortexWeaponKit::Find(Gear->GetDefinition()->WeaponId);
            bFlexibleMelee = Kit && (Kit->Family == TEXT("Whip") || Kit->Family == TEXT("Chain"));
        }
        // arma de perto: bate no que está à frente do herói (ele já se virou para o alvo); as outras seguem a mira
        if (Attacker->IsPlayerControlled() && !bPlayerMelee)
        {
            FHitResult AimHit;
            const bool bAimHit = Attacker->TraceAim(AimHit);
            const FVector AimPoint = bAimHit ? AimHit.ImpactPoint : AimHit.TraceEnd;
            const FVector AimDirection = (AimPoint - From).GetSafeNormal2D();
            if (!AimDirection.IsNearlyZero()) AttackForward = AimDirection;
        }
    }
    // golpe de perto do herói: ele já se virou para o alvo, então o leque é o do corte (~70° para cada lado);
    // antes ia a ~95° e pegava quem estava do lado e até um pouco atrás
    const float PlayerMinDot = bFlexibleMelee ? MinForwardDot : FMath::Max(MinForwardDot, .35f);
    const float Cone = bPlayerMelee ? FMath::Cos(FMath::Min(UE_PI, FMath::Acos(PlayerMinDot) + Spread)) : ConeDot;
#if !UE_BUILD_SHIPPING
    // -VortexMeleeSweep: por que o golpe do herói não pegou
    static const bool bExplain = FParse::Param(FCommandLine::Get(), TEXT("VortexMeleeSweep"));
    auto Miss = [&](const TCHAR* Why) { if (bExplain && bPlayerMelee) UE_LOG(LogTemp, Display, TEXT("VORTEX_MELEE_MISS %s: %s borda=%.0f alcance=%.0f dot=%.2f cone=%.2f dz=%.0f"),
        *Target->GetName(), Why, Surface, ReachCm, FlatDistance > 1.f ? FVector::DotProduct(AttackForward, Flat.GetSafeNormal()) : 1.f, Cone, Delta.Z); };
#else
    auto Miss = [](const TCHAR*) {};
#endif
    if (Surface > ReachCm) { Miss(TEXT("longe")); return false; }
    if (FMath::Abs(Delta.Z) > 170.f + TargetHalfHeight) { Miss(TEXT("altura")); return false; }
    if (FlatDistance > 1.f && FVector::DotProduct(AttackForward, Flat.GetSafeNormal()) < Cone) { Miss(TEXT("fora do leque")); return false; }

    FHitResult Obstruction;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexMeleeLOS), false, Source);
    Query.AddIgnoredActor(Target);
    if (GetWorld()->LineTraceSingleByChannel(Obstruction, From, To, ECC_Visibility, Query) &&
        Obstruction.GetActor() != Target)
    {
        Miss(*FString::Printf(TEXT("parede no meio (%s)"), *GetNameSafe(Obstruction.GetActor())));
        return false;
    }

    return ApplyHit(Target, Damage, bMagic ? EVortexDamageKind::Magic : EVortexDamageKind::Melee, StunSeconds, DamageScale);
}

bool UVortexCombatComponent::CanHarm(const AActor* Target) const
{
    const AActor* Source = GetOwner();
    if (!Source || !IsValid(Target) || Target == Source) return false;
    const UVortexCombatComponent* OtherCombat = Target->FindComponentByClass<UVortexCombatComponent>();
    // duelo: os dois heróis se ferem (só um com o outro, só durante a luta)
    const UVortexSocial* Duel = Source->FindComponentByClass<UVortexSocial>();
    const bool bDuel = Duel && Duel->IsDuelFighting(Target);
    // 02/11: PvP aberto (pedido do usuário): herói contra herói de fora do grupo, fora da vila (a vila é segura)
    const bool bPvp = Duel && Source->IsA<AVortexCharacter>() && Target->IsA<AVortexCharacter>() && !Duel->SameParty(Target) &&
        !AVortexTown::Contains(Source->GetActorLocation()) && !AVortexTown::Contains(Target->GetActorLocation());
    if (!OtherCombat || (!bDuel && !bPvp && (Faction == EVortexFaction::Neutral || OtherCombat->Faction == EVortexFaction::Neutral ||
        Faction == OtherCombat->Faction))) return false;
    // 02/11: dentro do domo do duelo só os dois se ferem; de fora para dentro (e de dentro para fora) nada passa
    if (!bDuel && (AVortexDuelDome::Containing(GetWorld(), Target->GetActorLocation(), 60.f) || AVortexDuelDome::Containing(GetWorld(), Source->GetActorLocation(), 60.f))) return false;
    const IAbilitySystemInterface* TargetAbility = Cast<IAbilitySystemInterface>(Target);
    const UAbilitySystemComponent* TargetASC = TargetAbility ? TargetAbility->GetAbilitySystemComponent() : nullptr;
    if (!TargetASC || TargetASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) <= 0.f) return false;
    if (const AVortexCharacter* Character = Cast<AVortexCharacter>(Target))
    {
        if (Character->GetVortexMovement()->IsDodging()) return false;
        if (AVortexAwakeningStadium::Contains(Character->GetActorLocation())) return false;   // 08/11: a arena da escolha da arma é segura
    }
    return true;
}

// Dano já validado (alcance/visada de quem chamou): defesa, efeito de dano, eventos e o aviso aos clientes.
bool UVortexCombatComponent::ApplyHit(AActor* Target, float Damage, EVortexDamageKind Kind, float StunSeconds, float DamageScale)
{
    AActor* Source = GetOwner();
    if (!Source || !Source->HasAuthority() || !FMath::IsFinite(Damage) || Damage <= 0.f || Damage > 10000.f || !CanHarm(Target)) return false;
    IAbilitySystemInterface* SourceAbility = Cast<IAbilitySystemInterface>(Source);
    IAbilitySystemInterface* TargetAbility = Cast<IAbilitySystemInterface>(Target);
    UAbilitySystemComponent* SourceASC = SourceAbility ? SourceAbility->GetAbilitySystemComponent() : nullptr;
    UAbilitySystemComponent* TargetASC = TargetAbility ? TargetAbility->GetAbilitySystemComponent() : nullptr;
    if (!SourceASC || !TargetASC) return false;
    UVortexCombatComponent* TargetCombat = Target->FindComponentByClass<UVortexCombatComponent>();
    UVortexHeroSheet* SourceSheet = Source->FindComponentByClass<UVortexHeroSheet>();
    UVortexHeroSheet* TargetSheet = Target->FindComponentByClass<UVortexHeroSheet>();
    const double Now = GetWorld()->GetTimeSeconds();

    bool bPerfectParry = false;
    const UVortexWeaponComponent* TargetWeapon = Target->FindComponentByClass<UVortexWeaponComponent>();
    const float EffectiveDamage = TargetWeapon ? TargetWeapon->ResolveGuardDamage(Source, Damage, bPerfectParry) : Damage;
    if (bPerfectParry)
    {
        FGameplayEventData ParryEvent;
        ParryEvent.EventTag = TAG_Event_Combat_Parry;
        ParryEvent.Instigator = Source;
        ParryEvent.Target = Target;
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Source, TAG_Event_Combat_Parry, ParryEvent);
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, TAG_Event_Combat_Parry, ParryEvent);
        // o Godot (StunParriedMonster): quem foi aparado fica atordoado 0,65 s; o herói ganha o contra-ataque
        const bool bStunned = !Source->IsA<AVortexCharacter>() && !bStunImmune;
        if (bStunned) Stun(ParryStunSeconds);
        if (TargetCombat)
        {
            TargetCombat->CounterUntil = Now + CounterWindowSeconds;
            const AGameStateBase* State = GetWorld()->GetGameState();
            TargetCombat->CounterServerUntil = float((State ? State->GetServerWorldTimeSeconds() : Now) + CounterWindowSeconds);
        }
        if (TargetSheet) TargetSheet->PushNotice(EffectiveDamage <= 0.f ? TEXT("Aparado!  Contra-ataque pronto") : TEXT("Aparado de raspão"));
        MulticastParry(Source, bStunned);
        UE_LOG(LogTemp, Display, TEXT("VORTEX_COMBAT_PARRY target=%s incoming=%.1f through=%.1f stun=%d"),
            *GetNameSafe(Target), Damage, EffectiveDamage, bStunned ? 1 : 0);
    }
    if (EffectiveDamage <= 0.f) return true; // consumed by perfect parry; still one contact this swing
    // ficha de RPG: atributos e equipamento de quem bate; defesa/resistência e raça de quem apanha.
    // Só entre herói e criatura (heróis não se ferem no jogo; os testes de duelo usam o dano puro).
    float FinalDamage = EffectiveDamage * OutgoingScale() * (TargetCombat ? TargetCombat->IncomingScale() : 1.f);
    if (TargetCombat && TargetCombat->bShieldFront)
    {
        const FVector Toward = (Source->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
        if (FVector::DotProduct(Target->GetActorForwardVector().GetSafeNormal2D(), Toward) > .35f)
        {
            FinalDamage *= .4f;
            if (SourceSheet) SourceSheet->PushNotice(TEXT("Bloqueado pelo escudo: ataque pelas costas"));
        }
    }
    bool bCrit = false;
    if (SourceSheet && !TargetSheet)
    {
        // contra-ataque: o primeiro golpe depois do aparo perfeito sai crítico e 50% mais forte
        const bool bCounter = HasCounterReady();
        if (bCounter)
        {
            CounterUntil = 0.0;
            CounterServerUntil = 0.f;
            SourceSheet->PushNotice(TEXT("Contra-ataque!"), 3);
        }
        // 07/11: a armadura (VortexArmor): o golpe oportuno depois da esquiva sai crítico; feras e vida cheia batem mais
        const bool bArmorCrit = VortexArmor::ConsumeCrit(Source);
        FinalDamage = SourceSheet->OutgoingDamage(FinalDamage * (bCounter ? 1.5f : 1.f), Kind, bCrit, bCounter || bArmorCrit);
        FinalDamage *= VortexArmor::OutgoingScale(SourceSheet, Target);
        if (const UVortexClassSkills* Blessing = Source->FindComponentByClass<UVortexClassSkills>()) FinalDamage *= 1.f + Blessing->DamageBonus();
        // criatura atordoada fica exposta: +25%
        if (TargetCombat && TargetCombat->IsStunned()) FinalDamage = FMath::RoundToFloat(FinalDamage * 1.25f);
    }
    // 02/11: herói contra herói (duelo, PvP) usava só o dano puro da arma (1 ou 2 por golpe). Agora a ficha de quem bate
    // (arma, nível, atributos, crítico), a defesa de quem apanha e o pvpDamage do balance.json (a luta dura um pouco)
    else if (SourceSheet && TargetSheet)
    {
        FinalDamage = SourceSheet->OutgoingDamage(FinalDamage, Kind, bCrit, false);
        if (const UVortexClassSkills* Blessing = Source->FindComponentByClass<UVortexClassSkills>()) FinalDamage *= 1.f + Blessing->DamageBonus();
        FinalDamage = TargetSheet->IncomingDamage(FinalDamage, Kind == EVortexDamageKind::Magic) * FVortexRpgCatalog::Get().PvpDamage;
    }
    FinalDamage *= FMath::Max(0.f, DamageScale);
    if (TargetSheet && !SourceSheet) FinalDamage = TargetSheet->IncomingDamage(FinalDamage, bMagicAttacks || Kind == EVortexDamageKind::Magic);
    // diferença de nível entre herói e criatura
    {
        const IVortexCreature* SourceCreature = Cast<IVortexCreature>(Source);
        const IVortexCreature* TargetCreature = Cast<IVortexCreature>(Target);
        if (SourceSheet && TargetCreature) FinalDamage *= VortexRpg::GapScale(SourceSheet->GetLevel(), TargetCreature->GetCreatureLevel(), false);
        // 02/11: o golem vulcânico atordoado leva mais dano (Data/Creatures/azhur.json: stagger.damageTaken)
        if (const UVortexVolcanicBoss* Volcano = Target->FindComponentByClass<UVortexVolcanicBoss>()) FinalDamage *= Volcano->IncomingScale();
        // 02/11: com mais heróis em volta o bicho aguenta mais (a vida vale 1 + groupHealthPerHero por herói a mais)
        if (SourceSheet && TargetCreature)
        {
            const FVortexRpgCatalog& Rpg = FVortexRpgCatalog::Get();
            int32 Group = 0;
            for (TActorIterator<AVortexCharacter> It(Target->GetWorld()); It; ++It)
            {
                const UAbilitySystemComponent* A = It->GetAbilitySystemComponent();
                if (A && A->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) > 0.f &&
                    FVector::Dist(It->GetActorLocation(), Target->GetActorLocation()) <= Rpg.GroupRange) ++Group;
            }
            if (Group > 1) FinalDamage /= 1.f + Rpg.GroupHealthPerHero * (Group - 1);
        }
        else if (SourceCreature && TargetSheet) FinalDamage *= VortexRpg::GapScale(SourceCreature->GetCreatureLevel(), TargetSheet->GetLevel(), true);
    }
    // 07/11: as passivas da armadura de quem apanhou (o bloqueio, os espinhos, o renascer...)
    if (TargetSheet && !SourceSheet) FinalDamage = VortexArmor::Incoming(TargetSheet, Source, FinalDamage);
    if (UVortexClassSkills* Guard = Target->FindComponentByClass<UVortexClassSkills>(); Guard && !SourceSheet)
        FinalDamage = Guard->AbsorbIncoming(FinalDamage, TargetASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()));
    if (FinalDamage <= 0.f) return true;
    const float Before = TargetASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
    // duelo: ninguém passa de 15% da vida; quem chega lá perde (e os dois voltam à vida de antes)
    UVortexSocial* DuelTarget = Target->FindComponentByClass<UVortexSocial>();
    const bool bDuelHit = DuelTarget && DuelTarget->IsDuelFighting(Source);
    if (bDuelHit)
    {
        const float Floor = TargetASC->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute()) * UVortexSocial::DuelLoseAt;
        FinalDamage = FMath::Min(FinalDamage, FMath::Max(0.f, Before - Floor));
    }
    const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(UVortexDamageEffect::StaticClass(), 1.f, SourceASC->MakeEffectContext());
    if (!Spec.IsValid()) return false;
    Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Damage")), -FinalDamage);
    SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
    if (TargetCombat)
    {
        TargetCombat->LastAttacker = Source;
        TargetCombat->LastHurtTime = Now;
    }
    const float Actual = Before - TargetASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
    if (Actual <= 0.f) return false;
    if (SourceSheet && !TargetSheet) SourceSheet->Leech(Actual);
    const bool bDied = Before > 0.f && TargetASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) <= 0.f;
    // 02/11: matou um herói de fora do grupo (sem ser duelo, e ele não era caveira): caveira por 3 dias do jogo
    if (bDied && !bDuelHit && SourceSheet && TargetSheet && !TargetSheet->IsOutlaw())
    {
        const UVortexSocial* Killer = Source->FindComponentByClass<UVortexSocial>();
        if (!Killer || !Killer->SameParty(Target)) SourceSheet->MarkOutlaw();
    }
    if (bDuelHit && TargetASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) <= TargetASC->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute()) * UVortexSocial::DuelLoseAt + .5f)
        DuelTarget->DuelLost(Cast<AVortexCharacter>(Source));
    // atordoamento por golpe: só os golpes pesados (>= 0,3 s) e só de vez em quando. Depois de um
    // atordoamento a criatura fica 3 s firme (senão ficava presa na animação para sempre)
    bool bStaggered = false;
    if (!bDied && StunSeconds >= .3f && TargetCombat && !Target->IsA<AVortexCharacter>() && Now >= TargetCombat->StunGuardUntil)
    {
        bStaggered = !TargetCombat->bStunImmune;
        TargetCombat->Stun(FMath::Min(StunSeconds, 1.2f));
    }

    FGameplayEventData Event;
    Event.EventTag = TAG_Event_Combat_Hit;
    Event.Instigator = Source;
    Event.Target = Target;
    Event.EventMagnitude = Actual;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Source, TAG_Event_Combat_Hit, Event);
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, TAG_Event_Combat_Hit, Event);
    if (bDied)
    {
        Event.EventTag = TAG_Event_Combat_Death;
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, TAG_Event_Combat_Death, Event);
        UVortexHeroSheet::AwardKill(Source, Target);
    }
    // empurrão: a criatura acertada recua um pouco (mais no crítico); heróis e chefes não são empurrados
    // só no crítico ou quando atordoa: empurrar a cada golpe afastava o bicho e o herói tinha que correr atrás
    if (ACharacter* Body = Cast<ACharacter>(Target); Body && (bCrit || bStaggered) && !Target->IsA<AVortexCharacter>() && !(TargetCombat && TargetCombat->bStunImmune))
    {
        const FVector Away = (Target->GetActorLocation() - Source->GetActorLocation()).GetSafeNormal2D();
        Body->LaunchCharacter(Away * (bCrit ? 300.f : 180.f) + FVector(0.f, 0.f, bCrit ? 120.f : 60.f), true, false);
    }
    MulticastConfirmedHit(Target, Actual, bCrit);
    // arma com elemento ou peculiaridade (queimadura, lentidão, raio em cadeia...)
    if (Source->IsA<AVortexCharacter>()) VortexWeaponKit::OnWeaponHit(Source, Target, Actual);
    if (Source->IsA<AVortexCharacter>() && !TargetSheet) VortexArmor::OnDealt(Source, Target, Actual, bDied);
    return true;
}

void UVortexCombatComponent::ApplySlow(float Factor, float Seconds)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Seconds <= 0.f) return;
    // chefes não ficam presos (só mais lentos)
    SlowFactor = bStunImmune ? FMath::Max(.6f, Factor) : FMath::Clamp(Factor, 0.f, 1.f);
    SlowUntil = GetWorld()->GetTimeSeconds() + Seconds;
}

void UVortexCombatComponent::ApplyWeaken(float Factor, float Seconds)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Seconds <= 0.f) return;
    WeakenFactor = FMath::Clamp(Factor, 0.f, .8f);
    WeakenUntil = GetWorld()->GetTimeSeconds() + Seconds;
}

void UVortexCombatComponent::ApplyExpose(float Factor, float Seconds)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Seconds <= 0.f) return;
    ExposeFactor = FMath::Clamp(Factor, 0.f, 1.f);
    ExposeUntil = GetWorld()->GetTimeSeconds() + Seconds;
}

void UVortexCombatComponent::ApplyEmpower(float Factor, float Seconds)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Seconds <= 0.f) return;
    EmpowerFactor = FMath::Clamp(Factor, 0.f, 1.f);
    EmpowerUntil = GetWorld()->GetTimeSeconds() + Seconds;
}

void UVortexCombatComponent::ApplyDot(float Dps, float Seconds, AActor* Source)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Seconds <= 0.f || Dps <= 0.f) return;
    DotDps = FMath::Max(DotDps * (GetWorld()->GetTimeSeconds() < DotUntil ? 1.f : 0.f), Dps);
    DotUntil = GetWorld()->GetTimeSeconds() + Seconds;
    DotSource = Source;
    if (!GetWorld()->GetTimerManager().IsTimerActive(DotTimer))
        GetWorld()->GetTimerManager().SetTimer(DotTimer, this, &UVortexCombatComponent::TickDot, .5f, true);
}

void UVortexCombatComponent::TickDot()
{
    AActor* Source = DotSource.Get();
    UVortexCombatComponent* From = Source ? Source->FindComponentByClass<UVortexCombatComponent>() : nullptr;
    if (!From || GetWorld()->GetTimeSeconds() >= DotUntil)
    {
        GetWorld()->GetTimerManager().ClearTimer(DotTimer);
        return;
    }
    // metade do dano por segundo a cada meio segundo; não atordoa nem empurra
    From->ApplyHit(GetOwner(), FMath::Max(1.f, FMath::RoundToFloat(DotDps * .5f)), EVortexDamageKind::Magic, 0.f);
}

float UVortexCombatComponent::MoveScale() const
{
    if (!GetWorld()) return 1.f;
    const double Now = GetWorld()->GetTimeSeconds();
    return (Now < SlowUntil ? SlowFactor : 1.f) * (Now < EmpowerUntil ? 1.f + EmpowerFactor : 1.f);
}

float UVortexCombatComponent::OutgoingScale() const
{
    if (!GetWorld()) return 1.f;
    const double Now = GetWorld()->GetTimeSeconds();
    return (Now < WeakenUntil ? 1.f - WeakenFactor : 1.f) * (Now < EmpowerUntil ? 1.f + EmpowerFactor : 1.f);
}

float UVortexCombatComponent::IncomingScale() const
{
    return GetWorld() && GetWorld()->GetTimeSeconds() < ExposeUntil ? 1.f + ExposeFactor : 1.f;
}

void UVortexCombatComponent::Stun(float Seconds)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || bStunImmune || Seconds <= 0.f) return;
    StunnedUntil = FMath::Max(StunnedUntil, GetWorld()->GetTimeSeconds() + double(Seconds));
    StunGuardUntil = StunnedUntil + StunGuardSeconds;
    OnStunned.Broadcast(Seconds);
}

bool UVortexCombatComponent::IsStunned() const { return StunRemaining() > 0.f; }

float UVortexCombatComponent::StunRemaining() const
{
    return GetWorld() ? FMath::Max(0.f, float(StunnedUntil - GetWorld()->GetTimeSeconds())) : 0.f;
}

bool UVortexCombatComponent::HasCounterReady() const
{
    return GetWorld() && CounterUntil > GetWorld()->GetTimeSeconds();
}

float UVortexCombatComponent::CounterReadyFor() const
{
    const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    const double Now = State ? State->GetServerWorldTimeSeconds() : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
    return FMath::Max(0.f, float(CounterServerUntil - Now));
}

void UVortexCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UVortexCombatComponent, CounterServerUntil, COND_OwnerOnly);
}

void UVortexCombatComponent::MulticastParry_Implementation(AActor* Attacker, bool bStunned)
{
    // o tinido do aço e as faíscas entre os dois; atordoado: as estrelinhas sobre quem foi aparado
    const AActor* Owner = GetOwner();
    if (!Owner || !Attacker) return;
    const FVector Between = (Owner->GetActorLocation() + Attacker->GetActorLocation()) * .5f + FVector(0.f, 0.f, 60.f);
    VortexAudio::Play(this, TEXT("skill_barrier_hit"), Between, 1.f, .04f);
    VortexAudio::Play(this, TEXT("sword_hit"), Between, .9f, .04f);
    VortexHitFx::Parry(Attacker, Between, bStunned);
}

int32 UVortexCombatComponent::StrikeInArc(float Damage, float ReachCm, float MinForwardDot,
    TSet<TWeakObjectPtr<AActor>>& AlreadyHit, float StunSeconds, float DamageScale)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return 0;
    // barris e caixotes da vila no arco do golpe quebram, só com a espada (05/11: flecha, magia e as outras armas não)
    const UVortexWeaponComponent* WeaponComp = GetOwner()->FindComponentByClass<UVortexWeaponComponent>();
    const UVortexWeaponDefinition* Held = WeaponComp ? WeaponComp->GetDefinition() : nullptr;
    if (Held && Held->WeaponId.ToString().Contains(TEXT("espada")))
        AVortexBreakable::BreakInArc(GetWorld(), GetOwner()->GetActorLocation(), GetOwner()->GetActorForwardVector(), ReachCm, MinForwardDot, 380.f);
    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexMeleeOverlap), false, GetOwner());
    GetWorld()->OverlapMultiByObjectType(Overlaps, GetOwner()->GetActorLocation(), FQuat::Identity, Objects,
        FCollisionShape::MakeSphere(ReachCm + 400.f), Query);
    int32 Hits = 0;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Target = Overlap.GetActor();
        if (Target && !AlreadyHit.Contains(Target) && TryMeleeHit(Target, Damage, ReachCm, MinForwardDot, StunSeconds, false, DamageScale))
        {
            AlreadyHit.Add(Target);
            ++Hits;
        }
    }
    return Hits;
}

void UVortexCombatComponent::MulticastConfirmedHit_Implementation(AActor* Target, float Damage, bool bCrit)
{
    // o som do acerto: a arma do herói no alvo; quando quem acerta é um bicho, o golpe dele (03/11: antes todo bicho
    // tocava a mordida do lobo): o lobo morde, o goblin bate a clava, a aranha pica... (<bicho>_strike)
    if (Target)
    {
        const bool bCreature = GetOwner() && !GetOwner()->IsA<AVortexCharacter>();
        FName Strike;
        if (bCreature)
        {
            const IVortexCreature* Attacker = Cast<IVortexCreature>(GetOwner());
            const FName Species = Attacker ? Attacker->GetSpecies() : NAME_None;
            Strike = Species.ToString().StartsWith(TEXT("Wolf")) ? FName(TEXT("wolf_bite")) : VortexAudio::CreatureVoice(Species, TEXT("strike"));
            if (Strike.IsNone()) Strike = Attacker && Attacker->IsBoss() ? FName(TEXT("boss_impact")) : FName(TEXT("wfx_fist_hit"));
        }
        VortexAudio::Play(this, bCreature ? Strike : VortexAudio::WeaponSound(GetOwner(), TEXT("hit"), FMath::Rand(), TEXT("sword_hit")),
            Target->GetActorLocation(), 1.f);
        if (!bCreature && !Target->IsA<AVortexCharacter>())
        {
            // a voz da espécie que apanhou (lobo, coelho, golem...), senão o baque comum
            const IVortexCreature* Creature = Cast<IVortexCreature>(Target);
            const FName Voice = Creature ? VortexAudio::CreatureVoice(Creature->GetSpecies(), TEXT("hit")) : NAME_None;
            VortexAudio::Play(this, Voice.IsNone() ? FName(TEXT("creature_hit")) : Voice, Target->GetActorLocation(), Voice.IsNone() ? .7f : .85f);
        }
    }
    if (bCrit && GetWorld())
    {
        LastCritTime = float(GetWorld()->GetTimeSeconds());
        LastCritTarget = Target;
    }
    // brilho, pausa de impacto, tranco na câmera e respingos (Combat/VortexHitFx)
    VortexHitFx::Play(GetOwner(), Target, Damage, bCrit);
    OnConfirmedHit.Broadcast(GetOwner(), Target, Damage);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_COMBAT_HIT source=%s target=%s damage=%.1f"),
        *GetNameSafe(GetOwner()), *GetNameSafe(Target), Damage);
}
