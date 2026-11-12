#pragma once

#include "GameFramework/SpringArmComponent.h"
#include "VortexCameraBoom.generated.h"

/** Volta à distância normal aos poucos depois de um obstáculo (só no comprimento do braço), mantendo a resposta imediata à obstrução. */
UCLASS()
class VORTEX_API UVortexCameraBoom : public USpringArmComponent
{
    GENERATED_BODY()

public:
    void SetSmoothRecoveryEnabled(bool bEnabled) { bSmoothRecoveryEnabled = bEnabled; bRecoveringFromObstacle = false; }

protected:
    virtual FVector BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation,
        bool bHitSomething, float DeltaTime) override;

private:
    // comprimento atual do braço (cm), do herói até a câmera, durante a volta suave
    float CurrentLength = 0.f;
    bool bRecoveringFromObstacle = false;
    bool bSmoothRecoveryEnabled = true;
};
