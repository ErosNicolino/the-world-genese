#include "Combat/VortexMonsterFx.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

namespace
{
    struct FLook { float Life; TArray<FLinearColor> Colors; };

    // a cor de cada tom (do escuro ao claro) e quanto dura cada efeito
    const FLook& LookOf(FName Kind)
    {
        static const TMap<FName, FLook> Looks = {
            { TEXT("PoisonCloud"), { 2.2f, { FLinearColor(.06f, .2f, .03f), FLinearColor(.14f, .42f, .06f), FLinearColor(.32f, .7f, .14f) } } },
            { TEXT("PoisonPool"), { 2.6f, { FLinearColor(.05f, .18f, .03f), FLinearColor(.12f, .38f, .05f), FLinearColor(.4f, .85f, .2f) } } },
            { TEXT("Drip"), { .45f, { FLinearColor(.12f, .45f, .05f), FLinearColor(.3f, .8f, .14f), FLinearColor(.3f, .8f, .14f) } } },
            { TEXT("Tornado"), { 1.7f, { FLinearColor(.55f, .62f, .7f), FLinearColor(.8f, .86f, .92f), FLinearColor(1.f, 1.f, 1.f) } } },
            { TEXT("Dust"), { 1.f, { FLinearColor(.16f, .13f, .1f), FLinearColor(.3f, .25f, .19f), FLinearColor(.45f, .4f, .32f) } } },
            { TEXT("Splash"), { .8f, { FLinearColor(.1f, .14f, .05f), FLinearColor(.2f, .3f, .1f), FLinearColor(.35f, .5f, .2f) } } },
        };
        static const FLook Fallback = { 1.f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } };
        const FLook* Look = Looks.Find(Kind);
        return Look ? *Look : Fallback;
    }

    /** A cor da névoa e do círculo de cada efeito. */
    FLinearColor TintOf(FName Kind)
    {
        if (Kind == TEXT("Tornado")) return FLinearColor(.6f, .55f, 1.f);
        if (Kind == TEXT("PoisonCloud") || Kind == TEXT("PoisonPool") || Kind == TEXT("Drip")) return FLinearColor(.28f, .75f, .12f);
        if (Kind == TEXT("Dust")) return FLinearColor(.55f, .43f, .3f);
        if (Kind == TEXT("Splash")) return FLinearColor(.3f, .42f, .16f);
        return FLinearColor::White;
    }

    float Bump(float T) { return FMath::Sin(FMath::Clamp(T, 0.f, 1.f) * UE_PI); }
}

AVortexMonsterFx::AVortexMonsterFx()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    SetActorEnableCollision(false);
}

AVortexMonsterFx* AVortexMonsterFx::Spawn(UWorld* World, FName InKind, const FVector& At, float InRadius, float InDelay, AActor* Follow)
{
    if (!World || World->GetNetMode() == NM_DedicatedServer) return nullptr;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AVortexMonsterFx* Fx = World->SpawnActor<AVortexMonsterFx>(At, FRotator::ZeroRotator, Params);
    if (!Fx) return nullptr;
    Fx->Kind = InKind;
    Fx->Radius = FMath::Clamp(InRadius, 40.f, 900.f);
    Fx->Delay = FMath::Max(0.f, InDelay);
    Fx->FollowActor = Follow;
    Fx->Life = LookOf(InKind).Life;
    Fx->Begin();
    return Fx;
}

