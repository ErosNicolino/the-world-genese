#include "Framework/VortexPlayerState.h"

#include "Net/UnrealNetwork.h"

AVortexPlayerState::AVortexPlayerState()
{
    bReplicates = true;
}

void AVortexPlayerState::SetSessionSlot(int32 NewSlot)
{
    if (!HasAuthority())
    {
        return;
    }
    SessionSlot = NewSlot;
    ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("Vortex framework: server assigned session slot=%d"), SessionSlot);
}

void AVortexPlayerState::OnRep_SessionSlot()
{
    UE_LOG(LogTemp, Display, TEXT("Vortex framework: client received session slot=%d"), SessionSlot);
}

void AVortexPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AVortexPlayerState, SessionSlot);
    DOREPLIFETIME(AVortexPlayerState, LobbyHero);
    DOREPLIFETIME(AVortexPlayerState, LobbyLevel);
    DOREPLIFETIME(AVortexPlayerState, bLobbyReady);
    DOREPLIFETIME(AVortexPlayerState, bLobbyHost);
    DOREPLIFETIME(AVortexPlayerState, LobbyLook);
}

bool AVortexPlayerState::ServerSetLobbyHero_Validate(const FString& Hero, int32 Level, const FVortexNetLook& Look)
{
    return Hero.Len() <= 24 && Level >= 1 && Level < 1000 && Look.Categories.Num() <= 32 && Look.Colors.Num() <= 32;
}

void AVortexPlayerState::ServerSetLobbyHero_Implementation(const FString& Hero, int32 Level, const FVortexNetLook& Look)
{
    LobbyHero = Hero;
    LobbyLevel = Level;
    LobbyLook = Look;
    ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY %s escolheu %s (nivel %d)"), *GetPlayerName(), *Hero, Level);
}

void AVortexPlayerState::ServerSetLobbyReady_Implementation(bool bReady)
{
    // pronto só com herói escolhido (o convidado agora entra na sala antes de escolher)
    bLobbyReady = bReady && HasLobbyHero();
    ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY %s %s"), *GetPlayerName(), bReady ? TEXT("pronto") : TEXT("esperando"));
}
