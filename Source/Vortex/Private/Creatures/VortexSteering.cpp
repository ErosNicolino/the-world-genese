#include "Creatures/VortexSteering.h"

#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/Character.h"

namespace VortexSteering
{
    namespace
    {
        // Bate em algo que não é chão (árvore, pedra, parede) ao varrer a cápsula naquela direção?
        bool Blocked(const ACharacter* Self, const FVector& Direction, float LookAhead)
        {
            const UCapsuleComponent* Capsule = Self->GetCapsuleComponent();
            const float Radius = Capsule->GetScaledCapsuleRadius() * .9f;
            const float Half = Capsule->GetScaledCapsuleHalfHeight();
            // sobe um pouco: degraus e o próprio chão em rampa não contam como obstáculo
            const FVector From = Self->GetActorLocation() + FVector(0.f, 0.f, FMath::Min(45.f, Half * .5f));
            const FVector To = From + Direction * LookAhead;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexSteer), false, Self);
            FHitResult Hit;
            const FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, FMath::Max(Radius, Half * .6f));
            if (!Self->GetWorld()->SweepSingleByChannel(Hit, From, To, FQuat::Identity, ECC_Pawn, Shape, Query)) return false;
            // heróis não são obstáculo para quem vai atacá-los; chão inclinado (normal para cima) também não
            if (Hit.GetActor() && Hit.GetActor()->IsA<AVortexCharacter>()) return false;
            if (Hit.ImpactNormal.Z > .7f) return false;
            return true;
        }
    }

    FVector Avoid(const ACharacter* Self, const FVector& Desired, float LookAhead, FMemory& Memory)
    {
        FVector Want = Desired.GetSafeNormal2D();
        if (!Self || Want.IsNearlyZero() || !Self->GetWorld()) return Want;
        if (!Blocked(Self, Want, LookAhead)) return Want;
        const double Now = Self->GetWorld()->GetTimeSeconds();
        const int32 First = Now < Memory.SideUntil ? Memory.Side : 1;
        for (const float Angle : { 30.f, 55.f, 80.f, 110.f, 140.f })
        {
            for (const int32 Side : { First, -First })
            {
                const FVector Try = Want.RotateAngleAxis(Angle * Side, FVector::UpVector);
                if (!Blocked(Self, Try, LookAhead * (Angle > 90.f ? .6f : 1.f)))
                {
                    Memory.Side = Side;
                    Memory.SideUntil = Now + 1.5;
                    return Try;
                }
            }
        }
        // cercado: anda de lado (melhor que ficar parado empurrando a árvore)
        return Want.RotateAngleAxis(90.f * First, FVector::UpVector);
    }

    FVector Separation(const ACharacter* Self, float Radius)
    {
        if (!Self || !Self->GetWorld()) return FVector::ZeroVector;
        TArray<FOverlapResult> Near;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexSeparation), false, Self);
        FCollisionObjectQueryParams Pawns(ECC_Pawn);
        Self->GetWorld()->OverlapMultiByObjectType(Near, Self->GetActorLocation(), FQuat::Identity, Pawns, FCollisionShape::MakeSphere(Radius), Query);
        FVector Push = FVector::ZeroVector;
        for (const FOverlapResult& Other : Near)
        {
            const AActor* Actor = Other.GetActor();
            if (!Actor || Actor == Self || Actor->IsA<AVortexCharacter>()) continue;
            FVector Away = Self->GetActorLocation() - Actor->GetActorLocation();
            Away.Z = 0.f;
            const float Distance = Away.Size();
            if (Distance < 1.f) Away = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f);
            Push += Away.GetSafeNormal() * (1.f - FMath::Clamp(Distance / Radius, 0.f, 1.f));
        }
        return Push.GetClampedToMaxSize(1.f);
    }

    bool Unstick(const ACharacter* Self, const FVector& Desired, float DeltaSeconds, FMemory& Memory, FVector& Out)
    {
        if (!Self || !Self->GetWorld()) return false;
        const double Now = Self->GetWorld()->GetTimeSeconds();
        if (Now < Memory.ForcedUntil) { Out = Memory.Forced; return true; }
        const FVector Here = Self->GetActorLocation();
        if (Desired.IsNearlyZero() || FVector::Dist2D(Here, Memory.LastPlace) > 30.f)
        {
            Memory.LastPlace = Here;
            Memory.StuckFor = 0.f;
            return false;
        }
        Memory.StuckFor += DeltaSeconds;
        if (Memory.StuckFor < 1.2f) return false;
        // preso: um passo firme de lado (e um pouco para trás) por 0,8 s, trocando o lado da próxima vez
        Memory.StuckFor = 0.f;
        Memory.Side = -Memory.Side;
        const FVector Ahead = Desired.GetSafeNormal2D();
        Memory.Forced = (FVector(-Ahead.Y, Ahead.X, 0.f) * Memory.Side - Ahead * .35f).GetSafeNormal();
        Memory.ForcedUntil = Now + .8;
        Out = Memory.Forced;
        return true;
    }
}
