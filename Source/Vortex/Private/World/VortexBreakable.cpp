#include "World/VortexBreakable.h"

#include "Combat/VortexHitFx.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexAudio.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTown.h"
#include "World/VortexTerrainStreamer.h"

namespace
{
    constexpr float PuddleLife = 20.f;
    constexpr float RespawnSeconds = 180.f;

    FLinearColor LiquidColor(FName Kind)
    {
        return Kind == TEXT("wine") ? FLinearColor(.42f, .02f, .09f) : FLinearColor(.45f, .7f, .95f);
    }
}

AVortexBreakable::AVortexBreakable()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicatingMovement(true);
    Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
    Box->SetCollisionProfileName(TEXT("PhysicsActor"));
    Box->SetBoxExtent(FVector(40.f, 40.f, 50.f));
    Box->SetCanEverAffectNavigation(false);
    RootComponent = Box;
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Box);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetCanEverAffectNavigation(false);
}

void AVortexBreakable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AVortexBreakable, Kind, COND_InitialOnly);
    DOREPLIFETIME(AVortexBreakable, bBroken);
    DOREPLIFETIME(AVortexBreakable, BreakAt);
    DOREPLIFETIME(AVortexBreakable, BreakPush);
}

FVector AVortexBreakable::HalfSize() const
{
    return IsBarrel() ? FVector(40.f, 40.f, 50.f) : FVector(37.5f, 37.5f, 37.5f);
}

void AVortexBreakable::BeginPlay()
{
    Super::BeginPlay();
    const FVector Half = HalfSize();
    Box->SetBoxExtent(Half);
    const TCHAR* Model = IsBarrel() ? TEXT("town_barrel") : TEXT("town_crate");
    Body->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Props/%s/%s/StaticMeshes/%s.%s"), Model, Model, Model, Model)));
    Body->SetRelativeLocation(FVector(0.f, 0.f, -Half.Z));
    Home = GetActorTransform();
    // a física liga no Tick, quando já houver chão embaixo (a vila monta antes de o terreno ter colisão: o barril caía
    // através do chão e sumia)
    Box->SetSimulatePhysics(false);
    if (bBroken) OnRep_Broken();
}

int32 AVortexBreakable::BreakNear(UWorld* World, const FVector& At, float Radius, const FVector& Push)
{
    if (!World || World->GetNetMode() == NM_Client) return 0;
    int32 Count = 0;
    for (TActorIterator<AVortexBreakable> It(World); It; ++It)
    {
        if (It->bBroken || FVector::Dist(It->GetActorLocation(), At) > Radius + 45.f) continue;
        It->Break(At, Push);
        ++Count;
    }
    return Count;
}

int32 AVortexBreakable::BreakInArc(UWorld* World, const FVector& From, const FVector& Forward, float Reach, float MinDot, float PushSpeed)
{
    if (!World || World->GetNetMode() == NM_Client) return 0;
    const FVector Ahead = Forward.GetSafeNormal2D();
    int32 Count = 0;
    for (TActorIterator<AVortexBreakable> It(World); It; ++It)
    {
        if (It->bBroken) continue;
        const FVector To = It->GetActorLocation() - From;
        if (To.Size2D() > Reach + 45.f || FMath::Abs(To.Z) > 220.f) continue;
        if (To.Size2D() > 60.f && FVector::DotProduct(To.GetSafeNormal2D(), Ahead) < MinDot) continue;
        It->Break(It->GetActorLocation(), Ahead * PushSpeed);
        ++Count;
    }
    return Count;
}

void AVortexBreakable::Break(const FVector& At, const FVector& Push)
{
    if (bBroken || !HasAuthority()) return;
    bBroken = true;
    BreakAt = GetActorLocation();
    BreakPush = Push;
    Box->SetSimulatePhysics(false);
    Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ForceNetUpdate();
    OnRep_Broken();   // o anfitrião desenha também
    SetLifeSpan(PuddleLife + 8.f);
    // volta inteiro no mesmo lugar depois de um tempo (o mundo guarda o aviso, este ator some antes)
    const FName SameKind = Kind;
    const FTransform Where = Home;
    FTimerHandle Handle;
    TWeakObjectPtr<UWorld> WeakWorld(GetWorld());
    GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakWorld, SameKind, Where]()
    {
        UWorld* World = WeakWorld.Get();
        if (!World) return;
        AVortexBreakable* Fresh = World->SpawnActorDeferred<AVortexBreakable>(AVortexBreakable::StaticClass(), Where, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
        if (!Fresh) return;
        Fresh->Kind = SameKind;
        Fresh->FinishSpawning(Where);
    }), RespawnSeconds, false);
}

