// Passeio de desempenho "de verdade" (-VortexRoamBench, Tools/RunRoamBench.ps1): sai da cidade para o
// leste a galope (12,6 m/s) por 60 s, atravessando a emenda do vale com o continente, com criaturas, HUD
// e tudo ligado. Mede FPS, o pior 1% e os quadros longos, e o tempo de cada sistema (VortexProbe).
#if !UE_BUILD_SHIPPING
#include "Framework/VortexCreatureSmoke.h"
#include "World/VortexCreatureSubsystem.h"
#include "Misc/CoreDelegates.h"

#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Containers/Ticker.h"
#include "Creatures/VortexMonster.h"
#include "EngineUtils.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexProbe.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "HAL/PlatformMemory.h"
#include "ProceduralMeshComponent.h"
#include "RHIStats.h"
#include "UObject/UObjectIterator.h"
#include "Rpg/VortexHeroSheet.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexWeatherSubsystem.h"
#include "Framework/VortexGameState.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/HUD.h"
#include "UnrealClient.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Engine/Engine.h"
#include "Framework/VortexCharacterMovementComponent.h"
#include "World/VortexWorldLayout.h"

namespace VortexRoamBench
{
    // 12/11: -VortexRoamBench -VortexFilm[=trailer]: tomadas de câmera de Data/Film/<nome>.json (Tools/RecordFilm.ps1), com passo
    // fixo (-BENCHMARK -FPS=30). Em cada tomada: leva o herói ao lugar, acerta a hora e o clima, roda os comandos do chat
    // (Vortex.Cmd), espera o terreno carregar e salva cada quadro (sem a HUD) em Saved/Film/<tomada>/. Medidas do JSON em metros.
    // Câmeras: orbit (gira em volta), dolly (anda de At até To) e chase (atrás do que segue, girada From graus). Track "boss"
    // ou "hero" faz o centro andar junto; Fight faz o herói bater no inimigo mais perto; Ride galopa a cavalo (curva em TurnAt).
    void BeginFilm(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FShot
        {
            FString Name, Kind, Weather, Track;
            FVector2D At, To, Look, Hero, HeroNear;
            float Radius = 3000.f, Height = 1500.f, LookHeight = 200.f, From = 0.f, Sweep = 20.f, Seconds = 5.f, Hour = 12.f, Fov = 70.f;
            float HeightTo = -1.f, RadiusTo = -1.f, LookZ = -1.f, HeroYaw = 0.f, ScareAt = -1.f, Turn = 0.f, TurnAt = .5f;
            bool bHideHero = false, bLook = false, bNear = false, bFight = false, bRide = false, bKeep = false;
            TArray<FString> Commands;
        };
        TArray<FShot> Shots;
        FString File = TEXT("trailer"), Text;
        FParse::Value(FCommandLine::Get(), TEXT("VortexFilm="), File);
        TSharedPtr<FJsonObject> Root;
        if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Film") / (File + TEXT(".json")))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("VORTEX_FILM sem Data/Film/%s.json (ou JSON com erro)"), *File);
            FPlatformMisc::RequestExit(false);
            return;
        }
        auto Point = [](const TSharedPtr<FJsonObject>& O, const TCHAR* Key, FVector2D& Out)
        {
            const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
            if (!O->TryGetArrayField(Key, A) || A->Num() < 2) return false;
            Out = FVector2D((*A)[0]->AsNumber(), (*A)[1]->AsNumber()) * 100.f;
            return true;
        };
        auto Number = [](const TSharedPtr<FJsonObject>& O, const TCHAR* Key, float& Out, float Scale) { double D; if (O->TryGetNumberField(Key, D)) Out = float(D) * Scale; };
        for (const TSharedPtr<FJsonValue>& Value : Root->GetArrayField(TEXT("Shots")))
        {
            const TSharedPtr<FJsonObject> O = Value->AsObject();
            FShot Shot;
            Shot.Name = O->GetStringField(TEXT("Name"));
            Shot.Kind = O->GetStringField(TEXT("Kind"));
            O->TryGetStringField(TEXT("Weather"), Shot.Weather);
            O->TryGetStringField(TEXT("Track"), Shot.Track);
            Point(O, TEXT("At"), Shot.At);
            Shot.To = Shot.Hero = Shot.At;
            Point(O, TEXT("To"), Shot.To);
            Point(O, TEXT("Hero"), Shot.Hero);
            Shot.bLook = Point(O, TEXT("Look"), Shot.Look);
            Shot.bNear = Point(O, TEXT("HeroNear"), Shot.HeroNear);
            Number(O, TEXT("Radius"), Shot.Radius, 100.f); Number(O, TEXT("Height"), Shot.Height, 100.f); Number(O, TEXT("LookHeight"), Shot.LookHeight, 100.f);
            Number(O, TEXT("RadiusTo"), Shot.RadiusTo, 100.f); Number(O, TEXT("HeightTo"), Shot.HeightTo, 100.f); Number(O, TEXT("LookZ"), Shot.LookZ, 100.f);
            Number(O, TEXT("From"), Shot.From, 1.f); Number(O, TEXT("Sweep"), Shot.Sweep, 1.f); Number(O, TEXT("Seconds"), Shot.Seconds, 1.f);
            Number(O, TEXT("Hour"), Shot.Hour, 1.f); Number(O, TEXT("Fov"), Shot.Fov, 1.f); Number(O, TEXT("HeroYaw"), Shot.HeroYaw, 1.f);
            Number(O, TEXT("ScareAt"), Shot.ScareAt, 1.f); Number(O, TEXT("Turn"), Shot.Turn, 1.f); Number(O, TEXT("TurnAt"), Shot.TurnAt, 1.f);
            O->TryGetBoolField(TEXT("HideHero"), Shot.bHideHero);
            O->TryGetBoolField(TEXT("Fight"), Shot.bFight);
            O->TryGetBoolField(TEXT("Ride"), Shot.bRide);
            O->TryGetBoolField(TEXT("Keep"), Shot.bKeep);
            O->TryGetStringArrayField(TEXT("Commands"), Shot.Commands);
            Shots.Add(Shot);
        }
        struct FState
        {
            int32 Index = -1, Frame = 0; float Time = 0.f, SwingIn = 0.f; bool bRolling = false, bEntered = false, bSpawnedBoss = false;
            FVector Eye = FVector::ZeroVector; TWeakObjectPtr<ACameraActor> Camera; TWeakObjectPtr<AVortexMonster> Boss, Horse;
        };
        TSharedRef<FState> S = MakeShared<FState>();
        TWeakObjectPtr<AGameModeBase> WeakMode(Mode);
        TWeakObjectPtr<APlayerController> Controller(Player);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakMode, Controller, S, Shots](float Delta) -> bool
        {
            AGameModeBase* Game = WeakMode.Get();
            AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
            if (!Game) return false;
            UWorld* World = Game->GetWorld();
            AVortexTerrainStreamer* Streamer = nullptr;
            for (TActorIterator<AVortexTerrainStreamer> It(World); It; ++It) Streamer = *It;
            if (!Hero || !Streamer || !Streamer->IsReady() || !Streamer->IsInitialFillDone() || !Hero->GetSheet() || !Hero->GetSheet()->IsLoaded()) return true;
            AVortexGameState* State = World->GetGameState<AVortexGameState>();
            UVortexWeatherSubsystem* Weather = World->GetSubsystem<UVortexWeatherSubsystem>();
            if (S->Index < 0)
            {
                FActorSpawnParameters Params;
                Params.ObjectFlags |= RF_Transient;
                S->Camera = World->SpawnActor<ACameraActor>(Hero->GetActorLocation(), FRotator::ZeroRotator, Params);
                if (!S->Camera.IsValid()) return false;
                S->Camera->GetCameraComponent()->SetConstraintAspectRatio(false);
                Controller->SetViewTarget(S->Camera.Get());
                if (AHUD* Hud = Controller->GetHUD()) Hud->bShowHUD = false;
                GEngine->Exec(World, TEXT("Vortex.Cmd deus"));
                S->Index = 0;
            }
            if (S->Index >= Shots.Num())
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_FILM fim"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
            const FShot& Shot = Shots[S->Index];
            auto Ground = [Streamer](const FVector2D& At) { return Streamer->GroundAt(At.X, At.Y); };
            const FVector2D HeroDir = FVector2D(FRotator(0.f, Shot.HeroYaw, 0.f).Vector()).GetSafeNormal();
            if (!S->bEntered)
            {
                S->bEntered = true;
                S->Eye = FVector::ZeroVector;
                S->SwingIn = 0.f;
                if (!Shot.bRide) Hero->GetCharacterMovement()->DisableMovement();
                else Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);   // a tomada anterior deixou travado
                Hero->SetActorLocationAndRotation(FVector(Shot.Hero, Ground(Shot.Hero) + 110.f), FRotator(0.f, Shot.HeroYaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
                Controller->SetControlRotation(FRotator(0.f, Shot.HeroYaw, 0.f));
                Hero->SetActorHiddenInGame(Shot.bHideHero);
                if (Weather)
                {
                    Weather->ForceWeather(EVortexWeather::Clear);
                    for (uint8 W = 0; W <= uint8(EVortexWeather::Fog); ++W)
                        if (Shot.Weather.Equals(UVortexWeatherSubsystem::WeatherName(EVortexWeather(W)), ESearchCase::IgnoreCase)) Weather->ForceWeather(EVortexWeather(W));
                }
                for (const FString& Command : Shot.Commands) GEngine->Exec(World, *(TEXT("Vortex.Cmd ") + Command));
                // ScareAt: o herói sai de perto logo depois dos comandos (as fadas fogem de quem chega) e volta para At na hora
                if (Shot.ScareAt >= 0.f)
                {
                    const FVector2D Away = Shot.Hero - HeroDir * 3000.f;
                    Hero->SetActorLocation(FVector(Away, Ground(Away) + 110.f), false, nullptr, ETeleportType::TeleportPhysics);
                }
                S->Camera->GetCameraComponent()->SetFieldOfView(Shot.Fov);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_FILM tomada %s"), *Shot.Name);
            }
            S->Time += Delta;
            if (State) State->SetDayHour(Shot.Hour);
            const float T = S->bRolling ? FMath::Clamp(S->Time / Shot.Seconds, 0.f, 1.f) : 0.f;
            // o chefe: o do posto da Caldeira (um de teste só se ele não nasceu)
            if (Shot.Track == TEXT("boss") && !S->Boss.IsValid())
            {
                for (TActorIterator<AVortexMonster> It(World); It; ++It) if (It->IsBoss() && !It->IsCreatureDead()) S->Boss = *It;
                if (!S->Boss.IsValid() && !S->bSpawnedBoss && S->Time > 8.f)
                    if (UVortexCreatureSubsystem* Creatures = World->GetSubsystem<UVortexCreatureSubsystem>())
                    {
                        S->bSpawnedBoss = true;
                        const FVector2D Center(VortexWorldLayout::CalderaCenterX, VortexWorldLayout::CalderaCenterY);
                        Creatures->DebugSpawnNear(TEXT("RuneColossus"), FVector(Center, Ground(Center) + 100.f), 12);
                    }
            }
            // o cavalo: o herói ganha um domado e monta (a corrida só começa quando grava)
            if (Shot.bRide && !Hero->IsMounted())
            {
                if (!S->Horse.IsValid())
                {
                    Hero->GetSheet()->SetMountCoat(TEXT("Horse_Palomino"));
                    const FVector Side = Hero->GetActorLocation() + FVector(FVector2D(-HeroDir.Y, HeroDir.X) * 220.f, 0.f);
                    S->Horse = AVortexMonster::SpawnTamed(World, TEXT("Horse_Palomino"), FTransform(FRotator(0.f, Shot.HeroYaw, 0.f), Side), Hero);
                }
                else if (S->Time > 1.f) Hero->DebugHorse(S->Horse.Get());
            }
            AVortexMonster* Boss = S->Boss.Get();
            FVector2D Center = Shot.At;
            if (Shot.Track == TEXT("boss") && Boss) Center = FVector2D(Boss->GetActorLocation());
            else if (Shot.Track == TEXT("hero")) Center = FVector2D(Hero->GetActorLocation());
            // o herói perto do chefe (o chefe acorda e briga)
            if (Shot.bNear && Boss && FVector2D::Distance(FVector2D(Hero->GetActorLocation()), Center + Shot.HeroNear) > 400.f)
            {
                const FVector2D Spot = Center + Shot.HeroNear;
                Hero->SetActorLocation(FVector(Spot, Ground(Spot) + 110.f), false, nullptr, ETeleportType::TeleportPhysics);
            }
            if (Shot.ScareAt >= 0.f && S->bRolling && S->Time >= Shot.ScareAt && S->Time - Delta < Shot.ScareAt)
                Hero->SetActorLocation(FVector(Shot.At, Ground(Shot.At) + 110.f), false, nullptr, ETeleportType::TeleportPhysics);
            if (Shot.bRide && S->bRolling && Hero->IsMounted())
            {
                const float Yaw = Shot.HeroYaw + Shot.Turn * FMath::SmoothStep(Shot.TurnAt, FMath::Min(1.f, Shot.TurnAt + .25f), T);
                Hero->GetVortexMovement()->bWantsToSprint = true;
                Controller->SetControlRotation(FRotator(0.f, Yaw, 0.f));
                Hero->AddMovementInput(FRotator(0.f, Yaw, 0.f).Vector(), 1.f);
            }
            if (Shot.bFight)
            {
                // bate no inimigo vivo mais perto (até 25 m), virado para ele
                AActor* Foe = Boss;
                float Best = 2500.f;
                if (!Foe)
                    for (TActorIterator<AVortexMonster> It(World); It; ++It)
                        if (!It->IsCreatureDead() && !It->IsPassive() && !It->IsHorse())
                            if (const float D = FVector::Dist2D(It->GetActorLocation(), Hero->GetActorLocation()); D < Best) { Best = D; Foe = *It; }
                if (Foe)
                {
                    const float Yaw = (Foe->GetActorLocation() - Hero->GetActorLocation()).Rotation().Yaw;
                    Hero->SetActorRotation(FRotator(0.f, Yaw, 0.f));
                    Controller->SetControlRotation(FRotator(0.f, Yaw, 0.f));
                    S->SwingIn -= Delta;
                    if (S->SwingIn <= 0.f) { Hero->BeginPrimaryAttack(); S->SwingIn = .55f; }
                }
            }
            const float Radius = Shot.RadiusTo >= 0.f ? FMath::Lerp(Shot.Radius, Shot.RadiusTo, FMath::SmoothStep(0.f, 1.f, T)) : Shot.Radius;
            const float Height = Shot.HeightTo >= 0.f ? FMath::Lerp(Shot.Height, Shot.HeightTo, FMath::SmoothStep(0.f, 1.f, T)) : Shot.Height;
            FVector Eye, Target;
            auto Aim = [&](const FVector2D& Spot) { return FVector(Spot, Shot.LookZ >= 0.f ? Shot.LookZ : Ground(Spot) + Shot.LookHeight); };
            if (Shot.Kind == TEXT("chase"))
            {
                // atrás do que segue (From = 0: bem atrás; 90: do lado), olhando para ele
                const float Facing = Shot.Track == TEXT("hero") ? Hero->GetActorRotation().Yaw : Shot.HeroYaw;
                const FVector2D Dir = FVector2D(FRotator(0.f, Facing + 180.f + Shot.From + Shot.Sweep * T, 0.f).Vector()).GetSafeNormal();
                const FVector2D Spot = Center + Dir * Radius;
                Eye = FVector(Spot, FMath::Max(Ground(Center) + Height, Ground(Spot) + 150.f));
                Target = Aim(Shot.bLook ? Shot.Look : Center);
            }
            else if (Shot.Kind == TEXT("orbit"))
            {
                const float Angle = FMath::DegreesToRadians(Shot.From + Shot.Sweep * T);
                const FVector2D Spot = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
                Eye = FVector(Spot, FMath::Max(Ground(Center) + Height, Ground(Spot) + 150.f));
                Target = Aim(Shot.bLook ? Shot.Look : Center);
            }
            else   // dolly: de At até To, olhando para Look (ou para a frente)
            {
                const FVector2D Spot = FMath::Lerp(Shot.At, Shot.To, FMath::SmoothStep(0.f, 1.f, T));
                Eye = FVector(Spot, Ground(Spot) + Height);
                Target = Aim(Shot.bLook ? Shot.Look : Spot + (Shot.To - Shot.At).GetSafeNormal() * 2000.f);
            }
            // a câmera que segue anda macia (o galope e o chefe sacodem)
            if (!Shot.Track.IsEmpty() && !S->Eye.IsZero() && S->bRolling) Eye = FMath::VInterpTo(S->Eye, Eye, Delta, 4.f);
            S->Eye = Eye;
            S->Camera->SetActorLocationAndRotation(Eye, (Target - Eye).Rotation());
            if (!S->bRolling)
            {
                // o terreno e os bichos em volta carregam antes de gravar (3 a 25 s); o chefe e o cavalo precisam existir
                const bool bWaitBoss = Shot.Track == TEXT("boss") && !Boss && S->Time < 30.f;
                const bool bWaitHorse = Shot.bRide && !Hero->IsMounted() && S->Time < 20.f;
                if (S->Time < 3.f || bWaitBoss || bWaitHorse || (S->Time < 25.f && (Streamer->PendingChunks() > 0 || GetNumAsyncPackages() > 0))) return true;
                S->bRolling = true; S->Time = 0.f; S->Frame = 0;
                return true;
            }
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Film/%s/f_%05d.png"), *Shot.Name, S->Frame++), false, false);
            if (S->Time < Shot.Seconds) return true;
            if (Shot.bRide) Hero->GetVortexMovement()->bWantsToSprint = false;
            if (!Shot.bKeep) { GEngine->Exec(World, TEXT("Vortex.Cmd matar 300")); S->Boss = nullptr; }
            ++S->Index; S->Time = 0.f; S->bRolling = S->bEntered = false;
            return true;
        }));
    }

    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        FString Film;
        if (FParse::Param(FCommandLine::Get(), TEXT("VortexFilm")) || FParse::Value(FCommandLine::Get(), TEXT("VortexFilm="), Film)) { BeginFilm(Mode, Player); return; }
        struct FState { float Time = 0.f, WindowTime = 0.f; int32 WindowFrames = 0; double WindowGame = 0.0, WindowRender = 0.0, WindowGpu = 0.0; TArray<double> Frames, Settled; double GameMs = 0.0, RenderMs = 0.0, GpuMs = 0.0, Draws = 0.0, Prims = 0.0; int32 Samples = 0; bool bStarted = false; float Waited = 0.f; };
        TSharedRef<FState> S = MakeShared<FState>();
        // 08/11: quem carrega do disco "na marra" no meio do passeio (cada um é uma travada): o nome vai para o log
        {
            const double Start = FPlatformTime::Seconds();
            FCoreDelegates::OnSyncLoadPackage.AddLambda([Start](const FString& Package)
            {
                if (FPlatformTime::Seconds() - Start > 2.0 && Package.StartsWith(TEXT("/Game/")))
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_SYNC_LOAD t=%.1f %s"), FPlatformTime::Seconds() - Start, *Package);
            });
        }
        TWeakObjectPtr<AGameModeBase> WeakMode(Mode);
        TWeakObjectPtr<APlayerController> Controller(Player);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakMode, Controller, S](float Delta) -> bool
        {
            AGameModeBase* Game = WeakMode.Get();
            AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
            if (!Game) return false;
            AVortexTerrainStreamer* Streamer = nullptr;
            for (TActorIterator<AVortexTerrainStreamer> It(Game->GetWorld()); It; ++It) Streamer = *It;
            if (!Hero || !Streamer || !Streamer->IsReady() || !Streamer->IsInitialFillDone() || !Hero->GetSheet() || !Hero->GetSheet()->IsLoaded()) return true;
            // 11/11: como a tela de carregamento do jogador: só começa com a pré-carga pronta e a fila de carregamento vazia
            // (até 25 s). Antes as travadas da carga inicial entravam na conta do passeio
            if (!S->bStarted && S->Time <= 0.f)
            {
                S->Waited += Delta;
                const UVortexCreatureSubsystem* Creatures = Game->GetWorld()->GetSubsystem<UVortexCreatureSubsystem>();
                if (S->Waited < 25.f && ((Creatures && !Creatures->IsPreloadDone()) || GetNumAsyncPackages() > 0)) return true;
                UE_LOG(LogTemp, Display, TEXT("VORTEX_ROAM esperou a carga %.1f s (fila: %d pacotes)"), S->Waited, GetNumAsyncPackages());
            }
            S->Time += Delta;
            const float T = S->Time - 3.f;
            // galope para o leste saindo da cidade (a emenda do vale fica em x = 198 m)
            // -VortexRoamSeconds=N: passeio longo em volta de um círculo de 400 m (volta aos mesmos lugares: pedaços
            // saem e voltam), com um registro a cada 10 s para achar o que cresce com o tempo
            static float Seconds = 60.f;
            static const bool bLong = FParse::Value(FCommandLine::Get(), TEXT("VortexRoamSeconds="), Seconds);
            // -VortexRoamTown: parado na praça da cidade, girando a câmera devagar (como no print do jogador)
            static const bool bTown = FParse::Param(FCommandLine::Get(), TEXT("VortexRoamTown"));
            const FVector2D At = bTown ? FVector2D(600.f, 2600.f) : bLong
                ? FVector2D(20000.f, 6000.f) + FVector2D(FMath::Cos(FMath::Max(T, 0.f) * 1260.f / 40000.f), FMath::Sin(FMath::Max(T, 0.f) * 1260.f / 40000.f)) * 40000.f - FVector2D(40000.f, 0.f)
                : FVector2D(0.f, 6000.f) + FVector2D(1260.f, 80.f) * FMath::Max(T, 0.f);
            Hero->GetCharacterMovement()->DisableMovement();
            Hero->SetActorLocation(FVector(At, Streamer->GroundAt(At.X, At.Y) + 110.f), false, nullptr, ETeleportType::TeleportPhysics);
            Controller->SetControlRotation(FRotator(-10.f, bTown ? S->Time * 12.f : 4.f + 20.f * FMath::Sin(S->Time * .4f), 0.f));
            if (UAbilitySystemComponent* Abilities = Hero->GetAbilitySystemComponent())
                Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Hero->GetVitals()->GetMaxHealth());
            if (T <= 0.f) return true;
            if (!S->bStarted) { S->bStarted = true; VortexProbe::Entries().Reset(); }
            S->Frames.Add(Delta * 1000.0);
            if (T > 8.f) S->Settled.Add(Delta * 1000.0);   // depois da carga inicial
            if (Delta > .1f) UE_LOG(LogTemp, Display, TEXT("VORTEX_ROAM_SLOW %.0f ms em %.1f s"), Delta * 1000.0, T);
            S->GameMs += FPlatformTime::ToMilliseconds(GGameThreadTime);
            S->RenderMs += FPlatformTime::ToMilliseconds(GRenderThreadTime);
            S->GpuMs += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
            S->Draws += GNumDrawCallsRHI[0];
            S->Prims += GNumPrimitivesDrawnRHI[0];
            ++S->Samples;
            S->WindowTime += Delta; ++S->WindowFrames;
            S->WindowGame += FPlatformTime::ToMilliseconds(GGameThreadTime);
            S->WindowRender += FPlatformTime::ToMilliseconds(GRenderThreadTime);
            S->WindowGpu += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
            if (S->WindowTime >= 10.f)
            {
                int32 Ism = 0, Instances = 0, Actors = 0, Prims = 0;
                for (TObjectIterator<UPrimitiveComponent> It; It; ++It)
                {
                    if (!IsValid(*It) || !It->IsRegistered() || It->GetWorld() != Game->GetWorld()) continue;
                    ++Prims;
                    if (const UInstancedStaticMeshComponent* I = Cast<UInstancedStaticMeshComponent>(*It)) { ++Ism; Instances += I->GetInstanceCount(); }
                }
                for (TActorIterator<AActor> It(Game->GetWorld()); It; ++It) ++Actors;
                const int32 F = FMath::Max(1, S->WindowFrames);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_ROAM_JANELA t=%.0fs fps=%.1f jogo=%.1f render=%.1f gpu=%.1f memoria_mb=%.0f atores=%d primitivos=%d grupos=%d instancias=%d pedacos=%d"),
                    T, S->WindowFrames / S->WindowTime, S->WindowGame / F, S->WindowRender / F, S->WindowGpu / F,
                    FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0), Actors, Prims, Ism, Instances, Streamer->LoadedChunkCount());
                S->WindowTime = 0.f; S->WindowFrames = 0; S->WindowGame = S->WindowRender = S->WindowGpu = 0.0;
            }
            if (T < Seconds) return true;
            TArray<double> Sorted = S->Frames;
            Sorted.Sort([](double A, double B) { return A > B; });
            double Sum = 0.0;
            for (double F : Sorted) Sum += F;
            double Low = 0.0;
            const int32 LowCount = FMath::Max(1, Sorted.Num() / 100);
            for (int32 i = 0; i < LowCount; ++i) Low += Sorted[i];
            Low /= LowCount;
            int32 Over25 = 0, Over50 = 0;
            for (double F : Sorted) { Over25 += F > 25.0; Over50 += F > 50.0; }
            int32 Monsters = 0;
            for (TActorIterator<AVortexMonster> It(Game->GetWorld()); It; ++It) ++Monsters;
            const int32 N = FMath::Max(1, S->Samples);
            TArray<double> Calm = S->Settled;
            Calm.Sort([](double A, double B) { return A > B; });
            double CalmLow = 0.0;
            const int32 CalmCount = FMath::Max(1, Calm.Num() / 100);
            for (int32 i = 0; i < CalmCount && i < Calm.Num(); ++i) CalmLow += Calm[i];
            CalmLow /= CalmCount;
            int32 CalmOver25 = 0;
            for (double F : Calm) CalmOver25 += F > 25.0;
            if (Calm.Num())
                UE_LOG(LogTemp, Display, TEXT("VORTEX_ROAM_CALMO (depois de 8 s) low1%%=%.1f pior=%.0fms >25ms=%d de %d quadros"), 1000.0 / CalmLow, Calm[0], CalmOver25, Calm.Num());
            UE_LOG(LogTemp, Display, TEXT("VORTEX_ROAM fps=%.1f low1%%=%.1f pior=%.0fms >25ms=%d >50ms=%d jogo_ms=%.2f render_ms=%.2f gpu_ms=%.2f criaturas=%d pedacos=%d memoria_mb=%.0f"),
                1000.0 / (Sum / Sorted.Num()), 1000.0 / Low, Sorted[0], Over25, Over50, S->GameMs / N, S->RenderMs / N, S->GpuMs / N, Monsters,
                Streamer->LoadedChunkCount(), FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0));
            TArray<TPair<FString, VortexProbe::FEntry>> Rows;
            for (const TPair<FString, VortexProbe::FEntry>& Pair : VortexProbe::Entries()) Rows.Add(Pair);
            Rows.Sort([](const auto& A, const auto& B) { return A.Value.MaxMs > B.Value.MaxMs; });
            for (const auto& Row : Rows)
                UE_LOG(LogTemp, Display, TEXT("VORTEX_ROAM_PART %-18s total=%8.1fms max=%6.1fms vezes=%6d acima_8ms=%d"),
                    *Row.Key, Row.Value.TotalMs, Row.Value.MaxMs, Row.Value.Calls, Row.Value.Over8);
            // o que a cena tem: pedidos de desenho por quadro e quantos objetos de cada tipo estão montados
            int32 Ism = 0, Instances = 0, Procedural = 0, Statics = 0;
            for (TObjectIterator<UPrimitiveComponent> It; It; ++It)
            {
                const UPrimitiveComponent* C = *It;
                if (!IsValid(C) || !C->IsRegistered() || C->GetWorld() != Game->GetWorld() || !C->IsVisible()) continue;
                if (const UInstancedStaticMeshComponent* I = Cast<UInstancedStaticMeshComponent>(C)) { ++Ism; Instances += I->GetInstanceCount(); }
                else if (C->IsA<UProceduralMeshComponent>()) ++Procedural;
                else if (C->IsA<UStaticMeshComponent>()) ++Statics;
            }
            UE_LOG(LogTemp, Display, TEXT("VORTEX_ROAM_CENA desenhos=%.0f triangulos_mil=%.0f grupos_instancias=%d instancias=%d malhas_procedurais=%d malhas_estaticas=%d"),
                S->Draws / N, S->Prims / N / 1000.0, Ism, Instances, Procedural, Statics);
            FPlatformMisc::RequestExit(false);
            return false;
        }));
    }
}

