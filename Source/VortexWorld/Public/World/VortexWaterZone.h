#pragma once

#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "VortexWaterZone.generated.h"

class UBoxComponent;

/**
 * Água como dado: uma caixa no mundo com a altura da superfície. O movimento pergunta "tem água
 * aqui e em que altura?" (UVortexWaterSubsystem) em vez de depender de um volume de água montado
 * no editor. É o mesmo modelo do Godot (WaterLevel do vale, lagos e rios gerados): o gerador de
 * terreno vai registrar as zonas dos rios e lagos de cada pedaço do mesmo jeito.
 */
UCLASS()
class VORTEXWORLD_API AVortexWaterZone : public AActor
{
    GENERATED_BODY()

public:
    AVortexWaterZone();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    // Altura da superfície (o topo da caixa), em coordenadas do mundo.
    float SurfaceZ() const;
    bool Contains(const FVector& Location) const;

    UPROPERTY(VisibleAnywhere, Category = "Vortex|Water")
    TObjectPtr<UBoxComponent> Bounds;
};

UCLASS()
class VORTEXWORLD_API UVortexWaterSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void Register(AVortexWaterZone* Zone) { Zones.AddUnique(Zone); }
    void Unregister(AVortexWaterZone* Zone) { Zones.Remove(Zone); }

    // Superfície da água sobre o ponto (false = sem água).
    bool SurfaceAt(const FVector& Location, float& OutSurfaceZ) const;

    // Água vinda do terreno gerado (rios, lagos, mar): consultada quando nenhuma zona responde.
    TFunction<bool(const FVector&, float&)> Provider;

private:
    UPROPERTY()
    TArray<TObjectPtr<AVortexWaterZone>> Zones;
};
