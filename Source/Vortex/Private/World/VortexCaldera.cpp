#include "World/VortexCaldera.h"

#include "Framework/VortexProbe.h"
#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexHitFx.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Creatures/VortexCreature.h"
#include "Creatures/VortexMonster.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProceduralMeshComponent.h"
#include "World/VortexLandmarks.h"
#include "Rpg/VortexHeroSheet.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexTerrainData.h"

namespace
{
    const TCHAR* LavaMaterial = TEXT("/Game/Vortex/Combat/M_LavaToon.M_LavaToon");
    const TCHAR* LavaSmoke = TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Floor_01_Smoke.NS_Fire_Floor_01_Smoke");
    const TCHAR* CrustMaterial = TEXT("/Game/Vortex/Creatures/RuneColossus/Materials/M_AzhurRock.M_AzhurRock");
    const TCHAR* FloorFire = TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Floor_01_Simple.NS_Fire_Floor_01_Simple");

    AVortexTerrainStreamer* StreamerOf(UWorld* World)
    {
        if (TActorIterator<AVortexTerrainStreamer> It(World); It) return *It;
        return nullptr;
    }
}

// ---- subsistema ------------------------------------------------------------------------------------

bool UVortexCalderaSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

float UVortexCalderaSubsystem::RedSkyAt(const FVector& Where) const
{
    const float Distance = FVector::Dist2D(Where, FVector(CenterX, CenterY, 0.f));
    const float Zone = 1.f - FMath::SmoothStep(ZoneInner, ZoneOuter, Distance);
    // dormindo, um vermelho de brasa; acordado e nas fases finais, o céu pega fogo
    return Zone * (BossPhase >= 3 ? 1.f : BossPhase == 2 ? .9f : BossPhase == 1 ? .8f : .65f);
}

void UVortexCalderaSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !StreamerOf(World)) return;
    const FVector Center(CenterX, CenterY, 0.f);
    // a arena: existe a partir do momento em que alguém chega perto (o servidor fere quem pisa)
    if (!Arena.IsValid())
    {
        bool bNear = false;
        for (TActorIterator<AVortexCharacter> It(World); It && !bNear; ++It) bNear = FVector::Dist2D(It->GetActorLocation(), Center) < 12000.f;
        if (bNear)
        {
            FActorSpawnParameters Params;
            Params.ObjectFlags |= RF_Transient;
            Arena = World->SpawnActor<AVortexLavaArena>(Center, FRotator::ZeroRotator, Params);
        }
    }
    // a fase do chefe acordado perto (escurece a zona)
    int32 Phase = 0;
    for (TActorIterator<AVortexMonster> It(World); It; ++It)
    {
        if (It->IsBoss() && !It->IsCreatureDead() && It->IsEngaged() && FVector::Dist2D(It->GetActorLocation(), Center) < 8000.f) Phase = FMath::Max(Phase, It->GetBossPhase());
    }
    BossPhase = Phase;
    // brasas e cinzas em volta da câmera de quem joga aqui
    if (World->GetNetMode() == NM_DedicatedServer) return;
    APlayerController* Local = World->GetFirstPlayerController();
    if (!Local || !Local->PlayerCameraManager) return;
    const FVector Eye = Local->PlayerCameraManager->GetCameraLocation();
    const float Red = RedSkyAt(Eye);
    if (Red > .02f && !Embers.IsValid())
    {
        FActorSpawnParameters Params;
        Params.ObjectFlags |= RF_Transient;
        Embers = World->SpawnActor<AVortexEmbers>(Eye, FRotator::ZeroRotator, Params);
    }
    if (AVortexEmbers* Bits = Embers.Get())
    {
        Bits->Strength = Red;
        Bits->SetActorLocation(Eye);
    }
}

void UVortexCalderaSubsystem::AddPool(const FVector& At, float Radius, float Life)
{
    if (!Arena.IsValid())
    {
        FActorSpawnParameters Params;
        Params.ObjectFlags |= RF_Transient;
        Arena = GetWorld()->SpawnActor<AVortexLavaArena>(FVector(CenterX, CenterY, 0.f), FRotator::ZeroRotator, Params);
    }
    if (Arena.IsValid()) Arena->AddPool(At, Radius, Life);
}

// ---- arena de lava ---------------------------------------------------------------------------------

AVortexLavaArena::AVortexLavaArena()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AVortexLavaArena::Seed()
{
    // poças fixas: espalhadas no anel em volta do posto do chefe (o miolo, onde ele luta, fica livre)
    bSeeded = true;
    FRandomStream Rng(4242);
    // 02/11: 8 poças (eram 18)
    for (int32 i = 0; i < 8; ++i)
    {
        FPool Pool;
        const float Angle = Rng.FRandRange(0.f, 2.f * UE_PI);
        const float Distance = Rng.FRandRange(1100.f, 3900.f);
        Pool.At = FVector2D(UVortexCalderaSubsystem::CenterX, UVortexCalderaSubsystem::CenterY) + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance;
        Pool.Radius = Rng.FRandRange(150.f, 360.f);
        Pools.Add(Pool);
    }
}

void AVortexLavaArena::AddPool(const FVector& At, float Radius, float Life)
{
    FPool Pool;
    Pool.At = FVector2D(At);
    Pool.Radius = Radius;
    Pool.Life = Life;
    Pool.Ground = At.Z;
    Pools.Add(Pool);
}

