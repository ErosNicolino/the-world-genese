#include "World/VortexTerrainStreamer.h"
#include "World/VortexWorldHooks.h"
#include "World/VortexWorldLayout.h"
#include "World/VortexPointsOfInterest.h"
#include "World/VortexPortal.h"
#include "Framework/VortexProbe.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialParameterCollection.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpectatorPawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "UObject/UObjectIterator.h"
#include "HAL/PlatformMemory.h"
#include "ProceduralMeshComponent.h"
#include "Tasks/Task.h"
#include "World/VortexWaterZone.h"
#include "World/VortexFarTerrain.h"
#include "World/VortexMist.h"
#include "World/VortexLandmarks.h"
#include "World/VortexTown.h"
#include "Framework/VortexPlayerSettings.h"
#include "Framework/VortexViewScales.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
    // Plantas baixas: modelos do Godot em lote, sem sombra e visíveis só de perto.
    bool IsSmallPlant(const FString& Name)
    {
        return Name.StartsWith(TEXT("grass")) || Name.StartsWith(TEXT("flowers")) ||
            Name.Contains(TEXT("clover")) || Name.Contains(TEXT("dandelion")) ||
            Name.Contains(TEXT("poppy")) || Name.Contains(TEXT("wheat")) ||
            Name.Contains(TEXT("fern")) || Name.Contains(TEXT("leaf")) || Name.Contains(TEXT("pebbles")) ||
            Name == TEXT("sunflower") || Name == TEXT("strawberry") || Name == TEXT("reeds") ||
            Name == TEXT("desert_brush") || Name == TEXT("ash_grass") || Name.Contains(TEXT("bloom")) ||
            Name == TEXT("frost_flower") || Name == TEXT("bleached_bones") || Name == TEXT("soul_flower") ||
            Name == TEXT("thorn_bramble") || Name == TEXT("bog_lantern") || Name == TEXT("storm_crystal") ||
            Name == TEXT("pumpkin_patch") || Name == TEXT("glowcap");
    }

    // Tipos de touceira (as 12 variantes, em ordem): rasteira densa (0-3), de prado (4-8) e alta
    // com espigas (9-11). Cada uma é uma malha pronta, repetida por cópias.
    enum class EGrassKind : uint8 { Lawn, Meadow, Tall };
    EGrassKind GrassKindOf(int32 Variant) { return Variant < 4 ? EGrassKind::Lawn : Variant < 9 ? EGrassKind::Meadow : EGrassKind::Tall; }

    // Touceira realista: folhas finas, de 3 segmentos, curvas (a ponta cai para fora) e afinando até
    // a ponta; cada folha com altura, largura, curva, giro e brilho próprios. As altas ganham
    // espigas. A cor sai no M_GrassWind a partir dos dados por vértice (as cores de vértice não
    // sobrevivem à montagem rápida da malha):
    //   Data = (sorteio da folha 0..1, espiga 0/1, brilho, altura 0 na raiz .. 1 na ponta).
    void AddGrassClump(EGrassKind Kind, uint32 Seed,
        TArray<FVector>& Vertices, TArray<FVector>& Normals, TArray<int32>& Triangles, TArray<FVector4f>& Data)
    {
        auto Rand = [&Seed]() { Seed = Seed * 1664525u + 1013904223u; return (Seed >> 8) / float(1 << 24); };
        // a rasteira é larga e cheia (cobre o chão perto da câmera); a de prado é média; a alta, rala
        const int32 Blades = Kind == EGrassKind::Lawn ? 64 + int32(Rand() * 18.f) : Kind == EGrassKind::Meadow ? 30 + int32(Rand() * 10.f) : 14 + int32(Rand() * 6.f);
        const float MinH = Kind == EGrassKind::Lawn ? 7.f : Kind == EGrassKind::Meadow ? 14.f : 28.f;
        const float RangeH = Kind == EGrassKind::Lawn ? 10.f : Kind == EGrassKind::Meadow ? 22.f : 32.f;
        const float Spread = Kind == EGrassKind::Lawn ? 30.f : Kind == EGrassKind::Meadow ? 16.f : 10.f;
        for (int32 Blade = 0; Blade < Blades; ++Blade)
        {
            const float Pick = Rand();
            const float Angle = UE_TWO_PI * Rand();
            const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
            // a face da folha gira um pouco em relação à direção em que ela cai
            const float Twist = (Rand() - .5f) * 1.2f;
            const FVector Side = FVector(-Out.Y, Out.X, 0.f) * FMath::Cos(Twist) + Out * FMath::Sin(Twist);
            // algumas folhas bem mais altas que as outras (alturas desencontradas = menos repetição)
            const float Height = (MinH + RangeH * FMath::Pow(Rand(), 1.6f)) * (Rand() < .12f ? 1.35f : 1.f);
            const float Width = (Kind == EGrassKind::Lawn ? .4f : .5f) + Rand() * .45f;
            const float Lean = (.12f + Rand() * .55f) * Height;
            const FVector Root = Out * FMath::Sqrt(Rand()) * Spread + FVector(0, 0, -1.5f);
            const float Bright = .78f + Rand() * .44f;
            const int32 Start = Vertices.Num();
            static const float Ts[4] = { 0.f, .38f, .72f, 1.f };
            for (int32 k = 0; k < 4; ++k)
            {
                const float T = Ts[k];
                // curva: sobe quase reta e a ponta tomba para fora (lean * t²), com uma leve queda no fim
                const FVector P = Root + Out * Lean * T * T + FVector(0, 0, Height * T - Lean * .18f * T * T * T);
                const float W = Width * (1.f - T * .82f);
                const FVector N = (FVector::UpVector * 1.6f + Out * (.6f - T * .3f)).GetSafeNormal();
                if (k < 3)
                {
                    Vertices.Append({ P - Side * W, P + Side * W });
                    Normals.Append({ N, N });
                    Data.Append({ FVector4f(Pick, 0.f, Bright, T), FVector4f(Pick, 0.f, Bright, T) });
                }
                else
                {
                    Vertices.Add(P);
                    Normals.Add(N);
                    Data.Add(FVector4f(Pick, 0.f, Bright, T));
                }
            }
            Triangles.Append({ Start, Start + 2, Start + 1, Start + 2, Start + 3, Start + 1,
                Start + 2, Start + 4, Start + 3, Start + 4, Start + 5, Start + 3, Start + 4, Start + 6, Start + 5 });
            // espiga: um cacho estreito na ponta de parte das folhas altas
            if (Kind == EGrassKind::Tall && Rand() < .55f)
            {
                const FVector Tip = Vertices.Last();
                const FVector Up = (FVector(0, 0, 1) + Out * .35f).GetSafeNormal();
                const float Len = 5.f + Rand() * 4.f, Fat = .9f + Rand() * .5f;
                for (int32 Card = 0; Card < 2; ++Card)
                {
                    const FVector Across = (Card == 0 ? Side : FVector::CrossProduct(Up, Side).GetSafeNormal()) * Fat;
                    const int32 S0 = Vertices.Num();
                    Vertices.Append({ Tip - Across, Tip + Across, Tip + Up * Len });
                    for (int32 i = 0; i < 3; ++i) { Normals.Add(Up); Data.Add(FVector4f(Pick, 1.f, Bright, 1.f)); }
                    Triangles.Append({ S0, S0 + 2, S0 + 1 });
                }
            }
        }
    }

    // Ruído suave genérico (0..1): grade de Cells células, com semente própria.
    float CellNoise(int32 Gx, int32 Gy, float Cells, int32 SeedOffset)
    {
        auto Lattice = [SeedOffset](int32 X, int32 Y) { return (HashCombineFast(GetTypeHash(X * 92821 + SeedOffset), GetTypeHash(Y * 68917 - SeedOffset)) % 1000) / 999.f; };
        const float Fx = Gx / Cells, Fy = Gy / Cells;
        const int32 X0 = FMath::FloorToInt(Fx), Y0 = FMath::FloorToInt(Fy);
        const float Tx = FMath::SmoothStep(0.f, 1.f, Fx - X0), Ty = FMath::SmoothStep(0.f, 1.f, Fy - Y0);
        return FMath::Lerp(FMath::Lerp(Lattice(X0, Y0), Lattice(X0 + 1, Y0), Tx), FMath::Lerp(Lattice(X0, Y0 + 1), Lattice(X0 + 1, Y0 + 1), Tx), Ty);
    }

    // Topo de cada pedaço: quadros na ordem que a Unreal desenha de frente (normal N).
    void AddQuad(TArray<FVector>& V, TArray<FVector>& Nrm, TArray<int32>& T, TArray<FLinearColor>* C,
        const FVector& A, const FVector& B, const FVector& Cc, const FVector& D, const FVector& N, const FLinearColor& Color)
    {
        const int32 Start = V.Num();
        V.Add(A); V.Add(B); V.Add(Cc); V.Add(D);
        for (int32 i = 0; i < 4; ++i) { Nrm.Add(N); if (C) C->Add(Color); }
        // frente: cross(B - A, C - A) aponta para o lado oposto de N (convenção da Unreal)
        const bool bFlip = FVector::DotProduct(FVector::CrossProduct(B - A, Cc - A), N) > 0.f;
        if (!bFlip) { T.Append({ Start, Start + 1, Start + 2, Start, Start + 2, Start + 3 }); }
        else { T.Append({ Start, Start + 2, Start + 1, Start, Start + 3, Start + 2 }); }
    }

    FVector2D AtlasUV(uint8 Layer, const FVector2D& UV)
    {
        constexpr float Tile = 32.f, AtlasWidth = 21.f * Tile;
        return FVector2D((Layer * Tile + .5f + UV.X * (Tile - 1.f)) / AtlasWidth,
            (.5f + UV.Y * (Tile - 1.f)) / Tile);
    }

    FVector2D RotateTileUV(FVector2D UV, uint8 Rotation)
    {
        UV -= FVector2D(.5f, .5f);
        for (int32 Step = 0; Step < (Rotation & 3); ++Step) UV = FVector2D(-UV.Y, UV.X);
        if (Rotation & 4) UV.X = -UV.X;
        return UV + FVector2D(.5f, .5f);
    }

    uint8 SideLayer(uint8 Top)
    {
        switch (Top)
        {
        case 11: case 12: return 19; // snow, ice
        case 14: return 20; // red sand
        case 5: return 10; // beach sand
        case 6: case 13: case 16: return 9; // stone, ash, purple soil
        case 4: case 15: return 8; // path, mud
        default: return 7; // grassy lip
        }
    }
}

AVortexTerrainStreamer::AVortexTerrainStreamer()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    // o terreno nunca se move: estático, o chão de colisão (abaixo) também pode ser. Chão "móvel" fazia a
    // Unreal mandar a posição do herói relativa a cada pedaço, que o cliente não sabe achar (são criados
    // em cada máquina): o convidado descartava as correções do servidor e o log enchia de avisos
    RootComponent->SetMobility(EComponentMobility::Static);
}

