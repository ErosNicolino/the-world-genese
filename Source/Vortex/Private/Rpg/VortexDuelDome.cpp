#include "Rpg/VortexDuelDome.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AVortexDuelDome::AVortexDuelDome()
{
    bReplicates = true;
    SetReplicateMovement(true);
    bAlwaysRelevant = true;
    PrimaryActorTick.bCanEverTick = false;
    Shell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shell"));
    RootComponent = Shell;
    Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Shell->SetCastShadow(false);
    // a esfera da engine tem 50 cm de raio; o meio fica no chão (a metade de baixo some na terra)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded()) Shell->SetStaticMesh(Sphere.Object);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Energy(TEXT("/Game/Vortex/World/Duel/M_DuelDome.M_DuelDome"));
    if (Energy.Succeeded()) Shell->SetMaterial(0, Energy.Object);
    Shell->SetWorldScale3D(FVector(Radius / 50.f));
}

bool AVortexDuelDome::Contains(const FVector& At, float Margin) const
{
    return FVector::Dist2D(At, GetActorLocation()) <= Radius + Margin;
}

AVortexDuelDome* AVortexDuelDome::Containing(const UWorld* World, const FVector& At, float Margin)
{
    if (!World) return nullptr;
    for (TActorIterator<AVortexDuelDome> It(World); It; ++It)
        if (It->Contains(At, Margin)) return *It;
    return nullptr;
}
