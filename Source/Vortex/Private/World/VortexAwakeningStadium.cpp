#include "World/VortexAwakeningStadium.h"

#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    // medidas do AwakeningStadium.cs do Godot (metros)
    constexpr float ArenaRadius = 17.f;
    // os pedestais das armas: anel dentro da arena, 10 graus entre um e outro (~2,3 m)
    constexpr float PedestalRing = 13.3f, PedestalStep = 10.f;
    constexpr float PodiumHeight = 3.2f, StandsInner = 18.f, Tier = 1.3f, TierRise = .95f, FacadeRadius = 28.5f;
    constexpr int32 Tiers = 7;
    constexpr float GateHalf = 2.6f, ExitAngle = 135.f;
    // só a saída abre o anel (as alas viraram grupos de pedestais dentro da arena, sem corredor)
    const float GateAngles[] = { ExitAngle };

    struct FWing { const TCHAR* Id; float Angle; FColor Color; const TCHAR* Name; };
    const FWing Wings[] = {
        { TEXT("marcial"), 0.f, FColor(0xa8, 0x28, 0x1f), TEXT("ALA MARCIAL") },
        { TEXT("distancia"), 90.f, FColor(0x2f, 0x7a, 0x34), TEXT("ALA DE DISTÂNCIA") },
        { TEXT("exotica"), 180.f, FColor(0x5e, 0x2f, 0x80), TEXT("ALA EXÓTICA") },
        { TEXT("arcana"), 270.f, FColor(0x27, 0x50, 0x9c), TEXT("ALA ARCANA") },
    };

    FLinearColor Lighten(const FLinearColor& C, float T) { return FMath::Lerp(C, FLinearColor::White, T); }
}

// ---- boneco de treino --------------------------------------------------------------------------

AVortexTrainingDummy::AVortexTrainingDummy()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    Body = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Body"));
    Body->InitCapsuleSize(40.f, 95.f);
    Body->SetCollisionProfileName(TEXT("Pawn"));
    RootComponent = Body;
    AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    AbilitySystem->SetIsReplicated(true);
    Vitals = CreateDefaultSubobject<UVortexAttributes>(TEXT("Vitals"));
    Combat = CreateDefaultSubobject<UVortexCombatComponent>(TEXT("Combat"));
    Combat->Faction = EVortexFaction::Hostile;
    UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto Part = [&](const TCHAR* Name, UStaticMesh* Mesh, const FVector& At, const FVector& Scale) -> UStaticMeshComponent*
    {
        UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        C->SetupAttachment(Body);
        C->SetStaticMesh(Mesh);
        C->SetRelativeLocation(At);
        C->SetRelativeScale3D(Scale);
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        return C;
    };
    // poste de madeira, saco de palha, braços em cruz e a cabeça de palha
    Post = Part(TEXT("Post"), CylinderMesh, FVector(0.f, 0.f, -10.f), FVector(.16f, .16f, 1.9f));
    Sack = Part(TEXT("Sack"), CylinderMesh, FVector(0.f, 0.f, 20.f), FVector(.62f, .62f, .9f));
    Arms = Part(TEXT("Arms"), CubeMesh, FVector(0.f, 0.f, 45.f), FVector(.14f, 1.3f, .14f));
    Head = Part(TEXT("Head"), CubeMesh, FVector(0.f, 0.f, 82.f), FVector(.42f, .42f, .42f));
}

void AVortexTrainingDummy::BeginPlay()
{
    Super::BeginPlay();
    AbilitySystem->InitAbilityActorInfo(this, this);
    if (HasAuthority())
    {
        AbilitySystem->SetNumericAttributeBase(UVortexAttributes::GetMaxHealthAttribute(), 999.f);
        AbilitySystem->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), 999.f);
    }
    LastHealth = 999.f;
    UMaterialInterface* Wood = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Stadium/MI_Stadium_wood.MI_Stadium_wood"));
    UMaterialInterface* Straw = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Stadium/MI_Stadium_cloth.MI_Stadium_cloth"));
    if (Wood) { Post->SetMaterial(0, Wood); Arms->SetMaterial(0, Wood); }
    if (Straw)
    {
        UMaterialInstanceDynamic* Hay = UMaterialInstanceDynamic::Create(Straw, this);
        Hay->SetVectorParameterValue(TEXT("Tint"), FLinearColor(.85f, .68f, .32f));
        Sack->SetMaterial(0, Hay);
        Head->SetMaterial(0, Hay);
    }
}

void AVortexTrainingDummy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Health = Vitals->GetHealth();
    if (Health < LastHealth - .1f) { SinceHit = 0.f; Wobble = 1.f; }
    LastHealth = Health;
    SinceHit += DeltaSeconds;
    // balança ao apanhar
    Wobble = FMath::Max(0.f, Wobble - DeltaSeconds * 2.5f);
    const float Swing = Wobble * 12.f * FMath::Sin(SinceHit * 22.f);
    Sack->SetRelativeRotation(FRotator(Swing, 0.f, 0.f));
    Head->SetRelativeRotation(FRotator(Swing * 1.3f, 0.f, 0.f));
    // aguenta qualquer golpe: a vida volta cheia
    if (HasAuthority() && SinceHit > 1.5f && Health < 999.f)
    {
        AbilitySystem->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), 999.f);
        LastHealth = 999.f;
    }
}

