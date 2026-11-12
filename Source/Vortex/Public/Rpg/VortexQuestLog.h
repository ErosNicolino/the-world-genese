#pragma once

#include "Components/ActorComponent.h"
#include "Rpg/VortexStory.h"
#include "VortexQuestLog.generated.h"

class UVortexHeroSheet;

/**
 * Diário de missões do herói (Main.Quests.cs do Godot). O servidor decide (aceitar, progresso,
 * entregar, recompensas); o dono recebe o diário e grava no save local, como a ficha.
 * Objetivos: kill (espécie), boss (chefe), collect (item da mochila, entregue ao terminar), visit
 * (chegar perto de um ponto) e talk (falar com um NPC). Missões "auto" entram sozinhas no diário
 * quando os requisitos se cumprem (a linha principal começa ao sair do Estádio).
 */
UCLASS(ClassGroup=(Vortex), meta=(BlueprintSpawnableComponent))
class VORTEX_API UVortexQuestLog : public UActorComponent
{
    GENERATED_BODY()
public:
    UVortexQuestLog();
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    /** Anfitrião: copia o diário do convidado para o save do mundo. */
    void StoreGuest();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ---- leitura (dono e servidor)
    const FVortexQuestState* State(FName Quest) const;
    bool IsDone(FName Quest) const;
    bool IsActive(FName Quest) const;
    bool IsAvailable(const FVortexQuestDef& Quest) const;
    bool IsReady(const FVortexQuestDef& Quest) const;
    int32 ObjectiveValue(const FVortexQuestDef& Quest, int32 Index) const;
    /** "✓  Derrote gotas de jade  3/3" por linha. */
    FString ObjectiveLines(const FVortexQuestDef& Quest) const;
    /** A missão do painel da HUD: a escolhida na aba Missões, ou a ativa mais recente. */
    const FVortexQuestDef* Tracked() const;
    /** "!" (tem missão), "?" (pode entregar) ou vazio, para a marca sobre o NPC. */
    FString MarkFor(FName NpcId) const;
    bool HasFlag(FName Flag) const { return Flags.Contains(Flag); }
    const TArray<FVortexQuestState>& GetQuests() const { return Quests; }
    int32 GetRevision() const { return Revision; }

    // ---- servidor
    void OnKill(FName Species, bool bBoss);
    void OnTalk(FName NpcId);
    /** Mudou a mochila (coletas) ou a arma do Estádio (a linha principal começa). */
    void Refresh();
    /** Marca do save do herói (baú aberto, presente recebido...). */
    void SetFlag(FName Flag) { Flags.AddUnique(Flag); Changed(); }

    // ---- pedidos do dono
    UFUNCTION(Server, Reliable) void ServerAccept(FName Quest);
    UFUNCTION(Server, Reliable) void ServerTurnIn(FName Quest);
    UFUNCTION(Server, Reliable) void ServerTrack(FName Quest);
    /** Falou com um NPC (objetivos "talk") e, se o nó tem ações (presente, marca), elas acontecem. */
    UFUNCTION(Server, Reliable) void ServerTalk(FName NpcId, FName Node);
    UFUNCTION(Server, Reliable, WithValidation) void ServerLoad(const TArray<FVortexQuestState>& Saved, const TArray<FName>& SavedFlags, FName SavedTracked, FGuid HeroId);

private:
    UPROPERTY(ReplicatedUsing = OnRep_Log) TArray<FVortexQuestState> Quests;
    UPROPERTY(ReplicatedUsing = OnRep_Log) TArray<FName> Flags;
    UPROPERTY(ReplicatedUsing = OnRep_Log) FName TrackedQuest;
    UFUNCTION() void OnRep_Log();

    UVortexHeroSheet* Sheet() const;
    FVortexQuestState* Find(FName Quest);
    void Accept(const FVortexQuestDef& Quest, bool bQuiet = false);
    void Progress(FName Type, FName Target, const FVector* At = nullptr);
    void Changed();
    bool IsOwnerLocal() const;
    void TryLoad();
    void SaveLocal();
    bool CanReach(FName NpcId) const;

    int32 Revision = 0;
    float VisitClock = 0.f, SaveTimer = 0.f;
    bool bLoaded = false, bSentLoad = false, bSaveDirty = false;
    UPROPERTY(Replicated) bool bServerLoaded = false;   // 09/11: o servidor aplica uma vez só, e o dono só grava depois disso
    bool bWasLocal = false, bGuest = false;
    FString GuestAccount;
    FGuid GuestHero;
};
