// Os heróis novos de frente (-VortexHeroShots, Tools/RunHeroShots.ps1): troca a raça do herói (o modelo fixo de cada
// uma), vira a câmera para o rosto e fotografa parado e golpeando (Saved/Screenshots/heroi_<raça>[_golpe].png).
// Termina com VORTEX_HERO_SHOTS_PASS quando todas as raças trocaram de modelo.
#if !UE_BUILD_SHIPPING
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Framework/VortexCreatureSmoke.h"

#include "Combat/VortexWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexLook.h"
#include "Animation/VortexAnimInstance.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "ClothingSystemRuntimeTypes.h"
#include "Engine/WindDirectionalSource.h"
#include "EngineUtils.h"
#include "World/VortexFairies.h"

namespace VortexHeroShots
{
    // -VortexClothTest (09/11, Tools/RunElfChecks.ps1): o vento mexe o vestido parado? E quanto custam vários vestidos?
    void BeginClothTest(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState { int32 Phase = 0; double Next = 0.0, Sum = 0.0; int32 Frames = 0; float MaxMove = 0.f; TArray<FVector3f> First; TArray<TWeakObjectPtr<AVortexCharacter>> Crowd; double Base = 0.0, One = 0.0, Plain = 0.0; bool bPass = true; FString Pending; TArray<TWeakObjectPtr<USkeletalMeshComponent>> Armor; };
        TSharedRef<FState> S = MakeShared<FState>();
        TWeakObjectPtr<AGameModeBase> WeakMode = Mode;
        TWeakObjectPtr<APlayerController> Controller = Player;
        static auto Dress = [](AVortexCharacter* C, bool bCloth)
        {
            FVortexLook Look;
            Look.Race = bCloth ? TEXT("elfo") : TEXT("humano");
            Look.Appearance = TEXT("female");
            if (bCloth) Look.Pieces.Add(TEXT("block_outfit"), TEXT("block_outfit_Elfo_mago_f"));
            C->ApplyLook(Look);
        };
        static auto ClothOf = [](AVortexCharacter* C) -> USkeletalMeshComponent*
        {
            TArray<USkeletalMeshComponent*> Parts;
            C->GetComponents(Parts);
            for (USkeletalMeshComponent* P : Parts) if (P->GetFName() == TEXT("BlockOutfit_tecido")) return P;
            return nullptr;
        };
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakMode, Controller, S](float Delta) -> bool
        {
            UWorld* World = WeakMode.IsValid() ? WeakMode->GetWorld() : nullptr;
            AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
            if (!World || !Hero) return true;
            const double Now = World->GetTimeSeconds();
            S->Sum += Delta;
            ++S->Frames;
            auto Sample = [&]()
            {
                USkeletalMeshComponent* Cloth = ClothOf(Hero);
                if (!Cloth) return;
                for (const TPair<int32, FClothSimulData>& Pair : Cloth->GetCurrentClothingData_GameThread())
                {
                    const auto& Now_ = Pair.Value.Positions;
                    if (S->First.Num() != Now_.Num()) { S->First = TArray<FVector3f>(Now_); continue; }
                    for (int32 i = 0; i < Now_.Num(); ++i) S->MaxMove = FMath::Max(S->MaxMove, (Now_[i] - S->First[i]).Size());
                }
            };
            auto Fps = [&]() { const double V = S->Frames / FMath::Max(1e-3, S->Sum); return V; };
            auto Snap = [](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/tecido_%s.png"), Name), false, false); };
            auto Restart = [&](double Seconds) { S->Sum = 0.0; S->Frames = 0; S->MaxMove = 0.f; S->First.Reset(); S->Next = Now + Seconds; };
            if (S->Phase == 1 || S->Phase == 3) Sample();
            if (Now < S->Next) return true;
            switch (S->Phase)
            {
            case 0: Dress(Hero, false); Restart(4.0); S->Phase = 20; break;
            case 20: S->Base = Fps(); Dress(Hero, true); Restart(4.0); S->Phase = 21; break;   // sem vestido; depois assenta o pano
            case 21: Hero->GetMesh()->bPauseAnims = true; Snap(TEXT("vento_a")); Restart(4.0); S->Phase = 1; break;   // o corpo parado: o que mudar entre as fotos é o pano
            case 1:
                S->One = Fps();
                Snap(TEXT("vento_b"));
                for (TActorIterator<AWindDirectionalSource> It(World); It; ++It) It->Destroy();
                Restart(3.0);
                S->Phase = 2;
                break;
            case 2: Snap(TEXT("calmo_a")); Restart(4.0); S->Phase = 3; break;
            case 3:
                Snap(TEXT("calmo_b"));
                Hero->GetMesh()->bPauseAnims = false;
                Dress(Hero, true);   // o vento volta
                for (int32 i = 0; i < 8; ++i)
                {
                    FActorSpawnParameters Params;
                    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                    const FVector At = Hero->GetActorLocation() + FRotator(0.f, i * 45.f, 0.f).Vector() * 260.f;
                    if (AVortexCharacter* C = World->SpawnActor<AVortexCharacter>(Hero->GetClass(), At, FRotator::ZeroRotator, Params)) { Dress(C, false); S->Crowd.Add(C); }
                }
                Restart(6.0);
                S->Phase = 4;
                break;
            case 4:
                S->Plain = Fps();
                for (const TWeakObjectPtr<AVortexCharacter>& C : S->Crowd) if (C.IsValid()) Dress(C.Get(), true);
                Restart(3.0);
                S->Phase = 5;
                break;
            case 5: Restart(5.0); S->Phase = 6; break;
            case 6:
            {
                int32 Dressed = 0;
                for (const TWeakObjectPtr<AVortexCharacter>& C : S->Crowd) if (C.IsValid() && ClothOf(C.Get()) && ClothOf(C.Get())->GetSkeletalMeshAsset()) ++Dressed;
                UE_LOG(LogTemp, Display, TEXT("VORTEX_CLOTH_TEST fps: 1 sem vestido %.0f | 1 com vestido %.0f | 9 sem vestido %.0f | 9 com vestido %.0f (com pano nos extras: %d)"), S->Base, S->One, S->Plain, Fps(), Dressed);
                S->bPass &= Dressed == 8;
                Controller->SetControlRotation(FRotator(-18.f, Hero->GetActorRotation().Yaw + 180.f, 0.f));
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/tecido_grupo.png"), true, false);
                Restart(1.5);
                S->Phase = 7;
                break;
            }
            case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 14:
            {
                // as armaduras na luz do jogo: cada conjunto de frente e de costas (armadura_<conjunto>_<frente|costas>.png)
                static const TCHAR* Sets[] = { TEXT("guarda_real"), TEXT("magma"), TEXT("bosque_ancestral"), TEXT("geada") };
                static const TCHAR* Slots[] = { TEXT("head"), TEXT("chest"), TEXT("legs"), TEXT("boots") };
                const int32 Step = S->Phase - 7, SetIndex = Step / 2;
                if (Step % 2 == 0)
                {
                    FVortexLook Bare;
                    Bare.Race = TEXT("humano");
                    Bare.Pieces.Add(TEXT("block_outfit"), TEXT("block_outfit_nenhuma"));
                    Hero->ApplyLook(Bare);
                    for (const TWeakObjectPtr<AVortexCharacter>& C : S->Crowd) if (C.IsValid()) C->Destroy();
                    S->Crowd.Reset();
                    for (int32 i = 0; i < 4; ++i)
                    {
                        const FString Name = FString::Printf(TEXT("A_%s_%s"), Sets[SetIndex], Slots[i]);
                        USkeletalMeshComponent* Part = S->Armor.IsValidIndex(i) ? S->Armor[i].Get() : nullptr;
                        if (!Part)
                        {
                            Part = NewObject<USkeletalMeshComponent>(Hero);
                            Part->SetupAttachment(Hero->GetMesh());
                            Part->RegisterComponent();
                            S->Armor.Add(Part);
                        }
                        Part->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/humano/Armaduras/%s.%s"), *Name, *Name)));
                        Part->EmptyOverrideMaterials();
                        Part->SetLeaderPoseComponent(Hero->GetMesh());
                        S->bPass &= Part->GetSkeletalMeshAsset() != nullptr;
                    }
                }
                Controller->SetControlRotation(FRotator(-10.f, Hero->GetActorRotation().Yaw + (Step % 2 ? 0.f : 180.f), 0.f));
                S->Pending = FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/armadura_%s_%s.png"), Sets[SetIndex], Step % 2 ? TEXT("costas") : TEXT("frente"));
                Restart(12.0);   // a primeira vez que o material aparece a Unreal ainda compila a versão nova
                S->Phase += 100;
                break;
            }
            case 107: case 108: case 109: case 110: case 111: case 112: case 113: case 114:
                FScreenshotRequest::RequestScreenshot(S->Pending, true, false);
                Restart(.8);
                S->Phase -= 99;
                break;
            default:
                UE_LOG(LogTemp, Display, TEXT("VORTEX_CLOTH_TEST_%s"), S->bPass ? TEXT("PASS") : TEXT("FAIL"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
            return true;
        }));
    }

    // -VortexFairyShot (10/11, Tools/RunHeroShots.ps1 -Extra -VortexFairyShot): a fada parada no ar na frente do herói,
    // de frente, de lado e de costas, e 3 fotos seguidas da batida das asas (Saved/Screenshots/fada_*.png)
    void BeginFairyShot(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState { int32 Phase = 0; double Next = 0.0; TWeakObjectPtr<ACameraActor> Camera; TWeakObjectPtr<AVortexFairies> Fairies; FVector At; };
        TSharedRef<FState> S = MakeShared<FState>();
        TWeakObjectPtr<AGameModeBase> WeakMode = Mode;
        TWeakObjectPtr<APlayerController> Controller = Player;
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakMode, Controller, S](float) -> bool
        {
            UWorld* World = WeakMode.IsValid() ? WeakMode->GetWorld() : nullptr;
            APawn* Hero = Controller.IsValid() ? Controller->GetPawn() : nullptr;
            if (!World || !Hero) return true;
            const double Now = World->GetTimeSeconds();
            if (Now < S->Next) return true;
            if (!S->Fairies.IsValid())
            {
                TActorIterator<AVortexFairies> It(World);
                S->Fairies = It ? *It : World->SpawnActor<AVortexFairies>();
                S->At = Hero->GetActorLocation() + FVector(0.f, 0.f, 120.f);   // no lugar do herói (escondido); no mapa de verdade (-Map L_TerrainProto), com céu e luz
                S->Camera = World->SpawnActor<ACameraActor>();
                S->Camera->GetCameraComponent()->SetFieldOfView(30.f);
                S->Camera->GetCameraComponent()->bConstrainAspectRatio = false;
                Controller->SetViewTarget(S->Camera.Get());
                Hero->SetActorHiddenInGame(true);
                S->Next = Now + 1.5;
                return true;
            }
            AVortexFairies* F = S->Fairies.Get();
            const auto Look = [&](float Around, const TCHAR* Name, int32 Style)
            {
                F->DebugPlace(S->At, 0.f, Style);
                const FVector Dir = FRotator(0.f, Around, 0.f).Vector();
                const FVector Eye = S->At + FVector(0.f, 0.f, 5.f) + Dir * 38.f;
                S->Camera->SetActorLocationAndRotation(Eye, (S->At + FVector(0.f, 0.f, 5.f) - Eye).Rotation());
                if (Name) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/fada_%s.png"), Name), false, false);
            };
            // a fada olha para Yaw 0 (+X); a câmera dá a volta: 0 de frente, 90 de lado, 180 de costas
            switch (S->Phase++)
            {
            case 0: Look(0.f, nullptr, 0); break;
            case 1: Look(0.f, TEXT("frente"), 0); break;
            case 2: Look(70.f, TEXT("lado"), 1); break;
            case 3: Look(180.f, TEXT("costas"), 3); break;
            case 4: Look(30.f, TEXT("batida_1"), 5); break;
            case 5: Look(30.f, TEXT("batida_2"), 5); break;
            case 6: Look(30.f, TEXT("batida_3"), 5); break;
            default:
                UE_LOG(LogTemp, Display, TEXT("VORTEX_HERO_SHOTS_PASS fada"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
            S->Next = Now + (S->Phase >= 5 ? .033 : .8);
            return true;
        }));
    }

    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("VortexFairyShot"))) { BeginFairyShot(Mode, Player); return; }
        if (FParse::Param(FCommandLine::Get(), TEXT("VortexClothTest"))) { BeginClothTest(Mode, Player); return; }
        struct FState { int32 Index = 0, Phase = 0, Face = 0; double Step = 0.0; bool bPass = true; FVector Start = FVector::ZeroVector; FRotator Facing;
            TWeakObjectPtr<ACameraActor> Camera; };
        TSharedRef<FState> S = MakeShared<FState>();
        TWeakObjectPtr<AGameModeBase> WeakMode = Mode;
        TWeakObjectPtr<APlayerController> Controller = Player;
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakMode, Controller, S](float) -> bool
        {
            static const FName Races[] = { TEXT("humano"), TEXT("anao"), TEXT("elfo"), TEXT("barbaro"), TEXT("diabo"), TEXT("morto_vivo") };
            UWorld* World = WeakMode.IsValid() ? WeakMode->GetWorld() : nullptr;
            AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
            if (!World || !Hero) return true;
            const double Now = World->GetTimeSeconds();
            if (Now < S->Step) return true;
            if (S->Index >= UE_ARRAY_COUNT(Races))
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_HERO_SHOTS_%s"), S->bPass ? TEXT("PASS") : TEXT("FAIL"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
            const FName Race = Races[S->Index];
            const FString Shot = FPaths::ProjectSavedDir() / TEXT("Screenshots") / (TEXT("heroi_") + Race.ToString());
            switch (S->Phase)
            {
            case 0:   // veste a raça (espera a textura carregar), sempre no mesmo lugar (a foto correndo anda)
            {
                if (S->Start.IsZero()) { S->Start = Hero->GetActorLocation(); S->Facing = Hero->GetActorRotation(); }
                Hero->SetActorLocationAndRotation(S->Start, S->Facing, false, nullptr, ETeleportType::TeleportPhysics);
                FVortexLook Look;
                Look.Race = Race;
                if (Race == TEXT("elfo")) { Look.Appearance = TEXT("female"); Look.Pieces.Add(TEXT("block_outfit"), TEXT("block_outfit_Elfo_mago_f")); }   // 09/11: o vestido de tecido na foto
                Hero->ApplyLook(Look);
                const USkeletalMesh* Mesh = Hero->GetMesh()->GetSkeletalMeshAsset();
                const bool bOk = Mesh && Mesh->GetName() == TEXT("Hero_") + (Race == TEXT("elfo") ? FString(TEXT("humano")) : Race.ToString());   // o elfo usa o corpo do humano
                S->bPass &= bOk;
                UE_LOG(LogTemp, Display, TEXT("VORTEX_HERO_SHOTS %s modelo %s %s"), *Race.ToString(), Mesh ? *Mesh->GetName() : TEXT("nenhum"), bOk ? TEXT("ok") : TEXT("FALHOU"));
                S->Phase = 1;
                S->Step = Now + 2.5;
                return true;
            }
            case 1:   // de frente, parado
            {
                // o pé no chão: a sola (o osso do pé) perto da base da cápsula
                const float Floor = Hero->GetActorLocation().Z - Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
                UVortexAnimInstance* Anim = Cast<UVortexAnimInstance>(Hero->GetMesh()->GetAnimInstance());
                UE_LOG(LogTemp, Display, TEXT("VORTEX_HERO_SHOTS %s velocidade a pé %.0f cm/s (raça %.2f)"), *Race.ToString(),
                    Hero->GetCharacterMovement()->GetMaxSpeed(), Hero->GetRaceSpeed());
                UE_LOG(LogTemp, Display, TEXT("VORTEX_HERO_SHOTS %s clipe %s pés acima do chão %.1f / %.1f cm, quadril %.1f, malha z %.1f escala %s"), *Race.ToString(),
                    Anim ? *Anim->CurrentClip().ToString() : TEXT("-"), Hero->GetMesh()->GetSocketLocation(TEXT("Foot_L")).Z - Floor,
                    Hero->GetMesh()->GetSocketLocation(TEXT("Foot_R")).Z - Floor, Hero->GetMesh()->GetSocketLocation(TEXT("Hips")).Z - Floor,
                    Hero->GetMesh()->GetComponentLocation().Z - Floor, *Hero->GetMesh()->GetComponentScale().ToString());
            }
                Controller->SetControlRotation(FRotator(-8.f, Hero->GetActorRotation().Yaw + 180.f, 0.f));   // de frente para o rosto
                FScreenshotRequest::RequestScreenshot(Shot + TEXT(".png"), true, false);
                S->Phase = 100;
                S->Step = Now + .6;
                return true;
            case 100:   // o rosto de perto, um quadro de cada vez (heroi_<raça>_rosto_<n>.png)
            {
                if (!S->Camera.IsValid())
                {
                    FActorSpawnParameters Params;
                    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                    S->Camera = World->SpawnActor<ACameraActor>(Params);
                }
                Hero->SetActorRotation(S->Facing);     // a câmera de frente virava o herói
                // o meio do rosto: entre o osso da cabeça e o topo dela
                const FVector Head = FMath::Lerp(Hero->GetMesh()->GetSocketLocation(TEXT("mixamorig_Head")),
                    Hero->GetMesh()->GetSocketLocation(TEXT("mixamorig_HeadTop_End")), .4f);
                const FVector Eye = Head + Hero->GetActorForwardVector() * 55.f;
                S->Camera->SetActorLocationAndRotation(Eye, (Head - Eye).Rotation());
                Controller->SetViewTargetWithBlend(S->Camera.Get(), 0.f);
                AVortexCharacter::DebugFace = S->Face;
                S->Phase = 101;
                S->Step = Now + .3;
                return true;
            }
            case 101:
                FScreenshotRequest::RequestScreenshot(Shot + FString::Printf(TEXT("_rosto_%d.png"), S->Face), false, false);
                S->Phase = ++S->Face < 8 ? 100 : 102;
                S->Step = Now + .2;
                return true;
            case 102:
                AVortexCharacter::DebugFace = -1;
                S->Face = 0;
                Controller->SetViewTargetWithBlend(Hero, 0.f);
                S->Phase = 2;
                S->Step = Now + .3;
                return true;
            case 2:   // um golpe (a arma na mão e o corpo no meio do corte)
                if (Hero->GetWeapon()) Hero->GetWeapon()->PressPrimary();
                S->Phase = 3;
                S->Step = Now + .3;
                return true;
            case 3:
                Controller->SetControlRotation(FRotator(-8.f, Hero->GetActorRotation().Yaw + 180.f, 0.f));   // de frente para o rosto
                FScreenshotRequest::RequestScreenshot(Shot + TEXT("_golpe.png"), true, false);
                S->Phase = 4;
                S->Step = Now + 1.2;
                return true;
            case 4:   // de lado, correndo no lugar (a animação do passo normal, 460)
                UVortexAnimInstance::DebugSpeed = 460.f * Hero->GetRaceSpeed();
                Controller->SetControlRotation(FRotator(-8.f, Hero->GetActorRotation().Yaw + 90.f, 0.f));
                S->Phase = 5;
                S->Step = Now + 1.0;
                return true;
            case 5:
                if (const UVortexAnimInstance* Gait = Cast<UVortexAnimInstance>(Hero->GetMesh()->GetAnimInstance()))
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_HERO_SHOTS %s passo corrida %.0f cm/s: clipe %s taxa %.2f"), *Race.ToString(),
                        UVortexAnimInstance::DebugSpeed, *Gait->CurrentClip().ToString(), Gait->CurrentRate());
                FScreenshotRequest::RequestScreenshot(Shot + TEXT("_corrida.png"), true, false);
                S->Phase = 51;
                S->Step = Now + .4;
                return true;
            case 51:   // andando devagar e na disparada (Shift), de lado
                UVortexAnimInstance::DebugSpeed = 120.f * Hero->GetRaceSpeed();
                S->Phase = 52;
                S->Step = Now + 1.0;
                return true;
            case 52:
if (const UVortexAnimInstance* Gait = Cast<UVortexAnimInstance>(Hero->GetMesh()->GetAnimInstance()))
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_HERO_SHOTS %s passo andar %.0f cm/s: clipe %s taxa %.2f"), *Race.ToString(),
                        UVortexAnimInstance::DebugSpeed, *Gait->CurrentClip().ToString(), Gait->CurrentRate());
                FScreenshotRequest::RequestScreenshot(Shot + TEXT("_andar.png"), true, false);
                UVortexAnimInstance::DebugSpeed = 650.f * Hero->GetRaceSpeed();
                S->Phase = 53;
                S->Step = Now + 1.0;
                return true;
            case 53:
