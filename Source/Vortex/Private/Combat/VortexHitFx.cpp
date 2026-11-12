#include "Combat/VortexHitFx.h"
#include "Framework/VortexAudio.h"
#include "ProceduralMeshComponent.h"
#include "TimerManager.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Containers/Ticker.h"
#include "GameFramework/Character.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Framework/VortexCharacter.h"
#include "Creatures/VortexMonster.h"
#include "Creatures/VortexDress.h"
#include "Components/SkeletalMeshComponent.h"
#include "Creatures/VortexMonsterSpecies.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

namespace VortexHitFx
{
    namespace
    {
        bool IsDummy(const AActor* Target) { return Target && Target->GetClass()->GetName().Contains(TEXT("TrainingDummy")); }

        // recuo: o corpo desenhado vai uns centímetros para trás e volta (só na tela; a posição de verdade, a rede
        // e a IA não mudam). Golpes seguidos no mesmo alvo partem da posição de repouso guardada
        void Recoil(USceneComponent* Mesh, const FVector& WorldPush)
        {
            static TMap<TWeakObjectPtr<USceneComponent>, FVector> Rest;
            if (!Mesh || !Mesh->GetAttachParent()) return;
            const FVector* Saved = Rest.Find(Mesh);
            const FVector Base = Saved ? *Saved : Mesh->GetRelativeLocation();
            Rest.Add(Mesh, Base);
            const FVector Push = Mesh->GetAttachParent()->GetComponentTransform().InverseTransformVectorNoScale(WorldPush);
            TWeakObjectPtr<USceneComponent> Weak(Mesh);
            float Age = 0.f;
            FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, Base, Push, Age](float Delta) mutable
            {
                USceneComponent* Body = Weak.Get();
                if (!Body) { Rest.Remove(Weak); return false; }
                Age += Delta;
                // sai rápido (40 ms) e volta devagar (140 ms)
                const float Out = Age < .04f ? Age / .04f : 1.f - FMath::Clamp((Age - .04f) / .14f, 0.f, 1.f);
                Body->SetRelativeLocation(Base + Push * FMath::SmoothStep(0.f, 1.f, Out));
                if (Age < .18f) return true;
                Body->SetRelativeLocation(Base);
                Rest.Remove(Weak);
                return false;
            }));
        }
    }

    void Play(AActor* Source, AActor* Target, float Damage, bool bCrit)
    {
        if (!Target || !Target->GetWorld() || Target->GetNetMode() == NM_DedicatedServer) return;
        UWorld* World = Target->GetWorld();

        // 1) brilho por cima do corpo (todas as malhas do alvo, inclusive as peças de roupa)
        if (UMaterialInterface* Flash = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_HitFlash.M_HitFlash")))
        {
            UMaterialInstanceDynamic* Glow = UMaterialInstanceDynamic::Create(Flash, Target);
            Glow->SetVectorParameterValue(TEXT("Color"), bCrit ? FLinearColor(1.f, .7f, .25f) : FLinearColor(1.f, .45f, .4f));
            Glow->SetScalarParameterValue(TEXT("Glow"), bCrit ? 2.4f : 1.4f);
            TArray<UMeshComponent*> Meshes;
            Target->GetComponents<UMeshComponent>(Meshes);
            for (UMeshComponent* Mesh : Meshes) if (Mesh && Mesh->IsVisible() && Mesh->GetFName() != TEXT("SlimeBody")) Mesh->SetOverlayMaterial(Glow);   // o clarão não acompanha a deformação da gelatina
            TWeakObjectPtr<AActor> Weak(Target);
            FTimerHandle Clear;
            World->GetTimerManager().SetTimer(Clear, FTimerDelegate::CreateLambda([Weak]()
            {
                if (!Weak.IsValid()) return;
                TArray<UMeshComponent*> All;
                Weak->GetComponents<UMeshComponent>(All);
                for (UMeshComponent* Mesh : All) if (Mesh) Mesh->SetOverlayMaterial(nullptr);
            }), bCrit ? .16f : .11f, false);
        }

        // 2) pausa de impacto: só a animação de quem bate e de quem apanha
        // (quem apanha para menos: num combo, 65 ms a cada golpe deixava o bicho quase sempre congelado)
        for (AActor* Actor : { Source, Target })
        {
            if (!Actor) continue;
            // o boneco de um jogador remoto não para (o movimento dele é previsto lá na máquina dele)
            if (const APawn* Pawn = Cast<APawn>(Actor); Pawn && Pawn->IsPlayerControlled() && !Pawn->IsLocallyControlled()) continue;
            const float Stop = bCrit ? .1f : Actor == Target ? .035f : .065f;
            Actor->CustomTimeDilation = .05f;
            TWeakObjectPtr<AActor> Weak(Actor);
            FTimerHandle Resume;
            World->GetTimerManager().SetTimer(Resume, FTimerDelegate::CreateLambda([Weak]() { if (Weak.IsValid()) Weak->CustomTimeDilation = 1.f; }), Stop, false);
        }

        // 3) tranco na câmera de quem bate e de quem apanha (só o herói desta máquina)
        if (AVortexCharacter* Hitter = Cast<AVortexCharacter>(Source); Hitter && Hitter->IsLocallyControlled()) Hitter->KickCamera(bCrit ? 1.6f : 1.f);
        if (AVortexCharacter* Hurt = Cast<AVortexCharacter>(Target); Hurt && Hurt->IsLocallyControlled()) Hurt->KickCamera(1.3f);

        // 4) respingos no ponto do acerto (do lado de quem bate), sangue ou palha
        const FVector Toward = Source ? (Source->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D() : FVector::ForwardVector;
        // 5) o alvo recua um pouco a cada golpe (chefes quase nada; o herói desta máquina não, a câmera já treme)
        if (const ACharacter* Body = Cast<ACharacter>(Target); Body && Body->GetMesh() && !(Body->IsLocallyControlled() && Body->IsPlayerControlled()))
        {
            const AVortexMonster* Monster = Cast<AVortexMonster>(Target);
            const float Size = Monster && Monster->IsBoss() ? 3.f : bCrit ? 15.f : 9.f;
            Recoil(Body->GetMesh(), -Toward * Size);
        }
        const FVector At = Target->GetActorLocation() + Toward * 30.f + FVector(0.f, 0.f, IsDummy(Target) ? 50.f : 25.f);
        FActorSpawnParameters Params;
        Params.ObjectFlags |= RF_Transient;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (AVortexHitBurst* Burst = World->SpawnActor<AVortexHitBurst>(At, FRotator::ZeroRotator, Params))
        {
            const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
            const float Ground = Terrain ? Terrain->GroundAt(At.X, At.Y) : At.Z - 100.f;
            // sangue nos bichos e heróis, palha nos bonecos e a cor própria das criaturas do Godot (gel, esporos, pedra)
            FLinearColor Color = IsDummy(Target) ? FLinearColor(.75f, .58f, .22f) : FLinearColor(.48f, .02f, .03f);
            if (const AVortexMonster* Monster = Cast<AVortexMonster>(Target); Monster && Monster->Def()) Color = Monster->Def()->Splash;
            Burst->Start(-Toward, Color, FMath::Clamp(int32(6 + Damage * .8f) + (bCrit ? 8 : 0), 6, 28), Ground > -299.f ? Ground : At.Z - 100.f);
        }
    }
}

UNiagaraSystem* VortexHitFx::System(const TCHAR* Path)
{
    static TMap<FString, UNiagaraSystem*> Kept;
    if (UNiagaraSystem** Found = Kept.Find(Path)) return *Found;
    UNiagaraSystem* Loaded = LoadObject<UNiagaraSystem>(nullptr, Path);
    if (Loaded) Loaded->AddToRoot();   // poucos e pequenos: ficam carregados (recarregar = recompilar no editor)
    Kept.Add(Path, Loaded);
    return Loaded;
}

void VortexHitFx::Parry(AActor* Attacker, const FVector& Between, bool bStunned)
{
    if (!Attacker || !Attacker->GetWorld() || Attacker->GetNetMode() == NM_DedicatedServer) return;
    UWorld* World = Attacker->GetWorld();
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (AVortexHitBurst* Sparks = World->SpawnActor<AVortexHitBurst>(Between, FRotator::ZeroRotator, Params))
    {
        const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
        const float Ground = Terrain ? Terrain->GroundAt(Between.X, Between.Y) : Between.Z - 150.f;
        Sparks->Start(FVector::UpVector, FLinearColor(4.f, 3.f, 1.2f), 22, Ground > -299.f ? Ground : Between.Z - 150.f);
    }
    // o golpe do bicho é rebatido: um tranco curto na câmera de quem aparou
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (AVortexCharacter* Hero = It->IsValid() ? Cast<AVortexCharacter>((*It)->GetPawn()) : nullptr; Hero && Hero->IsLocallyControlled() &&
            FVector::Dist(Hero->GetActorLocation(), Between) < 400.f) Hero->KickCamera(1.2f);
    }
    if (bStunned) StunStars(Attacker, .65f);
}

