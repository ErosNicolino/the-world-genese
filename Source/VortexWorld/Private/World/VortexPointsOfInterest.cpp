#include "World/VortexPointsOfInterest.h"
#include "World/VortexWorldHooks.h"
#include "World/VortexWorldLayout.h"

#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "World/VortexLandmarks.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexPortal.h"

namespace
{
    /** Cor do bloco (sRGB do código) para o M_PoiBlocks, que usa a cor do vértice direto. */
    FLinearColor PoiHex(const TCHAR* Code, float Shade = 1.f)
    {
        const FLinearColor C = FLinearColor(FColor::FromHex(Code)) * Shade;
        return FLinearColor(C.R, C.G, C.B, 1.f);
    }

    /** Blocos em metros no espaço do ponto (X para o lado, Y para cima, Z para trás), girados em Y. */
    /** Um bloco das construções = um bloco do terreno (75 cm): assim tudo fica na grade do chão
     *  ("as estruturas não ficaram alinhadas com o solo"). As medidas no código são em blocos. */
    constexpr float PoiBlock = .75f;

    struct FPoiBoxes
    {
        FVector Node = FVector::ZeroVector;   // (x, altura, y) em metros
        float Turn = 0.f;
        float Scale = 1.f;                    // metros por unidade (PoiBlock nas construções; 1 nas teias)
        TArray<FVector> Vertices, Normals;
        TArray<int32> Triangles;
        TArray<FLinearColor> Colors;

        FVector ToUnreal(const FVector& L) const
        {
            const float C = FMath::Cos(Turn), S = FMath::Sin(Turn);
            const float X = Node.X + (L.X * C + L.Z * S) * Scale, Z = Node.Z + (-L.X * S + L.Z * C) * Scale;
            return FVector(X, Z, Node.Y + L.Y * Scale) * 100.f;
        }
        int32 Odd = 0;
        void Box(const FVector& Center, const FVector& Size, const FLinearColor& Color)
        {
            // chão ainda sem dados (o terreno longe) dá alturas absurdas: a peça fica de fora
            if (Size.GetAbsMax() > 30.f || FMath::Abs(Center.Y) > 60.f) { ++Odd; return; }
            const FVector H = Size * .5f;
            static const int32 Faces[6][4] = { {0,1,3,2}, {4,6,7,5}, {0,4,5,1}, {2,3,7,6}, {0,2,6,4}, {1,5,7,3} };
            FVector Corner[8];
            for (int32 i = 0; i < 8; ++i)
                Corner[i] = ToUnreal(Center + FVector((i & 4) ? H.X : -H.X, (i & 2) ? H.Y : -H.Y, (i & 1) ? H.Z : -H.Z));
            const FVector Mid = ToUnreal(Center);
            for (const auto& F : Faces)
            {
                const FVector P[4] = { Corner[F[0]], Corner[F[1]], Corner[F[2]], Corner[F[3]] };
                const FVector Out = ((P[0] + P[1] + P[2] + P[3]) * .25f - Mid).GetSafeNormal();
                const int32 Base = Vertices.Num();
                for (const FVector& V : P) { Vertices.Add(V); Normals.Add(Out); Colors.Add(Color); }
                const bool bFlip = FVector::DotProduct(FVector::CrossProduct(P[1] - P[0], P[2] - P[0]), Out) > 0.f;
                if (bFlip) Triangles.Append({ Base, Base + 2, Base + 1, Base, Base + 3, Base + 2 });
                else Triangles.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
            }
        }
        /** Fio de A até B (m, espaço do ponto), um bloco fino girado. */
        void Beam(const FVector& A, const FVector& B, float Thick, const FLinearColor& Color)
        {
            const FVector Axis = B - A;
            const float Length = Axis.Size();
            if (Length < .01f) return;
            const FVector Y = Axis / Length;
            FVector X = FVector::CrossProduct(Y, FVector(0.f, 1.f, 0.f));
            if (X.SizeSquared() < 1e-4f) X = FVector(1.f, 0.f, 0.f);
            X.Normalize();
            const FVector Z = FVector::CrossProduct(X, Y);
            const FVector Mid = (A + B) * .5f;
            const float H = Thick * .5f, L = Length * .5f + H;
            static const int32 Faces[6][4] = { {0,1,3,2}, {4,6,7,5}, {0,4,5,1}, {2,3,7,6}, {0,2,6,4}, {1,5,7,3} };
            FVector Corner[8];
            for (int32 i = 0; i < 8; ++i)
                Corner[i] = ToUnreal(Mid + X * ((i & 4) ? H : -H) + Y * ((i & 2) ? L : -L) + Z * ((i & 1) ? H : -H));
            const FVector Center = ToUnreal(Mid);
            for (const auto& F : Faces)
            {
                const FVector P[4] = { Corner[F[0]], Corner[F[1]], Corner[F[2]], Corner[F[3]] };
                const FVector Out = ((P[0] + P[1] + P[2] + P[3]) * .25f - Center).GetSafeNormal();
                const int32 Base = Vertices.Num();
                for (const FVector& V : P) { Vertices.Add(V); Normals.Add(Out); Colors.Add(Color); }
                const bool bFlip = FVector::DotProduct(FVector::CrossProduct(P[1] - P[0], P[2] - P[0]), Out) > 0.f;
                if (bFlip) Triangles.Append({ Base, Base + 2, Base + 1, Base, Base + 3, Base + 2 });
                else Triangles.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
            }
        }
        /** Bloco que vai do chão (enterrado 1 m) até Top. */
        void Pillar(float X, float Z, float Ground, float Top, float W, float D, const FLinearColor& Color)
        {
            const float Bottom = Ground - 1.f;
            Top = FMath::Max(Top, Ground + .1f);   // chão mais alto que o topo pedido (barranco): ainda aparece
            Box(FVector(X, (Bottom + Top) * .5f, Z), FVector(W, Top - Bottom, D), Color);
        }
    };

