#include "Persistence/VortexPlayerSave.h"

bool UVortexPlayerSave::IsSupported() const
{
    // Schema 1 profiles from earlier framework tests remain readable.
    return SaveSchemaVersion >= 1 && SaveSchemaVersion <= CurrentSchemaVersion && Identity.IsValid() && !GameVersion.IsEmpty();
}
