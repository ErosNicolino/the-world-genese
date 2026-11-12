#include "Rpg/VortexSocial.h"

#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexGameInstance.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Persistence/VortexPlayerSave.h"
#include "Persistence/VortexSaveSubsystem.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexDuelDome.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
    int32 GNextPartyId = 1;
}

UVortexSocial::UVortexSocial()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = .1f;
    SetIsReplicatedByDefault(true);
}

void UVortexSocial::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UVortexSocial, PartyId);
    DOREPLIFETIME(UVortexSocial, HeroName);
    DOREPLIFETIME(UVortexSocial, PlayerColor);
    DOREPLIFETIME(UVortexSocial, DuelOpponent);
    DOREPLIFETIME(UVortexSocial, DuelPhase);
    DOREPLIFETIME(UVortexSocial, DuelFightAt);
    DOREPLIFETIME_CONDITION(UVortexSocial, TradePartner, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVortexSocial, MyOffer, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVortexSocial, TheirOffer, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVortexSocial, Pending, COND_OwnerOnly);
}

AVortexCharacter* UVortexSocial::Hero() const { return Cast<AVortexCharacter>(GetOwner()); }
UVortexSocial* UVortexSocial::Of(const AActor* Actor) { return Actor ? Actor->FindComponentByClass<UVortexSocial>() : nullptr; }

float UVortexSocial::Now() const
{
    const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    return State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}

void UVortexSocial::Notice(const FString& Text, uint8 Kind) const
{
    if (const AVortexCharacter* Me = Hero()) if (UVortexHeroSheet* Sheet = Me->GetSheet()) Sheet->PushNotice(Text, Kind);
}

FLinearColor UVortexSocial::PaletteColor(int32 Index)
{
    // sem vermelho (é a cor do perigo)
    static const FLinearColor Colors[PaletteCount] = {
        FLinearColor(.35f, .9f, 1.f), FLinearColor(.45f, 1.f, .4f), FLinearColor(1.f, .85f, .25f), FLinearColor(1.f, .55f, .15f),
        FLinearColor(1.f, .45f, .8f), FLinearColor(.72f, .5f, 1.f), FLinearColor(.95f, .95f, .95f), FLinearColor(.35f, .55f, 1.f) };
    return Colors[((Index % PaletteCount) + PaletteCount) % PaletteCount];
}

const TCHAR* UVortexSocial::PaletteName(int32 Index)
{
    static const TCHAR* Names[PaletteCount] = { TEXT("ciano"), TEXT("verde"), TEXT("amarelo"), TEXT("laranja"), TEXT("rosa"), TEXT("roxo"), TEXT("branco"), TEXT("azul") };
    return Names[((Index % PaletteCount) + PaletteCount) % PaletteCount];
}

int32 UVortexSocial::GetPlayerColorIndex() const
{
    if (PlayerColor < PaletteCount) return PlayerColor;
    // automática: pelo número do jogador na sessão (cada um num diferente)
    const APawn* Pawn = Cast<APawn>(GetOwner());
    const APlayerState* State = Pawn ? Pawn->GetPlayerState() : nullptr;
    return State ? State->GetPlayerId() % PaletteCount : int32(GetTypeHash(GetHeroName()) % PaletteCount);
}

void UVortexSocial::CyclePlayerColor()
{
    const int32 Next = (GetPlayerColorIndex() + 1) % PaletteCount;
    PlayerColor = uint8(Next);
    ServerSetPlayerColor(uint8(Next));
    if (GConfig) { GConfig->SetInt(TEXT("Vortex"), TEXT("PlayerColor"), Next, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); }
    if (AVortexCharacter* Me = Hero(); Me && Me->GetSheet())
        Me->GetSheet()->PushNotice(FString::Printf(TEXT("Sua cor no mapa e no ping: %s (Shift+Z troca)"), PaletteName(Next)));
}

void UVortexSocial::ServerSetPlayerColor_Implementation(uint8 Index)
{
    PlayerColor = Index < PaletteCount ? Index : 255;
}

