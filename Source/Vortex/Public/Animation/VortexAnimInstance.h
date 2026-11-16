#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Framework/VortexLook.h"
#include "VortexAnimInstance.generated.h"

class UAnimSequence;
class USkeletalMesh;

/** 13/11: a camada de vida, já em números finais para a pose (TickLife calcula, ApplyLife aplica; ajustes em
 *  Data/Characters/vida.json): respiração, troca de peso, olhar, alerta e o passo do golpe. */
struct FVortexLifePose
{
    float BreathDeg = 0.f, BreathLift = 0.f;      // o peito sobe e desce
    float SwayCm = 0.f, SwayDeg = 0.f;            // o peso troca de perna (e o quadril parado de lado, o jeito do corpo)
    float LookYaw = 0.f, LookPitch = 0.f;         // a cabeça segue quem está perto (ou dá olhadas)
    float StepCm = 0.f, CrouchCm = 0.f;           // o passo à frente do golpe; a postura de alerta
    float SpineDeg = 0.f, ChinDeg = 0.f;
};

/**
 * Animação do aventureiro feita em C++ (sem grafo de Animation Blueprint): a cada quadro escolhe o
 * clipe pelo estado do movimento (parado, andar, correr, pulo, esquiva, nado) e mistura do clipe
 * anterior para o novo em BlendTime. Os clipes vêm do aventureiro do Godot convertido
 * (Tools/ConvertGodotAdventurer.py) e o ritmo de andar e correr segue o do Godot
 * (AdventurerActor.Animate: andar calibrado em 2,4 m/s, correr em 6,2 m/s).
 */
UCLASS(Transient)
class VORTEX_API UVortexAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    // Estado lido pelo proxy (thread de animação).
    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> Current;
    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> Previous;
    float CurrentTime = 0.f, PreviousTime = 0.f;
    bool bCurrentLoops = true, bPreviousLoops = true;
    float BlendAlpha = 1.f;          // 1 = só o atual
    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> UpperBodyOverlay;
    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> PreviousUpperBody;
    float UpperBodyTime = 0.f, UpperBodyAlpha = 0.f;
    float PreviousUpperBodyTime = 0.f, UpperBodyCrossfadeAlpha = 1.f;
    bool bUpperBodyLoops = false;
    bool bPreviousUpperBodyLoops = false;

    /** Pose de segurar da família (clipe *_Hold): o tronco e os braços seguram a arma por cima de
     *  parado, andar e correr (as pernas continuam as do movimento). */
    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> HoldPose;
    float HoldTime = 0.f, HoldAlpha = 0.f;
    /** O golpe na mão mexe o corpo inteiro (clipes *_Full: giros, investidas, pancadas no chão). */
    bool bUpperBodyFull = false;
    /** Bebendo uma poção: o braço esquerdo e a cabeça (a arma continua na direita). */
    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> DrinkPose;
    float DrinkTime = 0.f, DrinkAlpha = 0.f;
    /** Arma na mão andando e correndo (heróis do Mixamo): o braço da arma vai para a pose de segurar do parado (as
     *  corridas do Mixamo são sem arma e balançavam a arma para todo lado). bSteadyLeft: o arco, na mão esquerda. */
    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> SteadyPose;
    float SteadyAlpha = 0.f;
    bool bSteadyLeft = false;

    FName CurrentClip() const { return CurrentName; }
    /** A taxa em que o clipe atual toca (testes: o passo casa com o chão). */
    float CurrentRate() const { return Rate; }
    /** O herói tem este clipe (emotes e danças: só os heróis com clipes do Mixamo). */
    bool HasClip(FName Name) { return Clip(Name) != nullptr; }
    /** Duração do clipe em segundos (0 se não tem). */
    float ClipLength(FName Name);

    // proporção da raça (pernas, tronco, braços, cabeça), aplicada na pose de cada quadro
    FVortexBodyShape Body;
    /** Primeira pessoa (só na máquina de quem joga): a cabeça encolhe a zero para a câmera não ficar dentro dela. */
    bool bHideHead = false;
    /** Primeira pessoa com arco (só na máquina de quem joga): os braços do corpo somem, a tela mostra os da cópia. */
    bool bHideArms = false;
    /** Primeira pessoa (corpo-cópia preso à câmera): só os antebraços, as mãos e a arma aparecem; tronco,
     *  cabeça e pernas encolhem a zero (os braços continuam no mesmo lugar). */
    bool bArmsOnly = false;
    /** 0..1: correndo com Shift o tronco inclina para a frente (andar e correr pareciam a mesma animação). */
    float Lean = 0.f;
    FVortexLifePose Life;
