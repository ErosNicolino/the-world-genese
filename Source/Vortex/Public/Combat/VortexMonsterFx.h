#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VortexMonsterFx.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

/**
 * Efeitos das habilidades dos monstros (01/11): cada espécie com o seu, no estilo de blocos do jogo (cubinhos em 3 tons,
 * sem partículas). Antes todas as habilidades mostravam só o mesmo aviso no chão.
 * - PoisonCloud: a bomba de fumaça de veneno do cogumelo (os esporos);
 * - PoisonPool / Drip: a poça e os pingos do cuspe de veneno;
 * - Tornado: o mini tornado de vento do morcego (o guincho), um funil girando que se abre para cima e a poeira na base;
 * - Dust: o anel de poeira e pedrinhas da pancada no chão (golem, gota de ferro);
 * - Splash: o respingo de lama do salto do sapo.
 * Só no cliente (visual); quem chama é o multicast do monstro ou o AVortexLobbed.
 */
UCLASS()
class VORTEX_API AVortexMonsterFx : public AActor
{
    GENERATED_BODY()

public:
    AVortexMonsterFx();
    /** Cria o efeito em At (Follow: segue o ator, o tornado no morcego); Delay: espera antes de começar (o impacto). */
    static AVortexMonsterFx* Spawn(UWorld* World, FName Kind, const FVector& At, float Radius, float Delay = 0.f, AActor* Follow = nullptr);
    virtual void Tick(float DeltaSeconds) override;

private:
    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Shades;   // um componente por tom (o material é de uma cor)
    struct FBit
    {
        int32 Shade = 0;
        FVector Dir = FVector::ZeroVector;   // a direção para fora (nuvem, poeira) ou o ângulo/altura (tornado)
        float A = 0.f, B = 0.f, C = 0.f;     // números próprios de cada efeito (raio, altura, fase)
        float Size = 10.f;
    };
    TArray<FBit> Bits;
    // 08/11: as partes de material animado: o funil, o círculo no chão e as nuvens macias
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Funnels;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> GroundRing;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Puffs;
    UPROPERTY() TArray<TObjectPtr<class UMaterialInstanceDynamic>> Fading;
    TArray<float> FadingStrength;
    TArray<FBit> PuffBits;
    float BaseZ = 0.f;
    FTransform PlacePuff(const FBit& Bit, float T) const;
    TWeakObjectPtr<AActor> FollowActor;
    FName Kind;
    float Radius = 100.f, Delay = 0.f, Age = 0.f, Life = 1.5f;
    void Begin();
    FTransform Place(const FBit& Bit, float T) const;
};
