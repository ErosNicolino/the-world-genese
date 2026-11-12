#pragma once

#include "GameFramework/Actor.h"
#include "VortexFairies.generated.h"

class UMaterialInstanceDynamic;
class UPoseableMeshComponent;
class USkeletalMesh;

/**
 * As fadas (10/11; Tools/Blender/MakeFairy.py, Tools/ImportFairy.py): pequeninas (~9 cm), de blocos, com asas finas e
 * translúcidas de libélula. Só enfeite em cada máquina que desenha, como os pássaros da AVortexWildlife: sem colisão
 * (não dá para acertar) e sem rede. Algumas voam perto da câmera, passeiam em volta de um ponto e param no ar; quando
 * alguém chega perto, fogem para longe e para cima e voltam depois em outro lugar, fora da vista. As asas batem por
 * código (o par de baixo atrasado, como nos insetos). 6 penteados x 6 cores de cabelo. Ajustes em
 * Data/Creatures/Fada/fadas.json. -VortexNoFairies desliga.
 */
UCLASS(Transient)
class VORTEXWORLD_API AVortexFairies : public AActor
{
    GENERATED_BODY()

public:
    AVortexFairies();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    // testes
    int32 Visible() const;
    int32 Fleeing() const;
    /** Testes: põe uma fada parada no ar em At, olhando para Yaw (graus), com o penteado Style (para a foto). */
    void DebugPlace(const FVector& At, float Yaw, int32 Style = 0);
    /** Chat de testes (/spawn fada): uma fada livre aparece em At e passeia ali (a que estiver mais longe da câmera). */
    bool Summon(const FVector& At);

private:
    struct FFairy
    {
        TObjectPtr<UPoseableMeshComponent> Mesh;
        TObjectPtr<UMaterialInstanceDynamic> Wing, Face;
        float Blink = 2.f, Happy = 0.f;   // o rosto: até a próxima piscada; quanto tempo ainda fica feliz
        int32 Frame = -1;                  // o quadro do rosto (0 normal, 1 piscando, 2 feliz, 3 assustada)
        FVector Pos = FVector::ZeroVector, Vel = FVector::ZeroVector, Home = FVector::ZeroVector, Target = FVector::ZeroVector;
        float Phase = 0.f, Timer = 0.f, Pause = 0.f, Yaw = 0.f, Flee = 0.f;
        bool bActive = false, bHeld = false;   // bHeld: posta à mão (teste), fica parada
    };
    TArray<FFairy> Fairies;
    UPROPERTY(Transient) TArray<TObjectPtr<UObject>> Keep;
    TArray<TObjectPtr<USkeletalMesh>> Meshes;
    TArray<FName> WingBones;
    TMap<FName, FTransform> Rest;      // a pose de descanso de cada osso da asa (espaço da malha)
    TArray<FVector> Scarers;
    float ScareTimer = 0.f, Daylight = 1.f, Clock = 0.f, YawFix = 0.f;
    TMap<FString, float> Tune;
    FRandomStream Rng = FRandomStream(9187);

    float T(const TCHAR* Key, float Default) const { const float* V = Tune.Find(Key); return V ? *V : Default; }
    FVector CameraLocation(FVector* Forward = nullptr) const;
    bool FindSpot(const FVector& Camera, const FVector& Forward, bool bBehind, FVector& Out);
    void Spawn(FFairy& F, const FVector& Camera, const FVector& Forward, bool bFirst);
    void Dress(FFairy& F, int32 Style = -1);
    void Animate(FFairy& F, float Speed);
    void Expression(FFairy& F, float Dt);
};
