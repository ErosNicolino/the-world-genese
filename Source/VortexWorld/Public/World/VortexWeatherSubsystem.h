#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "VortexWeatherSubsystem.generated.h"

class AActor;
class UAudioComponent;
class UExponentialHeightFogComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/** O tempo que está fazendo (o que a HUD mostra embaixo do minimapa). */
UENUM()
enum class EVortexWeather : uint8 { Clear, Cloudy, Rain, Storm, Snow, Blizzard, Fog };

/**
 * Clima e estações (11/11, o usuário: "faça o clima no jogo, neve, chuva, tempestade; embaixo do minimapa o horário e a
 * estação"), em cada máquina que desenha.
 *
 * - Estação: 3 dias de jogo cada (Primavera, Verão, Outono, Inverno; o dia 1 é o começo da primavera).
 * - O tempo é sorteado a cada 6 horas de jogo pelo dia, pela janela e pela região da câmera: todas as máquinas veem o
 *   mesmo sem rede. Solcânion quase não chove; Brasafenda só fecha (cinza); Nivaluz neva; Lodonegro e Morthúria têm
 *   neblina e chuva; no inverno a chuva vira neve (menos no deserto e no vulcão). O dia 1 é sempre limpo.
 * - Cada número (nuvens, chuva, neve, neblina, vento) anda devagar até o do tempo novo (~15 s); a última hora de cada
 *   janela já mistura com a próxima.
 * - O que o tempo muda: o sol, o céu e a luz ambiente (UVortexDayNightSubsystem lê GetOvercast/GetFlash), a neblina,
 *   a chuva e a neve em volta da câmera (dois cilindros com o material M_VortexPrecip, que somem debaixo de um teto), os
 *   relâmpagos com trovão na tempestade e os sons de chuva e vento.
 * -VortexNoWeather desliga; -VortexWeather=Rain (ou Storm, Snow, Blizzard, Fog, Cloudy, Clear) força um tempo.
 */
UCLASS()
class VORTEXWORLD_API UVortexWeatherSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; }

    /** 0 Primavera, 1 Verão, 2 Outono, 3 Inverno. */
    int32 GetSeason() const { return Season; }
    static const TCHAR* SeasonName(int32 InSeason);
    EVortexWeather GetWeather() const { return Shown; }
    static const TCHAR* WeatherName(EVortexWeather Weather);
    /** 0 céu limpo .. 1 fechado (o sol e o céu escurecem). */
    float GetOvercast() const { return Overcast; }
    float GetRain() const { return Rain; }
    float GetSnow() const { return Snow; }
    /** O relâmpago agora (0..1, some em ~0,3 s). */
    float GetFlash() const { return Flash; }
    /** Chat de testes (/clima): força um tempo, já cheio (sem os ~15 s de transição); ClearForcedWeather volta ao sorteio. */
    void ForceWeather(EVortexWeather Weather) { DebugWeather = Weather; bDebugWeather = bSnap = true; }
    void ClearForcedWeather() { bDebugWeather = false; bSnap = true; }

private:
    int32 Season = 0;
    EVortexWeather Shown = EVortexWeather::Clear;
    float Overcast = 0.f, Rain = 0.f, Snow = 0.f, FogAmount = 0.f, Wind = .2f, Flash = 0.f;
    float BoltIn = 8.f, FlashAge = 10.f, ThunderIn = -1.f, RoofCheckIn = 0.f, Shelter = 0.f, BaseFog = -1.f, BaseFogStart = 6000.f;
    bool bUnderRoof = false, bBuilt = false, bDisabled = false, bDebugWeather = false, bSnap = false;
    EVortexWeather DebugWeather = EVortexWeather::Clear;

    UPROPERTY(Transient) TObjectPtr<AActor> Fx;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> RainNear;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> RainFar;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> SnowNear;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> SnowFar;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Sheets;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> RainLoop;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> StormLoop;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> WindLoop;
    TWeakObjectPtr<UExponentialHeightFogComponent> Fog;

    /** O tempo da janela (6 h de jogo) para a região. */
    static EVortexWeather Pick(int32 Window, int32 Region, int32 InSeason);
    void Build();
};
