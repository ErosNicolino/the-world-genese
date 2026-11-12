#pragma once

#include "CoreMinimal.h"

class APlayerController;

/**
 * A HQ de abertura (Docs/03_Mundo/ABERTURA_QUADRINHOS.md): 5 páginas de 4 quadros contando a história do mundo,
 * com o narrador por cima. Páginas em /Game/Vortex/UI/Abertura/T_Pagina_N, quadros e falas em
 * Data/UI/Abertura/abertura.json (Tools/MakeIntroComic.py), vozes em /Game/Vortex/Audio/Narracao/pN_qM (se houver).
 *
 * Animação: a página chega girando sobre a mesa, os quadros "pintam" um a um, a câmera desliza até cada quadro
 * (os outros escurecem) e se aproxima devagar enquanto a legenda é datilografada numa caixa de HQ; clarão e tremor
 * no impacto da estrela, brilho pulsando na magia; no fim da página, ela vira; no "Acorde", tudo clareia em branco.
 * Só o botão PULAR sai dela (vai para o "Acorde" final); teclas e cliques fora dele não fazem nada.
 */
namespace VortexIntroComic
{
    VORTEX_API void Open(APlayerController* Controller, TFunction<void()> OnClosed = nullptr);
    VORTEX_API void Close();
    VORTEX_API bool IsOpen();
    /** Testes: pula para o quadro (página 0..4, quadro 0..3) e diz onde está ("página quadro tempo"). */
    VORTEX_API void DebugJump(int32 Page, int32 Panel);
    VORTEX_API FString DebugState();
    /** Testes: em que segundo da canção estão a imagem e a música; falso se a música ainda não deu sinal. */
    VORTEX_API bool DebugSync(float& Image, float& Song);
}
