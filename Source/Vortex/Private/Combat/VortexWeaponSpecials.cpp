// Os secundários (botão direito) e os golpes normais de jeito próprio das armas revisadas
// (Docs/02_Combate/REVISAO_ARMAS.md). Os números vêm de VortexWeaponKit; os desenhos, de AVortexWeaponFx.
// Tudo no servidor; as poses vão pelo AttackState (o clipe Vx_<Família>_Special em todas as máquinas).

#include "Combat/VortexWeaponComponent.h"

#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexClassSkills.h"
#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexProjectile.h"
#include "Combat/VortexSlashWave.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Combat/VortexWeaponFx.h"
#include "Combat/VortexWeaponKit.h"
#include "Combat/VortexSwords.h"
#include "Creatures/VortexCreature.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexCharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexSocial.h"
#include "TimerManager.h"

namespace VortexVolley { extern float Yaw; }   // o giro de cada flecha do leque (VortexWeaponComponent::LaunchProjectile)

namespace
{
    const FLinearColor Fire(1.f, .45f, .12f), Holy(1.f, .82f, .35f), Acid(.5f, 1.f, .2f), Rune(.55f, .45f, 1.f), Nature(.35f, 1.f, .35f);

    /** Chão embaixo de um ponto (o que a mira enxerga pode ser uma árvore ou o ar). */
    FVector Ground(const UWorld* World, const FVector& At, const AActor* Ignore)
    {
        FHitResult Hit;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexKitGround), false, Ignore);
        if (World->LineTraceSingleByChannel(Hit, At + FVector(0.f, 0.f, 300.f), At - FVector(0.f, 0.f, 3000.f), ECC_WorldStatic, Query)) return Hit.ImpactPoint;
        return At;
    }

    /** 07/11: o que o texto do poder promete além do dano (Data/Rpg/swords.json): "dot" (veneno, queimadura: fração do
     *  golpe por segundo, por "dot_time"), "slow" (a velocidade que sobra, por "slow_time") e "drain" (bebe a vida). */
    void PowerTouch(AVortexCharacter* Hero, const VortexWeaponKit::FKit* Kit, const TSet<TWeakObjectPtr<AActor>>& Struck, float Scale)
    {
        if (!Hero || !Kit || Struck.IsEmpty() || !Hero->GetSheet()) return;
        // o golpe que o poder deu (o dano do herói x a força do poder): o veneno e o beber a vida saem dele
        bool bCrit = false;
        const float Damage = Hero->GetSheet()->OutgoingDamage(1.f, EVortexDamageKind::Melee, bCrit) * Scale;
        const float Dot = Kit->Value(TEXT("dot"), 0.f), Slow = Kit->Value(TEXT("slow"), 1.f), Drain = Kit->Value(TEXT("drain"), 0.f);
        for (const TWeakObjectPtr<AActor>& Weak : Struck)
            if (UVortexCombatComponent* Their = Weak.IsValid() ? Weak->FindComponentByClass<UVortexCombatComponent>() : nullptr)
            {
                if (Dot > 0.f) Their->ApplyDot(Damage * Dot, Kit->Value(TEXT("dot_time"), 3.f), Hero);
                if (Slow < 1.f) Their->ApplySlow(Slow, Kit->Value(TEXT("slow_time"), 2.f));
            }
        if (Drain > 0.f && Hero->GetSheet()) Hero->GetSheet()->HealBy(Damage * Drain * Struck.Num());
    }
}

void UVortexWeaponComponent::KitSound(FName Sound, const FVector& At, float Volume, float Jitter)
{
    MulticastKitSound(Sound, At, Volume, Jitter);
}

void UVortexWeaponComponent::MulticastKitSound_Implementation(FName Sound, FVector_NetQuantize At, float Volume, float Jitter)
{
    VortexAudio::Play(this, Sound, At, Volume, Jitter);
}

float UVortexWeaponComponent::PrimaryDamage() const
{
    return EquippedDefinition && EquippedDefinition->Swings.Num() ? EquippedDefinition->Swings[0].Damage : 10.f;
}

float UVortexWeaponComponent::DrawCharge() const
{
    if (!bDrawing || !GetWorld()) return 0.f;
    const AGameStateBase* State = GetWorld()->GetGameState();
    const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    const VortexWeaponKit::FKit* Kit = EquippedDefinition ? VortexWeaponKit::Find(EquippedDefinition->WeaponId) : nullptr;
    return FMath::Clamp(float(Now - DrawStartServerTime) / (Kit ? Kit->Value(TEXT("full"), .8f) : .8f), 0.f, 1.f);
}

void UVortexWeaponComponent::Later(float Seconds, TFunction<void()> Run)
{
    FTimerHandle& Handle = KitTimers.AddDefaulted_GetRef();
    GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, MoveTemp(Run)), FMath::Max(.01f, Seconds), false);
}

bool UVortexWeaponComponent::SpendMana(float Amount, const TCHAR* What)
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    UAbilitySystemComponent* ASC = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
    if (!ASC) return false;
    if (Amount <= 0.f) return true;
    if (ASC->GetNumericAttribute(UVortexAttributes::GetManaAttribute()) < Amount)
    {
        if (Owner->GetSheet()) Owner->GetSheet()->PushNotice(FString::Printf(TEXT("Mana insuficiente para %s"), What));
        return false;
    }
    ASC->ApplyModToAttribute(UVortexAttributes::GetManaAttribute(), EGameplayModOp::Additive, -Amount);
    return true;
}

void UVortexWeaponComponent::StartKitCooldown(float Seconds)
{
    const AGameStateBase* State = GetWorld()->GetGameState();
    const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    SpecialReadyServerTime = float(Now + Seconds * GetCooldownScale());
    if (GetOwner()) GetOwner()->ForceNetUpdate();
}

void UVortexWeaponComponent::KitPose(int32 Pose, float Seconds)
{
    bSpecialActive = true;
    ++AttackState.Serial;
    AttackState.SwingIndex = Pose;
    AttackState.ServerStartTime = GetWorld()->GetTimeSeconds();
    if (GetOwner()) GetOwner()->ForceNetUpdate();
    if (Seconds > 0.f) GetWorld()->GetTimerManager().SetTimer(SpecialFinishTimer, this, &UVortexWeaponComponent::FinishSpecial, Seconds, false);
}

