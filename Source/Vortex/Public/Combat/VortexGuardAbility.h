#pragma once

#include "Abilities/GameplayAbility.h"
#include "VortexGuardAbility.generated.h"

class UVortexWeaponComponent;

/** Held secondary sword ability: guard, with an initial perfect-parry window. */
UCLASS()
class VORTEX_API UVortexGuardAbility : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UVortexGuardAbility();
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    void Release();
private:
    UPROPERTY(Transient) TObjectPtr<UVortexWeaponComponent> Weapon;
};