TSharedPtr<FVortexTerrainData> AVortexTerrainStreamer::SharedData()
{
    static TSharedPtr<FVortexTerrainData> Cached;
    if (!Cached)
    {
        FString Error;
        Cached = FVortexTerrainData::Load(FPaths::Combine(FPaths::ProjectDir(), TEXT("Data/GodotTerrain")), Error);
        if (!Cached)
        {
            UE_LOG(LogTemp, Error, TEXT("Vortex terrain: %s"), *Error);
        }
        // o continente em volta do vale (-VortexNoWorld: só o vale, como antes)
        else if (!FParse::Param(FCommandLine::Get(), TEXT("VortexNoWorld")))
        {
            if (!Cached->LoadWorld(FPaths::Combine(FPaths::ProjectDir(), TEXT("Data/GodotWorld")), Error))
            {
                UE_LOG(LogTemp, Error, TEXT("Vortex terrain: %s"), *Error);
            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("Vortex terrain: continente carregado (%d pontes e cabanas)"), Cached->Features().Num());
            }
        }
    }
    return Cached;
}

bool AVortexTerrainStreamer::GroundReadyAt(float X, float Y, float& OutGroundZ) const
{
    if (!Data) return false;
    const float Expected = Data->GroundAt(X, Y);
    if (Expected <= -299.f) return false;
    const FIntPoint Key(FMath::FloorToInt(X / Data->ChunkSize()), FMath::FloorToInt(Y / Data->ChunkSize()));
    const FChunk* Chunk = Chunks.Find(Key);
    if (!Chunk || !Chunk->Collision) return false;
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexTerrainGroundReady), false);
    if (!Chunk->Collision->LineTraceComponent(Hit, FVector(X, Y, Expected + 400.f),
        FVector(X, Y, Expected - 400.f), Query)) return false;
    OutGroundZ = Hit.ImpactPoint.Z;
    return true;
}