bool UVortexWeaponComponent::BeginKitSpecial()
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    const VortexWeaponKit::FKit* Kit = EquippedDefinition ? VortexSwords::SpecialKit(GetOwner(), EquippedDefinition) : nullptr;
    if (!Owner || !Kit || Kit->Special.IsNone() || Kit->Special == TEXT("seriouspunch") || Kit->Special == TEXT("shield")) return false;
    FName S = Kit->Special;
    // 02/11: o cajado usa a magia que o herói escolheu (cura, vinhas ou bola de fogo); sem escolha, não gasta nada (o
    // cliente abre a escolha, VortexSpellChoice)
    if (S == TEXT("vines"))
    {
        const FName Chosen = Owner->GetSheet() ? Owner->GetSheet()->GetStaffSpell() : NAME_None;
        if (Chosen.IsNone())
        {
            if (Owner->GetSheet()) Owner->GetSheet()->PushNotice(TEXT("Escolha a magia do cajado (botão direito)"));
            return true;
        }
        S = Chosen;
    }
    const float Base = PrimaryDamage();
    const UAnimSequence* SwordClip = VortexSwords::SpecialClip(Owner, EquippedDefinition, false);   // 07/11: o clipe do poder da espada
    const float Clip = SwordClip ? SwordClip->GetPlayLength() : EquippedDefinition->SecondaryClip ? EquippedDefinition->SecondaryClip->GetPlayLength() : 1.f;
    const FVector Feet = Owner->GetActorLocation() - FVector(0.f, 0.f, 68.f);
    const FVector Forward = Owner->GetActorForwardVector().GetSafeNormal2D();
    UWorld* World = GetWorld();
    UE_LOG(LogTemp, Display, TEXT("VORTEX_KIT_SPECIAL %s %s"), *EquippedDefinition->WeaponId.ToString(), *S.ToString());

    // ---- instrumento: canta enquanto segura (gasta mana por segundo)
    if (S == TEXT("song"))
    {
        if (!SpendMana(Kit->SpecialMana * .25f, TEXT("tocar"))) return true;
        bSinging = true;
        SongStart = World->GetTimeSeconds();
        KitPose(VortexSpecialPose::Hold, 0.f);
        SongAura = AVortexWeaponFx::Spawn(Owner, TEXT("aura"), Owner->GetActorLocation(), FRotator::ZeroRotator, [&](AVortexWeaponFx& F)
        {
            F.Color = FLinearColor(1.f, .75f, .95f);
            F.Radius = Kit->Value(TEXT("radius"), 800.f);
            F.Life = 60.f;
        });
        KitSound(TEXT("wfx_lute_song"), Owner->GetActorLocation(), 1.f, 0.f);
        World->GetTimerManager().SetTimer(SongTimer, this, &UVortexWeaponComponent::TickSong, .25f, true);
        TickSong();
        return true;
    }
    // ---- escudo segue no botão; os outros têm recarga e mana na hora
    if (!SpendMana(Kit->SpecialMana, *EquippedDefinition->DisplayName)) return true;

    if (S == TEXT("orbit"))
    {
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(.45f, [this, Kit, Base]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (!Hero) return;
            AVortexWeaponFx::Spawn(Hero, TEXT("orbit"), Hero->GetActorLocation(), FRotator::ZeroRotator, [&](AVortexWeaponFx& F)
            {
                F.Color = Holy;
                F.Life = Kit->Value(TEXT("life"), 12.f);
                F.Every = Kit->Value(TEXT("every"), 1.5f);
                F.Damage = Base * Kit->Value(TEXT("scale"), .4f);
                F.Length = Kit->Value(TEXT("reach"), 1800.f);
            });
            KitSound(TEXT("wfx_orb_summon"), Hero->GetActorLocation(), 1.f);
        });
        return true;
    }
    if (S == TEXT("firewall"))
    {
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(.55f, [this, Kit, Base]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (!Hero) return;
            const FVector From = Hero->GetActorLocation();
            FVector At = AimPoint(Kit->Value(TEXT("reach"), 2200.f) + 600.f);
            At = From + (At - From).GetClampedToMaxSize(Kit->Value(TEXT("reach"), 2200.f));
            At = Ground(GetWorld(), At, Hero);
            const FVector Dir = (At - From).GetSafeNormal2D().IsNearlyZero() ? Hero->GetActorForwardVector() : (At - From).GetSafeNormal2D();
            const FName Element(TEXT("fire"));
            AVortexWeaponFx::Spawn(Hero, TEXT("firewall"), At, Dir.Rotation(), [&](AVortexWeaponFx& F)
            {
                F.Element = Element;
                F.Color = Fire;
                F.Length = Kit->Value(TEXT("width"), 800.f);
                F.Life = Kit->Value(TEXT("life"), 6.f);
                F.Damage = Base * Kit->Value(TEXT("scale"), .5f);
                F.Every = .5f;
            });
            KitSound(TEXT("wfx_firewall"), At, 1.f);
        });
        return true;
    }
    if (S == TEXT("overload"))
    {
        // varinha: carga curta (não instantânea) e um disparo grosso
        const float Charge = Kit->Value(TEXT("charge"), .6f);
        KitPose(VortexSpecialPose::Hold, 0.f);
        StartKitCooldown(Kit->SpecialCooldown);
        KitSound(TEXT("wfx_wand_charge"), Owner->GetActorLocation(), .9f);
        Later(Charge, [this, Kit, Base]()
        {
            if (!EquippedDefinition) return;
            const UAnimSequence* Own = VortexSwords::SpecialClip(GetOwner(), EquippedDefinition, false);
            const float Release = Own ? Own->GetPlayLength() : EquippedDefinition->SecondaryClip ? EquippedDefinition->SecondaryClip->GetPlayLength() : .6f;
            KitPose(VortexSpecialPose::Release, Release);
            const FVortexWeaponSwing& First = EquippedDefinition->Swings[0];
            // 07/11: o raio do poder (o elemento da arma) e a explosão no alvo ("radius"), como o texto promete
            const FName Element = VortexSwords::PowerElement(GetOwner());
            LaunchProjectile(TEXT("bolt"), Element.IsNone() ? FName(TEXT("arcane")) : Element, Base * Kit->Value(TEXT("scale"), 2.5f), First.ProjectileSpeed * 1.15f, First.ReachCm,
                Kit->Value(TEXT("radius"), 0.f), 2.3f, true);
            KitSound(TEXT("wfx_wand_blast"), GetOwner()->GetActorLocation(), 1.f);
        });
        return true;
    }
    if (S == TEXT("vines"))
    {
        // cajado: mira um inimigo e conjura (1,2 s, dá para cancelar apertando de novo)
        FHitResult Aim;
        AActor* Target = nullptr;
        if (Owner->TraceAim(Aim, Kit->Value(TEXT("reach"), 2600.f) + 600.f) && Aim.GetActor() && Owner->GetCombat()->CanHarm(Aim.GetActor())) Target = Aim.GetActor();
        if (!Target)
        {
            // 02/11: o inimigo mais perto da linha da mira (antes, a 3,5 m do ponto onde a mira batia: mirando num bicho, o
            // raio passava por ele e batia no chão atrás, longe dele, e o cipó dizia que não havia alvo quase sempre)
            const FVector From = Aim.TraceStart, To = Aim.bBlockingHit ? FVector(Aim.ImpactPoint) + (FVector(Aim.ImpactPoint) - Aim.TraceStart).GetSafeNormal() * 400.f : FVector(Aim.TraceEnd);
            float Best = TNumericLimits<float>::Max();
            for (TActorIterator<APawn> It(World); It; ++It)
            {
                if (!Owner->GetCombat()->CanHarm(*It)) continue;
                const FVector At = It->GetActorLocation();
                const float Off = FMath::PointDistToSegment(At, From, To);
                // tolerância maior longe (a mira de longe treme mais): 1,5 m perto, até 3 m no alcance
                const float Allowed = 150.f + 150.f * FMath::Clamp(FVector::Dist(At, Owner->GetActorLocation()) / Kit->Value(TEXT("reach"), 2600.f), 0.f, 1.f);
                if (Off < Allowed && Off < Best) { Best = Off; Target = *It; }
            }
        }
        if (!Target || FVector::Dist(Target->GetActorLocation(), Owner->GetActorLocation()) > Kit->Value(TEXT("reach"), 2600.f))
        {
            if (Owner->GetSheet()) Owner->GetSheet()->PushNotice(TEXT("Mire num inimigo para a prisão de vinhas"));
            if (UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent())
                ASC->ApplyModToAttribute(UVortexAttributes::GetManaAttribute(), EGameplayModOp::Additive, Kit->SpecialMana);
            return true;
        }
        VineTarget = Target;
        bCasting = true;
        KitPose(VortexSpecialPose::Release, Clip);
        KitSound(TEXT("wfx_staff_cast"), Owner->GetActorLocation(), .9f);
        Later(Kit->Value(TEXT("cast"), 1.2f), [this, Kit]()
        {
            if (!bCasting) return;
            bCasting = false;
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            AActor* Foe = VineTarget.Get();
            StartKitCooldown(Kit->SpecialCooldown);
            if (!Hero || !Foe) return;
            const IVortexCreature* Creature = Cast<IVortexCreature>(Foe);
            const float Hold = Creature && Creature->IsBoss() ? Kit->Value(TEXT("hold_boss"), 1.5f) : Kit->Value(TEXT("hold"), 3.f);
            if (UVortexCombatComponent* Their = Foe->FindComponentByClass<UVortexCombatComponent>()) Their->ApplySlow(0.f, Hold);
            AVortexWeaponFx::Spawn(Hero, TEXT("vines"), Foe->GetActorLocation(), FRotator::ZeroRotator, [&](AVortexWeaponFx& F)
            {
                F.Color = Nature;
                F.Life = Hold;
                F.Follow = Foe;
            });
            KitSound(TEXT("wfx_vines"), Foe->GetActorLocation(), 1.f);
        });
        return true;
    }
    if (S == TEXT("heal"))
    {
        // cajado, cura (02/11): o herói e o grupo dele a até 8 m, um quarto da vida mais o poder da arma
        // 03/11: no duelo não cura ninguém (os dois se curavam e o duelo não acabava) e a recarga é 1,6x a das outras
        if (const UVortexSocial* Duelist = Owner->GetSocial(); Duelist && Duelist->GetDuelPhase() != 0)
        {
            if (Owner->GetSheet()) Owner->GetSheet()->PushNotice(TEXT("No duelo a cura não funciona"));
            if (UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent())
                ASC->ApplyModToAttribute(UVortexAttributes::GetManaAttribute(), EGameplayModOp::Additive, Kit->SpecialMana);
            return true;
        }
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown * 1.6f);
        // 03/11: o aliado da mira (a HUD marca quem vai receber); sem ninguém na mira, o herói e o grupo perto
        AVortexCharacter* Aimed = HealTarget(Kit->Value(TEXT("heal_reach"), 2200.f));
        TArray<AVortexCharacter*> Healed = { Aimed ? Aimed : Owner };
        if (const UVortexSocial* Social = Owner->GetSocial(); Social && !Aimed)
            for (AVortexCharacter* Mate : Social->PartyMates())
                if (Mate && FVector::Dist(Mate->GetActorLocation(), Owner->GetActorLocation()) < 800.f &&
                    !(Mate->GetSocial() && Mate->GetSocial()->GetDuelPhase() != 0)) Healed.Add(Mate);
        for (AVortexCharacter* Hero : Healed)
        {
            UVortexHeroSheet* Sheet = Hero->GetSheet();
            if (!Sheet) continue;
            Sheet->HealBy(Sheet->ComputeStats().MaxHealth * .25f + Base * 2.f);
            AVortexWeaponFx::Spawn(Owner, TEXT("aura"), Hero->GetActorLocation(), FRotator::ZeroRotator, [&](AVortexWeaponFx& F)
            {
                F.Color = FLinearColor(.45f, 1.f, .55f);
                F.Life = 1.2f;
                F.Follow = Hero;
            });
        }
        KitSound(TEXT("holy_cast"), Owner->GetActorLocation(), 1.f);
        return true;
    }
    if (S == TEXT("fireball"))
    {
        // cajado, bola de fogo (02/11): uma esfera grande que explode onde bate (3 m), 4x o golpe
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(.3f, [this, Base, Kit]()
        {
            if (AVortexProjectile* Ball = LaunchProjectile(TEXT("orb"), TEXT("fire"), Base * 4.f, 3800.f, Kit->Value(TEXT("reach"), 2600.f), 320.f, 2.6f))
                Ball->Stun = .4f;
        });
        KitSound(TEXT("wfx_staff_cast"), Owner->GetActorLocation(), 1.f);
        return true;
    }
    if (S == TEXT("potion"))
    {
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(.42f, [this, Kit]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (!Hero) return;
            const FVector From = MuzzleLocation();
            AActor* Foe = AimTarget(Kit->Value(TEXT("reach"), 1600.f));
            FVector At = Foe ? Foe->GetActorLocation() : AimPoint(Kit->Value(TEXT("reach"), 1600.f) + 600.f);
            At = Ground(GetWorld(), Hero->GetActorLocation() + (At - Hero->GetActorLocation()).GetClampedToMaxSize(Kit->Value(TEXT("reach"), 1600.f)), Foe ? Foe : (AActor*)Hero);
            AVortexWeaponFx::Spawn(Hero, TEXT("potion"), From, FRotator::ZeroRotator, [&](AVortexWeaponFx& F)
            {
                F.Color = FLinearColor(1.f, .5f, .9f);
                F.Target = At;
                F.Homing = Foe;
                F.Delay = .75f;
                F.Life = .75f + 1.5f;
                F.Radius = Kit->Value(TEXT("radius"), 400.f);
                F.Damage = Kit->Value(TEXT("buff"), .15f);   // fortalecimento
                F.Every = Kit->Value(TEXT("life"), 10.f);     // duração
                F.Mesh = EquippedDefinition ? EquippedDefinition->VisualMesh : nullptr;
            });
            KitSound(TEXT("wfx_throw"), From, .9f);
        });
        return true;
    }
    if (S == TEXT("holyslam"))
    {
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(.62f, [this, Kit, Base]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (!Hero) return;
            const FVector Dir = Hero->GetActorForwardVector().GetSafeNormal2D();
            AVortexWeaponFx::Spawn(Hero, TEXT("holytrail"), Hero->GetActorLocation() - FVector(0.f, 0.f, 66.f), Dir.Rotation(), [&](AVortexWeaponFx& F)
            {
                F.Color = Holy;
                F.Length = Kit->Value(TEXT("length"), 700.f);
                F.Life = 1.2f;
                F.Damage = Base * Kit->Value(TEXT("scale"), 1.6f);
                F.Stun = Kit->Value(TEXT("stun"), 2.5f);
                F.Count = int32(Kit->Value(TEXT("levels"), 5.f));
                F.DamageKind = EVortexDamageKind::Melee;
            });
            AVortexWeaponFx::Spawn(Hero, TEXT("slam"), Hero->GetActorLocation(), Dir.Rotation(), [&](AVortexWeaponFx& F) { F.Radius = 260.f; F.Life = 1.f; });
            // cura um pouco de quem bate
            if (UAbilitySystemComponent* ASC = Hero->GetAbilitySystemComponent())
            {
                const float Max = ASC->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute());
                const float Now = ASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
                ASC->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), FMath::Min(Max, Now + Max * Kit->Value(TEXT("heal"), .08f)));
            }
            KitSound(TEXT("wfx_holy_slam"), Hero->GetActorLocation(), 1.f);
        });
        return true;
    }
    if (S == TEXT("dash"))
    {
        // adagas: avanço rápido bem para a frente, tesourada em quem estiver no caminho
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        const float Time = Kit->Value(TEXT("time"), .25f);
        Owner->LaunchCharacter(Forward * Kit->Value(TEXT("distance"), 600.f) / Time * 1.35f + FVector(0.f, 0.f, 60.f), true, true);
        KitSound(TEXT("wfx_dagger_dash"), Owner->GetActorLocation(), 1.f);
        TSharedRef<TSet<TWeakObjectPtr<AActor>>> Struck = MakeShared<TSet<TWeakObjectPtr<AActor>>>();
        for (int32 i = 0; i < 9; ++i)
            Later(.03f + i * .035f, [this, Struck, Base, Kit]()
            {
                if (AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner()))
                {
                    const TSet<TWeakObjectPtr<AActor>> Before = *Struck;
                    Hero->GetCombat()->StrikeInArc(1.f, 220.f, -.6f, *Struck, 0.f, Base * Kit->Value(TEXT("scale"), 1.5f));
                    PowerTouch(Hero, Kit, Struck->Difference(Before), Base * Kit->Value(TEXT("scale"), 1.5f));
                }
            });
        return true;
    }
    if (S == TEXT("shockwave"))
    {
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(.62f, [this, Kit, Base]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (!Hero) return;
            const FVector Dir = Hero->GetActorForwardVector().GetSafeNormal2D();
            FActorSpawnParameters Params;
            Params.Owner = Hero;
            Params.Instigator = Hero;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            if (AVortexSlashWave* Wave = GetWorld()->SpawnActorDeferred<AVortexSlashWave>(AVortexSlashWave::StaticClass(),
                FTransform(FRotator(0.f, Dir.Rotation().Yaw, 90.f), Hero->GetActorLocation() + Dir * 90.f - FVector(0.f, 0.f, 30.f)), Hero, Hero,
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
            {
                Wave->Damage = Base * Kit->Value(TEXT("scale"), 2.f);
                Wave->Range = Kit->Value(TEXT("length"), 800.f);
                Wave->FinishSpawning(FTransform(FRotator(0.f, Dir.Rotation().Yaw, 90.f), Hero->GetActorLocation() + Dir * 90.f - FVector(0.f, 0.f, 30.f)));
            }
            AVortexWeaponFx::Spawn(Hero, TEXT("slam"), Hero->GetActorLocation(), Dir.Rotation(), [&](AVortexWeaponFx& F) { F.Radius = 220.f; F.Life = 1.f; });
            KitSound(TEXT("wfx_heavy_impact"), Hero->GetActorLocation(), 1.f);
        });
        return true;
    }
    if (S == TEXT("flurry") || S == TEXT("chainflurry"))
    {
        // lança: rajada de estocadas; corrente: 3 golpes rápidos
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        const int32 N = int32(Kit->Value(TEXT("count"), 6.f));
        const float Step = Kit->Value(TEXT("time"), 1.2f) / N;
        const float Reach = Kit->Value(TEXT("reach"), EquippedDefinition->Swings[0].ReachCm);
        for (int32 i = 0; i < N; ++i)
            Later(.1f + i * Step, [this, Kit, Base, Reach, S]()
            {
                AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
                if (!Hero) return;
                TSet<TWeakObjectPtr<AActor>> Fresh;
                Hero->GetCombat()->StrikeInArc(1.f, Reach, .3f, Fresh, 0.f, Base * Kit->Value(TEXT("scale"), .5f));
                PowerTouch(Hero, Kit, Fresh, Base * Kit->Value(TEXT("scale"), .5f));
                KitSound(S == TEXT("flurry") ? TEXT("wfx_spear_thrust") : TEXT("wfx_chain_lash"), Hero->GetActorLocation(), .7f, .1f);
            });
        return true;
    }
    if (S == TEXT("groundslam"))
    {
        KitPose(VortexSpecialPose::Release, EquippedDefinition->Swings.Last().Duration);
        StartKitCooldown(Kit->SpecialCooldown);
        // 07/11: "delay" = o instante da batida (a espada que crava no chão bate no fim da animação dela)
        Later(Kit->Value(TEXT("delay"), FMath::Max(.2f, EquippedDefinition->Swings.Last().ActiveStart)), [this, Kit, Base]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (!Hero) return;
            LaunchProjectile(TEXT("burst"), TEXT("steel"), Base * Kit->Value(TEXT("scale"), 1.8f), 0.f, 0.f, Kit->Value(TEXT("radius"), 400.f), 1.6f);
            AVortexWeaponFx::Spawn(Hero, TEXT("slam"), Hero->GetActorLocation(), Hero->GetActorRotation(), [&](AVortexWeaponFx& F) { F.Radius = Kit->Value(TEXT("radius"), 400.f); F.Life = 1.f; });
            KitSound(TEXT("wfx_hammer_slam"), Hero->GetActorLocation(), 1.f);
        });
        return true;
    }
    if (S == TEXT("throwaxes"))
    {
        if (ThrowMask) return true;
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(.38f, [this, Kit, Base]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (!Hero || !EquippedDefinition) return;
            const FVector Target = AimPoint(Kit->Value(TEXT("reach"), 1200.f) + 600.f);
            FVector Dir = (Target - Hero->GetActorLocation()).GetSafeNormal2D();
            if (Dir.IsNearlyZero() || FVector::DotProduct(Dir, Hero->GetActorForwardVector()) < -.2f) Dir = Hero->GetActorForwardVector().GetSafeNormal2D();
            ThrowMask = 3;
            OnRep_ThrowMask();
            Hero->ForceNetUpdate();
            for (int32 Side = 0; Side < 2; ++Side)
            {
                const FVector D = Dir.RotateAngleAxis(Side ? 7.f : -7.f, FVector::UpVector);
                AVortexWeaponFx::Spawn(Hero, TEXT("boomerang"), Hero->GetActorLocation() + Hero->GetActorRightVector() * (Side ? -30.f : 30.f), D.Rotation(), [&](AVortexWeaponFx& F)
                {
                    F.Mesh = Side ? EquippedDefinition->LeftMesh.Get() : EquippedDefinition->VisualMesh.Get();
                    F.Length = Kit->Value(TEXT("reach"), 1200.f);
                    F.Speed = Kit->Value(TEXT("speed"), 2400.f);
                    F.Radius = 90.f;
                    F.Size = 1.2f;
                    F.Count = 0;
                    F.Life = 2.f * F.Length / F.Speed + 2.f;
                    F.Damage = Base * Kit->Value(TEXT("scale"), 1.6f);
                    F.DamageKind = EVortexDamageKind::Melee;
                    const uint8 Bit = Side ? 2 : 1;
                    F.OnReturn = [this, Bit]() { ThrowMask &= ~Bit; OnRep_ThrowMask(); if (GetOwner()) GetOwner()->ForceNetUpdate(); };
                });
            }
            KitSound(TEXT("wfx_throw_heavy"), Hero->GetActorLocation(), 1.f);
        });
        return true;
    }
    if (S == TEXT("whirl") || S == TEXT("reap"))
    {
        // machado: gira 2 s acertando 4 vezes em volta; foice: um giro de 360°
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        const bool bAxe = S == TEXT("whirl");
        const int32 Hits = bAxe ? int32(Kit->Value(TEXT("hits"), 4.f)) : 1;
        const float Total = Kit->Value(TEXT("time"), bAxe ? 2.f : .6f);
        AVortexWeaponFx::Spawn(Owner, TEXT("sweep"), Owner->GetActorLocation(), Owner->GetActorRotation(), [&](AVortexWeaponFx& F)
        {
            F.Color = bAxe ? FLinearColor(1.f, .72f, .45f) : FLinearColor(.78f, .55f, 1.f);
            F.Radius = Kit->Value(TEXT("radius"), 250.f);
            F.Life = Total + .25f;
            F.Speed = bAxe ? 760.f : 820.f;   // graus por segundo, o mesmo giro do corpo
            F.Delay = .12f;                   // começa junto com o giro do clipe
        });
        for (int32 i = 0; i < Hits; ++i)
            Later(.3f + i * (Total - .2f) / FMath::Max(1, Hits), [this, Kit, Base, bAxe]()
            {
                AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
                if (!Hero) return;
                TSet<TWeakObjectPtr<AActor>> Fresh;
                Hero->GetCombat()->StrikeInArc(1.f, Kit->Value(TEXT("radius"), 250.f), -1.f, Fresh, 0.f, Base * Kit->Value(TEXT("scale"), 1.f));
                PowerTouch(Hero, Kit, Fresh, Base * Kit->Value(TEXT("scale"), 1.f));
                KitSound(bAxe ? TEXT("wfx_axe_whirl") : TEXT("wfx_scythe_reap"), Hero->GetActorLocation(), .9f, .08f);
            });
        return true;
    }
    if (S == TEXT("tidewave"))
    {
        // 07/11 (o usuário: "uma maré na linha que mirei empurrando os inimigos"): a onda nasce na frente do herói e corre
        // em linha reta para onde ele olha (a mira), acertando cada inimigo uma vez e jogando-o junto com ela
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(Kit->Value(TEXT("delay"), .5f), [this, Kit, Base]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (!Hero) return;
            const FVector Dir = Hero->GetActorForwardVector().GetSafeNormal2D();
            const FVector From = Hero->GetActorLocation() + Dir * 80.f;
            const float Length = Kit->Value(TEXT("length"), 1400.f), Width = Kit->Value(TEXT("width"), 320.f), Speed = FMath::Max(300.f, Kit->Value(TEXT("speed"), 1500.f));
            const int32 Steps = FMath::CeilToInt(Length / (Speed * .05f));
            TSharedRef<TSet<TWeakObjectPtr<AActor>>> Hit = MakeShared<TSet<TWeakObjectPtr<AActor>>>();
            for (int32 k = 0; k <= Steps; ++k)
                Later(k * .05f, [this, Kit, Base, Dir, From, Length, Width, Speed, k, Hit]()
                {
                    AVortexCharacter* H = Cast<AVortexCharacter>(GetOwner());
                    if (!H) return;
                    const FVector Center = From + Dir * FMath::Min(Length, Speed * .05f * k);
                    TArray<FOverlapResult> Overlaps;
                    FCollisionObjectQueryParams Pawns;
                    Pawns.AddObjectTypesToQuery(ECC_Pawn);
                    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexTideWave), false, H);
                    GetWorld()->OverlapMultiByObjectType(Overlaps, Center, Dir.Rotation().Quaternion(), Pawns,
                        FCollisionShape::MakeBox(FVector(110.f, Width * .5f, 160.f)), Query);
                    for (const FOverlapResult& O : Overlaps)
                    {
                        AActor* Foe = O.GetActor();
                        if (!Foe || Hit->Contains(Foe) || !H->GetCombat()->CanHarm(Foe)) continue;
                        Hit->Add(Foe);
                        H->GetCombat()->ApplyHit(Foe, 1.f, EVortexDamageKind::Melee, .3f, Base * Kit->Value(TEXT("scale"), 3.f));
                        const IVortexCreature* Creature = Cast<IVortexCreature>(Foe);
                        if (ACharacter* Body = Cast<ACharacter>(Foe); Body && !(Creature && Creature->IsBoss()))
                            Body->LaunchCharacter(Dir * Kit->Value(TEXT("push"), 900.f) * 1.6f + FVector(0.f, 0.f, 320.f), true, true);
                    }
                });
            KitSound(TEXT("water_splash"), Hero->GetActorLocation() + Dir * 300.f, 1.f);
        });
        return true;
    }
    if (S == TEXT("sweep"))
    {
        // espadão: corte lateral de 180° que empurra todos para longe (menos chefes)
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(.62f, [this, Kit, Base]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (!Hero) return;
            TSet<TWeakObjectPtr<AActor>> Struck;
            Hero->GetCombat()->StrikeInArc(1.f, Kit->Value(TEXT("reach"), 420.f), -.05f, Struck, 0.f, Base * Kit->Value(TEXT("scale"), 1.8f));
            PowerTouch(Hero, Kit, Struck, Base * Kit->Value(TEXT("scale"), 1.8f));
            for (const TWeakObjectPtr<AActor>& Weak : Struck)
            {
                ACharacter* Body = Cast<ACharacter>(Weak.Get());
                const IVortexCreature* Creature = Cast<IVortexCreature>(Body);
                if (!Body || (Creature && Creature->IsBoss())) continue;
                const FVector Away = (Body->GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D();
                Body->LaunchCharacter(Away * Kit->Value(TEXT("push"), 600.f) * 2.4f + FVector(0.f, 0.f, 380.f), true, true);
            }
            AVortexWeaponFx::Spawn(Hero, TEXT("sweep"), Hero->GetActorLocation(), Hero->GetActorRotation(), [&](AVortexWeaponFx& F)
            {
                F.Color = FLinearColor(.8f, .9f, 1.f);
                F.Radius = Kit->Value(TEXT("reach"), 420.f);
                F.Life = .3f;
                F.Speed = -620.f;   // meia volta, da direita para a esquerda
                F.Count = 1;        // uma lâmina só
            });
            KitSound(TEXT("wfx_greatsword_sweep"), Hero->GetActorLocation(), 1.f);
        });
        return true;
    }
    if (S == TEXT("grapple"))
    {
        // chicote: gruda no que a mira pega (até 15 m); parede/árvore puxa o herói, inimigo é afastado
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(.3f, [this, Kit, Base]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (!Hero) return;
            const float Reach = Kit->Value(TEXT("reach"), 1500.f);
            FHitResult Aim;
            const bool bHit = Hero->TraceAim(Aim, Reach + 800.f);
            FVector Point = bHit ? FVector(Aim.ImpactPoint) : FVector(Aim.TraceEnd);
            const FVector Hand = Hero->GetActorLocation() + FVector(0.f, 0.f, 30.f);
            const bool bInReach = FVector::Dist(Point, Hand) <= Reach;
            if (!bInReach) Point = Hand + (Point - Hand).GetSafeNormal() * Reach;
            AActor* Struck = bHit && bInReach ? Aim.GetActor() : nullptr;
            AVortexWeaponFx::Spawn(Hero, TEXT("beam"), Hand, FRotator::ZeroRotator, [&](AVortexWeaponFx& F)
            {
                F.Color = FLinearColor(.45f, .28f, .16f);
                F.Target = Point;
                F.Life = .45f;
            });
            KitSound(TEXT("wfx_whip_crack"), Hand, 1.f);
            if (Struck && Hero->GetCombat()->CanHarm(Struck))
            {
                Hero->GetCombat()->ApplyHit(Struck, 1.f, EVortexDamageKind::Melee, 0.f, Base * Kit->Value(TEXT("scale"), 1.2f));
                if (ACharacter* Body = Cast<ACharacter>(Struck))
                {
                    const IVortexCreature* Creature = Cast<IVortexCreature>(Body);
                    if (!(Creature && Creature->IsBoss()))
                        Body->LaunchCharacter((Body->GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D() * Kit->Value(TEXT("push"), 400.f) * 2.6f + FVector(0.f, 0.f, 300.f), true, true);
                }
            }
            else if (bHit && bInReach)
            {
                // puxa até lá, como os ganchos de Attack on Titan
                const FVector Pull = (Point - Hero->GetActorLocation()).GetSafeNormal();
                Hero->LaunchCharacter(Pull * Kit->Value(TEXT("pull"), 2500.f) + FVector(0.f, 0.f, 350.f), true, true);
                KitSound(TEXT("wfx_grapple_pull"), Hand, .9f);
            }
        });
        return true;
    }
    if (S == TEXT("giantchakram"))
    {
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(.5f, [this, Kit, Base]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (!Hero || !EquippedDefinition) return;
            FVector Dir = (AimPoint(Kit->Value(TEXT("giant_reach"), 1600.f) + 600.f) - Hero->GetActorLocation()).GetSafeNormal2D();
            if (Dir.IsNearlyZero()) Dir = Hero->GetActorForwardVector();
            AVortexWeaponFx::Spawn(Hero, TEXT("boomerang"), Hero->GetActorLocation() + FVector(0.f, 0.f, 10.f), Dir.Rotation(), [&](AVortexWeaponFx& F)
            {
                F.Mesh = EquippedDefinition->VisualMesh;
                F.Color = FLinearColor(.6f, .9f, 1.f);
                F.Length = Kit->Value(TEXT("giant_reach"), 1600.f);
                F.Speed = 2000.f;
                F.Radius = Kit->Value(TEXT("giant_size"), 200.f) * .6f;
                F.Size = 5.5f;
                F.Count = 1;
                F.Life = 2.f * F.Length / F.Speed + 2.f;
                F.Damage = Base * Kit->Value(TEXT("scale"), 2.f);
                F.DamageKind = EVortexDamageKind::Ranged;
            });
            KitSound(TEXT("wfx_chakram_giant"), Hero->GetActorLocation(), 1.f);
        });
        return true;
    }
    if (S == TEXT("runeseal"))
    {
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(.32f, [this, Kit, Base]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (!Hero) return;
            FHitResult Aim;
            const bool bHit = Hero->TraceAim(Aim, Kit->Value(TEXT("reach"), 2600.f) + 600.f);
            AActor* Stuck = bHit && Aim.GetActor() && Hero->GetCombat()->CanHarm(Aim.GetActor()) ? Aim.GetActor() : nullptr;
            const FVector Point = Stuck ? Stuck->GetActorLocation() : Ground(GetWorld(), bHit ? FVector(Aim.ImpactPoint) : FVector(Aim.TraceEnd), Hero);
            const float Mark = Kit->Value(TEXT("mark"), .25f), MarkLife = Kit->Value(TEXT("mark_life"), 5.f), Delay = Kit->Value(TEXT("delay"), 1.5f);
            if (Stuck)
                if (UVortexCombatComponent* Their = Stuck->FindComponentByClass<UVortexCombatComponent>()) Their->ApplyExpose(Mark, Delay + MarkLife);
            AVortexWeaponFx::Spawn(Hero, TEXT("seal"), Point, FRotator::ZeroRotator, [&](AVortexWeaponFx& F)
            {
                F.Color = Rune;
                F.Follow = Stuck ? Stuck : Hero;
                F.Delay = Delay;
                F.Life = Delay + .5f;
                F.Radius = Kit->Value(TEXT("radius"), 300.f);
                F.Damage = Base * Kit->Value(TEXT("scale"), 2.f);
                F.Expose = Mark;
                F.ExposeTime = MarkLife;
            });
            LaunchProjectile(TEXT("bolt"), TEXT("rune"), 0.f, 4200.f, FVector::Dist(Hero->GetActorLocation(), Point), 0.f, 1.3f);
            // 07/11: a fenda que prende ("root"): quem está no raio quando ela explode fica parado
            if (const float Root = Kit->Value(TEXT("root"), 0.f); Root > 0.f)
                Later(Delay, [this, Point, Root, Radius = Kit->Value(TEXT("radius"), 300.f)]()
                {
                    AVortexCharacter* H = Cast<AVortexCharacter>(GetOwner());
                    if (!H) return;
                    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
                        if (H->GetCombat()->CanHarm(*It) && FVector::Dist2D(It->GetActorLocation(), Point) < Radius)
                            if (UVortexCombatComponent* Their = It->FindComponentByClass<UVortexCombatComponent>()) Their->ApplySlow(0.f, Root);
                });
            KitSound(TEXT("wfx_rune_shot"), Hero->GetActorLocation(), 1.f);
        });
        return true;
    }
    if (S == TEXT("burst3"))
    {
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        const int32 N = int32(Kit->Value(TEXT("count"), 3.f));
        for (int32 i = 0; i < N; ++i)
            Later(.06f + i * Kit->Value(TEXT("time"), .45f) / N, [this, Kit]()
            {
                if (!EquippedDefinition) return;
                const FVortexWeaponSwing& First = EquippedDefinition->Swings[0];
                const FName Own = VortexSwords::PowerElement(GetOwner());   // 07/11: os arcos com elemento
                LaunchProjectile(TEXT("arrow"), Own.IsNone() ? First.Element : Own, First.Damage * Kit->Value(TEXT("scale"), 1.8f), First.ProjectileSpeed, Kit->Value(TEXT("reach"), 4000.f), 0.f, 1.2f, true);
                KitSound(TEXT("wfx_crossbow_shot"), GetOwner()->GetActorLocation(), .9f, .06f);
            });
        return true;
    }
    // ---- 07/11: os poderes novos dos arcos (Data/Rpg/bows.json): o leque, a flecha que atravessa e a que explode,
    // todos com o elemento do arco
    if (S == TEXT("volley") || S == TEXT("pierce") || S == TEXT("explosive"))
    {
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(.3f, [this, Kit, Base, S]()
        {
            if (!EquippedDefinition || !EquippedDefinition->Swings.Num()) return;
            const FVortexWeaponSwing& First = EquippedDefinition->Swings[0];
            const FName Own = VortexSwords::PowerElement(GetOwner());
            const FName Element = Own.IsNone() ? First.Element : Own;
            const float Reach = Kit->Value(TEXT("reach"), 3000.f);
            if (S == TEXT("volley"))
            {
                const int32 N = FMath::Max(1, int32(Kit->Value(TEXT("count"), 5.f)));
                const float Spread = Kit->Value(TEXT("spread"), 40.f);
                for (int32 i = 0; i < N; ++i)
                {
                    VortexVolley::Yaw = N > 1 ? -Spread * .5f + Spread * i / (N - 1) : 0.f;
                    LaunchProjectile(TEXT("arrow"), Element, Base * Kit->Value(TEXT("scale"), 1.3f), First.ProjectileSpeed, Reach, 0.f, 1.f, false);
                }
                VortexVolley::Yaw = 0.f;
            }
            else if (S == TEXT("pierce"))
                LaunchProjectile(TEXT("arrow"), Element, Base * Kit->Value(TEXT("scale"), 3.5f), First.ProjectileSpeed * 1.3f, Reach, 0.f, Kit->Value(TEXT("size"), 2.f), true);
            else
                LaunchProjectile(TEXT("arrow"), Element, Base * Kit->Value(TEXT("scale"), 3.f), First.ProjectileSpeed, Reach, Kit->Value(TEXT("radius"), 320.f), 1.4f, false);
            KitSound(TEXT("wfx_bow_release"), GetOwner()->GetActorLocation(), 1.f);
        });
        return true;
    }
    if (S == TEXT("arrowrain"))
    {
        KitPose(VortexSpecialPose::Release, Clip);
        StartKitCooldown(Kit->SpecialCooldown);
        Later(.55f, [this, Kit, Base]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (!Hero) return;
            FVector At = AimPoint(Kit->Value(TEXT("reach"), 2500.f) + 600.f);
            At = Ground(GetWorld(), Hero->GetActorLocation() + (At - Hero->GetActorLocation()).GetClampedToMaxSize(Kit->Value(TEXT("reach"), 2500.f)), Hero);
            AVortexWeaponFx::Spawn(Hero, TEXT("rain"), At, FRotator::ZeroRotator, [&](AVortexWeaponFx& F)
            {
                F.Color = Nature;
                F.Target = At;
                F.Radius = Kit->Value(TEXT("radius"), 400.f);
                F.Life = Kit->Value(TEXT("life"), 2.5f);
                F.Count = int32(Kit->Value(TEXT("arrows"), 20.f));
                F.Damage = Base * Kit->Value(TEXT("scale"), .6f);
                F.DamageKind = EVortexDamageKind::Ranged;
            });
            KitSound(TEXT("wfx_bow_release"), Hero->GetActorLocation(), 1.f);
            KitSound(TEXT("wfx_arrow_rain"), At, 1.f);
        });
        return true;
    }
    return false;
}

