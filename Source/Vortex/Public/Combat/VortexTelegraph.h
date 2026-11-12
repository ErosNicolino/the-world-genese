#pragma once

#include "GameFramework/Actor.h"
#include "VortexTelegraph.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Aviso no chão dos golpes de área (chefes, pancada do golem, salto da gota): um círculo (ou um anel)
 * que enche até a hora do impacto, como o BossTelegraph do Godot. No impacto o servidor fere os
 * heróis dentro (anel: só quem está entre 45% e 100% do raio, o miolo é seguro) e todos veem o tremor.
 * Criado só no servidor (Spawn); vai para os clientes com a posição, o raio, a espera e a cor.
 */
UCLASS()
class VORTEX_API AVortexTelegraph : public AActor
{
    GENERATED_BODY()
public:
    AVortexTelegraph();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Servidor: marca o chão em At (cm), com raio (cm), espera (s) e dano; Source é quem golpeia. */
    static AVortexTelegraph* Spawn(AActor* Source, const FVector& At, float Radius, float Delay, float Damage, bool bRing,
        const FLinearColor& Color, bool bMagic = false, float Stun = 0.f);

    UPROPERTY(Replicated) float Radius = 300.f;
    UPROPERTY(Replicated) float Delay = 1.f;
    UPROPERTY(Replicated) bool bRing = false;
    UPROPERTY(Replicated) FLinearColor Color = FLinearColor(1.f, .4f, .2f);

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Disc;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Material;
    float Damage = 10.f, Stun = 0.f, Age = 0.f;
    bool bMagic = false, bResolved = false;
    void Resolve();
    UFUNCTION(NetMulticast, Unreliable) void MulticastImpact();
};