// ---- estádio -------------------------------------------------------------------------------------

AVortexAwakeningStadium::AVortexAwakeningStadium()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;   // cada máquina monta a sua cópia
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    // o estádio não se move: com a raiz estática as peças (estáticas) se prendem a ela; antes a Unreal
    // recusava as 25 e avisava no log a cada partida
    Root->SetMobility(EComponentMobility::Static);
    RootComponent = Root;
}

AVortexAwakeningStadium* AVortexAwakeningStadium::Find(const UWorld* World)
{
    if (!World) return nullptr;
    if (TActorIterator<AVortexAwakeningStadium> It(World); It) return *It;
    return nullptr;
}

AVortexAwakeningStadium* AVortexAwakeningStadium::Ensure(UWorld* World)
{
    if (AVortexAwakeningStadium* Found = Find(World)) return Found;
    if (!World) return nullptr;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    return World->SpawnActor<AVortexAwakeningStadium>(Origin(), FRotator::ZeroRotator, Params);
}

float AVortexAwakeningStadium::GateDistance(float Degrees, float Radius)
{
    float Best = TNumericLimits<float>::Max();
    for (const float Gate : GateAngles)
    {
        const float Diff = FMath::FindDeltaAngleDegrees(Gate, Degrees);
        if (FMath::Abs(Diff) > 90.f) continue;
        Best = FMath::Min(Best, FMath::Abs(FMath::Sin(FMath::DegreesToRadians(Diff))) * Radius);
    }
    return Best;
}

UMaterialInterface* AVortexAwakeningStadium::Material(FName Name)
{
    if (TObjectPtr<UMaterialInterface>* Found = Materials.Find(Name)) return *Found;
    const FString Id = Name.ToString();
    UMaterialInterface* Loaded = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Stadium/MI_Stadium_%s.MI_Stadium_%s"), *Id, *Id));
    Materials.Add(Name, Loaded);
    return Loaded;
}

UMaterialInterface* AVortexAwakeningStadium::Cloth(const FLinearColor& Color)
{
    const FName Key(*FString::Printf(TEXT("cloth_%s"), *Color.ToFColor(true).ToHex()));
    if (TObjectPtr<UMaterialInterface>* Found = Materials.Find(Key)) return *Found;
    UMaterialInstanceDynamic* Made = Material(TEXT("cloth")) ? UMaterialInstanceDynamic::Create(Material(TEXT("cloth")), this) : nullptr;
    if (Made) Made->SetVectorParameterValue(TEXT("Tint"), Color * 1.6f);
    Materials.Add(Key, Made);
    return Made;
}

UInstancedStaticMeshComponent* AVortexAwakeningStadium::Batch(FName Shape, UMaterialInterface* M, bool bSolid)
{
    const FName Key(*FString::Printf(TEXT("%s_%s_%d"), *Shape.ToString(), *GetNameSafe(M), bSolid ? 1 : 0));
    if (TObjectPtr<UInstancedStaticMeshComponent>* Found = Batches.Find(Key)) return *Found;
    UInstancedStaticMeshComponent* Ism = NewObject<UInstancedStaticMeshComponent>(this);
    Ism->SetupAttachment(Root);
    Ism->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), *Shape.ToString(), *Shape.ToString())));
    Ism->SetMaterial(0, M);
    Ism->SetCollisionEnabled(bSolid ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    if (bSolid) Ism->SetCollisionProfileName(TEXT("BlockAll"));
    Ism->SetMobility(EComponentMobility::Static);
    Ism->RegisterComponent();
    Batches.Add(Key, Ism);
    return Ism;
}

void AVortexAwakeningStadium::Box(UMaterialInterface* Mat, const FVector& Center, const FVector& Size, float FacingDeg, bool bSolid)
{
    // eixo X da caixa = para onde ela "olha" (o raio); Y = tangente; Z = altura
    const FVector Facing = Dir(FacingDeg);
    const FRotator Rotation(0.f, FMath::RadiansToDegrees(FMath::Atan2(Facing.Y, Facing.X)), 0.f);
    Batch(TEXT("Cube"), Mat, bSolid)->AddInstance(FTransform(Rotation, Origin() + M(Center), FVector(Size.Z, Size.X, Size.Y)), true);
}

void AVortexAwakeningStadium::Cylinder(UMaterialInterface* Mat, const FVector& Center, float Radius, float Height, bool bSolid)
{
    Batch(TEXT("Cylinder"), Mat, bSolid)->AddInstance(FTransform(FRotator::ZeroRotator, Origin() + M(Center), FVector(Radius * 2.f, Radius * 2.f, Height)), true);
}