void AVortexMonsterFx::Begin()
{
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UMaterialInterface* Solid = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SolidColor.M_SolidColor"));
    for (const FLinearColor& Color : LookOf(Kind).Colors)
    {
        UInstancedStaticMeshComponent* Shade = NewObject<UInstancedStaticMeshComponent>(this);
        Shade->SetupAttachment(GetRootComponent());
        Shade->SetUsingAbsoluteLocation(true);
        Shade->SetUsingAbsoluteRotation(true);
        Shade->SetStaticMesh(Cube);
        Shade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Shade->SetCastShadow(false);
        Shade->bAffectDistanceFieldLighting = false;
        if (Solid)
        {
            UMaterialInstanceDynamic* Tint = UMaterialInstanceDynamic::Create(Solid, this);
            Tint->SetVectorParameterValue(TEXT("Color"), Color);
            Shade->SetMaterial(0, Tint);
        }
        Shade->RegisterComponent();
        Shades.Add(Shade);
    }
    // os pedaços de cada efeito
    auto Add = [this](int32 Count, TFunction<void(FBit&, int32)> Make)
    {
        for (int32 i = 0; i < Count; ++i)
        {
            FBit Bit;
            Bit.Shade = FMath::RandRange(0, Shades.Num() - 1);
            Make(Bit, i);
            Bits.Add(Bit);
        }
    };
    if (Kind == TEXT("PoisonCloud"))
        Add(16, [this](FBit& B, int32) { B.Dir = FVector(FMath::VRand().GetSafeNormal2D()); B.A = Radius * FMath::FRandRange(.25f, 1.f);
            B.B = FMath::FRandRange(20.f, 110.f); B.C = FMath::FRandRange(0.f, .35f); B.Size = FMath::FRandRange(22.f, 46.f); });
    else if (Kind == TEXT("PoisonPool"))
    {
        Add(22, [this](FBit& B, int32) { B.Dir = FVector(FMath::VRand().GetSafeNormal2D()); B.A = Radius * FMath::Sqrt(FMath::FRand());
            B.C = -1.f; B.Size = FMath::FRandRange(22.f, 38.f); });
        Add(12, [this](FBit& B, int32) { B.Dir = FVector(FMath::VRand().GetSafeNormal2D()); B.A = Radius * .8f * FMath::FRand();
            B.C = FMath::FRandRange(0.f, .8f); B.Size = FMath::FRandRange(6.f, 12.f); B.Shade = 2; });
    }
    else if (Kind == TEXT("Drip"))
        Add(1, [](FBit& B, int32) { B.Size = FMath::FRandRange(7.f, 11.f); });
    else if (Kind == TEXT("Tornado"))
    {
        // os pedaços que o vento leva, subindo em espiral (o raio abre com a altura); as pedrinhas giram em volta da base
        Add(34, [](FBit& B, int32 i) { B.A = FMath::FRand(); B.B = FMath::FRandRange(0.f, UE_TWO_PI); B.C = FMath::FRandRange(.85f, 1.25f);
            B.Size = FMath::FRandRange(12.f, 26.f); B.Shade = 1 + (i % 2); });
        Add(22, [](FBit& B, int32) { B.A = -1.f; B.B = FMath::FRandRange(0.f, UE_TWO_PI); B.C = FMath::FRandRange(.8f, 1.2f);
            B.Size = FMath::FRandRange(8.f, 16.f); B.Shade = 0; });
    }
    else if (Kind == TEXT("Dust"))
        Add(18, [this](FBit& B, int32) { B.Dir = FVector(FMath::VRand().GetSafeNormal2D()); B.A = Radius * FMath::FRandRange(.6f, 1.1f);
            B.B = FMath::FRandRange(30.f, 140.f); B.Size = FMath::FRandRange(10.f, 26.f); });
    else if (Kind == TEXT("Splash"))
        Add(26, [this](FBit& B, int32) { B.Dir = FVector(FMath::VRand().GetSafeNormal2D()); B.A = Radius * FMath::FRandRange(.4f, 1.f);
            B.B = FMath::FRandRange(60.f, 180.f); B.Size = FMath::FRandRange(7.f, 14.f); });
    for (const FBit& Bit : Bits) Shades[Bit.Shade]->AddInstance(FTransform(FVector::ZeroVector), true);
    if (Kind != TEXT("Drip"))
    {
        const FLinearColor Tint = TintOf(Kind);
        auto Fx = [&](const TCHAR* Name, float Strength) -> UMaterialInstanceDynamic*
        {
            UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Combat/%s.%s"), Name, Name));
            UMaterialInstanceDynamic* Mat = Base ? UMaterialInstanceDynamic::Create(Base, this) : nullptr;
            if (Mat)
            {
                Mat->SetVectorParameterValue(TEXT("Color"), Tint);
                Mat->SetScalarParameterValue(TEXT("Strength"), Strength);
                Mat->SetScalarParameterValue(TEXT("Seed"), FMath::FRand());
                Fading.Add(Mat);
                FadingStrength.Add(Strength);
            }
            return Mat;
        };
        auto Piece = [&](const TCHAR* Shape, UMaterialInterface* Mat)
        {
            UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
            C->SetupAttachment(GetRootComponent());
            C->SetUsingAbsoluteLocation(true);
            C->SetUsingAbsoluteRotation(true);
            C->SetUsingAbsoluteScale(true);
            C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Shape));
            C->SetMaterial(0, Mat);
            C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            C->SetCastShadow(false);
            C->RegisterComponent();
            return C;
        };
        GroundRing = Piece(TEXT("/Engine/BasicShapes/Plane.Plane"), Fx(TEXT("M_DropRing"), Kind == TEXT("Tornado") ? 1.1f : .9f));
        if (Kind == TEXT("Tornado"))
        {
            Funnels.Add(Piece(TEXT("/Engine/BasicShapes/Cone.Cone"), Fx(TEXT("M_FxFunnel"), .55f)));
            Funnels.Add(Piece(TEXT("/Engine/BasicShapes/Cone.Cone"), Fx(TEXT("M_FxFunnel"), .85f)));
        }
        Puffs = NewObject<UInstancedStaticMeshComponent>(this);
        Puffs->SetupAttachment(GetRootComponent());
        Puffs->SetUsingAbsoluteLocation(true);
        Puffs->SetUsingAbsoluteRotation(true);
        Puffs->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        Puffs->SetMaterial(0, Fx(TEXT("M_FxPuff"), Kind == TEXT("PoisonCloud") ? .55f : .34f));
        Puffs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Puffs->SetCastShadow(false);
        Puffs->RegisterComponent();
        const int32 Count = Kind == TEXT("PoisonCloud") ? 26 : Kind == TEXT("Tornado") ? 18 : Kind == TEXT("Dust") ? 20 : Kind == TEXT("PoisonPool") ? 12 : 10;
        for (int32 i = 0; i < Count; ++i)
        {
            FBit P;
            P.Dir = FVector(FMath::VRand().GetSafeNormal2D());
            P.A = Radius * (Kind == TEXT("PoisonCloud") ? FMath::Sqrt(FMath::FRand()) : FMath::FRandRange(.55f, 1.f));
            P.B = Kind == TEXT("PoisonPool") ? FMath::FRandRange(6.f, 22.f) : FMath::FRandRange(20.f, 120.f);
            P.C = FMath::FRandRange(0.f, .3f);
            P.Size = FMath::Clamp(Radius * FMath::FRandRange(.14f, .26f), 30.f, 85.f);   // grandes demais tapavam a visão
            PuffBits.Add(P);
            Puffs->AddInstance(FTransform(FQuat::Identity, GetActorLocation(), FVector(.001f)), true);
        }
    }
    SetActorHiddenInGame(Delay > 0.f);
    Tick(0.f);
}