void UVortexWeaponComponent::ReleaseKitSpecial()
{
    if (bSinging) { EndSong(); return; }
    if (bCasting)
    {
        // cancelou a prisão de vinhas: a mana volta e não entra em recarga
        bCasting = false;
        const VortexWeaponKit::FKit* Kit = EquippedDefinition ? VortexSwords::SpecialKit(GetOwner(), EquippedDefinition) : nullptr;
        if (AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner()); Hero && Kit && Hero->GetAbilitySystemComponent())
            Hero->GetAbilitySystemComponent()->ApplyModToAttribute(UVortexAttributes::GetManaAttribute(), EGameplayModOp::Additive, Kit->SpecialMana);
        FinishSpecial();
        return;
    }
    // espada e escudo: soltar o botão abaixa o escudo
    const VortexWeaponKit::FKit* Kit = EquippedDefinition ? VortexSwords::SpecialKit(GetOwner(), EquippedDefinition) : nullptr;
    if (Kit && Kit->Special == TEXT("shield")) SetGuardPressedOnServer(false);
}

void UVortexWeaponComponent::TickSong()
{
    AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
    const VortexWeaponKit::FKit* Kit = EquippedDefinition ? VortexWeaponKit::Find(EquippedDefinition->WeaponId) : nullptr;
    if (!Hero || !Kit || !bSinging) return;
    // 4 de mana por segundo; sem mana (ou 20 s tocando) a música para sozinha
    if (GetWorld()->GetTimeSeconds() - SongStart > .2f && !SpendMana(Kit->SpecialMana * .25f, TEXT("continuar tocando"))) { EndSong(); return; }
    if (GetWorld()->GetTimeSeconds() - SongStart > 20.f) { EndSong(); return; }
    // a melodia (4 s) recomeça enquanto toca
    if (FMath::Fmod(GetWorld()->GetTimeSeconds() - SongStart, 4.f) < .25f && GetWorld()->GetTimeSeconds() - SongStart > 1.f)
        KitSound(TEXT("wfx_lute_song"), Hero->GetActorLocation(), 1.f, 0.f);
    FVortexClassSkill Buff;
    Buff.BuffDamage = Kit->Value(TEXT("damage"), .15f);
    Buff.BuffArmor = Kit->Value(TEXT("armor"), .15f);
    Buff.BuffDuration = .6f;
    Buff.Element = TEXT("arcane");
    const float Radius = Kit->Value(TEXT("radius"), 800.f);
    for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
        if (*It == Hero || FVector::Dist(It->GetActorLocation(), Hero->GetActorLocation()) <= Radius)
            if (It->GetClassSkills()) It->GetClassSkills()->GiveBuff(Buff, 1.f);
}

