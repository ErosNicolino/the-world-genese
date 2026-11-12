#pragma once

#include "GameFramework/Actor.h"
#include "VortexSeriousPunch.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Soco Sério (especial das manoplas, botão direito): o herói junta força por meio segundo e solta um
 * punho fantasma gigante que voa reto, atravessando e arremessando tudo num corredor largo.
 * O servidor decide o dano (uma vez por alvo); todas as máquinas desenham o punho, o cone de vento,
 * os anéis de ar, a onda de choque e o quadro de impacto.
 */
UCLASS()
class VORTEX_API AVortexSeriousPunch : public AActor
{
    GENERATED_BODY()
public:
    AVortexSeriousPunch();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(Replicated) float Damage = 20.f;
    UPROPERTY(Replicated) float Range = 2600.f;
    UPROPERTY(Replicated) float Speed = 3400.f;
    UPROPERTY(Replicated) float Width = 260.f;
    /** Malha do punho (a manopla equipada). */
    UPROPERTY(Replicated) TObjectPtr<class UStaticMesh> FistMesh;

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Fist;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Wind;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FistGlow;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> WindGlow;
    FVector Start = FVector::ZeroVector, Direction = FVector::ForwardVector;
    float Travelled = 0.f, Age = 0.f, NextRing = 0.f;
    bool bLanded = false;
    TSet<TWeakObjectPtr<AActor>> Struck;
    void StrikeAlong(float From, float To);
    void SpawnRing(const FVector& At) const;
};
