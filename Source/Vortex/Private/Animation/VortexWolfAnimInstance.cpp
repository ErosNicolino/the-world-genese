#include "Animation/VortexWolfAnimInstance.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"
#include "Creatures/VortexWolf.h"
#include "GameFramework/CharacterMovementComponent.h"

UAnimSequence* UVortexWolfAnimInstance::Clip(FName Name)
{
    if (TObjectPtr<UAnimSequence>* Existing = Clips.Find(Name)) return *Existing;
    const FString N = Name.ToString();
    const FString Path = FString::Printf(TEXT("/Game/Vortex/Creatures/Wolf/Ametista/A_LoboAmetista_%s.A_LoboAmetista_%s"), *N, *N);
    UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, *Path);
    Clips.Add(Name, Sequence);
    return Sequence;
}

void UVortexWolfAnimInstance::Play(FName Name, bool bLoop, float NewRate)
{
    if (CurrentName == Name) { Rate = NewRate; return; }
    UAnimSequence* Next = Clip(Name);
    if (!Next)
    {
        UE_LOG(LogTemp, Error, TEXT("VORTEX_WOLF_CLIP_MISSING %s"), *Name.ToString());
        return;
    }
    Previous = Current;
    PreviousTime = CurrentTime;
    bPreviousLoops = bCurrentLoops;
    PreviousRate = Rate;
    Current = Next;
    CurrentName = Name;
    CurrentTime = 0.f;
    bCurrentLoops = bLoop;
    Rate = NewRate;
    BlendAlpha = Previous ? 0.f : 1.f;
}

void UVortexWolfAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    for (const FName Name : { FName(TEXT("Idle")), FName(TEXT("Walk")), FName(TEXT("Run")),
        FName(TEXT("StrafeL")), FName(TEXT("StrafeR")),
        FName(TEXT("Attack")), FName(TEXT("Pounce")), FName(TEXT("Howl")),
        FName(TEXT("Hit")), FName(TEXT("Death")), FName(TEXT("Bite2")), FName(TEXT("Rear")), FName(TEXT("IdleSniff")) })
    {
        if (!Clip(Name)) UE_LOG(LogTemp, Error, TEXT("VORTEX_WOLF_CLIP_MISSING %s"), *Name.ToString());
    }
    Play(TEXT("Idle"), true);
}

void UVortexWolfAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    const AVortexWolf* Wolf = Cast<AVortexWolf>(TryGetPawnOwner());
    if (Wolf)
    {
        FName Desired = TEXT("Idle");
        bool bLoop = true;
        switch (Wolf->WolfState)
        {
        case EVortexWolfState::Howl: Desired = TEXT("Howl"); bLoop = false; break;
        case EVortexWolfState::Bite: Desired = TEXT("Attack"); bLoop = false; break;
        case EVortexWolfState::Pounce: Desired = TEXT("Pounce"); bLoop = false; break;
        // 10/11: os especiais (Tools/Blender/RetargetWolf.py)
        case EVortexWolfState::Rear: Desired = TEXT("Rear"); bLoop = false; break;
        case EVortexWolfState::Combo: Desired = TEXT("Bite2"); bLoop = false; break;
        case EVortexWolfState::Hit: Desired = TEXT("Hit"); bLoop = false; break;
        case EVortexWolfState::Dead: Desired = TEXT("Death"); bLoop = false; break;
        default:
            const FVector Velocity = Wolf->GetVelocity();
            const float Speed = Velocity.Size2D();
            // de lado (rondando de frente para o herói): StrafeL/StrafeR
            const float Side = FVector::DotProduct(Velocity, Wolf->GetActorRightVector());
            if (Speed > 35.f && Speed <= 300.f && FMath::Abs(Side) > .6f * Speed)
                Desired = Side > 0.f ? TEXT("StrafeR") : TEXT("StrafeL");
            else
                Desired = Speed > 300.f ? TEXT("Run") : Speed > 35.f ? TEXT("Walk")
                    // 10/11: em casa, sem ninguém, fareja o chão
                    : Wolf->WolfState == EVortexWolfState::Idle && Clip(TEXT("IdleSniff")) ? TEXT("IdleSniff") : TEXT("Idle");
            break;
        }
        if (LastActionSerial != Wolf->ActionSerial || CurrentName != Desired)
        {
            Play(Desired, bLoop);
            LastActionSerial = Wolf->ActionSerial;
        }
        // 02/11: o passo de cada clipe medido na pata no chão (A_LoboAmetista: andar ~160 cm/s, correr ~380 cm/s no
        // tamanho do modelo), vezes a escala do lobo (1,2; o alfa 1,35 a mais). Antes 260 e 520: a animação tocava
        // devagar para a velocidade e o lobo deslizava, "andando parado"
        const float Scale = FMath::Max(.1f, GetSkelMeshComponent()->GetComponentScale().Z);
        const float Speed = Wolf->GetVelocity().Size2D();
        // 10/11: os clipes da Quaternius (Tools/Blender/RetargetWolf.py): andar ~110 cm/s e galope ~265 cm/s medidos na pata
        if (CurrentName == TEXT("Run")) Rate = FMath::Clamp(Speed / (265.f * Scale), .6f, 2.2f);
        else if (CurrentName == TEXT("StrafeL") || CurrentName == TEXT("StrafeR")) Rate = FMath::Clamp(Speed / (160.f * Scale), .6f, 2.f);
        else if (CurrentName == TEXT("Walk")) Rate = FMath::Clamp(Speed / (110.f * Scale), .6f, 2.2f);
    }
    CurrentTime += DeltaSeconds * Rate;
    PreviousTime += DeltaSeconds * PreviousRate;
    BlendAlpha = FMath::Min(1.f, BlendAlpha + DeltaSeconds / BlendDuration);
    if (BlendAlpha >= 1.f) Previous = nullptr;
}

FAnimInstanceProxy* UVortexWolfAnimInstance::CreateAnimInstanceProxy()
{
    return new FVortexWolfAnimInstanceProxy(this);
}

void FVortexWolfAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
    FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
    const UVortexWolfAnimInstance* Instance = CastChecked<UVortexWolfAnimInstance>(InAnimInstance);
    Current = Instance->Current;
    Previous = Instance->Previous;
    CurrentTime = Instance->CurrentTime;
    PreviousTime = Instance->PreviousTime;
    Alpha = Instance->BlendAlpha;
    bCurrentLoops = Instance->bCurrentLoops;
    bPreviousLoops = Instance->bPreviousLoops;
}

bool FVortexWolfAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
    auto Sample = [](UAnimSequence* Sequence, float Time, bool bLoop, FPoseContext& Pose)
    {
        const float Length = Sequence->GetPlayLength();
        const float Position = bLoop && Length > 0.f ? FMath::Fmod(Time, Length) : FMath::Min(Time, Length);
        FAnimationPoseData Data(Pose);
        Sequence->GetAnimationPose(Data, FAnimExtractContext(static_cast<double>(Position), false, FDeltaTimeRecord(), bLoop));
    };
    if (!Current) return false;
    Sample(Current, CurrentTime, bCurrentLoops, Output);
    if (Previous && Alpha < 1.f)
    {
        FPoseContext Old(Output), Blended(Output);
        Sample(Previous, PreviousTime, bPreviousLoops, Old);
        FAnimationPoseData OldData(Old), NewData(Output), BlendedData(Blended);
        FAnimationRuntime::BlendTwoPosesTogether(NewData, OldData, Alpha, BlendedData);
        Output = Blended;
    }
    return true;
}
