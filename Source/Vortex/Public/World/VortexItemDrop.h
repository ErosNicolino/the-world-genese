#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VortexItemDrop.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UTextRenderComponent;

/**
 * O item no chão (07/11, o usuário: "como no Fortnite: o item fica rodando no chão com um efeitinho"): cai dos bichos,
 * dos baús e da mochila (jogar fora). Gira e flutua em cima de um anel, com o facho de luz na cor da raridade; perto
 * dele aparece "[E] pegar" e o E põe na mochila (UVortexHeroSheet::ServerPickup). Some sozinho depois de 5 minutos.
 * Só o servidor cria e tira; cada máquina monta o desenho pelo item (replicado).
 */
UCLASS()
class VORTEX_API AVortexItemDrop : public AActor
{
    GENERATED_BODY()

public:
    AVortexItemDrop();
    /** Servidor: um item no chão em At (pula um pouco para o lado, como saindo do bicho). */
    static AVortexItemDrop* Spawn(UWorld* World, FName Item, int32 Count, const FVector& At, float Toss = 1.f);
    /** O item no chão mais perto do ponto, até Range cm (o E). */
    static AVortexItemDrop* FindNear(const UWorld* World, const FVector& At, float Range);
    /** Este item está perto desse ponto (a mesma medida do FindNear). */
    bool IsNear(const FVector& At, float Range) const { return FVector::Dist2D(Landing, At) < Range && FMath::Abs(Landing.Z - At.Z) < 250.f; }

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    FName GetItem() const { return Item; }
    int32 GetCount() const { return Count; }
    /** Servidor: pegou (parte ou tudo); sem nada, some. */
    void Take(int32 Taken);

private:
    UFUNCTION() void OnRep_Item();
    void BuildLook();

    UPROPERTY(ReplicatedUsing = OnRep_Item) FName Item;
    UPROPERTY(Replicated) int32 Count = 1;
    UPROPERTY(Replicated) FVector_NetQuantize Landing;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Show;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> ShowPiece;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Beam;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Ring;
    UPROPERTY() TObjectPtr<UPointLightComponent> Light;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Motes;
    FVector Start = FVector::ZeroVector;
    float Age = 0.f, Offset = 0.f, RingSize = 1.f, BeamWidth = .2f;
    bool bBuilt = false;
};
