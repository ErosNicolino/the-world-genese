#include "Framework/VortexMenuStage.h"

#include "Camera/CameraComponent.h"
#include "Combat/VortexWeaponComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexGameState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexDayNightSubsystem.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    // o canto do vale do menu: perto do lago, com flores, árvores e o lago ao fundo
    const FVector2D MenuSpot(-30000.f, -8000.f);
    constexpr float MenuHour = 10.5f;

    // a luz das telas do herói (Data/UI/menu_palco.json: mudar não precisa compilar)
    struct FStageLight { float Fill = 35.f, Height = 60.f, Exposure = -.73f; };
    const FStageLight& StageLight()
    {
        static FStageLight Light = []()
        {
            FStageLight Out;
            FString Text;
            TSharedPtr<FJsonObject> Root;
            if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/UI/menu_palco.json"))) &&
                FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
            {
                Root->TryGetNumberField(TEXT("luz_preenchimento"), Out.Fill);
                Root->TryGetNumberField(TEXT("luz_altura"), Out.Height);
                Root->TryGetNumberField(TEXT("exposicao"), Out.Exposure);
            }
            return Out;
        }();
        return Light;
    }
}

AVortexMenuStage::AVortexMenuStage()
{
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Root);
    Camera->SetFieldOfView(55.f);
    Camera->bConstrainAspectRatio = false;
    // a luz e a exposição são as do jogo (pós-processo do mundo); aqui só sem borrão de movimento
    Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
    Camera->PostProcessSettings.MotionBlurAmount = 0.f;
    Camera->PostProcessBlendWeight = 1.f;
    Ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
    Ring->SetupAttachment(Root);
    Ring->SetUsingAbsoluteLocation(true);
    Ring->SetUsingAbsoluteScale(true);
    Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Ring->SetCastShadow(false);
    Ring->SetVisibleInRayTracing(false);
    Ring->bAffectDistanceFieldLighting = false;
    Ring->bAffectDynamicIndirectLighting = false;
    // 01/11: na criação a frente (no sol) ficava clara demais e as costas (na sombra) escuras demais. Uma luz suave sem
    // sombra que anda com a câmera clareia o lado que se vê, e a exposição das telas do herói baixa um pouco
    Fill = CreateDefaultSubobject<UPointLightComponent>(TEXT("Fill"));
    Fill->SetupAttachment(Root);
    Fill->SetUsingAbsoluteLocation(true);
    Fill->SetIntensityUnits(ELightUnits::Candelas);
    Fill->SetIntensity(35.f);
    Fill->SetAttenuationRadius(900.f);
    Fill->SetSourceRadius(60.f);
    Fill->SetSoftSourceRadius(120.f);
    Fill->SetLightColor(FLinearColor(1.f, .96f, .9f));
    Fill->SetCastShadows(false);
    Fill->SetAffectTranslucentLighting(false);
    Fill->SetVisibility(false);
}

