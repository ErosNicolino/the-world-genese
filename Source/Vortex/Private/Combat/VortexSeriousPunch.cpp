#include "Combat/VortexSeriousPunch.h"

#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexHitFx.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
    const TCHAR* GlowPath = TEXT("/Game/Vortex/Combat/M_SpectralGlow.M_SpectralGlow");
    const FLinearColor PunchColor(1.f, .72f, .25f);
}

AVortexSeriousPunch::AVortexSeriousPunch()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);
    SetNetCullDistanceSquared(FMath::Square(15000.f));
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Fist = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fist"));
    Fist->SetupAttachment(Root);
    Fist->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Fist->SetCastShadow(false);
    Wind = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wind"));
    Wind->SetupAttachment(Root);
    Wind->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Wind->SetCastShadow(false);
    if (UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"))) Wind->SetStaticMesh(Cone);
    // o cone de vento aponta para trás do punho (rastro)
    Wind->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
}

void AVortexSeriousPunch::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AVortexSeriousPunch, Damage, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexSeriousPunch, Range, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexSeriousPunch, Speed, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexSeriousPunch, Width, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexSeriousPunch, FistMesh, COND_InitialOnly);
}

void AVortexSeriousPunch::BeginPlay()
{
    Super::BeginPlay();
    Start = GetActorLocation();
    Direction = GetActorForwardVector().GetSafeNormal2D();
    if (Direction.IsNearlyZero()) Direction = FVector::ForwardVector;
    SetLifeSpan(Range / FMath::Max(Speed, 1.f) + 1.2f);
    if (GetNetMode() == NM_DedicatedServer) return;

    UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, GlowPath);
    if (FistMesh) Fist->SetStaticMesh(FistMesh);
    // a manopla do Godot aponta os dedos para +Y no modelo: o punho gira para avançar de frente
    Fist->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    Fist->SetRelativeScale3D(FVector(2.f));
    if (Glow)
    {
        FistGlow = UMaterialInstanceDynamic::Create(Glow, this);
        FistGlow->SetVectorParameterValue(TEXT("Color"), PunchColor);
        FistGlow->SetScalarParameterValue(TEXT("Opacity"), .4f);
        for (int32 i = 0; i < Fist->GetNumMaterials(); ++i) Fist->SetMaterial(i, FistGlow);
        WindGlow = UMaterialInstanceDynamic::Create(Glow, this);
        WindGlow->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, .9f, .7f));
        WindGlow->SetScalarParameterValue(TEXT("Opacity"), .35f);
        Wind->SetMaterial(0, WindGlow);
    }
    Wind->SetRelativeScale3D(FVector(.1f));

    // a largada: onda de choque no chão, quadro de impacto, estalo e tranco na câmera de quem está perto
    const FVector Feet = Start - FVector(0.f, 0.f, 80.f);
    if (UNiagaraSystem* Shock = VortexHitFx::System(TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/VFX_Shockwave_01_White_Big_1s.VFX_Shockwave_01_White_Big_1s")))
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Shock, Feet, FRotator::ZeroRotator, FVector(1.f));
    if (UNiagaraSystem* Frame = VortexHitFx::System(TEXT("/Game/Vefects/Easy_Impact_Frames/VFX/Frames/Particles/NS_Impact_Frame_Advanced_03_Always.NS_Impact_Frame_Advanced_03_Always")))
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Frame, Start + Direction * 120.f, Direction.Rotation());
    VortexAudio::Play(this, TEXT("boss_punch"), Start, 1.f, .02f);
    VortexAudio::Play(this, TEXT("boss_seismic"), Start, .8f, .02f);
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AVortexCharacter* Hero = It->Get() ? Cast<AVortexCharacter>(It->Get()->GetPawn()) : nullptr;
        if (Hero && Hero->IsLocallyControlled() && FVector::Dist(Hero->GetActorLocation(), Start) < 2500.f)
            Hero->KickCamera(Hero == GetInstigator() ? 3.f : 1.5f);
    }
    VortexHitFx::Splash(GetWorld(), Feet + Direction * 150.f, FLinearColor(.55f, .45f, .32f), 18);
}

