#include "Creatures/VortexWolf.h"
#include "Rpg/VortexRpg.h"
#include "World/VortexTown.h"
#include "Combat/VortexWeaponComponent.h"
#include "Framework/VortexAudio.h"

#include "AbilitySystemComponent.h"
#include "Animation/VortexWolfAnimInstance.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexWeaponFx.h"
#include "Creatures/VortexGroupCoordinator.h"
#include "Combat/VortexTelegraph.h"
#include "Creatures/VortexSteering.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Creatures/VortexMist.h"
#include "Components/BoxComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "World/VortexTerrainStreamer.h"
#include "EngineUtils.h"

AVortexWolf::AVortexWolf()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(20.f);
    GetCapsuleComponent()->InitCapsuleSize(72.f, 79.f);
    SetActorHiddenInGame(true);
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 430.f, 0.f);
    GetCharacterMovement()->MaxWalkSpeed = 520.f;
    // arranca em ~0,25 s e para em ~0,3 s; antes a velocidade era imposta 10 vezes por segundo e
    // o freio padrão a zerava entre uma decisão e outra (andar aos trancos, patas paradas)
    GetCharacterMovement()->MaxAcceleration = 2600.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1800.f;
    GetCharacterMovement()->GroundFriction = 6.f;
    GetCharacterMovement()->MaxStepHeight = 40.f;
    GetCharacterMovement()->SetWalkableFloorAngle(45.f);
    AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    AbilitySystem->SetIsReplicated(true);
    AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
    Vitals = CreateDefaultSubobject<UVortexAttributes>(TEXT("Vitals"));
    Combat = CreateDefaultSubobject<UVortexCombatComponent>(TEXT("Combat"));
    Combat->Faction = EVortexFaction::Hostile;
    BodyHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("WolfBody"));
    BodyHitbox->SetupAttachment(GetCapsuleComponent());
    BodyHitbox->SetBoxExtent(FVector(136.f, 42.f, 75.f));
    BodyHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BodyHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
    // o corpo (bem mais longo que a cápsula) bloqueia personagens: o herói não entra mais no lobo
    BodyHitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    // a câmera de quem joga atravessa personagens (sem isto ela pulava para dentro do herói ao encostar)
    GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

    // lobo ametista (Meshy, Tools/BuildAmethystWolf.py + ImportAmethystWolf.py): 2,25 m no arquivo,
    // 1,2 vez para ter o porte do lobo antigo (2,7 m, a caixa do corpo)
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> WolfMesh(
        TEXT("/Game/Vortex/Creatures/Wolf/Ametista/SK_LoboAmetista.SK_LoboAmetista"));
    if (WolfMesh.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(WolfMesh.Object);
        GetMesh()->SetAnimInstanceClass(UVortexWolfAnimInstance::StaticClass());
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -79.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
        GetMesh()->SetRelativeScale3D(FVector(1.2f));
        GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        GetMesh()->bEnableUpdateRateOptimizations = true;   // de longe anima menos vezes por segundo
    }
}

void AVortexWolf::BeginPlay()
{
    Super::BeginPlay();
    AbilitySystem->InitAbilityActorInfo(this, this);
    // o alfa é maior (em todas as máquinas: o bAlpha chega junto com o lobo)
    if (bAlpha) SetActorScale3D(FVector(1.35f));
    // névoa roxa de corrupção saindo das patas (como nos outros monstros, AVortexMonster::ApplyMist)
    if (GetNetMode() != NM_DedicatedServer)
        if (UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Soul.NS_Free_Spells_Aura_Soul")))
            if (UNiagaraComponent* Mist = UNiagaraFunctionLibrary::SpawnSystemAttached(System, GetCapsuleComponent(), NAME_None,
                FVector(0.f, 0.f, 5.f - GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false, true, ENCPoolMethod::None))
            {
                VortexMist::MakePurpleMist(Mist);
                Mist->SetRelativeScale3D(FVector(.6f, .6f, .3f));
                WolfMist = Mist;
            }
    if (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It) { bUsesTerrainStreamer = true; }
    if (!bUsesTerrainStreamer && HasAuthority()) bTerrainReady = true;
    bLocalTerrainReady = !bUsesTerrainStreamer;
    RefreshTerrainPresentation();
    if (bUsesTerrainStreamer)
        GetWorldTimerManager().SetTimer(TerrainReadyHandle, this, &AVortexWolf::TryActivateTerrain, .1f, true, .1f);
    if (!HasAuthority()) return;
    if (HomeLocation.IsNearlyZero()) HomeLocation = GetActorLocation();
    const FVortexRpgCatalog& Rpg = FVortexRpgCatalog::Get();   // Data/Rpg/balance.json
    const float MaxHealth = FMath::RoundToFloat((Rpg.MonsterHitBase + Rpg.MonsterHitPerLevel * (Level - 1)) * 4.f * 2.7f * (bAlpha ? 2.2f : 1.f));
    AttackDamage = FMath::RoundToFloat(14.f * (1.f + Rpg.MonsterDamagePerLevel * (Level - 1)) * (bAlpha ? 1.5f : 1.f));
    AbilitySystem->SetNumericAttributeBase(UVortexAttributes::GetMaxHealthAttribute(), MaxHealth);
    AbilitySystem->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), MaxHealth);
    LastHealth = MaxHealth;
    if (PackId >= 0) Coordinator()->RegisterMember(PackId, this,
        PackSlot == 0 ? EVortexGroupRole::Leader : PackSlot % 2 ? EVortexGroupRole::Flanker : EVortexGroupRole::Anchor);
    AbilitySystem->GetGameplayAttributeValueChangeDelegate(UVortexAttributes::GetHealthAttribute()).AddUObject(this, &AVortexWolf::OnHealthChanged);
    // aparado no tempo certo (ou golpe que atordoa): o bote é cancelado e ele fica parado, exposto
    Combat->OnStunned.AddWeakLambda(this, [this](float Seconds)
    {
        if (WolfState == EVortexWolfState::Dead) return;
        GetWorldTimerManager().ClearTimer(ImpactHandle);
        if (PackId >= 0) Coordinator()->ReleaseAttack(PackId, this, GetWorld()->GetTimeSeconds());
        StopMoving();
        SetState(EVortexWolfState::Hit);
        NextAttackTime = FMath::Max(NextAttackTime, GetWorld()->GetTimeSeconds() + Seconds + .3f);
    });
    if (bTerrainReady) { GetWorldTimerManager().SetTimer(ThinkHandle, this, &AVortexWolf::Think, .1f, true, .15f); SetActorTickEnabled(true); }
}

