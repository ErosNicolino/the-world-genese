#include "Rpg/VortexQuestLog.h"
#include "Persistence/VortexWorldSave.h"
#include "Framework/VortexGameMode.h"

#include "Creatures/VortexNpc.h"
#include "EngineUtils.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexGameInstance.h"
#include "Net/UnrealNetwork.h"
#include "Persistence/VortexPlayerSave.h"
#include "Persistence/VortexSaveSubsystem.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexRpg.h"

namespace
{
    namespace QuestNotice { enum : uint8 { Plain = 0, Experience = 1, Loot = 3, Refused = 4, Big = 5, Quest = 6 }; }
}

UVortexQuestLog::UVortexQuestLog()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UVortexQuestLog::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UVortexQuestLog, Quests, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVortexQuestLog, bServerLoaded, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVortexQuestLog, Flags, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVortexQuestLog, TrackedQuest, COND_OwnerOnly);
}

UVortexHeroSheet* UVortexQuestLog::Sheet() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<UVortexHeroSheet>() : nullptr;
}

bool UVortexQuestLog::IsOwnerLocal() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    return Pawn && Pawn->IsLocallyControlled() && Pawn->IsPlayerControlled();
}

const FVortexQuestState* UVortexQuestLog::State(FName Quest) const
{
    return Quests.FindByPredicate([Quest](const FVortexQuestState& S) { return S.Id == Quest; });
}

FVortexQuestState* UVortexQuestLog::Find(FName Quest)
{
    return Quests.FindByPredicate([Quest](const FVortexQuestState& S) { return S.Id == Quest; });
}

bool UVortexQuestLog::IsDone(FName Quest) const { const FVortexQuestState* S = State(Quest); return S && S->State == 2; }
bool UVortexQuestLog::IsActive(FName Quest) const { const FVortexQuestState* S = State(Quest); return S && S->State == 1; }

bool UVortexQuestLog::IsAvailable(const FVortexQuestDef& Quest) const
{
    if (State(Quest.Id) || Quest.IsLocked()) return false;
    for (const FName& Required : Quest.Requires) if (!IsDone(Required)) return false;
    return true;
}

int32 UVortexQuestLog::ObjectiveValue(const FVortexQuestDef& Quest, int32 Index) const
{
    if (!Quest.Objectives.IsValidIndex(Index)) return 0;
    const FVortexQuestObjective& Objective = Quest.Objectives[Index];
    const FVortexQuestState* S = State(Quest.Id);
    if (S && S->State == 2) return Objective.Count;
    if (Objective.Type == TEXT("collect"))
    {
        const UVortexHeroSheet* Hero = Sheet();
        return Hero ? FMath::Min(Objective.Count, Hero->CountOf(Objective.Target)) : 0;
    }
    return S && S->Counts.IsValidIndex(Index) ? FMath::Min(Objective.Count, S->Counts[Index]) : 0;
}

bool UVortexQuestLog::IsReady(const FVortexQuestDef& Quest) const
{
    if (!IsActive(Quest.Id)) return false;
    for (int32 i = 0; i < Quest.Objectives.Num(); ++i) if (ObjectiveValue(Quest, i) < Quest.Objectives[i].Count) return false;
    return true;
}

FString UVortexQuestLog::ObjectiveLines(const FVortexQuestDef& Quest) const
{
    TArray<FString> Lines;
    for (int32 i = 0; i < Quest.Objectives.Num(); ++i)
    {
        const FVortexQuestObjective& O = Quest.Objectives[i];
        const int32 Value = ObjectiveValue(Quest, i);
        const bool bCount = O.Type != TEXT("visit") && O.Type != TEXT("talk") && O.Count > 1;
        Lines.Add(FString::Printf(TEXT("%s  %s%s"), Value >= O.Count ? TEXT("✓") : TEXT("◇"), *O.Text,
            bCount ? *FString::Printf(TEXT("  %d/%d"), Value, O.Count) : TEXT("")));
    }
    if (IsReady(Quest)) Lines.Add(FString::Printf(TEXT("↟  Fale com %s"), *FVortexStoryCatalog::Get().NpcCallName(Quest.Giver)));
    return FString::Join(Lines, TEXT("\n"));
}

const FVortexQuestDef* UVortexQuestLog::Tracked() const
{
    const FVortexStoryCatalog& Story = FVortexStoryCatalog::Get();
    if (!TrackedQuest.IsNone() && IsActive(TrackedQuest)) return Story.Quest(TrackedQuest);
    // a linha principal primeiro; depois a mais recente
    const FVortexQuestDef* Latest = nullptr;
    for (const FVortexQuestState& S : Quests)
    {
        if (S.State != 1) continue;
        const FVortexQuestDef* Def = Story.Quest(S.Id);
        if (!Def) continue;
        if (Def->bMain) return Def;
        Latest = Def;
    }
    return Latest;
}

