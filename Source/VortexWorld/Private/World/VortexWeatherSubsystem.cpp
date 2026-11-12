#include "World/VortexWeatherSubsystem.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundBase.h"
#include "World/VortexDayNightSubsystem.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexWorldHooks.h"
#include "World/VortexWorldLayout.h"

namespace
{
    struct FTarget { float Overcast, Rain, Snow, Fog, Wind; };

    FTarget TargetOf(EVortexWeather W)
    {
        switch (W)
        {
        case EVortexWeather::Cloudy:   return { .45f, 0.f, 0.f, .1f, .35f };
        case EVortexWeather::Rain:     return { .72f, .6f, 0.f, .25f, .4f };
        case EVortexWeather::Storm:    return { .95f, 1.f, 0.f, .35f, 1.f };
        case EVortexWeather::Snow:     return { .6f, 0.f, .65f, .3f, .35f };
        case EVortexWeather::Blizzard: return { .85f, 0.f, 1.f, .6f, 1.f };
        case EVortexWeather::Fog:      return { .4f, 0.f, 0.f, 1.f, .1f };
        default:                       return { 0.f, 0.f, 0.f, 0.f, .2f };
        }
    }

    FTarget Lerp(const FTarget& A, const FTarget& B, float T)
    {
        return { FMath::Lerp(A.Overcast, B.Overcast, T), FMath::Lerp(A.Rain, B.Rain, T), FMath::Lerp(A.Snow, B.Snow, T),
            FMath::Lerp(A.Fog, B.Fog, T), FMath::Lerp(A.Wind, B.Wind, T) };
    }

    float HourOf(float Clock)
    {
        using namespace VortexWorldLayout;
        return Clock < DaySeconds ? 6.f + 12.f * Clock / DaySeconds : FMath::Fmod(18.f + 12.f * (Clock - DaySeconds) / NightSeconds, 24.f);
    }

    bool ForcedWeather(EVortexWeather& Out)
    {
        FString Name;
        if (!FParse::Value(FCommandLine::Get(), TEXT("VortexWeather="), Name)) return false;
        static const TPair<const TCHAR*, EVortexWeather> Names[] = { { TEXT("Clear"), EVortexWeather::Clear }, { TEXT("Cloudy"), EVortexWeather::Cloudy },
            { TEXT("Rain"), EVortexWeather::Rain }, { TEXT("Storm"), EVortexWeather::Storm }, { TEXT("Snow"), EVortexWeather::Snow },
            { TEXT("Blizzard"), EVortexWeather::Blizzard }, { TEXT("Fog"), EVortexWeather::Fog } };
        for (const auto& Entry : Names) if (Name.Equals(Entry.Key, ESearchCase::IgnoreCase)) { Out = Entry.Value; return true; }
        return false;
    }
}

bool UVortexWeatherSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

TStatId UVortexWeatherSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UVortexWeatherSubsystem, STATGROUP_Tickables);
}

const TCHAR* UVortexWeatherSubsystem::SeasonName(int32 InSeason)
{
    static const TCHAR* Names[] = { TEXT("Primavera"), TEXT("Verão"), TEXT("Outono"), TEXT("Inverno") };
    return Names[FMath::Clamp(InSeason, 0, 3)];
}

const TCHAR* UVortexWeatherSubsystem::WeatherName(EVortexWeather Weather)
{
    switch (Weather)
    {
    case EVortexWeather::Cloudy: return TEXT("Nublado");
    case EVortexWeather::Rain: return TEXT("Chuva");
    case EVortexWeather::Storm: return TEXT("Tempestade");
    case EVortexWeather::Snow: return TEXT("Neve");
    case EVortexWeather::Blizzard: return TEXT("Nevasca");
    case EVortexWeather::Fog: return TEXT("Neblina");
    default: return TEXT("Céu limpo");
    }
}

