#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "VortexAwakeningStadium.generated.h"

class UAbilitySystemComponent;
class UCapsuleComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;
class UVortexAttributes;
class UVortexCombatComponent;
class UVortexWeaponDefinition;

/**
 * Boneco de treino do Estádio: aguenta qualquer golpe (a vida volta cheia logo depois), não ataca.
 * Mostra os números de dano como qualquer criatura.
 */
UCLASS()
class VORTEX_API AVortexTrainingDummy : public AActor, public IAbilitySystemInterface
{
    GENERATED_BODY()
public:
    AVortexTrainingDummy();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UAbilitySystemComponent> AbilitySystem;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UVortexAttributes> Vitals;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UVortexCombatComponent> Combat;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Post;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Sack;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Arms;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Head;
    float LastHealth = 0.f, SinceHit = 10.f, Wobble = 0.f;
};

/**
 * Estádio do Despertar (Game/World/AwakeningStadium*.cs do Godot): o herói novo acorda sem arma
 * num coliseu de treino, longe do vale. As 24 armas giram em pedestais num anel dentro da própria arena,
 * em quatro grupos coloridos, um por categoria (marcial, distância, arcana, exótica). Perto de um pedestal
 * aparecem nome, golpe e especial; E pega a arma (devolvendo a anterior); os bonecos de treino da
 * arena aguentam qualquer golpe; E no portão dourado, com uma arma na mão, leva ao vale.
 *
 * Montado em código, peça por peça como no Godot (medidas em metros em volta de Origin), com as
 * texturas do Godot projetadas no mundo (Tools/ImportStadium.py). Toda a pedra, madeira, bronze e
 * tecido parados ficam em poucas malhas instanciadas por material. Existe em cada máquina (cada
 * uma monta a sua cópia, igual); só o servidor cria os bonecos.
 */
UCLASS()
class VORTEX_API AVortexAwakeningStadium : public AActor
{
    GENERATED_BODY()
public:
    AVortexAwakeningStadium();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    /** Longe do vale (fora do mapa), no alto. */
    static FVector Origin() { return FVector(400000.f, 0.f, 2000.f); }
    static bool Contains(const FVector& Where) { return FVector::Dist2D(Where, Origin()) < 9000.f; }
    static AVortexAwakeningStadium* Find(const UWorld* World);
    static AVortexAwakeningStadium* Ensure(UWorld* World);

    FVector SpawnPoint() const { return Origin() + FVector(0.f, -600.f, 120.f); }
    FVector ExitPoint() const { return ExitAt; }

    struct FPedestal
    {
        FName Type;
        FVector Where = FVector::ZeroVector;
        TWeakObjectPtr<UVortexWeaponDefinition> Definition;
        TWeakObjectPtr<UStaticMeshComponent> Display;
        float BaseZ = 0.f;
        FQuat Upright = FQuat::Identity;
        FVector CenterOffset = FVector::ZeroVector;   // centro do modelo (já de pé e escalado)
    };
    /** Pedestal a até 2,3 m (no plano), ou nenhum. */
    const FPedestal* NearestPedestal(const FVector& Where) const;
    /** O pedestal de uma arma (o servidor confere a escolha do jogador por ele, com folga). */
    const FPedestal* FindPedestal(FName Type) const;
    const TArray<FPedestal>& GetPedestals() const { return Pedestals; }
    bool IsAtExit(const FVector& Where) const { return FVector::Dist2D(Where, ExitAt) < 320.f; }
    /** A arma que está na mão some do pedestal. */
    void SetHeldType(FName Type) { HeldType = Type; }

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UInstancedStaticMeshComponent>> Batches;
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UMaterialInterface>> Materials;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Flames;
    UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> FlameLights;
    // as definições das armas dos pedestais ficam seguras aqui: só com o ponteiro fraco do
    // pedestal, a coleta de lixo levava as que não estavam na mão (e nenhuma arma podia mais ser pega)
    UPROPERTY(Transient) TArray<TObjectPtr<UVortexWeaponDefinition>> KeptDefinitions;

    void Build();
    void BuildGrounds();
    void BuildArena();
    void BuildStands();
    void BuildFacade();
    /** Um grupo de pedestais (uma ala: marcial, distância, arcana, exótica) na borda da arena. */
    void BuildWing(FName Wing, float Angle);
    void BuildExit();
    void Gate(float Angle, float From, float To, const FLinearColor& Color, float WallH = 5.6f);
    void BuildPedestal(FName Type, const FVector& At, const FVector2D& Face, const FLinearColor& Color);

    // peças (em metros, relativas a Origin; ângulo em graus: 0 = norte (+Y), 90 = leste (+X))
    UMaterialInterface* Material(FName Name);
    UMaterialInterface* Cloth(const FLinearColor& Color);
    UInstancedStaticMeshComponent* Batch(FName Shape, UMaterialInterface* Material, bool bSolid);
    /** Caixa: Size = (ao longo da tangente, altura, ao longo do raio); Facing = para onde o "raio" aponta. */
    void Box(UMaterialInterface* M, const FVector& Center, const FVector& Size, float FacingDeg, bool bSolid = false);
    void Cylinder(UMaterialInterface* M, const FVector& Center, float Radius, float Height, bool bSolid = false);
    void Cone(UMaterialInterface* M, const FVector& Base, float Radius, float Height);
    void Column(UMaterialInterface* Shaft, const FVector& Foot, float Height, float Radius, bool bSolid);
    void Arch(UMaterialInterface* M, const FVector& Center, float FacingDeg, float Span, float Thickness, float Depth, int32 Pieces = 9);
    void Brazier(const FVector& At, const FLinearColor& LightColor);
    void Sign(const FVector& At, float FacingDeg, const FString& Text, float Size, const FLinearColor& Color);

    static FVector Dir(float Degrees) { const float A = FMath::DegreesToRadians(Degrees); return FVector(FMath::Sin(A), FMath::Cos(A), 0.f); }
    static FVector M(const FVector& Meters) { return Meters * 100.f; }
    static float GateDistance(float Degrees, float Radius);

    TArray<FPedestal> Pedestals;
    FVector ExitAt = FVector::ZeroVector;
    FName HeldType;
    float Clock = 0.f;
};