void AVortexAwakeningStadium::Cone(UMaterialInterface* Mat, const FVector& Base, float Radius, float Height)
{
    Batch(TEXT("Cone"), Mat, false)->AddInstance(FTransform(FRotator::ZeroRotator, Origin() + M(Base + FVector(0.f, 0.f, Height * .5f)), FVector(Radius * 2.f, Radius * 2.f, Height)), true);
}

void AVortexAwakeningStadium::Column(UMaterialInterface* Shaft, const FVector& Foot, float Height, float Radius, bool bSolid)
{
    Box(Material(TEXT("basalt")), Foot + FVector(0, 0, .15f), FVector(Radius * 2.6f, .3f, Radius * 2.6f), 0.f);
    Cylinder(Shaft, Foot + FVector(0, 0, Height * .5f), Radius, Height - .9f, bSolid);
    Box(Shaft, Foot + FVector(0, 0, Height - .38f), FVector(Radius * 2.4f, .2f, Radius * 2.4f), 0.f);
    Box(Material(TEXT("travertine")), Foot + FVector(0, 0, Height - .14f), FVector(Radius * 2.8f, .28f, Radius * 2.8f), 0.f);
}

void AVortexAwakeningStadium::Arch(UMaterialInterface* Mat, const FVector& Center, float FacingDeg, float Span, float Thickness, float Depth, int32 Pieces)
{
    // semicírculo de aduelas no plano da parede (tangente x altura)
    const FVector Facing = Dir(FacingDeg);
    const FVector Tangent(Facing.Y, -Facing.X, 0.f);
    const float R = Span * .5f;
    const FQuat Base = FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(Facing.Y, Facing.X)), 0.f).Quaternion();
    UInstancedStaticMeshComponent* Ism = Batch(TEXT("Cube"), Mat, false);
    for (int32 k = 0; k < Pieces; ++k)
    {
        const float A0 = PI * k / Pieces, A1 = PI * (k + 1) / Pieces, A = (A0 + A1) * .5f;
        const FVector P = Center + Tangent * FMath::Cos(A) * (R + Thickness * .5f) + FVector(0, 0, FMath::Sin(A) * (R + Thickness * .5f));
        const float Len = (R + Thickness) * (A1 - A0) + .04f;
        // o eixo Y da caixa (comprimento da pedra) segue o arco: Y gira em direção a Z por (90° - A)
        // (o Y do giro é o oposto da tangente usada nas posições)
        const FQuat Roll(FVector::XAxisVector, HALF_PI - A);
        Ism->AddInstance(FTransform(Base * Roll, Origin() + M(P), FVector(Depth, Len, Thickness)), true);
    }
    Box(Mat, Center + FVector(0, 0, R + Thickness * .55f), FVector(.38f, Thickness * 1.15f, Depth + .12f), FacingDeg);
}

void AVortexAwakeningStadium::Brazier(const FVector& At, const FLinearColor& LightColor)
{
    Box(Material(TEXT("basalt")), At + FVector(0, 0, .1f), FVector(.7f, .2f, .7f), 0.f, true);
    Cylinder(Material(TEXT("bronze")), At + FVector(0, 0, .6f), .09f, .9f);
    Cylinder(Material(TEXT("bronze")), At + FVector(0, 0, 1.15f), .32f, .3f);
    // chama: três placas cruzadas brilhando (tremem no Tick) e uma luz quente
    UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Stadium/M_StadiumGlow.M_StadiumGlow"));
    UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
    UStaticMeshComponent* Flame = NewObject<UStaticMeshComponent>(this);
    Flame->SetupAttachment(Root);
    Flame->SetStaticMesh(Plane);
    Flame->SetMaterial(0, Glow);
    Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Flame->SetCastShadow(false);
    Flame->SetWorldLocation(Origin() + M(At + FVector(0, 0, 1.62f)));
    Flame->SetWorldScale3D(FVector(.45f, .45f, .7f));
    Flame->RegisterComponent();
    Flames.Add(Flame);
    UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetupAttachment(Root);
    Light->SetWorldLocation(Origin() + M(At + FVector(0, 0, 1.9f)));
    Light->SetLightColor(LightColor);
    Light->SetIntensityUnits(ELightUnits::Candelas);
    Light->SetIntensity(60.f);
    Light->SetAttenuationRadius(1000.f);
    Light->SetCastShadows(false);
    Light->RegisterComponent();
    FlameLights.Add(Light);
}

void AVortexAwakeningStadium::Sign(const FVector& At, float FacingDeg, const FString& Text, float Size, const FLinearColor& Color)
{
    UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this);
    Label->SetupAttachment(Root);
    Label->SetText(FText::FromString(Text));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
    Label->SetWorldSize(Size * 100.f);
    Label->SetTextRenderColor(Color.ToFColor(true));
    // o texto fica de frente para Facing (quem olha de lá lê)
    const FVector Facing = Dir(FacingDeg);
    Label->SetWorldLocationAndRotation(Origin() + M(At), FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(Facing.Y, Facing.X)), 0.f));
    Label->RegisterComponent();
}