FTransform AVortexMonsterFx::PlacePuff(const FBit& Bit, float T) const
{
    // as nuvens: saem do centro, sobem devagar, incham e somem encolhendo
    const FVector Center(FollowActor.IsValid() ? FollowActor->GetActorLocation().X : GetActorLocation().X, FollowActor.IsValid() ? FollowActor->GetActorLocation().Y : GetActorLocation().Y, BaseZ);
    const float Local = FMath::Clamp((T - Bit.C) / (1.f - Bit.C), 0.f, 1.f);
    const float Out = 1.f - FMath::Square(1.f - Local);
    FVector Dir = Bit.Dir;
    if (Kind == TEXT("Tornado")) Dir = Bit.Dir.RotateAngleAxis(Age * 340.f, FVector::UpVector);   // a poeira roda em volta da base
    const float Size = Bit.Size * (.45f + .8f * Local) * Bump(FMath::Min(1.f, Local * .85f + .1f)) / 50.f;
    return FTransform(FQuat::Identity, Center + Dir * Bit.A * Out + FVector(0.f, 0.f, 10.f + Bit.B * Local), FVector(FMath::Max(Size, .001f)));
}

FTransform AVortexMonsterFx::Place(const FBit& Bit, float T) const
{
    const FVector Center = FollowActor.IsValid() ? FollowActor->GetActorLocation() : GetActorLocation();
    const float Fade = FMath::Clamp((1.f - T) * 4.f, 0.f, 1.f);
    if (Kind == TEXT("PoisonCloud"))
    {
        // sai do centro, se espalha e sobe devagar; incha e murcha
        const float Local = FMath::Clamp((T - Bit.C) / (1.f - Bit.C), 0.f, 1.f);
        const float Out = 1.f - FMath::Square(1.f - Local);
        const FVector At = Center + Bit.Dir * Bit.A * Out + FVector(0.f, 0.f, 15.f + Bit.B * Local);
        return FTransform(FRotator(Age * 40.f, Bit.A, Age * 30.f), At, FVector(Bit.Size * Bump(Local * .9f + .1f) * Fade / 100.f));
    }
    if (Kind == TEXT("PoisonPool"))
    {
        if (Bit.C < 0.f)   // a poça: placas baixas no chão que crescem e somem
            return FTransform(FRotator(0.f, Bit.A * 7.f, 0.f), Center + Bit.Dir * Bit.A + FVector(0.f, 0.f, 2.f),
                FVector(Bit.Size, Bit.Size, 3.f) * FMath::Min(1.f, T * 6.f) * Fade / 100.f);
        // as bolhas: sobem e estouram
        const float Cycle = FMath::Fmod(T * 2.5f + Bit.C, 1.f);
        return FTransform(FRotator::ZeroRotator, Center + Bit.Dir * Bit.A + FVector(0.f, 0.f, 4.f + Cycle * 30.f),
            FVector(Bit.Size * Bump(Cycle) * Fade / 100.f));
    }
    if (Kind == TEXT("Drip"))
        return FTransform(FRotator::ZeroRotator, Center - FVector(0.f, 0.f, 90.f * T * T), FVector(Bit.Size * (1.f - T) / 100.f));
    if (Kind == TEXT("Tornado"))
    {
        const float Grow = FMath::Min(1.f, T * 5.f) * Fade;
        const float Height = FMath::Clamp(Radius * .85f, 240.f, 480.f);
        const float Up = BaseZ - Center.Z;   // do chão, mesmo com o morcego voando
        if (Bit.A < 0.f)
        {
            const float Angle = Bit.B + Age * 6.f * Bit.C;
            return FTransform(FRotator(0.f, Angle * 57.3f, 0.f), Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius * .7f
                + FVector(0.f, 0.f, Up + 12.f + 10.f * FMath::Sin(Age * 9.f + Bit.B)), FVector(Bit.Size * Grow / 100.f));
        }
        // a altura do cubo sobe (e volta para baixo): o vento subindo pelo funil
        const float H = FMath::Fmod(Bit.A + Age * .55f * Bit.C, 1.f);
        const float R = FMath::Lerp(.1f, .56f, H * H) * Radius;
        const float Angle = Bit.B + Age * (10.f + 6.f * (1.f - H)) * Bit.C;
        const FVector At = Center + FVector(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R, Up + H * Height);
        return FTransform(FRotator(0.f, Angle * 57.3f, 20.f), At, FVector(Bit.Size * (.6f + .4f * H) * Grow * Bump(H * .9f + .05f) / 100.f));
    }
    if (Kind == TEXT("Dust") || Kind == TEXT("Splash"))
    {
        // saem em anel, sobem num arco e caem; a poeira fica mais tempo no ar
        const float Out = 1.f - FMath::Square(1.f - T);
        const FVector At = Center + Bit.Dir * Bit.A * Out + FVector(0.f, 0.f, Bit.B * Bump(T) + 5.f);
        return FTransform(FRotator(Age * 300.f, Bit.A, 0.f), At, FVector(Bit.Size * Fade / 100.f));
    }
    return FTransform(Center);
}

