#pragma once

#include "GameFramework/Actor.h"
#include "World/VortexTerrainData.h"
#include "VortexTerrainStreamer.generated.h"

class UProceduralMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UPrimitiveComponent;

/**
 * Protótipo de terreno (Fase 6T): monta em volta dos jogadores os pedaços de 18 m da região
 * exportada do Godot.
 * - Malha visual em degraus (topo + paredões, cor de cada célula), sem colisão.
 * - Malha de colisão suave separada (a regra da Fase 5): o canto de cada célula fica na média das
 *   quatro células em volta, então degraus de 25-50 cm viram rampa andável e paredões de 1 m ou
 *   mais continuam íngremes demais para subir (> 45 graus).
 * - Água onde o chão está abaixo do nível, ligada à água por dados (UVortexWaterSubsystem).
 * - Plantas pequenas, moitas e pedras em lote por pedaço; árvores em lote por região de 72 m até
 *   TreeRange (Nanite cuida do detalhe pela distância).
 * A geometria de cada pedaço é calculada numa tarefa de fundo; a thread do jogo só entrega as
 * malhas, com orçamento por quadro.
 * Fase 13: a área desejada é a união das áreas de todos os jogadores. Quem desenha a tela nesta
 * máquina pede pedaços completos até StreamRadius; no servidor, cada jogador (inclusive os
 * remotos) pede só colisão até CollisionRadius. Os mais próximos de alguém saem primeiro, a
 * liberação tem um pedaço de folga e uma tarefa pronta para um pedaço que já saiu da área é
 * descartada em vez de entregue. O servidor dedicado não monta nada visual.
 */
UCLASS()
class VORTEXWORLD_API AVortexTerrainStreamer : public AActor
{
    GENERATED_BODY()

public:
    AVortexTerrainStreamer();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, Category = "Vortex|Terrain")
    int32 StreamRadius = 6;            // pedaços (o Godot usa 6 na distância de visão média)

    UPROPERTY(EditAnywhere, Category = "Vortex|Terrain")
    int32 CollisionRadius = 2;

    UPROPERTY(EditAnywhere, Category = "Vortex|Terrain")
    int32 ChunksPerFrame = 1;

    UPROPERTY(EditAnywhere, Category = "Vortex|Terrain")
    // cm. Era 300 m (as árvores em imagem do Godot), mas o chão só existe até ~110-126 m: além
    // disso as copas ficavam soltas no vazio. A névoa da Fase 12 fecha por volta de 130 m.
    float TreeRange = 15000.f;

    UPROPERTY(EditAnywhere, Category = "Vortex|Terrain")
    // cm. Antes 32/58/30 m: plantas e grama surgiam à vista, antes da névoa engrossar (~70 m)
    float SmallPropRange = 6000.f;

    UPROPERTY(EditAnywhere, Category = "Vortex|Terrain")
    float PropRange = 9000.f;

    UPROPERTY(EditAnywhere, Category = "Vortex|Terrain")
    float GrassRange = 4500.f;         // cm: a grama com vento some (por cópia) até aqui

    static TSharedPtr<FVortexTerrainData> SharedData();
    /** Opções gráficas: material da água pela qualidade e distâncias da grama e das plantas (já nos pedaços prontos). */
    void ApplyGraphicsSettings();
    /** A água da qualidade escolhida (a fonte da cidade usa a mesma). */
    UMaterialInterface* GetWaterMaterial() const { return WaterMaterial; }
    float GroundAt(float X, float Y) const { return Data ? Data->GroundAt(X, Y) : 0.f; }
    bool GroundReadyAt(float X, float Y, float& OutGroundZ) const;
    bool IsReady() const { return Data.IsValid(); }

    // Pedaços com chão de colisão em volta de cada jogador (servidor: também os remotos). 4 pedaços
    // garantem chão a pelo menos 72 m, além da materialização de NPCs (Fase 14) e dos lobos.
    // Os corpos de árvores e rochas continuam só até CollisionRadius.
    UPROPERTY(EditAnywhere, Category = "Vortex|Terrain")
    int32 SimulationRadius = 4;

    // Ao entrar no jogo: tela escura até a área em volta estar montada, com mais pedaços por
    // quadro nesse tempo (não se vê o mundo se montando). Limite de segurança em segundos.
    UPROPERTY(EditAnywhere, Category = "Vortex|Terrain")
    int32 InitialChunksPerFrame = 6;

    UPROPERTY(EditAnywhere, Category = "Vortex|Terrain")
    float InitialFillTimeout = 15.f;

    bool IsInitialFillDone() const { return bInitialFillDone; }

    // Pedaços que saíram da área soltos por quadro
    UPROPERTY(EditAnywhere, Category = "Vortex|Terrain")
    int32 RetirePerFrame = 6;

    // Pedaços de colisão novos por quadro (o do jogador primeiro)
    UPROPERTY(EditAnywhere, Category = "Vortex|Terrain")
    int32 CollisionMeshesPerFrame = 2;

    // Medições
    int32 ChunksBuilt = 0;
    double ChunkGameThreadMsTotal = 0.0, ChunkGameThreadMsMax = 0.0;
    double ChunkGroundMs = 0.0, ChunkPropsMs = 0.0, ChunkTuftMs = 0.0;
    int32 StaleDiscarded = 0;          // tarefas prontas cujo pedaço já não era desejado
    double TickMsMax = 0.0, TickMsTotal = 0.0;   // custo do Tick inteiro na thread do jogo
    int32 Ticks = 0;
    double PartMsMax[5] = {};          // área, liberação, entrega, colisão, árvores
    void ResetTickStats() { TickMsMax = 0.0; TickMsTotal = 0.0; Ticks = 0; for (double& Ms : PartMsMax) Ms = 0.0; }
    int32 PendingChunks() const { return Pending.Num() + Building.Num(); }
    int32 VisualChunkCount() const;
    int32 CollisionChunkCount() const;
    int32 LoadedChunkCount() const { return Chunks.Num(); }
    int32 TreeRegionCount() const { return TreeRegions.Num(); }
    // pedaços montados fora da área de qualquer jogador (com a folga): deve ser sempre 0
    int32 ChunksOutsideKeep() const;
    bool HasVisualChunk(const FIntPoint& Chunk) const;
    bool IsChunkLoaded(const FIntPoint& Chunk) const { return Chunks.Contains(Chunk); }
    FIntPoint ChunkAt(const FVector& Location) const;