void AVortexBreakable::OnRep_Broken()
{
    if (!bBroken) return;
    Body->SetVisibility(false);
    Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_DedicatedServer) return;
    const FVector Center = BreakAt.IsZero() ? GetActorLocation() : FVector(BreakAt);
    const FVector Half = HalfSize();
    // as tábuas: aduelas do barril ou as tábuas do caixote, voando para fora e para cima
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UMaterialInterface* Wood = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Stadium/MI_Stadium_wood.MI_Stadium_wood"));
    const int32 Count = IsBarrel() ? 12 : 9;
    for (int32 i = 0; i < Count && Cube; ++i)
    {
        const float A = i * 2.f * PI / Count + FMath::FRandRange(-.2f, .2f);
        const FVector Out(FMath::Cos(A), FMath::Sin(A), 0.f);
        UStaticMeshComponent* Shard = NewObject<UStaticMeshComponent>(this);
        Shard->SetStaticMesh(Cube);
        if (Wood) Shard->SetMaterial(0, Wood);
        const FVector Size = IsBarrel() ? FVector(.06f, .2f, .9f) : FVector(.08f, .7f, .16f);
        Shard->SetWorldTransform(FTransform(FRotator(0.f, FMath::RadiansToDegrees(A), FMath::FRandRange(-10.f, 10.f)), Center + Out * Half.X * .8f, Size));
        Shard->SetCollisionProfileName(TEXT("PhysicsActor"));
        Shard->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        Shard->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        Shard->SetCastShadow(true);
        Shard->RegisterComponent();
        Shard->SetSimulatePhysics(true);
        Shard->SetMassOverrideInKg(NAME_None, 2.f, true);
        Shard->AddImpulse(Out * FMath::FRandRange(220.f, 420.f) + FVector(0.f, 0.f, FMath::FRandRange(250.f, 480.f)) + FVector(BreakPush) * .4f, NAME_None, true);
        Shard->AddAngularImpulseInDegrees(FVector(FMath::FRandRange(-600.f, 600.f), FMath::FRandRange(-600.f, 600.f), FMath::FRandRange(-300.f, 300.f)), NAME_None, true);
        Shards.Add(Shard);
    }
    VortexAudio::Play(this, TEXT("wfx_axe_hit"), Center, 1.f, .08f);
    if (!IsBarrel()) { VortexHitFx::Splash(World, Center, FLinearColor(.55f, .4f, .25f), 18); return; }
    // o líquido: espirra em cubinhos e escorre numa poça brilhante (ou, na água, só o anel)
    const FLinearColor Liquid = LiquidColor(Kind);
    VortexHitFx::Splash(World, Center, Liquid, 70);
    VortexAudio::Play(this, TEXT("water_splash"), Center, .9f, .08f);
    float Surface = 0.f;
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    if (Terrain && Terrain->IsWaterAt(Center.X, Center.Y, Surface) && Surface > Center.Z - Half.Z - 40.f)
    {
        VortexHitFx::Ripple(World, FVector(Center.X, Center.Y, Surface), 1.6f, 1.4f);
        return;
    }
    FHitResult Floor;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexPuddle), false, this);
    const bool bFloor = World->LineTraceSingleByChannel(Floor, Center + FVector(0.f, 0.f, 40.f), Center - FVector(0.f, 0.f, 400.f), ECC_Visibility, Query);
    FVector Ground = bFloor ? FVector(Floor.ImpactPoint) : Center - FVector(0.f, 0.f, Half.Z);
    // na vila as pedras da rua (sem colisão, 9 cm acima do terreno) cobriam a poça
    if (AVortexTown::Contains(Ground)) Ground.Z += 10.f;
    UStaticMesh* Disc = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UMaterialInterface* Wet = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_LiquidPuddle.M_LiquidPuddle"));
    if (!Disc || !Wet) return;
    // três manchas sobrepostas, de tamanhos e lugares um pouco diferentes: o contorno fica irregular
    PuddleOffsets.Reset();
    for (int32 i = 0; i < 3; ++i)
    {
        const float A = FMath::FRandRange(0.f, 2.f * PI);
        PuddleOffsets.Add(FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * FMath::FRandRange(10.f, 45.f) + FVector(0.f, 0.f, FMath::FRandRange(.8f, 1.6f)));
        UStaticMeshComponent* Puddle = NewObject<UStaticMeshComponent>(this);
        Puddle->SetStaticMesh(Disc);
        Puddle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Puddle->SetCastShadow(false);
        Puddle->SetUsingAbsoluteLocation(true);
        Puddle->SetUsingAbsoluteRotation(true);
        Puddle->SetUsingAbsoluteScale(true);
        Puddle->SetupAttachment(Box);
        Puddle->SetWorldLocation(Ground + PuddleOffsets.Last());
        Puddle->SetWorldScale3D(FVector(.01f, .01f, .004f));
        UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(Wet, this);
        Look->SetVectorParameterValue(TEXT("Color"), Kind == TEXT("wine") ? FLinearColor(.12f, .003f, .02f) : FLinearColor(.02f, .05f, .08f));
        Look->SetScalarParameterValue(TEXT("Opacity"), 0.f);
        Puddle->SetMaterial(0, Look);
        Puddle->RegisterComponent();
        Puddles.Add(Puddle);
        PuddleLooks.Add(Look);
    }
    PuddleSize = FMath::FRandRange(1.5f, 2.1f);
    PuddleAge = 0.f;
}