void VortexHitFx::Splash(UWorld* World, const FVector& At, const FLinearColor& Color, int32 Count)
{
    if (!World || World->GetNetMode() == NM_DedicatedServer) return;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (AVortexHitBurst* Burst = World->SpawnActor<AVortexHitBurst>(At + FVector(0.f, 0.f, 20.f), FRotator::ZeroRotator, Params))
    {
        const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
        const float Ground = Terrain ? Terrain->GroundAt(At.X, At.Y) : At.Z;
        Burst->Start(FVector::UpVector, Color, Count, Ground > -299.f ? Ground : At.Z);
    }
}

void VortexHitFx::Ripple(UWorld* World, const FVector& At, float Size, float Life)
{
    if (!World || World->GetNetMode() == NM_DedicatedServer) return;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (AVortexRipple* Ring = World->SpawnActor<AVortexRipple>(At + FVector(0.f, 0.f, 2.f), FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Params))
        Ring->Start(Size, Life);
}

void VortexHitFx::WaterSplash(UWorld* World, const FVector& Surface, float Strength)
{
    if (!World || World->GetNetMode() == NM_DedicatedServer) return;
    Strength = FMath::Clamp(Strength, 0.f, 1.f);
    // coroa de gotas (cubinhos azul-claros e brancos), dois anéis que abrem e o tchibum
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    for (int32 k = 0; k < 2; ++k)
    {
        if (AVortexHitBurst* Burst = World->SpawnActor<AVortexHitBurst>(Surface + FVector(0.f, 0.f, 10.f), FRotator::ZeroRotator, Params))
        {
            Burst->Start(FVector::UpVector * (1.2f + Strength), k == 0 ? FLinearColor(.72f, .9f, 1.f) : FLinearColor(.95f, .98f, 1.f),
                FMath::RoundToInt(14 + 26 * Strength), Surface.Z - 5.f);
        }
    }
    Ripple(World, Surface, 140.f + 160.f * Strength, 1.1f);
    Ripple(World, Surface, 70.f + 90.f * Strength, .7f);
    VortexAudio::Play(World, TEXT("water_splash"), Surface, .5f + .5f * Strength);
}

