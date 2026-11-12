// Teste com fotos da rotina dos moradores (08/11, -VortexNpcShots, Tools/RunNpcShots.ps1; Saved/Screenshots/npc_*.png):
// o dia da vila em quatro horas (9h no serviço, 12h30 o almoço na taverna, 17h30 nos bancos, 23h dormindo), com a
// conta de quem chegou, sentou e dormiu, e o cumprimento quando o herói passa perto.
#if !UE_BUILD_SHIPPING
#include "Framework/VortexCreatureSmoke.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Containers/Ticker.h"
#include "Creatures/VortexNpc.h"
#include "Creatures/VortexNpcRoutine.h"
#include "EngineUtils.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexGameState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Rpg/VortexHeroSheet.h"
#include "UnrealClient.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

namespace VortexNpcShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        // a hora, onde o herói fica, de onde a câmera olha (alto) e para onde, o que conferir
        struct FStep { float Hour; FVector2D Hero; FVector Eye; FVector2D Look; const TCHAR* Name; };
        static const FStep Steps[] = {
            { 9.f, FVector2D(1800.f, 2400.f), FVector(1800.f, 4600.f, 1500.f), FVector2D(1800.f, 2100.f), TEXT("manha_praca") },
            { 9.2f, FVector2D(-1300.f, 4700.f), FVector(-700.f, 4200.f, 450.f), FVector2D(-1600.f, 4990.f), TEXT("manha_forja") },
            { 12.5f, FVector2D(600.f, 1500.f), FVector(600.f, 1700.f, 380.f), FVector2D(600.f, 900.f), TEXT("almoco_taverna") },
            { 17.5f, FVector2D(1800.f, 2400.f), FVector(1800.f, 3900.f, 900.f), FVector2D(1800.f, 2000.f), TEXT("tarde_bancos") },
            { 23.f, FVector2D(1800.f, 2400.f), FVector(1800.f, 4600.f, 1500.f), FVector2D(1800.f, 2100.f), TEXT("noite_praca") },
        };
        struct FState { int32 Index = -1; float Time = 0.f; bool bPass = true; TArray<FString> Lines; TWeakObjectPtr<AActor> Camera; };
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
            const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
            AVortexGameState* State = World->GetGameState<AVortexGameState>();
            if (!Hero || !Streamer || !Streamer->IsReady() || !Data || !State || !Hero->GetSheet() || !Hero->GetSheet()->IsLoaded()) return true;
            S->Time += Delta;
            auto Check = [&](bool bOk, const FString& What) { S->Lines.Add(FString::Printf(TEXT("%s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What)); S->bPass &= bOk; };
            if (S->Index < 0 || S->Time > 63.f)
            {
                if (S->Index >= 0)
                {
                    // a conta da hora que acabou
                    int32 Routine = 0, Arrived = 0, Sitting = 0, Asleep = 0, Working = 0, Greeted = 0;
                    for (TActorIterator<AVortexNpc> It(World); It; ++It)
                    {
                        if (!VortexNpcRoutine::HasRoutine(It->GetNpcId())) continue;
                        ++Routine;
                        FVector2D At;
                        bool bArrived = false;
                        It->GetRoutineGoal(At, bArrived);
                        Arrived += bArrived ? 1 : 0;
                        if (!bArrived)
                            S->Lines.Add(FString::Printf(TEXT("   longe: %s a %.0f m do destino (%.0f, %.0f) m, vel %.0f"), *It->GetNpcId().ToString(),
                                FVector2D::Distance(FVector2D(It->GetActorLocation()), At) / 100.f, At.X / 100.f, At.Y / 100.f, It->GetVelocity().Size2D()));
                        Sitting += It->IsSitting() ? 1 : 0;
                        Asleep += It->IsAsleep() ? 1 : 0;
                        Working += !It->GetActivity().IsNone() ? 1 : 0;
                        Greeted += It->GetGestureSeq() > 0 ? 1 : 0;
                    }
                    const FStep& Was = Steps[S->Index];
                    const FString Line = FString::Printf(TEXT("%s (%.1fh): %d moradores, %d chegaram, %d fazendo algo, %d sentados, %d dormindo, %d cumprimentaram"),
                        Was.Name, Was.Hour, Routine, Arrived, Working, Sitting, Asleep, Greeted);
                    bool bOk = Routine >= 10 && Arrived * 100 >= Routine * 65;
                    if (Was.Hour > 17.f && Was.Hour < 18.f) bOk &= Sitting >= 1;
                    if (Was.Hour > 22.f) bOk &= Asleep >= 8;
                    if (S->Index == 1) bOk &= Greeted > 0;
                    Check(bOk, Line);
                    if (AActor* Camera = S->Camera.Get()) Camera->Destroy();
                }
                if (++S->Index >= int32(UE_ARRAY_COUNT(Steps)))
                {
                    for (const FString& L : S->Lines) UE_LOG(LogTemp, Display, TEXT("VORTEX_NPC_SHOTS %s"), *L);
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_NPC_SHOTS_%s"), S->bPass ? TEXT("PASS") : TEXT("FAIL"));
                    FPlatformMisc::RequestExit(false);
                    return false;
                }
                const FStep& Step = Steps[S->Index];
                State->SetDayClock(AVortexGameState::ClockOfHour(Step.Hour), 1);
                Hero->SetActorLocation(FVector(Step.Hero, Data->GroundAt(Step.Hero.X, Step.Hero.Y) + 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                FActorSpawnParameters Params;
                Params.ObjectFlags |= RF_Transient;
                const FVector Look(Step.Look, Data->GroundAt(Step.Look.X, Step.Look.Y) + 120.f);
                const FVector Eye(FVector2D(Step.Eye), Data->GroundAt(Step.Eye.X, Step.Eye.Y) + Step.Eye.Z);
                if (ACameraActor* Camera = World->SpawnActor<ACameraActor>(Eye, (Look - Eye).Rotation(), Params))
                {
                    Camera->GetCameraComponent()->SetConstraintAspectRatio(false);
                    Camera->GetCameraComponent()->SetFieldOfView(75.f);
                    Controller->SetViewTarget(Camera);
                    S->Camera = Camera;
                }
                S->Time = 0.f;
                return true;
            }
            // o relógio para nessa hora (o dia anda enquanto esperamos os moradores chegarem)
            State->SetDayClock(AVortexGameState::ClockOfHour(Steps[S->Index].Hour), 1);
            if (S->Time > 60.f && S->Time - Delta <= 60.f)
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/npc_%s.png"), Steps[S->Index].Name), true, false);
            return true;
        }));
    }
}
#endif