void AVortexBreakable::MulticastSplash_Implementation(FVector_NetQuantize At, float Strength)
{
    VortexHitFx::WaterSplash(GetWorld(), At, Strength);
}

void AVortexBreakable::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // a poça: escorre (cresce rápido e devagar), fica brilhando e seca (some e encolhe um pouco)
    if (PuddleAge >= 0.f)
    {
        PuddleAge += DeltaSeconds;
        const float Grow = 1.f - FMath::Pow(1.f - FMath::Clamp(PuddleAge / 2.2f, 0.f, 1.f), 3.f);
        const float Dry = FMath::Clamp((PuddleAge - (PuddleLife - 6.f)) / 6.f, 0.f, 1.f);
        for (int32 i = 0; i < Puddles.Num(); ++i)
        {
            if (!Puddles[i]) continue;
            const float S = PuddleSize * (i == 0 ? 1.f : .75f - .12f * i) * Grow * (1.f - .25f * Dry);
            Puddles[i]->SetWorldScale3D(FVector(FMath::Max(.01f, S), FMath::Max(.01f, S * (i == 1 ? .8f : 1.f)), .004f));
            if (PuddleLooks[i]) PuddleLooks[i]->SetScalarParameterValue(TEXT("Opacity"), .93f * FMath::Min(1.f, PuddleAge * 3.f) * (1.f - Dry));
        }
        if (Dry >= 1.f)
        {
            for (UStaticMeshComponent* Puddle : Puddles) if (Puddle) Puddle->DestroyComponent();
            Puddles.Reset();
            PuddleLooks.Reset();
            PuddleAge = -1.f;
        }
    }
    if (!HasAuthority() || bBroken) return;
    // o terreno só tem colisão perto dos jogadores (~2 pedaços de 18 m): longe de todos, a física desliga e o barril fica
    // parado onde está (antes ele caía através do chão quando o herói se afastava); se mesmo assim cair, volta ao lugar
    bool bSomeoneNear = false;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (const APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr; Pawn && FVector::Dist2D(Pawn->GetActorLocation(), GetActorLocation()) < 2500.f)
            bSomeoneNear = true;
    if (GetActorLocation().Z < Home.GetLocation().Z - 1500.f)
    {
        Box->SetSimulatePhysics(false);
        SetActorTransform(Home);
        return;
    }
    if (!bSomeoneNear)
    {
        if (Box->IsSimulatingPhysics() && Box->GetPhysicsLinearVelocity().Size() < 30.f) Box->SetSimulatePhysics(false);
        return;
    }
    if (!Box->IsSimulatingPhysics())
    {
        FHitResult Floor;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexBreakableFloor), false, this);
        const FVector From = GetActorLocation() + FVector(0.f, 0.f, 200.f);
        if (!GetWorld()->LineTraceSingleByChannel(Floor, From, From - FVector(0.f, 0.f, 800.f), ECC_WorldStatic, Query)) return;
        if (!bPlaced)
        {
            // a primeira vez: assenta no chão de verdade e guarda o lugar (para voltar inteiro depois de quebrar)
            SetActorLocation(FVector(Floor.ImpactPoint) + FVector(0.f, 0.f, HalfSize().Z + 2.f));
            Home = GetActorTransform();
            bPlaced = true;
        }
        Box->SetMassOverrideInKg(NAME_None, IsBarrel() ? 45.f : 18.f, true);
        Box->SetLinearDamping(.4f);
        Box->SetAngularDamping(.8f);
        Box->SetSimulatePhysics(true);
        return;
    }
    // na água: boia (empuxo pela parte submersa), balança e freia; ao cair, o mergulho para todos
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    float Surface = 0.f;
    const FVector At = GetActorLocation();
    const FVector Half = HalfSize();
    const bool bWater = Terrain && Terrain->IsWaterAt(At.X, At.Y, Surface) && At.Z - Half.Z < Surface;
    if (bWater && Box->IsSimulatingPhysics())
    {
        const float Under = FMath::Clamp((Surface - (At.Z - Half.Z)) / (2.f * Half.Z), 0.f, 1.f);
        const FVector Velocity = Box->GetPhysicsLinearVelocity();
        const FVector Lift(-Velocity.X * .8f, -Velocity.Y * .8f, 980.f * 1.7f * Under - Velocity.Z * 1.6f);
        Box->AddForce(Lift, NAME_None, true);
        Box->AddTorqueInDegrees(FVector(FMath::Sin(GetWorld()->GetTimeSeconds() * 1.3f) * 40.f, FMath::Cos(GetWorld()->GetTimeSeconds() * 1.1f) * 40.f, 0.f), NAME_None, true);
        if (!bInWater) MulticastSplash(FVector(At.X, At.Y, Surface), FMath::Clamp(-Velocity.Z / 600.f, .3f, 1.f));
    }
    bInWater = bWater;
}