void UVortexWeaponComponent::EndSong()
{
    AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
    const VortexWeaponKit::FKit* Kit = EquippedDefinition ? VortexWeaponKit::Find(EquippedDefinition->WeaponId) : nullptr;
    bSinging = false;
    GetWorld()->GetTimerManager().ClearTimer(SongTimer);
    if (AVortexWeaponFx* Aura = SongAura.Get()) Aura->Destroy();
    SongAura = nullptr;
    if (Hero && Kit)
    {
        // ao parar: o fortalecimento fica 15 s em quem tocou e 3 s nos aliados; a recarga conta daqui
        FVortexClassSkill Buff;
        Buff.BuffDamage = Kit->Value(TEXT("damage"), .15f);
        Buff.BuffArmor = Kit->Value(TEXT("armor"), .15f);
        Buff.Element = TEXT("arcane");
        const float Radius = Kit->Value(TEXT("radius"), 800.f);
        for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
        {
            if (*It != Hero && FVector::Dist(It->GetActorLocation(), Hero->GetActorLocation()) > Radius) continue;
            Buff.BuffDuration = *It == Hero ? Kit->Value(TEXT("self"), 15.f) : Kit->Value(TEXT("allies"), 3.f);
            if (It->GetClassSkills()) It->GetClassSkills()->GiveBuff(Buff, 1.f);
        }
        StartKitCooldown(Kit->SpecialCooldown);
        KitSound(TEXT("wfx_lute_end"), Hero->GetActorLocation(), .9f);
    }
    FinishSpecial();
}

