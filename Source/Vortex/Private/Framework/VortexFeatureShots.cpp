// Teste com fotos das novidades do combate e da água (-VortexFeatureShots, Tools/RunFeatureShots.ps1).
// Em L_TerrainProto (Saved/Screenshots/novo_*.png):
//  1. golpe de perto sem mira: um golem a ~2 m da borda; o herói (de costas para ele) clica e acerta;
//  2. Soco Sério das manoplas contra uma fila de gotas a 6-16 m;
//  3. primeira pessoa (C): olhando para a frente e para baixo;
//  4. água: cai de 6 m num lago (respingo), nada para a frente (crawl) e boia parado.
#if !UE_BUILD_SHIPPING
#include "Framework/VortexCreatureSmoke.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Containers/Ticker.h"
#include "Creatures/VortexMonster.h"
#include "EngineUtils.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexCharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Rpg/VortexHeroSheet.h"
#include "UnrealClient.h"
#include "World/VortexCreatureSubsystem.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexWaterZone.h"
#include "Framework/VortexHeroWindow.h"

namespace VortexFeatureShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState
        {
            int32 Step = 0;
            float Time = 0.f, Before = 0.f;
            bool bPass = true;
            TArray<FString> Lines;
            TArray<TWeakObjectPtr<AVortexMonster>> Targets;
            FVector Lake = FVector::ZeroVector;
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
            UAbilitySystemComponent* Abilities = Hero->GetAbilitySystemComponent();
            UVortexCreatureSubsystem* Creatures = World->GetSubsystem<UVortexCreatureSubsystem>();
            S->Time += Delta;
            const float T = S->Time;
            auto At = [&](float When) { return T >= When && T - Delta < When; };
            auto Shot = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/novo_%s.png"), Name), true, false); };
            auto Check = [&](bool bOk, const FString& What) { S->Lines.Add(FString::Printf(TEXT("%s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What)); S->bPass &= bOk; };
            auto Next = [&](int32 Step) { S->Step = Step; S->Time = 0.f; };
            auto Spawn = [&](FName Species, const FVector& Where) -> AVortexMonster*
            {
                if (Creatures) Creatures->DebugSpawnNear(Species, Where, 3);
                AVortexMonster* Best = nullptr;
                float BestDistance = 1e9f;
                for (TActorIterator<AVortexMonster> It(World); It; ++It)
                {
                    const float D = FVector::Dist2D(It->GetActorLocation(), Where);
                    if (It->GetAssetName() == Species && !It->IsCreatureDead() && D < BestDistance) { Best = *It; BestDistance = D; }
                }
                return Best;
            };
            auto Health = [](AVortexMonster* M) { return M ? M->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) : 0.f; };
            const FVector2D Open(-30000.f, -8000.f);
            switch (S->Step)
            {
            case 0:
            {
                float Ground = 0.f;
                Hero->SetActorLocation(FVector(Open, Streamer->GroundAt(Open.X, Open.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
                if (!Streamer->GroundReadyAt(Open.X, Open.Y, Ground) || Streamer->PendingChunks() > 0 || T < 3.f) return true;
                Controller->SetControlRotation(FRotator(-12.f, 0.f, 0.f));
                Next(1);
                return true;
            }
            case 1:   // golem ao lado (a 90° da câmera): o golpe de perto vira e acerta sem mira
            {
                if (At(.1f))
                {
                    const FVector Side = FRotator(0.f, Controller->GetControlRotation().Yaw + 90.f, 0.f).Vector();
                    // ao alcance da lâmina (~1,2 m da borda): o herói vira e acerta sem andar (o alcance de 4,3 m e o avanço
                    // automático saíram em 23/10)
                    AVortexMonster* Golem = Spawn(TEXT("Golem"), Hero->GetActorLocation() + Side * 230.f);
                    S->Targets = { Golem };
                    S->Before = Health(Golem);
                    Check(Golem != nullptr, TEXT("golem para o teste de alcance"));
                }
                AVortexMonster* Golem = S->Targets.Num() ? S->Targets[0].Get() : nullptr;
                if (!Golem) { if (T > .3f) Next(2); return true; }
                // o golem fica parado (o teste é do herói)
                if (At(1.2f)) Hero->BeginPrimaryAttack();
                if (At(1.45f)) Shot(TEXT("alcance"));
                if (At(2.2f))
                {
                    const UCapsuleComponent* Capsule = Golem->GetCapsuleComponent();
                    const float Gap = FVector::Dist2D(Golem->GetActorLocation(), Hero->GetActorLocation()) - Capsule->GetScaledCapsuleRadius();
                    Check(Health(Golem) < S->Before, FString::Printf(TEXT("golpe de perto acertou o golem a %.0f cm da borda, de lado para a câmera"), Gap));
                    Golem->Destroy();
                    Next(2);
                }
                return true;
            }
            case 2:   // manoplas: Soco Sério numa fila de gotas
            {
                if (At(.1f))
                {
                    if (UVortexWeaponDefinition* Gauntlets = UVortexWeaponComponent::LoadType(TEXT("manoplas_de_ferro"))) Hero->GetWeapon()->Equip(Gauntlets);
                    Check(Hero->GetWeapon()->HasSeriousPunch(), TEXT("manoplas equipadas (Soco Sério)"));
                    const FVector Forward = Controller->GetControlRotation().Vector().GetSafeNormal2D();
                    S->Targets.Reset();
                    for (int32 i = 0; i < 3; ++i) S->Targets.Add(Spawn(TEXT("Slime"), Hero->GetActorLocation() + Forward * (650.f + 450.f * i)));
                    Abilities->SetNumericAttributeBase(UVortexAttributes::GetManaAttribute(), Hero->GetVitals()->GetMaxMana());
                }
                if (At(1.f)) { Controller->SetControlRotation(FRotator(-8.f, Controller->GetControlRotation().Yaw, 0.f)); Hero->GetWeapon()->PressSpecial(); }
                if (At(1.35f)) Shot(TEXT("soco_carga"));
                if (At(1.78f)) Shot(TEXT("soco_saida"));
                if (At(2.05f)) Shot(TEXT("soco_voo"));
                if (At(2.4f)) Shot(TEXT("soco_fim"));
                if (At(3.4f))
                {
                    int32 Hit = 0;
                    for (const TWeakObjectPtr<AVortexMonster>& M : S->Targets) Hit += !M.IsValid() || M->IsCreatureDead() || Health(M.Get()) < M->GetMaxHealthValue() ? 1 : 0;
                    Check(Hit >= 2, FString::Printf(TEXT("Soco Sério acertou %d de 3 gotas na fila"), Hit));
                    for (const TWeakObjectPtr<AVortexMonster>& M : S->Targets) if (M.IsValid()) M->Destroy();
                    Next(3);
                }
                return true;
            }
            case 3:   // primeira pessoa
            {
                if (At(.1f)) { Hero->ToggleFirstPerson(); Controller->SetControlRotation(FRotator(-5.f, Controller->GetControlRotation().Yaw + 40.f, 0.f)); }
                if (At(1.f)) { Check(Hero->IsFirstPerson(), TEXT("primeira pessoa ligada")); Shot(TEXT("primeira_pessoa")); }
                if (At(1.3f)) Hero->BeginPrimaryAttack();
                if (At(1.42f)) Shot(TEXT("primeira_pessoa_golpe"));
                if (At(1.9f)) { if (UVortexWeaponDefinition* Sword = UVortexWeaponComponent::LoadType(NAME_None)) Hero->GetWeapon()->Equip(Sword); }
                if (At(2.4f)) Shot(TEXT("primeira_pessoa_espada"));
                if (At(2.6f)) Hero->BeginPrimaryAttack();
                if (At(2.75f)) Shot(TEXT("primeira_pessoa_espada_golpe"));
                if (At(3.2f)) Controller->SetControlRotation(FRotator(-55.f, Controller->GetControlRotation().Yaw, 0.f));
                if (At(3.7f)) Shot(TEXT("primeira_pessoa_baixo"));
                if (At(4.f)) Hero->ToggleFirstPerson();
                if (At(5.f)) { Check(!Hero->IsFirstPerson(), TEXT("voltou para a terceira pessoa")); Shot(TEXT("terceira_pessoa_de_volta")); Next(4); }
                return true;
            }
            case 4:   // procura um lago fundo perto (água sobre o chão > 2,5 m)
            {
                const UVortexWaterSubsystem* Water = World->GetSubsystem<UVortexWaterSubsystem>();
                float BestDepth = 0.f;
                for (float R = 2000.f; R <= 40000.f && BestDepth < 260.f; R += 1500.f)
                {
                    for (int32 k = 0; k < 24 && BestDepth < 260.f; ++k)
                    {
                        const float A = 2.f * UE_PI * k / 24.f;
                        const FVector P(Open.X + FMath::Cos(A) * R, Open.Y + FMath::Sin(A) * R, 0.f);
                        float Surface = 0.f;
                        const float Ground = Streamer->GroundAt(P.X, P.Y);
                        if (Ground < -299.f || !Water || !Water->SurfaceAt(FVector(P.X, P.Y, Ground + 1.f), Surface)) continue;
                        if (Surface - Ground > BestDepth) { BestDepth = Surface - Ground; S->Lake = FVector(P.X, P.Y, Surface); }
                    }
                }
                Check(BestDepth >= 200.f, FString::Printf(TEXT("lago encontrado (%.0f cm de fundura)"), BestDepth));
                if (BestDepth < 200.f) { Next(9); return true; }
                Next(5);
                return true;
            }
            case 5:   // espera o chão do lago carregar e cai de 6 m
            {
                float Ground = 0.f;
                Hero->SetActorLocation(S->Lake + FVector(0.f, 0.f, 650.f), false, nullptr, ETeleportType::TeleportPhysics);
                Hero->GetCharacterMovement()->Velocity = FVector::ZeroVector;
                if (!Streamer->GroundReadyAt(S->Lake.X, S->Lake.Y, Ground) || Streamer->PendingChunks() > 0 || T < 2.f) return true;
                Controller->SetControlRotation(FRotator(-25.f, 30.f, 0.f));
                Next(6);
                return true;
            }
            case 6:   // queda e respingo
            {
                if (Hero->GetVortexMovement()->IsSwimmingInWater() && S->Before >= 0.f)
                {
                    S->Before = -1.f;
                    S->Time = 0.f;
                    Check(true, TEXT("entrou nadando depois da queda"));
                }
                if (S->Before < 0.f && At(.12f)) Shot(TEXT("agua_respingo"));
                if (S->Before < 0.f && At(.45f)) Shot(TEXT("agua_respingo_2"));
                if (S->Before < 0.f && T > 1.2f) Next(7);
                if (S->Before >= 0.f && T > 6.f) { Check(false, TEXT("não entrou na água")); Next(9); }
                return true;
            }
            case 7:   // nada para a frente
            {
                Hero->AddMovementInput(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f).Vector(), 1.f);
                if (At(1.6f)) Shot(TEXT("agua_nadando"));
                if (At(2.3f)) { Controller->SetControlRotation(FRotator(-15.f, Controller->GetControlRotation().Yaw + 90.f, 0.f)); }
                if (At(3.f)) Shot(TEXT("agua_nadando_lado"));
                if (At(3.4f)) Next(8);
                return true;
            }
            case 8:   // boia parado
            {
                if (At(1.5f)) Shot(TEXT("agua_boiando"));
                if (At(2.f)) { Check(Hero->GetVortexMovement()->IsSwimmingInWater(), TEXT("boiando parado")); Next(9); }
                return true;
            }
            default:
                for (const FString& Line : S->Lines) UE_LOG(LogTemp, Display, TEXT("VORTEX_FEATURE_SHOTS %s"), *Line);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_FEATURE_SHOTS_%s"), S->bPass ? TEXT("PASS") : TEXT("FAIL"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
        }));
    }
}
#endif

#if !UE_BUILD_SHIPPING
#include "World/VortexCaldera.h"
#include "Creatures/VortexVolcanicBoss.h"

// O chefe da Caldeira (-VortexBossShots, Tools/RunBossShots.ps1; fotos Saved/Screenshots/chefe_*.png):
// céu vermelho, o despertar, as fases 2 e 3 (vida forçada), as poças de lava e a queimadura.
namespace VortexBossShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState
        {
            int32 Step = 0; float Time = 0.f, Before = 0.f; bool bPass = true; TArray<FString> Lines; TWeakObjectPtr<AVortexMonster> Boss;
            // 02/11: as 12 habilidades do golem vulcânico, uma a uma
            TArray<FName> Abilities; int32 Ability = 0; float NextAt = 0.f, ShotAt = -1.f;
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
            UVortexCreatureSubsystem* Creatures = World->GetSubsystem<UVortexCreatureSubsystem>();
            UVortexCalderaSubsystem* Caldera = World->GetSubsystem<UVortexCalderaSubsystem>();
            UAbilitySystemComponent* Abilities = Hero->GetAbilitySystemComponent();
            S->Time += Delta;
            const float T = S->Time;
            auto At = [&](float When) { return T >= When && T - Delta < When; };
            auto Shot = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/chefe_%s.png"), Name), true, false); };
            auto Check = [&](bool bOk, const FString& What) { S->Lines.Add(FString::Printf(TEXT("%s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What)); S->bPass &= bOk; };
            auto Next = [&](int32 Step) { S->Step = Step; S->Time = 0.f; };
            const FVector Center(UVortexCalderaSubsystem::CenterX, UVortexCalderaSubsystem::CenterY, 0.f);
            const FVector Stand = Center + FVector(-2300.f, -600.f, 0.f);
            AVortexMonster* Boss = S->Boss.Get();
            auto Face = [&](const FVector& Where, float Pitch) { Controller->SetControlRotation(FRotator(Pitch, (Where - Hero->GetActorLocation()).Rotation().Yaw, 0.f)); };
            auto Keep = [&]() { Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Hero->GetVitals()->GetMaxHealth()); };
            switch (S->Step)
            {
            case 0:
            {
                float Ground = 0.f;
                Hero->SetActorLocation(FVector(Stand.X, Stand.Y, Streamer->GroundAt(Stand.X, Stand.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
                if (!Streamer->GroundReadyAt(Stand.X, Stand.Y, Ground) || !Streamer->GroundReadyAt(Center.X, Center.Y, Ground) || Streamer->PendingChunks() > 0 || T < 4.f) return true;
                // o chefe do posto da Caldeira (ou um de teste, se o posto ainda não nasceu)
                for (TActorIterator<AVortexMonster> It(World); It; ++It) if (It->IsBoss()) S->Boss = *It;
                if (!S->Boss.IsValid() && Creatures) Creatures->DebugSpawnNear(TEXT("RuneColossus"), Center, 12);
                for (TActorIterator<AVortexMonster> It(World); It; ++It) if (It->IsBoss()) S->Boss = *It;
                Check(S->Boss.IsValid(), TEXT("Azhûr na Caldeira"));
                Next(S->Boss.IsValid() ? 1 : 9);
                return true;
            }
            case 1:   // céu vermelho, a arena e o chefe dormindo (de longe)
            {
                Keep();
                if (Boss) Face(Boss->GetActorLocation(), 2.f);
                if (At(3.f))
                {
                    Check(Caldera && Caldera->RedSkyAt(Hero->GetActorLocation()) > .3f, TEXT("céu vermelho na zona"));
                    Check(Caldera && Caldera->GetArena() && Caldera->GetArena()->PoolCount() >= 8, FString::Printf(TEXT("arena de lava (%d poças)"), Caldera && Caldera->GetArena() ? Caldera->GetArena()->PoolCount() : 0));
                    Shot(TEXT("caldeira"));
                }
                if (At(3.5f)) { Face(Center, -25.f); }
                if (At(4.2f)) { Shot(TEXT("arena")); Next(2); }
                return true;
            }
            case 2:   // chega perto: desperta
            {
                Keep();
                if (!Boss) { Next(9); return true; }
                Face(Boss->GetActorLocation(), 4.f);
                if (At(.1f)) Hero->SetActorLocation(Boss->GetActorLocation() + FVector(-1500.f, -300.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                if (At(1.4f)) Shot(TEXT("desperta"));
                if (At(4.f)) { Check(Boss->GetBossPhase() == 1, TEXT("acordou na fase 1")); Shot(TEXT("fase1")); }
                if (At(4.5f)) Next(3);
                return true;
            }
            case 3:   // 02/11: as 12 habilidades do golem vulcânico, uma de cada vez, com a foto no impacto
            {
                Keep();
                UVortexVolcanicBoss* Brain = Boss ? Boss->GetVolcanic() : nullptr;
                if (!Brain) { Check(false, TEXT("Azhûr com o cérebro de golem vulcânico")); Next(4); return true; }
                if (S->Abilities.IsEmpty())
                {
                    S->Abilities = Brain->AbilityNames();
                    Check(S->Abilities.Num() == 12, FString::Printf(TEXT("12 habilidades no azhur.json (%d)"), S->Abilities.Num()));
                    S->NextAt = 1.f;
                }
                // o herói de lado, a 10 m: a câmera pega o chefe e as marcas no chão
                const FVector Spot = Boss->GetActorLocation() + FVector(-1500.f, -1000.f, 0.f);
                if (FVector::Dist2D(Hero->GetActorLocation(), Spot) > 250.f)
                    Hero->SetActorLocation(FVector(Spot.X, Spot.Y, Streamer->GroundAt(Spot.X, Spot.Y) + 120.f), false, nullptr, ETeleportType::TeleportPhysics);
                Face(Boss->GetActorLocation() + FVector(0.f, 0.f, 150.f), -14.f);
                if (S->ShotAt > 0.f && T >= S->ShotAt)
                {
                    Shot(*FString::Printf(TEXT("hab_%s"), *S->Abilities[S->Ability - 1].ToString()));
                    S->ShotAt = -1.f;
                }
                if (T >= S->NextAt)
                {
                    if (S->Ability >= S->Abilities.Num()) { Next(5); return true; }
                    const FName Id = S->Abilities[S->Ability++];
                    const bool bUsed = Brain->DebugUse(Id);
                    const EVolcanicState St = Brain->GetState();
                    Check(bUsed && (St == EVolcanicState::Attack || St == EVolcanicState::Ability), FString::Printf(TEXT("habilidade %s começou (estado %d)"), *Id.ToString(), int32(St)));
                    S->ShotAt = T + Brain->AbilityValue(Id, TEXT("impact"), 1.f) + .08f;
                    S->NextAt = T + Brain->AbilityValue(Id, TEXT("duration"), 2.f) + 1.2f;
                }
                return true;
            }
            case 5:   // a troca de fase (40%) e o atordoamento
            {
                Keep();
                if (!Boss || !Boss->GetVolcanic()) { Next(4); return true; }
                UVortexVolcanicBoss* Brain = Boss->GetVolcanic();
                UAbilitySystemComponent* BossAbilities = Boss->GetAbilitySystemComponent();
                const float BossMax = BossAbilities->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute());
                Face(Boss->GetActorLocation() + FVector(0.f, 0.f, 150.f), -14.f);
                if (At(.2f))
                {
                    Check(Boss->GetBossPhase() == 1, TEXT("ainda na fase 1 antes dos 40%"));
                    BossAbilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), BossMax * .38f);
                }
                if (At(1.2f))
                {
                    Check(Boss->GetBossPhase() == 2 && Brain->GetState() == EVolcanicState::PhaseTransition,
                        FString::Printf(TEXT("fase 2 com 40%% da vida, na transição (estado %d)"), int32(Brain->GetState())));
                    Shot(TEXT("fase2_transicao"));
                }
                if (At(5.f)) Shot(TEXT("fase2_luta"));
                // o atordoamento: um tranco de dano maior que a resistência
                if (At(7.f)) BossAbilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), BossMax * .25f);
                if (At(7.8f))
                {
                    Check(Brain->GetState() == EVolcanicState::Stunned, FString::Printf(TEXT("atordoado depois de um tranco de dano (estado %d)"), int32(Brain->GetState())));
                    Shot(TEXT("atordoado"));
                }
                if (At(11.f)) Shot(TEXT("fase2_luta2"));
                if (At(11.5f)) Next(4);
                return true;
            }
            case 4:   // pisa numa poça de lava: perde vida e pega fogo
            {
                AVortexLavaArena* Arena = Caldera ? Caldera->GetArena() : nullptr;
                FVector Pool; float Radius = 0.f;
                bool bFound = false;
                for (int32 i = 0; Arena && i < Arena->PoolCount() && !bFound; ++i) bFound = Arena->PoolAt(i, Pool, Radius) && Radius > 200.f;
                if (!bFound) { Check(false, TEXT("poça para pisar")); Next(9); return true; }
                if (At(.1f))
                {
                    Keep();
                    Hero->SetActorLocation(Pool + FVector(0.f, 0.f, Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f), false, nullptr, ETeleportType::TeleportPhysics);
                    S->Before = Hero->GetVitals()->GetHealth();
                    Face(Pool + FVector(400.f, 0.f, 0.f), -30.f);
                }
                if (At(1.6f))
                {
                    Check(Hero->GetVitals()->GetHealth() < S->Before, FString::Printf(TEXT("a lava fere (%.0f -> %.0f)"), S->Before, Hero->GetVitals()->GetHealth()));
                    Check(Hero->GetSheet()->BurnRemaining() > 0.f, TEXT("queimando depois de pisar na lava"));
                    Shot(TEXT("queimando"));
                }
                // de perto, de cima: a correnteza, as placas, as bolhas e a fumaça
                if (At(2.f))
                {
                    Keep();
                    Hero->SetActorLocation(Pool + FVector(-Radius - 260.f, 0.f, Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 40.f), false, nullptr, ETeleportType::TeleportPhysics);
                    Face(Pool, -42.f);
                }
                if (At(3.2f)) Shot(TEXT("lava_perto"));
                if (At(3.9f)) Shot(TEXT("lava_perto_2"));
                if (At(4.2f)) Next(9);
                return true;
            }
            default:
                for (const FString& Line : S->Lines) UE_LOG(LogTemp, Display, TEXT("VORTEX_BOSS_SHOTS %s"), *Line);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_BOSS_SHOTS_%s"), S->bPass ? TEXT("PASS") : TEXT("FAIL"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
        }));
    }
}
#endif

#if !UE_BUILD_SHIPPING
#include "Framework/VortexAwakening.h"
#include "Combat/VortexClassSkills.h"

// O Despertar e as habilidades de classe (-VortexClassShots, Tools/RunClassShots.ps1; fotos classe_*.png):
// o herói chega ao nível 10, a cena das cartas abre e vira as seis; escolhe a mais rara; depois testa as
// 4 habilidades de cinco classes que juntas cobrem os 13 tipos (contra gotas de jade à frente).
// Com -VortexSkillAudit pula o Despertar e lança as 4 de TODAS as classes (fotos auditoria_*.png), com alvos novos
// a cada uma, e mede o efeito: dano nos alvos, bênção, cura e quanto o herói andou (VORTEX_SKILL_AUDIT).
namespace VortexClassShots
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState
        {
            int32 Step = 0, Class = 0, Slot = 0, Casts = 0; float Time = 0.f; bool bPass = true; TArray<FString> Lines; TArray<TWeakObjectPtr<AVortexMonster>> Targets; float Before = 0.f;
            bool bAudit = false; TArray<FString> Classes; float HeroHealth = 0.f; FVector HeroAt = FVector::ZeroVector; int32 Silent = 0; TMap<TWeakObjectPtr<AVortexMonster>, float> Health;
        };
        TSharedRef<FState> S = MakeShared<FState>();
        S->bAudit = FParse::Param(FCommandLine::Get(), TEXT("VortexSkillAudit"));
        if (S->bAudit)
        {
            for (const TPair<FName, TArray<FVortexClassSkill>>& Pair : FVortexRpgCatalog::Get().ClassSkills) S->Classes.Add(Pair.Key.ToString());
            S->Classes.Sort();
        }
        else S->Classes = { TEXT("knight"), TEXT("ranger"), TEXT("santo_guerreiro"), TEXT("sorcerer"), TEXT("arcane_blade") };
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
            UAbilitySystemComponent* Abilities = Hero->GetAbilitySystemComponent();
            UVortexCreatureSubsystem* Creatures = World->GetSubsystem<UVortexCreatureSubsystem>();
            S->Time += Delta;
            const float T = S->Time;
            auto At = [&](float When) { return T >= When && T - Delta < When; };
            auto Shot = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/%s_%s.png"), S->bAudit ? TEXT("auditoria") : TEXT("classe"), *Name), true, false); };
            auto Check = [&](bool bOk, const FString& What) { S->Lines.Add(FString::Printf(TEXT("%s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What)); S->bPass &= bOk; };
            auto Next = [&](int32 Step) { S->Step = Step; S->Time = 0.f; };
            const FVector2D Open(-30000.f, -8000.f);
            switch (S->Step)
            {
            case 0:
            {
                float Ground = 0.f;
                Hero->SetActorLocation(FVector(Open, Streamer->GroundAt(Open.X, Open.Y) + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
                if (!Streamer->GroundReadyAt(Open.X, Open.Y, Ground) || Streamer->PendingChunks() > 0 || T < 3.f) return true;
                Controller->SetControlRotation(FRotator(-10.f, 0.f, 0.f));
                if (S->bAudit)
                {
                    // a auditoria pula a cena: classe direto e nível 25 (as 4 liberadas)
                    Sheet->DebugSetClass(FName(*S->Classes[0]));
                    Sheet->AddXp(FVortexRpgCatalog::XpForLevel(25) - Sheet->GetXp() + 5);
                    Next(3);
                    return true;
                }
                // nível 10: as cartas saem e a cena abre (com pontos postos em Ataque e Vida)
                Sheet->ServerSpendPoint(0);
                Sheet->AddXp(FVortexRpgCatalog::XpForLevel(10) - Sheet->GetXp() + 5);
                Next(1);
                return true;
            }
            case 1:   // a cena: título, cartas chegando, virando, a lendária...
            {
                if (T > 3.f && !VortexAwakening::IsOpen() && Sheet->GetClassId().IsNone()) { Check(false, TEXT("a cena do Despertar abriu")); Next(3); return true; }
                if (At(1.2f)) { Check(Sheet->GetClassCards().Num() == 6, FString::Printf(TEXT("seis cartas no nível %d"), Sheet->GetLevel())); Check(VortexAwakening::IsOpen(), TEXT("a cena do Despertar abriu")); }
                for (float When : { 1.6f, 2.6f, 4.2f, 6.f, 8.f, 10.5f, 13.5f, 16.5f }) if (At(When)) Shot(FString::Printf(TEXT("despertar_%02d"), FMath::RoundToInt(When * 10.f)));
                if (At(17.f)) { VortexAwakening::DebugPick(VortexAwakening::DebugRarest()); }
                if (At(17.6f)) Shot(TEXT("despertar_detalhe"));
                if (At(18.f)) VortexAwakening::DebugConfirm();
                if (At(18.7f)) Shot(TEXT("despertar_escolha"));
                if (At(21.5f)) { Check(!Sheet->GetClassId().IsNone(), FString::Printf(TEXT("despertou como %s"), *Sheet->GetClassId().ToString())); Next(2); }
                return true;
            }
            case 2:   // a janela do herói, aba Classes: no nível 10 só a 1ª habilidade; as outras trancadas (15, 20, 25)
            {
                if (At(.1f))
                {
                    const UVortexClassSkills* Skills = Hero->GetClassSkills();
                    Check(Skills && Skills->IsUnlocked(0) && !Skills->IsUnlocked(1) && !Skills->IsUnlocked(3),
                        FString::Printf(TEXT("nível %d: só a 1ª habilidade liberada"), Sheet->GetLevel()));
                }
                if (At(.2f)) VortexHeroWindow::Open(Hero, 3);
                if (At(1.2f)) Shot(TEXT("aba_classes"));
                if (At(1.4f)) Shot(TEXT("barra_trancada"));
                if (At(1.6f))
                {
                    VortexHeroWindow::Close();
                    // nível 25: as quatro liberadas (o resto do teste lança todas)
                    Sheet->AddXp(FVortexRpgCatalog::XpForLevel(25) - Sheet->GetXp() + 5);
                    const UVortexClassSkills* Skills = Hero->GetClassSkills();
                    Check(Skills && Skills->IsUnlocked(1) && Skills->IsUnlocked(3), FString::Printf(TEXT("nível %d: as 4 habilidades liberadas"), Sheet->GetLevel()));
                    Next(3);
                }
                return true;
            }
            case 3:   // uma classe: alvos à frente, as 4 habilidades
            {
                if (S->Class >= S->Classes.Num()) { Next(9); return true; }
                if (At(.1f))
                {
                    Sheet->DebugSetClass(FName(*S->Classes[S->Class]));
                    for (const TWeakObjectPtr<AVortexMonster>& M : S->Targets) if (M.IsValid()) M->Destroy();
                    S->Targets.Reset();
                    // auditoria: meia vida já aqui (para ver a cura), antes do lançamento
                    if (S->bAudit) Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Hero->GetVitals()->GetMaxHealth() * .5f);
                    const FVector Forward = FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f).Vector();
                    for (int32 i = 0; i < 3; ++i)
                    {
                        const FVector P = Hero->GetActorLocation() + Forward * (550.f + 220.f * i) + FRotator(0.f, 90.f, 0.f).RotateVector(Forward) * ((i - 1) * 180.f);
                        if (Creatures) Creatures->DebugSpawnNear(TEXT("Slime"), P, 10);
                    }
                    for (TActorIterator<AVortexMonster> It(World); It; ++It) if (!It->IsCreatureDead() && FVector::Dist(It->GetActorLocation(), Hero->GetActorLocation()) < 1500.f) S->Targets.Add(*It);
                }
                if (T > .6f) { Next(S->bAudit ? 5 : 4); }
                return true;
            }
            case 4:   // lança a habilidade do espaço e fotografa
            {
                UVortexClassSkills* Skills = Hero->GetClassSkills();
                if (At(.05f))
                {
                    Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), Hero->GetVitals()->GetMaxHealth());
                    Abilities->SetNumericAttributeBase(UVortexAttributes::GetManaAttribute(), Hero->GetVitals()->GetMaxMana());
                    Skills->DebugResetCooldowns();
                    // mira no alvo vivo mais perto
                    AVortexMonster* Aim = nullptr;
                    for (const TWeakObjectPtr<AVortexMonster>& M : S->Targets) if (M.IsValid() && !M->IsCreatureDead()) { Aim = M.Get(); break; }
                    if (Aim) Controller->SetControlRotation(FRotator(-6.f, (Aim->GetActorLocation() - Hero->GetActorLocation()).Rotation().Yaw, 0.f));
                }
                if (At(.15f))
                {
                    const FVortexClassSkill* Skill = Skills->Skill(S->Slot);
                    Check(Skill && Skills->CanCast(S->Slot), FString::Printf(TEXT("%s %d: %s"), *S->Classes[S->Class], S->Slot + 1, Skill ? *Skill->Name : TEXT("?")));
                    Skills->Press(S->Slot);
                }
                if (At(.5f)) Shot(FString::Printf(TEXT("poder_%s_%d"), *S->Classes[S->Class], S->Slot + 1));
                if (At(.95f))
                {
                    Check(Skills->CooldownLeft(S->Slot) > 0.f, FString::Printf(TEXT("%s %d lançada (recarga correndo)"), *S->Classes[S->Class], S->Slot + 1));
                    Shot(FString::Printf(TEXT("poder_%s_%d_b"), *S->Classes[S->Class], S->Slot + 1));
                }
                if (T > 1.6f)
                {
                    if (++S->Slot >= 4) { S->Slot = 0; ++S->Class; Next(3); }
                    else S->Time = 0.f;
                }
                return true;
            }
            case 5:   // auditoria: lança, fotografa e mede o efeito (alvos novos a cada habilidade)
            {
                UVortexClassSkills* Skills = Hero->GetClassSkills();
                const FVortexClassSkill* Skill = Skills ? Skills->Skill(S->Slot) : nullptr;
                // os alvos nascem um instante depois do pedido: a medida pega as criaturas vivas perto na hora do lançamento
                auto Alive = [&](const AVortexMonster* M) { return M && !M->IsCreatureDead() ? M->GetHealthFraction() * M->GetMaxHealthValue() : 0.f; };
                auto Nearest = [&]()
                {
                    AVortexMonster* Best = nullptr;
                    for (TActorIterator<AVortexMonster> It(World); It; ++It)
                        if (!It->IsCreatureDead() && (!Best || FVector::DistSquared(It->GetActorLocation(), Hero->GetActorLocation()) < FVector::DistSquared(Best->GetActorLocation(), Hero->GetActorLocation()))) Best = *It;
                    return Best;
                };
                auto HeroHealth = [&]() { return Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()); };
                if (At(.05f))
                {
                    // mana cheia, recargas prontas, mira no alvo vivo mais perto (a meia vida foi posta antes, no passo 3)
                    Abilities->SetNumericAttributeBase(UVortexAttributes::GetManaAttribute(), Hero->GetVitals()->GetMaxMana());
                    Skills->DebugResetCooldowns();
                    if (AVortexMonster* Aim = Nearest()) Controller->SetControlRotation(FRotator(-6.f, (Aim->GetActorLocation() - Hero->GetActorLocation()).Rotation().Yaw, 0.f));
                }
                if (At(.12f))
                {
                    S->Health.Reset();
                    for (TActorIterator<AVortexMonster> It(World); It; ++It)
                        if (FVector::Dist(It->GetActorLocation(), Hero->GetActorLocation()) < 3500.f) S->Health.Add(*It, Alive(*It));
                    S->HeroHealth = HeroHealth();
                    S->HeroAt = Hero->GetActorLocation();
                    Check(Skill && Skills->CanCast(S->Slot), FString::Printf(TEXT("%s %d: %s"), *S->Classes[S->Class], S->Slot + 1, Skill ? *Skill->Name : TEXT("?")));
                    Skills->Press(S->Slot);
                }
                // a foto no meio do efeito (o golpe do céu cai depois do atraso)
                const float ShotAt = .12f + (Skill && Skill->Kind == TEXT("strike") ? Skill->Delay + .12f : .4f);
                if (At(ShotAt)) Shot(FString::Printf(TEXT("%s_%d"), *S->Classes[S->Class], S->Slot + 1));
                if (At(.4f) && Skill)
                {
                    float CastTime = 0.f;
                    const UAnimSequence* Clip = Skills->CastAnimation(CastTime);
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_SKILL_GESTURE %s %d %s anim=%s clip=%s"), *S->Classes[S->Class], S->Slot + 1, *Skill->Id.ToString(), *Skill->Anim.ToString(), *GetNameSafe(Clip));
                }
                if (At(2.2f) && Skill)
                {
                    float Damage = 0.f;
                    for (const TPair<TWeakObjectPtr<AVortexMonster>, float>& H : S->Health) Damage += H.Value - Alive(H.Key.Get());
                    const float Healed = HeroHealth() - S->HeroHealth;
                    const int32 Buffs = Skills->ActiveBuffs().Num();
                    const float Moved = FVector::Dist2D(S->HeroAt, Hero->GetActorLocation());
                    const bool bEffect = Damage > .5f || Buffs > 0 || Healed > 1.f || Moved > 150.f;
                    S->Silent += bEffect ? 0 : 1;
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_SKILL_AUDIT %s %d %s kind=%s element=%s anim=%s dano=%.0f bencaos=%d cura=%.0f andou=%.0f %s"),
                        *S->Classes[S->Class], S->Slot + 1, *Skill->Id.ToString(), *Skill->Kind.ToString(), *Skill->Element.ToString(), *Skill->Anim.ToString(),
                        Damage, Buffs, Healed, Moved, bEffect ? TEXT("ok") : TEXT("SEM_EFEITO"));
                }
                if (T > 2.3f)
                {
                    // volta ao ponto de partida (investidas o levam longe) e põe alvos novos
                    Hero->SetActorLocation(FVector(Open, Streamer->GroundAt(Open.X, Open.Y) + 120.f), false, nullptr, ETeleportType::TeleportPhysics);
                    if (++S->Slot >= 4) { S->Slot = 0; ++S->Class; }
                    Next(3);
                }
                return true;
            }
            default:
                if (S->bAudit) S->Lines.Add(FString::Printf(TEXT("auditoria: %d classes, %d sem efeito medido"), S->Classes.Num(), S->Silent));
                for (const FString& Line : S->Lines) UE_LOG(LogTemp, Display, TEXT("VORTEX_CLASS_SHOTS %s"), *Line);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_CLASS_SHOTS_%s"), S->bPass ? TEXT("PASS") : TEXT("FAIL"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
        }));
    }
}
#endif
