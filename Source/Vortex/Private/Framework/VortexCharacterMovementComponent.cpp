#include "Framework/VortexCharacterMovementComponent.h"
#include "Rpg/VortexArmor.h"
#include "Combat/VortexClassSkills.h"
#include "Framework/VortexCharacter.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "World/VortexWaterZone.h"

namespace
{
    // 05/11: o rolamento vai 15% mais longe no mesmo tempo (o usuário: "ampliar o deslocamento do rolamento")
    constexpr float RollReach = 1.15f;

    // Movimento salvo com os bits de corrida e esquiva e o estado da esquiva no início do passo
    // (para refazer o passo igual numa correção do servidor).
    class FSavedMove_Vortex : public FSavedMove_Character
    {
    public:
        uint8 bSavedWantsToSprint : 1;
        uint8 bSavedWantsToWalk : 1;
        uint8 bSavedWantsToDodge : 1;
        float SavedDodgeTimeLeft = 0.f;
        float SavedDodgeCooldownLeft = 0.f;
        float SavedDodgeChargeProgress = 0.f;
        int32 SavedDodgeCharges = 0;
        FVector SavedDodgeDirection = FVector::ForwardVector;
        uint8 SavedDodgeVisualDirection = 0;

        virtual void Clear() override
        {
            FSavedMove_Character::Clear();
            bSavedWantsToSprint = false;
            bSavedWantsToWalk = false;
            bSavedWantsToDodge = false;
            SavedDodgeTimeLeft = SavedDodgeCooldownLeft = SavedDodgeChargeProgress = 0.f;
            SavedDodgeCharges = 0;
            SavedDodgeDirection = FVector::ForwardVector;
            SavedDodgeVisualDirection = 0;
        }

        virtual uint8 GetCompressedFlags() const override
        {
            uint8 Flags = FSavedMove_Character::GetCompressedFlags();
            if (bSavedWantsToSprint) Flags |= FLAG_Custom_0;
            if (bSavedWantsToDodge) Flags |= FLAG_Custom_1;
            if (bSavedWantsToWalk) Flags |= FLAG_Custom_2;
            return Flags;
        }

        virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override
        {
            const FSavedMove_Vortex* Other = static_cast<const FSavedMove_Vortex*>(NewMove.Get());
            if (bSavedWantsToSprint != Other->bSavedWantsToSprint || bSavedWantsToWalk != Other->bSavedWantsToWalk || bSavedWantsToDodge || Other->bSavedWantsToDodge)
            {
                return false;
            }
            // durante a esquiva cada passo conta (a velocidade muda pela curva)
            if (SavedDodgeTimeLeft > 0.f || Other->SavedDodgeTimeLeft > 0.f)
            {
                return false;
            }
            return FSavedMove_Character::CanCombineWith(NewMove, Character, MaxDelta);
        }

        virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel,
            FNetworkPredictionData_Client_Character& ClientData) override
        {
            FSavedMove_Character::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
            const UVortexCharacterMovementComponent* Movement =
                Cast<UVortexCharacterMovementComponent>(Character->GetCharacterMovement());
            bSavedWantsToSprint = Movement->bWantsToSprint;
            bSavedWantsToWalk = Movement->bWantsToWalk;
            bSavedWantsToDodge = Movement->bWantsToDodge;
            SavedDodgeTimeLeft = Movement->DodgeTimeLeft;
            SavedDodgeCooldownLeft = Movement->DodgeCooldownLeft;
            SavedDodgeChargeProgress = Movement->DodgeChargeProgress;
            SavedDodgeCharges = Movement->DodgeCharges;
            SavedDodgeDirection = Movement->DodgeDirection;
            SavedDodgeVisualDirection = Movement->DodgeVisualDirection;
        }

        virtual void PrepMoveFor(ACharacter* Character) override
        {
            FSavedMove_Character::PrepMoveFor(Character);
            UVortexCharacterMovementComponent* Movement =
                Cast<UVortexCharacterMovementComponent>(Character->GetCharacterMovement());
            Movement->bWantsToSprint = bSavedWantsToSprint;
            Movement->bWantsToWalk = bSavedWantsToWalk;
            Movement->bWantsToDodge = bSavedWantsToDodge;
            Movement->DodgeTimeLeft = SavedDodgeTimeLeft;
            Movement->DodgeCooldownLeft = SavedDodgeCooldownLeft;
            Movement->DodgeChargeProgress = SavedDodgeChargeProgress;
            Movement->DodgeCharges = SavedDodgeCharges;
            Movement->DodgeDirection = SavedDodgeDirection;
            Movement->DodgeVisualDirection = SavedDodgeVisualDirection;
        }
    };

    class FNetworkPredictionData_Client_Vortex : public FNetworkPredictionData_Client_Character
    {
    public:
        explicit FNetworkPredictionData_Client_Vortex(const UCharacterMovementComponent& Movement)
            : FNetworkPredictionData_Client_Character(Movement)
        {
        }

        virtual FSavedMovePtr AllocateNewMove() override
        {
            return FSavedMovePtr(new FSavedMove_Vortex());
        }
    };
}

