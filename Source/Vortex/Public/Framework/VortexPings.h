#pragma once

#include "CoreMinimal.h"

/** Marcação do Z (de qualquer jogador): um lugar, ou uma criatura ("perigo"). O HUD (VortexUI) desenha. */
struct FVortexPing
{
    FVector At = FVector::ZeroVector;
    TWeakObjectPtr<AActor> Target;
    bool bDanger = false;
    float Time = 0.f;
    FString Who;
    FLinearColor Color = FLinearColor(.35f, .9f, 1.f);   // 03/11: a cor de quem marcou (UVortexSocial::GetPlayerColor)
};
namespace VortexPings
{
    VORTEX_API TArray<FVortexPing>& All();
    VORTEX_API void Add(UWorld* World, const FVector& At, AActor* Target, bool bDanger, const FString& Who,
        const FLinearColor& Color = FLinearColor(.35f, .9f, 1.f));
    constexpr float Life = 8.f;
}
