#pragma once

#include "CoreMinimal.h"

class ACharacter;

/**
 * Desvio de obstáculos das criaturas e NPCs (o terreno é gerado e muda com o streaming: em vez de
 * NavMesh, cada um "sente" o caminho à frente).
 *
 * - Avoid: a direção desejada é testada com uma varredura da cápsula alguns metros à frente; se
 *   bate numa árvore, pedra, parede ou casa, testa ângulos cada vez mais abertos para os dois lados
 *   (preferindo o lado que já vinha usando, para não ficar indeciso) e devolve o primeiro livre.
 * - Separation: um empurrão leve para longe de outras criaturas perto (não empilham).
 * - FStuckWatch: se ficou 1,2 s sem sair do lugar tentando andar, força um desvio para o lado.
 */
namespace VortexSteering
{
    struct FMemory
    {
        int32 Side = 1;            // lado preferido do último desvio
        double SideUntil = 0.0;    // até quando manter o lado
        FVector LastPlace = FVector::ZeroVector;
        float StuckFor = 0.f;
        double ForcedUntil = 0.0;
        FVector Forced = FVector::ZeroVector;
    };

    /** Direção livre mais próxima da desejada (no plano). LookAhead em cm. */
    VORTEX_API FVector Avoid(const ACharacter* Self, const FVector& Desired, float LookAhead, FMemory& Memory);
    /** Empurrão para longe das outras criaturas e NPCs num raio (cm). */
    VORTEX_API FVector Separation(const ACharacter* Self, float Radius);
    /** Chame a cada decisão quando quer andar: devolve true (e a direção em Out) se está preso e deve desviar. */
    VORTEX_API bool Unstick(const ACharacter* Self, const FVector& Desired, float DeltaSeconds, FMemory& Memory, FVector& Out);
}
