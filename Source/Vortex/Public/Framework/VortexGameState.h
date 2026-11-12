#pragma once

#include "World/VortexWorldLayout.h"

#include "GameFramework/GameStateBase.h"
#include "VortexGameState.generated.h"

// Relógio do dia (Fase 15). O servidor guarda só o ponto de partida; todas as máquinas calculam a
// mesma hora a partir do tempo de servidor replicado, sem mandar a hora a cada quadro.
USTRUCT()
struct FVortexDayClockSync
{
    GENERATED_BODY()

    UPROPERTY()
    float ClockAtSync = 0.f;       // segundos dentro do ciclo (0 = nascer do sol)

    UPROPERTY()
    int32 DayAtSync = 1;

    UPROPERTY()
    double SyncServerTime = 0.0;

    UPROPERTY()
    float Scale = 1.f;             // 1 = tempo real do jogo; 0 = parado
};

UCLASS()
class VORTEX_API AVortexGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AVortexGameState();

    UFUNCTION(BlueprintPure, Category = "Vortex|Session")
    int32 GetConnectedPlayers() const { return ConnectedPlayers; }

    UFUNCTION(BlueprintPure, Category = "Vortex|World")
    FGuid GetActiveWorldId() const { return ActiveWorldId; }

    UFUNCTION(BlueprintPure, Category = "Vortex|World")
    int32 GetWorldSeed() const { return WorldSeed; }

    UFUNCTION(BlueprintPure, Category = "Vortex|World")
    int32 GetGeneratorVersion() const { return GeneratorVersion; }

    // Dia e noite, como o DayNight.cs do Godot: 30 min de dia (6h às 18h) e 8 min de noite.
    static constexpr float DaySeconds = VortexWorldLayout::DaySeconds;
    static constexpr float NightSeconds = VortexWorldLayout::NightSeconds;
    static constexpr float CycleSeconds = VortexWorldLayout::CycleSeconds;

    float GetDayClock() const { return ClockAt(GetServerWorldTimeSeconds()); }
    int32 GetDayNumber() const;
    float GetHour() const { return HourOfClock(GetDayClock()); }
    // relógio em qualquer instante do tempo de servidor (o mesmo cálculo em todas as máquinas)
    float ClockAt(double ServerTime) const;
    float GetDayTimeScale() const { return DayClock.Scale; }
    static float HourOfClock(float Clock);
    static float ClockOfHour(float Hour);
    // Servidor
    void SetDayClock(float Clock, int32 Day);
    void SetDayHour(float Hour) { SetDayClock(ClockOfHour(Hour), GetDayNumber()); }
    void SetDayTimeScale(float Scale);

    void SetConnectedPlayers(int32 NewCount);
    void SetWorldDescriptor(const FGuid& NewWorldId, int32 NewSeed, int32 NewGeneratorVersion);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Coletáveis colhidos e até quando (tempo de servidor): quem entra depois vê as moitas certas. */
    UPROPERTY(Replicated) TArray<FName> TakenForage;
    UPROPERTY(Replicated) TArray<float> TakenForageUntil;
    /** Servidor: marca (e esquece os que já voltaram). */
    void MarkForage(FName Id, float Until);

private:
    UPROPERTY(Replicated)
    FVortexDayClockSync DayClock;

    double TotalAt(double ServerTime) const;

    UPROPERTY(ReplicatedUsing = OnRep_ConnectedPlayers)
    int32 ConnectedPlayers = 0;

    UPROPERTY(ReplicatedUsing = OnRep_WorldDescriptor)
    FGuid ActiveWorldId;

    UPROPERTY(ReplicatedUsing = OnRep_WorldDescriptor)
    int32 WorldSeed = 0;

    UPROPERTY(ReplicatedUsing = OnRep_WorldDescriptor)
    int32 GeneratorVersion = 0;

    UFUNCTION()
    void OnRep_ConnectedPlayers();

    UFUNCTION()
    void OnRep_WorldDescriptor();
};
