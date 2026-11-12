#include "Combat/VortexWeaponFx.h"
#include "Combat/VortexSwords.h"

#include "Combat/VortexClassSkills.h"
#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexHitFx.h"
#include "Combat/VortexProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "Rpg/VortexHeroSheet.h"
#include "Creatures/VortexCreature.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProceduralMeshComponent.h"

namespace
{
    const TCHAR* SpectralGlow = TEXT("/Game/Vortex/Combat/M_SpectralGlow.M_SpectralGlow");
    const TCHAR* CoreGlow = TEXT("/Game/Vortex/Weapons/M_ProjectileGlow.M_ProjectileGlow");

    TArray<TWeakObjectPtr<AVortexWeaponFx>>& Walls()
    {
        static TArray<TWeakObjectPtr<AVortexWeaponFx>> List;
        return List;
    }


    /** O efeito de cada elemento (chamas, água, raios, gelo, folhas, almas, luz). */
    const TCHAR* ElementSystem(FName Element)
    {
        if (Element == TEXT("water")) return TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Mana.NS_Free_Spells_Aura_Mana");
        if (Element == TEXT("lightning")) return TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Lightning.NS_Free_Spells_Aura_Lightning");
        if (Element == TEXT("ice")) return TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Air.NS_Free_Spells_Aura_Air");
        if (Element == TEXT("nature") || Element == TEXT("poison")) return TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Healing.NS_Free_Spells_Aura_Healing");
        if (Element == TEXT("shadow")) return TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Soul.NS_Free_Spells_Aura_Soul");
        if (Element == TEXT("holy")) return TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Healing.NS_Free_Spells_Buff_Healing");
        return TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Medium.NS_Fire_Medium");
    }

    bool IsFire(FName Element) { return Element.IsNone() || Element == TEXT("fire"); }

    /** Ponto da parábola do arremesso (garrafa, poção): sai de From, cai em To, com a altura do meio. */
    FVector Arc(const FVector& From, const FVector& To, float T, float Apex)
    {
        return FMath::Lerp(From, To, T) + FVector(0.f, 0.f, 4.f * Apex * T * (1.f - T));
    }
}

AVortexWeaponFx::AVortexWeaponFx()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);
    SetNetCullDistanceSquared(FMath::Square(15000.f));
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

void AVortexWeaponFx::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Kind, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Color, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Life, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Radius, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Length, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Size, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Speed, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Delay, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Count, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Target, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Follow, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Mesh, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Homing, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexWeaponFx, Element, COND_InitialOnly);
}

AVortexWeaponFx* AVortexWeaponFx::Spawn(AActor* Hero, FName InKind, const FVector& At, const FRotator& Rot, TFunctionRef<void(AVortexWeaponFx&)> Setup)
{
    UWorld* World = Hero ? Hero->GetWorld() : nullptr;
    if (!World || !Hero->HasAuthority()) return nullptr;
    const FTransform Where(Rot, At);
    AVortexWeaponFx* Fx = World->SpawnActorDeferred<AVortexWeaponFx>(AVortexWeaponFx::StaticClass(), Where, Hero, Cast<APawn>(Hero),
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Fx) return nullptr;
    Fx->Kind = InKind;
    Fx->Follow = Hero;
    Setup(*Fx);
    // 07/11 (o usuário: "os efeitos são sempre o mesmo"): nas espadas e arcos com poder próprio o efeito é o deles
    // (AVortexPowerFx); os enfeites genéricos do tipo do poder somem (o dano, quando há, continua: o ator só fica oculto)
    static const TSet<FName> Decorative = { TEXT("slam"), TEXT("sweep"), TEXT("holytrail"), TEXT("seal"), TEXT("aura"), TEXT("beam") };
    if (Decorative.Contains(InKind))
        if (const AVortexCharacter* Owner = Cast<AVortexCharacter>(Hero); Owner && Owner->GetSheet())
            if (const VortexSwords::FSword* Sword = VortexSwords::Find(Owner->GetSheet()->GetEquipped(VortexRpg::Weapon)); Sword && Sword->bHasKit)
                Fx->SetActorHiddenInGame(true);
    Fx->FinishSpawning(Where);
    return Fx;
}

void AVortexWeaponFx::SpawnLocal(AActor* Hero, FName InKind, float Delay, float Reach, const FLinearColor& Tint)
{
    UWorld* World = Hero ? Hero->GetWorld() : nullptr;
    if (!World || World->GetNetMode() == NM_DedicatedServer) return;
    TWeakObjectPtr<AActor> Weak(Hero);
    auto Make = [Weak, InKind, Reach, Tint]()
    {
        AActor* H = Weak.Get();
        if (!H) return;
        const FVector Forward = H->GetActorForwardVector().GetSafeNormal2D();
        const FTransform Where(Forward.Rotation(), H->GetActorLocation() + Forward * 40.f + FVector(0.f, 0.f, 15.f));
        AVortexWeaponFx* Fx = H->GetWorld()->SpawnActorDeferred<AVortexWeaponFx>(AVortexWeaponFx::StaticClass(), Where, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Fx) return;
        Fx->SetReplicates(false);
        Fx->Kind = InKind;
        Fx->Color = Tint;
        Fx->Length = Reach;
        Fx->Life = InKind == TEXT("punch") ? .22f : .26f;
        Fx->FinishSpawning(Where);
    };
    if (Delay <= .01f) { Make(); return; }
    FTimerHandle Handle;
    World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda(Make), Delay, false);
}

void AVortexWeaponFx::BeginPlay()
{
    Super::BeginPlay();
    Start = GetActorLocation();
    Out = GetActorForwardVector().GetSafeNormal();
    if (HasAuthority()) SetLifeSpan(Life + 1.f);
    if (Kind == TEXT("firewall")) Walls().Add(this);
    if (GetNetMode() != NM_DedicatedServer) BuildLook();
}

