#pragma once

#include "GameFramework/SaveGame.h"
#include "Persistence/VortexSaveTypes.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexStory.h"
#include "VortexWorldSave.generated.h"

/** 09/11: a cópia que o anfitrião guarda do herói de um convidado (Key = conta da Epic + herói). */
USTRUCT()
struct FVortexGuestHero
{
    GENERATED_BODY()
    UPROPERTY() FString Key;
    UPROPERTY() bool bHasSheet = false;
    UPROPERTY() FVortexSheetSave Sheet;
    UPROPERTY() bool bHasQuests = false;
    UPROPERTY() TArray<FVortexQuestState> Quests;
    UPROPERTY() TArray<FName> Flags;
    UPROPERTY() FName Tracked;
};

UCLASS()
class VORTEX_API UVortexWorldSave : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentSchemaVersion = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    int32 SaveSchemaVersion = CurrentSchemaVersion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    FString GameVersion = TEXT("0.1.0");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    FGuid WorldId;

    // Human-readable name. Empty on saves created before the front end existed.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    FString DisplayName;

    // Local host's last safe position. Older version-1 saves leave this unset.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    bool bHasHostLocation = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    FVector HostLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    int32 Seed = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    int32 GeneratorVersion = 1;

    // Only lasting changes are stored; procedural base content comes from Seed + GeneratorVersion.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    TArray<FVortexWorldDelta> PersistentDelta;

    // Fase 14: estado dos NPCs por ID estável. Campo novo com valor padrão vazio: saves antigos
    // da versão 1 carregam sem ele.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    TArray<FVortexNpcState> NpcStates;

    // Fase 15: hora do mundo (segundos dentro do ciclo de dia e noite) e dia corrente.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    float DayClock = 300.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    int32 DayNumber = 1;

    // Menu de 23/10: mundo de multiplayer (sala para amigos, até 3 heróis) ou de um jogador (1 herói).
    // Campo novo com padrão falso: os mundos antigos aparecem em "Jogar".
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    bool bMultiplayer = false;

    // 09/11: campo novo, vazio nos mundos antigos
    UPROPERTY()
    TArray<FVortexGuestHero> GuestHeroes;

    // Mundos criados a partir de 09/11: herói de convidado que o anfitrião nunca viu entra como novo. Nos antigos fica falso
    // (quem já jogava neles não tem cópia aqui e perderia o herói).
    UPROPERTY()
    bool bGuestCopies = false;

    bool IsSupported() const;
};
