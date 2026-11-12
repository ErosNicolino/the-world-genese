#pragma once

#include "CoreMinimal.h"

class AVortexCharacter;

/**
 * A escolha da magia do cajado (02/11): na primeira vez que o herói usa o botão direito com o cajado, abre uma janela
 * com três magias (Cura, Prisão de Vinhas e Bola de Fogo). Só uma fica (UVortexHeroSheet::ServerChooseStaffSpell).
 */
namespace VortexSpellChoice
{
    VORTEX_API void Open(AVortexCharacter* Hero);
    VORTEX_API bool IsOpen();
    /** 03/11: a janela para dar nome ao cavalo recém-domado (UVortexHeroSheet::ServerSetMountName). */
    VORTEX_API void OpenHorseName(AVortexCharacter* Hero);
}