void VortexHitFx::SwingTrail(AActor* Owner, float Reach, int32 SwingIndex, float Delay, const FLinearColor& Tint)
{
    if (!Owner || !Owner->GetWorld() || Owner->GetNetMode() == NM_DedicatedServer) return;
    TWeakObjectPtr<AActor> Weak(Owner);
    auto Spawn = [Weak, Reach, SwingIndex, Tint]()
    {
        AActor* Who = Weak.Get();
        if (!Who) return;
        // em primeira pessoa, quem joga vê a arma da câmera; o arco ficaria uma faixa na cara
        if (const AVortexCharacter* Hero = Cast<AVortexCharacter>(Who); Hero && Hero->IsLocallyControlled() && Hero->IsFirstPerson()) return;
        FActorSpawnParameters Params;
        Params.ObjectFlags |= RF_Transient;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (AVortexSwingTrail* Trail = Who->GetWorld()->SpawnActor<AVortexSwingTrail>(Who->GetActorLocation(), Who->GetActorRotation(), Params))
        {
            // cada golpe do combo corta num ângulo: diagonal, a diagonal de volta, e o largo na horizontal. 07/11 (o usuário:
            // "o primeiro corte, o efeito no ar está inverso"): a ida e a volta são a MESMA diagonal, só no sentido oposto
            // (o 1º estava inclinado para o outro lado)
            static const float Tilts[] = { 28.f, 28.f, 4.f, -50.f };
            Trail->Start(Who, Reach, Tilts[SwingIndex % 4], SwingIndex % 2 == 1, Tint);
        }
    };
    if (Delay <= .01f) { Spawn(); return; }
    FTimerHandle Handle;
    Owner->GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda(Spawn), Delay, false);
}