void AVortexAwakeningStadium::BeginPlay()
{
    Super::BeginPlay();
    Build();
    if (HasAuthority())
    {
        // bonecos de treino na arena (Godot: 7 m do centro, em 45, 225, 315 e 160 graus)
        for (const float A : { 45.f, 225.f, 315.f, 160.f })
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            const FVector At = Origin() + M(Dir(A) * 7.f) + FVector(0, 0, 95.f);
            GetWorld()->SpawnActor<AVortexTrainingDummy>(At, (-Dir(A)).Rotation(), Params);
        }
    }
}

void AVortexAwakeningStadium::Build()
{
    BuildGrounds();
    BuildArena();
    BuildStands();
    BuildFacade();
    for (const FWing& Wing : Wings) BuildWing(Wing.Id, Wing.Angle);
    BuildExit();
    UE_LOG(LogTemp, Display, TEXT("VORTEX_STADIUM montado lotes=%d pedestais=%d"), Batches.Num(), Pedestals.Num());
}

void AVortexAwakeningStadium::BuildGrounds()
{
    // platô de grama (10 cm abaixo da areia), colinas em volta e ciprestes
    Cylinder(Material(TEXT("grass")), FVector(0, 0, -.7f), 150.f, 1.2f, true);
    FRandomStream Rng(7);
    for (int32 i = 0; i < 40; ++i)
    {
        const float A = i * 360.f / 40 + Rng.FRand() * 6.f;
        const float R = 105.f + Rng.FRand() * 35.f, H = 10.f + Rng.FRand() * 22.f, W = 22.f + Rng.FRand() * 20.f;
        Cone(Material(TEXT("grass")), Dir(A) * R - FVector(0, 0, 1.2f), W * .5f, H);
        if (Rng.FRand() < .6f) Box(Material(TEXT("travertine")), Dir(A) * R + FVector(0, 0, H * .45f), FVector(W * .22f, H * .3f, W * .16f), Rng.FRand() * 360.f);
    }
    for (int32 i = 0; i < 70; ++i)
    {
        const float A = i * 360.f / 70 + Rng.FRand() * 4.f;
        const FVector At = Dir(A) * (64.f + Rng.FRand() * 34.f);
        Cylinder(Material(TEXT("wood")), At + FVector(0, 0, 1.2f), .25f, 2.4f);
        Cone(Material(TEXT("grass")), At + FVector(0, 0, 2.f), 1.5f, 6.4f);
    }
}

void AVortexAwakeningStadium::BuildArena()
{
    // areia com a rosa dos ventos de mármore no centro
    Cylinder(Material(TEXT("sand")), FVector(0, 0, -.25f), ArenaRadius + .4f, .5f, true);
    Cylinder(Material(TEXT("basalt")), FVector(0, 0, .01f), 3.2f, .04f);
    Cylinder(Material(TEXT("marble")), FVector(0, 0, .03f), 2.6f, .06f);
    for (int32 k = 0; k < 8; ++k)
    {
        Box(Material(k % 2 == 0 ? TEXT("bronze") : TEXT("marble_dark")), Dir(k * 45.f) * (k % 2 == 0 ? 1.45f : 1.1f) + FVector(0, 0, .07f),
            FVector(.12f, .03f, k % 2 == 0 ? 2.2f : 1.4f), k * 45.f);
    }
    // pódio: muro de blocos com rodapé de basalto, capa de mármore, balaustrada e estandartes
    const int32 Segments = 72;
    for (int32 k = 0; k < Segments; ++k)
    {
        const float A = (k + .5f) * 360.f / Segments;
        if (GateDistance(A, ArenaRadius) < GateHalf + .2f) continue;
        const FVector D = Dir(A);
        const float Chord = 2.f * PI * ArenaRadius / Segments + .06f;
        Box(Material(TEXT("limestone_blocks")), D * (ArenaRadius + .35f) + FVector(0, 0, PodiumHeight * .5f), FVector(Chord, PodiumHeight, .7f), A, true);
        Box(Material(TEXT("basalt")), D * (ArenaRadius - .02f) + FVector(0, 0, .3f), FVector(Chord, .6f, .16f), A);
        Box(Material(TEXT("marble")), D * (ArenaRadius + .32f) + FVector(0, 0, PodiumHeight + .1f), FVector(Chord + .02f, .2f, .95f), A);
        if (k % 3 == 0) Cylinder(Material(TEXT("bronze")), D * (ArenaRadius + .1f) + FVector(0, 0, PodiumHeight + .55f), .05f, .7f);
        Box(Material(TEXT("bronze")), D * (ArenaRadius + .1f) + FVector(0, 0, PodiumHeight + .92f), FVector(Chord, .06f, .08f), A);
        if (k % 6 == 3)
        {
            const FLinearColor Color(Wings[FMath::RoundToInt(A / 90.f) % 4].Color);
            Box(Cloth(Color), D * (ArenaRadius - .03f) + FVector(0, 0, 2.f), FVector(1.f, 2.f, .04f), A);
            Box(Material(TEXT("bronze")), D * (ArenaRadius - .06f) + FVector(0, 0, 3.02f), FVector(1.2f, .08f, .08f), A);
        }
    }
    // braseiros nos vãos entre os grupos de pedestais (a Ala Marcial, com 10 armas, vai de -45 a 45 graus)
    for (const float A : { 60.f, 222.5f, 320.f }) Brazier(Dir(A) * (ArenaRadius - 1.4f), FLinearColor(1.f, .69f, .38f));
    // título sobre o portão da Ala Marcial, de frente para quem nasce (olhando para o norte)
    Sign(Dir(0.f) * (ArenaRadius - .4f) + FVector(0, 0, 8.6f), 180.f, TEXT("ESTÁDIO DO DESPERTAR"), 1.3f, FLinearColor(1.f, .89f, .63f));
    Sign(Dir(0.f) * (ArenaRadius - .4f) + FVector(0, 0, 7.6f), 180.f, TEXT("Escolha sua arma nos pedestais e teste nos bonecos"), .5f, FLinearColor(.96f, .92f, .82f));
}

