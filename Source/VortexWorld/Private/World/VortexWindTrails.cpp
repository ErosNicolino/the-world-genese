#include "World/VortexWindTrails.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshAttributes.h"
#include "World/VortexDayNightSubsystem.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

AVortexWindTrails::AVortexWindTrails()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

// A fita: uma linha de 6 m ondulando de leve (e, na com laço, uma volta inteira no meio), feita de duas tiras em cruz
// (uma deitada e uma em pé) para não sumir quando vista de lado. UV0.x = 0..1 ao longo, UV0.y = 0..1 na largura.
UStaticMesh* AVortexWindTrails::BuildRibbon(UMaterialInterface* Surface, bool bLoop)
{
    const float Len = T(TEXT("comprimento"), 600.f), Width = T(TEXT("largura"), 6.f), Loop = T(TEXT("raioLaco"), 45.f);
    constexpr int32 Steps = 72;
    TArray<FVector> Line;
    for (int32 i = 0; i <= Steps; ++i)
    {
        const float U = float(i) / Steps;
        FVector P(Len * U, 25.f * FMath::Sin(UE_TWO_PI * 1.1f * U), 18.f * FMath::Sin(UE_TWO_PI * .7f * U + 1.f));
        if (bLoop && U > .45f && U < .7f)
        {
            const float Angle = (U - .45f) / .25f * UE_TWO_PI;
            P += FVector(Loop * FMath::Sin(Angle), 0.f, Loop * (1.f - FMath::Cos(Angle)));
        }
        Line.Add(P);
    }
    FMeshDescription Description;
    FStaticMeshAttributes Attributes(Description);
    Attributes.Register();
    TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
    TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
    TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
    const FPolygonGroupID Group = Description.CreatePolygonGroup();
    Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Trail");
    for (int32 Strip = 0; Strip < 2; ++Strip)
    {
        TArray<FVertexInstanceID> Row;
        for (int32 i = 0; i <= Steps; ++i)
        {
            const FVector Along = (Line[FMath::Min(i + 1, Steps)] - Line[FMath::Max(i - 1, 0)]).GetSafeNormal();
            FVector Side = FVector::CrossProduct(Along, FVector::UpVector).GetSafeNormal();
            if (Side.IsNearlyZero()) Side = FVector::RightVector;
            const FVector Up = FVector::CrossProduct(Side, Along).GetSafeNormal();
            const FVector Offset = (Strip == 0 ? Side : Up) * Width * .5f;
            for (int32 Edge = 0; Edge < 2; ++Edge)
            {
                const FVertexID Vertex = Description.CreateVertex();
                Positions[Vertex] = FVector3f(Line[i] + (Edge == 0 ? -Offset : Offset));
                const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertex);
                Normals[Instance] = FVector3f(Strip == 0 ? Up : Side);
                UVs.Set(Instance, 0, FVector2f(float(i) / Steps, float(Edge)));
                Row.Add(Instance);
            }
        }
        for (int32 i = 0; i < Steps; ++i)
        {
            const FVertexInstanceID A = Row[i * 2], B = Row[i * 2 + 1], C = Row[i * 2 + 3], D = Row[i * 2 + 2];
            Description.CreateTriangle(Group, { A, B, C });
            Description.CreateTriangle(Group, { A, C, D });
        }
    }
    UStaticMesh* Ribbon = NewObject<UStaticMesh>(this);
    Ribbon->GetStaticMaterials().Add(FStaticMaterial(Surface, TEXT("Trail")));
    UStaticMesh::FBuildMeshDescriptionsParams Params;
    Params.bFastBuild = true;
    Params.bBuildSimpleCollision = false;
    Params.bCommitMeshDescription = false;
    Ribbon->BuildFromMeshDescriptions({ &Description }, Params);
    // malha feita no código: sem a densidade de UV, o streaming de texturas dispara um ensure (no pacote de medição o
    // jogo travava nele)
    for (FStaticMaterial& Slot : Ribbon->GetStaticMaterials()) Slot.UVChannelData = FMeshUVChannelInfo(1.f);
    return Ribbon;
}

