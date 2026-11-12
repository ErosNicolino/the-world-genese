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

#if !UE_BUILD_SHIPPING
void AVortexGameMode::BeginClientInputObserve(APlayerController* NewPlayer)
{
    struct FInputMetrics
    {
        float Elapsed = 0.f;
        float StartX = 0.f;
        float MaxX = 0.f;
        bool bStarted = false;
        bool bSawSprint = false;
        bool bSawAirborne = false;
        int32 Dodges = 0;
    };
    TWeakObjectPtr<APlayerController> WeakController(NewPlayer);
    TSharedRef<FInputMetrics> Metrics = MakeShared<FInputMetrics>();
    FTimerDelegate Observe = FTimerDelegate::CreateWeakLambda(this, [this, WeakController, Metrics]()
    {
        AVortexCharacter* Character = WeakController.IsValid()
            ? Cast<AVortexCharacter>(WeakController->GetPawn()) : nullptr;
        if (!Character)
        {
            return;
        }
        if (!Metrics->bStarted)
        {
            Metrics->StartX = Character->GetActorLocation().X;
            Metrics->MaxX = Metrics->StartX;
            Metrics->bStarted = true;
        }
        Metrics->Elapsed += 0.1f;
        Metrics->MaxX = FMath::Max(Metrics->MaxX, Character->GetActorLocation().X);
        Metrics->bSawSprint |= Character->GetVortexMovement()->IsSprinting();
        Metrics->Dodges = Character->GetVortexMovement()->DodgesStarted;
        Metrics->bSawAirborne |= Character->GetCharacterMovement()->IsFalling();
        if (Metrics->Elapsed >= 5.f)
        {
            GetWorldTimerManager().ClearTimer(ClientInputObserveTimer);
            const float DeltaX = Metrics->MaxX - Metrics->StartX;
            const bool bPassed = DeltaX > 300.f && Metrics->bSawSprint && Metrics->bSawAirborne && Metrics->Dodges == 1;
            UE_LOG(LogTemp, Display,
                TEXT("Vortex movement: SERVER_INPUT_SMOKE_%s deltaX=%.1f sprint=%d airborne=%d dodges=%d"),
                bPassed ? TEXT("PASS") : TEXT("FAIL"), DeltaX,
                Metrics->bSawSprint ? 1 : 0, Metrics->bSawAirborne ? 1 : 0, Metrics->Dodges);
        }
    });
    GetWorldTimerManager().SetTimer(ClientInputObserveTimer, Observe, 0.1f, true, 0.1f);
}

// Custo do movimento: Count personagens andando em círculos no chão do mapa de teste (colidindo
// entre si e com o obstáculo), sem renderização e sem limite de FPS: o quadro é praticamente só a
// thread do jogo. Média depois de 3 s de aquecimento, por 10 s.
void AVortexGameMode::BeginMoveBench(int32 Count)
{
    TArray<TWeakObjectPtr<AVortexCharacter>> Bots;
    for (int32 i = 0; i < Count; ++i)
    {
        const FVector At(-1100.f + (i % 12) * 90.f, -350.f + (i / 12 % 8) * 90.f, 114.f + (i / 96) * 220.f);
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        if (AVortexCharacter* Bot = GetWorld()->SpawnActor<AVortexCharacter>(AVortexCharacter::StaticClass(), At, FRotator::ZeroRotator, Params))
        {
            UVortexCharacterMovementComponent* Movement = Bot->GetVortexMovement();
            Movement->bRunPhysicsWithNoController = true;
            Movement->GroundFriction = 0.f;
            Movement->BrakingDecelerationWalking = 0.f;
            Bots.Add(Bot);
        }
    }
    struct FBench { float Elapsed = 0.f; double FrameMs = 0.0; int32 Frames = 0; };
    TSharedRef<FBench> Bench = MakeShared<FBench>();
    // uma vez por quadro (ver BeginTerrainBench)
    TWeakObjectPtr<AVortexGameMode> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis, Bots, Bench, Count](float Delta) -> bool
    {
        if (!WeakThis.IsValid()) return false;
        Bench->Elapsed += Delta;
        for (int32 i = 0; i < Bots.Num(); ++i)
        {
            if (AVortexCharacter* Bot = Bots[i].Get())
            {
                const float Angle = Bench->Elapsed * 0.8f + i * 0.37f;
                Bot->GetVortexMovement()->Velocity = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 460.f +
                    FVector(0.f, 0.f, Bot->GetVortexMovement()->Velocity.Z);
            }
        }
        if (Bench->Elapsed < 3.f)
        {
            return true;
        }
        Bench->FrameMs += Delta * 1000.0;
        ++Bench->Frames;
        if (Bench->Elapsed >= 13.f)
        {
            UE_LOG(LogTemp, Display, TEXT("Vortex movement: MOVE_BENCH personagens=%d quadro_ms=%.3f quadros=%d"),
                Count, Bench->FrameMs / FMath::Max(1, Bench->Frames), Bench->Frames);
            FPlatformMisc::RequestExit(false);
            return false;
        }
        return true;
    }));
}

// Passeio na mata do protótipo de terreno: o mesmo trajeto e a mesma câmera do --bench-wild do
// Godot (início em (-420, -60) m, 8,5 m/s em x e -1,6 m/s em z, câmera a 7 m balançando o giro),
// medindo de t=1 s a t=26 s depois de 2 s de preparo, como lá.
void AVortexGameMode::BeginTerrainBench(APlayerController* NewPlayer)
{
    struct FBench
    {
        float Time = 0.f;
        TArray<double> Frames;
        double GameMs = 0.0, GpuMs = 0.0, RenderMs = 0.0;
        int32 Samples = 0;
        bool bShot = false;
    };
    TWeakObjectPtr<APlayerController> WeakController(NewPlayer);
    TSharedRef<FBench> Bench = MakeShared<FBench>();
    // uma vez por quadro (um cronômetro curto rodava várias vezes por quadro para recuperar o
    // atraso e cada vez somava o quadro inteiro)
    TWeakObjectPtr<AVortexGameMode> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis, WeakController, Bench](float Delta) -> bool
    {
        AVortexGameMode* Self = WeakThis.Get();
        if (!Self) return false;
        APlayerController* Controller = WeakController.Get();
        AVortexCharacter* Character = Controller ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        AVortexTerrainStreamer* Streamer = nullptr;
        for (TActorIterator<AVortexTerrainStreamer> It(Self->GetWorld()); It; ++It) Streamer = *It;
        if (!Character || !Streamer || !Streamer->IsReady())
        {
            return true;
        }
        Bench->Time += Delta;
        const float T = Bench->Time - 2.f;
        const FVector2D Godot = FVector2D(-420.f, -60.f) + FVector2D(8.5f, -1.6f) * FMath::Max(T, 0.f);
        const FVector At(Godot.X * 100.f, Godot.Y * 100.f, 0.f);
        const float Ground = Streamer->GroundAt(At.X, At.Y);
        Character->GetCharacterMovement()->DisableMovement();
        Character->SetActorLocation(FVector(At.X, At.Y, Ground + 80.f), false, nullptr, ETeleportType::TeleportPhysics);
        // câmera do Godot: olhando para +x, balançando 0,5 rad, 0,28 rad para baixo, a 7 m. O giro
        // positivo do Godot vira para -z (esquerda); na Unreal o positivo vira para +Y (= +z): sinal trocado
        Character->UseGodotBenchCamera();
        const float Yaw = -FMath::RadiansToDegrees(FMath::Sin(Bench->Time * 0.7f) * 0.5f);
        Controller->SetControlRotation(FRotator(-FMath::RadiansToDegrees(0.28f), Yaw, 0.f));
        if (T <= 1.f)
        {
            return true;
        }
        // -VortexBenchProfile: no meio do passeio, o perfil da GPU e a média dos contadores de render vão para o log
        static bool bProfiled = false, bStatsOn = false;
        if (!bStatsOn && T >= 6.f && FParse::Param(FCommandLine::Get(), TEXT("VortexBenchProfile")))
        {
            bStatsOn = true;
            for (const TCHAR* Group : { TEXT("stat scenerendering"), TEXT("stat initviews"), TEXT("stat sceneupdate"), TEXT("stat gpu"), TEXT("stat game") })
            {
                GEngine->Exec(Self->GetWorld(), Group);
            }
        }
        if (!bProfiled && T >= 12.f && FParse::Param(FCommandLine::Get(), TEXT("VortexBenchProfile")))
        {
            bProfiled = true;
            GEngine->Exec(Self->GetWorld(), TEXT("ProfileGPU"));
            GEngine->Exec(Self->GetWorld(), TEXT("stat dumpave -num=90 -ms=0.15"));
        }
        // foto no meio do trajeto (Saved/Screenshots), para comparar com o Godot
        if (!Bench->bShot && T >= 13.f && FParse::Param(FCommandLine::Get(), TEXT("VortexTerrainShot")))
        {
            Bench->bShot = true;
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/terrain_bench.png"), false, false);
        }
        Bench->Frames.Add(Delta * 1000.0);
        Bench->GameMs += FPlatformTime::ToMilliseconds(GGameThreadTime);
        Bench->RenderMs += FPlatformTime::ToMilliseconds(GRenderThreadTime);
        Bench->GpuMs += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
        ++Bench->Samples;
        if (T < 26.f)
        {
            return true;
        }
        TArray<double> Sorted = Bench->Frames;
        Sorted.Sort([](double A, double B) { return A > B; });
        double Sum = 0.0;
        for (double F : Sorted) Sum += F;
        const int32 LowCount = FMath::Max(1, Sorted.Num() / 100);
        double Low = 0.0;
        for (int32 i = 0; i < LowCount; ++i) Low += Sorted[i];
        Low /= LowCount;
        int32 Over25 = 0, Over40 = 0;
        for (double F : Sorted) { Over25 += F > 25.0; Over40 += F > 40.0; }
        const double Avg = Sum / Sorted.Num();
        const int32 N = FMath::Max(1, Bench->Samples);
        UE_LOG(LogTemp, Display,
            TEXT("Vortex terrain: TERRAIN_BENCH fps=%.1f low1%%=%.1f pior=%.1fms >25ms=%d >40ms=%d quadros=%d jogo_ms=%.2f render_ms=%.2f gpu_ms=%.2f pedacos=%d pedaco_ms_medio=%.2f pedaco_ms_max=%.2f desenhos=%d triangulos=%d"),
            1000.0 / Avg, 1000.0 / Low, Sorted[0], Over25, Over40, Sorted.Num(),
            Bench->GameMs / N, Bench->RenderMs / N, Bench->GpuMs / N,
            Streamer->ChunksBuilt, Streamer->ChunkGameThreadMsTotal / FMath::Max(1, Streamer->ChunksBuilt), Streamer->ChunkGameThreadMsMax,
            GNumDrawCallsRHI[0], GNumPrimitivesDrawnRHI[0]);
        const double Built = FMath::Max(1, Streamer->ChunksBuilt);
        UE_LOG(LogTemp, Display, TEXT("Vortex terrain: TERRAIN_BENCH_CHUNK chao_ms=%.2f objetos_ms=%.2f tufos_ms=%.2f streamer_ms_medio=%.2f streamer_ms_max=%.2f carregados=%d memoria_mb=%.0f pico_mb=%.0f"),
            Streamer->ChunkGroundMs / Built, Streamer->ChunkPropsMs / Built, Streamer->ChunkTuftMs / Built,
            Streamer->TickMsTotal / FMath::Max(1, Streamer->Ticks), Streamer->TickMsMax, Streamer->LoadedChunkCount(),
            FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0), FPlatformMemory::GetStats().PeakUsedPhysical / (1024.0 * 1024.0));
        // o que estava ligado de verdade (iluminação global, reflexos, sombras virtuais)
        auto CVar = [](const TCHAR* Name) { IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name); return V ? V->GetInt() : -1; };
        UE_LOG(LogTemp, Display, TEXT("Vortex terrain: TERRAIN_BENCH_RENDER gi=%d reflexos=%d sombras_virtuais=%d nanite=%d aa=%d"),
            CVar(TEXT("r.DynamicGlobalIlluminationMethod")), CVar(TEXT("r.ReflectionMethod")), CVar(TEXT("r.Shadow.Virtual.Enable")),
            CVar(TEXT("r.Nanite")), CVar(TEXT("r.AntiAliasingMethod")));
        FPlatformMisc::RequestExit(false);
        return false;
    }));
}

// Anda de verdade (entrada de movimento, não teletransporte) contra troncos de árvores do terreno
// exportado e confere se o corpo para antes do centro do tronco.
void AVortexGameMode::BeginSolidWalkSmoke(APlayerController* NewPlayer)
{
    struct FState { int32 Index = 0; float Time = 0.f; bool bPlaced = false; int32 Failures = 0; TArray<FVortexTerrainSolid> Trunks; };
    TWeakObjectPtr<APlayerController> WeakController(NewPlayer);
    TSharedRef<FState> State = MakeShared<FState>();
    TWeakObjectPtr<AVortexGameMode> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis, WeakController, State](float Delta) -> bool
    {
        if (!WeakThis.IsValid()) return false;
        AVortexCharacter* Character = WeakController.IsValid() ? Cast<AVortexCharacter>(WeakController->GetPawn()) : nullptr;
        const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
        if (!Character || !Data) return true;
        if (State->Trunks.IsEmpty())
        {
            // troncos grossos e isolados perto do começo do passeio do Godot (-420, -60)
            const FIntPoint Center(FMath::FloorToInt(-42000.f / Data->ChunkSize()), FMath::FloorToInt(-6000.f / Data->ChunkSize()));
            // seis troncos e depois três rochas (casca convexa) grandes o bastante para barrar
            int32 Rocks = 0;
            for (int32 dx = -6; dx <= 6 && State->Trunks.Num() < 9; ++dx)
            for (int32 dy = -6; dy <= 6 && State->Trunks.Num() < 9; ++dy)
            {
                const TArray<FVortexTerrainSolid>* Specs = Data->SolidsInChunk(Center + FIntPoint(dx, dy));
                if (!Specs) continue;
                for (const FVortexTerrainSolid& S : *Specs)
                {
                    const bool bTrunk = S.Shape == 0 && S.B >= 200.f && S.A >= 20.f && State->Trunks.Num() - Rocks < 6;
                    FBox RockBox(ForceInit);
                    for (const FVector& P : S.Points) RockBox += P;
                    const bool bRock = S.Shape == 2 && Rocks < 3 && RockBox.IsValid && RockBox.GetExtent().Z > 50.f && RockBox.GetExtent().GetMin() > 45.f;
                    if (!bTrunk && !bRock) continue;
                    // caminho livre: nada entre 1,6 e 4,5 m antes do tronco (as raízes ficam em volta dele)
                    bool bClear = true;
                    for (const FVortexTerrainSolid& O : *Specs)
                    {
                        if (&O == &S) continue;
                        const FVector D = O.Location - S.Location;
                        if (D.X < -160.f && D.X > -450.f && FMath::Abs(D.Y) < 150.f) { bClear = false; break; }
                    }
                    if (bClear)
                    {
                        FVortexTerrainSolid Target = S;
                        if (bRock)
                        {
                            // mira o meio da casca (no mundo), não o ponto do chão onde a rocha nasce
                            Target.Location += FRotator(0.f, S.YawDegrees, 0.f).RotateVector(RockBox.GetCenter()) * FVector(1, 1, 0);
                            Target.A = FMath::Min(RockBox.GetExtent().X, RockBox.GetExtent().Y);
                        }
                        State->Trunks.Add(Target);
                        Rocks += bRock;
                        break;
                    }
                }
            }
            if (State->Trunks.IsEmpty())
            {
                UE_LOG(LogTemp, Error, TEXT("Vortex terrain: SOLID_WALK_SMOKE_FAIL sem troncos"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
        }
        // espera o aquecimento dos efeitos (o primeiro quadro trava uns segundos ao entrar no mundo)
        if (Character->GetWorld()->GetTimeSeconds() < 7.0) return true;
        const FVortexTerrainSolid& Trunk = State->Trunks[State->Index];
        UVortexCharacterMovementComponent* Movement = Character->GetVortexMovement();
        if (!State->bPlaced)
        {
            const FVector Start = Trunk.Location - FVector(350.f, 0.f, 0.f);
            Movement->Velocity = FVector::ZeroVector;
            Character->SetActorLocation(FVector(Start.X, Start.Y, Data->GroundAt(Start.X, Start.Y) + 120.f), false, nullptr, ETeleportType::TeleportPhysics);
            Movement->SetMovementMode(MOVE_Falling);
            WeakController->SetControlRotation(FRotator::ZeroRotator);
            State->bPlaced = true;
            State->Time = 0.f;
        }
        State->Time += Delta;
        // 1,5 s para a colisão do pedaço ficar pronta, depois 2,5 s andando para +X
        if (State->Time > 1.5f) Character->AddMovementInput(FVector::ForwardVector, 1.f);
        if (State->Time < 4.f) return true;
        const FVector At = Character->GetActorLocation();
        const float Reached = At.X - Trunk.Location.X;
        // troncos: para antes do centro; rochas compridas e giradas podem desviar o corpo pela face
        // inclinada (desliza, como no Godot), então desviar 80 cm ou mais também conta como barrado
        const bool bPass = Reached < -Trunk.A * 0.5f || (Trunk.Shape == 2 && FMath::Abs(At.Y - Trunk.Location.Y) > 80.f);
        UE_LOG(LogTemp, Display, TEXT("Vortex terrain: SOLID_WALK %d %s %s=(%.0f,%.0f) raio=%.0f fim=(%.0f,%.0f) avancou_alem_do_centro=%.0f"),
            State->Index, bPass ? TEXT("PASS") : TEXT("FAIL"), Trunk.Shape == 0 ? TEXT("tronco") : TEXT("rocha"), Trunk.Location.X, Trunk.Location.Y, Trunk.A, At.X, At.Y, Reached);
        State->Failures += bPass ? 0 : 1;
        if (++State->Index >= State->Trunks.Num())
        {
            UE_LOG(LogTemp, Display, TEXT("Vortex terrain: SOLID_WALK_SMOKE_%s failed=%d"), State->Failures == 0 ? TEXT("PASS") : TEXT("FAIL"), State->Failures);
            FPlatformMisc::RequestExit(false);
            return false;
        }
        State->bPlaced = false;
        return true;
    }));
}

void AVortexGameMode::BeginDodgeDirectionSmoke(APlayerController* NewPlayer)
{
    struct FState { int32 Stage = 0; float Time = 0.f; bool bPlaced = false, bTriggered = false, bChecked = false; int32 Failures = 0; };
    TWeakObjectPtr<APlayerController> WeakController(NewPlayer);
    TSharedRef<FState> State = MakeShared<FState>();
    TWeakObjectPtr<AVortexGameMode> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis, WeakController, State](float Delta) -> bool
    {
        if (!WeakThis.IsValid()) return false;
        AVortexCharacter* Character = WeakController.IsValid() ? Cast<AVortexCharacter>(WeakController->GetPawn()) : nullptr;
        if (!Character) return true;
        static const FVector Directions[] = { FVector::ForwardVector, -FVector::ForwardVector, -FVector::RightVector, FVector::RightVector };
        static const FName Clips[] = { TEXT("Dodge"), TEXT("Dodge_Back"), TEXT("Dodge_Left"), TEXT("Dodge_Right") };
        UVortexCharacterMovementComponent* Movement = Character->GetVortexMovement();
        if (!State->bPlaced)
        {
            Movement->Velocity = FVector::ZeroVector;
            Character->SetActorLocation(FVector(-750.f, 0.f, 140.f), false, nullptr, ETeleportType::TeleportPhysics);
            Movement->SetMovementMode(MOVE_Falling);
            WeakController->SetControlRotation(FRotator::ZeroRotator);
            State->bPlaced = true;
        }
        State->Time += Delta;
        if (State->Time >= 0.3f && !State->bTriggered && Movement->IsMovingOnGround())
        {
            Character->AddMovementInput(Directions[State->Stage], 1.f);
            Movement->bWantsToDodge = true;
            State->bTriggered = true;
        }
        if (State->bTriggered && !State->bChecked && State->Time >= 0.6f)
        {
            const UVortexAnimInstance* Anim = Cast<UVortexAnimInstance>(Character->GetMesh()->GetAnimInstance());
            const bool bPass = Movement->IsDodging() && Movement->DodgeVisualDirection == State->Stage &&
                Anim && Anim->CurrentClip() == Clips[State->Stage];
            UE_LOG(LogTemp, Display, TEXT("Vortex anim: DODGE_%s_%s visual=%d clip=%s"),
                *Clips[State->Stage].ToString(), bPass ? TEXT("PASS") : TEXT("FAIL"),
                Movement->DodgeVisualDirection, Anim ? *Anim->CurrentClip().ToString() : TEXT("none"));
            State->Failures += bPass ? 0 : 1;
            State->bChecked = true;
        }
        if (State->Time >= 1.8f)
        {
            ++State->Stage;
            if (State->Stage == 4)
            {
                UE_LOG(LogTemp, Display, TEXT("Vortex anim: DODGE_DIRECTION_SMOKE_%s failed=%d"),
                    State->Failures == 0 ? TEXT("PASS") : TEXT("FAIL"), State->Failures);
                FPlatformMisc::RequestExit(false);
                return false;
            }
            State->Time = 0.f;
            State->bPlaced = State->bTriggered = State->bChecked = false;
        }
        return true;
    }));
}

