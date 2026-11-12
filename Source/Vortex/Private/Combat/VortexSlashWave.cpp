#include "Combat/VortexSlashWave.h"

#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexHitFx.h"
#include "Components/PointLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProceduralMeshComponent.h"

namespace
{
    // camadas: núcleo (fio branco, fino) e brilho (largo, suave), na frente; e o rastro: cópias
    // do arco que ficam para trás e se apagam. Back = cm atrás da frente.
    struct FBladeLook { float Back, Size, Strength, Glow, Sharp, Width; };
    const FBladeLook BladeLooks[] = {
        { 0.f, 1.f, 1.f, 7.f, 7.f, 1.f },        // núcleo
        { 6.f, 1.06f, 1.f, 2.2f, 2.5f, 1.9f },   // brilho em volta
        { 70.f, .97f, .55f, 2.6f, 4.f, 1.3f },   // rastro
        { 140.f, .93f, .32f, 2.f, 3.5f, 1.4f },
        { 210.f, .89f, .18f, 1.6f, 3.f, 1.5f },
        { 280.f, .85f, .08f, 1.2f, 2.5f, 1.6f },
    };
    constexpr float BurstSeconds = .28f;
    const FLinearColor LightColor(.35f, .7f, 1.f);

    /**
     * Fita em arco (lua de energia) em pé, de frente para +X: as pontas 72 graus para cada lado do
     * alto, curvadas para trás. V = 0 na borda da frente, 1 na de trás; U ao longo do arco.
     */
    void BuildArc(UProceduralMeshComponent* Mesh, float Radius, float Width, float Depth, float Back)
    {
        constexpr int32 Segments = 36, Rows = 4;
        const float Half = FMath::DegreesToRadians(72.f);
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        for (int32 i = 0; i <= Segments; ++i)
        {
            const float U = float(i) / Segments;
            const float Theta = FMath::Lerp(-Half, Half, U);
            // a largura cai até zero nas pontas
            const float Taper = FMath::Pow(FMath::Max(0.f, FMath::Cos(Theta / Half * HALF_PI)), .7f);   // (cos < 0 nas pontas daria NaN)
            const float Bend = -Depth * (1.f - FMath::Cos(Theta));
            for (int32 j = 0; j <= Rows; ++j)
            {
                const float V = float(j) / Rows;
                const float R = Radius - Width * Taper * V;
                const float X = Bend - Back * Taper * V * V;
                Vertices.Add(FVector(X, R * FMath::Sin(Theta), R * FMath::Cos(Theta) - Radius * .55f));
                Normals.Add(FVector::ForwardVector);
                UVs.Add(FVector2D(U, V));
            }
        }
        for (int32 i = 0; i < Segments; ++i)
        {
            for (int32 j = 0; j < Rows; ++j)
            {
                const int32 A = i * (Rows + 1) + j, B = A + Rows + 1;
                Triangles.Append({ A, B, A + 1, A + 1, B, B + 1 });
            }
        }
        Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, {}, {}, false);
    }
}

AVortexSlashWave::AVortexSlashWave()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicatingMovement(false);
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
    Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
    Light->SetMobility(EComponentMobility::Movable);   // luz criada em jogo: nada a "construir"
    Light->SetupAttachment(Root);
    Light->SetIntensityUnits(ELightUnits::Candelas);
    Light->SetIntensity(10.f);
    Light->SetAttenuationRadius(380.f);
    Light->SetLightColor(LightColor);
    Light->SetCastShadows(false);
}

void AVortexSlashWave::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AVortexSlashWave, BurstAt);
    DOREPLIFETIME(AVortexSlashWave, bBurst);
}

void AVortexSlashWave::BeginPlay()
{
    Super::BeginPlay();
    Start = GetActorLocation();
    Direction = GetActorForwardVector().GetSafeNormal2D();
    if (HasAuthority()) SetLifeSpan(Range / Speed + 2.f);
    if (GetNetMode() == NM_DedicatedServer) return;
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SlashArc.M_SlashArc"));
    if (Base)
    {
        // o arco em pé, inclinado na diagonal do golpe
        const FQuat Tilt(FVector::ForwardVector, FMath::DegreesToRadians(-20.f));
        for (int32 i = 0; i < UE_ARRAY_COUNT(BladeLooks); ++i)
        {
            const FBladeLook& Look = BladeLooks[i];
            UProceduralMeshComponent* Blade = NewObject<UProceduralMeshComponent>(this);
            Blade->SetupAttachment(Root);
            Blade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Blade->SetCastShadow(false);
            Blade->bAffectDistanceFieldLighting = false;
            Blade->bAffectDynamicIndirectLighting = false;
            Blade->SetVisibleInRayTracing(false);
            Blade->bVisibleInReflectionCaptures = false;
            Blade->bVisibleInRealTimeSkyCaptures = false;
            BuildArc(Blade, 150.f, 34.f * Look.Width, 55.f, 30.f * Look.Width);
            Blade->SetRelativeLocation(FVector(-Look.Back, 0.f, 0.f));
            Blade->SetRelativeRotation(Tilt);
            UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, this);
            Material->SetScalarParameterValue(TEXT("Seed"), FMath::FRand());
            Material->SetScalarParameterValue(TEXT("Glow"), Look.Glow);
            Material->SetScalarParameterValue(TEXT("Sharp"), Look.Sharp);
            Blade->SetMaterial(0, Material);
            Blade->RegisterComponent();
            Blades.Add(Blade);
            Materials.Add(Material);
        }
    }
    // faíscas que se soltam no caminho (pacote FreeParticle_SoftTofu das referências)
    if (UNiagaraSystem* Trail = VortexHitFx::System(TEXT("/Game/FreeParticle_SoftTofu/Niagara/NS_Sparkling_Glow.NS_Sparkling_Glow")))
    {
        Sparks = UNiagaraFunctionLibrary::SpawnSystemAttached(Trail, Root, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset, true);
    }
    if (USoundBase* Swing = LoadObject<USoundBase>(nullptr, TEXT("/Game/FreeModularMagicSFX/D1_Spell/Mono/SW_MagicWeapon_Swing_1.SW_MagicWeapon_Swing_1")))
    {
        UGameplayStatics::PlaySoundAtLocation(this, Swing, Start, .8f, FMath::FRandRange(.95f, 1.08f));
    }
    UpdateLook(0.f, 1.f);
}

