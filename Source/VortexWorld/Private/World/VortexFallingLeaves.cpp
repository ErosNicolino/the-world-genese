#include "World/VortexFallingLeaves.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshAttributes.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexTown.h"
#include "World/VortexWorldLayout.h"

AVortexFallingLeaves::AVortexFallingLeaves()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

// A folha: uma plaquinha de 12 cm deitada (do tamanho dos blocos das copas; 7 cm sumia) (o recorte em folha é do material)
UStaticMesh* AVortexFallingLeaves::BuildLeaf(UMaterialInterface* Surface)
{
    FMeshDescription Description;
    FStaticMeshAttributes Attributes(Description);
    Attributes.Register();
    TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
    TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
    TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
    const FPolygonGroupID Group = Description.CreatePolygonGroup();
    Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Leaf");
    TArray<FVertexInstanceID> Corners;
    for (const FVector2f UV : { FVector2f(0, 0), FVector2f(1, 0), FVector2f(1, 1), FVector2f(0, 1) })
    {
        const FVertexID Vertex = Description.CreateVertex();
        Positions[Vertex] = FVector3f((UV.Y - .5f) * -12.f, (UV.X - .5f) * 12.f, 0.f);
        const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertex);
        Normals[Instance] = FVector3f(0.f, 0.f, 1.f);
        UVs.Set(Instance, 0, UV);
        Corners.Add(Instance);
    }
    Description.CreateTriangle(Group, { Corners[0], Corners[1], Corners[2] });
    Description.CreateTriangle(Group, { Corners[0], Corners[2], Corners[3] });
    UStaticMesh* Mesh = NewObject<UStaticMesh>(this);
    Mesh->GetStaticMaterials().Add(FStaticMaterial(Surface, TEXT("Leaf")));
    UStaticMesh::FBuildMeshDescriptionsParams Params;
    Params.bFastBuild = true;
    Params.bBuildSimpleCollision = false;
    Params.bCommitMeshDescription = false;
    Mesh->BuildFromMeshDescriptions({ &Description }, Params);
    // malha feita no código: sem a densidade de UV, o streaming de texturas dispara um ensure (no pacote de medição o
    // jogo travava nele)
    for (FStaticMaterial& Slot : Mesh->GetStaticMaterials()) Slot.UVChannelData = FMeshUVChannelInfo(1.f);
    return Mesh;
}

void AVortexFallingLeaves::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer) { Destroy(); return; }
    FString Text;
    TSharedPtr<FJsonObject> Json;
    if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/World/folhas.json")))
        && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) && Json.IsValid())
    {
        for (const auto& Pair : Json->Values)
        {
            double Number = 0.0;
            if (Pair.Value.IsValid() && Pair.Value->TryGetNumber(Number)) Tune.Add(FString(Pair.Key), float(Number));
        }
        const TSharedPtr<FJsonObject>* Table = nullptr;
        if (Json->TryGetObjectField(TEXT("arvores"), Table))
            for (const auto& Pair : (*Table)->Values)
            {
                const TSharedPtr<FJsonObject> Entry = Pair.Value.IsValid() ? Pair.Value->AsObject() : nullptr;
                if (!Entry) continue;
                FKindInfo Info;
                Info.Rate = float(Entry->GetNumberField(TEXT("taxa")));
                const TArray<TSharedPtr<FJsonValue>>* Colors = nullptr;
                if (Entry->TryGetArrayField(TEXT("cores"), Colors))
                    for (const TSharedPtr<FJsonValue>& C : *Colors)
                    {
                        const TArray<TSharedPtr<FJsonValue>>& Rgb = C->AsArray();
                        // o JSON é sRGB; os dados por cópia vão direto para a cor do material (linear)
                        if (Rgb.Num() >= 3)
                            Info.Colors.Add(FLinearColor(FMath::Pow(float(Rgb[0]->AsNumber()), 2.2f), FMath::Pow(float(Rgb[1]->AsNumber()), 2.2f), FMath::Pow(float(Rgb[2]->AsNumber()), 2.2f)));
                    }
                if (Info.Rate > 0.f && Info.Colors.Num() > 0) ByName.Add(FString(Pair.Key), Info);
            }
    }
    UMaterialInterface* Surface = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_FallingLeaf.M_FallingLeaf"));
    if (!Surface || ByName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("VORTEX_FALLING_LEAVES desligadas (M_FallingLeaf=%d, tipos=%d)"), Surface ? 1 : 0, ByName.Num());
        Destroy();
        return;
    }
    bDebug = FParse::Param(FCommandLine::Get(), TEXT("VortexLeavesDebug"));
    Leaves = NewObject<UInstancedStaticMeshComponent>(this);
    Leaves->SetStaticMesh(BuildLeaf(Surface));
    Leaves->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Leaves->SetCastShadow(false);
    Leaves->SetCanEverAffectNavigation(false);
    Leaves->SetNumCustomDataFloats(3);
    Leaves->SetupAttachment(GetRootComponent());
    Leaves->RegisterComponent();
    const int32 Count = FMath::Clamp(FMath::RoundToInt(T(TEXT("quantidade"), 70.f) * (bDebug ? 3.f : 1.f)), 1, 400);
    TArray<FTransform> Hidden;
    Hidden.Init(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, -100000.f), FVector(.01f)), Count);
    Leaves->AddInstances(Hidden, false, true);
    Leaf.SetNum(Count);
    // as primeiras nascem espalhadas no tempo
    for (FLeaf& L : Leaf) L.Timer = Rng.FRandRange(0.f, bDebug ? .5f : 6.f);
}

