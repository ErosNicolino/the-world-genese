#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "VortexCharacterMovementComponent.generated.h"

/**
 * Movimento do herói com corrida e esquiva previstas pela rede.
 *
 * Corrida e esquiva viajam como bits do próprio movimento salvo (FSavedMove), não como RPC
 * separado: cliente e servidor simulam o mesmo passo com a mesma entrada, e numa correção o
 * cliente refaz os passos pendentes com o estado da esquiva restaurado (tempo, direção,
 * recarga e cargas). Valores da esquiva copiados do Godot (PlayerController.BeginDash):
 * 0,58 s, 4,4 a 9 m/s numa curva de seno, recarga de 0,8 s, 5 cargas que voltam a cada 4 s,
 * só no chão.
 */
// Modos próprios de movimento (MOVE_Custom + este número).
UENUM()
enum class EVortexMovementMode : uint8
{
    Swim = 1,
};

UCLASS()
class VORTEX_API UVortexCharacterMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()

public:
    UVortexCharacterMovementComponent();

    UPROPERTY(EditAnywhere, Category = "Vortex|Movement")
    float WalkSpeed = 380.f;      // 01/11: 460 (as corridas do Mixamo tocavam rápido demais)

    UPROPERTY(EditAnywhere, Category = "Vortex|Movement")
    float SprintSpeed = 460.f;    // 01/11: 620 (o Shift é a corrida lenta do Mixamo, no ritmo dela; a rápida saiu)

    /** Andando (o normal; o Caps Lock troca para correr sempre, na WalkSpeed): devagar, com o clipe de andar. */
    UPROPERTY(EditAnywhere, Category = "Vortex|Movement")
    float StrollSpeed = 150.f;

    UPROPERTY(EditAnywhere, Category = "Vortex|Dodge")
    float DodgeDuration = 0.7f;        // 01/11: 0,58 s, 4,25 m (a cambalhota do Mixamo rola ~1,9 m: parecia teletransporte)

    UPROPERTY(EditAnywhere, Category = "Vortex|Dodge")
    float DodgeBaseSpeed = 335.f;   // 03/11: era 300 (o rolamento um pouco mais longo)

    UPROPERTY(EditAnywhere, Category = "Vortex|Dodge")
    float DodgePeakExtraSpeed = 260.f;

    UPROPERTY(EditAnywhere, Category = "Vortex|Dodge")
    float DodgeCooldown = 0.8f;

    UPROPERTY(EditAnywhere, Category = "Vortex|Dodge")
    int32 MaxDodgeCharges = 5;

    UPROPERTY(EditAnywhere, Category = "Vortex|Dodge")
    float DodgeChargeRegen = 4.f;

    // Entrada (lado do dono): o personagem liga/desliga e o movimento salvo leva junto.
    bool bWantsToSprint = false;
    bool bWantsToWalk = true;
    bool bWantsToDodge = false;

    // Estado da esquiva (simulado igual no cliente e no servidor).
    float DodgeTimeLeft = 0.f;
    float DodgeCooldownLeft = 0.f;
    float DodgeChargeProgress = 0.f;
    int32 DodgeCharges = 5;
    FVector DodgeDirection = FVector::ForwardVector;
    // Índice visual relativo à câmera no instante da esquiva: frente, trás, esquerda, direita.
    uint8 DodgeVisualDirection = 0;

    // ---- nado (água por dados: UVortexWaterSubsystem) ----
    // Entra nadando com cerca de 70% do corpo submerso; Godot usa SwimDepth * BodyHeight.
    UPROPERTY(EditAnywhere, Category = "Vortex|Swim")
    float SwimEnterDepth = 98.f;

    UPROPERTY(EditAnywhere, Category = "Vortex|Swim")
    float SwimExitDepth = 62.f;

    // Profundidade em que o corpo boia parado (cm abaixo da superfície, da sola).
    UPROPERTY(EditAnywhere, Category = "Vortex|Swim")
    float SwimFloatDepth = 98.f;

    UPROPERTY(EditAnywhere, Category = "Vortex|Swim")
    float SwimSpeed = 300.f;

    UPROPERTY(EditAnywhere, Category = "Vortex|Swim")
    float SwimUpSpeed = 260.f;

    bool IsSwimmingInWater() const { return MovementMode == MOVE_Custom && CustomMovementMode == static_cast<uint8>(EVortexMovementMode::Swim); }
    // Profundidade (cm) da sola abaixo da superfície da água; false = sem água aqui.
    bool WaterDepth(float& OutDepth) const;
    /** Altura da superfície da água aqui (sem água: a altura dos pés). */
    float WaterSurfaceZ() const;

    // Contador de esquivas iniciadas (testes e futura animação).
    int32 DodgesStarted = 0;

    bool IsDodging() const { return DodgeTimeLeft > 0.f; }
    bool IsSprinting() const { return bWantsToSprint && IsMovingOnGround() && !IsDodging(); }

    virtual float GetMaxSpeed() const override;
    virtual void UpdateFromCompressedFlags(uint8 Flags) override;
    virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
    virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
    virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
    virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

private:
    void TryStartDodge();
    void PhysSwim(float DeltaTime, int32 Iterations);
};