void AVortexTerrainStreamer::BeginPlay()
{
    Super::BeginPlay();
    Data = SharedData();
    if (!Data)
    {
        SetActorTickEnabled(false);
        return;
    }
    TerrainMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_TerrainTiles.M_TerrainTiles"));
    // água estilizada: M_VortexWater (Tools/CreateVortexWater.py), turquesa no raso e quase opaca no fundo, com espuma na margem;
    // -VortexWaterMaterial=<caminho> testa outra (ex.: a realista MI_VortexLake). A antiga fica de reserva.
    FString WaterPath = TEXT("/Game/Vortex/Materials/M_VortexWater.M_VortexWater");
    bWaterOverride = FParse::Value(FCommandLine::Get(), TEXT("VortexWaterMaterial="), WaterPath);
    WaterMaterial = LoadObject<UMaterialInterface>(nullptr, *WaterPath);
    if (!WaterMaterial) WaterMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_Water.M_Water"));
    // níveis da água (Opções → Qualidade da água; Tools/CreateVortexWater.py): a média é a mesma água
    // sem as marolinhas
    UMaterialInterface* Simple = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_VortexWaterSimple.M_VortexWaterSimple"));
    UMaterialInterface* Epic = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_VortexWaterEpic.M_VortexWaterEpic"));
    UMaterialInstanceDynamic* Still = UMaterialInstanceDynamic::Create(WaterMaterial, this);
    if (Still) Still->SetScalarParameterValue(TEXT("Ripple"), 0.f);
    AVortexTown::LoadGrassMask();
    WaterLevels = { Simple ? Simple : WaterMaterial.Get(), Still ? static_cast<UMaterialInterface*>(Still) : WaterMaterial.Get(), WaterMaterial.Get(), Epic ? Epic : WaterMaterial.Get() };
    ApplyGraphicsSettings();
    // a Cidade do Santuário (vila inicial do Godot) fica no meio do vale; o mundinho do menu não precisa dela
    if (!GetWorld()->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu")) && !FParse::Param(FCommandLine::Get(), TEXT("VortexNoTown")))
    {
        AVortexTown::Ensure(GetWorld());
    }
    GrassMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_GrassWind.M_GrassWind"));
    // a pisada dos jogadores vai numa coleção de parâmetros (MPC_GrassStep): barata de atualizar,
    // ao contrário de um material dinâmico, que refazia o desenho de toda a grama a cada mudança
    GrassStep = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Vortex/Materials/MPC_GrassStep.MPC_GrassStep"));
    for (FVector& Foot : GrassFeet) Foot = FVector(0.f, 0.f, -1.f);
    BuildGrassMeshes();
    for (const FString& Kind : Data->Kinds)
    {
        const FString Path = FString::Printf(TEXT("/Game/Vortex/Props/%s/%s/StaticMeshes/%s.%s"), *Kind, *Kind, *Kind, *Kind);
        KindMeshes.Add(LoadObject<UStaticMesh>(nullptr, *Path));
    }
    // pontes e cabanas do continente (com colisão: também no servidor)
    if (Data->HasWorld() && !GetWorld()->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu")))
    {
        FActorSpawnParameters FeatureParams;
        FeatureParams.ObjectFlags |= RF_Transient;
        // os recursos do mundo (pontes, cabanas, baús) são do jogo: o módulo Vortex cria (VortexWorldHooks)
        VortexWorldHooks::SpawnGameFeatures(GetWorld());
        GetWorld()->SpawnActor<AVortexLandmarks>(FVector::ZeroVector, FRotator::ZeroRotator, FeatureParams);
        // ruínas, torres, santuários, minas, cemitérios, caravanas e cais (montados perto de quem chega)
        GetWorld()->SpawnActor<AVortexPointsOfInterest>(FVector::ZeroVector, FRotator::ZeroRotator, FeatureParams);
        // dungeons dos portais: em construção (o AVortexDungeons está pronto em parte, mas ainda não é criado)
        // um portal de dungeon (ranque E) perto da cidade, para as dungeons que vão chegar. 06/11: a 95 m ele caía
        // dentro do portão leste da vila nova; agora fica fora da muralha, a nordeste do portão, de frente para ela
        if (HasAuthority()) AVortexPortal::Open(GetWorld(), FVector(12000.f, -2000.f, Data->GroundAt(12000.f, -2000.f)), 180.f, TEXT("E"), TEXT("dungeon_inicial"));
    }
    // o horizonte do continente: montanhas, vales e costa ao longe (só quem desenha a tela)
    if (Data->HasWorld() && GetNetMode() != NM_DedicatedServer && !GetWorld()->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu"))
        && !FParse::Param(FCommandLine::Get(), TEXT("VortexNoFar")))
    {
        FActorSpawnParameters FarParams;
        FarParams.ObjectFlags |= RF_Transient;
        GetWorld()->SpawnActor<AVortexFarTerrain>(FVector::ZeroVector, FRotator::ZeroRotator, FarParams);
    }
    // neblina baixa nos vales, na água e nas matas (pacote FogArea)
    if (GetNetMode() != NM_DedicatedServer && !GetWorld()->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu")) && !FParse::Param(FCommandLine::Get(), TEXT("VortexNoMist")))
    {
        FActorSpawnParameters MistParams;
        MistParams.ObjectFlags |= RF_Transient;
        GetWorld()->SpawnActor<AVortexMist>(FVector::ZeroVector, FRotator::ZeroRotator, MistParams);
    }
    // a água do terreno: "tem água aqui?" responde pela altura das células
    if (UVortexWaterSubsystem* Water = GetWorld()->GetSubsystem<UVortexWaterSubsystem>())
    {
        TWeakPtr<FVortexTerrainData> Weak = Data;
        Water->Provider = [Weak](const FVector& Location, float& OutSurface)
        {
            const TSharedPtr<FVortexTerrainData> Pinned = Weak.Pin();
            return Pinned && Pinned->IsWaterAt(Location.X, Location.Y, OutSurface);
        };
    }
    const FBox2D Bounds = Data->PrototypeBounds();
    UE_LOG(LogTemp, Display, TEXT("Vortex terrain: prototype bounds=(%.0f,%.0f)-(%.0f,%.0f) chunks=%dx%d kinds=%d"),
        Bounds.Min.X, Bounds.Min.Y, Bounds.Max.X, Bounds.Max.Y,
        Data->ChunkCountX, Data->ChunkCountY, Data->Kinds.Num());
}

// Touceiras de grama montadas por código: 12 variantes (rasteira, de prado, alta com espigas), cada
// uma uma touceira de folhas finas. Malhas pequenas, sem Nanite.
void AVortexTerrainStreamer::BuildGrassMeshes()
{
    GrassMeshes.Reset();
    // 10/11: a grama da Quaternius (Tools/ImportGrassPack.py). As 12 variantes apontam para os 4 tufos: o tipo de cada
    // variante (rasteira, prado, alta) continua o mesmo, e com ele a distância de sumiço (SetPieceDistances acha a
    // primeira variante do tufo). -VortexOldGrass volta às touceiras feitas por código
    if (!FParse::Param(FCommandLine::Get(), TEXT("VortexOldGrass")))
    {
        const auto Load = [](const TCHAR* Name) { return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Nature/Grass/SM_GrassPack_%s.SM_GrassPack_%s"), Name, Name)); };
        UStaticMesh* Lawn = Load(TEXT("Rasteira"));
        UStaticMesh* Fine = Load(TEXT("PradoFina"));
        UStaticMesh* Meadow = Load(TEXT("PradoAlta"));
        UStaticMesh* Tall = Load(TEXT("Alta"));
        if (Lawn && Fine && Meadow && Tall)
        {
            GrassMeshes = { Lawn, Lawn, Lawn, Lawn, Fine, Meadow, Fine, Meadow, Fine, Tall, Tall, Tall };
            return;
        }
    }
    for (int32 Variant = 0; Variant < GrassVariants; ++Variant)
    {
        TArray<FVector> Vertices, Normals;
        TArray<int32> Triangles;
        TArray<FVector4f> BladeData;
        AddGrassClump(GrassKindOf(Variant), 7919u * (Variant + 1) + 13u, Vertices, Normals, Triangles, BladeData);
        FMeshDescription Description;
        FStaticMeshAttributes Attributes(Description);
        Attributes.Register();
        TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
        TVertexInstanceAttributesRef<FVector3f> InstanceNormals = Attributes.GetVertexInstanceNormals();
        TVertexInstanceAttributesRef<FVector2f> InstanceUVs = Attributes.GetVertexInstanceUVs();
        InstanceUVs.SetNumChannels(2);
        const FPolygonGroupID Group = Description.CreatePolygonGroup();
        Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Grass");
        TArray<FVertexInstanceID> Instances;
        for (int32 i = 0; i < Vertices.Num(); ++i)
        {
            const FVertexID Vertex = Description.CreateVertex();
            Positions[Vertex] = FVector3f(Vertices[i]);
            const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertex);
            InstanceNormals[Instance] = FVector3f(Normals[i]);
            // UV0 = (sorteio da folha, altura), UV1 = (espiga, brilho): lidos pelo M_GrassWind
            InstanceUVs.Set(Instance, 0, FVector2f(BladeData[i].X, BladeData[i].W));
            InstanceUVs.Set(Instance, 1, FVector2f(BladeData[i].Y, BladeData[i].Z));
            Instances.Add(Instance);
        }
        for (int32 i = 0; i + 2 < Triangles.Num(); i += 3)
        {
            Description.CreateTriangle(Group, { Instances[Triangles[i]], Instances[Triangles[i + 1]], Instances[Triangles[i + 2]] });
        }
        UStaticMesh* Mesh = NewObject<UStaticMesh>(this);
        Mesh->GetStaticMaterials().Add(FStaticMaterial(GrassMaterial ? GrassMaterial.Get() : TerrainMaterial.Get(), TEXT("Grass")));
        UStaticMesh::FBuildMeshDescriptionsParams Params;
        Params.bFastBuild = true;
        Params.bBuildSimpleCollision = false;
        Params.bCommitMeshDescription = false;
        Mesh->BuildFromMeshDescriptions({ &Description }, Params);
        // malha feita no código: sem a densidade de UV, o streaming de texturas dispara um ensure (no pacote de medição o
        // jogo travava nele)
        for (FStaticMaterial& Slot : Mesh->GetStaticMaterials()) Slot.UVChannelData = FMeshUVChannelInfo(1.f);
        GrassMeshes.Add(Mesh);
    }
}

void AVortexTerrainStreamer::EndPlay(const EEndPlayReason::Type Reason)
{
    for (auto& Pair : Building) Pair.Value.Task.Wait();
    Building.Empty();
    if (UVortexWaterSubsystem* Water = GetWorld() ? GetWorld()->GetSubsystem<UVortexWaterSubsystem>() : nullptr)
    {
        Water->Provider = nullptr;
    }
    Super::EndPlay(Reason);
}

FIntPoint AVortexTerrainStreamer::ChunkAt(const FVector& Location) const
{
    const float Size = Data ? Data->ChunkSize() : 1800.f;
    return FIntPoint(FMath::FloorToInt(Location.X / Size), FMath::FloorToInt(Location.Y / Size));
}

// Servidor: todos os controladores (remotos inclusive) são simulados aqui. Cliente: só o local
// existe. "Local" é quem desenha a tela nesta máquina; o servidor dedicado não tem nenhum.
void AVortexTerrainStreamer::GatherViewers(FViewers& Out, TArray<FVector>& OutLocal, TArray<FVector>& OutPlayers) const
{
    const bool bDedicated = GetNetMode() == NM_DedicatedServer;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* Controller = It->Get();
        // no menu o jogador é espectador (sem pawn): o chão é montado em volta do espectador
        const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
        if (!Pawn && Controller) Pawn = Controller->GetSpectatorPawn();
        if (!Pawn) continue;
        const FVector At = Pawn->GetActorLocation();
        OutPlayers.Add(At);
        Out.Players.AddUnique(ChunkAt(At));
        if (!bDedicated && Controller->IsLocalController())
        {
            OutLocal.Add(At);
            Out.Local.AddUnique(ChunkAt(At));
        }
    }
    // ordem estável: a comparação com o quadro anterior não depende da ordem dos controladores
    auto Less = [](const FIntPoint& A, const FIntPoint& B) { return A.X != B.X ? A.X < B.X : A.Y < B.Y; };
    Out.Local.Sort(Less);
    Out.Players.Sort(Less);
}

int32 AVortexTerrainStreamer::ChebyshevTo(const FIntPoint& Key, const TArray<FIntPoint>& Centers)
{
    int32 Best = MAX_int32;
    for (const FIntPoint& C : Centers) Best = FMath::Min(Best, FMath::Max(FMath::Abs(Key.X - C.X), FMath::Abs(Key.Y - C.Y)));
    return Best;
}

// O que um pedaço já montado pode manter, com um pedaço de folga além da área desejada (evita
// montar e soltar o mesmo pedaço quando alguém anda na divisa).
AVortexTerrainStreamer::ELevel AVortexTerrainStreamer::KeepLevel(const FIntPoint& Key) const
{
    if (ChebyshevTo(Key, LastViewers.Local) <= StreamRadius + 1) return ELevel::Full;
    if (ChebyshevTo(Key, LastViewers.Players) <= SimulationRadius + 1) return ELevel::Collision;
    return ELevel::None;
}

// Tela escura para quem desenha a tela aqui até não faltar nenhum pedaço da área; depois clareia
// em 0,6 s. -VortexNoInitialFade desliga (testes que precisam da imagem desde o início).
void AVortexTerrainStreamer::UpdateInitialFill(float DeltaSeconds)
{
    if (bInitialFillDone || LastViewers.Local.IsEmpty()) return;
    static const bool bDisabled = FParse::Param(FCommandLine::Get(), TEXT("VortexNoInitialFade"));
    TArray<APlayerController*> Locals;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (It->Get() && It->Get()->IsLocalController() && It->Get()->PlayerCameraManager) Locals.Add(It->Get());
    }
    if (!bInitialFillStarted)
    {
        bInitialFillStarted = true;
        if (!bDisabled) for (APlayerController* Controller : Locals) Controller->PlayerCameraManager->SetManualCameraFade(1.f, FLinearColor::Black, false);
    }
    InitialFillTime += DeltaSeconds;
    const bool bComplete = Pending.IsEmpty() && Building.IsEmpty() && TreeQueue.IsEmpty();
    if (!bComplete && InitialFillTime < InitialFillTimeout) return;
    bInitialFillDone = true;
    if (!bDisabled) for (APlayerController* Controller : Locals) Controller->PlayerCameraManager->StartCameraFade(1.f, 0.f, .6f, FLinearColor::Black);
    UE_LOG(LogTemp, Display, TEXT("Vortex terrain: INITIAL_FILL %.2f s chunks=%d completo=%d"), InitialFillTime, VisualChunkCount(), bComplete ? 1 : 0);
}

// Menor sai primeiro: o chão em volta de qualquer jogador vem antes da paisagem; depois, o mais
// perto de alguém.
int32 AVortexTerrainStreamer::Priority(const FIntPoint& Key) const
{
    int32 Nearest = MAX_int32;
    for (const FIntPoint& C : LastViewers.Players) Nearest = FMath::Min(Nearest, (Key - C).SizeSquared());
    for (const FIntPoint& C : LastViewers.Local) Nearest = FMath::Min(Nearest, (Key - C).SizeSquared());
    const bool bGround = ChebyshevTo(Key, LastViewers.Players) <= CollisionRadius;
    return (bGround ? 0 : 1000000) + FMath::Min(Nearest, 999999);
}

int32 AVortexTerrainStreamer::VisualChunkCount() const
{
    int32 Count = 0;
    for (const auto& Pair : Chunks) Count += Pair.Value.bVisual ? 1 : 0;
    return Count;
}

int32 AVortexTerrainStreamer::CollisionChunkCount() const
{
    int32 Count = 0;
    for (const auto& Pair : Chunks) Count += Pair.Value.Collision ? 1 : 0;
    return Count;
}

int32 AVortexTerrainStreamer::ChunksOutsideKeep() const
{
    int32 Count = 0;
    for (const auto& Pair : Chunks)
    {
        const ELevel Keep = KeepLevel(Pair.Key);
        // um pedaço visual que só precisa de chão espera na StripQueue (alguns por quadro)
        Count += Keep == ELevel::None || (Pair.Value.bVisual && Keep != ELevel::Full && !StripQueue.Contains(Pair.Key)) ? 1 : 0;
    }
    return Count;
}

bool AVortexTerrainStreamer::HasVisualChunk(const FIntPoint& Chunk) const
{
    const FChunk* Found = Chunks.Find(Chunk);
    return Found && Found->bVisual;
}

void AVortexTerrainStreamer::Tick(float DeltaSeconds)
{
    VORTEX_PROBE("Terreno");
    Super::Tick(DeltaSeconds);
    const double TickStart = FPlatformTime::Seconds();
    ON_SCOPE_EXIT
    {
        const double Ms = (FPlatformTime::Seconds() - TickStart) * 1000.0;
        TickMsMax = FMath::Max(TickMsMax, Ms);
        TickMsTotal += Ms;
        ++Ticks;
    };
    FViewers Viewers;
    TArray<FVector> LocalAt, PlayerAt;
    GatherViewers(Viewers, LocalAt, PlayerAt);
    if (Viewers.Players.IsEmpty())
    {
        return;
    }
    if (LocalAt.Num() > 0) UpdateGrassStep(LocalAt[0]);
    double PartStart = FPlatformTime::Seconds();
    const auto Part = [this, &PartStart](int32 Index)
    {
        const double Now = FPlatformTime::Seconds();
        PartMsMax[Index] = FMath::Max(PartMsMax[Index], (Now - PartStart) * 1000.0);
        PartStart = Now;
    };
    if (!bHasViewers || !(Viewers == LastViewers))
    {
        LastViewers = Viewers;
        bHasViewers = true;
        UpdateWanted(Viewers);
    }
    Part(0);
    // pedaços que saíram da área: no máximo RetirePerFrame soltos por quadro (um salto longe
    // soltava 100+ pedaços, com milhares de componentes, no mesmo quadro)
    for (int32 Count = 0; Count < RetirePerFrame && Retiring.Num() > 0; ++Count)
    {
        Release(Retiring.Last());
        Retiring.Pop(EAllowShrinking::No);
    }
    for (int32 Count = 0; Count < RetirePerFrame && StripQueue.Num() > 0; ++Count)
    {
        const FIntPoint Key = StripQueue.Pop(EAllowShrinking::No);
        FChunk* Chunk = Chunks.Find(Key);
        if (Chunk && Chunk->bVisual && KeepLevel(Key) != ELevel::Full) ReleaseVisual(*Chunk);
    }
    Part(1);
    UpdateInitialFill(DeltaSeconds);
    LaunchAndAttach();
    Part(2);
    UpdateCollision(Viewers.Players);
    Part(3);
#if !UE_BUILD_SHIPPING
    // Fase 13, cliente: avisa quando o chão e a malha do pedaço do jogador local estão prontos
    static const bool bStreamSmoke = FParse::Param(FCommandLine::Get(), TEXT("VortexStreamSmoke"));
    if (bStreamSmoke && GetNetMode() == NM_Client && LocalAt.Num() > 0)
    {
        float Ground = 0.f;
        const FIntPoint Here = ChunkAt(LocalAt[0]);
        if (Here != SmokeReportedChunk && HasVisualChunk(Here) && GroundReadyAt(LocalAt[0].X, LocalAt[0].Y, Ground))
        {
            SmokeReportedChunk = Here;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_STREAM_CLIENT_READY at=(%.0f,%.0f) chunks=%d visual=%d ground=%d outside=%d stale=%d tick_ms_max=%.2f attach_ms_max=%.2f partes_ms_max=%.1f/%.1f/%.1f/%.1f/%.1f mem_mb=%.0f"),
                LocalAt[0].X / 100.f, LocalAt[0].Y / 100.f, LoadedChunkCount(), VisualChunkCount(), CollisionChunkCount(),
                ChunksOutsideKeep(), StaleDiscarded, TickMsMax, ChunkGameThreadMsMax,
                PartMsMax[0], PartMsMax[1], PartMsMax[2], PartMsMax[3], PartMsMax[4],
                FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0));
        }
        // memória a cada 5 s, logo depois de uma coleta forçada (os componentes soltos só saem
        // da memória na coleta de lixo)
        SmokeStatsTime += DeltaSeconds;
        if (SmokeStatsTime >= 5.f)
        {
            SmokeStatsTime = 0.f;
            // devolve ao sistema o que o alocador guardou depois da coleta anterior: o que sobra é uso
            FMemory::Trim(true);
            int32 MeshObjects = 0;
            for (TObjectIterator<UProceduralMeshComponent> It; It; ++It) MeshObjects += IsValid(*It) ? 1 : 0;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_STREAM_CLIENT_STATS chunks=%d visual=%d retiring=%d trees=%d stale=%d componentes=%d malhas_vivas=%d mem_mb=%.0f"),
                LoadedChunkCount(), VisualChunkCount(), Retiring.Num(), TreeRegionCount(), StaleDiscarded,
                GetComponents().Num(), MeshObjects, FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0));
            if (GEngine) GEngine->ForceGarbageCollection(true);
        }
    }
