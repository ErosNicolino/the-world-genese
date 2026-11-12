#pragma once

#include "CoreMinimal.h"

class AVortexCharacter;

/**
 * Janelas de jogador com jogador (UVortexSocial):
 * - menu de interação (E olhando para outro herói a até 3,2 m): grupo, troca, amizade e duelo;
 * - janela de troca: a sua mochila (clique põe 1 na mesa), a sua oferta (clique tira), as moedas
 *   (-10, -1, +1, +10) e a oferta do outro; "Pronto" dos dois fecha a troca. Abre e fecha sozinha
 *   quando a troca começa ou acaba (nos dois).
 */
namespace VortexSocialUi
{
    /** O outro herói para quem a câmera olha, perto (o mais perto do meio da tela, ~22°). */
    VORTEX_API AVortexCharacter* FindLookedAt(const AVortexCharacter* Hero);
    VORTEX_API void OpenMenu(AVortexCharacter* Hero, AVortexCharacter* Other);
    VORTEX_API bool IsOpen();
    VORTEX_API void Close();
    /** A cada quadro (herói local): abre ou fecha a janela de troca conforme o servidor. */
    VORTEX_API void TickTrade(AVortexCharacter* Hero);
}