void AVortexSeriousPunch::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    const float Before = Travelled;
    Travelled = FMath::Min(Range, Travelled + Speed * DeltaSeconds);
    const FVector Here = Start + Direction * Travelled;
    SetActorLocation(Here);
    if (HasAuthority()) StrikeAlong(Before, Travelled);

    if (GetNetMode() == NM_DedicatedServer) return;
    // o punho cresce na saída (2x -> 9x) e some no fim do alcance
    const float Grow = FMath::Clamp(Age / .14f, 0.f, 1.f);
    const float Left = 1.f - FMath::Clamp((Travelled - Range * .82f) / (Range * .18f), 0.f, 1.f);
    Fist->SetRelativeScale3D(FVector(FMath::Lerp(2.f, 9.f, Grow) * (.6f + .4f * Left)));
    // rastro de vento: um cone comprido atrás, mais longo quanto mais viajou
    const float Trail = FMath::Min(Travelled, 900.f);
    Wind->SetRelativeLocation(FVector(-Trail * .5f - 60.f, 0.f, 0.f));
    Wind->SetRelativeScale3D(FVector(Width / 50.f * .9f * Grow, Width / 50.f * .9f * Grow, Trail / 100.f));
    // brilho contido: com mais, o punho estourava numa bola branca e perdia a forma da manopla
    if (FistGlow) FistGlow->SetScalarParameterValue(TEXT("Opacity"), .4f * Left);
    if (WindGlow) WindGlow->SetScalarParameterValue(TEXT("Opacity"), .35f * Left);
    // anéis de ar a cada 3 m
    while (NextRing < Travelled)
    {
        SpawnRing(Start + Direction * NextRing);
        NextRing += 520.f;
    }
    if (!bLanded && Travelled >= Range)
    {
        bLanded = true;
        if (UNiagaraSystem* Boom = VortexHitFx::System(TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Explosion.NS_Free_Spells_Explosion")))
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Boom, Here, FRotator::ZeroRotator, FVector(.8f));
        VortexAudio::Play(this, TEXT("boss_stomp"), Here, .7f);
        Fist->SetVisibility(false);
        Wind->SetVisibility(false);
    }
}

void AVortexSeriousPunch::SpawnRing(const FVector& At) const
{
    if (UNiagaraSystem* Ring = VortexHitFx::System(TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Vertical/VFX_Shockwave_Vertical_01_White_Small_1s.VFX_Shockwave_Vertical_01_White_Small_1s")))
    {
        // anel de ar em pé, virado para a direção do soco
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Ring, At, Direction.Rotation(), FVector(1.f));
    }
}

void AVortexSeriousPunch::StrikeAlong(float From, float To)
{
    AVortexCharacter* Hero = Cast<AVortexCharacter>(GetInstigator());
    UVortexCombatComponent* Combat = Hero ? Hero->GetCombat() : nullptr;
    if (!Combat) return;
    // todos no corredor entre onde o punho estava e onde está (uma vez cada)
    const FVector A = Start + Direction * From, B = Start + Direction * To;
    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexSeriousPunch), false, Hero);
    const FVector Mid = (A + B) * .5f;
    GetWorld()->OverlapMultiByObjectType(Overlaps, Mid, FQuat::Identity, Objects, FCollisionShape::MakeSphere((B - A).Size() * .5f + Width + 300.f), Query);
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Target = Overlap.GetActor();
        if (!Target || Struck.Contains(Target) || !Combat->CanHarm(Target)) continue;
        const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Target->GetRootComponent());
        const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 40.f;
        const FVector P = Target->GetActorLocation();
        const FVector Closest = FMath::ClosestPointOnSegment(P, A, B);
        if (FVector::Dist2D(P, Closest) > Width + Radius || FMath::Abs(P.Z - Closest.Z) > 350.f) continue;
        Struck.Add(Target);
        Combat->ApplyHit(Target, 1.f, EVortexDamageKind::Melee, 1.f, Damage);   // 07/11: o golpe da arma multiplica
        // arremesso: para a frente e para cima (chefes ficam)
        const UVortexCombatComponent* Other = Target->FindComponentByClass<UVortexCombatComponent>();
        if (ACharacter* Body = Cast<ACharacter>(Target); Body && !(Other && Other->bStunImmune))
            Body->LaunchCharacter(Direction * 1400.f + FVector(0.f, 0.f, 650.f), true, true);
    }
}