void AVortexWeaponFx::EndPlay(const EEndPlayReason::Type Reason)
{
    Walls().RemoveAll([this](const TWeakObjectPtr<AVortexWeaponFx>& W) { return !W.IsValid() || W.Get() == this; });
    Super::EndPlay(Reason);
}

// ---- desenho -----------------------------------------------------------------------------------

UStaticMeshComponent* AVortexWeaponFx::AddPart(const TCHAR* Name, bool bGlow, float Opacity)
{
    UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
    Part->SetupAttachment(Root);
    Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name)));
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetCastShadow(false);
    Part->bAffectDistanceFieldLighting = false;
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, bGlow ? CoreGlow : SpectralGlow);
    if (Base)
    {
        UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, this);
        Mat->SetVectorParameterValue(TEXT("Color"), Color);
        Mat->SetScalarParameterValue(bGlow ? TEXT("Glow") : TEXT("Opacity"), bGlow ? 4.f : Opacity);
        Part->SetMaterial(0, Mat);
        Glows.Add(Mat);
    }
    else Glows.Add(nullptr);
    Part->RegisterComponent();
    Parts.Add(Part);
    return Part;
}

UStaticMeshComponent* AVortexWeaponFx::AddMeshPart(UStaticMesh* InMesh)
{
    UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
    Part->SetupAttachment(Root);
    Part->SetStaticMesh(InMesh);
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetCastShadow(true);
    Part->RegisterComponent();
    Parts.Add(Part);
    Glows.Add(nullptr);
    return Part;
}

UNiagaraComponent* AVortexWeaponFx::AddSystem(const TCHAR* Path, const FVector& At, const FVector& Scale)
{
    UNiagaraSystem* System = VortexHitFx::System(Path);
    if (!System) return nullptr;
    UNiagaraComponent* N = UNiagaraFunctionLibrary::SpawnSystemAttached(System, Root, NAME_None, At, FRotator::ZeroRotator, Scale,
        EAttachLocation::KeepRelativeOffset, false, ENCPoolMethod::None);
    if (N) Systems.Add(N);
    return N;
}