// O vento da grama no ponto P (M_GrassWind): a direção ventoGraus e a rajada lenta que atravessa o campo, mais um
// redemoinho fraco de lado
FVector AVortexFallingLeaves::WindAt(const FVector& P, float Time) const
{
    const FVector Dir = FRotator(0.f, T(TEXT("ventoGraus"), 26.6f), 0.f).Vector();
    const float Gust = .55f + .45f * FMath::Sin(Time * .55f + P.X * .0021f + P.Y * .0016f);
    const float Swirl = FMath::Sin(Time * 1.7f + P.X * .019f - P.Y * .008f) * .18f;
    return (Dir * Gust + FVector(-Dir.Y, Dir.X, 0.f) * Swirl) * T(TEXT("vento"), 140.f);
}

void AVortexFallingLeaves::ScanTrees(const FVector& Camera)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return;
    if (!bKindsReady)
    {
        // o tamanho da copa de cada tipo sai do próprio modelo da árvore
        for (int32 Kind = 0; Kind < Data->Kinds.Num(); ++Kind)
        {
            const FKindInfo* Info = ByName.Find(Data->Kinds[Kind]);
            if (!Info) continue;
            const FString& Name = Data->Kinds[Kind];
            const UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Props/%s/%s/StaticMeshes/%s.%s"), *Name, *Name, *Name, *Name));
            FKindInfo Sized = *Info;
            const FBoxSphereBounds Bounds = Mesh ? Mesh->GetBounds() : FBoxSphereBounds(FVector(0, 0, 300), FVector(200, 200, 300), 400);
            Sized.Top = Bounds.Origin.Z + Bounds.BoxExtent.Z * .85f;
            Sized.Low = Bounds.Origin.Z + Bounds.BoxExtent.Z * .05f;
            Sized.Radius = FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y) * .7f;
            Kinds.Add(uint16(Kind), Sized);
        }
        bKindsReady = true;
        UE_LOG(LogTemp, Display, TEXT("VORTEX_FALLING_LEAVES tipos com folha=%d (de %d no json)"), Kinds.Num(), ByName.Num());
    }
    Trees.Reset();
    TotalWeight = 0.f;
    const float Radius = T(TEXT("raio"), 2600.f);
    const float Chunk = Data->ChunkSize();
    for (int32 Cx = FMath::FloorToInt((Camera.X - Radius) / Chunk); Cx <= FMath::FloorToInt((Camera.X + Radius) / Chunk); ++Cx)
    for (int32 Cy = FMath::FloorToInt((Camera.Y - Radius) / Chunk); Cy <= FMath::FloorToInt((Camera.Y + Radius) / Chunk); ++Cy)
    {
        const TArray<FVortexTerrainProp>* Props = Data->PropsInChunk(FIntPoint(Cx, Cy));
        if (!Props) continue;
        for (const FVortexTerrainProp& Prop : *Props)
        {
            const FKindInfo* Info = Kinds.Find(Prop.Kind);
            if (!Info) continue;
            const float Distance = FVector::Dist2D(Prop.Location, Camera);
            if (Distance > Radius) continue;
            // as árvores que o streamer não desenha (vila, cratera) também não soltam folha
            if (AVortexTown::Contains(Prop.Location) || VortexWorldLayout::CalderaClearsVegetation(Prop.Location.X, Prop.Location.Y)) continue;
            FTree& Tree = Trees.AddDefaulted_GetRef();
            Tree.At = Prop.Location;
            Tree.Scale = Prop.Scale;
            Tree.Kind = Prop.Kind;
            // mais das árvores perto (é onde se vê)
            Tree.Weight = Info->Rate / (1.f + Distance / 900.f);
            TotalWeight += Tree.Weight;
        }
    }
    static int32 Logged = 0;
    if (Logged++ < 3) UE_LOG(LogTemp, Display, TEXT("VORTEX_FALLING_LEAVES arvores perto=%d em %s"), Trees.Num(), *Camera.ToCompactString());
}

