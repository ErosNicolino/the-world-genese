#include "Framework/VortexPlayerController.h"
#include "Combat/VortexHitFx.h"
#include "Framework/VortexGameInstance.h"
#include "Framework/VortexHudBase.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexDialogue.h"
#include "Framework/VortexHeroWindow.h"
#include "Framework/VortexShopWindow.h"
#include "Framework/VortexSocialUi.h"
#include "Components/InputComponent.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"

void AVortexPlayerController::BeginPlay()
{
    Super::BeginPlay();
    // efeitos dos golpes e poderes carregados ao entrar (no editor o mapa não passa pelo aviso de carregamento)
    if (IsLocalController() && !GetWorld()->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu"))) VortexHitFx::Warmup(GetWorld());
    UE_LOG(LogTemp, Display, TEXT("Vortex framework: controller BeginPlay, local=%d, authority=%d"),
        IsLocalController() ? 1 : 0, HasAuthority() ? 1 : 0);
}

void AVortexPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    UE_LOG(LogTemp, Display, TEXT("Vortex framework: server possessed pawn=%s"), *GetNameSafe(InPawn));
}

void AVortexPlayerController::OnRep_Pawn()
{
    Super::OnRep_Pawn();
    UE_LOG(LogTemp, Display, TEXT("Vortex framework: client received pawn=%s"), *GetNameSafe(GetPawn()));
}

void AVortexPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (InputComponent) InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AVortexPlayerController::ToggleVortexPause);
}

void AVortexPlayerController::ToggleVortexPause()
{
    UVortexGameInstance* Instance = Cast<UVortexGameInstance>(GetGameInstance());
    if (!Instance) return;
    if (Instance->IsPauseMenuOpen())
    {
        const double Now = FPlatformTime::Seconds();
        if (LastPauseEscapePress >= 0.0 && Now - LastPauseEscapePress <= .65)
        {
            LastPauseEscapePress = -1.0;
            Instance->TogglePauseMenu();
        }
        else LastPauseEscapePress = Now;
        return;
    }
    LastPauseEscapePress = -1.0;
    // Fecha apenas a tela ativa; não deixa mapa, inventário ou modal atrás da pausa.
    if (VortexDialogue::IsOpen()) { VortexDialogue::Close(); return; }
    if (VortexShopWindow::IsOpen()) { VortexShopWindow::Close(); return; }
    if (VortexSocialUi::IsOpen()) { VortexSocialUi::Close(); return; }
    if (VortexHeroWindow::IsOpen()) { VortexHeroWindow::Close(); return; }
    if (AVortexHudBase* Hud = Cast<AVortexHudBase>(GetHUD()); Hud && Hud->IsWorldMapOpen())
    {
        Hud->ToggleWorldMap();
        return;
    }
    Instance->TogglePauseMenu();
}