    /** As cores de cada região: pedra, pedra 2, escuro, musgo, madeira e o destaque (runas, minério). */
    struct FPalette { FLinearColor Stone, Stone2, Dark, Moss, Wood, Accent; };
    FPalette PaletteFor(int32 Region)
    {
        switch (Region)
        {
        case 2: return { PoiHex(TEXT("4a4340")), PoiHex(TEXT("3a3533")), PoiHex(TEXT("1e1a19")), PoiHex(TEXT("6a2c1a")), PoiHex(TEXT("3d2a1e")), PoiHex(TEXT("ff7a2a")) };   // basalto e brasa
        case 3: return { PoiHex(TEXT("7f8790")), PoiHex(TEXT("6a727c")), PoiHex(TEXT("2e3238")), PoiHex(TEXT("5a6e4a")), PoiHex(TEXT("5a4430")), PoiHex(TEXT("8fd0ff")) };   // granito
        case 4: return { PoiHex(TEXT("d8dde4")), PoiHex(TEXT("b8c2cc")), PoiHex(TEXT("5a6470")), PoiHex(TEXT("a8d8f0")), PoiHex(TEXT("6e5a48")), PoiHex(TEXT("7ae0ff")) };   // pedra branca e gelo
        case 5: return { PoiHex(TEXT("d2b07a")), PoiHex(TEXT("c49a5e")), PoiHex(TEXT("6a4a2a")), PoiHex(TEXT("9a8a50")), PoiHex(TEXT("7a5532")), PoiHex(TEXT("ffd24a")) };   // arenito
        case 6: return { PoiHex(TEXT("c8b48a")), PoiHex(TEXT("b29d70")), PoiHex(TEXT("4e4234")), PoiHex(TEXT("8a9a4a")), PoiHex(TEXT("7a5532")), PoiHex(TEXT("ffcf5a")) };   // pedra dourada
        case 7: return { PoiHex(TEXT("5f6a56")), PoiHex(TEXT("4f5a46")), PoiHex(TEXT("232a20")), PoiHex(TEXT("3f6a30")), PoiHex(TEXT("4a3a28")), PoiHex(TEXT("9aff6a")) };   // pedra com lodo
        case 8: return { PoiHex(TEXT("5a5560")), PoiHex(TEXT("48434e")), PoiHex(TEXT("1c1a22")), PoiHex(TEXT("4a3a5a")), PoiHex(TEXT("3a2e2a")), PoiHex(TEXT("b07aff")) };   // pedra roxa
        case 9: return { PoiHex(TEXT("7a8a70")), PoiHex(TEXT("6a7a62")), PoiHex(TEXT("2a3226")), PoiHex(TEXT("4a8a3a")), PoiHex(TEXT("5c4430")), PoiHex(TEXT("7affb0")) };   // pedra élfica com musgo
        default: return { PoiHex(TEXT("9a958c")), PoiHex(TEXT("b0aca2")), PoiHex(TEXT("3a3834")), PoiHex(TEXT("5f7a3a")), PoiHex(TEXT("6e4a2a")), PoiHex(TEXT("6ad0ff")) };   // vale
        }
    }

    FLinearColor BlessingColor(uint8 Variant)
    {
        return Variant == 0 ? FLinearColor(1.f, .25f, .15f) : Variant == 1 ? FLinearColor(.25f, 1.f, .4f) : FLinearColor(1.f, .8f, .2f);
    }
}

AVortexPointsOfInterest::AVortexPointsOfInterest()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

AVortexPointsOfInterest* AVortexPointsOfInterest::Find(const UWorld* World)
{
    if (!World) return nullptr;
    if (TActorIterator<AVortexPointsOfInterest> It(const_cast<UWorld*>(World)); It) return *It;
    return nullptr;
}

FString AVortexPointsOfInterest::KindName(EVortexPoi Kind)
{
    switch (Kind)
    {
    case EVortexPoi::Tower: return TEXT("Torre de vigia");
    case EVortexPoi::Shrine: return TEXT("Santuário");
    case EVortexPoi::Mine: return TEXT("Mina abandonada");
    case EVortexPoi::Graveyard: return TEXT("Cemitério");
    case EVortexPoi::Caravan: return TEXT("Caravana tombada");
    case EVortexPoi::Dock: return TEXT("Cais de pesca");
    default: return TEXT("Ruínas");
    }
}

FString AVortexPointsOfInterest::BlessingName(uint8 Variant)
{
    return Variant == 0 ? TEXT("Bênção da Força") : Variant == 1 ? TEXT("Bênção da Vida") : TEXT("Bênção da Sorte");
}

FName AVortexPointsOfInterest::BlessingKind(uint8 Variant)
{
    return Variant == 0 ? FName(TEXT("forca")) : Variant == 1 ? FName(TEXT("vida")) : FName(TEXT("sorte"));
}

FVector2D AVortexPointsOfInterest::LocalToWorld(const FVortexPoi& Poi, float LX, float LZ)
{
    const float T = FMath::DegreesToRadians(Poi.Yaw), C = FMath::Cos(T), S = FMath::Sin(T);
    return Poi.At + FVector2D(LX * C + LZ * S, -LX * S + LZ * C) * 100.f * PoiBlock;
}

bool AVortexPointsOfInterest::PoiAt(const FIntPoint& Cell, FVortexPoi& Out)
{
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    if (!Terrain || !Terrain->HasWorld()) return false;
    FRandomStream R(HashCombine(GetTypeHash(Cell.X * 7349), GetTypeHash(Cell.Y - 5113)));
    if (R.FRand() > .4f) return false;
    // uma coisa por quadra: onde há acampamento de bandidos, não
    FVector2D Camp;
    if (VortexWorldLayout::BanditCampAt(Cell, Camp)) return false;
    const float X = (Cell.X + .25f + R.FRand() * .5f) * CellSize, Y = (Cell.Y + .25f + R.FRand() * .5f) * CellSize;
    if (!Terrain->InMap(X, Y)) return false;
    const float Ground = Terrain->QuickGround(X, Y);
    if (Ground < Terrain->WaterLevel + 40.f) return false;
    // longe da cidade, dos marcos (vilas, Yggdrasil, covis) e das pontes e cabanas
    if (FVector2D::Distance(FVector2D(X, Y), FVector2D(0.f, 1500.f)) < 15000.f) return false;
    for (const AVortexLandmarks::FSite& Site : AVortexLandmarks::Sites())
        if (FVector2D::Distance(FVector2D(X, Y), Site.Center) < Site.Radius + 4000.f) return false;
    for (const FVortexTerrainData::FFeature& Feature : Terrain->Features())
        if (FVector2D::Distance(FVector2D(X, Y), Feature.At) < 4000.f) return false;
    // chão sem muito desnível em volta (as peças têm alicerce, mas um barranco engoliria tudo)
    float Low = Ground, High = Ground;
    for (int32 i = 0; i < 8; ++i)
    {
        const float A = i * UE_TWO_PI / 8.f;
        const float G = Terrain->QuickGround(X + FMath::Cos(A) * 500.f, Y + FMath::Sin(A) * 500.f);
        Low = FMath::Min(Low, G);
        High = FMath::Max(High, G);
    }
    if (High - Low > 450.f) return false;
    const int32 Region = FMath::Clamp(Terrain->RegionAt(X, Y), 0, 9);
    if (Region == 0) return false;
    // água por perto: o cais aponta para ela
    float WaterYaw = -1.f;
    for (int32 i = 0; i < 12 && WaterYaw < 0.f; ++i)
    {
        const float A = i * UE_TWO_PI / 12.f;
        if (Terrain->QuickGround(X + FMath::Cos(A) * 1400.f, Y + FMath::Sin(A) * 1400.f) < Terrain->WaterLevel)
        {
            // a frente do ponto (-Z local) vai para (cos A, sin A): -Z gira para (-sin T, -cos T)
            WaterYaw = FMath::RadiansToDegrees(FMath::Atan2(-FMath::Cos(A), -FMath::Sin(A)));
        }
    }
    // o que aparece em cada região: ruínas, torre, santuário, mina, cemitério, caravana
    static const int32 Weights[10][6] = {
        { 0, 0, 0, 0, 0, 0 },
        { 30, 15, 20, 8, 10, 17 },   // Valdora
        { 30, 10, 20, 30, 5, 5 },    // Brasafenda
        { 20, 25, 15, 35, 5, 0 },    // Raikor
        { 30, 20, 25, 15, 10, 0 },   // Nivaluz
        { 35, 15, 15, 15, 5, 15 },   // Solcânion
        { 25, 15, 15, 5, 10, 30 },   // Aurivale
        { 25, 10, 15, 0, 40, 10 },   // Lodonegro
        { 20, 10, 10, 5, 55, 0 },    // Morthúria
        { 40, 15, 30, 0, 10, 5 } };  // Verdalume
    EVortexPoi Kind = EVortexPoi::Ruins;
    if (WaterYaw >= 0.f && R.FRand() < .55f) Kind = EVortexPoi::Dock;
    else
    {
        int32 Total = 0;
        for (int32 i = 0; i < 6; ++i) Total += Weights[Region][i];
        int32 Pick = R.RandRange(0, FMath::Max(0, Total - 1));
        for (int32 i = 0; i < 6; ++i)
        {
            if (Pick < Weights[Region][i]) { Kind = EVortexPoi(i); break; }
            Pick -= Weights[Region][i];
        }
    }
    Out = FVortexPoi();
    Out.Kind = Kind;
    Out.Cell = Cell;
    const float TerrainCell = Terrain->CellSize;
    Out.At = FVector2D((FMath::FloorToFloat(X / TerrainCell) + .5f) * TerrainCell, (FMath::FloorToFloat(Y / TerrainCell) + .5f) * TerrainCell);
    Out.Yaw = Kind == EVortexPoi::Dock ? FMath::RoundToFloat(WaterYaw / 90.f) * 90.f : 90.f * R.RandRange(0, 3);
    Out.Region = Region;
    Out.Variant = uint8(R.RandRange(0, 2));
    Out.bRare = R.FRand() < .2f;
    Out.Id = FName(*FString::Printf(TEXT("poi:%d_%d"), Cell.X, Cell.Y));
    return true;
}

