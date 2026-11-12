#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "Persistence/VortexSaveTypes.h"
#include "Framework/VortexLook.h"
#include "VortexPopulationSubsystem.generated.h"

class AVortexNpc;
class AVortexTerrainStreamer;
class UVortexWorldSave;

/**
 * População do vale (Fase 14), só no servidor e só em mapas com terreno (L_TerrainProto).
 * Os postos vêm de Data/Population/npc_posts.json (Tools/ExtractNpcPosts.py, a partir do Godot).
 * Cada posto tem ID estável e estado próprio (FVortexNpcState); o ator AVortexNpc é só a aparição:
 * - aparece quando algum jogador está a menos de MaterializeRadius e o chão de colisão do ponto já
 *   existe, no máximo SpawnsPerUpdate por atualização;
 * - some quando todos estão além de DematerializeRadius (maior: sem alternância na divisa),
 *   guardando posição, giro e ponto da patrulha;
 * - postos fora do protótipo de terreno ficam só como estado, sem ator.
 * Longe de todos, o NPC não é simulado: o estado fica parado até alguém voltar.
 */
UCLASS()
class VORTEX_API UVortexPopulationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    float MaterializeRadius = 6400.f;     // cm, como os 64 m do Godot
    float DematerializeRadius = 7800.f;   // cm, como os 78 m do Godot
    int32 SpawnsPerUpdate = 2;
    float UpdateInterval = .25f;

    bool IsActive() const { return bActive; }
    int32 NpcCount() const { return Records.Num(); }
    int32 InPrototypeCount() const;
    int32 MaterializedCount() const;
    const FVortexNpcState* FindState(FName Id) const;
    AVortexNpc* FindActor(FName Id) const;
    void WriteTo(UVortexWorldSave& Save) const;
    void ReadFrom(const UVortexWorldSave& Save);

    // Medições
    int32 Spawned = 0, Despawned = 0;
    double UpdateMsMax = 0.0, UpdateMsTotal = 0.0, SpawnMsMax = 0.0;
    int32 Updates = 0;

private:
    struct FRecord
    {
        FName Id;
        FString Name, Title;
        FVector2D Post = FVector2D::ZeroVector;
        float Yaw = 0.f;
        TArray<FVector2D> Patrol;
        FVortexLook Look;        // peças e cores (Godot npcs.json ou npc_extra.json do Vortex)
        bool bHasLook = false;
        FName Pet;               // bichinho que segue o NPC ("dog")
        bool bInPrototype = false;
        bool bChild = false;     // 08/11: os passantes crianças (VortexNpcRoutine::Extras)
        FVortexNpcState State;
        TWeakObjectPtr<AVortexNpc> Actor;
    };

    TArray<FRecord> Records;
    TArray<FVortexNpcState> UnknownStates;   // IDs de um save que não estão nos dados: preservados
    FTimerHandle UpdateHandle;
    bool bActive = false;

    bool LoadPosts();
    void Update();
    static void Capture(FRecord& Record);
    bool Materialize(FRecord& Record, AVortexTerrainStreamer* Streamer);
};
