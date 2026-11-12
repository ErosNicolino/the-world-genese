#include "Framework/VortexViewScales.h"

#include "Framework/VortexPlayerSettings.h"

namespace
{
    const float GrassScales[] = { .5f, .75f, 1.f, 1.5f };
    const float ViewScales[] = { .45f, .65f, .85f, 1.f };
    const float Densities[] = { .35f, .6f, .8f, 1.f };
}

float VortexGraphics::GrassDistanceScale() { return GrassScales[FMath::Clamp(UVortexPlayerSettings::Get()->GrassDistance, 0, 3)]; }
float VortexGraphics::ViewDistanceScale() { return ViewScales[FMath::Clamp(UVortexPlayerSettings::Get()->ViewDistance, 0, 3)]; }
float VortexGraphics::GrassDensity() { return Densities[FMath::Clamp(UVortexPlayerSettings::Get()->GrassDensity, 0, 3)]; }