EVortexWeather UVortexWeatherSubsystem::Pick(int32 Window, int32 Region, int32 InSeason)
{
    if (Window < 4) return EVortexWeather::Clear;   // o dia 1 é sempre limpo
    FRandomStream Rng(HashCombine(GetTypeHash(Window * 7919 + 3), GetTypeHash(Region * 131 + 17)));
    const float X = Rng.FRand();
    using W = EVortexWeather;
    struct FChance { W Weather; float P; };
    TArray<FChance, TInlineAllocator<7>> Table;
    const bool bWinter = InSeason == 3;
    switch (Region)
    {
    case 5: Table = { { W::Clear, .86f }, { W::Cloudy, .1f }, { W::Storm, .04f } }; break;                      // Solcânion: deserto
    case 2: Table = { { W::Clear, .55f }, { W::Cloudy, .45f } }; break;                                         // Brasafenda: cinza do vulcão
    case 4: Table = { { W::Clear, .3f }, { W::Cloudy, .2f }, { W::Snow, .38f }, { W::Blizzard, .12f } }; break; // Nivaluz
    case 7: case 8: Table = { { W::Clear, .22f }, { W::Cloudy, .2f }, { W::Rain, .25f }, { W::Storm, .08f }, { W::Fog, .25f } }; break;
    default:
    {
        // Valdora, Raikor, Aurivale, Verdalume: pela estação (no inverno a chuva vira neve)
        static const float Seasons[4][7] = {
            // limpo nublado chuva tempestade neve nevasca neblina
            { .45f, .2f, .23f, .07f, 0.f, 0.f, .05f },
            { .62f, .15f, .1f, .1f, 0.f, 0.f, .03f },
            { .36f, .24f, .24f, .08f, 0.f, 0.f, .08f },
            { .36f, .24f, 0.f, 0.f, .24f, .06f, .1f } };
        const float* P = Seasons[FMath::Clamp(InSeason, 0, 3)];
        Table = { { W::Clear, P[0] }, { W::Cloudy, P[1] }, { W::Rain, P[2] }, { W::Storm, P[3] }, { W::Snow, P[4] }, { W::Blizzard, P[5] }, { W::Fog, P[6] } };
        // Raikor (montanhas): mais tempestade; neve já no outono
        if (Region == 3)
        {
            Table[3].P += .06f;
            if (InSeason == 2) { Table.Add({ W::Snow, .15f }); }
            if (bWinter) { Table[4].P += .12f; Table[5].P += .06f; }
        }
        break;
    }
    }
    float Sum = 0.f;
    for (const FChance& C : Table) Sum += C.P;
    float Acc = 0.f;
    for (const FChance& C : Table)
    {
        Acc += C.P / FMath::Max(Sum, .001f);
        if (X <= Acc) return C.Weather;
    }
    return Table.Last().Weather;
}

