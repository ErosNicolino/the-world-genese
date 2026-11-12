// Teste com fotos do continente (-VortexWorldShots, Tools/RunWorldShots.ps1; Saved/Screenshots/mundo_*.png):
// a emenda do vale exportado com o continente, o centro de cada uma das nove regiões e três rios/vales. Em cada lugar o
// herói espera o chão montar, fica de pé (não cai) e tira a foto; no fim, o mapa (M) do continente.
#if !UE_BUILD_SHIPPING
#include "Framework/VortexCreatureSmoke.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraActor.h"
#include "Framework/VortexGameState.h"
#include "Camera/CameraComponent.h"
#include "Combat/VortexAttributes.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "Framework/VortexHudBase.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Rpg/VortexHeroSheet.h"
#include "UnrealClient.h"
#include "World/VortexLandmarks.h"
#include "World/VortexTerrainData.h"
#include "World/VortexWorldFeatures.h"
#include "Rpg/VortexQuestLog.h"
#include "World/VortexTerrainStreamer.h"

namespace VortexWorldShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FPlace { FString Name; FVector2D At; float Yaw; float Pitch = -14.f; FVector2D Target = FVector2D::ZeroVector; float Lift = 0.f; };
        static const FPlace Fixed[] = {
            { TEXT("emenda_leste"), FVector2D(23000.f, 1500.f), 180.f },
            { TEXT("emenda_oeste"), FVector2D(-92000.f, -2000.f), 0.f },
            { TEXT("valdora"), FVector2D(48000.f, -30000.f), 30.f },
            { TEXT("brasafenda"), FVector2D(-147000.f, -270000.f), 60.f },
            { TEXT("raikor"), FVector2D(78000.f, -255000.f), 0.f },
            { TEXT("nivaluz"), FVector2D(300000.f, -270000.f), 200.f },
            { TEXT("solcanion"), FVector2D(-207000.f, -102000.f), 90.f },
            { TEXT("aurivale"), FVector2D(264000.f, -126000.f), 120.f },
            { TEXT("lodonegro"), FVector2D(-198000.f, 96000.f), 300.f },
            { TEXT("morthuria"), FVector2D(48000.f, 171000.f), 45.f },
            { TEXT("verdalume"), FVector2D(288000.f, 72000.f), 240.f },
            // rios e vales (Tools/World/BakeWorld.py): a câmera olha da margem para a água
            { TEXT("rio_valdora"), FVector2D(101100.f, 47100.f), 0.f },
            { TEXT("vale_raikor"), FVector2D(-43500.f, -308100.f), 0.f },
            { TEXT("rio_morthuria"), FVector2D(49500.f, 98700.f), 0.f },
            // o horizonte (AVortexFarTerrain): as montanhas ao longe, para o norte (-Y)
            { TEXT("horizonte_vulcao"), FVector2D(-160000.f, -180000.f), -75.f, 4.f },
            { TEXT("horizonte_raikor"), FVector2D(60000.f, -120000.f), -90.f, 2.f },
            { TEXT("horizonte_nivaluz"), FVector2D(240000.f, -100000.f), -70.f, 2.f },
            // marcos (AVortexLandmarks): a árvore do mundo, as vilas e um covil
            { TEXT("yggdrasil"), FVector2D(291000.f, 16000.f), 90.f, 12.f },
            // a miragem da árvore vista de longe (1,5 km a oeste)
            { TEXT("miragem_15km"), FVector2D(141000.f, 39000.f), 0.f, 5.f },
            { TEXT("aldeia_elfica"), FVector2D(291000.f, 26500.f), 90.f, -6.f },
            { TEXT("acampamento_dunas"), FVector2D(-218800.f, -98200.f), -90.f, -10.f },
            { TEXT("vila_cacadores"), FVector2D(287000.f, 63900.f), -90.f, -10.f },
            { TEXT("covil_seryth"), FVector2D(-218000.f, -106200.f), -90.f, -16.f },
            // a cachoeira de perto, da beira do lago
            { TEXT("lugar_mirante_perto"), FVector2D(81506.f, 2639.f), 0.f, 0.f, FVector2D(92500.f, 2984.f), 1000.f },
            // Lift < 0: atravessar andando de At até Target por cima da raiz (a Ponte de Raiz e a raiz de rio mais perto)
            { TEXT("lugar_andar_ponte"), FVector2D(82760.f, 48619.f), 0.f, 0.f, FVector2D(77240.f, 45381.f), -1.f },
            // 11/11: a Forja de Bram nova (kit da Quaternius, Tools/Blender/BuildForjaQuaternius.py), vista da frente e de cima
            { TEXT("forja"), FVector2D(-1100.f, 4000.f), 0.f, 0.f, FVector2D(-1830.f, 5630.f), 700.f },
            { TEXT("forja_frente"), FVector2D(-1750.f, 4750.f), 90.f, -6.f },
            // 11/11: a vila maior, com as casas do kit e o chão novo: do alto, a praça da fonte e duas ruas na altura dos olhos
            { TEXT("vila_alto"), FVector2D(-4200.f, -3900.f), 0.f, 0.f, FVector2D(1800.f, 2100.f), 4200.f },
            { TEXT("vila_praca"), FVector2D(700.f, 1300.f), 0.f, 0.f, FVector2D(1800.f, 2100.f), 500.f },
            { TEXT("vila_rua"), FVector2D(2300.f, -600.f), 0.f, -3.f },
            { TEXT("vila_rua_sul"), FVector2D(4500.f, 3300.f), 90.f, -3.f },
            { TEXT("vila_casas"), FVector2D(3000.f, -2900.f), 0.f, 0.f, FVector2D(4300.f, -1500.f), 900.f },
            // 11/11, noite: a saída do portão norte (o largo de terra e a clareira), a placa da padaria e a quina noroeste
            { TEXT("vila_portao_fora"), FVector2D(1800.f, -10500.f), 90.f, -4.f },
            { TEXT("vila_padaria"), FVector2D(4350.f, 2900.f), 0.f, 10.f },
            { TEXT("vila_quina"), FVector2D(-2400.f, -2400.f), 0.f, 0.f, FVector2D(-3900.f, -3900.f), 700.f },
            // 11/11: os bichos (o cercado perto do estábulo e as galinhas de um quintal)
            { TEXT("vila_cercado"), FVector2D(7460.f, 820.f), -90.f, -18.f },
            { TEXT("vila_galinhas"), FVector2D(7345.f, 4650.f), -90.f, -32.f },
        };
        struct FState { int32 Index = 0, Step = 0, Feature = 0, Coins = 0; float Time = 0.f, StartZ = 0.f; bool bPass = true; TArray<FString> Lines; TArray<FPlace> Places; TWeakObjectPtr<AActor> Drone; float MidFeet = -1e9f; bool bMidShot = false; };
        TSharedRef<FState> S = MakeShared<FState>();
        S->Places.Append(Fixed, UE_ARRAY_COUNT(Fixed));
        // 07/11: os lugares marcantes (Tools/World/StampMarvels.py), vistos do lado da cidade (de onde se chega)
        // de uma câmera alta (Lift, cm acima do chão), olhando para o centro: as árvores da borda não tampam
        struct FView { const TCHAR* Id; float Distance, Lift, Side = 0.f; };
        static const FView Views[] = { { TEXT("mirante"), 17000.f, 3000.f }, { TEXT("anel"), 21000.f, 3500.f }, { TEXT("arco"), 14000.f, 2200.f },
            { TEXT("espelho"), 20000.f, 2500.f }, { TEXT("raiz"), 13500.f, 2200.f, 90.f }, { TEXT("cristais"), 15000.f, 3500.f } };
        for (const AVortexLandmarks::FSite& Site : AVortexLandmarks::Sites())
            for (const FView& View : Views)
                if (Site.Id == View.Id)
                {
                    const FVector2D ToCity = (-Site.Center).GetSafeNormal().GetRotated(View.Side);
                    S->Places.Add({ TEXT("lugar_") + Site.Id, Site.Center + ToCity * View.Distance, float(FMath::RadiansToDegrees(FMath::Atan2(-ToCity.Y, -ToCity.X))), 0.f, Site.Center, View.Lift });
                }
        // 07/11: as raízes da Árvore do Mundo mais perto da cidade (world_features.json: mudam quando o mapa é refeito):
        // a de rio de cima e atravessada andando, a que sai do chão de cima
        {
            FString Text;
            TSharedPtr<FJsonObject> Json;
            const TArray<TSharedPtr<FJsonValue>>* Roots = nullptr;
            if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/GodotWorld/world_features.json"))) &&
                FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) && Json.IsValid() && Json->TryGetArrayField(TEXT("roots"), Roots))
            {
                TSharedPtr<FJsonObject> Arch, Emerge;
                auto At = [](const TSharedPtr<FJsonObject>& O) { const TArray<TSharedPtr<FJsonValue>>& A = O->GetArrayField(TEXT("at")); return FVector2D(A[0]->AsNumber(), A[1]->AsNumber()) * 100.f; };
                for (const TSharedPtr<FJsonValue>& V : *Roots)
                {
                    const TSharedPtr<FJsonObject> O = V->AsObject();
                    TSharedPtr<FJsonObject>& Best = O->GetStringField(TEXT("kind")) == TEXT("root_arch") ? Arch : Emerge;
                    if (!Best || At(O).Size() < At(Best).Size()) Best = O;
                }
                if (Arch)
                {
                    const FVector2D A = At(Arch);
                    const float Yaw = FMath::DegreesToRadians(Arch->GetNumberField(TEXT("yaw"))), Scale = Arch->GetNumberField(TEXT("scale"));
                    const FVector2D Across(FMath::Cos(Yaw), FMath::Sin(Yaw)), Along(-Across.Y, Across.X);
                    S->Places.Add({ TEXT("lugar_raiz_rio"), A + Along * 4000.f, 0.f, 0.f, A, 1200.f });
                    S->Places.Add({ TEXT("lugar_andar_raiz_rio"), A - Across * 1700.f * Scale, 0.f, 0.f, A + Across * 1700.f * Scale, -1.f });
                }
                if (Emerge) S->Places.Add({ TEXT("lugar_raiz_chao"), At(Emerge) - FVector2D(0.f, 3500.f), 0.f, 0.f, At(Emerge), 900.f });
            }
        }
        // 07/11: o relevo de cada região visto de cima (câmera alta a 220 m ao sul do centro, olhando para ele; -Only relevo_)
        {
            struct FRelief { const TCHAR* Name; FVector2D At; float Lift; };
            static const FRelief Reliefs[] = { { TEXT("valdora"), FVector2D(48000.f, -30000.f), 5000.f }, { TEXT("brasafenda"), FVector2D(-147000.f, -270000.f), 7000.f },
                { TEXT("raikor"), FVector2D(78000.f, -255000.f), 12000.f }, { TEXT("nivaluz"), FVector2D(300000.f, -270000.f), 8000.f },
                { TEXT("solcanion"), FVector2D(-207000.f, -102000.f), 6000.f }, { TEXT("aurivale"), FVector2D(264000.f, -126000.f), 5000.f },
                { TEXT("lodonegro"), FVector2D(-198000.f, 96000.f), 4000.f }, { TEXT("morthuria"), FVector2D(48000.f, 171000.f), 6000.f },
                { TEXT("verdalume"), FVector2D(288000.f, 72000.f), 6000.f } };
            for (const FRelief& R : Reliefs)
                S->Places.Add({ FString(TEXT("relevo_")) + R.Name, R.At + FVector2D(0.f, 22000.f), 0.f, 0.f, R.At, R.Lift * .6f });
        }
        // -VortexShotsOnly=lugar_: só as fotos com esse começo (e sem a ponte e a cabana)
        FString Only;
        if (FParse::Value(FCommandLine::Get(), TEXT("VortexShotsOnly="), Only))
        {
            S->Places.RemoveAll([&](const FPlace& P) { return !P.Name.StartsWith(Only); });
            S->Feature = 2;
        }
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
            if (!Hero || !Streamer || !Streamer->IsReady() || !Data || !Hero->GetSheet() || !Hero->GetSheet()->IsLoaded()) return true;
            S->Time += Delta;
            const float T = S->Time;
            auto Shot = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/mundo_%s.png"), *Name), true, false); };
            auto Check = [&](bool bOk, const FString& What) { S->Lines.Add(FString::Printf(TEXT("%s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What)); S->bPass &= bOk; };
            // ninguém morre no passeio (lava, criaturas)
            if (UAbilitySystemComponent* Abilities = Hero->GetAbilitySystemComponent())
                Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Hero->GetVitals()->GetMaxHealth());
            if (S->Step == 0)
            {
                Check(Data->HasWorld(), TEXT("continente carregado"));
                if (!Data->HasWorld()) S->Index = S->Places.Num();
                S->Step = 1; S->Time = 0.f;
                return true;
            }
            if (S->Index < S->Places.Num())
            {
                const FPlace& Place = S->Places[S->Index];
                // um ponto em terra perto do centro (o centro pode cair num lago) e sem penhasco em volta: em Raikor o herói
                // posto na beira de um degrau escorregava 20 m e o teste falhava (10/11)
                auto Steep = [&](const FVector2D& P)
                {
                    // 3 m não bastava: o centro de Raikor é um patamar pequeno numa encosta de ~47° (o herói escorregava 26 m)
                    const float Z = Data->QuickGround(P.X, P.Y);
                    for (const float R : { 300.f, 800.f })
                        for (const FVector2D& D : { FVector2D(R, 0.f), FVector2D(-R, 0.f), FVector2D(0.f, R), FVector2D(0.f, -R) })
                            if (FMath::Abs(Data->QuickGround(P.X + D.X, P.Y + D.Y) - Z) > R * .7f) return true;
                    return false;
                };
                FVector2D Spot = Place.At;
                for (int32 Ring = 0; Ring < 60 && Place.Lift >= 0.f && (Data->QuickGround(Spot.X, Spot.Y) < Data->WaterLevel + 30.f || (Place.Lift == 0.f && Steep(Spot))); ++Ring)
                    Spot = Place.At + FVector2D(FMath::Cos(Ring * 2.4f), FMath::Sin(Ring * 2.4f)) * 1500.f * FMath::Sqrt(float(Ring + 1));
                if (S->Step == 1)
                {
                    Hero->GetCharacterMovement()->DisableMovement();
                    Hero->SetActorLocation(FVector(Spot, Data->QuickGround(Spot.X, Spot.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
                    const float Yaw = FVector2D::Distance(Spot, Place.At) > 100.f ? FMath::RadiansToDegrees(FMath::Atan2(Place.At.Y - Spot.Y, Place.At.X - Spot.X)) : Place.Yaw;
                    Controller->SetControlRotation(FRotator(Place.Pitch, Yaw, 0.f));
                    float Ground = 0.f;
                    if ((Streamer->GroundReadyAt(Spot.X, Spot.Y, Ground) && Streamer->PendingChunks() == 0 && T > 1.5f) || T > 40.f)
                    {
                        Hero->SetActorLocation(FVector(Spot, Ground + 110.f), false, nullptr, ETeleportType::TeleportPhysics);
                        Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
                        S->StartZ = Ground;
                        S->Step = 2; S->Time = 0.f;
                    }
                    return true;
                }
                if (S->Step == 2)
                {
                    if (Place.Lift < 0.f)
                    {
                        // anda até Target; no meio do caminho (o meio da raiz) os pés têm que estar bem acima da água
                        const FVector2D To = Place.Target - FVector2D(Hero->GetActorLocation());
                        const FVector2D Middle = (Place.At + Place.Target) * .5f;
                        Controller->SetControlRotation(FRotator(-12.f, float(FMath::RadiansToDegrees(FMath::Atan2(To.Y, To.X))), 0.f));
                        if (To.Size() > 150.f) Hero->AddMovementInput(FVector(To.GetSafeNormal(), 0.f), 1.f);
                        const float Feet = Hero->GetActorLocation().Z - Hero->GetSimpleCollisionHalfHeight();
                        if (FVector2D::Distance(FVector2D(Hero->GetActorLocation()), Middle) < 300.f) S->MidFeet = FMath::Max(S->MidFeet, Feet);
                        if (!S->bMidShot && T > 1.f && FVector2D::Distance(FVector2D(Hero->GetActorLocation()), Middle) < 300.f)
                        {
                            Shot(Place.Name);
                            S->bMidShot = true;
                        }
                        if (To.Size() <= 150.f || T > 45.f)
                        {
                            Check(To.Size() <= 400.f && S->MidFeet > Data->WaterLevel + 150.f, FString::Printf(TEXT("%s: atravessou andando (faltaram %.0f m); no meio, pés a %.1f m, água a %.1f m"),
                                *Place.Name, To.Size() / 100.f, S->MidFeet / 100.f, Data->WaterLevel / 100.f));
                            S->MidFeet = -1e9f; S->bMidShot = false;
                            ++S->Index; S->Step = 1; S->Time = 0.f;
                        }
                        return true;
                    }
                    // o horizonte espera o relevo distante se refazer em volta do lugar novo
                    const float When = Place.Pitch > 0.f || Place.Lift > 0.f ? 6.f : 3.5f;
                    if (Place.Lift > 0.f && !S->Drone.IsValid())
                    {
                        // meio-dia: de manhã a névoa esconde os lugares
                        if (AVortexGameState* State = World->GetGameState<AVortexGameState>()) State->SetDayClock(AVortexGameState::ClockOfHour(13.f), 1);
                        const FVector Eye(Spot, S->StartZ + Place.Lift), Look(Place.Target, Data->QuickGround(Place.Target.X, Place.Target.Y) + 800.f);
                        FActorSpawnParameters Params;
                        Params.ObjectFlags |= RF_Transient;
                        if (ACameraActor* Camera = World->SpawnActor<ACameraActor>(Eye, (Look - Eye).Rotation(), Params))
                        {
                            Camera->GetCameraComponent()->SetConstraintAspectRatio(false);
                            Controller->SetViewTarget(Camera);
                            S->Drone = Camera;
                        }
                    }
                    if (T > When && T - Delta <= When) Shot(Place.Name);
                    if (T > When + .5f)
                    {
                        const float Feet = Hero->GetActorLocation().Z - Hero->GetSimpleCollisionHalfHeight();
                        const int32 Region = Data->RegionAt(Spot.X, Spot.Y);
                        const float Terrain = Data->QuickGround(Spot.X, Spot.Y);
                        Check(FMath::Abs(Feet - S->StartZ) < 250.f,
                            FString::Printf(TEXT("%s: de pé em (%.0f, %.0f) m, chão %.1f m, terreno %.1f m, pés %.1f m (termina em %.0f, %.0f m, terreno lá %.1f m, no chão %d), região %s, nível %d"),
                            *Place.Name, Spot.X / 100.f, Spot.Y / 100.f, S->StartZ / 100.f, Terrain / 100.f, Feet / 100.f, Hero->GetActorLocation().X / 100.f, Hero->GetActorLocation().Y / 100.f,
                            Data->QuickGround(Hero->GetActorLocation().X, Hero->GetActorLocation().Y) / 100.f, Hero->GetCharacterMovement()->IsMovingOnGround() ? 1 : 0, *Data->RegionName(Region), Data->AreaLevel(Spot.X, Spot.Y)));
                        ++S->Index;
                        S->Step = 1; S->Time = 0.f;
                        if (AActor* Camera = S->Drone.Get()) { Controller->SetViewTarget(Hero); Camera->Destroy(); }
                    }
                    return true;
                }
            }
            // uma ponte (de pé no meio, por cima da água) e uma cabana (abre o baú)
            if (S->Feature < 2)
            {
                const FVortexTerrainData::FFeature* Pick = nullptr;
                for (const FVortexTerrainData::FFeature& F : Data->Features())
                    if (F.bBridge == (S->Feature == 0) && (!Pick || FVector2D::Distance(F.At, FVector2D::ZeroVector) < FVector2D::Distance(Pick->At, FVector2D::ZeroVector))) Pick = &F;
                if (!Pick) { ++S->Feature; return true; }
                AVortexWorldFeatures* Features = AVortexWorldFeatures::Find(World);
                const FVector2D Across(FMath::Cos(Pick->Radians), -FMath::Sin(Pick->Radians));
                if (S->Step == 1)
                {
                    // perto dela primeiro (a ponte/cabana monta a 200 m), depois em cima
                    Hero->GetCharacterMovement()->DisableMovement();
                    const FVector2D Near = Pick->At + FVector2D(-Across.Y, Across.X) * 1500.f;
                    Hero->SetActorLocation(FVector(Near, Data->QuickGround(Near.X, Near.Y) + 300.f), false, nullptr, ETeleportType::TeleportPhysics);
                    float Ground = 0.f;
                    if ((Features && Features->BuiltCount() > 0 && Streamer->GroundReadyAt(Pick->At.X, Pick->At.Y, Ground) && T > 2.f) || T > 40.f) { S->Step = 2; S->Time = 0.f; }
                    return true;
                }
                if (S->Feature == 0)
                {
                    if (S->Step == 2)
                    {
                        Hero->SetActorLocation(FVector(Pick->At, Data->WaterLevel + 600.f), false, nullptr, ETeleportType::TeleportPhysics);
                        Hero->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
                        Controller->SetControlRotation(FRotator(-18.f, Pick->Yaw + 90.f, 0.f));
                        S->Step = 3; S->Time = 0.f;
                        return true;
                    }
                    if (T > 2.5f && T - Delta <= 2.5f) Shot(TEXT("ponte"));
                    if (T > 3.f)
                    {
                        const float Feet = Hero->GetActorLocation().Z - Hero->GetSimpleCollisionHalfHeight();
                        Check(Feet > Data->WaterLevel + 40.f && Hero->GetCharacterMovement()->IsMovingOnGround(), FString::Printf(TEXT("de pé na ponte (%.0f, %.0f) m: pés a %.1f m, água a %.1f m"), Pick->At.X / 100.f, Pick->At.Y / 100.f, Feet / 100.f, Data->WaterLevel / 100.f));
                        ++S->Feature; S->Step = 1; S->Time = 0.f;
                    }
                    return true;
                }
                // cabana: entra pela porta, abre o baú
                if (S->Step == 2)
                {
                    const FVector2D Door = Pick->At + FVector2D(FMath::Sin(Pick->Radians), FMath::Cos(Pick->Radians)) * -100.f;
                    Hero->SetActorLocation(FVector(Door, Data->GroundAt(Pick->At.X, Pick->At.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
                    Hero->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
                    S->Coins = Hero->GetSheet()->GetCoins();
                    S->Step = 3; S->Time = 0.f;
                    return true;
                }
                const AVortexWorldFeatures::FChest* Chest = Features ? Features->NearestChest(Hero->GetActorLocation(), 600.f, nullptr) : nullptr;
                if (Chest && T < 1.5f) Controller->SetControlRotation(FRotator(-25.f, (Chest->Location - Hero->GetActorLocation()).Rotation().Yaw, 0.f));
                if (T > 1.4f && T - Delta <= 1.4f) Shot(TEXT("cabana"));
                if (T > 1.6f && T - Delta <= 1.6f && Chest)
                {
                    Hero->SetActorLocation(Chest->Location + (Hero->GetActorLocation() - Chest->Location).GetSafeNormal2D() * 120.f + FVector(0.f, 0.f, 60.f), false, nullptr, ETeleportType::TeleportPhysics);
                    Hero->DebugOpenChest(Chest->Id);
                    Features->PlayOpen(Chest->Id);
                }
                if (T > 2.6f && T - Delta <= 2.6f) Shot(TEXT("bau"));
                if (T > 3.f)
                {
                    const FName Id = FName(*FString::Printf(TEXT("chest:%d"), Pick->Index));
                    Check(Chest != nullptr, TEXT("baú dentro da cabana"));
                    Check(Hero->GetQuestLog()->HasFlag(Id), FString::Printf(TEXT("baú %s aberto e guardado no save"), *Id.ToString()));
                    Check(Hero->GetSheet()->GetCoins() > S->Coins, FString::Printf(TEXT("moedas do baú (%d -> %d)"), S->Coins, Hero->GetSheet()->GetCoins()));
                    ++S->Feature; S->Step = 1; S->Time = 0.f;
                }
                return true;
            }
            // o mapa do continente
            AVortexHudBase* Hud = Cast<AVortexHudBase>(Controller->GetHUD());
            if (S->Step == 1 && Hud)
            {
                if (!Hud->IsWorldMapOpen() && T < 1.f) Hud->ToggleWorldMap();
                if (T > 2.f && T - Delta <= 2.f) Shot(TEXT("mapa"));
                if (T > 2.4f)
                {
                    Check(Hud->ExploredFraction() > 0.f, FString::Printf(TEXT("mapa do continente, explorado %.1f%%"), Hud->ExploredFraction() * 100.f));
                    Hud->ToggleWorldMap();
                    S->Step = 3;
                }
                return true;
            }
            for (const FString& Line : S->Lines) UE_LOG(LogTemp, Display, TEXT("VORTEX_WORLD_SHOTS %s"), *Line);
            UE_LOG(LogTemp, Display, TEXT("VORTEX_WORLD_SHOTS_%s"), S->bPass ? TEXT("PASS") : TEXT("FAIL"));
            FPlatformMisc::RequestExit(false);
            return false;
        }));
    }
}
#endif
