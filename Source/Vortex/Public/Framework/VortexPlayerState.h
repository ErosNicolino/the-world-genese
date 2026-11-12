#pragma once

#include "GameFramework/PlayerState.h"
#include "Framework/VortexCharacter.h"
#include "VortexPlayerState.generated.h"

UCLASS()
class VORTEX_API AVortexPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AVortexPlayerState();

    UFUNCTION(BlueprintPure, Category = "Vortex|Session")
    int32 GetSessionSlot() const { return SessionSlot; }

    void SetSessionSlot(int32 NewSlot);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ---- sala de espera (no mapa do menu, antes de começar a aventura): cada um mostra o herói que
    // escolheu no próprio computador; o anfitrião começa quando todos estão prontos
    const FString& GetLobbyHero() const { return LobbyHero; }
    int32 GetLobbyLevel() const { return LobbyLevel; }
    bool IsLobbyReady() const { return bLobbyReady; }
    bool IsLobbyHost() const { return bLobbyHost; }
    const FVortexNetLook& GetLobbyLook() const { return LobbyLook; }
    bool HasLobbyHero() const { return !LobbyHero.IsEmpty(); }
    void SetLobbyHost(bool bHost) { bLobbyHost = bHost; ForceNetUpdate(); }

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetLobbyHero(const FString& Hero, int32 Level, const FVortexNetLook& Look);
    UFUNCTION(Server, Reliable)
    void ServerSetLobbyReady(bool bReady);

private:
    UPROPERTY(ReplicatedUsing = OnRep_SessionSlot)
    int32 SessionSlot = INDEX_NONE;

    UFUNCTION()
    void OnRep_SessionSlot();

    UPROPERTY(Replicated) FString LobbyHero;
    UPROPERTY(Replicated) int32 LobbyLevel = 1;
    UPROPERTY(Replicated) bool bLobbyReady = false;
    UPROPERTY(Replicated) bool bLobbyHost = false;
    UPROPERTY(Replicated) FVortexNetLook LobbyLook;
};
