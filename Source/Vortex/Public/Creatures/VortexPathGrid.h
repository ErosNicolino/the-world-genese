#pragma once

#include "CoreMinimal.h"

class ACharacter;

/**
 * Caminho em volta de casas, muros, pedras e barrancos (10/11, IA dos monstros, fase 3). O mundo é gerado e carregado
 * em pedaços, então não há NavMesh: quando a linha reta até o destino está bloqueada, uma grade pequena em volta do
 * monstro (40 x 40 m, células de 1 m) é testada só onde a busca (A*) passa: a cápsula dele cabe ali (sem bater em
 * nada que não seja chão) e o desnível para a célula vizinha dá para subir. O caminho sai já encurtado (pula os
 * pontos que dão para ir em linha reta). Sem terreno carregado (dungeon, mapas de teste) não faz nada.
 */
namespace VortexPathGrid
{
    /** A cápsula de Self, andando em linha reta até To, bate em algo que não é chão nem herói? */
    VORTEX_API bool LineBlocked(const ACharacter* Self, const FVector& To);
    /** Os pontos do caminho de Self até Goal (sem o ponto de partida); false se não achou (ou passou do limite). */
    VORTEX_API bool Find(const ACharacter* Self, const FVector& Goal, TArray<FVector>& OutWaypoints);
}