void AVortexWeaponFx::BuildLook()
{
    Light = NewObject<UPointLightComponent>(this);
    Light->SetupAttachment(Root);
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetIntensityUnits(ELightUnits::Candelas);
    Light->SetIntensity(0.f);
    Light->SetLightColor(Color);
    Light->SetAttenuationRadius(FMath::Max(300.f, Radius * 2.f));
    Light->SetCastShadows(false);
    Light->RegisterComponent();
    if (Kind == TEXT("aura"))
    {
        // anel no chão e notas subindo em volta de quem toca
        UStaticMeshComponent* Ring = AddPart(TEXT("Cylinder"), false, .25f);
        Ring->SetRelativeScale3D(FVector(Radius / 50.f, Radius / 50.f, .01f));
        Ring->SetRelativeLocation(FVector(0.f, 0.f, -68.f));
        UStaticMeshComponent* Inner = AddPart(TEXT("Cylinder"), false, .35f);
        Inner->SetRelativeScale3D(FVector(1.6f, 1.6f, .015f));
        Inner->SetRelativeLocation(FVector(0.f, 0.f, -66.f));
        AddSystem(TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Notes.NS_Free_Spells_Aura_Notes"), FVector(0, 0, -60), FVector(1.f));
        AddSystem(TEXT("/Game/FreeParticle_SoftTofu/Niagara/NS_Sparkling_Note.NS_Sparkling_Note"), FVector(0, 0, 40), FVector(1.f));
        Light->SetIntensity(4.f);
    }
    else if (Kind == TEXT("orbit"))
    {
        for (int32 i = 0; i < 2; ++i)
        {
            UStaticMeshComponent* Orb = AddPart(TEXT("Sphere"), true);
            Orb->SetRelativeScale3D(FVector(.3f));
            if (Glows.Last()) Glows.Last()->SetScalarParameterValue(TEXT("Glow"), 9.f);
            if (UNiagaraSystem* Sparkle = VortexHitFx::System(TEXT("/Game/FreeParticle_SoftTofu/Niagara/NS_Sparkling_Glow.NS_Sparkling_Glow")))
                Systems.Add(UNiagaraFunctionLibrary::SpawnSystemAttached(Sparkle, Orb, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.f),
                    EAttachLocation::KeepRelativeOffset, false, ENCPoolMethod::None));
            UPointLightComponent* OrbLight = NewObject<UPointLightComponent>(this);
            OrbLight->SetupAttachment(Orb);
            OrbLight->SetIntensityUnits(ELightUnits::Candelas);
            OrbLight->SetIntensity(6.f);
            OrbLight->SetLightColor(Color);
            OrbLight->SetAttenuationRadius(260.f);
            OrbLight->SetCastShadows(false);
            OrbLight->RegisterComponent();
        }
        Light->SetIntensity(0.f);
    }
    else if (Kind == TEXT("tornado"))
    {
        // o círculo mágico aparece primeiro; o tornado sobe depois
        // o círculo mágico no chão primeiro; depois o tornado: chamas numa espiral que abre para cima (funil) e gira
        UStaticMeshComponent* Circle = AddPart(TEXT("Cylinder"), false, .45f);
        Circle->SetRelativeScale3D(FVector(.01f));
        AddSystem(TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Floor_01.NS_Fire_Floor_01"), FVector(0, 0, 5), FVector(Radius / 200.f));
        Spinner = NewObject<USceneComponent>(this);
        Spinner->SetupAttachment(Root);
        Spinner->RegisterComponent();
    }
    else if (Kind == TEXT("firewall"))
    {
        const int32 N = FMath::Max(3, FMath::RoundToInt(Length / 100.f));
        for (int32 i = 0; i < N; ++i)
        {
            const float Y = -Length * .5f + Length * (i + .5f) / N;
            AddSystem(ElementSystem(Element), FVector(0.f, Y, 0.f), FVector(IsFire(Element) ? 1.f : .7f, IsFire(Element) ? 1.f : .7f, IsFire(Element) ? 1.f : 1.6f));
        }
        UStaticMeshComponent* Band = AddPart(TEXT("Cube"), false, .2f);
        Band->SetRelativeScale3D(FVector(.6f, Length / 100.f, .02f));
        Band->SetRelativeLocation(FVector(0.f, 0.f, 4.f));
        Light->SetIntensity(30.f);
        Light->SetAttenuationRadius(Length);
    }
    else if (Kind == TEXT("vines"))
    {
        for (int32 i = 0; i < 6; ++i)
        {
            UStaticMeshComponent* Vine = AddPart(TEXT("Cylinder"), false, .7f);
            Vine->SetRelativeScale3D(FVector(.06f, .06f, .01f));
        }
        AddSystem(TEXT("/Game/FreeParticle_SoftTofu/Niagara/NS_leaf.NS_leaf"), FVector::ZeroVector, FVector(.6f));
    }
    else if (Kind == TEXT("lob") || Kind == TEXT("potion"))
    {
        if (Mesh) AddMeshPart(Mesh)->SetRelativeScale3D(FVector(1.1f));
        else AddPart(TEXT("Sphere"), true)->SetRelativeScale3D(FVector(.12f));
    }
    else if (Kind == TEXT("holytrail"))
    {
        for (int32 i = 0; i < 7; ++i)
        {
            UStaticMeshComponent* Crack = AddPart(TEXT("Cube"), false, .6f);
            Crack->SetRelativeLocation(FVector(Length * (i + .5f) / 7.f, FMath::Sin(i * 1.7f) * 18.f, 3.f));
            Crack->SetRelativeScale3D(FVector(.01f));
        }
        AddSystem(TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Healing.NS_Free_Spells_Buff_Healing"), FVector(0, 0, -60), FVector(1.f));
        Light->SetIntensity(25.f);
    }
    else if (Kind == TEXT("boomerang"))
    {
        if (Mesh)
        {
            UStaticMeshComponent* Spin = AddMeshPart(Mesh);
            Spin->SetRelativeScale3D(FVector(Size));
            if (Count == 1)
            {
                // chakram: o disco deitado (o eixo mais fino da malha para cima), girando em volta de si
                const FVector Ext = Mesh->GetBoundingBox().GetExtent();
                FVector Thin = FVector::ZeroVector;
                Thin[Ext.X <= Ext.Y && Ext.X <= Ext.Z ? 0 : (Ext.Y <= Ext.Z ? 1 : 2)] = 1.f;
                const FVector Center = Mesh->GetBoundingBox().GetCenter();
                const FQuat Flat = FQuat::FindBetweenNormals(Thin, FVector::UpVector);
                Spin->SetRelativeRotation(Flat);
                Spin->SetRelativeLocation(-Flat.RotateVector(Center) * Size);
            }
        }
        if (Size > 2.f)
        {
            UStaticMeshComponent* Halo = AddPart(TEXT("Cylinder"), false, .3f);
            Halo->SetRelativeScale3D(FVector(Size * .36f, Size * .36f, .01f));
            Light->SetIntensity(10.f);
        }
    }
    else if (Kind == TEXT("seal"))
    {
        UStaticMeshComponent* Disc = AddPart(TEXT("Cylinder"), true);
        Disc->SetRelativeScale3D(FVector(.35f, .35f, .01f));
        UStaticMeshComponent* Ring = AddPart(TEXT("Cylinder"), false, .4f);
        Ring->SetRelativeScale3D(FVector(.6f, .6f, .005f));
        UStaticMeshComponent* Blast = AddPart(TEXT("Sphere"), false, .0f);
        Blast->SetRelativeScale3D(FVector(.01f));
        Light->SetIntensity(6.f);
    }
    else if (Kind == TEXT("beam"))
    {
        UStaticMeshComponent* Rope = AddPart(TEXT("Cylinder"), false, .9f);
        Rope->SetRelativeScale3D(FVector(.025f, .025f, .01f));
    }
    else if (Kind == TEXT("slam"))
    {
        if (AActor* Hero = Follow) VortexHitFx::GroundSlam(Hero, Radius, 0.f);
        AddSystem(TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave"), FVector(0, 0, 5), FVector(Radius / 300.f));
    }
    else if (Kind == TEXT("punch"))
    {
        // soco: um anel de ar curto na frente do punho (sem arco de lâmina)
        UStaticMeshComponent* Ring = AddPart(TEXT("Cylinder"), false, .5f);
        Ring->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
        Ring->SetRelativeLocation(FVector(Length, 0.f, 0.f));
        Ring->SetRelativeScale3D(FVector(.2f, .2f, .01f));
        UStaticMeshComponent* Puff = AddPart(TEXT("Sphere"), false, .35f);
        Puff->SetRelativeLocation(FVector(Length, 0.f, 0.f));
        Puff->SetRelativeScale3D(FVector(.1f));
    }
    else if (Kind == TEXT("thrust"))
    {
        // estocada: um traço reto que estica para a frente e some
        UStaticMeshComponent* Line = AddPart(TEXT("Cylinder"), false, .55f);
        Line->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
        Line->SetRelativeScale3D(FVector(.04f, .04f, .01f));
        UStaticMeshComponent* Tip = AddPart(TEXT("Cone"), false, .5f);
        Tip->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
        Tip->SetRelativeScale3D(FVector(.01f));
    }
    else if (Kind == TEXT("sweep"))
    {
        // giro cortante: lâminas de luz em crescente (fino na ponta da frente, rastro largo atrás),
        // na altura da arma, girando em volta do herói
        Blades = NewObject<UProceduralMeshComponent>(this);
        Blades->SetupAttachment(Root);
        Blades->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Blades->SetCastShadow(false);
        Blades->RegisterComponent();
        TArray<FVector> V;
        TArray<int32> T;
        TArray<FVector> N;
        TArray<FVector2D> UV;
        const int32 BladeCount = Count == 1 ? 1 : 2, Seg = 24, Rows = 3;
        const float R = FMath::Clamp(Radius, 150.f, 420.f), W = R * .42f;
        for (int32 b = 0; b < BladeCount; ++b)
        {
            const int32 Base = V.Num();
            for (int32 i = 0; i <= Seg; ++i)
            {
                const float U = float(i) / Seg;
                const float Theta = FMath::DegreesToRadians(b * 180.f - U * 150.f);
                const float Taper = FMath::Sin(FMath::Min(1.f, U * 1.25f) * UE_HALF_PI) * (1.f - .4f * U);
                for (int32 j = 0; j <= Rows; ++j)
                {
                    const float Vv = float(j) / Rows;
                    const float Rr = R - W * Taper * Vv;
                    V.Add(FVector(FMath::Cos(Theta) * Rr, FMath::Sin(Theta) * Rr, (j % 2) * 2.f));
                    N.Add(FVector::UpVector);
                    UV.Add(FVector2D(1.f - U, Vv));
                }
            }
            for (int32 i = 0; i < Seg; ++i)
                for (int32 j = 0; j < Rows; ++j)
                {
                    const int32 A = Base + i * (Rows + 1) + j, B = A + Rows + 1;
                    T.Append({ A, B, A + 1, A + 1, B, B + 1, A, A + 1, B, A + 1, B + 1, B });
                }
        }
        Blades->CreateMeshSection(0, V, T, N, UV, {}, {}, false);
        if (UMaterialInterface* Arc = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SlashArc.M_SlashArc")))
        {
            BladeLook = UMaterialInstanceDynamic::Create(Arc, this);
            BladeLook->SetScalarParameterValue(TEXT("Seed"), FMath::FRand());
            BladeLook->SetScalarParameterValue(TEXT("Glow"), 4.f);
            BladeLook->SetScalarParameterValue(TEXT("Sharp"), 5.f);
            BladeLook->SetVectorParameterValue(TEXT("Color"), Color);
            Blades->SetMaterial(0, BladeLook);
        }
        Blades->SetVisibility(false);
        Light->SetIntensity(10.f);
    }
    else if (Kind == TEXT("rain"))
    {
        UStaticMeshComponent* Mark = AddPart(TEXT("Cylinder"), false, .2f);
        Mark->SetRelativeScale3D(FVector(Radius / 50.f, Radius / 50.f, .01f));
    }
}

void AVortexWeaponFx::TickLook(float DeltaSeconds)
{
    const float T = Life > 0.f ? FMath::Clamp(Age / Life, 0.f, 1.f) : 1.f;
    const float Fade = FMath::Clamp((Life - Age) / .4f, 0.f, 1.f);
    auto SetOpacity = [&](int32 I, float O) { if (Glows.IsValidIndex(I) && Glows[I]) Glows[I]->SetScalarParameterValue(TEXT("Opacity"), O); };
    if (Kind == TEXT("aura"))
    {
        if (Follow) SetActorLocation(Follow->GetActorLocation());
        if (Parts.Num() >= 2)
        {
            Parts[0]->AddLocalRotation(FRotator(0.f, 40.f * DeltaSeconds, 0.f));
            SetOpacity(0, .18f + .1f * FMath::Sin(Age * 6.f));
            Parts[1]->SetRelativeScale3D(FVector(1.6f + .4f * FMath::Sin(Age * 4.f), 1.6f + .4f * FMath::Sin(Age * 4.f), .015f));
        }
    }
    else if (Kind == TEXT("orbit"))
    {
        if (Follow) SetActorLocation(Follow->GetActorLocation() + FVector(0.f, 0.f, 35.f));
        for (int32 i = 0; i < Parts.Num(); ++i)
        {
            const float A = Age * 2.6f + i * PI;
            Parts[i]->SetRelativeLocation(FVector(FMath::Cos(A) * 80.f, FMath::Sin(A) * 80.f, 20.f + FMath::Sin(Age * 3.f + i) * 10.f));
        }
        Light->SetIntensity(8.f * Fade);
    }
    else if (Kind == TEXT("tornado"))
    {
        const float Grow = FMath::Clamp(Age / .35f, 0.f, 1.f);
        if (Parts.Num())
        {
            Parts[0]->SetRelativeScale3D(FVector(Radius / 50.f * Grow, Radius / 50.f * Grow, .01f));
            Parts[0]->AddLocalRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));
            SetOpacity(0, .45f * Fade);
        }
        if (Age > .3f && Spinner && Spinner->GetNumChildrenComponents() == 0)
        {
            // 12 chamas numa espiral de 3 m: estreita embaixo, larga em cima
            UNiagaraSystem* Flame = VortexHitFx::System(ElementSystem(Element));
            for (int32 i = 0; Flame && i < 12; ++i)
            {
                const float H = i / 11.f;
                const float A = i * 1.1f;
                const float R = FMath::Lerp(Radius * .15f, Radius * .7f, H);
                Systems.Add(UNiagaraFunctionLibrary::SpawnSystemAttached(Flame, Spinner, NAME_None, FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 20.f + H * 280.f),
                    FRotator::ZeroRotator, FVector(FMath::Lerp(.6f, 1.2f, H) * (IsFire(Element) ? 1.f : .55f)), EAttachLocation::KeepRelativeOffset, false, ENCPoolMethod::None));
            }
            VortexAudio::Play(this, IsFire(Element) ? TEXT("wfx_tornado") : Element == TEXT("lightning") ? TEXT("lightning") : TEXT("wfx_vines"), GetActorLocation(), .9f);
        }
        if (Spinner) Spinner->AddLocalRotation(FRotator(0.f, 420.f * DeltaSeconds, 0.f));
        Light->SetIntensity(40.f * Fade * FMath::Clamp(Age / .4f, 0.f, 1.f));
        if (Age > Life - .3f) for (UNiagaraComponent* N : Systems) if (N) N->Deactivate();
    }
    else if (Kind == TEXT("firewall"))
    {
        SetOpacity(0, .2f * Fade);
        Light->SetIntensity(30.f * Fade);
        if (Age > Life - .5f) for (UNiagaraComponent* N : Systems) if (N) N->Deactivate();
    }
    else if (Kind == TEXT("vines"))
    {
        const ACharacter* Body = Cast<ACharacter>(Follow);
        const float R = Body ? Body->GetCapsuleComponent()->GetScaledCapsuleRadius() + 12.f : 50.f;
        const float H = Body ? Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f : 140.f;
        if (Body) SetActorLocation(Body->GetActorLocation() - FVector(0.f, 0.f, H * .5f));
        const float Grow = FMath::Clamp(Age / .35f, 0.f, 1.f) * Fade;
        for (int32 i = 0; i < Parts.Num(); ++i)
        {
            const float A = i * (2.f * PI / Parts.Num()) + Age * .3f;
            Parts[i]->SetRelativeLocation(FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, H * .5f * Grow));
            Parts[i]->SetRelativeRotation(FRotator(0.f, FMath::RadiansToDegrees(A), 18.f * ((i % 2) ? 1.f : -1.f)));
            Parts[i]->SetRelativeScale3D(FVector(.07f, .07f, H / 100.f * Grow));
        }
    }
    else if (Kind == TEXT("lob") || Kind == TEXT("potion"))
    {
        const float Flight = FMath::Max(.1f, Delay);
        // mirou num inimigo: o arco termina nos pés dele, onde ele estiver
        if (Age < Flight && Homing)
            if (const ACharacter* Body = Cast<ACharacter>(Homing))
                Target = Body->GetActorLocation() - FVector(0.f, 0.f, Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 10.f);
        if (Age < Flight)
        {
            const float Apex = 80.f + .18f * FVector::Dist2D(Start, Target);
            SetActorLocation(Arc(Start, Target, Age / Flight, Apex));
            if (Parts.Num()) Parts[0]->AddLocalRotation(FRotator(-720.f * DeltaSeconds, 0.f, 0.f));
        }
        else if (!bDone)
        {
            // estilhaça: cacos, respingo e a poça (ou a névoa da poção)
            bDone = true;
            SetActorLocation(Target);
            if (Parts.Num()) Parts[0]->SetVisibility(false);
            VortexHitFx::Splash(GetWorld(), FVector(Target) + FVector(0, 0, 20), Color, 18);
            VortexHitFx::Splash(GetWorld(), FVector(Target) + FVector(0, 0, 20), FLinearColor(.85f, .95f, 1.f), 8);
            UStaticMeshComponent* Pool = AddPart(TEXT("Cylinder"), false, .45f);
            Pool->SetRelativeScale3D(FVector(Radius / 50.f, Radius / 50.f, .01f));
            Pool->SetRelativeLocation(FVector(0, 0, 3));
            AddSystem(Kind == TEXT("potion") ? TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Healing.NS_Free_Spells_Buff_Healing")
                : TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Explosion.NS_Free_Spells_Explosion"), FVector::ZeroVector, FVector(Radius / 250.f));
            VortexAudio::Play(this, TEXT("wfx_glass_break"), Target, .9f, .08f);
            VortexAudio::Play(this, Kind == TEXT("potion") ? TEXT("wfx_potion") : TEXT("wfx_acid"), Target, .8f);
            Light->SetIntensity(15.f);
        }
        else if (Parts.Num() > 1)
        {
            SetOpacity(Parts.Num() - 1, .45f * Fade);
            Light->SetIntensity(15.f * Fade);
        }
    }
    else if (Kind == TEXT("holytrail"))
    {
        const float Reach = FMath::Clamp(Age / .35f, 0.f, 1.f);
        for (int32 i = 0; i < Parts.Num(); ++i)
        {
            const float On = FMath::Clamp(Reach * Parts.Num() - i, 0.f, 1.f) * Fade;
            Parts[i]->SetRelativeScale3D(FVector(.9f * On, .3f * On, .05f + .6f * On * (1.f - T)));
            SetOpacity(i, .7f * On);
        }
        Light->SetIntensity(25.f * Fade);
    }
    else if (Kind == TEXT("boomerang"))
    {
        bool bBack = false;
        SetActorLocation(BoomerangAt(Age, bBack));
        if (Count == 1) SetActorRotation(FRotator(0.f, Age * 1080.f, 0.f));   // disco girando deitado
        else if (Parts.Num()) Parts[0]->AddLocalRotation(FRotator(-1080.f * DeltaSeconds, 0.f, 0.f));   // machado: cambalhota
    }
    else if (Kind == TEXT("seal"))
    {
        if (Follow && Follow != GetOwner()) SetActorLocation(Follow->GetActorLocation());
        const float Pulse = FMath::Sin(Age * FMath::Lerp(6.f, 30.f, FMath::Clamp(Age / FMath::Max(.1f, Delay), 0.f, 1.f)));
        if (Parts.Num() >= 3)
        {
            Parts[1]->AddLocalRotation(FRotator(0.f, 200.f * DeltaSeconds, 0.f));
            if (Age >= Delay)
            {
                const float B = FMath::Clamp((Age - Delay) / .3f, 0.f, 1.f);
                Parts[0]->SetVisibility(false);
                Parts[1]->SetVisibility(false);
                Parts[2]->SetRelativeScale3D(FVector(Radius / 50.f * B));
                SetOpacity(2, .5f * (1.f - B));
                Light->SetIntensity(40.f * (1.f - B));
                if (!bDone) { bDone = true; VortexAudio::Play(this, TEXT("wfx_rune_burst"), GetActorLocation(), 1.f); }
            }
            else Light->SetIntensity(6.f + 4.f * Pulse);
        }
    }
    else if (Kind == TEXT("beam"))
    {
        const FVector From = Follow ? Follow->GetActorLocation() + FVector(0.f, 0.f, 30.f) : GetActorLocation();
        const FVector To = Target;
        const float Out2 = FMath::Clamp(Age / .12f, 0.f, 1.f) * Fade;
        const FVector End = FMath::Lerp(From, To, Out2);
        SetActorLocationAndRotation(From, (End - From).Rotation());
        if (Parts.Num())
        {
            const float L = FVector::Dist(From, End);
            Parts[0]->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
            Parts[0]->SetRelativeLocation(FVector(L * .5f, 0.f, 0.f));
            Parts[0]->SetRelativeScale3D(FVector(.025f, .025f, L / 100.f));
        }
    }
    else if (Kind == TEXT("punch") && Parts.Num() >= 2)
    {
        const float P = FMath::Clamp(Age / Life, 0.f, 1.f);
        Parts[0]->SetRelativeScale3D(FVector(.2f + 1.1f * P, .2f + 1.1f * P, .01f));
        Parts[1]->SetRelativeScale3D(FVector(.1f + .45f * P));
        SetOpacity(0, .55f * (1.f - P));
        SetOpacity(1, .35f * (1.f - P));
    }
    else if (Kind == TEXT("thrust") && Parts.Num() >= 2)
    {
        const float P = FMath::Clamp(Age / Life, 0.f, 1.f);
        const float Out2 = FMath::Min(1.f, P * 2.5f);
        Parts[0]->SetRelativeLocation(FVector(Length * Out2 * .5f, 0.f, 0.f));
        Parts[0]->SetRelativeScale3D(FVector(.05f * (1.f - P * .5f), .05f * (1.f - P * .5f), Length * Out2 / 100.f));
        Parts[1]->SetRelativeLocation(FVector(Length * Out2, 0.f, 0.f));
        Parts[1]->SetRelativeScale3D(FVector(.12f, .12f, .25f) * (1.f - P));
        SetOpacity(0, .55f * (1.f - P));
        SetOpacity(1, .5f * (1.f - P));
    }
    else if (Kind == TEXT("sweep"))
    {
        // as lâminas giram com o herói (na velocidade do giro do corpo), aparecem e somem rápido
        if (Follow) SetActorLocation(Follow->GetActorLocation() + FVector(0.f, 0.f, 18.f));
        const float On = FMath::Clamp((Age - Delay) / .08f, 0.f, 1.f) * FMath::Clamp((Life - Age) / .15f, 0.f, 1.f);
        if (Blades) Blades->SetVisibility(On > 0.f);
        const float Yaw = (Follow ? Follow->GetActorRotation().Yaw : 0.f) - Speed * FMath::Max(0.f, Age - Delay);
        SetActorRotation(FRotator(0.f, Yaw, 0.f));
        if (BladeLook) BladeLook->SetScalarParameterValue(TEXT("Fade"), On);
        Light->SetIntensity(10.f * On);
    }
    else if (Kind == TEXT("rain"))
    {
        if (Parts.Num()) SetOpacity(0, .2f * Fade);
        // flechas caindo (cada máquina sorteia as suas; o dano é do servidor)
        if (Age >= LookTick && Age < Life - .3f)
        {
            LookTick = Age + Life / 30.f;
            // flechas caindo do céu, um pouco inclinadas, que ficam cravadas no chão
            const FVector Land = RainLanding();
            const FVector Slant = FVector(FMath::FRandRange(-.12f, .12f), FMath::FRandRange(-.12f, .12f), -1.f).GetSafeNormal();
            FTransform Where(Slant.Rotation(), Land - Slant * 1100.f);
            if (AVortexProjectile* Arrow = GetWorld()->SpawnActorDeferred<AVortexProjectile>(AVortexProjectile::StaticClass(), Where, nullptr, nullptr,
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
            {
                Arrow->Mode = TEXT("arrow");
                // 02/11: 1.900 cm/s de 14 m (0,75 s no ar) era lento: o bicho saía do lugar antes
                Arrow->Speed = 4200.f;
                Arrow->Range = 1300.f;
                Arrow->Size = 1.25f;
                Arrow->Damage = 0.f;
                Arrow->SetReplicates(false);
                Arrow->FinishSpawning(Where);
            }
            if (Count++ % 4 == 0) VortexAudio::Play(this, TEXT("wfx_arrow_whistle"), Land + FVector(0.f, 0.f, 300.f), .7f, .12f);
        }
    }
}

// ---- servidor ----------------------------------------------------------------------------------

UVortexCombatComponent* AVortexWeaponFx::SourceCombat() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<UVortexCombatComponent>() : nullptr;
}

FVector AVortexWeaponFx::RainLanding() const
{
    // 3 de cada 4 flechas descem num inimigo dentro do círculo (a até 50 cm dele); as outras, num ponto qualquer
    const TArray<AActor*> Near = Enemies(FVector(Target), Radius);
    if (Near.Num() && FMath::FRand() < .75f)
    {
        const FVector2D J = FMath::RandPointInCircle(50.f);
        const FVector At = Near[FMath::RandHelper(Near.Num())]->GetActorLocation();
        return FVector(At.X + J.X, At.Y + J.Y, Target.Z);
    }
    const FVector2D R = FMath::RandPointInCircle(Radius * .85f);
    return FVector(Target) + FVector(R.X, R.Y, 0.f);
}

TArray<AActor*> AVortexWeaponFx::Enemies(const FVector& Center, float RadiusCm) const
{
    TArray<AActor*> Out2;
    UVortexCombatComponent* Combat = SourceCombat();
    if (!Combat) return Out2;
    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams Bodies;
    Bodies.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexWeaponFx), false, this);
    if (GetOwner()) Query.AddIgnoredActor(GetOwner());
    GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, Bodies, FCollisionShape::MakeSphere(RadiusCm), Query);
    for (const FOverlapResult& O : Overlaps)
        if (AActor* A = O.GetActor(); A && Combat->CanHarm(A)) Out2.AddUnique(A);
    return Out2;
}