FString UVortexQuestLog::MarkFor(FName NpcId) const
{
    bool bOffer = false;
    for (const FVortexQuestDef& Q : FVortexStoryCatalog::Get().Quests)
    {
        if (Q.Giver != NpcId) continue;
        if (IsReady(Q)) return TEXT("?");
        if (IsAvailable(Q) && !Q.bAuto) bOffer = true;
    }
    // objetivo "falar com" este NPC em andamento também marca
    for (const FVortexQuestState& S : Quests)
    {
        if (S.State != 1) continue;
        if (const FVortexQuestDef* Q = FVortexStoryCatalog::Get().Quest(S.Id))
        {
            for (int32 i = 0; i < Q->Objectives.Num(); ++i)
            {
                if (Q->Objectives[i].Type == TEXT("talk") && Q->Objectives[i].Target == NpcId && ObjectiveValue(*Q, i) < 1) return TEXT("?");
            }
        }
    }
    return bOffer ? TEXT("!") : FString();
}

// ---- servidor ------------------------------------------------------------------------------------

void UVortexQuestLog::Accept(const FVortexQuestDef& Quest, bool bQuiet)
{
    FVortexQuestState S;
    S.Id = Quest.Id;
    S.State = 1;
    S.Counts.SetNumZeroed(Quest.Objectives.Num());
    Quests.Add(S);
    if (UVortexHeroSheet* Hero = Sheet()) Hero->PushNotice(FString::Printf(TEXT("Nova missão: %s"), *Quest.Title), QuestNotice::Quest);
    if (!bQuiet) VortexAudio::Play(this, TEXT("quest_accept"), GetOwner()->GetActorLocation(), .8f);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_QUEST aceita %s"), *Quest.Id.ToString());
    Changed();
}

void UVortexQuestLog::Progress(FName Type, FName Target, const FVector* At)
{
    const FVortexStoryCatalog& Story = FVortexStoryCatalog::Get();
    bool bChanged = false;
    for (FVortexQuestState& S : Quests)
    {
        if (S.State != 1) continue;
        const FVortexQuestDef* Q = Story.Quest(S.Id);
        if (!Q) continue;
        const bool bWasReady = IsReady(*Q);
        S.Counts.SetNumZeroed(Q->Objectives.Num(), EAllowShrinking::No);
        for (int32 i = 0; i < Q->Objectives.Num(); ++i)
        {
            const FVortexQuestObjective& O = Q->Objectives[i];
            if (O.Type != Type || S.Counts[i] >= O.Count) continue;
            const bool bMatch = Type == TEXT("visit")
                ? At && FVector2D::Distance(FVector2D(At->X, At->Y), O.Point) <= O.Radius
                : O.Target.ToString().Equals(Target.ToString(), ESearchCase::IgnoreCase);
            if (!bMatch) continue;
            ++S.Counts[i];
            bChanged = true;
            if (S.Counts[i] >= O.Count)
            {
                if (UVortexHeroSheet* Hero = Sheet()) Hero->PushNotice(FString::Printf(TEXT("%s: %s ✓"), *Q->Title, *O.Text), QuestNotice::Experience);
            }
        }
        if (!bWasReady && IsReady(*Q))
        {
            if (UVortexHeroSheet* Hero = Sheet())
            {
                Hero->PushNotice(FString::Printf(TEXT("Missão pronta: fale com %s"), *Story.NpcCallName(Q->Giver)), QuestNotice::Quest);
            }
        }
    }
    if (bChanged) Changed();
}

void UVortexQuestLog::OnKill(FName Species, bool bBoss)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    Progress(TEXT("kill"), Species);
    if (bBoss) Progress(TEXT("boss"), Species);
}

void UVortexQuestLog::OnTalk(FName NpcId)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    Progress(TEXT("talk"), NpcId);
}

void UVortexQuestLog::Refresh()
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !bLoaded) return;
    const UVortexHeroSheet* Hero = Sheet();
    // a jornada começa ao sair do Estádio (herói de teste sem menu também tem a linha principal)
    if (!Hero || !Hero->IsLoaded() || Hero->NeedsAwakening()) return;
    for (const FVortexQuestDef& Q : FVortexStoryCatalog::Get().Quests)
    {
        if (Q.bAuto && IsAvailable(Q)) Accept(Q, true);
    }
    // sem ++Revision aqui: isto roda a cada meio segundo e a janela do herói se refazia inteira
    // (piscava). As coletas mudam com a mochila, que já avisa a janela pela revisão da ficha.
}