void AVortexLavaArena::Place(FPool& Pool, float Ground)
{
    Pool.Ground = Ground;
    Pool.bPlaced = true;
    if (GetNetMode() == NM_DedicatedServer) return;
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UMaterialInterface* Lava = LoadObject<UMaterialInterface>(nullptr, LavaMaterial);
    UMaterialInterface* Crust = LoadObject<UMaterialInterface>(nullptr, CrustMaterial);
    FRandomStream Rng(int32(Pool.At.X * 3.f + Pool.At.Y));
    // acima do chão de basalto da cratera (que fica até ~40 cm acima do terreno de degraus)
    const FVector Base(Pool.At.X, Pool.At.Y, Ground + 45.f);
    // a poça: uma mancha irregular (o contorno ondula), com o miolo quente (cor R = 1) e a borda de crosta
    // (R = 0) que passa um pouco do raio de dano; a lava corre numa direção própria
    {
        constexpr int32 Sides = 56;
        const float S1 = Rng.FRandRange(0.f, 6.28f), S2 = Rng.FRandRange(0.f, 6.28f), S3 = Rng.FRandRange(0.f, 6.28f);
        auto Edge = [&](float A) { return 1.f + .16f * FMath::Sin(3.f * A + S1) + .09f * FMath::Sin(5.f * A + S2) + .05f * FMath::Sin(9.f * A + S3); };
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N; TArray<FLinearColor> Col;
        V.Add(FVector::ZeroVector); N.Add(FVector::UpVector); Col.Add(FLinearColor(1.f, 0.f, 0.f));
        const float Rings[3] = { .55f, .9f, 1.14f };
        const float Heat[3] = { .85f, .45f, 0.f };
        for (int32 r = 0; r < 3; ++r)
            for (int32 k = 0; k < Sides; ++k)
            {
                const float A = 2.f * UE_PI * k / Sides;
                const float R = Pool.Radius * Rings[r] * Edge(A);
                // a crosta da borda fica um tico abaixo (assenta no chão)
                V.Add(FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, r == 2 ? -1.5f : 0.f));
                N.Add(FVector::UpVector);
                Col.Add(FLinearColor(Heat[r], 0.f, 0.f));
            }
        for (int32 k = 0; k < Sides; ++k)
        {
            const int32 K1 = (k + 1) % Sides;
            T.Append({ 0, 1 + K1, 1 + k });
            for (int32 r = 0; r < 2; ++r)
            {
                const int32 A = 1 + r * Sides + k, B = 1 + r * Sides + K1, Cc = 1 + (r + 1) * Sides + k, D = 1 + (r + 1) * Sides + K1;
                T.Append({ A, B, Cc, B, D, Cc });
            }
        }
        UProceduralMeshComponent* Blob = NewObject<UProceduralMeshComponent>(this);
        Blob->SetupAttachment(RootComponent);
        Blob->SetUsingAbsoluteLocation(true);
        Blob->SetWorldLocation(Base);
        Blob->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Blob->SetCastShadow(false);
        Blob->RegisterComponent();
        Blob->CreateMeshSection_LinearColor(0, V, T, N, {}, Col, {}, false);
        if (Lava)
        {
            UMaterialInstanceDynamic* Flow = UMaterialInstanceDynamic::Create(Lava, Blob);
            const float Dir = Rng.FRandRange(0.f, 2.f * UE_PI);
            Flow->SetScalarParameterValue(TEXT("FlowX"), FMath::Cos(Dir));
            Flow->SetScalarParameterValue(TEXT("FlowY"), FMath::Sin(Dir));
            Flow->SetScalarParameterValue(TEXT("Speed"), Rng.FRandRange(.8f, 1.3f));
            Blob->SetMaterial(0, Flow);
        }
        Pool.Parts.Add(Blob);
    }
    // pedras de basalto meio enterradas na borda
    const int32 Rocks = 5 + int32(Pool.Radius / 70.f);
    for (int32 k = 0; k < Rocks; ++k)
    {
        const float A = 2.f * UE_PI * k / Rocks + Rng.FRandRange(-.25f, .25f);
        const float Size = Rng.FRandRange(.3f, .7f);
        UStaticMeshComponent* Rock = NewObject<UStaticMeshComponent>(this);
        Rock->SetupAttachment(RootComponent);
        Rock->SetStaticMesh(Cube);
        if (Crust) Rock->SetMaterial(0, Crust);
        Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Rock->SetCastShadow(false);
        Rock->SetWorldLocationAndRotation(Base + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * Pool.Radius * Rng.FRandRange(1.08f, 1.24f) + FVector(0, 0, Size * 18.f - 12.f),
            FRotator(Rng.FRandRange(-14.f, 14.f), Rng.FRandRange(0.f, 90.f), Rng.FRandRange(-8.f, 8.f)));
        Rock->SetWorldScale3D(FVector(Size, Size * Rng.FRandRange(.7f, 1.3f), Size * .7f));
        Rock->RegisterComponent();
        Pool.Parts.Add(Rock);
    }
    // fumaça subindo (no lugar da chama pequena) e o brilho alaranjado no chão em volta
    {
        FActorSpawnParameters Params;
        Params.ObjectFlags |= RF_Transient;
        if (AVortexSmokeColumn* Column = GetWorld()->SpawnActor<AVortexSmokeColumn>(Base + FVector(0, 0, 20.f), FRotator::ZeroRotator, Params))
        {
            Column->Setup(Pool.Radius * .7f, 900.f + Pool.Radius * 2.f, 6, FLinearColor(.24f, .2f, .19f), .32f, 6.f);
            Column->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Pool.Smoke = Column;
        }
    }
    if (Pool.Radius > 220.f || Pool.Life > 0.f)
    {
        UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
        Light->SetupAttachment(RootComponent);
        Light->SetWorldLocation(Base + FVector(0, 0, 110.f));
        Light->SetLightColor(FLinearColor(1.f, .38f, .08f));
        Light->SetIntensity(Pool.Life > 0.f ? 3000.f : 4000.f);
        Light->SetAttenuationRadius(Pool.Radius * 3.f);
        Light->SetCastShadows(false);
        Light->RegisterComponent();
        Pool.Parts.Add(Light);
    }
}