// Fotos do rosto e da roupa de frente (Saved/Screenshots/wardrobe_*.png): peças padrão e depois
// todas as alternativas, com a mesma câmera, para conferir que a peça trocada some de verdade.
void AVortexGameMode::BeginWardrobeShot(APlayerController* NewPlayer)
{
    struct FShot { float Time = 0.f; int32 Taken = 0; bool bPlaced = false, bSwapped = false; float Yaw = 0.f; };
    TWeakObjectPtr<APlayerController> WeakController(NewPlayer);
    TSharedRef<FShot> Shot = MakeShared<FShot>();
    TWeakObjectPtr<AVortexGameMode> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis, WeakController, Shot](float Delta) -> bool
    {
        AVortexGameMode* Self = WeakThis.Get();
        APlayerController* Controller = WeakController.Get();
        AVortexCharacter* Character = Controller ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        AVortexTerrainStreamer* Streamer = nullptr;
        if (Self) for (TActorIterator<AVortexTerrainStreamer> It(Self->GetWorld()); It; ++It) Streamer = *It;
        if (!Self) return false;
        if (!Character || !Streamer || !Streamer->IsReady()) return true;
        Shot->Time += Delta;
        if (!Shot->bPlaced)
        {
            const FVector At(-30000.f, -8000.f, 0.f);
            Character->SetActorLocation(FVector(At.X, At.Y, Streamer->GroundAt(At.X, At.Y) + 94.f), false, nullptr, ETeleportType::TeleportPhysics);
            Shot->Yaw = Character->GetActorRotation().Yaw;
            Shot->bPlaced = true;
        }
        if (Shot->Time < 5.5f)
        {
            Character->UseGodotBenchCamera(230.f, 0.f);
            Controller->SetControlRotation(FRotator(-12.f, Shot->Yaw + 180.f, 0.f));
        }
        if (!Shot->bSwapped && Shot->Time >= 3.f)
        {
            static const TPair<FName, FName> Alternatives[] = {
                {TEXT("Hair"), TEXT("hair_male_2")}, {TEXT("Eyes"), TEXT("eyes_m_fierce")}, {TEXT("Mouth"), TEXT("mouth_smile")},
                {TEXT("Shirt"), TEXT("shirt_punk")}, {TEXT("Pants"), TEXT("pants_jeans_punk")}, {TEXT("Shoes"), TEXT("boots_punk")},
                {TEXT("Gloves"), TEXT("gloves_punk")},
            };
            for (const TPair<FName, FName>& Entry : Alternatives) Character->SetWardrobePiece(Entry.Key, Entry.Value);
            Shot->bSwapped = true;
        }
        static const float Times[] = { 2.5f, 5.f };
        static const TCHAR* Names[] = { TEXT("padrao"), TEXT("alternativo") };
        if (Shot->Taken < 2 && Shot->Time >= Times[Shot->Taken])
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/wardrobe_%s.png"), Names[Shot->Taken]), false, false);
            ++Shot->Taken;
        }
        // terceira foto: em pé no meio da grama, vista de cima, para ver a grama aberta em volta
        if (Shot->Taken == 2 && Shot->Time >= 5.5f && Shot->Time - Delta < 5.5f)
        {
            const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
            const FVector Base = Character->GetActorLocation();
            const int32 Cx = FMath::FloorToInt(Base.X / Data->CellSize), Cy = FMath::FloorToInt(Base.Y / Data->CellSize);
            auto Green = [&Data](int32 X, int32 Y)
            {
                const FLinearColor G = FLinearColor::FromSRGBColor(Data->CellColor(X, Y));
                return Data->CellHeight(X, Y) > Data->WaterLevel && G.G > G.R * 1.05f && G.G > G.B * 1.2f;
            };
            for (int32 R = 1; R < 40; ++R)
            {
                bool bFound = false;
                for (int32 dx = -R; dx <= R && !bFound; ++dx)
                for (int32 dy = -R; dy <= R && !bFound; ++dy)
                {
                    bool bAll = true;
                    for (int32 ox = -1; ox <= 1; ++ox) for (int32 oy = -1; oy <= 1; ++oy)
                        bAll &= Green(Cx + dx + ox, Cy + dy + oy) && FMath::Abs(Data->CellHeight(Cx + dx + ox, Cy + dy + oy) - Data->CellHeight(Cx + dx, Cy + dy)) < 30.f;
                    if (!bAll) continue;
                    const FVector At((Cx + dx + 0.5f) * Data->CellSize, (Cy + dy + 0.5f) * Data->CellSize, 0.f);
                    Character->SetActorLocation(FVector(At.X, At.Y, Data->GroundAt(At.X, At.Y) + 72.f), false, nullptr, ETeleportType::TeleportPhysics);
                    UE_LOG(LogTemp, Display, TEXT("Vortex grass: foto da pisada em (%.0f, %.0f)"), At.X, At.Y);
                    bFound = true;
                }
                if (bFound) break;
            }
        }
        if (Shot->Time >= 5.5f)
        {
            Character->UseGodotBenchCamera(260.f, 0.f);
            Controller->SetControlRotation(FRotator(-50.f, Shot->Yaw + 150.f, 0.f));
        }
        if (Shot->Taken == 2 && Shot->Time >= 8.f)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/grass_step.png"), false, false);
            ++Shot->Taken;
        }
        if (Shot->Time > 9.f)
        {
            FPlatformMisc::RequestExit(false);
            return false;
        }
        return true;
    }));
}

// Fotos do aventureiro animado (Saved/Screenshots/anim_*.png): parado, correndo, no ar, esquivando,
// com a câmera de lado, no terreno do protótipo.
void AVortexGameMode::BeginAnimShot(APlayerController* NewPlayer)
{
    struct FShot { float Time = 0.f; int32 Taken = 0; bool bPlaced = false; };
    TWeakObjectPtr<APlayerController> WeakController(NewPlayer);
    TSharedRef<FShot> Shot = MakeShared<FShot>();
    TWeakObjectPtr<AVortexGameMode> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis, WeakController, Shot](float Delta) -> bool
    {
        AVortexGameMode* Self = WeakThis.Get();
        APlayerController* Controller = WeakController.Get();
        AVortexCharacter* Character = Controller ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        AVortexTerrainStreamer* Streamer = nullptr;
        if (Self) for (TActorIterator<AVortexTerrainStreamer> It(Self->GetWorld()); It; ++It) Streamer = *It;
        if (!Self) return false;
        if (!Character || !Streamer || !Streamer->IsReady()) return true;
        Shot->Time += Delta;
        if (!Shot->bPlaced)
        {
            // um trecho plano e aberto do passeio na mata
            const FVector At(-30000.f, -8000.f, 0.f);
            Character->SetActorLocation(FVector(At.X, At.Y, Streamer->GroundAt(At.X, At.Y) + 94.f), false, nullptr, ETeleportType::TeleportPhysics);
            Shot->bPlaced = true;
        }
        Character->UseGodotBenchCamera(380.f, 0.f);
        Controller->SetControlRotation(FRotator(-10.f, 90.f, 0.f));     // câmera olhando de lado, perto
        const float T = Shot->Time;
        if (T > 3.f && T < 9.f) Character->AddMovementInput(FVector::ForwardVector, 1.f);
        if (T > 5.f && T - Delta <= 5.f) Character->Jump();
        if (T > 5.4f) Character->StopJumping();
        if (T > 7.f && T - Delta <= 7.f) Character->GetVortexMovement()->bWantsToDodge = true;
        static const float Times[] = { 2.5f, 4.5f, 5.35f, 7.2f, 10.5f };
        static const TCHAR* Names[] = { TEXT("parado"), TEXT("correndo"), TEXT("no_ar"), TEXT("esquiva"), TEXT("parado_depois") };
        if (Shot->Taken < 5 && T >= Times[Shot->Taken])
        {
            UE_LOG(LogTemp, Display, TEXT("Vortex anim: foto %s clipe=%s velocidade=%.0f"), Names[Shot->Taken],
                Character->GetMesh()->GetAnimInstance() ? *Cast<UVortexAnimInstance>(Character->GetMesh()->GetAnimInstance())->CurrentClip().ToString() : TEXT("-"),
                Character->GetVelocity().Size2D());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/anim_%d_%s.png"), Shot->Taken, Names[Shot->Taken]), false, false);
            ++Shot->Taken;
        }
        if (T > 11.f)
        {
            FPlatformMisc::RequestExit(false);
            return false;
        }
        return true;
    }));
}

// Matriz de movimento (Tools/AddMovementMatrixLanes.py): o personagem percorre cada pista com a
// mesma velocidade fixa dos outros testes e o resultado de cada uma sai no registro.
void AVortexGameMode::BeginMatrixSmoke(APlayerController* NewPlayer)
{
    enum class EExpect : uint8 { Climb, Block, Swim };
    struct FLane { const TCHAR* Name; float Y; float Duration; EExpect Expect; float MinTopZ; };
    static const FLane Lanes[] = {
        { TEXT("RAMP20"), 2000.f, 5.f, EExpect::Climb, 264.f },
        { TEXT("RAMP40"), 2600.f, 6.f, EExpect::Climb, 534.f },
        { TEXT("RAMP50"), 3200.f, 5.f, EExpect::Block, 0.f },
        { TEXT("STAIRS"), 3800.f, 5.f, EExpect::Climb, 244.f },
        { TEXT("POOL"), 4400.f, 10.f, EExpect::Swim, 0.f },
    };
    struct FState
    {
        int32 Lane = -1;
        float Elapsed = 0.f;
        float MaxZ = -1e9f;
        float MinSwimZ = 1e9f;
        bool bSawSwim = false;
        bool bOutOnPlatform = false;
        int32 Failed = 0;
    };
    TWeakObjectPtr<APlayerController> WeakController(NewPlayer);
    TSharedRef<FState> State = MakeShared<FState>();
    FTimerDelegate Sample = FTimerDelegate::CreateWeakLambda(this, [this, WeakController, State]()
    {
        AVortexCharacter* Character = WeakController.IsValid() ? Cast<AVortexCharacter>(WeakController->GetPawn()) : nullptr;
        if (!Character)
        {
            return;
        }
        UVortexCharacterMovementComponent* Movement = Character->GetVortexMovement();
        auto StartLane = [&](int32 Index)
        {
            State->Lane = Index;
            State->Elapsed = 0.f;
            State->MaxZ = -1e9f;
            State->MinSwimZ = 1e9f;
            State->bSawSwim = false;
            State->bOutOnPlatform = false;
            Movement->GroundFriction = 0.f;
            Movement->BrakingDecelerationWalking = 0.f;
            Movement->Velocity = FVector::ZeroVector;
            Movement->SetMovementMode(MOVE_Falling);
            Character->SetActorLocation(FVector(-750.f, Lanes[Index].Y, 114.f), false, nullptr, ETeleportType::TeleportPhysics);
        };
        if (State->Lane < 0)
        {
            StartLane(0);
            return;
        }
        const FLane& Lane = Lanes[State->Lane];
        const FVector Position = Character->GetActorLocation();
        State->Elapsed += 0.02f;
        State->MaxZ = FMath::Max(State->MaxZ, Position.Z);
        if (Movement->IsSwimmingInWater())
        {
            State->bSawSwim = true;
            State->MinSwimZ = FMath::Min(State->MinSwimZ, Position.Z);
        }
        // piscina: depois de nadar, chegou andando na plataforma do outro lado da margem
        State->bOutOnPlatform |= State->bSawSwim && Movement->IsMovingOnGround() &&
            Position.X > 1650.f && Position.X < 2150.f && Position.Z > 44.f && Position.Z < 114.f;
        Movement->Velocity.X = 460.f;
        if (State->Elapsed < Lane.Duration)
        {
            return;
        }
        bool bPassed = false;
        switch (Lane.Expect)
        {
        case EExpect::Climb: bPassed = State->MaxZ > Lane.MinTopZ && Position.X > 450.f; break;
        case EExpect::Block: bPassed = Position.X < 200.f && State->MaxZ < 250.f; break;
        // nadou, boiou (centro agora 26 cm mais baixo) e saiu pela margem
        case EExpect::Swim: bPassed = State->bSawSwim && State->MinSwimZ > -146.f && State->bOutOnPlatform; break;
        }
        State->Failed += bPassed ? 0 : 1;
        UE_LOG(LogTemp, Display, TEXT("Vortex movement: MATRIX_%s_%s x=%.1f z=%.1f maxZ=%.1f swim=%d minSwimZ=%.1f outOnPlatform=%d"),
            Lane.Name, bPassed ? TEXT("PASS") : TEXT("FAIL"), Position.X, Position.Z, State->MaxZ,
            State->bSawSwim ? 1 : 0, State->bSawSwim ? State->MinSwimZ : 0.f, State->bOutOnPlatform ? 1 : 0);
        if (State->Lane + 1 < UE_ARRAY_COUNT(Lanes))
        {
            StartLane(State->Lane + 1);
            return;
        }
        GetWorldTimerManager().ClearTimer(MatrixSmokeTimer);
        UE_LOG(LogTemp, Display, TEXT("Vortex movement: MATRIX_SMOKE_%s failed=%d"), State->Failed == 0 ? TEXT("PASS") : TEXT("FAIL"), State->Failed);
        FPlatformMisc::RequestExit(false);
    });
    GetWorldTimerManager().SetTimer(MatrixSmokeTimer, Sample, 0.02f, true, 1.f);
}

void AVortexGameMode::BeginMovementSmoke(APlayerController* NewPlayer, bool bStepProbe)
{
    struct FMetrics
    {
        float Elapsed = 0.f;
        float MaximumVerticalStep = 0.f;
        float InitialSlopeHeight = -1.f;
        float FinalSlopeHeight = -1.f;
        FVector PreviousLocation = FVector::ZeroVector;
        bool bConfigured = false;
        bool bJumped = false;
        bool bSawAirborne = false;
        bool bLandedAfterJump = false;
        float MaximumJumpHeight = 0.f;
        float HeightOnStepLedge = 0.f;
    };

    TWeakObjectPtr<APlayerController> WeakController(NewPlayer);
    TSharedRef<FMetrics> Metrics = MakeShared<FMetrics>();
    FTimerDelegate Sample = FTimerDelegate::CreateWeakLambda(this, [this, WeakController, Metrics, bStepProbe]()
    {
        AVortexCharacter* Character = WeakController.IsValid()
            ? Cast<AVortexCharacter>(WeakController->GetPawn()) : nullptr;
        if (!Character)
        {
            return;
        }
        if (!Metrics->bConfigured)
        {
            Character->GetCharacterMovement()->GroundFriction = 0.f;
            Character->GetCharacterMovement()->BrakingDecelerationWalking = 0.f;
            if (bStepProbe)
            {
                Character->SetActorLocation(FVector(-750.f, 1000.f, 114.f), false, nullptr,
                    ETeleportType::TeleportPhysics);
            }
            Metrics->bConfigured = true;
        }
        const FVector Position = Character->GetActorLocation();
        Metrics->Elapsed += 0.02f;
        if (bStepProbe)
        {
            if (Position.X > 250.f && Position.X < 550.f)
            {
                Metrics->HeightOnStepLedge = FMath::Max(Metrics->HeightOnStepLedge, Position.Z);
            }
            Character->GetCharacterMovement()->Velocity.X = 460.f;
            if (Metrics->Elapsed >= 7.f)
            {
                GetWorldTimerManager().ClearTimer(MovementSmokeTimer);
                const bool bPassed = Position.X > 550.f && Position.X < 670.f &&
                    Position.Z > 84.f && Position.Z < 134.f &&
                    Metrics->HeightOnStepLedge > 84.f && Metrics->HeightOnStepLedge < 134.f;
                UE_LOG(LogTemp, Display,
                    TEXT("Vortex movement: STEP_SMOKE_%s x=%.1f z=%.1f ledgeZ=%.1f"),
                    bPassed ? TEXT("PASS") : TEXT("FAIL"), Position.X, Position.Z,
                    Metrics->HeightOnStepLedge);
                FPlatformMisc::RequestExit(false);
            }
            return;
        }
        if (Position.X >= 0.f && Position.X <= 1000.f)
        {
            if (Metrics->InitialSlopeHeight < 0.f)
            {
                Metrics->InitialSlopeHeight = Position.Z;
            }
            Metrics->FinalSlopeHeight = Position.Z;
            if (!Metrics->PreviousLocation.IsZero())
            {
                Metrics->MaximumVerticalStep = FMath::Max(Metrics->MaximumVerticalStep,
                    FMath::Abs(Position.Z - Metrics->PreviousLocation.Z));
            }
        }
        Metrics->PreviousLocation = Position;
        // Drive the physics probe directly; UI input is validated separately in interactive play.
        Character->GetCharacterMovement()->Velocity.X = 460.f;
        if (Metrics->Elapsed >= 5.f && !Metrics->bJumped)
        {
            Character->Jump();
            Metrics->bJumped = true;
        }
        if (Metrics->bJumped)
        {
            Metrics->MaximumJumpHeight = FMath::Max(Metrics->MaximumJumpHeight, Position.Z);
            Metrics->bSawAirborne |= Character->GetCharacterMovement()->IsFalling();
            Metrics->bLandedAfterJump |= Metrics->bSawAirborne &&
                Character->GetCharacterMovement()->IsMovingOnGround();
        }

        if (Metrics->Elapsed >= 9.f)
        {
            GetWorldTimerManager().ClearTimer(MovementSmokeTimer);
            const bool bJumpPassed = GetNetMode() == NM_DedicatedServer ||
                (Metrics->bSawAirborne && Metrics->bLandedAfterJump && Metrics->MaximumJumpHeight > 394.f);
            const bool bPassed = Position.X > 950.f && Position.X < 1120.f &&
                Metrics->InitialSlopeHeight > 0.f &&
                Metrics->FinalSlopeHeight - Metrics->InitialSlopeHeight > 200.f &&
                Metrics->MaximumVerticalStep < 25.f && bJumpPassed;
            UE_LOG(LogTemp, Display,
                TEXT("Vortex movement: MOVEMENT_SMOKE_%s x=%.1f z=%.1f climb=%.1f maxVerticalStep=%.1f jumpHeight=%.1f landed=%d mode=%d"),
                bPassed ? TEXT("PASS") : TEXT("FAIL"), Position.X, Position.Z,
                Metrics->FinalSlopeHeight - Metrics->InitialSlopeHeight,
                Metrics->MaximumVerticalStep, Metrics->MaximumJumpHeight,
                Metrics->bLandedAfterJump ? 1 : 0,
                static_cast<int32>(Character->GetCharacterMovement()->MovementMode));
            FPlatformMisc::RequestExit(false);
        }
    });
    GetWorldTimerManager().SetTimer(MovementSmokeTimer, Sample, 0.02f, true, 1.f);
}
#endif

#if !UE_BUILD_SHIPPING
void AVortexGameMode::BeginCombatSmoke(APlayerController* NewPlayer)
{
    TWeakObjectPtr<APlayerController> Player = NewPlayer;
    FTimerHandle SmokeTimer;
    GetWorldTimerManager().SetTimer(SmokeTimer, FTimerDelegate::CreateWeakLambda(this, [this, Player]()
    {
        AVortexCharacter* Source = Player.IsValid() ? Cast<AVortexCharacter>(Player->GetPawn()) : nullptr;
        bool bPassed = Source && Source->GetCombat() && Source->GetAbilitySystemComponent();
        if (bPassed)
        {
            const FTransform SpawnAt(Source->GetActorRotation(), Source->GetActorLocation() + Source->GetActorForwardVector() * 180.f);
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AVortexCharacter* Target = GetWorld()->SpawnActor<AVortexCharacter>(AVortexCharacter::StaticClass(), SpawnAt, Params);
            bPassed = Target && Target->GetAbilitySystemComponent() && Target->GetCombat();
            if (bPassed)
            {
                UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
                const FGameplayAttribute Health = UVortexAttributes::GetHealthAttribute();
                bPassed &= !Source->GetCombat()->TryMeleeHit(Target, 20.f, 260.f, 0.3f); // same faction
                Target->GetCombat()->Faction = EVortexFaction::Hostile;
                bPassed &= Source->GetCombat()->TryMeleeHit(Target, 20.f, 260.f, 0.3f);
                bPassed &= FMath::IsNearlyEqual(TargetASC->GetNumericAttribute(Health), 80.f);
                bPassed &= !Source->GetCombat()->TryMeleeHit(Target, -2.f, 260.f, 0.3f);
                bPassed &= !Source->GetCombat()->TryMeleeHit(Target, 20.f, 100.f, 0.3f);
                bPassed &= !Source->GetCombat()->TryMeleeHit(Target, 20.f, 260.f, 1.1f);
                Source->GetAbilitySystemComponent()->SetNumericAttributeBase(Health, 70.f);
                // recuperar custa 10 de mana (a mana máxima vem da ficha: 60 no herói humano)
                const float ManaBefore = Source->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetManaAttribute());
                Source->GetAbilitySystemComponent()->TryActivateAbilityByClass(UVortexRecoverAbility::StaticClass());
                bPassed &= FMath::IsNearlyEqual(Source->GetAbilitySystemComponent()->GetNumericAttribute(Health), 90.f);
                bPassed &= FMath::IsNearlyEqual(Source->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetManaAttribute()), ManaBefore - 10.f);
                Target->Destroy();
            }
        }
        UE_LOG(LogTemp, Display, TEXT("VORTEX_COMBAT_SMOKE_%s"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
        FPlatformMisc::RequestExit(false);
    }), 2.f, false);
}

