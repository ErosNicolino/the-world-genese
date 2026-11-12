#pragma once

#include "CoreMinimal.h"

/**
 * As escalas das opções de gráficos que o terreno usa (02/11: desceram do VortexGraphics para o módulo VortexWorld, que
 * não vê o resto das opções). Lidas do UVortexPlayerSettings.
 */
namespace VortexGraphics
{
    /** O alcance da grama (0,5 a 1,5). */
    VORTEXWORLD_API float GrassDistanceScale();
    /** O alcance das árvores e pedras (a máxima é a de sempre: o chão gerado vai até ~110-126 m; as outras encurtam). */
    VORTEXWORLD_API float ViewDistanceScale();
    /** A densidade da grama. */
    VORTEXWORLD_API float GrassDensity();
}
