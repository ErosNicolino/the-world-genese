#pragma once

#include "CoreMinimal.h"
#include "Misc/ScopeRWLock.h"

/**
 * Região do mundo exportada do Godot (Data/GodotTerrain, gerada por --export-ue-terrain no
 * projeto Godot): altura, camada e cor de cada célula de 0,75 m, e as plantas/pedras com posição,
 * giro e escala. Só leitura depois de carregada: as threads de montagem dos pedaços leem daqui.
 *
 * Coordenadas: o Godot usa metros com Y para cima; aqui X = x, Y = z, Z = y, em centímetros.
 */
struct FVortexTerrainProp
{
    uint16 Kind = 0;
    FVector Location = FVector::ZeroVector;   // cm, coordenadas Unreal
    float YawDegrees = 0.f;
    float Scale = 1.f;
};

struct FVortexTerrainSolid
{
    uint8 Shape = 0; // 0 cilindro, 1 esfera, 2 casco convexo, 3 caixa
    FVector Location = FVector::ZeroVector;
    float YawDegrees = 0.f;
    float A = 0.f, B = 0.f; // cm
    FVector Size = FVector::ZeroVector; // cm
    TArray<FVector> Points; // cm, relativos ao centro
};

class VORTEXWORLD_API FVortexTerrainData
{
public:
    static TSharedPtr<FVortexTerrainData> Load(const FString& Directory, FString& OutError);

    // Grade: células de CellSize cm; pedaços de CellsPerChunk células (18 m).
    int32 ChunkMinX = 0, ChunkMinY = 0, ChunkCountX = 0, ChunkCountY = 0, CellsPerChunk = 24;
    float CellSize = 75.f;            // cm
    float WaterLevel = 205.f;         // cm (abaixo disso é água)
    float WaterSurface = 195.f;       // cm (altura da lâmina)
    TArray<FString> Kinds;

    float ChunkSize() const { return CellSize * CellsPerChunk; }
    FBox2D PrototypeBounds() const
    {
        const float Size = ChunkSize();
        return FBox2D(FVector2D(ChunkMinX * Size, ChunkMinY * Size),
            FVector2D((ChunkMinX + ChunkCountX) * Size, (ChunkMinY + ChunkCountY) * Size));
    }
    bool ContainsWorldXY(float X, float Y) const
    {
        const FBox2D Bounds = PrototypeBounds();
        return X >= Bounds.Min.X && Y >= Bounds.Min.Y && X < Bounds.Max.X && Y < Bounds.Max.Y;
    }
    /** Há chão neste pedaço: a região exportada ou, com o continente carregado, qualquer pedaço (o mar não acaba). */
    bool HasChunk(const FIntPoint& Chunk) const;
    /** Pedaço da região exportada do Godot (o vale feito à mão). */
    bool IsExported(const FIntPoint& Chunk) const;

    // ---- o continente (data/world do Godot, ValleyWorld.Ground/Scatter): fora da região exportada as
    // células e as plantas saem na hora do mapa do mundo (relevo de 6 m, regiões, tom, gelo e lava),
    // com o ruído fino por cima; na borda da região exportada as duas alturas se misturam em 40 m.
    bool LoadWorld(const FString& Directory, FString& OutError);
    bool HasWorld() const { return World.IsValid(); }
    /** O mapa do jogo (cm): o continente com o mar em volta, ou só a região exportada. */
    FBox2D MapBounds() const;
    FIntPoint MapChunkMin() const;
    FIntPoint MapChunkCount() const;
    bool InMap(float X, float Y) const { return MapBounds().IsInside(FVector2D(X, Y)); }
    /** Região do continente no ponto (cm): 0 mar, 1 Valdora, 2 Brasafenda, 3 Raikor, 4 Nivaluz,
     *  5 Solcânion, 6 Aurivale, 7 Lodonegro, 8 Morthúria, 9 Verdalume. */
    int32 RegionAt(float X, float Y) const;
    FString RegionName(int32 Region) const;
    /** Centro de cada região (cm), para o nome no mapa. */
    FVector2D RegionCenter(int32 Region) const { return World.IsValid() && Region > 0 && Region < 10 ? World->Centers[Region] : FVector2D::ZeroVector; }
    /** Altura do chão (cm) sem montar o pedaço (só o relevo): para decidir postos e lugares no mundo todo. */
    float QuickGround(float X, float Y) const;
    /** Nível das criaturas do lugar (WorldMap.LevelAt/AreaLevel do Godot). */
    int32 AreaLevel(float X, float Y) const;
    /** Altura e cor de uma célula sem guardar nada (o mapa do mundo inteiro passa por aqui). */
    void SampleCell(int32 CellX, int32 CellY, float& OutHeight, FColor& OutColor) const;
    /** Pontes e cabanas do continente (world_features.json): centro (cm), giro (graus), comprimento (cm). */
    struct FFeature { FVector2D At; float Yaw = 0.f, Radians = 0.f, Length = 0.f; bool bBridge = true; int32 Region = 0; int32 Index = 0; };
    const TArray<FFeature>& Features() const { return WorldFeatures; }
    // Célula global (índice a partir de 0 no mundo, pode ser negativo).
    bool HasCell(int32 CellX, int32 CellY) const;
    float CellHeight(int32 CellX, int32 CellY) const;           // cm; fora da região: -300
    FColor CellColor(int32 CellX, int32 CellY) const;
    uint8 CellLayer(int32 CellX, int32 CellY) const;
    uint8 CellRotation(int32 CellX, int32 CellY) const;
    FLinearColor CellTint(int32 CellX, int32 CellY) const;
    // Altura do chão (em degraus) sob um ponto, em cm.
    float GroundAt(float X, float Y) const;
    bool IsWaterAt(float X, float Y, float& OutSurface) const;