FString UVortexSocial::GetHeroName() const
{
    if (!HeroName.IsEmpty()) return HeroName;
    const APawn* Pawn = Cast<APawn>(GetOwner());
    const APlayerState* State = Pawn ? Pawn->GetPlayerState() : nullptr;
    const FString Name = State ? State->GetPlayerName() : FString();
    return Name.IsEmpty() || Name.StartsWith(TEXT("DESKTOP")) ? FString(TEXT("Aventureiro")) : Name;
}

void UVortexSocial::BeginPlay()
{
    Super::BeginPlay();
}

bool UVortexSocial::ServerSetHeroName_Validate(const FString& Name) { return Name.Len() <= 24; }
void UVortexSocial::ServerSetHeroName_Implementation(const FString& Name) { HeroName = Name.TrimStartAndEnd(); }

bool UVortexSocial::SameParty(const AActor* Other) const
{
    const UVortexSocial* Them = Of(Other);
    return Them && PartyId != 0 && Them->PartyId == PartyId;
}

TArray<AVortexCharacter*> UVortexSocial::PartyMates() const
{
    TArray<AVortexCharacter*> Out;
    if (PartyId == 0) return Out;
    for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
        if (*It != GetOwner() && SameParty(*It)) Out.Add(*It);
    return Out;
}

float UVortexSocial::DuelCountdown() const { return FMath::Max(0.f, DuelFightAt - Now()); }

bool UVortexSocial::IsDuelFighting(const AActor* Other) const
{
    return DuelPhase == 2 && DuelOpponent && DuelOpponent == Other;
}

bool UVortexSocial::HasPending(const AActor* From, FName Kind) const
{
    for (const FVortexSocialPending& P : Pending) if (P.From == From && P.Kind == Kind && P.Until > Now()) return true;
    return false;
}

bool UVortexSocial::IsFriend(const UObject* WorldContext, const FString& Name)
{
    const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
    const UVortexGameInstance* Game = World ? Cast<UVortexGameInstance>(World->GetGameInstance()) : nullptr;
    const UVortexPlayerSave* Save = Game ? Game->GetSelectedHero() : nullptr;
    return Save && Save->Friends.Contains(Name);
}

TArray<UVortexSocial::FMenuItem> UVortexSocial::MenuFor(const AVortexCharacter* Other) const
{
    TArray<FMenuItem> List;
    const UVortexSocial* Them = Of(Other);
    if (!Them) return List;
    // grupo
    if (SameParty(Other)) List.Add({ TEXT("party_leave"), TEXT("Sair do grupo") });
    else if (HasPending(Other, TEXT("party"))) List.Add({ TEXT("party_accept"), TEXT("Aceitar convite de grupo") });
    else List.Add({ TEXT("party_invite"), TEXT("Convidar para o grupo"), PartyMates().Num() + 1 < PartyLimit });
    // troca
    if (TradePartner) List.Add({ NAME_None, TEXT("Troca em andamento"), false });
    else if (HasPending(Other, TEXT("trade"))) List.Add({ TEXT("trade_accept"), TEXT("Aceitar troca") });
    else List.Add({ TEXT("trade_invite"), TEXT("Propor troca") });
    // amizade
    if (IsFriend(this, Them->GetHeroName())) List.Add({ TEXT("friend_remove"), TEXT("Desfazer amizade") });
    else if (HasPending(Other, TEXT("friend"))) List.Add({ TEXT("friend_accept"), TEXT("Aceitar amizade") });
    else List.Add({ TEXT("friend_add"), TEXT("Pedir amizade") });
    // duelo
    if (DuelPhase != 0) List.Add({ TEXT("duel_cancel"), DuelOpponent == Other ? FString(TEXT("Cancelar duelo")) : FString(TEXT("Você já está num duelo")), DuelOpponent == Other });
    else if (HasPending(Other, TEXT("duel"))) List.Add({ TEXT("duel_accept"), TEXT("Aceitar duelo") });
    else List.Add({ TEXT("duel_invite"), TEXT("Desafiar para duelo") });
    return List;
}

