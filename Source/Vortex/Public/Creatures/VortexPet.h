#pragma once

#include "GameFramework/Actor.h"
#include "VortexPet.generated.h"

class UStaticMesh;

/**
 * Cachorrinho da Julia: um bichon branco em blocos (Tools/Blender/MakeDog.py, 02/11), como em
 * modelando ainda/referencia.jpeg: cabeça grande com focinho, olhos, nariz e língua, orelhas compridas caídas,
 * corpo fofo, patas curtas e o rabo pompom. Cada parte é um modelo (/Game/Vortex/Creatures/Pet/Dog_<parte>) num pivô.
 * Anda e corre atrás do dono (ao lado e um pouco atrás), com patas, corpo, cabeça e rabo animados por
 * código (passo pela velocidade, rabo abanando, cabeça que inclina de vez em quando).
 * Só visual: cada máquina tem o seu, seguindo o NPC (que vai pela rede).
 */
UCLASS()
class VORTEX_API AVortexPet : public AActor
{
    GENERATED_BODY()
public:
    AVortexPet();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void Follow(AActor* InOwner) { Leader = InOwner; }

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
    UPROPERTY(Transient) TObjectPtr<USceneComponent> BodyPivot;
    UPROPERTY(Transient) TObjectPtr<USceneComponent> HeadPivot;
    UPROPERTY(Transient) TObjectPtr<USceneComponent> TailPivot;
    UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> LegPivots;

    TWeakObjectPtr<AActor> Leader;
    float Clock = 0.f, Gait = 0.f, Speed = 0.f, Yaw = 0.f, TiltAt = 3.f, Seed = 0.f;

    void Build();
    /** Um pivô com a parte (Dog_<Part>) presa nele. */
    USceneComponent* Part(USceneComponent* Parent, const FVector& At, const TCHAR* PartName);
    float GroundAt(const FVector& Where) const;
};
