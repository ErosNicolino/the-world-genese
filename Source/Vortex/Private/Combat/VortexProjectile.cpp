#include "Combat/VortexProjectile.h"

#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexHitFx.h"
#include "Combat/VortexSwords.h"
#include "Creatures/VortexCreature.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Framework/VortexAudio.h"
#include "GameFramework/GameStateBase.h"
#include "World/VortexBreakable.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
    constexpr float ProjectileBurstSeconds = .3f;

    const TCHAR* TrailFor(FName Element)
    {
        if (Element == TEXT("nature") || Element == TEXT("jade") || Element == TEXT("poison") || Element == TEXT("vine")) return TEXT("NS_ArrowTrail_Nature");
        if (Element == TEXT("rune") || Element == TEXT("music")) return TEXT("NS_ArrowTrail_Magic_2");
        if (Element == TEXT("holy")) return TEXT("NS_ArrowTrail_Holy");
        if (Element == TEXT("arcane") || Element == TEXT("shadow")) return TEXT("NS_ArrowTrail_Magic");
        if (Element == TEXT("fire")) return TEXT("NS_ArrowTrail_Fire");
        if (Element == TEXT("ice")) return TEXT("NS_ArrowTrail_Ice");
        if (Element == TEXT("water")) return TEXT("NS_ArrowTrail_Water");
        return TEXT("NS_ArrowTrail_Basic");
    }
}

FLinearColor AVortexProjectile::ElementColor(FName Element)
{
    if (Element == TEXT("nature") || Element == TEXT("vine")) return FLinearColor(.35f, 1.f, .3f);
    if (Element == TEXT("rune")) return FLinearColor(.5f, .45f, 1.f);
    if (Element == TEXT("music")) return FLinearColor(1.f, .7f, .95f);
    if (Element == TEXT("jade")) return FLinearColor(.2f, 1.f, .55f);
    if (Element == TEXT("holy")) return FLinearColor(1.f, .8f, .35f);
    if (Element == TEXT("arcane")) return FLinearColor(.7f, .4f, 1.f);
    if (Element == TEXT("shadow")) return FLinearColor(.55f, .3f, .9f);
    if (Element == TEXT("fire")) return FLinearColor(1.f, .45f, .15f);
    if (Element == TEXT("ice") || Element == TEXT("water")) return FLinearColor(.5f, .85f, 1.f);
    if (Element == TEXT("poison")) return FLinearColor(.55f, 1.f, .2f);
    return FLinearColor(.85f, .9f, 1.f);   // aço
}

FVector AVortexProjectile::BallisticDirection(const FVector& From, const FVector& Target, float Speed, const FVector& Fallback, float Drop)
{
    // tiro de ângulo baixo: tan = (v² - sqrt(v⁴ - g(g d² + 2 h v²))) / (g d)
    const FVector Flat = FVector(Target.X - From.X, Target.Y - From.Y, 0.f);
    const float D = Flat.Size();
    const float H = Target.Z - From.Z;
    const float G = -ArrowGravity * FMath::Max(.05f, Drop);
    if (D < 1.f) return Fallback;
    const float V2 = Speed * Speed;
    const float Root = V2 * V2 - G * (G * D * D + 2.f * H * V2);
    if (Root < 0.f) return Fallback;
    const float Angle = FMath::Atan((V2 - FMath::Sqrt(Root)) / (G * D));
    return (Flat / D * FMath::Cos(Angle) + FVector(0.f, 0.f, FMath::Sin(Angle))).GetSafeNormal();
}

AVortexProjectile::AVortexProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicatingMovement(false);
    // 03/11: os tiros vão na frente dos outros atores pela rede (com muitos bichos por perto a flecha chegava tarde)
    NetPriority = 3.f;
    SetNetUpdateFrequency(60.f);
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
    Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
    Light->SetMobility(EComponentMobility::Movable);   // luz criada em jogo: nada a "construir"
    Light->SetupAttachment(Root);
    Light->SetIntensityUnits(ELightUnits::Candelas);
    Light->SetIntensity(6.f);
    Light->SetAttenuationRadius(300.f);
    Light->SetCastShadows(false);
}

void AVortexProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AVortexProjectile, Mode, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexProjectile, Element, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexProjectile, Speed, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexProjectile, Range, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexProjectile, Radius, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexProjectile, Size, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexProjectile, bPierce, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexProjectile, Drop, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexProjectile, LaunchFrom, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexProjectile, LaunchDir, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexProjectile, LaunchTime, COND_InitialOnly);
    DOREPLIFETIME(AVortexProjectile, BurstAt);
    DOREPLIFETIME(AVortexProjectile, bStuck);
    DOREPLIFETIME(AVortexProjectile, StuckAt);
    DOREPLIFETIME(AVortexProjectile, StuckDir);
    DOREPLIFETIME(AVortexProjectile, StuckOn);
    DOREPLIFETIME(AVortexProjectile, bBurst);
}

void AVortexProjectile::BeginPlay()
{
    Super::BeginPlay();
    const AGameStateBase* State = GetWorld()->GetGameState();
    const float ServerNow = State ? float(State->GetServerWorldTimeSeconds()) : GetWorld()->GetTimeSeconds();
    if (HasAuthority())
    {
        LaunchFrom = GetActorLocation();
        LaunchDir = GetActorForwardVector().GetSafeNormal();
        LaunchTime = ServerNow;
    }
    Start = LaunchFrom.IsZero() ? GetActorLocation() : FVector(LaunchFrom);
    Direction = LaunchDir.IsNearlyZero() ? GetActorForwardVector().GetSafeNormal() : FVector(LaunchDir).GetSafeNormal();
    // 03/11: atirou em primeira pessoa (neste computador): o desenho da flecha sai do arco da tela
    if (const APawn* Shooter = Cast<APawn>(GetOwner()); Shooter && Shooter->IsLocallyControlled() && IsPhysical() && GetNetMode() != NM_DedicatedServer)
        for (UActorComponent* Part : Shooter->GetComponents())
            if (const USceneComponent* View = Cast<USceneComponent>(Part); View && View->GetFName() == TEXT("ViewRight") && View->IsVisible())
            {
                ViewOffset = View->GetComponentLocation() - Start;
                if (ViewOffset.Size() > 300.f) ViewOffset = FVector::ZeroVector;
                break;
            }
    if (!HasAuthority() && LaunchTime > 0.f)
    {
        // 03/11: chegou atrasado pela rede: até 0,15 s de atraso o voo começa do arco (todos veem a flecha sair); mais que
        // isso, ela pula para onde já está
        Age = FMath::Clamp(ServerNow - LaunchTime - .15f, 0.f, 3.f);
        SetActorLocation(IsPhysical() ? ArrowAt(Age) : Start + Direction * FMath::Min(Range, Speed * Age));
    }
    const FLinearColor Color = ElementColor(Element);
    if (HasAuthority()) SetLifeSpan(IsPhysical() ? 12.f : Range / FMath::Max(100.f, Speed) + 2.f);
    if (GetNetMode() != NM_DedicatedServer && IsPhysical())
    {
        // flecha de verdade (a ponta na origem do ator, para a frente): haste, ponta e penas
        Light->SetVisibility(false);
        const float Length = Mode == TEXT("bolt") ? .55f : .78f;
        auto Part = [this](const TCHAR* Shape, const TCHAR* Material, const FVector& At, const FRotator& Rot, const FVector& Scale)
        {
            UStaticMeshComponent* P = NewObject<UStaticMeshComponent>(this);
            P->SetupAttachment(Root);
            P->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Shape, Shape)));
            P->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Stadium/MI_Stadium_%s.MI_Stadium_%s"), Material, Material)));
            P->SetRelativeLocationAndRotation(At, Rot);
            P->SetRelativeScale3D(Scale * Size);
            P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            P->SetCastShadow(true);
            P->bAffectDistanceFieldLighting = false;
            P->RegisterComponent();
        };
        // a flecha nova (Tools/Blender/FitNewWeapons.py: 80 cm, a ponta na origem, para +X); o virote, das formas básicas
        // 07/11: cada arco tem a sua flecha (Tools/Blender/MakePixelBows.py)
        UStaticMesh* Own = Mode == TEXT("bolt") ? nullptr : VortexSwords::ArrowMesh(GetOwner());
        UStaticMesh* Arrow = Mode == TEXT("bolt") ? nullptr : Own ? Own : LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Vortex/Weapons/Models/W_flecha/StaticMeshes/W_flecha.W_flecha"));
        if (Arrow)
        {
            UStaticMeshComponent* P = NewObject<UStaticMeshComponent>(this);
            P->SetupAttachment(Root);
            P->SetStaticMesh(Arrow);
            P->SetRelativeScale3D(FVector(Size));
            P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            P->SetCastShadow(true);
            P->bAffectDistanceFieldLighting = false;
            P->RegisterComponent();
        }
        else
        {
            Part(TEXT("Cylinder"), TEXT("wood"), FVector(-50.f * Length - 6.f, 0.f, 0.f) * Size, FRotator(-90.f, 0.f, 0.f), FVector(.018f, .018f, Length));
            Part(TEXT("Cone"), TEXT("basalt"), FVector(-4.f, 0.f, 0.f) * Size, FRotator(-90.f, 0.f, 0.f), FVector(.045f, .045f, .09f));
            for (const float Roll : { 0.f, 90.f })
            {
                Part(TEXT("Cube"), Mode == TEXT("bolt") ? TEXT("bronze") : TEXT("marble"), FVector(-100.f * Length - 1.f, 0.f, 0.f) * Size,
                    FRotator(0.f, 0.f, Roll), FVector(.13f, .004f, .045f));
            }
        }
    }
    else if (GetNetMode() != NM_DedicatedServer)
    {
        Light->SetLightColor(Color);
        // núcleo: flecha = lasca fina e comprida; dardo e esfera = bola brilhante
        const bool bArrow = Mode == TEXT("arrow") || Element == TEXT("vine");
        UStaticMesh* Shape = LoadObject<UStaticMesh>(nullptr, bArrow ? TEXT("/Engine/BasicShapes/Cylinder.Cylinder") : TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Weapons/M_ProjectileGlow.M_ProjectileGlow"));
        if (Shape && Glow && Mode != TEXT("burst"))
        {
            Core = NewObject<UStaticMeshComponent>(this);
            Core->SetupAttachment(Root);
            Core->SetStaticMesh(Shape);
            Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Core->SetCastShadow(false);
            Core->bAffectDistanceFieldLighting = false;
            Core->bAffectDynamicIndirectLighting = false;
            Core->SetVisibleInRayTracing(false);
            const float S = Size * (Mode == TEXT("orb") ? 1.8f : 1.f);
            if (bArrow)
            {
                Core->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));   // o cilindro em pé vira deitado, para a frente
                Core->SetRelativeScale3D(FVector(.05f, .05f, .9f) * S);
            }
            else Core->SetRelativeScale3D(FVector(.22f) * S);
            CoreMaterial = UMaterialInstanceDynamic::Create(Glow, this);
            CoreMaterial->SetVectorParameterValue(TEXT("Color"), Color);
            CoreMaterial->SetScalarParameterValue(TEXT("Glow"), 6.f * Size);
            Core->SetMaterial(0, CoreMaterial);
            Core->RegisterComponent();
        }
        // instrumento: notas musicais voando junto; varinha: brilho a mais no caminho
        if (Element == TEXT("music") || Element == TEXT("arcane"))
        {
            const TCHAR* Extra = Element == TEXT("music") ? TEXT("/Game/FreeParticle_SoftTofu/Niagara/NS_Sparkling_Note.NS_Sparkling_Note")
                : TEXT("/Game/FreeParticle_SoftTofu/Niagara/NS_Sparkling_Glow.NS_Sparkling_Glow");
            if (UNiagaraSystem* Sparkle = VortexHitFx::System(Extra))
                UNiagaraFunctionLibrary::SpawnSystemAttached(Sparkle, Root, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
        }
        // 03/11: a bola de fogo (o usuário: "não parece bola de fogo"): o miolo amarelo quente, uma casca laranja maior e
        // chamas de verdade em volta, que ficam para trás no voo; o rastro de fogo mais grosso
        const bool bFireball = Element == TEXT("fire") && Mode == TEXT("orb");
        const FString Path = bFireball ? FString(TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Fire_02.NS_ArrowTrail_Fire_02"))
            : FString::Printf(TEXT("/Game/ArrowTrail/FX/%s.%s"), TrailFor(Element), TrailFor(Element));
        if (UNiagaraSystem* System = Mode == TEXT("burst") ? nullptr : VortexHitFx::System(*Path))
        {
            Trail = UNiagaraFunctionLibrary::SpawnSystemAttached(System, Root, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
                EAttachLocation::KeepRelativeOffset, true);
        }
        if (bFireball)
        {
            if (CoreMaterial)
            {
                CoreMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, .78f, .3f));
                CoreMaterial->SetScalarParameterValue(TEXT("Glow"), 14.f);
            }
            if (Core) Core->SetRelativeScale3D(FVector(.16f) * Size * 1.8f);
            UMaterialInterface* ShellGlow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Weapons/M_ProjectileGlow.M_ProjectileGlow"));
            if (UStaticMesh* Ball = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")); Ball && ShellGlow)
            {
                UStaticMeshComponent* Shell = NewObject<UStaticMeshComponent>(this);
                Shell->SetupAttachment(Root);
                Shell->SetStaticMesh(Ball);
                Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Shell->SetCastShadow(false);
                Shell->SetRelativeScale3D(FVector(.3f) * Size * 1.8f);
                UMaterialInstanceDynamic* Hot = UMaterialInstanceDynamic::Create(ShellGlow, this);
                Hot->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, .22f, .02f));
                Hot->SetScalarParameterValue(TEXT("Glow"), 4.f);
                Shell->SetMaterial(0, Hot);
                Shell->RegisterComponent();
            }
            if (UNiagaraSystem* Fire = VortexHitFx::System(TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Medium.NS_Fire_Medium")))
            {
                Flames = UNiagaraFunctionLibrary::SpawnSystemAttached(Fire, Root, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
                    EAttachLocation::KeepRelativeOffset, true);
                if (Flames) Flames->SetRelativeScale3D(FVector(.32f * Size));
            }
            Light->SetLightColor(FLinearColor(1.f, .45f, .1f));
            Light->SetAttenuationRadius(700.f);
        }
    }
    if (Mode == TEXT("burst") && HasAuthority())
    {
        // explosão no ponto: já nasce lá (quem lança calcula onde), acerta e some
        Explode(GetActorLocation());
        StartBurst(GetActorLocation());
    }
}

