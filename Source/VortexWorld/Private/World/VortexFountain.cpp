#include "World/VortexFountain.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "Sound/SoundBase.h"
#include "World/VortexTerrainStreamer.h"

namespace
{
    constexpr float Gravity = -980.f;
    constexpr int32 Jets = 8, DropsPerJet = 12, SplashPerJet = 5, Bubbles = 14;
    const FVector JetVelocityOut(180.f, 0.f, 130.f);   // para fora e para cima (cm/s)
    constexpr float NozzleRadius = 88.f, NozzleHeight = 217.f;

    // tempo até a gota do jato cair na água de baixo
    float JetFlight()
    {
        const float A = .5f * Gravity, B = JetVelocityOut.Z, C = NozzleHeight - AVortexFountain::WaterLow;
        return (-B - FMath::Sqrt(B * B - 4.f * A * C)) / (2.f * A);
    }
    FVector JetDir(int32 Jet) { const float A = Jet * UE_TWO_PI / Jets; return FVector(FMath::Cos(A), FMath::Sin(A), 0.f); }
}

AVortexFountain::AVortexFountain()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
}

AVortexFountain* AVortexFountain::Find(const UWorld* World)
{
    if (!World) return nullptr;
    if (TActorIterator<AVortexFountain> It(World); It) return *It;
    return nullptr;
}

FTransform AVortexFountain::RespawnPoint() const
{
    // ao sul da fonte (o lado do portão de entrada), olhando para ela
    const FVector At = GetActorLocation() + FVector(0.f, -380.f, 110.f);
    return FTransform(FRotator(0.f, 90.f, 0.f), At);
}

void AVortexFountain::BeginPlay()
{
    Super::BeginPlay();
    Build();
}

UStaticMeshComponent* AVortexFountain::Piece(const TCHAR* Shape, const TCHAR* Material, const FVector& At, const FVector& Size, float Yaw, bool bCollide)
{
    UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
    Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Shape, Shape)));
    // 11/11 (o usuário: "a fonte" combinando com a vila): a pedra usa as texturas do chão e das casas (kit da Quaternius);
    // só o bronze continua o do estádio
    const TCHAR* Stone = !FCString::Strcmp(Material, TEXT("bronze")) ? nullptr : !FCString::Strcmp(Material, TEXT("travertine")) ? TEXT("UnevenBrick") : TEXT("Brick");
    Part->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, Stone ? *FString::Printf(TEXT("/Game/Vortex/Town/Vila/MI_Vila_Q_%s.MI_Vila_Q_%s"), Stone, Stone)
        : *FString::Printf(TEXT("/Game/Vortex/Stadium/MI_Stadium_%s.MI_Stadium_%s"), Material, Material)));
    Part->SetupAttachment(Root);
    Part->SetRelativeLocationAndRotation(At, FRotator(0.f, Yaw, 0.f));
    Part->SetRelativeScale3D(Size / 100.f);
    Part->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
    Part->RegisterComponent();
    return Part;
}

