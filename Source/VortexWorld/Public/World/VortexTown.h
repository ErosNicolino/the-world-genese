#pragma once

#include "GameFramework/Actor.h"
#include "VortexTown.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;

/**
 * Cidade do Santuário, a vila inicial do Godot (ValleyWorld.Town.cs + Village.cs), montada a partir de
 * Data/GodotTown/town.json (Tools/BuildTownLayout.py): muralhas com torres e dois portões, a taverna,
 * os sobrados, lojas e chalés, a torre do sino, as três oficinas em volta da praça da fonte, barracas,
 * carroça, poço, quadro de missões, bancos, jardineiras, lampiões, árvores e as pedras da praça e das
 * ruas. Os modelos de voxel vêm do Godot (Tools/ImportGodotProps.py). Cada máquina monta a sua (não
 * vai pela rede); as caixas de colisão são as do Godot. Os lampiões acendem à noite.
 */
UCLASS()
class VORTEXWORLD_API AVortexTown : public AActor
{
    GENERATED_BODY()
public:
    AVortexTown();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    /** Monta a cidade neste mundo (uma vez). */
    static AVortexTown* Ensure(UWorld* World);
    /** Dentro das muralhas (cm, Unreal). */
    static bool Contains(const FVector& Where);
    /**
     * Sem grama aqui (cm): pedras da praça e das ruas e o chão das construções. Lê a planta uma vez
     * (LoadGrassMask, na thread do jogo); depois é só leitura, segura nas tarefas do terreno.
     */
    static bool BlocksGrass(float X, float Y);
    /** 11/11: na frente dos portões (vila.json "clearings"): sem árvores, arbustos, pedras nem grama alta. */
    static bool Clears(float X, float Y);
    /** Para o mapa (02/11): 0 nada, 1 chão de pedra (praça e ruas), 2 construção. Mesma planta e mesmas regras do
     *  BlocksGrass (só leitura depois do LoadGrassMask). */
    static int32 MapKindAt(float X, float Y);
    static void LoadGrassMask();
    int32 GetPieceCount() const { return PieceCount; }
    /** 07/11 (o usuário: "as portas, apertar uma tecla para abrir"): a porta da vila mais perto (até Range cm, pela
     *  maçaneta); bOpen diz se ela está aberta. ToggleDoor abre ou fecha (só nesta máquina, como o resto da vila). */
    static bool FindDoor(const UWorld* World, const FVector& At, float Range, bool& bOpen);
    static bool ToggleDoor(const UWorld* World, const FVector& At, float Range);

private:
    struct FDoor
    {
        TWeakObjectPtr<USceneComponent> Pivot;
        FRotator Closed = FRotator::ZeroRotator;
        FVector Handle = FVector::ZeroVector;    // o meio da folha fechada (mundo), para achar a mais perto
        float Angle = 0.f, Target = 0.f;
    };
    TArray<FDoor> Doors;
    FDoor* NearestDoor(const FVector& At, float Range);
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UInstancedStaticMeshComponent>> Batches;
    UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> Lamps;
    /** 03/11: a luz das janelas à noite (town_extra.json: glows). */
    UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> Windows;
    float LampLevel = -1.f;
    int32 PieceCount = 0;

    void Build();
    UInstancedStaticMeshComponent* Batch(FName Key, const FString& MeshPath, const FString& MaterialPath, bool bShadows);
};
