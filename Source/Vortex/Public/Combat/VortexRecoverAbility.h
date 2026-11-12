#pragma once

#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "VortexRecoverAbility.generated.h"

/** Small server-authoritative GAS action used to exercise attributes, costs and events. */
UCLASS()
class VORTEX_API UVortexRecoverAbility : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UVortexRecoverAbility();
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

UCLASS()
class VORTEX_API UVortexRecoverEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UVortexRecoverEffect();
};
