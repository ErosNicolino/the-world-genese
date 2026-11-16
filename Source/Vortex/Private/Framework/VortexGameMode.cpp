#include "Framework/VortexGameMode.h"
#include "Rpg/VortexQuestLog.h"
#include "Engine/NetConnection.h"
#include "World/VortexBreakable.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Framework/VortexNetLog.h"
#include "Framework/VortexCreatureSmoke.h"
#include "World/VortexFountain.h"
#include "World/VortexTerrainData.h"
#include "World/VortexAwakeningStadium.h"
#include "Framework/VortexHeroWindow.h"
#include "Framework/VortexDevChat.h"
#include "Rpg/VortexHeroSheet.h"

#include "Framework/VortexCharacter.h"
#include "Framework/VortexHudBase.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexRecoverAbility.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Creatures/VortexWolf.h"
#include "Creatures/VortexGroupCoordinator.h"
#include "AbilitySystemComponent.h"
#include "Framework/VortexCharacterMovementComponent.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexPopulationSubsystem.h"
#include "World/VortexDayNightSubsystem.h"
#include "World/VortexForestLife.h"
#include "Creatures/VortexNpc.h"
#include "Creatures/VortexPet.h"
#include "Animation/VortexAnimInstance.h"
#include "Animation/VortexWolfAnimInstance.h"
#include "EngineUtils.h"
#include "RHI.h"
#include "RenderCore.h"
#include "Misc/App.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Framework/VortexGameState.h"
#include "Framework/VortexPlayerController.h"
#include "Framework/VortexPlayerState.h"
#include "Framework/VortexGameInstance.h"
#include "Persistence/VortexSaveSubsystem.h"
#include "Persistence/VortexPlayerSave.h"
#include "Persistence/VortexWorldSave.h"

#include "Engine/GameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"

AVortexGameMode::AVortexGameMode()
{
    GameStateClass = AVortexGameState::StaticClass();
    PlayerControllerClass = AVortexPlayerController::StaticClass();
    PlayerStateClass = AVortexPlayerState::StaticClass();
    DefaultPawnClass = AVortexCharacter::StaticClass();
    // HUDClass: o VortexUI põe o AVortexHudBase ao abrir (VortexUiModule.cpp)
}

void AVortexGameMode::StartPlay()
{
    Super::StartPlay();
    // o mundo aberto direto (Play no editor com o mapa do vale aberto) começava com um herói padrão:
    // sem herói escolhido e sem ser um teste (-Vortex...), o jogo vai para o menu
    if (GetNetMode() == NM_Standalone && !FString(FCommandLine::Get()).Contains(TEXT(" -Vortex")))
    {
        UVortexGameInstance* Game = Cast<UVortexGameInstance>(GetGameInstance());
        if (Game && !Game->StartedFromFrontEnd() && !Game->GetSelectedHeroId().IsValid())
        {
            UE_LOG(LogTemp, Display, TEXT("VORTEX_START mundo aberto sem herói: indo para o menu"));
            TWeakObjectPtr<UVortexGameInstance> Weak(Game);
            GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([Weak]()
            {
                if (Weak.IsValid()) UGameplayStatics::OpenLevel(Weak.Get(), TEXT("/Game/Vortex/Maps/L_Menu"));
            }));
            return;
        }
    }

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexRegionSmoke")))
    {
        const TSharedPtr<FVortexTerrainData> Region = AVortexTerrainStreamer::SharedData();
        const FBox2D Bounds = Region ? Region->PrototypeBounds() : FBox2D();
        const bool bPassed = Region && Region->ChunkCountX == 61 && Region->ChunkCountY == 61 &&
            Region->CellsPerChunk == 24 && FMath::IsNearlyEqual(Region->CellSize, 75.f) &&
            FMath::IsNearlyEqual(Bounds.Min.X, -90000.f) && FMath::IsNearlyEqual(Bounds.Min.Y, -54000.f) &&
            FMath::IsNearlyEqual(Bounds.Max.X, 19800.f) && FMath::IsNearlyEqual(Bounds.Max.Y, 55800.f) &&
            Region->ContainsWorldXY(0.f, 0.f) && !Region->ContainsWorldXY(20000.f, 0.f) &&
            Region->HasChunk(FIntPoint(0, 0)) && Region->GroundAt(0.f, 0.f) > -299.f;
        UE_LOG(LogTemp, Display, TEXT("VORTEX_REGION_SMOKE_%s bounds=(%.0f,%.0f)-(%.0f,%.0f)"),
            bPassed ? TEXT("PASS") : TEXT("FAIL"), Bounds.Min.X, Bounds.Min.Y, Bounds.Max.X, Bounds.Max.Y);
        FPlatformMisc::RequestExit(false);
        return;
    }