void AVortexProjectile::HitTarget(AActor* Target, float Amount)
{
    UVortexCombatComponent* Combat = GetOwner() ? GetOwner()->FindComponentByClass<UVortexCombatComponent>() : nullptr;
    if (!Target || Hits.Contains(Target) || !Combat || !Combat->CanHarm(Target)) return;
    Hits.Add(Target);
    if (Amount <= 0.f) return;   // só o desenho (o tiro do selo rúnico)
    // luz (orbe): +50% em mortos-vivos
    if (Element == TEXT("holy"))
        if (const IVortexCreature* Creature = Cast<IVortexCreature>(Target))
        {
            const FString Species = Creature->GetSpecies().ToString();
            if (Species.Contains(TEXT("Skeleton")) || Species.Contains(TEXT("Zombie")) || Species.Contains(TEXT("Ghost")) || Species.Contains(TEXT("Undead")) || Species.Contains(TEXT("Lich")))
                Amount *= 1.5f;
        }
    // flecha e virote: Destreza; pancada de aço no chão (martelo): Força; o resto (dardos, orbes, explosões): magia
    const EVortexDamageKind Kind = IsPhysical() || (Mode == TEXT("bolt") && Element.IsNone()) ? EVortexDamageKind::Ranged
        : Mode == TEXT("burst") && (Element.IsNone() || Element == TEXT("steel")) ? EVortexDamageKind::Melee : EVortexDamageKind::Magic;
    // 07/11: o golpe da arma multiplica o dano do herói (1 + bônus) x o dano do golpe, como os outros jogos (valor do movimento); o poder conta o dano da arma (a espada e o arco da tabela)
    if (ActorHasTag(TEXT("arma"))) Combat->ApplyHit(Target, 1.f, EVortexDamageKind(VortexSwords::PowerKind(GetOwner(), uint8(Kind))), Stun, Amount);
    else Combat->ApplyHit(Target, Amount, Kind, Stun);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_PROJECTILE_HIT %s modo=%s dano=%.0f distancia=%.0f"), *GetNameSafe(Target), *Mode.ToString(), Amount, Travelled);
}