void UVortexSocial::Act(AVortexCharacter* Other, FName Action)
{
    if (!Other || Action.IsNone()) return;
    if (Action == TEXT("friend_remove"))
    {
        // amizade é do save de cada um: desfaz só aqui
        const UWorld* World = GetWorld();
        UVortexGameInstance* Game = World ? Cast<UVortexGameInstance>(World->GetGameInstance()) : nullptr;
        UVortexPlayerSave* Save = Game ? Game->GetSelectedHero() : nullptr;
        const UVortexSocial* Them = Of(Other);
        if (Save && Them)
        {
            Save->Friends.Remove(Them->GetHeroName());
            if (UVortexSaveSubsystem* Saves = Game->GetSubsystem<UVortexSaveSubsystem>()) Saves->SaveLocalPlayer(Save);
            if (const AVortexCharacter* Me = Hero()) if (Me->GetSheet()) Me->GetSheet()->PushNotice(FString::Printf(TEXT("%s não é mais seu amigo"), *Them->GetHeroName()));
        }
        return;
    }
    ServerAct(Other, Action);
}

void UVortexSocial::ClientFriend_Implementation(const FString& Name)
{
    UVortexGameInstance* Game = GetWorld() ? Cast<UVortexGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
    UVortexPlayerSave* Save = Game ? Game->GetSelectedHero() : nullptr;
    if (!Save || Name.IsEmpty()) return;
    Save->Friends.AddUnique(Name);
    if (UVortexSaveSubsystem* Saves = Game->GetSubsystem<UVortexSaveSubsystem>()) Saves->SaveLocalPlayer(Save);
}

// ---- servidor ----------------------------------------------------------------------------------------

void UVortexSocial::AddPending(AVortexCharacter* From, FName Kind)
{
    Pending.RemoveAll([&](const FVortexSocialPending& P) { return (P.From == From && P.Kind == Kind) || P.Until < Now(); });
    FVortexSocialPending P;
    P.From = From;
    P.Kind = Kind;
    P.Until = Now() + 60.f;
    Pending.Add(P);
}

bool UVortexSocial::TakePending(AVortexCharacter* From, FName Kind)
{
    const int32 Removed = Pending.RemoveAll([&](const FVortexSocialPending& P) { return P.From == From && P.Kind == Kind && P.Until >= Now(); });
    return Removed > 0;
}