// Sol, céu, névoa e exposição com os valores do mapa do jogo (TuneTerrainLighting.py,
// TuneDistanceFog.py, TuneDayNight.py) e o terreno do vale. O dia e noite e a floresta viva
// (nuvens, borboletas, sons) começam sozinhos ao achar o sol.
void AVortexMenuStage::SpawnWorld()
{
    UWorld* World = GetWorld();
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    if (!TActorIterator<ADirectionalLight>(World))
    {
        ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-40.f, -30.f, 0.f), Params);
        Sun->GetRootComponent()->SetMobility(EComponentMobility::Movable);
        UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetIntensity(6.f);
        Light->SetAtmosphereSunLight(true);
        Light->SetCastShadows(true);
    }
    if (!TActorIterator<ASkyLight>(World))
    {
        ASkyLight* Sky = World->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        USkyLightComponent* Light = Sky->GetLightComponent();
        Light->SetMobility(EComponentMobility::Movable);
        Light->bRealTimeCapture = true;
        Light->SetIntensity(1.15f);
        Light->RecaptureSky();
    }
    bool bAtmosphere = false;
    for (TActorIterator<AActor> It(World); It; ++It) bAtmosphere |= It->FindComponentByClass<USkyAtmosphereComponent>() != nullptr;
    if (!bAtmosphere)
    {
        AActor* Atmosphere = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
        USkyAtmosphereComponent* Component = NewObject<USkyAtmosphereComponent>(Atmosphere);
        Atmosphere->SetRootComponent(Component);
        Component->RegisterComponent();
    }
    if (!TActorIterator<AExponentialHeightFog>(World))
    {
        AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        UExponentialHeightFogComponent* Component = Fog->GetComponent();
        Component->SetFogDensity(.32f);
        Component->SetFogHeightFalloff(.35f);
        Component->SetStartDistance(4500.f);
        Component->SetFogMaxOpacity(1.f);
        Component->SetFogInscatteringColor(FLinearColor(.40f, .48f, .58f));
    }
    bool bGrade = false;
    for (TActorIterator<APostProcessVolume> It(World); It; ++It) bGrade |= It->bUnbound;
    if (!bGrade)
    {
        APostProcessVolume* Grade = World->SpawnActor<APostProcessVolume>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        Grade->bUnbound = true;
        FPostProcessSettings& Settings = Grade->Settings;
        Settings.bOverride_AutoExposureBias = true;
        Settings.AutoExposureBias = -.38f;
        Settings.bOverride_AutoExposureSpeedUp = Settings.bOverride_AutoExposureSpeedDown = true;
        Settings.AutoExposureSpeedUp = 2.f;
        Settings.AutoExposureSpeedDown = 1.f;
    }
    for (TActorIterator<AVortexTerrainStreamer> It(World); It; ++It) Streamer = *It;
    if (!Streamer) Streamer = World->SpawnActor<AVortexTerrainStreamer>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
    // fim de manhã, parado
    if (AVortexGameState* State = World->GetGameState<AVortexGameState>())
    {
        State->SetDayTimeScale(0.f);
        State->SetDayClock(AVortexGameState::ClockOfHour(MenuHour), 1);
    }
}

void AVortexMenuStage::BeginPlay()
{
    Super::BeginPlay();
    SpawnWorld();
    if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/UI/Menu/M_StageRing.M_StageRing")))
    {
        Ring->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
        RingMaterial = UMaterialInstanceDynamic::Create(Base, this);
        RingMaterial->SetScalarParameterValue(TEXT("Intensity"), 1.6f);
        Ring->SetMaterial(0, RingMaterial);
    }
    Ring->SetVisibility(false);
    // o espectador fica no canto do menu: o terreno é montado em volta dele
    if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
    {
        APawn* Spectator = Controller->GetPawn();
        if (!Spectator) Spectator = Controller->GetSpectatorPawn();
        if (Spectator)
        {
            Spectator->SetActorLocation(FVector(MenuSpot, 2000.f));
            Spectator->SetActorHiddenInGame(true);
        }
        Controller->SetViewTarget(this);
    }
    for (const TCHAR* Name : { TEXT("ui_click"), TEXT("ui_open"), TEXT("ui_close"), TEXT("ui_error"), TEXT("level_up"), TEXT("equip") })
    {
        if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Audio/UI/%s.%s"), Name, Name)))
        {
            Sounds.Add(Name, Sound);
        }
    }
    if (USoundBase* Theme = LoadObject<USoundBase>(nullptr, TEXT("/Game/Vortex/Audio/Music/MUS_Theme.MUS_Theme")))
    {
        Music = UGameplayStatics::CreateSound2D(this, Theme, .5f, 1.f, 0.f, nullptr, false, false);
        if (Music) Music->FadeIn(3.f, .5f);
    }
}