void AVortexMonsterFx::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Delay > 0.f)
    {
        Delay -= DeltaSeconds;
        if (Delay > 0.f) return;
        SetActorHiddenInGame(false);
    }
    else Age += DeltaSeconds;
    if (Age > Life) { Destroy(); return; }
    const float T = Age / Life;
    // a altura do chão embaixo do efeito (o morcego voa; o círculo e o funil nascem do chão)
    const FVector Over = FollowActor.IsValid() ? FollowActor->GetActorLocation() : GetActorLocation();
    // o terreno, não o primeiro obstáculo: embaixo de árvore, a linha batia na copa e o círculo nascia acima do morcego
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    BaseZ = Terrain ? Terrain->GroundAt(Over.X, Over.Y) : Over.Z - 90.f;
    const float Open = FMath::Min(1.f, T * 5.f) * FMath::Clamp((1.f - T) * 4.f, 0.f, 1.f);
    for (int32 m = 0; m < Fading.Num(); ++m) if (Fading[m]) Fading[m]->SetScalarParameterValue(TEXT("Strength"), FadingStrength[m] * Open);
    if (GroundRing)
    {
        const float Size = FMath::Max(.01f, 2.f * Radius / 100.f * FMath::Min(1.f, T * 6.f) * (1.f + .12f * Bump(T * 6.f)));
        GroundRing->SetWorldLocation(FVector(Over.X, Over.Y, BaseZ + 5.f));
        GroundRing->SetWorldScale3D(FVector(Size, Size, 1.f));
    }
    for (int32 f = 0; f < Funnels.Num(); ++f)
    {
        // o cone de ponta para baixo: bem largo em cima, do tamanho da área de dano
        const float Tall = FMath::Clamp(Radius * .85f, 240.f, 480.f) * (f ? .82f : 1.f);
        const float Wide = 2.f * Radius * (f ? .34f : .56f) / 100.f * FMath::Max(.02f, Open);
        const FVector Sway(FMath::Sin(Age * 3.1f + f) * 14.f, FMath::Cos(Age * 2.6f + f) * 14.f, 0.f);
        Funnels[f]->SetWorldLocationAndRotation(FVector(Over.X, Over.Y, BaseZ + Tall * .5f) + Sway, FRotator(180.f, Age * (f ? -160.f : 120.f), 0.f));
        Funnels[f]->SetWorldScale3D(FVector(Wide, Wide, Tall / 100.f));
    }
    if (Puffs && PuffBits.Num())
    {
        TArray<FTransform> Clouds;
        Clouds.Reserve(PuffBits.Num());
        for (const FBit& Bit : PuffBits) Clouds.Add(PlacePuff(Bit, T));
        Puffs->BatchUpdateInstancesTransforms(0, Clouds, true, true, true);
    }
    TArray<TArray<FTransform>> Poses;
    Poses.SetNum(Shades.Num());
    for (const FBit& Bit : Bits) Poses[Bit.Shade].Add(Place(Bit, T));
    for (int32 s = 0; s < Shades.Num(); ++s)
        if (Poses[s].Num()) Shades[s]->BatchUpdateInstancesTransforms(0, Poses[s], true, true);
}
