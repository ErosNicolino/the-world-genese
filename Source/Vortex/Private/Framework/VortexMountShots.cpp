// Teste com fotos dos coletáveis e do cavalo (-VortexMountShots, Tools/RunMountShots.ps1).
// Em L_TerrainProto (Saved/Screenshots/montaria_*.png):
//  1. coletáveis: vai até a moita (ou cogumelo) mais perto, mostra o brilho e o [E], colhe;
//  2. domar: um cavalo selvagem na frente, frutas silvestres uma a uma até ficar domado;
//  3. montar: parado de lado, galopando, desmontar;
//  4. assobio: o herói se afasta 40 m e chama; o cavalo aparece perto;
//  5. o mapa (M) com a névoa: só o caminho andado aparece; zoom na roda.
#if !UE_BUILD_SHIPPING
#include "Framework/VortexCreatureSmoke.h"

#include "Containers/Ticker.h"
#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Creatures/VortexMonster.h"
#include "EngineUtils.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexHudBase.h"
#include "Framework/VortexCharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Rpg/VortexHeroSheet.h"
#include "UnrealClient.h"
#include "World/VortexCreatureSubsystem.h"
#include "World/VortexForage.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

namespace VortexMountShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState
        {
            int32 Step = 0, Feeds = 0, Before = 0;
            float Time = 0.f, Speed = 0.f;
            bool bPass = true;
            TArray<FString> Lines;
            TWeakObjectPtr<AVortexMonster> Horse;
            FName SpotId;
            FVector Spot = FVector::ZeroVector;
        };
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
            if (!Hero || !Streamer || !Streamer->IsReady() || !Hero->GetSheet() || !Hero->GetSheet()->IsLoaded()) return true;
            UVortexHeroSheet* Sheet = Hero->GetSheet();
            UVortexForageSubsystem* Forage = World->GetSubsystem<UVortexForageSubsystem>();
            UVortexCreatureSubsystem* Creatures = World->GetSubsystem<UVortexCreatureSubsystem>();
            S->Time += Delta;
            const float T = S->Time;
            auto At = [&](float When) { return T >= When && T - Delta < When; };
            auto Shot = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/montaria_%s.png"), Name), true, false); };
            auto Check = [&](bool bOk, const FString& What) { S->Lines.Add(FString::Printf(TEXT("%s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What)); S->bPass &= bOk; };
            auto Next = [&](int32 Step) { S->Step = Step; S->Time = 0.f; };
            auto Face = [&](const FVector& Where, float Pitch, float Turn = 0.f) { Controller->SetControlRotation(FRotator(Pitch, (Where - Hero->GetActorLocation()).Rotation().Yaw + Turn, 0.f)); };
            auto Place = [&](const FVector2D& Where) { Hero->SetActorLocation(FVector(Where, Streamer->GroundAt(Where.X, Where.Y) + 110.f), false, nullptr, ETeleportType::TeleportPhysics); };
            AVortexMonster* Horse = S->Horse.Get();
            // clareira: nenhuma árvore ou pedra a 14 m (as fotos do cavalo precisam de vista livre)
            auto Clearing = [&](const FVector2D& From) -> FVector2D
            {
                const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
                if (!Data) return From;
                const float Size = Data->ChunkSize();
                for (int32 Ring = 0; Ring < 160; ++Ring)
                {
                    const float A = Ring * 2.39996f, R = 700.f * FMath::Sqrt(float(Ring));
                    const FVector2D P = From + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R;
                    float Surface = 0.f;
                    if (Data->IsWaterAt(P.X, P.Y, Surface) || !Data->ContainsWorldXY(P.X, P.Y)) continue;
                    bool bClear = true;
                    const FIntPoint C(FMath::FloorToInt(P.X / Size), FMath::FloorToInt(P.Y / Size));
                    for (int32 dx = -1; dx <= 1 && bClear; ++dx) for (int32 dy = -1; dy <= 1 && bClear; ++dy)
                        if (const TArray<FVortexTerrainProp>* Props = Data->PropsInChunk(C + FIntPoint(dx, dy)))
                            for (const FVortexTerrainProp& Prop : *Props)
                            {
                                const FString& Kind = Data->Kinds.IsValidIndex(Prop.Kind) ? Data->Kinds[Prop.Kind] : FString();
                                const bool bTall = Data->IsTree(Prop.Kind) || Kind.Contains(TEXT("rock")) || Kind.Contains(TEXT("fern_giant")) || Kind.StartsWith(TEXT("poi_"));
                                const float Keep = bTall ? 750.f : Kind.Contains(TEXT("bush")) ? 550.f : 0.f;
                                if (FVector2D::Distance(FVector2D(Prop.Location), P) < Keep)
                                {
                                    if (Ring % 20 == 0) UE_LOG(LogTemp, Display, TEXT("VORTEX_MOUNT_SHOTS clareira %d barrada por %s a %.0f cm"), Ring, *Kind, FVector2D::Distance(FVector2D(Prop.Location), P));
                                    bClear = false; break;
                                }
                            }
                    if (bClear) return P;
                }
                UE_LOG(LogTemp, Warning, TEXT("VORTEX_MOUNT_SHOTS sem clareira perto"));
                return From;
            };
            const FVector2D Open(-30000.f, -8000.f);
            switch (S->Step)
            {
            case 0:
            {
                float Ground = 0.f;
                Place(Open);
                if (!Streamer->GroundReadyAt(Open.X, Open.Y, Ground) || Streamer->PendingChunks() > 0 || T < 3.f) return true;
                Next(1);
                return true;
            }
            case 1:   // coletáveis: o mais perto, de preferência uma moita
            {
                if (At(.1f))
                {
                    TArray<const FVortexForageSpot*> Near;
                    if (Forage) Forage->Nearby(Hero->GetActorLocation(), 9000.f, Near);
                    const FVortexForageSpot* Pick = nullptr;
                    for (const FVortexForageSpot* Spot : Near)
                        if (!Pick || (Spot->Kind == TEXT("bush_berry")) > (Pick->Kind == TEXT("bush_berry"))
                            || ((Spot->Kind == TEXT("bush_berry")) == (Pick->Kind == TEXT("bush_berry")) && FVector::Dist2D(Spot->At, Hero->GetActorLocation()) < FVector::Dist2D(Pick->At, Hero->GetActorLocation())))
                            Pick = Spot;
                    Check(Near.Num() > 0, FString::Printf(TEXT("%d coletáveis a até 90 m"), Near.Num()));
                    if (!Pick) { Next(2); return true; }
                    S->SpotId = Pick->Id;
                    S->Spot = Pick->At;
                    S->Before = Sheet->CountOf(Pick->Item);
                    Check(true, FString::Printf(TEXT("coletável: %s (%s)"), *Pick->Name, *Pick->Kind.ToString()));
                    Place(FVector2D(Pick->At) + FVector2D(-150.f, 60.f));
                }
                if (S->SpotId.IsNone()) return true;
                if (T < 2.f) Face(S->Spot, -22.f, 25.f);
                if (At(2.2f)) Shot(TEXT("coletavel"));
                if (At(2.6f))
                {
                    const FVortexForageSpot* Spot = Forage->Find(S->SpotId, S->Spot);
                    const FName Item = Spot ? Spot->Item : NAME_None;
                    // a mesma função da tecla E (conversa, baú, cavalo e, sem eles, colher)
                    Hero->Interact();
                    Check(Sheet->CountOf(Item) > S->Before, FString::Printf(TEXT("%s na mochila (%d -> %d)"), *Item.ToString(), S->Before, Sheet->CountOf(Item)));
                    Check(!Forage->IsAvailable(S->SpotId), TEXT("a moita colhida sumiu (volta em 5 min)"));
                }
                if (At(3.4f)) Shot(TEXT("colhido"));
                if (At(3.7f)) Next(2);
                return true;
            }
            case 2:   // um cavalo selvagem na frente; frutas uma a uma
            {
                if (At(.1f))
                {
                    Place(Clearing(Open + FVector2D(800.f, 0.f)));
                    Sheet->GiveItem(TEXT("frutas_silvestres"), 6, false);
                    const FVector Where = Hero->GetActorLocation() + FVector(330.f, 0.f, 0.f);
                    if (Creatures) Creatures->DebugSpawnNear(TEXT("Horse_Palomino"), Where, 3);
                    for (TActorIterator<AVortexMonster> It(World); It; ++It)
                        if (It->IsHorse() && !It->GetTamer() && FVector::Dist2D(It->GetActorLocation(), Where) < 600.f) S->Horse = *It;
                    Check(S->Horse.IsValid(), TEXT("cavalo selvagem perto"));
                }
                if (!Horse) { if (T > .5f) Next(9); return true; }
                if (T < 1.5f) Face(Horse->GetActorLocation(), -12.f, 50.f);
                if (At(1.4f)) Shot(TEXT("cavalo_selvagem"));
                // uma fruta a cada 2,8 s (o cavalo para e come)
                if (T > 1.6f && FMath::Fmod(T - 1.6f, 2.8f) < Delta && S->Feeds < 6)
                {
                    ++S->Feeds;
                    Hero->SetActorLocation(Horse->GetActorLocation() - (Horse->GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D() * 260.f, false, nullptr, ETeleportType::TeleportPhysics);
                    Hero->DebugHorse(Horse);
                    if (S->Feeds == 1) Check(Horse->GetTameFed() == 1, FString::Printf(TEXT("primeira fruta (precisa de %d)"), Horse->GetTameNeed()));
                }
                if (S->Feeds == 1 && T > 2.5f && T - Delta <= 2.5f) Shot(TEXT("cavalo_comendo"));
                // domado: o selvagem vira o cavalo do herói
                AVortexMonster* Own = nullptr;
                for (TActorIterator<AVortexMonster> It(World); It; ++It) if (It->GetTamer() == Hero) Own = *It;
                if (Own)
                {
                    Check(true, FString::Printf(TEXT("domado com %d frutas"), S->Feeds));
                    Check(Sheet->GetMountCoat() == TEXT("Horse_Palomino"), TEXT("pelagem salva na ficha"));
                    S->Horse = Own;
                    Next(3);
                }
                else if (T > 20.f) { Check(false, TEXT("domar em até 6 frutas")); Next(9); }
                return true;
            }
            case 3:   // montar: de lado parado, galope, desmontar
            {
                if (!Horse) { Next(9); return true; }
                if (At(1.f)) Shot(TEXT("domado"));
                if (At(1.5f))
                {
                    Hero->SetActorLocation(Horse->GetActorLocation() + Horse->GetActorRightVector() * -200.f, false, nullptr, ETeleportType::TeleportPhysics);
                    Hero->DebugHorse(Horse);
                    Check(Hero->IsMounted() && Horse->GetRider() == Hero, TEXT("montou com o E"));
                }
                if (T > 1.6f && T < 3.f) Controller->SetControlRotation(FRotator(-10.f, Hero->GetActorRotation().Yaw + 80.f, 0.f));
                if (At(2.8f))
                {
                    Shot(TEXT("montado"));
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_MOUNT_SHOTS giro: herói %.0f, malha do herói %.0f, cavalo %.0f, malha do cavalo %.0f, câmera %.0f"),
                        Hero->GetActorRotation().Yaw, Hero->GetMesh()->GetComponentRotation().Yaw, Horse->GetActorRotation().Yaw, Horse->GetMesh()->GetComponentRotation().Yaw, Controller->GetControlRotation().Yaw);
                }
                if (T > 3.f && T < 7.f)
                {
                    Hero->GetVortexMovement()->bWantsToSprint = true;
                    Hero->AddMovementInput(FRotator(0.f, Controller->GetControlRotation().Yaw - 80.f, 0.f).Vector(), 1.f);
                    if (T > 3.2f) Controller->SetControlRotation(FMath::RInterpTo(Controller->GetControlRotation(), FRotator(-10.f, Hero->GetActorRotation().Yaw + 35.f, 0.f), Delta, 3.f));
                    S->Speed = FMath::Max(S->Speed, Hero->GetVelocity().Size2D());
                }
                if (At(5.5f)) Shot(TEXT("galope"));
                if (At(7.1f))
                {
                    Hero->GetVortexMovement()->bWantsToSprint = false;
                    Check(S->Speed > Hero->GetVortexMovement()->SprintSpeed * 1.4f, FString::Printf(TEXT("galope a %.0f cm/s (a pé correndo: %.0f)"), S->Speed, Hero->GetVortexMovement()->SprintSpeed));
                }
                if (At(8.f))
                {
                    Hero->Dismount();
                    Check(!Hero->IsMounted() && !Horse->GetRider() && !Horse->GetAttachParentActor(), TEXT("desmontou"));
                }
                if (At(9.f)) { Face(Horse->GetActorLocation(), -10.f, 15.f); }
                if (At(9.6f)) Shot(TEXT("desmontado"));
                if (At(10.f)) Next(4);
                return true;
            }
            case 4:   // assobio: 40 m longe e chama
            {
                if (!Horse && T < 11.9f) { Next(9); return true; }   // depois de 12 s ele some de propósito (longe do herói)
                if (At(.1f)) Place(FVector2D(Horse->GetActorLocation()) + FVector2D(0.f, 4000.f));
                if (At(.6f)) { S->Before = FMath::RoundToInt(FVector::Dist2D(Horse->GetActorLocation(), Hero->GetActorLocation())); Hero->DebugWhistle(); }
                if (At(1.2f)) Face(Horse->GetActorLocation(), -8.f);
                if (At(3.5f)) Face(Horse->GetActorLocation(), -8.f);
                if (At(4.f)) Shot(TEXT("assobio"));
                if (At(6.f))
                {
                    const float Now = FVector::Dist2D(Horse->GetActorLocation(), Hero->GetActorLocation());
                    Check(Now < 1200.f, FString::Printf(TEXT("o cavalo veio com o assobio (%d m -> %.0f m)"), S->Before / 100, Now / 100.f));
                }
                // depois de chegar ele fica: o herói anda 15 m e o cavalo não vai atrás
                if (At(9.5f)) Place(FVector2D(Hero->GetActorLocation()) + FVector2D(1500.f, 0.f));   // já chegou (para a 5 m)
                if (At(12.f))
                {
                    const float Now = FVector::Dist2D(Horse->GetActorLocation(), Hero->GetActorLocation());
                    Check(Now > 1200.f, FString::Printf(TEXT("o cavalo ficou onde chegou (não seguiu: %.0f m)"), Now / 100.f));
                    // e longe (80 m) vai embora
                    Place(FVector2D(Hero->GetActorLocation()) + FVector2D(8000.f, 0.f));
                }
                if (At(13.f))
                {
                    Check(!S->Horse.IsValid(), TEXT("longe do herói o cavalo foi embora"));
                    Next(5);
                }
                return true;
            }
            case 5:   // o mapa (M): só o que foi andado sai da névoa; zoom na roda
            {
                AVortexHudBase* Hud = Cast<AVortexHudBase>(Controller->GetHUD());
                if (!Hud) { Next(9); return true; }
                if (At(.1f)) Hud->ToggleWorldMap();
                if (At(1.6f)) Shot(TEXT("mapa"));
                if (At(1.8f)) { Hud->ZoomWorldMap(-1.f); Hud->ZoomWorldMap(-1.f); Hud->ZoomWorldMap(-1.f); }
                if (At(2.4f)) Shot(TEXT("mapa_zoom"));
                if (At(2.6f))
                {
                    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
                    const FBox2D Bounds = Data ? Data->PrototypeBounds() : FBox2D(ForceInit);
                    Check(Hud->IsDiscovered(Hero->GetActorLocation()), TEXT("onde o herói está saiu da névoa"));
                    Check(!Hud->IsDiscovered(FVector(Bounds.Max.X - 3000.f, Bounds.Max.Y - 3000.f, 0.f)), TEXT("o canto longe do mapa segue na névoa"));
                    Check(Hud->ExploredFraction() > 0.f && Hud->ExploredFraction() < .5f, FString::Printf(TEXT("explorado %.1f%%"), Hud->ExploredFraction() * 100.f));
                    Hud->ToggleWorldMap();
                    Next(6);
                }
                return true;
            }
            case 6:   // morte: animação, tela "VOCÊ CAIU" e renascer na fonte
            {
                UAbilitySystemComponent* Abilities = Hero->GetAbilitySystemComponent();
                if (At(.1f)) { Controller->SetControlRotation(FRotator(-20.f, 30.f, 0.f)); Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), 0.f); }
                if (At(.6f)) Shot(TEXT("morte_1"));
                if (At(1.8f)) Shot(TEXT("morte_2"));
                if (At(3.f)) { Shot(TEXT("morte_3")); Check(Hero->IsDead() && Hero->GetDeadTime() > 2.f, TEXT("caído, com a tela de morte")); }
                if (At(9.f))
                {
                    Check(!Hero->IsDead(), TEXT("renasceu na fonte depois de 5 s"));
                    Next(7);
                }
                return true;
            }
            case 7:   // tela de carregamento: salto longe; some quando o chão em volta monta
            {
                AVortexHudBase* Hud = Cast<AVortexHudBase>(Controller->GetHUD());
                if (!Hud) { Next(9); return true; }
                if (At(.1f)) { Hud->DebugBeginLoading(); Place(FVector2D(-60000.f, 30000.f)); }
                if (At(.8f)) Shot(TEXT("carregando"));
                if (At(1.f)) Check(Hud->IsLoadingScreen(), TEXT("tela de carregamento aberta"));
                if (T > 3.f && !Hud->IsLoadingScreen()) { Check(true, FString::Printf(TEXT("carregamento terminou em %.1f s"), T)); Next(9); }
                if (T > 30.f) { Check(false, TEXT("carregamento terminou")); Next(9); }
                return true;
            }
            default:
                for (const FString& Line : S->Lines) UE_LOG(LogTemp, Display, TEXT("VORTEX_MOUNT_SHOTS %s"), *Line);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_MOUNT_SHOTS_%s"), S->bPass ? TEXT("PASS") : TEXT("FAIL"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
        }));
    }
}
#endif