UVortexCharacterMovementComponent::UVortexCharacterMovementComponent()
{
    MaxWalkSpeed = WalkSpeed;
    DodgeCharges = MaxDodgeCharges;
}

float UVortexCharacterMovementComponent::GetMaxSpeed() const
{
    if (IsSwimmingInWater())
    {
        return SwimSpeed;
    }
    if (IsDodging())
    {
        const AVortexCharacter* Roller = Cast<AVortexCharacter>(CharacterOwner);
        return (DodgeBaseSpeed + DodgePeakExtraSpeed) * RollReach * (Roller ? Roller->GetRaceSpeed() : 1.f);
    }
    if (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking)
    {
        // bênção de velocidade das habilidades de classe (replicada: cliente e servidor andam igual)
        const AVortexCharacter* Hero = Cast<AVortexCharacter>(CharacterOwner);
        const float Blessing = Hero && Hero->GetClassSkills() ? 1.f + Hero->GetClassSkills()->SpeedBonus() : 1.f;
        // a cavalo: o galope (correr) e o trote, bem mais rápidos que a pé (o galope é o de antes de 01/11, 620 x 1,75)
        if (Hero && Hero->IsMounted()) return (bWantsToSprint ? WalkSpeed * 2.85f : WalkSpeed * 1.6f) * Blessing;
        // presa na teia: quase parada até cortar
        if (Hero && Hero->IsWebbed()) return WalkSpeed * .12f;
        // bebendo uma poção: só anda devagar
        if (Hero && Hero->IsDrinking()) return WalkSpeed * .4f;
        // água pela canela ou mais (antes do nado): anda mais devagar (igual no cliente e no servidor)
        float Wade = 0.f;
        const float Drag = WaterDepth(Wade) && Wade > 30.f ? FMath::GetMappedRangeValueClamped(FVector2f(30.f, SwimEnterDepth), FVector2f(.85f, .65f), Wade) : 1.f;
        // 02/11: a corrida a pé mais rápida (o usuário achou "meio lentinha"; +8% e +15% ainda curtos, agora +30%, com os
        // clipes no ritmo natural: passos longos de verdade); o andar e o cavalo iguais. O corpo não muda a velocidade
        // 03/11: 1,4 (era 1,3): corre um pouco mais, com o passo mais longo (VortexAnimInstance: Stride)
        // 05/11: 1,5 correndo (era 1,4) e 1,08 andando: passadas mais longas cobrem mais chão (com o Stride do
        // VortexAnimInstance, a animação ainda fica um pouco mais lenta que antes)
        constexpr float FootRun = 1.5f, FootWalk = 1.08f;
        const float Base = bWantsToSprint ? SprintSpeed * FootRun : bWantsToWalk ? StrollSpeed * FootWalk : WalkSpeed * FootRun;
        // andando, a raça de perna curta vai ainda mais devagar (o passo do anão é bem menor: a 1,2 m/s o andar tocava 2,2x)
        const float Race = Hero ? Hero->GetRaceSpeed() : 1.f;
        return Base * Blessing * Drag * Race * (bWantsToWalk && !bWantsToSprint ? Race : 1.f);
    }
    return Super::GetMaxSpeed();
}

void UVortexCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
    Super::UpdateFromCompressedFlags(Flags);
    bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
    bWantsToWalk = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;
    bWantsToDodge = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}

FNetworkPredictionData_Client* UVortexCharacterMovementComponent::GetPredictionData_Client() const
{
    if (!ClientPredictionData)
    {
        UVortexCharacterMovementComponent* Mutable = const_cast<UVortexCharacterMovementComponent*>(this);
        Mutable->ClientPredictionData = new FNetworkPredictionData_Client_Vortex(*this);
    }
    return ClientPredictionData;
}

void UVortexCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
    Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
    DodgeTimeLeft = FMath::Max(0.f, DodgeTimeLeft - DeltaSeconds);
    DodgeCooldownLeft = FMath::Max(0.f, DodgeCooldownLeft - DeltaSeconds);
    if (DodgeCharges < MaxDodgeCharges)
    {
        DodgeChargeProgress += DeltaSeconds;
        if (DodgeChargeProgress >= DodgeChargeRegen)
        {
            DodgeChargeProgress -= DodgeChargeRegen;
            ++DodgeCharges;
        }
    }
    else
    {
        DodgeChargeProgress = 0.f;
    }
    if (bWantsToDodge)
    {
        TryStartDodge();
    }
    // 06/11 (o usuário: "com o arco viro rápido e ele continua de costas, atiro para o outro lado"): mirando, o corpo
    // segue a câmera (bUseControllerRotationYaw); antes este laço religava o virar-para-onde-anda e os dois brigavam
    bOrientRotationToMovement = !IsDodging() && !(CharacterOwner && CharacterOwner->bUseControllerRotationYaw);
    // água: entra nadando quando fica fundo o bastante (andando ou caindo)
    float Depth = 0.f;
    if ((MovementMode == MOVE_Walking || MovementMode == MOVE_Falling) && WaterDepth(Depth) && Depth >= SwimEnterDepth)
    {
        DodgeTimeLeft = 0.f;
        SetMovementMode(MOVE_Custom, static_cast<uint8>(EVortexMovementMode::Swim));
    }
}

bool UVortexCharacterMovementComponent::WaterDepth(float& OutDepth) const
{
    const UVortexWaterSubsystem* Water = GetWorld() ? GetWorld()->GetSubsystem<UVortexWaterSubsystem>() : nullptr;
    if (!Water || !UpdatedComponent || !CharacterOwner)
    {
        return false;
    }
    const FVector Center = UpdatedComponent->GetComponentLocation();
    float Surface = 0.f;
    if (!Water->SurfaceAt(Center, Surface))
    {
        return false;
    }
    const float Sole = Center.Z - CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    OutDepth = Surface - Sole;
    return true;
}

float UVortexCharacterMovementComponent::WaterSurfaceZ() const
{
    float Depth = 0.f;
    const float Sole = UpdatedComponent && CharacterOwner
        ? UpdatedComponent->GetComponentLocation().Z - CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
    return WaterDepth(Depth) ? Sole + Depth : Sole;
}

void UVortexCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
    if (CustomMovementMode == static_cast<uint8>(EVortexMovementMode::Swim))
    {
        PhysSwim(DeltaTime, Iterations);
        return;
    }
    Super::PhysCustom(DeltaTime, Iterations);
}

// Nado: boia na superfície (mola até SwimFloatDepth), anda na direção apertada a SwimSpeed, Espaço
// sobe e, encostando numa borda baixa, sobe para fora pelo mesmo degrau do andar.
void UVortexCharacterMovementComponent::PhysSwim(float DeltaTime, int32 Iterations)
{
    if (DeltaTime < MIN_TICK_TIME)
    {
        return;
    }
    float Depth = 0.f;
    if (!WaterDepth(Depth) || Depth < SwimExitDepth)
    {
        // saindo da água (margem rasa): sem impulso para cima vindo da mola ou do degrau
        Velocity.Z = FMath::Min(Velocity.Z, 0.f);
        SetMovementMode(MOVE_Falling);
        StartNewPhysics(DeltaTime, Iterations);
        return;
    }
    // horizontal: a mesma aceleração/atrito do resto, com o teto de velocidade do nado
    const float VerticalVelocity = Velocity.Z;
    Velocity.Z = 0.f;
    CalcVelocity(DeltaTime, 4.f, true, 1200.f);
    // vertical: mola até a profundidade de boiar, com amortecimento; Espaço sobe
    float Target = SwimFloatDepth;
    if (CharacterOwner && CharacterOwner->bPressedJump)
    {
        Target = SwimExitDepth + 5.f;
    }
    // fundo demais (Error > 0) sobe; raso demais desce
    const float Error = Depth - Target;
    float NewVertical = VerticalVelocity + (Error * 9.f - VerticalVelocity * 4.f) * DeltaTime;
    Velocity.Z = FMath::Clamp(NewVertical, -SwimUpSpeed, SwimUpSpeed);

    const FVector OldLocation = UpdatedComponent->GetComponentLocation();
    const FVector Delta = Velocity * DeltaTime;
    FHitResult Hit(1.f);
    SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
    if (Hit.IsValidBlockingHit())
    {
        // borda baixa: sobe como um degrau (sai da água andando)
        const FVector GravityDir(0.f, 0.f, -1.f);
        if (!(CanStepUp(Hit) && StepUp(GravityDir, Delta * (1.f - Hit.Time), Hit)))
        {
            HandleImpact(Hit, DeltaTime, Delta);
            SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
        }
    }
    if (!bJustTeleported && DeltaTime > 0.f)
    {
        // o deslize nas paredes vale para o horizontal; o vertical continua o da mola (subir um
        // degrau da margem não vira velocidade para cima)
        const FVector Moved = (UpdatedComponent->GetComponentLocation() - OldLocation) / DeltaTime;
        Velocity.X = Moved.X;
        Velocity.Y = Moved.Y;
    }
}