#endif
    if (!bSolidProbeReported && FParse::Param(FCommandLine::Get(), TEXT("VortexSolidProbe")))
    {
        SolidProbeTime += DeltaSeconds;
        if (SolidProbeTime >= 4.f)
        {
            int32 ActiveChunks = 0, Bodies = 0;
            bool bHit = false;
            for (const auto& Pair : Chunks)
            {
                if (Pair.Value.SolidBodies.IsEmpty()) continue;
                ++ActiveChunks;
                Bodies += Pair.Value.SolidBodies.Num();
                const TArray<FVortexTerrainSolid>* Specs = Data->SolidsInChunk(Pair.Key);
                if (!Specs) continue;
                for (const FVortexTerrainSolid& Spec : *Specs)
                {
                    if (Spec.Shape != 0 || Spec.B < 200.f) continue;
                    FHitResult Hit;
                    const FVector Start = Spec.Location + FVector(0, 0, Spec.B * 0.5f + 60.f);
                    const bool bTraced = GetWorld()->LineTraceSingleByChannel(Hit, Start, Spec.Location, ECC_Visibility);
                    bHit = bTraced && Hit.Component.IsValid() && Hit.Component->ComponentHasTag(TEXT("VortexSolid"));
                    if (bHit) break;
                }
                if (bHit) break;
            }
            if (bHit) { UE_LOG(LogTemp, Display, TEXT("Vortex terrain: SOLID_PROBE PASS chunks=%d bodies=%d"), ActiveChunks, Bodies); }
            else { UE_LOG(LogTemp, Error, TEXT("Vortex terrain: SOLID_PROBE FAIL chunks=%d bodies=%d"), ActiveChunks, Bodies); }
            bSolidProbeReported = true;
        }
    }
    TArray<FIntPoint> TreeCenters;
    const float RegionSize = Data->ChunkSize() * 4;
    for (const FVector& P : LocalAt)
    {
        TreeCenters.AddUnique(FIntPoint(FMath::FloorToInt(P.X / RegionSize), FMath::FloorToInt(P.Y / RegionSize)));
    }
    TreeCenters.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X != B.X ? A.X < B.X : A.Y < B.Y; });
    UpdateTrees(TreeCenters);
    Part(4);
}

// Pés dos 8 personagens mais perto da câmera local (inclusive jogadores remotos, que no cliente
// não têm controlador aqui) vão para o material da grama, que se abre e deita em volta deles.
void AVortexTerrainStreamer::UpdateGrassStep(const FVector& Viewer)
{
    UMaterialParameterCollectionInstance* Step = GrassStep ? GetWorld()->GetParameterCollectionInstance(GrassStep) : nullptr;
    if (!Step) return;
    TArray<FVector, TInlineAllocator<16>> Feet;
    // -VortexNoGrassStep: desliga a pisada (comparação nas fotos de teste)
    static const bool bDisabled = FParse::Param(FCommandLine::Get(), TEXT("VortexNoGrassStep"));
    for (TActorIterator<ACharacter> It(GetWorld()); It && !bDisabled; ++It)
    {
        const ACharacter* Character = *It;
        const float Half = Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 70.f;
        Feet.Add(Character->GetActorLocation() - FVector(0.f, 0.f, Half));
    }
    Feet.Sort([&Viewer](const FVector& A, const FVector& B) { return FVector::DistSquared(A, Viewer) < FVector::DistSquared(B, Viewer); });
    static const FName Names[] = { TEXT("Player0"), TEXT("Player1"), TEXT("Player2"), TEXT("Player3"),
        TEXT("Player4"), TEXT("Player5"), TEXT("Player6"), TEXT("Player7") };
    for (int32 i = 0; i < UE_ARRAY_COUNT(Names); ++i)
    {
        // só manda quando o pé andou (1 cm): parado, nada muda na grama
        const FVector Foot = Feet.IsValidIndex(i) ? Feet[i] : FVector(0.f, 0.f, -100000.f);
        if (FVector::DistSquared(Foot, GrassFeet[i]) < 1.f) continue;
        GrassFeet[i] = Foot;
        Step->SetVectorParameterValue(Names[i], Feet.IsValidIndex(i) ? FLinearColor(Foot.X, Foot.Y, Foot.Z, 1.f) : FLinearColor(0.f, 0.f, -100000.f, 0.f));
    }
}

// União das áreas: completo em volta de quem desenha a tela, só chão em volta de cada jogador
// simulado aqui. Refeito quando algum jogador muda de pedaço, entra ou sai.
void AVortexTerrainStreamer::UpdateWanted(const FViewers& Viewers)
{
    Wanted.Reset();
    for (const FIntPoint& Center : Viewers.Local)
    for (int32 dx = -StreamRadius; dx <= StreamRadius; ++dx)
    for (int32 dy = -StreamRadius; dy <= StreamRadius; ++dy)
    {
        const FIntPoint C = Center + FIntPoint(dx, dy);
        if (Data->HasChunk(C)) Wanted.Add(C, ELevel::Full);
    }
    for (const FIntPoint& Center : Viewers.Players)
    for (int32 dx = -SimulationRadius; dx <= SimulationRadius; ++dx)
    for (int32 dy = -SimulationRadius; dy <= SimulationRadius; ++dy)
    {
        const FIntPoint C = Center + FIntPoint(dx, dy);
        if (Data->HasChunk(C) && !Wanted.Contains(C)) Wanted.Add(C, ELevel::Collision);
    }
    Pending.Reset();
    for (const TPair<FIntPoint, ELevel>& Pair : Wanted)
    {
        const bool bNeedVisual = Pair.Value == ELevel::Full;
        const FChunk* Chunk = Chunks.Find(Pair.Key);
        const FBuild* Build = Building.Find(Pair.Key);
        if (Chunk && (Chunk->bVisual || !bNeedVisual)) continue;
        if (Build && (Build->bVisual || !bNeedVisual)) continue;
        Pending.Add(Pair.Key);
    }
    Pending.Sort([this](const FIntPoint& A, const FIntPoint& B) { return Priority(A) < Priority(B); });
    // fora da área com folga: solta tudo, ou só a parte visual se ainda há alguém simulado perto
    // (a destruição dos componentes vai para filas, alguns por quadro; o pedaço já sai da lista)
    TArray<FIntPoint> Drop;
    for (auto& Pair : Chunks)
    {
        const ELevel Keep = KeepLevel(Pair.Key);
        if (Keep == ELevel::None) Drop.Add(Pair.Key);
        else if (Keep == ELevel::Collision && Pair.Value.bVisual) StripQueue.AddUnique(Pair.Key);
    }
    for (const FIntPoint& Key : Drop)
    {
        Retiring.Add(MoveTemp(Chunks[Key]));
        Chunks.Remove(Key);
    }
}

void AVortexTerrainStreamer::LaunchAndAttach()
{
    // tarefas novas (várias threads), na ordem da prioridade; um pedaço que ainda tem tarefa
    // (por exemplo, só chão e agora precisa da malha) espera a anterior terminar
    const int32 MaxBuilding = FMath::Max(2, FPlatformMisc::NumberOfCoresIncludingHyperthreads() / 2);
    for (int32 i = 0; i < Pending.Num() && Building.Num() < MaxBuilding;)
    {
        const FIntPoint Key = Pending[i];
        if (Building.Contains(Key)) { ++i; continue; }
        const bool bVisual = Wanted.FindRef(Key) == ELevel::Full;
        Pending.RemoveAt(i);
        FBuild& Build = Building.Add(Key);
        Build.bVisual = bVisual;
        Build.Task = UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, Key, bVisual]() { return BuildMesh(Key, bVisual); });
    }
    // entrega as prontas, mais importantes primeiro, com orçamento por quadro: ChunksPerFrame
    // pedaços com malha e até 4 só de chão (estes só guardam os dados; a colisão vem depois)
    TArray<FIntPoint> Ready;
    for (const auto& Pair : Building) if (Pair.Value.Task.IsCompleted()) Ready.Add(Pair.Key);
    Ready.Sort([this](const FIntPoint& A, const FIntPoint& B) { return Priority(A) < Priority(B); });
    int32 VisualAttached = 0, GroundAttached = 0;
    for (const FIntPoint& Key : Ready)
    {
        FBuild& Build = Building.FindChecked(Key);
        const ELevel Want = Wanted.FindRef(Key);
        if (Want == ELevel::None)
        {
            // a área mudou enquanto a tarefa rodava: não entrega pedaço fora dela
            ++StaleDiscarded;
            Building.Remove(Key);
            continue;
        }
        const bool bVisual = Build.bVisual && Want == ELevel::Full;
        const int32 VisualBudget = bInitialFillStarted && !bInitialFillDone ? InitialChunksPerFrame : ChunksPerFrame;
        if (bVisual ? VisualAttached >= VisualBudget : GroundAttached >= 4) continue;
        Attach(Key, Build.Task.GetResult(), bVisual);
        ++(bVisual ? VisualAttached : GroundAttached);
        Building.Remove(Key);
    }
}

