#include "Combat/VortexTelegraph.h"

#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexHitFx.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Creatures/VortexMonster.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

AVortexTelegraph::AVortexTelegraph()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);
    NetCullDistanceSquared = FMath::Square(15000.f);
    Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
    RootComponent = Disc;
    Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Disc->SetCastShadow(false);
    Disc->bAffectDistanceFieldLighting = false;
    Disc->SetGenerateOverlapEvents(false);
}

AVortexTelegraph* AVortexTelegraph::Spawn(AActor* Source, const FVector& At, float InRadius, float InDelay, float InDamage, bool bInRing,
    const FLinearColor& InColor, bool bInMagic, float InStun)
{
    if (!Source || !Source->HasAuthority()) return nullptr;
    UWorld* World = Source->GetWorld();
    // no chão de verdade (o ponto pedido pode estar no ar ou embaixo do relevo)
    FVector Ground = At;
    if (const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData())
    {
        const float Z = Terrain->GroundAt(At.X, At.Y);
        if (Z > -299.f) Ground.Z = Z;
    }
    FActorSpawnParameters Params;
    Params.Owner = Source;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AVortexTelegraph* Mark = World->SpawnActorDeferred<AVortexTelegraph>(StaticClass(), FTransform(Ground + FVector(0.f, 0.f, 6.f)), Source, nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Mark) return nullptr;
    Mark->Radius = FMath::Max(50.f, InRadius);
    Mark->Delay = FMath::Max(.1f, InDelay);
    Mark->Damage = InDamage;
    Mark->bRing = bInRing;
    Mark->Color = InColor;
    Mark->bMagic = bInMagic;
    Mark->Stun = InStun;
    Mark->FinishSpawning(FTransform(Ground + FVector(0.f, 0.f, 6.f)));
    return Mark;
}

void AVortexTelegraph::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority()) SetLifeSpan(Delay + .6f);
    if (GetNetMode() == NM_DedicatedServer) return;
    // 02/11: o aviso em blocos, no estilo do jogo (o disco do M_TelegraphRunes ficava cinza). A borda: cubinhos na cor da
    // espécie; a contagem: um anel mais escuro que cresce do centro até a borda; nos golpes em anel, o miolo seguro
    // marcado por dentro. Os tons: Marks[0] a borda, Marks[1] a contagem
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UMaterialInterface* Solid = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SolidColor.M_SolidColor"));
    for (int32 Tone = 0; Tone < 2; ++Tone)
    {
        UInstancedStaticMeshComponent* Marks = NewObject<UInstancedStaticMeshComponent>(this, Tone ? TEXT("Count") : TEXT("Edge"));
        Marks->SetupAttachment(Disc);
        Marks->SetUsingAbsoluteScale(true);
        Marks->SetStaticMesh(Cube);
        Marks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Marks->SetCastShadow(false);
        Marks->bAffectDistanceFieldLighting = false;
        if (Solid)
        {
            UMaterialInstanceDynamic* Tint = UMaterialInstanceDynamic::Create(Solid, this);
            Tint->SetVectorParameterValue(TEXT("Color"), Tone ? Color * .45f : Color * 1.6f);
            Marks->SetMaterial(0, Tint);
        }
        Marks->RegisterComponent();
        auto Circle = [Marks](float R, float Size)
        {
            const int32 Count = FMath::Clamp(FMath::RoundToInt(UE_TWO_PI * R / (Size * 1.7f)), 8, 160);
            for (int32 i = 0; i < Count; ++i)
            {
                const float A = UE_TWO_PI * i / Count;
                Marks->AddInstance(FTransform(FRotator(0.f, A * 57.3f, 0.f), FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f),
                    FVector(Size / 100.f, Size / 100.f, .04f)));
            }
        };
        if (Tone == 0)
        {
            Circle(Radius, 16.f);
            if (bRing) Circle(Radius * .45f, 12.f);
        }
        else Circle(Radius, 11.f);   // a contagem: o mesmo anel, encolhido no centro e crescendo (Tick)
    }
}