void AVortexMenuStage::PlaceHero()
{
    if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
    {
        APawn* Spectator = Controller->GetPawn();
        if (!Spectator) Spectator = Controller->GetSpectatorPawn();
        if (Spectator && FVector::Dist2D(Spectator->GetActorLocation(), FVector(MenuSpot, 0.f)) > 100.f)
        {
            Spectator->SetActorLocation(FVector(MenuSpot, 2000.f));
            Spectator->SetActorHiddenInGame(true);
        }
        if (Controller->GetViewTarget() != this) Controller->SetViewTarget(this);
    }
    float Ground = 0.f;
    if (!Streamer || !Streamer->IsReady() || !Streamer->GroundReadyAt(MenuSpot.X, MenuSpot.Y, Ground)) return;
    // clareira: o ponto mais perto do canto do menu sem árvore a 12 m (a câmera gira em volta do
    // herói e, numa copa, a tela ficava toda escura)
    FVector2D Spot = MenuSpot;
    if (const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData())
    {
        auto TreeDistance = [&Data](const FVector2D& P)
        {
            float Best = 1e9f;
            const float Chunk = Data->ChunkSize();
            const FIntPoint C(FMath::FloorToInt(P.X / Chunk), FMath::FloorToInt(P.Y / Chunk));
            for (int32 dx = -1; dx <= 1; ++dx) for (int32 dy = -1; dy <= 1; ++dy)
                if (const TArray<FVortexTerrainProp>* Props = Data->PropsInChunk(C + FIntPoint(dx, dy)))
                    for (const FVortexTerrainProp& Prop : *Props)
                        if (Data->IsTree(Prop.Kind)) Best = FMath::Min(Best, FVector2D::Distance(P, FVector2D(Prop.Location)));
            return Best;
        };
        bool bFound = TreeDistance(Spot) > 1200.f;
        for (float R = 400.f; R <= 6000.f && !bFound; R += 400.f)
        {
            for (int32 k = 0; k < 16 && !bFound; ++k)
            {
                const float A = 2.f * UE_PI * k / 16.f;
                const FVector2D P = MenuSpot + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R;
                float Z = 0.f;
                if (!Streamer->GroundReadyAt(P.X, P.Y, Z) || Z < Data->WaterLevel + 20.f) continue;
                if (TreeDistance(P) > 1200.f) { Spot = P; Ground = Z; bFound = true; }
            }
        }
    }
    Feet = FVector(Spot, Ground);
    Hero = SpawnStageHero(Feet + FVector(0.f, 0.f, 80.f));
    if (!Hero) return;
    if (bHasPendingLook) Hero->ApplyLook(PendingLook);
    // a câmera fica do lado do sol (um pouco de lado): o herói é iluminado de frente
    if (const UVortexDayNightSubsystem* DayNight = GetWorld()->GetSubsystem<UVortexDayNightSubsystem>())
    {
        const FVector Sun = DayNight->GetSunDirection().GetSafeNormal2D();
        if (!Sun.IsNearlyZero()) BaseYaw = FMath::RadiansToDegrees(FMath::Atan2(-Sun.Y, -Sun.X)) + 25.f;
    }
    CamYaw = YawShown = BaseYaw;
    Hero->SetActorHiddenInGame(!bHeroVisible);
    Ring->SetVisibility(bHeroVisible);
    bPlaced = true;
    PlacedAt = Clock;
}

// Herói só de vitrine: cada computador monta os seus (na sala de espera o mapa do menu é um
// servidor, mas estes não vão pela rede).
AVortexCharacter* AVortexMenuStage::SpawnStageHero(const FVector& Where)
{
    AVortexCharacter* Character = GetWorld()->SpawnActorDeferred<AVortexCharacter>(AVortexCharacter::StaticClass(),
        FTransform(FRotator(0.f, 180.f, 0.f), Where), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Character) return nullptr;
    Character->SetReplicates(false);
    Character->SetFlags(RF_Transient);
    Character->FinishSpawning(FTransform(FRotator(0.f, 180.f, 0.f), Where));
    Character->GetCharacterMovement()->GravityScale = 0.f;
    Character->GetCharacterMovement()->DisableMovement();
    Character->SetActorEnableCollision(false);
    return Character;
}

