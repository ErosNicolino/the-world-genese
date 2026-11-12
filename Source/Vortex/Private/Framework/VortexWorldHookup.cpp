// O jogo preenche o que o módulo VortexWorld pede (02/11, a separação em módulos: World/VortexWorldHooks.h): o som, o
// relógio do dia, os baús, o céu da caldeira e os recursos do mundo. Registrado ao carregar o módulo Vortex.
#include "World/VortexWorldHooks.h"

#include "Engine/World.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexGameState.h"
#include "World/VortexCaldera.h"
#include "World/VortexWorldFeatures.h"

namespace
{
    struct FVortexWorldHookup
    {
        FVortexWorldHookup()
        {
            VortexWorldHooks::FHandlers& H = VortexWorldHooks::Handlers();
            H.PlaySound = [](const UObject* Context, FName Sound, const FVector& At, float Volume, float PitchJitter)
            {
                VortexAudio::Play(Context, Sound, At, Volume, PitchJitter);
            };
            H.DayClock = [](const UWorld* World, float& OutClock, int32& OutDay)
            {
                const AVortexGameState* State = World ? World->GetGameState<AVortexGameState>() : nullptr;
                if (!State) return false;
                OutClock = State->GetDayClock();
                OutDay = State->GetDayNumber();
                return true;
            };
            H.DayNightSmokeLine = [](const UWorld* World)
            {
                const AVortexGameState* State = World ? World->GetGameState<AVortexGameState>() : nullptr;
                return State ? FString::Printf(TEXT("dia=%d hora=%.2f relogio_T1000=%.3f"), State->GetDayNumber(), State->GetHour(), State->ClockAt(1000.0))
                    : FString();
            };
            H.AddCampChest = [](const UWorld* World, FName Id, const FVector& Ground, float Yaw, int32 Region, bool bRare, bool bGuarded)
            {
                if (AVortexWorldFeatures* Features = AVortexWorldFeatures::Find(World)) Features->AddCampChest(Id, Ground, Yaw, Region, bRare, bGuarded);
            };
            H.RemoveChest = [](const UWorld* World, FName Id)
            {
                if (AVortexWorldFeatures* Features = AVortexWorldFeatures::Find(World)) Features->RemoveChest(Id);
            };
            H.CalderaSky = [](const UWorld* World, const FVector& Camera, float& OutRed, int32& OutBossPhase)
            {
                if (const UVortexCalderaSubsystem* Caldera = World ? World->GetSubsystem<UVortexCalderaSubsystem>() : nullptr)
                {
                    OutRed = Caldera->RedSkyAt(Camera);
                    OutBossPhase = Caldera->GetBossPhase();
                }
            };
            H.SpawnGameFeatures = [](UWorld* World)
            {
                FActorSpawnParameters Params;
                Params.ObjectFlags |= RF_Transient;
                World->SpawnActor<AVortexWorldFeatures>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
            };
        }
    };
    FVortexWorldHookup GVortexWorldHookup;
}