#if !UE_BUILD_SHIPPING
    /** Testes (VortexHeroShots): força a inclinação da corrida para a foto. */
    static inline bool bDebugLean = false;
    /** Testes (VortexHeroShots): a velocidade fingida para escolher a animação parado no lugar (0 = a de verdade). */
    static inline float DebugSpeed = 0.f;
#endif

    static constexpr float BlendTime = 0.15f;

protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

private:
    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<UAnimSequence>> Clips;
    TWeakObjectPtr<const USkeletalMesh> ClipsMesh;
    FName CurrentName;
    float Rate = 1.f, PreviousRate = 1.f, Blend = BlendTime, StateTime = 0.f;
    bool bWasFalling = false;
    float LandTime = 0.f;
    FName UpperBodyName;

    UAnimSequence* Clip(FName Name);
    /** O clipe é do mesmo esqueleto da malha (os do aventureiro não servem no herói do Mixamo). */
    bool Fits(const UAnimSequence* Sequence) const;
    /** O golpe Index da arma feito no Mixamo para a raça (-1: o puxar do arco), ou nada (fica o do aventureiro). */
    UAnimSequence* RaceAttack(const class UVortexWeaponDefinition* Definition, int32 Index);
    float IdleFor = 0.f;
    uint8 NpcGestureSeq = 0;   // 08/11: o último gesto do morador já tocado
    float NpcGestureLeft = 0.f;
    float LastHealth = -1.f, HitAge = 99.f, DrawAge = 99.f;
    // a camada de vida
    float LifeClock = 0.f, LifeIdle = 0.f, LifeAlert = 0.f, BreathPhase = 0.f, SwayPhase = 0.f, LifeScan = 0.f, GlanceIn = 4.f, GlanceLeft = 0.f;
    float StepAge = 99.f, StepSize = 0.f;
    FVector2D LookGoal = FVector2D::ZeroVector, Glance = FVector2D::ZeroVector;
    bool bLifeFoe = false;
    int32 LifeSerial = 0;
    FName IdleGesture;            // o gesto de quem está parado há um tempo (Vida_*, ArmStretching)
    float IdleGestureAt = 10.f;
    void TickLife(float DeltaSeconds, const class ACharacter* Owner, float Speed, bool bBusy);
    TWeakObjectPtr<const class UVortexWeaponDefinition> LastDefinition;
    void Play(FName Name, bool bLoop, float NewRate, float InBlend = BlendTime, UAnimSequence* Override = nullptr);
};

struct FVortexAnimInstanceProxy : public FAnimInstanceProxy
{
    FVortexAnimInstanceProxy() = default;
    explicit FVortexAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

    virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
    virtual bool Evaluate(FPoseContext& Output) override;

private:
    TObjectPtr<UAnimSequence> Current, Previous;
    float CurrentTime = 0.f, PreviousTime = 0.f, Alpha = 1.f;
    bool bCurrentLoops = true, bPreviousLoops = true;
    TObjectPtr<UAnimSequence> UpperBody;
    TObjectPtr<UAnimSequence> PreviousUpperBody;
    TObjectPtr<UAnimSequence> HoldPose;
    float HoldTime = 0.f, HoldAlpha = 0.f;
    bool bUpperBodyFull = false;
    TObjectPtr<UAnimSequence> DrinkPose;
    float DrinkTime = 0.f, DrinkAlpha = 0.f;
    TObjectPtr<UAnimSequence> SteadyPose;
    float SteadyAlpha = 0.f;
    bool bSteadyLeft = false;
    float UpperBodyTime = 0.f, UpperBodyAlpha = 0.f;
    float PreviousUpperBodyTime = 0.f, UpperBodyCrossfadeAlpha = 1.f;
    bool bUpperBodyLoops = false;
    bool bPreviousUpperBodyLoops = false;
    FVortexBodyShape Body;
    bool bHideHead = false;
    bool bHideArms = false;
    bool bArmsOnly = false;
    float Lean = 0.f;
    FVortexLifePose Life;
    void ApplyLife(FPoseContext& Output) const;
    void ApplyBody(FPoseContext& Output) const;
    /** Braços abertos (Body.ArmSpread) e a inclinação da corrida (Lean), girando no espaço do componente. */
    void ApplyPosture(FPoseContext& Output) const;
    void KeepArmsOnly(FPoseContext& Output) const;
};
