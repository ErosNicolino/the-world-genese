#pragma once

#include "GameFramework/Actor.h"
#include "VortexWindTrails.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;

/**
 * Rastros de vento no ar (10/11): fitas finas e claras que aparecem perto da câmera, correm na direção do vento (a mesma
 * da grama), às vezes dão um laço, e somem. Só enfeite em cada máquina que desenha, como as fadas: sem colisão e sem
 * rede. A fita é uma malha feita aqui (em cruz, para aparecer de qualquer ângulo) e o pedaço claro que corre por ela é
 * do material (M_WindTrail, Tools/CreateWindTrailMaterial.py). Mais fracas à noite. Ajustes em Data/World/vento.json.
 * -VortexNoWindTrails desliga; -VortexWindDebug põe todas na frente da câmera, mais lentas (fotos).
 */
UCLASS(Transient)
class VORTEXWORLD_API AVortexWindTrails : public AActor
{
    GENERATED_BODY()

public:
    AVortexWindTrails();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    // duas fitas: ondulada e com laço; cada rastro é uma cópia de uma delas
    UPROPERTY(Transient) TArray<TObjectPtr<UInstancedStaticMeshComponent>> Ribbons;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Material;
    struct FTrail { int32 Shape = 0, Instance = 0; float NextAt = 0.f; };
    TArray<FTrail> Trails;
    TMap<FString, float> Tune;
    FRandomStream Rng = FRandomStream(4421);
    float Daylight = -1.f;
    bool bDebug = false;

    float T(const TCHAR* Key, float Default) const { const float* V = Tune.Find(Key); return V ? *V : Default; }
    UStaticMesh* BuildRibbon(UMaterialInterface* Surface, bool bLoop);
    void Respawn(int32 Index, float Now);
};