void AVortexLavaArena::Remove(FPool& Pool)
{
    for (const TWeakObjectPtr<USceneComponent>& Part : Pool.Parts) if (Part.IsValid()) Part->DestroyComponent();
    Pool.Parts.Reset();
    if (Pool.Fire.IsValid()) Pool.Fire->DestroyComponent();
    if (Pool.Smoke.IsValid()) Pool.Smoke->Destroy();
}

bool AVortexLavaArena::InLava(const FVector& Feet) const
{
    for (const FPool& Pool : Pools)
    {
        if (!Pool.bPlaced) continue;
        const float Shrink = Pool.Life > 0.f ? FMath::Clamp((Pool.Life - Pool.Age) / 1.f, 0.f, 1.f) : 1.f;
        if (FVector2D::Distance(Pool.At, FVector2D(Feet)) < Pool.Radius * .92f * Shrink && FMath::Abs(Feet.Z - Pool.Ground) < 70.f) return true;
    }
    return false;
}

void AVortexLavaArena::BuildCrater()
{
    // com as mesmas peças de pedra do chefe (M_AzhurRock, M_AzhurObsidian, M_AzhurLava e M_AzhurCore): o lugar
    // combina com ele. Mesma semente em todas as máquinas: os paredões (com colisão) ficam no mesmo lugar para todos
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data.IsValid()) return;
    bCraterBuilt = true;
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    const TCHAR* Folder = TEXT("/Game/Vortex/Creatures/RuneColossus/Materials/");
    auto Mat = [Folder](const TCHAR* Name) { return LoadObject<UMaterialInterface>(nullptr, *(FString(Folder) + Name + TEXT(".") + Name)); };
    const bool bVisual = GetNetMode() != NM_DedicatedServer;
    auto Set = [this, Cube](const TCHAR* Name, UMaterialInterface* Material, bool bSolid)
    {
        UInstancedStaticMeshComponent* Ism = NewObject<UInstancedStaticMeshComponent>(this, Name);
        Ism->SetupAttachment(RootComponent);
        Ism->SetUsingAbsoluteLocation(true);
        Ism->SetUsingAbsoluteRotation(true);
        Ism->SetStaticMesh(Cube);
        if (Material) Ism->SetMaterial(0, Material);
        Ism->SetCollisionEnabled(bSolid ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        if (bSolid) Ism->SetCollisionResponseToAllChannels(ECR_Block);
        Ism->SetCastShadow(bSolid);
        Ism->RegisterComponent();
        return Ism;
    };
    // o basalto da Poly Haven (Tools/BuildCalderaMaterials.py): o chão inteiro e os paredões
    UMaterialInterface* WallStone = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/World/Caldeira/M_CalderaWall.M_CalderaWall"));
    UMaterialInterface* GroundStone = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/World/Caldeira/M_CalderaGround.M_CalderaGround"));
    UInstancedStaticMeshComponent* Walls = Set(TEXT("CraterWalls"), WallStone ? WallStone : Mat(TEXT("M_AzhurRock")), true);
    UInstancedStaticMeshComponent* Spikes = Set(TEXT("CraterSpikes"), Mat(TEXT("M_AzhurObsidian")), true);
    UInstancedStaticMeshComponent* Veins = bVisual ? Set(TEXT("CraterVeins"), Mat(TEXT("M_AzhurLava")), false) : nullptr;
    UInstancedStaticMeshComponent* Barrier = Set(TEXT("CraterBarrier"), nullptr, true);
    Barrier->SetVisibility(false);
    Barrier->SetHiddenInGame(true);
    const FVector2D C(UVortexCalderaSubsystem::CenterX, UVortexCalderaSubsystem::CenterY);
    FRandomStream Rng(7331);
    const auto Ground = [&Data](float X, float Y) { return Data->GroundAt(X, Y); };
    const auto Box = [](UInstancedStaticMeshComponent* Ism, const FVector& At, const FRotator& Rot, const FVector& SizeCm)
    {
        if (Ism) Ism->AddInstance(FTransform(Rot, At, SizeCm / 100.f), true);
    };
    const auto NearPool = [this](const FVector2D& P, float Pad)
    {
        for (const FPool& Pool : Pools) if (FVector2D::Distance(Pool.At, P) < Pool.Radius + Pad) return true;
        return false;
    };
    // as duas entradas: a do norte (de onde se chega da cidade) e a do sul
    const float Gates[2] = { .5f * UE_PI, 1.5f * UE_PI };
    const auto GateAt = [&Gates](float Angle)
    {
        float Best = 10.f;
        for (const float G : Gates) Best = FMath::Min(Best, FMath::Abs(FMath::FindDeltaAngleRadians(Angle, G)));
        return Best;
    };
    // ---- as rachaduras de lava: saem do Selo (no meio) e se ramificam até a borda
    struct FCrack { FVector2D A, B; float Width; };
    TArray<FCrack> CrackLines;
    // 02/11: 7 rachaduras finas (eram 12 largas, com galhos: o chão inteiro brilhava e a luta ficava ilegível)
    for (int32 k = 0; k < 7; ++k)
    {
        float Angle = 2.f * UE_PI * k / 7.f + Rng.FRandRange(-.2f, .2f);
        float R = Rng.FRandRange(300.f, 600.f);
        const float End = Rng.FRandRange(3200.f, 4400.f);
        while (R < End)
        {
            const float Step = Rng.FRandRange(160.f, 260.f);
            const FVector2D A = C + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * R;
            Angle += Rng.FRandRange(-.16f, .16f);
            const FVector2D B = C + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (R + Step);
            CrackLines.Add({ A, B, FMath::Lerp(55.f, 18.f, R / End) });
            // galhos finos de vez em quando
            if (Rng.FRand() < .08f)
            {
                const float Side = Angle + (Rng.FRand() < .5f ? 1.f : -1.f) * Rng.FRandRange(.5f, 1.f);
                const FVector2D Tip = B + FVector2D(FMath::Cos(Side), FMath::Sin(Side)) * Rng.FRandRange(250.f, 600.f);
                CrackLines.Add({ B, Tip, 14.f });
            }
            R += Step;
        }
    }
    // ---- o chão: uma malha só de basalto (sem emendas), no relevo, até debaixo dos paredões; o calor (cor R) é a
    // distância até a rachadura mais perto; nas poças a malha afunda para a lava delas aparecer
    if (bVisual && GroundStone)
    {
        constexpr float Step = 75.f, Reach = 5000.f;   // o tamanho da célula do terreno
        const int32 N = int32(2.f * Reach / Step) + 1;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> Nrm; TArray<FLinearColor> Col; TArray<FVector2D> UV;
        V.Reserve(N * N);
        TArray<int32> Index;
        Index.Init(INDEX_NONE, N * N);
        for (int32 j = 0; j < N; ++j)
            for (int32 i = 0; i < N; ++i)
            {
                const FVector2D P = C + FVector2D(-Reach + i * Step, -Reach + j * Step);
                if (FVector2D::Distance(P, C) > Reach) continue;
                float Heat = 0.f;
                for (const FCrack& K : CrackLines)
                {
                    const FVector2D AB = K.B - K.A;
                    const float T01 = FMath::Clamp(FVector2D::DotProduct(P - K.A, AB) / FMath::Max(1.f, AB.SizeSquared()), 0.f, 1.f);
                    const float D = FVector2D::Distance(P, K.A + AB * T01);
                    Heat = FMath::Max(Heat, 1.f - D / K.Width);
                }
                // o terreno é de degraus (uma altura por célula): o ponto fica na maior altura das células em volta,
                // senão o degrau mais alto furava a malha entre dois pontos (os quadrados de xadrez)
                float Top = Ground(P.X, P.Y);
                for (const FVector2D& O : { FVector2D(-50.f, -50.f), FVector2D(50.f, -50.f), FVector2D(-50.f, 50.f), FVector2D(50.f, 50.f) })
                    Top = FMath::Max(Top, Ground(P.X + O.X, P.Y + O.Y));
                float Z = Top + 12.f;
                Index[j * N + i] = V.Num();
                V.Add(FVector(P.X, P.Y, Z) - FVector(C, 0.f));
                Nrm.Add(FVector::UpVector);
                Col.Add(FLinearColor(FMath::Clamp(Heat, 0.f, 1.f), 0.f, 0.f));
                UV.Add(P / 500.f);
            }
        for (int32 j = 0; j + 1 < N; ++j)
            for (int32 i = 0; i + 1 < N; ++i)
            {
                const int32 A = Index[j * N + i], B = Index[j * N + i + 1], Cc = Index[(j + 1) * N + i], D = Index[(j + 1) * N + i + 1];
                if (A < 0 || B < 0 || Cc < 0 || D < 0) continue;
                T.Append({ A, Cc, B, B, Cc, D });
            }
        UProceduralMeshComponent* Ground3 = NewObject<UProceduralMeshComponent>(this, TEXT("CraterFloor"));
        Ground3->SetupAttachment(RootComponent);
        Ground3->SetUsingAbsoluteLocation(true);
        Ground3->SetWorldLocation(FVector(C, 0.f));
        Ground3->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Ground3->RegisterComponent();
        Ground3->CreateMeshSection_LinearColor(0, V, T, Nrm, UV, Col, {}, false);
        Ground3->SetMaterial(0, GroundStone);
        UE_LOG(LogTemp, Display, TEXT("VORTEX_CALDERA chão: %d vértices, %d rachaduras"), V.Num(), CrackLines.Num());
    }
    // ---- os paredões: blocos de basalto em volta, de 7 a 17 m, com as duas entradas baixas; veios de lava na face
    // de dentro e espinhos de obsidiana em cima
    constexpr int32 Columns = 118;
    for (int32 k = 0; k < Columns; ++k)
    {
        const float Angle = 2.f * UE_PI * k / Columns;
        const float Gate = GateAt(Angle);
        if (Gate < .16f) continue;   // o vão da entrada (as laterais dele sobem logo a toda a altura)
        const float Ramp = FMath::Clamp((Gate - .16f) / .12f, 0.f, 1.f);
        const float Wave = .5f + .3f * FMath::Sin(Angle * 3.f + 1.3f) + .2f * FMath::Sin(Angle * 7.f + .4f);
        // de 13 a 23 m, sem degrau: ninguém sobe pela borda, só se entra pelas duas entradas
        const float Height = FMath::Lerp(1300.f, 1300.f + 1000.f * Wave, Ramp) + Rng.FRandRange(-100.f, 150.f);
        const float R = UVortexCalderaSubsystem::RimRadius + Rng.FRandRange(-180.f, 220.f);
        const FVector2D P = C + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * R;
        const float Z = FMath::Min(Ground(P.X, P.Y), Ground(P.X - FMath::Cos(Angle) * 400.f, P.Y - FMath::Sin(Angle) * 400.f)) - 250.f;
        const float Yaw = FMath::RadiansToDegrees(Angle);
        const float Tilt = Rng.FRandRange(-7.f, -3.f);    // a parede de dentro debruça sobre a arena (não dá apoio)
        Box(Walls, FVector(P, Z + Height * .5f + 125.f), FRotator(Tilt, Yaw, Rng.FRandRange(-4.f, 4.f)),
            FVector(Rng.FRandRange(700.f, 1000.f), 2.f * UE_PI * R / Columns * 1.25f, Height + 250.f));
        // barreira invisível acima do paredão (nem pulando por cima)
        Box(Barrier, FVector(P, Z + Height + 1700.f), FRotator(0.f, Yaw, 0.f), FVector(900.f, 2.f * UE_PI * R / Columns * 1.3f, 3000.f));
        if (Ramp > .9f && Rng.FRand() < .12f)
        {
            const FVector2D Face = C + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (R - 470.f);
            Box(Veins, FVector(Face, Z + Height * .45f + 200.f), FRotator(0.f, Yaw, Rng.FRandRange(-10.f, 10.f)), FVector(30.f, 45.f, Height * .7f));
        }
        if (Ramp > .9f && Rng.FRand() < .45f)
            Box(Spikes, FVector(P, Z + Height + 300.f), FRotator(Rng.FRandRange(-20.f, 20.f), Rng.FRandRange(0.f, 90.f), Rng.FRandRange(-25.f, 5.f)),
                FVector(110.f, 110.f, Rng.FRandRange(350.f, 650.f)));
    }
    // ---- agulhas de obsidiana espalhadas no chão (longe do meio, onde ele luta)
    for (int32 k = 0; k < 13; ++k)
    {
        const float Angle = Rng.FRandRange(0.f, 2.f * UE_PI);
        const FVector2D P = C + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Rng.FRandRange(2700.f, 4100.f);
        if (NearPool(P, 120.f) || GateAt(Angle) < .25f) continue;
        const float H = Rng.FRandRange(500.f, 1100.f);
        Box(Spikes, FVector(P, Ground(P.X, P.Y) + H * .4f), FRotator(Rng.FRandRange(-18.f, 18.f), Rng.FRandRange(0.f, 90.f), Rng.FRandRange(-18.f, 18.f)),
            FVector(Rng.FRandRange(120.f, 190.f), Rng.FRandRange(120.f, 190.f), H));
    }
    // ---- o primeiro Selo (a história: a Estrela Partida caiu exatamente sobre ele e acordou o Azhûr). Um disco de
    // pedra antiga no meio, rachado, com dois anéis de runas; a lava sai das rachaduras dele
    {
        const float Z = Ground(C.X, C.Y);
        UInstancedStaticMeshComponent* Seal = bVisual ? Set(TEXT("SealStone"), WallStone ? WallStone : Mat(TEXT("M_AzhurRock")), false) : nullptr;
        for (int32 Ring = 0; Ring < 3; ++Ring)
        {
            const int32 Count = 8 + Ring * 8;
            const float R0 = Ring * 330.f, R1 = R0 + 330.f;
            for (int32 k = 0; k < Count; ++k)
            {
                const float A = 2.f * UE_PI * (k + .5f * (Ring % 2)) / Count;
                const float Rm = (R0 + R1) * .5f;
                const FVector2D P = C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rm;
                // as placas do Selo, levantadas e tortas onde a estrela bateu
                Box(Seal, FVector(P, Z + 4.f + Rng.FRandRange(0.f, 14.f)), FRotator(Rng.FRandRange(-3.f, 3.f), FMath::RadiansToDegrees(A), Rng.FRandRange(-3.f, 3.f)),
                    FVector(330.f, 2.f * UE_PI * Rm / Count * .94f, 40.f));
            }
        }
        if (bVisual)
        {
            if (UMaterialInterface* Spectral = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SpectralGlow.M_SpectralGlow")))
            {
                SealRunes = UMaterialInstanceDynamic::Create(Spectral, this);
                SealRunes->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, .3f, .05f));
                SealRunes->SetScalarParameterValue(TEXT("Opacity"), 1.f);
            }
            UInstancedStaticMeshComponent* Runes = Set(TEXT("SealRunes"), SealRunes, false);
            for (const float Rr : { 560.f, 960.f })
            {
                const int32 Count = Rr < 700.f ? 12 : 20;
                for (int32 k = 0; k < Count; ++k)
                {
                    const float A = 2.f * UE_PI * k / Count;
                    const FVector2D P = C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rr;
                    // cada runa: um traço e um traço menor cruzando (como as do corpo dele)
                    Box(Runes, FVector(P, Z + 26.f), FRotator(0.f, FMath::RadiansToDegrees(A), 0.f), FVector(28.f, 120.f, 6.f));
                    Box(Runes, FVector(P + FVector2D(FMath::Cos(A), FMath::Sin(A)) * 30.f, Z + 26.f), FRotator(0.f, FMath::RadiansToDegrees(A) + 55.f, 0.f), FVector(22.f, 60.f, 6.f));
                }
            }
        }
        // os oito pilares dos antigos em volta do Selo: uns de pé e quebrados, outros caídos
        for (int32 k = 0; k < 8; ++k)
        {
            const float A = 2.f * UE_PI * k / 8.f + .2f;
            const FVector2D P = C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * 1550.f;
            if (NearPool(P, 150.f)) continue;
            const float Pz = Ground(P.X, P.Y);
            const bool bFallen = k % 3 == 1;
            const float H = bFallen ? 900.f : Rng.FRandRange(450.f, 1000.f);
            Box(Walls, FVector(P, Pz + (bFallen ? 110.f : H * .5f - 20.f)), bFallen ? FRotator(86.f, FMath::RadiansToDegrees(A) + 70.f, 0.f) : FRotator(Rng.FRandRange(-4.f, 4.f), Rng.FRandRange(0.f, 90.f), Rng.FRandRange(-4.f, 4.f)),
                FVector(220.f, 220.f, H));
            if (!bFallen && bVisual)
            {
                UInstancedStaticMeshComponent* Band = Set(*FString::Printf(TEXT("PillarRunes%d"), k), SealRunes, false);
                Box(Band, FVector(P, Pz + H * .6f), FRotator::ZeroRotator, FVector(232.f, 232.f, 26.f));
            }
        }
    }
    // ---- o pedaço da Estrela Partida (o pedaço do Vórtice que caiu no Selo), cravado na borda do Selo, brilhando
    // em violeta
    {
        const FVector2D P = C + FVector2D(FMath::Cos(UE_PI), FMath::Sin(UE_PI)) * 3000.f;
        const float Z = Ground(P.X, P.Y);
        // pedras quebradas em volta do núcleo (baixas: não tapam a luta)
        for (int32 k = 0; k < 7; ++k)
        {
            const float S = Rng.FRandRange(110.f, 210.f);
            Box(Walls, FVector(P + FVector2D(Rng.FRandRange(-260.f, 260.f), Rng.FRandRange(-260.f, 260.f)), Z + S * .2f),
                FRotator(Rng.FRandRange(-30.f, 30.f), Rng.FRandRange(0.f, 90.f), Rng.FRandRange(-30.f, 30.f)), FVector(S, S * .8f, S * 1.4f));
        }
        if (bVisual)
        {
            UInstancedStaticMeshComponent* Star = Set(TEXT("CraterStar"), Mat(TEXT("M_AzhurCore")), false);
            Box(Star, FVector(P, Z + 110.f), FRotator(35.f, 20.f, 25.f), FVector(150.f, 150.f, 150.f));
            UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
            Light->SetupAttachment(RootComponent);
            Light->SetWorldLocation(FVector(P, Z + 450.f));
            Light->SetLightColor(FLinearColor(.75f, .4f, 1.f));
            Light->SetIntensity(16000.f);
            Light->SetAttenuationRadius(2600.f);
            Light->SetCastShadows(false);
            Light->RegisterComponent();
        }
    }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_CALDERA cratera: paredões %d, agulhas %d"), Walls->GetInstanceCount(), Spikes->GetInstanceCount());
}