void AVortexGameMode::BeginSwordSmoke(APlayerController* NewPlayer)
{
    TWeakObjectPtr<APlayerController> Player = NewPlayer;
    FTimerHandle SetupTimer;
    GetWorldTimerManager().SetTimer(SetupTimer, FTimerDelegate::CreateWeakLambda(this, [this, Player]()
    {
        AVortexCharacter* Source = Player.IsValid() ? Cast<AVortexCharacter>(Player->GetPawn()) : nullptr;
        if (!Source || !Source->GetWeapon() || !Source->GetWeapon()->GetDefinition())
        {
            UE_LOG(LogTemp, Error, TEXT("VORTEX_SWORD_SMOKE_FAIL missing weapon"));
            FPlatformMisc::RequestExit(false);
            return;
        }
        const UVortexWeaponDefinition* Definition = Source->GetWeapon()->GetDefinition();
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        const FTransform SpawnAt(Source->GetActorRotation(), Source->GetActorLocation() + Source->GetActorForwardVector() * 180.f);
        AVortexCharacter* Target = GetWorld()->SpawnActor<AVortexCharacter>(AVortexCharacter::StaticClass(), SpawnAt, Params);
        if (!Target)
        {
            UE_LOG(LogTemp, Error, TEXT("VORTEX_SWORD_SMOKE_FAIL target spawn"));
            FPlatformMisc::RequestExit(false);
            return;
        }
        Target->GetCombat()->Faction = EVortexFaction::Hostile;
        Target->GetCharacterMovement()->DisableMovement();
        const bool bAssets = Definition->Swings.Num() == 3 && Definition->VisualMesh && Definition->GuardAnimation && Definition->SecondaryAbility &&
            Source->GetWeaponVisual()->GetStaticMesh() &&   // o modelo vem do swords.json (blocos), não do VisualMesh do DataAsset
            Definition->Swings[0].Animation && Definition->Swings[1].Animation && Definition->Swings[2].Animation;
        UE_LOG(LogTemp, Display, TEXT("VORTEX_SWORD_ASSETS swings=%d mesh=%s visual=%s guard=%d secondary=%d"), Definition->Swings.Num(),
            *GetNameSafe(Definition->VisualMesh), *GetNameSafe(Source->GetWeaponVisual()->GetStaticMesh()), Definition->GuardAnimation ? 1 : 0, Definition->SecondaryAbility ? 1 : 0);
        TWeakObjectPtr<AVortexCharacter> Attacker = Source, Victim = Target;
        Source->GetWeapon()->PressPrimary();
        FTimerHandle FirstBuffer, SecondBuffer, AssertTimer;
        GetWorldTimerManager().SetTimer(FirstBuffer, FTimerDelegate::CreateWeakLambda(this, [Attacker]()
        {
            if (Attacker.IsValid()) Attacker->GetWeapon()->PressPrimary();
        }), .30f, false);
        GetWorldTimerManager().SetTimer(SecondBuffer, FTimerDelegate::CreateWeakLambda(this, [Attacker]()
        {
            if (Attacker.IsValid()) Attacker->GetWeapon()->PressPrimary();
        }), .90f, false);
        GetWorldTimerManager().SetTimer(AssertTimer, FTimerDelegate::CreateWeakLambda(this, [this, Attacker, Victim, bAssets]()
        {
            const float Health = Victim.IsValid()
                ? Victim->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) : -1.f;
            // 10/11: o combo tira 6 (1,5 + 1,5 + 3) desde a conta de dano de 07/11; antes tirava 4
            const bool bCombo = bAssets && Attacker.IsValid() && Victim.IsValid() && FMath::IsNearlyEqual(Health, 94.f) &&
                Attacker->GetWeapon()->GetAttackState().SwingIndex == INDEX_NONE;
            if (!bCombo)
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_SWORD_SMOKE_FAIL comboHealth=%.1f assets=%d swing=%d"), Health, bAssets ? 1 : 0,
                    Attacker.IsValid() ? Attacker->GetWeapon()->GetAttackState().SwingIndex : -99);
                FPlatformMisc::RequestExit(false);
                return;
            }
            Victim->SetActorRotation((Attacker->GetActorLocation() - Victim->GetActorLocation()).Rotation());
            Victim->GetWeapon()->SetGuardPressed(true);
            const bool bParried = Attacker->GetCombat()->TryMeleeHit(Victim.Get(), 10.f, 430.f, .1f) &&
                FMath::IsNearlyEqual(Victim->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()), Health);
            FTimerHandle GuardTimer;
            GetWorldTimerManager().SetTimer(GuardTimer, FTimerDelegate::CreateWeakLambda(this, [Attacker, Victim, bParried, Health]()
            {
                bool bPassed = bParried && Attacker.IsValid() && Victim.IsValid();
                float Blocked = -1.f;
                if (bPassed)
                {
                    bPassed &= Attacker->GetCombat()->TryMeleeHit(Victim.Get(), 10.f, 430.f, .1f);
                    Blocked = Victim->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
                    Victim->GetWeapon()->SetGuardPressed(false);
                    bPassed &= Attacker->GetCombat()->TryMeleeHit(Victim.Get(), 10.f, 430.f, .1f);
                }
                const float FinalHealth = Victim.IsValid()
                    ? Victim->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) : -1.f;
                // 10/11: o dano agora passa pela defesa (07/11; 10 vira 6), então sem números fixos: o golpe bloqueado tira algo, e menos que o livre
                bPassed &= Blocked < Health && FinalHealth < Blocked && Health - Blocked < Blocked - FinalHealth;
                UE_LOG(LogTemp, Display, TEXT("VORTEX_SWORD_SMOKE_%s combo=%.1f block=%.1f release=%.1f"),
                    bPassed ? TEXT("PASS") : TEXT("FAIL"), Health, Blocked, FinalHealth);
                FPlatformMisc::RequestExit(false);
            }), .30f, false);
        }), 2.6f, false);
    }), 2.f, false);
}

void AVortexGameMode::BeginSwordShot(APlayerController* NewPlayer, bool bNormalCamera)
{
    struct FShot { float Time = 0.f, MinHeadClearance = FLT_MAX, MinTorsoClearance = FLT_MAX;
        bool bPlaced = false, bIdleShot = false, bStillAttack = false, bSecond = false, bThird = false, bStillShot = false, bWalkShot = false; };
    TWeakObjectPtr<APlayerController> WeakController(NewPlayer);
    TSharedRef<FShot> Shot = MakeShared<FShot>();
    TWeakObjectPtr<AVortexGameMode> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis, WeakController, Shot, bNormalCamera](float Delta) -> bool
    {
        AVortexGameMode* Self = WeakThis.Get();
        APlayerController* Controller = WeakController.Get();
        AVortexCharacter* Character = Controller ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        AVortexTerrainStreamer* Streamer = nullptr;
        if (Self) for (TActorIterator<AVortexTerrainStreamer> It(Self->GetWorld()); It; ++It) Streamer = *It;
        if (!Self) return false;
        if (!Character || (Streamer && !Streamer->IsReady())) return true;
        // o primeiro quadro traz o tempo de carregamento inteiro: sem o limite as três fotos eram pedidas
        // no mesmo quadro e só a última saía
        Shot->Time += FMath::Min(Delta, .05f);
        if (!Shot->bPlaced)
        {
            if (Streamer)
            {
                const FVector At(-30000.f, -8000.f, 0.f);
                Character->SetActorLocation(FVector(At.X, At.Y, Streamer->GroundAt(At.X, At.Y) + 94.f), false, nullptr, ETeleportType::TeleportPhysics);
            }
            Shot->bPlaced = true;
        }
        if (!bNormalCamera) Character->UseGodotBenchCamera(380.f, 0.f);
        Controller->SetControlRotation(FRotator(-10.f, 90.f, 0.f));
        if (!Shot->bIdleShot && Shot->Time >= 1.5f)
        {
            UStaticMeshComponent* Visual = Character->GetWeaponVisual();
            UE_LOG(LogTemp, Display, TEXT("VORTEX_SWORD_VISUAL actor=%s bone=%s mesh=%s origin=%s extent=%s scale=%s visible=%d"),
                *Character->GetActorLocation().ToCompactString(),
                *Character->GetMesh()->GetSocketTransform(TEXT("Sword")).GetLocation().ToCompactString(),
                *GetNameSafe(Visual->GetStaticMesh()),
                *Visual->Bounds.Origin.ToCompactString(), *Visual->Bounds.BoxExtent.ToCompactString(),
                *Visual->GetComponentScale().ToCompactString(), Visual->IsVisible() ? 1 : 0);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / (bNormalCamera ? TEXT("Screenshots/sword_normal_idle.png") : TEXT("Screenshots/sword_idle.png")), false, false);
            Shot->bIdleShot = true;
        }
        if (!Shot->bStillAttack && Shot->Time >= 2.f) { Character->GetWeapon()->PressPrimary(); Shot->bStillAttack = true; }
        if (!Shot->bSecond && Shot->Time >= 2.45f) { Character->GetWeapon()->PressPrimary(); Shot->bSecond = true; }
        if (!Shot->bStillShot && Shot->Time >= 2.25f)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / (bNormalCamera ? TEXT("Screenshots/sword_normal_still.png") : TEXT("Screenshots/sword_still.png")), false, false);
            Shot->bStillShot = true;
        }
        if (Shot->Time >= 3.f && Shot->Time < 4.5f) Character->AddMovementInput(FVector::ForwardVector, 1.f);
        if (!Shot->bThird && Shot->Time >= 3.05f) { Character->GetWeapon()->PressPrimary(); Shot->bThird = true; }
        if (!Shot->bWalkShot && Shot->Time >= 3.5f)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / (bNormalCamera ? TEXT("Screenshots/sword_normal_walking.png") : TEXT("Screenshots/sword_walking.png")), false, false);
            Shot->bWalkShot = true;
        }
        if (Shot->Time >= 2.f && Shot->Time <= 4.2f)
        {
            if (const UStaticMeshComponent* Visual = Character->GetWeaponVisual())
            {
                // Imported embedded sword's local blade axis is +Y, tip at 73.44 cm.
                const FVector Grip = Visual->GetComponentLocation();
                const FVector Tip = Visual->GetComponentTransform().TransformPosition(FVector(0.f, 73.44f, 0.f));
                Shot->MinHeadClearance = FMath::Min(Shot->MinHeadClearance,
                    FMath::PointDistToSegment(Character->GetMesh()->GetSocketLocation(TEXT("Head")), Grip, Tip));
                Shot->MinTorsoClearance = FMath::Min(Shot->MinTorsoClearance,
                    FMath::PointDistToSegment(Character->GetMesh()->GetSocketLocation(TEXT("Torso")), Grip, Tip));
            }
        }
        if (Shot->Time >= 5.f)
        {
            UE_LOG(LogTemp, Display, TEXT("VORTEX_SWORD_SHOT_DONE still=%d walking=%d headClearance=%.1f torsoClearance=%.1f"),
                Shot->bStillShot, Shot->bWalkShot, Shot->MinHeadClearance, Shot->MinTorsoClearance);
            FPlatformMisc::RequestExit(false);
            return false;
        }
        return true;
    }));
}

void AVortexGameMode::BeginWolfSmoke(APlayerController* NewPlayer)
{
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, Controller]()
    {
        AVortexCharacter* Player = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        if (!Player) { UE_LOG(LogTemp, Error, TEXT("VORTEX_WOLF_SMOKE_FAIL no player")); FPlatformMisc::RequestExit(false); return; }
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        const FVector Position = Player->GetActorLocation() + Player->GetActorForwardVector() * 400.f;
        AVortexWolf* Wolf = GetWorld()->SpawnActor<AVortexWolf>(AVortexWolf::StaticClass(), Position,
            (Player->GetActorLocation() - Position).Rotation(), Params);
        TWeakObjectPtr<AVortexWolf> WeakWolf(Wolf);
        TWeakObjectPtr<AVortexCharacter> WeakPlayer(Player);
        GetWorldTimerManager().SetTimer(WolfSmokeTimer, FTimerDelegate::CreateWeakLambda(this, [this, WeakWolf, WeakPlayer]()
        {
            AVortexWolf* W = WeakWolf.Get();
            AVortexCharacter* P = WeakPlayer.Get();
            const float PlayerHealth = P ? P->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) : -1.f;
            const bool bAttacked = W && P && PlayerHealth < 100.f && W->Health() > 0.f;
            bool bDeath = false;
            if (bAttacked)
            {
                P->SetActorRotation((W->GetActorLocation() - P->GetActorLocation()).Rotation());
                bDeath = P->GetCombat()->TryMeleeHit(W, 50.f, 430.f, .1f) && W->Health() <= 0.f && W->WolfState == EVortexWolfState::Dead;
            }
            UE_LOG(LogTemp, Display, TEXT("VORTEX_WOLF_SMOKE_%s playerHealth=%.1f wolfHealth=%.1f state=%d"),
                bAttacked && bDeath ? TEXT("PASS") : TEXT("FAIL"), PlayerHealth, W ? W->Health() : -1.f,
                W ? int32(W->WolfState) : -1);
            FPlatformMisc::RequestExit(false);
        // uivo (1,8 s) + bote (0,55 s até o impacto) + o começo da IA: 3 s ficava no limite
        }), 4.0f, false);
    }));
}

// Lobo perseguindo o herói de verdade (a IA decide): o herói fica a 14 m; depois do uivo o lobo
// corre. Mede quanto o osso da pata e os vértices se movem (osso mexe e vértice não = pele
// quebrada), a velocidade, os clipes usados e grava três fotos do passo.
void AVortexGameMode::BeginWolfMotionProbe(APlayerController* NewPlayer)
{
    struct FProbe { float Time = 0.f; bool bPlaced = false; int32 Samples = 0; FVector LastBone = FVector::ZeroVector;
        TArray<FVector3f> LastVerts; double BoneTravel = 0.0, VertexTravel = 0.0, SpeedSum = 0.0, SpeedJerk = 0.0, LastSpeed = -1.0;
        TSet<FString> Clips; int32 IdleWhileChasing = 0, ChasingSamples = 0; float MaxInside = 0.f; TWeakObjectPtr<ACameraActor> Camera; };
    TSharedRef<FProbe> Probe = MakeShared<FProbe>();
    TWeakObjectPtr<AVortexGameMode> Self(this);
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Controller, Probe](float Delta) -> bool
    {
        AVortexGameMode* Mode = Self.Get();
        if (!Mode) return false;
        AVortexWolf* Wolf = nullptr;
        if (TActorIterator<AVortexWolf> It(Mode->GetWorld()); It) { Wolf = *It; }
        APlayerController* PC = Controller.Get();
        if (!PC || !Wolf || !Wolf->bTerrainReady) return true;
        USkeletalMeshComponent* Mesh = Wolf->GetMesh();
        if (!Probe->bPlaced)
        {
            Probe->bPlaced = true;
            Probe->Camera = Mode->GetWorld()->SpawnActor<ACameraActor>();
            PC->SetViewTarget(Probe->Camera.Get());
            if (AVortexCharacter* Hero = Cast<AVortexCharacter>(PC->GetPawn()))
            {
                const FVector At = Wolf->GetActorLocation() + FVector(1400.f, 0.f, 0.f);
                float Ground = 0.f;
                for (TActorIterator<AVortexTerrainStreamer> It(Mode->GetWorld()); It; ++It) Ground = It->GroundAt(At.X, At.Y);
                Hero->SetActorLocation(FVector(At.X, At.Y, Ground + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
            }
        }
        Probe->Time += Delta;
        if (ACameraActor* Camera = Probe->Camera.Get())
        {
            const FVector Side = FVector::CrossProduct(Wolf->GetActorForwardVector(), FVector::UpVector).GetSafeNormal();
            const FVector Focus = Wolf->GetActorLocation() + FVector(0.f, 0.f, 20.f);
            const FVector Eye = Focus + Side * 450.f + FVector(0.f, 0.f, 60.f);
            Camera->SetActorLocationAndRotation(Eye, (Focus - Eye).Rotation());
        }
        const double Speed = Wolf->GetVelocity().Size2D();
        if (const UVortexWolfAnimInstance* Anim = Cast<UVortexWolfAnimInstance>(Mesh->GetAnimInstance())) Probe->Clips.Add(GetNameSafe(Anim->Current).Replace(TEXT("A_LoboAmetista_"), TEXT("")));
        if (Probe->LastSpeed >= 0.0 && Probe->Time > 3.f) Probe->SpeedJerk = FMath::Max(Probe->SpeedJerk, FMath::Abs(Speed - Probe->LastSpeed) / FMath::Max(Delta, 1e-3f));
        Probe->LastSpeed = Speed;
        // quanto a cápsula do herói entra no corpo do lobo (caixa do corpo), a qualquer momento
        if (const UBoxComponent* Body = Wolf->FindComponentByClass<UBoxComponent>())
        {
            if (const ACharacter* Hero = Cast<ACharacter>(PC->GetPawn()))
            {
                const FTransform Frame(Body->GetComponentQuat(), Body->GetComponentLocation());
                const FVector Local = Frame.InverseTransformPosition(Hero->GetActorLocation());
                const FVector Ext = Body->GetScaledBoxExtent();
                const float R = Hero->GetCapsuleComponent()->GetScaledCapsuleRadius();
                const float InX = Ext.X + R - FMath::Abs(Local.X), InY = Ext.Y + R - FMath::Abs(Local.Y);
                if (InX > 0.f && InY > 0.f && FMath::Abs(Local.Z) < Ext.Z + 100.f) Probe->MaxInside = FMath::Max(Probe->MaxInside, FMath::Min(InX, InY));
            }
        }
        if (Probe->Time < 3.f) return true;
        Probe->SpeedSum += Speed;
        // perseguindo (corre, circula, volta) mas com a pose parada ou quase sem velocidade
        const EVortexWolfState State = Wolf->WolfState;
        if (State == EVortexWolfState::Chase || State == EVortexWolfState::Circle || State == EVortexWolfState::ReturnHome)
        {
            ++Probe->ChasingSamples;
            const UVortexWolfAnimInstance* Pose = Cast<UVortexWolfAnimInstance>(Mesh->GetAnimInstance());
            Probe->IdleWhileChasing += (Speed < 35.0 || (Pose && GetNameSafe(Pose->Current).EndsWith(TEXT("Idle")))) ? 1 : 0;
        }
        // amostras a cada quadro entre 3 e 4 s (depois do uivo), no espaço do componente
        const FVector Bone = Mesh->GetBoneLocation(TEXT("frontleg1"), EBoneSpaces::ComponentSpace);
        TArray<FMatrix44f> RefToLocals;
        Mesh->CacheRefToLocalMatrices(RefToLocals);
        TArray<FVector3f> Verts;
        const FSkeletalMeshRenderData* Render = Mesh->GetSkeletalMeshRenderData();
        if (Render && !Render->LODRenderData.IsEmpty() && Mesh->GetSkinWeightBuffer(0))
        {
            USkeletalMeshComponent::ComputeSkinnedPositions(Mesh, Verts, RefToLocals, Render->LODRenderData[0], *Mesh->GetSkinWeightBuffer(0));
        }
        if (Probe->Samples > 0)
        {
            Probe->BoneTravel += FVector::Distance(Bone, Probe->LastBone);
            double MaxMove = 0.0;
            for (int32 i = 0; i < Verts.Num() && i < Probe->LastVerts.Num(); ++i) MaxMove = FMath::Max(MaxMove, double(FVector3f::Distance(Verts[i], Probe->LastVerts[i])));
            Probe->VertexTravel += MaxMove;
        }
        Probe->LastBone = Bone;
        Probe->LastVerts = Verts;
        ++Probe->Samples;
        static const float ShotTimes[] = { 3.2f, 3.35f, 3.5f };
        for (int32 i = 0; i < 3; ++i)
        {
            if (Probe->Time >= ShotTimes[i] && Probe->Time - Delta < ShotTimes[i])
            {
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/wolf_walk_%d.png"), i), false, false);
            }
        }
        if (Probe->Time < 7.f) return true;
        UE_LOG(LogTemp, Display, TEXT("VORTEX_WOLF_MOTION dentro_max_cm=%.0f amostras=%d parado_perseguindo=%d/%d velocidade_media=%.0f variacao_max_cm_s2=%.0f osso_cm=%.1f vertice_max_cm=%.1f clipes=%s estado=%d"),
            Probe->MaxInside, Probe->Samples, Probe->IdleWhileChasing, Probe->ChasingSamples, Probe->SpeedSum / FMath::Max(1, Probe->Samples), Probe->SpeedJerk, Probe->BoneTravel, Probe->VertexTravel,
            *FString::Join(Probe->Clips.Array(), TEXT(",")), int32(Wolf->WolfState));
        FPlatformMisc::RequestExit(false);
        return false;
    }));
}

void AVortexGameMode::BeginWolfVisualShot(APlayerController* NewPlayer)
{
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    FTimerHandle ShotTimer;
    GetWorldTimerManager().SetTimer(ShotTimer, FTimerDelegate::CreateWeakLambda(this, [this, Controller]()
    {
        APlayerController* PC = Controller.Get();
        AVortexWolf* Wolf = nullptr;
        if (TActorIterator<AVortexWolf> It(GetWorld()); It) { Wolf = *It; }
        if (!PC || !Wolf || !Wolf->bTerrainReady)
        {
            UE_LOG(LogTemp, Error, TEXT("VORTEX_WOLF_VISUAL_FAIL not ready"));
            FPlatformMisc::RequestExit(false);
            return;
        }
        for (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It; ++It)
        {
            float WolfGround = 0.f, HeroGround = 0.f;
            if (It->GroundReadyAt(-41400.f, -5200.f, WolfGround) &&
                It->GroundReadyAt(-41500.f, -5440.f, HeroGround))
            {
                Wolf->SetActorLocation(FVector(-41400.f, -5200.f, WolfGround + Wolf->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f),
                    false, nullptr, ETeleportType::TeleportPhysics);
                PC->GetPawn()->SetActorLocation(FVector(-41500.f, -5440.f, HeroGround + 70.f), false, nullptr, ETeleportType::TeleportPhysics);
                Wolf->SetActorRotation(FRotator(0.f, 180.f, 0.f));
                break;
            }
        }
        Wolf->GetCharacterMovement()->DisableMovement();
        Wolf->WolfState = EVortexWolfState::Idle;
        PC->SetControlRotation(FRotator(-8.f, (Wolf->GetActorLocation() - PC->GetPawn()->GetActorLocation()).Rotation().Yaw, 0.f));
        UE_LOG(LogTemp, Display, TEXT("VORTEX_WOLF_VISUAL_SHOT wolf=%s mesh=%s fur=%s eyes=%s"),
            *Wolf->GetActorLocation().ToCompactString(), *GetNameSafe(Wolf->GetMesh()->GetSkeletalMeshAsset()),
            *GetNameSafe(Wolf->GetMesh()->GetMaterial(0)), *GetNameSafe(Wolf->GetMesh()->GetMaterial(1)));
        UE_LOG(LogTemp, Display, TEXT("VORTEX_WOLF_AXIS head=%s body=%s meshYaw=%.1f actorYaw=%.1f"), *Wolf->GetMesh()->GetBoneLocation(TEXT("Head"), EBoneSpaces::ComponentSpace).ToCompactString(), *Wolf->GetMesh()->GetBoneLocation(TEXT("Body"), EBoneSpaces::ComponentSpace).ToCompactString(), Wolf->GetMesh()->GetRelativeRotation().Yaw, Wolf->GetActorRotation().Yaw);
        if (const FSkeletalMeshRenderData* Render = Wolf->GetMesh()->GetSkeletalMeshAsset()->GetResourceForRendering())
        {
            if (!Render->LODRenderData.IsEmpty())
            {
                const FColorVertexBuffer& Colors = Render->LODRenderData[0].StaticVertexBuffers.ColorVertexBuffer;
                uint64 R = 0, G = 0, B = 0;
                const uint32 Count = Colors.GetNumVertices();
                for (uint32 Vertex = 0; Vertex < Count; ++Vertex)
                {
                    const FColor Value = Colors.VertexColor(Vertex);
                    R += Value.R; G += Value.G; B += Value.B;
                }
                if (Count) UE_LOG(LogTemp, Display, TEXT("VORTEX_WOLF_IMPORTED_COLOR vertices=%u rgb8=%.1f,%.1f,%.1f"),
                    Count, double(R) / Count, double(G) / Count, double(B) / Count);
            }
        }
        ACameraActor* Camera = GetWorld()->SpawnActor<ACameraActor>();
        Camera->GetCameraComponent()->SetFieldOfView(68.f);
        PC->SetViewTarget(Camera);
        TWeakObjectPtr<AVortexWolf> WolfRef(Wolf);
        TWeakObjectPtr<ACameraActor> CameraRef(Camera);
        const TCHAR* Views[] = { TEXT("front"), TEXT("right"), TEXT("back"), TEXT("left") };
        for (int32 Index = 0; Index < 4; ++Index)
        {
            FTimerHandle FrameTimer;
            GetWorldTimerManager().SetTimer(FrameTimer, FTimerDelegate::CreateWeakLambda(this, [WolfRef, CameraRef, Index, View = FString(Views[Index])]()
            {
                AVortexWolf* Subject = WolfRef.Get();
                ACameraActor* CameraActor = CameraRef.Get();
                if (!Subject || !CameraActor) return;
                const FVector Directions[] = { Subject->GetActorForwardVector(), Subject->GetActorRightVector(),
                    -Subject->GetActorForwardVector(), -Subject->GetActorRightVector() };
                const FVector Focus = Subject->GetActorLocation() + FVector(0.f, 0.f, 15.f);
                const FVector Eye = Focus + Directions[Index] * 380.f + FVector(0.f, 0.f, 70.f);
                CameraActor->SetActorLocationAndRotation(Eye, (Focus - Eye).Rotation());
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() /
                    FString::Printf(TEXT("Screenshots/wolf_%s.png"), *View), false, false);
            }), 0.5f + Index * 0.7f, false);
        }
        FTimerHandle ExitTimer;
        GetWorldTimerManager().SetTimer(ExitTimer, FTimerDelegate::CreateWeakLambda(this, []() { FPlatformMisc::RequestExit(false); }), 3.6f, false);
    }), 18.f, false);
}

