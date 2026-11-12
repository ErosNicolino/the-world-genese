#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "VortexWolfAnimInstance.generated.h"

class UAnimSequence;

/** Eight original wolf clips, with short locomotion/attack crossfades and no Blueprint graph. */
UCLASS(Transient)
class VORTEX_API UVortexWolfAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> Current;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> Previous;
    float CurrentTime = 0.f, PreviousTime = 0.f, BlendAlpha = 1.f;
    bool bCurrentLoops = true, bPreviousLoops = true;
protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
private:
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UAnimSequence>> Clips;
    FName CurrentName;
    float Rate = 1.f, PreviousRate = 1.f, BlendDuration = .12f;
    int32 LastActionSerial = INDEX_NONE;
    UAnimSequence* Clip(FName Name);
    void Play(FName Name, bool bLoop, float NewRate = 1.f);
};

struct FVortexWolfAnimInstanceProxy : public FAnimInstanceProxy
{
    FVortexWolfAnimInstanceProxy() = default;
    explicit FVortexWolfAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
    virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
    virtual bool Evaluate(FPoseContext& Output) override;
private:
    TObjectPtr<UAnimSequence> Current, Previous;
    float CurrentTime = 0.f, PreviousTime = 0.f, Alpha = 1.f;
    bool bCurrentLoops = true, bPreviousLoops = true;
};