void AVortexProjectile::Explode(const FVector& Where)
{
    if (Radius <= 0.f) return;
    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams Bodies;
    Bodies.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexProjectileBlast), false, this);
    if (GetOwner()) Query.AddIgnoredActor(GetOwner());
    GetWorld()->OverlapMultiByObjectType(Overlaps, Where, FQuat::Identity, Bodies, FCollisionShape::MakeSphere(Radius), Query);
    for (const FOverlapResult& Overlap : Overlaps) HitTarget(Overlap.GetActor(), Damage);
}

void AVortexProjectile::StartBurst(const FVector& Where)
{
    if (bBurst) return;
    bBurst = true;
    BurstAt = Where;
    BurstAge = 0.f;
    SetActorLocation(Where);
    ForceNetUpdate();
    OnRep_Burst();
}

FVector AVortexProjectile::ArrowAt(float Time) const
{
    return Start + Direction * Speed * Time + FVector(0.f, 0.f, .5f * ArrowGravity * Drop * Time * Time);
}

// Flecha crava: para, aponta para onde ia, fica presa ao que acertou (servidor; vai pela rede)
void AVortexProjectile::Stick(const FVector& Where, const FVector& Dir, AActor* On)
{
    if (bStuck) return;
    bStuck = true;
    // 05/11: barris e caixotes só quebram com a espada (UVortexCombatComponent::StrikeInArc); a flecha crava neles
    StuckDir = Dir.GetSafeNormal();
    // a ponta entra: no bicho, o toque é na borda da cápsula (bem fora do corpo), então ela afunda uns 30 cm (02/11:
    // ficavam penduradas pela ponta); na parede, 15 cm (a flecha tem 80)
    StuckAt = Where + StuckDir * (On && On->IsA<AVortexBreakable>() ? 10.f : On ? 30.f : 15.f);
    StuckOn = On;
    // flechas cravadas somem em 3 s (02/11); no máximo 40 no mundo (as mais antigas somem primeiro)
    SetLifeSpan(3.f);
    static TArray<TWeakObjectPtr<AVortexProjectile>> Stuck;
    Stuck.RemoveAll([](const TWeakObjectPtr<AVortexProjectile>& P) { return !P.IsValid(); });
    Stuck.Add(this);
    while (Stuck.Num() > 40) { if (AVortexProjectile* Old = Stuck[0].Get()) Old->Destroy(); Stuck.RemoveAt(0); }
    ForceNetUpdate();
    OnRep_Stuck();
}