void UVortexSocial::ServerAct_Implementation(AVortexCharacter* Target, FName Action)
{
    AVortexCharacter* Me = Hero();
    UVortexSocial* Them = Of(Target);
    if (!Me || !Them || Target == Me) return;
    const FString TheirName = Them->GetHeroName(), MyName = GetHeroName();
    const bool bNear = FVector::Dist(Me->GetActorLocation(), Target->GetActorLocation()) < InteractRange * 1.6f;
    const bool bLeaveOrCancel = Action == TEXT("party_leave") || Action == TEXT("duel_cancel") || Action == TEXT("trade_cancel");
    if (!bNear && !bLeaveOrCancel) { Notice(TEXT("Chegue mais perto")); return; }
    if (Action == TEXT("party_invite"))
    {
        if (SameParty(Target)) return;
        if (PartyMates().Num() + 1 >= PartyLimit) { Notice(TEXT("O grupo está cheio (8)"), 4); return; }
        Them->AddPending(Me, TEXT("party"));
        Them->Notice(FString::Printf(TEXT("%s convidou você para o grupo  ·  chegue perto e aperte E"), *MyName), 2);
        Notice(FString::Printf(TEXT("Convite de grupo enviado para %s"), *TheirName));
    }
    else if (Action == TEXT("party_accept"))
    {
        if (!TakePending(Target, TEXT("party"))) return;
        JoinParty(Target);
    }
    else if (Action == TEXT("party_leave")) LeaveParty();
    else if (Action == TEXT("friend_add") || Action == TEXT("friend_accept"))
    {
        if (TakePending(Target, TEXT("friend")))
        {
            ClientFriend(TheirName);
            Them->ClientFriend(MyName);
            Notice(FString::Printf(TEXT("Você e %s agora são amigos"), *TheirName), 2);
            Them->Notice(FString::Printf(TEXT("Você e %s agora são amigos"), *MyName), 2);
            return;
        }
        Them->AddPending(Me, TEXT("friend"));
        Them->Notice(FString::Printf(TEXT("%s quer ser seu amigo  ·  chegue perto e aperte E"), *MyName), 2);
        Notice(FString::Printf(TEXT("Pedido de amizade enviado para %s"), *TheirName));
    }
    else if (Action == TEXT("duel_invite"))
    {
        if (DuelPhase != 0 || Them->DuelPhase != 0) { Notice(TEXT("Alguém já está num duelo")); return; }
        Them->AddPending(Me, TEXT("duel"));
        Them->Notice(FString::Printf(TEXT("%s desafiou você para um duelo  ·  chegue perto e aperte E"), *MyName), 4);
        Notice(FString::Printf(TEXT("Você desafiou %s para um duelo"), *TheirName));
    }
    else if (Action == TEXT("duel_accept"))
    {
        if (!TakePending(Target, TEXT("duel")) || DuelPhase != 0 || Them->DuelPhase != 0) return;
        BeginDuel(Target);
    }
    else if (Action == TEXT("duel_cancel"))
    {
        if (DuelOpponent) EndDuel(FString::Printf(TEXT("Duelo cancelado por %s"), *MyName), nullptr);
    }
    else if (Action == TEXT("trade_invite"))
    {
        if (TradePartner || Them->TradePartner) { Notice(TEXT("Alguém já está numa troca")); return; }
        Them->AddPending(Me, TEXT("trade"));
        Them->Notice(FString::Printf(TEXT("%s quer trocar com você  ·  chegue perto e aperte E"), *MyName), 2);
        Notice(FString::Printf(TEXT("Proposta de troca enviada para %s"), *TheirName));
    }
    else if (Action == TEXT("trade_accept"))
    {
        if (!TakePending(Target, TEXT("trade")) || TradePartner || Them->TradePartner) return;
        BeginTrade(Target);
    }
    else if (Action == TEXT("trade_cancel")) EndTrade(FString::Printf(TEXT("Troca cancelada por %s"), *MyName));
}

void UVortexSocial::JoinParty(AVortexCharacter* Inviter)
{
    UVortexSocial* Them = Of(Inviter);
    if (!Them) return;
    if (Them->PartyId == 0) Them->PartyId = GNextPartyId++;
    if (Them->PartyMates().Num() + 1 >= PartyLimit) { Notice(TEXT("O grupo está cheio (8)"), 4); return; }
    if (PartyId != 0 && PartyId != Them->PartyId) LeaveParty();
    PartyId = Them->PartyId;
    for (AVortexCharacter* Mate : PartyMates())
        if (UVortexSocial* S = Of(Mate)) S->Notice(FString::Printf(TEXT("%s entrou no grupo"), *GetHeroName()), 2);
    Notice(FString::Printf(TEXT("Você entrou no grupo de %s"), *Them->GetHeroName()), 2);
}

void UVortexSocial::LeaveParty()
{
    if (PartyId == 0) return;
    TArray<AVortexCharacter*> Mates = PartyMates();
    PartyId = 0;
    Notice(TEXT("Você saiu do grupo"));
    for (AVortexCharacter* Mate : Mates)
        if (UVortexSocial* S = Of(Mate))
        {
            S->Notice(FString::Printf(TEXT("%s saiu do grupo"), *GetHeroName()));
            if (Mates.Num() == 1) S->PartyId = 0;   // sobrou um só: o grupo acaba
        }
}