void AVortexGameMode::BeginPackSmoke(APlayerController* NewPlayer)
{
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, Controller]()
    {
        AVortexCharacter* Player = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        if (!Player) { UE_LOG(LogTemp, Error, TEXT("VORTEX_PACK_SMOKE_FAIL no player")); FPlatformMisc::RequestExit(false); return; }
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        constexpr int32 GroupId = 4401;
        int32 CountWanted = 5;
        FParse::Value(FCommandLine::Get(), TEXT("VortexPackCount="), CountWanted);
        CountWanted = FMath::Clamp(CountWanted, 3, 7);
        for (int32 Slot = 0; Slot < CountWanted; ++Slot)
        {
            const float Angle = 2.f * UE_PI * Slot / CountWanted;
            const FVector Position = Player->GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 400.f;
            AVortexWolf* Wolf = GetWorld()->SpawnActor<AVortexWolf>(AVortexWolf::StaticClass(), Position,
                (Player->GetActorLocation() - Position).Rotation(), Params);
            if (Wolf) Wolf->SetPack(GroupId, Slot);
        }
        TWeakObjectPtr<AVortexCharacter> WeakPlayer(Player);
        GetWorldTimerManager().SetTimer(WolfSmokeTimer, FTimerDelegate::CreateWeakLambda(this, [this, WeakPlayer, CountWanted]()
        {
            UVortexGroupCoordinator* Groups = GetWorld()->GetSubsystem<UVortexGroupCoordinator>();
            AVortexCharacter* P = WeakPlayer.Get();
            const float Health = P ? P->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) : -1.f;
            const int32 Count = Groups->MemberCount(4401);
            const int32 Tokens = Groups->ActiveTokens(4401);
            const bool bPass = Count == CountWanted && Tokens <= 2 && Health > 0.f && Health < 100.f;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_PACK_SMOKE_%s members=%d tokens=%d health=%.1f"),
                bPass ? TEXT("PASS") : TEXT("FAIL"), Count, Tokens, Health);
            FPlatformMisc::RequestExit(false);
        }), 2.8f, false);
    }));
}

void AVortexGameMode::BeginCameraShot(APlayerController* NewPlayer, bool bBefore)
{
    struct FCameraShot { float Time = 0.f; bool bPlaced = false, bGrounded = false, bIdle = false, bRun = false; };
    TWeakObjectPtr<AVortexGameMode> Self(this);
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    TSharedRef<FCameraShot> Shot = MakeShared<FCameraShot>();
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Controller, Shot, bBefore](float Delta) -> bool
    {
        AVortexGameMode* Mode = Self.Get();
        AVortexCharacter* Character = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        if (!Mode) return false;
        if (!Character) return true;
        AVortexTerrainStreamer* Streamer = nullptr;
        for (TActorIterator<AVortexTerrainStreamer> It(Mode->GetWorld()); It; ++It) Streamer = *It;
        if (Streamer && !Streamer->IsReady()) return true;
        if (!Shot->bPlaced)
        {
            const FVector At(-30000.f, -8000.f, 0.f);
            if (Streamer) Character->SetActorLocation(FVector(At.X, At.Y, Streamer->GroundAt(At.X, At.Y) + 94.f), false, nullptr, ETeleportType::TeleportPhysics);
            Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            Controller->SetControlRotation(FRotator(-10.f, 90.f, 0.f));
            if (bBefore)
            {
                USpringArmComponent* Boom = Character->FindComponentByClass<USpringArmComponent>();
                UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>();
                if (Boom) { Boom->TargetArmLength = 600.f; Boom->SetRelativeLocation(FVector::ZeroVector); Boom->SocketOffset = FVector(0.f, 70.f, 156.f); }
                if (Camera) Camera->SetFieldOfView(64.f);
            }
            Shot->bPlaced = true;
        }
        if (!Shot->bGrounded && Streamer)
        {
            constexpr float X = -30000.f, Y = -8000.f;
            float Ground = 0.f;
            if (!Streamer->GroundReadyAt(X, Y, Ground)) return true;
            Character->SetActorLocation(FVector(X, Y, Ground + AVortexCharacter::HeroCapsuleHalfHeight + 4.f),
                false, nullptr, ETeleportType::TeleportPhysics);
            Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            Shot->bGrounded = true;
        }
        Shot->Time += Delta;
        const FString Prefix = bBefore ? TEXT("camera_before_") : TEXT("camera_after_");
        if (!Shot->bIdle && Shot->Time >= 1.5f)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/") / (Prefix + TEXT("idle.png")), false, false);
            Shot->bIdle = true;
        }
        if (Shot->Time >= 2.f && Shot->Time < 3.5f) Character->AddMovementInput(FVector::ForwardVector, 1.f);
        if (!Shot->bRun && Shot->Time >= 2.6f)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/") / (Prefix + TEXT("run.png")), false, false);
            Shot->bRun = true;
        }
        if (Shot->Time >= 4.f)
        {
            UE_LOG(LogTemp, Display, TEXT("VORTEX_CAMERA_SHOT_DONE before=%d idle=%d run=%d"), bBefore ? 1 : 0, Shot->bIdle ? 1 : 0, Shot->bRun ? 1 : 0);
            FPlatformMisc::RequestExit(false);
            return false;
        }
        return true;
    }));
}

void AVortexGameMode::BeginCameraSequenceShot(APlayerController* NewPlayer)
{
    struct FCameraSequence { float Time = 0.f; bool bPlaced = false; bool bGrounded = false; bool bRotated = false; bool bRunning = false;
        bool bAttacked = false; bool bAttackShot = false; bool bAiming = false; bool bAimShot = false;
        bool bSwapped = false; bool bLeftShot = false; };
    TWeakObjectPtr<AVortexGameMode> Self(this);
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    TSharedRef<FCameraSequence> Shot = MakeShared<FCameraSequence>();
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Controller, Shot](float Delta) -> bool
    {
        AVortexGameMode* Mode = Self.Get();
        AVortexCharacter* Character = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        if (!Mode) return false;
        if (!Character) return true;
        AVortexTerrainStreamer* Streamer = nullptr;
        for (TActorIterator<AVortexTerrainStreamer> It(Mode->GetWorld()); It; ++It) Streamer = *It;
        if (Streamer && !Streamer->IsReady()) return true;
        if (!Shot->bPlaced)
        {
            const FVector At(-30000.f, -8000.f, 0.f);
            if (Streamer) Character->SetActorLocation(FVector(At.X, At.Y, Streamer->GroundAt(At.X, At.Y) + 94.f), false, nullptr, ETeleportType::TeleportPhysics);
            Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            Controller->SetControlRotation(FRotator(-10.f, 90.f, 0.f));
            Shot->bPlaced = true;
        }
        if (!Shot->bGrounded && Streamer)
        {
            constexpr float X = -30000.f, Y = -8000.f;
            float Ground = 0.f;
            if (!Streamer->GroundReadyAt(X, Y, Ground)) return true;
            Character->SetActorLocation(FVector(X, Y, Ground + AVortexCharacter::HeroCapsuleHalfHeight + 4.f),
                false, nullptr, ETeleportType::TeleportPhysics);
            Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            Shot->bGrounded = true;
        }
        Shot->Time += Delta;
        const auto Capture = [](const TCHAR* Name)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/") / Name, false, false);
        };
        if (!Shot->bRotated && Shot->Time >= 1.2f)
        {
            Controller->SetControlRotation(FRotator(-10.f, 135.f, 0.f));
            Capture(TEXT("camera_sequence_turn.png"));
            Shot->bRotated = true;
        }
        if (Shot->Time >= 2.f && Shot->Time < 2.7f) Character->AddMovementInput(FVector::ForwardVector, 1.f);
        if (!Shot->bRunning && Shot->Time >= 2.4f)
        {
            Capture(TEXT("camera_sequence_run.png"));
            Shot->bRunning = true;
        }
        if (!Shot->bAttacked && Shot->Time >= 3.f)
        {
            Character->BeginPrimaryAttack();
            Shot->bAttacked = true;
        }
        if (Shot->bAttacked && !Shot->bAttackShot && Shot->Time >= 3.3f)
        {
            Capture(TEXT("camera_sequence_attack.png"));
            Shot->bAttackShot = true;
        }
        if (!Shot->bAiming && Shot->Time >= 4.f)
        {
            Character->SetAimHeld(true);
            Shot->bAiming = true;
        }
        if (Shot->bAiming && !Shot->bAimShot && Shot->Time >= 4.8f)
        {
            Capture(TEXT("camera_sequence_aim.png"));
            Shot->bAimShot = true;
        }
        if (!Shot->bSwapped && Shot->Time >= 5.f)
        {
            Character->SwapCameraShoulder();
            Shot->bSwapped = true;
        }
        if (Shot->bSwapped && !Shot->bLeftShot && Shot->Time >= 5.8f)
        {
            Capture(TEXT("camera_sequence_left.png"));
            Shot->bLeftShot = true;
        }
        if (Shot->Time >= 6.5f)
        {
            UE_LOG(LogTemp, Display, TEXT("VORTEX_CAMERA_SEQUENCE_DONE turn=%d run=%d attack=%d aim=%d swap=%d"),
                Shot->bRotated ? 1 : 0, Shot->bRunning ? 1 : 0, Shot->bAttacked ? 1 : 0,
                Shot->bAiming ? 1 : 0, Shot->bSwapped ? 1 : 0);
            FPlatformMisc::RequestExit(false);
            return false;
        }
        return true;
    }));
}

void AVortexGameMode::BeginCameraSmoke(APlayerController* NewPlayer)
{
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, Controller]()
    {
        AVortexCharacter* Character = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        USpringArmComponent* Boom = Character ? Character->FindComponentByClass<USpringArmComponent>() : nullptr;
        UCameraComponent* Camera = Character ? Character->FindComponentByClass<UCameraComponent>() : nullptr;
        if (!Character || !Boom || !Camera)
        {
            UE_LOG(LogTemp, Error, TEXT("VORTEX_CAMERA_SMOKE_FAIL missing components"));
            FPlatformMisc::RequestExit(false);
            return;
        }
        Controller->SetControlRotation(FRotator::ZeroRotator);
        AStaticMeshActor* Blocker = GetWorld()->SpawnActor<AStaticMeshActor>(
            Character->GetActorLocation() + FVector(-180.f, 60.f, 80.f), FRotator::ZeroRotator);
        if (Blocker)
        {
            Blocker->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
            Blocker->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
            Blocker->GetStaticMeshComponent()->SetWorldScale3D(FVector(3.f, 3.f, 3.f));
            Blocker->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
            Blocker->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        }
        TWeakObjectPtr<AVortexCharacter> WeakCharacter(Character);
        TWeakObjectPtr<AStaticMeshActor> WeakBlocker(Blocker);
        GetWorldTimerManager().SetTimer(CameraSmokeTimer, FTimerDelegate::CreateWeakLambda(this, [this, WeakCharacter, WeakBlocker]()
        {
            AVortexCharacter* C = WeakCharacter.Get();
            USpringArmComponent* Arm = C ? C->FindComponentByClass<USpringArmComponent>() : nullptr;
            const bool bBlocked = Arm && Arm->IsCollisionFixApplied();
            UE_LOG(LogTemp, Display, TEXT("VORTEX_CAMERA_BLOCKER actor=%s arm=%s camera=%s blocked=%d"),
                *GetNameSafe(WeakBlocker.Get()), Arm ? *Arm->GetComponentLocation().ToCompactString() : TEXT("none"),
                C && C->FindComponentByClass<UCameraComponent>() ? *C->FindComponentByClass<UCameraComponent>()->GetComponentLocation().ToCompactString() : TEXT("none"), bBlocked ? 1 : 0);
            if (WeakBlocker.IsValid()) WeakBlocker->Destroy();
            // logo depois do obstáculo o herói salta 30 m: a câmera precisa vir junto já no quadro
            // seguinte (antes ela voltava "solta" no mundo e ficava para trás)
            TSharedRef<float> FollowGap = MakeShared<float>(-1.f);
            if (C)
            {
                C->SetActorLocation(C->GetActorLocation() + FVector(0.f, 3000.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
                TWeakObjectPtr<AVortexCharacter> Jumper(C);
                TSharedRef<float> Waited = MakeShared<float>(0.f);
                FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Jumper, FollowGap, Waited](float Delta) -> bool
                {
                    *Waited += Delta;
                    if (*Waited < .1f) return true;
                    const AVortexCharacter* X = Jumper.Get();
                    const UCameraComponent* Eye = X ? X->FindComponentByClass<UCameraComponent>() : nullptr;
                    if (Eye) *FollowGap = float(FVector::Dist(Eye->GetComponentLocation(), X->GetActorLocation()));
                    return false;
                }));
            }
            if (C) C->StepZoom(-10.f);
            GetWorldTimerManager().SetTimer(CameraSmokeTimer, FTimerDelegate::CreateWeakLambda(this, [this, WeakCharacter, bBlocked, FollowGap]()
            {
                AVortexCharacter* C2 = WeakCharacter.Get();
                USpringArmComponent* Arm2 = C2 ? C2->FindComponentByClass<USpringArmComponent>() : nullptr;
                const bool bNear = Arm2 && FMath::Abs(Arm2->TargetArmLength - 150.f) < 3.f;
                if (C2) C2->StepZoom(20.f);
                GetWorldTimerManager().SetTimer(CameraSmokeTimer, FTimerDelegate::CreateWeakLambda(this, [this, WeakCharacter, bBlocked, bNear, FollowGap]()
                {
                    AVortexCharacter* C3 = WeakCharacter.Get();
                    USpringArmComponent* Arm3 = C3 ? C3->FindComponentByClass<USpringArmComponent>() : nullptr;
                    FHitResult Hit;
                    const bool bAim = C3 && (C3->TraceAim(Hit) || Hit.TraceEnd != FVector::ZeroVector);
                    const bool bFar = Arm3 && FMath::Abs(Arm3->TargetArmLength - 800.f) < 3.f;
                    if (C3) { C3->SetAimHeld(true); C3->SwapCameraShoulder(); }
                    GetWorldTimerManager().SetTimer(CameraSmokeTimer, FTimerDelegate::CreateWeakLambda(this,
                        [WeakCharacter, bBlocked, bNear, bFar, bAim, FollowGap]()
                    {
                        AVortexCharacter* C4 = WeakCharacter.Get();
                        USpringArmComponent* Arm4 = C4 ? C4->FindComponentByClass<USpringArmComponent>() : nullptr;
                        UCameraComponent* Camera4 = C4 ? C4->FindComponentByClass<UCameraComponent>() : nullptr;
                        const bool bShoulderAim = C4 && Arm4 && Camera4 && C4->bUseControllerRotationYaw &&
                            Arm4->SocketOffset.Y < -60.f && Arm4->TargetArmLength < 213.f && Camera4->FieldOfView < 69.f;
                        if (C4) C4->SetAimHeld(false);
                        // a câmera fica a no máximo ~8 m do herói (braço máximo + ombro)
                        const bool bFollow = *FollowGap >= 0.f && *FollowGap < 1000.f;
                        const bool bPassed = bBlocked && bNear && bFar && bAim && bShoulderAim && bFollow;
                        UE_LOG(LogTemp, Display, TEXT("VORTEX_CAMERA_SMOKE_%s collision=%d near=%d far=%d aim=%d shoulderAim=%d follow=%d gap_cm=%.0f"),
                            bPassed ? TEXT("PASS") : TEXT("FAIL"), bBlocked ? 1 : 0, bNear ? 1 : 0,
                            bFar ? 1 : 0, bAim ? 1 : 0, bShoulderAim ? 1 : 0, bFollow ? 1 : 0, *FollowGap);
                        FPlatformMisc::RequestExit(false);
                    }), .8f, false);
                }), 1.f, false);
            }), 1.f, false);
        }), 1.f, false);
    }));
}

