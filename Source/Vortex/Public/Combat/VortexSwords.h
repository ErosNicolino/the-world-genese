#pragma once

#include "CoreMinimal.h"
#include "Combat/VortexWeaponKit.h"

class AActor;
class ACharacter;
class UAnimSequence;
class UNiagaraSystem;
class UStaticMesh;
class UVortexWeaponDefinition;

/**
 * As 35 espadas (07/11, das pranchas do usuário, do Normal ao Divino): Data/Rpg/swords.json (Tools/Weapons/BuildSwords.py).
 * Cada uma tem o modelo de blocos (/Game/Vortex/Weapons/Models/W_espada_<chave>_blocos) e o poder do botão direito,
 * um dos secundários das outras armas com os números dela (Kit). A Lâmina do Recruta (a inicial) não tem Kit: fica com
 * o corte à distância. Lido uma vez, do disco.
 * 07/11: e os 35 arcos (Data/Rpg/bows.json, Tools/Weapons/BuildBows.py), do mesmo jeito: o modelo sem corda (a corda é
 * do VortexWeaponParts), a flecha própria de cada um e o poder (com a animação do próprio arco).
 */
namespace VortexSwords
{
    struct FSword
    {
        FString Key;               // a chave do modelo (raro_3): o som é sw_<chave> (espada) ou bw_<chave> (arco)
        FName Weapon;              // o tipo de arma em que ele vale (espada_de_cinzas, arco_de_betula)
        FString Arrow;             // a malha da flecha (só os arcos)
        bool bSelfAnim = false;    // o poder usa a animação da própria arma (os arcos)
        FName Element;             // o elemento (o rastro e a explosão do poder)
        FString Model;             // o nome da malha
        FString PowerName;         // o nome do poder ("Voo da Fênix")
        FString PowerText;         // o que ele faz (a janela das técnicas)
        FName PowerIcon;           // o ícone do poder na barra (special_<arma>)
        bool bHasKit = false;
        VortexWeaponKit::FKit Kit; // o poder (o Special e os Values)
    };

    /** A espada (ou o arco) deste item (o id do item), ou nada. */
    VORTEX_API const FSword* Find(FName Item);
    /** O visual do item: o da tabela; para as variantes do arsenal (a família espada_de_cinzas ou arco_de_betula), um dos
     *  5 modelos da raridade delas. Nada = o modelo da definição da arma. */
    VORTEX_API const FSword* ForVisual(FName Item);
    /** A flecha do arco equipado (se é ele que está na mão), ou nada (a flecha comum). */
    VORTEX_API UStaticMesh* ArrowMesh(const AActor* Owner);
    /** O elemento do poder da arma equipada (NAME_None sem tabela ou sem elemento). */
    VORTEX_API FName PowerElement(const AActor* Owner);
    /** 07/11: o tipo de dano que o poder conta (o da arma: a espada, de perto; o arco, de longe), mesmo quando o poder é de
     *  fogo, água ou raio (o elemento fica no efeito). Sem espada ou arco da tabela na mão, o Kind que veio. */
    VORTEX_API uint8 PowerKind(const AActor* Owner, uint8 Kind);
    /** A explosão do elemento (os efeitos Niagara do projeto: fogo, gelo, raio...; a onda de choque sem elemento). */
    VORTEX_API UNiagaraSystem* ElementBurst(FName Element);
    /** O poder do botão direito: o da espada equipada (se ela tem um próprio), senão o da arma (VortexWeaponKit::Find). */
    VORTEX_API const VortexWeaponKit::FKit* SpecialKit(const AActor* Owner, const UVortexWeaponDefinition* Definition);
    /** A animação do poder (07/11): a do secundário da arma de onde o poder veio (o redemoinho com o giro do machado, o golpe
     *  no chão com o do martelo...); bHold = a de segurar. Nada = a da espada. */
    VORTEX_API UAnimSequence* SpecialClip(const AActor* Owner, const UVortexWeaponDefinition* Definition, bool bHold);
    /** Chamado pela animação em todas as máquinas a cada quadro do especial: no começo de cada um (Serial novo), o rastro do
     *  elemento na lâmina, a explosão do elemento no chão e o som da espada (sw_<chave>). Só as espadas com poder próprio. */
    VORTEX_API void OnSpecialPose(const ACharacter* Hero, int32 Serial);
    /** Ao equipar (07/11): carrega já a animação, o som e os materiais do poder da arma (nada carrega no meio da luta). */
    VORTEX_API void Preload(const ACharacter* Hero);
}