bool AVortexPointsOfInterest::GateAt(const FIntPoint& Cell, FVector2D& OutAt, FString& OutRank, float& OutYaw)
{
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    if (!Terrain || !Terrain->HasWorld()) return false;
    FRandomStream R(HashCombine(GetTypeHash(Cell.X * 9973), GetTypeHash(Cell.Y + 4441)));
    if (R.FRand() > .07f) return false;
    FVortexPoi Poi;
    FVector2D Camp;
    if (PoiAt(Cell, Poi) || VortexWorldLayout::BanditCampAt(Cell, Camp)) return false;
    const float X = (Cell.X + .3f + R.FRand() * .4f) * CellSize, Y = (Cell.Y + .3f + R.FRand() * .4f) * CellSize;
    if (!Terrain->InMap(X, Y) || Terrain->QuickGround(X, Y) < Terrain->WaterLevel + 40.f) return false;
    if (FVector2D::Distance(FVector2D(X, Y), FVector2D(0.f, 1500.f)) < 20000.f) return false;
    for (const AVortexLandmarks::FSite& Site : AVortexLandmarks::Sites())
        if (FVector2D::Distance(FVector2D(X, Y), Site.Center) < Site.Radius + 4000.f) return false;
    // azul comum, vermelho raro; e mais longe da cidade os ranques altos aparecem um pouco mais
    const float Far = FMath::Clamp(FVector2D(X, Y).Size() / 300000.f, 0.f, 1.f);
    const float Roll = FMath::Max(0.f, R.FRand() - Far * .12f);
    OutRank = Roll < .03f ? TEXT("S") : Roll < .09f ? TEXT("A") : Roll < .19f ? TEXT("B") : Roll < .35f ? TEXT("C") : Roll < .60f ? TEXT("D") : TEXT("E");
    OutAt = FVector2D(X, Y);
    OutYaw = R.FRandRange(0.f, 360.f);
    return true;
}

bool AVortexPointsOfInterest::WebsAt(const FIntPoint& Cell, TArray<FWebSpot>& Out)
{
    Out.Reset();
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    if (!Terrain || !Terrain->HasWorld()) return false;
    FRandomStream R(HashCombine(GetTypeHash(Cell.X * 6007), GetTypeHash(Cell.Y - 2203)));
    const float X = (Cell.X + .3f + R.FRand() * .4f) * CellSize, Y = (Cell.Y + .3f + R.FRand() * .4f) * CellSize;
    const int32 Region = FMath::Clamp(Terrain->RegionAt(X, Y), 0, 9);
    static const float Chance[10] = { 0.f, .12f, .05f, .15f, .05f, .05f, .06f, .35f, .3f, .3f };
    if (R.FRand() > Chance[Region]) return false;
    FVortexPoi Poi;
    FVector2D Camp, Gate;
    FString Rank;
    float Yaw = 0.f;
    if (PoiAt(Cell, Poi) || VortexWorldLayout::BanditCampAt(Cell, Camp) || GateAt(Cell, Gate, Rank, Yaw)) return false;
    if (!Terrain->InMap(X, Y) || Terrain->QuickGround(X, Y) < Terrain->WaterLevel + 40.f) return false;
    if (FVector2D::Distance(FVector2D(X, Y), FVector2D(0.f, 1500.f)) < 12000.f) return false;
    const int32 Count = 2 + R.RandRange(0, 2);
    for (int32 K = 0; K < Count; ++K)
    {
        FWebSpot Spot;
        const float A = R.FRand() * UE_TWO_PI, D = K == 0 ? 0.f : R.FRandRange(300.f, 650.f);
        Spot.At = FVector2D(X + FMath::Cos(A) * D, Y + FMath::Sin(A) * D);
        Spot.Yaw = R.FRandRange(0.f, 180.f);
        Spot.Width = R.FRandRange(2.6f, 3.6f);
        Spot.bGround = R.FRand() < .3f;
        if (Terrain->QuickGround(Spot.At.X, Spot.At.Y) > Terrain->WaterLevel + 20.f) Out.Add(Spot);
    }
    return Out.Num() > 0;
}

void AVortexPointsOfInterest::AddPinnedWebs(const FIntPoint& Key, const TArray<FWebSpot>& Spots, float GroundZ)
{
    if (Webs.Contains(Key)) return;
    BuildWebs(Key, Spots, GroundZ);
    PinnedWebs.Add(Key);
}

void AVortexPointsOfInterest::RemovePinnedWebs(const FIntPoint& Key)
{
    PinnedWebs.Remove(Key);
    if (TArray<FWeb>* List = Webs.Find(Key)) for (FWeb& Web : *List) if (UProceduralMeshComponent* Mesh = Web.Mesh.Get()) Mesh->DestroyComponent();
    Webs.Remove(Key);
}