bool UVortexQuestLog::CanReach(FName NpcId) const
{
    const AActor* Owner = GetOwner();
    if (!Owner) return false;
    for (TActorIterator<AVortexNpc> It(Owner->GetWorld()); It; ++It)
    {
        if (It->GetNpcId() == NpcId && FVector::Dist(It->GetActorLocation(), Owner->GetActorLocation()) < 900.f) return true;
    }
    return false;
}

void UVortexQuestLog::ServerAccept_Implementation(FName Quest)
{
    const FVortexQuestDef* Q = FVortexStoryCatalog::Get().Quest(Quest);
    if (!Q || !IsAvailable(*Q) || !CanReach(Q->Giver)) return;
    Accept(*Q);
    TrackedQuest = Q->Id;
}

void UVortexQuestLog::ServerTurnIn_Implementation(FName Quest)
{
    const FVortexQuestDef* Q = FVortexStoryCatalog::Get().Quest(Quest);
    FVortexQuestState* S = Find(Quest);
    UVortexHeroSheet* Hero = Sheet();
    if (!Q || !S || !Hero || !IsReady(*Q) || !CanReach(Q->Giver)) return;
    // coletas entregues saem da mochila (menos as que a missão manda guardar)
    for (const FVortexQuestObjective& O : Q->Objectives)
    {
        if (O.Type == TEXT("collect") && !O.bKeep) Hero->TakeItem(O.Target, O.Count);
    }
    S->State = 2;
    if (TrackedQuest == Quest) TrackedQuest = NAME_None;
    Hero->PushNotice(FString::Printf(TEXT("Missão concluída: %s"), *Q->Title), QuestNotice::Big);
    if (Q->Xp > 0) Hero->AddXp(Q->Xp);
    if (Q->Coins > 0) Hero->AddCoins(Q->Coins);
    for (const TPair<FName, int32>& Item : Q->Items) Hero->GiveItem(Item.Key, Item.Value);
    VortexAudio::Play(this, TEXT("quest_complete"), GetOwner()->GetActorLocation(), 1.f, 0.f);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_QUEST entregue %s"), *Q->Id.ToString());
    Changed();
    Refresh();
}

void UVortexQuestLog::ServerTrack_Implementation(FName Quest)
{
    TrackedQuest = IsActive(Quest) ? Quest : NAME_None;
    Changed();
}

void UVortexQuestLog::ServerTalk_Implementation(FName NpcId, FName Node)
{
    if (!CanReach(NpcId)) return;
    for (TActorIterator<AVortexNpc> It(GetWorld()); It; ++It)
    {
        if (It->GetNpcId() == NpcId) It->TalkTo(GetOwner());
    }
    OnTalk(NpcId);
    if (Node.IsNone()) return;
    const FVortexDialogue* Dialogue = FVortexStoryCatalog::Get().DialogueOf(NpcId);
    const FVortexDialogueNode* N = Dialogue ? Dialogue->Nodes.Find(Node) : nullptr;
    if (!N || N->Actions.IsEmpty()) return;
    // presentes só uma vez: se o nó marca uma flag que já existe, nada acontece de novo
    for (const TArray<FString>& Action : N->Actions)
    {
        if (Action.Num() >= 2 && Action[0] == TEXT("set_flag") && Flags.Contains(FName(*Action[1]))) return;
    }
    UVortexHeroSheet* Hero = Sheet();
    for (const TArray<FString>& Action : N->Actions)
    {
        if (Action.Num() >= 3 && Action[0] == TEXT("give_item") && Hero) Hero->GiveItem(FName(*Action[1]), FCString::Atoi(*Action[2]));
        else if (Action.Num() >= 2 && Action[0] == TEXT("set_flag")) Flags.AddUnique(FName(*Action[1]));
    }
    Changed();
}

void UVortexQuestLog::Changed()
{
    ++Revision;
    if (IsOwnerLocal()) bSaveDirty = true;
}

void UVortexQuestLog::OnRep_Log()
{
    ++Revision;
    if (IsOwnerLocal() && bLoaded) bSaveDirty = true;
}

// ---- save (do dono, como a ficha) -----------------------------------------------------------------

void UVortexQuestLog::TryLoad()
{
    if (bSentLoad || !IsOwnerLocal()) return;
    bSentLoad = true;
    TArray<FVortexQuestState> Saved;
    TArray<FName> SavedFlags;
    FName SavedTracked;
    FGuid HeroId;
    const UVortexGameInstance* Game = GetWorld() ? Cast<UVortexGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
    if (const UVortexPlayerSave* Hero = Game ? Game->GetSelectedHero() : nullptr)
    {
        Saved = Hero->QuestLog;
        SavedFlags = Hero->StoryFlags;
        SavedTracked = Hero->TrackedQuest;
        HeroId = Hero->Identity.PlayerId;
    }
    bLoaded = true;
    ServerLoad(Saved, SavedFlags, SavedTracked, HeroId);
}