void AVortexProjectile::OnRep_Stuck()
{
    if (!bStuck) return;
    SetActorLocationAndRotation(StuckAt, StuckDir.Rotation());
    if (StuckOn) AttachToActor(StuckOn, FAttachmentTransformRules::KeepWorldTransform);
    if (GetNetMode() != NM_DedicatedServer) VortexAudio::Play(this, StuckOn ? TEXT("creature_hit") : TEXT("land"), StuckAt, StuckOn ? .55f : .35f);
}

void AVortexProjectile::OnRep_Burst()
{
    if (!bBurst) return;
    BurstAge = 0.f;
    SetActorLocation(BurstAt);
    if (Trail) Trail->Deactivate();
    if (Flames) Flames->Deactivate();
    if (GetNetMode() != NM_DedicatedServer)
    {
        // 07/11: a flecha que explode some e estoura no efeito do elemento dela
        if (IsPhysical() && Radius > 0.f)
        {
            SetActorHiddenInGame(true);
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), VortexSwords::ElementBurst(Element), BurstAt, FRotator::ZeroRotator, FVector(FMath::Max(.6f, Radius / 300.f)));
        }
        // 03/11: a bola de fogo explode em chamas de verdade
        if (Element == TEXT("fire") && Mode == TEXT("orb"))
            if (UNiagaraSystem* Boom = VortexHitFx::System(TEXT("/Game/Vortex/Bosses/Azhur/FX/NS_MagmaCore_Explosion.NS_MagmaCore_Explosion")))
                if (UNiagaraComponent* Blast = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Boom, BurstAt, FRotator::ZeroRotator, FVector(FMath::Max(.4f, Radius / 300.f))))
                {
                    // 10/11: esse efeito (do chefe) repete sem parar; o chefe desliga depois de uns segundos, e aqui ficava
                    // um fogo eterno no lugar da explosão (o usuário: "fica um fogo bugado infinito")
                    FTimerHandle Off;
                    GetWorld()->GetTimerManager().SetTimer(Off, FTimerDelegate::CreateWeakLambda(Blast, [Blast]() { Blast->Deactivate(); }), 1.8f, false);
                }
        VortexAudio::Play(this, Radius > 0.f ? TEXT("skill_barrier_hit") : TEXT("creature_hit"), BurstAt, .5f);
    }
}

void AVortexProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bBurst)
    {
        // clarão: o núcleo cresce e some; a explosão (orb, burst) abre do tamanho do raio
        BurstAge += DeltaSeconds;
        const float T = FMath::Clamp(BurstAge / ProjectileBurstSeconds, 0.f, 1.f);
        const float Grow = Radius > 0.f ? FMath::Lerp(.3f, Radius / 50.f, T) : 1.f + 2.f * T;
        if (Core) Core->SetRelativeScale3D(FVector(.22f) * Grow);
        if (CoreMaterial) CoreMaterial->SetScalarParameterValue(TEXT("Glow"), 8.f * (1.f - T));
        Light->SetIntensity(20.f * (1.f - T));
        Light->SetAttenuationRadius(FMath::Max(300.f, Radius * 2.f));
        if (T >= 1.f)
        {
            if (HasAuthority()) Destroy();
            else SetActorHiddenInGame(true);
        }
        return;
    }
    if (IsPhysical())
    {
        if (bStuck) return;
        // voo com gravidade: a flecha aponta para onde está indo
        const float Was = Age;
        Age += DeltaSeconds;
        const FVector From = ArrowAt(Was), To = ArrowAt(Age);
        const FVector Velocity = Direction * Speed + FVector(0.f, 0.f, ArrowGravity * Drop * Age);
        const float Blend = FMath::Clamp(1.f - Age / .2f, 0.f, 1.f);
        SetActorLocationAndRotation(To + ViewOffset * Blend, (Velocity - ViewOffset * (Blend > 0.f ? 5.f : 0.f)).Rotation());
        Travelled = FVector::Dist(Start, To);
        if (!HasAuthority()) return;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexArrow), false, this);
        if (GetOwner()) Query.AddIgnoredActor(GetOwner());
        TArray<FHitResult> Bodies;
        FCollisionObjectQueryParams Pawns;
        Pawns.AddObjectTypesToQuery(ECC_Pawn);
        GetWorld()->SweepMultiByObjectType(Bodies, From, To, FQuat::Identity, Pawns, FCollisionShape::MakeSphere(18.f * FMath::Max(1.f, Size)), Query);
        UVortexCombatComponent* Combat = GetOwner() ? GetOwner()->FindComponentByClass<UVortexCombatComponent>() : nullptr;
        for (const FHitResult& Body : Bodies)
        {
            AActor* Target = Body.GetActor();
            if (!Target || Hits.Contains(Target) || !Combat || !Combat->CanHarm(Target)) continue;
            if (Radius > 0.f)
            {
                // 07/11: a flecha que explode (o poder "explosive" dos arcos)
                Explode(Body.Location);
                StartBurst(Body.Location);
                return;
            }
            HitTarget(Target, Damage);
            if (!bPierce)
            {
                Stick(Body.ImpactPoint.IsZero() ? FVector(Body.Location) : FVector(Body.ImpactPoint), Velocity, Target);
                return;
            }
        }
        TArray<FHitResult> Touches;
        FCollisionObjectQueryParams Solid;
        Solid.AddObjectTypesToQuery(ECC_WorldStatic);
        // 05/11: os barris e caixotes da vila são corpos com física: sem isto a flecha passava por dentro deles
        Solid.AddObjectTypesToQuery(ECC_PhysicsBody);
        GetWorld()->SweepMultiByObjectType(Touches, From, To, FQuat::Identity, Solid, FCollisionShape::MakeSphere(3.f), Query);
        if (const FHitResult* Wall = Touches.FindByPredicate([](const FHitResult& Hit) { return Hit.GetComponent() && !Hit.GetComponent()->IsA<UInstancedStaticMeshComponent>(); }))
        {
            if (Radius > 0.f) { Explode(Wall->ImpactPoint); StartBurst(Wall->ImpactPoint); return; }
            // no barril, crava nele (e rola junto); na parede e no chão, fica no lugar
            AVortexBreakable* Barrel = Cast<AVortexBreakable>(Wall->GetActor());
            Stick(Wall->ImpactPoint, Velocity, Barrel);
            return;
        }
        // sem bater em nada: cai até o chão (o terreno é estático) ou some longe demais
        if (Travelled > Range * 2.5f) Destroy();
        return;
    }
    Age += DeltaSeconds;
    const float Before = Travelled;
    Travelled = FMath::Min(Range, Speed * Age);
    const FVector From = Start + Direction * Before, To = Start + Direction * Travelled;
    if (!HasAuthority())
    {
        SetActorLocation(To);
        return;
    }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexProjectile), false, this);
    if (GetOwner()) Query.AddIgnoredActor(GetOwner());
    // corpos no caminho deste quadro
    TArray<FHitResult> Bodies;
    FCollisionObjectQueryParams Pawns;
    Pawns.AddObjectTypesToQuery(ECC_Pawn);
    GetWorld()->SweepMultiByObjectType(Bodies, From, To, FQuat::Identity, Pawns, FCollisionShape::MakeSphere(28.f * FMath::Max(1.f, Size)), Query);
    for (const FHitResult& Body : Bodies)
    {
        AActor* Target = Body.GetActor();
        UVortexCombatComponent* Combat = GetOwner() ? GetOwner()->FindComponentByClass<UVortexCombatComponent>() : nullptr;
        if (!Target || Hits.Contains(Target) || !Combat || !Combat->CanHarm(Target)) continue;
        if (Radius > 0.f)
        {
            // esfera: explode no primeiro que tocar
            Explode(Body.Location);
            StartBurst(Body.Location);
            return;
        }
        HitTarget(Target, Damage);
        if (!bPierce)
        {
            StartBurst(Body.Location);
            return;
        }
    }
    // chão e paredes param (a vegetação instanciada não)
    TArray<FHitResult> Touches;
    FCollisionObjectQueryParams Solid;
    Solid.AddObjectTypesToQuery(ECC_WorldStatic);
    GetWorld()->SweepMultiByObjectType(Touches, From, To, FQuat::Identity, Solid, FCollisionShape::MakeSphere(8.f), Query);
    const FHitResult* Wall = Touches.FindByPredicate([](const FHitResult& Hit)
    {
        return Hit.GetComponent() && !Hit.GetComponent()->IsA<UInstancedStaticMeshComponent>();
    });
    if (Wall)
    {
        Explode(Wall->Location);
        StartBurst(Wall->Location);
        return;
    }
    SetActorLocation(To);
    if (Travelled >= Range)
    {
        Explode(To);
        StartBurst(To);
    }
}
