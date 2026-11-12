#pragma once

#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "VortexAttributes.generated.h"

#define VORTEX_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/** Shared player/creature vitals: só vida e mana (o Vortex não tem vigor). The server owns base values; GAS replicates final values. */
UCLASS()
class VORTEX_API UVortexAttributes : public UAttributeSet
{
    GENERATED_BODY()
public:
    UVortexAttributes();

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="Vitals")
    FGameplayAttributeData Health;
    VORTEX_ATTRIBUTE_ACCESSORS(UVortexAttributes, Health)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="Vitals")
    FGameplayAttributeData MaxHealth;
    VORTEX_ATTRIBUTE_ACCESSORS(UVortexAttributes, MaxHealth)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Mana, Category="Vitals")
    FGameplayAttributeData Mana;
    VORTEX_ATTRIBUTE_ACCESSORS(UVortexAttributes, Mana)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxMana, Category="Vitals")
    FGameplayAttributeData MaxMana;
    VORTEX_ATTRIBUTE_ACCESSORS(UVortexAttributes, MaxMana)

    /** Comando /god (teste, servidor): a vida não desce. */
    bool bGodMode = false;

    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION() void OnRep_Health(const FGameplayAttributeData& OldValue);
    UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
    UFUNCTION() void OnRep_Mana(const FGameplayAttributeData& OldValue);
    UFUNCTION() void OnRep_MaxMana(const FGameplayAttributeData& OldValue);
};
