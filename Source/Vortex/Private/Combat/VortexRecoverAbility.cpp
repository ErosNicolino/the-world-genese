#include "Combat/VortexRecoverAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Recover, "Ability.Recover");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Event_Recover, "Event.Ability.Recover");

UVortexRecoverEffect::UVortexRecoverEffect()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    FGameplayModifierInfo Heal;
    Heal.Attribute = UVortexAttributes::GetHealthAttribute();
    Heal.ModifierOp = EGameplayModOp::Additive;
    Heal.ModifierMagnitude = FScalableFloat(20.f);
    Modifiers.Add(Heal);
    FGameplayModifierInfo Cost;
    Cost.Attribute = UVortexAttributes::GetManaAttribute();
    Cost.ModifierOp = EGameplayModOp::Additive;
    Cost.ModifierMagnitude = FScalableFloat(-10.f);
    Modifiers.Add(Cost);
}

UVortexRecoverAbility::UVortexRecoverAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    SetAssetTags(FGameplayTagContainer(TAG_Ability_Recover));
}

bool UVortexRecoverAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;
    const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    return ASC && ASC->GetNumericAttribute(UVortexAttributes::GetManaAttribute()) >= 10.f &&
        ASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) > 0.f &&
        ASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) <
            ASC->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute());
}

void UVortexRecoverAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UVortexRecoverEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
    if (Spec.IsValid()) ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    FGameplayEventData Event;
    Event.EventTag = TAG_Event_Recover;
    Event.Instigator = ActorInfo->AvatarActor.Get();
    Event.Target = ActorInfo->AvatarActor.Get();
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(ActorInfo->AvatarActor.Get(), TAG_Event_Recover, Event);
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
