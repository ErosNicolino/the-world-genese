#pragma once

#include "World/VortexWorldLayout.h"

#include "Subsystems/WorldSubsystem.h"
#include "GameFramework/Actor.h"
#include "VortexCaldera.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UNiagaraComponent;

/**
 * A Caldeira do Meteoro, a arena do Azhûr (o primeiro chefe), em todas as máquinas:
 * - perto dela (a 120 m) a arena de lava aparece (AVortexLavaArena, local, sem rede);
 * - o céu fica vermelho dentro da zona (RedSkyAt, lido pelo dia e noite), mais forte nas fases
 *   finais do chefe; brasas sobem e cinzas caem em volta da câmera;
 * - poças novas (os meteoros do chefe) chegam por AddPool (o chefe avisa todos pela rede).
 */
UCLASS()
class VORTEX_API UVortexCalderaSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UVortexCalderaSubsystem, STATGROUP_Tickables); }

    /** Centro da Caldeira (o posto do chefe em Data/Population/creature_posts.json). */
    static constexpr float CenterX = VortexWorldLayout::CalderaCenterX;
    static constexpr float CenterY = VortexWorldLayout::CalderaCenterY;
    /** Raio da zona vermelha (cm): cheio até ZoneInner, some até ZoneOuter. */
    static constexpr float ZoneInner = 4200.f;
    static constexpr float ZoneOuter = 6500.f;
    /** A borda da cratera (paredões de basalto) e até onde a floresta e a grama somem. */
    static constexpr float RimRadius = 4700.f;
    static constexpr float ClearRadius = VortexWorldLayout::CalderaClearRadius;
    /** Dentro da cratera (sem árvore, grama nem pedrinha do terreno: o chão é o de basalto dela). */
    static bool ClearsVegetation(float X, float Y) { return VortexWorldLayout::CalderaClearsVegetation(X, Y); }
    /** 0 fora da zona .. 1 dentro (o céu, a névoa e a luz avermelham). */
    float RedSkyAt(const FVector& Where) const;
    /** Fase do chefe (0 = dormindo ou longe): a zona fica mais escura e mais vermelha. */
    void SetBossPhase(int32 Phase) { BossPhase = Phase; }
    int32 GetBossPhase() const { return BossPhase; }
    /** Poça de lava nova (meteoro, cataclismo): em cada máquina; o servidor também fere quem pisa. */
    void AddPool(const FVector& At, float Radius, float Life);
    class AVortexLavaArena* GetArena() const { return Arena.Get(); }

private:
    TWeakObjectPtr<class AVortexLavaArena> Arena;
    TWeakObjectPtr<class AVortexEmbers> Embers;
    int32 BossPhase = 0;
};

/** As poças de lava da Caldeira: as fixas (da semente da arena) e as que os meteoros deixam. */
UCLASS()
class VORTEX_API AVortexLavaArena : public AActor
{
    GENERATED_BODY()
public:
    AVortexLavaArena();
    virtual void Tick(float DeltaSeconds) override;
    void AddPool(const FVector& At, float Radius, float Life);
    /** Poça embaixo deste ponto (no chão)? */
    bool InLava(const FVector& Feet) const;
    int32 PoolCount() const { return Pools.Num(); }
    /** Centro (no chão) e raio de uma poça já posta (testes). */
    bool PoolAt(int32 Index, FVector& OutCenter, float& OutRadius) const
    {
        if (!Pools.IsValidIndex(Index) || !Pools[Index].bPlaced) return false;
        OutCenter = FVector(Pools[Index].At.X, Pools[Index].At.Y, Pools[Index].Ground);
        OutRadius = Pools[Index].Radius;
        return true;
    }

private:
    struct FPool
    {
        FVector2D At;
        float Radius = 200.f;
        float Ground = 0.f;
        float Life = -1.f;      // < 0: para sempre
        float Age = 0.f;
        bool bPlaced = false;
        TArray<TWeakObjectPtr<USceneComponent>> Parts;
        TWeakObjectPtr<UNiagaraComponent> Fire;
        TWeakObjectPtr<AActor> Smoke;
    };
    TArray<FPool> Pools;
    /** Bolhas de lava: incham devagar e estouram (respingo e um sopro de fumaça). */
    struct FBubble { TWeakObjectPtr<UStaticMeshComponent> Ball; float Age = 0.f, Life = 1.f, Size = 1.f; };
    TArray<FBubble> Bubbles;
    float DamageTimer = 0.f, BubbleTimer = 0.f;
    bool bSeeded = false;
    void Seed();
    /** A cratera: chão de placas de basalto com rachaduras de lava, paredões em volta com duas entradas, agulhas de
     *  obsidiana e o pedaço da Estrela Partida meio enterrado. Uma vez, quando o chão do centro chega. */
    bool bCraterBuilt = false;
    void BuildCrater();
    /** O primeiro Selo no meio da cratera: as runas queimam laranja (corrompidas) enquanto o Azhûr vive e voltam ao
     *  ciano quando ele cai (o Coração de Azhûr reacende o Selo). */
    UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> SealRunes;
    float SealLit = 0.f;
    void Place(FPool& Pool, float Ground);
    void Remove(FPool& Pool);
};

/** Meteoro do chefe: rocha em brasa caindo do céu até a marca (só visual; o dano é da marca). */
UCLASS()
class VORTEX_API AVortexMeteor : public AActor
{
    GENERATED_BODY()
public:
    AVortexMeteor();
    virtual void Tick(float DeltaSeconds) override;
    void Start(const FVector& InLand, float InFlight);
private:
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Rock;
    FVector From = FVector::ZeroVector, Land = FVector::ZeroVector;
    float Flight = 1.f, Age = 0.f;
};

/** Brasas que sobem e cinzas que caem em volta da câmera, na zona vermelha (só visual). */
UCLASS()
class VORTEX_API AVortexEmbers : public AActor
{
    GENERATED_BODY()
public:
    AVortexEmbers();
    virtual void Tick(float DeltaSeconds) override;
    float Strength = 0.f;

private:
    UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Glow;
    UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Ash;
    struct FBit { FVector At, Velocity; float Life, Age, Size; bool bEmber; };
    TArray<FBit> Bits;
    float SpawnDebt = 0.f;
    FRandomStream Rng;
};