TSharedPtr<AVortexTerrainStreamer::FChunkMesh> AVortexTerrainStreamer::BuildMesh(FIntPoint Chunk, bool bVisual) const
{
    TSharedPtr<FChunkMesh> Mesh = MakeShared<FChunkMesh>();
    Mesh->bVisual = bVisual;
    Mesh->Collision = MakeShared<FChunkCollision>();
    const float Density = GrassDensity;
    const FVortexTerrainData& D = *Data;
    const int32 N = D.CellsPerChunk;
    const float S = D.CellSize;
    const int32 Cx0 = Chunk.X * N, Cy0 = Chunk.Y * N;
    if (bVisual)
    {
        Mesh->Vertices.Reserve(N * N * 8);
        Mesh->UV0.Reserve(N * N * 8);
    }
    for (int32 ix = 0; ix < N && bVisual; ++ix)
    for (int32 iy = 0; iy < N; ++iy)
    {
        const int32 Gx = Cx0 + ix, Gy = Cy0 + iy;
        const float H = D.CellHeight(Gx, Gy);
        const FLinearColor Top = D.CellTint(Gx, Gy);
        const uint8 Layer = D.CellLayer(Gx, Gy);
        const uint8 Rotation = D.CellRotation(Gx, Gy);
        const float X0 = Gx * S, Y0 = Gy * S, X1 = X0 + S, Y1 = Y0 + S;
        AddQuad(Mesh->Vertices, Mesh->Normals, Mesh->Triangles, &Mesh->Colors,
            FVector(X0, Y0, H), FVector(X0, Y1, H), FVector(X1, Y1, H), FVector(X1, Y0, H), FVector::UpVector, Top);
        for (const FVector2D UV : { FVector2D(0, 0), FVector2D(0, 1), FVector2D(1, 1), FVector2D(1, 0) })
            Mesh->UV0.Add(AtlasUV(Layer, RotateTileUV(UV, Rotation)));
        // 10/11: sombra de canto dos jogos de bloco: cada canto do topo escurece conforme as vizinhas mais altas que encostam
        // nele (lado, lado e diagonal; os dois lados = canto fechado). Fica na cor dos vértices, sem textura nova
        {
            const auto Higher = [&](int32 Dx, int32 Dy) { return D.CellHeight(Gx + Dx, Gy + Dy) > H + 1.f; };
            const auto Corner = [&](int32 Dx, int32 Dy)
            {
                static const float Shade[4] = { 1.f, .8f, .68f, .56f };
                const bool bA = Higher(Dx, 0), bB = Higher(0, Dy);
                return Shade[bA && bB ? 3 : int32(bA) + int32(bB) + int32(Higher(Dx, Dy))];
            };
            const float Ao[4] = { Corner(-1, -1), Corner(-1, 1), Corner(1, 1), Corner(1, -1) };   // os cantos na ordem do AddQuad
            const int32 First = Mesh->Colors.Num() - 4;
            for (int32 k = 0; k < 4; ++k) Mesh->Colors[First + k] = FLinearColor(Top.R * Ao[k], Top.G * Ao[k], Top.B * Ao[k], Top.A);
        }
        // paredões onde a vizinha é mais baixa (terra escura por baixo, como os lados do Godot)
        const FLinearColor Side = Top * .9f;
        struct FDir { int32 Dx, Dy; FVector N; };
        static const FDir Dirs[] = { { -1, 0, FVector(-1, 0, 0) }, { 1, 0, FVector(1, 0, 0) }, { 0, -1, FVector(0, -1, 0) }, { 0, 1, FVector(0, 1, 0) } };
        for (const FDir& Dir : Dirs)
        {
            const float NH = D.CellHeight(Gx + Dir.Dx, Gy + Dir.Dy);
            if (NH >= H) continue;
            const float Xe = Dir.Dx < 0 ? X0 : X1, Ye = Dir.Dy < 0 ? Y0 : Y1;
            // 07/11: paredão alto (os penhascos dos lugares marcantes): em faixas de uma célula, para a textura repetir
            // em vez de esticar; embaixo da primeira faixa, rocha (a não ser na neve, na areia e na areia vermelha)
            const uint8 Lip = SideLayer(Layer);
            const bool bTall = H - NH > 2.5f * S;
            const int32 Bands = bTall ? FMath::CeilToInt((H - NH) / S) : 1;
            for (int32 Band = 0; Band < Bands; ++Band)
            {
                const float Hi = H - (H - NH) * Band / Bands, Lo = H - (H - NH) * (Band + 1) / Bands;
                if (Dir.Dx != 0)
                {
                    AddQuad(Mesh->Vertices, Mesh->Normals, Mesh->Triangles, &Mesh->Colors,
                        FVector(Xe, Y0, Lo), FVector(Xe, Y1, Lo), FVector(Xe, Y1, Hi), FVector(Xe, Y0, Hi), Dir.N, Side);
                }
                else
                {
                    AddQuad(Mesh->Vertices, Mesh->Normals, Mesh->Triangles, &Mesh->Colors,
                        FVector(X0, Ye, Lo), FVector(X1, Ye, Lo), FVector(X1, Ye, Hi), FVector(X0, Ye, Hi), Dir.N, Side);
                }
                // 10/11: a parede escurece descendo (até 40% a 3 blocos da quina) e um pouco mais rente ao chão de baixo
                {
                    const auto Depth = [&](float Z) { return 1.f - .4f * FMath::Clamp((H - Z) / (3.f * S), 0.f, 1.f); };
                    const float Low = Depth(Lo) * (Band == Bands - 1 ? .8f : 1.f), High = Depth(Hi);
                    const int32 First = Mesh->Colors.Num() - 4;
                    const float Wall4[4] = { Low, Low, High, High };   // os dois de baixo (Lo) e os dois de cima (Hi)
                    for (int32 k = 0; k < 4; ++k) Mesh->Colors[First + k] = FLinearColor(Side.R * Wall4[k], Side.G * Wall4[k], Side.B * Wall4[k], Side.A);
                }
                const uint8 Wall = Band == 0 || Lip == 19 || Lip == 20 || Lip == 10 ? Lip : 9;
                Mesh->UV0.Append({ AtlasUV(Wall, FVector2D(0, 1)), AtlasUV(Wall, FVector2D(1, 1)),
                    AtlasUV(Wall, FVector2D(1, 0)), AtlasUV(Wall, FVector2D(0, 0)) });
            }
        }
        if (H < D.WaterLevel)
        {
            const int32 Start = Mesh->WaterVertices.Num();
            AddQuad(Mesh->WaterVertices, Mesh->WaterNormals, Mesh->WaterTriangles, nullptr,
                FVector(X0, Y0, D.WaterSurface), FVector(X0, Y1, D.WaterSurface), FVector(X1, Y1, D.WaterSurface), FVector(X1, Y0, D.WaterSurface),
                FVector::UpVector, FLinearColor::White);
            // UV no mundo (uma repetição a cada 10 m): as ondas do M_Lake ficam do mesmo tamanho em
            // todo lago, sem emenda entre células e pedaços
            for (int32 Corner = Start; Corner < Mesh->WaterVertices.Num(); ++Corner)
            {
                const FVector& V = Mesh->WaterVertices[Corner];
                Mesh->WaterUV0.Add(FVector2D(V.X / 1000.f, V.Y / 1000.f));
            }
        }
    }
    // colisão suave: canto = média das quatro células em volta
    const int32 C = N + 1;
    Mesh->Collision->Vertices.Reserve(C * C);
    for (int32 ix = 0; ix < C; ++ix)
    for (int32 iy = 0; iy < C; ++iy)
    {
        const int32 Gx = Cx0 + ix, Gy = Cy0 + iy;
        const float H = (D.CellHeight(Gx - 1, Gy - 1) + D.CellHeight(Gx, Gy - 1) + D.CellHeight(Gx - 1, Gy) + D.CellHeight(Gx, Gy)) * 0.25f;
        Mesh->Collision->Vertices.Add(FVector(Gx * S, Gy * S, H));
    }
    for (int32 ix = 0; ix < N; ++ix)
    for (int32 iy = 0; iy < N; ++iy)
    {
        const int32 A = ix * C + iy, B = ix * C + iy + 1, Cc = (ix + 1) * C + iy + 1, Dd = (ix + 1) * C + iy;
        Mesh->Collision->Triangles.Append({ A, B, Cc, A, Cc, Dd });
    }
    // mesma regra de frente dos quadros do topo (normal para cima)
    TArray<FVector>& CollisionVertices = Mesh->Collision->Vertices;
    TArray<int32>& CollisionTriangles = Mesh->Collision->Triangles;
    if (CollisionTriangles.Num() >= 3)
    {
        const FVector& P0 = CollisionVertices[CollisionTriangles[0]];
        const FVector& P1 = CollisionVertices[CollisionTriangles[1]];
        const FVector& P2 = CollisionVertices[CollisionTriangles[2]];
        if (FVector::DotProduct(FVector::CrossProduct(P1 - P0, P2 - P0), FVector::UpVector) > 0.f)
        {
            for (int32 i = 0; i < CollisionTriangles.Num(); i += 3) Swap(CollisionTriangles[i + 1], CollisionTriangles[i + 2]);
        }
    }
    // só chão (servidor, jogador remoto): sem objetos nem grama
    if (!bVisual) return Mesh;
    // cada pedaço usa só 6 das 12 variantes (2 rasteiras, 3 de prado, 1 alta), sorteadas pelo
    // pedaço: a variedade aparece de um pedaço para outro com metade dos componentes (desenhos)
    const uint32 ChunkSeed = HashCombineFast(GetTypeHash(Chunk.X * 7331), GetTypeHash(Chunk.Y * 1597));
    const int32 LawnPick[2] = { int32(ChunkSeed % 4), int32((ChunkSeed % 4 + 1 + (ChunkSeed >> 4) % 3) % 4) };
    const int32 MeadowSkip[2] = { int32((ChunkSeed >> 8) % 5), int32(((ChunkSeed >> 8) % 5 + 1 + (ChunkSeed >> 12) % 4) % 5) };
    const int32 TallPick = 9 + int32((ChunkSeed >> 16) % 3);
    const auto Remap = [&](int32 Variant)
    {
        if (Variant < 4) return LawnPick[Variant % 2];
        if (Variant >= 9) return TallPick;
        int32 Meadow = Variant - 4;
        while (Meadow == MeadowSkip[0] || Meadow == MeadowSkip[1]) Meadow = (Meadow + 1) % 5;
        return 4 + Meadow;
    };
    // cada cópia de grama leva 4 números para o material: cor do chão ali (linear) e secura da região
    const auto AddGrass = [&D, &Mesh, &Remap, S, Density](int32 Variant, const FTransform& Pose)
    {
        const FVector At = Pose.GetLocation();
        // densidade da vegetação: cada tufo fica ou sai sempre igual (pelo lugar), sem piscar
        // cidade: nada de grama nas pedras da praça e das ruas nem dentro das construções
        if (AVortexTown::BlocksGrass(At.X, At.Y)) return;
        // 11/11: na frente dos portões só a grama baixa (a alta e as flores tampavam a visão)
        if (Variant >= 4 && AVortexTown::Clears(At.X, At.Y)) return;
        // a cratera do Azhûr: o chão é o de basalto dela (UVortexCalderaSubsystem)
        if (VortexWorldLayout::CalderaClearsVegetation(At.X, At.Y)) return;
        if (Density < .999f && (HashCombineFast(GetTypeHash(FMath::RoundToInt(At.X)), GetTypeHash(FMath::RoundToInt(At.Y))) % 1000u) >= uint32(Density * 1000.f)) return;
        const int32 Gx = FMath::FloorToInt(At.X / S), Gy = FMath::FloorToInt(At.Y / S);
        const FLinearColor Ground = FLinearColor::FromSRGBColor(D.CellColor(Gx, Gy));
        const float Dry = CellNoise(Gx, Gy, 26.f, 311) * .7f + CellNoise(Gx, Gy, 7.f, 913) * .3f;
        const int32 Used = Remap(Variant);
        Mesh->GrassGroups[Used].Add(Pose);
        Mesh->GrassData[Used].Append({ Ground.R, Ground.G, Ground.B, Dry });
    };
    // objetos por espécie e tufos de preenchimento (só leitura dos dados: pode rodar aqui fora)
    if (const TArray<FVortexTerrainProp>* Props = D.PropsInChunk(Chunk))
    {
        for (const FVortexTerrainProp& Prop : *Props)
        {
            if (D.IsTree(Prop.Kind)) continue;
            if (VortexWorldLayout::CalderaClearsVegetation(Prop.Location.X, Prop.Location.Y)) continue;   // a cratera do Azhûr: só basalto
            if (AVortexTown::Contains(Prop.Location) || AVortexTown::Clears(Prop.Location.X, Prop.Location.Y)) continue;   // 03/11: dentro da vila, só o que a vila põe; 11/11: e a frente dos portões limpa
            // a grama voxel do Godot vira a grama com vento no mesmo lugar (alta, com semente, baixa)
            const FString& Name = D.Kinds[Prop.Kind];
            if (Name == TEXT("grass_short") || Name == TEXT("grass_tall") || Name == TEXT("grass_seed"))
            {
                const uint32 Seed = HashCombineFast(GetTypeHash(Prop.Location.X), GetTypeHash(Prop.Location.Y));
                // baixa -> rasteira, alta -> de prado, com semente -> alta com espigas
                const int32 Variant = Name == TEXT("grass_seed") ? 9 + Seed % 3 : Name == TEXT("grass_tall") ? 4 + Seed % 5 : Seed % 4;
                AddGrass(Variant, FTransform(FRotator(0.f, Prop.YawDegrees, 0.f), Prop.Location, FVector(Prop.Scale * (0.9f + (Seed % 5) * 0.06f))));
                continue;
            }
            // flores, cogumelos, samambaias e moitas: cada cópia com tamanho, altura e inclinação um pouco
            // diferentes (sempre iguais no mesmo lugar); pedras e construções ficam como vieram
            static const TCHAR* Soft[] = { TEXT("flower"), TEXT("bloom"), TEXT("poppy"), TEXT("dandelion"), TEXT("clover"), TEXT("fern"),
                TEXT("mushroom"), TEXT("shroom"), TEXT("glowcap"), TEXT("sunflower"), TEXT("wheat"), TEXT("reeds"), TEXT("bush"), TEXT("brush"), TEXT("shrub") };
            bool bSoft = false;
            for (const TCHAR* Word : Soft) if (Name.Contains(Word)) { bSoft = true; break; }
            if (bSoft)
            {
                const uint32 Seed = HashCombineFast(GetTypeHash(FMath::RoundToInt(Prop.Location.X)), GetTypeHash(FMath::RoundToInt(Prop.Location.Y)) * 31u);
                const float Size = .78f + (Seed % 1000) / 999.f * .5f;          // 0,78x .. 1,28x
                const float Tall = .88f + ((Seed >> 10) % 1000) / 999.f * .3f;  // mais baixa ou mais esticada
                const float Lean = (((Seed >> 20) % 100) / 99.f - .5f) * 10.f;  // até 5 graus
                // flores coloridas: 1 de 4 cores pelo lugar (a 0 é a original; as outras vão em lotes à parte)
                static const TCHAR* Petals[] = { TEXT("flowers_tulip"), TEXT("flowers_bell"), TEXT("flowers_lavender"), TEXT("poppy"),
                    TEXT("alpine_bloom"), TEXT("elf_bloom"), TEXT("mushroom_red") };
                uint16 Key = Prop.Kind;
                for (const TCHAR* Petal : Petals) if (Name == Petal && Prop.Kind < 4096) { Key = uint16(Prop.Kind + ((Seed >> 24) % 4) * 4096); break; }
                Mesh->PropGroups.FindOrAdd(Key).Add(FTransform(FRotator(Lean, Prop.YawDegrees, ((Seed >> 7) % 2 ? Lean : -Lean) * .6f), Prop.Location,
                    FVector(Prop.Scale * Size, Prop.Scale * Size, Prop.Scale * Size * Tall)));
                continue;
            }
            Mesh->PropGroups.FindOrAdd(Prop.Kind).Add(FTransform(FRotator(0.f, Prop.YawDegrees, 0.f), Prop.Location, FVector(Prop.Scale)));
        }
    }
    // Grama entre os modelos, só em chão verde e seco: de 1 a 6 touceiras por célula de 75 cm
    // conforme o ruído do campo (manchas densas, ralas e clareiras). O tipo segue o campo (rasteira
    // nas clareiras, de prado no meio, alta nas manchas mais densas), e cada cópia leva a cor do chão
    // daquele ponto e a secura da região: a grama nasce da cor do chão e varia em manchas grandes.
    for (int32 ix = 0; ix < N; ++ix)
    for (int32 iy = 0; iy < N; ++iy)
    {
        const int32 Gx = Cx0 + ix, Gy = Cy0 + iy;
        const float H = D.CellHeight(Gx, Gy);
        if (H < D.WaterLevel || H <= -299.f) continue;
        const FLinearColor Ground = FLinearColor::FromSRGBColor(D.CellColor(Gx, Gy));
        if (Ground.G < Ground.R * 1.05f || Ground.G < Ground.B * 1.2f) continue;
        const float Field = CellNoise(Gx, Gy, 10.f, 0) * .75f + CellNoise(Gx, Gy, 3.f, 77) * .25f;
        uint32 Seed = HashCombineFast(GetTypeHash(Gx), GetTypeHash(Gy * 7919));
        // rasteira: 1 ou 2 touceiras largas (60 cm) por célula, quase sempre (cobre o chão de perto,
        // com poucas cópias: cada cópia custa na montagem do pedaço);
        // de prado: 0 a 3 conforme o campo; alta: rara, nas manchas mais densas
        const int32 LawnClumps = Field < 0.3f ? 1 : 1 + (Seed % 2);
        const int32 Clumps = LawnClumps + (Field < 0.3f ? 0 : Field < 0.5f ? 1 : Field < 0.7f ? 2 : 3);
        for (int32 t = 0; t < Clumps; ++t)
        {
            Seed = Seed * 1664525u + 1013904223u;
            const FVector At(Gx * S + (Seed >> 8) % 75, Gy * S + (Seed >> 16) % 75, H);
            const float Roll = ((Seed >> 20) % 1000) / 999.f;
            const int32 Variant = t < LawnClumps ? (Seed >> 4) % 4
                : (Field > 0.6f && Roll > 0.82f) ? 9 + (Seed >> 4) % 3 : 4 + (Seed >> 4) % 5;
            const float Scale = (0.75f + (Seed % 11) * 0.05f) * (0.85f + Field * 0.35f);
            AddGrass(Variant, FTransform(FRotator(0.f, float((Seed >> 12) % 360), 0.f), At,
                FVector(Scale, Scale, Scale * (0.8f + Field * 0.45f) * (0.85f + ((Seed >> 6) % 7) * 0.05f))));
        }
    }
    return Mesh;
}

