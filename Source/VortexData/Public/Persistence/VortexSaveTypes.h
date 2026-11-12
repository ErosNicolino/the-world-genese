#pragma once

#include "CoreMinimal.h"
#include "VortexSaveTypes.generated.h"

// Stable IDs belong to saved entities. Session slots and network connection IDs must never replace them.
USTRUCT(BlueprintType)
struct VORTEXDATA_API FVortexIdentity
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex|Identity")
    FGuid PlayerId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex|Identity")
    FGuid CharacterId;

    bool IsValid() const { return PlayerId.IsValid() && CharacterId.IsValid(); }
};

// Estado de um NPC da população (Fase 14). Pertence ao ID estável do posto, não ao ator: o ator
// aparece e some conforme os jogadores se aproximam, o estado continua.
USTRUCT(BlueprintType)
struct VORTEXDATA_API FVortexNpcState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex|Population")
    FName NpcId;

    // Onde ele estava quando o ator sumiu (cm). Sem bPlaced, ainda está no posto de origem.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex|Population")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex|Population")
    float Yaw = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex|Population")
    int32 PatrolIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex|Population")
    int32 Materializations = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex|Population")
    bool bPlaced = false;

    bool operator==(const FVortexNpcState& Other) const
    {
        return NpcId == Other.NpcId && Location.Equals(Other.Location, 0.5) && FMath::IsNearlyEqual(Yaw, Other.Yaw, 0.1f) &&
            PatrolIndex == Other.PatrolIndex && Materializations == Other.Materializations && bPlaced == Other.bPlaced;
    }
};

USTRUCT(BlueprintType)
struct VORTEXDATA_API FVortexWorldDelta
{
    GENERATED_BODY()

    // Stable gameplay key, such as a placed structure or defeated boss ID.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    FName ObjectId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex|Save")
    FName StateId;
};