#endif

    // Direct map launches used by automated tests keep their temporary world. The front end
    // explicitly chooses a validated hosted save before travelling to this map.
    UVortexSaveSubsystem* Saves = GetGameInstance() ? GetGameInstance()->GetSubsystem<UVortexSaveSubsystem>() : nullptr;
    if (UVortexGameInstance* Instance = Cast<UVortexGameInstance>(GetGameInstance()))
    {
        ActiveWorldSave = Instance->ConsumeRequestedWorld();
        bPersistentWorld = ActiveWorldSave != nullptr;
    }
    if (!ActiveWorldSave) ActiveWorldSave = Saves ? Saves->CreateNewWorld(FMath::Rand(), 1) : nullptr;
    if (ActiveWorldSave)
    {
        if (bPersistentWorld)
        {
            if (UVortexPopulationSubsystem* Population = GetWorld()->GetSubsystem<UVortexPopulationSubsystem>())
            {
                Population->ReadFrom(*ActiveWorldSave);
            }
        }
        if (AVortexGameState* State = GetGameState<AVortexGameState>())
        {
            State->SetWorldDescriptor(ActiveWorldSave->WorldId, ActiveWorldSave->Seed,
                ActiveWorldSave->GeneratorVersion);
            // hora do mundo salvo (um mundo novo começa às 8h); -VortexHour= e -VortexDayScale= para testes
            State->SetDayClock(ActiveWorldSave->DayClock, ActiveWorldSave->DayNumber);
            float Value = 0.f;
            if (FParse::Value(FCommandLine::Get(), TEXT("VortexHour="), Value)) State->SetDayHour(Value);
            if (FParse::Value(FCommandLine::Get(), TEXT("VortexDayScale="), Value)) State->SetDayTimeScale(Value);
        }
    }
    if (bPersistentWorld && ActiveWorldSave && ActiveWorldSave->bHasHostLocation)
    {
        BeginRestoreHost();
    }
    // a ficha e o diário do herói já gravam a cada mudança; o mundo grava a cada 45 s
    if (bPersistentWorld && ActiveWorldSave) GetWorldTimerManager().SetTimer(AutosaveTimer, this, &AVortexGameMode::Autosave, 45.f, true, 45.f);

