#include "World/VortexDayNightSubsystem.h"
#include "World/VortexWindTrails.h"
#include "World/VortexFallingLeaves.h"
#include "World/VortexFairies.h"
#include "World/VortexWorldHooks.h"
#include "World/VortexWeatherSubsystem.h"
#include "World/VortexWorldLayout.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Framework/VortexPlayerSettings.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "World/VortexTerrainStreamer.h"
#include "Components/VolumetricCloudComponent.h"
#include "World/VortexForestLife.h"
#include "World/VortexWildlife.h"

namespace
{
    FLinearColor Hex(const TCHAR* Value) { return FLinearColor(FColor::FromHex(Value)); }

    // DayNight.cs do Godot
    // o sol da Unreal já avermelha sozinho ao atravessar a atmosfera (SkyAtmosphere): sem o laranja
    // manual do Godot, que dobrava o vermelho no fim da tarde
    // luar, ambiente e névoa da noite menos saturados (era um azul-neon: areia, água e troncos ficavam azuis)
    const FLinearColor SunDay = Hex(TEXT("ffe7c2")), MoonLight = Hex(TEXT("b6c4e4"));
    const FLinearColor AmbientDay = Hex(TEXT("b8cbd8")), AmbientNight = Hex(TEXT("5f6b8e"));
    const FLinearColor FogGolden(0.62f, 0.48f, 0.38f), FogNight(0.04f, 0.05f, 0.08f);
}

bool UVortexDayNightSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

TStatId UVortexDayNightSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UVortexDayNightSubsystem, STATGROUP_Tickables);
}

