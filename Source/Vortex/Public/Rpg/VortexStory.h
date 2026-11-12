#pragma once

#include "CoreMinimal.h"
#include "VortexStory.generated.h"

/** Uma missão no diário do herói (vai no save e para o dono pela rede). */
USTRUCT()
struct VORTEX_API FVortexQuestState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FName Id;
    /** 1 = em andamento, 2 = entregue. */
    UPROPERTY(SaveGame) uint8 State = 0;
    UPROPERTY(SaveGame) TArray<int32> Counts;
    bool operator==(const FVortexQuestState& Other) const { return Id == Other.Id && State == Other.State && Counts == Other.Counts; }
};

struct VORTEX_API FVortexQuestObjective
{
    FName Type;        // kill, collect, visit, talk, boss
    FName Target;      // espécie, item ou NPC
    FVector2D Point = FVector2D::ZeroVector;   // visit: cm no mundo
    float Radius = 1200.f;                     // visit: cm
    int32 Count = 1;
    bool bKeep = false;                        // collect: não tira da mochila ao entregar
    FString Text;
};

struct VORTEX_API FVortexQuestDef
{
    FName Id, Giver;
    FString Title, Offer, Progress, Done, Region;
    int32 Chapter = 1;
    bool bMain = false, bAuto = false;
    TArray<FName> Requires;
    TArray<FVortexQuestObjective> Objectives;
    int64 Xp = 0;
    int32 Coins = 0;
    TArray<TPair<FName, int32>> Items;
    /** Fora do vale: fica travada até o continente chegar ao Vortex. */
    bool IsLocked() const { return !Region.IsEmpty(); }
};

struct VORTEX_API FVortexDialogueChoice
{
    FString Text;
    FName Next;                 // NAME_None = fim da conversa
    FName IfFlag, IfNotFlag;
};

struct VORTEX_API FVortexDialogueNode
{
    FName Id;
    FString Speaker, Text;
    TArray<FVortexDialogueChoice> Choices;
    FName Next;                 // sem escolhas: botão Continuar (NAME_None = fim)
    bool bHasNext = false;
    /** give_item <id> <qtd>, set_flag <nome>, open_shop <loja> */
    TArray<TArray<FString>> Actions;
};

struct VORTEX_API FVortexDialogue
{
    FName Start;
    TMap<FName, FVortexDialogueNode> Nodes;
};

struct VORTEX_API FVortexNpcInfo
{
    FName Id, Dialogue;
    FString Name, Title;
    TArray<FString> Barks;
};

struct VORTEX_API FVortexChapter
{
    int32 Id = 1;
    FString Title, Region, Text;
};

/** História, missões, diálogos e falas (Data/Story/story.json, Tools/BuildStoryData.py). */
class VORTEX_API FVortexStoryCatalog
{
public:
    static const FVortexStoryCatalog& Get();
    TArray<FVortexQuestDef> Quests;
    TMap<FName, FVortexDialogue> Dialogues;
    TMap<FName, FVortexNpcInfo> Npcs;
    TArray<FVortexChapter> Chapters;

    const FVortexQuestDef* Quest(FName Id) const;
    const FVortexNpcInfo* Npc(FName Id) const { return Npcs.Find(Id); }
    const FVortexDialogue* DialogueOf(FName NpcId) const;
    /** "a Capitã Rhea", "o Bram"... (como o Godot) */
    FString NpcCallName(FName NpcId) const;

private:
    FVortexStoryCatalog();
};