void AVortexPointsOfInterest::BuildWebs(const FIntPoint& Cell, const TArray<FWebSpot>& Spots, TOptional<float> FixedGroundZ)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data && !FixedGroundZ.IsSet()) return;
    if (!Material) Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/World/M_PoiBlocks.M_PoiBlocks"));
    TArray<FWeb>& List = Webs.Add(Cell);
    const FLinearColor Silk = PoiHex(TEXT("e6e6ee")), Stick = PoiHex(TEXT("3a2c20"));
    for (const FWebSpot& Spot : Spots)
    {
        FWeb& Web = List.AddDefaulted_GetRef();
        Web.Spot = Spot;
        Web.GroundZ = FixedGroundZ.IsSet() ? FixedGroundZ.GetValue() : Data->GroundAt(Spot.At.X, Spot.At.Y);
        FPoiBoxes M;
        M.Node = FVector(Spot.At.X / 100.f, Web.GroundZ / 100.f, Spot.At.Y / 100.f);
        M.Turn = FMath::DegreesToRadians(Spot.Yaw);
        const float R = Spot.Width * .5f;
        // a teia: 8 raios saindo do centro e 4 voltas ligando os raios
        auto Point = [&](int32 Spoke, float Radius)
        {
            const float A = Spoke * UE_TWO_PI / 8.f;
            return Spot.bGround ? FVector(FMath::Cos(A) * Radius, .06f, FMath::Sin(A) * Radius)
                : FVector(FMath::Cos(A) * Radius, R + .15f + FMath::Sin(A) * Radius * .85f, 0.f);
        };
        const FVector Center = Spot.bGround ? FVector(0.f, .06f, 0.f) : FVector(0.f, R + .15f, 0.f);
        for (int32 S = 0; S < 8; ++S) M.Beam(Center, Point(S, R), .035f, Silk);
        for (int32 Ring = 1; Ring <= 4; ++Ring)
            for (int32 S = 0; S < 8; ++S) M.Beam(Point(S, R * Ring / 4.f), Point(S + 1, R * Ring / 4.f), .028f, Silk);
        if (!Spot.bGround)
            for (const float Side : { -1.f, 1.f })   // os dois galhos secos que seguram a teia
                M.Beam(FVector(Side * (R + .1f), -.6f, 0.f), FVector(Side * (R + .25f), R * 2.f + .5f, .1f), .12f, Stick);
        UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetUsingAbsoluteLocation(true);
        Mesh->SetUsingAbsoluteRotation(true);
        Mesh->RegisterComponent();
        Mesh->CreateMeshSection_LinearColor(0, M.Vertices, M.Triangles, M.Normals, {}, M.Colors, {}, false);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (Material) Mesh->SetMaterial(0, Material);
        Web.Mesh = Mesh;
    }
}

bool AVortexPointsOfInterest::IsInWeb(const FVector& At) const
{
    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    for (const TPair<FIntPoint, TArray<FWeb>>& Cluster : Webs)
        for (const FWeb& Web : Cluster.Value)
        {
            if (Web.CutUntil > Now) continue;
            const FVector2D Local = (FVector2D(At) - Web.Spot.At).GetRotated(-Web.Spot.Yaw);
            const float Half = Web.Spot.Width * 50.f;
            const float Up = At.Z - Web.GroundZ;
            if (Web.Spot.bGround ? (FMath::Abs(Local.X) < Half && FMath::Abs(Local.Y) < Half && Up < 170.f)
                : (FMath::Abs(Local.X) < Half && FMath::Abs(Local.Y) < 70.f && Up > -60.f && Up < Web.Spot.Width * 100.f + 60.f)) return true;
        }
    return false;
}

bool AVortexPointsOfInterest::SlashWeb(const FVector& From, const FVector& Forward)
{
    const double Now = GetWorld()->GetTimeSeconds();
    const FVector Reach = From + Forward.GetSafeNormal2D() * 120.f;
    bool bHit = false;
    for (TPair<FIntPoint, TArray<FWeb>>& Cluster : Webs)
        for (FWeb& Web : Cluster.Value)
        {
            if (Web.CutUntil > Now || FVector2D::Distance(FVector2D(Reach), Web.Spot.At) > Web.Spot.Width * 50.f + 150.f || FMath::Abs(From.Z - Web.GroundZ) > 400.f) continue;
            bHit = true;
            if (++Web.Hits < 3) continue;
            // cortada: some e volta a crescer em 90 s
            Web.Hits = 0;
            Web.CutUntil = Now + 90.0;
            if (UProceduralMeshComponent* Mesh = Web.Mesh.Get()) Mesh->SetVisibility(false);
        }
    if (bHit) VortexWorldHooks::PlaySound(this, TEXT("creature_hit"), Reach, .5f, .2f);
    return bHit;
}

FString AVortexPointsOfInterest::LoreText(const FVortexPoi& Poi)
{
    static const TCHAR* Lines[] = {
        TEXT("\"Antes do Vórtice, sete selos guardavam o mundo. Este era o quarto.\""),
        TEXT("\"Os reis de pedra dormem sob o vale. Não acorde quem sonha com fogo.\""),
        TEXT("\"Quando a estrela cair, os portais se abrirão. Só os Despertos entrarão.\""),
        TEXT("\"Aqui os elfos e os anões fizeram a paz. A paz durou um inverno.\""),
        TEXT("\"Azhûr não queima o que é seu. Queima o que esquece.\""),
        TEXT("\"Os mortos de Morthúria contam os vivos. Nunca chegam ao fim da conta.\""),
        TEXT("\"Siga as runas azuis: elas levam aonde a água não chega.\""),
        TEXT("\"O primeiro herói não tinha arma. Tinha só a pergunta certa.\"") };
    return Lines[(Poi.Cell.X * 31 + Poi.Cell.Y * 17 + 999999) % UE_ARRAY_COUNT(Lines)];
}

const FVortexPoi* AVortexPointsOfInterest::NearestShrine(const FVector& At, float Range) const
{
    for (const TPair<FIntPoint, FBuilt>& B : Built)
        if (B.Value.Poi.Kind == EVortexPoi::Shrine && FVector2D::Distance(B.Value.Poi.At, FVector2D(At)) < Range) return &B.Value.Poi;
    return nullptr;
}

const FVortexPoi* AVortexPointsOfInterest::NearestLore(const FVector& At, float Range) const
{
    for (const TPair<FIntPoint, FBuilt>& B : Built)
        if (B.Value.Poi.Kind == EVortexPoi::Ruins && B.Value.Poi.Variant == 1 && FVector2D::Distance(LocalToWorld(B.Value.Poi, 0.f, 1.6f), FVector2D(At)) < Range) return &B.Value.Poi;
    return nullptr;
}

bool AVortexPointsOfInterest::IsOnTowerTop(const FVector& At) const
{
    for (const TPair<FIntPoint, FBuilt>& B : Built)
        if (B.Value.Poi.Kind == EVortexPoi::Tower && FVector2D::Distance(B.Value.Poi.At, FVector2D(At)) < 320.f && At.Z > B.Value.TowerTopZ - 30.f) return true;
    return false;
}