void AVortexAwakeningStadium::BuildStands()
{
    const int32 Segments = 64;
    for (int32 k = 0; k < Segments; ++k)
    {
        const float A = (k + .5f) * 360.f / Segments;
        const FVector D = Dir(A);
        for (int32 t = 0; t < Tiers; ++t)
        {
            const float R = StandsInner + t * Tier + Tier * .5f;
            if (GateDistance(A, R) < GateHalf + .4f) continue;
            const float Chord = 2.f * PI * R / Segments + .05f;
            const float Top = PodiumHeight + (t + 1) * TierRise;
            Box(Material(TEXT("limestone_blocks")), D * R + FVector(0, 0, Top * .5f - .1f), FVector(Chord, Top - .2f, Tier), A, true);
            Box(Material(TEXT("travertine")), D * (R - Tier * .25f) + FVector(0, 0, Top - .05f), FVector(Chord + .01f, .2f, Tier * .55f), A);
        }
    }
    // galeria do alto: colunata com arquitrave e o muro de fundo
    const float GalleryR = StandsInner + Tiers * Tier + .8f, GalleryY = PodiumHeight + Tiers * TierRise;
    for (int32 k = 0; k < 40; ++k)
    {
        const float A = k * 360.f / 40;
        if (GateDistance(A, GalleryR) < GateHalf + 1.2f) continue;
        Column(Material(TEXT("travertine")), Dir(A) * GalleryR + FVector(0, 0, GalleryY), 4.2f, .32f, false);
    }
    for (int32 k = 0; k < 80; ++k)
    {
        const float A = (k + .5f) * 360.f / 80;
        if (GateDistance(A, GalleryR) < GateHalf + 1.2f) continue;
        const float Chord = 2.f * PI * GalleryR / 80 + .08f;
        Box(Material(TEXT("travertine")), Dir(A) * GalleryR + FVector(0, 0, GalleryY + 4.45f), FVector(Chord, .5f, 1.f), A);
        Box(Material(TEXT("limestone_blocks")), Dir(A) * (GalleryR + 1.4f) + FVector(0, 0, GalleryY + 2.6f), FVector(Chord, 5.2f, .6f), A, true);
    }
    // mastros com flâmulas nas cores das alas
    for (int32 k = 0; k < 16; ++k)
    {
        const float A = k * 22.5f + 11.25f;
        if (GateDistance(A, GalleryR) < GateHalf + 1.5f) continue;
        const FVector Foot = Dir(A) * (GalleryR + 1.f) + FVector(0, 0, GalleryY + 5.3f);
        Cylinder(Material(TEXT("wood")), Foot + FVector(0, 0, 3.f), .14f, 6.f);
        Box(Cloth(FLinearColor(Wings[k % 4].Color)), Foot + FVector(0, 0, 4.6f) - Dir(A) * .7f, FVector(1.3f, 2.4f, .04f), A + 90.f);
    }
}