void AVortexWolf::OnRep_TerrainReady()
{
    RefreshTerrainPresentation();
}

void AVortexWolf::RefreshTerrainPresentation()
{
    const bool bReady = bTerrainReady && bLocalTerrainReady;
    SetActorHiddenInGame(!bReady);
    SetActorEnableCollision(bReady);
    if (!bReady) GetCharacterMovement()->DisableMovement();
    else if (GetCharacterMovement()->MovementMode == MOVE_None) GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    if (bReady && !bPresentationReadyLogged)
    {
        bPresentationReadyLogged = true;
        UE_LOG(LogTemp, Display, TEXT("VORTEX_WOLF_VISIBLE net=%d z=%.1f tick=%d"),
            int32(GetNetMode()), GetActorLocation().Z, int32(GetMesh()->VisibilityBasedAnimTickOption));
    }
}

void AVortexWolf::TryActivateTerrain()
{
    AVortexTerrainStreamer* Streamer = nullptr;
    if (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It) { Streamer = *It; }
    if (!Streamer) return;
    float GroundZ = 0.f;
    const FVector Here = GetActorLocation();
    if (!Streamer->GroundReadyAt(Here.X, Here.Y, GroundZ)) return;
    if (!HasAuthority())
    {
        const float WantedZ = GroundZ + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f;
        if (!bTerrainReady || FMath::Abs(Here.Z - WantedZ) > 100.f) return;
        bLocalTerrainReady = true;
        RefreshTerrainPresentation();
        GetWorldTimerManager().ClearTimer(TerrainReadyHandle);
        return;
    }
    GetWorldTimerManager().ClearTimer(TerrainReadyHandle);
    SetActorLocation(FVector(Here.X, Here.Y, GroundZ + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f),
        false, nullptr, ETeleportType::TeleportPhysics);
    HomeLocation = GetActorLocation();
    bTerrainReady = true;
    bLocalTerrainReady = true;
    RefreshTerrainPresentation();
    { GetWorldTimerManager().SetTimer(ThinkHandle, this, &AVortexWolf::Think, .1f, true, .15f); SetActorTickEnabled(true); }
    ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("VORTEX_WOLF_GROUND_READY z=%.1f"), GetActorLocation().Z);
}

void AVortexWolf::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(ThinkHandle);
    GetWorldTimerManager().ClearTimer(ImpactHandle);
    GetWorldTimerManager().ClearTimer(TerrainReadyHandle);
    if (HasAuthority() && PackId >= 0)
    {
        Coordinator()->ReleaseAttack(PackId, this, GetWorld()->GetTimeSeconds());
        Coordinator()->UnregisterMember(PackId, this);
    }
    Super::EndPlay(Reason);
}

UVortexGroupCoordinator* AVortexWolf::Coordinator() const
{
    return GetWorld()->GetSubsystem<UVortexGroupCoordinator>();
}

