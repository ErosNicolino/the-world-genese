// Golpe de perto de cada arma contra criaturas de verdade (-VortexMeleeSweep, Tools/RunMeleeSweep.ps1).
// Em L_MovementProbe: para cada arma de perto e cada espécie, põe a criatura parada em volta do herói (colada, perto,
// no limite do alcance, de lado e atrás), aperta o golpe uma vez e confere se a vida dela caiu. Cada falha sai com o
// motivo que o TryMeleeHit deu (VORTEX_MELEE_MISS). Termina com VORTEX_MELEE_SWEEP_PASS quando todo golpe que devia
// acertar acertou.
#if !UE_BUILD_SHIPPING
#include "Framework/VortexCreatureSmoke.h"

#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Containers/Ticker.h"
#include "Components/CapsuleComponent.h"
#include "Creatures/VortexMonster.h"
#include "Creatures/VortexMonsterSpecies.h"
#include "Engine/World.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"

namespace VortexMeleeSweep
{
    struct FCase { FName Weapon, Species; float Distance, Angle; bool bShould; bool bDecoy = false; };
    struct FState
    {
        TArray<FCase> Cases;
        int32 Index = 0, Hits = 0, Misses = 0, Wrong = 0;
        double Step = 0.0;
        float Before = 0.f;
        TWeakObjectPtr<AVortexMonster> Target, Decoy;
        int32 Phase = 0;
    };

    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        TSharedRef<FState> S = MakeShared<FState>();
        // a distância é da borda do herói até a borda da criatura (cm); ângulo a partir da frente do herói
        static const FName Weapons[] = { TEXT("espada_de_cinzas"), TEXT("martelo_de_bram"), TEXT("adagas_gemeas"), TEXT("cajado_de_ilsa") };
        static const FName Species[] = { TEXT("Slime"), TEXT("Goblin"), TEXT("Golem"), TEXT("Mushroom") };
        struct FSpot { float Distance, Angle; bool bShould; };
        static const FSpot Spots[] = { { 0.f, 0.f, true }, { 25.f, 0.f, true }, { 60.f, 0.f, true }, { 25.f, 60.f, true }, { 25.f, 120.f, true }, { 25.f, 180.f, true } };
        for (const FName W : Weapons) for (const FName Sp : Species)
        {
            // o cajado atira para a mira (a câmera, que aqui olha para a frente): só os da frente contam
            const bool bRanged = W == TEXT("cajado_de_ilsa");
            for (const FSpot& P : Spots) S->Cases.Add({ W, Sp, P.Distance, P.Angle, P.bShould && (!bRanged || P.Angle < 30.f) });
            // um sapo preso no foco a 4 m na frente e este colado atrás-de-lado: o golpe tem de ir no de perto
            if (!bRanged) S->Cases.Add({ W, Sp, 0.f, 150.f, true, true });
        }
        TWeakObjectPtr<AGameModeBase> WeakMode = Mode;
        TWeakObjectPtr<APlayerController> Controller = Player;
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakMode, Controller, S](float) -> bool
        {
            UWorld* World = WeakMode.IsValid() ? WeakMode->GetWorld() : nullptr;
            AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
            if (!World || !Hero) return true;
            const double Now = World->GetTimeSeconds();
            if (Now < S->Step) return true;
            if (S->Index >= S->Cases.Num())
            {
                if (AVortexMonster* Old = S->Target.Get()) Old->Destroy();
                UE_LOG(LogTemp, Display, TEXT("VORTEX_MELEE_SWEEP_%s acertos=%d erros=%d"), S->Misses == 0 ? TEXT("PASS") : TEXT("FAIL"), S->Hits, S->Misses);
                FPlatformMisc::RequestExit(false);
                return false;
            }
            const FCase& C = S->Cases[S->Index];
            UAbilitySystemComponent* HeroAbilities = Hero->GetAbilitySystemComponent();
            if (S->Phase == 0)
            {
                // arma, herói parado olhando para +X, criatura parada no lugar do caso
                UVortexWeaponComponent* Weapon = Hero->GetWeapon();
                UVortexWeaponDefinition* Def = UVortexWeaponComponent::LoadType(C.Weapon);
                if (Weapon && Def && Weapon->GetDefinition() != Def) Weapon->Equip(Def);
                if (AVortexMonster* Old = S->Target.Get()) Old->Destroy();
                if (AVortexMonster* Old = S->Decoy.Get()) Old->Destroy();
                HeroAbilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(),
                    HeroAbilities->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute()));
                Hero->SetActorRotation(FRotator::ZeroRotator);
                if (Controller.IsValid()) Controller->SetControlRotation(FRotator::ZeroRotator);
                const FVortexSpeciesDef* D = FVortexSpeciesDef::Find(C.Species);
                const float Reach = Hero->GetCapsuleComponent()->GetScaledCapsuleRadius() + (D ? D->Radius : 40.f) + C.Distance;
                const FVector Dir = FRotator(0.f, C.Angle, 0.f).Vector();
                const FVector At = Hero->GetActorLocation() + Dir * Reach + FVector(0.f, 0.f, (D ? D->Height * .5f : 60.f) - Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f);
                FActorSpawnParameters Params;
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                auto Spawn = [World](FName Sp, const FVector& Where, const FRotator& Facing)
                {
                    AVortexMonster* New = World->SpawnActorDeferred<AVortexMonster>(AVortexMonster::StaticClass(), FTransform(Facing, Where), nullptr, nullptr,
                        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
                    if (!New) return New;
                    New->Setup(Sp, 1);
                    New->FinishSpawning(FTransform(Facing, Where));
                    New->GetCharacterMovement()->DisableMovement();
                    return New;
                };
                if (C.bDecoy)
                {
                    S->Decoy = Spawn(TEXT("Frog"), Hero->GetActorLocation() + FVector(400.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f));
                    Hero->PreviewMeleeTarget();   // o lobo fica preso no foco (o único por perto)
                }
                AVortexMonster* M = World->SpawnActorDeferred<AVortexMonster>(AVortexMonster::StaticClass(), FTransform((-Dir).Rotation(), At), nullptr, nullptr,
                    ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
                if (M)
                {
                    M->Setup(C.Species, 1);
                    M->FinishSpawning(FTransform((-Dir).Rotation(), At));
                    M->GetCharacterMovement()->DisableMovement();
                }
                S->Target = M;
                S->Phase = 1;
                S->Step = Now + .5;
                return true;
            }
            AVortexMonster* M = S->Target.Get();
            UAbilitySystemComponent* Abilities = M ? M->GetAbilitySystemComponent() : nullptr;
            if (S->Phase == 1)
            {
                // parada de novo (o golpe pode ter sido empurrado) e o golpe
                if (M) { M->GetCharacterMovement()->DisableMovement(); }
                if (AVortexMonster* D = S->Decoy.Get()) D->GetCharacterMovement()->DisableMovement();
                S->Before = Abilities ? Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) : 0.f;
                Hero->BeginPrimaryAttack();   // como o clique do jogador: vira para o alvo e golpeia
                S->Phase = 2;
                S->Step = Now + 1.1;
                return true;
            }
            const float After = Abilities ? Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) : S->Before;
            const bool bHit = After < S->Before;
            const float Gap = M ? FVector::Dist2D(M->GetActorLocation(), Hero->GetActorLocation()) - Hero->GetCapsuleComponent()->GetScaledCapsuleRadius()
                - M->GetCapsuleComponent()->GetScaledCapsuleRadius() : -1.f;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_MELEE_SWEEP %s %s borda=%.0f (agora %.0f) angulo=%.0f%s %s vida %.1f -> %.1f virou=%.0f"),
                *C.Weapon.ToString(), *C.Species.ToString(), C.Distance, Gap, C.Angle, C.bDecoy ? TEXT(" (sapo preso no foco)") : TEXT(""), bHit ? TEXT("ACERTOU") : TEXT("ERROU"), S->Before, After,
                Hero->GetActorRotation().Yaw);
            if (bHit) ++S->Hits;
            else if (C.bShould) ++S->Misses;
            ++S->Index;
            S->Phase = 0;
            return true;
        }));
    }
}
#endif
