#include "Framework/VortexCameraBoom.h"

// A volta suave depois de um obstáculo acontece só no COMPRIMENTO do braço, ao longo da linha que
// sai do herói: a câmera continua presa ao personagem. Antes a posição era interpolada no mundo,
// e com o herói correndo (ou teleportado) a câmera ficava para trás "solta", até ele parar.
FVector UVortexCameraBoom::BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation,
    bool bHitSomething, float DeltaTime)
{
    const FVector Origin = GetComponentLocation() + TargetOffset;
    const FVector ToDesired = DesiredArmLocation - Origin;
    const float DesiredLength = ToDesired.Size();
    if (DesiredLength < KINDA_SMALL_NUMBER)
    {
        bRecoveringFromObstacle = false;
        return DesiredArmLocation;
    }
    const FVector Direction = ToDesired / DesiredLength;
    if (!bSmoothRecoveryEnabled)
    {
        bRecoveringFromObstacle = false;
        CurrentLength = bHitSomething ? float(FVector::Dist(TraceHitLocation, Origin)) : DesiredLength;
        return bHitSomething ? TraceHitLocation : DesiredArmLocation;
    }
    if (bHitSomething)
    {
        // obstrução: resposta imediata, no ponto da batida
        CurrentLength = FMath::Min(float(FVector::Dist(TraceHitLocation, Origin)), DesiredLength);
        bRecoveringFromObstacle = true;
        return TraceHitLocation;
    }
    if (bRecoveringFromObstacle)
    {
        CurrentLength = FMath::FInterpTo(CurrentLength, DesiredLength, DeltaTime, 8.f);
        if (FMath::Abs(CurrentLength - DesiredLength) < 1.f)
        {
            CurrentLength = DesiredLength;
            bRecoveringFromObstacle = false;
        }
        return Origin + Direction * CurrentLength;
    }
    CurrentLength = DesiredLength;
    return DesiredArmLocation;
}
