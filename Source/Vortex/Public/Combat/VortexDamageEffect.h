#pragma once

#include "GameplayEffect.h"
#include "VortexDamageEffect.generated.h"

/** Instant GAS damage. The magnitude is supplied by the server after validation. */
UCLASS()
class VORTEX_API UVortexDamageEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UVortexDamageEffect();
};