void AVortexTerrainStreamer::ApplyGraphicsSettings()
{
    const UVortexPlayerSettings* Settings = UVortexPlayerSettings::Get();
    if (!bWaterOverride && WaterLevels.Num() == 4) WaterMaterial = WaterLevels[FMath::Clamp(Settings->WaterQuality, 0, 3)];
    const float NewGrass = VortexGraphics::GrassDistanceScale(), NewProp = VortexGraphics::ViewDistanceScale();
    const bool bDistances = NewGrass != GrassScale || NewProp != PropScale;
    GrassScale = NewGrass;
    PropScale = NewProp;
    TreeScale = NewProp;
    // densidade nova: a parte visual dos pedaços é remontada (o streaming refaz, os mais perto primeiro)
    const float NewDensity = VortexGraphics::GrassDensity();
    if (!FMath::IsNearlyEqual(NewDensity, GrassDensity))
    {
        GrassDensity = NewDensity;
        for (TPair<FIntPoint, FChunk>& Pair : Chunks) if (Pair.Value.bVisual) ReleaseVisual(Pair.Value);
        // recalcula o que montar já no próximo quadro (antes só voltava quando o jogador mudava de pedaço,
        // e o chão e a grama sumiam até andar)
        bHasViewers = false;
    }
    if (bDistances)
    {
        for (TPair<FIntPoint, TArray<TObjectPtr<UInstancedStaticMeshComponent>>>& Region : TreeRegions)
            for (UInstancedStaticMeshComponent* Tree : Region.Value) if (Tree) Tree->SetCullDistances(0, TreeRange * TreeScale);
    }
    for (TPair<FIntPoint, FChunk>& Pair : Chunks)
    {
        UProceduralMeshComponent* Visual = Pair.Value.Visual;
        if (Visual && Visual->GetNumSections() > 1 && Visual->GetMaterial(1) != WaterMaterial) Visual->SetMaterial(1, WaterMaterial);
    }
    if (bDistances) for (TPair<FIntVector, FBatch>& Batch : Batches) SetPieceDistances(Batch.Value.Ism);
}

// distância de sumiço de um tufo de grama ou de uma planta/pedra já montados, com a escala das opções
void AVortexTerrainStreamer::SetPieceDistances(UInstancedStaticMeshComponent* Instances) const
{
    if (!Instances) return;
    const UStaticMesh* Mesh = Instances->GetStaticMesh();
    const int32 Variant = GrassMeshes.IndexOfByKey(Mesh);
    if (Variant != INDEX_NONE)
    {
        const float Range = (GrassKindOf(Variant) == EGrassKind::Lawn ? GrassRange * .45f : GrassRange) * GrassScale;
        Instances->SetCullDistances(Range * 0.8f, Range);
        return;
    }
    const int32 Kind = KindMeshes.IndexOfByKey(Mesh);
    if (Kind == INDEX_NONE || !Data || !Data->Kinds.IsValidIndex(Kind)) return;
    Instances->SetCullDistances(0, (IsSmallPlant(Data->Kinds[Kind]) ? SmallPropRange : PropRange) * PropScale);
}

