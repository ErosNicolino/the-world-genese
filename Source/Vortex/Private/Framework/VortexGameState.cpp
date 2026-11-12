#include "Framework/VortexGameState.h"

#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
    // Só quem é servidor (jogo local ou anfitrião) muda a hora; o cliente recebe pela rede.
    AVortexGameState* ServerState(UWorld* World)
    {
        AVortexGameState* State = World ? World->GetGameState<AVortexGameState>() : nullptr;
        return State && State->HasAuthority() ? State : nullptr;
    }

    FAutoConsoleCommandWithWorldAndArgs HourCommand(TEXT("Vortex.Hora"),
        TEXT("Hora do mundo (0 a 24; 6 = nascer, 18 = pôr do sol). Sem número, mostra a hora. Ex.: Vortex.Hora 20.5"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
        {
            AVortexGameState* State = World ? World->GetGameState<AVortexGameState>() : nullptr;
            if (!State) return;
            if (Args.Num() > 0)
            {
                if (AVortexGameState* Server = ServerState(World)) Server->SetDayHour(FCString::Atof(*Args[0]));
                else UE_LOG(LogTemp, Warning, TEXT("Vortex.Hora: só o servidor muda a hora"));
            }
            UE_LOG(LogTemp, Display, TEXT("Vortex.Hora = %.2f (dia %d)"), State->GetHour(), State->GetDayNumber());
        }));

    FAutoConsoleCommandWithWorldAndArgs ScaleCommand(TEXT("Vortex.Tempo.Escala"),
        TEXT("Velocidade do relógio do dia (1 = normal, 0 = parado, 60 = um minuto por segundo)."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
        {
            AVortexGameState* Server = ServerState(World);
            if (Server && Args.Num() > 0) Server->SetDayTimeScale(FCString::Atof(*Args[0]));
            if (const AVortexGameState* State = World ? World->GetGameState<AVortexGameState>() : nullptr)
                UE_LOG(LogTemp, Display, TEXT("Vortex.Tempo.Escala = %.1f"), State->GetDayTimeScale());
        }));
}

AVortexGameState::AVortexGameState()
{
    bReplicates = true;
}

double AVortexGameState::TotalAt(double ServerTime) const
{
    return DayClock.ClockAtSync + FMath::Max(0.0, ServerTime - DayClock.SyncServerTime) * DayClock.Scale;
}

float AVortexGameState::ClockAt(double ServerTime) const
{
    const double Total = TotalAt(ServerTime);
    return float(Total - FMath::FloorToDouble(Total / CycleSeconds) * CycleSeconds);
}

int32 AVortexGameState::GetDayNumber() const
{
    return DayClock.DayAtSync + int32(FMath::FloorToDouble(TotalAt(GetServerWorldTimeSeconds()) / CycleSeconds));
}

// 6h no nascer, 18h no pôr do sol (mesma conta do Hour do Godot)
float AVortexGameState::HourOfClock(float Clock)
{
    return Clock < DaySeconds ? 6.f + 12.f * Clock / DaySeconds
        : FMath::Fmod(18.f + 12.f * (Clock - DaySeconds) / NightSeconds, 24.f);
}

float AVortexGameState::ClockOfHour(float Hour)
{
    Hour = FMath::Fmod(FMath::Fmod(Hour, 24.f) + 24.f, 24.f);
    if (Hour >= 6.f && Hour < 18.f) return (Hour - 6.f) / 12.f * DaySeconds;
    const float SinceDusk = FMath::Fmod(Hour - 18.f + 24.f, 24.f);
    return DaySeconds + SinceDusk / 12.f * NightSeconds;
}

void AVortexGameState::SetDayClock(float Clock, int32 Day)
{
    if (!HasAuthority()) return;
    DayClock.ClockAtSync = FMath::Clamp(Clock, 0.f, CycleSeconds - 0.001f);
    DayClock.DayAtSync = FMath::Max(1, Day);
    DayClock.SyncServerTime = GetServerWorldTimeSeconds();
    ForceNetUpdate();
}

void AVortexGameState::SetDayTimeScale(float Scale)
{
    if (!HasAuthority()) return;
    // recomeça a contagem do instante atual para a hora não pular
    const int32 Day = GetDayNumber();
    const float Clock = GetDayClock();
    DayClock.Scale = FMath::Clamp(Scale, 0.f, 1000.f);
    SetDayClock(Clock, Day);
}

void AVortexGameState::SetConnectedPlayers(int32 NewCount)
{
    if (!HasAuthority())
    {
        return;
    }
    ConnectedPlayers = FMath::Max(0, NewCount);
    ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("Vortex framework: server connected players=%d"), ConnectedPlayers);
}

void AVortexGameState::OnRep_ConnectedPlayers()
{
    UE_LOG(LogTemp, Display, TEXT("Vortex framework: client received connected players=%d"), ConnectedPlayers);
}

void AVortexGameState::SetWorldDescriptor(const FGuid& NewWorldId, int32 NewSeed, int32 NewGeneratorVersion)
{
    if (!HasAuthority() || !NewWorldId.IsValid() || NewGeneratorVersion < 1)
    {
        return;
    }
    ActiveWorldId = NewWorldId;
    WorldSeed = NewSeed;
    GeneratorVersion = NewGeneratorVersion;
    ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("Vortex world: server descriptor id=%s seed=%d generator=%d"),
        *ActiveWorldId.ToString(), WorldSeed, GeneratorVersion);
}

void AVortexGameState::OnRep_WorldDescriptor()
{
    if (ActiveWorldId.IsValid() && GeneratorVersion > 0)
    {
        UE_LOG(LogTemp, Display, TEXT("Vortex world: client received descriptor id=%s seed=%d generator=%d"),
            *ActiveWorldId.ToString(), WorldSeed, GeneratorVersion);
    }
}

void AVortexGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AVortexGameState, ConnectedPlayers);
    DOREPLIFETIME(AVortexGameState, TakenForage);
    DOREPLIFETIME(AVortexGameState, TakenForageUntil);
    DOREPLIFETIME(AVortexGameState, DayClock);
    DOREPLIFETIME(AVortexGameState, ActiveWorldId);
    DOREPLIFETIME(AVortexGameState, WorldSeed);
    DOREPLIFETIME(AVortexGameState, GeneratorVersion);
}

void AVortexGameState::MarkForage(FName Id, float Until)
{
    if (!HasAuthority()) return;
    const float Now = GetServerWorldTimeSeconds();
    for (int32 i = TakenForage.Num() - 1; i >= 0; --i)
        if (TakenForage[i] == Id || !TakenForageUntil.IsValidIndex(i) || TakenForageUntil[i] < Now) { TakenForage.RemoveAt(i); if (TakenForageUntil.IsValidIndex(i)) TakenForageUntil.RemoveAt(i); }
    TakenForage.Add(Id);
    TakenForageUntil.Add(Until);
}