void AVortexLavaArena::Tick(float DeltaSeconds)
{
    VORTEX_PROBE("Lava");
    Super::Tick(DeltaSeconds);
    if (!bSeeded) Seed();
    AVortexTerrainStreamer* Streamer = StreamerOf(GetWorld());
    if (!bCraterBuilt) BuildCrater();
    if (SealRunes)
    {
        bool bBossDown = false, bBoss = false;
        for (TActorIterator<AVortexMonster> It(GetWorld()); It; ++It)
            if (It->IsBoss() && FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) < 8000.f) { bBoss = true; bBossDown |= It->IsCreatureDead(); }
        SealLit = FMath::FInterpTo(SealLit, bBoss && bBossDown ? 1.f : 0.f, DeltaSeconds, .6f);
        const float Pulse = .75f + .25f * FMath::Sin(GetWorld()->GetTimeSeconds() * (2.5f - 1.5f * SealLit));
        SealRunes->SetVectorParameterValue(TEXT("Color"), FMath::Lerp(FLinearColor(1.f, .3f, .05f), FLinearColor(.2f, .95f, 1.f), SealLit) * Pulse);
    }
    int32 Placed = 0;
    for (int32 i = Pools.Num() - 1; i >= 0; --i)
    {
        FPool& Pool = Pools[i];
        if (!Pool.bPlaced)
        {
            // montar uma poça (malha, pedras, fumaça, luz) custa: uma por quadro, menos as do chefe
            float Ground = 0.f;
            if (Pool.Life <= 0.f && Placed > 0) continue;
            ++Placed;
            if (Pool.Life > 0.f) Place(Pool, Pool.Ground);
            else if (Streamer && Streamer->GroundReadyAt(Pool.At.X, Pool.At.Y, Ground)) Place(Pool, Ground);
            continue;
        }
        if (Pool.Life <= 0.f) continue;
        Pool.Age += DeltaSeconds;
        // poça de meteoro: esfria no último segundo (encolhe) e some
        const float Left = Pool.Life - Pool.Age;
        if (Left < 1.f)
        {
            for (const TWeakObjectPtr<USceneComponent>& Part : Pool.Parts)
                if (Part.IsValid()) Part->SetWorldScale3D(Part->GetComponentScale() * FMath::Clamp(1.f - DeltaSeconds * 1.2f, 0.f, 1.f));
        }
        if (Left <= 0.f)
        {
            Remove(Pool);
            Pools.RemoveAt(i);
        }
    }
    // bolhas: nascem numa poça qualquer, incham devagar e estouram (respingo de brasa e um sopro de fumaça)
    if (GetNetMode() != NM_DedicatedServer)
    {
        const APlayerController* Viewer = GetWorld()->GetFirstPlayerController();
        const FVector Eye = Viewer && Viewer->PlayerCameraManager ? Viewer->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
        if ((BubbleTimer -= DeltaSeconds) <= 0.f && Pools.Num() && Bubbles.Num() < 24)
        {
            BubbleTimer = .18f;
            const FPool& Pool = Pools[FMath::RandRange(0, Pools.Num() - 1)];
            if (Pool.bPlaced && FVector::DistSquared2D(FVector(Pool.At, 0.f), Eye) < FMath::Square(6000.f))
            {
                const FVector2D P = Pool.At + FVector2D(FMath::VRand()).GetSafeNormal() * Pool.Radius * FMath::FRandRange(0.f, .75f);
                UStaticMeshComponent* Ball = NewObject<UStaticMeshComponent>(this);
                Ball->SetupAttachment(RootComponent);
                Ball->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
                if (UMaterialInterface* Hot = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Creatures/RuneColossus/Materials/M_AzhurLava.M_AzhurLava"))) Ball->SetMaterial(0, Hot);
                Ball->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Ball->SetCastShadow(false);
                Ball->SetWorldLocation(FVector(P.X, P.Y, Pool.Ground + 2.f));
                Ball->SetWorldScale3D(FVector(.01f));
                Ball->RegisterComponent();
                Bubbles.Add({ Ball, 0.f, FMath::FRandRange(.7f, 1.4f), FMath::FRandRange(.25f, .6f) });
            }
        }
        for (int32 b = Bubbles.Num() - 1; b >= 0; --b)
        {
            FBubble& Bubble = Bubbles[b];
            UStaticMeshComponent* Ball = Bubble.Ball.Get();
            Bubble.Age += DeltaSeconds;
            const float U = Bubble.Age / Bubble.Life;
            if (!Ball || U >= 1.f)
            {
                if (Ball)
                {
                    const FVector At = Ball->GetComponentLocation();
                    VortexHitFx::Splash(GetWorld(), At + FVector(0.f, 0.f, 6.f), FLinearColor(1.f, .45f, .05f), 6);
                    Ball->DestroyComponent();
                }
                Bubbles.RemoveAt(b);
                continue;
            }
            // meia esfera que incha (cresce mais na largura) e treme antes de estourar
            const float Grow = FMath::Pow(U, .7f) * Bubble.Size;
            const float Wobble = U > .8f ? 1.f + .12f * FMath::Sin(Bubble.Age * 40.f) : 1.f;
            Ball->SetWorldScale3D(FVector(Grow * Wobble, Grow * Wobble, Grow * .55f));
        }
    }
    // servidor: quem pisa na lava se queima (a cada meio segundo, mais a queimadura que fica)
    if (!HasAuthority() || (DamageTimer += DeltaSeconds) < .5f) return;
    DamageTimer = 0.f;
    for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
    {
        AVortexCharacter* Hero = *It;
        UAbilitySystemComponent* Abilities = Hero->GetAbilitySystemComponent();
        if (!Abilities || Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) <= 0.f) continue;
        const FVector Feet = Hero->GetActorLocation() - FVector(0.f, 0.f, Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
        if (!InLava(Feet)) continue;
        const float MaxHealth = Abilities->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute());
        const float Burn = FMath::Max(6.f, FMath::RoundToFloat(MaxHealth * .06f));
        Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(),
            FMath::Max(0.f, Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) - Burn));
        if (UVortexHeroSheet* Sheet = Hero->GetSheet()) Sheet->ApplyBurn(3.f);
        VortexAudio::Play(this, TEXT("lava_burn"), Feet, .6f);
    }
}