// lugares da sala: o primeiro no centro, os outros alternando direita e esquerda, num arco que
// vem um pouco para a câmera nas pontas
FVector AVortexMenuStage::LobbySpot(int32 Index) const
{
    const int32 Step = (Index + 1) / 2;
    const float Side = (Index % 2 == 1 ? 1.f : -1.f) * Step * 125.f;
    const FRotator Facing(0.f, BaseYaw, 0.f);
    FVector Spot2 = Feet + Facing.RotateVector(FVector::RightVector) * Side - Facing.Vector() * (FMath::Abs(Side) * .22f);
    if (Streamer)
    {
        const float Ground = Streamer->GroundAt(Spot2.X, Spot2.Y);
        if (Ground > -299.f) Spot2.Z = Ground;
    }
    return Spot2;
}

void AVortexMenuStage::ShowLobby(const TArray<FVortexLook>& Looks, const TArray<bool>& Ready)
{
    if (!bPlaced || !Hero) return;
    const int32 Count = FMath::Clamp(Looks.Num(), 1, 8);
    Members.SetNum(Count);
    for (int32 i = 0; i < Count; ++i)
    {
        FLobbyMember& Member = Members[i];
        if (!Member.Character.IsValid())
        {
            Member.Character = i == 0 ? Hero.Get() : SpawnStageHero(LobbySpot(i) + FVector(0.f, 0.f, 80.f));
            if (i > 0 && Member.Character.IsValid()) LobbyParty.Add(Member.Character.Get());
            Member.LookKey.Empty();
        }
        AVortexCharacter* Character = Member.Character.Get();
        if (!Character) continue;
        if (Looks.IsValidIndex(i))
        {
            FString Key = Looks[i].Race.ToString() + TEXT("/") + Looks[i].Appearance.ToString();
            for (const TPair<FName, FName>& Piece : Looks[i].Pieces) Key += TEXT("/") + Piece.Value.ToString();
            for (const TPair<FName, FColor>& Color : Looks[i].Colors) Key += TEXT("/") + Color.Value.ToHex();
            if (Key != Member.LookKey) { Character->ApplyLook(Looks[i]); Member.LookKey = Key; }
        }
        const bool bNowReady = Ready.IsValidIndex(i) && Ready[i];
        if (bNowReady && !Member.bReady) { Member.SpinAt = Clock; Member.bSwung = false; PlayUi(TEXT("equip"), .7f); }
        Member.bReady = bNowReady;
        Character->SetActorHiddenInGame(false);
    }
    // quem saiu da sala some
    for (int32 i = LobbyParty.Num() - 1; i >= 0; --i)
    {
        bool bUsed = false;
        for (const FLobbyMember& Member : Members) bUsed |= Member.Character.Get() == LobbyParty[i];
        if (!bUsed) { if (LobbyParty[i]) LobbyParty[i]->Destroy(); LobbyParty.RemoveAt(i); }
    }
    if (Looks.Num())
    {
        const FVortexRaceInfo* Race = FVortexLookCatalog::Get().Race(Looks[0].Race);
        HeroScale = Race ? Race->Shape.Stature() : 1.f;
        if (Race) RingTarget = Race->Accent;
    }
}

void AVortexMenuStage::PlayUi(FName Sound, float Volume)
{
    if (TObjectPtr<USoundBase>* Found = Sounds.Find(Sound)) UGameplayStatics::PlaySound2D(this, *Found, Volume);
}

void AVortexMenuStage::SetMode(EVortexStageMode NewMode)
{
    // saindo da sala: só o herói do centro fica
    if (Mode == EVortexStageMode::Lobby && NewMode != EVortexStageMode::Lobby)
    {
        for (AVortexCharacter* Member : LobbyParty) if (Member) Member->Destroy();
        LobbyParty.Reset();
        Members.Reset();
    }
    Mode = NewMode;
    if (Mode != EVortexStageMode::Creator) { bFaceFocus = false; ZoomLevel = 0.f; PanZ = 0.f; }
    SetHeroVisible(Mode != EVortexStageMode::Races);
}

void AVortexMenuStage::SetHeroVisible(bool bVisible)
{
    bHeroVisible = bVisible;
    if (Hero) Hero->SetActorHiddenInGame(!bVisible);
    Ring->SetVisibility(bVisible && bPlaced);
}

