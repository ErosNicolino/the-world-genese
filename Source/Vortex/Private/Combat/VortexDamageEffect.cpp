#include "Combat/VortexDamageEffect.h"

#include "Combat/VortexAttributes.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Data_Damage, "Data.Damage");

UVortexDamageEffect::UVortexDamageEffect()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    FGameplayModifierInfo Modifier;
    Modifier.Attribute = UVortexAttributes::GetHealthAttribute();
    Modifier.ModifierOp = EGameplayModOp::Additive;
    FSetByCallerFloat Magnitude;
    Magnitude.DataTag = TAG_Data_Damage;
    Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
    Modifiers.Add(Modifier);
}
