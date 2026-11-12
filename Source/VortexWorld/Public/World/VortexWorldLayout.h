#pragma once

#include "CoreMinimal.h"

/**
 * Regras do mapa que o mundo e o jogo usam juntos (02/11: desceram do jogo para o módulo VortexWorld): o relógio do dia,
 * a cratera do Azhûr e onde ficam os acampamentos de bandidos. O AVortexGameState, o UVortexCalderaSubsystem e o
 * UVortexCreatureSubsystem usam estas (os números são os mesmos de antes).
 */
namespace VortexWorldLayout
{
    // o dia: 30 min de dia e 8 de noite
    constexpr float DaySeconds = 30.f * 60.f;
    constexpr float NightSeconds = 8.f * 60.f;
    constexpr float CycleSeconds = DaySeconds + NightSeconds;

    // a cratera do Azhûr
    constexpr float CalderaCenterX = 17000.f;
    constexpr float CalderaCenterY = 11000.f;
    constexpr float CalderaClearRadius = 5700.f;
    /** Dentro da cratera (sem árvore, grama nem pedrinha do terreno: o chão é o de basalto dela). */
    inline bool CalderaClearsVegetation(float X, float Y)
    {
        return FVector2D::DistSquared(FVector2D(X, Y), FVector2D(CalderaCenterX, CalderaCenterY)) < CalderaClearRadius * CalderaClearRadius;
    }

    // os acampamentos de bandidos: um sorteio por quadra
    constexpr float BanditCellSize = 7200.f;
    /** O acampamento da quadra (como o Godot, ValleyPopulation.BanditCampAt), se ela tiver. */
    VORTEXWORLD_API bool BanditCampAt(const FIntPoint& Cell, FVector2D& OutAt);
}
