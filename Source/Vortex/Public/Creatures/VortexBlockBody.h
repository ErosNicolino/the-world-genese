#pragma once

#include "CoreMinimal.h"

class ACharacter;
struct FVortexLook;

/**
 * O corpo de blocos do herói num personagem do jogo (os moradores, os viajantes e, desde 11/11, os bandidos): a malha
 * Hero_<raça>, o rosto, o cabelo (com a física e a cor), a roupa (npc_outfit: Roupa_<id> de Tools/Blender/MakeBlockOutfit.py,
 * ou block_outfit do menu) e, se ArmorSet vier, a armadura A_<conjunto>_<head/chest/legs/boots> por cima (a roupa some onde
 * ela cobre). Só o humano tem o corpo de blocos: devolve false nas outras raças.
 */
namespace VortexBlockBody
{
    VORTEX_API bool Apply(ACharacter* Owner, const FVortexLook& Look, FName ArmorSet = NAME_None);
}