    const TArray<FVortexTerrainProp>* PropsInChunk(const FIntPoint& Chunk) const;
    /** As plantas deste pedaço já existem (exportado ou continente já montado): ler não custa nada. */
    bool IsScattered(const FIntPoint& Chunk) const;
    const TArray<FVortexTerrainSolid>* SolidsInChunk(const FIntPoint& Chunk) const;
    bool IsTree(uint16 Kind) const { return Kinds.IsValidIndex(Kind) && Kinds[Kind].StartsWith(TEXT("tree")); }

private:
    struct FCell { int16 Height4; uint8 Layer; uint8 Rotation; uint8 R, G, B; };
    TArray<FCell> Cells;              // pedaço por pedaço (Y por fora, X por dentro), célula ix por fora
    TMap<FIntPoint, TArray<FVortexTerrainProp>> PropsByChunk;
    TMap<FIntPoint, TArray<FVortexTerrainSolid>> SolidsByChunk;
    const FCell* Find(int32 CellX, int32 CellY) const;
    /** A célula de qualquer lugar: a exportada ou a do continente (guardada por pedaço). */
    bool Cell(int32 CellX, int32 CellY, FCell& Out) const;

    // continente
    struct FWorldMap
    {
        int32 Size = 0;
        float Mpp = 6.f, HomeU = 0.f, HomeV = 0.f;
        TArray<float> Height;
        TArray<uint8> Region, Tint, Flags;
        FString Names[10];
        FVector2D Centers[10];
    };
    TSharedPtr<FWorldMap> World;
    TArray<FFeature> WorldFeatures;
    TMap<FString, uint16> KindIndex;
    uint16 KindOf(const TCHAR* Name);
    struct FSynthChunk
    {
        TArray<FCell> Cells;
        TArray<FVortexTerrainProp> Props;
        TArray<FVortexTerrainSolid> Solids;
        bool bScatter = false;
    };
    mutable FRWLock SynthLock;
    mutable TMap<FIntPoint, TSharedPtr<FSynthChunk>> Synth;
    mutable TMap<FIntPoint, TSharedPtr<FSynthChunk>> SynthOld;
    TSharedPtr<FSynthChunk> SynthChunk(const FIntPoint& Chunk, bool bScatter) const;
    void SynthCell(int32 CellX, int32 CellY, FCell& Out) const;
    void SynthScatter(const FIntPoint& Chunk, FSynthChunk& Out) const;
    // Godot (metros, x e z): relevo, região, mata, chão e tom
    float WorldHeight(float X, float Z) const;
    float WorldGround(float X, float Z) const;
    int32 WorldRegion(float X, float Z) const;
    int32 MixedRegion(float X, float Z) const;
    FLinearColor WorldTint(float X, float Z) const;
    uint8 WorldFlag(float X, float Z) const;
    float Forest(float X, float Z, int32 Region) const;
    uint8 RegionLayer(float X, float Z, float H, int32 Region, uint32 Seed) const;
    const TCHAR* TreeKind(float X, float Z, float H, int32 Region) const;
};
