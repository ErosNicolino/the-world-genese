#include "World/VortexPortal.h"
#include "World/VortexWorldHooks.h"

#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr float PortalHeight = 520.f, PortalWidth = 320.f, PortalCenterZ = 290.f;
}

AVortexPortal::AVortexPortal()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto Plane = [&](const TCHAR* Name)
    {
        UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetStaticMesh(PlaneMesh.Object);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
        return Mesh;
    };
    // o rasgo em pé (o plano gira para ficar de frente para X); a camada de trás gira ao contrário
    Swirl = Plane(TEXT("Swirl"));
    Swirl->SetRelativeLocationAndRotation(FVector(0.f, 0.f, PortalCenterZ), FRotator(90.f, 0.f, 0.f));
    Swirl->SetRelativeScale3D(FVector(PortalHeight / 100.f, PortalWidth / 100.f, 1.f));
    SwirlBack = Plane(TEXT("SwirlBack"));
    SwirlBack->SetRelativeLocationAndRotation(FVector(-6.f, 0.f, PortalCenterZ), FRotator(90.f, 0.f, 0.f));
    SwirlBack->SetRelativeScale3D(FVector(PortalHeight / 100.f * 1.08f, PortalWidth / 100.f * 1.08f, 1.f));
    // o círculo no chão
    Floor = Plane(TEXT("Floor"));
    Floor->SetRelativeLocation(FVector(0.f, 0.f, 4.f));
    Floor->SetRelativeScale3D(FVector(7.f, 7.f, 1.f));
    auto Blocks = [&](const TCHAR* Name)
    {
        UInstancedStaticMeshComponent* Mesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetStaticMesh(CubeMesh.Object);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
        return Mesh;
    };
    Rim = Blocks(TEXT("Rim"));
    Sparks = Blocks(TEXT("Sparks"));
    Rocks = Blocks(TEXT("Rocks"));
    Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
    Light->SetupAttachment(RootComponent);
    Light->SetRelativeLocation(FVector(120.f, 0.f, PortalCenterZ));
    Light->SetAttenuationRadius(1800.f);
    Light->SetIntensity(9000.f);
    Light->SetCastShadows(false);
    Hum = CreateDefaultSubobject<UAudioComponent>(TEXT("Hum"));
    Hum->SetupAttachment(RootComponent);
    Hum->SetRelativeLocation(FVector(0.f, 0.f, PortalCenterZ));
    Hum->bAutoActivate = false;
    Hum->bOverrideAttenuation = true;
    Hum->AttenuationOverrides.bAttenuate = true;
    Hum->AttenuationOverrides.bSpatialize = true;
    Hum->AttenuationOverrides.AttenuationShapeExtents = FVector(300.f);
    Hum->AttenuationOverrides.FalloffDistance = 2600.f;
}

void AVortexPortal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AVortexPortal, Rank);
    DOREPLIFETIME(AVortexPortal, DungeonId);
    DOREPLIFETIME(AVortexPortal, Destination);
}

FLinearColor AVortexPortal::RankColor(const FString& InRank)
{
    // "quanto mais escuro, mais difícil; vermelho o mais difícil": do azul-claro (E) ao vermelho-sangue (S),
    // cada ranque mais escuro e mais fechado que o anterior
    if (InRank == TEXT("D")) return FLinearColor(.12f, .38f, 1.f);    // azul
    if (InRank == TEXT("C")) return FLinearColor(.18f, .12f, .85f);   // azul-escuro, quase índigo
    if (InRank == TEXT("B")) return FLinearColor(.42f, .06f, .7f);    // roxo
    if (InRank == TEXT("A")) return FLinearColor(.55f, .02f, .32f);   // vinho
    if (InRank == TEXT("S")) return FLinearColor(.75f, .0f, .02f);    // vermelho-sangue
    return FLinearColor(.3f, .68f, 1.f);                             // E: azul-claro
}

float AVortexPortal::RankGlow(const FString& InRank)
{
    // o brilho também cai com o ranque: o portal difícil é mais escuro
    return InRank == TEXT("D") ? .85f : InRank == TEXT("C") ? .7f : InRank == TEXT("B") ? .6f : InRank == TEXT("A") ? .5f : InRank == TEXT("S") ? .45f : 1.f;
}

float AVortexPortal::RankSize(const FString& InRank)
{
    // "o portal crescer com a dificuldade, o vermelho bem maior que o azul"
    return InRank == TEXT("D") ? 1.15f : InRank == TEXT("C") ? 1.3f : InRank == TEXT("B") ? 1.5f : InRank == TEXT("A") ? 1.75f : InRank == TEXT("S") ? 2.2f : 1.f;
}