// Fase 13, servidor (dedicado ou anfitrião) com dois jogadores: o primeiro vai para A e o segundo
// para B, a 316 m. O servidor precisa ter chão nos dois pontos; no dedicado nada é visual, no
// anfitrião só A (dele) é visual e B fica só com chão. Depois o segundo salta oito vezes entre
// pontos distantes a cada 0,25 s (tarefas ficam velhas no meio do caminho) e volta para perto de
// A: a área de B precisa ser liberada e nenhum pedaço pode ficar fora da área com folga.
void AVortexGameMode::BeginStreamSmoke()
{
    struct FStreamSmoke
    {
        int32 Step = 0;
        float Time = 0.f, StepTime = 0.f;
        int32 Jumps = 0;
        int32 MaxOutside = 0, MaxLoaded = 0;
        double GroundSeconds = 0.0;
        double MemStart = 0.0, MemPeak = 0.0;
        bool bLevelsOk = false;
        TArray<TWeakObjectPtr<APlayerController>> Players;
    };
    TSharedRef<FStreamSmoke> Smoke = MakeShared<FStreamSmoke>();
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        // anfitrião primeiro: é ele quem desenha a tela no servidor com tela
        if (It->Get() && It->Get()->IsLocalController()) Smoke->Players.Insert(It->Get(), 0);
        else Smoke->Players.Add(It->Get());
    }
    TWeakObjectPtr<AVortexGameMode> Self(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Smoke](float Delta) -> bool
    {
        AVortexGameMode* Mode = Self.Get();
        if (!Mode) return false;
        AVortexTerrainStreamer* Streamer = nullptr;
        for (TActorIterator<AVortexTerrainStreamer> It(Mode->GetWorld()); It; ++It) Streamer = *It;
        if (!Streamer || !Streamer->IsReady() || Smoke->Players.Num() < 2) return true;
        AVortexCharacter* First = Smoke->Players[0].IsValid() ? Cast<AVortexCharacter>(Smoke->Players[0]->GetPawn()) : nullptr;
        AVortexCharacter* Second = Smoke->Players[1].IsValid() ? Cast<AVortexCharacter>(Smoke->Players[1]->GetPawn()) : nullptr;
        if (!First || !Second) return true;
        const FVector2D A(-30000.f, -8000.f), B(0.f, 0.f), Home(-29400.f, -7700.f);
        const auto Place = [Streamer](AVortexCharacter* Character, const FVector2D& At)
        {
            Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            Character->GetCharacterMovement()->Velocity = FVector::ZeroVector;
            Character->SetActorLocation(FVector(At.X, At.Y, Streamer->GroundAt(At.X, At.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
        };
        const auto Land = [Streamer](AVortexCharacter* Character, const FVector2D& At)
        {
            float Ground = 0.f;
            if (!Streamer->GroundReadyAt(At.X, At.Y, Ground)) return false;
            Character->SetActorLocation(FVector(At.X, At.Y, Ground + AVortexCharacter::HeroCapsuleHalfHeight + 4.f), false, nullptr, ETeleportType::TeleportPhysics);
            Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            return true;
        };
        const double Mem = FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0);
        Smoke->MemPeak = FMath::Max(Smoke->MemPeak, Mem);
        Smoke->MaxOutside = FMath::Max(Smoke->MaxOutside, Streamer->ChunksOutsideKeep());
        Smoke->MaxLoaded = FMath::Max(Smoke->MaxLoaded, Streamer->LoadedChunkCount());
        Smoke->Time += Delta;
        Smoke->StepTime += Delta;
        const auto Next = [&Smoke]() { ++Smoke->Step; Smoke->StepTime = 0.f; };
        const auto Finish = [&](bool bPassed, const TCHAR* Reason)
        {
            const bool bDedicated = Mode->GetNetMode() == NM_DedicatedServer;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_STREAM_SMOKE_%s %s dedicated=%d ground_s=%.2f levels=%d outside_max=%d loaded_max=%d loaded_end=%d visual_end=%d stale=%d tick_ms_med=%.2f tick_ms_max=%.2f attach_ms_max=%.2f mem_start_mb=%.0f mem_peak_mb=%.0f mem_end_mb=%.0f"),
                bPassed ? TEXT("PASS") : TEXT("FAIL"), Reason, bDedicated ? 1 : 0, Smoke->GroundSeconds, Smoke->bLevelsOk ? 1 : 0,
                Smoke->MaxOutside, Smoke->MaxLoaded, Streamer->LoadedChunkCount(), Streamer->VisualChunkCount(), Streamer->StaleDiscarded,
                Streamer->TickMsTotal / FMath::Max(1, Streamer->Ticks), Streamer->TickMsMax, Streamer->ChunkGameThreadMsMax,
                Smoke->MemStart, Smoke->MemPeak, Mem);
            FPlatformMisc::RequestExit(false);
        };
        switch (Smoke->Step)
        {
        case 0:
            Place(First, A);
            Place(Second, B);
            Smoke->MemStart = Mem;
            Streamer->ResetTickStats();
            Next();
            return true;
        case 1: // chão nos dois pontos ao mesmo tempo
        {
            if (Smoke->StepTime > 40.f) { Finish(false, TEXT("sem_chao")); return false; }
            float Ground = 0.f;
            if (!Streamer->GroundReadyAt(A.X, A.Y, Ground) || !Streamer->GroundReadyAt(B.X, B.Y, Ground)) return true;
            Smoke->GroundSeconds = Smoke->StepTime;
            Land(First, A);
            Land(Second, B);
            Next();
            return true;
        }
        case 2: // níveis certos depois que tudo assentar
        {
            if (Streamer->PendingChunks() > 0 && Smoke->StepTime < 30.f) return true;
            if (Smoke->StepTime < 2.f) return true;
            const FIntPoint ChunkA = Streamer->ChunkAt(FVector(A, 0.f)), ChunkB = Streamer->ChunkAt(FVector(B, 0.f));
            Smoke->bLevelsOk = Mode->GetNetMode() == NM_DedicatedServer
                ? Streamer->VisualChunkCount() == 0
                : Streamer->HasVisualChunk(ChunkA) && !Streamer->HasVisualChunk(ChunkB) && Streamer->IsChunkLoaded(ChunkB);
            UE_LOG(LogTemp, Display, TEXT("VORTEX_STREAM_SMOKE levels=%d loaded=%d visual=%d collision=%d"),
                Smoke->bLevelsOk ? 1 : 0, Streamer->LoadedChunkCount(), Streamer->VisualChunkCount(), Streamer->CollisionChunkCount());
            Next();
            return true;
        }
        case 3: // saltos rápidos: as tarefas dos pontos anteriores ficam para trás
        {
            static const FVector2D Points[] = { {-80000.f, -40000.f}, {10000.f, 40000.f}, {-60000.f, 45000.f}, {-10000.f, -45000.f},
                {-85000.f, 0.f}, {15000.f, -20000.f}, {-45000.f, 30000.f}, {5000.f, 10000.f} };
            if (Smoke->StepTime < 0.25f && Smoke->Jumps > 0) return true;
            // -VortexStreamJumps=N repete a volta pelos oito pontos (medição de memória)
            static int32 JumpCount = 0;
            if (JumpCount == 0 && !FParse::Value(FCommandLine::Get(), TEXT("VortexStreamJumps="), JumpCount)) JumpCount = UE_ARRAY_COUNT(Points);
            if (Smoke->Jumps < JumpCount)
            {
                Place(Second, Points[Smoke->Jumps++ % UE_ARRAY_COUNT(Points)]);
                Smoke->StepTime = 0.f;
                return true;
            }
            Place(Second, Home);
            Next();
            return true;
        }
        case 4: // de volta perto de A: B e os saltos liberados
        {
            if (Smoke->StepTime > 40.f) { Finish(false, TEXT("sem_assentar")); return false; }
            if (!Land(Second, Home)) return true;
            // 12 s: os clientes medem a memória depois de duas coletas de lixo
            if (Streamer->PendingChunks() > 0 || Smoke->StepTime < 12.f) return true;
            const int32 Bound = FMath::Square(2 * (Streamer->StreamRadius + 1) + 1);
            const bool bReleased = !Streamer->IsChunkLoaded(Streamer->ChunkAt(FVector(B, 0.f))) &&
                Streamer->LoadedChunkCount() <= Bound && Streamer->ChunksOutsideKeep() == 0;
            const bool bPassed = Smoke->bLevelsOk && bReleased && Smoke->MaxOutside == 0;
            Finish(bPassed, bReleased ? TEXT("ok") : TEXT("nao_liberou"));
            return false;
        }
        default:
            return false;
        }
    }));
}

// Fase 14, servidor (local, anfitrião ou dedicado) com o primeiro jogador:
// 1. vai à praça V: os 10 postos do protótipo aparecem (só depois do chão), os 4 de fora não;
// 2. vai longe F: todos somem; o estado de cada ID fica parado enquanto ninguém está perto;
// 3. histerese com a Rhea: a 70 m não aparece, a 50 m aparece, de volta a 70 m continua, a 85 m some;
// 4. volta à praça: a Lary reaparece onde estava e no mesmo ponto da patrulha;
// 5. o estado de todos vai ao save do mundo e volta igual.
// Durante tudo: nunca dois atores com o mesmo ID.
void AVortexGameMode::BeginPopulationSmoke(APlayerController* NewPlayer)
{
    struct FPopulationSmoke
    {
        int32 Step = 0;
        float StepTime = 0.f;
        float GatherSeconds = 0.f;
        int32 MaxDuplicates = 0;
        int32 RheaStart = 0;
        bool bOutsideOk = true, bFrozenOk = false, bHysteresisOk = true, bResumeOk = false, bSaveOk = false;
        FVortexNpcState Frozen;
        FString Failure;
        TWeakObjectPtr<ACameraActor> Camera;
        bool bShot = false;
        int32 Portraits = 0;
    };
    TSharedRef<FPopulationSmoke> Smoke = MakeShared<FPopulationSmoke>();
    TWeakObjectPtr<AVortexGameMode> Self(this);
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Controller, Smoke](float Delta) -> bool
    {
        AVortexGameMode* Mode = Self.Get();
        if (!Mode) return false;
        AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        AVortexTerrainStreamer* Streamer = nullptr;
        for (TActorIterator<AVortexTerrainStreamer> It(Mode->GetWorld()); It; ++It) Streamer = *It;
        UVortexPopulationSubsystem* Population = Mode->GetWorld()->GetSubsystem<UVortexPopulationSubsystem>();
        if (!Hero || !Streamer || !Streamer->IsReady() || !Population || !Population->IsActive()) return true;
        const FName Lary(TEXT("lary")), Rhea(TEXT("rhea"));
        const FVector2D Village(0.f, 1500.f), Far(-30000.f, -8000.f);
        const FVector2D RheaPost(-450.f, -250.f);
        const auto Go = [Hero, Streamer](const FVector2D& At)
        {
            Hero->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            Hero->GetCharacterMovement()->Velocity = FVector::ZeroVector;
            Hero->SetActorLocation(FVector(At.X, At.Y, Streamer->GroundAt(At.X, At.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
        };
        // atores por ID neste quadro
        TMap<FName, int32> Actors;
        for (TActorIterator<AVortexNpc> It(Mode->GetWorld()); It; ++It) if (!It->IsActorBeingDestroyed()) ++Actors.FindOrAdd(It->GetNpcId());
        for (const TPair<FName, int32>& Pair : Actors) Smoke->MaxDuplicates = FMath::Max(Smoke->MaxDuplicates, Pair.Value - 1);
        for (const TCHAR* Outside : { TEXT("sahil"), TEXT("amal"), TEXT("tavian"), TEXT("lyra") })
        {
            Smoke->bOutsideOk &= Population->FindState(Outside) != nullptr && !Actors.Contains(Outside);
        }
        Smoke->StepTime += Delta;
        const auto Next = [&Smoke]() { ++Smoke->Step; Smoke->StepTime = 0.f; };
        const auto Fail = [&Smoke](const TCHAR* Why) { Smoke->Failure = Why; Smoke->Step = 100; Smoke->StepTime = 0.f; };
        const int32 RheaCount = Population->FindState(Rhea) ? Population->FindState(Rhea)->Materializations : -1;
        switch (Smoke->Step)
        {
        case 0: Go(Village); Next(); return true;
        case 1: // todos os do protótipo na praça
            if (Smoke->StepTime > 40.f) { Fail(TEXT("praca_incompleta")); return true; }
            if (Population->MaterializedCount() < Population->InPrototypeCount()) return true;
            Smoke->GatherSeconds = Smoke->StepTime;
            Next();
            return true;
        case 2: // a Lary anda um pouco; os clientes veem todos
            // -VortexPopulationShot: foto da praça (precisa de render)
            if (FParse::Param(FCommandLine::Get(), TEXT("VortexPopulationShot")))
            {
                if (!Smoke->Camera.IsValid())
                {
                    Smoke->Camera = Mode->GetWorld()->SpawnActor<ACameraActor>();
                    const FVector Target(-200.f, 1700.f, Streamer->GroundAt(-200.f, 1700.f) + 120.f);
                    const FVector Eye = Target + FVector(-1700.f, -1300.f, 650.f);
                    Smoke->Camera->SetActorLocationAndRotation(Eye, (Target - Eye).Rotation());
                    Smoke->Camera->GetCameraComponent()->SetFieldOfView(70.f);
                    Smoke->Camera->GetCameraComponent()->bConstrainAspectRatio = false;
                    Controller->SetViewTarget(Smoke->Camera.Get());
                }
                if (Smoke->StepTime >= 4.f && !Smoke->bShot)
                {
                    Smoke->bShot = true;
                    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/npc_praca.png"), false, false);
                }
                // 02/11: de perto, a Lexy, a Julia e o cachorro dela (Saved/Screenshots/npc_<id>.png), uma etapa por vez
                // (o teste não roda todo quadro: o "uma vez" pelo tempo do quadro pulava a Lexy)
                const auto Frame = [&](const AActor* Who, float Distance, float Side, float Up)
                {
                    if (!Who || !Smoke->Camera.IsValid()) return;
                    const FVector Face = Who->GetActorLocation() + FVector(0.f, 0.f, Up * .3f);
                    const FVector Eye = Face + Who->GetActorForwardVector() * Distance + Who->GetActorRightVector() * Side + FVector(0.f, 0.f, Up);
                    Smoke->Camera->SetActorLocationAndRotation(Eye, (Face - Eye).Rotation());
                    Smoke->Camera->GetCameraComponent()->SetFieldOfView(50.f);
                };
                const auto FindNpc = [&](FName Id) -> AActor*
                {
                    for (TActorIterator<AVortexNpc> It(Mode->GetWorld()); It; ++It) if (It->GetNpcId() == Id) return *It;
                    return nullptr;
                };
                static const float Times[] = { 4.6f, 5.4f, 5.8f, 6.6f, 6.8f, 7.6f };
                if (Smoke->Portraits < 6 && Smoke->StepTime >= Times[Smoke->Portraits])
                {
                    switch (Smoke->Portraits)
                    {
                    case 0: Frame(FindNpc(TEXT("lexy")), 300.f, 60.f, 40.f); break;
                    case 1: FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/npc_lexy.png"), false, false); break;
                    case 2: Frame(FindNpc(TEXT("julia")), 320.f, 150.f, 40.f); break;
                    case 3: FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/npc_julia.png"), false, false); break;
                    case 4:
                        // o cachorro: do lado da fonte (virado para a dona, ele às vezes olha para uma parede)
                        for (TActorIterator<AVortexPet> It(Mode->GetWorld()); It; ++It)
                        {
                            // de lado, pelo lado da fonte, e um pouco à frente (o rosto): ele fica virado para a dona
                            const FVector Dog = It->GetActorLocation() + It->GetActorForwardVector() * 20.f + FVector(0.f, 0.f, 35.f);
                            const FVector Toward = (FVector(0.f, 1500.f, Dog.Z) - Dog).GetSafeNormal2D();
                            const FVector Side = It->GetActorRightVector() * (FVector::DotProduct(It->GetActorRightVector(), Toward) >= 0.f ? 1.f : -1.f);
                            const FVector Eye = Dog + Side * 230.f + It->GetActorForwardVector() * 120.f + FVector(0.f, 0.f, 40.f);
                            Smoke->Camera->SetActorLocationAndRotation(Eye, (Dog - Eye).Rotation());
                            Smoke->Camera->GetCameraComponent()->SetFieldOfView(50.f);
                            break;
                        }
                        break;
                    case 5: FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/npc_cachorro.png"), false, false); break;
                    }
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_NPC_SHOT etapa %d"), Smoke->Portraits);
                    ++Smoke->Portraits;
                }
                if (Smoke->StepTime >= 8.f && Smoke->Camera.IsValid()) { Controller->SetViewTarget(Hero); Smoke->Camera->Destroy(); }
            }
            if (Smoke->StepTime < (FParse::Param(FCommandLine::Get(), TEXT("VortexPopulationShot")) ? 8.5f : 6.f)) return true;
            Go(Far);
            Next();
            return true;
        case 3: // todos somem
            if (Smoke->StepTime > 20.f) { Fail(TEXT("nao_sumiram")); return true; }
            if (Population->MaterializedCount() > 0 || Actors.Num() > 0) return true;
            Smoke->Frozen = *Population->FindState(Lary);
            Next();
            return true;
        case 4: // ninguém perto: nada muda no estado
            if (Smoke->StepTime < 3.f) return true;
            Smoke->bFrozenOk = Smoke->Frozen.bPlaced && *Population->FindState(Lary) == Smoke->Frozen;
            Smoke->RheaStart = RheaCount;
            Go(RheaPost + FVector2D(0.f, -7000.f));
            Next();
            return true;
        case 5: // 70 m sem ela: não aparece
            if (Smoke->StepTime < 2.5f) return true;
            Smoke->bHysteresisOk &= !Population->FindActor(Rhea) && RheaCount == Smoke->RheaStart;
            Go(RheaPost + FVector2D(0.f, -5000.f));
            Next();
            return true;
        case 6: // 50 m: aparece
            if (Smoke->StepTime > 15.f) { Fail(TEXT("rhea_nao_apareceu")); return true; }
            if (!Population->FindActor(Rhea)) return true;
            Go(RheaPost + FVector2D(0.f, -7000.f));
            Next();
            return true;
        case 7: // de volta a 70 m: continua (sem alternância)
            if (Smoke->StepTime < 2.5f) return true;
            Smoke->bHysteresisOk &= Population->FindActor(Rhea) != nullptr && RheaCount == Smoke->RheaStart + 1;
            Go(RheaPost + FVector2D(0.f, -8500.f));
            Next();
            return true;
        case 8: // 85 m: some
            if (Smoke->StepTime > 15.f) { Fail(TEXT("rhea_nao_sumiu")); return true; }
            if (Population->FindActor(Rhea)) return true;
            Smoke->bHysteresisOk &= RheaCount == Smoke->RheaStart + 1;
            // nada na praça mudou com a Lary longe
            Smoke->bFrozenOk &= FVector2D(Population->FindState(Lary)->Location).Equals(FVector2D(Smoke->Frozen.Location), 0.5f) &&
                Population->FindState(Lary)->PatrolIndex == Smoke->Frozen.PatrolIndex;
            Go(Village);
            Next();
            return true;
        case 9: // a Lary volta onde estava
        {
            if (Smoke->StepTime > 40.f) { Fail(TEXT("lary_nao_voltou")); return true; }
            const AVortexNpc* Npc = Population->FindActor(Lary);
            if (!Npc) return true;
            Smoke->bResumeOk = FVector2D(Npc->GetActorLocation()).Equals(FVector2D(Smoke->Frozen.Location), 25.f) &&
                Npc->GetPatrolIndex() == Smoke->Frozen.PatrolIndex &&
                Population->FindState(Lary)->Materializations == Smoke->Frozen.Materializations + 1;
            Next();
            return true;
        }
        case 10: // save do mundo: escreve, grava, lê de volta
        {
            if (Population->MaterializedCount() < Population->InPrototypeCount() && Smoke->StepTime < 20.f) return true;
            UVortexSaveSubsystem* Saves = Mode->GetGameInstance() ? Mode->GetGameInstance()->GetSubsystem<UVortexSaveSubsystem>() : nullptr;
            UVortexWorldSave* World = Mode->ActiveWorldSave;
            if (Saves && World)
            {
                Population->WriteTo(*World);
                const TArray<FVortexNpcState> Written = World->NpcStates;
                const UVortexWorldSave* Loaded = Saves->SaveHostedWorld(World) ? Saves->LoadHostedWorld(World->WorldId) : nullptr;
                Smoke->bSaveOk = Loaded && Written.Num() == Population->NpcCount() && Loaded->NpcStates == Written;
                const FString Slot = TEXT("Vortex_World_") + World->WorldId.ToString(EGuidFormats::Digits);
                for (const TCHAR* Suffix : { TEXT(""), TEXT("_backup"), TEXT("_staging") }) UGameplayStatics::DeleteGameInSlot(Slot + Suffix, 0);
            }
            Next();
            return true;
        }
        case 11: // clientes veem a praça de novo antes do fim
            if (Smoke->StepTime < 3.f) return true;
            Smoke->Step = 100;
            return true;
        default:
        {
            const bool bPassed = Smoke->Failure.IsEmpty() && Smoke->MaxDuplicates == 0 && Smoke->bOutsideOk && Smoke->bFrozenOk &&
                Smoke->bHysteresisOk && Smoke->bResumeOk && Smoke->bSaveOk;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_POPULATION_SMOKE_%s motivo=%s net=%d postos=%d prototipo=%d reunidos_s=%.2f duplicados=%d fora=%d congelado=%d histerese=%d retomada=%d save=%d lary_andou_cm=%.0f lary_ponto=%d aparicoes=%d sumicos=%d update_ms_med=%.3f update_ms_max=%.2f spawn_ms_max=%.2f"),
                bPassed ? TEXT("PASS") : TEXT("FAIL"), Smoke->Failure.IsEmpty() ? TEXT("ok") : *Smoke->Failure, int32(Mode->GetNetMode()),
                Population->NpcCount(), Population->InPrototypeCount(), Smoke->GatherSeconds, Smoke->MaxDuplicates,
                Smoke->bOutsideOk ? 1 : 0, Smoke->bFrozenOk ? 1 : 0, Smoke->bHysteresisOk ? 1 : 0, Smoke->bResumeOk ? 1 : 0, Smoke->bSaveOk ? 1 : 0,
                float(FVector2D::Distance(FVector2D(Smoke->Frozen.Location), FVector2D(690.f, 1500.f))), Smoke->Frozen.PatrolIndex,
                Population->Spawned, Population->Despawned, Population->UpdateMsTotal / FMath::Max(1, Population->Updates),
                Population->UpdateMsMax, Population->SpawnMsMax);
            FPlatformMisc::RequestExit(false);
            return false;
        }
        }
    }));
}

