#pragma once

#include "GameFramework/Actor.h"
#include "VortexFallingLeaves.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * Folhas caindo das árvores (10/11). Só enfeite em cada máquina que desenha (sem colisão, sem rede), como as fadas.
 * Cada folha sai da copa de uma árvore perto da câmera (altura e largura tiradas do modelo da árvore), com a cor e a
 * frequência do tipo de árvore (Data/World/folhas.json: outono e cerejeira soltam muito, pinheiros nada). Na queda:
 * o vento da grama (a mesma direção e as mesmas rajadas do M_GrassWind no ponto onde a folha está) a leva, e ela
 * balança de um lado para o outro inclinando, como folha de verdade. No chão fica deitada uns segundos e some; na
 * água boia e vai devagar com o vento. A folha é uma plaquinha feita aqui; o desenho de pixel art é do material
 * (M_FallingLeaf, Tools/CreateFallingLeafMaterial.py). -VortexNoFallingLeaves desliga; -VortexLeavesDebug solta muitas
 * de uma vez (fotos).
 */
UCLASS(Transient)
class VORTEXWORLD_API AVortexFallingLeaves : public AActor
{
    GENERATED_BODY()

public:
    AVortexFallingLeaves();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    struct FKindInfo { float Rate = 0.f; TArray<FLinearColor> Colors; float Top = 0.f, Low = 0.f, Radius = 0.f; };
    struct FTree { FVector At = FVector::ZeroVector; float Scale = 1.f, Weight = 0.f; int32 Kind = 0; };
    enum class EState : uint8 { Waiting, Falling, Ground, Water };
    struct FLeaf
    {
        FVector Pos = FVector::ZeroVector, Vel = FVector::ZeroVector;
        float Phase = 0.f, Swing = 1.f, Spin = 0.f, Yaw = 0.f, Size = 1.f, Timer = 0.f, Fade = 1.f;
        EState State = EState::Waiting;
    };

    UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Leaves;
    TArray<FLeaf> Leaf;
    TMap<FString, FKindInfo> ByName; // do folhas.json
    TMap<uint16, FKindInfo> Kinds;   // pelo índice do tipo no FVortexTerrainData (montado quando o terreno chega)
    bool bKindsReady = false;
    TArray<FTree> Trees;             // as árvores com folha perto da câmera (refeito a cada 2 s)
    float TotalWeight = 0.f, ScanTimer = 0.f;
    FVector LastScan = FVector(1e9f);   // onde a câmera estava na última busca (um pulo grande refaz tudo)
    TMap<FString, float> Tune;
    FRandomStream Rng = FRandomStream(7741);
    bool bDebug = false;
    FVector Eye = FVector::ZeroVector, Look = FVector::ForwardVector;   // a câmera (o teste solta algumas bem na frente)

    float T(const TCHAR* Key, float Default) const { const float* V = Tune.Find(Key); return V ? *V : Default; }
    UStaticMesh* BuildLeaf(UMaterialInterface* Surface);
    void ScanTrees(const FVector& Camera);
    /** bMidAir: já no meio da queda (entre a copa e o chão), para o lugar não começar vazio. */
    bool Spawn(int32 Index, bool bMidAir = false);
    FVector WindAt(const FVector& P, float Time) const;
};
