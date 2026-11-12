#pragma once

#include "World/VortexWorldLayout.h"

#include "Subsystems/WorldSubsystem.h"
#include "VortexCreatureSubsystem.generated.h"

/**
 * Criaturas do vale (ValleyPopulation.cs do Godot), só no servidor e só em mapas com terreno.
 * - Postos fixos da área do vale (Data/Population/creature_posts.json: Tools/ExtractCreaturePosts.py)
 *   e o chefe da Caldeira (Azhûr, nível 12).
 * - Fora da área feita à mão, quadras de 72 m sorteadas pela posição (iguais em toda partida):
 *   gotas, lobos, cogumelos, morcegos, goblins (em bando, com chefe) e golens, com sapos perto da
 *   água; o nível sobe com a distância da vila.
 * - Animais (coelhos, cervos e cavalos selvagens) em toda parte, menos na água e na cidade.
 * Cada posto aparece quando um jogador chega a 70 m (e o chão de colisão de lá existe) e some além
 * de 85 m; vencido, volta depois de 1 a 2 minutos (chefes: 5 minutos).
 */
UCLASS()
class VORTEX_API UVortexCreatureSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    float MaterializeRadius = 7000.f;
    float DematerializeRadius = 8500.f;
    int32 SpawnsPerUpdate = 2;
    static constexpr float CellSize = VortexWorldLayout::BanditCellSize;

    int32 PostCount() const { return Posts.Num(); }
    int32 AliveCount() const;
    /** 08/11: a pré-carga das espécies terminou (a tela de carregamento espera por ela). */
    bool IsPreloadDone() const { return PendingPreloads == 0; }
    /** Testes: força todos os postos de uma espécie a aparecer perto de um ponto (e devolve quantos). */
    int32 DebugSpawnNear(FName Species, const FVector& Where, int32 Level);
    /** Acampamento de bandidos da quadra de 72 m (Cell), se houver: o centro (cm). Igual em todas as
     *  máquinas (só a semente e o relevo): o mapa e a decoração do acampamento usam também. */
    static bool BanditCampAt(const FIntPoint& Cell, FVector2D& OutAt) { return VortexWorldLayout::BanditCampAt(Cell, OutAt); }
    /** Testes: o acampamento mais perto de um ponto (busca em volta, até Rings quadras). */
    static bool NearestBanditCamp(const FVector2D& From, int32 Rings, FVector2D& OutAt);

private:
    struct FPost
    {
        FName Species;
        int32 Level = 1;
        int32 LookSeed = 0;
        FVector2D At = FVector2D::ZeroVector;
        int32 PackId = -1, PackSlot = 0, PackSize = 1;
        bool bBoss = false;
        /** Semi-chefe da região (um por região, lugar fixo). */
        bool bElite = false;
        /** Só nasce de noite (19h às 5h) e some de manhã. */
        bool bNight = false;
    bool bHang = false;   // aranha pendurada no fio (perto das teias)
        /** Do mato (bicho, monstro da quadra, bicho da noite): depois de vencido, volta noutro ponto perto do
         *  Origin (o bando junto); os acampamentos, chefes e postos fixos voltam no mesmo lugar. */
        bool bWander = false;
        FVector2D Origin = FVector2D::ZeroVector;
        int32 Deaths = 0, MovedFor = 0;
        double DeadUntil = 0.0;
        TWeakObjectPtr<AActor> Actor;
    };
    TArray<FPost> Posts;
    /** Os postos por quadra de 72 m (só as quadras perto de alguém são conferidas). */
    TMap<FIntPoint, TArray<int32>> Grid;
    /** Quadras do continente já sorteadas (sorteadas quando alguém chega perto, não todas na entrada). */
    TSet<FIntPoint> Generated;
    /** Postos com criatura em pé ou esperando voltar (conferidos mesmo longe). */
    TSet<int32> Live;
    /** As malhas das espécies, carregadas antes (sem travar o quadro em que o bicho nasce). */
    UPROPERTY() TArray<TObjectPtr<UObject>> Preloaded;
    /** 08/11: quantos pacotes da pré-carga ainda faltam e os que já chegaram. Enquanto falta, nenhum bicho nasce:
     *  pedir a malha na hora fazia a Unreal esperar a fila inteira (o Colosso travava 0,9 s, o cavalo 1,4 s). */
    int32 PendingPreloads = 0;
    TSet<FName> ReadyPackages, RequestedPackages;
    /** A malha da espécie já carregou? Se não, pede em segundo plano e responde false (nasce no próximo giro). */
    bool AssetsReady(FName Species);
    void RequestPackage(FName Package);
    FTimerHandle UpdateHandle;
    bool bBuilt = false, bActive = false;
    FBox2D Area = FBox2D(ForceInit);
    int32 PackCounter = 1 << 20;
    /** Sorteio desta partida (02/11): as criaturas do mato mudam de lugar e de espécie a cada partida. 0 nos
     *  testes (-Vortex...Smoke, Shots, Bench), que contam com os lugares de sempre. */
    int32 SpawnSalt = 0;
    /** O ponto serve para uma criatura do mato (dentro do mapa, fora da água e longe da vila). */
    bool WanderSpotFree(const FVector2D& At) const;

    void Build();
    void LoadFixed();
    void AddPost(const FPost& Post);
    void GenerateCell(const FIntPoint& Cell);
    void Preload();
    void Update();
    AActor* Spawn(FPost& Post, float GroundZ);
};