// Fase 15: a mesma vista do vale em oito horas do dia (Saved/Screenshots/daynight_HHMM.png) e duas
// olhando para o sol entre as árvores (daynight_sol_HHMM.png: raios, reflexo de lente e brilho).
// Cada hora espera 3 s: a captura do céu e a luz global se ajustam à luz nova.
void AVortexGameMode::BeginDayNightShot(APlayerController* NewPlayer)
{
    struct FShot { int32 Index = 0; float Time = 0.f; bool bGround = false, bCaptured = false; TWeakObjectPtr<ACameraActor> Camera; };
    TSharedRef<FShot> Shot = MakeShared<FShot>();
    TWeakObjectPtr<AVortexGameMode> Self(this);
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Controller, Shot](float Delta) -> bool
    {
        // hora, dia (fase da lua) e vista: 0 = vale, 1 = olhando o sol, 2 = olhando a lua, 3 = céu (nuvens),
        // 4 = o lago de perto
        struct FView { float Hour; int32 Day; int32 Look; const TCHAR* Name; };
        static const FView Views[] = {
            { 6.2f, 1, 0, TEXT("0612") }, { 8.f, 1, 0, TEXT("0800") }, { 12.f, 1, 0, TEXT("1200") }, { 16.5f, 1, 0, TEXT("1630") },
            { 17.7f, 1, 0, TEXT("1742") }, { 18.6f, 1, 0, TEXT("1836") }, { 21.f, 1, 0, TEXT("2100") }, { 1.f, 1, 0, TEXT("0100") },
            { 7.f, 1, 1, TEXT("sol_0700") }, { 17.3f, 1, 1, TEXT("sol_1718") },
            { 22.f, 1, 2, TEXT("lua_cheia") }, { 22.f, 3, 2, TEXT("lua_minguante") }, { 22.f, 4, 2, TEXT("lua_fina") },
            { 12.f, 1, 3, TEXT("nuvens_1200") }, { 17.6f, 1, 3, TEXT("nuvens_1736") }, { 21.f, 1, 3, TEXT("nuvens_2100") },
            { 10.5f, 1, 4, TEXT("lago_1030") }, { 17.3f, 1, 4, TEXT("lago_1718") },
            { 11.f, 1, 5, TEXT("vida_1100") }, { 22.f, 1, 5, TEXT("vida_2200") } };
        // -VortexShotOnly=vida: só as vistas cujo nome contém o texto
        static const FString Only = []() { FString Value; FParse::Value(FCommandLine::Get(), TEXT("VortexShotOnly="), Value); return Value; }();
        auto Wanted = [](int32 Index) { return Only.IsEmpty() || FString(Views[Index].Name).Contains(Only); };
        AVortexGameMode* Mode = Self.Get();
        if (!Mode) return false;
        AVortexGameState* State = Mode->GetGameState<AVortexGameState>();
        AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        AVortexTerrainStreamer* Streamer = nullptr;
        for (TActorIterator<AVortexTerrainStreamer> It(Mode->GetWorld()); It; ++It) Streamer = *It;
        if (!State || !Hero || !Streamer || !Streamer->IsReady()) return true;
        const FVector2D At(-30000.f, -8000.f);
        if (!Shot->bGround)
        {
            float Ground = 0.f;
            Hero->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            Hero->SetActorLocation(FVector(At, Streamer->GroundAt(At.X, At.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
            if (!Streamer->GroundReadyAt(At.X, At.Y, Ground) || Streamer->PendingChunks() > 0) return true;
            Hero->SetActorLocation(FVector(At, Ground + AVortexCharacter::HeroCapsuleHalfHeight + 4.f), false, nullptr, ETeleportType::TeleportPhysics);
            Hero->GetCharacterMovement()->DisableMovement();
            Shot->Camera = Mode->GetWorld()->SpawnActor<ACameraActor>();
            const FVector Eye(At.X - 600.f, At.Y, Ground + 380.f);
            Shot->Camera->SetActorLocationAndRotation(Eye, FRotator(-2.f, 0.f, 0.f));
            Shot->Camera->GetCameraComponent()->SetFieldOfView(75.f);
            Shot->Camera->GetCameraComponent()->bConstrainAspectRatio = false;
            Controller->SetViewTarget(Shot->Camera.Get());
            State->SetDayTimeScale(0.f);
            while (Shot->Index + 1 < UE_ARRAY_COUNT(Views) && !Wanted(Shot->Index)) ++Shot->Index;
            State->SetDayClock(AVortexGameState::ClockOfHour(Views[Shot->Index].Hour), Views[Shot->Index].Day);
            Shot->bGround = true;
            return true;
        }
        Shot->Time += Delta;
        const UVortexDayNightSubsystem* SunState = Mode->GetWorld()->GetSubsystem<UVortexDayNightSubsystem>();
        if (Views[Shot->Index].Look == 3 && Shot->Camera.IsValid()) Shot->Camera->SetActorRotation(FRotator(18.f, 20.f, 0.f));
        else if (Views[Shot->Index].Look == 5 && Shot->Camera.IsValid())
        {
            // floresta viva: rente ao campo, olhando em volta (borboletas, sementes, vaga-lumes)
            const FVector Eye(At.X + 250.f, At.Y - 150.f, Streamer->GroundAt(At.X + 250.f, At.Y - 150.f) + 130.f);
            Shot->Camera->SetActorLocationAndRotation(Eye, FRotator(-4.f, 0.f, 0.f));
        }
        else if (Views[Shot->Index].Look == 4 && Shot->Camera.IsValid())
        {
            // baixo, na margem, olhando o lago à direita do vale
            const FVector Eye(At.X + 300.f, At.Y + 250.f, Streamer->GroundAt(At.X + 300.f, At.Y + 250.f) + 160.f);
            Shot->Camera->SetActorLocationAndRotation(Eye, FRotator(-14.f, 40.f, 0.f));
        }
        else if (Views[Shot->Index].Look > 0 && SunState && Shot->Camera.IsValid())
        {
            // de frente para o sol (ou a lua), um pouco abaixo: copas e troncos na frente do disco
            const FVector Target = Views[Shot->Index].Look == 1 ? SunState->GetSunDirection() : -SunState->GetSunDirection();
            const FRotator Toward = Target.Rotation();
            Shot->Camera->SetActorRotation(FRotator(Toward.Pitch - 6.f, Toward.Yaw, 0.f));
        }
        else if (Shot->Camera.IsValid()) Shot->Camera->SetActorRotation(FRotator(-2.f, 0.f, 0.f));
        if (Shot->Time < 3.f) return true;
        // a foto é tirada no fim deste quadro: a hora só muda no quadro seguinte
        if (Shot->bCaptured)
        {
            Shot->bCaptured = false;
            do { ++Shot->Index; } while (Shot->Index < UE_ARRAY_COUNT(Views) && !Wanted(Shot->Index));
            if (Shot->Index >= UE_ARRAY_COUNT(Views))
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_DAYNIGHT_SHOT_DONE"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
            State->SetDayClock(AVortexGameState::ClockOfHour(Views[Shot->Index].Hour), Views[Shot->Index].Day);
            Shot->Time = 0.f;
            return true;
        }
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/daynight_%s.png"),
            Views[Shot->Index].Name), false, false);
        const UVortexDayNightSubsystem* DayNight = Mode->GetWorld()->GetSubsystem<UVortexDayNightSubsystem>();
        UE_LOG(LogTemp, Display, TEXT("VORTEX_DAYNIGHT_SHOT %s hora=%.2f dia=%d claridade=%.2f dourado=%.2f sol_z=%.2f lua=%.2f giros=%d ativo=%d"),
            Views[Shot->Index].Name, State->GetHour(), State->GetDayNumber(), DayNight ? DayNight->GetDaylight() : -1.f,
            DayNight ? DayNight->GetGolden() : -1.f, DayNight ? DayNight->GetSunDirection().Z : 0.f,
            DayNight ? DayNight->GetMoonLit() : -1.f, DayNight ? DayNight->LightTurns : -1, DayNight && DayNight->IsActive() ? 1 : 0);
        for (TActorIterator<AVortexForestLife> Life(Mode->GetWorld()); Life; ++Life)
        {
            UE_LOG(LogTemp, Display, TEXT("VORTEX_FOREST_LIFE %s bichos=%d agua_perto=%.2f %s"), Views[Shot->Index].Name,
                Life->VisibleCritters(), Life->GetWaterNear(), *Life->DescribeSound());
        }
        Shot->bCaptured = true;
        return true;
    }));
}

// Grama: o mesmo prado (morro mais alto do protótipo) em quatro vistas fixas, relógio parado
// (Saved/Screenshots/grass_*.png): rente ao chão, câmera do jogo, aberta e fim de tarde.
void AVortexGameMode::BeginGrassShot(APlayerController* NewPlayer)
{
    struct FShot { int32 Index = 0; float Time = 0.f; bool bReady = false, bCaptured = false; FVector2D At; TWeakObjectPtr<ACameraActor> Camera; };
    TSharedRef<FShot> Shot = MakeShared<FShot>();
    TWeakObjectPtr<AVortexGameMode> Self(this);
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Controller, Shot](float Delta) -> bool
    {
        // altura acima do chão, recuo, inclinação, hora
        struct FView { float Height, Back, Pitch, Hour; const TCHAR* Name; };
        static const FView Views[] = { { 110.f, 0.f, -10.f, 12.f, TEXT("rente") }, { 330.f, 650.f, -16.f, 12.f, TEXT("jogo") },
            { 1100.f, 1500.f, -22.f, 12.f, TEXT("aberta") }, { 330.f, 650.f, -16.f, 17.4f, TEXT("tarde") } };
        AVortexGameMode* Mode = Self.Get();
        if (!Mode) return false;
        AVortexGameState* State = Mode->GetGameState<AVortexGameState>();
        AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        AVortexTerrainStreamer* Streamer = nullptr;
        for (TActorIterator<AVortexTerrainStreamer> It(Mode->GetWorld()); It; ++It) Streamer = *It;
        if (!State || !Hero || !Streamer || !Streamer->IsReady()) return true;
        if (!Shot->bReady)
        {
            Shot->At = FVector2D(-73400.f, -46400.f);
            float Ground = 0.f;
            Hero->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            Hero->SetActorLocation(FVector(Shot->At + FVector2D(0.f, -2500.f), Streamer->GroundAt(Shot->At.X, Shot->At.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
            if (!Streamer->GroundReadyAt(Shot->At.X, Shot->At.Y, Ground) || Streamer->PendingChunks() > 0) return true;
            Hero->SetHidden(true);
            Hero->GetCharacterMovement()->DisableMovement();
            Shot->Camera = Mode->GetWorld()->SpawnActor<ACameraActor>();
            Shot->Camera->GetCameraComponent()->SetFieldOfView(75.f);
            Shot->Camera->GetCameraComponent()->bConstrainAspectRatio = false;
            Controller->SetViewTarget(Shot->Camera.Get());
            State->SetDayTimeScale(0.f);
            State->SetDayHour(Views[0].Hour);
            Shot->bReady = true;
        }
        const FView& View = Views[Shot->Index];
        const float Yaw = 51.f;
        const FVector Forward = FRotator(0.f, Yaw, 0.f).Vector();
        const FVector2D Eye2D = Shot->At - FVector2D(Forward) * View.Back;
        Shot->Camera->SetActorLocationAndRotation(FVector(Eye2D, Streamer->GroundAt(Eye2D.X, Eye2D.Y) + View.Height), FRotator(View.Pitch, Yaw, 0.f));
        Shot->Time += Delta;
        if (Shot->Time < 3.f) return true;
        if (Shot->bCaptured)
        {
            Shot->bCaptured = false;
            if (++Shot->Index >= UE_ARRAY_COUNT(Views))
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_GRASS_SHOT_DONE"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
            State->SetDayHour(Views[Shot->Index].Hour);
            Shot->Time = 0.f;
            return true;
        }
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/grass_%s.png"), View.Name), false, false);
        Shot->bCaptured = true;
        return true;
    }));
}

// Fase 15, servidor: conversões de hora, virada do dia com o relógio acelerado, hora no save do
// mundo. Servidor e clientes registram o relógio calculado para o mesmo instante de servidor
// (T = 1000 s): os parâmetros replicados precisam dar a mesma hora em todos.
void AVortexGameMode::BeginDayNightSmoke()
{
    struct FSmoke { int32 Step = 0; float Time = 0.f; int32 DayBefore = 0; bool bConvert = true, bRollover = false, bSave = false; };
    TSharedRef<FSmoke> Smoke = MakeShared<FSmoke>();
    TWeakObjectPtr<AVortexGameMode> Self(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Smoke](float Delta) -> bool
    {
        AVortexGameMode* Mode = Self.Get();
        if (!Mode) return false;
        AVortexGameState* State = Mode->GetGameState<AVortexGameState>();
        if (!State) return true;
        Smoke->Time += Delta;
        if (Smoke->Step == 0)
        {
            for (const float Hour : { 6.f, 9.5f, 12.f, 17.75f, 18.f, 20.f, 23.5f, 0.f, 3.f, 5.9f })
            {
                Smoke->bConvert &= FMath::IsNearlyEqual(AVortexGameState::HourOfClock(AVortexGameState::ClockOfHour(Hour)), Hour, 0.01f);
            }
            // 5h48, 600x mais rápido: amanhece em ~0,01 s e o dia 2 segue de dia por 2 s
            State->SetDayHour(5.8f);
            Smoke->DayBefore = State->GetDayNumber();
            State->SetDayTimeScale(600.f);
            Smoke->Step = 1;
            Smoke->Time = 0.f;
            return true;
        }
        if (Smoke->Step == 1)
        {
            if (Smoke->Time < 2.f) return true;
            Smoke->bRollover = State->GetDayNumber() == Smoke->DayBefore + 1 && State->GetHour() > 6.f && State->GetHour() < 18.f;
            State->SetDayTimeScale(1.f);
            State->SetDayHour(19.25f);
            UVortexSaveSubsystem* Saves = Mode->GetGameInstance() ? Mode->GetGameInstance()->GetSubsystem<UVortexSaveSubsystem>() : nullptr;
            if (UVortexWorldSave* World = Mode->ActiveWorldSave)
            {
                World->DayClock = State->GetDayClock();
                World->DayNumber = State->GetDayNumber();
                const UVortexWorldSave* Loaded = Saves && Saves->SaveHostedWorld(World) ? Saves->LoadHostedWorld(World->WorldId) : nullptr;
                Smoke->bSave = Loaded && FMath::IsNearlyEqual(Loaded->DayClock, World->DayClock, 0.01f) && Loaded->DayNumber == World->DayNumber &&
                    FMath::IsNearlyEqual(AVortexGameState::HourOfClock(Loaded->DayClock), 19.25f, 0.02f);
                const FString Slot = TEXT("Vortex_World_") + World->WorldId.ToString(EGuidFormats::Digits);
                for (const TCHAR* Suffix : { TEXT(""), TEXT("_backup"), TEXT("_staging") }) UGameplayStatics::DeleteGameInSlot(Slot + Suffix, 0);
            }
            Smoke->Step = 2;
            Smoke->Time = 0.f;
            return true;
        }
        // espera os clientes receberem a hora nova e registrarem
        if (Smoke->Time < 6.f) return true;
        const bool bPassed = Smoke->bConvert && Smoke->bRollover && Smoke->bSave;
        UE_LOG(LogTemp, Display, TEXT("VORTEX_DAYNIGHT_SMOKE_%s conversao=%d virada=%d save=%d dia=%d hora=%.2f relogio_T1000=%.3f"),
            bPassed ? TEXT("PASS") : TEXT("FAIL"), Smoke->bConvert ? 1 : 0, Smoke->bRollover ? 1 : 0, Smoke->bSave ? 1 : 0,
            State->GetDayNumber(), State->GetHour(), State->ClockAt(1000.0));
        FPlatformMisc::RequestExit(false);
        return false;
    }));
}

// Fase 12: vistas longas do vale, cada uma com e sem névoa no mesmo ponto (showflag.fog), para
// avaliar se o fim do terreno montado (~110 m) e as árvores soltas além dele somem na névoa.
// Pontos: o meio do vale olhando o comprimento, o início do passeio olhando a borda oeste e o
// ponto mais alto do protótipo olhando o centro. Câmera solta, 6 m atrás do herói.
void AVortexGameMode::BeginFogShot(APlayerController* NewPlayer)
{
    struct FView { FString Name; FVector2D At; float Yaw = 0.f; float Height = 0.f; float Pitch = 0.f; };
    struct FFogShot
    {
        TArray<FView> Views;
        int32 Index = 0;
        int32 Step = 0;
        float Time = 0.f;
        bool bLogged = false;
        TWeakObjectPtr<ACameraActor> Camera;
    };
    TWeakObjectPtr<AVortexGameMode> Self(this);
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    TSharedRef<FFogShot> Shot = MakeShared<FFogShot>();
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Controller, Shot](float Delta) -> bool
    {
        AVortexGameMode* Mode = Self.Get();
        if (!Mode) return false;
        AVortexCharacter* Character = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        AVortexTerrainStreamer* Streamer = nullptr;
        for (TActorIterator<AVortexTerrainStreamer> It(Mode->GetWorld()); It; ++It) Streamer = *It;
        if (!Character || !Streamer || !Streamer->IsReady()) return true;
        const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
        if (!Shot->bLogged)
        {
            Shot->bLogged = true;
            for (TActorIterator<AExponentialHeightFog> It(Mode->GetWorld()); It; ++It)
            {
                // ajuste de teste sem regravar o mapa: -VortexFogDensity= -VortexFogStart= -VortexFogOpacity= -VortexFogFalloff=
                UExponentialHeightFogComponent* Fog = It->GetComponent();
                float Value = 0.f;
                if (FParse::Value(FCommandLine::Get(), TEXT("VortexFogDensity="), Value)) Fog->SetFogDensity(Value);
                if (FParse::Value(FCommandLine::Get(), TEXT("VortexFogStart="), Value)) Fog->SetStartDistance(Value);
                if (FParse::Value(FCommandLine::Get(), TEXT("VortexFogOpacity="), Value)) Fog->SetFogMaxOpacity(Value);
                if (FParse::Value(FCommandLine::Get(), TEXT("VortexFogFalloff="), Value)) Fog->SetFogHeightFalloff(Value);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_FOG_SHOT fog z=%.0f density=%.4f falloff=%.4f start=%.0f opacity=%.2f"),
                    It->GetActorLocation().Z, Fog->FogDensity, Fog->FogHeightFalloff, Fog->StartDistance, Fog->FogMaxOpacity);
            }
            const FBox2D Bounds = Data->PrototypeBounds();
            const FVector2D Center = Bounds.GetCenter();
            FVector2D High = Center;
            float HighZ = -1.e9f;
            for (float X = Bounds.Min.X + 6000.f; X < Bounds.Max.X - 6000.f; X += 1500.f)
            for (float Y = Bounds.Min.Y + 6000.f; Y < Bounds.Max.Y - 6000.f; Y += 1500.f)
            {
                const float Z = Data->GroundAt(X, Y);
                if (Z > HighZ && Z > Data->WaterLevel) { HighZ = Z; High = FVector2D(X, Y); }
            }
            const FVector2D ToCenter = Center - High;
            // altura da câmera acima do chão e inclinação: ao nível do herói, numa árvore e no morro
            Shot->Views = {
                { TEXT("vale"), FVector2D(-30000.f, -8000.f), 0.f, 350.f, -3.f },
                { TEXT("borda"), FVector2D(-42000.f, -6000.f), 180.f, 1500.f, -8.f },
                { TEXT("alto"), High, ToCenter.IsNearlyZero() ? 0.f : float(FMath::RadiansToDegrees(FMath::Atan2(ToCenter.Y, ToCenter.X))), 2200.f, -7.f },
            };
            UE_LOG(LogTemp, Display, TEXT("VORTEX_FOG_SHOT high=(%.0f,%.0f) z=%.0f trees=%.0f stream=%d"),
                High.X, High.Y, HighZ, Streamer->TreeRange, Streamer->StreamRadius);
        }
        if (Shot->Index >= Shot->Views.Num())
        {
            UE_LOG(LogTemp, Display, TEXT("VORTEX_FOG_SHOT_DONE views=%d"), Shot->Views.Num());
            FPlatformMisc::RequestExit(false);
            return false;
        }
        const FView& View = Shot->Views[Shot->Index];
        // câmera solta acima do herói: o braço de 9 m batia nas árvores e mostrava só a mata
        if (!Shot->Camera.IsValid())
        {
            Shot->Camera = Mode->GetWorld()->SpawnActor<ACameraActor>();
            if (Shot->Camera.IsValid())
            {
                Shot->Camera->GetCameraComponent()->SetFieldOfView(75.f);
                Shot->Camera->GetCameraComponent()->bConstrainAspectRatio = false;
                Controller->SetViewTarget(Shot->Camera.Get());
            }
        }
        if (Shot->Camera.IsValid())
        {
            const FVector Forward = FRotator(0.f, View.Yaw, 0.f).Vector();
            const FVector Eye = FVector(View.At.X, View.At.Y, Streamer->GroundAt(View.At.X, View.At.Y)) - Forward * 600.f +
                FVector(0.f, 0.f, View.Height);
            Shot->Camera->SetActorLocationAndRotation(Eye, FRotator(View.Pitch, View.Yaw, 0.f));
        }
        const auto Capture = [&View](const TCHAR* Suffix)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/") /
                (TEXT("fog_") + View.Name + Suffix), false, false);
        };
        if (Shot->Step == 0)
        {
            // perto do ponto, voando até o chão dali estar pronto
            Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            Character->SetActorLocation(FVector(View.At.X, View.At.Y, Streamer->GroundAt(View.At.X, View.At.Y) + 200.f),
                false, nullptr, ETeleportType::TeleportPhysics);
            Shot->Step = 1;
            Shot->Time = 0.f;
            return true;
        }
        if (Shot->Step == 1)
        {
            float Ground = 0.f;
            if (!Streamer->GroundReadyAt(View.At.X, View.At.Y, Ground) || Streamer->PendingChunks() > 0) return true;
            Character->SetActorLocation(FVector(View.At.X, View.At.Y, Ground + AVortexCharacter::HeroCapsuleHalfHeight + 4.f),
                false, nullptr, ETeleportType::TeleportPhysics);
            Character->GetCharacterMovement()->DisableMovement();
            Shot->Step = 2;
            Shot->Time = 0.f;
            return true;
        }
        Shot->Time += Delta;
        // árvores entram duas regiões por quadro: 3 s de folga antes da primeira foto
        if (Shot->Step == 2 && Shot->Time >= 3.f) { Capture(TEXT("_com.png")); Shot->Step = 3; }
        else if (Shot->Step == 3 && Shot->Time >= 3.5f) { Controller->ConsoleCommand(TEXT("showflag.fog 0")); Shot->Step = 4; }
        else if (Shot->Step == 4 && Shot->Time >= 4.2f) { Capture(TEXT("_sem.png")); Shot->Step = 5; }
        else if (Shot->Step == 5 && Shot->Time >= 4.7f)
        {
            Controller->ConsoleCommand(TEXT("showflag.fog 1"));
            UE_LOG(LogTemp, Display, TEXT("VORTEX_FOG_SHOT view=%s at=(%.0f,%.0f) yaw=%.0f"), *View.Name, View.At.X, View.At.Y, View.Yaw);
            ++Shot->Index;
            Shot->Step = 0;
        }
        return true;
    }));
}
#endif

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