// ---- meteoro -----------------------------------------------------------------------------------------

AVortexMeteor::AVortexMeteor()
{
    PrimaryActorTick.bCanEverTick = true;
    Rock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rock"));
    SetRootComponent(Rock);
    Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Rock->SetCastShadow(false);
}

void AVortexMeteor::Start(const FVector& InLand, float InFlight)
{
    Land = InLand;
    Flight = FMath::Max(.2f, InFlight);
    // vem de cima e um pouco de lado (do céu vermelho), girando
    From = Land + FVector(FMath::FRandRange(-900.f, 900.f), FMath::FRandRange(-900.f, 900.f), 3200.f);
    Rock->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    if (UMaterialInterface* Lava = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Creatures/RuneColossus/Materials/M_AzhurLava.M_AzhurLava"))) Rock->SetMaterial(0, Lava);
    Rock->SetWorldScale3D(FVector(1.3f));
    SetActorLocation(From);
    if (UNiagaraSystem* Fire = VortexHitFx::System(TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Small.NS_Fire_Small")))
        UNiagaraFunctionLibrary::SpawnSystemAttached(Fire, Rock, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.6f), EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None);
}

void AVortexMeteor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    const float T = FMath::Clamp(Age / Flight, 0.f, 1.f);
    SetActorLocation(FMath::Lerp(From, Land, T * T));
    SetActorRotation(FRotator(Age * 260.f, Age * 190.f, 0.f));
    if (T >= 1.f)
    {
        VortexHitFx::Splash(GetWorld(), Land, FLinearColor(1.f, .45f, .05f), 26);
        VortexHitFx::Splash(GetWorld(), Land, FLinearColor(.15f, .12f, .1f), 14);
        VortexAudio::Play(this, TEXT("boss_impact_magma"), Land, 1.f);
        Destroy();
    }
}

// ---- brasas e cinzas -------------------------------------------------------------------------------

AVortexEmbers::AVortexEmbers()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    Glow = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Glow"));
    Glow->SetupAttachment(RootComponent);
    Glow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Glow->SetCastShadow(false);
    Glow->SetUsingAbsoluteLocation(true);
    Glow->SetUsingAbsoluteRotation(true);
    Ash = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Ash"));
    Ash->SetupAttachment(RootComponent);
    Ash->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Ash->SetCastShadow(false);
    Ash->SetUsingAbsoluteLocation(true);
    Ash->SetUsingAbsoluteRotation(true);
    Rng.Initialize(777);
}

