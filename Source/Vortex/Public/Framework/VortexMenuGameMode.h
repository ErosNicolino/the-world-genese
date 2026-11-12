#pragma once

#include "GameFramework/GameModeBase.h"
#include "VortexMenuGameMode.generated.h"

/**
 * The front end loads no combat pawn or gameplay HUD. Aberto como servidor (listen), o mapa do menu
 * vira a sala de espera: até 8 jogadores, cada um com o herói do próprio computador.
 */
UCLASS()
class VORTEX_API AVortexMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AVortexMenuGameMode();
    static constexpr int32 MaxLobbyPlayers = 8;
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
};