// ---- golpes normais de jeito próprio ------------------------------------------------------------

bool UVortexWeaponComponent::FireKitPrimary(int32 SwingIndex)
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    const VortexWeaponKit::FKit* Kit = EquippedDefinition ? VortexWeaponKit::Find(EquippedDefinition->WeaponId) : nullptr;
    if (!Owner || !Kit || !EquippedDefinition->Swings.IsValidIndex(SwingIndex)) return false;
    const FVortexWeaponSwing& Swing = EquippedDefinition->Swings[SwingIndex];
    if (Kit->Family == TEXT("Book"))
    {
        if (GetWorld()->GetTimeSeconds() < BookTornadoReadyTime) return false;
        BookTornadoReadyTime = GetWorld()->GetTimeSeconds() + Kit->Value(TEXT("tornado_cooldown"), 3.f);
        // grimório de fogo: o círculo aparece em cima do inimigo mirado (ou no chão) e sobe um tornado
        FHitResult Aim;
        const float Reach = Kit->Value(TEXT("reach"), 2200.f);
        const bool bHit = Owner->TraceAim(Aim, Reach + 600.f);
        FVector At = bHit ? FVector(Aim.ImpactPoint) : FVector(Aim.TraceEnd);
        At = Owner->GetActorLocation() + (At - Owner->GetActorLocation()).GetClampedToMaxSize(Reach);
        At = Ground(GetWorld(), At, Owner);
        const float Big = SwingIndex == EquippedDefinition->Swings.Num() - 1 ? 1.3f : 1.f;
        const FName Element(TEXT("fire"));
        AVortexWeaponFx::Spawn(Owner, TEXT("tornado"), At, FRotator::ZeroRotator, [&](AVortexWeaponFx& F)
        {
            F.Element = Element;
            F.Color = Fire;
            F.Radius = Kit->Value(TEXT("tornado_radius"), 250.f) * Big;
            F.Life = Kit->Value(TEXT("tornado_life"), 2.5f);
            F.Every = .5f;
            F.Damage = Swing.Damage * VortexWeaponKit::PrimaryDamageScale(EquippedDefinition->WeaponId) * .4f * Big;
        });
        KitSound(TEXT("wfx_book_cast"), Owner->GetActorLocation(), .9f);
        return true;
    }
    if (Kit->Family == TEXT("Flask"))
    {
        // garrafa arremessada em arco: estilhaça no ponto (dano em volta e ácido: −10% de defesa por 4 s)
        const float Reach = Kit->Value(TEXT("reach"), 1600.f);
        // mirando num inimigo: vai nele; senão, no chão onde a mira aponta
        AActor* Foe = AimTarget(Reach);
        FVector At = Foe ? Foe->GetActorLocation() : AimPoint(Reach + 600.f);
        At = Ground(GetWorld(), Owner->GetActorLocation() + (At - Owner->GetActorLocation()).GetClampedToMaxSize(Reach), Foe ? Foe : (AActor*)Owner);
        const FVector From = MuzzleLocation();
        const float Flight = FMath::Clamp(FVector::Dist(From, At) / 1500.f, .3f, .85f);
        AVortexWeaponFx::Spawn(Owner, TEXT("lob"), From, FRotator::ZeroRotator, [&](AVortexWeaponFx& F)
        {
            F.Color = Acid;
            F.Target = At;
            F.Homing = Foe;
            F.Delay = Flight;
            F.Life = Flight + 1.4f;
            F.Radius = 160.f + (SwingIndex == 2 ? 60.f : 0.f);
            F.Damage = Swing.Damage * VortexWeaponKit::PrimaryDamageScale(EquippedDefinition->WeaponId);
            F.Expose = Kit->Value(TEXT("acid"), .1f);
            F.ExposeTime = Kit->Value(TEXT("acid_life"), 4.f);
            F.Mesh = EquippedDefinition->VisualMesh;
        });
        KitSound(TEXT("wfx_throw"), From, .8f, .08f);
        return true;
    }
    if (Kit->Family == TEXT("Chakram"))
    {
        // o chakram voa até o alvo girando, corta e volta para a mão (alternando as mãos)
        const int32 Hand = SwingIndex == 2 ? 2 : ChakramHand;
        ChakramHand = 1 - ChakramHand;
        const float Reach = Kit->Value(TEXT("reach"), 1400.f);
        const FVector Target = AimPoint(Reach + 600.f);
        for (int32 Side = 0; Side < 2; ++Side)
        {
            if (Hand != 2 && Hand != Side) continue;
            const uint8 Bit = Side ? 2 : 1;
            if (ThrowMask & Bit) continue;
            const FVector From = Owner->GetActorLocation() + Owner->GetActorRightVector() * (Side ? -28.f : 28.f) + FVector(0.f, 0.f, 25.f);
            FVector Dir = (Target - From).GetSafeNormal();
            if (FVector::DotProduct(Dir.GetSafeNormal2D(), Owner->GetActorForwardVector()) < -.2f) Dir = Owner->GetActorForwardVector();
            const float Length = FMath::Clamp(FVector::Dist(From, Target), 300.f, Reach);
            ThrowMask |= Bit;
            AVortexWeaponFx::Spawn(Owner, TEXT("boomerang"), From, Dir.Rotation(), [&](AVortexWeaponFx& F)
            {
                F.Mesh = Side ? EquippedDefinition->LeftMesh.Get() : EquippedDefinition->VisualMesh.Get();
                F.Length = Length;
                F.Speed = Length / FMath::Max(.15f, Kit->Value(TEXT("return"), .6f) * .5f);
                F.Radius = 70.f;
                F.Size = 1.3f;
                F.Count = 1;
                F.Life = 2.f * Length / F.Speed + 1.5f;
                // corta na ida e de novo na volta: cada passada leva metade (antes eram duas vezes o dano cheio, e o
                // chakram passava muito das outras armas)
                F.Damage = Swing.Damage * VortexWeaponKit::PrimaryDamageScale(EquippedDefinition->WeaponId) * .5f;
                F.DamageKind = EVortexDamageKind::Ranged;
                F.OnReturn = [this, Bit]() { ThrowMask &= ~Bit; OnRep_ThrowMask(); if (GetOwner()) GetOwner()->ForceNetUpdate(); };
            });
        }
        OnRep_ThrowMask();
        Owner->ForceNetUpdate();
        KitSound(TEXT("wfx_chakram_throw"), Owner->GetActorLocation(), .8f, .08f);
        return true;
    }
    return false;
}

