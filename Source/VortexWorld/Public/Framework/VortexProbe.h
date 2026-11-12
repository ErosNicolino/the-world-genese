#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformTime.h"

/**
 * Cronômetro por sistema na thread do jogo (para achar o que trava): cada trecho soma o tempo total,
 * o pior e quantas vezes passou de 8 ms. O passeio de desempenho (-VortexRoamBench) escreve o resumo.
 */
namespace VortexProbe
{
    struct FEntry { double TotalMs = 0.0, MaxMs = 0.0; int32 Calls = 0, Over8 = 0; };
    inline TMap<FString, FEntry>& Entries() { static TMap<FString, FEntry> Map; return Map; }
    inline void Add(const TCHAR* Name, double Ms)
    {
        FEntry& E = Entries().FindOrAdd(Name);
        E.TotalMs += Ms; E.MaxMs = FMath::Max(E.MaxMs, Ms); ++E.Calls; E.Over8 += Ms > 8.0 ? 1 : 0;
    }
    struct FScope
    {
        const TCHAR* Name; double Start;
        explicit FScope(const TCHAR* InName) : Name(InName), Start(FPlatformTime::Seconds()) {}
        ~FScope() { Add(Name, (FPlatformTime::Seconds() - Start) * 1000.0); }
    };
}

#define VORTEX_PROBE_JOIN2(A, B) A##B
#define VORTEX_PROBE_JOIN(A, B) VORTEX_PROBE_JOIN2(A, B)
#define VORTEX_PROBE(Name) VortexProbe::FScope VORTEX_PROBE_JOIN(VortexProbeScope_, __LINE__)(TEXT(Name))