void VortexHitFx::GroundSlam(AActor* Owner, float Radius, float Delay)
{
    if (!Owner || !Owner->GetWorld() || Owner->GetNetMode() == NM_DedicatedServer) return;
    TWeakObjectPtr<AActor> Weak(Owner);
    FTimerHandle Handle;
    Owner->GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([Weak, Radius]()
    {
        AActor* Who = Weak.Get();
        if (!Who) return;
        const FVector At = Who->GetActorLocation() + Who->GetActorForwardVector() * 90.f - FVector(0.f, 0.f, Who->GetSimpleCollisionHalfHeight() - 5.f);
        if (UNiagaraSystem* Shock = System(TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/VFX_Shockwave_01_White_1s.VFX_Shockwave_01_White_1s")))
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(Who->GetWorld(), Shock, At, FRotator::ZeroRotator, FVector(FMath::Max(.25f, Radius / 900.f)));
        Splash(Who->GetWorld(), At, FLinearColor(.55f, .47f, .36f), 16);
        if (AVortexCharacter* Hero = Cast<AVortexCharacter>(Who); Hero && Hero->IsLocallyControlled()) Hero->KickCamera(1.2f);
    }), FMath::Max(.01f, Delay), false);
}

void VortexHitFx::StunStars(AActor* Target, float Seconds)
{
    if (!Target || !Target->GetWorld() || Target->GetNetMode() == NM_DedicatedServer || Seconds <= 0.f) return;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (AVortexStunStars* Stars = Target->GetWorld()->SpawnActor<AVortexStunStars>(Target->GetActorLocation(), FRotator::ZeroRotator, Params))
    {
        Stars->Start(Target, Seconds);
    }
}

void VortexHitFx::Warmup(UWorld* World)
{
    if (!World || World->GetNetMode() == NM_DedicatedServer) return;
    if (TActorIterator<AVortexFxWarmup> It(World); It) return;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    World->SpawnActor<AVortexFxWarmup>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
}

AVortexFxWarmup::AVortexFxWarmup()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AVortexFxWarmup::BeginPlay()
{
    Super::BeginPlay();
    const double Started = FPlatformTime::Seconds();
    // rastros das flechas e dardos (todos os elementos) e as faíscas do corte à distância
    static const TCHAR* Systems[] = {
        TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Basic.NS_ArrowTrail_Basic"), TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Fire.NS_ArrowTrail_Fire"),
        TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Holy.NS_ArrowTrail_Holy"), TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Ice.NS_ArrowTrail_Ice"),
        TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Magic.NS_ArrowTrail_Magic"), TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Nature.NS_ArrowTrail_Nature"),
        TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Water.NS_ArrowTrail_Water"),
        TEXT("/Game/FreeParticle_SoftTofu/Niagara/NS_Sparkling_Glow.NS_Sparkling_Glow"),
        // Soco Sério e poderes de classe
        TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/VFX_Shockwave_01_White_Big_1s.VFX_Shockwave_01_White_Big_1s"),
        TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Vertical/VFX_Shockwave_Vertical_01_White_Small_1s.VFX_Shockwave_Vertical_01_White_Small_1s"),
        TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Explosion.NS_Free_Spells_Explosion"),
        // 02/11: a pancada do martelo (o especial travava o jogo na hora: os dois carregavam ali)
        TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave"),
        TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/VFX_Shockwave_01_White_1s.VFX_Shockwave_01_White_1s"),
        TEXT("/Game/Vefects/Easy_Impact_Frames/VFX/Frames/Particles/NS_Impact_Frame_Advanced_03_Always.NS_Impact_Frame_Advanced_03_Always"),
        // Caldeira: chamas da lava e de quem queima
        TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Floor_01_Simple.NS_Fire_Floor_01_Simple"),
        TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Small.NS_Fire_Small"),
        // 10/11: a bola de fogo do cajado (o usuário: "muito travada quando lança"): o rastro, as chamas e a explosão
        TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Fire_02.NS_ArrowTrail_Fire_02"),
        TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Medium.NS_Fire_Medium"),
        TEXT("/Game/Vortex/Bosses/Azhur/FX/NS_MagmaCore_Explosion.NS_MagmaCore_Explosion"),
    };
    for (const TCHAR* Path : Systems)
    {
        if (UNiagaraSystem* Effect = VortexHitFx::System(Path))
        {
            UNiagaraFunctionLibrary::SpawnSystemAttached(Effect, RootComponent, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
        }
    }
    // materiais nas malhas em que eles aparecem (malha comum, lote e malha gerada)
    auto Mesh = [this](const TCHAR* Shape, const TCHAR* MaterialPath)
    {
        UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
        Part->SetupAttachment(RootComponent);
        Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Shape));
        Part->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, MaterialPath));
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetCastShadow(false);
        Part->RegisterComponent();
    };
    Mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"), TEXT("/Game/Vortex/Weapons/M_ProjectileGlow.M_ProjectileGlow"));
    Mesh(TEXT("/Engine/BasicShapes/Cone.Cone"), TEXT("/Game/Vortex/Combat/M_SpectralGlow.M_SpectralGlow"));
    Mesh(TEXT("/Engine/BasicShapes/Plane.Plane"), TEXT("/Game/Vortex/Combat/M_WaterRipple.M_WaterRipple"));
    Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), TEXT("/Game/Vortex/Combat/M_LavaFlow.M_LavaFlow"));
    Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("/Game/Vortex/Creatures/RuneColossus/Materials/M_AzhurLava.M_AzhurLava"));
    Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("/Game/Vortex/Combat/M_HitFlash.M_HitFlash"));
    Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), TEXT("/Game/Vortex/Stadium/MI_Stadium_wood.MI_Stadium_wood"));
    // 11/11: o brilho e a fumaça do saque (o primeiro bicho vencido travou 1,8 s no jogo empacotado carregando estes)
    Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), TEXT("/Game/Vortex/Combat/M_DropRing.M_DropRing"));
    Mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"), TEXT("/Game/Vortex/Combat/M_FxPuff.M_FxPuff"));
    UInstancedStaticMeshComponent* Bits = NewObject<UInstancedStaticMeshComponent>(this);
    Bits->SetupAttachment(RootComponent);
    Bits->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Bits->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SolidColor.M_SolidColor")));
    Bits->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bits->RegisterComponent();
    Bits->AddInstance(FTransform::Identity);
    UProceduralMeshComponent* Arc = NewObject<UProceduralMeshComponent>(this);
    Arc->SetupAttachment(RootComponent);
    Arc->RegisterComponent();
    Arc->CreateMeshSection(0, { FVector(0, 0, 0), FVector(0, 100, 0), FVector(0, 0, 100) }, { 0, 1, 2 }, {}, { FVector2D(0, 0), FVector2D(1, 0), FVector2D(0, 1) }, {}, {}, false);
    Arc->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SlashArc.M_SlashArc")));
    // 11/11: o primeiro bandido montava as peças da roupa pela primeira vez no meio do jogo (370 ms no jogo empacotado;
    // os seguintes, menos de 1 ms): dois bonecos vestidos aqui, na tela de carregamento, fazem essa primeira vez
    if (USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned.Adventurer_Skinned")))
        for (const bool bLeader : { true, false })
        {
            USkeletalMeshComponent* Doll = NewObject<USkeletalMeshComponent>(this);
            Doll->SetupAttachment(RootComponent);
            Doll->SetSkeletalMesh(Body);
            Doll->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Doll->RegisterComponent();
            UVortexDressComponent* Dress = NewObject<UVortexDressComponent>(this);
            Dress->RegisterComponent();
            Dress->Apply(Doll, UVortexDressComponent::BanditLook(bLeader ? 1 : 7, bLeader));
        }
    SetActorScale3D(FVector(.004f));
    UE_LOG(LogTemp, Display, TEXT("VORTEX_FX_WARMUP carregados em %.0f ms"), (FPlatformTime::Seconds() - Started) * 1000.0);
}

