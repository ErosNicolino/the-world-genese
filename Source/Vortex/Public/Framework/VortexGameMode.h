#pragma once

#include "GameFramework/GameModeBase.h"
#include "VortexGameMode.generated.h"

UCLASS()
class VORTEX_API AVortexGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AVortexGameMode();

    virtual void StartPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    /** Até 8 jogadores no mundo (como a sala). */
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void Logout(AController* Exiting) override;
    bool SaveActiveWorld();
    /** Hora (relógio do mundo) do último save automático (a HUD mostra "Jogo salvo"). */
    static float LastAutosaveTime;
    /** 09/11: os heróis dos convidados guardados no mundo do anfitrião. */
    /** Account: a conta da Epic do convidado; vazio na rede local. */
    static FString GuestAccount(const class APawn* Pawn);
    struct FVortexGuestHero* FindGuest(const FString& Account, const FGuid& HeroId, bool bCreate = false);
    /** A ficha que vale para o convidado: a cópia daqui; herói desconhecido entra como novo (nos mundos novos). */
    struct FVortexSheetSave DecideGuestSheet(const FString& Account, const FVortexSheetSave& Incoming);
    bool UsesGuestCopies() const;
    /** Grava só o mundo com os convidados como estão, sem mexer na posição do anfitrião. */
    void SaveGuestsNow();

private:
#if !UE_BUILD_SHIPPING
    void BeginMovementSmoke(APlayerController* NewPlayer, bool bStepProbe);
    void BeginClientInputObserve(APlayerController* NewPlayer);
    void BeginMatrixSmoke(APlayerController* NewPlayer);
    void BeginMoveBench(int32 Count);
    void BeginTerrainBench(APlayerController* NewPlayer);
    void BeginAnimShot(APlayerController* NewPlayer);
    void BeginDodgeDirectionSmoke(APlayerController* NewPlayer);
    void BeginSolidWalkSmoke(APlayerController* NewPlayer);
    void BeginWardrobeShot(APlayerController* NewPlayer);
    void BeginCombatSmoke(APlayerController* NewPlayer);
    void BeginSwordSmoke(APlayerController* NewPlayer);
    void BeginSwordShot(APlayerController* NewPlayer, bool bNormalCamera = false);
    void BeginWolfSmoke(APlayerController* NewPlayer);
    void BeginWolfVisualShot(APlayerController* NewPlayer);
    void BeginWolfMotionProbe(APlayerController* NewPlayer);
    void BeginPackSmoke(APlayerController* NewPlayer);
    void BeginCameraShot(APlayerController* NewPlayer, bool bBefore);
    void BeginCameraSequenceShot(APlayerController* NewPlayer);
    void BeginCameraSmoke(APlayerController* NewPlayer);
    void BeginFogShot(APlayerController* NewPlayer);
    void BeginStreamSmoke();
    void BeginPopulationSmoke(APlayerController* NewPlayer);
    void BeginDayNightShot(APlayerController* NewPlayer);
    void BeginDayNightSmoke();
    void BeginGrassShot(APlayerController* NewPlayer);
    void BeginHudShot(APlayerController* NewPlayer);
    void BeginWeaponShot(APlayerController* NewPlayer);
    void BeginStadiumShot(APlayerController* NewPlayer);
    void BeginStadiumPick(APlayerController* NewPlayer);
    void BeginTownShot(APlayerController* NewPlayer);
    FTimerHandle TerrainBenchTimer;
    FTimerHandle MatrixSmokeTimer;
    FTimerHandle MoveBenchTimer;
    FTimerHandle MovementSmokeTimer;
    FTimerHandle ClientInputObserveTimer;
    FTimerHandle WolfSmokeTimer;
    FTimerHandle CameraSmokeTimer;
#endif

    UPROPERTY()
    TObjectPtr<class UVortexWorldSave> ActiveWorldSave;
    bool bPersistentWorld = false;
    TWeakObjectPtr<class AVortexCharacter> ResumingHost;
    FVector ResumeLocation = FVector::ZeroVector;
    float ResumeStartedAt = 0.f;
    FTimerHandle ResumeTimer;
    /** Save automático do mundo a cada 45 s (posição do anfitrião, moradores, hora do dia). */
    FTimerHandle AutosaveTimer;
    void Autosave();
    void BeginRestoreHost();
    void TickRestoreHost();

    int32 NextSessionSlot = 0;
    int32 ConnectedPlayerCount = 0;
};