bool UVortexDayNightSubsystem::FindActors()
{
    UWorld* World = GetWorld();
    if (World->GetNetMode() == NM_DedicatedServer || FParse::Param(FCommandLine::Get(), TEXT("VortexNoDayNight"))) return false;
    bool bTerrain = false;
    if (TActorIterator<AVortexTerrainStreamer> It(World); It) { bTerrain = true; }
    if (!bTerrain) return false;
    for (TActorIterator<ADirectionalLight> It(World); It; ++It)
    {
        UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(It->GetLightComponent());
        if (Light && (!Sun.IsValid() || Light->IsUsedAsAtmosphereSunLight())) Sun = Light;
    }
    if (TActorIterator<ASkyLight> It(World); It) { Sky = It->GetLightComponent(); }
    if (TActorIterator<AExponentialHeightFog> It(World); It) { Fog = It->GetComponent(); }
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (USkyAtmosphereComponent* Found = It->FindComponentByClass<USkyAtmosphereComponent>()) { Atmosphere = Found; break; }
    }
    if (TActorIterator<APostProcessVolume> It(World); It) { if (It->bUnbound) { Grade = *It; } }
    if (!Sun.IsValid()) return false;
    if (!FParse::Param(FCommandLine::Get(), TEXT("VortexNoClouds")))
    {
        FActorSpawnParameters CloudParams;
        CloudParams.ObjectFlags |= RF_Transient;
        // nuvens volumétricas realistas da Unreal (a direção de arte: mundo em blocos, céu realista);
        // a luz delas vem do sol/lua do SkyAtmosphere, sem cor manual. Usa a do mapa, se houver.
        if (TActorIterator<AVolumetricCloud> It(World); It) { Clouds = *It; }
        if (!Clouds) Clouds = World->SpawnActor<AVolumetricCloud>(FVector::ZeroVector, FRotator::ZeroRotator, CloudParams);
        if (UVolumetricCloudComponent* Layer = Clouds ? Clouds->FindComponentByClass<UVolumetricCloudComponent>() : nullptr)
        {
            // menos amostras por raio: ~metade do custo na placa de vídeo, quase sem diferença à vista
            Layer->SetViewSampleCountScale(.5f);
            Layer->SetReflectionViewSampleCountScale(.25f);
            Layer->SetShadowViewSampleCountScale(.5f);
            Layer->SetShadowReflectionViewSampleCountScale(.25f);
        }
    }
    if (!FParse::Param(FCommandLine::Get(), TEXT("VortexNoForestLife")))
    {
        // floresta viva: sons de ambiente e bichinhos em volta da câmera (segue a claridade daqui)
        FActorSpawnParameters LifeParams;
        LifeParams.ObjectFlags |= RF_Transient;
        World->SpawnActor<AVortexForestLife>(FVector::ZeroVector, FRotator::ZeroRotator, LifeParams);
        // pássaros, peixes, plantas da água e esporos (enfeite em cada máquina que desenha)
        if (!FParse::Param(FCommandLine::Get(), TEXT("VortexNoWildlife"))) World->SpawnActor<AVortexWildlife>(FVector::ZeroVector, FRotator::ZeroRotator, LifeParams);
        // 10/11: as fadas (enfeite, como os pássaros)
        if (!FParse::Param(FCommandLine::Get(), TEXT("VortexNoFairies"))) World->SpawnActor<AVortexFairies>(FVector::ZeroVector, FRotator::ZeroRotator, LifeParams);
        // 10/11: rastros de vento no ar (enfeite)
        if (!FParse::Param(FCommandLine::Get(), TEXT("VortexNoWindTrails"))) World->SpawnActor<AVortexWindTrails>(FVector::ZeroVector, FRotator::ZeroRotator, LifeParams);
        // 10/11: folhas caindo das árvores (enfeite)
        if (!FParse::Param(FCommandLine::Get(), TEXT("VortexNoFallingLeaves"))) World->SpawnActor<AVortexFallingLeaves>(FVector::ZeroVector, FRotator::ZeroRotator, LifeParams);
    }
    if (Fog.IsValid())
    {
        // névoa só até 1 km: o terreno some nela como antes, mas céu, nuvens e estrelas ficam limpos
        Fog->SetFogCutoffDistance(FogCutoff);
        // a névoa ganha a cor do próprio céu (SkyAtmosphere): a mata ao longe, a saia e o horizonte
        // têm o mesmo tom em qualquer hora, sem emenda
        Fog->SetSkyAtmosphereAmbientContributionColorScale(FLinearColor::White);
        UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
        UMaterialInterface* SkirtBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Sky/M_HorizonSkirt.M_HorizonSkirt"));
        if (Plane && SkirtBase)
        {
            FActorSpawnParameters SkirtParams;
            SkirtParams.ObjectFlags |= RF_Transient;
            HorizonSkirt = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, SkirtParams);
            UStaticMeshComponent* Mesh = HorizonSkirt->GetStaticMeshComponent();
            Mesh->SetMobility(EComponentMobility::Movable);
            Mesh->SetStaticMesh(Plane);
            Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Mesh->SetCastShadow(false);
            Mesh->bAffectDynamicIndirectLighting = false;
            Mesh->bAffectDistanceFieldLighting = false;
            Mesh->SetVisibleInRayTracing(false);
            Mesh->bVisibleInRealTimeSkyCaptures = false;
            // plano de 1 m -> 20 km de lado (até perto do horizonte), um pouco abaixo do fundo dos lagos
            HorizonSkirt->SetActorScale3D(FVector(20000.f, 20000.f, 1.f));
            SkirtMaterial = UMaterialInstanceDynamic::Create(SkirtBase, HorizonSkirt);
            Mesh->SetMaterial(0, SkirtMaterial);
        }
    }
    // o giro precisa de luz móvel; o mapa já vem assim (Tools/TuneDayNight.py), isto é só garantia
    if (Sun->Mobility != EComponentMobility::Movable) Sun->SetMobility(EComponentMobility::Movable);
    Sun->SetUseTemperature(false);
    // raios de luz: a máscara de oclusão escurece onde troncos e copas tapam o sol
    Sun->bEnableLightShaftOcclusion = true;
    Sun->SetEnableLightShaftBloom(true);
    Sun->SetOcclusionMaskDarkness(.35f);
    Sun->SetBloomThreshold(.6f);
    Sun->SetBloomMaxBrightness(.9f);
    // sombras de contato (na tela, baratas): pés, pedras e troncos "assentam" no chão e na grama
    Sun->ContactShadowLength = .025f;
    Sun->MarkRenderStateDirty();
    float GlareArg = 0.f;
    if (FParse::Value(FCommandLine::Get(), TEXT("VortexSunGlare="), GlareArg)) SunGlare = GlareArg;
    BaseSunIntensity = Sun->Intensity;
    SunSourceAngle = Sun->LightSourceAngle;
    if (Sky.IsValid()) BaseSkyIntensity = Sky->Intensity;
    if (Fog.IsValid()) BaseFogColor = Fog->FogInscatteringLuminance;
    float EV = 0.f;
    if (FParse::Value(FCommandLine::Get(), TEXT("VortexExposure="), EV)) ExposureLuminance = EV;
    if (APostProcessVolume* Volume = Grade.Get())
    {
        FPostProcessSettings& Settings = Volume->Settings;
        Settings.bOverride_AutoExposureMinBrightness = Settings.bOverride_AutoExposureMaxBrightness = true;
        Settings.bOverride_AutoExposureBias = true;
        Settings.AutoExposureMinBrightness = Settings.AutoExposureMaxBrightness = ExposureLuminance;
        Settings.AutoExposureBias = 0.f;
        Settings.bOverride_LensFlareIntensity = Settings.bOverride_LensFlareTint = Settings.bOverride_LensFlareThreshold = true;
        Settings.bOverride_BloomIntensity = true;
    }
    return true;
}