void AVortexFxWarmup::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    // fica bem na frente da câmera (precisa ser desenhado para os shaders ficarem prontos)
    if (const APlayerController* Viewer = GetWorld()->GetFirstPlayerController(); Viewer && Viewer->PlayerCameraManager)
    {
        const FVector Eye = Viewer->PlayerCameraManager->GetCameraLocation();
        SetActorLocation(Eye + Viewer->PlayerCameraManager->GetCameraRotation().Vector() * 60.f);
    }
    if (Age > 1.5f) Destroy();
}

AVortexStunStars::AVortexStunStars()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;
    Stars = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Stars"));
    RootComponent = Stars;
    Stars->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Stars->SetCastShadow(false);
}

void AVortexStunStars::Start(AActor* Target, float Seconds)
{
    Over = Target;
    Life = Seconds + .25f;
    // acima da cápsula (ou do contorno) de quem foi atordoado
    FVector Origin, Extent;
    Target->GetActorBounds(true, Origin, Extent);
    Height = FMath::Clamp(Origin.Z + Extent.Z - Target->GetActorLocation().Z + 25.f, 60.f, 600.f);
    Stars->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    if (UMaterialInterface* Solid = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SolidColor.M_SolidColor")))
    {
        UMaterialInstanceDynamic* Tint = UMaterialInstanceDynamic::Create(Solid, this);
        Tint->SetVectorParameterValue(TEXT("Color"), FLinearColor(6.f, 4.6f, 1.2f));
        Stars->SetMaterial(0, Tint);
    }
    for (int32 i = 0; i < 4; ++i) Stars->AddInstance(FTransform(FVector::ZeroVector), true);
    Tick(0.f);
}

void AVortexStunStars::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    if (!Over.IsValid() || Age > Life) { Destroy(); return; }
    const FVector Center = Over->GetActorLocation() + FVector(0.f, 0.f, Height);
    const float Fade = FMath::Clamp((Life - Age) * 4.f, 0.f, 1.f) * FMath::Clamp(Age * 8.f, 0.f, 1.f);
    TArray<FTransform> Poses;
    for (int32 i = 0; i < 4; ++i)
    {
        const float A = Age * 6.f + i * UE_HALF_PI;
        const FVector At = Center + FVector(FMath::Cos(A) * 32.f, FMath::Sin(A) * 32.f, FMath::Sin(Age * 9.f + i) * 5.f);
        Poses.Add(FTransform(FRotator(45.f, A * 57.3f, 45.f), At, FVector(.07f * Fade)));
    }
    Stars->BatchUpdateInstancesTransforms(0, Poses, true, true);
}

