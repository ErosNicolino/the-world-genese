#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "GameFramework/Actor.h"
#include "VortexForage.generated.h"

class UInstancedStaticMeshComponent;

/** Um coletável no chão (moita de amora, cogumelo), igual em todas as máquinas (sai da semente do lugar). */
struct FVortexForageSpot
{
    FName Id, Kind, Item;
    FString Name;
    FVector At = FVector::ZeroVector;
    float Scale = 1.f, Yaw = 0.f;
};

/**
 * Coletáveis do mundo (ValleyWorld.Scatter/Forage e Main.Forage do Godot): cogumelos ao pé das árvores
 * (vermelho ou marrom: "Chapéu de cogumelo") e moitas de amora no campo (metade dá frutas silvestres,
 * que curam e domam cavalos; metade, ervas curativas). Cada pedaço do terreno (18 m) gera os seus pela
 * posição, então todos veem os mesmos. Quem colhe pede ao servidor (AVortexCharacter::ServerHarvest);
 * a moita some para todos e volta em 5 minutos. A HUD mostra o brilho e "[E] Colher".
 */
UCLASS()
class VORTEX_API UVortexForageSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UVortexForageSubsystem, STATGROUP_Tickables); }

    /** O coletável de pé mais perto (até Range cm), ou nada. */
    const FVortexForageSpot* Nearest(const FVector& Where, float Range);
    /** Os de pé até Radius cm (para os brilhos). */
    void Nearby(const FVector& Where, float Radius, TArray<const FVortexForageSpot*>& Out);
    const FVortexForageSpot* Find(FName Id, const FVector& Near);
    bool IsAvailable(FName Id) const;
    /** Colhido (em cada máquina; o servidor decide e avisa): some e volta depois de Respawn segundos. */
    void MarkTaken(FName Id);
    static constexpr float Respawn = 300.f;
    static constexpr float Reach = 230.f;

private:
    TMap<FIntPoint, TArray<FVortexForageSpot>> Chunks;
    TMap<FName, double> TakenUntil;
    TWeakObjectPtr<class AVortexForageField> Field;
    float Clock = 0.f;
    bool bDirty = true;
    const TArray<FVortexForageSpot>& ChunkSpots(const FIntPoint& Chunk);
};

/** As moitas e cogumelos perto de quem joga, desenhados em lote (só visual). */
UCLASS()
class VORTEX_API AVortexForageField : public AActor
{
    GENERATED_BODY()
public:
    AVortexForageField();
    void Show(const TArray<const FVortexForageSpot*>& Spots);
private:
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UInstancedStaticMeshComponent>> Kinds;
    UInstancedStaticMeshComponent* KindMesh(FName Kind);
};
