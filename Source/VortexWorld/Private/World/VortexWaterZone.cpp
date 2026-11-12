#include "World/VortexWaterZone.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"

AVortexWaterZone::AVortexWaterZone()
{
    Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
    Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bounds->SetBoxExtent(FVector(400.f, 200.f, 100.f));
    Bounds->ShapeColor = FColor(60, 140, 230);
    RootComponent = Bounds;
    bReplicates = false;          // os dois lados montam a mesma água a partir do mapa/gerador
}

void AVortexWaterZone::BeginPlay()
{
    Super::BeginPlay();
    if (UVortexWaterSubsystem* Water = GetWorld()->GetSubsystem<UVortexWaterSubsystem>())
    {
        Water->Register(this);
    }
}

void AVortexWaterZone::EndPlay(const EEndPlayReason::Type Reason)
{
    if (UVortexWaterSubsystem* Water = GetWorld() ? GetWorld()->GetSubsystem<UVortexWaterSubsystem>() : nullptr)
    {
        Water->Unregister(this);
    }
    Super::EndPlay(Reason);
}

float AVortexWaterZone::SurfaceZ() const
{
    return Bounds->GetComponentLocation().Z + Bounds->GetScaledBoxExtent().Z;
}

bool AVortexWaterZone::Contains(const FVector& Location) const
{
    const FVector Local = Bounds->GetComponentTransform().InverseTransformPositionNoScale(Location);
    const FVector Extent = Bounds->GetScaledBoxExtent();
    // acima da superfície também conta (para saber a altura da água sob o personagem)
    return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && Local.Z >= -Extent.Z;
}

bool UVortexWaterSubsystem::SurfaceAt(const FVector& Location, float& OutSurfaceZ) const
{
    bool bFound = false;
    for (const AVortexWaterZone* Zone : Zones)
    {
        if (Zone && Zone->Contains(Location))
        {
            OutSurfaceZ = bFound ? FMath::Max(OutSurfaceZ, Zone->SurfaceZ()) : Zone->SurfaceZ();
            bFound = true;
        }
    }
    if (!bFound && Provider)
    {
        bFound = Provider(Location, OutSurfaceZ);
    }
    return bFound;
}
