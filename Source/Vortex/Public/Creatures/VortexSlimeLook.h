#pragma once

#include "Components/ActorComponent.h"
#include "VortexSlimeLook.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;
class AVortexMonster;

/**
 * O corpo do slime (Tools/Blender/BuildSlime.py + Tools/ImportSlime.py), só desenho, em cada máquina:
 * uma gota de gelatina que o material deforma (VortexSlime: os dados primitivos do componente).
 * - anda aos pulinhos: estica no ar, amassa ao cair e balança depois;
 * - parado respira (incha e desincha devagar);
 * - no ataque (Tackle) estica um braço de gelatina até o alvo e volta tremendo; no salto (Leap) se encolhe
 *   antes e se estica no ar;
 * - pisca de vez em quando (os olhos achatam);
 * - apanhando treme; morto derrete e some.
 * Os tipos (Slime, SlimeFire, SlimePoison, SlimeIron) mudam as cores, o brilho, as faíscas (fogo) e as bolhas roxas (veneno).
 */
UCLASS(ClassGroup=(Vortex))
class VORTEX_API UVortexSlimeLook : public UActorComponent
{
    GENERATED_BODY()
public:
    UVortexSlimeLook();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    /** Cria o corpo do tipo (a espécie do monstro) sob a cápsula. HalfHeight: meia altura da cápsula. */
    void Build(AVortexMonster* Monster, float HalfHeight, float Scale);
    /** "Normal", "Fogo", "Veneno" ou "Ferro" pela espécie. */
    static const TCHAR* KindOf(FName Species);

private:
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Aura;
    UPROPERTY(Transient) TObjectPtr<class UInstancedStaticMeshComponent> Bubbles;
    // o rosto é desenhado na própria gelatina (Tools/ImportSlime.py, FACE); estes são os humores que vão para o material
    float Mood = 1.f, MouthOpen = 0.f, Tongue = 0.f, Angry = 0.f, Squint = 0.f, HitFace = 0.f, NextBleh = 0.f, BlehUntil = -1.f;
    void TickFace(class AVortexMonster* Monster, float Dt, float Now, float Speed, bool bAir);
    float BodyHeight = 70.f;
    float Hop = 0.f, Squash = 0.f, SquashSpeed = 0.f, Stretch = 0.f, StretchSpeed = 0.f, Wobble = 0.f, Melt = 0.f;
    int32 LastSerial = -1;
    float ActAge = 10.f;
    FName ActClip;
    bool bWasAir = false;
    float NextBlink = 0.f, BlinkStart = -10.f;
    FVector BaseLocation = FVector::ZeroVector;
    // 03/11: a inércia (o alto do corpo fica para trás e balança) e o tempo próprio de cada gota
    FVector2D Lean = FVector2D::ZeroVector, LeanSpeed = FVector2D::ZeroVector, LastVelocity = FVector2D::ZeroVector;
    float Seed = -1.f;
};