void AVortexEmbers::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Glow->GetStaticMesh())
    {
        UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        Glow->SetStaticMesh(Cube);
        Ash->SetStaticMesh(Cube);
        if (UMaterialInterface* Spectral = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SpectralGlow.M_SpectralGlow")))
        {
            UMaterialInstanceDynamic* Hot = UMaterialInstanceDynamic::Create(Spectral, this);
            Hot->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, .35f, .05f));
            Hot->SetScalarParameterValue(TEXT("Opacity"), 1.f);
            Glow->SetMaterial(0, Hot);
        }
        if (UMaterialInterface* Solid = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SolidColor.M_SolidColor")))
        {
            UMaterialInstanceDynamic* Grey = UMaterialInstanceDynamic::Create(Solid, this);
            Grey->SetVectorParameterValue(TEXT("Color"), FLinearColor(.12f, .11f, .11f));
            Ash->SetMaterial(0, Grey);
        }
    }
    const FVector Center = GetActorLocation();
    SpawnDebt += DeltaSeconds * 70.f * Strength;
    while (SpawnDebt >= 1.f && Bits.Num() < 360)
    {
        SpawnDebt -= 1.f;
        FBit Bit;
        Bit.bEmber = Rng.FRand() < .55f;
        const FVector2D Ring = FVector2D(Rng.FRandRange(-1.f, 1.f), Rng.FRandRange(-1.f, 1.f)) * 2400.f;
        Bit.At = Center + FVector(Ring.X, Ring.Y, Bit.bEmber ? Rng.FRandRange(-500.f, 200.f) : Rng.FRandRange(300.f, 1200.f));
        Bit.Velocity = Bit.bEmber ? FVector(Rng.FRandRange(-40.f, 40.f), Rng.FRandRange(-40.f, 40.f), Rng.FRandRange(70.f, 160.f))
                                  : FVector(Rng.FRandRange(-30.f, 30.f), Rng.FRandRange(-30.f, 30.f), -Rng.FRandRange(35.f, 80.f));
        Bit.Life = Rng.FRandRange(3.f, 5.5f);
        Bit.Age = 0.f;
        Bit.Size = Bit.bEmber ? Rng.FRandRange(.035f, .07f) : Rng.FRandRange(.025f, .05f);
        Bits.Add(Bit);
    }
    SpawnDebt = FMath::Min(SpawnDebt, 1.f);
    TArray<FTransform> Hot, Grey;
    for (int32 i = Bits.Num() - 1; i >= 0; --i)
    {
        FBit& Bit = Bits[i];
        Bit.Age += DeltaSeconds;
        if (Bit.Age >= Bit.Life) { Bits.RemoveAtSwap(i); continue; }
        // brasas sobem tremulando; cinzas caem rodando
        Bit.At += (Bit.Velocity + FVector(FMath::Sin(Bit.Age * 3.f + i) * 30.f, FMath::Cos(Bit.Age * 2.3f + i) * 30.f, 0.f)) * DeltaSeconds;
        const float Fade = FMath::Clamp(FMath::Min(Bit.Age / .5f, (Bit.Life - Bit.Age) / .8f), 0.f, 1.f);
        const FTransform Pose(FRotator(Bit.Age * 90.f, Bit.Age * 70.f, 0.f), Bit.At, FVector(Bit.Size * Fade));
        (Bit.bEmber ? Hot : Grey).Add(Pose);
    }
    Glow->ClearInstances();
    Ash->ClearInstances();
    if (Hot.Num()) Glow->AddInstances(Hot, false, true);
    if (Grey.Num()) Ash->AddInstances(Grey, false, true);
    if (Strength <= .01f && Bits.IsEmpty()) Destroy();
}
