#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "VortexDayNightSubsystem.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class USkyAtmosphereComponent;
class APostProcessVolume;

/**
 * Dia e noite na tela (Fase 15), em cada máquina que desenha (não no servidor dedicado), só em
 * mapas com terreno. A hora vem de AVortexGameState; as cores e o arco seguem o DayNight.cs do
 * Godot:
 * - uma luz só: sol acima do horizonte, lua (azulada e fraca) abaixo; elevação mínima de 8° para
 *   as sombras não esticarem; o giro só é aplicado a cada 0,3° (sombras virtuais refeitas raramente);
 * - céu: o SkyAtmosphere da Unreal segue a luz; à noite o brilho do céu cai e fica azul-escuro;
 * - luz ambiente: o SkyLight (captura em tempo real) ganha o tom do Godot (dia, dourado, noite);
 * - névoa: a cor da Fase 12 de dia, dourada no nascer/pôr, azul-escura à noite;
 * - exposição fixa (como o Godot), para a noite não clarear sozinha;
 * - olhar para o sol: raios de luz entre as árvores (light shafts), reflexo de lente e brilho,
 *   mais fortes e dourados no nascer e no pôr do sol, quase nada à noite. Folhas e morros na
 *   frente do sol cortam os raios e o reflexo (como o flare com oclusão do Godot).
 * - noite: uma cúpula grande em volta da câmera (M_NightSky) desenha estrelas que piscam e a lua
 *   com fases (ciclo de MoonCycleDays dias, começando cheia) e halo; a luz da lua acompanha a
 *   fase e faz raios suaves e azulados entre as árvores.
 * - nuvens volumétricas realistas (VolumetricCloud da Unreal), iluminadas pela mesma luz de sol/lua;
 *   a névoa só vale até FogCutoff (1 km), para céu, nuvens e estrelas ficarem limpos; abaixo do
 *   horizonte, depois do fim do terreno, uma "saia" plana na cor da névoa esconde o chão escuro
 *   do céu.
 * -VortexNoDayNight desliga (mantém a luz do mapa); -VortexNoClouds tira só as nuvens.
 */
UCLASS()
class VORTEXWORLD_API UVortexDayNightSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; }

    bool IsActive() const { return bActive; }
    float GetDaylight() const { return Daylight; }
    float GetGolden() const { return Golden; }
    FVector GetSunDirection() const { return SunDirection; }
    int32 LightTurns = 0;   // quantas vezes a direção da luz foi aplicada (medição)

    // luminância fixa da exposição (o projeto usa a faixa linear antiga, não EV100): menor =
    // mais claro. 0,5 dá ao meio-dia o mesmo brilho da exposição automática da Fase 12.
    float ExposureLuminance = 0.5f;
    float SunGlare = 1.f;   // multiplicador dos efeitos ao olhar o sol (-VortexSunGlare=)
    float MoonCycleDays = 8.f;
    // 0 = lua nova, 0,5 = cheia; parte iluminada 0..1
    float GetMoonPhase() const { return MoonPhase; }
    float GetMoonLit() const { return 0.5f - 0.5f * FMath::Cos(MoonPhase * UE_TWO_PI); }

private:
    TWeakObjectPtr<UDirectionalLightComponent> Sun;
    TWeakObjectPtr<USkyLightComponent> Sky;
    TWeakObjectPtr<UExponentialHeightFogComponent> Fog;
    TWeakObjectPtr<USkyAtmosphereComponent> Atmosphere;
    TWeakObjectPtr<APostProcessVolume> Grade;
    UPROPERTY(Transient)
    TObjectPtr<class AStaticMeshActor> NightDome;
    UPROPERTY(Transient)
    TObjectPtr<class UMaterialInstanceDynamic> NightSky;
    UPROPERTY(Transient)
    TObjectPtr<class AVolumetricCloud> Clouds;
    UPROPERTY(Transient)
    TObjectPtr<class AStaticMeshActor> HorizonSkirt;
    UPROPERTY(Transient)
    TObjectPtr<class UMaterialInstanceDynamic> SkirtMaterial;
    float FogCutoff = 100000.f;   // cm
    float MoonPhase = 0.5f;
    bool bSearched = false, bActive = false;
    float SmokeLogTime = 0.f;
    float Daylight = 1.f, Golden = 0.f;
    float BaseSunIntensity = 6.f, BaseSkyIntensity = 1.15f, SunSourceAngle = .5357f;
    FLinearColor BaseFogColor = FLinearColor(0.40f, 0.48f, 0.58f);
    FVector SunDirection = FVector::UpVector;
    FVector AppliedLightDirection = FVector::ZeroVector;
    bool bAppliedSunUp = false;

    bool FindActors();
    void Apply(float Clock, int32 Day);
    void UpdateNightSky(const FVector& Moon);
};