void AVortexAwakeningStadium::BuildFacade()
{
    // anel de arcos de três andares (visto de fora e das alas)
    const int32 Bays = 44;
    const float StoryH = 5.2f, Span = 2.6f;
    for (int32 k = 0; k < Bays; ++k)
    {
        const float A = (k + .5f) * 360.f / Bays;
        if (GateDistance(A, FacadeRadius) < GateHalf + 1.6f) continue;
        const FVector D = Dir(A);
        const FVector Tangent(D.Y, -D.X, 0.f);
        const float Chord = 2.f * PI * FacadeRadius / Bays;
        for (int32 s = 0; s < 3; ++s)
        {
            const float Y0 = s * StoryH;
            const FVector Base = D * FacadeRadius + FVector(0, 0, Y0);
            for (const float Side : { -1.f, 1.f })
            {
                Box(Material(TEXT("limestone_blocks")), Base + Tangent * Side * (Span * .5f + (Chord - Span) * .25f) + FVector(0, 0, StoryH * .5f),
                    FVector((Chord - Span) * .5f + .02f, StoryH, 1.f), A);
            }
            const float Springing = StoryH - Span * .5f - .75f;
            Box(Material(TEXT("basalt")), Base + D * .06f + FVector(0, 0, Springing * .5f), FVector(Span, Springing, .06f), A);
            Box(Material(TEXT("limestone_blocks")), Base + FVector(0, 0, (Springing + StoryH) * .5f), FVector(Span + .1f, StoryH - Springing, .1f), A);
            Arch(Material(TEXT("travertine")), Base + D * .42f + FVector(0, 0, Springing), A, Span, .32f, .3f);
            Box(Material(TEXT("travertine")), Base + D * .1f + FVector(0, 0, StoryH + .1f), FVector(Chord + .04f, .26f, 1.3f), A);
        }
        Box(Material(TEXT("limestone_blocks")), D * FacadeRadius + FVector(0, 0, 3.f * StoryH + 1.5f), FVector(Chord + .02f, 2.6f, 1.f), A);
        // colisão: um painel alto por vão
        Box(Material(TEXT("limestone_blocks")), D * (FacadeRadius - .45f) + FVector(0, 0, 8.f), FVector(Chord, 16.f, .1f), A, true);
    }
}

void AVortexAwakeningStadium::Gate(float Angle, float From, float To, const FLinearColor& Color, float WallH)
{
    const FVector D = Dir(Angle);
    const FVector Side(D.Y, -D.X, 0.f);
    const float Len = To - From;
    const FVector Mid = D * (From + Len * .5f);
    // corredor: piso de basalto com tapete na cor da ala, paredes de blocos e arcos nas pontas
    Box(Material(TEXT("basalt")), Mid - FVector(0, 0, .24f), FVector(GateHalf * 2.f + .2f, .5f, Len + .4f), Angle, true);
    Box(Cloth(Color), Mid + FVector(0, 0, .025f), FVector(1.6f, .03f, Len), Angle);
    for (const float S : { -1.f, 1.f })
    {
        Box(Material(TEXT("limestone_blocks")), Mid + Side * (GateHalf + .4f) * S + FVector(0, 0, WallH * .5f), FVector(.8f, WallH, Len + .4f), Angle, true);
        Box(Material(TEXT("basalt")), Mid + Side * (GateHalf - .02f) * S + FVector(0, 0, .3f), FVector(.08f, .6f, Len), Angle);
        Box(Material(TEXT("travertine")), Mid + Side * (GateHalf + .4f) * S + FVector(0, 0, WallH + .12f), FVector(1.1f, .25f, Len + .6f), Angle);
    }
    for (const float At : { From + .3f, To - .3f })
    {
        const FVector Center = D * At + FVector(0, 0, WallH - GateHalf - .3f);
        Arch(Material(TEXT("travertine")), Center, Angle, GateHalf * 2.f + .1f, .45f, .9f, 11);
        Box(Material(TEXT("limestone_blocks")), Center + FVector(0, 0, GateHalf + 1.25f), FVector(GateHalf * 2.f + 2.4f, 1.f, .9f), Angle);
    }
}

