#pragma once

#include "GameFramework/Actor.h"
#include "VortexFarTerrain.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

/**
 * O relevo distante do continente (como o FarTerrain do Godot): uma malha só, de 18 m por ponto, até
 * 2,6 km em volta de quem joga, com a cor do chão de cada região, a neve dos picos e o mar. Assim as
 * montanhas, os vales e a costa aparecem no horizonte, atrás da névoa (agora mais leve).
 * - Tem um buraco onde o terreno de verdade está montado e fica um pouco abaixo dele; cada ponto usa a
 *   menor altura em volta, então a malha não fura um vale perto.
 * - É feita em blocos fixos no mundo (576 m, cada um uma seção da malha), montados em outra thread e
 *   guardados: andar só refaz os poucos blocos do buraco (a cada 20 m) e põe ou tira blocos na borda.
 *   Antes a malha inteira (85 mil pontos) era trocada a cada 40 m e travava o quadro.
 * - Só visual (sem colisão nem sombra); o servidor dedicado não monta.
 */
UCLASS()
class VORTEXWORLD_API AVortexFarTerrain : public AActor
{
    GENERATED_BODY()
public:
    AVortexFarTerrain();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    static constexpr float Radius = 260000.f;   // cm
    static constexpr float Step = 1800.f;       // cm (um pedaço do terreno)
    static constexpr float Hole = 9800.f;       // cm (o terreno de verdade vai até ~110 m)
    static constexpr float Sink = 250.f;        // cm abaixo do chão
    static constexpr int32 TilePoints = 32;     // pontos por lado de um bloco

    /** Testes: já tem malha montada. */
    bool IsBuilt() const { return bBuilt; }

private:
    struct FFarTile
    {
        FIntPoint Key = FIntPoint::ZeroValue;
        TArray<FVector> Vertices;
        TArray<FVector> Normals;
        TArray<FLinearColor> Colors;
        int32 Section = -1;
        bool bHoled = false;   // montado com triângulos cortados pelo buraco
    };
    UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Mesh;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> Material;
    TFuture<TArray<TSharedPtr<FFarTile>>> Job;
    TMap<FIntPoint, TSharedPtr<FFarTile>> Tiles;
    TArray<TSharedPtr<FFarTile>> Ready;   // montados, esperando a vez de virar seção
    TArray<int32> FreeSections;
    int32 NextSection = 0;
    FVector2D HoleAt = FVector2D(1e12f, 1e12f);
    bool bBuilt = false, bFogTuned = false;
    static float TileWorld() { return TilePoints * Step; }
    static TSharedPtr<FFarTile> BuildTile(FIntPoint Key);
    void Upload(FFarTile& Tile, const FVector2D& Hole);
    void TuneFog();
};