void AVortexWolf::SetPack(int32 NewId, int32 NewSlot)
{
    if (HasActorBegunPlay() && HasAuthority() && PackId >= 0) Coordinator()->UnregisterMember(PackId, this);
    PackId = NewId;
    PackSlot = NewSlot;
    if (HasActorBegunPlay() && HasAuthority() && PackId >= 0)
        Coordinator()->RegisterMember(PackId, this, PackSlot == 0 ? EVortexGroupRole::Leader :
            PackSlot % 2 ? EVortexGroupRole::Flanker : EVortexGroupRole::Anchor);
}

FString AVortexWolf::GetCreatureAction() const
{
    switch (WolfState)
    {
    case EVortexWolfState::Observe: return TEXT("Observando");
    case EVortexWolfState::Howl: return TEXT("Uivando: a matilha acelera");
    case EVortexWolfState::Chase: return TEXT("Perseguindo");
    case EVortexWolfState::Circle: return TEXT("Rondando");
    case EVortexWolfState::Bite: return TEXT("Mordida");
    case EVortexWolfState::Pounce: return TEXT("Bote");
    case EVortexWolfState::Rear: return TEXT("Salto de Ametista");
    case EVortexWolfState::Combo: return TEXT("Mordida Dupla");
    case EVortexWolfState::Retreat: return TEXT("Recuando");
    case EVortexWolfState::ReturnHome: return TEXT("Voltando para a toca");
    case EVortexWolfState::Hit: return TEXT("Atordoado");
    case EVortexWolfState::Dead: return TEXT("Derrotado");
    default: return TEXT("Descansando");
    }
}

float AVortexWolf::GetHealthFraction() const
{
    const float Max = AbilitySystem ? AbilitySystem->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute()) : 0.f;
    return Max > 0.f ? FMath::Clamp(Health() / Max, 0.f, 1.f) : 0.f;
}

bool AVortexWolf::IsEngaged() const
{
    return WolfState != EVortexWolfState::Idle && WolfState != EVortexWolfState::ReturnHome && WolfState != EVortexWolfState::Dead;
}

float AVortexWolf::Health() const
{
    return AbilitySystem ? AbilitySystem->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) : 0.f;
}

void AVortexWolf::SetState(EVortexWolfState NewState)
{
    if (WolfState == NewState) return;
    WolfState = NewState;
    ActionServerTime = GetWorld()->GetTimeSeconds();
    ++ActionSerial;
    ForceNetUpdate();
}

void AVortexWolf::StopMoving()
{
    MoveSpeed = 0.f;
}

void AVortexWolf::UpdateSounds(float DeltaSeconds)
{
    if (GetNetMode() == NM_DedicatedServer || IsHidden()) return;
    const FVector At = GetActorLocation();
    GrowlCooldown = FMath::Max(0.f, GrowlCooldown - DeltaSeconds);
    if (WolfState != HeardState)
    {
        switch (WolfState)
        {
        case EVortexWolfState::Howl: VortexAudio::Play(this, TEXT("wolf_howl"), At, 1.f, .08f); break;
        case EVortexWolfState::Pounce: VortexAudio::Play(this, TEXT("wolf_pounce"), At, .9f); break;
        case EVortexWolfState::Observe:
        case EVortexWolfState::Chase:
        case EVortexWolfState::Circle:
            if (GrowlCooldown <= 0.f) { VortexAudio::Play(this, TEXT("wolf_growl"), At, .8f, .1f); GrowlCooldown = 5.f; }
            break;
        case EVortexWolfState::Dead: VortexAudio::Play(this, VortexAudio::Has(TEXT("wolf_death")) ? FName(TEXT("wolf_death")) : FName(TEXT("creature_hit")), At, 1.f, .04f); break;
        default: break;
        }
        HeardState = WolfState;
    }
    // patas: só correndo, baixinho
    const float Speed = GetVelocity().Size2D();
    WolfStepTimer -= DeltaSeconds;
    if (Speed > 180.f && WolfStepTimer <= 0.f && GetCharacterMovement()->IsMovingOnGround())
    {
        WolfStepTimer = Speed > 450.f ? .24f : .34f;
        const FVector Feet = At - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
        VortexAudio::Play(this, VortexAudio::StepSound(Feet, WolfStepVariant++), Feet, .3f, .15f);
    }
}

