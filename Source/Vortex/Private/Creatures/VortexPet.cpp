#include "Creatures/VortexPet.h"

#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

// 02/11: as partes em blocos (Tools/Blender/MakeDog.py): o meio do corpo a 25 cm do chão; a pata (16 cm) sai do
// ombro/quadril, a cabeça do pescoço e o rabo da garupa
namespace VortexPetArt
{
    constexpr float BodyHeight = 25.f;
}

AVortexPet::AVortexPet()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
}

void AVortexPet::BeginPlay()
{
    Super::BeginPlay();
    Seed = FMath::FRand() * 10.f;
    Yaw = GetActorRotation().Yaw;
    Build();
}

float AVortexPet::GroundAt(const FVector& Where) const
{
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    const float Ground = Terrain ? Terrain->GroundAt(Where.X, Where.Y) : Where.Z;
    return Ground > -299.f ? Ground : Where.Z;
}

USceneComponent* AVortexPet::Part(USceneComponent* Parent, const FVector& At, const TCHAR* PartName)
{
    USceneComponent* Pivot = NewObject<USceneComponent>(this);
    Pivot->SetupAttachment(Parent);
    Pivot->SetRelativeLocation(At);
    Pivot->RegisterComponent();
    const FString Path = FString::Printf(TEXT("/Game/Vortex/Creatures/Pet/Dog_%s/StaticMeshes/Dog_%s.Dog_%s"), PartName, PartName, PartName);
    if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path))
    {
        UStaticMeshComponent* Shape = NewObject<UStaticMeshComponent>(this);
        Shape->SetupAttachment(Pivot);
        Shape->SetStaticMesh(Mesh);
        Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Shape->RegisterComponent();
    }
    return Pivot;
}

void AVortexPet::Build()
{
    using namespace VortexPetArt;
    BodyPivot = Part(Root, FVector(0.f, 0.f, BodyHeight), TEXT("Body"));
    HeadPivot = Part(BodyPivot, FVector(15.f, 0.f, 8.f), TEXT("Head"));
    TailPivot = Part(BodyPivot, FVector(-16.f, 0.f, 12.f), TEXT("Tail"));
    // as patas, na ordem que o passo usa (0 e 3 juntas, 1 e 2 juntas)
    const FVector Hips[4] = { FVector(10.f, 8.f, -9.f), FVector(10.f, -8.f, -9.f), FVector(-10.f, 8.f, -9.f), FVector(-10.f, -8.f, -9.f) };
    for (const FVector& At : Hips) LegPivots.Add(Part(BodyPivot, At, TEXT("Leg")));
}

void AVortexPet::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Clock += DeltaSeconds;
    const AActor* Owner_ = Leader.Get();
    FVector Here = GetActorLocation();
    float WantSpeed = 0.f;
    FVector Facing = FRotator(0.f, Yaw, 0.f).Vector();
    if (Owner_)
    {
        // lugar ao lado (direita) e um pouco atrás do dono
        const FVector Goal = Owner_->GetActorLocation() + Owner_->GetActorRightVector() * 75.f - Owner_->GetActorForwardVector() * 45.f;
        const FVector To = FVector(Goal.X - Here.X, Goal.Y - Here.Y, 0.f);
        const float Distance = To.Size();
        if (Distance > 2500.f) Here = FVector(Goal.X, Goal.Y, Here.Z);                    // ficou muito para trás: alcança
        else if (Distance > 35.f)
        {
            WantSpeed = Distance > 380.f ? 430.f : FMath::Clamp(Distance * 2.2f, 90.f, 260.f);
            Facing = To.GetSafeNormal();
        }
        else Facing = (Owner_->GetActorLocation() - Here).GetSafeNormal2D();             // parado: olha para ela
    }
    Speed = FMath::FInterpTo(Speed, WantSpeed, DeltaSeconds, 6.f);
    const float WantYaw = Facing.IsNearlyZero() ? Yaw : Facing.Rotation().Yaw;
    Yaw = FMath::FixedTurn(Yaw, WantYaw, (Speed > 20.f ? 420.f : 160.f) * DeltaSeconds);
    Here += FRotator(0.f, Yaw, 0.f).Vector() * Speed * DeltaSeconds;
    Here.Z = FMath::FInterpTo(Here.Z, GroundAt(Here), DeltaSeconds, 14.f);
    SetActorLocationAndRotation(Here, FRotator(0.f, Yaw, 0.f));

    // passo: trote pela velocidade (diagonais juntas), lombo que balança
    const float Move = FMath::Clamp(Speed / 260.f, 0.f, 1.6f);
    Gait += DeltaSeconds * (4.f + 9.f * Move);
    const float Swing = 34.f * FMath::Min(Move, 1.f);
    for (int32 i = 0; i < LegPivots.Num(); ++i)
    {
        const bool bDiagonal = i == 0 || i == 3;
        LegPivots[i]->SetRelativeRotation(FRotator(FMath::Sin(Gait + (bDiagonal ? 0.f : PI)) * Swing, 0.f, 0.f));
    }
    BodyPivot->SetRelativeLocation(FVector(0.f, 0.f, VortexPetArt::BodyHeight + FMath::Abs(FMath::Sin(Gait)) * 2.2f * FMath::Min(Move, 1.f)));
    BodyPivot->SetRelativeRotation(FRotator(FMath::Sin(Gait * 2.f) * 2.f * Move, 0.f, 0.f));
    // rabo abanando (mais rápido parado e feliz), cabeça que inclina de vez em quando
    TailPivot->SetRelativeRotation(FRotator(0.f, FMath::Sin(Clock * (Move > .1f ? 10.f : 15.f) + Seed) * 32.f, 0.f));
    float Tilt = 0.f;
    if (Speed < 30.f)
    {
        TiltAt -= DeltaSeconds;
        if (TiltAt < -1.2f) TiltAt = FMath::FRandRange(2.5f, 6.f);
        if (TiltAt < 0.f) Tilt = FMath::Sin(FMath::Clamp(-TiltAt / 1.2f, 0.f, 1.f) * PI) * 18.f;
    }
    HeadPivot->SetRelativeRotation(FRotator(FMath::Sin(Gait * 2.f) * 4.f * Move - 4.f * (1.f - FMath::Min(Move, 1.f)), 0.f, Tilt));
}
