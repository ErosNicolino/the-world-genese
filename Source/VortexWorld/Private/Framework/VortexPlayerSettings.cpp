#include "Framework/VortexPlayerSettings.h"

#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"

void UVortexPlayerSettings::PostInitProperties()
{
    Super::PostInitProperties();
    if (!HasAnyFlags(RF_ClassDefaultObject) || !GConfig) return;
    const FString& File = GGameUserSettingsIni;
    const FString Old = TEXT("/Script/Vortex.VortexPlayerSettings");
    const FString New = GetClass()->GetPathName();
    if (GConfig->DoesSectionExist(*New, File) || !GConfig->DoesSectionExist(*Old, File)) return;
    TArray<FString> Lines;
    GConfig->GetSection(*Old, Lines, File);
    for (const FString& Line : Lines)
    {
        FString Key, Value;
        if (Line.Split(TEXT("="), &Key, &Value)) GConfig->SetString(*New, *Key, *Value, File);
    }
    GConfig->Flush(false, File);
    LoadConfig();
    UE_LOG(LogTemp, Display, TEXT("VORTEX_SETTINGS opções copiadas de %s (%d)"), *Old, Lines.Num());
}

void UVortexPlayerSettings::SetMouseSensitivity(float Value)
{
    // passos de 0,1 sem acumular erro de arredondamento
    MouseSensitivity = FMath::Clamp(FMath::RoundToFloat(Value * 100.f) / 100.f, MinSensitivity, MaxSensitivity);
    SaveConfig();
}

void UVortexPlayerSettings::SetAimSensitivityScale(float Value)
{
    AimSensitivityScale = FMath::Clamp(Value, 0.1f, 2.f);
    SaveConfig();
}

void UVortexPlayerSettings::SetInvertMouseY(bool bInvert)
{
    bInvertMouseY = bInvert;
    SaveConfig();
}

void UVortexPlayerSettings::CycleMinimapRadius()
{
    MinimapRadius = MinimapRadius < 80.f ? 100.f : MinimapRadius < 130.f ? 160.f : 60.f;
    SaveConfig();
}

void UVortexPlayerSettings::SetShowHud(bool bShow)
{
    bShowHud = bShow;
    SaveConfig();
}

namespace
{
    FAutoConsoleCommand HudCommand(TEXT("Vortex.HUD"),
        TEXT("Mostra (1) ou esconde (0) a HUD. Ex.: Vortex.HUD 0"),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            UVortexPlayerSettings* Settings = UVortexPlayerSettings::Get();
            Settings->SetShowHud(Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !Settings->bShowHud);
            UE_LOG(LogTemp, Display, TEXT("Vortex.HUD = %d"), Settings->bShowHud ? 1 : 0);
        }));

    // Sem argumento, mostra o valor atual; com argumento, muda e grava.
    FAutoConsoleCommand SensitivityCommand(TEXT("Vortex.Mouse.Sensibilidade"),
        TEXT("Sensibilidade do mouse (0,1 a 3; 1 = velocidade original). Ex.: Vortex.Mouse.Sensibilidade 0.5"),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            if (Args.Num() > 0) UVortexPlayerSettings::Get()->SetMouseSensitivity(FCString::Atof(*Args[0]));
            UE_LOG(LogTemp, Display, TEXT("Vortex.Mouse.Sensibilidade = %.2f"), UVortexPlayerSettings::Get()->MouseSensitivity);
        }));

    FAutoConsoleCommand AimCommand(TEXT("Vortex.Mouse.Mira"),
        TEXT("Multiplicador da sensibilidade enquanto mira (0,1 a 2). Ex.: Vortex.Mouse.Mira 0.7"),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            if (Args.Num() > 0) UVortexPlayerSettings::Get()->SetAimSensitivityScale(FCString::Atof(*Args[0]));
            UE_LOG(LogTemp, Display, TEXT("Vortex.Mouse.Mira = %.2f"), UVortexPlayerSettings::Get()->AimSensitivityScale);
        }));

    FAutoConsoleCommand InvertCommand(TEXT("Vortex.Mouse.InverterY"),
        TEXT("Inverte o eixo vertical do mouse (0 ou 1)."),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            if (Args.Num() > 0) UVortexPlayerSettings::Get()->SetInvertMouseY(FCString::Atoi(*Args[0]) != 0);
            UE_LOG(LogTemp, Display, TEXT("Vortex.Mouse.InverterY = %d"), UVortexPlayerSettings::Get()->bInvertMouseY ? 1 : 0);
        }));
}
