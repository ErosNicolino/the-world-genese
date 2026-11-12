#pragma once

#include "CoreMinimal.h"

class AActor;
class UVortexHeroSheet;

/**
 * As 21 armaduras das pranchas do usuário (07/11): Data/Rpg/armors.json (Tools/Armor/BuildArmors.py), 84 peças.
 *
 * O visual: cada peça é uma malha de blocos no esqueleto do herói (Tools/Blender/MakeBlockArmor.py) que troca a parte
 * do traje que ela cobre (AVortexCharacter::ApplyBlockArmor); o capacete troca o cabelo.
 *
 * As passivas: cada peça tem uma e o conjunto inteiro (4 peças) mais uma. Os tipos são efeitos daqui (o bloqueio, os
 * espinhos, o gelo em quem bate, a regeneração, o renascer...), com os números do armors.json: ajustar não compila.
 * Peças com o mesmo tipo somam; os tempos de espera usam o menor. Tudo no servidor, menos a velocidade e a esquiva
 * (os dois lados calculam igual: a ficha vai pela rede).
 */
namespace VortexArmor
{
    struct FPiece
    {
        FName Set, Slot;
        FString Mesh;          // A_<conjunto>_<peça> (/Game/Vortex/Characters/Heroes/humano/Armaduras)
        bool bFringe = false;  // capuz ou gorro: a franja na cor do cabelo
    };

    /** A peça deste item, ou nada (as armaduras antigas não têm malha). */
    VORTEX_API const FPiece* Piece(FName Item);
    /** O visual do item: o da peça nova, ou o do conjunto novo parecido para as armaduras antigas ("visual" no json). */
    VORTEX_API const FPiece* VisualPiece(FName Item);

    /** A soma de um valor de um tipo de passiva nas peças vestidas e no conjunto completo. */
    VORTEX_API float Sum(const UVortexHeroSheet* Sheet, FName Kind, FName Key);
    VORTEX_API bool Has(const UVortexHeroSheet* Sheet, FName Kind);

    /** O herói apanhou de uma criatura (o dano já com a defesa): bloqueio, guarda alta, última defesa, espinhos, gelo,
     *  brasa e lava em quem bateu, e o renascer. Devolve o dano que fica. */
    VORTEX_API float Incoming(UVortexHeroSheet* Target, AActor* Source, float Damage);
    /** O multiplicador do golpe do herói (feras, vida cheia). */
    VORTEX_API float OutgoingScale(const UVortexHeroSheet* Source, const AActor* Target);
    /** Esquivou há pouco (o golpe oportuno): o próximo golpe sai crítico, uma vez. */
    VORTEX_API bool ConsumeCrit(AActor* Hero);
    /** O herói acertou (veneno, brasa, roubo de vida) ou venceu (a cura da vitória). */
    VORTEX_API void OnDealt(AActor* Hero, AActor* Target, float Damage, bool bKilled);
    /** Começou uma esquiva (servidor). */
    VORTEX_API void OnDodge(AActor* Hero);
    /** A cada quadro, no servidor: a regeneração, a mana, o abrigo e a chuva de estrelas. */
    VORTEX_API void Tick(UVortexHeroSheet* Sheet, float DeltaSeconds);

    /** A velocidade de andar (1 = normal) e a espera da esquiva (1 = normal). */
    VORTEX_API float MoveScale(const UVortexHeroSheet* Sheet);
    VORTEX_API float DodgeCooldownScale(const AActor* Hero);
}
