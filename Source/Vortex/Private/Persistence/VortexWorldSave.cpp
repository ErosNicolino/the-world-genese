#include "Persistence/VortexWorldSave.h"

bool UVortexWorldSave::IsSupported() const
{
    return SaveSchemaVersion == CurrentSchemaVersion && WorldId.IsValid() &&
        GeneratorVersion > 0 && !GameVersion.IsEmpty();
}