bool UVortexQuestLog::ServerLoad_Validate(const TArray<FVortexQuestState>& Saved, const TArray<FName>& SavedFlags, FName SavedTracked, FGuid HeroId)
{
    return Saved.Num() <= 512 && SavedFlags.Num() <= 512;
}

void UVortexQuestLog::ServerLoad_Implementation(const TArray<FVortexQuestState>& InSaved, const TArray<FName>& InFlags, FName InTracked, FGuid HeroId)
{
    if (bServerLoaded) return;
    bServerLoaded = true;
    TArray<FVortexQuestState> Saved = InSaved;
    TArray<FName> SavedFlags = InFlags;
    FName SavedTracked = InTracked;
    // como a ficha: o convidado conhecido volta com o diário do anfitrião; o desconhecido, nos mundos novos, começa vazio
    AVortexGameMode* Mode = GetWorld()->GetAuthGameMode<AVortexGameMode>();
    if (const APawn* Pawn = Cast<APawn>(GetOwner()); Pawn && Mode && Pawn->GetNetConnection())
    {
        bGuest = true;
        GuestAccount = AVortexGameMode::GuestAccount(Pawn);
        GuestHero = HeroId;
        if (const FVortexGuestHero* Known = Mode->FindGuest(GuestAccount, HeroId); Known && Known->bHasQuests)
        {
            Saved = Known->Quests;
            SavedFlags = Known->Flags;
            SavedTracked = Known->Tracked;
        }
        else if (Mode->UsesGuestCopies())
        {
            Saved.Reset();
            SavedFlags.Reset();
            SavedTracked = NAME_None;
        }
    }
    // só missões que existem, uma vez cada
    Quests.Reset();
    for (const FVortexQuestState& S : Saved)
    {
        if (FVortexStoryCatalog::Get().Quest(S.Id) && !State(S.Id) && (S.State == 1 || S.State == 2)) Quests.Add(S);
    }
    Flags = SavedFlags;
    TrackedQuest = SavedTracked;
    bLoaded = true;
    if (bGuest && Mode) { StoreGuest(); Mode->SaveGuestsNow(); }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_QUEST diário carregado: %d missões"), Quests.Num());
    Changed();
    Refresh();
}

void UVortexQuestLog::StoreGuest()
{
    if (!bServerLoaded || !bGuest || !GetWorld()) return;
    AVortexGameMode* Mode = GetWorld()->GetAuthGameMode<AVortexGameMode>();
    if (FVortexGuestHero* Entry = Mode ? Mode->FindGuest(GuestAccount, GuestHero, true) : nullptr)
    {
        Entry->bHasQuests = true;
        Entry->Quests = Quests;
        Entry->Flags = Flags;
        Entry->Tracked = TrackedQuest;
    }
}

void UVortexQuestLog::EndPlay(const EEndPlayReason::Type Reason)
{
    if (bSaveDirty && bServerLoaded && bWasLocal) SaveLocal();
    Super::EndPlay(Reason);
}

void UVortexQuestLog::SaveLocal()
{
    bSaveDirty = false;
    UVortexGameInstance* Game = GetWorld() ? Cast<UVortexGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
    UVortexSaveSubsystem* Saves = Game ? Game->GetSubsystem<UVortexSaveSubsystem>() : nullptr;
    UVortexPlayerSave* Hero = Game ? Game->GetSelectedHero() : nullptr;
    if (!Saves || !Hero) return;
    Hero->QuestLog = Quests;
    Hero->StoryFlags = Flags;
    Hero->TrackedQuest = TrackedQuest;
    bSaveDirty = !Saves->SaveLocalPlayer(Hero);   // falhou: tenta de novo
}

void UVortexQuestLog::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    AActor* Owner = GetOwner();
    if (!Owner) return;
    if (IsOwnerLocal())
    {
        TryLoad();
        bWasLocal = true;
        if (bSaveDirty && bServerLoaded && (SaveTimer += DeltaTime) > 1.5f) { SaveTimer = 0.f; SaveLocal(); }
    }
    if (!Owner->HasAuthority()) return;
    // heróis sem dono humano (testes): diário vazio
    if (!bLoaded && !Cast<APawn>(Owner)->IsPlayerControlled()) { bLoaded = true; Refresh(); }
    VisitClock -= DeltaTime;
    if (VisitClock > 0.f || !bLoaded) return;
    VisitClock = .5f;
    const FVector At = Owner->GetActorLocation();
    Progress(TEXT("visit"), NAME_None, &At);
    Refresh();
}