void AVortexAwakeningStadium::BuildWing(FName Wing, float Angle)
{
    // a ala virou um grupo de pedestais dentro da própria arena (antes: uma sala no fim de um corredor de
    // 30 m, uma por direção): dá para ver todas as armas assim que acorda
    const FWing* Info = nullptr;
    for (const FWing& W : Wings) if (Wing == W.Id) Info = &W;
    if (!Info) return;
    const FLinearColor Color(Info->Color);
    const FVector D = Dir(Angle);

    // as armas dessa ala (sem a katana, como no Godot)
    TArray<FName> Types;
    FString Text;
    if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Rpg/weapons.json"))))
    {
        TSharedPtr<FJsonObject> Json;
        if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) && Json)
        {
            for (const TSharedPtr<FJsonValue>& V : Json->GetArrayField(TEXT("types")))
            {
                const TSharedPtr<FJsonObject> T = V->AsObject();
                if (T->GetStringField(TEXT("wing")) == Wing.ToString() && T->GetStringField(TEXT("id")) != TEXT("katana_do_vento"))
                {
                    Types.Add(FName(*T->GetStringField(TEXT("id"))));
                }
            }
        }
    }
    // ala sem nenhuma arma no jogo (as armas tiradas em 28/10/2026): nem estandarte
    if (Types.IsEmpty()) return;
    // estandarte grande no muro do pódio, atrás do grupo, e o nome da ala em cima, de frente para o centro
    Box(Cloth(Color), D * (ArenaRadius - .04f) + FVector(0, 0, 1.75f), FVector(3.4f, 2.7f, .04f), Angle);
    Box(Material(TEXT("bronze")), D * (ArenaRadius - .08f) + FVector(0, 0, 3.15f), FVector(3.7f, .1f, .1f), Angle);
    Sign(D * (ArenaRadius - .4f) + FVector(0, 0, 4.9f), Angle + 180.f, Info->Name, .95f, Lighten(Color, .5f));

    // em arco a PedestalRing do centro, PedestalStep graus entre um e outro, virados para o meio, cada um num
    // tapete da cor da ala
    const int32 N = Types.Num();
    for (int32 i = 0; i < N; ++i)
    {
        const float A = Angle + (i - (N - 1) * .5f) * PedestalStep;
        const FVector At = Dir(A) * PedestalRing;
        Box(Cloth(Color), At + FVector(0, 0, .015f), FVector(1.9f, .02f, 1.9f), A);
        BuildPedestal(Types[i], At, FVector2D(-Dir(A)), Color);
    }
}

void AVortexAwakeningStadium::BuildPedestal(FName Type, const FVector& At, const FVector2D& Face, const FLinearColor& Color)
{
    const float Facing = FMath::RadiansToDegrees(FMath::Atan2(Face.X, Face.Y));
    // plinto de basalto, corpo de mármore com frisos de bronze, tampo na cor da ala
    Box(Material(TEXT("basalt")), At + FVector(0, 0, .12f), FVector(1.3f, .24f, 1.3f), Facing, true);
    Box(Material(TEXT("marble")), At + FVector(0, 0, .62f), FVector(.95f, .78f, .95f), Facing, true);
    Box(Material(TEXT("bronze")), At + FVector(0, 0, .3f), FVector(1.f, .06f, 1.f), Facing);
    Box(Material(TEXT("bronze")), At + FVector(0, 0, 1.04f), FVector(1.12f, .1f, 1.12f), Facing);
    Box(Cloth(Color), At + FVector(0, 0, 1.1f), FVector(.9f, .03f, .9f), Facing);
    Box(Material(TEXT("bronze")), At + FVector(0, 0, .66f) + FVector(Face.X, Face.Y, 0) * .49f, FVector(.82f, .26f, .03f), Facing);
    FPedestal Pedestal;
    Pedestal.Type = Type;
    Pedestal.Where = Origin() + M(At);
    UVortexWeaponDefinition* Definition = UVortexWeaponComponent::LoadType(Type);
    Pedestal.Definition = Definition;
    if (Definition) KeptDefinitions.AddUnique(Definition);
    if (Definition && Definition->VisualMesh)
    {
        // a arma em pé (eixo mais comprido na vertical), cabe em 1,1 m de altura e 0,4 m de raio,
        // flutuando 15 cm acima do tampo e girando (antes 1,7 m: subia a mais de 3 m do chão)
        UStaticMeshComponent* Display = NewObject<UStaticMeshComponent>(this);
        Display->SetupAttachment(Root);
        Display->SetStaticMesh(Definition->VisualMesh);
        Display->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Display->SetCastShadow(false);   // a sombra da arma sobre ela mesma pintava triângulos pretos
        const FBox Bounds = Definition->VisualMesh->GetBoundingBox();
        const FVector Size = Bounds.GetSize();
        FRotator Upright = FRotator::ZeroRotator;
        float Height = Size.Z, Radius = FMath::Max(Size.X, Size.Y) * .5f;
        if (Size.X > Size.Z && Size.X >= Size.Y) { Upright = FRotator(90.f, 0.f, 0.f); Height = Size.X; Radius = FMath::Max(Size.Y, Size.Z) * .5f; }
        else if (Size.Y > Size.Z) { Upright = FRotator(0.f, 0.f, 90.f); Height = Size.Y; Radius = FMath::Max(Size.X, Size.Z) * .5f; }
        const float Scale = FMath::Min(110.f / FMath::Max(Height, 1.f), 40.f / FMath::Max(Radius, 1.f));
        Display->SetRelativeScale3D(FVector(Scale));
        Display->SetRelativeRotation(Upright);
        Display->RegisterComponent();
        // centraliza: o centro do volume no eixo do pedestal
        const FVector CenterOffset = Upright.RotateVector(Bounds.GetCenter()) * Scale;
        Pedestal.BaseZ = (1.27f * 100.f) + Height * Scale * .5f;
        Display->SetWorldLocation(Pedestal.Where + FVector(0, 0, Pedestal.BaseZ) - CenterOffset);
        Pedestal.Upright = Upright.Quaternion();
        Pedestal.CenterOffset = CenterOffset;
        Pedestal.Display = Display;
    }
    Sign(At + FVector(0, 0, 2.75f), Facing, Definition ? Definition->DisplayName.ToUpper() : Type.ToString(), .2f, Lighten(Color, .6f));
    // facho suave sobre a arma (o spot do Godot), na cor da ala
    UPointLightComponent* Beam = NewObject<UPointLightComponent>(this);
    Beam->SetMobility(EComponentMobility::Movable);
    Beam->SetupAttachment(Root);
    Beam->SetWorldLocation(Origin() + M(At + FVector(0, 0, 3.2f) + FVector(Face.X, Face.Y, 0) * .6f));
    // 01/11: quase branca e mais fraca (a 45 cd na cor da ala estourava a lâmina e tingia a arma de rosa)
    Beam->SetLightColor(Lighten(Color, .85f));
    Beam->SetIntensityUnits(ELightUnits::Candelas);
    Beam->SetIntensity(14.f);
    Beam->SetAttenuationRadius(450.f);
    Beam->SetCastShadows(false);
    Beam->RegisterComponent();
    Pedestals.Add(Pedestal);
}