void AVortexWolf::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateSounds(DeltaSeconds);
    if (WolfState == EVortexWolfState::Dead && WolfMist.IsValid() && WolfMist->IsActive()) WolfMist->Deactivate();
    // morto: o corpo não segura mais ninguém (em todas as máquinas: o herói de quem entrou pela sala anda no cliente)
    if (WolfState == EVortexWolfState::Dead && BodyHitbox->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Ignore)
    {
        BodyHitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    }
    if (!HasAuthority() || WolfState == EVortexWolfState::Dead) return;
    // rondando devagar: encara o herói e anda de lado (StrafeL/StrafeR na animação); indo ao lugar
    // na roda em velocidade, vira para onde corre
    const bool bFaceHero = WolfState == EVortexWolfState::Circle && Target.IsValid() && MoveSpeed <= 380.f;
    GetCharacterMovement()->bOrientRotationToMovement = !bFaceHero;
    if (bFaceHero)
    {
        const FVector ToHero = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
        if (!ToHero.IsNearlyZero())
            SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), FRotator(0.f, ToHero.Rotation().Yaw, 0.f), DeltaSeconds, 430.f));
    }
    // A cápsula do movimento (raio 72 cm) é bem menor que o corpo (caixa de ~2,7 m): sem isto a
    // cabeça e o salto entravam no herói. Perto do corpo: não avança mais na direção dele, perde o
    // embalo naquela direção e, se já entrou, se afasta.
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const ACharacter* Hero = It->Get() ? Cast<ACharacter>(It->Get()->GetPawn()) : nullptr;
        if (!Hero) continue;
        const FTransform Frame(BodyHitbox->GetComponentQuat(), BodyHitbox->GetComponentLocation());
        const FVector Local = Frame.InverseTransformPosition(Hero->GetActorLocation());
        const FVector Ext = BodyHitbox->GetScaledBoxExtent();
        const float R = Hero->GetCapsuleComponent()->GetScaledCapsuleRadius();
        if (FMath::Abs(Local.Z) > Ext.Z + 120.f) continue;
        const float GapX = FMath::Abs(Local.X) - (Ext.X + R), GapY = FMath::Abs(Local.Y) - (Ext.Y + R);
        if (GapX > 25.f || GapY > 25.f) continue;
        const FVector ToHero = (Hero->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
        UCharacterMovementComponent* Movement = GetCharacterMovement();
        const float Closing = FVector::DotProduct(Movement->Velocity, ToHero);
        if (Closing > 0.f) Movement->Velocity -= ToHero * Closing;
        if (GapX < -5.f && GapY < -5.f)
        {
            // dentro: sai para trás, sem empurrar o herói
            Movement->MaxWalkSpeed = 300.f;
            AddMovementInput(-ToHero, 1.f);
            return;
        }
        if (MoveSpeed > 0.f && FVector::DotProduct(MoveDirection, ToHero) > .3f) return;
    }
    if (MoveSpeed <= 0.f || Combat->IsRooted()) return;
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed * Combat->MoveScale();
    // parede de fogo do grimório: desvia (pedra e fogo atravessam)
    MoveDirection = AVortexWeaponFx::SteerAroundWalls(this, MoveDirection);
    AddMovementInput(MoveDirection, 1.f);
}

void AVortexWolf::MoveToward(const FVector& Location, float Speed)
{
    FVector Direction = (Location - GetActorLocation()).GetSafeNormal2D();
    if (Direction.IsNearlyZero()) { StopMoving(); return; }
    // desvio de árvores e pedras (VortexSteering): varre a cápsula à frente e procura o ângulo livre
    // mais perto; preso há mais de 1,2 s, dá um passo firme de lado. O corpo do lobo é comprido:
    // olha mais longe que a cápsula.
    FVector Forced;
    if (VortexSteering::Unstick(this, Direction, .1f, Steer, Forced)) Direction = Forced;
    else Direction = VortexSteering::Avoid(this, Direction, FMath::Clamp(Speed * .4f, 180.f, 320.f) + 80.f, Steer);
    // matilha: não se empilham
    Direction = (Direction + VortexSteering::Separation(this, 220.f) * .5f).GetSafeNormal2D();
    MoveDirection = Direction;
    MoveSpeed = Speed;
}
bool AVortexWolf::CanSee(const AVortexCharacter* Candidate) const
{
    if (!Candidate || !Candidate->GetAbilitySystemComponent() ||
        Candidate->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) <= 0.f)
        return false;
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexWolfSight), false, this);
    const FVector From = GetActorLocation() + FVector(0, 0, 60.f);
    const FVector To = Candidate->GetActorLocation() + FVector(0, 0, 60.f);
    return !GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Query) || Hit.GetActor() == Candidate;
}

namespace
{
    // a vila só existe no continente (o AVortexTown nasce com o terreno): nos mapas de teste, perto da origem, a regra
    // da vila fazia o lobo largar a caça (RunWolfSmoke e RunPackSmoke falhavam)
    bool InTown(UWorld* World, const FVector& At)
    {
        return AVortexTown::Contains(At) && TActorIterator<AVortexTown>(World);
    }
}

