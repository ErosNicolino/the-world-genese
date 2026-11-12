#include "Framework/VortexPings.h"

#include "Engine/World.h"
#include "Framework/VortexAudio.h"

TArray<FVortexPing>& VortexPings::All()
{
    static TArray<FVortexPing> Pings;
    return Pings;
}

void VortexPings::Add(UWorld* World, const FVector& At, AActor* Target, bool bDanger, const FString& Who, const FLinearColor& Color)
{
    if (!World) return;
    TArray<FVortexPing>& Pings = All();
    const float Now = World->GetTimeSeconds();
    Pings.RemoveAll([Now](const FVortexPing& P) { return Now - P.Time > Life || Now < P.Time; });
    Pings.Add({ At, Target, bDanger, Now, Who, Color });
    VortexAudio::Play2D(World, bDanger ? TEXT("bat_screech") : TEXT("dialogue_blip"), bDanger ? .45f : .8f);
}
