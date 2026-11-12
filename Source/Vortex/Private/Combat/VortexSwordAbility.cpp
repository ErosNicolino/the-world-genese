#include "Combat/VortexSwordAbility.h"

#include "AbilitySystemComponent.h"
#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Combat/VortexWeaponKit.h"
#include "Rpg/VortexHeroSheet.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexProjectile.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexCharacterMovementComponent.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Weapon_Sword, "Ability.Weapon.Sword.Primary");

UVortexSwordAbility::UVortexSwordAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    SetAssetTags(FGameplayTagContainer(TAG_Ability_Weapon_Sword));
}

bool UVortexSwordAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;
    const AVortexCharacter* Character = ActorInfo ? Cast<AVortexCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    return Character && Character->HasAuthority() && Character->GetWeapon() && Character->GetWeapon()->GetDefinition() &&
        Character->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) > 0.f &&
        !Character->GetVortexMovement()->IsDodging() && !Character->GetVortexMovement()->IsSwimmingInWater() &&
        !Character->GetWeapon()->IsGuarding();
}

void UVortexSwordAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    AVortexCharacter* Character = CastChecked<AVortexCharacter>(ActorInfo->AvatarActor.Get());
    Weapon = Character->GetWeapon();
    StartSwing(Weapon->NextSwingIndex());
}

void UVortexSwordAbility::StartSwing(int32 Index)
{
    const UVortexWeaponDefinition* Definition = Weapon ? Weapon->GetDefinition() : nullptr;
    if (!Definition || !Definition->Swings.IsValidIndex(Index))
    {
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
        return;
    }
    // G6: arma mágica gasta um pouco de mana a cada golpe normal; sem mana, não sai
    if (const VortexWeaponKit::FKit* Kit = VortexWeaponKit::Find(Definition->WeaponId); Kit && Kit->PrimaryMana > 0.f)
    {
        AVortexCharacter* Character = Cast<AVortexCharacter>(Weapon->GetOwner());
        UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
        if (!ASC || ASC->GetNumericAttribute(UVortexAttributes::GetManaAttribute()) < Kit->PrimaryMana)
        {
            if (Character && Character->GetSheet()) Character->GetSheet()->PushNotice(TEXT("Mana insuficiente"));
            EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
            return;
        }
        ASC->ApplyModToAttribute(UVortexAttributes::GetManaAttribute(), EGameplayModOp::Additive, -Kit->PrimaryMana);
    }
    SwingIndex = Index;
    SwingStartTime = GetWorld()->GetTimeSeconds();
    bBuffered = false;
    bWindowOpen = false;
    HitActors.Reset();
    Weapon->RegisterAttackStart(this, Index);
    const FVortexWeaponSwing& Swing = Definition->Swings[Index];
    // Velocidade de ataque (% do equipamento e dos pontos): o golpe inteiro anda mais rápido
    const float Pace = 1.f / Weapon->GetAttackSpeed();
    FTimerManager& Timers = GetWorld()->GetTimerManager();
    Timers.SetTimer(OpenTimer, this, &UVortexSwordAbility::OpenWindow, FMath::Max(.01f, Swing.ActiveStart * Pace), false);
    Timers.SetTimer(CloseTimer, this, &UVortexSwordAbility::CloseWindow, FMath::Max(Swing.ActiveStart * Pace + .01f, Swing.ActiveEnd * Pace), false);
    Timers.SetTimer(FinishTimer, this, &UVortexSwordAbility::FinishSwing, FMath::Max(Swing.ActiveEnd * Pace + .01f, Swing.Duration * Pace), false);
}

void UVortexSwordAbility::BufferNext()
{
    const UVortexWeaponDefinition* Definition = Weapon ? Weapon->GetDefinition() : nullptr;
    if (!Definition || !Definition->Swings.IsValidIndex(SwingIndex + 1) || bBuffered) return;
    const FVortexWeaponSwing& Swing = Definition->Swings[SwingIndex];
    if (GetWorld()->GetTimeSeconds() - SwingStartTime >= Swing.Duration / Weapon->GetAttackSpeed() * Definition->BufferOpenFraction)
        bBuffered = true;
}