#if !UE_BUILD_SHIPPING
// HUD: fotos COM a interface (Saved/Screenshots/hud_*.png): de dia parado no vale; em combate (um
// lobo na frente, o herói ferido e um golpe dado: números de dano e a barra do alvo); à noite.
// Estádio do Despertar (-VortexStadiumShot -VortexAwakening): o herói novo acorda no coliseu, vai
// ao pedestal do arco, pega a arma (E), atira num boneco, sai pelo portão dourado e pousa no vale.
// Fotos estadio_*.png e VORTEX_STADIUM_SHOT PASS/FAIL.
void AVortexGameMode::BeginStadiumShot(APlayerController* NewPlayer)
{
    struct FShot { int32 Step = 0; float Time = 0.f; bool bPass = true; FVector Pedestal = FVector::ZeroVector; };
    TSharedRef<FShot> Shot = MakeShared<FShot>();
    TWeakObjectPtr<AVortexGameMode> Self(this);
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Controller, Shot](float Delta) -> bool
    {
        AVortexGameMode* Mode = Self.Get();
        if (!Mode) return false;
        AVortexGameState* State = Mode->GetGameState<AVortexGameState>();
        AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
        if (!State || !Hero || !Sheet || !Sheet->IsLoaded()) return true;
        AVortexAwakeningStadium* Stadium = AVortexAwakeningStadium::Find(Mode->GetWorld());
        Shot->Time += Delta;
        auto Capture = [&](const TCHAR* Name)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/estadio_%s.png"), Name), true, false);
            UE_LOG(LogTemp, Display, TEXT("VORTEX_STADIUM_SHOT %s arma=%s posicao=%s"), Name, *Sheet->GetWeaponType().ToString(), *Hero->GetActorLocation().ToCompactString());
        };
        auto Check = [&](bool bOk, const TCHAR* What)
        {
            UE_LOG(LogTemp, Display, TEXT("VORTEX_STADIUM_SHOT %s %s"), What, bOk ? TEXT("ok") : TEXT("FALHOU"));
            Shot->bPass &= bOk;
        };
        switch (Shot->Step)
        {
        case 0:
            State->SetDayTimeScale(0.f);
            State->SetDayClock(AVortexGameState::ClockOfHour(15.f), 1);
            if (Shot->Time < 4.f) return true;
            Check(Stadium && AVortexAwakeningStadium::Contains(Hero->GetActorLocation()) && Sheet->NeedsAwakening() && Sheet->GetWeaponType().IsNone(),
                TEXT("acordou no estadio sem arma"));
            Controller->SetControlRotation(FRotator(-6.f, 90.f, 0.f));
            Shot->Step = 1; Shot->Time = 0.f;
            return true;
        case 1:
            if (Shot->Time < 1.f) return true;
            if (Shot->Time - Delta < 1.f) { Capture(TEXT("arena")); return true; }
            // de cima, para ver o coliseu inteiro
            Controller->SetControlRotation(FRotator(-12.f, 0.f, 0.f));
            Shot->Step = 2; Shot->Time = 0.f;
            return true;
        case 2:
        {
            if (Shot->Time < 1.f) return true;
            if (Shot->Time - Delta < 1.f) { Capture(TEXT("arquibancada")); return true; }
            // até o pedestal do arco (na ala de distância)
            if (!Stadium) return true;
            // (os pedestais ficam num anel dentro da arena; o do arco, no grupo da ala de distância, a leste)
            const AVortexAwakeningStadium::FPedestal* Bow = Stadium->FindPedestal(TEXT("arco_de_betula"));
            if (!Bow) { Check(false, TEXT("achou o pedestal do arco")); Shot->Step = 99; return true; }
            Shot->Pedestal = Bow->Where;
            const FVector Toward = (AVortexAwakeningStadium::Origin() - Bow->Where).GetSafeNormal2D();   // de frente para o pedestal, do lado da arena
            Hero->SetActorLocation(Bow->Where + Toward * 150.f + FVector(0.f, 0.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
            Controller->SetControlRotation(FRotator(-10.f, (-Toward).Rotation().Yaw + 25.f, 0.f));
            Shot->Step = 3; Shot->Time = 0.f;
            return true;
        }
        case 3:
            if (Shot->Time < 1.2f) return true;
            if (Shot->Time - Delta < 1.2f) { Capture(TEXT("pedestal")); return true; }
            Hero->Interact();
            Shot->Step = 4; Shot->Time = 0.f;
            return true;
        case 4:
            if (Shot->Time < .8f) return true;
            Check(Sheet->GetWeaponType() == TEXT("arco_de_betula"), TEXT("pegou o arco"));
            Capture(TEXT("arco_na_mao"));
            // de volta ao centro, mirando um boneco de treino
            Hero->SetActorLocation(AVortexAwakeningStadium::Origin() + FVector(0.f, -300.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
            Shot->Step = 5; Shot->Time = 0.f;
            return true;
        case 5:
        {
            AVortexTrainingDummy* Dummy = nullptr;
            if (TActorIterator<AVortexTrainingDummy> It(Mode->GetWorld()); It) { Dummy = *It; }
            if (!Dummy) { Check(false, TEXT("bonecos de treino")); Shot->Step = 99; return true; }
            const FVector Toward = (Dummy->GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D();
            Hero->SetActorRotation(Toward.Rotation());
            Controller->SetControlRotation(FRotator(-6.f, Toward.Rotation().Yaw, 0.f));
            if (Shot->Time > .5f && FMath::Fmod(Shot->Time, .3f) < Delta) Hero->GetWeapon()->PressPrimary();
            if (Shot->Time > 1.1f && Shot->Time - Delta <= 1.1f) Capture(TEXT("treino"));
            if (Shot->Time < 3.f) return true;
            Shot->Step = 6; Shot->Time = 0.f;
            return true;
        }
        case 6:
        {
            if (!Stadium) return true;
            const FVector Exit = Stadium->ExitPoint();
            const FVector Toward = (Exit - AVortexAwakeningStadium::Origin()).GetSafeNormal2D();
            Hero->SetActorLocation(Exit - Toward * 120.f + FVector(0.f, 0.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
            Controller->SetControlRotation(FRotator(-6.f, Toward.Rotation().Yaw, 0.f));
            Shot->Step = 7; Shot->Time = 0.f;
            return true;
        }
        case 7:
            if (Shot->Time < 1.f) return true;
            if (Shot->Time - Delta < 1.f) { Capture(TEXT("saida")); return true; }
            Hero->Interact();
            Shot->Step = 8; Shot->Time = 0.f;
            return true;
        case 8:
            if (Shot->Time < 6.f && Hero->GetCharacterMovement()->MovementMode != MOVE_Walking) return true;
            if (Shot->Time < 2.5f) return true;
            Check(!Sheet->NeedsAwakening() && !AVortexAwakeningStadium::Contains(Hero->GetActorLocation()) &&
                Hero->GetCharacterMovement()->MovementMode == MOVE_Walking, TEXT("saiu e pousou no vale"));
            Capture(TEXT("vale"));
            Shot->Step = 99; Shot->Time = 0.f;
            return true;
        default:
            if (Shot->Time < .6f) return true;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_STADIUM_SHOT_DONE %s"), Shot->bPass ? TEXT("PASS") : TEXT("FAIL"));
            FPlatformMisc::RequestExit(false);
            return false;
        }
    }));
}

// Cidade do Santuário (-VortexTownShot): fotos cidade_*.png do portão sul, da praça, da taverna, de
// cima e da praça à noite (lampiões). VORTEX_TOWN_SHOT_DONE.
void AVortexGameMode::BeginTownShot(APlayerController* NewPlayer)
{
    struct FView { FString Name; FVector Eye; FVector Target; float Hour; };
    static const FView Defaults[] = {
        { TEXT("portao"), FVector(0.f, -3900.f, 420.f), FVector(0.f, 1500.f, 300.f), 15.f },
        { TEXT("praca"), FVector(1000.f, 200.f, 520.f), FVector(-300.f, 1700.f, 100.f), 15.f },
        { TEXT("taverna"), FVector(-1300.f, 3000.f, 380.f), FVector(-2900.f, 3700.f, 350.f), 15.f },
        { TEXT("oficinas"), FVector(-600.f, 1900.f, 450.f), FVector(1650.f, 1500.f, 250.f), 15.f },
        { TEXT("aerea"), FVector(-5200.f, -5600.f, 4200.f), FVector(0.f, 1600.f, 0.f), 15.f },
        { TEXT("noite"), FVector(1000.f, 200.f, 520.f), FVector(-300.f, 1700.f, 100.f), 21.5f },
        { TEXT("fonte"), FVector(380.f, 1080.f, 260.f), FVector(0.f, 1500.f, 150.f), 16.f },
        { TEXT("fonte_noite"), FVector(-420.f, 1050.f, 230.f), FVector(0.f, 1500.f, 150.f), 21.f },
        // 03/11, a vila maior (Tools/BuildTownExpansion.py)
        { TEXT("mercado"), FVector(1800.f, 5200.f, 600.f), FVector(-500.f, 7000.f, 200.f), 15.f },
        { TEXT("capela"), FVector(-1300.f, 5300.f, 450.f), FVector(-2700.f, 6900.f, 600.f), 15.f },
        { TEXT("quartel"), FVector(3700.f, 600.f, 520.f), FVector(5300.f, -1900.f, 300.f), 15.f },
        { TEXT("moinho"), FVector(1200.f, -3000.f, 650.f), FVector(3000.f, -4700.f, 600.f), 15.f },
        { TEXT("barris"), FVector(500.f, 6150.f, 260.f), FVector(-750.f, 6650.f, 60.f), 15.f },
        { TEXT("aerea_nova"), FVector(-9500.f, -9500.f, 8000.f), FVector(0.f, 2300.f, 0.f), 15.f },
        { TEXT("mercado_noite"), FVector(1800.f, 5200.f, 600.f), FVector(-500.f, 7000.f, 200.f), 21.5f },
        { TEXT("barris_quebrados"), FVector(500.f, 6150.f, 260.f), FVector(-750.f, 6650.f, 60.f), 15.f },
    };
    // 06/11: as câmeras podem vir de Data/Tools/town_shots.json ([{name, eye:[x,y,z], target:[x,y,z], hour}], cm da Unreal;
    // a altura do olho soma ao chão): a vila nova muda de lugar sem recompilar
    static TArray<FView> Views;
    Views.Reset();
    {
        FString Text;
        TArray<TSharedPtr<FJsonValue>> List;
        if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Tools/town_shots.json"))) &&
            FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), List))
            for (const TSharedPtr<FJsonValue>& Value : List)
            {
                const TSharedPtr<FJsonObject> V = Value->AsObject();
                const TArray<TSharedPtr<FJsonValue>>& E = V->GetArrayField(TEXT("eye"));
                const TArray<TSharedPtr<FJsonValue>>& T = V->GetArrayField(TEXT("target"));
                Views.Add({ V->GetStringField(TEXT("name")), FVector(E[0]->AsNumber(), E[1]->AsNumber(), E[2]->AsNumber()),
                    FVector(T[0]->AsNumber(), T[1]->AsNumber(), T[2]->AsNumber()), (float)V->GetNumberField(TEXT("hour")) });
            }
        if (Views.IsEmpty()) for (const FView& View : Defaults) Views.Add(View);
    }
    struct FShot { int32 Step = -1; float Time = 0.f; int32 Julia = 0; TWeakObjectPtr<ACameraActor> Camera; };
    TSharedRef<FShot> Shot = MakeShared<FShot>();
    TWeakObjectPtr<AVortexGameMode> Self(this);
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Controller, Shot](float Delta) -> bool
    {
        AVortexGameMode* Mode = Self.Get();
        if (!Mode || !Controller.IsValid()) return false;
        AVortexGameState* State = Mode->GetGameState<AVortexGameState>();
        const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
        if (!State || !Terrain) return true;
        Shot->Time += Delta;
        if (Shot->Step < 0)
        {
            if (Shot->Time < 6.f) return true;
            FActorSpawnParameters Params;
            Shot->Camera = Mode->GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
            if (Shot->Camera.IsValid()) Shot->Camera->GetCameraComponent()->SetFieldOfView(70.f);
            Shot->Step = 0; Shot->Time = 0.f;
        }
        if (Shot->Step >= Views.Num())
        {
            // santuário: o herói cai longe da fonte e tem de renascer nela, com a vida cheia
            AVortexCharacter* Hero = Cast<AVortexCharacter>(Controller->GetPawn());
            UAbilitySystemComponent* Abilities = Hero ? Hero->GetAbilitySystemComponent() : nullptr;
            const AVortexFountain* Fountain = AVortexFountain::Find(Mode->GetWorld());
            if (!Hero || !Abilities || !Fountain) { UE_LOG(LogTemp, Display, TEXT("VORTEX_TOWN_SHOT_DONE FAIL sem heroi ou fonte")); FPlatformMisc::RequestExit(false); return false; }
            // a Julia e o cachorrinho, de frente (ela passeia em volta da fonte)
            if (Shot->Julia < 3)
            {
                State->SetDayClock(AVortexGameState::ClockOfHour(15.f), 1);
                AActor* Julia = nullptr;
                for (TActorIterator<AVortexNpc> It(Mode->GetWorld()); It; ++It) if (It->GetNpcId() == TEXT("julia")) Julia = *It;
                if (!Julia) { if (Shot->Time > 45.f) Shot->Julia = 3; return true; }
                // num ponto aberto da praça (ao sul da fonte), de frente para o sul, para a foto
                {
                    const FVector Spot(-150.f, 520.f, Terrain->GroundAt(-150.f, 520.f) + 72.f);
                    if (Shot->Time < 1.2f || Shot->Julia > 0) Julia->SetActorLocationAndRotation(Spot, FRotator(0.f, -90.f, 0.f));
                }
                // a terceira foto é o cachorrinho, de perto e na altura dele
                if (Shot->Julia == 2)
                {
                    AActor* Dog = nullptr;
                    for (TActorIterator<AVortexPet> It(Mode->GetWorld()); It; ++It) Dog = *It;
                    // na frente dela, ao sol (atrás ele fica na sombra dela)
                    if (Dog)
                    {
                        const FVector At = Julia->GetActorLocation() + Julia->GetActorForwardVector() * 130.f;
                        Dog->SetActorLocationAndRotation(FVector(At.X, At.Y, Terrain->GroundAt(At.X, At.Y)), FRotator(0.f, Julia->GetActorRotation().Yaw + 60.f, 0.f));
                        Julia = Dog;
                    }
                }
                const float Back = Shot->Julia == 0 ? 330.f : Shot->Julia == 1 ? 170.f : 150.f;
                // o cachorrinho fica atrás dela: a foto dele é de lado
                const FVector Front = Shot->Julia == 2 ? Julia->GetActorRightVector() : Julia->GetActorForwardVector();
                const FVector Eye = Julia->GetActorLocation() + Front * Back + Julia->GetActorRightVector() * (Shot->Julia == 0 ? 60.f : 25.f)
                    + FVector(0.f, 0.f, Shot->Julia == 0 ? 40.f : Shot->Julia == 1 ? 20.f : 30.f);
                if (Shot->Camera.IsValid())
                {
                    Shot->Camera->SetActorLocationAndRotation(Eye, (Julia->GetActorLocation() + FVector(0.f, 0.f, Shot->Julia == 0 ? -20.f : Shot->Julia == 1 ? 10.f : 22.f) - Eye).Rotation());
                    Controller->SetViewTarget(Shot->Camera.Get());
                }
                if (Shot->Time < 2.5f) return true;
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/cidade_julia_%d.png"), Shot->Julia), false, false);
                Shot->Julia++; Shot->Time = 0.f;
                return true;
            }
            if (Shot->Step == Views.Num())
            {
                if (Shot->Time < .8f) return true;
                Controller->SetViewTarget(Hero);
                Hero->SetActorLocation(FVector(2500.f, -1500.f, Terrain->GroundAt(2500.f, -1500.f) + 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), 0.f);
                Shot->Step++; Shot->Time = 0.f;
                return true;
            }
            if (Shot->Time < 7.f) return true;
            const float Health = Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
            const float Distance = FVector::Dist2D(Hero->GetActorLocation(), Fountain->GetActorLocation());
            const bool bPass = Health > 1.f && Distance < 600.f;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_TOWN_SHOT_DONE %s santuario vida=%.0f distancia_da_fonte=%.0f"), bPass ? TEXT("PASS") : TEXT("FAIL"), Health, Distance);
            FPlatformMisc::RequestExit(false);
            return false;
        }
        const FView& View = Views[Shot->Step];
        if (Shot->Time - Delta <= 0.f)
        {
            State->SetDayTimeScale(0.f);
            State->SetDayClock(AVortexGameState::ClockOfHour(View.Hour), 1);
            const float Ground = Terrain->GroundAt(View.Eye.X, View.Eye.Y);
            const FVector Eye(View.Eye.X, View.Eye.Y, View.Eye.Z + FMath::Max(Ground, 0.f));
            const FVector Target(View.Target.X, View.Target.Y, View.Target.Z + Terrain->GroundAt(View.Target.X, View.Target.Y));
            // o herói vai junto (o terreno e a vida em volta são montados perto dele)
            if (APawn* Hero = Controller->GetPawn()) Hero->SetActorLocation(FVector(View.Eye.X, View.Eye.Y, Ground + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
            if (Shot->Camera.IsValid())
            {
                Shot->Camera->SetActorLocationAndRotation(Eye, (Target - Eye).Rotation());
                Controller->SetViewTarget(Shot->Camera.Get());
            }
        }
        // 03/11: os barris do mercado quebram (as tábuas, o líquido e a poça) e a foto sai 1 s depois
        if (View.Name == TEXT("barris_quebrados") && Shot->Time >= 1.6f && Shot->Time - Delta < 1.6f)
        {
            const FVector Market(-750.f, 6650.f, Terrain->GroundAt(-750.f, 6650.f) + 50.f);
            int32 All = 0;
            float Nearest = 1e9f;
            FVector NearestAt = FVector::ZeroVector;
            for (TActorIterator<AVortexBreakable> It(Mode->GetWorld()); It; ++It)
            {
                ++All;

                if (FVector::Dist(It->GetActorLocation(), Market) < Nearest) { Nearest = FVector::Dist(It->GetActorLocation(), Market); NearestAt = It->GetActorLocation(); }
            }
            UE_LOG(LogTemp, Display, TEXT("VORTEX_TOWN_SHOT barris=%d mais_perto=%.0f cm em %s quebrados=%d"), All, Nearest, *NearestAt.ToCompactString(),
                AVortexBreakable::BreakNear(Mode->GetWorld(), Market, 450.f));
        }
        if (Shot->Time < 3.5f) return true;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/cidade_%s.png"), *View.Name), false, false);
        ++Shot->Step;
        Shot->Time = -.6f;
        return true;
    }));
}

// Pedestais do Estádio (-VortexStadiumPick -VortexAwakening): passa por todos, de ala em ala, aperta E
// em cada um e confere a arma na mão; a coleta de lixo é forçada no caminho (antes ela levava as
// definições das armas dos pedestais). VORTEX_STADIUM_PICK PASS/FAIL.
void AVortexGameMode::BeginStadiumPick(APlayerController* NewPlayer)
{
    struct FPick { int32 Index = -1; int32 Phase = 0; float Time = 0.f; int32 Ok = 0, Bad = 0, Skipped = 0; };
    TSharedRef<FPick> Pick = MakeShared<FPick>();
    TWeakObjectPtr<AVortexGameMode> Self(this);
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Controller, Pick](float Delta) -> bool
    {
        AVortexGameMode* Mode = Self.Get();
        if (!Mode) return false;
        AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
        AVortexAwakeningStadium* Stadium = AVortexAwakeningStadium::Find(Mode->GetWorld());
        if (!Hero || !Sheet || !Sheet->IsLoaded() || !Stadium) return true;
        Pick->Time += Delta;
        if (Pick->Index < 0) { if (Pick->Time < 4.f) return true; Pick->Index = 0; Pick->Time = 0.f; }
        const TArray<AVortexAwakeningStadium::FPedestal>& All = Stadium->GetPedestals();
        if (Pick->Index >= All.Num())
        {
            const bool bPass = Pick->Bad == 0 && Pick->Ok > 0;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_STADIUM_PICK %s pegas=%d falhas=%d recusadas_de_proposito=%d"), bPass ? TEXT("PASS") : TEXT("FAIL"), Pick->Ok, Pick->Bad, Pick->Skipped);
            FPlatformMisc::RequestExit(false);
            return false;
        }
        const AVortexAwakeningStadium::FPedestal& P = All[Pick->Index];
        if (Pick->Phase == 0)
        {
            // de pé na frente do pedestal (do lado do meio da sala), como o jogador fica
            const FVector Toward = (AVortexAwakeningStadium::Origin() - P.Where).GetSafeNormal2D();
            Hero->SetActorLocation(P.Where + Toward * 140.f + FVector(0.f, 0.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
            if (Pick->Index % 5 == 2) CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
            Pick->Phase = 1; Pick->Time = 0.f;
            return true;
        }
        if (Pick->Phase == 1)
        {
            if (Pick->Time < .5f) return true;
            Hero->Interact();
            Pick->Phase = 2; Pick->Time = 0.f;
            return true;
        }
        if (Pick->Time < .5f) return true;
        const UVortexWeaponDefinition* D = P.Definition.Get();
        const bool bRefusedOnPurpose = D && (!D->bPlayable || (D->bMagic && Sheet->GetRace() == TEXT("anao")));
        const bool bHas = Sheet->GetWeaponType() == P.Type;
        if (bHas) ++Pick->Ok; else if (bRefusedOnPurpose) ++Pick->Skipped; else ++Pick->Bad;
        UE_LOG(LogTemp, Display, TEXT("VORTEX_STADIUM_PICK %02d %s %s"), Pick->Index, *P.Type.ToString(),
            bHas ? TEXT("ok") : bRefusedOnPurpose ? TEXT("recusada (regra)") : D ? TEXT("FALHOU") : TEXT("FALHOU (definicao perdida)"));
        ++Pick->Index;
        Pick->Phase = 0; Pick->Time = 0.f;
        return true;
    }));
}