void AVortexMenuStage::ShowLook(const FVortexLook& Look)
{
    PendingLook = Look;
    bHasPendingLook = true;
    const FVortexRaceInfo* Race = FVortexLookCatalog::Get().Race(Look.Race);
    HeroScale = Race ? Race->Shape.Stature() : 1.f;
    if (Race) RingTarget = Race->Accent;
    if (Hero) Hero->ApplyLook(Look);
}

void AVortexMenuStage::Orbit(float YawDegrees, float PitchDegrees)
{
    CamYaw += YawDegrees;
    CamPitch = FMath::Clamp(CamPitch + PitchDegrees, -55.f, 12.f);
}

void AVortexMenuStage::Pan(float Centimeters)
{
    PanZ = FMath::Clamp(PanZ + Centimeters, -60.f, 120.f);
}

void AVortexMenuStage::ResetView()
{
    // volta para a frente do herói pelo caminho mais curto
    CamYaw = BaseYaw + FMath::RoundToFloat((CamYaw - BaseYaw) / 360.f) * 360.f;
    CamPitch = -8.f;
    ZoomLevel = 0.f;
    PanZ = 0.f;
    bFaceFocus = false;
}

void AVortexMenuStage::Celebrate()
{
    CelebrateTime = Clock;
    bSwung = false;
    PlayUi(TEXT("level_up"));
}

void AVortexMenuStage::Pulse()
{
    PulseTime = Clock;
}

