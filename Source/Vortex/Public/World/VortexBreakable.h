#pragma once

#include "GameFramework/Actor.h"
#include "VortexBreakable.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Barril de água, barril de vinho ou caixote da vila (03/11; a vila monta no servidor, AVortexTown, pela
 * Data/GodotTown/town_extra.json). Vai pela rede.
 * - Física de verdade (no servidor): o herói empurra, rola, cai.
 * - Na água ele boia, balançando, e faz o mergulho ao cair.
 * - Golpe, flecha ou magia quebra (BreakNear / BreakInArc). Quebrando:
 *   - as tábuas voam (com física, cada máquina as suas);
 *   - o barril espirra o líquido em cubinhos e deixa uma poça brilhante, que se espalha e seca em ~20 s;
 *   - na água, faz só o anel.
 * - Volta inteiro no mesmo lugar 3 minutos depois.
 */
UCLASS()
class VORTEX_API AVortexBreakable : public AActor
{
    GENERATED_BODY()
public:
    AVortexBreakable();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** "water", "wine" ou "crate" (definido antes do BeginPlay). */
    UPROPERTY(Replicated) FName Kind = TEXT("crate");

    /** Servidor: quebra os que estão a até Radius do ponto (flechas, magias, explosões). Quantos quebrou. */
    static int32 BreakNear(UWorld* World, const FVector& At, float Radius, const FVector& Push = FVector::ZeroVector);
    /** Servidor: quebra os que estão no arco do golpe (de From para Forward, até Reach, com o cosseno mínimo MinDot). */
    static int32 BreakInArc(UWorld* World, const FVector& From, const FVector& Forward, float Reach, float MinDot, float PushSpeed);

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Box;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(ReplicatedUsing = OnRep_Broken) bool bBroken = false;
    UPROPERTY(Replicated) FVector_NetQuantize BreakAt;
    UPROPERTY(Replicated) FVector_NetQuantize BreakPush;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Puddles;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> PuddleLooks;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Shards;

    UFUNCTION() void OnRep_Broken();
    UFUNCTION(NetMulticast, Unreliable) void MulticastSplash(FVector_NetQuantize At, float Strength);
    void Break(const FVector& At, const FVector& Push);
    bool IsBarrel() const { return Kind == TEXT("water") || Kind == TEXT("wine"); }
    FVector HalfSize() const;

    FTransform Home;
    bool bInWater = false;
    bool bPlaced = false;
    float PuddleAge = -1.f, PuddleSize = 1.f;
    TArray<FVector> PuddleOffsets;
};
