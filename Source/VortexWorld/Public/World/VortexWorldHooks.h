#pragma once

#include "CoreMinimal.h"

class UObject;
class UWorld;

/**
 * O que o mundo pede ao jogo (02/11, a separação em módulos): o módulo VortexWorld fica embaixo do Vortex e não pode
 * incluir o herói, o som, o game state nem os recursos do mundo. Ele chama estas funções; o módulo Vortex preenche os
 * Handlers ao abrir (Framework/VortexWorldHookup.cpp). Sem handler, não fazem nada (o editor sem o jogo, testes).
 */
namespace VortexWorldHooks
{
    struct FHandlers
    {
        /** VortexAudio::Play. */
        TFunction<void(const UObject* Context, FName Sound, const FVector& At, float Volume, float PitchJitter)> PlaySound;
        /** O relógio do dia do AVortexGameState: false enquanto não há game state. */
        TFunction<bool(const UWorld* World, float& OutClock, int32& OutDay)> DayClock;
        /** O teste de rede do dia (-VortexDayNightSmoke): "dia=.. hora=.. relogio_T1000=.." do game state. */
        TFunction<FString(const UWorld* World)> DayNightSmokeLine;
        /** AVortexWorldFeatures::AddCampChest / RemoveChest (os baús dos acampamentos e dos pontos de interesse). */
        TFunction<void(const UWorld* World, FName Id, const FVector& Ground, float Yaw, int32 Region, bool bRare, bool bGuarded)> AddCampChest;
        TFunction<void(const UWorld* World, FName Id)> RemoveChest;
        /** A zona da caldeira (UVortexCalderaSubsystem): quanto o céu avermelha onde a câmera está e a fase do chefe. */
        TFunction<void(const UWorld* World, const FVector& Camera, float& OutRed, int32& OutBossPhase)> CalderaSky;
        /** O streamer criou o mundo: o jogo cria os atores dele que dependem do jogo (AVortexWorldFeatures). */
        TFunction<void(UWorld* World)> SpawnGameFeatures;
    };

    VORTEXWORLD_API FHandlers& Handlers();

    inline void PlaySound(const UObject* Context, FName Sound, const FVector& At, float Volume = 1.f, float PitchJitter = .06f)
    {
        if (Handlers().PlaySound) Handlers().PlaySound(Context, Sound, At, Volume, PitchJitter);
    }
    inline bool DayClock(const UWorld* World, float& OutClock, int32& OutDay)
    {
        return Handlers().DayClock ? Handlers().DayClock(World, OutClock, OutDay) : false;
    }
    inline void AddCampChest(const UWorld* World, FName Id, const FVector& Ground, float Yaw, int32 Region, bool bRare, bool bGuarded = true)
    {
        if (Handlers().AddCampChest) Handlers().AddCampChest(World, Id, Ground, Yaw, Region, bRare, bGuarded);
    }
    inline void RemoveChest(const UWorld* World, FName Id)
    {
        if (Handlers().RemoveChest) Handlers().RemoveChest(World, Id);
    }
    inline void CalderaSky(const UWorld* World, const FVector& Camera, float& OutRed, int32& OutBossPhase)
    {
        if (Handlers().CalderaSky) Handlers().CalderaSky(World, Camera, OutRed, OutBossPhase);
    }
    inline void SpawnGameFeatures(UWorld* World)
    {
        if (Handlers().SpawnGameFeatures) Handlers().SpawnGameFeatures(World);
    }
}