void AVortexWindTrails::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer) { Destroy(); return; }
    FString Text;
    TSharedPtr<FJsonObject> Json;
    if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/World/vento.json")))
        && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) && Json.IsValid())
        for (const auto& Pair : Json->Values)
        {
            double Number = 0.0;
            if (Pair.Value.IsValid() && Pair.Value->TryGetNumber(Number)) Tune.Add(FString(Pair.Key), float(Number));
        }
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_WindTrail.M_WindTrail"));
    if (!Base || T(TEXT("quantidade"), 10.f) < 1.f)
    {
        if (!Base) UE_LOG(LogTemp, Warning, TEXT("VORTEX_WIND_TRAILS sem o M_WindTrail (Tools/CreateWindTrailMaterial.py)"));
        Destroy();
        return;
    }
    bDebug = FParse::Param(FCommandLine::Get(), TEXT("VortexWindDebug"));
    Material = UMaterialInstanceDynamic::Create(Base, this);
    Material->SetVectorParameterValue(TEXT("Cor"), FLinearColor(T(TEXT("corR"), 1.f), T(TEXT("corG"), 1.f), T(TEXT("corB"), 1.f)));
    for (int32 Shape = 0; Shape < 2; ++Shape)
    {
        UInstancedStaticMeshComponent* Ism = NewObject<UInstancedStaticMeshComponent>(this);
        Ism->SetStaticMesh(BuildRibbon(Material, Shape == 1));
        Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Ism->SetCastShadow(false);
        Ism->SetCanEverAffectNavigation(false);
        Ism->SetNumCustomDataFloats(4);
        // a fita anda com o vento pelo material: a caixa maior para não sumir antes da hora
        Ism->SetBoundsScale(3.f);
        Ism->SetupAttachment(GetRootComponent());
        Ism->RegisterComponent();
        Ribbons.Add(Ism);
    }
    const int32 Count = FMath::Clamp(FMath::RoundToInt(T(TEXT("quantidade"), 10.f)), 1, 64);
    const float Now = GetWorld()->GetTimeSeconds();
    for (int32 i = 0; i < Count; ++i)
    {
        FTrail& Trail = Trails.AddDefaulted_GetRef();
        // 1 em cada 3 com laço
        Trail.Shape = i % 3 == 2 ? 1 : 0;
        Trail.Instance = Ribbons[Trail.Shape]->AddInstance(FTransform(FVector(0.f, 0.f, -100000.f)), true);
        for (int32 Slot = 0; Slot < 4; ++Slot) Ribbons[Trail.Shape]->SetCustomDataValue(Trail.Instance, Slot, Slot == 1 ? 1.f : -1000.f);
        // nascem espalhadas no tempo (não todas juntas)
        Trail.NextAt = Now + Rng.FRandRange(0.f, bDebug ? .3f : 4.f);
    }
}

void AVortexWindTrails::Respawn(int32 Index, float Now)
{
    FTrail& Trail = Trails[Index];
    UInstancedStaticMeshComponent* Ism = Ribbons[Trail.Shape];
    const APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Controller || !Controller->PlayerCameraManager) { Trail.NextAt = Now + 1.f; return; }
    const FVector Camera = Controller->PlayerCameraManager->GetCameraLocation();
    const float CameraYaw = Controller->PlayerCameraManager->GetCameraRotation().Yaw;
    // na frente da câmera (dá para ver), entre 5 e 22 m, a 0,8 a 3,5 m do chão
    const float Yaw = CameraYaw + Rng.FRandRange(-T(TEXT("anguloVista"), 65.f), T(TEXT("anguloVista"), 65.f)) * (bDebug ? .4f : 1.f);
    const float Distance = Rng.FRandRange(T(TEXT("distanciaMin"), 500.f), bDebug ? 900.f : T(TEXT("distanciaMax"), 2200.f));
    FVector At = Camera + FRotator(0.f, Yaw, 0.f).Vector() * Distance;
    const float Ground = Data ? Data->GroundAt(At.X, At.Y) : Camera.Z - 170.f;
    if (Data && Ground <= -299.f) { Trail.NextAt = Now + .5f; return; }   // fora do mapa (sobre a água pode)
    At.Z = FMath::Max(Ground, Data ? Data->WaterLevel : Ground) + Rng.FRandRange(T(TEXT("alturaMin"), 80.f), T(TEXT("alturaMax"), 350.f));
    // a direção do vento (a da grama, M_GrassWind: mais para +X do que +Y) com um pouco de variação; a fita começa
    // meio comprimento antes, para o meio dela passar no ponto sorteado
    const float WindYaw = T(TEXT("ventoGraus"), 26.6f) + Rng.FRandRange(-20.f, 20.f);
    const FVector Wind = FRotator(0.f, WindYaw, 0.f).Vector();
    const float Scale = Rng.FRandRange(.7f, 1.25f);
    At -= Wind * T(TEXT("comprimento"), 600.f) * Scale * .5f;
    const FTransform Pose(FRotator(Rng.FRandRange(-8.f, 8.f), WindYaw, Rng.FRandRange(-25.f, 25.f)), At, FVector(Scale));
    const float Life = Rng.FRandRange(T(TEXT("vidaMin"), 2.2f), T(TEXT("vidaMax"), 3.6f)) * (bDebug ? 2.f : 1.f);
    const float Push = T(TEXT("velocidade"), 160.f) * (bDebug ? .3f : 1.f);
    Ism->UpdateInstanceTransform(Trail.Instance, Pose, true, false, true);
    Ism->SetCustomDataValue(Trail.Instance, 0, Now);
    Ism->SetCustomDataValue(Trail.Instance, 1, Life);
    Ism->SetCustomDataValue(Trail.Instance, 2, Wind.X * Push);
    Ism->SetCustomDataValue(Trail.Instance, 3, Wind.Y * Push, true);
    Trail.NextAt = Now + Life + (bDebug ? .2f : Rng.FRandRange(T(TEXT("pausaMin"), .6f), T(TEXT("pausaMax"), 3.5f)));
}

void AVortexWindTrails::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Now = GetWorld()->GetTimeSeconds();
    // de noite quase somem (o claro do dia de 0 a 1)
    const UVortexDayNightSubsystem* DayNight = GetWorld()->GetSubsystem<UVortexDayNightSubsystem>();
    const float Light = DayNight && DayNight->IsActive() ? DayNight->GetDaylight() : 1.f;
    if (FMath::Abs(Light - Daylight) > .02f)
    {
        Daylight = Light;
        Material->SetScalarParameterValue(TEXT("Opacidade"), T(TEXT("opacidade"), .4f) * FMath::Lerp(T(TEXT("noite"), .2f), 1.f, Daylight));
    }
    for (int32 i = 0; i < Trails.Num(); ++i)
        if (Now >= Trails[i].NextAt) Respawn(i, Now);
}