AVortexPortal* AVortexPortal::FindNear(const UWorld* World, const FVector& At, float Range)
{
    if (!World) return nullptr;
    AVortexPortal* Best = nullptr;
    float BestDistance = Range;
    for (TActorIterator<AVortexPortal> It(const_cast<UWorld*>(World)); It; ++It)
    {
        // o portal grande alcança mais longe
        const float D = FVector::Dist2D(It->GetActorLocation(), At) / RankSize(It->Rank);
        if (D < BestDistance && FMath::Abs(It->GetActorLocation().Z - At.Z) < 400.f) { Best = *It; BestDistance = D; }
    }
    return Best;
}

AVortexPortal* AVortexPortal::Open(UWorld* World, const FVector& Ground, float Yaw, const FString& InRank, FName InDungeon)
{
    if (!World) return nullptr;
    const FTransform Pose(FRotator(0.f, Yaw, 0.f), Ground);
    AVortexPortal* Portal = World->SpawnActorDeferred<AVortexPortal>(AVortexPortal::StaticClass(), Pose, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Portal) return nullptr;
    Portal->Rank = InRank.ToUpper();
    Portal->DungeonId = InDungeon;
    Portal->FinishSpawning(Pose);
    return Portal;
}

void AVortexPortal::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer) { SetActorTickEnabled(false); return; }
    UMaterialInterface* SwirlMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/World/Portal/M_PortalSwirl.M_PortalSwirl"));
    UMaterialInterface* GlowMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/World/Portal/M_PortalGlow.M_PortalGlow"));
    UMaterialInterface* StoneMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    auto Look = [this](UMeshComponent* Mesh, UMaterialInterface* Base)
    {
        if (!Mesh || !Base) return (UMaterialInstanceDynamic*)nullptr;
        UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
        Mesh->SetMaterial(0, MID);
        Looks.Add(MID);
        return MID;
    };
    Look(Swirl, SwirlMat);
    if (UMaterialInstanceDynamic* Back = Look(SwirlBack, SwirlMat))
    {
        Back->SetScalarParameterValue(TEXT("Speed"), -.9f);
        Back->SetScalarParameterValue(TEXT("Arms"), 3.f);
        Back->SetScalarParameterValue(TEXT("Pixels"), 30.f);
        Back->SetScalarParameterValue(TEXT("Intensity"), 3.5f);
    }
    if (UMaterialInstanceDynamic* Ground = Look(Floor, SwirlMat))
    {
        Ground->SetScalarParameterValue(TEXT("Speed"), .35f);
        Ground->SetScalarParameterValue(TEXT("Arms"), 8.f);
        Ground->SetScalarParameterValue(TEXT("Twist"), 2.f);
        Ground->SetScalarParameterValue(TEXT("Intensity"), 1.6f);
        Ground->SetScalarParameterValue(TEXT("Pixels"), 40.f);
    }
    Look(Rim, GlowMat);
    Look(Sparks, GlowMat);
    if (UMaterialInstanceDynamic* Stone = Look(Rocks, StoneMat)) Stone->SetVectorParameterValue(TEXT("Color"), FLinearColor(.06f, .06f, .08f));
    ApplyRank();
    PlaceInstances(true);
    if (USoundBase* Wave = LoadObject<USoundBase>(nullptr, TEXT("/Game/Vortex/Audio/Sfx/portal_hum.portal_hum")))
    {
        Hum->SetSound(Wave);
        Hum->SetVolumeMultiplier(.8f);
        Hum->Play();
    }
    VortexWorldHooks::PlaySound(this, TEXT("portal_open"), GetActorLocation() + FVector(0.f, 0.f, PortalCenterZ), 1.f);
}

void AVortexPortal::OnRep_Rank() { ApplyRank(); }

void AVortexPortal::ApplyRank()
{
    const FLinearColor Color = RankColor(Rank);
    const float Glow = RankGlow(Rank);
    for (UMaterialInstanceDynamic* MID : Looks)
    {
        if (!MID || (MID->Parent && MID->Parent->GetName() == TEXT("BasicShapeMaterial"))) continue;
        MID->SetVectorParameterValue(TEXT("Color"), Color);
    }
    // brilho forte ("o portal gostei forte assim"), que cai com o ranque: frente, fundo, chão e blocos
    if (Looks.IsValidIndex(0) && Looks[0]) Looks[0]->SetScalarParameterValue(TEXT("Intensity"), 6.f * Glow);
    if (Looks.IsValidIndex(1) && Looks[1]) Looks[1]->SetScalarParameterValue(TEXT("Intensity"), 3.5f * Glow);
    if (Looks.IsValidIndex(2) && Looks[2]) Looks[2]->SetScalarParameterValue(TEXT("Intensity"), 1.6f * Glow);
    for (int32 i = 3; i < 5; ++i) if (Looks.IsValidIndex(i) && Looks[i]) Looks[i]->SetScalarParameterValue(TEXT("Intensity"), 8.f * Glow);
    if (Light) { Light->SetLightColor(Color); LightScale = Glow; Light->SetAttenuationRadius(1800.f * RankSize(Rank)); }
    SetActorScale3D(FVector(RankSize(Rank)));
    if (Hum) Hum->AttenuationOverrides.FalloffDistance = 2600.f * RankSize(Rank);
}