void UVortexSocial::BeginDuel(AVortexCharacter* Other)
{
    UVortexSocial* Them = Of(Other);
    AVortexCharacter* Me = Hero();
    if (!Them || !Me) return;
    for (UVortexSocial* Side : { this, Them })
    {
        Side->DuelOpponent = Side == this ? Other : Me;
        Side->DuelPhase = 1;
        Side->DuelFightAt = Now() + 3.5f;
        const UAbilitySystemComponent* Abilities = Side->Hero() ? Side->Hero()->GetAbilitySystemComponent() : nullptr;
        Side->DuelHealthBefore = Abilities ? Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) : -1.f;
        Side->Notice(FString::Printf(TEXT("Duelo contra %s!"), *(Side == this ? Them->GetHeroName() : GetHeroName())), 4);
    }
    // 02/11: o domo de energia entre os dois, com o meio no chão
    const FVector Mid = (Me->GetActorLocation() + Other->GetActorLocation()) * .5f;
    const float Floor = FMath::Min(Me->GetActorLocation().Z, Other->GetActorLocation().Z) - Me->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (AVortexDuelDome* Dome = GetWorld()->SpawnActor<AVortexDuelDome>(FVector(Mid.X, Mid.Y, Floor), FRotator::ZeroRotator, Params))
    {
        DuelDome = Dome;
        UE_LOG(LogTemp, Display, TEXT("VORTEX_DUEL domo em %s"), *Dome->GetActorLocation().ToString());
        Them->DuelDome = Dome;
    }
    for (UVortexSocial* Side : { this, Them }) Side->SetComponentTickInterval(0.f);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_DUEL começou %s x %s"), *GetHeroName(), *Them->GetHeroName());
}

void UVortexSocial::FinishDuelSide(const FString& Message)
{
    // volta à vida de antes da luta
    if (AVortexCharacter* Me = Hero())
        if (UAbilitySystemComponent* Abilities = Me->GetAbilitySystemComponent(); Abilities && DuelHealthBefore > 0.f)
            Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), FMath::Min(DuelHealthBefore, Abilities->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute())));
    DuelOpponent = nullptr;
    DuelPhase = 0;
    DuelHealthBefore = -1.f;
    if (AVortexDuelDome* Dome = DuelDome.Get()) Dome->Destroy();
    DuelDome.Reset();
    SetComponentTickInterval(.1f);
    if (!Message.IsEmpty()) Notice(Message, 2);
}

void UVortexSocial::EndDuel(const FString& Message, AVortexCharacter* Winner)
{
    UVortexSocial* Them = Of(DuelOpponent);
    const FString WinnerName = Winner ? (Of(Winner) ? Of(Winner)->GetHeroName() : FString()) : FString();
    const FString Line = Winner ? FString::Printf(TEXT("%s venceu o duelo!"), *WinnerName) : Message;
    UE_LOG(LogTemp, Display, TEXT("VORTEX_DUEL acabou: %s"), *Line);
    FinishDuelSide(Line);
    if (Them) Them->FinishDuelSide(Line);
}

void UVortexSocial::DuelLost(AVortexCharacter* Winner)
{
    if (DuelPhase == 2 && DuelOpponent == Winner) EndDuel(FString(), Winner);
}

void UVortexSocial::BeginTrade(AVortexCharacter* Other)
{
    UVortexSocial* Them = Of(Other);
    if (!Them) return;
    TradePartner = Other;
    Them->TradePartner = Hero();
    MyOffer = TheirOffer = Them->MyOffer = Them->TheirOffer = FVortexTradeOffer();
    Notice(FString::Printf(TEXT("Troca com %s"), *Them->GetHeroName()), 2);
    Them->Notice(FString::Printf(TEXT("Troca com %s"), *GetHeroName()), 2);
}