void AVortexWolf::Think()
{
    if (WolfState == EVortexWolfState::Dead) return;
    const float Now = GetWorld()->GetTimeSeconds();
    bFrenzied = Now < FrenzyUntil;
    if (Combat->IsStunned()) { StopMoving(); return; }
    if (WolfState == EVortexWolfState::Bite || WolfState == EVortexWolfState::Pounce ||
        WolfState == EVortexWolfState::Howl || WolfState == EVortexWolfState::Hit ||
        WolfState == EVortexWolfState::Rear || WolfState == EVortexWolfState::Combo)
    {
        // The pounce starts outside bite range. Advance during its windup so the
        // impact check can actually reach the target, then brake for recovery.
        if (WolfState == EVortexWolfState::Pounce && Now < ActionServerTime + .55f &&
            Target.IsValid() && FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation()) > FMath::Square(150.f))
            MoveToward(Target->GetActorLocation(), 780.f);
        else
            StopMoving();
        // 02/11: parado só enquanto a ação toca (a mordida ~0,7 s); antes esperava o próximo ataque liberar (1,2 s) e
        // o lobo ficava congelado na pose final. Depois ele volta a andar (e só ataca de novo quando liberar)
        const float Lock = WolfState == EVortexWolfState::Bite ? .7f : WolfState == EVortexWolfState::Pounce ? 1.f
            : WolfState == EVortexWolfState::Hit ? .45f : WolfState == EVortexWolfState::Rear ? 1.3f
            : WolfState == EVortexWolfState::Combo ? .55f : 1.6f;
        if (Now < FMath::Min(NextAttackTime, ActionServerTime + Lock)) return;
    }
    // 02/11: a vila é proibida: o lobo que entrou larga a caça e volta para a toca
    if (InTown(GetWorld(), GetActorLocation()))
    {
        Target = nullptr;
        SetState(EVortexWolfState::ReturnHome);
        MoveToward(HomeLocation, 300.f);
        return;
    }
    AVortexCharacter* Nearest = nullptr;
    float BestDistanceSq = FMath::Square(2500.f);
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AVortexCharacter* Candidate = It->Get() ? Cast<AVortexCharacter>(It->Get()->GetPawn()) : nullptr;
        // dentro da cidade ninguém é atacado (como os outros monstros, AVortexMonster::FindTarget)
        if (!Candidate || InTown(GetWorld(), Candidate->GetActorLocation())) continue;
        const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
        if (DistanceSq < BestDistanceSq && CanSee(Candidate)) { Nearest = Candidate; BestDistanceSq = DistanceSq; }
    }
    Target = Nearest;
    if (Nearest && PackId >= 0) Coordinator()->ReportTarget(PackId, Nearest);
    if (!Nearest && PackId >= 0)
    {
        AVortexCharacter* Shared = Coordinator()->SharedTarget(PackId);
        if (Shared && !InTown(GetWorld(), Shared->GetActorLocation()) && FVector::DistSquared2D(GetActorLocation(), Shared->GetActorLocation()) < FMath::Square(2500.f) && CanSee(Shared))
        {
            Nearest = Shared;
            Target = Shared;
            BestDistanceSq = FVector::DistSquared2D(GetActorLocation(), Shared->GetActorLocation());
        }
    }
    if (!Nearest)
    {
        if (FVector::DistSquared2D(GetActorLocation(), HomeLocation) > FMath::Square(100.f))
        {
            SetState(EVortexWolfState::ReturnHome);
            MoveToward(HomeLocation, 260.f);
        }
        else { SetState(EVortexWolfState::Idle); StopMoving(); }
        return;
    }
    const float Distance = FMath::Sqrt(BestDistanceSq);
    const FVector HeroAt = Nearest->GetActorLocation();
    const FVector Away = (GetActorLocation() - HeroAt).GetSafeNormal2D();
    const FVector HeroForward = Nearest->GetActorForwardVector().GetSafeNormal2D();
    // de costas para este lobo (ou olhando para outro): a hora de atacar
    const float Facing = FVector::DotProduct(HeroForward, Away);
    const bool bBehind = Facing < .15f;
    const int32 Alive = PackId >= 0 ? FMath::Max(1, Coordinator()->AliveCount(PackId)) : 1;
    const int32 Rank = PackId >= 0 ? Coordinator()->MemberIndex(PackId, this) : 0;
    if (PackId >= 0) Coordinator()->SetMaxAttackers(PackId, Alive >= 4 || bFrenzied ? 2 : 1);

    // matilha quebrada (alfa caído e menos de 40% de pé): cada um foge para um lado e some na mata
    if (PackId >= 0 && !bBrokenFled && Coordinator()->IsBroken(PackId))
    {
        bBrokenFled = true;
        BrokenFleeUntil = Now + 6.f;
    }
    if (Now < BrokenFleeUntil)
    {
        SetState(EVortexWolfState::Retreat);
        MoveToward(GetActorLocation() + (Away + FVector(-Away.Y, Away.X, 0.f) * ((PackSlot % 2) ? .5f : -.5f)) * 600.f, 620.f);
        return;
    }
    if (bBrokenFled)
    {
        // passado o susto: volta para a toca e esquece
        SetState(EVortexWolfState::ReturnHome);
        MoveToward(HomeLocation, 300.f);
        if (FVector::DistSquared2D(GetActorLocation(), HomeLocation) < FMath::Square(150.f)) bBrokenFled = false;
        return;
    }

    // 10/11 (especial): o alfa com menos da metade da vida uiva de novo e um anel de cristais de ametista estoura em volta
    // dele (o chão marca antes; quem fica perto apanha). Uma vez por luta
    if (bAlpha && !bCrystalBurst && GetHealthFraction() < .5f)
    {
        bCrystalBurst = true;
        SetState(EVortexWolfState::Howl);
        StopMoving();
        FrenzyUntil = Now + 6.f;
        bFrenzied = true;
        NextAttackTime = Now + 1.8f;
        AVortexTelegraph::Spawn(this, GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), 420.f, .95f,
            FMath::RoundToFloat(AttackDamage * 1.1f), true, FLinearColor(.65f, .3f, 1.f), true);
        return;
    }
    if (!bHasHowled && (PackId < 0 || Rank == 0) && Distance > 300.f)
    {
        bHasHowled = true;
        SetState(EVortexWolfState::Howl);
        StopMoving();
        FrenzyUntil = Now + 7.f;
        bFrenzied = true;
        NextAttackTime = Now + 1.8f;
        return;
    }
    if (Now < RetreatUntil)
    {
        SetState(EVortexWolfState::Retreat);
        // recua de lado (não em linha reta, para não ficar na frente do golpe)
        MoveToward(GetActorLocation() + (Away + FVector(-Away.Y, Away.X, 0.f) * ((Rank % 2) ? .6f : -.6f)) * 400.f, 430.f);
        return;
    }
    // o herói começou um golpe e este lobo está na frente dele: pula para trás (às vezes)
    if (const UVortexWeaponComponent* Gear = Nearest->GetWeapon())
    {
        const FVortexAttackState& Swing = Gear->GetAttackState();
        if (Swing.Serial != SeenHeroSwing)
        {
            SeenHeroSwing = Swing.Serial;
            const bool bThreat = Swing.SwingIndex != INDEX_NONE && Distance < 360.f && Facing > .55f;
            if (bThreat && Now >= NextEvadeTime && FMath::FRand() < (bFrenzied ? .3f : .5f))
            {
                NextEvadeTime = Now + 2.6f;
                RetreatUntil = Now + .55f;
                if (PackId >= 0) Coordinator()->ReleaseAttack(PackId, this, Now);
                SetState(EVortexWolfState::Retreat);
                MoveToward(GetActorLocation() + Away * 400.f, 700.f);
                return;
            }
        }
    }
    // vontade de atacar: cresce com o tempo esperando; de costas para o lobo, ataca logo
    const float Waiting = Now - LastAttackEnd;
    const bool bWants = PackId < 0 || bBehind || Waiting > (Rank == 0 ? 2.2f : 3.2f) || (bFrenzied && Waiting > 1.4f);
    if (Now >= NextAttackTime && bWants)
    {
        if (Distance < 300.f && StartAttack(EVortexWolfState::Bite)) return;
        if (Distance >= 300.f && Distance < 520.f && StartAttack(EVortexWolfState::Pounce)) return;
        // 10/11 (especial): de 5 a 9,5 m, de vez em quando: empina e salta onde o herói está (o chão marca o lugar)
        if (Distance >= 500.f && Distance < 950.f && Now >= NextLeapTime && (PackId < 0 || Coordinator()->AcquireAttack(PackId, this, Now)))
        {
            StartLeap(Nearest);
            return;
        }
    }
    if (Distance < 900.f)
    {
        // cerco: o alfa encara pela frente (chama a atenção); os outros se espalham pelos lados e por
        // trás do herói, cada um no seu ângulo, e vão fechando a roda enquanto esperam a vez
        const float HeroYaw = FMath::RadiansToDegrees(FMath::Atan2(HeroForward.Y, HeroForward.X));
        float SlotYaw = HeroYaw;
        if (Rank > 0 || PackId < 0)
        {
            const int32 Others = FMath::Max(1, Alive - 1);
            const int32 K = PackId < 0 ? 0 : Rank - 1;
            SlotYaw = HeroYaw + (Others == 1 ? 150.f : 75.f + 210.f * K / float(Others - 1));
        }
        SlotYaw += FMath::Sin(Now * .7f + PackSlot * 1.7f) * 18.f;
        const float Tighten = FMath::Clamp(Waiting / 3.f, 0.f, 1.f);
        const float Radius = (Rank == 0 && PackId >= 0 ? 400.f : 430.f + 30.f * (Rank % 2)) - 110.f * Tighten;
        const FVector Slot = HeroAt + FRotator(0.f, SlotYaw, 0.f).Vector() * Radius;
        SetState(EVortexWolfState::Circle);
        const float ToSlot = FVector::Dist2D(GetActorLocation(), Slot);
        // finta: o lobo da frente avança meio passo e recua, para o herói errar o tempo
        if (Rank == 0 && PackId >= 0 && Now >= NextFeintTime && Distance < 520.f && Distance > 330.f)
        {
            NextFeintTime = Now + FMath::FRandRange(3.f, 5.f);
            MoveToward(HeroAt, 640.f);
            RetreatUntil = Now + .9f;
            return;
        }
        if (ToSlot > 70.f)
        {
            // chega ao lugar pelo lado de fora (não corta pela frente do herói)
            const FVector Around = (Slot - HeroAt).GetSafeNormal2D() * FMath::Max(Radius, Distance);
            const FVector Via = FVector::DotProduct((Slot - GetActorLocation()).GetSafeNormal2D(), -Away) > .5f ? HeroAt + Around : Slot;
            MoveToward(Via, ToSlot > 400.f ? 560.f * (bFrenzied ? 1.15f : 1.f) : 340.f);
        }
        else
        {
            // no lugar: anda de lado em volta, sem parar
            const FVector Tangent = FVector(-Away.Y, Away.X, 0.f) * ((PackSlot % 2) ? 1.f : -1.f);
            MoveToward(GetActorLocation() + Tangent * 200.f, 200.f);
        }
        return;
    }
    SetState(EVortexWolfState::Chase);
    MoveToward(HeroAt, 560.f * (bFrenzied ? 1.25f : 1.f));
}