void UVortexDayNightSubsystem::Tick(float DeltaTime)
{
#if !UE_BUILD_SHIPPING
    // teste de rede: o cliente registra a hora e o relógio para o instante T = 1000 s de servidor
    static const bool bSmoke = FParse::Param(FCommandLine::Get(), TEXT("VortexDayNightSmoke"));
    if (bSmoke && GetWorld()->GetNetMode() == NM_Client)
    {
        SmokeLogTime += DeltaTime;
        const FString Line = VortexWorldHooks::Handlers().DayNightSmokeLine ? VortexWorldHooks::Handlers().DayNightSmokeLine(GetWorld()) : FString();
        // em todo quadro (02/11: o cliente travava uns segundos montando os efeitos e o servidor fechava antes de ele
        // somar 1 s; o teste lê a última linha, que já tem o relógio replicado)
        if (!Line.IsEmpty())
        {
            SmokeLogTime = 0.f;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_DAYNIGHT_CLIENT %s"), *Line);
        }
    }
#endif
    // o relógio vem do AVortexGameState (do jogo: VortexWorldHooks::DayClock)
    float Clock = 0.f;
    int32 Day = 0;
    const bool bClock = VortexWorldHooks::DayClock(GetWorld(), Clock, Day);
    if (!bSearched)
    {
        if (!bClock) return;
        bSearched = true;
        bActive = FindActors();
    }
    if (!bActive || !Sun.IsValid()) return;
    if (bClock) Apply(Clock, Day);
}