private:
    // Nível que cada pedaço precisa: Collision só para o servidor simular alguém ali; Full também
    // com malha, objetos e grama, para quem desenha a tela nesta máquina.
    enum class ELevel : uint8 { None, Collision, Full };

    struct FChunkCollision
    {
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
    };
    struct FChunkMesh
    {
        bool bVisual = false;
        TArray<FVector> Vertices, Normals;
        TArray<int32> Triangles;
        TArray<FLinearColor> Colors;
        TArray<FVector2D> UV0;
        TArray<FVector> WaterVertices, WaterNormals;
        TArray<FVector2D> WaterUV0;      // posição no mundo / 10 m (escala das ondas do material)
        TArray<int32> WaterTriangles;
        TSharedPtr<FChunkCollision> Collision;
        TArray<FTransform> GrassGroups[12];
        TArray<float> GrassData[12];   // 4 por cópia: cor do chão (linear) e secura da região
        TMap<uint16, TArray<FTransform>> PropGroups;
    };
    struct FChunk
    {
        TObjectPtr<UProceduralMeshComponent> Visual;
        TObjectPtr<UProceduralMeshComponent> Collision;
        TArray<TObjectPtr<UPrimitiveComponent>> SolidBodies;
        TArray<int32> BatchSlots;   // em quais grupos da região este pedaço tem cópias
        FIntPoint BatchRegion = FIntPoint::ZeroValue;
        int32 Epoch = 0;            // número único desta montagem (as cópias nos grupos guardam ele)
        // só a colisão fica guardada; a malha visual é solta depois de entregue
        TSharedPtr<FChunkCollision> CollisionData;
        bool bVisual = false;
    };
    struct FBuild
    {
        UE::Tasks::TTask<TSharedPtr<FChunkMesh>> Task;
        bool bVisual = false;
    };
    struct FViewers
    {
        TArray<FIntPoint> Local;       // pedaço de cada jogador que desenha a tela aqui
        TArray<FIntPoint> Players;     // pedaço de cada jogador simulado aqui (servidor: todos)
        bool operator==(const FViewers& Other) const { return Local == Other.Local && Players == Other.Players; }
    };

    TSharedPtr<FVortexTerrainData> Data;
    TMap<FIntPoint, FChunk> Chunks;
    TMap<FIntPoint, ELevel> Wanted;
    TArray<FIntPoint> Pending;
    TMap<FIntPoint, FBuild> Building;
    FViewers LastViewers;
    bool bHasViewers = false;
    TArray<FChunk> Retiring;          // fora da área, esperando a vez de soltar os componentes
    TArray<FIntPoint> StripQueue;     // só chão agora: a parte visual sai aos poucos
    bool bTreesPrimed = false;
    bool bInitialFillStarted = false, bInitialFillDone = false;
    float InitialFillTime = 0.f;
    void UpdateInitialFill(float DeltaSeconds);
    float SmokeStatsTime = 0.f;
    FIntPoint SmokeReportedChunk = FIntPoint(MAX_int32, MAX_int32);
    float SolidProbeTime = 0.f;
    bool bSolidProbeReported = false;

    // árvores por região (só onde há quem desenhe a tela)
    TMap<FIntPoint, TArray<TObjectPtr<UInstancedStaticMeshComponent>>> TreeRegions;
    /**
     * Plantas, pedras e grama agrupadas por região de 4x4 pedaços (72 m) e por tipo: um grupo de cópias
     * por tipo e região, e não um por pedaço (eram ~3000 grupos na cena; cada um custa à thread de
     * renderização e vira um pedido de desenho por passe). Cada cópia guarda o número da montagem do
     * pedaço; o pedaço que sai tira só as suas (troca com a última, sem refazer o grupo), e um pedaço
     * novo no mesmo lugar nunca perde cópias para o antigo que ainda está na fila de saída.
     */
    struct FBatch
    {
        TObjectPtr<UInstancedStaticMeshComponent> Ism;
        TArray<int32> Owner;
    };
    TMap<FIntVector, FBatch> Batches;   // (região X, região Y, tipo: espécie de planta, 10000 + variante de grama ou 20000 + flor colorida)
    static constexpr int32 BatchChunks = 4;
    static constexpr int32 GrassSlot = 10000;
    /** Flores com 4 cores (M_VoxelProp "Variety"): a variante 1..3 de uma espécie vai num lote à parte,
     *  20000 + espécie * 4 + variante, com o material da espécie e "PetalHue" = variante. */
    static constexpr int32 PetalSlot = 20000;
    UPROPERTY(Transient) TMap<int32, TObjectPtr<UMaterialInstanceDynamic>> PetalMaterials;
    int32 NextEpoch = 0;
    FIntPoint BatchRegion(const FIntPoint& Chunk) const { return FIntPoint(FMath::FloorToInt(Chunk.X / float(BatchChunks)), FMath::FloorToInt(Chunk.Y / float(BatchChunks))); }
    UInstancedStaticMeshComponent* BatchFor(const FIntPoint& Region, int32 Slot);
    void BatchAdd(FChunk& Chunk, int32 Slot, const TArray<FTransform>& Transforms, const TArray<float>* CustomData);
    void BatchRemove(FChunk& Chunk);
    TArray<FIntPoint> LastTreeCenters;
    TArray<FIntPoint> TreeQueue;
    TSet<FIntPoint> TreeWarming;   // regiões com as plantas sendo montadas em outra thread

    UPROPERTY()
    TArray<TObjectPtr<UStaticMesh>> KindMeshes;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> TerrainMaterial;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> WaterMaterial;
    /** Água de cada qualidade: baixa (opaca), média (parada), alta, épica. */
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> WaterLevels;
    bool bWaterOverride = false;
    float GrassScale = 1.f, PropScale = 1.f, TreeScale = 1.f;
    // fração da grama montada (opção Densidade da vegetação); lida pelas tarefas de fundo
    float GrassDensity = 1.f;
    void SetPieceDistances(UInstancedStaticMeshComponent* Instances) const;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> GrassMaterial;
    UPROPERTY()
    TObjectPtr<class UMaterialParameterCollection> GrassStep;
    FVector GrassFeet[8];
    void UpdateGrassStep(const FVector& Viewer);
    static constexpr int32 GrassVariants = 12;
    UPROPERTY()
    TArray<TObjectPtr<UStaticMesh>> GrassMeshes;
    void BuildGrassMeshes();

    void GatherViewers(FViewers& Out, TArray<FVector>& OutLocal, TArray<FVector>& OutPlayers) const;
    static int32 ChebyshevTo(const FIntPoint& Key, const TArray<FIntPoint>& Centers);
    ELevel KeepLevel(const FIntPoint& Key) const;
    int32 Priority(const FIntPoint& Key) const;
    void UpdateWanted(const FViewers& Viewers);
    void LaunchAndAttach();
    TSharedPtr<FChunkMesh> BuildMesh(FIntPoint Chunk, bool bVisual) const;
    void Attach(const FIntPoint& Chunk, const TSharedPtr<FChunkMesh>& Mesh, bool bVisual);
    void UpdateCollision(const TArray<FIntPoint>& Centers);
    void CreateSolidCollision(const FIntPoint& Key, FChunk& Chunk);
    /** Componente de colisão de um pedaço com nome fixo (igual no servidor e no cliente: a rede reconhece o
     *  chão em que o herói pisa). Um antigo com o mesmo nome, ainda não recolhido, é renomeado antes. */
    class UProceduralMeshComponent* MakeChunkCollision(const TCHAR* Kind, const FIntPoint& Key);
    void UpdateTrees(const TArray<FIntPoint>& Centers);
    void ReleaseVisual(FChunk& Chunk);
    void Release(FChunk& Chunk);
    UInstancedStaticMeshComponent* NewInstances(uint16 Kind, float CullDistance, bool bShadows);
};