AVortexSwingTrail::AVortexSwingTrail()
{
    PrimaryActorTick.bCanEverTick = true;
    Arc = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Arc"));
    SetRootComponent(Arc);
    Arc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Arc->SetCastShadow(false);
    Arc->bAffectDistanceFieldLighting = false;
    Arc->bAffectDynamicIndirectLighting = false;
}

void AVortexSwingTrail::Start(AActor* InOwner, float Reach, float InTilt, bool bReverse, const FLinearColor& Tint)
{
    Follow = InOwner;
    Tilt = InTilt;
    Sweep = bReverse ? -1.f : 1.f;
    // fita em leque: 150 graus na frente, do alcance da arma para dentro; mais larga no meio
    constexpr int32 Segments = 28, Rows = 3;
    // a borda de fora do leque na ponta da arma (o alcance do acerto): de dentro do braço até onde o golpe pega
    const float Radius = FMath::Clamp(Reach, 70.f, 380.f), Width = Radius * .45f;
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    for (int32 i = 0; i <= Segments; ++i)
    {
        const float U = float(i) / Segments;
        const float Theta = FMath::DegreesToRadians(FMath::Lerp(-75.f, 75.f, U));
        const float Taper = FMath::Sin(U * UE_PI);
        for (int32 j = 0; j <= Rows; ++j)
        {
            const float V = float(j) / Rows;
            const float R = Radius - Width * Taper * V;
            Vertices.Add(FVector(FMath::Cos(Theta) * R, FMath::Sin(Theta) * R, 0.f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(U, V));
        }
    }
    for (int32 i = 0; i < Segments; ++i)
        for (int32 j = 0; j < Rows; ++j)
        {
            const int32 A = i * (Rows + 1) + j, B = A + Rows + 1;
            Triangles.Append({ A, B, A + 1, A + 1, B, B + 1 });
            Triangles.Append({ A, A + 1, B, A + 1, B + 1, B });   // dos dois lados
        }
    Arc->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, {}, {}, false);
    if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SlashArc.M_SlashArc")))
    {
        Look = UMaterialInstanceDynamic::Create(Base, this);
        Look->SetScalarParameterValue(TEXT("Seed"), FMath::FRand());
        Look->SetScalarParameterValue(TEXT("Glow"), 3.f);
        Look->SetScalarParameterValue(TEXT("Sharp"), 4.f);
        Look->SetVectorParameterValue(TEXT("Color"), Tint);
        Arc->SetMaterial(0, Look);
    }
    Tick(0.f);
}