bool AVortexFallingLeaves::Spawn(int32 Index, bool bMidAir)
{
    if (Trees.IsEmpty() || TotalWeight <= 0.f) return false;
    float Pick = Rng.FRandRange(0.f, TotalWeight);
    const FTree* Tree = &Trees.Last();
    for (const FTree& Candidate : Trees)
    {
        Pick -= Candidate.Weight;
        if (Pick <= 0.f) { Tree = &Candidate; break; }
    }
    const FKindInfo& Info = Kinds[uint16(Tree->Kind)];
    FLeaf& L = Leaf[Index];
    const FVector2D Offset = FVector2D(Rng.FRandRange(-1.f, 1.f), Rng.FRandRange(-1.f, 1.f)).GetSafeNormal() * FMath::Sqrt(Rng.FRand()) * Info.Radius * Tree->Scale;
    L.Pos = Tree->At + FVector(Offset, Rng.FRandRange(Info.Low, Info.Top) * Tree->Scale);
    // teste: 1 em cada 4 bem na frente da câmera (para a foto mostrar a folha de perto)
    if (bDebug && Index % 4 == 0)
        L.Pos = Eye + Look * Rng.FRandRange(250.f, 600.f) + FVector(-Look.Y, Look.X, 0.f) * Rng.FRandRange(-250.f, 250.f) + FVector(0.f, 0.f, Rng.FRandRange(-60.f, 150.f));
    else if (bMidAir)
        if (const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData())
        {
            const float Ground = Data->GroundAt(L.Pos.X, L.Pos.Y);
            L.Pos.Z = FMath::Lerp(Ground + 30.f, L.Pos.Z, Rng.FRand());
        }
    L.Vel = FVector::ZeroVector;
    L.Phase = Rng.FRandRange(0.f, UE_TWO_PI);
    L.Swing = Rng.FRandRange(.6f, 1.3f);
    L.Spin = Rng.FRandRange(-90.f, 90.f);
    L.Yaw = Rng.FRandRange(0.f, 360.f);
    L.Size = Rng.FRandRange(T(TEXT("tamanhoMin"), .8f), T(TEXT("tamanhoMax"), 1.35f));
    L.Fade = 1.f;
    L.State = EState::Falling;
    const FLinearColor Color = Info.Colors[Rng.RandRange(0, Info.Colors.Num() - 1)] * Rng.FRandRange(.88f, 1.1f);
    Leaves->SetCustomDataValue(Index, 0, Color.R);
    Leaves->SetCustomDataValue(Index, 1, Color.G);
    Leaves->SetCustomDataValue(Index, 2, Color.B, true);
    return true;
}

