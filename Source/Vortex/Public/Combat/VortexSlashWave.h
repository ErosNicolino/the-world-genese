#pragma once

#include "GameFramework/Actor.h"
#include "VortexSlashWave.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;
class UProceduralMeshComponent;
class UNiagaraComponent;

/**
 * Corte à distância da espada (especial, botão direito): uma lua crescente de energia azul que
 * corre reto para onde o herói mirou, atravessa os inimigos (cada um leva o golpe uma vez) e se
 * desfaz ao bater no terreno ou numa construção (passa pela vegetação), ou no fim do alcance.
 * O servidor decide os acertos (ApplyHit do combate de quem lançou); cada máquina anda e desenha
 * a sua cópia pelo mesmo caminho (posição inicial, direção e velocidade), sem replicar movimento.
 * Desenho: um arco de energia em 3D (fita curva gerada em código: o meio à frente, as pontas
 * curvadas para trás) com M_SlashArc (Tools/CreateSlashArc.py) em duas camadas — o fio branco do
 * núcleo e o brilho azul em volta —, quatro cópias que ficam para trás e se apagam (o rastro),
 * faíscas que se soltam no caminho, uma luz azul que acompanha e um clarão ao bater.
 */
UCLASS()
class VORTEX_API AVortexSlashWave : public AActor
{
    GENERATED_BODY()
public:
    AVortexSlashWave();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    float Damage = 16.f;
    float Speed = 2600.f;       // cm/s
    float Range = 2600.f;       // cm
    float HalfWidth = 150.f;    // cm, para os lados
    float HalfHeight = 90.f;    // cm, para cima e para baixo

    int32 HitCount() const { return Hits.Num(); }

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Light;
    UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Blades;
    UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Sparks;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
    UPROPERTY(ReplicatedUsing=OnRep_Burst) FVector_NetQuantize BurstAt;
    UPROPERTY(ReplicatedUsing=OnRep_Burst) bool bBurst = false;

    UFUNCTION() void OnRep_Burst();
    void StartBurst(const FVector& Where);
    void UpdateLook(float Fade, float Grow);

    FVector Start = FVector::ZeroVector, Direction = FVector::ForwardVector;
    float Age = 0.f, BurstAge = 0.f, Travelled = 0.f;
    TSet<TWeakObjectPtr<AActor>> Hits;
};