void AVortexSwingTrail::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    const AActor* Who = Follow.Get();
    if (!Who || Age > .3f) { Destroy(); return; }
    const float T = Age / .3f;
    // varre 50 graus no sentido do golpe e se apaga; inclinado no ângulo do corte, na altura do peito
    const FRotator Facing = Who->GetActorRotation();
    const FQuat Sweep2 = FQuat(FVector::UpVector, FMath::DegreesToRadians(Sweep * FMath::Lerp(-25.f, 25.f, T)));
    const FQuat Lean = FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Tilt));
    SetActorLocationAndRotation(Who->GetActorLocation() + FVector(0.f, 0.f, 25.f), FQuat(Facing) * Lean * Sweep2);
    if (Look) Look->SetScalarParameterValue(TEXT("Fade"), FMath::Square(1.f - T));
}

AVortexRipple::AVortexRipple()
{
    PrimaryActorTick.bCanEverTick = true;
    Plane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plane"));
    SetRootComponent(Plane);
    Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Plane->SetCastShadow(false);
    Plane->SetReceivesDecals(false);
}

void AVortexRipple::Start(float InSize, float InLife)
{
    Size = InSize;
    Life = FMath::Max(.1f, InLife);
    Plane->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
    if (UMaterialInterface* Ring = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_WaterRipple.M_WaterRipple")))
    {
        Wave = UMaterialInstanceDynamic::Create(Ring, this);
        Plane->SetMaterial(0, Wave);
    }
    Plane->SetWorldScale3D(FVector(.01f));
}

void AVortexRipple::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    const float T = FMath::Clamp(Age / Life, 0.f, 1.f);
    // abre rápido no começo e desacelera; o anel afina e some
    const float Open = 1.f - FMath::Square(1.f - T);
    Plane->SetWorldScale3D(FVector(FMath::Max(.01f, Size * Open / 50.f), FMath::Max(.01f, Size * Open / 50.f), 1.f));
    if (Wave) Wave->SetScalarParameterValue(TEXT("Fade"), 1.f - T);
    if (Age >= Life) Destroy();
}

