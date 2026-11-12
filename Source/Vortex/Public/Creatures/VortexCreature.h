#pragma once

#include "UObject/Interface.h"
#include "VortexCreature.generated.h"

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UVortexCreature : public UInterface
{
    GENERATED_BODY()
};

/**
 * O que a HUD, o minimapa, as missões e a ficha (XP e saque) leem de qualquer criatura: o lobo
 * (AVortexWolf) e as outras espécies do Godot (AVortexMonster).
 */
class VORTEX_API IVortexCreature
{
    GENERATED_BODY()
public:
    /** Espécie do Godot ("Wolf", "Slime", "Golem"...): XP, saque e as missões "derrote". */
    virtual FName GetSpecies() const = 0;
    virtual int32 GetCreatureLevel() const = 0;
    /** Nome mostrado ("LOBO DA BRUMA", "ALFA · LOBO DA BRUMA"...). */
    virtual FString GetCreatureTitle() const = 0;
    /** O que ela está fazendo agora (barra do alvo na HUD). */
    virtual FString GetCreatureAction() const = 0;
    virtual float GetHealthFraction() const = 0;
    virtual bool IsCreatureDead() const = 0;
    /** Animais (coelho, cervo, cavalo): não atacam. */
    virtual bool IsPassive() const { return false; }
    /** Chefes: barra grande no topo da tela. */
    virtual bool IsBoss() const { return false; }
    /** Semi-chefe da região ("ANCESTRAL"): maior e mais forte, também com a barra do topo. */
    virtual bool IsElite() const { return false; }
    /** Chefe: a fase da luta (1, 2, 3; 0 = não é chefe ou ainda dorme). */
    virtual int32 GetBossPhase() const { return 0; }
    /** Está brigando com alguém (persegue, ataca ou foi provocada). */
    virtual bool IsEngaged() const = 0;
};
