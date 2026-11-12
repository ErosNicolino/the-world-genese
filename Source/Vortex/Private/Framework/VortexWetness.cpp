#include "Framework/VortexWetness.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

namespace
{
    constexpr float DrySeconds = 45.f;
}

UVortexWetness::UVortexWetness()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = .1f;
}

void UVortexWetness::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    const ACharacter* Hero = Cast<ACharacter>(GetOwner());
    if (!Hero || GetNetMode() == NM_DedicatedServer) return;
    const float Half = Hero->GetCapsuleComponent() ? Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
    const FVector Feet = Hero->GetActorLocation() - FVector(0.f, 0.f, Half);
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    float Surface = 0.f;
    const bool bInWater = Terrain && Terrain->IsWaterAt(Feet.X, Feet.Y, Surface) && Feet.Z < Surface;
    if (bInWater)
    {
        // a água sobe pelo corpo até onde chegou (um pouco acima, pelo respingo da caminhada)
        const float Reached = Surface - Feet.Z + 8.f;
        WetHeight = Wetness > .25f ? FMath::Max(WetHeight, Reached) : Reached;
        Wetness = 1.f;
    }
    else Wetness = FMath::Max(0.f, Wetness - DeltaTime / DrySeconds);
    // as primeiras partes secam primeiro: a linha baixa devagar enquanto seca
    const float Line = Feet.Z + WetHeight * FMath::Lerp(.75f, 1.f, Wetness);
    if (FMath::Abs(Line - SentLine) < 1.f && FMath::Abs(Wetness - SentWetness) < .01f) return;
    SentLine = Line;
    SentWetness = Wetness;
    TArray<USkeletalMeshComponent*> Parts;
    GetOwner()->GetComponents(Parts);
    for (USkeletalMeshComponent* Part : Parts)
    {
        Part->SetCustomPrimitiveDataFloat(30, Line);
        Part->SetCustomPrimitiveDataFloat(31, Wetness);
    }
}