AVortexHitBurst::AVortexHitBurst()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;
    Bits = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Bits"));
    RootComponent = Bits;
    Bits->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bits->SetCastShadow(false);
}

void AVortexHitBurst::Start(const FVector& Away, const FLinearColor& Color, int32 Count, float Ground)
{
    Floor = Ground + 1.f;
    Bits->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    if (UMaterialInterface* Solid = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SolidColor.M_SolidColor")))
    {
        UMaterialInstanceDynamic* Tint = UMaterialInstanceDynamic::Create(Solid, this);
        Tint->SetVectorParameterValue(TEXT("Color"), Color);
        Bits->SetMaterial(0, Tint);
    }
    FRandomStream Rng(int32(GetActorLocation().X * 7.f + GetActorLocation().Y));
    const FVector Origin = GetActorLocation();
    for (int32 i = 0; i < Count; ++i)
    {
        FBit Bit;
        Bit.At = Origin + Rng.VRand() * 6.f;
        // espalha para o lado de fora do golpe e para cima
        Bit.Velocity = (Away * Rng.FRandRange(80.f, 240.f) + Rng.VRand() * 110.f + FVector(0.f, 0.f, Rng.FRandRange(120.f, 300.f)));
        Bit.Size = Rng.FRandRange(.025f, .055f);
        Bit.Life = Rng.FRandRange(.45f, .85f);
        State.Add(Bit);
        Poses.Add(FTransform(FRotator(Rng.FRandRange(0.f, 90.f), Rng.FRandRange(0.f, 90.f), 0.f), Bit.At, FVector(Bit.Size)));
    }
    SetActorLocation(FVector::ZeroVector);   // as cópias ficam em coordenadas do mundo
    Bits->AddInstances(Poses, false, true);
}

void AVortexHitBurst::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    bool bAlive = false;
    for (int32 i = 0; i < State.Num(); ++i)
    {
        FBit& Bit = State[i];
        const float Left = 1.f - Age / Bit.Life;
        if (Left <= 0.f) { Poses[i].SetScale3D(FVector::ZeroVector); continue; }
        bAlive = true;
        Bit.Velocity.Z -= 980.f * DeltaSeconds;
        Bit.At += Bit.Velocity * DeltaSeconds;
        // bate no chão e espalha (fica uma gotinha até sumir)
        if (Bit.At.Z < Floor) { Bit.At.Z = Floor; Bit.Velocity *= FVector(.3f, .3f, 0.f); }
        Poses[i].SetLocation(Bit.At);
        Poses[i].SetScale3D(FVector(Bit.Size * FMath::Clamp(Left * 1.6f, 0.f, 1.f)));
    }
    Bits->BatchUpdateInstancesTransforms(0, Poses, true, true);
    if (!bAlive || Age > 1.2f) Destroy();
}