void UVortexWeatherSubsystem::Build()
{
    bBuilt = true;
    UWorld* World = GetWorld();
    UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UMaterialInterface* Sheet = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Weather/M_VortexPrecip.M_VortexPrecip"));
    UTexture* RainTex = LoadObject<UTexture>(nullptr, TEXT("/Game/Vortex/Weather/T_Rain.T_Rain"));
    UTexture* SnowTex = LoadObject<UTexture>(nullptr, TEXT("/Game/Vortex/Weather/T_Snow.T_Snow"));
    if (!Cylinder || !Sheet || !RainTex || !SnowTex)
    {
        UE_LOG(LogTemp, Warning, TEXT("VORTEX_WEATHER sem o material ou as texturas da chuva (Tools/ImportTownLife.py)"));
        return;
    }
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Fx = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
    if (!Fx) return;
    USceneComponent* Root = NewObject<USceneComponent>(Fx, TEXT("Root"));
    Root->SetMobility(EComponentMobility::Movable);
    Fx->SetRootComponent(Root);
    Root->RegisterComponent();
    // (raio cm, altura cm, textura, repetição, velocidade em UV/s, balanço)
    auto Layer = [&](const TCHAR* Name, float Radius, float Height, UTexture* Tex, FVector2D Tiling, FVector2D Speed, float Sway) -> UStaticMeshComponent*
    {
        UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Fx, Name);
        Mesh->SetStaticMesh(Cylinder);
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
        Mesh->bAffectDistanceFieldLighting = false;
        Mesh->bAffectDynamicIndirectLighting = false;
        Mesh->bVisibleInReflectionCaptures = false;
        Mesh->bVisibleInRealTimeSkyCaptures = false;
        Mesh->SetVisibleInRayTracing(false);
        Mesh->SetupAttachment(Root);
        Mesh->SetRelativeScale3D(FVector(Radius / 50.f, Radius / 50.f, Height / 100.f));
        Mesh->SetRelativeLocation(FVector(0.f, 0.f, Height * .2f));
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Sheet, Fx);
        Material->SetTextureParameterValue(TEXT("Tex"), Tex);
        Material->SetVectorParameterValue(TEXT("Tiling"), FLinearColor(Tiling.X, Tiling.Y, 0.f, 0.f));
        Material->SetVectorParameterValue(TEXT("Speed"), FLinearColor(Speed.X, Speed.Y, 0.f, 0.f));
        Material->SetScalarParameterValue(TEXT("Sway"), Sway);
        Material->SetScalarParameterValue(TEXT("Intensity"), 0.f);
        Mesh->SetMaterial(0, Material);
        Sheets.Add(Material);
        Mesh->SetVisibility(false);
        Mesh->RegisterComponent();
        return Mesh;
    };
    RainNear = Layer(TEXT("RainNear"), 500.f, 1800.f, RainTex, FVector2D(9.f, 3.f), FVector2D(0.f, 1.9f), 0.f);
    RainFar = Layer(TEXT("RainFar"), 1600.f, 2800.f, RainTex, FVector2D(24.f, 5.f), FVector2D(0.f, 1.3f), 0.f);
    SnowNear = Layer(TEXT("SnowNear"), 500.f, 1800.f, SnowTex, FVector2D(6.f, 3.f), FVector2D(0.f, .16f), .025f);
    SnowFar = Layer(TEXT("SnowFar"), 1600.f, 2800.f, SnowTex, FVector2D(16.f, 5.f), FVector2D(0.f, .1f), .015f);
    auto Loop = [&](const TCHAR* Path) -> UAudioComponent*
    {
        USoundBase* Sound = LoadObject<USoundBase>(nullptr, Path);
        if (!Sound) return nullptr;
        UAudioComponent* Audio = NewObject<UAudioComponent>(Fx);
        Audio->SetSound(Sound);
        Audio->bAllowSpatialization = false;
        Audio->bAutoActivate = false;
        Audio->SetupAttachment(Root);
        Audio->RegisterComponent();
        Audio->SetVolumeMultiplier(.001f);
        Audio->Play();
        return Audio;
    };
    RainLoop = Loop(TEXT("/Game/Vortex/Audio/Sfx/amb_rain.amb_rain"));
    StormLoop = Loop(TEXT("/Game/Vortex/Audio/Sfx/amb_rain_storm.amb_rain_storm"));
    WindLoop = Loop(TEXT("/Game/Vortex/Audio/Sfx/amb_wind_snow.amb_wind_snow"));
    if (TActorIterator<AExponentialHeightFog> It(World); It) Fog = It->GetComponent();
}

