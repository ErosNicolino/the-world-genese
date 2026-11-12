#pragma once

#include "CoreMinimal.h"

class AVortexCharacter;
class AVortexNpc;

/**
 * Conversa com um NPC (E perto dele), na parte de baixo da tela: quem fala, a fala (escrita aos
 * poucos) e as escolhas. As falas vêm do diálogo do Godot (Data/Story/story.json); as missões do NPC
 * entram como escolhas, como no Godot:
 * - "[Missão] Título": a oferta, com Aceitar e Agora não;
 * - "Sobre: Título": o que falta (com os objetivos);
 * - "[Entregar] Título": a fala de conclusão e a recompensa.
 * Ações das falas (presentes, marcas) e o "falar com" das missões são pedidos ao servidor.
 * 1-9 escolhem, Espaço/Enter continua, Esc fecha.
 */
namespace VortexDialogue
{
    VORTEX_API void Open(AVortexCharacter* Hero, AVortexNpc* Npc);
    VORTEX_API void Close();
    VORTEX_API bool IsOpen();
    /** O NPC mais perto do herói, à frente e a até 3,5 m (E conversa com ele). */
    VORTEX_API AVortexNpc* FindTalkable(const AVortexCharacter* Hero);
}