void AVortexPortal::PlaceInstances(bool bFirst)
{
    const float T = Age;
    // a borda: blocos acesos em volta do oval, tremendo
    constexpr int32 RimCount = 56;
    if (bFirst) for (int32 i = 0; i < RimCount; ++i) Rim->AddInstance(FTransform::Identity);
    for (int32 i = 0; i < RimCount; ++i)
    {
        const float A = i * UE_TWO_PI / RimCount;
        const float Jitter = FMath::Sin(T * 7.f + i * 1.7f) * .5f + .5f;
        const FVector At(0.f, FMath::Cos(A) * (PortalWidth * .5f + 8.f), PortalCenterZ + FMath::Sin(A) * (PortalHeight * .5f + 8.f));
        const float S = .18f + .1f * Jitter;
        Rim->UpdateInstanceTransform(i, FTransform(FRotator(0.f, 0.f, FMath::RadiansToDegrees(A) + T * 20.f), At, FVector(S * .6f, S, S)), false, false);
    }
    // faíscas em blocos puxadas em espiral para o centro
    constexpr int32 SparkCount = 80;
    if (bFirst) for (int32 i = 0; i < SparkCount; ++i) Sparks->AddInstance(FTransform::Identity);
    for (int32 i = 0; i < SparkCount; ++i)
    {
        const float Phase = FMath::Frac(i * .6180339f);
        const float Life = FMath::Frac(T * (.22f + .1f * FMath::Frac(i * .37f)) + Phase);
        const float Radius = (1.f - Life) * 4.2f;
        const float A = Phase * UE_TWO_PI + Life * 5.f;
        const FVector At(40.f + Radius * 18.f, FMath::Cos(A) * Radius * (PortalWidth * .21f), PortalCenterZ + FMath::Sin(A) * Radius * (PortalHeight * .2f));
        const float S = (.05f + .08f * FMath::Frac(i * .913f)) * FMath::Min(1.f, Radius);
        Sparks->UpdateInstanceTransform(i, FTransform(FRotator(T * 90.f + i, T * 60.f, 0.f), At, FVector(S)), false, false);
    }
    // pedras do chão flutuando em volta
    constexpr int32 RockCount = 12;
    if (bFirst) for (int32 i = 0; i < RockCount; ++i) Rocks->AddInstance(FTransform::Identity);
    for (int32 i = 0; i < RockCount; ++i)
    {
        const float A = i * UE_TWO_PI / RockCount + T * .15f;
        const float D = 260.f + 60.f * FMath::Frac(i * .71f);
        const float S = .25f + .3f * FMath::Frac(i * .53f);
        const FVector At(FMath::Cos(A) * D, FMath::Sin(A) * D, 40.f + 70.f * FMath::Frac(i * .29f) + 25.f * FMath::Sin(T * 1.3f + i));
        Rocks->UpdateInstanceTransform(i, FTransform(FRotator(T * 15.f + i * 30.f, T * 25.f, i * 10.f), At, FVector(S)), false, false);
    }
    Rim->MarkRenderStateDirty();
    Sparks->MarkRenderStateDirty();
    Rocks->MarkRenderStateDirty();
}

void AVortexPortal::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    // abre rasgando: cresce do nada, passa um pouco do tamanho e assenta
    const float Grow = Age < 1.4f ? 1.f - FMath::Exp(-4.f * Age) * FMath::Cos(Age * 7.f) : 1.f;
    const float Breath = 1.f + .02f * FMath::Sin(Age * 2.1f);
    Swirl->SetRelativeScale3D(FVector(PortalHeight / 100.f * Grow * Breath, PortalWidth / 100.f * Grow, 1.f));
    SwirlBack->SetRelativeScale3D(FVector(PortalHeight / 100.f * 1.08f * Grow, PortalWidth / 100.f * 1.08f * Grow * Breath, 1.f));
    Rim->SetRelativeScale3D(FVector(1.f, Grow, Grow));
    Rim->SetRelativeLocation(FVector(0.f, 0.f, PortalCenterZ * (1.f - Grow)));
    // 30/10: ~40% mais fraco (o brilho estourava em volta à noite)
    Light->SetIntensity((4200.f + 1800.f * FMath::Sin(Age * 5.3f) * FMath::Sin(Age * 1.7f)) * Grow * LightScale);
    // os blocos (borda, faíscas, pedras) custam: perto, a cada quadro; longe, 10 vezes por segundo; muito longe, parados
    float Distance = 0.f;
    if (const APlayerController* Local = GetWorld()->GetFirstPlayerController(); Local && Local->PlayerCameraManager)
        Distance = FVector::Dist(Local->PlayerCameraManager->GetCameraLocation(), GetActorLocation());
    InstanceClock += DeltaSeconds;
    const float Every = Distance < 4000.f ? 0.f : Distance < 15000.f ? .1f : -1.f;
    if (Every >= 0.f && InstanceClock >= Every) { InstanceClock = 0.f; PlaceInstances(false); }
}
