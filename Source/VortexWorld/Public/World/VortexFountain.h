#pragma once

#include "GameFramework/Actor.h"
#include "VortexFountain.generated.h"

class UAudioComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UProceduralMeshComponent;

/**
 * Fonte de Jade da praça da Cidade do Santuário, refeita na Unreal (no lugar do vil_fountain do Godot):
 * bacia octogonal de pedra com degrau de basalto, coluna de mármore com anéis de bronze, bacia de cima
 * com oito bicos e o cristal de jade brilhando no topo. As duas bacias têm a água do Vortex (a da
 * qualidade escolhida nas opções); oito jatos de gotas em arco caem na bacia de baixo, com respingos
 * onde batem e um borbulhar em volta da coluna (como o FountainWater.cs do Godot), e o som da água.
 *
 * É o santuário: perto dela a vida e a mana voltam depressa, e quem cai em combate renasce aqui
 * (UVortexHeroSheet). Cada máquina monta a sua; a cura e o renascimento são do servidor.
 */
UCLASS()
class VORTEXWORLD_API AVortexFountain : public AActor
{
    GENERATED_BODY()
public:
    AVortexFountain();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    static AVortexFountain* Find(const UWorld* World);
    /** Onde quem caiu renasce: na frente da fonte, de frente para ela. */
    FTransform RespawnPoint() const;
    /** Perto o bastante para a água do santuário curar (cm). */
    bool IsNear(const FVector& Where) const { return FVector::Dist2D(Where, GetActorLocation()) < 420.f; }

    static constexpr float LowerRadius = 250.f;
    static constexpr float WaterLow = 45.f;
    static constexpr float UpperRadius = 106.f;
    static constexpr float WaterHigh = 215.f;
    static constexpr float ColumnRadius = 65.f;

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
    UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Water;
    UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Drops;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Sound;
    UPROPERTY(Transient) TObjectPtr<UPointLightComponent> JadeLight;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Crystal;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> CurrentWater;

    struct FDrop { int32 Kind = 0; int32 Jet = 0; float Phase = 0.f; FVector Velocity = FVector::ZeroVector; FVector Start = FVector::ZeroVector; float Life = 1.f; };
    TArray<FDrop> DropState;
    TArray<FTransform> DropPoses;
    float Clock = 0.f, WaterCheck = 0.f;

    void Build();
    UStaticMeshComponent* Piece(const TCHAR* Shape, const TCHAR* Material, const FVector& At, const FVector& Size, float Yaw = 0.f, bool bCollide = true);
    void RefreshWater();
};
