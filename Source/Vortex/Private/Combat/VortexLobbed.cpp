#include "Combat/VortexLobbed.h"

#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexHitFx.h"
#include "Combat/VortexMonsterFx.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "Rpg/VortexHeroSheet.h"

// 13/11 (o usuário: "se a pedra acertar, derrubar no chão ele"): o herói derrubado cai (o clipe Death, em
// VortexAnimInstance.cpp), fica atordoado e levanta sozinho.
// shortcut: o estado fica só no servidor (convidados sentem o atordoamento, não veem a queda) e a declaração é repetida no
// VortexAnimInstance.cpp; passar para o UVortexCombatComponent (replicado) quando for mexer no cabeçalho dele
namespace VortexKnockdown
{
    static TMap<TWeakObjectPtr<const AActor>, double> Until;

    float Remaining(const AActor* Who)
    {
        const double* End = Who ? Until.Find(Who) : nullptr;
        return End && Who->GetWorld() ? FMath::Max(0.f, float(*End - Who->GetWorld()->GetTimeSeconds())) : 0.f;
    }

    /** Só derruba (quem chama já arremessou): o agarrão do golem. */
    void Down(AVortexCharacter* Hero, float Seconds)
    {
        if (!Hero || !Hero->HasAuthority()) return;
        Until.Add(Hero, Hero->GetWorld()->GetTimeSeconds() + Seconds);
        if (UVortexCombatComponent* Theirs = Hero->GetCombat()) Theirs->Stun(Seconds);
        if (Hero->GetSheet()) Hero->GetSheet()->PushNotice(TEXT("Derrubado!"), 2);
    }

    void Apply(AVortexCharacter* Hero, float Seconds, const FVector& From)
    {
        if (!Hero || !Hero->HasAuthority()) return;
        for (auto It = Until.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
        Until.Add(Hero, Hero->GetWorld()->GetTimeSeconds() + Seconds);
        if (UVortexCombatComponent* Theirs = Hero->GetCombat()) Theirs->Stun(Seconds);
        const FVector Away = (Hero->GetActorLocation() - From).GetSafeNormal2D();
        Hero->LaunchCharacter(Away * 380.f + FVector(0.f, 0.f, 260.f), true, true);
        if (Hero->GetSheet()) Hero->GetSheet()->PushNotice(TEXT("Derrubado!"), 2);
    }
}

AVortexLobbed::AVortexLobbed()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    RootComponent = Body;
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->bAffectDistanceFieldLighting = false;
}

AVortexLobbed* AVortexLobbed::Throw(AActor* Source, const FVector& InFrom, const FVector& InLand, float FlightTime, float InDamage, float InRadius, bool bInRock,
    float InScale, const FLinearColor& InTint)
{
    if (!Source || !Source->HasAuthority()) return nullptr;
    AVortexLobbed* Shot = Source->GetWorld()->SpawnActorDeferred<AVortexLobbed>(StaticClass(), FTransform(InFrom), Source, nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Shot) return nullptr;
    Shot->From = InFrom;
    Shot->Land = InLand;
    Shot->Flight = FMath::Max(.2f, FlightTime);
    Shot->Damage = InDamage;
    Shot->Radius = InRadius;
    Shot->bRock = bInRock;
    Shot->Scale = InScale;
    Shot->Tint = InTint;
    Shot->FinishSpawning(FTransform(InFrom));
    return Shot;
}

