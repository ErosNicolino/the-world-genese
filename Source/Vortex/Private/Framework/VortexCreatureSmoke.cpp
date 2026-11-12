// Teste das criaturas do Godot e das missões (-VortexCreatureSmoke, Tools/RunCreatureSmoke.ps1).
// Em L_TerrainProto: faz nascer cada espécie perto do herói, fotografa (Saved/Screenshots/criatura_*.png),
// confere que aparece, anima, ataca (as hostis), morre e dá XP; depois percorre o começo da linha
// principal: "O Despertado" (falar com a Rhea), "Recruta do Vale" (3 gotas de jade) e as entregas.
#if !UE_BUILD_SHIPPING
#include "Framework/VortexCreatureSmoke.h"
#include "World/VortexItemDrop.h"

#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexCombatComponent.h"
#include "Containers/Ticker.h"
#include "Creatures/VortexMonster.h"
#include "Components/StaticMeshComponent.h"
#include "Creatures/VortexNpc.h"
#include "EngineUtils.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexRpg.h"
#include "Rpg/VortexQuestLog.h"
#include "UnrealClient.h"
#include "World/VortexCreatureSubsystem.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexLandmarks.h"
#include "World/VortexWorldFeatures.h"
#include "World/VortexPointsOfInterest.h"
#include "World/VortexPortal.h"
#include "World/VortexTerrainData.h"
#include "World/VortexWildlife.h"
#include "World/VortexForestLife.h"
#include "Framework/VortexAudio.h"
#include "Combat/VortexProjectile.h"
#include "Camera/CameraActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Framework/VortexHudBase.h"
#include "Framework/VortexDialogue.h"
#include "Framework/VortexHeroWindow.h"
#include "Framework/VortexShopWindow.h"
#include "Rpg/VortexShop.h"
#include "Framework/VortexPlayerSettings.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Framework/VortexIntroComic.h"