bool AVortexWolf::StartAttack(EVortexWolfState Attack)
{
    if (PackId >= 0 && !Coordinator()->AcquireAttack(PackId, this, GetWorld()->GetTimeSeconds())) return false;
    SetState(Attack);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_WOLF_ATTACK state=%d distance=%.1f"),
        int32(Attack), Target.IsValid() ? FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) : -1.f);
    StopMoving();
    const int32 Serial = ActionSerial;
    const float Impact = Attack == EVortexWolfState::Bite ? .3f : .55f;
    NextAttackTime = GetWorld()->GetTimeSeconds() + Impact + (Attack == EVortexWolfState::Bite ? .9f : 1.f);
    GetWorldTimerManager().SetTimer(ImpactHandle, FTimerDelegate::CreateWeakLambda(this, [this, Attack, Serial]()
    {
        ResolveAttack(Attack, Serial);
    }), Impact, false);
    return true;
}

void AVortexWolf::StartLeap(AVortexCharacter* Victim)
{
    // Salto de Ametista: empina (0,55 s), salta onde o herói estava quando empinou (o chão marca: dá para sair) e os
    // cristais estouram na queda
    const float Now = GetWorld()->GetTimeSeconds();
    SetState(EVortexWolfState::Rear);
    StopMoving();
    NextLeapTime = Now + (bAlpha ? 7.f : 9.5f);
    NextAttackTime = Now + 2.f;
    const FVector Feet = Victim->GetActorLocation() - FVector(0.f, 0.f, Victim->GetSimpleCollisionHalfHeight());
    LeapTo = Feet;
    AVortexTelegraph::Spawn(this, Feet, 260.f, 1.15f, FMath::RoundToFloat(AttackDamage * 1.2f), false, FLinearColor(.65f, .3f, 1.f), true);
    const int32 Serial = ActionSerial;
    GetWorldTimerManager().SetTimer(ImpactHandle, FTimerDelegate::CreateWeakLambda(this, [this, Serial]()
    {
        if (WolfState != EVortexWolfState::Rear || ActionSerial != Serial || Health() <= 0.f) return;
        // o pulo em arco até o lugar marcado (~0,6 s no ar)
        const FVector Flat = LeapTo - GetActorLocation();
        const float Air = .6f;
        const float Gravity = FMath::Abs(GetCharacterMovement()->GetGravityZ());
        FVector Velocity = FVector(Flat.X, Flat.Y, 0.f) / Air;
        Velocity.Z = (Flat.Z + .5f * Gravity * Air * Air) / Air;
        LaunchCharacter(Velocity, true, true);
        GetWorldTimerManager().SetTimer(ImpactHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            RetreatUntil = GetWorld()->GetTimeSeconds() + .8f;
            LastAttackEnd = GetWorld()->GetTimeSeconds();
            if (PackId >= 0) Coordinator()->ReleaseAttack(PackId, this, GetWorld()->GetTimeSeconds());
        }), Air, false);
    }), .55f, false);
}

