#pragma once

#include "GameFramework/Actor.h"
#include "VortexProjectile.generated.h"

class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * Projétil das armas do Godot: flecha (arco, besta), dardo (cajado, varinha, pistola, livro...),
 * esfera que explode no impacto (orb) e explosão num ponto à frente (burst).
 * Como o corte da espada: o servidor decide os acertos (ApplyHit de quem lançou) e cada máquina
 * anda e desenha a sua cópia pelo mesmo caminho (os números vão uma vez, na criação).
 * Desenho: um núcleo brilhante na cor do elemento, o rastro do pacote ArrowTrail (por elemento),
 * uma luz que acompanha e um clarão ao bater.
 * Flechas (arco e besta) são físicas: haste de madeira, ponta e penas, caem com a gravidade apontando para
 * onde vão e ficam cravados onde batem (no chão, na parede ou no bicho, acompanhando-o) por uns
 * segundos, sem brilho nem rastro.
 */
UCLASS()
class VORTEX_API AVortexProjectile : public AActor
{
    GENERATED_BODY()
public:
    AVortexProjectile();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Cor de cada elemento (nature, jade, holy, arcane, steel, fire, ice...). */
    static FLinearColor ElementColor(FName Element);
    /** Direção para uma flecha (que cai com a gravidade) sair de From e descer em Target; sem solução, Fallback. */
    static FVector BallisticDirection(const FVector& From, const FVector& Target, float Speed, const FVector& Fallback, float Drop = 1.f);

    // definidos por quem lança, antes do BeginPlay (SpawnActorDeferred)
    UPROPERTY(Replicated) FName Mode = TEXT("arrow");
    UPROPERTY(Replicated) FName Element;
    UPROPERTY(Replicated) float Speed = 2800.f;     // cm/s
    UPROPERTY(Replicated) float Range = 1600.f;     // cm
    UPROPERTY(Replicated) float Radius = 0.f;       // explosão (orb, burst)
    UPROPERTY(Replicated) float Size = 1.f;         // carga do disparo: maior e mais brilhante
    float Damage = 10.f;
    /** Só no servidor: segundos que a criatura acertada fica atordoada. */
    float Stun = 0.f;
    /** Atravessa a fila (virote pesado, tiro rúnico): cada alvo uma vez. */
    UPROPERTY(Replicated) bool bPierce = false;
    /** 03/11: quanto a flecha cai (1 = a gravidade da flecha). O arco pouco puxado cai muito; puxado até o fim, quase reto. */
    UPROPERTY(Replicated) float Drop = 1.f;

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Light;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Core;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> CoreMaterial;
    UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Trail;
    UPROPERTY(ReplicatedUsing=OnRep_Burst) FVector_NetQuantize BurstAt;
    UPROPERTY(ReplicatedUsing=OnRep_Burst) bool bBurst = false;

    UFUNCTION() void OnRep_Burst();
    // flecha cravada: onde, para onde apontava e em quem (vão pela rede)
    UPROPERTY(ReplicatedUsing=OnRep_Stuck) bool bStuck = false;
    UPROPERTY(Replicated) FVector_NetQuantize StuckAt;
    UPROPERTY(Replicated) FVector_NetQuantizeNormal StuckDir;
    UPROPERTY(Replicated) TObjectPtr<AActor> StuckOn;
    UFUNCTION() void OnRep_Stuck();
    void Stick(const FVector& Where, const FVector& Dir, AActor* On);
    bool IsPhysical() const { return Mode == TEXT("arrow"); }
    FVector ArrowAt(float Time) const;
    static constexpr float ArrowGravity = -620.f;   // cm/s² (um pouco menos que a real: o arco ainda alcança)
    void StartBurst(const FVector& Where);
    void Explode(const FVector& Where);
    void HitTarget(AActor* Target, float Amount);

    // 03/11: de onde, para onde e quando saiu, como o servidor lançou. Nos outros jogos a flecha chegava pela rede já no
    // meio do voo (ou nem aparecia, se acertava antes): agora cada máquina refaz o voo desde o arco
    UPROPERTY(Replicated) FVector_NetQuantize LaunchFrom;
    UPROPERTY(Replicated) FVector_NetQuantizeNormal LaunchDir;
    UPROPERTY(Replicated) float LaunchTime = 0.f;
    UPROPERTY(Transient) TObjectPtr<class UNiagaraComponent> Flames;
    FVector Start = FVector::ZeroVector, Direction = FVector::ForwardVector;
    /** 03/11: só no desenho de quem atirou em primeira pessoa: a flecha sai do arco da tela e em 0,2 s entra no voo de
     *  verdade (o voo do servidor sai do arco do corpo, que a câmera não mostra). */
    FVector ViewOffset = FVector::ZeroVector;
    float Age = 0.f, BurstAge = 0.f, Travelled = 0.f;
    TSet<TWeakObjectPtr<AActor>> Hits;
};