UInstancedStaticMeshComponent* AVortexTerrainStreamer::BatchFor(const FIntPoint& Region, int32 Slot)
{
    FBatch& Batch = Batches.FindOrAdd(FIntVector(Region.X, Region.Y, Slot));
    if (Batch.Ism) return Batch.Ism;
    UInstancedStaticMeshComponent* Ism = nullptr;
    if (Slot >= GrassSlot && Slot < PetalSlot)
    {
        const int32 Variant = Slot - GrassSlot;
        if (!GrassMeshes.IsValidIndex(Variant) || !GrassMeshes[Variant]) return nullptr;
        Ism = NewObject<UInstancedStaticMeshComponent>(this);
        Ism->SetStaticMesh(GrassMeshes[Variant]);
        // 11/11: a grama importada (Quaternius) vem com campo de distância; com ~300 mil tufos, entrar na luz por campo de
        // distância custava ~6 ms por quadro na preparação do desenho (a feita por código não tinha campo)
        Ism->bAffectDistanceFieldLighting = false;
        Ism->bAffectDynamicIndirectLighting = false;
        Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Ism->SetCastShadow(false);
        Ism->SetCanEverAffectNavigation(false);
        Ism->SetNumCustomDataFloats(4);
        // some por cópia na placa de vídeo, com a distância; a rasteira (pequena e numerosa) mais cedo
        const float Range = (GrassKindOf(Variant) == EGrassKind::Lawn ? GrassRange * .45f : GrassRange) * GrassScale;
        Ism->SetCullDistances(Range * 0.8f, Range);
        Ism->bSupportRemoveAtSwap = true;
        Ism->SetupAttachment(RootComponent);
        Ism->RegisterComponent();
    }
    else
    {
        const int32 Kind = Slot >= PetalSlot ? (Slot - PetalSlot) / 4 : Slot;
        const bool bSmall = Data && Data->Kinds.IsValidIndex(Kind) && IsSmallPlant(Data->Kinds[Kind]);
        Ism = NewInstances(uint16(Kind), (bSmall ? SmallPropRange : PropRange) * PropScale, !bSmall);
        if (!Ism) return nullptr;
        if (Slot >= PetalSlot)
        {
            // a mesma flor com outra cor nas pétalas (um material por espécie e cor, dividido entre os lotes)
            TObjectPtr<UMaterialInstanceDynamic>& Petal = PetalMaterials.FindOrAdd(Slot - PetalSlot);
            if (!Petal && Ism->GetMaterial(0))
            {
                Petal = UMaterialInstanceDynamic::Create(Ism->GetMaterial(0), this);
                Petal->SetScalarParameterValue(TEXT("PetalHue"), float((Slot - PetalSlot) % 4));
            }
            if (Petal) Ism->SetMaterial(0, Petal);
        }
        Ism->bSupportRemoveAtSwap = true;
    }
    Batch.Ism = Ism;
    return Ism;
}

void AVortexTerrainStreamer::BatchAdd(FChunk& Chunk, int32 Slot, const TArray<FTransform>& Transforms, const TArray<float>* CustomData)
{
    if (Transforms.IsEmpty()) return;
    const FIntPoint Region = Chunk.BatchRegion;
    UInstancedStaticMeshComponent* Ism = BatchFor(Region, Slot);
    if (!Ism) return;
    FBatch& Batch = Batches[FIntVector(Region.X, Region.Y, Slot)];
    const int32 First = Ism->GetInstanceCount();
    // todas as cópias do pedaço de uma vez (uma atualização de render por lote)
    Ism->AddInstances(Transforms, false, true);
    Batch.Owner.Reserve(First + Transforms.Num());
    for (int32 i = 0; i < Transforms.Num(); ++i) Batch.Owner.Add(Chunk.Epoch);
    if (CustomData)
    {
        const int32 Floats = Ism->NumCustomDataFloats;
        for (int32 i = 0; i < Transforms.Num(); ++i)
            Ism->SetCustomData(First + i, TArrayView<const float>(CustomData->GetData() + i * Floats, Floats), false);
    }
}

void AVortexTerrainStreamer::BatchRemove(FChunk& Chunk)
{
    const FIntPoint Region = Chunk.BatchRegion;
    for (const int32 Slot : Chunk.BatchSlots)
    {
        const FIntVector BatchKey(Region.X, Region.Y, Slot);
        FBatch* Batch = Batches.Find(BatchKey);
        if (!Batch || !Batch->Ism) continue;
        // de trás para a frente: a cópia que vem do fim para o buraco nunca é deste pedaço
        for (int32 i = Batch->Owner.Num() - 1; i >= 0; --i)
        {
            if (Batch->Owner[i] != Chunk.Epoch) continue;
            Batch->Ism->RemoveInstance(i);
            Batch->Owner.RemoveAtSwap(i, EAllowShrinking::No);
        }
        if (Batch->Owner.IsEmpty())
        {
            Batch->Ism->DestroyComponent();
            Batches.Remove(BatchKey);
        }
    }
    Chunk.BatchSlots.Reset();
}

UInstancedStaticMeshComponent* AVortexTerrainStreamer::NewInstances(uint16 Kind, float CullDistance, bool bShadows)
{
    UStaticMesh* Mesh = KindMeshes.IsValidIndex(Kind) ? KindMeshes[Kind] : nullptr;
    if (!Mesh) return nullptr;
    UInstancedStaticMeshComponent* Instances = NewObject<UInstancedStaticMeshComponent>(this);
    // ISM simples: com Nanite a árvore de grupos do HISM só custava tempo na montagem
    Instances->SetStaticMesh(Mesh);
    Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Instances->SetCastShadow(bShadows);
    Instances->SetCullDistances(0, CullDistance);
    Instances->SetupAttachment(RootComponent);
    Instances->RegisterComponent();
    return Instances;
}

void AVortexTerrainStreamer::Attach(const FIntPoint& Key, const TSharedPtr<FChunkMesh>& Mesh, bool bVisual)
{
    const double Start = FPlatformTime::Seconds();
    // o pedaço pode já existir só com chão (jogador remoto passou antes): ganha a parte visual
    FChunk& Chunk = Chunks.FindOrAdd(Key);
    if (!Chunk.CollisionData) Chunk.CollisionData = Mesh->Collision;
    if (!bVisual || !Mesh->bVisual || Chunk.bVisual) return;
    Chunk.bVisual = true;
    UProceduralMeshComponent* Visual = NewObject<UProceduralMeshComponent>(this);
    Visual->SetupAttachment(RootComponent);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->bUseAsyncCooking = true;
    Visual->RegisterComponent();
    Visual->CreateMeshSection_LinearColor(0, Mesh->Vertices, Mesh->Triangles, Mesh->Normals, Mesh->UV0, Mesh->Colors, {}, false);
    Visual->SetMaterial(0, TerrainMaterial);
    if (Mesh->WaterVertices.Num() > 0)
    {
        Visual->CreateMeshSection_LinearColor(1, Mesh->WaterVertices, Mesh->WaterTriangles, Mesh->WaterNormals, Mesh->WaterUV0, {}, {}, false);
        Visual->SetMaterial(1, WaterMaterial);
    }
    Chunk.Visual = Visual;
    const double AfterGround = FPlatformTime::Seconds();
    double AfterProps = AfterGround;
    // plantas pequenas, moitas e pedras deste pedaço (árvores vão pelas regiões): entram no grupo da
    // região de cada espécie; os grupos e os tufos já vêm prontos da tarefa de fundo
    Chunk.BatchRegion = BatchRegion(Key);
    Chunk.Epoch = ++NextEpoch;
    for (const TPair<uint16, TArray<FTransform>>& Pair : Mesh->PropGroups)
    {
        const int32 Kind = Pair.Key % 4096, Petal = Pair.Key / 4096;
        const int32 Slot = Petal ? PetalSlot + Kind * 4 + Petal : Kind;
        BatchAdd(Chunk, Slot, Pair.Value, nullptr);
        Chunk.BatchSlots.AddUnique(Slot);
    }
    AfterProps = FPlatformTime::Seconds();
    for (int32 Variant = 0; Variant < GrassVariants; ++Variant)
    {
        if (Mesh->GrassGroups[Variant].IsEmpty() || !GrassMeshes.IsValidIndex(Variant) || !GrassMeshes[Variant]) continue;
        // cor do chão e secura de cada cópia (preparadas na tarefa de fundo) vão junto
        const bool bData = Mesh->GrassData[Variant].Num() == Mesh->GrassGroups[Variant].Num() * 4;
        BatchAdd(Chunk, GrassSlot + Variant, Mesh->GrassGroups[Variant], bData ? &Mesh->GrassData[Variant] : nullptr);
        Chunk.BatchSlots.AddUnique(GrassSlot + Variant);
    }
    const double End = FPlatformTime::Seconds();
    const double Ms = (End - Start) * 1000.0;
    ChunkGroundMs += (AfterGround - Start) * 1000.0;
    ChunkPropsMs += (AfterProps - AfterGround) * 1000.0;
    ChunkTuftMs += (End - AfterProps) * 1000.0;
    ++ChunksBuilt;
    ChunkGameThreadMsTotal += Ms;
    ChunkGameThreadMsMax = FMath::Max(ChunkGameThreadMsMax, Ms);
}

// Chão de colisão até SimulationRadius de qualquer jogador simulado aqui (no máximo
// CollisionMeshesPerFrame novos por quadro, o mais perto primeiro); corpos de árvores e rochas até
// CollisionRadius, um pedaço por quadro.
void AVortexTerrainStreamer::UpdateCollision(const TArray<FIntPoint>& Centers)
{
    TArray<FIntPoint> NeedGround, NeedSolids;
    for (auto& Pair : Chunks)
    {
        const int32 Distance = ChebyshevTo(Pair.Key, Centers);
        FChunk& Chunk = Pair.Value;
        if (Distance <= SimulationRadius && !Chunk.Collision && Chunk.CollisionData) NeedGround.Add(Pair.Key);
        const bool bSolids = Distance <= CollisionRadius;
        if (bSolids && Chunk.SolidBodies.IsEmpty())
        {
            const TArray<FVortexTerrainSolid>* Specs = Data->SolidsInChunk(Pair.Key);
            if (Specs && !Specs->IsEmpty()) NeedSolids.Add(Pair.Key);
        }
        if (!bSolids && Distance > CollisionRadius + 1 && !Chunk.SolidBodies.IsEmpty())
        {
            for (UPrimitiveComponent* Body : Chunk.SolidBodies) if (Body) Body->DestroyComponent();
            Chunk.SolidBodies.Reset();
        }
    }
    auto DistanceToPlayers = [&Centers](const FIntPoint& Key)
    {
        int32 Best = MAX_int32;
        for (const FIntPoint& Center : Centers) Best = FMath::Min(Best, (Key - Center).SizeSquared());
        return Best;
    };
    auto Nearer = [&DistanceToPlayers](const FIntPoint& A, const FIntPoint& B) { return DistanceToPlayers(A) < DistanceToPlayers(B); };
    NeedGround.Sort(Nearer);
    for (int32 i = 0; i < NeedGround.Num() && i < CollisionMeshesPerFrame; ++i)
    {
        FChunk& Chunk = Chunks.FindChecked(NeedGround[i]);
        UProceduralMeshComponent* Collision = MakeChunkCollision(TEXT("TerrainGround"), NeedGround[i]);
        Collision->SetupAttachment(RootComponent);
        Collision->bUseAsyncCooking = true;
        Collision->SetVisibility(false);
        Collision->SetCollisionProfileName(TEXT("BlockAll"));
        Collision->SetMobility(EComponentMobility::Static);
        Collision->RegisterComponent();
        Collision->CreateMeshSection(0, Chunk.CollisionData->Vertices, Chunk.CollisionData->Triangles, {}, {}, {}, {}, true);
        Chunk.Collision = Collision;
    }
    // Um pedaço de corpos por quadro; primeiro o do jogador. A cozinha dos corpos nunca compete
    // com dois pedaços visuais e dois lotes de obstáculos no mesmo quadro.
    NeedSolids.Sort(Nearer);
    if (!NeedSolids.IsEmpty()) CreateSolidCollision(NeedSolids[0], Chunks.FindChecked(NeedSolids[0]));
}

