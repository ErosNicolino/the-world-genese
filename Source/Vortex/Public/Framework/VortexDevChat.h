#pragma once

#include "CoreMinimal.h"

class AVortexCharacter;

/**
 * Chat de comandos de teste (tecla T, fora do jogo final): uma caixa de texto embaixo à esquerda e as
 * últimas respostas em cima dela. Enter manda, Esc fecha, setas para cima/baixo voltam aos comandos já
 * usados. /help, /fps, /hud, /pos e /clear rodam no próprio PC; o resto vai para o servidor
 * (AVortexCharacter::ServerDevCommand), que responde para o chat de quem pediu.
 */
namespace VortexDevChat
{
    VORTEX_API void Open(AVortexCharacter* Hero);
    VORTEX_API void Close();
    /** A caixa de texto está aberta (o teclado é dela). */
    VORTEX_API bool IsTyping();
    /** Uma linha no registro do chat (aparece por alguns segundos mesmo com a caixa fechada). */
    VORTEX_API void AddLine(const FString& Text, bool bError = false);
    /** Servidor: executa um comando e devolve a resposta (bOutError quando não deu). */
    VORTEX_API FString RunOnServer(AVortexCharacter* Hero, const FString& Line, bool& bOutError);
}