// ---- arco: carregar e soltar --------------------------------------------------------------------

void UVortexWeaponComponent::TickBowGrip(float DeltaSeconds)
{
    const AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    UStaticMeshComponent* Visual = Owner ? Owner->GetWeaponVisual() : nullptr;
    if (!Visual || !EquippedDefinition) return;
    const FName Bone = HandBoneFor(EquippedDefinition, Owner->GetMesh());
    if (Bone != TEXT("Bow_L")) { BowUpright = 0.f; return; }
    // atirando (puxando, soltando ou no especial): a pegada medida no Arco_Draw. Parado, andando e correndo a mão do
    // boneco deita, e o arco ia junto (o usuário: "voltou a ficar deitado"): ele gira em volta da mão até ficar em pé
    const bool bShooting = bDrawing || AttackState.SwingIndex != INDEX_NONE || bSpecialActive;
    BowUpright = FMath::FInterpTo(BowUpright, bShooting ? 0.f : 1.f, DeltaSeconds, 10.f);
    Visual->SetRelativeRotation(GripRotationFor(Bone));
    if (BowUpright < .01f) return;
    const FQuat World = Visual->GetComponentQuat();
    const FVector Length = World.GetAxisZ();
    const FVector Up = (FVector::UpVector + Owner->GetActorForwardVector().GetSafeNormal2D() * .12f).GetSafeNormal();
    const FQuat Fix = FQuat::FindBetweenNormals(Length, FVector::DotProduct(Length, Up) < 0.f ? -Up : Up);
    Visual->SetWorldRotation(FQuat::Slerp(World, Fix * World, BowUpright));
}