if (const UVortexAnimInstance* Gait = Cast<UVortexAnimInstance>(Hero->GetMesh()->GetAnimInstance()))
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_HERO_SHOTS %s passo disparada %.0f cm/s: clipe %s taxa %.2f"), *Race.ToString(),
                        UVortexAnimInstance::DebugSpeed, *Gait->CurrentClip().ToString(), Gait->CurrentRate());
                FScreenshotRequest::RequestScreenshot(Shot + TEXT("_disparada.png"), true, false);
                S->Phase = 54;
                S->Step = Now + .4;
                return true;
            case 54:   // a roda de emotes (B) aberta, apontando a terceira fatia; solta e dança
                UVortexAnimInstance::DebugSpeed = 0.f;
                Controller->SetControlRotation(FRotator(-8.f, Hero->GetActorRotation().Yaw + 150.f, 0.f));
                Hero->DebugEmoteWheel(true, FVector2D(8.f, -4.f));
                S->Phase = 55;
                S->Step = Now + .5;
                return true;
            case 55:
                FScreenshotRequest::RequestScreenshot(Shot + TEXT("_roda.png"), true, false);
                S->Phase = 56;
                S->Step = Now + .3;
                return true;
            case 56:
                Hero->DebugEmoteWheel(false, FVector2D::ZeroVector);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_HERO_SHOTS %s emote %s (tem samba=%d)"), *Race.ToString(), *Hero->GetEmote().ToString(), Hero->HasEmote(TEXT("Danca_Samba")));
                S->Phase = 57;
                S->Step = Now + 2.0;
                return true;
            case 57:
                FScreenshotRequest::RequestScreenshot(Shot + TEXT("_danca.png"), true, false);
                Hero->PlayEmote(NAME_None);
                S->Phase = 6;
                S->Step = Now + .6;
                return true;
            case 6:   // primeira pessoa: só os braços e a arma
                UVortexAnimInstance::DebugSpeed = 0.f;
                Controller->SetControlRotation(FRotator(-10.f, Hero->GetActorRotation().Yaw, 0.f));
                if (!Hero->IsFirstPerson()) Hero->ToggleFirstPerson();
                S->Phase = 7;
                S->Step = Now + 1.0;
                return true;
            case 7:   // primeira pessoa olhando para a frente, depois para baixo (o próprio corpo) e no golpe
                FScreenshotRequest::RequestScreenshot(Shot + TEXT("_1p.png"), true, false);
                S->Phase = 71;
                S->Step = Now + .3;
                return true;
            case 71:
                Controller->SetControlRotation(FRotator(-82.f, Hero->GetActorRotation().Yaw, 0.f));
                S->Phase = 8;
                S->Step = Now + .5;
                return true;
            case 8:
                UE_LOG(LogTemp, Display, TEXT("VORTEX_HERO_SHOTS %s primeira pessoa: corpo visível=%d escondido-para-mim=%d"), *Race.ToString(),
                    Hero->GetMesh()->IsVisible(), Hero->GetMesh()->bOwnerNoSee);
                FScreenshotRequest::RequestScreenshot(Shot + TEXT("_1p_baixo.png"), true, false);
                S->Phase = 81;
                S->Step = Now + .3;
                return true;
            case 81:   // a foto sai no quadro seguinte: só depois volta a olhar para a frente e golpeia
                Controller->SetControlRotation(FRotator(-10.f, Hero->GetActorRotation().Yaw, 0.f));
                if (Hero->GetWeapon()) Hero->GetWeapon()->PressPrimary();
                S->Phase = 9;
                S->Step = Now + .3;
                return true;
            case 9:
                FScreenshotRequest::RequestScreenshot(Shot + TEXT("_1p_golpe.png"), true, false);
                S->Phase = 10;
                S->Step = Now + .6;
                return true;
            default:
                if (Hero->IsFirstPerson()) Hero->ToggleFirstPerson();
                S->Phase = 0;
                ++S->Index;
                S->Step = Now + .8;
                return true;
            }
        }));
    }
}
#endif