bool AVortexWeaponFx::HitOne(AActor* Victim, float Amount)
{
    UVortexCombatComponent* Combat = SourceCombat();
    if (!Victim || !Combat || !Combat->CanHarm(Victim)) return false;
    // 07/11: o golpe da arma multiplica o dano do herói (1 + bônus) x o dano do golpe, como os outros jogos (valor do movimento) (os desenhos que ferem são todos das armas)
    Combat->ApplyHit(Victim, 1.f, EVortexDamageKind(VortexSwords::PowerKind(GetOwner(), uint8(DamageKind))), Stun, Amount);
    if (Expose > 0.f)
        if (UVortexCombatComponent* Their = Victim->FindComponentByClass<UVortexCombatComponent>()) Their->ApplyExpose(Expose, ExposeTime);
    return true;
}

void AVortexWeaponFx::HitArea(const FVector& Center, float RadiusCm, float Amount, bool bOnce)
{
    for (AActor* A : Enemies(Center, RadiusCm))
    {
        if (bOnce && Hits.Contains(A)) continue;
        if (HitOne(A, Amount)) Hits.Add(A);
    }
}

FVector AVortexWeaponFx::BoomerangAt(float Time, bool& bBack) const
{
    const float OutTime = Length / FMath::Max(100.f, Speed);
    if (Time <= OutTime)
    {
        bBack = false;
        // sai rápido e freia no fim (curva de bumerangue)
        const float X = Time / OutTime;
        return Start + Out * Length * (1.f - (1.f - X) * (1.f - X)) + FVector(0.f, 0.f, FMath::Sin(X * PI) * 30.f);
    }
    bBack = true;
    const FVector Far = Start + Out * Length;
    const FVector Home = Follow ? Follow->GetActorLocation() + FVector(0.f, 0.f, 20.f) : Start;
    const float Back = FMath::Clamp((Time - OutTime) / OutTime, 0.f, 1.f);
    return FMath::Lerp(Far, Home, Back * Back * (3.f - 2.f * Back));
}