void UVortexCharacterMovementComponent::TryStartDodge()
{
    // o pedido vale para um passo: começou ou não, ele é consumido (no movimento salvo o bit
    // continua marcado para o servidor e para a repetição)
    bWantsToDodge = false;
    if (IsDodging() || DodgeCooldownLeft > 0.f || DodgeCharges <= 0 || !IsMovingOnGround())
    {
        return;
    }
    FVector Direction = GetCurrentAcceleration();
    Direction.Z = 0.f;
    const FRotator CameraYaw(0.f, CharacterOwner ? CharacterOwner->GetControlRotation().Yaw : GetOwner()->GetActorRotation().Yaw, 0.f);
    const FVector CameraForward = CameraYaw.RotateVector(FVector::ForwardVector);
    const FVector CameraRight = CameraYaw.RotateVector(FVector::RightVector);
    if (!Direction.Normalize())
    {
        Direction = CameraForward;
    }
    const float Forward = FVector::DotProduct(Direction, CameraForward);
    const float Right = FVector::DotProduct(Direction, CameraRight);
    DodgeVisualDirection = FMath::Abs(Right) > FMath::Abs(Forward) ? (Right > 0.f ? 3 : 2) : (Forward < 0.f ? 1 : 0);
    DodgeDirection = Direction;
    if (CharacterOwner) CharacterOwner->SetActorRotation(CameraYaw);
    bOrientRotationToMovement = false;
    DodgeTimeLeft = DodgeDuration;
    DodgeCooldownLeft = DodgeCooldown * VortexArmor::DodgeCooldownScale(GetOwner());   // 07/11: as armaduras de esquiva
    --DodgeCharges;
    // numa correção o cliente refaz passos antigos: só conta a esquiva de verdade
    if (!CharacterOwner || !CharacterOwner->bClientUpdating)
    {
        ++DodgesStarted;
        VortexArmor::OnDodge(GetOwner());
    }
}

void UVortexCharacterMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
    if (IsDodging() && IsMovingOnGround())
    {
        // mesma curva do Godot: começa em 4,4 m/s, pico de 9 m/s no meio, volta no fim
        const float Phase = 1.f - DodgeTimeLeft / DodgeDuration;
        // a raça de perna curta rola mais curto (o anão a 80%)
        const AVortexCharacter* Roller = Cast<AVortexCharacter>(CharacterOwner);
        const float Speed = (DodgeBaseSpeed + DodgePeakExtraSpeed * FMath::Sin(Phase * PI)) * RollReach * (Roller ? Roller->GetRaceSpeed() : 1.f);
        Velocity = DodgeDirection * Speed + FVector(0.f, 0.f, Velocity.Z);
        return;
    }
    Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
    // 02/11: a cavalo, no ar, nunca mais rápido que o galope. Pulando sem parar (como o "bunny hop"), cada pouso durava
    // pouco para o atrito frear e a velocidade do ar somava, ainda mais na diagonal
    const AVortexCharacter* Rider = Cast<AVortexCharacter>(CharacterOwner);
    if (IsFalling() && Rider && Rider->IsMounted())
    {
        const float Blessing = Rider->GetClassSkills() ? 1.f + Rider->GetClassSkills()->SpeedBonus() : 1.f;
        const float Gallop = WalkSpeed * 2.85f * Blessing;
        const FVector Flat(Velocity.X, Velocity.Y, 0.f);
        if (Flat.SizeSquared() > Gallop * Gallop)
        {
            const FVector Capped = Flat.GetSafeNormal() * Gallop;
            Velocity.X = Capped.X;
            Velocity.Y = Capped.Y;
        }
    }
}
