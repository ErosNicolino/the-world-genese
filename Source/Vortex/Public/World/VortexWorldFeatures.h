#pragma once

#include "GameFramework/Actor.h"
#include "VortexWorldFeatures.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class UVortexQuestLog;

/**
 * Pontes sobre os rios e cabanas abandonadas com baú (ValleyWorld.Features e Main.Chests do Godot,
 * Data/GodotWorld/world_features.json). Cada uma é montada em blocos de madeira e pedra quando alguém
 * chega a 200 m (com colisão: dá para atravessar a ponte e entrar na cabana) e sai quando todos se
 * afastam. O baú abre com E (o servidor sorteia o saque pela região); aberto fica aberto no save de
 * cada herói, como no Godot (em grupo, cada um abre o seu).
 */
UCLASS()
class VORTEX_API AVortexWorldFeatures : public AActor
{
    GENERATED_BODY()
public:
    AVortexWorldFeatures();
    virtual void Tick(float DeltaSeconds) override;

    struct FChest
    {
        FName Id;
        int32 Feature = INDEX_NONE, Region = 0;
        bool bRare = false;
        FVector Location = FVector::ZeroVector;
        /** A tampa (gira na dobradiça) e o corpo: o baú em blocos (Tools/Blender/MakeChest.py) ou, sem ele, caixas. */
        TWeakObjectPtr<USceneComponent> Lid, Body;
        float OpenAge = -1.f, LidYaw = 0.f;
        /** Baú de acampamento de bandidos: só abre com o bando (perto dele) vencido. */
        bool bGuarded = false;
    };
    /** O baú guardado de um acampamento de bandidos (VortexLandmarks): o mesmo baú das cabanas, no chão em
     *  Ground (cm), virado Yaw graus; sai com RemoveChest quando o acampamento desmonta. */
    void AddCampChest(FName Id, const FVector& Ground, float Yaw, int32 Region, bool bRare, bool bGuarded = true);
    void RemoveChest(FName Id);
    /** Criaturas hostis vivas perto do baú (o bando, os goblins da mina, os mortos do cemitério). */
    static int32 GuardsAlive(const UWorld* World, const FVector& At);
    static AVortexWorldFeatures* Find(const UWorld* World);
    /** O baú fechado (para esse herói) mais perto, até Range cm. */
    const FChest* NearestChest(const FVector& At, float Range, const UVortexQuestLog* Log) const;
    /** Todas as máquinas: a tampa abre (o herói que abriu vê a animação). */
    void PlayOpen(FName Id);
    int32 BuiltCount() const { return Built.Num(); }
    static constexpr float BuildRange = 20000.f;
    static constexpr float DropRange = 28000.f;

private:
    /** O baú em blocos no lugar (o corpo no chão em Floor, virado Yaw; a tampa na dobradiça). false sem o modelo. */
    bool AddChestModel(FChest& Chest, const FVector& Floor, float Yaw);
    UPROPERTY(Transient) TMap<int32, TObjectPtr<UProceduralMeshComponent>> Built;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> Material;
    TArray<FChest> Chests;
    void BuildFeature(int32 Index);
    void DropFeature(int32 Index);
};
