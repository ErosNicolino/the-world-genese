#include "Framework/VortexMenuGameMode.h"
#include "GameFramework/HUD.h"
#include "GameFramework/SpectatorPawn.h"
#include "Framework/VortexGameState.h"
#include "Framework/VortexPlayerState.h"
#include "Framework/VortexNetLog.h"

AVortexMenuGameMode::AVortexMenuGameMode()
{
    DefaultPawnClass = ASpectatorPawn::StaticClass();
    // HUD vazia (o menu é Slate): sem classe o controle tentava criar a HUD e avisava "no class was specified"
    HUDClass = AHUD::StaticClass();
    bStartPlayersAsSpectators = true;
    // relógio do dia: o mundinho do menu usa o dia e noite do jogo (parado num fim de manhã)
    GameStateClass = AVortexGameState::StaticClass();
    // sala de espera: nome, nível, visual e "pronto" de cada jogador
    PlayerStateClass = AVortexPlayerState::StaticClass();
}

void AVortexMenuGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    if (ErrorMessage.IsEmpty() && GetNumPlayers() >= MaxLobbyPlayers) ErrorMessage = TEXT("A sala está cheia (8 jogadores).");
    VortexNetLog::Write(FString::Printf(TEXT("Sala: chegando %s %s"), *Address, ErrorMessage.IsEmpty() ? TEXT("(aceito)") : *(TEXT("(recusado: ") + ErrorMessage + TEXT(")"))));
}

void AVortexMenuGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (AVortexPlayerState* State = NewPlayer ? NewPlayer->GetPlayerState<AVortexPlayerState>() : nullptr)
    {
        State->SetLobbyHost(NewPlayer->IsLocalController());
        UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY entrou %s (jogadores=%d)"), *State->GetPlayerName(), GetNumPlayers());
        if (!NewPlayer->IsLocalController()) VortexNetLog::Write(FString::Printf(TEXT("Sala: entrou %s (jogadores=%d)"), *State->GetPlayerName(), GetNumPlayers()));
    }
}

void AVortexMenuGameMode::Logout(AController* Exiting)
{
    if (const APlayerState* State = Exiting ? Exiting->GetPlayerState<APlayerState>() : nullptr)
    {
        VortexNetLog::Write(FString::Printf(TEXT("Sala: saiu %s"), *State->GetPlayerName()));
    }
    Super::Logout(Exiting);
}