void AVortexPointsOfInterest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Clock += DeltaSeconds;
    // runas dos santuários: a luz pulsa e o cristal gira e flutua
    for (TPair<FIntPoint, FBuilt>& B : Built)
    {
        if (UPointLightComponent* Light = B.Value.Light.Get()) Light->SetIntensity(1750.f + 1250.f * (.5f + .5f * FMath::Sin(Clock * 2.4f + B.Key.X)));
        if (UProceduralMeshComponent* Crystal = B.Value.Crystal.Get())
        {
            Crystal->SetRelativeRotation(FRotator(0.f, Clock * 60.f, 0.f));
            Crystal->SetRelativeLocation(FVector(0.f, 0.f, 18.f * FMath::Sin(Clock * 1.7f)));
        }
    }
    // monta a 200 m, desmonta a 280 m (de meio em meio segundo, no máximo duas por vez)
    Since += DeltaSeconds;
    if (Since < .5f) return;
    Since = 0.f;
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data || !Data->HasWorld()) return;
    TArray<FVector2D> Players;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (const APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr) Players.Add(FVector2D(Pawn->GetActorLocation()));
    if (Players.IsEmpty()) return;
    TSet<FIntPoint> Wanted;
    int32 Budget = 2;
    for (const FVector2D& P : Players)
    {
        const FIntPoint Home(FMath::FloorToInt(P.X / CellSize), FMath::FloorToInt(P.Y / CellSize));
        for (int32 DX = -4; DX <= 4; ++DX) for (int32 DY = -4; DY <= 4; ++DY)
        {
            const FIntPoint Cell = Home + FIntPoint(DX, DY);
            FVortexPoi Poi;
            if (!PoiAt(Cell, Poi)) continue;
            const float Distance = FVector2D::Distance(Poi.At, P);
            if (Distance < 28000.f) Wanted.Add(Cell);
            if (Distance < 20000.f && !Built.Contains(Cell) && Budget > 0) { Build(Poi); --Budget; }
        }
    }
    TArray<FIntPoint> Drops;
    for (const TPair<FIntPoint, FBuilt>& B : Built) if (!Wanted.Contains(B.Key)) Drops.Add(B.Key);
    for (const FIntPoint& Cell : Drops) Drop(Cell);
    // teias: montadas a 150 m, desmontadas a 220 m; as cortadas voltam a crescer
    TSet<FIntPoint> WantWebs;
    for (const FVector2D& P : Players)
    {
        const FIntPoint Home(FMath::FloorToInt(P.X / CellSize), FMath::FloorToInt(P.Y / CellSize));
        for (int32 DX = -3; DX <= 3; ++DX) for (int32 DY = -3; DY <= 3; ++DY)
        {
            const FIntPoint Cell = Home + FIntPoint(DX, DY);
            TArray<FWebSpot> Spots;
            if (!WebsAt(Cell, Spots)) continue;
            const float Distance = FVector2D::Distance(Spots[0].At, P);
            if (Distance < 22000.f) WantWebs.Add(Cell);
            if (Distance < 15000.f && !Webs.Contains(Cell)) BuildWebs(Cell, Spots);
        }
    }
    TArray<FIntPoint> WebDrops;
    const double WebNow = GetWorld()->GetTimeSeconds();
    for (TPair<FIntPoint, TArray<FWeb>>& Cluster : Webs)
    {
        if (!WantWebs.Contains(Cluster.Key) && !PinnedWebs.Contains(Cluster.Key)) { WebDrops.Add(Cluster.Key); continue; }
        for (FWeb& Web : Cluster.Value)
            if (Web.CutUntil > 0.0 && Web.CutUntil <= WebNow) { Web.CutUntil = -1.0; if (UProceduralMeshComponent* Mesh = Web.Mesh.Get()) Mesh->SetVisibility(true); }
    }
    for (const FIntPoint& Cell : WebDrops)
    {
        for (FWeb& Web : Webs[Cell]) if (UProceduralMeshComponent* Mesh = Web.Mesh.Get()) Mesh->DestroyComponent();
        Webs.Remove(Cell);
    }
    // portais espalhados: o servidor abre a 250 m e fecha a 350 m (os clientes recebem pela rede)
    if (GetNetMode() == NM_Client) return;   // este ator existe em cada máquina; só o servidor abre os portais
    TSet<FIntPoint> WantGates;
    for (const FVector2D& P : Players)
    {
        const FIntPoint Home(FMath::FloorToInt(P.X / CellSize), FMath::FloorToInt(P.Y / CellSize));
        for (int32 DX = -5; DX <= 5; ++DX) for (int32 DY = -5; DY <= 5; ++DY)
        {
            const FIntPoint Cell = Home + FIntPoint(DX, DY);
            FVector2D At;
            FString Rank;
            float Yaw = 0.f;
            if (!GateAt(Cell, At, Rank, Yaw)) continue;
            const float Distance = FVector2D::Distance(At, P);
            if (Distance < 35000.f) WantGates.Add(Cell);
            if (Distance < 25000.f && !Gates.Contains(Cell))
                Gates.Add(Cell, AVortexPortal::Open(GetWorld(), FVector(At, Data->GroundAt(At.X, At.Y)), Yaw, Rank, FName(*FString::Printf(TEXT("portal_%d_%d"), Cell.X, Cell.Y))));
        }
    }
    TArray<FIntPoint> Closing;
    for (const TPair<FIntPoint, TWeakObjectPtr<AVortexPortal>>& G : Gates) if (!WantGates.Contains(G.Key)) Closing.Add(G.Key);
    for (const FIntPoint& Cell : Closing)
    {
        if (AVortexPortal* Portal = Gates[Cell].Get()) Portal->Destroy();
        Gates.Remove(Cell);
    }
}

void AVortexPointsOfInterest::Drop(const FIntPoint& Cell)
{
    FBuilt* B = Built.Find(Cell);
    if (!B) return;
    for (USceneComponent* Part : B->Parts) if (Part) Part->DestroyComponent();
    VortexWorldHooks::RemoveChest(GetWorld(), B->Poi.Id);
    Built.Remove(Cell);
}