bool UVortexWeaponComponent::UsesLock() const
{
    const VortexWeaponKit::FKit* Kit = EquippedDefinition ? VortexWeaponKit::Find(EquippedDefinition->WeaponId) : nullptr;
    return Kit && Kit->Family == TEXT("Staff");
}

AActor* UVortexWeaponComponent::GetLockTarget() const
{
    AActor* Target = LockTarget;
    if (!Target || !GetOwner() || !UsesLock()) return nullptr;
    if (const IVortexCreature* Creature = Cast<IVortexCreature>(Target); Creature && Creature->IsCreatureDead()) return nullptr;
    if (FVector::Dist(Target->GetActorLocation(), GetOwner()->GetActorLocation()) > 4000.f) return nullptr;
    return Target;
}

void UVortexWeaponComponent::ToggleLock()
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    if (!Owner) return;
    AActor* Next = GetLockTarget() ? nullptr : AimTarget(3000.f);
    if (!Next && !GetLockTarget())
    {
        if (Owner->GetSheet()) Owner->GetSheet()->PushNotice(TEXT("Mire num inimigo para travar a mira (Caps Lock)"));
        return;
    }
    LockTarget = Next;
    if (!Owner->HasAuthority()) ServerSetLock(Next);
    VortexAudio::Play2D(Owner, Next ? TEXT("ui_open") : TEXT("ui_close"), .5f);
}