void AVortexMenuStage::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Clock += DeltaSeconds;
    if (!bPlaced) PlaceHero();

    // herói de frente para a câmera inicial; a comemoração dá uma volta e um golpe
    const float Party = Clock - CelebrateTime;
    float Spin = 0.f;
    if (Party >= 0.f && Party < 1.1f)
    {
        const float T = Party / 1.1f;
        Spin = 360.f * (T < .5f ? 2.f * T * T : 1.f - FMath::Pow(-2.f * T + 2.f, 2.f) * .5f);
        if (!bSwung && Party > .45f && Hero && Hero->GetWeapon())
        {
            bSwung = true;
            Hero->GetWeapon()->PressPrimary();
        }
    }
    if (Hero && Mode != EVortexStageMode::Lobby)
    {
        Hero->SetActorLocationAndRotation(Feet + FVector(0.f, 0.f, Hero->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()),
            FRotator(0.f, BaseYaw + 180.f + Spin, 0.f));
    }
    // sala de espera: cada um no seu lugar, de frente para a câmera, olhando um pouco para o centro
    if (Mode == EVortexStageMode::Lobby)
    {
        for (int32 i = 0; i < Members.Num(); ++i)
        {
            FLobbyMember& Member = Members[i];
            AVortexCharacter* Character = Member.Character.Get();
            if (!Character) continue;
            const float Age = Clock - Member.SpinAt;
            float Turn = 0.f;
            if (Age >= 0.f && Age < 1.1f)
            {
                const float T = Age / 1.1f;
                Turn = 360.f * (T < .5f ? 2.f * T * T : 1.f - FMath::Pow(-2.f * T + 2.f, 2.f) * .5f);
                if (!Member.bSwung && Age > .45f && Character->GetWeapon()) { Member.bSwung = true; Character->GetWeapon()->PressPrimary(); }
            }
            const int32 Step = (i + 1) / 2;
            const float Toward = (i % 2 == 1 ? 1.f : -1.f) * Step * 6.f;
            Character->SetActorLocationAndRotation(LobbySpot(i) + FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()),
                FRotator(0.f, BaseYaw + 180.f + Toward + Turn, 0.f));
        }
    }
    RingColor = FMath::CInterpTo(RingColor, RingTarget, DeltaSeconds, 3.f);
    Ring->SetWorldLocation(Feet + FVector(0.f, 0.f, 2.f));
    Ring->SetWorldScale3D(FVector(1.4f * FMath::Max(HeroScale, .8f)));
    if (RingMaterial)
    {
        RingMaterial->SetVectorParameterValue(TEXT("Color"), RingColor);
        const float Flash = FMath::Max(Party >= 0.f ? FMath::Exp(-Party * 2.5f) : 0.f, FMath::Exp(-FMath::Max(Clock - PulseTime, 0.f) * 3.f) * .6f);
        RingMaterial->SetScalarParameterValue(TEXT("Flash"), Flash);
    }

    // câmera em volta do herói: cada tela tem distância, altura e um deslocamento de lado
    float BaseDist = 430.f, Shift = .30f, BasePitch = -11.f;
    switch (Mode)
    {
    case EVortexStageMode::Heroes: BaseDist = 400.f; Shift = -.10f; break;
    case EVortexStageMode::Races: BaseDist = 1100.f; Shift = 0.f; BasePitch = -14.f; break;
    case EVortexStageMode::Creator: BaseDist = 330.f; Shift = 0.f; break;
    // a sala abre conforme chega gente; o painel fica à esquerda
    case EVortexStageMode::Lobby: BaseDist = 520.f + 105.f * FMath::Max(Members.Num(), 1); Shift = .16f; BasePitch = -9.f; break;
    default: break;
    }
    // raças pequenas não ficam coladas na câmera; as grandes ficam mais longe
    const float Stature = FMath::Max(HeroScale, 1.f);
    ZoomShown = FMath::FInterpTo(ZoomShown, ZoomLevel, DeltaSeconds, 6.f);
    FaceShown = FMath::FInterpTo(FaceShown, bFaceFocus ? 1.f : 0.f, DeltaSeconds, 4.f);
    PanShown = FMath::FInterpTo(PanShown, PanZ, DeltaSeconds, 6.f);
    YawShown = FMath::FInterpTo(YawShown, CamYaw, DeltaSeconds, 7.f);
    PitchShown = FMath::FInterpTo(PitchShown, Mode == EVortexStageMode::Creator ? CamPitch : BasePitch, DeltaSeconds, 5.f);
    ShiftShown = FMath::FInterpTo(ShiftShown, Shift, DeltaSeconds, 3.f);
    DistShown = FMath::FInterpTo(DistShown, BaseDist * Stature * (1.f - .55f * ZoomShown), DeltaSeconds, 3.5f);
    const float Dist = FMath::Lerp(DistShown, 165.f * Stature, FaceShown);
    const float LookHeight = FMath::Lerp(78.f, 108.f, FaceShown) * FMath::Max(HeroScale, .6f) + PanShown;
    const FRotator View(PitchShown + FMath::Sin(Clock * .21f) * .4f, YawShown + FMath::Sin(Clock * .13f) * .8f, 0.f);
    FVector Target = Feet + FVector(0.f, 0.f, LookHeight);
    // herói à direita da tela (o menu fica à esquerda): o ponto olhado vai para a esquerda dele
    Target -= FRotator(0.f, View.Yaw, 0.f).RotateVector(FVector::RightVector) * Dist * ShiftShown * (1.f - FaceShown);
    FVector Eye = Target - View.Vector() * Dist;
    // a câmera nunca entra no chão
    if (Streamer)
    {
        const float Ground = Streamer->GroundAt(Eye.X, Eye.Y);
        if (Ground > -299.f) Eye.Z = FMath::Max(Eye.Z, Ground + 35.f);
    }
    Camera->SetWorldLocationAndRotation(Eye, (Target - Eye).Rotation());
    Camera->SetFieldOfView(FMath::Lerp(55.f, 40.f, FaceShown));
    // as telas do herói (lista e criação): a luz de preenchimento um pouco acima da câmera e a exposição mais baixa
    const bool bHeroView = Mode == EVortexStageMode::Creator || Mode == EVortexStageMode::Heroes;
    Fill->SetVisibility(bHeroView);
    Fill->SetIntensity(StageLight().Fill);
    Fill->SetWorldLocation(Eye + FVector(0.f, 0.f, StageLight().Height));
    Camera->PostProcessSettings.bOverride_AutoExposureBias = bHeroView;
    Camera->PostProcessSettings.AutoExposureBias = StageLight().Exposure;
}