void AVortexPointsOfInterest::Build(const FVortexPoi& Poi)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return;
    if (!Material) Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/World/M_PoiBlocks.M_PoiBlocks"));
    FBuilt& B = Built.Add(Poi.Cell);
    B.Poi = Poi;
    FRandomStream R(HashCombine(GetTypeHash(Poi.Cell.X * 131), GetTypeHash(Poi.Cell.Y * 977)));
    const FPalette P = PaletteFor(Poi.Region);
    FPoiBoxes M;
    const float Base = Data->GroundAt(Poi.At.X, Poi.At.Y) / 100.f;
    M.Node = FVector(Poi.At.X / 100.f, Base, Poi.At.Y / 100.f);
    M.Turn = FMath::DegreesToRadians(Poi.Yaw);
    M.Scale = PoiBlock;
    // o chão em cada ponto, relativo ao centro (em blocos)
    auto G = [&](float LX, float LZ) { const FVector W = M.ToUnreal(FVector(LX, 0.f, LZ)); return (Data->GroundAt(W.X, W.Y) / 100.f - Base) / PoiBlock; };
    auto Shade = [&R](const FLinearColor& C) { const float K = R.FRandRange(.88f, 1.08f); return FLinearColor(C.R * K, C.G * K, C.B * K, 1.f); };
    // onde vai o baú (local, m) e se é guardado
    FVector ChestAt(0.f, 0.f, 0.f);
    bool bChest = false, bGuarded = false;
    float ChestYaw = 0.f;

    switch (Poi.Kind)
    {
    case EVortexPoi::Ruins:
    {
        // piso quebrado (falta um quarto das lajes)
        for (int32 X = -4; X <= 4; ++X) for (int32 Z = -4; Z <= 4; ++Z)
            if (R.FRand() < .74f) M.Pillar(X, Z, G(X, Z), G(X, Z) + .15f, .96f, .96f, Shade(R.FRand() < .5f ? P.Stone : P.Stone2));
        // muros em dois ou três lados, com falhas e alturas desiguais
        const int32 Sides = 2 + R.RandRange(0, 1);
        for (int32 Side = 0; Side < Sides; ++Side)
            for (int32 K = -4; K <= 4; ++K)
            {
                const float H = R.FRand() < .25f ? 0.f : R.FRandRange(.6f, 3.4f);
                if (H <= 0.f) continue;
                const float X = Side == 0 ? K : Side == 1 ? 4.5f : K, Z = Side == 0 ? -4.5f : Side == 1 ? K : 4.5f;
                const float Gr = G(X, Z);
                M.Pillar(X, Z, Gr, Gr + H, Side == 1 ? .8f : 1.f, Side == 1 ? 1.f : .8f, Shade(P.Stone));
                if (R.FRand() < .35f) M.Box(FVector(X, Gr + H + .06f, Z), FVector(.9f, .12f, .9f), Shade(P.Moss));
            }
        // colunas nos cantos (uma pode estar caída, deitada no chão)
        const int32 Fallen = R.RandRange(0, 3);
        for (int32 C = 0; C < 4; ++C)
        {
            const float X = C % 2 ? 3.2f : -3.2f, Z = C / 2 ? 3.2f : -3.2f;
            const float Gr = G(X, Z);
            if (C == Fallen)
            {
                for (int32 K = 0; K < 3; ++K) M.Box(FVector(X - 1.2f + K * 1.05f, Gr + .4f, Z + (C / 2 ? -1.f : 1.f)), FVector(1.f, .8f, .8f), Shade(P.Stone2));
                continue;
            }
            const float H = R.FRandRange(1.8f, 4.6f);
            M.Pillar(X, Z, Gr, Gr + H, .8f, .8f, Shade(P.Stone2));
            if (H > 3.6f) M.Box(FVector(X, Gr + H + .15f, Z), FVector(1.2f, .3f, 1.2f), Shade(P.Stone));
        }
        // entulho
        for (int32 K = 0; K < 10; ++K)
        {
            const float A = R.FRand() * UE_TWO_PI, D = R.FRandRange(2.f, 6.5f), S = R.FRandRange(.35f, .75f);
            const float X = FMath::Cos(A) * D, Z = FMath::Sin(A) * D;
            M.Box(FVector(X, G(X, Z) + S * .4f, Z), FVector(S, S, S), Shade(R.FRand() < .3f ? P.Moss : P.Stone2));
        }
        if (Poi.Variant == 1)
        {
            // a pedra com inscrição: uma placa escura com a faixa de runas
            const float Gr = G(0.f, 1.6f);
            M.Pillar(0.f, 1.6f, Gr, Gr + 1.6f, 1.3f, .4f, P.Dark);
            for (int32 K = 0; K < 3; ++K) M.Box(FVector(0.f, Gr + .55f + K * .35f, 1.38f), FVector(.9f, .08f, .04f), P.Accent);
        }
        else { bChest = true; ChestAt = FVector(0.f, G(0.f, 0.f), 0.f); }
        break;
    }
    case EVortexPoi::Tower:
    {
        // núcleo de pedra (4x4) até 8 m, com janelas, e a escada de 20 degraus de 40 cm em volta
        constexpr float Top = 8.f;
        for (int32 X = -2; X < 2; ++X) for (int32 Z = -2; Z < 2; ++Z)
            M.Pillar(X + .5f, Z + .5f, G(X + .5f, Z + .5f), Top, 1.f, 1.f, Shade((X + Z) % 2 ? P.Stone : P.Stone2));
        for (const float Y : { 3.f, 5.8f })
            for (int32 Side = 0; Side < 4; ++Side)
            {
                const float X = Side == 0 ? 0.f : Side == 1 ? 2.02f : Side == 2 ? 0.f : -2.02f, Z = Side == 0 ? -2.02f : Side == 1 ? 0.f : Side == 2 ? 2.02f : 0.f;
                M.Box(FVector(X, Y, Z), Side % 2 ? FVector(.06f, 1.f, .5f) : FVector(.5f, 1.f, .06f), P.Dark);
            }
        int32 Step = 0;
        auto Stair = [&](float X, float Z)
        {
            const float Height = .4f * (++Step);
            M.Pillar(X, Z, G(X, Z), Height, 1.f, 1.f, Shade(P.Stone2));
        };
        for (int32 X = -2; X <= 2; ++X) Stair(X, -2.5f);
        for (int32 Z = -2; Z <= 2; ++Z) Stair(2.5f, Z);
        for (int32 X = 2; X >= -2; --X) Stair(X, 2.5f);
        for (int32 Z = 2; Z >= -2; --Z) Stair(-2.5f, Z);
        // ameias no topo (abertas onde a escada chega) e a bandeira
        for (int32 K = -2; K < 2; ++K)
            for (int32 Side = 0; Side < 4; ++Side)
            {
                if ((K + 2) % 2) continue;
                const float X = Side == 0 ? K + .5f : Side == 1 ? 1.7f : Side == 2 ? K + .5f : -1.7f, Z = Side == 0 ? -1.7f : Side == 1 ? K + .5f : Side == 2 ? 1.7f : K + .5f;
                if (X < -1.f && Z < -1.f) continue;
                M.Box(FVector(X, Top + .35f, Z), FVector(.55f, .7f, .55f), Shade(P.Stone));
            }
        M.Box(FVector(1.4f, Top + 1.6f, 1.4f), FVector(.12f, 3.2f, .12f), P.Wood);
        M.Box(FVector(1.4f, Top + 2.7f, 1.95f), FVector(.06f, .8f, 1.f), P.Accent);
        B.TowerTopZ = (Base + Top * PoiBlock) * 100.f;
        break;
    }
    case EVortexPoi::Shrine:
    {
        // plataforma de dois degraus, obelisco afinando para cima com a runa na cor da bênção, quatro tochas de pedra
        M.Pillar(0.f, 0.f, G(0.f, 0.f), .3f, 5.f, 5.f, Shade(P.Stone2));
        M.Box(FVector(0.f, .45f, 0.f), FVector(3.6f, .3f, 3.6f), Shade(P.Stone));
        M.Box(FVector(0.f, 1.8f, 0.f), FVector(1.f, 2.4f, 1.f), P.Dark);
        M.Box(FVector(0.f, 3.4f, 0.f), FVector(.75f, .8f, .75f), P.Dark);
        M.Box(FVector(0.f, 3.95f, 0.f), FVector(.45f, .3f, .45f), P.Dark);
        const FLinearColor Rune = BlessingColor(Poi.Variant);
        for (int32 Face = 0; Face < 4; ++Face)
        {
            const float X = Face == 1 ? .51f : Face == 3 ? -.51f : 0.f, Z = Face == 0 ? -.51f : Face == 2 ? .51f : 0.f;
            for (int32 K = 0; K < 3; ++K)
                M.Box(FVector(X, 1.2f + K * .6f, Z), Face % 2 ? FVector(.03f, .3f, .5f - K * .1f) : FVector(.5f - K * .1f, .3f, .03f), Rune);
        }
        for (int32 C = 0; C < 4; ++C)
        {
            const float X = C % 2 ? 2.f : -2.f, Z = C / 2 ? 2.f : -2.f;
            M.Box(FVector(X, 1.f, Z), FVector(.4f, 1.f, .4f), Shade(P.Stone));
            M.Box(FVector(X, 1.6f, Z), FVector(.6f, .2f, .6f), Shade(P.Stone2));
            M.Box(FVector(X, 1.78f, Z), FVector(.3f, .16f, .3f), Rune);
        }
        break;
    }
    case EVortexPoi::Mine:
    {
        // o morro de pedra (camadas que encolhem), com o túnel na frente (-Z) que acaba no escuro
        for (int32 Layer = 0; Layer < 5; ++Layer)
        {
            const float Radius = 5.2f - Layer * .95f;
            for (int32 X = -5; X <= 5; ++X) for (int32 Z = -5; Z <= 5; ++Z)
            {
                if (FVector2D(X, Z).Size() > Radius) continue;
                if (FMath::Abs(X) <= 1 && Z < 1 && Layer < 3) continue;
                const float Gr = Layer == 0 ? G(X, Z) : 0.f;
                if (Layer == 0) M.Pillar(X, Z, Gr, 1.f, 1.f, 1.f, Shade(R.FRand() < .5f ? P.Stone : P.Stone2));
                else M.Box(FVector(X, Layer + .5f, Z), FVector(1.f, 1.f, 1.f), Shade(R.FRand() < .15f ? P.Moss : R.FRand() < .5f ? P.Stone : P.Stone2));
            }
        }
        M.Box(FVector(0.f, 1.3f, .42f), FVector(3.f, 2.6f, .1f), FLinearColor(.01f, .01f, .012f));
        M.Box(FVector(0.f, .02f, -1.5f), FVector(3.f, .1f, 5.5f), P.Dark);
        // escoras de madeira na boca do túnel
        for (const float X : { -1.4f, 1.4f }) M.Pillar(X, -4.6f, G(X, -4.6f), 2.8f, .3f, .3f, P.Wood);
        M.Box(FVector(0.f, 2.95f, -4.6f), FVector(3.4f, .3f, .35f), P.Wood);
        // trilho saindo do túnel e o carrinho de minério
        for (int32 K = 0; K < 9; ++K)
        {
            const float Z = -1.f - K;
            const float Gr = G(0.f, Z);
            M.Box(FVector(0.f, Gr + .05f, Z), FVector(1.4f, .1f, .25f), P.Wood);
            for (const float X : { -.45f, .45f }) M.Box(FVector(X, Gr + .14f, Z), FVector(.1f, .08f, 1.f), P.Dark);
        }
        const float Cart = G(0.f, -7.f);
        M.Box(FVector(0.f, Cart + .65f, -7.f), FVector(1.2f, .7f, 1.6f), P.Dark);
        for (int32 K = 0; K < 4; ++K) M.Box(FVector(-.3f + (K % 2) * .55f, Cart + 1.1f, -7.4f + (K / 2) * .7f), FVector(.45f, .35f, .45f), P.Accent);
        for (const float X : { -.62f, .62f }) for (const float Z : { -7.5f, -6.5f }) M.Box(FVector(X, Cart + .25f, Z), FVector(.1f, .4f, .4f), P.Dark);
        bChest = true; bGuarded = true; ChestAt = FVector(0.f, G(0.f, -.6f), -.6f); ChestYaw = 180.f;
        break;
    }
    case EVortexPoi::Graveyard:
    {
        // muro baixo em volta (portão na frente), lápides e cruzes, árvore seca e o mausoléu no fundo
        for (int32 K = -6; K <= 6; ++K)
        {
            if (FMath::Abs(K) <= 1) { M.Pillar(K, -5.f, G(K, -5.f), G(K, -5.f) + .2f, 1.f, .7f, Shade(P.Stone2)); }
            else if (R.FRand() < .85f) M.Pillar(K, -5.f, G(K, -5.f), G(K, -5.f) + .8f, 1.f, .7f, Shade(P.Stone));
            if (R.FRand() < .85f) M.Pillar(K, 5.f, G(K, 5.f), G(K, 5.f) + .8f, 1.f, .7f, Shade(P.Stone));
        }
        for (int32 K = -4; K <= 4; ++K)
            for (const float X : { -6.5f, 6.5f })
                if (R.FRand() < .85f) M.Pillar(X, K, G(X, K), G(X, K) + .8f, .7f, 1.f, Shade(P.Stone));
        for (int32 Row = 0; Row < 3; ++Row) for (int32 Col = 0; Col < 4; ++Col)
        {
            if (R.FRand() < .2f) continue;
            const float X = -4.f + Col * 2.4f + R.FRandRange(-.3f, .3f), Z = -2.6f + Row * 2.2f;
            const float Gr = G(X, Z);
            if (R.FRand() < .3f)
            {
                M.Pillar(X, Z, Gr, Gr + 1.5f, .2f, .2f, Shade(P.Stone2));
                M.Box(FVector(X, Gr + 1.1f, Z), FVector(.8f, .2f, .2f), Shade(P.Stone2));
            }
            else M.Pillar(X, Z, Gr, Gr + R.FRandRange(.9f, 1.4f), .8f, .25f, Shade(P.Stone));
            M.Box(FVector(X, Gr + .1f, Z - 1.f), FVector(.8f, .2f, 1.5f), PoiHex(TEXT("3a2a1e")));
        }
        const float TreeG = G(5.f, -3.f);
        M.Pillar(5.f, -3.f, TreeG, TreeG + 4.2f, .5f, .5f, PoiHex(TEXT("2e2622")));
        M.Box(FVector(5.6f, TreeG + 3.2f, -3.f), FVector(1.4f, .25f, .25f), PoiHex(TEXT("2e2622")));
        M.Box(FVector(4.5f, TreeG + 3.8f, -2.7f), FVector(1.f, .25f, .25f), PoiHex(TEXT("2e2622")));
        M.Box(FVector(5.f, TreeG + 4.6f, -3.3f), FVector(.25f, .9f, .25f), PoiHex(TEXT("2e2622")));
        const float Tomb = G(0.f, 3.5f);
        M.Pillar(0.f, 3.5f, Tomb, Tomb + 2.6f, 3.2f, 2.4f, Shade(P.Stone2));
        M.Box(FVector(0.f, Tomb + 2.8f, 3.5f), FVector(3.8f, .4f, 3.f), Shade(P.Dark));
        M.Box(FVector(0.f, Tomb + 3.2f, 3.5f), FVector(2.6f, .4f, 2.f), Shade(P.Dark));
        M.Box(FVector(0.f, Tomb + 1.f, 2.28f), FVector(1.1f, 2.f, .06f), FLinearColor(.01f, .01f, .012f));
        for (const float X : { -.9f, .9f }) M.Box(FVector(X, Tomb + 2.f, 2.25f), FVector(.15f, .3f, .08f), P.Accent);
        bChest = true; bGuarded = true; ChestAt = FVector(0.f, G(0.f, 1.2f), 1.2f);
        break;
    }
    case EVortexPoi::Caravan:
    {
        // a carroça quebrada no chão, a roda caída, caixotes, barris e sacos espalhados
        const float Gw = G(0.f, 0.f);
        M.Box(FVector(0.f, Gw + .35f, 0.f), FVector(2.2f, .3f, 3.4f), P.Wood);
        for (const float X : { -1.05f, 1.05f }) M.Box(FVector(X, Gw + .75f, .3f), FVector(.12f, .6f, 2.6f), Shade(P.Wood));
        M.Box(FVector(0.f, Gw + .75f, 1.65f), FVector(2.2f, .6f, .12f), Shade(P.Wood));
        M.Box(FVector(0.f, Gw + .3f, -2.4f), FVector(.15f, .15f, 1.8f), P.Wood);
        const float Wheel = G(2.4f, -.8f);
        M.Box(FVector(2.4f, Wheel + .08f, -.8f), FVector(1.3f, .12f, .25f), PoiHex(TEXT("4a3424")));
        M.Box(FVector(2.4f, Wheel + .08f, -.8f), FVector(.25f, .12f, 1.3f), PoiHex(TEXT("4a3424")));
        M.Box(FVector(2.4f, Wheel + .12f, -.8f), FVector(.4f, .14f, .4f), P.Dark);
        for (int32 K = 0; K < 6; ++K)
        {
            const float A = R.FRand() * UE_TWO_PI, D = R.FRandRange(2.2f, 4.f);
            const float X = FMath::Cos(A) * D, Z = FMath::Sin(A) * D, Gr = G(X, Z);
            const int32 What = R.RandRange(0, 2);
            if (What == 0) { M.Box(FVector(X, Gr + .45f, Z), FVector(.9f, .9f, .9f), P.Wood); M.Box(FVector(X, Gr + .45f, Z), FVector(.94f, .12f, .94f), P.Dark); }
            else if (What == 1) { M.Box(FVector(X, Gr + .5f, Z), FVector(.7f, 1.f, .7f), PoiHex(TEXT("6a4426"))); M.Box(FVector(X, Gr + .75f, Z), FVector(.74f, .08f, .74f), P.Dark); }
            else M.Box(FVector(X, Gr + .25f, Z), FVector(.8f, .5f, .55f), PoiHex(TEXT("b8a07a")));
        }
        bChest = true; ChestAt = FVector(-1.6f, G(-1.6f, 1.8f), 1.8f); ChestYaw = 30.f;
        break;
    }
    case EVortexPoi::Dock:
    {
        // píer de tábuas entrando na água (a frente, -Z), estacas, barco a remo e barris
        const float Water = (Data->WaterLevel / 100.f - Base) / PoiBlock;
        for (int32 K = -1; K < 11; ++K)
        {
            const float Z = 1.f - K;
            const float Deck = FMath::Max(G(0.f, Z) + .25f, Water + .45f);
            M.Box(FVector(0.f, Deck, Z), FVector(2.4f, .15f, .95f), Shade(P.Wood));
            if (K % 2 == 0)
                for (const float X : { -1.15f, 1.15f }) M.Box(FVector(X, Deck - 1.4f, Z), FVector(.22f, 3.f, .22f), PoiHex(TEXT("3a2a1c")));
        }
        const float Boat = Water + .2f;
        M.Box(FVector(2.6f, Boat, -7.f), FVector(1.2f, .4f, 3.f), PoiHex(TEXT("5a3a22")));
        M.Box(FVector(2.6f, Boat + .3f, -7.f), FVector(1.f, .2f, 2.8f), PoiHex(TEXT("7a5232")));
        M.Box(FVector(2.6f, Boat + .45f, -7.f), FVector(.9f, .08f, .3f), PoiHex(TEXT("7a5232")));
        for (const float Z : { 0.f, -.8f })
        {
            const float Deck = FMath::Max(G(-.7f, Z) + .25f, Water + .45f);
            M.Box(FVector(-.7f, Deck + .55f, Z), FVector(.6f, .9f, .6f), PoiHex(TEXT("6a4426")));
        }
        const float Lamp = FMath::Max(G(1.f, -9.f) + .25f, Water + .45f);
        M.Box(FVector(1.f, Lamp + 1.f, -9.f), FVector(.15f, 2.f, .15f), P.Wood);
        M.Box(FVector(1.f, Lamp + 2.05f, -9.f), FVector(.35f, .35f, .35f), PoiHex(TEXT("ffd27a")));
        break;
    }
    default: break;
    }

    if (M.Vertices.Num())
    {
        UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetUsingAbsoluteLocation(true);
        Mesh->SetUsingAbsoluteRotation(true);
        Mesh->bUseComplexAsSimpleCollision = true;
        Mesh->RegisterComponent();
        Mesh->CreateMeshSection_LinearColor(0, M.Vertices, M.Triangles, M.Normals, {}, M.Colors, {}, true);
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        if (Material) Mesh->SetMaterial(0, Material);
        B.Parts.Add(Mesh);
    }
    // santuário: a luz da runa e o cristal que flutua em cima do obelisco
    if (Poi.Kind == EVortexPoi::Shrine && GetNetMode() != NM_DedicatedServer)
    {
        const FVector Top = M.ToUnreal(FVector(0.f, 5.2f, 0.f));
        UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
        Light->SetupAttachment(RootComponent);
        Light->SetUsingAbsoluteLocation(true);
        Light->SetWorldLocation(Top);
        Light->SetLightColor(BlessingColor(Poi.Variant));
        Light->SetAttenuationRadius(1100.f);
        Light->SetCastShadows(false);
        Light->RegisterComponent();
        B.Parts.Add(Light);
        B.Light = Light;
        FPoiBoxes Gem;
        Gem.Scale = PoiBlock;
        Gem.Box(FVector(0.f, 0.f, 0.f), FVector(.45f, .7f, .45f), BlessingColor(Poi.Variant));
        Gem.Box(FVector(0.f, 0.f, 0.f), FVector(.6f, .3f, .6f), BlessingColor(Poi.Variant) * .7f);
        USceneComponent* Pivot = NewObject<USceneComponent>(this);
        Pivot->SetupAttachment(RootComponent);
        Pivot->SetUsingAbsoluteLocation(true);
        Pivot->SetWorldLocation(Top);
        Pivot->RegisterComponent();
        UProceduralMeshComponent* Crystal = NewObject<UProceduralMeshComponent>(this);
        Crystal->SetupAttachment(Pivot);
        Crystal->RegisterComponent();
        Crystal->CreateMeshSection_LinearColor(0, Gem.Vertices, Gem.Triangles, Gem.Normals, {}, Gem.Colors, {}, false);
        if (Material) Crystal->SetMaterial(0, Material);
        B.Parts.Add(Pivot);
        B.Parts.Add(Crystal);
        B.Crystal = Crystal;
    }
    if (bChest)
        VortexWorldHooks::AddCampChest(GetWorld(), Poi.Id, M.ToUnreal(ChestAt), Poi.Yaw + ChestYaw, Poi.Region, Poi.bRare, bGuarded);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_POI montado %s em (%.0f, %.0f) região %d chão %.1f m, peças fora %d"), *KindName(Poi.Kind), Poi.At.X, Poi.At.Y, Poi.Region, Base, M.Odd);
}
