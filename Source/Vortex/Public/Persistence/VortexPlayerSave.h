#pragma once

#include "GameFramework/SaveGame.h"
#include "Persistence/VortexSaveTypes.h"
#include "Rpg/VortexRpg.h"
#include "Rpg/VortexStory.h"
#include "VortexPlayerSave.generated.h"

UCLASS()
class VORTEX_API UVortexPlayerSave : public USaveGame
{
    GENERATED_BODY()

public:
    // 5 (10/11): os pontos de nível são dos 8 atributos (antes 12); o menu pede para apagar os heróis de antes
    static constexpr int32 CurrentSchemaVersion = 5;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    int32 SaveSchemaVersion = CurrentSchemaVersion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    FString GameVersion = TEXT("0.1.0");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    FVortexIdentity Identity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Hero") FGuid WorldId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Hero") FString HeroName;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Hero") FName RaceId = TEXT("humano");
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Hero") FName AppearanceId = TEXT("male");
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Hero") TMap<FName, FName> Wardrobe;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Hero") TMap<FName, FLinearColor> Colors;

    // Ficha de RPG (schema 3, UVortexHeroSheet): sem ela, o herói começa como novo
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") bool bHasSheet = false;
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") int64 Experience = 0;
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") TArray<int32> AttributePoints;
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") TArray<FVortexItemStack> Bag;
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") TArray<FName> Equipped;
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") int32 Coins = 0;
    /** Herói novo que ainda não passou pelo Estádio do Despertar (escolher a arma inicial). */
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") bool bNeedsAwakening = false;
    /** Classe do Despertar (nível 10) e as cartas tiradas que ainda esperam a escolha. */
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") FName ClassId;
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") TArray<FName> ClassCards;
    /** Cavalo domado (pelagem), chamado com o assobio. */
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") FName MountCoat;
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") FString MountName;
    /** A magia do cajado escolhida (02/11) e as horas de caveira que faltam (PvP). */
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") FName StaffSpell;
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") float OutlawHours = 0.f;
    /** Lojas: dia do mundo das compras e o que já comprou nesse dia ("vendedor:oferta", uma vez por unidade). */
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") int32 ShopDay = 0;
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") TArray<FName> ShopBought;
    /** Amigos (nome do herói de cada um), como no Godot. */
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") TArray<FString> Friends;
    /** Mapa descoberto: um byte por pedaço de 18 m (0 = névoa), a partir de MapSeenOrigin (em pedaços). */
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") TArray<uint8> MapSeen;
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") FIntPoint MapSeenOrigin = FIntPoint::ZeroValue;
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") FIntPoint MapSeenSize = FIntPoint::ZeroValue;

    // Diário de missões (UVortexQuestLog): missões, marcas das conversas e a missão do painel
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") TArray<FVortexQuestState> QuestLog;
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") TArray<FName> StoryFlags;
    UPROPERTY(VisibleAnywhere, Category = "Vortex|Hero") FName TrackedQuest;

    // Definitions use stable content IDs. The data model will grow with inventory and quests.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    TMap<FName, int32> ItemCounts;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    TMap<FName, int32> QuestStages;

    bool IsSupported() const;
};
