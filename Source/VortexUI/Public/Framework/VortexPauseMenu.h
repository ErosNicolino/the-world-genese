#pragma once

#include "CoreMinimal.h"

class SWidget;
class UVortexGameInstance;

/**
 * Menu de pausa (Esc) no estilo do Vortex: continuar, salvar, voltar ao menu, o código da sala
 * online e as opções que dá para mudar sem sair do jogo (mouse, vídeo, resolução interna, HUD e
 * FPS). Tudo é aplicado e salvo na hora.
 */
TSharedRef<SWidget> MakeVortexPauseMenu(UVortexGameInstance* Owner);
