#pragma once

#include "Engine/GameInstance.h"
#include "Persistence/VortexSaveSubsystem.h"
#include "Framework/VortexLook.h"
#include "VortexGameInstance.generated.h"

class SWidget;
class UVortexWorldSave;
class UVortexPlayerSave;
class UTexture2D;

UCLASS()
class VORTEX_API UVortexGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;
    virtual void Shutdown() override;

    bool CreateAndOpenWorld(const FString& Name, int32 Seed, bool bHost, FString& Error);
    bool CreateWorldOnly(const FString& Name, int32 Seed, FGuid& OutWorldId, FString& Error, bool bMultiplayer = false);
    bool OpenHostedWorld(FGuid WorldId, bool bHost, FString& Error);
    bool CreateHero(FGuid WorldId, const FString& Name, FName RaceId, FName AppearanceId,
        const TMap<FName, FName>& Wardrobe, FGuid& OutPlayerId, FString& Error,
        const TMap<FName, FColor>& Colors = TMap<FName, FColor>());
    /** Visual salvo de um herói (raça, peças e cores), para o palco do menu e para o jogo. */
    bool GetHeroLook(FGuid PlayerId, FVortexLook& OutLook) const;
    static FVortexLook LookFromSave(const UVortexPlayerSave* Hero);
    class AVortexMenuStage* GetMenuStage() const { return MenuStage.Get(); }
    /** Segura um asset usado pelo menu (as imagens do Slate não impedem a coleta de lixo). */
    void KeepForMenu(UObject* Asset) { if (Asset) MenuAssets.AddUnique(Asset); }
    bool OpenWorldWithHero(FGuid WorldId, FGuid PlayerId, bool bHost, FString& Error);
    /** 07/11: o Arsenal (o modo de teste: todas as armas e armaduras num mapa leve), com a aparência deste herói; nada
     *  do que acontece lá vai para o save. Sem herói (PlayerId vazio), um herói de teste. */
    void OpenArsenal(FGuid PlayerId);
    TArray<FVortexHeroSummary> GetHeroes(FGuid WorldId) const;
    bool DeleteHero(FGuid WorldId, FGuid PlayerId, FString& Error);
    /** Apaga o mundo e os heróis dele. */
    bool DeleteWorld(FGuid WorldId, FString& Error);
    /** Quantos mundos cabem neste computador (o menu não cria mais que isso). */
    static constexpr int32 MaxWorlds = 5;
    UVortexPlayerSave* GetSelectedHero() const;
    bool JoinByAddress(const FString& Address, FString& Error);
    /** Convidado de uma sala online: viaja até o anfitrião pelo P2P da Epic ("EOS:<id>"). */
    bool JoinOnline(const FString& EosAddress, FString& Error);
    void ReturnToMenu();

    /** Heróis viajantes: os do próprio computador que entram no mundo de um amigo (sem mundo próprio). */
    static FGuid TravellerWorld() { return FGuid(0x56525458, 0x54524156, 0x454C4C45, 0x52000001); }
    void SetSelectedHero(FGuid PlayerId) { SelectedPlayerId = PlayerId; }
    FGuid GetSelectedHeroId() const { return SelectedPlayerId; }
    /** O mundo foi aberto pelo menu (ou pela sala de espera), e não direto pelo editor. */
    bool StartedFromFrontEnd() const { return bStartedFromFrontEnd; }
    /** Sala de espera: o mapa do menu abre como servidor (rede local ou sala da Epic) e todos se encontram lá. */
    bool OpenLobby(FGuid WorldId, FGuid PlayerId, bool bOnline, FString& Error);
    /** No mapa do menu e conectado (anfitrião ou convidado). */
    bool InLobby() const;
    bool IsLobbyHost() const;
    bool IsLobbyOnline() const { return bLobbyOnline; }
    /** Endereço para os amigos da rede local (IP:porta). */
    FString GetLobbyAddress() const;
    /**
     * Código da sala para passar aos amigos: o da Epic (5 letras) ou, na rede local, o IP escondido
     * num código "L" (ninguém precisa digitar IP).
     */
    FString GetLobbyCode() const;
    static FString LanCodeFromAddress(const FString& Address);
    static bool AddressFromLanCode(const FString& Code, FString& OutAddress);
    /** Aviso para o menu quando ele abrir (por exemplo, "a Epic não respondeu"). */
    void SetMenuNotice(const FString& Message) { PendingMenuError = Message; }
    /** Ainda tentando conectar num anfitrião da Epic que recusou (carregando o mundo). */
    void ClearHostRetry();
    bool IsRetryingHost() const { return !RetryHost.IsEmpty() && FPlatformTime::Seconds() < RetryUntil; }
    /** Anfitrião: leva todos da sala ao mundo escolhido. */
    bool StartAdventure(FString& Error);
    void TogglePauseMenu();
    bool IsPauseMenuOpen() const { return PauseWidget.IsValid(); }
    UVortexWorldSave* ConsumeRequestedWorld();
    TArray<FVortexWorldSummary> GetWorlds() const;
    /** Só os mundos de um jogador (Jogar) ou só os de multiplayer (cada lista guarda até MaxWorlds). */
    TArray<FVortexWorldSummary> GetWorlds(bool bMultiplayer) const;
    /** Heróis por mundo: 1 no mundo de um jogador, 3 no de multiplayer e 3 viajantes. */
    int32 MaxHeroes(FGuid WorldId) const;
    UTexture2D* GetMenuBackground();

private:
    void OnWorldLoaded(UWorld* LoadedWorld);
    void HideFrontEnd();
    void HidePauseMenu();
    void OnNetworkFailure(UWorld* World, class UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
    void OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
    TWeakObjectPtr<class AVortexMenuStage> MenuStage;
    FGuid RequestedWorldId;
    FGuid SelectedPlayerId;
    TSharedPtr<SWidget> FrontEndWidget;
    TSharedPtr<SWidget> PauseWidget;
    FDelegateHandle MapLoadedHandle;
    FDelegateHandle NetworkFailureHandle;
    FDelegateHandle TravelFailureHandle;
    bool bStartedFromFrontEnd = false;
    bool bLobbyOnline = false;
    // o timer é do GameInstance (sobrevive à troca de mapa): cada um para sozinho
    FTimerHandle LobbyHeroTimer, ApplyLookTimer;
    int32 ApplyLookTries = 0;
    FString PendingMenuError;
    FString PauseStatus;
    // reconectar ao anfitrião da Epic quando ele ainda está carregando o mundo (recusa a conexão por uns segundos)
    FString RetryHost;
    double RetryUntil = 0.0;
    int32 RetryCount = 0;
    bool bRetryToWorld = false;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> MenuBackground;
    UPROPERTY(Transient) TArray<TObjectPtr<UObject>> MenuAssets;
#if !UE_BUILD_SHIPPING
    int32 MenuFlowSmokePhase = 0;
    FGuid MenuFlowSmokeWorldId;
    FVector MenuFlowSmokeLocation = FVector::ZeroVector;
    bool bMenuFlowSmokePassed = true;
    bool bMenuJoinSmokeStarted = false;
    void BeginLobbySmoke();
    /** Saves criados pelo teste: apagados de novo ao fechar (o mundo ainda se salva ao sair). */
    TArray<FString> SmokeSlots;
    void DeleteSmokeSlots();
#endif
};