void AVortexFountain::Build()
{
    // ---- pedra: degrau de basalto, bacia octogonal, borda de mármore -------------------------
    auto Octagon = [this](const TCHAR* Material, float Apothem, float Thickness, float Z0, float Height, bool bCollide)
    {
        const float Length = 2.f * (Apothem + Thickness * .5f) * FMath::Tan(FMath::DegreesToRadians(22.5f)) + 2.f;
        for (int32 i = 0; i < 8; ++i)
        {
            const float Yaw = 22.5f + i * 45.f;
            const FVector Out = FRotator(0.f, Yaw, 0.f).Vector();
            Piece(TEXT("Cube"), Material, Out * (Apothem + Thickness * .5f) + FVector(0.f, 0.f, Z0 + Height * .5f), FVector(Thickness, Length, Height), Yaw, bCollide);
        }
    };
    Octagon(TEXT("basalt"), 262.f, 50.f, -10.f, 25.f, true);                  // degrau
    Piece(TEXT("Cylinder"), TEXT("travertine"), FVector(0.f, 0.f, 10.f), FVector(480.f, 480.f, 20.f), 0.f, false);   // fundo
    Octagon(TEXT("travertine"), 232.f, 30.f, 0.f, 62.f, true);               // parede
    Octagon(TEXT("marble"), 226.f, 44.f, 62.f, 10.f, false);                 // borda
    // coluna com anéis de bronze
    Piece(TEXT("Cylinder"), TEXT("marble"), FVector(0.f, 0.f, 105.f), FVector(ColumnRadius * 2.f, ColumnRadius * 2.f, 190.f));
    Piece(TEXT("Cylinder"), TEXT("bronze"), FVector(0.f, 0.f, 55.f), FVector(140.f, 140.f, 8.f), 0.f, false);
    Piece(TEXT("Cylinder"), TEXT("bronze"), FVector(0.f, 0.f, 165.f), FVector(140.f, 140.f, 8.f), 0.f, false);
    // bacia de cima: taça (cone virado), fundo, borda em 16 pedras e oito bicos de bronze
    UStaticMeshComponent* Cup = Piece(TEXT("Cone"), TEXT("marble"), FVector(0.f, 0.f, 180.f), FVector(230.f, 230.f, 50.f), 0.f, false);
    Cup->SetRelativeRotation(FRotator(180.f, 0.f, 0.f));
    Piece(TEXT("Cylinder"), TEXT("marble"), FVector(0.f, 0.f, 205.f), FVector(UpperRadius * 2.f, UpperRadius * 2.f, 10.f), 0.f, false);
    for (int32 i = 0; i < 16; ++i)
    {
        const float Yaw = i * 22.5f;
        Piece(TEXT("Cube"), TEXT("marble_dark"), FRotator(0.f, Yaw, 0.f).Vector() * (UpperRadius + 6.f) + FVector(0.f, 0.f, 211.f), FVector(12.f, 44.f, 18.f), Yaw, false);
    }
    for (int32 Jet = 0; Jet < Jets; ++Jet)
    {
        const FVector Dir = JetDir(Jet);
        Piece(TEXT("Cube"), TEXT("bronze"), Dir * (NozzleRadius + 18.f) + FVector(0.f, 0.f, NozzleHeight + 4.f), FVector(16.f, 8.f, 8.f), Dir.Rotation().Yaw, false);
    }
    // cristal de jade no topo, girando devagar, com luz própria
    Piece(TEXT("Cylinder"), TEXT("marble_dark"), FVector(0.f, 0.f, 240.f), FVector(30.f, 30.f, 50.f), 0.f, false);
    Crystal = Piece(TEXT("Cube"), TEXT("marble"), FVector(0.f, 0.f, 292.f), FVector(38.f, 38.f, 38.f), 0.f, false);
    if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Stadium/M_StadiumGlow.M_StadiumGlow")))
    {
        UMaterialInstanceDynamic* Jade = UMaterialInstanceDynamic::Create(Glow, this);
        Jade->SetVectorParameterValue(TEXT("Color"), FLinearColor(.02f, .85f, .32f));
        Jade->SetScalarParameterValue(TEXT("Glow"), .75f);
        Crystal->SetMaterial(0, Jade);
        Crystal->SetCastShadow(false);
    }
    JadeLight = NewObject<UPointLightComponent>(this);
    JadeLight->SetMobility(EComponentMobility::Movable);
    JadeLight->SetupAttachment(Root);
    JadeLight->SetRelativeLocation(FVector(0.f, 0.f, 300.f));
    JadeLight->SetLightColor(FLinearColor(.35f, 1.f, .65f));
    JadeLight->SetIntensityUnits(ELightUnits::Candelas);
    JadeLight->SetIntensity(35.f);
    JadeLight->SetAttenuationRadius(700.f);
    JadeLight->SetCastShadows(false);
    JadeLight->RegisterComponent();

    // ---- água das duas bacias (a malha usa o UV = mundo / 10 m, como a água do terreno) -------
    Water = NewObject<UProceduralMeshComponent>(this);
    Water->SetupAttachment(Root);
    Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Water->SetCastShadow(false);
    Water->RegisterComponent();
    auto Disc = [this](int32 Section, float Radius, float Hole, float Z, bool bOctagon)
    {
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N; TArray<FVector2D> UV;
        constexpr int32 Seg = 48, Rings = 4;
        const FVector Here = GetActorLocation();
        for (int32 R = 0; R <= Rings; ++R)
        for (int32 S = 0; S < Seg; ++S)
        {
            const float A = S * UE_TWO_PI / Seg;
            FVector2D Dir(FMath::Cos(A), FMath::Sin(A));
            if (bOctagon)
            {
                // octógono com os lados alinhados às paredes (giradas 22,5°)
                const FVector2D Rot(FMath::Cos(A - UE_PI / 8.f), FMath::Sin(A - UE_PI / 8.f));
                Dir /= FMath::Max(FMath::Max(FMath::Abs(Rot.X), FMath::Abs(Rot.Y)), (FMath::Abs(Rot.X) + FMath::Abs(Rot.Y)) / 1.41421f);
            }
            const float Rad = FMath::Lerp(Hole, Radius, float(R) / Rings);
            const FVector P(Dir.X * Rad, Dir.Y * Rad, Z);
            V.Add(P);
            N.Add(FVector::UpVector);
            UV.Add(FVector2D((Here.X + P.X) / 1000.f, (Here.Y + P.Y) / 1000.f));
        }
        for (int32 R = 0; R < Rings; ++R)
        for (int32 S = 0; S < Seg; ++S)
        {
            const int32 A0 = R * Seg + S, A1 = R * Seg + (S + 1) % Seg, B0 = (R + 1) * Seg + S, B1 = (R + 1) * Seg + (S + 1) % Seg;
            T.Append({ A0, A1, B1, A0, B1, B0 });
        }
        Water->CreateMeshSection(Section, V, T, N, UV, {}, {}, false);
    };
    Disc(0, LowerRadius - 22.f, ColumnRadius - 1.f, WaterLow, true);
    Disc(1, UpperRadius - 2.f, 14.f, WaterHigh, false);
    RefreshWater();

    // ---- gotas: jatos, respingos e borbulhar (cubinhos de água, como no Godot) ----------------
    Drops = NewObject<UInstancedStaticMeshComponent>(this);
    Drops->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    // gotas que brilham de leve (azul-claro), visíveis de dia e à noite
    if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Stadium/M_StadiumGlow.M_StadiumGlow")))
    {
        UMaterialInstanceDynamic* DropMaterial = UMaterialInstanceDynamic::Create(Glow, this);
        DropMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(.5f, .82f, 1.f));
        DropMaterial->SetScalarParameterValue(TEXT("Glow"), .9f);
        Drops->SetMaterial(0, DropMaterial);
    }
    Drops->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Drops->SetCastShadow(false);
    Drops->SetupAttachment(Root);
    Drops->RegisterComponent();
    FRandomStream Rng(31);
    const float Flight = JetFlight();
    for (int32 Jet = 0; Jet < Jets; ++Jet)
    {
        const FVector Dir = JetDir(Jet);
        for (int32 i = 0; i < DropsPerJet; ++i)
        {
            FDrop D; D.Kind = 0; D.Jet = Jet; D.Phase = (i + Rng.FRand() * .5f) / DropsPerJet; D.Life = Flight;
            D.Start = Dir * NozzleRadius + FVector(0.f, 0.f, NozzleHeight);
            D.Velocity = Dir * JetVelocityOut.X + FVector(0.f, 0.f, JetVelocityOut.Z) + FVector(Rng.FRandRange(-8.f, 8.f), Rng.FRandRange(-8.f, 8.f), Rng.FRandRange(-6.f, 6.f));
            DropState.Add(D);
        }
        const FVector Land = Dir * (NozzleRadius + JetVelocityOut.X * Flight) + FVector(0.f, 0.f, WaterLow);
        for (int32 i = 0; i < SplashPerJet; ++i)
        {
            FDrop D; D.Kind = 1; D.Jet = Jet; D.Phase = Rng.FRand(); D.Life = Rng.FRandRange(.28f, .42f); D.Start = Land;
            const FVector Side = FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f).Vector();
            D.Velocity = Side * Rng.FRandRange(25.f, 60.f) + FVector(0.f, 0.f, Rng.FRandRange(90.f, 150.f));
            DropState.Add(D);
        }
    }
    for (int32 i = 0; i < Bubbles; ++i)
    {
        FDrop D; D.Kind = 2; D.Phase = Rng.FRand(); D.Life = Rng.FRandRange(.5f, .9f);
        const FVector Side = FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f).Vector();
        D.Start = Side * Rng.FRandRange(24.f, 80.f) + FVector(0.f, 0.f, WaterHigh);
        DropState.Add(D);
    }
    DropPoses.SetNum(DropState.Num());
    for (FTransform& Pose : DropPoses) Pose = FTransform::Identity;
    Drops->AddInstances(DropPoses, false);

    // ---- som da água ---------------------------------------------------------------------------
    if (USoundBase* Loop = LoadObject<USoundBase>(nullptr, TEXT("/Game/Free_Sounds_Pack/wav/Ambient_Water_Stream_Calm_3.Ambient_Water_Stream_Calm_3")))
    {
        Sound = NewObject<UAudioComponent>(this);
        Sound->SetupAttachment(Root);
        Sound->SetSound(Loop);
        Sound->bOverrideAttenuation = true;
        Sound->AttenuationOverrides.bAttenuate = true;
        Sound->AttenuationOverrides.AttenuationShapeExtents = FVector(250.f);
        Sound->AttenuationOverrides.FalloffDistance = 1600.f;
        Sound->SetVolumeMultiplier(.75f);
        Sound->RegisterComponent();
        Sound->Play();
    }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_FOUNTAIN montada: gotas=%d"), DropState.Num());
}