void AVortexWolf::ResolveAttack(EVortexWolfState Attack, int32 Serial)
{
    if (WolfState != Attack || ActionSerial != Serial || Health() <= 0.f) return;
    AVortexCharacter* Victim = Target.Get();
    if (Victim)
    {
        const float Multiplier = Attack == EVortexWolfState::Bite ? .6f : 1.f;
        const float Reach = Attack == EVortexWolfState::Bite ? 310.f : 340.f;
        const bool bHit = Combat->TryMeleeHit(Victim, FMath::Max(1.f, FMath::RoundToFloat(AttackDamage * Multiplier)), Reach, .1f);
        if (!bHit) UE_LOG(LogTemp, Display, TEXT("VORTEX_WOLF_MISS state=%d distance=%.1f height=%.1f"),
            int32(Attack), FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()),
            FMath::Abs(GetActorLocation().Z - Victim->GetActorLocation().Z));
        // 10/11 (especial): a mordida que acertou às vezes emenda a segunda, mais rápida (mais vezes no frenesi)
        if (bHit && Attack == EVortexWolfState::Bite && FMath::FRand() < (bFrenzied ? .6f : .35f))
        {
            SetState(EVortexWolfState::Combo);
            const int32 ComboSerial = ActionSerial;
            NextAttackTime = GetWorld()->GetTimeSeconds() + .3f + .9f;
            GetWorldTimerManager().SetTimer(ImpactHandle, FTimerDelegate::CreateWeakLambda(this, [this, ComboSerial]()
            {
                if (WolfState != EVortexWolfState::Combo || ActionSerial != ComboSerial || Health() <= 0.f) return;
                if (AVortexCharacter* Second = Target.Get())
                    Combat->TryMeleeHit(Second, FMath::Max(1.f, FMath::RoundToFloat(AttackDamage * .5f)), 300.f, .1f);
                RetreatUntil = GetWorld()->GetTimeSeconds() + .6f;
                LastAttackEnd = GetWorld()->GetTimeSeconds();
                if (PackId >= 0) Coordinator()->ReleaseAttack(PackId, this, GetWorld()->GetTimeSeconds());
            }), .3f, false);
            return;
        }
    }
    if (Attack == EVortexWolfState::Pounce) RetreatUntil = NextAttackTime + .9f;
    else RetreatUntil = GetWorld()->GetTimeSeconds() + .6f;   // morde e sai (bate e corre)
    LastAttackEnd = GetWorld()->GetTimeSeconds();
    if (PackId >= 0) Coordinator()->ReleaseAttack(PackId, this, GetWorld()->GetTimeSeconds());
}