void UVortexWeaponComponent::ServerSetLock_Implementation(AActor* Target)
{
    LockTarget = Target;
}

AVortexCharacter* UVortexWeaponComponent::HealTarget(float Range) const
{
    const AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    if (!Owner || !Owner->GetCombat()) return nullptr;
    FHitResult Aim;
    Owner->TraceAim(Aim, Range + 900.f);
    const FVector From = Aim.TraceStart, To = Aim.bBlockingHit ? FVector(Aim.ImpactPoint) : FVector(Aim.TraceEnd);
    TArray<FHitResult> Hits;
    FCollisionObjectQueryParams Pawns;
    Pawns.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexHealTarget), false, Owner);
    GetWorld()->SweepMultiByObjectType(Hits, From, To + (To - From).GetSafeNormal() * 150.f, FQuat::Identity, Pawns, FCollisionShape::MakeSphere(60.f), Query);
    for (const FHitResult& Hit : Hits)
    {
        AVortexCharacter* Mate = Cast<AVortexCharacter>(Hit.GetActor());
        // outro herói (de um jogador), que não é inimigo agora (PvP) e não está em duelo
        if (!Mate || Mate == Owner || !Mate->GetPlayerState() || Owner->GetCombat()->CanHarm(Mate)) continue;
        if (Mate->GetSocial() && Mate->GetSocial()->GetDuelPhase() != 0) continue;
        if (FVector::Dist(Mate->GetActorLocation(), Owner->GetActorLocation()) > Range + 100.f) continue;
        return Mate;
    }
    return nullptr;
}

void UVortexWeaponComponent::ReleasePrimary()
{
    if (!EquippedDefinition) return;
    if (GetOwner()->HasAuthority()) ReleasePrimaryOnServer();
    else ServerReleasePrimary();
}

void UVortexWeaponComponent::ServerReleasePrimary_Implementation() { ReleasePrimaryOnServer(); }

void UVortexWeaponComponent::ReleasePrimaryOnServer()
{
    if (!bDrawing) return;
    // soltar cedo demais: a flecha sai quando a corda chega no mínimo (sem metralhar com cliques rápidos)
    const VortexWeaponKit::FKit* Kit = EquippedDefinition ? VortexWeaponKit::Find(EquippedDefinition->WeaponId) : nullptr;
    const float MinDraw = Kit ? Kit->Value(TEXT("min_draw"), BowMinDraw) : BowMinDraw;
    const float Held = GetWorld()->GetTimeSeconds() - DrawLocalStart;
    if (Held < MinDraw) GetWorld()->GetTimerManager().SetTimer(DrawTimer, this, &UVortexWeaponComponent::BowFire, MinDraw - Held, false);
    else BowFire();
}

void UVortexWeaponComponent::BowFire()
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    const VortexWeaponKit::FKit* Kit = EquippedDefinition ? VortexWeaponKit::Find(EquippedDefinition->WeaponId) : nullptr;
    if (!bDrawing || !Owner || !Kit || EquippedDefinition->Swings.IsEmpty()) { bDrawing = false; return; }
    GetWorld()->GetTimerManager().ClearTimer(DrawTimer);
    const float Charge = DrawCharge();
    bDrawing = false;
    // quanto mais tempo segurando, mais dano, alcance e velocidade, e menos a flecha cai. 03/11 (o usuário: "quanto mais
    // segurar mais longe e mais dano; segurando pouco, mais caimento"): a carga cheia leva mais tempo (VortexWeaponKit:
    // full), o tiro rápido dá bem pouco dano (min) e cai (LaunchProjectile: ArrowCharge), e o cheio dá mais (max)
    const FVortexWeaponSwing& Shot = EquippedDefinition->Swings[0];
    const float Scale = FMath::Lerp(Kit->Value(TEXT("min"), .5f), Kit->Value(TEXT("max"), 1.5f), FMath::Pow(Charge, 1.3f));
    const float Reach = Kit->Value(TEXT("reach"), 2500.f) * FMath::Lerp(.45f, 1.1f, Charge);
    LaunchProjectile(TEXT("arrow"), Shot.Element, Shot.Damage * Scale * VortexWeaponKit::PrimaryDamageScale(EquippedDefinition->WeaponId),
        Shot.ProjectileSpeed * FMath::Lerp(.55f, 1.3f, Charge), Reach, 0.f, FMath::Lerp(.9f, 1.15f, Charge), false, nullptr, Charge);
    KitSound(TEXT("wfx_bow_release"), Owner->GetActorLocation(), .7f + .3f * Charge);
    // o clipe de soltar a corda (golpe 0 = Vx_Bow_Release), para todos
    ++AttackState.Serial;
    AttackState.SwingIndex = 0;
    AttackState.ServerStartTime = GetWorld()->GetTimeSeconds();
    Owner->ForceNetUpdate();
    BowReadyTime = GetWorld()->GetTimeSeconds() + Kit->Value(TEXT("gap"), BowGap);
    Later(FMath::Max(.2f, Shot.Duration), [this]()
    {
        if (AttackState.SwingIndex != 0 || bDrawing) return;
        AttackState.SwingIndex = INDEX_NONE;
        ++AttackState.Serial;
        if (GetOwner()) GetOwner()->ForceNetUpdate();
    });
}