void AVortexLobbed::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority()) SetLifeSpan(Flight + 1.5f);
    if (GetNetMode() == NM_DedicatedServer) return;
    // a pedra do goblin: o pedregulho do mundo (com a textura dele) com ~35 cm; era um cubo cinza liso de 50 cm
    if (UStaticMesh* Boulder = bRock ? LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Vortex/Props/rock_boulder/rock_boulder/StaticMeshes/rock_boulder.rock_boulder")) : nullptr)
    {
        Body->SetStaticMesh(Boulder);
        Body->SetWorldScale3D(FVector(35.f * Scale / FMath::Max(1.f, Boulder->GetBounds().BoxExtent.GetMax() * 2.f)));
        return;
    }
    Body->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, bRock ? TEXT("/Engine/BasicShapes/Cube.Cube") : TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Body->SetWorldScale3D(FVector(bRock ? .5f : .38f));
    if (UMaterialInterface* Solid = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SolidColor.M_SolidColor")))
    {
        UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(Solid, this);
        Paint->SetVectorParameterValue(TEXT("Color"), bRock ? FLinearColor(.16f, .14f, .12f) : Tint.R >= 0.f ? Tint : FLinearColor(.25f, 1.2f, .12f));
        Body->SetMaterial(0, Paint);
    }
}

void AVortexLobbed::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bLanded) return;
    Age += DeltaSeconds;
    const float T = FMath::Clamp(Age / Flight, 0.f, 1.f);
    // arco: sobe até 1/3 da distância (no mínimo 2 m) e cai no ponto marcado
    const float Peak = FMath::Max(200.f, FVector::Dist2D(From, Land) * .33f);
    FVector At = FMath::Lerp(FVector(From), FVector(Land), T);
    At.Z += FMath::Sin(T * UE_PI) * Peak;
    SetActorLocation(At);
    SetActorRotation(FRotator(Age * 400.f, Age * 260.f, 0.f));
    // o cuspe de veneno vai pingando no caminho
    if (!bRock && GetNetMode() != NM_DedicatedServer && FMath::FloorToInt(Age / .06f) != FMath::FloorToInt((Age - DeltaSeconds) / .06f))
        AVortexMonsterFx::Spawn(GetWorld(), Tint.R >= 0.f ? TEXT("GelDrip") : TEXT("Drip"), At + FMath::VRand() * 6.f, 10.f, 0.f, nullptr, 0.f, Tint);
    if (T < 1.f) return;
    bLanded = true;
    Body->SetVisibility(false);
    if (!HasAuthority()) return;
    AActor* Source = GetOwner();
    UVortexCombatComponent* Combat = Source ? Source->FindComponentByClass<UVortexCombatComponent>() : nullptr;
    if (Combat)
    {
        for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
        {
            if (FVector::Dist2D(It->GetActorLocation(), Land) > Radius || FMath::Abs(It->GetActorLocation().Z - Land.Z) > 250.f) continue;
            if (!Combat->CanHarm(*It) || !Combat->ApplyHit(*It, Damage, bRock ? EVortexDamageKind::Melee : EVortexDamageKind::Magic)) continue;
            if (Status == TEXT("poison") && It->GetSheet()) It->GetSheet()->ApplyPoison(StatusTime);
            if (bRock && Scale > 1.5f) VortexKnockdown::Apply(*It, 2.2f, From);   // a pedra grande do golem derruba (0,97 s caindo, um instante no chão e 0,87 s levantando)
        }
    }
    MulticastLand();
}

void AVortexLobbed::MulticastLand_Implementation()
{
    VortexAudio::Play(this, bRock ? TEXT("rock_impact") : TEXT("poison_land"), Land, 1.f, .08f);
    VortexHitFx::Splash(GetWorld(), Land, bRock ? FLinearColor(.3f, .26f, .2f) : Tint.R >= 0.f ? Tint : FLinearColor(.3f, .9f, .15f), bRock ? FMath::RoundToInt(18 * Scale) : 14);
    // o veneno estoura numa poça borbulhando (a área do dano)
    if (!bRock) AVortexMonsterFx::Spawn(GetWorld(), Tint.R >= 0.f ? TEXT("GelPool") : TEXT("PoisonPool"), Land, FMath::Max(70.f, Radius * .7f), 0.f, nullptr, 0.f, Tint);
    // a pedra grande racha o chão onde cai
    if (bRock && Scale > 1.5f) AVortexMonsterFx::Spawn(GetWorld(), TEXT("Dust"), Land, Radius * .8f);
    if (bRock)
    {
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            if (AVortexCharacter* Hero = It->IsValid() ? Cast<AVortexCharacter>((*It)->GetPawn()) : nullptr; Hero && Hero->IsLocallyControlled() &&
                FVector::Dist2D(Hero->GetActorLocation(), Land) < 800.f) Hero->KickCamera(.8f);
        }
    }
}

void AVortexLobbed::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AVortexLobbed, From, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexLobbed, Land, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexLobbed, Flight, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexLobbed, bRock, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexLobbed, Scale, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexLobbed, Tint, COND_InitialOnly);
}
