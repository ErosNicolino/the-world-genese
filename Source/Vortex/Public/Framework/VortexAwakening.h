#pragma once

#include "CoreMinimal.h"

class AVortexCharacter;

/**
 * O Despertar (nível 10): a cena das seis cartas.
 * As cartas (de verso) voam para a tela e viram uma de cada vez, da mais comum para a mais rara: a
 * comum é de bronze, a rara de prata, a épica de ouro e a lendária de diamante (essa ainda treme antes,
 * clareia a tela e fica com raios de luz atrás). Depois, clicar numa carta mostra a classe inteira
 * (papel, passiva, bônus, armas e as 4 habilidades) e "Despertar como ..." escolhe (para sempre).
 * As cartas saem pela arma do Despertar e pelos pontos que o herói investiu (FVortexRpgCatalog::ClassWeight).
 * Esc fecha sem escolher: a aba Classes da janela do herói abre a cena de novo (já viradas).
 */
namespace VortexAwakening
{
    VORTEX_API void Open(AVortexCharacter* Hero, bool bAlreadyRevealed = false);
    VORTEX_API void Close();
    VORTEX_API bool IsOpen();
    /** Testes: escolhe a carta (índice) como se clicasse, confirma e diz qual é a mais rara. */
    VORTEX_API void DebugPick(int32 Index);
    VORTEX_API void DebugConfirm();
    VORTEX_API int32 DebugRarest();
}
