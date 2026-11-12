#pragma once

#include "GameFramework/PlayerController.h"
#include "VortexPlayerController.generated.h"

UCLASS()
class VORTEX_API AVortexPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnRep_Pawn() override;
    virtual void SetupInputComponent() override;

private:
    void ToggleVortexPause();
    double LastPauseEscapePress = -1.0;
};
