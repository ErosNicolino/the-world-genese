#pragma once

#include "GameFramework/Actor.h"
#include "VortexLobbed.generated.h"

class UStaticMeshComponent;

/**
 * O que as criaturas arremessam em arco: a pedra do golem e do goblin, a bolha de veneno do
 * cogumelo (PoisonProjectile do Godot). Voa do ponto de partida até onde o herói ESTAVA, num tempo
 * fixo; ao cair, o servidor confere quem está perto do ponto AGORA (dá tempo de sair de baixo).
 */
UCLASS()
class VORTEX_API AVortexLobbed : public AActor
{
    GENERATED_BODY()
public:
    AVortexLobbed();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Servidor: Rock = pedra (físico), senão veneno (mágico). */
    static AVortexLobbed* Throw(AActor* Source, const FVector& From, const FVector& Land, float FlightTime, float Damage, float Radius, bool bRock);

    UPROPERTY(Replicated) FVector_NetQuantize From;
    UPROPERTY(Replicated) FVector_NetQuantize Land;
    UPROPERTY(Replicated) float Flight = 1.f;
    UPROPERTY(Replicated) bool bRock = true;

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    float Damage = 10.f, Radius = 200.f, Age = 0.f;
    bool bLanded = false;
    UFUNCTION(NetMulticast, Unreliable) void MulticastLand();
};
