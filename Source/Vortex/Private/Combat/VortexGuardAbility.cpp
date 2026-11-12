#include "Combat/VortexGuardAbility.h"

#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexWeaponComponent.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexCharacterMovementComponent.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Weapon_Guard, "Ability.Weapon.Sword.Guard");

UVortexGuardAbility::UVortexGuardAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    SetAssetTags(FGameplayTagContainer(TAG_Ability_Weapon_Guard));
}

bool UVortexGuardAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;
    const AVortexCharacter* Character = ActorInfo ? Cast<AVortexCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    return Character && Character->HasAuthority() && Character->GetWeapon() && Character->GetWeapon()->GetDefinition() &&
        Character->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) > 0.f &&
        !Character->GetVortexMovement()->IsDodging() && !Character->GetVortexMovement()->IsSwimmingInWater();
}

void UVortexGuardAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
    Weapon->RegisterGuardStart(this);
}

void UVortexGuardAbility::Release()
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UVortexGuardAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (Weapon) Weapon->RegisterGuardEnd(this);
    Weapon = nullptr;
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