void UVortexWeatherSubsystem::Tick(float DeltaTime)
{
    if (bDisabled) return;
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_DedicatedServer || FParse::Param(FCommandLine::Get(), TEXT("VortexNoWeather"))) { bDisabled = true; return; }
    float Clock = 0.f;
    int32 Day = 0;
    if (!VortexWorldHooks::DayClock(World, Clock, Day)) return;
    const APlayerController* Viewer = World->GetFirstPlayerController();
    if (!Viewer || !Viewer->PlayerCameraManager) return;
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    if (!Terrain) return;   // menus e mapas sem o mundo
    if (!bBuilt) Build();
    const FVector Eye = Viewer->PlayerCameraManager->GetCameraLocation();
    const int32 Region = Terrain->HasWorld() ? FMath::Max(1, Terrain->RegionAt(Eye.X, Eye.Y)) : 1;
    // a estação (3 dias cada) e a janela de 6 horas
    const float Hours = float(FMath::Max(Day, 1) - 1) * 24.f + HourOf(Clock);
    Season = ((FMath::Max(Day, 1) - 1) / 3) % 4;
    const int32 Window = FMath::FloorToInt(Hours / 6.f);
    const float Into = Hours / 6.f - Window;
    EVortexWeather Now = Pick(Window, Region, Season), Next = Pick(Window + 1, Region, Season);
    EVortexWeather Forced;
    if (ForcedWeather(Forced)) Now = Next = Forced;
    if (bDebugWeather) Now = Next = DebugWeather;
    const float Blend = FMath::SmoothStep(5.f / 6.f, 1.f, Into);   // a última hora da janela já mistura
    const FTarget Want = Lerp(TargetOf(Now), TargetOf(Next), Blend);
    Shown = Blend > .5f ? Next : Now;
    Overcast = FMath::FInterpConstantTo(Overcast, Want.Overcast, DeltaTime, 1.f / 15.f);
    Rain = FMath::FInterpConstantTo(Rain, Want.Rain, DeltaTime, 1.f / 15.f);
    Snow = FMath::FInterpConstantTo(Snow, Want.Snow, DeltaTime, 1.f / 15.f);
    FogAmount = FMath::FInterpConstantTo(FogAmount, Want.Fog, DeltaTime, 1.f / 20.f);
    Wind = FMath::FInterpConstantTo(Wind, Want.Wind, DeltaTime, 1.f / 10.f);
    // /clima: muda na hora, sem esperar a transição
    if (bSnap) { Overcast = Want.Overcast; Rain = Want.Rain; Snow = Want.Snow; FogAmount = Want.Fog; Wind = Want.Wind; bSnap = false; }

    // debaixo de um teto (casas, taverna, túneis): a chuva e a neve somem e o som abafa
    RoofCheckIn -= DeltaTime;
    if (RoofCheckIn <= 0.f)
    {
        RoofCheckIn = .3f;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexWeatherRoof), false, Viewer->GetPawn());
        FHitResult Hit;
        bUnderRoof = World->LineTraceSingleByChannel(Hit, Eye, Eye + FVector(0.f, 0.f, 2500.f), ECC_Visibility, Query);
    }
    Shelter = FMath::FInterpConstantTo(Shelter, bUnderRoof ? 1.f : 0.f, DeltaTime, 2.5f);

    // relâmpagos na tempestade: dois clarões seguidos, o trovão depois (mais longe, mais atrasado e mais baixo)
    FlashAge += DeltaTime;
    if (Shown == EVortexWeather::Storm && Rain > .7f)
    {
        BoltIn -= DeltaTime;
        if (BoltIn <= 0.f)
        {
            BoltIn = FMath::FRandRange(6.f, 18.f);
            FlashAge = 0.f;
            ThunderIn = FMath::FRandRange(.4f, 2.6f);
        }
    }
    Flash = FlashAge < .5f ? FMath::Exp(-FlashAge * 10.f) + (FlashAge > .14f ? .7f * FMath::Exp(-(FlashAge - .14f) * 12.f) : 0.f) : 0.f;
    Flash *= 1.f - .7f * Shelter;
    if (ThunderIn > 0.f)
    {
        ThunderIn -= DeltaTime;
        if (ThunderIn <= 0.f)
            if (USoundBase* Thunder = LoadObject<USoundBase>(nullptr, TEXT("/Game/Vortex/Audio/Sfx/thunder.thunder")))
                UGameplayStatics::PlaySound2D(World, Thunder, FMath::Lerp(1.f, .45f, FMath::Clamp(ThunderIn + 2.6f, 0.f, 2.6f) / 2.6f), FMath::FRandRange(.85f, 1.1f));
    }

    // a chuva e a neve em volta da câmera
    if (Fx)
    {
        Fx->SetActorLocation(Eye);
        const float Open = 1.f - Shelter;
        const float RainShow = Rain * Open, SnowShow = Snow * Open;
        auto Show = [](UStaticMeshComponent* Mesh, UMaterialInstanceDynamic* Material, float Amount)
        {
            if (!Mesh || !Material) return;
            const bool bOn = Amount > .01f;
            if (Mesh->IsVisible() != bOn) Mesh->SetVisibility(bOn);
            if (bOn) Material->SetScalarParameterValue(TEXT("Intensity"), Amount);
        };
        if (Sheets.Num() == 4)
        {
            Show(RainNear, Sheets[0], RainShow);
            Show(RainFar, Sheets[1], RainShow * .8f);
            Show(SnowNear, Sheets[2], SnowShow);
            Show(SnowFar, Sheets[3], SnowShow * .8f);
            // a cor segue a luz do dia (sem isso a chuva brilhava branca no escuro) e clareia no relâmpago
            const UVortexDayNightSubsystem* DayNight = World->GetSubsystem<UVortexDayNightSubsystem>();
            const float Light = FMath::Lerp(.07f, .5f, DayNight && DayNight->IsActive() ? DayNight->GetDaylight() : 1.f) * (1.f - .35f * Overcast) * (1.f + 4.f * Flash);
            for (int32 i = 0; i < 4; ++i)
                Sheets[i]->SetVectorParameterValue(TEXT("Color"), i < 2 ? FLinearColor(.82f, .87f, .95f) * Light : FLinearColor(1.f, 1.f, 1.f) * Light * 1.6f);
            // o vento inclina a neve (o balanço do material) e acelera a queda dela
            Sheets[2]->SetScalarParameterValue(TEXT("Sway"), .025f + .05f * Wind);
            Sheets[3]->SetScalarParameterValue(TEXT("Sway"), .015f + .04f * Wind);
        }
        const float Muffle = FMath::Lerp(1.f, .35f, Shelter);
        if (RainLoop) RainLoop->SetVolumeMultiplier(FMath::Max(.001f, Rain * (Shown == EVortexWeather::Storm ? .4f : .9f) * Muffle));
        if (StormLoop) StormLoop->SetVolumeMultiplier(FMath::Max(.001f, (Shown == EVortexWeather::Storm ? Rain : 0.f) * .8f * Muffle));
        if (WindLoop) WindLoop->SetVolumeMultiplier(FMath::Max(.001f, (Snow * .5f + .4f * Wind * Overcast) * Muffle));
    }
    // a neblina engrossa na neblina, na chuva e na nevasca (a de base é a do mapa, lida uma vez)
    // (a de base é lida depois que o AVortexFarTerrain afina a névoa, nos primeiros segundos)
    if (UExponentialHeightFogComponent* Haze = Fog.Get(); Haze && World->GetTimeSeconds() > 5.f)
    {
        if (BaseFog < 0.f) { BaseFog = Haze->FogDensity; BaseFogStart = Haze->StartDistance; }
        const float Density = BaseFog * (1.f + 2.f * FogAmount + .6f * Rain + 1.2f * Snow * Wind);
        if (FMath::Abs(Density - Haze->FogDensity) > BaseFog * .01f) Haze->SetFogDensity(Density);
        // na neblina e na nevasca a névoa começa mais perto
        const float Start = BaseFogStart * FMath::Lerp(1.f, .3f, FMath::Max(FogAmount, Snow * Wind));
        if (FMath::Abs(Start - Haze->StartDistance) > 50.f) Haze->SetStartDistance(Start);
    }
}