void AVortexWolf::OnHealthChanged(const FOnAttributeChangeData& Data)
{
    if (!HasAuthority() || WolfState == EVortexWolfState::Dead) return;
    if (Data.NewValue <= 0.f)
    {
        GetWorldTimerManager().ClearTimer(ImpactHandle);
        if (PackId >= 0) Coordinator()->ReleaseAttack(PackId, this, GetWorld()->GetTimeSeconds());
        GetWorldTimerManager().ClearTimer(ThinkHandle);
        StopMoving();
        SetState(EVortexWolfState::Dead);
        SetLifeSpan(4.f);
    }
    else if (Data.NewValue < LastHealth)
    {
        if (Data.NewValue <= AbilitySystem->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute()) * .25f && !bRetreatedLowHealth)
        {
            bRetreatedLowHealth = true;
            RetreatUntil = GetWorld()->GetTimeSeconds() + 2.2f;
        }
        // encolhe no máximo a cada 2,2 s (a cada golpe o lobo ficava preso na animação)
        const float Now = GetWorld()->GetTimeSeconds();
        // fúria: no 3º golpe seguido o lobo não fica apanhando; salta para trás ou morde de volta na hora
        HitStreak = Now - LastHitAt < 1.5f ? HitStreak + 1 : 1;
        LastHitAt = Now;
        if (HitStreak >= 3 && WolfState != EVortexWolfState::Bite && WolfState != EVortexWolfState::Pounce)
        {
            HitStreak = 0;
            FlinchReadyAt = Now + 2.2f;
            Combat->StunGuardUntil = FMath::Max(Combat->StunGuardUntil, double(Now) + 2.5);
            if (FMath::FRand() < .5f) RetreatUntil = Now + .5f;
            else NextAttackTime = Now;
        }
        else if (WolfState != EVortexWolfState::Bite && WolfState != EVortexWolfState::Pounce && Now >= FlinchReadyAt)
        {
            FlinchReadyAt = Now + 2.2f;
            SetState(EVortexWolfState::Hit);
            NextAttackTime = FMath::Max(NextAttackTime, Now + .3f);
        }
    }
    LastHealth = Data.NewValue;
}

void AVortexWolf::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AVortexWolf, Level, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWolf, bAlpha, COND_InitialOnly);
    DOREPLIFETIME(AVortexWolf, WolfState);
    DOREPLIFETIME(AVortexWolf, ActionServerTime);
    DOREPLIFETIME(AVortexWolf, ActionSerial);
    DOREPLIFETIME(AVortexWolf, bFrenzied);
    DOREPLIFETIME(AVortexWolf, bTerrainReady);
}