void UVortexSocial::EndTrade(const FString& Message)
{
    UVortexSocial* Them = Of(TradePartner);
    for (UVortexSocial* Side : { this, Them })
    {
        if (!Side) continue;
        Side->TradePartner = nullptr;
        Side->MyOffer = Side->TheirOffer = FVortexTradeOffer();
        if (!Message.IsEmpty()) Side->Notice(Message);
    }
}

bool UVortexSocial::ServerTradeOffer_Validate(const TArray<FVortexItemStack>& Items, int32 Coins) { return Items.Num() <= 24 && Coins >= 0; }

void UVortexSocial::ServerTradeOffer_Implementation(const TArray<FVortexItemStack>& Items, int32 Coins)
{
    UVortexSocial* Them = Of(TradePartner);
    const UVortexHeroSheet* Sheet = Hero() ? Hero()->GetSheet() : nullptr;
    if (!Them || !Sheet) return;
    // só o que existe na mochila (somado por item) e as moedas que tem
    FVortexTradeOffer Offer;
    TMap<FName, int32> Wanted;
    for (const FVortexItemStack& Stack : Items) if (!Stack.IsEmpty()) Wanted.FindOrAdd(Stack.Item) += Stack.Count;
    for (const TPair<FName, int32>& Pair : Wanted)
    {
        const int32 Have = FMath::Min(Pair.Value, Sheet->CountOf(Pair.Key));
        if (Have > 0) { FVortexItemStack S; S.Item = Pair.Key; S.Count = Have; Offer.Items.Add(S); }
    }
    Offer.Coins = FMath::Clamp(Coins, 0, Sheet->GetCoins());
    // qualquer mudança desfaz o "pronto" dos dois
    MyOffer = Offer;
    Them->TheirOffer = Offer;
    Them->MyOffer.bReady = false;
    TheirOffer.bReady = false;
}

void UVortexSocial::ServerTradeReady_Implementation(bool bReady)
{
    UVortexSocial* Them = Of(TradePartner);
    if (!Them) return;
    MyOffer.bReady = bReady;
    Them->TheirOffer.bReady = bReady;
    if (MyOffer.bReady && TheirOffer.bReady) TryCompleteTrade();
}

void UVortexSocial::TryCompleteTrade()
{
    UVortexSocial* Them = Of(TradePartner);
    UVortexHeroSheet* A = Hero() ? Hero()->GetSheet() : nullptr;
    UVortexHeroSheet* B = Them && Them->Hero() ? Them->Hero()->GetSheet() : nullptr;
    if (!A || !B) return;
    auto Has = [](const UVortexHeroSheet* Sheet, const FVortexTradeOffer& Offer)
    {
        if (Sheet->GetCoins() < Offer.Coins) return false;
        for (const FVortexItemStack& S : Offer.Items) if (Sheet->CountOf(S.Item) < S.Count) return false;
        return true;
    };
    if (!Has(A, MyOffer) || !Has(B, Them->MyOffer)) { EndTrade(TEXT("A troca falhou: alguém não tem mais o que ofereceu")); return; }
    const FVortexTradeOffer Mine = MyOffer, Theirs = Them->MyOffer;
    auto Move = [](UVortexHeroSheet* From, UVortexHeroSheet* To, const FVortexTradeOffer& Offer)
    {
        for (const FVortexItemStack& S : Offer.Items)
        {
            const int32 Taken = From->TakeItem(S.Item, S.Count);
            const int32 Left = To->GiveItem(S.Item, Taken);
            if (Left > 0) From->GiveItem(S.Item, Left, false);   // não coube: volta para quem deu
        }
        if (Offer.Coins > 0 && From->SpendCoins(Offer.Coins)) To->AddCoins(Offer.Coins);
    };
    Move(A, B, Mine);
    Move(B, A, Theirs);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_TRADE %s <-> %s concluída"), *GetHeroName(), *Them->GetHeroName());
    VortexAudio::Play(Hero(), TEXT("coin"), Hero()->GetActorLocation(), .7f);
    EndTrade(TEXT("Troca concluída"));
}

