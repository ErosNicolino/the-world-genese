#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "VortexHairComponent.generated.h"

/**
 * Cabelo do humano de blocos (01/11): a malha à parte (Tools/Blender/MakeBlockHair.py) vestida por cima do corpo.
 * Sem física, segue o corpo como as roupas (SetLeaderPoseComponent: a mesma pose que vai para a tela). Com física, todo
 * quadro, depois da animação do corpo, copia a pose final dele osso a osso (os nomes são os mesmos). Nos cabelos soltos
 * cada corrente de ossos Hair_<nome>_NN (o rabo; a nuca e os lados do cabelo comprido) vira pontos com inércia e
 * gravidade (Verlet): cada ponto fica no comprimento do osso, é puxado de volta para a forma do modelo (mais ou menos,
 * pela Looseness) e empurrado para fora do corpo (esferas na cabeça, no pescoço, na coluna e nos ombros; CollisionMargin
 * a mais com armadura).
 */
UCLASS()
class VORTEX_API UVortexHairComponent : public UPoseableMeshComponent
{
    GENERATED_BODY()

public:
    UVortexHairComponent();

    /** O corpo que o cabelo segue (a pose e as esferas de colisão). */
    void SetBody(USkeletalMeshComponent* InBody);
    /** Troca a malha; Looseness 0 = sem física (só segue a cabeça), .5 = leve, 1 = solto. */
    void SetHair(USkeletalMesh* Mesh, float InLooseness);
    /** cm a mais nas esferas do corpo (armadura, capa). */
    float CollisionMargin = 1.5f;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    struct FChain
    {
        TArray<FName> Bones;                 // da raiz para a ponta
        TArray<FVector> Points, Previous;    // as juntas e a ponta, no mundo
    };
    TWeakObjectPtr<USkeletalMeshComponent> Body;
    float Looseness = 0.f;
    TArray<FChain> Chains;
    float StepLeft = 0.f;
    /** Cada osso do cabelo -> o osso do corpo com o mesmo nome (INDEX_NONE nos Hair_*), da malha MappedBody. */
    TArray<int32> BodyBone;
    TWeakObjectPtr<const USkinnedAsset> MappedBody;
    void CopyBodyPose(USkeletalMeshComponent* Driver);
    void Simulate(float DeltaTime);
};