void UVortexSwordAbility::OpenWindow()
{
    const UVortexWeaponDefinition* Definition = Weapon ? Weapon->GetDefinition() : nullptr;
    const FVortexWeaponSwing* Swing = Definition && Definition->Swings.IsValidIndex(SwingIndex) ? &Definition->Swings[SwingIndex] : nullptr;
    if (Swing && Swing->Mode != TEXT("melee"))
    {
        // grimório (tornado no alvo), garrafa (arremesso em arco), chakram (vai e volta)
        if (Weapon->FireKitPrimary(SwingIndex)) return;
        // golpes do Godot que não são de corte: um disparo (ou pancada) só, no instante do golpe
        if (Swing->Mode == TEXT("slam"))
        {
            if (AVortexProjectile* Shot = Weapon->LaunchProjectile(TEXT("burst"), Swing->Element.IsNone() ? FName(TEXT("steel")) : Swing->Element, Swing->Damage * VortexWeaponKit::PrimaryDamageScale(Definition->WeaponId), 0.f, 0.f, Swing->RadiusCm))
            {
                Shot->Stun = Swing->Stun;
            }
        }
        else
        {
            // 02/11: a magia (bolt e orb) maior, para ver e acertar (o usuário: "pequena")
            const float Size = Swing->Mode == TEXT("bolt") || Swing->Mode == TEXT("orb") ? 1.6f : 1.f;
            if (AVortexProjectile* Shot = Weapon->LaunchProjectile(Swing->Mode, Swing->Element, Swing->Damage * VortexWeaponKit::PrimaryDamageScale(Definition->WeaponId), Swing->ProjectileSpeed, Swing->ReachCm, Swing->RadiusCm, Size))
            {
                Shot->Stun = Swing->Stun;
            }
        }
        return;
    }
    bWindowOpen = true;
    SampleHit();
    GetWorld()->GetTimerManager().SetTimer(SampleTimer, this, &UVortexSwordAbility::SampleHit, .04f, true);
}

void UVortexSwordAbility::SampleHit()
{
    if (!bWindowOpen || !Weapon) return;
    AVortexCharacter* Character = Cast<AVortexCharacter>(Weapon->GetOwner());
    const UVortexWeaponDefinition* Definition = Weapon->GetDefinition();
    if (!Character || !Definition || !Definition->Swings.IsValidIndex(SwingIndex)) return;
    const FVortexWeaponSwing& Swing = Definition->Swings[SwingIndex];
    const float DamageScale = VortexWeaponKit::PrimaryDamageScale(Definition->WeaponId);
    // 07/11: o golpe da arma multiplica o dano do herói (1 + bônus) x o dano do golpe, como os outros jogos (valor do movimento)
    Character->GetCombat()->StrikeInArc(1.f, Weapon->MeleeReach(Swing), Weapon->MeleeCone(Swing), HitActors, Swing.Stun, Swing.Damage * DamageScale);
}

void UVortexSwordAbility::CloseWindow()
{
    bWindowOpen = false;
    GetWorld()->GetTimerManager().ClearTimer(SampleTimer);
}

void UVortexSwordAbility::FinishSwing()
{
    CloseWindow();
    if (bBuffered && Weapon && Weapon->GetDefinition() && Weapon->GetDefinition()->Swings.IsValidIndex(SwingIndex + 1))
    {
        StartSwing(SwingIndex + 1);
        return;
    }
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UVortexSwordAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (GetWorld())
    {
        FTimerManager& Timers = GetWorld()->GetTimerManager();
        Timers.ClearTimer(OpenTimer);
        Timers.ClearTimer(SampleTimer);
        Timers.ClearTimer(CloseTimer);
        Timers.ClearTimer(FinishTimer);
    }
    if (Weapon && SwingIndex != INDEX_NONE) Weapon->RegisterAttackEnd(this, SwingIndex);
    Weapon = nullptr;
    SwingIndex = INDEX_NONE;
    HitActors.Reset();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
