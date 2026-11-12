#include "Combat/VortexAttributes.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UVortexAttributes::UVortexAttributes()
{
    InitHealth(100.f);
    InitMaxHealth(100.f);
    InitMana(50.f);
    InitMaxMana(50.f);
}

void UVortexAttributes::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    if (Attribute == GetHealthAttribute()) NewValue = bGodMode ? GetMaxHealth() : FMath::Clamp(NewValue, 0.f, GetMaxHealth());
    else if (Attribute == GetManaAttribute()) NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
    else if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxManaAttribute())
        NewValue = FMath::Max(1.f, NewValue);
}

void UVortexAttributes::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
    if (Data.EvaluatedData.Attribute == GetHealthAttribute()) SetHealth(bGodMode ? GetMaxHealth() : FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
    else if (Data.EvaluatedData.Attribute == GetManaAttribute()) SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
}

void UVortexAttributes::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION_NOTIFY(UVortexAttributes, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UVortexAttributes, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UVortexAttributes, Mana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UVortexAttributes, MaxMana, COND_None, REPNOTIFY_Always);
}

void UVortexAttributes::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UVortexAttributes, Health, OldValue); }
void UVortexAttributes::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UVortexAttributes, MaxHealth, OldValue); }
void UVortexAttributes::OnRep_Mana(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UVortexAttributes, Mana, OldValue); }
void UVortexAttributes::OnRep_MaxMana(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UVortexAttributes, MaxMana, OldValue); }