#if !UE_BUILD_SHIPPING
    int32 BenchCount = 0;
    if (FParse::Value(FCommandLine::Get(), TEXT("VortexMoveBench="), BenchCount))
    {
        BeginMoveBench(BenchCount);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexSaveSmoke")) && Saves && ActiveWorldSave)
    {
        const FString WorldSlot = TEXT("Vortex_World_") + ActiveWorldSave->WorldId.ToString(EGuidFormats::Digits);
        UVortexPlayerSave* Player = Saves->CreateLocalPlayer();
        const FString PlayerSlot = Player
            ? TEXT("Vortex_Player_") + Player->Identity.PlayerId.ToString(EGuidFormats::Digits)
            : FString();
        const int32 OriginalSeed = ActiveWorldSave->Seed;
        bool bPassed = Player && Saves->SaveHostedWorld(ActiveWorldSave);
        bPassed = bPassed && Saves->LoadHostedWorld(ActiveWorldSave->WorldId) != nullptr;

        if (bPassed)
        {
            ActiveWorldSave->Seed = OriginalSeed + 1;
            bPassed = Saves->SaveHostedWorld(ActiveWorldSave);
            const TArray<uint8> InvalidBytes = { 0, 1, 2, 3 };
            bPassed = bPassed && UGameplayStatics::SaveDataToSlot(InvalidBytes, WorldSlot, 0);
            const UVortexWorldSave* Recovered = Saves->LoadHostedWorld(ActiveWorldSave->WorldId);
            bPassed = bPassed && Recovered && Recovered->Seed == OriginalSeed;
        }
        if (Player)
        {
            Player->ItemCounts.Add(TEXT("SmokeItem"), 3);
            bPassed = bPassed && Saves->SaveLocalPlayer(Player);
            const UVortexPlayerSave* LoadedPlayer = Saves->LoadLocalPlayerFromDisk(Player->Identity.PlayerId);
            bPassed = bPassed && LoadedPlayer && LoadedPlayer->Identity.CharacterId == Player->Identity.CharacterId &&
                LoadedPlayer->ItemCounts.FindRef(TEXT("SmokeItem")) == 3;
        }

        // 09/11: a cópia dos convidados: o herói editado não vale; o conhecido volta como o anfitrião guardou
        {
            FVortexSheetSave Cheat;
            Cheat.bFresh = false;
            Cheat.Coins = 999;
            Cheat.HeroId = FGuid::NewGuid();
            ActiveWorldSave->bGuestCopies = true;
            bPassed = bPassed && DecideGuestSheet(TEXT("conta"), Cheat).Coins == 0 && DecideGuestSheet(TEXT("conta"), Cheat).bFresh;
            FVortexSheetSave Stored = Cheat;
            Stored.Coins = 50;
            if (FVortexGuestHero* Entry = FindGuest(TEXT("conta"), Cheat.HeroId, true)) { Entry->bHasSheet = true; Entry->Sheet = Stored; }
            bPassed = bPassed && DecideGuestSheet(TEXT("conta"), Cheat).Coins == 50 && DecideGuestSheet(TEXT("outra"), Cheat).Coins == 0;
            // da rede local para a Epic, e o save guarda tudo
            FVortexSheetSave Lan = Cheat;
            Lan.HeroId = FGuid::NewGuid();
            Lan.Coins = 7;
            if (FVortexGuestHero* Entry = FindGuest(FString(), Lan.HeroId, true)) { Entry->bHasSheet = true; Entry->Sheet = Lan; }
            bPassed = bPassed && DecideGuestSheet(TEXT("conta"), Lan).Coins == 7;
            bPassed = bPassed && Saves->SaveHostedWorld(ActiveWorldSave);
            const UVortexWorldSave* Again = Saves->LoadHostedWorld(ActiveWorldSave->WorldId);
            bPassed = bPassed && Again && Again->GuestHeroes.Num() == 2 && Again->bGuestCopies;
            // mundo antigo: quem já jogava não perde o herói
            ActiveWorldSave->bGuestCopies = false;
            FVortexSheetSave OldWorld = Cheat;
            OldWorld.HeroId = FGuid::NewGuid();
            bPassed = bPassed && DecideGuestSheet(TEXT("conta"), OldWorld).Coins == 999;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_GUEST_COPY %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
        }

        for (const FString& Suffix : { TEXT(""), TEXT("_backup"), TEXT("_staging") })
        {
            UGameplayStatics::DeleteGameInSlot(WorldSlot + Suffix, 0);
            if (!PlayerSlot.IsEmpty())
            {
                UGameplayStatics::DeleteGameInSlot(PlayerSlot + Suffix, 0);
            }
        }
        UE_LOG(LogTemp, Display, TEXT("Vortex save: SAVE_SMOKE_%s"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
        FPlatformMisc::RequestExit(false);
    }
#endif
}

float AVortexGameMode::LastAutosaveTime = -100.f;

void AVortexGameMode::Autosave()
{
    // não grava no meio da volta ao último lugar, com o herói caído ou ainda no Estádio do Despertar
    APlayerController* Host = GetWorld()->GetFirstPlayerController();
    const AVortexCharacter* Hero = Host ? Cast<AVortexCharacter>(Host->GetPawn()) : nullptr;
    if (!Hero || ResumingHost.IsValid() || Hero->GetVitals()->GetHealth() <= 0.f || AVortexAwakeningStadium::Contains(Hero->GetActorLocation())) return;
    if (SaveActiveWorld())
    {
        LastAutosaveTime = GetWorld()->GetTimeSeconds();
        UE_LOG(LogTemp, Display, TEXT("VORTEX_AUTOSAVE mundo salvo"));
    }
}

FString AVortexGameMode::GuestAccount(const APawn* Pawn)
{
    UNetConnection* Net = Pawn ? Pawn->GetNetConnection() : nullptr;
    if (!Net) return FString();
    TArray<FString> Parts;
    Net->LowLevelGetRemoteAddress(false).ParseIntoArray(Parts, TEXT(":"));
    return Parts.Num() >= 2 && Parts[0] == TEXT("EOS") ? Parts[1] : FString();
}

FVortexGuestHero* AVortexGameMode::FindGuest(const FString& Account, const FGuid& HeroId, bool bCreate)
{
    if (!ActiveWorldSave || !HeroId.IsValid()) return nullptr;
    const FString Id = HeroId.ToString(EGuidFormats::Digits);
    const FString Lan = TEXT("LAN_") + Id, Key = Account.IsEmpty() ? Lan : Account + TEXT("_") + Id;
    auto Find = [this](const FString& Wanted) { return ActiveWorldSave->GuestHeroes.FindByPredicate([&Wanted](const FVortexGuestHero& G) { return G.Key == Wanted; }); };
    FVortexGuestHero* Found = Find(Key);
    if (!Found && !Account.IsEmpty() && (Found = Find(Lan)) != nullptr) Found->Key = Key;   // jogou na rede local e agora veio pela Epic: o herói passa a ser desta conta
    if (!Found && bCreate)
    {
        Found = &ActiveWorldSave->GuestHeroes.AddDefaulted_GetRef();
        Found->Key = Key;
    }
    return Found;
}

bool AVortexGameMode::UsesGuestCopies() const
{
    return ActiveWorldSave && ActiveWorldSave->bGuestCopies;
}

FVortexSheetSave AVortexGameMode::DecideGuestSheet(const FString& Account, const FVortexSheetSave& Incoming)
{
    if (const FVortexGuestHero* Known = FindGuest(Account, Incoming.HeroId); Known && Known->bHasSheet)
    {
        FVortexSheetSave Out = Known->Sheet;
        Out.HeroId = Incoming.HeroId;
        Out.bFromMenu = Incoming.bFromMenu;
        return Out;
    }
    if (!UsesGuestCopies() || Incoming.bFresh) return Incoming;
    FVortexSheetSave Fresh;   // diz que já jogou, mas o anfitrião nunca viu: começa do zero, como qualquer herói novo
    Fresh.HeroId = Incoming.HeroId;
    Fresh.bFromMenu = Incoming.bFromMenu;
    return Fresh;
}

void AVortexGameMode::SaveGuestsNow()
{
    UVortexSaveSubsystem* Saves = GetGameInstance() ? GetGameInstance()->GetSubsystem<UVortexSaveSubsystem>() : nullptr;
    if (Saves && ActiveWorldSave && bPersistentWorld) Saves->SaveHostedWorld(ActiveWorldSave);
}

bool AVortexGameMode::SaveActiveWorld()
{
    for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
    {
        if (It->GetSheet()) It->GetSheet()->StoreGuest();
        if (It->GetQuestLog()) It->GetQuestLog()->StoreGuest();
    }
    if (!bPersistentWorld || !ActiveWorldSave) return false;
    if (APlayerController* Host = GetWorld()->GetFirstPlayerController())
    {
        if (Host->IsLocalController() && Host->GetPawn() && !ResumingHost.IsValid())
        {
            ActiveWorldSave->HostLocation = Host->GetPawn()->GetActorLocation();
            ActiveWorldSave->bHasHostLocation = true;
        }
    }
    if (UVortexPopulationSubsystem* Population = GetWorld()->GetSubsystem<UVortexPopulationSubsystem>())
    {
        Population->WriteTo(*ActiveWorldSave);
    }
    if (const AVortexGameState* State = GetGameState<AVortexGameState>())
    {
        ActiveWorldSave->DayClock = State->GetDayClock();
        ActiveWorldSave->DayNumber = State->GetDayNumber();
    }
    UVortexSaveSubsystem* Saves = GetGameInstance() ? GetGameInstance()->GetSubsystem<UVortexSaveSubsystem>() : nullptr;
    return Saves && Saves->SaveHostedWorld(ActiveWorldSave);
}

void AVortexGameMode::BeginRestoreHost()
{
    APlayerController* Host = GetWorld()->GetFirstPlayerController();
    AVortexCharacter* Hero = Host && Host->IsLocalController() ? Cast<AVortexCharacter>(Host->GetPawn()) : nullptr;
    if (!Hero) return;
    ResumingHost = Hero;
    ResumeLocation = ActiveWorldSave->HostLocation;
    ResumeStartedAt = GetWorld()->GetTimeSeconds();
    Hero->GetCharacterMovement()->DisableMovement();
    Hero->SetActorLocation(ResumeLocation + FVector(0.f, 0.f, 300.f), false);
    GetWorldTimerManager().SetTimer(ResumeTimer, this, &AVortexGameMode::TickRestoreHost, 0.1f, true, 0.1f);
}

void AVortexGameMode::TickRestoreHost()
{
    AVortexCharacter* Hero = ResumingHost.Get();
    if (!Hero)
    {
        GetWorldTimerManager().ClearTimer(ResumeTimer);
        return;
    }
    AVortexTerrainStreamer* Streamer = nullptr;
    if (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It) { Streamer = *It; }
    float Ground = 0.f;
    if (Streamer && Streamer->GroundReadyAt(ResumeLocation.X, ResumeLocation.Y, Ground))
    {
        const float HalfHeight = Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        Hero->SetActorLocation(FVector(ResumeLocation.X, ResumeLocation.Y,
            FMath::Max(ResumeLocation.Z, Ground + HalfHeight + 3.f)), false);
        Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        ResumingHost.Reset();
        GetWorldTimerManager().ClearTimer(ResumeTimer);
        UE_LOG(LogTemp, Display, TEXT("Vortex world: host resumed at %.0f, %.0f"), ResumeLocation.X, ResumeLocation.Y);
    }
    else if (GetWorld()->GetTimeSeconds() - ResumeStartedAt > 15.f &&
        !ResumeLocation.IsNearlyZero())
    {
        // The saved spot may no longer exist after a map revision. Try the known valley start.
        ResumeLocation = FVector::ZeroVector;
        Hero->SetActorLocation(FVector(0.f, 0.f, 300.f), false);
        ResumeStartedAt = GetWorld()->GetTimeSeconds();
        UE_LOG(LogTemp, Warning, TEXT("Vortex world: saved host location unavailable; using valley start"));
    }
}

void AVortexGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(ResumeTimer);
    if (bPersistentWorld && !SaveActiveWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("Vortex world: could not save hosted world during shutdown"));
    }
    Super::EndPlay(EndPlayReason);
}

void AVortexGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    if (ErrorMessage.IsEmpty() && GetNumPlayers() >= 8) ErrorMessage = TEXT("O mundo está cheio (8 jogadores).");
    VortexNetLog::Write(FString::Printf(TEXT("Mundo: chegando %s %s"), *Address, ErrorMessage.IsEmpty() ? TEXT("(aceito)") : *(TEXT("(recusado: ") + ErrorMessage + TEXT(")"))));
}

void AVortexGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    ++ConnectedPlayerCount;
    if (AVortexPlayerState* State = NewPlayer ? NewPlayer->GetPlayerState<AVortexPlayerState>() : nullptr)
    {
        State->SetSessionSlot(NextSessionSlot++);
    }
    if (AVortexGameState* State = GetGameState<AVortexGameState>())
    {
        State->SetConnectedPlayers(ConnectedPlayerCount);
    }
    UE_LOG(LogTemp, Display, TEXT("Vortex framework: server PostLogin, players=%d"), ConnectedPlayerCount);

#if !UE_BUILD_SHIPPING
    const bool bStepProbe = FParse::Param(FCommandLine::Get(), TEXT("VortexStepSmoke"));
    if (bStepProbe || FParse::Param(FCommandLine::Get(), TEXT("VortexMovementSmoke")))
    {
        BeginMovementSmoke(NewPlayer, bStepProbe);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexClientInputObserve")))
    {
        BeginClientInputObserve(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexMatrixSmoke")))
    {
        BeginMatrixSmoke(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexCombatSmoke")))
    {
        BeginCombatSmoke(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexSwordSmoke")))
    {
        BeginSwordSmoke(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexSwordShot")) || FParse::Param(FCommandLine::Get(), TEXT("VortexSwordNormalShot")))
    {
        BeginSwordShot(NewPlayer, FParse::Param(FCommandLine::Get(), TEXT("VortexSwordNormalShot")));
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexWolfSmoke")))
    {
        BeginWolfSmoke(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexWolfMotionProbe")))
    {
        BeginWolfMotionProbe(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexWolfVisualShot")))
    {
        BeginWolfVisualShot(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexPackSmoke")))
    {
        BeginPackSmoke(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexCameraShot")) ||
        FParse::Param(FCommandLine::Get(), TEXT("VortexCameraBeforeShot")))
    {
        BeginCameraShot(NewPlayer, FParse::Param(FCommandLine::Get(), TEXT("VortexCameraBeforeShot")));
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexCameraSmoke")))
    {
        BeginCameraSmoke(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexCameraSequenceShot")))
    {
        BeginCameraSequenceShot(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexTerrainBench")))
    {
        BeginTerrainBench(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexAnimShot")))
    {
        BeginAnimShot(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexDodgeDirectionSmoke")))
    {
        BeginDodgeDirectionSmoke(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexSolidWalkSmoke")))
    {
        BeginSolidWalkSmoke(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexWardrobeShot")))
    {
        BeginWardrobeShot(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexFogShot")))
    {
        BeginFogShot(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexStreamSmoke")) && ConnectedPlayerCount == 2)
    {
        BeginStreamSmoke();
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexGrassShot")))
    {
        BeginGrassShot(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexDayNightShot")))
    {
        BeginDayNightShot(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexHudShot")))
    {
        BeginHudShot(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexCreatureSmoke")))
    {
        VortexCreatureSmoke::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexMeleeSweep")))
    {
        VortexMeleeSweep::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexHeroShots")))
    {
        VortexHeroShots::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexClassShots")))
    {
        VortexClassShots::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexBossShots")))
    {
        VortexBossShots::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexSocialSmoke")) && NewPlayer->IsLocalController())
    {
        VortexSocialSmoke::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexRoamBench")))
    {
        VortexRoamBench::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexApplySmoke")))
    {
        VortexApplySmoke::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexWorldShots")))
    {
        VortexWorldShots::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexNpcShots")))
    {
        VortexNpcShots::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexMountShots")))
    {
        VortexMountShots::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexFeatureShots")))
    {
        VortexFeatureShots::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexBanditSmoke")))
    {
        VortexBanditSmoke::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexPoiSmoke")))
    {
        VortexPoiSmoke::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexWildlifeSmoke")))
    {
        VortexWildlifeSmoke::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexMapShots")))
    {
        VortexMapShots::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexIntroShots")))
    {
        VortexIntroShots::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexShopSmoke")))
    {
        VortexShopSmoke::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexWeaponCloseUp")))
    {
        VortexWeaponCloseUp::Begin(this, NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexWeaponShot")))
    {
        BeginWeaponShot(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexStadiumShot")))
    {
        BeginStadiumShot(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexStadiumPick")))
    {
        BeginStadiumPick(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexTownShot")))
    {
        BeginTownShot(NewPlayer);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexDayNightSmoke")) && ConnectedPlayerCount == 1)
    {
        BeginDayNightSmoke();
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexPopulationSmoke")) && ConnectedPlayerCount == 1)
    {
        BeginPopulationSmoke(NewPlayer);
    }
#endif
}



void AVortexGameMode::Logout(AController* Exiting)
{
    if (const AVortexCharacter* Leaving = Exiting ? Cast<AVortexCharacter>(Exiting->GetPawn()) : nullptr)
    {
        if (Leaving->GetSheet()) Leaving->GetSheet()->StoreGuest();
        if (Leaving->GetQuestLog()) Leaving->GetQuestLog()->StoreGuest();
        SaveGuestsNow();
    }
    if (const APlayerState* Leaving = Exiting ? Exiting->GetPlayerState<APlayerState>() : nullptr)
        VortexNetLog::Write(FString::Printf(TEXT("Mundo: saiu %s"), *Leaving->GetPlayerName()));
    Super::Logout(Exiting);

    ConnectedPlayerCount = FMath::Max(0, ConnectedPlayerCount - 1);
    if (AVortexGameState* State = GetGameState<AVortexGameState>())
    {
        State->SetConnectedPlayers(ConnectedPlayerCount);
    }
    UE_LOG(LogTemp, Display, TEXT("Vortex framework: server Logout, players=%d"), ConnectedPlayerCount);
}
