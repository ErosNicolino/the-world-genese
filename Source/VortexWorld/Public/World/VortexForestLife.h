#pragma once

#include "GameFramework/Actor.h"
#include "VortexForestLife.generated.h"

class UAudioComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMesh;

/**
 * Floresta viva, em cada máquina que desenha (criada por UVortexDayNightSubsystem):
 * - Sons de ambiente (gravações SONNISS, Tools/BuildSfxMaster.py): pássaros de dia, mata e corujas à noite, vento
 *   sempre um pouco e, perto dos lagos e rios, água correndo. Por cima, o som do lugar: ondas no mar, vulcão em
 *   Brasafenda, tempestade em Raikor, vento frio em Nivaluz, deserto em Solcânion, pântano em Lodonegro, mata
 *   assombrada em Morthúria, mata élfica em Verdalume e a praça na cidade (onde os pássaros ficam mais baixos).
 *   Os loops se misturam devagar (0,5 por segundo) e os do lugar só tocam enquanto se ouvem.
 * - Borboletas e sementes ao vento de dia, vaga-lumes à noite, em volta da câmera. O voo, o bater
 *   de asas e o piscar são calculados na placa de vídeo (materiais M_Butterfly, M_Firefly,
 *   M_Seed); aqui só se decide onde cada um vive, perto do chão, e se repõe quem ficou longe.
 * -VortexNoForestLife desliga.
 */
UCLASS(Transient)
class VORTEXWORLD_API AVortexForestLife : public AActor
{
    GENERATED_BODY()

public:
    AVortexForestLife();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    float SoundLevel = .8f;   // volume geral dos ambientes

    // medições/testes
    int32 VisibleCritters() const;
    float GetWaterNear() const { return WaterNear; }     // 0 longe da água .. 1 na margem
    float GetDaylight() const { return Daylight; }
    FString DescribeSound() const;

private:
    struct FKind
    {
        UInstancedStaticMeshComponent* Instances = nullptr;   // presos em Keep
        UMaterialInstanceDynamic* Material = nullptr;
        TArray<FVector> Homes;
        float Radius = 2000.f, MinHeight = 30.f, MaxHeight = 150.f;
        FVector2f Scale = FVector2f(1.f, 1.f);   // tamanho sorteado entre X e Y
        bool bNight = false;
        float Fade = -1.f;
    };
    FKind Butterflies, Seeds, Fireflies;

    UPROPERTY() TArray<TObjectPtr<UObject>> Keep;
    struct FLoop { UAudioComponent* Audio = nullptr; float Volume = 0.f; };
    FLoop Day, Night, Wind, Water;
    /** O som do lugar (amb_ocean, amb_volcano...): criado quando o herói chega lá, parado quando some. */
    static constexpr int32 PlaceCount = 9;
    FLoop Places[PlaceCount];
    float PlaceTarget[PlaceCount] = {}, DayScale = 1.f;
    int32 PlaceIndex() const;
    void MixPlace(int32 Index, float DeltaSeconds);

    float Daylight = 1.f, WaterNear = 0.f, CheckTime = 0.f, PlaceTime = 0.f;
    FRandomStream Rng = FRandomStream(5813);

    void MakeLoop(FLoop& Loop, const TCHAR* Path);
    void Mix(FLoop& Loop, float Target, float DeltaSeconds);
    void SetupKind(FKind& Kind, UStaticMesh* Mesh, const TCHAR* MaterialPath, int32 Count, const FVector& Center);
    UStaticMesh* BuildButterfly();
    UStaticMesh* BuildFirefly();
    UStaticMesh* BuildSeed();
    UStaticMesh* BuildMesh(const TArray<FVector3f>& Positions, const TArray<FVector3f>& Normals,
        const TArray<FVector2f>& UV0, const TArray<FVector2f>& UV1, const TArray<int32>& Triangles, float Reach);
    bool PlaceHome(const FKind& Kind, const FVector& Center, float MinDistance, FVector& Out);
    void Refresh(FKind& Kind, const FVector& Center, float Fade, bool bReplace);
    float NearestWater(const FVector& Center) const;
    FVector CameraLocation() const;
};