// Cúpula de 20 km em volta da câmera: atrás de tudo (a névoa fecha em ~130 m), aditiva, sem sombra.
void UVortexDayNightSubsystem::UpdateNightSky(const FVector& Moon)
{
    const float Night = 1.f - Daylight;
    const bool bVisible = Night > .01f;
    if (!NightDome && bVisible)
    {
        UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Sky/M_NightSky.M_NightSky"));
        if (!Sphere || !Material) return;
        FActorSpawnParameters Params;
        Params.ObjectFlags |= RF_Transient;
        NightDome = GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (!NightDome) return;
        UStaticMeshComponent* Mesh = NightDome->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(Sphere);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
        Mesh->bAffectDynamicIndirectLighting = false;
        Mesh->bAffectDistanceFieldLighting = false;
        Mesh->SetVisibleInRayTracing(false);
        Mesh->bVisibleInReflectionCaptures = false;
        Mesh->bVisibleInRealTimeSkyCaptures = false;
        NightDome->SetActorScale3D(FVector(40000.f));   // esfera de 50 cm de raio -> 20 km
        NightSky = UMaterialInstanceDynamic::Create(Material, NightDome);
        Mesh->SetMaterial(0, NightSky);
    }
    if (!NightDome) return;
    NightDome->SetActorHiddenInGame(!bVisible);
    if (!bVisible || !NightSky) return;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (It->Get() && It->Get()->IsLocalController() && It->Get()->PlayerCameraManager)
        {
            NightDome->SetActorLocation(It->Get()->PlayerCameraManager->GetCameraLocation());
            break;
        }
    }
    NightSky->SetVectorParameterValue(TEXT("MoonDir"), FLinearColor(Moon.X, Moon.Y, Moon.Z, 0.f));
    NightSky->SetScalarParameterValue(TEXT("Phase"), MoonPhase);
    // estrelas só com o céu já escuro (não no começo do amanhecer)
    NightSky->SetScalarParameterValue(TEXT("Night"), FMath::SmoothStep(.55f, .95f, Night));
    // o céu gira uma volta por noite inteira de jogo
    const float Spin = float(FMath::Fmod(GetWorld()->GetTimeSeconds() / VortexWorldLayout::NightSeconds, 1.0)) * UE_TWO_PI;
    NightSky->SetScalarParameterValue(TEXT("StarSpin"), Spin * .25f);
    NightSky->SetScalarParameterValue(TEXT("MoonVisible"), FMath::SmoothStep(.1f, .7f, Night));
}