namespace
{
    // 12/11: -VortexDumpFrames=nome: salva todo quadro (sem a HUD) em Saved/Film/<nome>/, para os testes que jogam sozinhos
    // virarem filmagem do trailer (Tools/RecordGameplay.ps1, com passo fixo). Começa quando o herói existe e a carga acabou.
    struct FFrameDumper
    {
        FFrameDumper()
        {
            FCoreDelegates::GetOnPostEngineInit().AddLambda([]
            {
                FString Name;
                if (!FParse::Value(FCommandLine::Get(), TEXT("VortexDumpFrames="), Name)) return;
                TSharedRef<int32> Frame = MakeShared<int32>(-1);
                FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Name, Frame](float) -> bool
                {
                    bool bReady = false;
                    for (const FWorldContext& Context : GEngine->GetWorldContexts())
                        if (Context.WorldType == EWorldType::Game && Context.World())
                            if (APlayerController* PC = Context.World()->GetFirstPlayerController())
                            {
                                bReady |= PC->GetPawn() != nullptr;
                                if (AHUD* Hud = PC->GetHUD()) Hud->bShowHUD = false;   // filmagem sem a HUD (o teste pode reabrir)
                            }
                    if (*Frame < 0)
                    {
                        if (!bReady || GetNumAsyncPackages() > 0) return true;
                        *Frame = 0;
                    }
                    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Film/%s/f_%05d.png"), *Name, (*Frame)++), false, false);
                    return true;
                }));
            });
        }
    } GFrameDumper;
}
#endif