namespace VortexCreatureSmoke
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState
        {
            int32 Step = 0, Index = 0, Hits = 0;
            float Time = 0.f, HeroBefore = 0.f;
            int64 XpBefore = 0;
            TArray<FName> Species;
            TWeakObjectPtr<AVortexMonster> Monster;
            bool bPass = true;
            TArray<FString> Lines;
        };
        TSharedRef<FState> S = MakeShared<FState>();
        FString List = TEXT("Slime,Mushroom,Golem,Bat,Frog,Goblin,Rabbit,Deer,Horse_Palomino,GlassScorpion,RootTitan,RuneColossus");
        FParse::Value(FCommandLine::Get(), TEXT("VortexCreatureSmoke="), List, false);
        TArray<FString> Parts;
        List.ParseIntoArray(Parts, TEXT(","));
        for (const FString& Part : Parts) S->Species.Add(FName(*Part));
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
            UAbilitySystemComponent* Abilities = Hero->GetAbilitySystemComponent();
            UVortexHeroSheet* Sheet = Hero->GetSheet();
            UVortexQuestLog* Log = Hero->GetQuestLog();
            UVortexCreatureSubsystem* Creatures = World->GetSubsystem<UVortexCreatureSubsystem>();
            S->Time += Delta;
            auto Finish = [&]()
            {
                for (const FString& Line : S->Lines) UE_LOG(LogTemp, Display, TEXT("VORTEX_CREATURE_SMOKE %s"), *Line);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_CREATURE_SMOKE_%s"), S->bPass ? TEXT("PASS") : TEXT("FAIL"));
                FPlatformMisc::RequestExit(false);
            };
            auto Check = [&](bool bOk, const FString& What) { S->Lines.Add(FString::Printf(TEXT("%s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What)); S->bPass &= bOk; };
            const FVector2D Open(-30000.f, -8000.f);
            switch (S->Step)
            {
            case 0:
            {
                float Ground = 0.f;
                Hero->SetActorLocation(FVector(Open, Streamer->GroundAt(Open.X, Open.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
                if (!Streamer->GroundReadyAt(Open.X, Open.Y, Ground) || Streamer->PendingChunks() > 0 || S->Time < 3.f) return true;
                S->Step = 1; S->Time = 0.f;
                return true;
            }
            case 1:   // uma espécie: nasce a 7 m (chefes a 16 m), olha para ela
            {
                if (!S->Species.IsValidIndex(S->Index)) { S->Step = 10; S->Time = 0.f; return true; }
                const FName Species = S->Species[S->Index];
                const bool bBoss = Species == TEXT("GlassScorpion") || Species == TEXT("RootTitan") || Species == TEXT("RuneColossus");
                Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Hero->GetVitals()->GetMaxHealth());
                const FVector Forward = Controller->GetControlRotation().Vector().GetSafeNormal2D();
                const FVector At = Hero->GetActorLocation() + Forward * (bBoss ? 1600.f : 700.f);
                const int32 Before = Creatures ? Creatures->PostCount() : 0;
                if (Creatures) Creatures->DebugSpawnNear(Species, At, 3);
                AVortexMonster* Found = nullptr;
                for (TActorIterator<AVortexMonster> It(World); It; ++It) if (It->GetAssetName() == Species && !It->IsCreatureDead()) Found = *It;
                Check(Found != nullptr && Creatures && Creatures->PostCount() == Before + 1, FString::Printf(TEXT("%s nasceu"), *Species.ToString()));
                S->Monster = Found;
                S->HeroBefore = Hero->GetVitals()->GetHealth();
                S->Step = Found ? 2 : 1;
                if (!Found) ++S->Index;
                S->Time = 0.f;
                return true;
            }
            case 2:   // 4 s vivendo: aparece no chão, anima, e as hostis atacam
            {
                AVortexMonster* M = S->Monster.Get();
                if (!M) { Check(false, TEXT("sumiu antes do teste")); S->Step = 1; ++S->Index; return true; }
                const FVector To = M->GetActorLocation() - Hero->GetActorLocation();
                Controller->SetControlRotation(FRotator(-12.f, To.Rotation().Yaw, 0.f));
                // 08/11, -VortexSlimeFace: câmera de frente para o slime (o herói o tampava) e um tapinha para a cara de dor
                if (M->IsSlime() && FParse::Param(FCommandLine::Get(), TEXT("VortexSlimeFace")))
                {
                    static TWeakObjectPtr<ACameraActor> FaceCam;
                    if (!FaceCam.IsValid()) { FaceCam = World->SpawnActor<ACameraActor>(); Controller->SetViewTargetWithBlend(FaceCam.Get(), 0.f); }
                    const FVector Body = M->GetActorLocation();
                    const FVector Eye = Body + M->GetActorForwardVector() * 175.f + M->GetActorRightVector() * 45.f + FVector(0.f, 0.f, 25.f);
                    FaceCam->SetActorLocationAndRotation(Eye, (Body + FVector(0.f, 0.f, 8.f) - Eye).Rotation());
                    auto Past = [&](float T) { return S->Time > T && S->Time - Delta <= T; };
                    auto Face = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/criatura_slime_%s.png"), Name), false, false); };
                    if (Past(1.f)) Face(TEXT("a"));
                    if (Past(2.2f)) Face(TEXT("b"));
                    if (Past(2.7f)) Face(TEXT("c"));
                    if (Past(3.5f)) Hero->GetCombat()->ApplyHit(M, 1.f);
                    if (Past(3.95f)) Face(TEXT("dor"));
                }
                if (S->Time > 1.6f && S->Time - Delta <= 1.6f)
                {
                    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/criatura_%s.png"), *M->GetAssetName().ToString()), true, false);
                }
                // de perto, quando ela já chegou e golpeia (o bote do slime estica a gelatina)
                if (S->Time > 3.1f && S->Time - Delta <= 3.1f)
                {
                    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/criatura_%s_perto.png"), *M->GetAssetName().ToString()), true, false);
                }
                if (S->Time < 4.5f) return true;
                // o slime novo não tem esqueleto: o corpo é a malha de gelatina (SlimeBody)
                const UStaticMeshComponent* Jelly = M->IsSlime() ? M->FindComponentByClass<UStaticMeshComponent>() : nullptr;
                const bool bVisible = !M->IsHidden() && ((M->GetMesh() && M->GetMesh()->GetSkeletalMeshAsset()) || (Jelly && Jelly->GetStaticMesh() && Jelly->IsVisible()));
                Check(bVisible, FString::Printf(TEXT("%s visível (malha %s)"), *M->GetAssetName().ToString(), M->GetMesh() && M->GetMesh()->GetSkeletalMeshAsset() ? *M->GetMesh()->GetSkeletalMeshAsset()->GetName() : TEXT("nenhuma")));
                if (!M->IsPassive())
                {
                    const bool bAttacked = Hero->GetVitals()->GetHealth() < S->HeroBefore || M->IsEngaged();
                    Check(bAttacked, FString::Printf(TEXT("%s percebeu o herói (vida %.0f -> %.0f, luta=%d)"), *M->GetAssetName().ToString(),
                        S->HeroBefore, Hero->GetVitals()->GetHealth(), M->IsEngaged() ? 1 : 0));
                }
                // quem voa (morcego) tem que ficar ao alcance da espada: o centro dele até ~2 m acima dos pés do herói
                if (M->GetAssetName() == TEXT("Bat"))
                {
                    const float Above = M->GetActorLocation().Z - (Hero->GetActorLocation().Z - AVortexCharacter::HeroCapsuleHalfHeight);
                    Check(Above > 40.f && Above < 200.f, FString::Printf(TEXT("Bat voando na altura certa (%.0f cm acima dos pés do herói)"), Above));
                }
                S->XpBefore = Sheet->GetXp();
                S->Step = 3; S->Time = 0.f; S->Hits = 0;
                return true;
            }
            case 3:   // vencer: golpes até cair, e a ficha recebe o XP
            {
                AVortexMonster* M = S->Monster.Get();
                Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Hero->GetVitals()->GetMaxHealth());
                if (M && !M->IsCreatureDead() && S->Hits < 60)
                {
                    Hero->GetCombat()->ApplyHit(M, 200.f);
                    ++S->Hits;
                    return true;
                }
                if (S->Time < 1.f) return true;
                const FName Species = S->Species[S->Index];
                Check(M && M->IsCreatureDead(), FString::Printf(TEXT("%s vencido em %d golpes"), *Species.ToString(), S->Hits));
                Check(Sheet->GetXp() > S->XpBefore || Species.ToString().StartsWith(TEXT("Horse")), FString::Printf(TEXT("%s deu XP (%lld -> %lld)"), *Species.ToString(), S->XpBefore, Sheet->GetXp()));
                if (M) M->Destroy();
                ++S->Index;
                S->Step = 1; S->Time = 0.f;
                return true;
            }
            case 10:  // a linha principal começa sozinha ao sair do Estádio (herói de teste: já fora)
            {
                if (S->Time < 1.5f) return true;
                Check(Log && Log->IsActive(TEXT("v_despertar")), TEXT("missão automática O Despertado"));
                // ir até a Rhea (na praça): o posto aparece quando o herói chega perto
                Hero->SetActorLocation(FVector(-450.f + 150.f, -250.f, Streamer->GroundAt(-300.f, -250.f) + 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                S->Step = 11; S->Time = 0.f;
                return true;
            }
            case 11:
            {
                AVortexNpc* Rhea = nullptr;
                for (TActorIterator<AVortexNpc> It(World); It; ++It) if (It->GetNpcId() == TEXT("rhea")) Rhea = *It;
                if (!Rhea && S->Time < 15.f) return true;
                Check(Rhea != nullptr, TEXT("a Capitã Rhea apareceu na praça"));
                if (!Rhea || !Log) { Finish(); return false; }
                Hero->SetActorLocation(Rhea->GetActorLocation() + FVector(150.f, 0.f, 20.f), false, nullptr, ETeleportType::TeleportPhysics);
                S->Step = 12; S->Time = 0.f;
                return true;
            }
            case 12:
            {
                if (S->Time < .5f) return true;
                Log->ServerTalk_Implementation(TEXT("rhea"), NAME_None);
                const FVortexQuestDef* First = FVortexStoryCatalog::Get().Quest(TEXT("v_despertar"));
                Check(First && Log->IsReady(*First), TEXT("falar com a Rhea cumpre O Despertado"));
                const int64 Xp = Sheet->GetXp();
                Log->ServerTurnIn_Implementation(TEXT("v_despertar"));
                Check(Log->IsDone(TEXT("v_despertar")) && Sheet->GetXp() > Xp, TEXT("O Despertado entregue (XP)"));
                const FVortexQuestDef* Recruit = FVortexStoryCatalog::Get().Quest(TEXT("rhea_recruta"));
                Check(Recruit && Log->IsAvailable(*Recruit), TEXT("Recruta do Vale liberada"));
                Log->ServerAccept_Implementation(TEXT("rhea_recruta"));
                Check(Log->IsActive(TEXT("rhea_recruta")), TEXT("Recruta do Vale aceita"));
                // as gotas: três vencidas (perto do herói conta para ele)
                for (int32 i = 0; i < 3; ++i)
                {
                    if (Creatures) Creatures->DebugSpawnNear(TEXT("Slime"), Hero->GetActorLocation() + FVector(400.f + i * 150.f, 1200.f, 0.f), 2);
                }
                S->Step = 13; S->Time = 0.f;
                return true;
            }
            case 13:
            {
                if (S->Time < 1.f) return true;
                for (TActorIterator<AVortexMonster> It(World); It; ++It)
                {
                    if (It->GetSpecies() == TEXT("Slime") && !It->IsCreatureDead()) for (int32 i = 0; i < 40 && !It->IsCreatureDead(); ++i) Hero->GetCombat()->ApplyHit(*It, 200.f);
                }
                const FVortexQuestDef* Recruit = FVortexStoryCatalog::Get().Quest(TEXT("rhea_recruta"));
                Check(Recruit && Log->IsReady(*Recruit), FString::Printf(TEXT("3 gotas vencidas (%s)"), Recruit ? *Log->ObjectiveLines(*Recruit).Replace(TEXT("\n"), TEXT(" | ")) : TEXT("")));
                const int32 Coins = Sheet->GetCoins();
                Log->ServerTurnIn_Implementation(TEXT("rhea_recruta"));
                Check(Log->IsDone(TEXT("rhea_recruta")) && Sheet->GetCoins() > Coins, TEXT("Recruta do Vale entregue (moedas)"));
                // fotos da interface: conversa com a Rhea, janela do herói (Classes, Alquimia, Missões) e o mapa
                AVortexNpc* Rhea = nullptr;
                for (TActorIterator<AVortexNpc> It(World); It; ++It) if (It->GetNpcId() == TEXT("rhea")) Rhea = *It;
                if (Rhea) VortexDialogue::Open(Hero, Rhea);
                S->Step = 14; S->Time = 0.f;
                return true;
            }
            case 14:
            {
                auto Ui = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/ui_%s.png"), Name), true, false); };
                const float T = S->Time;
                auto At = [&](float When) { return T >= When && T - Delta < When; };
                if (At(2.5f)) Ui(TEXT("dialogo"));
                if (At(3.f)) { VortexDialogue::Close(); VortexHeroWindow::Open(Hero, 3); }
                if (At(4.f)) Ui(TEXT("classes"));
                if (At(4.5f)) { VortexHeroWindow::Close(); VortexHeroWindow::Open(Hero, 4); }
                if (At(5.5f)) Ui(TEXT("alquimia"));
                if (At(6.f)) { VortexHeroWindow::Close(); VortexHeroWindow::Open(Hero, 5); }
                if (At(7.f)) Ui(TEXT("missoes"));
                // 07/11: a página das técnicas (o combo e o poder do botão direito)
                if (At(7.1f)) { VortexHeroWindow::Close(); VortexHeroWindow::Open(Hero, 2); }
                if (At(7.4f)) Ui(TEXT("tecnicas"));
                if (At(7.5f)) { VortexHeroWindow::Close(); VortexHeroWindow::Open(Hero, 0); }
                if (At(8.5f)) Ui(TEXT("heroi"));
                // arsenal: uma arma com elemento e peculiaridade e duas peças de um conjunto vestidas
                if (At(8.6f))
                {
                    VortexHeroWindow::Close();
                    Sheet->GiveItem(TEXT("espada_de_cinzas_poison_bleed_rare"), 1, false);
                    for (const TCHAR* Piece : { TEXT("dunas_head"), TEXT("dunas_chest"), TEXT("dunas_legs") }) Sheet->GiveItem(Piece, 1, false);
                    for (const TCHAR* Piece : { TEXT("dunas_head"), TEXT("dunas_chest") })
                    {
                        const int32 Index = Sheet->GetBag().IndexOfByPredicate([Piece](const auto& Stack) { return Stack.Item == FName(Piece); });
                        if (Index != INDEX_NONE) Sheet->ServerUseSlot_Implementation(uint8(Index));
                    }
                    VortexHeroWindow::Open(Hero, 1);
                }
                auto SelectBag = [&](const TCHAR* Item)
                {
                    VortexHeroWindow::SelectForTest(Sheet->GetBag().IndexOfByPredicate([Item](const auto& Stack) { return Stack.Item == FName(Item); }), INDEX_NONE);
                };
                if (At(8.8f)) SelectBag(TEXT("espada_de_cinzas_poison_bleed_rare"));
                if (At(9.3f)) Ui(TEXT("item_elemento"));
                if (At(9.4f)) SelectBag(TEXT("dunas_legs"));
                if (At(9.9f)) Ui(TEXT("item_conjunto"));
                if (At(10.f))
                {
                    VortexHeroWindow::Close();
                    if (AVortexHudBase* Hud = Cast<AVortexHudBase>(Controller->GetHUD())) Hud->ToggleWorldMap();
                }
                if (At(11.f)) Ui(TEXT("mapa"));
                if (At(11.5f))
                {
                    if (AVortexHudBase* Hud = Cast<AVortexHudBase>(Controller->GetHUD())) Hud->ToggleWorldMap();
                    if (Creatures) Creatures->DebugSpawnNear(TEXT("Golem"), Hero->GetActorLocation() + Controller->GetControlRotation().Vector().GetSafeNormal2D() * 900.f, 4);
                }
                if (At(14.f)) Ui(TEXT("alvo"));
                if (T > 15.f) { Finish(); return false; }
                return true;
            }
            default: return false;
            }
        }));
    }
}

namespace VortexWeaponCloseUp
{
    // -VortexWeaponCloseUp[=tipo,...]: cada arma de perto, parada, no meio do golpe e no especial
    // (Saved/Screenshots/perto_<tipo>_<fase>.png), com uma câmera ao lado do herói.
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState { int32 Index = 0, Step = 0; float Time = 0.f, Side = 1.f; TArray<FName> Types; TWeakObjectPtr<ACameraActor> Camera; FVector Spot = FVector::ZeroVector; };
        TSharedRef<FState> S = MakeShared<FState>();
        FString List;
        if (!FParse::Value(FCommandLine::Get(), TEXT("VortexWeaponCloseUp="), List, false))
        {
            FString Text;
            TSharedPtr<FJsonObject> Json;
            if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Rpg/weapons.json"))) &&
                FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) && Json)
            {
                for (const TSharedPtr<FJsonValue>& V : Json->GetArrayField(TEXT("types"))) S->Types.Add(FName(*V->AsObject()->GetStringField(TEXT("id"))));
            }
        }
        else
        {
            TArray<FString> Parts;
            List.ParseIntoArray(Parts, TEXT(","));
            for (const FString& Part : Parts) S->Types.Add(FName(*Part));
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
            if (!Hero || !Streamer || !Streamer->IsReady() || !Hero->GetSheet() || !Hero->GetSheet()->IsLoaded()) return true;
            S->Time += Delta;
            const FVector2D Open(-30000.f, -8000.f);
            auto Shot = [&](const TCHAR* Phase)
            {
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/perto_%s_%s.png"), *S->Types[S->Index].ToString().Replace(TEXT(":"), TEXT("_")), Phase), false, false);
            };
            UVortexWeaponComponent* Weapon = Hero->GetWeapon();
            if (S->Step == 0)
            {
                float Ground = 0.f;
                Hero->SetActorLocation(FVector(Open, Streamer->GroundAt(Open.X, Open.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
                if (!Streamer->GroundReadyAt(Open.X, Open.Y, Ground) || Streamer->PendingChunks() > 0 || S->Time < 3.f) return true;
                Controller->SetControlRotation(FRotator(0.f, 0.f, 0.f));
                Hero->SetActorRotation(FRotator(0.f, 0.f, 0.f));
                S->Spot = Hero->GetActorLocation();
                FActorSpawnParameters Params;
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                S->Camera = World->SpawnActor<ACameraActor>(Params);
                UVortexPlayerSettings::Get()->bShowHud = false;   // só nesta sessão (não grava)
                Controller->SetViewTargetWithBlend(S->Camera.Get(), 0.f);
                S->Step = 1; S->Time = 0.f;
                return true;
            }
            if (S->Step == 1)
            {
                if (!S->Types.IsValidIndex(S->Index))
                {
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_CLOSEUP_DONE %d"), S->Types.Num());
                    FPlatformMisc::RequestExit(false);
                    return false;
                }
                // 07/11: "item:<id>" equipa a espada da ficha (o modelo de cada raridade: VortexWeaponComponent)
                FString Entry = S->Types[S->Index].ToString();
                const bool bItem = Entry.RemoveFromStart(TEXT("item:"));
                // 07/11: "set:<conjunto>" veste as 4 peças da armadura (Data/Rpg/armors.json) com a arma inicial
                const bool bSet = !bItem && Entry.RemoveFromStart(TEXT("set:"));
                // o tipo da arma do item (a família; o item inicial é o próprio tipo): antes ia sempre a espada, e do 2º
                // arco em diante a mão ficava com ela (a ficha já estava no arco e não trocava)
                FName ItemType = NAME_None;
                if (bItem)
                    if (const FVortexItemDef* ItemDef = FVortexRpgCatalog::Get().Item(FName(*Entry)))
                        ItemType = ItemDef->Family.IsEmpty() ? FName(*Entry) : FName(*ItemDef->Family);
                UVortexWeaponDefinition* Def = UVortexWeaponComponent::LoadType(bItem ? ItemType : bSet ? FName(TEXT("espada_de_cinzas")) : S->Types[S->Index]);
                if (bItem && !Def) Def = UVortexWeaponComponent::LoadType(NAME_None);
                if (!Def || !Weapon || !Weapon->Equip(Def)) { ++S->Index; return true; }
                if (bSet)
                {
                    UVortexHeroSheet* Sheet = Hero->GetSheet();
                    if (Sheet->GetLevel() < 100) Sheet->AddXp(FVortexRpgCatalog::XpForLevel(100) - Sheet->GetXp());
                    for (const TCHAR* Slot : { TEXT("head"), TEXT("chest"), TEXT("legs"), TEXT("boots") })
                    {
                        const FName Piece(*FString::Printf(TEXT("arm_%s_%s"), *Entry, Slot));
                        Sheet->GiveItem(Piece, 1, false);
                        const int32 Index = Sheet->GetBag().IndexOfByPredicate([Piece](const auto& Stack) { return Stack.Item == Piece; });
                        if (Index != INDEX_NONE) Sheet->ServerUseSlot_Implementation(uint8(Index));
                    }
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_CLOSEUP_SET %s"), *Entry);
                }
                if (bItem)
                {
                    UVortexHeroSheet* Sheet = Hero->GetSheet();
                    // 07/11: o item trava pelo nível: o herói das fotos vai ao 100 (senão as raras ficavam na mochila)
                    if (Sheet->GetLevel() < 100) Sheet->AddXp(FVortexRpgCatalog::XpForLevel(100) - Sheet->GetXp());
                    // a mochila vazia a cada arma (a anterior voltava para ela e, lá pela 21ª, não cabia mais nenhuma)
                    for (const FVortexItemStack& Stack : TArray<FVortexItemStack>(Sheet->GetBag()))
                        if (!Stack.IsEmpty()) Sheet->TakeItem(Stack.Item, Stack.Count);
                    Sheet->GiveItem(FName(*Entry), 1, false);
                    const int32 Slot = Sheet->GetBag().IndexOfByPredicate([&Entry](const auto& Stack) { return Stack.Item == FName(*Entry); });
                    if (Slot != INDEX_NONE) Sheet->ServerUseSlot_Implementation(uint8(Slot));
                    Weapon->RefreshVisual();
                }
                // cada arma começa no mesmo lugar (o avanço das adagas e o gancho do chicote levam o herói)
                if (!S->Spot.IsZero())
                {
                    Hero->GetCharacterMovement()->StopMovementImmediately();
                    Hero->SetActorLocation(S->Spot, false, nullptr, ETeleportType::TeleportPhysics);
                    Hero->SetActorRotation(FRotator::ZeroRotator);
                    Controller->SetControlRotation(FRotator::ZeroRotator);
                }
                // cada arma começa com o secundário pronto e a mana cheia
                Weapon->DebugResetSpecial();
                if (UAbilitySystemComponent* ASC = Hero->GetAbilitySystemComponent())
                    ASC->SetNumericAttributeBase(UVortexAttributes::GetManaAttribute(), ASC->GetNumericAttribute(UVortexAttributes::GetMaxManaAttribute()));
                S->Step = 2; S->Time = 0.f;
                return true;
            }
            // 08/11, -VortexBowFirstPerson: a conferência do arco em primeira pessoa (parado, puxando, olhando para baixo, solta)
            if (S->Step >= 2 && FParse::Param(FCommandLine::Get(), TEXT("VortexBowFirstPerson")))
            {
                const float Was = S->Time - Delta;
                auto Past = [&](float T) { return S->Time >= T && Was < T; };
                if (Past(.05f)) { Controller->SetViewTargetWithBlend(Hero, 0.f); if (!Hero->IsFirstPerson()) Hero->ToggleFirstPerson(); }
                if (Past(2.f)) Shot(TEXT("1p_parado"));
                if (Past(2.3f)) Weapon->PressPrimary();
                if (Past(3.4f)) Shot(TEXT("1p_puxando"));
                if (Past(3.7f)) Controller->SetControlRotation(FRotator(-60.f, 0.f, 0.f));
                if (Past(4.3f)) Shot(TEXT("1p_baixo"));
                if (Past(4.6f)) { Controller->SetControlRotation(FRotator::ZeroRotator); Weapon->ReleasePrimary(); }
                if (Past(4.72f)) Shot(TEXT("1p_solta"));
                if (Past(4.9f))
                {
                    // os itens no chão em blocos: uma fileira à frente
                    Controller->SetControlRotation(FRotator(-16.f, 0.f, 0.f));
                    static const TCHAR* Drops[] = { TEXT("potion_red"), TEXT("ring_jade"), TEXT("azhur_core"), TEXT("mushroom_cap"), TEXT("juramento_celeste"), TEXT("bread"), TEXT("meat"), TEXT("azhur_core"),
                        TEXT("ring_jade"), TEXT("obsidian_fragment"), TEXT("iron_scrap"), TEXT("bat_wing"), TEXT("bandit_bandana"), TEXT("deer_antler"), TEXT("shield_wood"), TEXT("frutas_silvestres") };
                    for (int32 i = 0; i < 5; ++i)
                        AVortexItemDrop::Spawn(World, Drops[i], 1, Hero->GetActorLocation() + FVector(260.f, (i - 2) * 130.f, 0.f), 0.f);
                }
                if (Past(7.f)) Shot(TEXT("1p_itens"));
                if (S->Time > 7.6f) { Hero->ToggleFirstPerson(); Controller->SetViewTargetWithBlend(S->Camera.Get(), 0.f); ++S->Index; S->Step = 1; S->Time = 0.f; }
                return true;
            }
            // câmera: à direita do herói (a mão da arma), na altura do peito, um pouco à frente
            if (ACameraActor* Camera = S->Camera.Get())
            {
                const FVector Chest = Hero->GetActorLocation() + FVector(0.f, 0.f, 5.f);
                const FVector Eye = Chest + Hero->GetActorRightVector() * 115.f * S->Side + Hero->GetActorForwardVector() * 85.f + FVector(0.f, 0.f, 15.f);
                Camera->SetActorLocationAndRotation(Eye, (Chest - Eye).Rotation());
                // 07/11: durante o poder, de lado e a meia distância (a animação e o efeito inteiros)
                if (S->Step == 6 || (S->Step == 7 && S->Time < .6f))
                {
                    const FVector Side = Chest + Hero->GetActorRightVector() * 430.f + Hero->GetActorForwardVector() * 140.f + FVector(0.f, 0.f, 120.f);
                    Camera->SetActorLocationAndRotation(Side, (Chest + Hero->GetActorForwardVector() * 160.f - Side).Rotation());
                }
                else if (S->Step == 7 || (S->Step == 5 && S->Time > .6f && S->Time < 1.f))
                {
                    // depois do secundário: de trás e de cima, olhando para onde ele caiu (uns 4 m à frente)
                    const FVector Back = Chest - Hero->GetActorForwardVector() * 320.f + FVector(0.f, 0.f, 200.f);
                    Camera->SetActorLocationAndRotation(Back, (Chest + Hero->GetActorForwardVector() * 380.f - Back).Rotation());
                }
            }
            const UVortexWeaponDefinition* Def = Weapon ? Weapon->GetDefinition() : nullptr;
            const float Strike = Def && Def->Swings.Num() ? FMath::Max(.12f, Def->Swings[0].ActiveStart) : .25f;
            switch (S->Step)
            {
            case 2: if (S->Time > .8f) { Shot(TEXT("pose")); S->Side = -1.f; S->Step = 8; S->Time = 0.f; } return true;
            case 8:
                if (S->Time > .2f)
                {
                    Shot(TEXT("esquerda")); S->Side = 1.f; S->Time = 0.f;
                    // na primeira arma: bebe uma poção (foto no meio dos goles)
                    if (S->Index == 0 && Hero->GetSheet()) { Hero->GetSheet()->ServerQuickUse_Implementation(TEXT("potion_red")); S->Step = 12; }
                    else S->Step = 3;
                }
                return true;
            case 12:
                if (S->Time > .75f && S->Time - Delta <= .75f) Shot(TEXT("bebendo"));
                if (S->Time > 1.6f)
                {
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_DRINK_CHECK bebendo=%d"), Hero->GetSheet() && Hero->GetSheet()->IsDrinking() ? 1 : 0);
                    S->Step = 3; S->Time = 0.f;
                }
                return true;
            case 3: if (S->Time > .3f) { Controller->SetControlRotation(FRotator(-24.f, 0.f, 0.f)); Weapon->PressPrimary(); S->Step = 4; S->Time = 0.f; } return true;
            case 4: if (S->Time > Strike) { Shot(TEXT("golpe")); S->Step = 5; S->Time = 0.f; } return true;
            case 5:
                if (S->Time > .8f && S->Time - Delta <= .8f) Shot(TEXT("golpe2"));
                if (S->Time > 1.5f)
                {
                    // mira no chão uns 4 m à frente: chuva, tornado, parede e selo caem na foto
                    Controller->SetControlRotation(FRotator(-24.f, 0.f, 0.f));
                    Weapon->PressSpecial(); S->Step = 6; S->Time = 0.f;
                }
                return true;
            case 6:
                if (S->Time > .25f && S->Time - Delta <= .25f) Shot(TEXT("poder1"));
                if (S->Time > .55f)
                {
                    Shot(TEXT("especial"));
                    Weapon->ReleaseSpecialInput();
                    S->Step = 7; S->Time = 0.f;
                }
                return true;
            case 7:
                if (S->Time > .3f && S->Time - Delta <= .3f) Shot(TEXT("poder3"));
                if (S->Time > .9f && S->Time - Delta <= .9f) Shot(TEXT("especial2"));
                if (S->Time > 1.6f)
                {
                    // primeira pessoa: parado e no meio do golpe, pela câmera do herói
                    Weapon->DebugResetSpecial();
                    Controller->SetViewTargetWithBlend(Hero, 0.f);
                    if (!Hero->IsFirstPerson()) Hero->ToggleFirstPerson();
                    Controller->SetControlRotation(FRotator(-8.f, 0.f, 0.f));
                    S->Step = 9; S->Time = 0.f;
                }
                return true;
            case 9: if (S->Time > 1.4f) { Shot(TEXT("1p")); Weapon->PressPrimary(); S->Step = 10; S->Time = 0.f; } return true;
            case 10: if (S->Time > Strike) { Shot(TEXT("1p_golpe")); S->Step = 11; S->Time = 0.f; } return true;
            case 11:
                if (S->Time > 1.2f)
                {
                    if (Hero->IsFirstPerson()) Hero->ToggleFirstPerson();
                    Controller->SetViewTargetWithBlend(S->Camera.Get(), 0.f);
                    ++S->Index; S->Step = 1;
                }
                return true;
            default: return false;
            }
        }));
    }
}
namespace VortexShopSmoke
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState { int32 Step = 0; float Time = 0.f; int32 Failures = 0; float Explored = 0.f; };
        TSharedRef<FState> S = MakeShared<FState>();
        TWeakObjectPtr<AGameModeBase> WeakMode(Mode);
        TWeakObjectPtr<APlayerController> Controller(Player);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakMode, Controller, S](float Delta) -> bool
        {
            AGameModeBase* Game = WeakMode.Get();
            AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
            if (!Game) return false;
            UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
            if (!Sheet || !Sheet->IsLoaded()) return true;
            S->Time += Delta;
            auto Check = [&](bool bOk, const FString& What)
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_SHOP_SMOKE %s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What);
                S->Failures += bOk ? 0 : 1;
            };
            auto Shot = [](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/loja_%s.png"), Name), true, false); };
            const FString World = Sheet->ShopWorldId();
            const int32 Day = Sheet->ShopWorldDay();
            switch (S->Step)
            {
            case 0:
            {
                if (S->Time < 2.f) return true;
                Sheet->bShopAnywhere = true;
                // mesmo sorteio para o mesmo mundo e dia, outro no dia seguinte
                const TArray<VortexShop::FOffer> A = VortexShop::Stock(TEXT("ducci"), World, Day), B = VortexShop::Stock(TEXT("ducci"), World, Day);
                const TArray<VortexShop::FOffer> Next = VortexShop::Stock(TEXT("ducci"), World, Day + 1);
                bool bSame = A.Num() == B.Num(), bDiffers = A.Num() != Next.Num();
                for (int32 i = 0; i < A.Num() && i < B.Num(); ++i) bSame &= A[i].Item == B[i].Item;
                for (int32 i = 0; i < A.Num() && i < Next.Num(); ++i) bDiffers |= A[i].Item != Next[i].Item;
                Check(A.Num() >= 8 && bSame, FString::Printf(TEXT("Ducci: %d ofertas, sorteio estável no dia"), A.Num()));
                Check(bDiffers, TEXT("Ducci: estoque novo no dia seguinte"));
                for (const TCHAR* V : { TEXT("bram"), TEXT("garrick"), TEXT("rosa"), TEXT("ilsa"), TEXT("marta"), TEXT("caravana"), TEXT("solcanion"), TEXT("verdalume") })
                    Check(VortexShop::Stock(V, World, Day).Num() > 0, FString::Printf(TEXT("%s tem estoque"), V));
                // sem moedas: não compra
                const int32 Before = Sheet->GetCoins();
                Sheet->SpendCoins(Before);
                Sheet->ServerShopBuy_Implementation(TEXT("marta"), 0);
                Check(Sheet->GetCoins() == 0 && Sheet->ShopLeft(TEXT("marta"), 0, VortexShop::Stock(TEXT("marta"), World, Day)[0].Qty, false) == VortexShop::Stock(TEXT("marta"), World, Day)[0].Qty,
                    TEXT("sem moedas não compra"));
                Sheet->AddCoins(5000);
                S->Step = 1; S->Time = 0.f;
                return true;
            }
            case 1:
            {
                // Marta: pão (8 por dia) até acabar
                const TArray<VortexShop::FOffer> Stock = VortexShop::Stock(TEXT("marta"), World, Day, Sheet->GetLevel());
                const VortexShop::FOffer Bread = Stock[0];
                const int32 Coins = Sheet->GetCoins(), Had = Sheet->CountOf(Bread.Item);
                for (int32 i = 0; i < Bread.Qty + 2; ++i) Sheet->ServerShopBuy_Implementation(TEXT("marta"), 0);
                Check(Sheet->CountOf(Bread.Item) == Had + Bread.Qty && Sheet->GetCoins() == Coins - Bread.Price * Bread.Qty,
                    FString::Printf(TEXT("Marta: comprou %d %s (o estoque acaba: %d)"), Sheet->CountOf(Bread.Item) - Had, *Bread.Item.ToString(), Bread.Qty));
                Check(Sheet->ShopLeft(TEXT("marta"), 0, Bread.Qty, false) == 0, TEXT("Marta: pão esgotado hoje"));
                // vende o pão de volta para a Marta (130%) e confere as moedas
                const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Bread.Item);
                const int32 Pays = Def ? VortexShop::Pays(TEXT("marta"), *Def) : 0, CoinsBefore = Sheet->GetCoins(), Count = Sheet->CountOf(Bread.Item);
                Sheet->ServerShopSell_Implementation(TEXT("marta"), Bread.Item, 3);
                Check(Sheet->CountOf(Bread.Item) == Count - 3 && Sheet->GetCoins() == CoinsBefore + Pays * 3, FString::Printf(TEXT("Marta: vendeu 3 por %d cada"), Pays));
                // Bram não compra comida
                const int32 C2 = Sheet->GetCoins(), N2 = Sheet->CountOf(Bread.Item);
                Sheet->ServerShopSell_Implementation(TEXT("bram"), Bread.Item, 1);
                Check(Sheet->GetCoins() == C2 && Sheet->CountOf(Bread.Item) == N2, TEXT("Bram não compra pão"));
                // Ducci: compra a primeira arma do dia
                const VortexShop::FOffer Weapon = VortexShop::Stock(TEXT("ducci"), World, Day, Sheet->GetLevel())[0];
                const int32 W0 = Sheet->CountOf(Weapon.Item);
                Sheet->ServerShopBuy_Implementation(TEXT("ducci"), 0);
                Sheet->ServerShopBuy_Implementation(TEXT("ducci"), 0);
                Check(Sheet->CountOf(Weapon.Item) == W0 + 1, FString::Printf(TEXT("Ducci: uma de cada (%s)"), *Weapon.Item.ToString()));
                // longe do comerciante não compra
                Sheet->bShopAnywhere = false;
                const int32 C3 = Sheet->GetCoins();
                Sheet->ServerShopBuy_Implementation(TEXT("caravana"), 0);
                Check(Sheet->GetCoins() == C3, TEXT("longe do comerciante não compra"));
                Sheet->bShopAnywhere = true;
                // mapa de viajante da caravana: compra e usa (a rota sai da névoa da carta)
                const TArray<VortexShop::FOffer> Caravan = VortexShop::Stock(TEXT("caravana"), World, Day);
                const int32 MapIndex = Caravan.IndexOfByPredicate([](const VortexShop::FOffer& O) { return O.Item == TEXT("mapa_de_viajante"); });
                Check(MapIndex != INDEX_NONE, TEXT("a caravana vende o mapa de viajante"));
                if (MapIndex != INDEX_NONE) Sheet->ServerShopBuy_Implementation(TEXT("caravana"), MapIndex);
                const AVortexHudBase* Hud = Cast<AVortexHudBase>(Controller->GetHUD());
                S->Explored = Hud ? Hud->ExploredFraction() : 0.f;
                const int32 MapSlot = Sheet->GetBag().IndexOfByPredicate([](const FVortexItemStack& St) { return St.Item == TEXT("mapa_de_viajante"); });
                Check(MapSlot != INDEX_NONE, TEXT("mapa na mochila"));
                if (MapSlot != INDEX_NONE) Sheet->ServerUseSlot_Implementation(uint8(MapSlot));
                S->Step = 3; S->Time = 0.f;
                return true;
            }
            case 3:
            {
                if (S->Time < 1.f) return true;
                const AVortexHudBase* Hud = Cast<AVortexHudBase>(Controller->GetHUD());
                const float Now = Hud ? Hud->ExploredFraction() : 0.f;
                Check(Now > S->Explored && Sheet->CountOf(TEXT("mapa_de_viajante")) == 0,
                    FString::Printf(TEXT("mapa usado: carta de %.2f%% para %.2f%% descoberta"), S->Explored * 100.f, Now * 100.f));
                if (AVortexHudBase* Open = Cast<AVortexHudBase>(Controller->GetHUD())) Open->ToggleWorldMap();
                S->Step = 4; S->Time = 0.f;
                return true;
            }
            case 4:
            {
                if (S->Time > 1.5f && S->Time - Delta <= 1.5f) Shot(TEXT("mapa_viajante"));
                if (S->Time < 2.5f) return true;
                if (AVortexHudBase* Open = Cast<AVortexHudBase>(Controller->GetHUD())) Open->ToggleWorldMap();
                VortexShopWindow::Open(Hero, TEXT("bram"));
                S->Step = 2; S->Time = 0.f;
                return true;
            }
            case 2:
            {
                const float T = S->Time;
                auto At = [&](float When) { return T >= When && T - Delta < When; };
                // a foto sai alguns quadros depois do pedido: cada loja fica 2 s na tela
                if (At(1.f)) Shot(TEXT("bram"));
                if (At(2.f)) { VortexShopWindow::Close(); VortexShopWindow::Open(Hero, TEXT("ducci")); }
                if (At(3.f)) Shot(TEXT("ducci"));
                if (At(4.f)) { VortexShopWindow::Close(); VortexShopWindow::Open(Hero, TEXT("marta")); }
                if (At(5.f)) Shot(TEXT("marta"));
                if (T > 6.f)
                {
                    VortexShopWindow::Close();
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_SHOP_SMOKE_%s falhas=%d"), S->Failures ? TEXT("FAIL") : TEXT("PASS"), S->Failures);
                    FPlatformMisc::RequestExit(false);
                    return false;
                }
                return true;
            }
            default: return false;
            }
        }));
    }
}
namespace VortexBanditSmoke
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState { int32 Step = 0; float Time = 0.f; int32 Failures = 0; float HealthBefore = 0.f; bool bArrow = false; int64 XpBefore = 0; FVector2D Camp; FName ChestId; FVector ChestAt = FVector::ZeroVector; };
        TSharedRef<FState> S = MakeShared<FState>();
        TWeakObjectPtr<AGameModeBase> WeakMode(Mode);
        TWeakObjectPtr<APlayerController> Controller(Player);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakMode, Controller, S](float Delta) -> bool
        {
            AGameModeBase* Game = WeakMode.Get();
            AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
            if (!Game) return false;
            UWorld* World = Game->GetWorld();
            UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
            AVortexTerrainStreamer* Streamer = nullptr;
            for (TActorIterator<AVortexTerrainStreamer> It(World); It; ++It) Streamer = *It;
            if (!Sheet || !Sheet->IsLoaded() || !Streamer || !Streamer->IsReady()) return true;
            S->Time += Delta;
            auto Check = [&](bool bOk, const FString& What)
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_BANDIT_SMOKE %s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What);
                S->Failures += bOk ? 0 : 1;
            };
            auto Shot = [](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/bandido_%s.png"), Name), false, false); };
            auto Bandits = [World]()
            {
                TArray<AVortexMonster*> Out;
                for (TActorIterator<AVortexMonster> It(World); It; ++It) if (It->IsBandit() && !It->IsCreatureDead()) Out.Add(*It);
                return Out;
            };
            UAbilitySystemComponent* ASC = Hero->GetAbilitySystemComponent();
            const FVector2D Open(-30000.f, -8000.f);
            switch (S->Step)
            {
            case 0:
            {
                float Ground = 0.f;
                Hero->SetActorLocation(FVector(Open, Streamer->GroundAt(Open.X, Open.Y) + 120.f), false, nullptr, ETeleportType::TeleportPhysics);
                if (!Streamer->GroundReadyAt(Open.X, Open.Y, Ground) || Streamer->PendingChunks() > 0 || S->Time < 3.f) return true;
                Hero->SetActorRotation(FRotator::ZeroRotator);
                Controller->SetControlRotation(FRotator(-10.f, 0.f, 0.f));
                UVortexCreatureSubsystem* Creatures = World->GetSubsystem<UVortexCreatureSubsystem>();
                int32 Made = 0;
                const TCHAR* Kinds[] = { TEXT("Bandit"), TEXT("BanditShield"), TEXT("BanditBrute"), TEXT("BanditArcher") };
                for (int32 i = 0; i < 4; ++i)
                    if (Creatures) Made += Creatures->DebugSpawnNear(Kinds[i], Hero->GetActorLocation() + FVector(520.f + i * 40.f, (i - 1.5f) * 230.f, 0.f), 3);
                Check(Made == 4, FString::Printf(TEXT("os 4 tipos nasceram (%d)"), Made));
                if (ASC) ASC->SetNumericAttributeBase(UVortexAttributes::GetMaxHealthAttribute(), 5000.f);
                if (ASC) ASC->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), 5000.f);
                S->Step = 1; S->Time = 0.f;
                return true;
            }
            case 1:
            {
                // fotos de perto: uma câmera própria olhando os 4 de frente (com o sol de lado), logo que nascem
                if (S->Time > .5f && S->Time - Delta <= .5f)
                {
                    FVector Mid = FVector::ZeroVector;
                    TArray<AVortexMonster*> All = Bandits();
                    for (AVortexMonster* B : All) Mid += B->GetActorLocation();
                    Mid /= FMath::Max(1, All.Num());
                    // de frente para a fila dos 4 (estão lado a lado no eixo Y), longe o bastante para caber todos
                    const FVector Eye = Mid + FVector(-820.f, 0.f, 150.f);
                    Hero->SetActorHiddenInGame(true);   // o herói fica no meio do caminho da câmera
                    FActorSpawnParameters Params;
                    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                    if (ACameraActor* Camera = World->SpawnActor<ACameraActor>(Eye, (Mid + FVector(0.f, 0.f, 20.f) - Eye).Rotation(), Params))
                        Controller->SetViewTargetWithBlend(Camera, 0.f);
                }
                if (S->Time > 1.1f && S->Time - Delta <= 1.1f) Shot(TEXT("quatro"));
                if (S->Time > 2.4f && S->Time - Delta <= 2.4f) { Controller->SetViewTargetWithBlend(Hero, 0.f); Hero->SetActorHiddenInGame(false); }
                if (S->Time < 3.f) return true;
                S->HealthBefore = ASC ? ASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) : 0.f;
                S->Step = 2; S->Time = 0.f;
                return true;
            }
            case 2:
            {
                // deixa o bando atacar uns segundos (o herói parado); procura a flecha no ar
                for (TActorIterator<AVortexProjectile> It(World); It; ++It) if (It->GetOwner() && Cast<AVortexMonster>(It->GetOwner())) S->bArrow = true;
                if (S->Time > 3.5f && S->Time - Delta <= 3.5f) Shot(TEXT("ataque"));
                if (S->Time < 8.f) return true;
                const float Now = ASC ? ASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) : 0.f;
                Check(Now < S->HealthBefore, FString::Printf(TEXT("o bando feriu o herói (%.0f -> %.0f)"), S->HealthBefore, Now));
                Check(S->bArrow, TEXT("o batedor atirou uma flecha de verdade"));
                // escudo: um golpe pela frente passa 40%
                AVortexMonster* Shield = nullptr;
                for (AVortexMonster* B : Bandits()) if (B->GetAssetName() == TEXT("BanditShield")) Shield = B;
                if (Shield)
                {
                    UVortexCombatComponent* Their = Shield->FindComponentByClass<UVortexCombatComponent>();
                    UAbilitySystemComponent* TheirAsc = Shield->GetAbilitySystemComponent();
                    Shield->SetActorRotation((Hero->GetActorLocation() - Shield->GetActorLocation()).GetSafeNormal2D().Rotation());
                    if (Their) Their->bShieldFront = true;
                    const float H0 = TheirAsc->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
                    Hero->GetCombat()->ApplyHit(Shield, 10.f);
                    const float Front = H0 - TheirAsc->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
                    Shield->SetActorRotation((Shield->GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D().Rotation());
                    if (Their) Their->bShieldFront = true;
                    const float H1 = TheirAsc->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
                    Hero->GetCombat()->ApplyHit(Shield, 10.f);
                    const float Back = H1 - TheirAsc->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
                    Check(Front > 0.f && Front < Back * .6f, FString::Printf(TEXT("escudo: pela frente %.1f, pelas costas %.1f"), Front, Back));
                }
                else Check(false, TEXT("guerreiro de escudo vivo"));
                // vence todos: XP e o saque "bandit"
                S->XpBefore = Sheet->GetXp();
                for (AVortexMonster* B : Bandits()) for (int32 i = 0; i < 60 && !B->IsCreatureDead(); ++i) Hero->GetCombat()->ApplyHit(B, 300.f);
                S->Step = 3; S->Time = 0.f;
                return true;
            }
            case 3:
            {
                if (S->Time < 2.f) return true;
                Check(Bandits().Num() == 0 && Sheet->GetXp() > S->XpBefore, FString::Printf(TEXT("bando vencido, XP %lld -> %lld"), S->XpBefore, Sheet->GetXp()));
                // um acampamento: vai até o mais perto e espera a fogueira montar
                Check(UVortexCreatureSubsystem::NearestBanditCamp(FVector2D(Hero->GetActorLocation()), 12, S->Camp), TEXT("há acampamento por perto"));
                Hero->SetActorLocation(FVector(S->Camp + FVector2D(-900.f, -500.f), Streamer->GroundAt(S->Camp.X - 900.f, S->Camp.Y - 500.f) + 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                S->Step = 4; S->Time = 0.f;
                return true;
            }
            case 4:
            {
                float Ground = 0.f;
                if ((!Streamer->GroundReadyAt(S->Camp.X, S->Camp.Y, Ground) || Streamer->PendingChunks() > 0) && S->Time < 30.f) return true;
                if (S->Time < 6.f) return true;
                const FVector Look = FVector(S->Camp, Ground + 80.f) - Hero->GetActorLocation();
                Hero->UseGodotBenchCamera(500.f, 0.f);
                Controller->SetControlRotation(FRotator(-16.f, Look.Rotation().Yaw, 0.f));
                int32 Camps = 0;
                for (TActorIterator<AVortexLandmarks> It(World); It; ++It) Camps += It->BuiltCamps();
                Check(Camps > 0, FString::Printf(TEXT("acampamento montado (%d)"), Camps));
                S->Step = 5; S->Time = 0.f;
                return true;
            }
            case 5:
            {
                if (S->Time > 2.f && S->Time - Delta <= 2.f) Shot(TEXT("acampamento"));
                if (S->Time < 3.f) return true;
                int32 Near = 0;
                for (TActorIterator<AVortexMonster> It(World); It; ++It) if (It->IsBandit() && FVector2D::Distance(FVector2D(It->GetActorLocation()), S->Camp) < 2000.f) ++Near;
                Check(Near >= 3, FString::Printf(TEXT("bando no acampamento (%d)"), Near));
                // o baú guardado: com o bando de pé não abre
                AVortexWorldFeatures* Features = AVortexWorldFeatures::Find(World);
                const FVector CampFloor(S->Camp, Streamer->GroundAt(S->Camp.X, S->Camp.Y));
                const AVortexWorldFeatures::FChest* Chest = Features ? Features->NearestChest(CampFloor, 1500.f, nullptr) : nullptr;
                Check(Chest && Chest->bGuarded, TEXT("o acampamento tem o baú guardado"));
                if (Chest)
                {
                    // 02/11: a vida de teste de novo (o nível que o primeiro bando deu recalculou a ficha e a vida máxima
                    // voltou ao normal; ao lado do bando o herói morria, voltava à fonte e o acampamento desmontava)
                    if (UAbilitySystemComponent* Life = Hero->GetAbilitySystemComponent())
                    {
                        Life->SetNumericAttributeBase(UVortexAttributes::GetMaxHealthAttribute(), 5000.f);
                        Life->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), 5000.f);
                    }
                    S->ChestId = Chest->Id;
                    S->ChestAt = Chest->Location;
                    Hero->SetActorLocation(Chest->Location + FVector(-150.f, 0.f, 90.f), false, nullptr, ETeleportType::TeleportPhysics);
                    Hero->DebugOpenChest(Chest->Id);
                    Check(!Hero->GetQuestLog()->HasFlag(Chest->Id), TEXT("com o bando de pé o baú não abre"));
                    Shot(TEXT("bau"));
                }
                S->Step = 6; S->Time = 0.f;
                return true;
            }
            case 6:
            {
                // vence o bando do acampamento e abre o baú
                if (S->Time < .5f) return true;
                if (S->Time - Delta < .5f)
                {
                    for (TActorIterator<AVortexMonster> It(World); It; ++It)
                        if (It->IsBandit() && FVector::Dist2D(It->GetActorLocation(), S->ChestAt) < 2200.f)
                            for (int32 i = 0; i < 60 && !It->IsCreatureDead(); ++i) Hero->GetCombat()->ApplyHit(*It, 300.f);
                    return true;
                }
                if (S->Time < 2.5f) return true;
                if (!S->ChestId.IsNone())
                {
                    const int32 CoinsBefore = Sheet->GetCoins();
                    Hero->SetActorLocation(S->ChestAt + FVector(-150.f, 0.f, 90.f), false, nullptr, ETeleportType::TeleportPhysics);
                    Hero->DebugOpenChest(S->ChestId);
                    int32 Alive = 0;
                    for (TActorIterator<AVortexMonster> It(World); It; ++It)
                        if (It->IsBandit() && !It->IsCreatureDead() && FVector::Dist2D(It->GetActorLocation(), S->ChestAt) < 2200.f) ++Alive;
                    // quem segura o baú (AVortexWorldFeatures::GuardsAlive conta qualquer criatura hostil viva perto)
                    for (TActorIterator<AActor> It(World); It; ++It)
                    {
                        const IVortexCreature* Creature = Cast<IVortexCreature>(*It);
                        if (Creature && !Creature->IsCreatureDead() && !Creature->IsPassive() && FVector::Dist2D(It->GetActorLocation(), S->ChestAt) < 2200.f)
                            UE_LOG(LogTemp, Display, TEXT("VORTEX_BANDIT_SMOKE guarda %s %s a %.0f cm"), *It->GetName(), *Creature->GetSpecies().ToString(),
                                FVector::Dist2D(It->GetActorLocation(), S->ChestAt));
                    }
                    Check(Hero->GetQuestLog()->HasFlag(S->ChestId) && Sheet->GetCoins() > CoinsBefore,
                        FString::Printf(TEXT("bando vencido: o baú abriu (moedas %d -> %d, aberto %d, bandidos vivos %d, herói a %.0f cm, vida %.0f)"), CoinsBefore,
                            Sheet->GetCoins(), Hero->GetQuestLog()->HasFlag(S->ChestId) ? 1 : 0, Alive, FVector::Dist(Hero->GetActorLocation(), S->ChestAt),
                            Hero->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute())));
                }
                UE_LOG(LogTemp, Display, TEXT("VORTEX_BANDIT_SMOKE_%s falhas=%d"), S->Failures ? TEXT("FAIL") : TEXT("PASS"), S->Failures);
                FPlatformMisc::RequestExit(false);
                return false;
            }
            default: return false;
            }
        }));
    }
}
namespace VortexPoiSmoke
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState
        {
            int32 Step = 0, Index = -1, Failures = 0;
            float Time = 0.f;
            TArray<FVortexPoi> Targets;
            TWeakObjectPtr<ACameraActor> Camera;
            AVortexPointsOfInterest::FWebSpot Web;
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
            UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
            AVortexTerrainStreamer* Streamer = nullptr;
            for (TActorIterator<AVortexTerrainStreamer> It(World); It; ++It) Streamer = *It;
            if (!Sheet || !Sheet->IsLoaded() || !Streamer || !Streamer->IsReady()) return true;
            S->Time += Delta;
            auto Check = [&](bool bOk, const FString& What)
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_POI_SMOKE %s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What);
                S->Failures += bOk ? 0 : 1;
            };
            auto Shot = [](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/poi_%s.png"), *Name), false, false); };
            auto Look = [&](const FVector& Eye, const FVector& At)
            {
                FActorSpawnParameters Params;
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                if (S->Camera.IsValid()) S->Camera->Destroy();
                S->Camera = World->SpawnActor<ACameraActor>(Eye, (At - Eye).Rotation(), Params);
                if (S->Camera.IsValid()) Controller->SetViewTargetWithBlend(S->Camera.Get(), 0.f);
            };
            static const TCHAR* Files[] = { TEXT("ruinas"), TEXT("torre"), TEXT("santuario"), TEXT("mina"), TEXT("cemiterio"), TEXT("caravana"), TEXT("cais") };
            if (UAbilitySystemComponent* ASC = Hero->GetAbilitySystemComponent())
            {
                ASC->SetNumericAttributeBase(UVortexAttributes::GetMaxHealthAttribute(), 99999.f);
                ASC->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), 99999.f);
            }
            switch (S->Step)
            {
            case 0:
            {
                // um de cada tipo, o mais perto do centro do mapa
                const FIntPoint Home(0, 0);
                TArray<float> Best;
                Best.Init(TNumericLimits<float>::Max(), int32(EVortexPoi::Count));
                S->Targets.SetNum(int32(EVortexPoi::Count));
                for (int32 DX = -60; DX <= 60; ++DX) for (int32 DY = -60; DY <= 60; ++DY)
                {
                    FVortexPoi Poi;
                    if (!AVortexPointsOfInterest::PoiAt(Home + FIntPoint(DX, DY), Poi)) continue;
                    const float D = Poi.At.Size();
                    if (D < Best[int32(Poi.Kind)]) { Best[int32(Poi.Kind)] = D; S->Targets[int32(Poi.Kind)] = Poi; }
                }
                for (int32 K = 0; K < int32(EVortexPoi::Count); ++K)
                    Check(Best[K] < TNumericLimits<float>::Max(), FString::Printf(TEXT("existe %s (%.0f m)"), *AVortexPointsOfInterest::KindName(EVortexPoi(K)), Best[K] / 100.f));
                // o portal de dungeon perto da cidade
                Hero->SetActorLocation(FVector(10500.f, -2000.f, Streamer->GroundAt(10500.f, -2000.f) + 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                S->Step = 1; S->Time = 0.f;
                return true;
            }
            case 1:
            {
                float Ground = 0.f;
                if ((!Streamer->GroundReadyAt(12000.f, -2000.f, Ground) || Streamer->PendingChunks() > 0) && S->Time < 30.f) return true;
                if (S->Time < 4.f) return true;
                AVortexPortal* Portal = AVortexPortal::FindNear(World, FVector(12000.f, -2000.f, Ground), 600.f);
                Check(Portal != nullptr, TEXT("o portal de ranque E perto da cidade"));
                if (Portal)
                {
                    Look(Portal->GetActorLocation() + FVector(-1100.f, 350.f, 260.f), Portal->GetActorLocation() + FVector(0.f, 0.f, 280.f));
                    Hero->SetActorLocation(Portal->GetActorLocation() + FVector(-200.f, 0.f, 120.f), false, nullptr, ETeleportType::TeleportPhysics);
                    Hero->DebugEnterPortal(Portal);
                }
                S->Step = 2; S->Time = 0.f;
                return true;
            }
            case 2:
            {
                if (S->Time > 1.f && S->Time - Delta <= 1.f) Shot(TEXT("portal"));
                if (S->Time < 1.5f) return true;
                S->Step = 3; S->Time = 0.f;
                return true;
            }
            case 3:
            {
                // o próximo ponto: vai para 14 m na frente dele
                if (++S->Index >= S->Targets.Num())
                {
                    // depois dos pontos: os monstros dos portais e as teias
                    if (GEngine)
                        for (const TCHAR* Flag : { TEXT("ShowFlag.InstancedStaticMeshes 1"), TEXT("ShowFlag.InstancedFoliage 1"), TEXT("ShowFlag.InstancedGrass 1") })
                            GEngine->Exec(World, Flag);
                    Controller->SetViewTargetWithBlend(Hero, 0.f);
                    Hero->SetActorLocation(FVector(7800.f, -2500.f, Streamer->GroundAt(7800.f, -2500.f) + 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                    Hero->SetActorRotation(FRotator::ZeroRotator);
                    S->Step = 7; S->Time = 0.f;
                    return true;
                }
                const FVortexPoi& Poi = S->Targets[S->Index];
                if (Poi.Id.IsNone()) return true;
                const FVector2D Stand = AVortexPointsOfInterest::LocalToWorld(Poi, 0.f, -14.f);
                Hero->SetActorLocation(FVector(Stand, Streamer->GroundAt(Stand.X, Stand.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
                S->Step = 4; S->Time = 0.f;
                return true;
            }
            case 4:
            {
                const FVortexPoi& Poi = S->Targets[S->Index];
                float Ground = 0.f;
                if ((!Streamer->GroundReadyAt(Poi.At.X, Poi.At.Y, Ground) || Streamer->PendingChunks() > 0) && S->Time < 30.f) return true;
                if (S->Time < 4.f) return true;
                const FVector Center(Poi.At, Ground);
                // de cima e de longe, sem as árvores (as copas tapavam tudo): só nas fotos dos pontos
                if (GEngine)
                    for (const TCHAR* Flag : { TEXT("ShowFlag.InstancedStaticMeshes 0"), TEXT("ShowFlag.InstancedFoliage 0"), TEXT("ShowFlag.InstancedGrass 0") })
                        GEngine->Exec(World, Flag);
                const FVector2D EyeAt = AVortexPointsOfInterest::LocalToWorld(Poi, 5.f, -17.f);
                Look(FVector(EyeAt, Ground + 1100.f), Center + FVector(0.f, 0.f, 120.f));
                AVortexPointsOfInterest* Pois = AVortexPointsOfInterest::Find(World);
                Check(Pois && Pois->BuiltCount() > 0, FString::Printf(TEXT("%s montado"), *AVortexPointsOfInterest::KindName(Poi.Kind)));
                AVortexWorldFeatures* Features = AVortexWorldFeatures::Find(World);
                const AVortexWorldFeatures::FChest* Chest = Features ? Features->NearestChest(Center, 1500.f, nullptr) : nullptr;
                switch (Poi.Kind)
                {
                case EVortexPoi::Shrine:
                {
                    Hero->SetActorLocation(FVector(AVortexPointsOfInterest::LocalToWorld(Poi, 0.f, -3.f), Ground + 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                    Hero->DebugPray(Poi.Id);
                    Check(!Sheet->GetBlessing().IsNone(), FString::Printf(TEXT("santuário deu a %s"), *AVortexPointsOfInterest::BlessingName(Poi.Variant)));
                    break;
                }
                case EVortexPoi::Mine: case EVortexPoi::Graveyard:
                    Check(Chest && Chest->bGuarded, FString::Printf(TEXT("%s tem o baú guardado"), *AVortexPointsOfInterest::KindName(Poi.Kind)));
                    break;
                case EVortexPoi::Caravan:
                    Check(Chest && !Chest->bGuarded, TEXT("caravana tem o baú"));
                    break;
                default: break;
                }
                S->Step = 5; S->Time = 0.f;
                return true;
            }
            case 5:
            {
                const FVortexPoi& Poi = S->Targets[S->Index];
                if (S->Time > 1.2f && S->Time - Delta <= 1.2f) Shot(Files[int32(Poi.Kind)]);
                if (S->Time < 1.6f) return true;
                if (Poi.Kind == EVortexPoi::Tower)
                {
                    // sobe: no topo a vista do mapa vai longe
                    const AVortexPointsOfInterest* Pois = AVortexPointsOfInterest::Find(World);
                    Hero->SetActorLocation(FVector(Poi.At, Streamer->GroundAt(Poi.At.X, Poi.At.Y) + 950.f), false, nullptr, ETeleportType::TeleportPhysics);
                    S->Step = 6; S->Time = 0.f;
                    return true;
                }
                S->Step = 3; S->Time = 0.f;
                return true;
            }
            case 7:
            {
                // aranha, esqueleto guerreiro e esqueleto arqueiro, de frente
                float Ground = 0.f;
                if ((!Streamer->GroundReadyAt(7800.f, -2500.f, Ground) || Streamer->PendingChunks() > 0) && S->Time < 30.f) return true;
                if (S->Time < 3.f) return true;
                UVortexCreatureSubsystem* Creatures = World->GetSubsystem<UVortexCreatureSubsystem>();
                int32 Made = 0;
                const TCHAR* Kinds[] = { TEXT("Skeleton"), TEXT("Spider"), TEXT("SkeletonArcher") };
                for (int32 i = 0; i < 3; ++i)
                    if (Creatures) Made += Creatures->DebugSpawnNear(Kinds[i], Hero->GetActorLocation() + FVector(i == 1 ? 420.f : 650.f, (i - 1) * 300.f, 0.f), 3);
                Check(Made == 3, FString::Printf(TEXT("aranha e esqueletos nasceram (%d)"), Made));
                Hero->SetActorHiddenInGame(true);
                Look(Hero->GetActorLocation() + FVector(-150.f, 0.f, 160.f), Hero->GetActorLocation() + FVector(600.f, 0.f, 20.f));
                S->Step = 8; S->Time = 0.f;
                return true;
            }
            case 8:
            {
                if (S->Time > 1.f && S->Time - Delta <= 1.f) Shot(TEXT("monstros"));
                if (S->Time < 1.4f) return true;
                Hero->SetActorHiddenInGame(false);
                Controller->SetViewTargetWithBlend(Hero, 0.f);
                for (TActorIterator<AVortexMonster> It(World); It; ++It) It->Destroy();
                // a teia mais perto do centro
                float BestD = TNumericLimits<float>::Max();
                for (int32 DX = -60; DX <= 60; ++DX) for (int32 DY = -60; DY <= 60; ++DY)
                {
                    TArray<AVortexPointsOfInterest::FWebSpot> Spots;
                    if (AVortexPointsOfInterest::WebsAt(FIntPoint(DX, DY), Spots) && Spots[0].At.Size() < BestD) { BestD = Spots[0].At.Size(); S->Web = Spots[0]; }
                }
                Check(BestD < TNumericLimits<float>::Max(), FString::Printf(TEXT("existem teias (%.0f m)"), BestD / 100.f));
                const FVector2D Near = S->Web.At + FVector2D(0.f, -900.f).GetRotated(S->Web.Yaw);
                Hero->SetActorLocation(FVector(Near, Streamer->GroundAt(Near.X, Near.Y) + 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                S->Step = 9; S->Time = 0.f;
                return true;
            }
            case 9:
            {
                float Ground = 0.f;
                if ((!Streamer->GroundReadyAt(S->Web.At.X, S->Web.At.Y, Ground) || Streamer->PendingChunks() > 0) && S->Time < 30.f) return true;
                if (S->Time < 4.f) return true;
                const FVector2D Eye = S->Web.At + FVector2D(0.f, -700.f).GetRotated(S->Web.Yaw);
                Look(FVector(Eye, Ground + 260.f), FVector(S->Web.At, Ground + 140.f));
                S->Step = 10; S->Time = 0.f;
                return true;
            }
            case 10:
            {
                if (S->Time > 1.f && S->Time - Delta <= 1.f) Shot(TEXT("teia"));
                if (S->Time < 1.4f) return true;
                // entra na teia: fica preso; três golpes cortam e solta
                Controller->SetViewTargetWithBlend(Hero, 0.f);
                Hero->SetActorLocation(FVector(S->Web.At, Streamer->GroundAt(S->Web.At.X, S->Web.At.Y) + 100.f), false, nullptr, ETeleportType::TeleportPhysics);
                S->Step = 11; S->Time = 0.f;
                return true;
            }
            case 11:
            {
                if (S->Time < .5f) return true;
                AVortexPointsOfInterest* Pois = AVortexPointsOfInterest::Find(World);
                Check(Hero->IsWebbed(), TEXT("dentro da teia: preso"));
                for (int32 i = 0; i < 3 && Pois; ++i) Pois->SlashWeb(Hero->GetActorLocation(), Hero->GetActorForwardVector());
                S->Step = 12; S->Time = 0.f;
                return true;
            }
            case 12:
            {
                if (S->Time < .4f) return true;
                Check(!Hero->IsWebbed(), TEXT("três golpes cortaram a teia: solto"));
                UE_LOG(LogTemp, Display, TEXT("VORTEX_POI_SMOKE_%s falhas=%d"), S->Failures ? TEXT("FAIL") : TEXT("PASS"), S->Failures);
                FPlatformMisc::RequestExit(false);
                return false;
            }
            case 6:
            {
                if (S->Time < 1.5f) return true;
                const AVortexPointsOfInterest* Pois = AVortexPointsOfInterest::Find(World);
                Check(Pois && Pois->IsOnTowerTop(Hero->GetActorLocation()), FString::Printf(TEXT("no topo da torre (z %.0f)"), Hero->GetActorLocation().Z));
                S->Step = 3; S->Time = 0.f;
                return true;
            }
            default: return false;
            }
        }));
    }
}
namespace VortexWildlifeSmoke
{
    // água funda (>= 1,2 m) perto de Center e um chão seco a até 20 m dela, para o herói
    bool FindSite(const FVortexTerrainData& Data, const FVector2D& Center, float MaxR, FVector& Water, FVector& Shore)
    {
        const float Step = 500.f;
        const int32 Reach = FMath::CeilToInt(MaxR / Step);
        float BestD = TNumericLimits<float>::Max();
        for (int32 DX = -Reach; DX <= Reach; ++DX) for (int32 DY = -Reach; DY <= Reach; ++DY)
        {
            const float X = Center.X + DX * Step, Y = Center.Y + DY * Step, D = FVector2D(DX, DY).Size() * Step;
            if (D >= BestD) continue;
            float Surface = 0.f;
            if (!Data.IsWaterAt(X, Y, Surface) || Surface - Data.GroundAt(X, Y) < 120.f) continue;
            for (int32 Ring = 1; Ring <= 8; ++Ring)
            {
                bool bFound = false;
                for (int32 A = 0; A < 12 && !bFound; ++A)
                {
                    const float Angle = A * UE_TWO_PI / 12.f, R = Ring * 250.f;
                    const float LX = X + FMath::Cos(Angle) * R, LY = Y + FMath::Sin(Angle) * R;
                    float LS = 0.f;
                    if (Data.IsWaterAt(LX, LY, LS) || Data.GroundAt(LX, LY) <= -299.f) continue;
                    Water = FVector(X, Y, Surface);
                    Shore = FVector(LX, LY, Data.GroundAt(LX, LY));
                    BestD = D;
                    bFound = true;
                }
                if (bFound) break;
            }
        }
        return BestD < TNumericLimits<float>::Max();
    }

    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FSite { FString Name; FVector Water, Shore; };
        struct FState
        {
            int32 Step = 0, Site = 0, Failures = 0;
            float Time = 0.f;
            TArray<FSite> Sites;
            FVector Flower = FVector::ZeroVector;
            TWeakObjectPtr<ACameraActor> Camera;
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
            UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
            AVortexTerrainStreamer* Streamer = nullptr;
            for (TActorIterator<AVortexTerrainStreamer> It(World); It; ++It) Streamer = *It;
            const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
            AVortexWildlife* Life = AVortexWildlife::Find(World);
            if (!Sheet || !Sheet->IsLoaded() || !Streamer || !Streamer->IsReady() || !Data) return true;
            S->Time += Delta;
            auto Check = [&](bool bOk, const FString& What)
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_WILDLIFE_SMOKE %s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What);
                S->Failures += bOk ? 0 : 1;
            };
            auto Shot = [](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/fauna_%s.png"), *Name), false, false); };
            auto Look = [&](const FVector& Eye, const FVector& At)
            {
                FActorSpawnParameters Params;
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                if (S->Camera.IsValid()) S->Camera->Destroy();
                S->Camera = World->SpawnActor<ACameraActor>(Eye, (At - Eye).Rotation(), Params);
                if (S->Camera.IsValid()) Controller->SetViewTargetWithBlend(S->Camera.Get(), 0.f);
            };
            auto Ready = [&](const FVector& At) { float Ground = 0.f; return (Streamer->GroundReadyAt(At.X, At.Y, Ground) && Streamer->PendingChunks() == 0) || S->Time > 30.f; };
            if (UAbilitySystemComponent* ASC = Hero->GetAbilitySystemComponent())
            {
                ASC->SetNumericAttributeBase(UVortexAttributes::GetMaxHealthAttribute(), 99999.f);
                ASC->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), 99999.f);
            }
            if (!Life) { Check(false, TEXT("o ator da fauna existe")); FPlatformMisc::RequestExit(false); return false; }
            switch (S->Step)
            {
            case 0:
            {
                // os lugares: o lago mais perto da cidade, o pântano (Lodonegro) e Verdalume
                const TPair<const TCHAR*, int32> Wanted[] = { { TEXT("vale"), 0 }, { TEXT("pantano"), 7 }, { TEXT("verdalume"), 9 } };
                for (const TPair<const TCHAR*, int32>& W : Wanted)
                {
                    FSite Site;
                    Site.Name = W.Key;
                    const FVector2D Center = W.Value ? Data->RegionCenter(W.Value) : FVector2D(0.f, 0.f);
                    const bool bFound = FindSite(*Data, Center, W.Value ? 30000.f : 40000.f, Site.Water, Site.Shore);
                    if (bFound) S->Sites.Add(Site);
                    if (W.Value == 0) Check(bFound, TEXT("um lago perto da cidade"));
                    else UE_LOG(LogTemp, Display, TEXT("VORTEX_WILDLIFE_SMOKE %s: %s"), W.Key, bFound ? TEXT("água achada") : TEXT("sem água funda perto do centro (pulado)"));
                }
                // uma flor a uns 25 m da cidade (para a dormideira)
                const float Size = Data->ChunkSize();
                float Best = TNumericLimits<float>::Max();
                for (int32 CX = -14; CX <= 14; ++CX) for (int32 CY = -14; CY <= 14; ++CY)
                {
                    const TArray<FVortexTerrainProp>* Props = Data->PropsInChunk(FIntPoint(CX, CY));
                    if (!Props) continue;
                    for (const FVortexTerrainProp& Prop : *Props)
                    {
                        const FString& Kind = Data->Kinds[Prop.Kind];
                        // coloridas (as pétalas brancas da margarida não mudam de cor)
                        if (Kind != TEXT("flowers_tulip") && Kind != TEXT("poppy") && Kind != TEXT("flowers_bell") && Kind != TEXT("flowers_lavender")) continue;
                        const float D = FVector2D(Prop.Location).Size();
                        // longe da cidade (as flores dos canteiros não são cópias instanciadas)
                        if (D < Best && D > 7000.f) { Best = D; S->Flower = Prop.Location; }
                    }
                }
                Check(!S->Flower.IsZero(), TEXT("uma flor perto da cidade"));
                // os sons novos: as gravações com 4 tomadas, a voz de cada espécie e os passos na água
                {
                    int32 Takes = 0;
                    for (int32 i = 1; i <= 4; ++i) Takes += VortexAudio::Has(FName(*FString::Printf(TEXT("sword_hit_v%d"), i)));
                    Check(Takes == 4 && VortexAudio::Has(TEXT("sword_hit")), FString::Printf(TEXT("som: as 4 tomadas gravadas do golpe (%d)"), Takes));
                    Check(VortexAudio::CreatureVoice(TEXT("Wolf"), TEXT("hit")) == TEXT("wolf_hit") && VortexAudio::CreatureVoice(TEXT("Rabbit"), TEXT("death")) == TEXT("rabbit_death")
                        && VortexAudio::CreatureVoice(TEXT("BanditBrute"), TEXT("hit")) == TEXT("bandit_hit") && VortexAudio::CreatureVoice(TEXT("SkeletonArcher"), TEXT("death")) == TEXT("skeleton_death")
                        && VortexAudio::CreatureVoice(TEXT("GoblinBrute"), TEXT("strike")) == TEXT("goblin_strike") && VortexAudio::CreatureVoice(TEXT("Dragon"), TEXT("hit")).IsNone(),
                        TEXT("som: a voz de cada espécie (lobo, coelho, bandido, esqueleto, goblin; um bicho sem voz fica sem)"));
                    Check(VortexAudio::Has(TEXT("step_water_1")) && VortexAudio::Has(TEXT("bird_crow")) && VortexAudio::Has(TEXT("fish_plop")),
                        TEXT("som: passo na água, corvo e peixe existem"));
                }
                if (S->Sites.IsEmpty()) { FPlatformMisc::RequestExit(false); return false; }
                S->Step = 1; S->Time = 0.f;
                return true;
            }
            case 1:
            {
                const FSite& Site = S->Sites[S->Site];
                Hero->SetActorLocation(Site.Shore + FVector(0.f, 0.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                Look(Site.Shore + FVector(-300.f, 0.f, 400.f), Site.Water);
                S->Step = 2; S->Time = 0.f;
                return true;
            }
            case 2:
            {
                const FSite& Site = S->Sites[S->Site];
                if (!Ready(Site.Shore) || S->Time < 4.f) return true;
                Life->DebugSpawnNear(Site.Shore);
                const FVector Flock = Life->NearestGroundFlock(Site.Shore);
                Check(!Flock.IsZero(), FString::Printf(TEXT("%s: pássaros no chão"), *Site.Name));
                if (!Flock.IsZero())
                {
                    const FVector Side = (Flock - Site.Shore).GetSafeNormal2D();
                    Look(Flock - Side * 450.f + FVector(0.f, 0.f, 300.f), Flock + FVector(0.f, 0.f, 20.f));
                }
                Life->SetActorHiddenInGame(true);
                S->Step = 3; S->Time = 0.f;
                return true;
            }
            case 3:
            {
                const FSite& Site = S->Sites[S->Site];
                if (S->Time > 1.f && S->Time - Delta <= 1.f) Shot(Site.Name + TEXT("_passaros_antes"));
                if (S->Time > 1.3f) Life->SetActorHiddenInGame(false);
                if (S->Time > 2.6f && S->Time - Delta <= 2.6f) Shot(Site.Name + TEXT("_passaros"));
                if (S->Time < 3.f) return true;
                UE_LOG(LogTemp, Display, TEXT("VORTEX_WILDLIFE_SMOKE %s: %s"), *Site.Name, *Life->Describe());
                // o som do lugar: pântano em Lodonegro (5), mata élfica em Verdalume (7)
                if (const AVortexForestLife* Forest = TActorIterator<AVortexForestLife>(World) ? *TActorIterator<AVortexForestLife>(World) : nullptr)
                {
                    const FString Sound = Forest->DescribeSound();
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_WILDLIFE_SMOKE %s: ambiente %s"), *Site.Name, *Sound);
                    if (Site.Name == TEXT("pantano")) Check(Sound.Contains(TEXT("lugar=5")), TEXT("pantano: toca o ambiente do pântano"));
                    if (Site.Name == TEXT("verdalume")) Check(Sound.Contains(TEXT("lugar=7")), TEXT("verdalume: toca o ambiente da mata élfica"));
                }
                Check(Life->SkyBirds() + Life->GroundBirds() > 0, FString::Printf(TEXT("%s: pássaros no céu e no chão"), *Site.Name));
                // a água: o cardume visto de cima, da margem
                const FVector School = Life->NearestSchool(Site.Water);
                Check(!School.IsZero() && Life->FishCount() > 0, FString::Printf(TEXT("%s: cardume na água (%d peixes)"), *Site.Name, Life->FishCount()));
                const FVector Focus = School.IsZero() ? Site.Water : School;
                const FVector Back = (Site.Shore - Focus).GetSafeNormal2D();
                Look(Focus + Back * 380.f + FVector(0.f, 0.f, 420.f), Focus);
                Life->SetActorHiddenInGame(true);
                S->Step = 4; S->Time = 0.f;
                return true;
            }
            case 4:
            {
                const FSite& Site = S->Sites[S->Site];
                if (S->Time > 1.f && S->Time - Delta <= 1.f) Shot(Site.Name + TEXT("_agua_antes"));
                if (S->Time > 1.3f) Life->SetActorHiddenInGame(false);
                if (S->Time > 2.6f && S->Time - Delta <= 2.6f) Shot(Site.Name + TEXT("_agua"));
                if (S->Time < 3.f) return true;
                // as vitórias-régias (se houver água rasa por perto)
                const FVector Pad = Life->NearestPad(Site.Water);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_WILDLIFE_SMOKE %s: vitórias-régias=%d algas=%d"), *Site.Name, Life->LilyPads(), Life->Seaweeds());
                if (!Pad.IsZero()) Look(Pad + FVector(-350.f, 120.f, 260.f), Pad);
                S->Step = Pad.IsZero() ? 6 : 5; S->Time = 0.f;
                return true;
            }
            case 5:
            {
                if (S->Time > 1.2f && S->Time - Delta <= 1.2f) Shot(S->Sites[S->Site].Name + TEXT("_plantas_agua"));
                if (S->Time < 1.5f) return true;
                S->Step = 6; S->Time = 0.f;
                return true;
            }
            case 6:
            {
                // próximo lugar
                if (S->Site + 1 < S->Sites.Num()) { ++S->Site; S->Step = 1; S->Time = 0.f; return true; }
                Check(Life->Seaweeds() + Life->LilyPads() > 0, TEXT("plantas da água montadas"));
                // o bando foge de quem chega
                const FVector Flock = Life->NearestGroundFlock(Hero->GetActorLocation());
                if (Flock.IsZero()) { Check(false, TEXT("um bando no chão para assustar")); S->Step = 8; S->Time = 0.f; return true; }
                Look(Flock + FVector(-900.f, 400.f, 250.f), Flock + FVector(0.f, 0.f, 200.f));
                Hero->SetActorLocation(Flock + FVector(-250.f, 0.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                S->Step = 7; S->Time = 0.f;
                return true;
            }
            case 7:
            {
                if (S->Time > .8f && S->Time - Delta <= .8f) Shot(TEXT("susto"));
                if (S->Time < 1.f) return true;
                Check(Life->FleeingBirds() > 0, FString::Printf(TEXT("o bando levantou voo com o herói perto (%d voando)"), Life->FleeingBirds()));
                S->Step = 8; S->Time = 0.f;
                return true;
            }
            case 8:
            {
                // esporos de cogumelo aceso
                const FVector At = Hero->GetActorLocation() + Hero->GetActorForwardVector() * 150.f - FVector(0.f, 0.f, 60.f);
                Life->DebugPuff(At, true);
                Look(At + FVector(-260.f, 160.f, 120.f), At + FVector(0.f, 0.f, 50.f));
                S->Step = 9; S->Time = 0.f;
                return true;
            }
            case 9:
            {
                if (S->Time > .5f && S->Time - Delta <= .5f) { Shot(TEXT("esporos")); Check(Life->Puffs() > 0, FString::Printf(TEXT("esporos no ar (%d)"), Life->Puffs())); }
                if (S->Time < .8f) return true;
                if (S->Flower.IsZero()) { S->Step = 12; S->Time = 0.f; return true; }
                Hero->SetActorLocation(S->Flower + FVector(-700.f, 0.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                Look(S->Flower + FVector(-170.f, 100.f, 300.f), S->Flower + FVector(0.f, 0.f, 15.f));
                S->Step = 10; S->Time = 0.f;
                return true;
            }
            case 10:
            {
                if (!Ready(S->Flower) || S->Time < 4.f) return true;
                Shot(TEXT("flor_longe"));
                S->Step = 11; S->Time = 0.f;
                return true;
            }
            case 11:
            {
                if (S->Time > .3f && S->Time - Delta <= .3f) Hero->SetActorLocation(S->Flower + FVector(60.f, 0.f, 120.f), false, nullptr, ETeleportType::TeleportPhysics);
                if (S->Time > 1.2f && S->Time - Delta <= 1.2f) Shot(TEXT("flor_perto"));
                if (S->Time < 1.5f) return true;
                S->Step = 12; S->Time = 0.f;
                return true;
            }
            case 12:
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_WILDLIFE_SMOKE_%s falhas=%d"), S->Failures ? TEXT("FAIL") : TEXT("PASS"), S->Failures);
                FPlatformMisc::RequestExit(false);
                return false;
            }
            default: return false;
            }
        }));
    }
}
namespace VortexMapShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState
        {
            int32 Step = 0, Failures = 0;
            float Time = 0.f;
            FVector Water = FVector::ZeroVector, Shore = FVector::ZeroVector;
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
            UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
            AVortexTerrainStreamer* Streamer = nullptr;
            for (TActorIterator<AVortexTerrainStreamer> It(World); It; ++It) Streamer = *It;
            const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
            AVortexHudBase* Hud = Controller.IsValid() ? Cast<AVortexHudBase>(Controller->GetHUD()) : nullptr;
            if (!Sheet || !Sheet->IsLoaded() || !Streamer || !Streamer->IsReady() || !Data || !Hud) return true;
            S->Time += Delta;
            auto Check = [&](bool bOk, const FString& What)
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_MAP_SHOTS %s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What);
                S->Failures += bOk ? 0 : 1;
            };
            auto Shot = [](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/hudmapa_%s.png"), *Name), false, false); };
            auto Ready = [&](const FVector& At) { float Ground = 0.f; return (Streamer->GroundReadyAt(At.X, At.Y, Ground) && Streamer->PendingChunks() == 0) || S->Time > 30.f; };
            switch (S->Step)
            {
            case 0:
            {
                // à beira de um lago perto da cidade (água, margem e mata no minimapa)
                Check(VortexWildlifeSmoke::FindSite(*Data, FVector2D(0.f, 0.f), 40000.f, S->Water, S->Shore), TEXT("um lago perto da cidade"));
                Hero->SetActorLocation(S->Shore + FVector(0.f, 0.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                S->Step = 1; S->Time = 0.f;
                return true;
            }
            case 1:
            {
                if (!Ready(S->Shore) || S->Time < 5.f || (!Hud->HasLocalMap() && S->Time < 20.f)) return true;
                Check(Hud->HasLocalMap(), TEXT("o mapa local do minimapa (1 ponto por bloco) foi montado"));
                // olha para a água
                Controller->SetControlRotation(FRotator(-15.f, (S->Water - S->Shore).Rotation().Yaw, 0.f));
                S->Step = 2; S->Time = 0.f;
                return true;
            }
            case 2:
            {
                if (S->Time > 1.5f && S->Time - Delta <= 1.5f) Shot(TEXT("hud"));
                if (S->Time < 2.f) return true;
                Hud->ToggleWorldMap();
                Check(Hud->IsWorldMapOpen(), TEXT("o mapa abriu"));
                S->Step = 3; S->Time = 0.f;
                return true;
            }
            case 3:
            {
                if (S->Time > 1.2f && S->Time - Delta <= 1.2f) Shot(TEXT("mapa_regiao"));
                if (S->Time < 1.6f) return true;
                for (int32 i = 0; i < 4; ++i) Hud->ZoomWorldMap(1.f);
                S->Step = 4; S->Time = 0.f;
                return true;
            }
            case 4:
            {
                if (S->Time > 1.f && S->Time - Delta <= 1.f) Shot(TEXT("mapa_mundo"));
                if (S->Time < 1.4f) return true;
                Hud->CenterWorldMap(FVector2D(Hero->GetActorLocation()));
                for (int32 i = 0; i < 6; ++i) Hud->ZoomWorldMap(-1.f);
                Hud->SetMapMarker(FVector2D(Hero->GetActorLocation()) + FVector2D(9000.f, -4000.f));
                Hud->CenterWorldMap(FVector2D(Hero->GetActorLocation()));
                S->Step = 5; S->Time = 0.f;
                return true;
            }
            case 5:
            {
                // o detalhe por bloco é montado em outra thread: espera ele (até 15 s)
                if (!Hud->HasDetailMap() && S->Time < 15.f) return true;
                if (S->Time < 1.f) return true;
                Check(Hud->HasDetailMap() && Hud->GetMapZoom() >= 8.f, FString::Printf(TEXT("de perto, o detalhe por bloco (zoom %.1f)"), Hud->GetMapZoom()));
                Shot(TEXT("mapa_perto"));
                S->Step = 6; S->Time = 0.f;
                return true;
            }
            case 6:
            {
                if (S->Time < .6f) return true;
                Hud->ZoomWorldMap(1.f);
                Hud->ZoomWorldMap(1.f);
                S->Step = 7; S->Time = 0.f;
                return true;
            }
            case 7:
            {
                if (S->Time > 1.f && S->Time - Delta <= 1.f) Shot(TEXT("mapa_marcador"));
                if (S->Time < 1.4f) return true;
                Check(Hud->HasMapMarker(), TEXT("o marcador ficou no mapa"));
                Hud->ToggleWorldMap();
                Check(!Hud->IsWorldMapOpen(), TEXT("o mapa fechou"));
                S->Step = 8; S->Time = 0.f;
                return true;
            }
            case 8:
            {
                if (S->Time > 1.f && S->Time - Delta <= 1.f) Shot(TEXT("hud_marcador"));
                if (S->Time < 1.4f) return true;
                UE_LOG(LogTemp, Display, TEXT("VORTEX_MAP_SHOTS_%s falhas=%d"), S->Failures ? TEXT("FAIL") : TEXT("PASS"), S->Failures);
                FPlatformMisc::RequestExit(false);
                return false;
            }
            default: return false;
            }
        }));
    }
}

namespace VortexIntroShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState { float Time = -1.f; double Clock = 0.0; int32 Failures = 0; bool bClosed = false; float SongBefore = 0.f; };
        TSharedRef<FState> S = MakeShared<FState>();
        TWeakObjectPtr<AGameModeBase> WeakMode(Mode);
        TWeakObjectPtr<APlayerController> Controller(Player);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakMode, Controller, S](float Delta) -> bool
        {
            AGameModeBase* Game = WeakMode.Get();
            AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
            if (!Game) return false;
            UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
            AVortexTerrainStreamer* Streamer = nullptr;
            for (TActorIterator<AVortexTerrainStreamer> It(Game->GetWorld()); It; ++It) Streamer = *It;
            if (!Sheet || !Sheet->IsLoaded() || !Streamer || !Streamer->IsReady()) return true;
            auto Check = [&](bool bOk, const FString& What)
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_INTRO_SHOTS %s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What);
                S->Failures += bOk ? 0 : 1;
            };
            auto Shot = [](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/abertura_%s.png"), *Name), true, false); };
            // relógio do sistema, como a HQ (o jogo de teste roda com passo fixo, mais rápido que o tempo real)
            const double Now = FPlatformTime::Seconds();
            if (S->Clock <= 0.0) S->Clock = Now;
            const float Before = S->Time;
            S->Time = float(Now - S->Clock);
            auto At = [&](float When) { return Before < When && S->Time >= When; };
            // -VortexIntroSync (com som): a imagem segue a música, mesmo depois de o jogo travar 3 s no meio da canção
            if (FParse::Param(FCommandLine::Get(), TEXT("VortexIntroSync")))
            {
                auto Report = [&](const TCHAR* When)
                {
                    float Image = -1.f, Song = -1.f;
                    const bool bHeard = VortexIntroComic::DebugSync(Image, Song);
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_INTRO_SYNC %s imagem=%.2f musica=%.2f aviso=%d %s"), When, Image, Song, bHeard ? 1 : 0, *VortexIntroComic::DebugState());
                    Check(bHeard && FMath::Abs(Image - Song) < .75f, FString::Printf(TEXT("a imagem acompanha a música (%s: %.2f x %.2f)"), When, Image, Song));
                    return Song;
                };
                if (At(3.f))
                {
                    const_cast<UVortexAttributes*>(Hero->GetVitals())->bGodMode = true;
                    VortexIntroComic::Open(Controller.Get(), [S]() { S->bClosed = true; });
                    Check(VortexIntroComic::IsOpen(), TEXT("a HQ abriu com a canção"));
                }
                if (At(14.f)) S->SongBefore = Report(TEXT("antes"));
                if (At(15.f)) { UE_LOG(LogTemp, Display, TEXT("VORTEX_INTRO_SYNC travando o jogo 3 s")); FPlatformProcess::Sleep(3.f); }
                if (At(19.5f))
                {
                    const float Song = Report(TEXT("depois_da_travada"));
                    Check(Song - S->SongBefore > 4.5f, FString::Printf(TEXT("a música seguiu durante a travada (+%.1f s)"), Song - S->SongBefore));
                }
                if (At(30.f)) Report(TEXT("fim"));
                if (At(30.5f))
                {
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_INTRO_SYNC_%s (%d falhas)"), S->Failures ? TEXT("FAIL") : TEXT("PASS"), S->Failures);
                    FPlatformMisc::RequestExit(false);
                    return false;
                }
                return true;
            }
            if (At(3.f))
            {
                const_cast<UVortexAttributes*>(Hero->GetVitals())->bGodMode = true;   // o herói fica 20 s parado no mundo: os lobos não o derrubam
                VortexIntroComic::Open(Controller.Get(), [S]() { S->bClosed = true; });
                Check(VortexIntroComic::IsOpen(), TEXT("a HQ abriu (2 folhas de 12 quadros, com a canção)"));
            }
            if (At(4.f)) { Shot(TEXT("0_escuro")); Check(VortexIntroComic::DebugState().StartsWith(TEXT("escuro")), TEXT("a introdução da canção fica no escuro: ") + VortexIntroComic::DebugState()); }
            if (At(4.4f)) VortexIntroComic::DebugJump(0, -3);
            if (At(4.9f)) Shot(TEXT("1_chega"));
            if (At(5.6f)) Shot(TEXT("2_pinta"));
            if (At(6.6f)) Shot(TEXT("3_pagina"));
            if (At(6.9f)) VortexIntroComic::DebugJump(0, 1);
            if (At(7.7f)) { Shot(TEXT("4_quadro_legenda")); Check(VortexIntroComic::DebugState().Contains(TEXT("quadro pagina=1 quadro=2")), TEXT("câmera no quadro 2: ") + VortexIntroComic::DebugState()); }
            if (At(9.8f)) Shot(TEXT("5_legenda_cantada"));
            if (At(10.2f)) VortexIntroComic::DebugJump(0, 10);
            if (At(10.32f)) Shot(TEXT("6_impacto"));
            if (At(11.6f)) Shot(TEXT("7_impacto_depois"));
            if (At(12.f)) VortexIntroComic::DebugJump(0, -1);
            if (At(12.7f)) { Shot(TEXT("8_vira")); Check(VortexIntroComic::DebugState().StartsWith(TEXT("vira")), TEXT("a página vira: ") + VortexIntroComic::DebugState()); }
            if (At(14.f)) Shot(TEXT("9_proxima_pinta"));
            if (At(15.f)) VortexIntroComic::DebugJump(1, 7);
            if (At(17.5f)) Shot(TEXT("10_heroi"));
            if (At(18.f)) VortexIntroComic::DebugJump(1, -2);
            if (At(19.1f)) Shot(TEXT("11_branco"));
            if (At(19.7f)) Shot(TEXT("12_acorde"));
            if (At(22.5f))
            {
                Check(!VortexIntroComic::IsOpen() && S->bClosed, TEXT("a HQ fechou sozinha depois do Acorde"));
                Shot(TEXT("13_jogo_de_volta"));
            }
            if (At(23.5f))
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_INTRO_SHOTS_%s (%d falhas)"), S->Failures ? TEXT("FAIL") : TEXT("PASS"), S->Failures);
                FPlatformMisc::RequestExit(false);
                return false;
            }
            return true;
        }));
    }
}
#endif