void UVortexDayNightSubsystem::Apply(float Clock, int32 Day)
{
    // zona do chefe (a Caldeira): o céu, a luz e a névoa avermelham perto dela (e escurecem nas fases finais)
    float Red = 0.f;
    int32 BossPhase = 0;
    // a caldeira é do jogo (UVortexCalderaSubsystem): o vermelho e a fase vêm por VortexWorldHooks::CalderaSky
    {
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            if (It->Get() && It->Get()->IsLocalController() && It->Get()->PlayerCameraManager)
            {
                VortexWorldHooks::CalderaSky(GetWorld(), It->Get()->PlayerCameraManager->GetCameraLocation(), Red, BossPhase);
                break;
            }
        }
    }
    const float Dark = Red * (BossPhase >= 3 ? .35f : BossPhase == 2 ? .2f : 0.f);
    // 11/11: o clima (UVortexWeatherSubsystem): o céu fechado escurece o sol e acinzenta o céu; o relâmpago clareia
    const UVortexWeatherSubsystem* Weather = GetWorld()->GetSubsystem<UVortexWeatherSubsystem>();
    const float Overcast = Weather ? Weather->GetOvercast() : 0.f, Flash = Weather ? Weather->GetFlash() : 0.f;
    const float SunCut = 1.f - .78f * Overcast;
    // fase da lua: começa cheia no dia 1 e dá uma volta a cada MoonCycleDays dias
    MoonPhase = FMath::Frac((float(Day - 1) + Clock / VortexWorldLayout::CycleSeconds) / FMath::Max(1.f, MoonCycleDays) + .5f);
    // arco do sol: 0..π de dia, π..2π à noite; nasce a leste e passa inclinado 38° para o sul.
    // Godot (x, y para cima, z) vira Unreal (X = x, Y = z, Z = y).
    const float DaySeconds = VortexWorldLayout::DaySeconds, NightSeconds = VortexWorldLayout::NightSeconds;
    const float Angle = Clock < DaySeconds ? UE_PI * Clock / DaySeconds : UE_PI * (1.f + (Clock - DaySeconds) / NightSeconds);
    const float Tilt = FMath::DegreesToRadians(38.f);
    const FVector Godot(FMath::Cos(Angle), FMath::Sin(Angle) * FMath::Cos(Tilt), FMath::Sin(Angle) * FMath::Sin(Tilt) * .9f + .12f);
    SunDirection = FVector(Godot.X, Godot.Z, Godot.Y).GetSafeNormal();
    const FVector Moon = -SunDirection;
    Daylight = FMath::SmoothStep(-.14f, .16f, float(SunDirection.Z));
    Golden = Daylight * (1.f - FMath::SmoothStep(.06f, .42f, float(SunDirection.Z)));

    // uma luz só: sol de dia, lua de noite
    const bool bSunUp = SunDirection.Z > -.02f;
    const FVector From = bSunUp ? SunDirection : Moon;
    const FVector LightDirection = FVector(From.X, From.Y, FMath::Max(From.Z, .14)).GetSafeNormal();
    UDirectionalLightComponent* Light = Sun.Get();
    // 0,3° ≈ 3 s de jogo: cada giro refaz as sombras virtuais, então não gira todo quadro
    // Quase todo ajuste da luz (raios, disco, ângulo, brilho) recria a luz na cena e joga fora o cache
    // das sombras virtuais: só junto com o giro, ou quando o sol nasce/se põe.
    const bool bTurn = FVector::DotProduct(LightDirection, AppliedLightDirection) < FMath::Cos(FMath::DegreesToRadians(.3f));
    const bool bRefresh = bTurn || bSunUp != bAppliedSunUp;
    if (bTurn)
    {
        Light->SetWorldRotation((-LightDirection).Rotation());
        AppliedLightDirection = LightDirection;
        ++LightTurns;
    }
    bAppliedSunUp = bSunUp;
    const FLinearColor SunColor = FMath::Lerp(SunDay, FLinearColor(1.f, .46f, .26f), Red);
    if (bSunUp)
    {
        Light->SetLightColor(SunColor, false);
        Light->SetIntensity(BaseSunIntensity * FMath::SmoothStep(-.02f, .14f, float(SunDirection.Z)) * SunCut);
    }
    else
    {
        // a lua: no Godot 0,32 contra 1,2 do sol; aqui também segue a fase (nova = 25% da cheia)
        Light->SetLightColor(FMath::Lerp(MoonLight, FLinearColor(1.f, .35f, .2f), Red), false);
        // entra logo depois do pôr do sol (antes ficava um intervalo quase preto no crepúsculo)
        // 30/10: um pouco mais forte (0,32 -> 0,4; pedido: "quando escurece muito escuro não dá para ver nada")
        Light->SetIntensity(BaseSunIntensity * .4f * FMath::Lerp(.35f, 1.f, GetMoonLit()) * FMath::SmoothStep(-.06f, .12f, float(Moon.Z)) * SunCut);
    }
    if (bRefresh)
    {
        // raios saindo do disco de verdade (a luz em si fica com elevação mínima de 8° pelas sombras)
        Light->SetLightShaftOverrideDirection(-From);
        // o disco da atmosfera é o sol; à noite a lua é desenhada pela cúpula (com fase)
        Light->SetAtmosphereSunDiskColorScale(bSunUp ? FLinearColor::White : FLinearColor::Black);
        // sem isto o céu ainda desenhava um disco cinza no lugar da lua nova
        Light->SetLightSourceAngle(bSunUp ? SunSourceAngle : 0.f);
    }
    UpdateNightSky(Moon);
    if (HorizonSkirt)
    {
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            if (It->Get() && It->Get()->IsLocalController() && It->Get()->PlayerCameraManager)
            {
                const FVector Eye = It->Get()->PlayerCameraManager->GetCameraLocation();
                HorizonSkirt->SetActorLocation(FVector(Eye.X, Eye.Y, -400.f));
                break;
            }
        }
    }

    if (USkyLightComponent* Ambient = Sky.Get())
    {
        // a captura em tempo real já escurece com o céu; o tom e um reforço à noite seguem o Godot
        // (luz ambiente quase constante, azulada à noite)
        Ambient->SetLightColor(FMath::Lerp(FMath::Lerp(AmbientNight, AmbientDay, Daylight), FLinearColor(1.f, .4f, .26f), Red * .85f));
        // crepúsculo: o céu capturado escurece antes da lua subir; o reforço é maior aí
        // (janela mais larga e reforço maior: com o sol já abaixo do horizonte o chão ficava preto embaixo de um céu claro)
        const float Twilight = FMath::Clamp(1.f - FMath::Abs(float(SunDirection.Z) + .04f) / .3f, 0.f, 1.f);
        // sem a luz global (Lumen, desligada por padrão) a sombra só recebe o céu: reforço para não ficar escuro demais
        const float NoGiBoost = UVortexPlayerSettings::Get() && !UVortexPlayerSettings::Get()->bShaderGI ? 2.f : 1.f;
        // 30/10: o reforço da noite subiu (4,5 -> 9): a sombra sob as árvores e as casas ficava preta embaixo da lua
        Ambient->SetIntensity(BaseSkyIntensity * (FMath::Lerp(9.f, 1.f, Daylight) * NoGiBoost + 4.f * Twilight) * FMath::Lerp(1.f, .8f, Overcast) * (1.f + 7.f * Flash));
    }
    if (USkyAtmosphereComponent* Air = Atmosphere.Get())
    {
        // à noite a "luz do sol" do céu é a lua: o céu fica azul-escuro em vez de dia fraco
        // no crepúsculo o céu continua claro um pouco depois do pôr do sol (e a luz ambiente junto)
        const float SkyLight = FMath::Max(Daylight, .55f * FMath::Clamp(1.f - FMath::Abs(float(SunDirection.Z)) / .2f, 0.f, 1.f));
        // de dia, levemente mais claro (a névoa não cobre mais o céu: sem o reforço de azul de antes)
        FLinearColor Luminance = FMath::Lerp(FLinearColor(.12f, .15f, .3f), FLinearColor(1.05f, 1.05f, 1.1f), SkyLight);
        // Caldeira: céu de brasa (vermelho forte; mais escuro com o chefe furioso)
        Luminance = FMath::Lerp(Luminance, FLinearColor(1.7f, .32f, .16f) * FMath::Max(.35f, SkyLight), Red) * (1.f - Dark);
        // céu fechado: cinza e mais escuro; o relâmpago clareia o céu inteiro
        Luminance = FMath::Lerp(Luminance, FLinearColor(.5f, .53f, .58f) * FMath::Max(.2f, SkyLight), .75f * Overcast) * (1.f + 5.f * Flash);
        // (cada mudança recria o céu na cena: só quando muda de verdade)
        if (!Luminance.Equals(Air->SkyLuminanceFactor, .004f)) Air->SetSkyLuminanceFactor(Luminance);
    }
    // olhar para o sol (o reflexo e os raios só aparecem com o disco do sol na tela)
    const float Glare = SunGlare * (bSunUp ? FMath::SmoothStep(-.02f, .1f, float(SunDirection.Z)) : 0.f);
    // luar: raios fracos e azulados entre as copas, mais fortes na lua cheia
    const float Bloom = bSunUp ? Glare * (.25f + .55f * Golden) : SunGlare * .22f * GetMoonLit() * FMath::SmoothStep(.02f, .2f, float(Moon.Z));
    if (bRefresh || FMath::Abs(Bloom - Light->BloomScale) > .02f)
    {
        Light->SetBloomScale(Bloom);
        Light->SetBloomTint(bSunUp ? SunDay.ToFColor(true) : FLinearColor(.62f, .74f, 1.f).ToFColor(true));
    }
    if (APostProcessVolume* Volume = Grade.Get())
    {
        FPostProcessSettings& Settings = Volume->Settings;
        // exposição como o olho, numa faixa estreita em volta do valor calibrado (0,5): era fixa, e a neve e a
        // areia do meio-dia estouravam. O mínimo é o calibrado (cenas escuras ficam como antes, a noite não
        // vira dia); cenas muito claras escurecem até 1,8x. Abre um pouco:
        //  - na hora dourada (o sol atravessa muita atmosfera): até 1,55x (era 2x, deixava tudo amarelo);
        //  - no crepúsculo, logo depois do pôr do sol (a "hora azul"; antes 18:36 ficava quase preto);
        //  - à noite, como o olho acostumado ao escuro (+0,85 EV)
        const float Dusk = FMath::Clamp(1.f - FMath::Abs(float(SunDirection.Z) + .05f) / .17f, 0.f, 1.f) * (1.f - Daylight * .5f);
        const float Open = FMath::Max(.35f * Golden, .45f * Dusk);
        // 30/10: à noite o olho se acostuma mais (,55 -> ,36): dá para ver o caminho sem virar dia
        const float Night = FMath::Lerp(.36f, 1.f, Daylight);
        const float Floor = ExposureLuminance * (1.f - Open) * Night;
        // (no crepúsculo e à noite o céu claro na tela não escurece o resto: a faixa quase fecha)
        Settings.AutoExposureMaxBrightness = Floor * FMath::Lerp(1.12f, 1.8f, Daylight * Daylight);
        // à noite o piso desce mais: dentro da mata (bem mais escura que o campo aberto) o olho se acostuma até
        // ~1,4x mais; no campo aberto a exposição continua a mesma (pedido: "a mata à noite está escura demais")
        Settings.AutoExposureMinBrightness = Floor * FMath::Lerp(.72f, 1.f, Daylight);
        // um pouco menos de saturação: as copas viravam um verde-limão no sol forte e na hora dourada
        Settings.bOverride_ColorSaturation = true;
        Settings.ColorSaturation = FVector4(1.f, 1.f, 1.f, FMath::Lerp(.96f, .9f, Golden) * (1.f - .25f * Overcast));
        // opção Brilho (Opções > Gráficos > Imagem): o padrão clareia +0,8 EV o visual calibrado antigo
        Settings.AutoExposureBias = UVortexPlayerSettings::BrightnessEV(UVortexPlayerSettings::Get() ? UVortexPlayerSettings::Get()->Brightness : 3);
        Settings.LensFlareIntensity = Glare * (.8f + 1.2f * Golden);
        Settings.LensFlareTint = FMath::Lerp(FLinearColor(1.f, .95f, .85f), FLinearColor(1.f, .7f, .45f), Golden);
        Settings.LensFlareThreshold = 4.f;
        // shaders (Opções > Shaders) mais suaves (pedido de 23/10: "pesado e muito forte"): o brilho das luzes
        // em 60%; o desfoque de movimento curto; e, com a luz global, o céu vaza um pouco por baixo das copas
        // (sem isso a sombra das árvores ficava quase preta)
        const UVortexPlayerSettings* Shaders = UVortexPlayerSettings::Get();
        Settings.BloomIntensity = (FMath::Lerp(.5f, .675f + .3f * Golden, Daylight) + .45f * Red) * (Shaders && Shaders->bShaderBloom ? .6f : 1.f);
        Settings.bOverride_MotionBlurAmount = true;
        Settings.MotionBlurAmount = .25f;
        Settings.bOverride_MotionBlurMax = true;
        Settings.MotionBlurMax = 2.f;
        Settings.bOverride_LumenSkylightLeaking = true;
        // 07/11 (o usuário: "sem shaders dá para ver tudo bonito, com eles fica muito escuro"): com a luz global o céu
        // vaza mais nas sombras e a exposição sobe +0,6 EV (sem ela era o ambiente claro do céu, sem oclusão)
        const bool bGI = Shaders && Shaders->bShaderGI;
        Settings.LumenSkylightLeaking = bGI ? .6f : .3f;
        if (bGI) Settings.AutoExposureBias += .6f;
        Settings.bOverride_SceneColorTint = true;
        Settings.SceneColorTint = FMath::Lerp(FLinearColor::White, FLinearColor(1.f, .8f, .74f), Red) * (1.f - .5f * Dark);
    }
    if (UExponentialHeightFogComponent* Haze = Fog.Get())
    {
        const FLinearColor HazeColor = FMath::Lerp(FMath::Lerp(FogNight, FMath::Lerp(BaseFogColor, FogGolden, Golden), Daylight), FLinearColor(1.3f, .24f, .08f), Red);
        // a cor própria da névoa vira só um toque (o grosso vem do céu)
        if (!(HazeColor * .3f).Equals(Haze->FogInscatteringLuminance, .002f)) Haze->SetFogInscatteringColor(HazeColor * .3f);
    }
}
