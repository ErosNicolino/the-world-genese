#pragma once

#include "GameFramework/Actor.h"
#include "VortexDungeons.generated.h"

class AVortexCharacter;
class AVortexMonster;
class AVortexPortal;
class UPointLightComponent;
class UProceduralMeshComponent;
class UMaterialInterface;

/** As regras especiais dos portais (bits em UVortexHeroSheet::DungeonRules). */
namespace VortexDungeonRules
{
    enum ERule : uint8 { NoPotions, Darkness, NoRegen, Fragile, Fury, Swift, Blessing, Luck, TimeTrial, Echo, Count };
    VORTEX_API FString Name(int32 Rule);
    VORTEX_API bool IsBonus(int32 Rule);
}

/** Os biomas das dungeons (os básicos são cavernas; os altos, outros mundos). */
enum class EVortexDungeonBiome : uint8 { Cave, Crystal, Mine, Flooded, SpiderNest, Catacombs, Jungle, Swamp, IceFortress, Volcano, SunkenCity, RedAbyss, PortalThrone, Count };

/** Uma sala da dungeon na grade (em salas) e o que ela é. */
struct FVortexDungeonRoom
{
    FIntPoint Grid = FIntPoint::ZeroValue;
    int32 Parent = INDEX_NONE, Depth = 0;
    enum EKind : uint8 { Start, Fight, Treasure, Boss } Kind = Fight;
};

/** Tudo o que sai do id do portal ("dg_<semente>_<ranque>_<nível>"): igual em todas as máquinas. */
struct FVortexDungeonSpec
{
    FName Id;
    int32 Seed = 0, Level = 1, Tier = 0;
    FString Rank = TEXT("E");
    EVortexDungeonBiome Biome = EVortexDungeonBiome::Cave;
    uint16 Rules = 0;
    FVector Origin = FVector::ZeroVector;   // centro da sala de entrada, no chão (cm)
    TArray<FVortexDungeonRoom> Rooms;
    bool IsValid() const { return Rooms.Num() > 0; }
    bool Has(int32 Rule) const { return (Rules >> Rule) & 1; }
    /** Centro do chão de uma sala (cm). */
    FVector RoomCenter(int32 Index) const;
};

/**
 * Dungeons dos portais ("nos portais básicos cavernas e nos mais altos realmente outros biomas", "cada portal
 * um mapa diferente"). Cada portal leva a uma dungeon montada em blocos numa área reservada fora do mapa: salas
 * ligadas por corredores, com teto, tochas e decoração do bioma (estalagmites, cristais, trilhos, teias, caixões,
 * cipós, gelo, lava...). O formato, o bioma, o tamanho (5 salas no E, 13 no S) e as regras saem do id do portal.
 * - O servidor faz nascer os monstros do bioma em cada sala (aranhas no teto das cavernas, inclusive), o chefe
 *   (semi-chefe do bioma) na última sala e o baú guardado por ele; o baú do tesouro numa sala do fundo.
 * - A sala de entrada tem o portal de volta, menos no vermelho (S): lá ele só aparece quando o chefe cai.
 * - Cada máquina monta a dungeon de quem está dentro (UVortexHeroSheet::DungeonId replicado) e desmonta depois.
 */
UCLASS()
class VORTEX_API AVortexDungeons : public AActor
{
    GENERATED_BODY()
public:
    AVortexDungeons();
    virtual void Tick(float DeltaSeconds) override;

    static AVortexDungeons* Find(const UWorld* World);
    /** O id da dungeon de um portal (semente pela posição, ranque e nível do lugar). */
    static FName MakeId(const FVector& PortalAt, const FString& Rank, int32 Level);
    static FVortexDungeonSpec SpecFor(FName Id);
    static FString BiomeName(EVortexDungeonBiome Biome);
    /** Servidor: o herói entra pelo portal (monta a dungeon, faz nascer os monstros, aplica as regras). */
    void Enter(AVortexCharacter* Hero, AVortexPortal* Portal);
    /** Servidor: o herói sai (portal de volta, morte) e as regras deixam de valer. */
    static void Leave(AVortexCharacter* Hero);
    bool IsBuilt(FName Id) const { return Built.Contains(Id); }

    // grade das salas (em blocos de 75 cm)
    static constexpr float Block = 75.f;
    static constexpr int32 RoomSize = 13;
    static constexpr int32 RoomHeight = 7;
    static constexpr int32 Pitch = 20;
    static constexpr int32 CorridorWidth = 3;
    static constexpr int32 CorridorHeight = 4;

private:
    struct FBuiltDungeon
    {
        FVortexDungeonSpec Spec;
        TArray<TObjectPtr<USceneComponent>> Parts;
    };
    struct FLive
    {
        TArray<TWeakObjectPtr<AActor>> Monsters;
        TWeakObjectPtr<AActor> Boss;
        TWeakObjectPtr<AVortexPortal> Exit;
        FVector Return = FVector::ZeroVector;
        double StartedAt = 0.0, EmptySince = -1.0;
        bool bCleared = false;
    };
    TMap<FName, FBuiltDungeon> Built;
    TMap<FName, FLive> Live;
    TMap<TWeakObjectPtr<AVortexCharacter>, FVector> Returns;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> Material;
    float Since = 0.f;
    void Build(const FVortexDungeonSpec& Spec);
    void Drop(FName Id);
    void Populate(const FVortexDungeonSpec& Spec, FLive& State);
    void SpawnExit(const FVortexDungeonSpec& Spec, FLive& State, const FVector& At);
};