void UVortexSocial::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    AVortexCharacter* Me = Hero();
    if (!Me) return;
    // o dono manda o nome do herói (o do save) uma vez
    if (!bNameSent && Me->IsLocallyControlled())
    {
        bNameSent = true;
        const UVortexGameInstance* Game = Cast<UVortexGameInstance>(GetWorld()->GetGameInstance());
        const UVortexPlayerSave* Save = Game ? Game->GetSelectedHero() : nullptr;
        if (Save && !Save->HeroName.IsEmpty()) ServerSetHeroName(Save->HeroName);
        // a cor escolhida antes (Shift+Z), se houver
        int32 Saved = -1;
        if (GConfig && GConfig->GetInt(TEXT("Vortex"), TEXT("PlayerColor"), Saved, GGameUserSettingsIni) && Saved >= 0 && Saved < PaletteCount)
        {
            PlayerColor = uint8(Saved);
            ServerSetPlayerColor(uint8(Saved));
        }
    }
    if (!Me->HasAuthority()) return;
    // duelo: contagem, luta, distância e quem saiu
    if (DuelPhase != 0)
    {
        UVortexSocial* Them = Of(DuelOpponent);
        if (!Them || !IsValid(DuelOpponent)) { FinishDuelSide(TEXT("Duelo cancelado")); }
        else if (FVector::Dist(Me->GetActorLocation(), DuelOpponent->GetActorLocation()) > 4500.f) EndDuel(TEXT("Duelo cancelado: longe demais"), nullptr);
        else if (DuelPhase == 1 && Now() >= DuelFightAt) { DuelPhase = 2; Notice(TEXT("LUTEM!"), 4); }
        if (DuelPhase != 0) KeepDome();
    }
    // troca: afastar-se cancela
    if (TradePartner && (!IsValid(TradePartner) || FVector::Dist(Me->GetActorLocation(), TradePartner->GetActorLocation()) > 1000.f))
        EndTrade(TEXT("Troca cancelada: longe demais"));
}

void UVortexSocial::KeepDome()
{
    // 02/11: o domo segura os dois dentro e empurra todo o resto (heróis e criaturas) para fora
    AVortexDuelDome* Dome = DuelDome.Get();
    AVortexCharacter* Me = Hero();
    if (!Dome || !Me) return;
    const FVector Center = Dome->GetActorLocation();
    auto Place = [&](ACharacter* Who, float Distance)
    {
        FVector Dir = Who->GetActorLocation() - Center;
        Dir.Z = 0.f;
        if (!Dir.Normalize()) Dir = FVector::ForwardVector;
        const FVector To(Center.X + Dir.X * Distance, Center.Y + Dir.Y * Distance, Who->GetActorLocation().Z + 5.f);
        Who->SetActorLocation(To, false, nullptr, ETeleportType::TeleportPhysics);
        if (UCharacterMovementComponent* Move = Who->GetCharacterMovement()) Move->Velocity = Distance < AVortexDuelDome::Radius ? FVector::ZeroVector : FVector(Dir.X, Dir.Y, 0.f) * 300.f;
    };
    // eu: dentro (quem bate na parede volta um pouco)
    const float Inner = AVortexDuelDome::Radius - Me->GetCapsuleComponent()->GetScaledCapsuleRadius() - 10.f;
    if (FVector::Dist2D(Me->GetActorLocation(), Center) > Inner) Place(Me, Inner - 20.f);
    // os outros: só um dos dois lados cuida (o que criou o domo guarda o mesmo ponteiro, então basta comparar)
    if (DuelOpponent && Of(DuelOpponent) && Of(DuelOpponent) < this) return;
    for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
    {
        ACharacter* Who = *It;
        if (Who == Me || Who == DuelOpponent) continue;
        const float Outer = AVortexDuelDome::Radius + Who->GetCapsuleComponent()->GetScaledCapsuleRadius() + 10.f;
        if (FVector::Dist2D(Who->GetActorLocation(), Center) < Outer) Place(Who, Outer + 30.f);
    }
}
