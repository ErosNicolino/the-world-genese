#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "VortexMonsterAnimInstance.generated.h"

class UAnimSequence;

/**
 * Animação das criaturas do Godot (AVortexMonster): os clipes do Blockbench de cada espécie, com
 * mistura curta entre eles e sem grafo de Blueprint (como o lobo).
 * - Ação (golpe, reação, morte): o clipe que o servidor mandou, uma vez; a morte fica no último quadro.
 * - Sem ação: parado, andando ou correndo pela velocidade (o morcego voa; animais pastam).
 */
UCLASS(Transient)
class VORTEX_API UVortexMonsterAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> Current;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> Previous;
    float CurrentTime = 0.f, PreviousTime = 0.f, BlendAlpha = 1.f;
    bool bCurrentLoops = true, bPreviousLoops = true;
protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
private:
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UAnimSequence>> Clips;
    TWeakObjectPtr<const USkeletalMesh> ClipsMesh;   // a malha para a qual os clipes guardados valem
    FName Species, CurrentName;
    float Rate = 1.f, PreviousRate = 1.f;
    int32 SeenSerial = INDEX_NONE;
    bool bOneShot = false;
    UAnimSequence* Clip(FName Name);
    void Play(FName Name, bool bLoop, float NewRate = 1.f, float Blend = .15f);
    float BlendDuration = .15f;
};

struct FVortexMonsterAnimInstanceProxy : public FAnimInstanceProxy
{
    FVortexMonsterAnimInstanceProxy() = default;
    explicit FVortexMonsterAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
    virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
    virtual bool Evaluate(FPoseContext& Output) override;
private:
    TObjectPtr<UAnimSequence> Current, Previous;
    float CurrentTime = 0.f, PreviousTime = 0.f, Alpha = 1.f;
    bool bCurrentLoops = true, bPreviousLoops = true;
};
