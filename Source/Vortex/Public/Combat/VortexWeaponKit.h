#pragma once

#include "CoreMinimal.h"

struct FVortexItemDef;

/**
 * Os números de cada arma da revisão (Docs/02_Combate/REVISAO_ARMAS.md), num lugar só:
 * mana do golpe normal e do secundário, recarga, de onde sai o tiro e qual é o secundário novo.
 * O resto (golpes, clipes, dano base) continua no DA_<tipo> de cada arma.
 */
namespace VortexWeaponKit
{
    /** De onde sai o que a arma dispara. */
    enum class EMuzzle : uint8
    {
        Tip,        // ponta da arma (varinha, cajado, pistola, besta, orbe...)
        Grip,       // a mão (arco: a flecha sai da corda junto da mão; garrafa e chakram: arremessados)
        Body,       // o meio da arma (as notas saem do corpo do instrumento)
    };

    struct FKit
    {
        FName Family;                 // família da animação (Lute, Orb, Book, Wand...)
        float PrimaryMana = 0.f;      // por golpe normal
        float SpecialMana = 0.f;      // o secundário (por segundo, no canto do instrumento)
        float SpecialCooldown = 0.f;  // s
        FName Special;                // secundário novo (WeaponSpecials.cpp)
        EMuzzle Muzzle = EMuzzle::Tip;
        float GuardKeep = .35f;       // fração do dano de frente que passa defendendo (espada e escudo: .2)
        TMap<FName, float> Values;    // números do secundário (cm, s, multiplicadores do dano do golpe normal)
        float Value(FName Key, float Default = 0.f) const { const float* V = Values.Find(Key); return V ? *V : Default; }
    };

    /** Primeira pessoa: onde fica na tela (cm, espaço da câmera) a mão que segura a arma de cada família, e qual mão. */
    VORTEX_API FVector ViewAnchor(FName Family, FName& OutBone);
    /** Cor do rastro do golpe de cada família (G3). */
    VORTEX_API FLinearColor TrailColor(FName Family);
    /** O item de arma que o herói tem na mão (o da mochila: raridade, elemento, peculiaridade). */
    VORTEX_API const FVortexItemDef* HeldItem(const AActor* Hero);
    /** Elemento da arma na mão (fire, water, lightning...), ou nada. */
    VORTEX_API FName HeldElement(const AActor* Hero);
    /** Servidor: o acerto de quem tem uma arma com elemento ou peculiaridade (queimadura, lentidão,
     *  raio em cadeia, veneno, vinhas, sangramento, peso...). Damage = o dano que entrou. */
    VORTEX_API void OnWeaponHit(AActor* Hero, AActor* Target, float Damage);
    /** Kit da arma (nada para a espada e a katana, que não mudaram). */
    VORTEX_API const FKit* Find(FName WeaponId);
    VORTEX_API float PrimaryDamageScale(FName WeaponId);
}