void AVortexWeaponFx::TickServer(float DeltaSeconds)
{
    if (Kind == TEXT("orbit"))
    {
        if (Age >= NextTick)
        {
            NextTick = Age + Every;
            const FVector Hero = Follow ? Follow->GetActorLocation() : GetActorLocation();
            TArray<AActor*> Near = Enemies(Hero, Length);
            Near.Sort([&Hero](const AActor& A, const AActor& B) { return FVector::DistSquared(A.GetActorLocation(), Hero) < FVector::DistSquared(B.GetActorLocation(), Hero); });
            for (int32 i = 0; i < 2 && Near.Num(); ++i)
            {
                AActor* Foe = Near[FMath::Min(i, Near.Num() - 1)];
                const float A = Age * 2.6f + i * PI;
                const FVector From = Hero + FVector(FMath::Cos(A) * 80.f, FMath::Sin(A) * 80.f, 55.f);
                const FVector Dir = (Foe->GetActorLocation() - From).GetSafeNormal();
                const FTransform Where(Dir.Rotation(), From);
                if (AVortexProjectile* Shot = GetWorld()->SpawnActorDeferred<AVortexProjectile>(AVortexProjectile::StaticClass(), Where, GetOwner(),
                    Cast<APawn>(GetOwner()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
                {
                    Shot->Mode = TEXT("bolt");
                    Shot->Element = TEXT("holy");
                    Shot->Damage = Damage;
                    Shot->Speed = 2600.f;
                    Shot->Range = Length + 200.f;
                    Shot->Size = .8f;
                    Shot->FinishSpawning(Where);
                }
            }
        }
    }
    else if (Kind == TEXT("tornado"))
    {
        if (Age >= .35f && Age >= NextTick) { NextTick = Age + Every; HitArea(GetActorLocation(), Radius, Damage, false); }
    }
    else if (Kind == TEXT("firewall"))
    {
        if (Age >= NextTick)
        {
            NextTick = Age + Every;
            for (AActor* A : Enemies(GetActorLocation(), Length * .5f + 80.f))
            {
                const FVector L = GetActorTransform().InverseTransformPosition(A->GetActorLocation());
                if (FMath::Abs(L.X) < 110.f && FMath::Abs(L.Y) < Length * .5f + 40.f) HitOne(A, Damage);
            }
        }
    }
    else if (Kind == TEXT("lob") || Kind == TEXT("potion"))
    {
        if (Age < Delay && Homing)
            if (const ACharacter* Body = Cast<ACharacter>(Homing))
                Target = Body->GetActorLocation() - FVector(0.f, 0.f, Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 10.f);
        if (Age >= Delay && !bReturning)
        {
            bReturning = true;   // (usado aqui como "já estourou")
            if (Kind == TEXT("lob")) HitArea(Target, Radius, Damage, true);
            else
            {
                // poção: fortalece todos no raio, heróis e criaturas
                TArray<FOverlapResult> Overlaps;
                FCollisionObjectQueryParams Bodies;
                Bodies.AddObjectTypesToQuery(ECC_Pawn);
                GetWorld()->OverlapMultiByObjectType(Overlaps, Target, FQuat::Identity, Bodies, FCollisionShape::MakeSphere(Radius));
                FVortexClassSkill Buff;
                Buff.BuffDamage = Damage;
                Buff.BuffSpeed = Damage;
                Buff.BuffDuration = Every;
                Buff.Element = TEXT("poison");
                TSet<AActor*> Done;
                for (const FOverlapResult& O : Overlaps)
                {
                    AActor* A = O.GetActor();
                    if (!A || Done.Contains(A)) continue;
                    Done.Add(A);
                    if (UVortexClassSkills* Skills = A->FindComponentByClass<UVortexClassSkills>()) Skills->GiveBuff(Buff, 1.f);
                    else if (UVortexCombatComponent* C = A->FindComponentByClass<UVortexCombatComponent>()) C->ApplyEmpower(Damage, Every);
                }
            }
        }
    }
    else if (Kind == TEXT("holytrail"))
    {
        if (!bReturning)
        {
            bReturning = true;
            for (AActor* A : Enemies(GetActorLocation() + Out * Length * .5f, Length * .5f + 120.f))
            {
                const FVector L = GetActorTransform().InverseTransformPosition(A->GetActorLocation());
                if (L.X < -60.f || L.X > Length + 60.f || FMath::Abs(L.Y) > 140.f) continue;
                // forte demais (5 níveis acima, ou chefe): o dano entra, o atordoamento não
                const IVortexCreature* Creature = Cast<IVortexCreature>(A);
                const AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
                const int32 HeroLevel = Hero && Hero->GetSheet() ? Hero->GetSheet()->GetLevel() : 1;
                const bool bTooStrong = Creature && (Creature->IsBoss() || Creature->GetCreatureLevel() >= HeroLevel + int32(Count));
                const float Was = Stun;
                if (bTooStrong) Stun = 0.f;
                HitOne(A, Damage);
                if (!bTooStrong && Was > 0.f)
                    if (UVortexCombatComponent* C = A->FindComponentByClass<UVortexCombatComponent>()) C->Stun(Was);
                Stun = Was;
            }
        }
    }
    else if (Kind == TEXT("boomerang"))
    {
        bool bBack = false;
        const FVector Was = GetActorLocation();
        const FVector Now = BoomerangAt(Age, bBack);
        if (bBack && !bReturning) { bReturning = true; Hits.Reset(); }   // o mesmo disco pode acertar de novo na volta
        HitArea(Now, Radius, Damage, true);
        const float OutTime = Length / FMath::Max(100.f, Speed);
        if (bBack && (Age >= OutTime * 2.f || (Follow && FVector::Dist(Now, Follow->GetActorLocation()) < 90.f)))
        {
            if (OnReturn) OnReturn();
            OnReturn = nullptr;
            Destroy();
        }
        (void)Was;
    }
    else if (Kind == TEXT("rain"))
    {
        if (Age >= NextTick && Age < Life)
        {
            NextTick = Age + Life / FMath::Max(1, Count);
            // 02/11: cada flecha cai num ponto do círculo, mas puxa para os inimigos (RainLanding), e acerta 1,5 m em
            // volta. Antes era um ponto qualquer com 1,1 m: um bicho no meio levava 1 ou 2 das 20
            for (AActor* A : Enemies(RainLanding(), 150.f)) HitOne(A, Damage);
        }
    }
    else if (Kind == TEXT("seal"))
    {
        if (Age >= Delay && !bReturning)
        {
            bReturning = true;
            HitArea(GetActorLocation(), Radius, Damage, true);
        }
    }
}

void AVortexWeaponFx::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    if (HasAuthority()) TickServer(DeltaSeconds);
    if (IsActorBeingDestroyed()) return;
    if (GetNetMode() != NM_DedicatedServer) TickLook(DeltaSeconds);
    if (HasAuthority() && Age >= Life && Kind != TEXT("boomerang")) Destroy();
}

FVector AVortexWeaponFx::SteerAroundWalls(const ACharacter* Creature, const FVector& Move)
{
    if (!Creature || Walls().IsEmpty()) return Move;
    // pedra e fogo atravessam a parede de fogo
    if (const IVortexCreature* C = Cast<IVortexCreature>(Creature))
    {
        const FName Species = C->GetSpecies();
        if (Species == TEXT("Golem") || Species == TEXT("RuneColossus") || Species.ToString().Contains(TEXT("Fire")) || Species.ToString().Contains(TEXT("Magma"))) return Move;
    }
    FVector Result = Move;
    for (const TWeakObjectPtr<AVortexWeaponFx>& Weak : Walls())
    {
        const AVortexWeaponFx* Wall = Weak.Get();
        if (!Wall) continue;
        const FTransform& T = Wall->GetActorTransform();
        const FVector L = T.InverseTransformPosition(Creature->GetActorLocation());
        const float Half = Wall->Length * .5f + 60.f;
        if (FMath::Abs(L.Y) > Half || FMath::Abs(L.X) > 220.f) continue;
        // o lado em que está; andar para dentro da parede vira andar ao longo dela
        const FVector Normal = T.GetUnitAxis(EAxis::X) * (L.X >= 0.f ? 1.f : -1.f);
        const float Into = FVector::DotProduct(Result, -Normal);
        if (Into > 0.f)
        {
            Result += Normal * Into;
            // para o lado mais perto da ponta da parede
            const FVector Along = T.GetUnitAxis(EAxis::Y) * (L.Y >= 0.f ? 1.f : -1.f);
            Result = (Result + Along * Into).GetSafeNormal2D() * Move.Size();
        }
        if (FMath::Abs(L.X) < 90.f) Result += Normal * Move.Size();   // dentro: sai
        if (UCharacterMovementComponent* Movement = Creature->GetCharacterMovement())
        {
            const float Closing = FVector::DotProduct(Movement->Velocity, -Normal);
            if (Closing > 0.f && FMath::Abs(L.X) < 160.f) Movement->Velocity += Normal * Closing;
        }
    }
    return Result;
}
