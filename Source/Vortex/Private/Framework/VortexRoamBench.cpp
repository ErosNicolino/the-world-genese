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

namespace VortexRoamBench
{
    // 12/11: -VortexRoamBench -VortexFilm: um passeio de câmera pela vila para gravar vídeo (Tools/RecordFilm.ps1).
    // Três tomadas: do alto à tarde, na rua no pôr do sol e na praça à noite com chuva. Salva cada quadro em Saved/Film/frames.
    void BeginFilm(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState { float Time = -1.f, Waited = 0.f; TWeakObjectPtr<ACameraActor> Camera; int32 Shot = -1, Frame = 0; bool bRolling = false; };
        TSharedRef<FState> S = MakeShared<FState>();
        TWeakObjectPtr<AGameModeBase> WeakMode(Mode);
        TWeakObjectPtr<APlayerController> Controller(Player);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakMode, Controller, S](float Delta) -> bool
        {
            AGameModeBase* Game = WeakMode.Get();
            AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
            if (!Game) return false;
            UWorld* World = Game->GetWorld();
            AVortexTerrainStreamer* Streamer = nullptr;
            for (TActorIterator<AVortexTerrainStreamer> It(World); It; ++It) Streamer = *It;
            if (!Hero || !Streamer || !Streamer->IsReady() || !Streamer->IsInitialFillDone() || !Hero->GetSheet() || !Hero->GetSheet()->IsLoaded()) return true;
            const FVector2D Plaza(1800.f, 2100.f);
            auto Ground = [Streamer](const FVector2D& At) { return Streamer->GroundAt(At.X, At.Y); };
            const FVector2D HeroAt = Plaza + FVector2D(-500.f, 300.f);
            Hero->GetCharacterMovement()->DisableMovement();
            Hero->SetActorLocation(FVector(HeroAt, Ground(HeroAt) + 110.f), false, nullptr, ETeleportType::TeleportPhysics);
            Hero->SetActorRotation(FRotator(0.f, -40.f, 0.f));
            if (AHUD* Hud = Controller->GetHUD()) Hud->bShowHUD = false;
            AVortexGameState* State = World->GetGameState<AVortexGameState>();
            UVortexWeatherSubsystem* Weather = World->GetSubsystem<UVortexWeatherSubsystem>();
            if (!S->bRolling)
            {
                // como o passeio de FPS: espera a pré-carga e a fila de carregamento (até 25 s), e mais 4 s para assentar
                S->Waited += Delta;
                const UVortexCreatureSubsystem* Creatures = World->GetSubsystem<UVortexCreatureSubsystem>();
                if (S->Waited < 25.f && ((Creatures && !Creatures->IsPreloadDone()) || GetNumAsyncPackages() > 0)) return true;
                if (State) State->SetDayHour(16.5f);
                if (Weather) Weather->ForceWeather(EVortexWeather::Clear);
                FActorSpawnParameters Params;
                Params.ObjectFlags |= RF_Transient;
                S->Camera = World->SpawnActor<ACameraActor>(FVector(Plaza, Ground(Plaza) + 3000.f), FRotator(-30.f, 0.f, 0.f), Params);
                if (!S->Camera.IsValid()) return false;
                S->Camera->GetCameraComponent()->SetConstraintAspectRatio(false);
                S->Camera->GetCameraComponent()->SetFieldOfView(70.f);
                Controller->SetViewTarget(S->Camera.Get());
                S->Time = -4.f;
                S->bRolling = true;
            }
            S->Time += Delta;
            const float T = FMath::Max(S->Time, 0.f);
            if (S->Time >= 0.f && S->Time - Delta < 0.f) UE_LOG(LogTemp, Display, TEXT("VORTEX_FILM começo"));
            const FVector Look(Plaza, Ground(Plaza) + 250.f);
            FVector Eye;
            FVector Target = Look;
            float Hour;
            const int32 Shot = T < 14.f ? 0 : T < 26.f ? 1 : 2;
            if (Shot == 0)
            {
                // do alto, girando devagar em volta da praça
                const float Angle = FMath::DegreesToRadians(-140.f + T * 3.f);
                Eye = FVector(Plaza + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * 5200.f, Ground(Plaza) + 2600.f);
                Hour = 16.5f + T * .1f;
            }
            else if (Shot == 1)
            {
                // na altura dos olhos, andando pela rua até a praça
                const float A = FMath::SmoothStep(0.f, 1.f, (T - 14.f) / 12.f);
                const FVector2D At = FMath::Lerp(FVector2D(2300.f, -600.f), FVector2D(2300.f, 700.f), A);
                Eye = FVector(At, Ground(At) + 200.f);
                Target = FVector(At + FVector2D(1.f, .35f) * 2000.f, Ground(At) + 150.f);
                Hour = 18.f + (T - 14.f) * .1f;
            }
            else
            {
                // à noite, com chuva, perto da fonte e do herói
                const float Angle = FMath::DegreesToRadians(150.f + (T - 26.f) * 2.5f);
                Eye = FVector(Plaza + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * 1300.f, Ground(Plaza) + 420.f);
                Target = FVector(Plaza, Ground(Plaza) + 120.f);
                Hour = 21.f;
            }
            if (Shot != S->Shot)
            {
                S->Shot = Shot;
                if (Weather) Weather->ForceWeather(Shot == 2 ? EVortexWeather::Rain : EVortexWeather::Clear);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_FILM tomada %d em %.1f s"), Shot, T);
            }
            if (State) State->SetDayHour(Hour);
            S->Camera->SetActorLocationAndRotation(Eye, (Target - Eye).Rotation());
            // cada quadro vira uma imagem (o jogo roda escondido, com passo fixo: -BENCHMARK -FPS=30); o ffmpeg junta depois
            if (S->Time >= 0.f)
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Film/frames/f_%05d.png"), S->Frame++), false, false);
            if (T < 40.f) return true;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_FILM fim"));
            FPlatformMisc::RequestExit(false);
            return false;
        }));
    }

    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("VortexFilm"))) { BeginFilm(Mode, Player); return; }
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
#endif