UProceduralMeshComponent* AVortexTerrainStreamer::MakeChunkCollision(const TCHAR* Kind, const FIntPoint& Key)
{
    // o herói em pé no terreno manda a "base" do movimento pela rede; com nome sorteado o componente não tinha
    // endereço de rede (~1000 avisos "ProceduralMeshComponent NOT Supported" por teste de rede)
    const FName Name(*FString::Printf(TEXT("%s_%d_%d"), Kind, Key.X, Key.Y));
    if (UObject* Old = StaticFindObjectFast(UProceduralMeshComponent::StaticClass(), this, Name))
    {
        Old->Rename(nullptr, this, REN_DontCreateRedirectors | REN_NonTransactional);
    }
    UProceduralMeshComponent* Component = NewObject<UProceduralMeshComponent>(this, Name);
    Component->SetNetAddressable();
    return Component;
}

void AVortexTerrainStreamer::CreateSolidCollision(const FIntPoint& Key, FChunk& Chunk)
{
    const TArray<FVortexTerrainSolid>* Solids = Data->SolidsInChunk(Key);
    if (!Solids || Solids->IsEmpty()) return;
    // Um corpo só por pedaço, com uma forma convexa por obstáculo (cozida fora da thread do
    // jogo). Antes era um componente por obstáculo, e registrar dezenas deles de uma vez pesava
    // nos piores quadros. Cilindros e esferas viram polígonos de 12 lados, como os do Godot; as
    // rochas usam a casca convexa exportada do modelo, não mais uma caixa envolvente.
    TArray<TArray<FVector>> Hulls;
    Hulls.Reserve(Solids->Num());
    constexpr int32 Sides = 12;
    for (const FVortexTerrainSolid& Solid : *Solids)
    {
        // 07/11 (o usuário: "blocos invisíveis no caminho"): dentro da vila não há mais as árvores e pedras do Godot
        // (AVortexTown::Contains as tira do desenho), então a colisão delas também sai
        if (AVortexTown::Contains(Solid.Location) || AVortexTown::Clears(Solid.Location.X, Solid.Location.Y)) continue;
        const FTransform Pose(FRotator(0.f, Solid.YawDegrees, 0.f), Solid.Location);
        TArray<FVector>& Points = Hulls.AddDefaulted_GetRef();
        if (Solid.Shape == 0)
        {
            const float R = FMath::Max(Solid.A, 1.f), H = FMath::Max(Solid.B * 0.5f, 1.f);
            for (int32 i = 0; i < Sides; ++i)
            {
                const float Angle = UE_TWO_PI * i / Sides;
                const FVector Ring(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R, 0.f);
                Points.Add(Pose.TransformPosition(Ring + FVector(0, 0, -H)));
                Points.Add(Pose.TransformPosition(Ring + FVector(0, 0, H)));
            }
        }
        else if (Solid.Shape == 1)
        {
            const float R = FMath::Max(Solid.A, 1.f);
            Points.Add(Solid.Location + FVector(0, 0, R));
            Points.Add(Solid.Location - FVector(0, 0, R));
            for (int32 Ring = 1; Ring < 4; ++Ring)
            {
                const float Polar = UE_PI * Ring / 4;
                for (int32 i = 0; i < Sides; ++i)
                {
                    const float Angle = UE_TWO_PI * (i + 0.5f * Ring) / Sides;
                    Points.Add(Solid.Location + FVector(FMath::Cos(Angle) * FMath::Sin(Polar), FMath::Sin(Angle) * FMath::Sin(Polar), FMath::Cos(Polar)) * R);
                }
            }
        }
        else if (Solid.Shape == 2 && Solid.Points.Num() >= 4)
        {
            for (const FVector& P : Solid.Points) Points.Add(Pose.TransformPosition(P));
        }
        else
        {
            const FVector Half = (Solid.Shape == 3 ? Solid.Size * 0.5f : FVector(Solid.A, Solid.A, Solid.B * 0.5f)).ComponentMax(FVector(1.f));
            for (int32 i = 0; i < 8; ++i)
            {
                Points.Add(Pose.TransformPosition(FVector(i & 1 ? Half.X : -Half.X, i & 2 ? Half.Y : -Half.Y, i & 4 ? Half.Z : -Half.Z)));
            }
        }
    }
    UProceduralMeshComponent* Body = MakeChunkCollision(TEXT("TerrainSolids"), Key);
    Body->SetupAttachment(RootComponent);
    Body->bUseAsyncCooking = true;
    Body->bUseComplexAsSimpleCollision = false;
    Body->SetVisibility(false);
    Body->SetCanEverAffectNavigation(false);
    Body->SetCollisionProfileName(TEXT("BlockAll"));
    Body->ComponentTags.Add(TEXT("VortexSolid"));
    Body->SetMobility(EComponentMobility::Static);
    Body->RegisterComponent();
    Body->SetCollisionConvexMeshes(Hulls);
    Chunk.SolidBodies.Add(Body);
}

// Regiões de 72 m com árvores em volta de quem desenha a tela (união, se houver mais de um);
// sem ninguém (servidor dedicado), nenhuma. As árvores só têm corpo pelos pedaços (CollisionRadius).
void AVortexTerrainStreamer::UpdateTrees(const TArray<FIntPoint>& Centers)
{
    const float RegionSize = Data->ChunkSize() * 4;
    const int32 Reach = FMath::CeilToInt(TreeRange / RegionSize) + 1;
    if (Centers != LastTreeCenters)
    {
        LastTreeCenters = Centers;
        TArray<FIntPoint> Drop;
        for (auto& Pair : TreeRegions)
        {
            if (ChebyshevTo(Pair.Key, Centers) > Reach + 1) Drop.Add(Pair.Key);
        }
        for (const FIntPoint& Key : Drop)
        {
            for (UInstancedStaticMeshComponent* C : TreeRegions[Key]) if (C) C->DestroyComponent();
            TreeRegions.Remove(Key);
        }
        TreeQueue.Reset();
        for (const FIntPoint& Center : Centers)
        for (int32 dx = -Reach; dx <= Reach; ++dx)
        for (int32 dy = -Reach; dy <= Reach; ++dy)
        {
            const FIntPoint Region = Center + FIntPoint(dx, dy);
            if (!TreeRegions.Contains(Region)) TreeQueue.AddUnique(Region);
        }
        auto Nearest = [&Centers](const FIntPoint& Key)
        {
            int32 Best = MAX_int32;
            for (const FIntPoint& C : Centers) Best = FMath::Min(Best, (Key - C).SizeSquared());
            return Best;
        };
        TreeQueue.Sort([&Nearest](const FIntPoint& A, const FIntPoint& B) { return Nearest(A) < Nearest(B); });
    }
    // no máximo duas regiões novas por quadro (antes: uma fileira inteira, ~13, no mesmo quadro);
    // a primeira volta, sem nada montado, sai de uma vez
    // (só o primeiro preenchimento do jogo; depois de um salto longe, 3 por quadro)
    const int32 Budget = !bTreesPrimed ? TreeQueue.Num() : 3;
    bTreesPrimed |= !TreeQueue.IsEmpty();
    for (int32 Built = 0; Built < Budget && TreeQueue.Num() > 0; ++Built)
    {
        const FIntPoint Region = TreeQueue[0];
        // continente: as plantas dos 16 pedaços saem numa tarefa de fundo; a região espera por elas
        // (montar aqui travava o quadro uns 200 ms a cada salto longe)
        bool bReady = true;
        for (int32 i = 0; i < 4 && bReady; ++i) for (int32 j = 0; j < 4 && bReady; ++j) bReady = Data->IsScattered(FIntPoint(Region.X * 4 + i, Region.Y * 4 + j));
        if (!bReady)
        {
            for (int32 k = 0; k < FMath::Min(6, TreeQueue.Num()); ++k)
            {
                const FIntPoint Next = TreeQueue[k];
                if (TreeWarming.Contains(Next)) continue;
                TreeWarming.Add(Next);
                TWeakPtr<FVortexTerrainData> Weak = Data;
                UE::Tasks::Launch(UE_SOURCE_LOCATION, [Weak, Next]()
                {
                    if (const TSharedPtr<FVortexTerrainData> Pinned = Weak.Pin())
                        for (int32 i = 0; i < 4; ++i) for (int32 j = 0; j < 4; ++j) Pinned->PropsInChunk(FIntPoint(Next.X * 4 + i, Next.Y * 4 + j));
                });
            }
            break;
        }
        TreeWarming.Remove(Region);
        TreeQueue.RemoveAt(0);
        if (TreeRegions.Contains(Region)) continue;
        TArray<TObjectPtr<UInstancedStaticMeshComponent>>& List = TreeRegions.Add(Region);
        TMap<uint16, TArray<FTransform>> ByKind;
        for (int32 i = 0; i < 4; ++i)
        for (int32 j = 0; j < 4; ++j)
        {
            const TArray<FVortexTerrainProp>* Props = Data->PropsInChunk(FIntPoint(Region.X * 4 + i, Region.Y * 4 + j));
            if (!Props) continue;
            for (const FVortexTerrainProp& Prop : *Props)
            {
                if (!Data->IsTree(Prop.Kind)) continue;
                if (VortexWorldLayout::CalderaClearsVegetation(Prop.Location.X, Prop.Location.Y)) continue;   // a cratera do Azhûr
                if (AVortexTown::Contains(Prop.Location) || AVortexTown::Clears(Prop.Location.X, Prop.Location.Y)) continue;   // 03/11: dentro da vila; 11/11: a frente dos portões
                ByKind.FindOrAdd(Prop.Kind).Add(FTransform(FRotator(0.f, Prop.YawDegrees, 0.f), Prop.Location, FVector(Prop.Scale)));
            }
        }
        for (const TPair<uint16, TArray<FTransform>>& Pair : ByKind)
        {
            if (UInstancedStaticMeshComponent* Instances = NewInstances(Pair.Key, TreeRange * TreeScale, true))
            {
                Instances->AddInstances(Pair.Value, false, true);
                List.Add(Instances);
            }
        }
    }
}

void AVortexTerrainStreamer::ReleaseVisual(FChunk& Chunk)
{
    if (Chunk.Visual) Chunk.Visual->DestroyComponent();
    BatchRemove(Chunk);
    Chunk.Visual = nullptr;
    Chunk.bVisual = false;
}

void AVortexTerrainStreamer::Release(FChunk& Chunk)
{
    if (Chunk.Visual) Chunk.Visual->DestroyComponent();
    if (Chunk.Collision) Chunk.Collision->DestroyComponent();
    for (UPrimitiveComponent* Body : Chunk.SolidBodies) if (Body) Body->DestroyComponent();
    BatchRemove(Chunk);
    Chunk = FChunk();
}
