#pragma once

#include "Abilities/GameplayAbility.h"
#include "VortexSwordAbility.generated.h"

class UVortexWeaponComponent;

/** One server-authoritative activation owns the complete buffered sword combo. */
UCLASS()
class VORTEX_API UVortexSwordAbility : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UVortexSwordAbility();
    void BufferNext();

    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    void StartSwing(int32 Index);
    void OpenWindow();
    void SampleHit();
    void CloseWindow();
    void FinishSwing();

    UPROPERTY(Transient)
    TObjectPtr<UVortexWeaponComponent> Weapon;
    int32 SwingIndex = INDEX_NONE;
    float SwingStartTime = 0.f;
    bool bBuffered = false;
    bool bWindowOpen = false;
    TSet<TWeakObjectPtr<AActor>> HitActors;
    FTimerHandle OpenTimer, SampleTimer, CloseTimer, FinishTimer;
};