void AVortexAwakeningStadium::BuildExit()
{
    const FVector D = Dir(ExitAngle);
    Gate(ExitAngle, ArenaRadius - .2f, ArenaRadius + 5.5f, FLinearColor(FColor(0x8a, 0x6a, 0x1e)), 6.4f);
    const FVector GateAt = D * (ArenaRadius + 5.2f);
    // portal dourado no fim do corredor
    UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Stadium/M_StadiumGlow.M_StadiumGlow"));
    UMaterialInstanceDynamic* Gold = Glow ? UMaterialInstanceDynamic::Create(Glow, this) : nullptr;
    if (Gold)
    {
        Gold->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, .78f, .35f));
        Gold->SetScalarParameterValue(TEXT("Glow"), .7f);
    }
    Box(Gold, GateAt + FVector(0, 0, 2.3f), FVector(GateHalf * 2.f - .2f, 4.4f, .06f), ExitAngle);
    Box(Material(TEXT("limestone_blocks")), GateAt + D * .7f + FVector(0, 0, 3.2f), FVector(GateHalf * 2.f + 1.f, 6.4f, .6f), ExitAngle, true);
    UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetupAttachment(Root);
    Light->SetWorldLocation(Origin() + M(GateAt + FVector(0, 0, 2.5f) - D * 1.2f));
    Light->SetLightColor(FLinearColor(1.f, .85f, .42f));
    Light->SetIntensityUnits(ELightUnits::Candelas);
    Light->SetIntensity(45.f);
    Light->SetAttenuationRadius(700.f);
    Light->SetCastShadows(false);
    Light->RegisterComponent();
    Sign(GateAt - D * 1.f + FVector(0, 0, 5.2f), ExitAngle + 180.f, TEXT("SAÍDA  •  A JORNADA"), .55f, FLinearColor(1.f, .89f, .6f));
    ExitAt = Origin() + M(GateAt - D * .9f);
}

const AVortexAwakeningStadium::FPedestal* AVortexAwakeningStadium::NearestPedestal(const FVector& Where) const
{
    const FPedestal* Best = nullptr;
    float BestDistance = 230.f;
    for (const FPedestal& P : Pedestals)
    {
        const float Distance = FVector::Dist2D(Where, P.Where);
        if (Distance < BestDistance) { BestDistance = Distance; Best = &P; }
    }
    return Best;
}

const AVortexAwakeningStadium::FPedestal* AVortexAwakeningStadium::FindPedestal(FName Type) const
{
    return Pedestals.FindByPredicate([Type](const FPedestal& P) { return P.Type == Type; });
}

void AVortexAwakeningStadium::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Clock += DeltaSeconds;
    // chamas tremendo
    for (int32 i = 0; i < Flames.Num(); ++i)
    {
        const float F = .85f + .15f * FMath::Sin(Clock * 9.f + i * 1.7f) + .08f * FMath::Sin(Clock * 23.f + i * 3.1f);
        Flames[i]->SetWorldScale3D(FVector(.45f, .45f, .7f * F));
        if (FlameLights.IsValidIndex(i)) FlameLights[i]->SetIntensity(60.f * F);
    }
    // armas girando e flutuando; a que está na mão sai do pedestal
    for (const FPedestal& P : Pedestals)
    {
        UStaticMeshComponent* Display = P.Display.Get();
        if (!Display) continue;
        Display->SetVisibility(P.Type != HeldType);
        const FQuat Spin = FRotator(0.f, Clock * 52.f, 0.f).Quaternion();
        const FVector Lift(0.f, 0.f, P.BaseZ + FMath::Sin(Clock * 1.6f + P.Where.X * .01f) * 6.f);
        Display->SetWorldLocationAndRotation(P.Where + Lift - Spin.RotateVector(P.CenterOffset), Spin * P.Upright);
    }
}
