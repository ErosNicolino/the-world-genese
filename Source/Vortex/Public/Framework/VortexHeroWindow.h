#pragma once

#include "CoreMinimal.h"

class AVortexCharacter;

/**
 * Janela do herói (I ou Tab), no estilo do menu do Vortex: três abas.
 * - PERSONAGEM: nível e XP, pontos para gastar nos oito atributos (com o que cada um faz), a raça
 *   e os números que valem no combate (vida, mana, ataque, defesa, crítico, regeneração, afinidades).
 * - MOCHILA: equipamento no corpo, 30 espaços com filtros, moedas e o painel do item escolhido
 *   (usar, equipar, desequipar). Clique duplo ou botão direito usa/equipa.
 * - HABILIDADES: as quatro técnicas da arma na mão (nível de desbloqueio, mana, recarga) e os
 *   comandos básicos.
 * Tudo só pede ao servidor (UVortexHeroSheet); a janela se redesenha quando a ficha muda.
 */
namespace VortexHeroWindow
{
    VORTEX_API void Toggle(AVortexCharacter* Hero);
    VORTEX_API void Open(AVortexCharacter* Hero, int32 Page = 0);
    VORTEX_API void Close();
    VORTEX_API bool IsOpen();
    /** Testes de foto: escolhe um item da mochila ou do corpo (INDEX_NONE no outro). */
    VORTEX_API void SelectForTest(int32 BagIndex, int32 EquipSlot);
    /** Testes: gasta um ponto como o botão "+" e diz quantas vezes a janela já foi refeita. */
    VORTEX_API void SpendForTest(int32 Attribute);
    VORTEX_API int32 RebuildsForTest();
}