void AVortexTelegraph::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    // a contagem cresce do centro (ou do miolo seguro) até a borda
    TArray<UInstancedStaticMeshComponent*> Marks;
    GetComponents<UInstancedStaticMeshComponent>(Marks);
    for (UInstancedStaticMeshComponent* Count : Marks)
    {
        if (Count->GetFName() != TEXT("Count")) continue;
        const float T = FMath::Clamp(Age / Delay, 0.f, 1.f);
        const float Start = bRing ? .45f : .05f;
        Count->SetRelativeScale3D(FVector(FMath::Lerp(Start, 1.f, T), FMath::Lerp(Start, 1.f, T), 1.f));
    }
    if (Age >= Delay && !bResolved)
    {
        bResolved = true;
        if (HasAuthority()) Resolve();
    }
    if (Age > Delay + .25f && Disc) Disc->SetVisibility(false, true);
}

void AVortexTelegraph::Resolve()
{
    AActor* Source = GetOwner();
    UVortexCombatComponent* Combat = Source ? Source->FindComponentByClass<UVortexCombatComponent>() : nullptr;
    const FVector Center = GetActorLocation();
    if (Combat)
    {
        for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
        {
            const FVector Feet = It->GetActorLocation();
            const float Distance = FVector::Dist2D(Feet, Center);
            if (FMath::Abs(Feet.Z - Center.Z) > 300.f || Distance > Radius) continue;
            if (bRing && Distance < Radius * .45f) continue;   // anel: o miolo é seguro
            if (!Combat->CanHarm(*It)) continue;
            Combat->ApplyHit(*It, Damage, bMagic ? EVortexDamageKind::Magic : EVortexDamageKind::Melee, Stun);
        }
    }
    MulticastImpact();
}

void AVortexTelegraph::MulticastImpact_Implementation()
{
    VortexAudio::Play(this, TEXT("rock_impact"), GetActorLocation(), FMath::Clamp(Radius / 400.f, .5f, 1.2f), .06f);
    if (GetNetMode() != NM_DedicatedServer)
    {
        // o impacto: onda de choque do tamanho da marca e, nas grandes, o fogo saindo do chão. Só nos chefes e nos golpes
        // do herói (02/11): os monstros comuns soltavam a onda dourada do golpe secundário do martelo em toda habilidade;
        // eles têm o efeito da espécie (AVortexMonsterFx)
        const AVortexMonster* Monster = Cast<AVortexMonster>(GetOwner());
        // (o golem vulcânico tem os efeitos dele, menores: as marcas dele em arco e em linha davam dezenas de ondas)
        const bool bBigShow = !Monster || (Monster->IsBoss() && !Monster->GetVolcanic());
        const float Size = Radius / 300.f;
        if (bBigShow)
            if (UNiagaraSystem* Wave = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave")))
                UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Wave, GetActorLocation() + FVector(0.f, 0.f, 10.f), FRotator::ZeroRotator, FVector(Size), true, true, ENCPoolMethod::AutoRelease);
        if (bBigShow && Radius >= 300.f)
            if (UNiagaraSystem* Burst = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Explosion.NS_Free_Spells_Explosion")))
                UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Burst, GetActorLocation(), FRotator::ZeroRotator, FVector(Size * .6f), true, true, ENCPoolMethod::AutoRelease);
    }
    // tremor para os heróis desta máquina que estão perto
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AVortexCharacter* Hero = It->IsValid() ? Cast<AVortexCharacter>((*It)->GetPawn()) : nullptr; Hero && Hero->IsLocallyControlled())
        {
            const float Distance = FVector::Dist2D(Hero->GetActorLocation(), GetActorLocation());
            if (Distance < Radius * 2.5f) Hero->KickCamera(FMath::Clamp(Radius / 300.f, .6f, 2.2f) * (1.f - Distance / (Radius * 2.5f)));
        }
    }
}

void AVortexTelegraph::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AVortexTelegraph, Radius, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexTelegraph, Delay, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexTelegraph, bRing, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexTelegraph, Color, COND_InitialOnly);
}