void AVortexFallingLeaves::Tick(float Dt)
{
    Super::Tick(Dt);
    Dt = FMath::Min(Dt, .1f);
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    const APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    if (!Data || !Controller || !Controller->PlayerCameraManager) return;
    const FVector Camera = Controller->PlayerCameraManager->GetCameraLocation();
    Eye = Camera;
    Look = Controller->PlayerCameraManager->GetCameraRotation().Vector().GetSafeNormal2D();
    ScanTimer -= Dt;
    // a câmera pulou (chegou ao mundo, viajou): procura as árvores na hora e já enche o ar de folhas no meio da queda
    const bool bJumped = FVector::Dist(Camera, LastScan) > 1500.f;
    if (ScanTimer <= 0.f || bJumped)
    {
        ScanTrees(Camera);
        ScanTimer = 2.f;
        LastScan = Camera;
        if (bJumped)
            for (int32 i = 0; i < Leaf.Num(); ++i)
                if (!Spawn(i, true)) { Leaf[i].State = EState::Waiting; Leaf[i].Timer = Rng.FRandRange(0.f, 2.f); }
    }
    const float Time = GetWorld()->GetTimeSeconds();
    const float Fall = T(TEXT("queda"), 55.f), Sway = T(TEXT("balanco"), 70.f), SwayRate = T(TEXT("balancoVelocidade"), 2.4f);
    // longe da câmera: volta para perto (antes 1,6x o raio, e as folhas ficavam para trás quando o herói andava)
    const float Far = T(TEXT("raio"), 2600.f) * 1.15f;
    const FVector WindDir = FRotator(0.f, T(TEXT("ventoGraus"), 26.6f), 0.f).Vector();
    const FVector Side(-WindDir.Y, WindDir.X, 0.f);
    TArray<FTransform> Poses;
    Poses.Reserve(Leaf.Num());
    for (int32 i = 0; i < Leaf.Num(); ++i)
    {
        FLeaf& L = Leaf[i];
        FRotator Turn = FRotator::ZeroRotator;
        switch (L.State)
        {
        case EState::Waiting:
            L.Timer -= Dt;
            if (L.Timer <= 0.f && !Spawn(i)) L.Timer = 1.f;
            break;
        case EState::Falling:
        {
            // o balanço da folha: vai de um lado ao outro na largura do vento e cai mais rápido no meio do arco
            // (como pêndulo) e devagar nas pontas, onde ela inclina mais
            L.Phase += Dt * SwayRate * (.8f + .4f * L.Swing);
            const float S = FMath::Sin(L.Phase);
            const FVector Want = WindAt(L.Pos, Time) + Side * FMath::Cos(L.Phase) * Sway * L.Swing
                - FVector(0.f, 0.f, Fall * (1.f + .7f * FMath::Abs(FMath::Cos(L.Phase))));
            L.Vel = FMath::Lerp(L.Vel, Want, 1.f - FMath::Exp(-3.f * Dt));
            L.Pos += L.Vel * Dt;
            L.Yaw += L.Spin * Dt;
            // inclina para o lado em que está balançando e um pouco para a frente, para onde o vento leva
            Turn = FRotator(10.f + 12.f * FMath::Cos(L.Phase * .5f), L.Yaw, S * 50.f * L.Swing);
            const float Ground = Data->GroundAt(L.Pos.X, L.Pos.Y);
            if (Ground < Data->WaterLevel && L.Pos.Z <= Data->WaterSurface + 1.f)
            {
                L.State = EState::Water;
                L.Pos.Z = Data->WaterSurface + 1.f;
                L.Timer = T(TEXT("naAgua"), 9.f);
            }
            else if (L.Pos.Z <= Ground + 1.f)
            {
                L.State = EState::Ground;
                L.Pos.Z = Ground + .6f;
                L.Timer = T(TEXT("noChao"), 6.f) * Rng.FRandRange(.7f, 1.3f);
            }
            if (FVector::Dist2D(L.Pos, Camera) > Far) { L.State = EState::Waiting; L.Timer = Rng.FRandRange(0.f, 1.f); }
            break;
        }
        case EState::Water:
            // boia e vai devagar com o vento, subindo e descendo com a água
            L.Pos += WindAt(L.Pos, Time) * .25f * Dt;
            L.Pos.Z = Data->WaterSurface + 1.f + FMath::Sin(Time * 2.f + L.Phase) * .6f;
            L.Yaw += L.Spin * .2f * Dt;
            Turn = FRotator(0.f, L.Yaw, 0.f);
            // e some igual à do chão
            [[fallthrough]];
        case EState::Ground:
            if (L.State == EState::Ground) Turn = FRotator(0.f, L.Yaw, 0.f);
            L.Timer -= Dt;
            L.Fade = FMath::Clamp(L.Timer, 0.f, 1.f);
            if (L.Timer <= 0.f) { L.State = EState::Waiting; L.Timer = Rng.FRandRange(0.f, bDebug ? .3f : 2.f); }
            break;
        }
        const bool bShown = L.State != EState::Waiting;
        Poses.Add(FTransform(Turn, bShown ? L.Pos : Eye - FVector(0.f, 0.f, 5000.f), FVector(bShown ? FMath::Max(L.Size * L.Fade, .01f) : .01f)));
    }
    Leaves->BatchUpdateInstancesTransforms(0, Poses, true, true, true);
    // a caixa de visibilidade do componente acompanha as folhas (sem isto ela ficava onde as cópias nasceram, a 1 km
    // embaixo do mapa, e nenhuma folha era desenhada)
    Leaves->UpdateBounds();
    Leaves->MarkRenderTransformDirty();
    if (bDebug && ScanTimer == 2.f)
    {
        int32 Falling = 0, Ground = 0, First = INDEX_NONE;
        for (int32 i = 0; i < Leaf.Num(); ++i)
        {
            if (Leaf[i].State == EState::Falling) { ++Falling; if (First == INDEX_NONE) First = i; }
            if (Leaf[i].State == EState::Ground) ++Ground;
        }
        FTransform Shown;
        if (First != INDEX_NONE) Leaves->GetInstanceTransform(First, Shown, true);
        UE_LOG(LogTemp, Display, TEXT("VORTEX_FALLING_LEAVES caindo=%d chao=%d instancias=%d primeira=%s escala=%.2f camera=%s"), Falling, Ground,
            Leaves->GetInstanceCount(), *Shown.GetLocation().ToCompactString(), Shown.GetScale3D().X, *Camera.ToCompactString());
    }
}