// Armas do Godot (-VortexWeaponShot[=tipo1,tipo2]): para cada tipo, equipa, mostra a pose, ataca um
// lobo de treino com o combo e solta o especial (carregado quando for charge). Fotos arma_<tipo>_*.png
// e VORTEX_WEAPON_SHOT <tipo> acertos=N (vida do lobo antes e depois).
void AVortexGameMode::BeginWeaponShot(APlayerController* NewPlayer)
{
    struct FShot
    {
        int32 Step = 0, Index = 0;
        float Time = 0.f, WolfBefore = 0.f;
        TArray<FName> Types;
        TWeakObjectPtr<AVortexWolf> Wolf;
        bool bPass = true;
    };
    TSharedRef<FShot> Shot = MakeShared<FShot>();
    FString List = TEXT("arco_de_betula,lanca_do_patrulheiro,martelo_de_bram,cajado_de_ilsa");
    FParse::Value(FCommandLine::Get(), TEXT("VortexWeaponShot="), List, false);   // (sem cortar na vírgula)
    TArray<FString> Parts;
    List.ParseIntoArray(Parts, TEXT(","));
    for (const FString& Part : Parts) Shot->Types.Add(FName(*Part));
    TWeakObjectPtr<AVortexGameMode> Self(this);
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Controller, Shot](float Delta) -> bool
    {
        AVortexGameMode* Mode = Self.Get();
        if (!Mode) return false;
        AVortexGameState* State = Mode->GetGameState<AVortexGameState>();
        AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        AVortexTerrainStreamer* Streamer = nullptr;
        for (TActorIterator<AVortexTerrainStreamer> It(Mode->GetWorld()); It; ++It) Streamer = *It;
        if (!State || !Hero || !Streamer || !Streamer->IsReady()) return true;
        const FVector2D At(-30000.f, -8000.f);
        Shot->Time += Delta;
        // o lobo de treino ainda morde: o herói não pode morrer no meio do teste
        if (Hero->GetAbilitySystemComponent()) Hero->GetAbilitySystemComponent()->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Hero->GetVitals()->GetMaxHealth());
        const FName Type = Shot->Types.IsValidIndex(Shot->Index) ? Shot->Types[Shot->Index] : NAME_None;
        auto Capture = [&](const TCHAR* Phase)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/arma_%s_%s.png"), *Type.ToString(), Phase), true, false);
        };
        auto SpawnWolf = [&]()
        {
            if (AVortexWolf* Old = Shot->Wolf.Get()) Old->Destroy();
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            const FVector Position = Hero->GetActorLocation() + Hero->GetActorForwardVector() * 230.f;
            AVortexWolf* Wolf = Mode->GetWorld()->SpawnActor<AVortexWolf>(AVortexWolf::StaticClass(), Position, (Hero->GetActorLocation() - Position).Rotation(), Params);
            if (Wolf)
            {
                // boneco de treino: muita vida e parado
                Wolf->GetAbilitySystemComponent()->SetNumericAttributeBase(UVortexAttributes::GetMaxHealthAttribute(), 500.f);
                Wolf->GetAbilitySystemComponent()->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), 500.f);
                Wolf->CustomTimeDilation = 0.f;
                Wolf->SetActorTickEnabled(false);
            }
            Shot->Wolf = Wolf;
            Shot->WolfBefore = 500.f;
        };
        switch (Shot->Step)
        {
        case 0:
        {
            float Ground = 0.f;
            Hero->SetActorLocation(FVector(At, Streamer->GroundAt(At.X, At.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
            if (!Streamer->GroundReadyAt(At.X, At.Y, Ground) || Streamer->PendingChunks() > 0) return true;
            Hero->SetActorLocation(FVector(At, Ground + AVortexCharacter::HeroCapsuleHalfHeight + 4.f), false, nullptr, ETeleportType::TeleportPhysics);
            Hero->SetActorRotation(FRotator::ZeroRotator);
            Controller->SetControlRotation(FRotator(-8.f, 0.f, 0.f));
            State->SetDayTimeScale(0.f);
            State->SetDayClock(AVortexGameState::ClockOfHour(10.f), 1);
            Shot->Step = 1; Shot->Time = 0.f;
            return true;
        }
        case 1:
        {
            // próxima arma
            if (Type.IsNone())
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_WEAPON_SHOT_DONE %s"), Shot->bPass ? TEXT("PASS") : TEXT("FAIL"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
            UVortexWeaponDefinition* Definition = UVortexWeaponComponent::LoadType(Type);
            const bool bEquipped = Definition && Hero->GetWeapon()->Equip(Definition);
            if (!bEquipped)
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_WEAPON_SHOT %s FALHOU ao equipar"), *Type.ToString());
                Shot->bPass = false;
                ++Shot->Index;
                return true;
            }
            // vista de lado (3/4) para ver a arma na mão
            Controller->SetControlRotation(FRotator(-12.f, 150.f, 0.f));
            SpawnWolf();
            Shot->Step = 2; Shot->Time = 0.f;
            return true;
        }
        case 2:
            if (Shot->Time < 1.6f) return true;
            if (Shot->Time - Delta < 1.6f) { Capture(TEXT("pose")); return true; }
            // de volta para trás do herói, olhando o lobo
            Controller->SetControlRotation(FRotator(-8.f, 0.f, 0.f));
            Hero->GetWeapon()->PressPrimary();
            Shot->Step = 3; Shot->Time = 0.f;
            return true;
        case 3:
        {
            // o combo inteiro (cliques seguidos)
            const UVortexWeaponDefinition* Definition = Hero->GetWeapon()->GetDefinition();
            const FVortexWeaponSwing* First = Definition && Definition->Swings.Num() ? &Definition->Swings[0] : nullptr;
            const float Mid = First ? First->ActiveStart + .05f : .3f;
            if (Shot->Time - Delta < Mid && Shot->Time >= Mid) Capture(TEXT("golpe"));
            if (FMath::Fmod(Shot->Time, .2f) < Delta) Hero->GetWeapon()->PressPrimary();
            if (Shot->Time < 3.f) return true;
            Shot->Step = 4; Shot->Time = 0.f;
            return true;
        }
        case 4:
            if (Shot->Time < 1.4f) return true;
            Hero->GetWeapon()->PressSpecial();
            Shot->Step = 5; Shot->Time = 0.f;
            return true;
        case 5:
        {
            const UVortexWeaponDefinition* Definition = Hero->GetWeapon()->GetDefinition();
            const bool bCharge = Definition && Definition->SecondaryKind == TEXT("charge");
            if (bCharge && Shot->Time >= 1.f && Shot->Time - Delta < 1.f) { Capture(TEXT("carga")); Hero->GetWeapon()->ReleaseSpecialInput(); }
            const float ShotAt = bCharge ? 1.25f : .45f;
            if (Shot->Time >= ShotAt && Shot->Time - Delta < ShotAt) Capture(TEXT("especial"));
            if (Shot->Time < 2.5f) return true;
            AVortexWolf* Wolf = Shot->Wolf.Get();
            const float After = Wolf ? Wolf->Health() : Shot->WolfBefore;
            const bool bHit = After < Shot->WolfBefore;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_WEAPON_SHOT %s vida_do_lobo=%.0f->%.0f %s (heroi vida=%.0f especial=%d defesa=%d combo=%d)"), *Type.ToString(), Shot->WolfBefore, After, bHit ? TEXT("ok") : TEXT("FALHOU"),
                Hero->GetVitals()->GetHealth(), Hero->GetWeapon()->IsSpecialActive() ? 1 : 0, Hero->GetWeapon()->IsGuarding() ? 1 : 0, Hero->GetWeapon()->GetAttackState().SwingIndex);
            Shot->bPass &= bHit;
            ++Shot->Index;
            Shot->Step = 1; Shot->Time = 0.f;
            return true;
        }
        default:
            return true;
        }
    }));
}

void AVortexGameMode::BeginHudShot(APlayerController* NewPlayer)
{
    struct FShot { int32 Step = 0; float Time = 0.f; TWeakObjectPtr<AVortexWolf> Wolf; int32 Pelts = 0, Coins = 0, Red = 0; float Health = 0.f; bool bPass = true; int32 WindowPage = 0, Rebuilds = 0, Free = 0; };
    TSharedRef<FShot> Shot = MakeShared<FShot>();
    TWeakObjectPtr<AVortexGameMode> Self(this);
    TWeakObjectPtr<APlayerController> Controller(NewPlayer);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Controller, Shot](float Delta) -> bool
    {
        AVortexGameMode* Mode = Self.Get();
        if (!Mode) return false;
        AVortexGameState* State = Mode->GetGameState<AVortexGameState>();
        AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
        AVortexTerrainStreamer* Streamer = nullptr;
        for (TActorIterator<AVortexTerrainStreamer> It(Mode->GetWorld()); It; ++It) Streamer = *It;
        if (!State || !Hero || !Streamer || !Streamer->IsReady()) return true;
        const FVector2D At(-30000.f, -8000.f);
        auto Capture = [&](const TCHAR* Name)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/hud_%s.png"), Name), true, false);
            const AVortexHudBase* Hud = Cast<AVortexHudBase>(Controller->GetHUD());
            if (Hud) Hud->ExportPortraits(FString::Printf(TEXT("hud_%s_retrato_"), Name));
            const UVortexAttributes* Vitals = Hero->GetVitals();
            UE_LOG(LogTemp, Display, TEXT("VORTEX_HUD_SHOT %s minimapa=%d alvo=%s vida=%.0f mana=%.0f"), Name,
                Hud && Hud->HasMinimap() ? 1 : 0, Hud && Hud->GetTargetWolf() ? *Hud->GetTargetWolf()->GetName() : TEXT("-"),
                Vitals ? Vitals->GetHealth() : -1.f, Vitals ? Vitals->GetMana() : -1.f);
        };
        Shot->Time += Delta;
        switch (Shot->Step)
        {
        case 0:
        {
            float Ground = 0.f;
            Hero->SetActorLocation(FVector(At, Streamer->GroundAt(At.X, At.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
            if (!Streamer->GroundReadyAt(At.X, At.Y, Ground) || Streamer->PendingChunks() > 0) return true;
            Hero->SetActorLocation(FVector(At, Ground + AVortexCharacter::HeroCapsuleHalfHeight + 4.f), false, nullptr, ETeleportType::TeleportPhysics);
            Controller->SetControlRotation(FRotator(-8.f, 0.f, 0.f));
            State->SetDayTimeScale(0.f);
            State->SetDayClock(AVortexGameState::ClockOfHour(10.f), 1);
            {
                // o visual da criação também vale no jogo: um anão sorteado (baixo e parrudo)
                FRandomStream Rng(7);
                Hero->ApplyLook(FVortexLookCatalog::Get().Random(TEXT("anao"), TEXT("male"), Rng));
            }
            Shot->Step = 1; Shot->Time = 0.f;
            return true;
        }
        case 1:
            // um corte de aquecimento (o material compila agora, não na foto do especial)
            if (Shot->Time > .5f && Shot->Time - Delta <= .5f) Hero->GetWeapon()->PressSpecial();
            // o letreiro do lugar caindo (chegou no vale agora) e o chat de comandos (T) com algumas respostas
            if (Shot->Time > 2.6f && Shot->Time - Delta <= 2.6f) Capture(TEXT("lugar"));
            if (Shot->Time > 2.9f && Shot->Time - Delta <= 2.9f)
            {
                VortexDevChat::Open(Hero);
                bool bError = false;
                for (const TCHAR* Line : { TEXT("/god"), TEXT("/heal"), TEXT("/give potion_red 2"), TEXT("/nada") })
                {
                    VortexDevChat::AddLine(FString(TEXT("> ")) + Line);
                    const FString Reply = VortexDevChat::RunOnServer(Hero, Line, bError);
                    VortexDevChat::AddLine(Reply, bError);
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_DEVCHAT %s -> %s%s"), Line, *Reply, bError ? TEXT(" (erro)") : TEXT(""));
                }
                const UVortexAttributes* Vitals = Hero->GetVitals();
                Hero->GetAbilitySystemComponent()->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), 1.f);
                const bool bGodHeld = Vitals && Vitals->GetHealth() >= Vitals->GetMaxHealth();
                VortexDevChat::RunOnServer(Hero, TEXT("/god"), bError);
                // deus desligado: a vida volta a descer
                Hero->GetAbilitySystemComponent()->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Vitals ? Vitals->GetMaxHealth() - 5.f : 1.f);
                const bool bGodOff = Vitals && Vitals->GetHealth() < Vitals->GetMaxHealth();
                Hero->GetAbilitySystemComponent()->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Vitals ? Vitals->GetMaxHealth() : 1.f);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_DEVCHAT deus segurou a vida: %d, desligado voltou: %d %s"), bGodHeld ? 1 : 0, bGodOff ? 1 : 0,
                    bGodHeld && bGodOff ? TEXT("ok") : TEXT("FALHOU"));
                Shot->bPass &= bGodHeld && bGodOff;
            }
            if (Shot->Time > 3.4f && Shot->Time - Delta <= 3.4f) Capture(TEXT("chat"));
            if (Shot->Time > 3.7f && Shot->Time - Delta <= 3.7f) VortexDevChat::Close();
            if (Shot->Time < 5.f) return true;
            Capture(TEXT("dia"));
            Shot->Step = 2; Shot->Time = 0.f;
            return true;
        case 2:
        {
            if (Shot->Time < .3f) return true;
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            const FVector Position = Hero->GetActorLocation() + FVector(450.f, 60.f, 0.f);
            AVortexWolf* Wolf = Mode->GetWorld()->SpawnActor<AVortexWolf>(AVortexWolf::StaticClass(), Position,
                (Hero->GetActorLocation() - Position).Rotation(), Params);
            Shot->Wolf = Wolf;
            UAbilitySystemComponent* Abilities = Hero->GetAbilitySystemComponent();
            Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Hero->GetVitals()->GetHealth() - 35.f);
            Abilities->SetNumericAttributeBase(UVortexAttributes::GetManaAttribute(), Hero->GetVitals()->GetMana() - 20.f);
            Shot->Step = 3; Shot->Time = 0.f;
            return true;
        }
        case 3:
            if (Shot->Time < .6f) return true;
            if (AVortexWolf* Wolf = Shot->Wolf.Get())
            {
                Wolf->GetAbilitySystemComponent()->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Wolf->Health() - 18.f);
                Hero->GetCombat()->OnConfirmedHit.Broadcast(Hero, Wolf, 18.f);
                // e um crítico, para a foto mostrar a explosão vermelha
                Hero->GetCombat()->LastCritTarget = Wolf;
                Hero->GetCombat()->LastCritTime = float(Mode->GetWorld()->GetTimeSeconds());
                Hero->GetCombat()->OnConfirmedHit.Broadcast(Hero, Wolf, 47.f);
            }
            Shot->Step = 4; Shot->Time = 0.f;
            return true;
        case 4:
            if (Shot->Time < .35f) return true;
            Capture(TEXT("combate"));
            Shot->Step = 40; Shot->Time = 0.f;
            return true;
        case 40:
            // especial da espada (botão direito): o corte à distância voando
            if (Shot->Time < 1.2f) return true;
            Hero->GetWeapon()->PressSpecial();
            Shot->Step = 41; Shot->Time = 0.f;
            return true;
        case 41:
            if (Shot->Time < .36f) return true;
            Capture(TEXT("especial"));
            Shot->Step = 42; Shot->Time = 0.f;
            return true;
        case 42:
        {
            // ficha de RPG: vencer o lobo de verdade (XP, saque e moedas) e subir de nível
            if (Shot->Time < .8f) return true;
            UVortexHeroSheet* Sheet = Hero->GetSheet();
            if (!Sheet || !Sheet->IsLoaded()) return true;
            Shot->Pelts = Sheet->CountOf(TEXT("wolf_pelt"));
            Shot->Coins = Sheet->GetCoins();
            const int64 XpBefore = Sheet->GetXp();
            if (AVortexWolf* Wolf = Shot->Wolf.Get())
            {
                for (int32 i = 0; i < 20 && Wolf->Health() > 0.f; ++i) Hero->GetCombat()->ApplyHit(Wolf, 400.f);
            }
            // (o corte especial da foto anterior às vezes já venceu o lobo: vale o que a ficha recebeu)
            const bool bKill = Sheet->GetXp() > 0 && Sheet->CountOf(TEXT("wolf_pelt")) > 0 && Sheet->GetCoins() > 0;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_RPG lobo vencido xp=%lld->%lld pele=%d moedas=%d %s"), XpBefore, Sheet->GetXp(),
                Sheet->CountOf(TEXT("wolf_pelt")), Sheet->GetCoins(), bKill ? TEXT("ok") : TEXT("FALHOU"));
            Shot->bPass &= bKill;
            Sheet->AddXp(FVortexRpgCatalog::XpForLevel(3) - Sheet->GetXp());   // direto ao nível 3 (2 níveis de pontos)
            Shot->Step = 43; Shot->Time = 0.f;
            return true;
        }
        case 43:
        {
            if (Shot->Time < .7f) return true;
            Capture(TEXT("rpg_nivel"));
            UVortexHeroSheet* Sheet = Hero->GetSheet();
            const float MaxBefore = Hero->GetVitals()->GetMaxHealth();
            Sheet->ServerSpendPoint(VortexRpg::GVitality);
            Sheet->ServerSpendPoint(VortexRpg::GStrength);
            const bool bPoints = Sheet->GetLevel() == 3 && Sheet->GetPointsFree() == 2 * VortexRpg::PointsPerLevel - 2 && Hero->GetVitals()->GetMaxHealth() > MaxBefore;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_RPG nivel=%d pontos_livres=%d vida_max=%.0f->%.0f %s"), Sheet->GetLevel(), Sheet->GetPointsFree(),
                MaxBefore, Hero->GetVitals()->GetMaxHealth(), bPoints ? TEXT("ok") : TEXT("FALHOU"));
            Shot->bPass &= bPoints;
            // poção de vida (Q): gasta uma e cura
            Hero->GetAbilitySystemComponent()->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), 30.f);
            Shot->Red = Sheet->CountOf(TEXT("potion_red"));
            Shot->Health = Hero->GetVitals()->GetHealth();
            Sheet->ServerQuickUse(TEXT("potion_red"));
            // beber leva um instante (DrinkSeconds): vale ter começado a beber ou já ter gastado e curado
            const bool bPotion = Sheet->IsDrinking() || (Sheet->CountOf(TEXT("potion_red")) == Shot->Red - 1 && Hero->GetVitals()->GetHealth() > Shot->Health);
            UE_LOG(LogTemp, Display, TEXT("VORTEX_RPG pocao vermelhas=%d->%d vida=%.0f->%.0f bebendo=%d %s"), Shot->Red, Sheet->CountOf(TEXT("potion_red")),
                Shot->Health, Hero->GetVitals()->GetHealth(), Sheet->IsDrinking() ? 1 : 0, bPotion ? TEXT("ok") : TEXT("FALHOU"));
            Shot->bPass &= bPotion;
            // equipar: o capuz de lobo no corpo dá defesa
            Sheet->GiveItem(TEXT("wolf_head"), 1);
            const int32 Index = Sheet->GetBag().IndexOfByPredicate([](const FVortexItemStack& S) { return S.Item == TEXT("wolf_head"); });
            if (Index != INDEX_NONE) Sheet->ServerUseSlot(uint8(Index));
            const bool bEquip = Sheet->GetEquipped(VortexRpg::Head) == TEXT("wolf_head") && Sheet->ComputeStats().Defense > 0.f;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_RPG equipar cabeca=%s defesa=%.0f %s"), *Sheet->GetEquipped(VortexRpg::Head).ToString(),
                Sheet->ComputeStats().Defense, bEquip ? TEXT("ok") : TEXT("FALHOU"));
            Shot->bPass &= bEquip;
            Hero->GetAbilitySystemComponent()->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Hero->GetVitals()->GetMaxHealth() * .8f);
            VortexHeroWindow::Open(Hero, 0);
            Shot->Step = 44; Shot->Time = 0.f;
            return true;
        }
        case 44: case 45: case 46:
        {
            // as seis abas da janela do herói; a foto sai no fim do quadro: tira, espera um pouco e só então troca
            static const TCHAR* Pages[] = { TEXT("janela_personagem"), TEXT("janela_mochila"), TEXT("janela_habilidades"),
                TEXT("janela_classe"), TEXT("janela_alquimia"), TEXT("janela_missoes") };
            const int32 Page = Shot->WindowPage;
            UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
            if (!Sheet) return true;
            // "o menu continua piscando quando coloco pontos": gastar pontos com a janela aberta não pode refazê-la
            if (Page == 0 && Shot->Time > .6f && Shot->Time - Delta <= .6f)
            {
                Shot->Rebuilds = VortexHeroWindow::RebuildsForTest();
                Shot->Free = Sheet->GetPointsFree();
                VortexHeroWindow::SpendForTest(VortexRpg::Defense);
                VortexHeroWindow::SpendForTest(VortexRpg::Crit);
            }
            if (Page == 0 && Shot->Time > 1.3f && Shot->Time - Delta <= 1.3f)
            {
                const bool bSteady = VortexHeroWindow::RebuildsForTest() == Shot->Rebuilds && Sheet->GetPointsFree() == FMath::Max(0, Shot->Free - 2);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_RPG pontos com a janela aberta: refeita %d->%d vezes, livres %d->%d %s"), Shot->Rebuilds,
                    VortexHeroWindow::RebuildsForTest(), Shot->Free, Sheet->GetPointsFree(), bSteady ? TEXT("ok") : TEXT("FALHOU"));
                Shot->bPass &= bSteady;
            }
            if (Shot->Time < 1.5f) return true;
            if (Shot->Time - Delta < 1.5f) { Capture(Pages[Page]); return true; }
            if (Shot->Time < 1.8f) return true;
            if (Page < UE_ARRAY_COUNT(Pages) - 1)
            {
                VortexHeroWindow::Close();
                VortexHeroWindow::Open(Hero, Page + 1);
                ++Shot->WindowPage;
            }
            else Shot->Step = 47;
            Shot->Time = 0.f;
            return true;
        }
        case 47:
            if (Shot->Time < .3f) return true;
            VortexHeroWindow::Close();
            UE_LOG(LogTemp, Display, TEXT("VORTEX_RPG_SMOKE %s"), Shot->bPass ? TEXT("PASS") : TEXT("FAIL"));
            // o menu de pausa (Esc) com as opções
            if (UVortexGameInstance* Game = Cast<UVortexGameInstance>(Mode->GetGameInstance())) Game->TogglePauseMenu();
            Shot->Step = 48; Shot->Time = 0.f;
            return true;
        case 48:
            if (Shot->Time < .8f) return true;
            if (Shot->Time - Delta < .8f) { Capture(TEXT("pausa")); return true; }
            if (Shot->Time < 1.1f) return true;
            if (UVortexGameInstance* Game = Cast<UVortexGameInstance>(Mode->GetGameInstance())) Game->TogglePauseMenu();
            Shot->Step = 5; Shot->Time = 0.f;
            return true;
        case 5:
            // a foto sai no fim do quadro anterior: só agora some o lobo e vira a noite
            if (Shot->Time < 4.f)
            {
                if (AVortexWolf* Wolf = Shot->Wolf.Get())
                {
                    Wolf->Destroy();
                    // o lobo empurra o herói na luta: volta ao ponto da foto do dia
                    float Ground = 0.f;
                    if (Streamer->GroundReadyAt(At.X, At.Y, Ground))
                    {
                        Hero->SetActorLocation(FVector(At, Ground + AVortexCharacter::HeroCapsuleHalfHeight + 4.f), false, nullptr, ETeleportType::TeleportPhysics);
                    }
                    Controller->SetControlRotation(FRotator(-8.f, 0.f, 0.f));
                }
                if (State->GetHour() < 21.f) State->SetDayClock(AVortexGameState::ClockOfHour(22.f), 1);
                return true;
            }
            Capture(TEXT("noite"));
            Shot->Step = 6; Shot->Time = 0.f;
            return true;
        default:
            if (Shot->Time < .5f) return true;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_HUD_SHOT_DONE"));
            FPlatformMisc::RequestExit(false);
            return false;
        }
    }));
}
#endif
