#pragma once

#include "GameFramework/Actor.h"
#include "VortexHitFx.generated.h"

class UInstancedStaticMeshComponent;
class UNiagaraSystem;

/**
 * Sensação de acerto, em cada máquina (chamada pelo aviso de acerto confirmado do combate):
 * - o corpo acertado brilha um instante (material "overlay" M_HitFlash; mais forte no crítico);
 * - pausa de impacto: quem bate e quem apanha param por ~70 ms (só a animação deles);
 * - a câmera de quem bate (e de quem apanha) dá um tranco;
 * - respingos de cubinhos: sangue nos bichos e heróis, palha nos bonecos de treino.
 * O empurrão do alvo é do servidor (UVortexCombatComponent::ApplyHit).
 */
namespace VortexHitFx
{
    VORTEX_API void Play(AActor* Source, AActor* Target, float Damage, bool bCrit);
    /** Aparo perfeito: faíscas douradas entre os dois e, se atordoou, estrelinhas girando sobre quem foi aparado. */
    VORTEX_API void Parry(AActor* Attacker, const FVector& Between, bool bStunned);
    /** Respingo de cubinhos num ponto (pedra caindo, veneno, poeira). */
    VORTEX_API void Splash(UWorld* World, const FVector& At, const FLinearColor& Color, int32 Count);
    /** Onda em anel na superfície da água (ou poça), que abre e some em Life segundos. */
    VORTEX_API void Ripple(UWorld* World, const FVector& At, float Size, float Life = .9f);
    /** Mergulho: gotas que sobem em coroa, o anel na água e o som (Strength 0..1 pela velocidade da queda). */
    VORTEX_API void WaterSplash(UWorld* World, const FVector& Surface, float Strength);
    /** Rastro do golpe de perto: um arco de energia que varre na frente do herói e some (Delay: até a lâmina passar). */
    VORTEX_API void SwingTrail(AActor* Owner, float Reach, int32 SwingIndex, float Delay, const FLinearColor& Tint = FLinearColor::White);
    /** Pancada no chão: onda de choque e poeira na frente do herói. */
    VORTEX_API void GroundSlam(AActor* Owner, float Radius, float Delay);
    /** Estrelinhas de atordoado sobre a cabeça, por tantos segundos. */
    VORTEX_API void StunStars(AActor* Target, float Seconds);
    /** Efeito Niagara carregado uma vez e mantido na memória (sem recarregar e recompilar a cada uso). */
    VORTEX_API UNiagaraSystem* System(const TCHAR* Path);
    /** Aquece os efeitos dos golpes e poderes ao entrar no mundo (ver AVortexFxWarmup). */
    VORTEX_API void Warmup(UWorld* World);
}

/**
 * Aquecimento: no editor, o primeiro uso de um efeito (Niagara) ou de um material num tipo de malha
 * novo carrega o arquivo e prepara os shaders na hora, e o primeiro poder dava uma travada grande.
 * Ao entrar no mundo, este ator mostra por 1,5 s, minúsculos na frente da câmera, todos os efeitos dos
 * golpes, projéteis, corte à distância e acerto, e some.
 */
UCLASS()
class VORTEX_API AVortexFxWarmup : public AActor
{
    GENERATED_BODY()
public:
    AVortexFxWarmup();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    float Age = 0.f;
};

/** Estrelinhas de atordoado: quatro cubinhos dourados girando sobre a cabeça. */
UCLASS()
class VORTEX_API AVortexStunStars : public AActor
{
    GENERATED_BODY()
public:
    AVortexStunStars();
    virtual void Tick(float DeltaSeconds) override;
    void Start(AActor* Target, float Seconds);
private:
    UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Stars;
    TWeakObjectPtr<AActor> Over;
    float Age = 0.f, Life = 1.f, Height = 120.f;
};

/** Rastro do golpe: fita em arco presa ao herói, que gira na direção do golpe e se apaga. */
UCLASS()
class VORTEX_API AVortexSwingTrail : public AActor
{
    GENERATED_BODY()
public:
    AVortexSwingTrail();
    virtual void Tick(float DeltaSeconds) override;
    void Start(AActor* InOwner, float Reach, float Tilt, bool bReverse, const FLinearColor& Tint);
private:
    UPROPERTY(Transient) TObjectPtr<class UProceduralMeshComponent> Arc;
    UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> Look;
    TWeakObjectPtr<AActor> Follow;
    float Age = 0.f, Tilt = 0.f, Sweep = 1.f;
};

/** Anel na água: um plano com o material de onda que cresce e desbota (só visual, local). */
UCLASS()
class VORTEX_API AVortexRipple : public AActor
{
    GENERATED_BODY()
public:
    AVortexRipple();
    virtual void Tick(float DeltaSeconds) override;
    void Start(float InSize, float InLife);
private:
    UPROPERTY(Transient) TObjectPtr<class UStaticMeshComponent> Plane;
    UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> Wave;
    float Age = 0.f, Life = 1.f, Size = 100.f;
};

/** Respingo: cubinhos que voam do ponto do acerto, caem e somem (só visual, local). */
UCLASS()
class VORTEX_API AVortexHitBurst : public AActor
{
    GENERATED_BODY()
public:
    AVortexHitBurst();
    virtual void Tick(float DeltaSeconds) override;
    void Start(const FVector& Away, const FLinearColor& Color, int32 Count, float Ground);

private:
    UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Bits;
    struct FBit { FVector At, Velocity; float Size, Life; };
    TArray<FBit> State;
    TArray<FTransform> Poses;
    float Age = 0.f, Floor = 0.f;
};