void AVortexFountain::RefreshWater()
{
    // a mesma água do terreno (muda com Opções → Qualidade da água)
    UMaterialInterface* Wanted = nullptr;
    for (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It; ++It) Wanted = It->GetWaterMaterial();
    if (!Wanted) Wanted = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_VortexWater.M_VortexWater"));
    if (Wanted && Wanted != CurrentWater && Water)
    {
        CurrentWater = Wanted;
        Water->SetMaterial(0, Wanted);
        Water->SetMaterial(1, Wanted);
    }
}

void AVortexFountain::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Clock += DeltaSeconds;
    if ((WaterCheck += DeltaSeconds) > 1.f) { WaterCheck = 0.f; RefreshWater(); }
    if (Crystal) Crystal->SetRelativeRotation(FRotator(35.f, Clock * 25.f, 45.f));
    if (JadeLight) JadeLight->SetIntensity(30.f + 8.f * FMath::Sin(Clock * 1.7f));
    // as gotas só andam com alguém perto (a 60 m)
    const APlayerController* Viewer = GetWorld()->GetFirstPlayerController();
    const FVector Eye = Viewer && Viewer->PlayerCameraManager ? Viewer->PlayerCameraManager->GetCameraLocation() : GetActorLocation();
    if (!Drops || FVector::DistSquared(Eye, GetActorLocation()) > FMath::Square(6000.f)) return;
    for (int32 i = 0; i < DropState.Num(); ++i)
    {
        const FDrop& D = DropState[i];
        const float T = FMath::Fmod(Clock + D.Phase * D.Life, D.Life);
        FVector P;
        float Size = .05f;
        if (D.Kind == 2)
        {
            P = D.Start + FVector(0.f, 0.f, 9.f * FMath::Sin(UE_PI * T / D.Life));
            Size = .035f * FMath::Sin(UE_PI * T / D.Life) + .005f;
        }
        else
        {
            P = D.Start + D.Velocity * T + FVector(0.f, 0.f, .5f * Gravity * T * T);
            if (D.Kind == 1) { P.Z = FMath::Max(P.Z, WaterLow); Size = .03f * (1.f - T / D.Life) + .008f; }
        }
        DropPoses[i] = FTransform(FRotator(0.f, i * 37.f, 0.f), P, FVector(Size));
    }
    Drops->BatchUpdateInstancesTransforms(0, DropPoses, false, true);
}