void AVortexSlashWave::UpdateLook(float Fade, float Grow)
{
    for (int32 i = 0; i < Materials.Num(); ++i)
    {
        Materials[i]->SetScalarParameterValue(TEXT("Fade"), Fade * BladeLooks[i].Strength);
        Blades[i]->SetRelativeScale3D(FVector(1.f) * BladeLooks[i].Size * Grow);
    }
    Light->SetIntensity(14.f * Fade);
}

void AVortexSlashWave::StartBurst(const FVector& Where)
{
    if (bBurst) return;
    bBurst = true;
    BurstAt = Where;
    BurstAge = 0.f;
    SetActorLocation(Where);
    ForceNetUpdate();
}

void AVortexSlashWave::OnRep_Burst()
{
    if (!bBurst) return;
    BurstAge = 0.f;
    SetActorLocation(BurstAt);
}

void AVortexSlashWave::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bBurst)
    {
        // ao bater: a lua se abre e some num instante
        BurstAge += DeltaSeconds;
        const float T = FMath::Clamp(BurstAge / BurstSeconds, 0.f, 1.f);
        // ao bater: o arco se abre num clarão e some
        UpdateLook((1.f - T * T) * (1.f + 1.5f * (1.f - T)), 1.f + .9f * T);
        if (Sparks) Sparks->Deactivate();
        if (T >= 1.f)
        {
            if (HasAuthority()) Destroy();
            else SetActorHiddenInGame(true);
        }
        return;
    }
    Age += DeltaSeconds;
    const float Before = Travelled;
    Travelled = FMath::Min(Range, Speed * Age);
    const FVector From = Start + Direction * Before, To = Start + Direction * Travelled;
    // aparece rápido e perde força perto do fim do alcance
    UpdateLook(FMath::Min(1.f, Age / .06f) * (1.f - .6f * FMath::Square(Travelled / Range)), 1.f + .15f * (Travelled / Range));
    if (!HasAuthority())
    {
        SetActorLocation(To);
        return;
    }
    // terreno e paredes param o corte
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexSlashWave), false, this);
    if (GetOwner()) Query.AddIgnoredActor(GetOwner());
    FCollisionObjectQueryParams Solid;
    Solid.AddObjectTypesToQuery(ECC_WorldStatic);
    // o corte atravessa a vegetação (árvores, arbustos, pedrinhas: cópias instanciadas); o chão
    // e as construções o param
    TArray<FHitResult> Touches;
    GetWorld()->SweepMultiByObjectType(Touches, From, To, FQuat::Identity, Solid, FCollisionShape::MakeSphere(20.f), Query);
    const FHitResult* Wall = Touches.FindByPredicate([](const FHitResult& Hit)
    {
        return Hit.GetComponent() && !Hit.GetComponent()->IsA<UInstancedStaticMeshComponent>();
    });
    const bool bWall = Wall != nullptr;
    const FVector End = bWall ? Wall->Location : To;
    SetActorLocation(End);
    // inimigos no caminho deste quadro: uma caixa do tamanho do corte, do ponto anterior até agora
    const FVector Segment = End - From;
    const float Length = Segment.Size();
    if (Length > KINDA_SMALL_NUMBER)
    {
        TArray<FOverlapResult> Overlaps;
        FCollisionObjectQueryParams Bodies;
        Bodies.AddObjectTypesToQuery(ECC_Pawn);
        const FCollisionShape Box = FCollisionShape::MakeBox(FVector(Length * .5f + 40.f, HalfWidth, HalfHeight));
        GetWorld()->OverlapMultiByObjectType(Overlaps, From + Segment * .5f, Direction.Rotation().Quaternion(), Bodies, Box, Query);
        UVortexCombatComponent* Combat = GetOwner() ? GetOwner()->FindComponentByClass<UVortexCombatComponent>() : nullptr;
        for (const FOverlapResult& Overlap : Overlaps)
        {
            AActor* Target = Overlap.GetActor();
            if (!Target || Hits.Contains(Target) || !Combat || !Combat->CanHarm(Target)) continue;
            Hits.Add(Target);
            Combat->ApplyHit(Target, 1.f, EVortexDamageKind::Melee, 0.f, Damage);   // 07/11: o golpe da arma multiplica
            UE_LOG(LogTemp, Display, TEXT("VORTEX_SLASH_HIT %s dano=%.0f distancia=%.0f"), *GetNameSafe(Target), Damage, Travelled);
        }
    }
    if (bWall) StartBurst(End);
    else if (Travelled >= Range) StartBurst(End);
}
