#include "World/VortexItemDrop.h"

#include "Camera/PlayerCameraManager.h"
#include "Combat/VortexSwords.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Rpg/VortexArmor.h"
#include "Rpg/VortexRpg.h"

namespace
{
    UStaticMesh* Basic(const TCHAR* Name)
    {
        return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name));
    }
    constexpr float LifeSeconds = 300.f, FlySeconds = .55f, Hover = 70.f;
}

AVortexItemDrop::AVortexItemDrop()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);
    SetNetCullDistanceSquared(FMath::Square(9000.f));
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AVortexItemDrop::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AVortexItemDrop, Item);
    DOREPLIFETIME(AVortexItemDrop, Count);
    DOREPLIFETIME(AVortexItemDrop, Landing);
}

AVortexItemDrop* AVortexItemDrop::Spawn(UWorld* World, FName InItem, int32 InCount, const FVector& At, float Toss)
{
    if (!World || InItem.IsNone() || InCount <= 0 || !FVortexRpgCatalog::Get().Item(InItem)) return nullptr;
    // onde cai: um pulo para um lado qualquer, no chão embaixo
    const FVector Side = FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * FMath::FRandRange(60.f, 160.f) * Toss;
    FVector Ground = At + Side;
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexItemDrop), false);
    if (World->LineTraceSingleByChannel(Hit, Ground + FVector(0, 0, 200), Ground - FVector(0, 0, 3000), ECC_WorldStatic, Query)) Ground = Hit.ImpactPoint;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AVortexItemDrop* Drop = World->SpawnActorDeferred<AVortexItemDrop>(StaticClass(), FTransform(At), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Drop) return nullptr;
    Drop->Item = InItem;
    Drop->Count = InCount;
    Drop->Landing = Ground;
    Drop->FinishSpawning(FTransform(At));
    UE_LOG(LogTemp, Display, TEXT("VORTEX_DROP %s x%d"), *InItem.ToString(), InCount);
    return Drop;
}

AVortexItemDrop* AVortexItemDrop::FindNear(const UWorld* World, const FVector& At, float Range)
{
    AVortexItemDrop* Best = nullptr;
    float BestD = Range;
    if (World)
        for (TActorIterator<AVortexItemDrop> It(const_cast<UWorld*>(World)); It; ++It)
        {
            const float D = FVector::Dist2D(It->Landing, At);
            if (D < BestD && FMath::Abs(It->Landing.Z - At.Z) < 250.f) { BestD = D; Best = *It; }
        }
    return Best;
}

void AVortexItemDrop::BeginPlay()
{
    Super::BeginPlay();
    Start = GetActorLocation();
    Offset = FMath::FRandRange(0.f, 6.f);
    if (GetNetMode() != NM_DedicatedServer) BuildLook();
}

void AVortexItemDrop::OnRep_Item()
{
    if (!bBuilt && HasActorBegunPlay()) BuildLook();
}

void AVortexItemDrop::BuildLook()
{
    const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Item);
    if (bBuilt || !Def) return;
    bBuilt = true;
    const FLinearColor Color = FVortexRpgCatalog::RarityColor(Def->Rarity);
    // 08/11: a raridade (0 comum ... 6 divino) deixa o efeito maior e mais forte
    const int32 Rank = FMath::Max(0, TArray<FString>({ TEXT("common"), TEXT("uncommon"), TEXT("rare"), TEXT("epic"), TEXT("legendary"), TEXT("unique"), TEXT("divine") }).IndexOfByKey(Def->Rarity));
    auto Fx = [this, &Color](const TCHAR* Name, float Strength) -> UMaterialInstanceDynamic*
    {
        UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Combat/%s.%s"), Name, Name));
        UMaterialInstanceDynamic* Mat = Base ? UMaterialInstanceDynamic::Create(Base, this) : nullptr;
        if (Mat)
        {
            Mat->SetVectorParameterValue(TEXT("Color"), Color);
            Mat->SetScalarParameterValue(TEXT("Strength"), Strength);
            Mat->SetScalarParameterValue(TEXT("Seed"), Offset);
        }
        return Mat;
    };
    UMaterialInterface* Solid = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SolidColor.M_SolidColor"));
    auto Part = [this](UStaticMesh* Mesh) -> UStaticMeshComponent*
    {
        UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
        C->SetupAttachment(RootComponent);
        C->SetUsingAbsoluteLocation(true);
        C->SetUsingAbsoluteRotation(true);
        C->SetStaticMesh(Mesh);
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->RegisterComponent();
        return C;
    };
    // o modelo: a arma da mão (a da tabela ou a do tipo), a peça de armadura, ou um saquinho na cor da raridade
    UStaticMesh* Mesh = nullptr;
    USkeletalMesh* Piece = nullptr;
    if (VortexRpg::EquipSlotOf(Def->Slot) == VortexRpg::Weapon)
    {
        if (const VortexSwords::FSword* Sword = VortexSwords::ForVisual(Item))
            Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Weapons/Models/%s.%s"), *Sword->Model, *Sword->Model));
        if (!Mesh)
            if (const UVortexWeaponDefinition* Type = UVortexWeaponComponent::LoadType(FVortexRpgCatalog::Get().WeaponType(Item))) Mesh = Type->VisualMesh;
    }
    else if (const VortexArmor::FPiece* Armor = VortexArmor::VisualPiece(Item))
        Piece = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/humano/Armaduras/%s.%s"), *Armor->Mesh, *Armor->Mesh));
    // 08/11: os materiais, poções e amuletos em blocos (Tools/Blender/MakeDropItems.py); sem modelo, continua o saquinho
    if (!Mesh && !Piece)
        Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Weapons/Models/W_item_%s_blocos.W_item_%s_blocos"), *Item.ToString(), *Item.ToString()));
    if (Piece)
    {
        ShowPiece = NewObject<USkeletalMeshComponent>(this);
        ShowPiece->SetupAttachment(RootComponent);
        ShowPiece->SetUsingAbsoluteLocation(true);
        ShowPiece->SetUsingAbsoluteRotation(true);
        ShowPiece->SetSkeletalMesh(Piece);
        ShowPiece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ShowPiece->RegisterComponent();
    }
    else
    {
        Show = Part(Mesh ? Mesh : Basic(TEXT("Cube")));
        if (!Mesh)
        {
            Show->SetWorldScale3D(FVector(.28f));
            if (Solid)
            {
                UMaterialInstanceDynamic* Bag = UMaterialInstanceDynamic::Create(Solid, this);
                Bag->SetVectorParameterValue(TEXT("Color"), FMath::Lerp(Color, FLinearColor(.45f, .3f, .15f), .5f));
                Show->SetMaterial(0, Bag);
            }
        }
    }
    // o efeito na cor da raridade: a coluna de luz, o círculo animado no chão, os cubinhos subindo e uma luz
    const float Tall = Def->Rarity == TEXT("common") ? 160.f : 320.f + 40.f * FMath::Max(0, int32(Def->Level / 25));
    Beam = Part(Basic(TEXT("Cylinder")));
    BeamWidth = .13f + Rank * .02f;   // fina: larga, escondia o item
    Beam->SetWorldScale3D(FVector(BeamWidth, BeamWidth, Tall / 100.f));
    Beam->SetMaterial(0, Fx(TEXT("M_DropBeam"), 1.5f + Rank * .2f));
    Ring = Part(Basic(TEXT("Plane")));
    RingSize = 1.5f + Rank * .12f;
    Ring->SetMaterial(0, Fx(TEXT("M_DropRing"), 1.f + Rank * .15f));
    Motes = NewObject<UInstancedStaticMeshComponent>(this);
    Motes->SetupAttachment(RootComponent);
    Motes->SetUsingAbsoluteLocation(true);
    Motes->SetUsingAbsoluteRotation(true);
    Motes->SetUsingAbsoluteScale(true);
    Motes->SetStaticMesh(Basic(TEXT("Cube")));
    Motes->SetMaterial(0, Fx(TEXT("M_DropMote"), 1.2f + Rank * .15f));
    Motes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Motes->SetCastShadow(false);
    Motes->RegisterComponent();
    Motes->SetWorldLocation(FVector(Landing));
    TArray<FTransform> None;
    None.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(.001f)), 6 + Rank * 2);
    Motes->AddInstances(None, false);
    Ring->SetCastShadow(false);
    Beam->SetCastShadow(false);
    Light = NewObject<UPointLightComponent>(this);
    Light->SetupAttachment(RootComponent);
    Light->SetUsingAbsoluteLocation(true);
    Light->SetLightColor(Color);
    Light->SetIntensity(700.f);   // colada no item: 2500 estourava o modelo em branco
    Light->SetAttenuationRadius(320.f);
    Light->SetCastShadows(false);
    Light->RegisterComponent();
    // "[E] pegar" perto (só para quem está perto: cada máquina olha o seu herói)
    Label = NewObject<UTextRenderComponent>(this);
    Label->SetupAttachment(RootComponent);
    Label->SetUsingAbsoluteLocation(true);
    Label->SetUsingAbsoluteRotation(true);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(16.f);
    Label->SetTextRenderColor(Color.ToFColor(true));
    Label->SetText(FText::FromString(FString::Printf(TEXT("[E] pegar %s%s%s"), *Def->Name,
        Def->ReqLevel > 0 ? *FString::Printf(TEXT(" (nv. %d)"), Def->Level) : TEXT(""), Count > 1 ? *FString::Printf(TEXT(" x%d"), Count) : TEXT(""))));
    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label->SetVisibility(false);
    Label->RegisterComponent();
}

void AVortexItemDrop::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    if (HasAuthority() && Age > LifeSeconds) { Destroy(); return; }
    if (!bBuilt) return;
    // sai pulando do bicho (em arco) e depois gira flutuando em cima do anel
    const float Fly = FMath::Clamp(Age / FlySeconds, 0.f, 1.f);
    const FVector Land = Landing;
    FVector At = FMath::Lerp(Start, Land + FVector(0, 0, Hover), Fly) + FVector(0, 0, 140.f * FMath::Sin(Fly * PI));
    if (Fly >= 1.f) At = Land + FVector(0, 0, Hover + 10.f * FMath::Sin(Age * 2.2f + Offset));
    const FRotator Spin(Fly < 1.f ? Age * 720.f : 0.f, Age * 110.f + Offset * 30.f, 0.f);
    if (Show) Show->SetWorldLocationAndRotation(At, Spin);
    if (ShowPiece && ShowPiece->GetSkeletalMeshAsset())
    {
        // a peça de armadura fica no lugar dela no corpo: o meio dela vai para o ponto que gira
        const FVector Center = ShowPiece->GetSkeletalMeshAsset()->GetBounds().Origin;
        ShowPiece->SetWorldLocationAndRotation(At - Spin.RotateVector(Center * .9f), Spin);
        ShowPiece->SetWorldScale3D(FVector(.9f));
    }
    const float Show01 = FMath::Clamp((Age - FlySeconds) / .3f, 0.f, 1.f);
    const float Pulse = .85f + .15f * FMath::Sin(Age * 3.f + Offset);
    if (Beam)
    {
        const float Tall = Beam->GetComponentScale().Z * 100.f;
        Beam->SetWorldLocation(Land + FVector(0, 0, Tall * .5f));
        Beam->SetWorldScale3D(FVector(BeamWidth * Show01, BeamWidth * Show01, Beam->GetComponentScale().Z));
        Beam->SetVisibility(Show01 > 0.f);
    }
    if (Ring)
    {
        // ao cair, o círculo abre passando um pouco do tamanho
        const float Open = RingSize * Show01 * (1.f + .3f * FMath::Sin(Show01 * PI));
        Ring->SetWorldLocation(Land + FVector(0, 0, 4.f));
        Ring->SetWorldScale3D(FVector(FMath::Max(Open, .01f), FMath::Max(Open, .01f), 1.f));
    }
    if (Motes)
    {
        // os cubinhos sobem em espiral, nascem e somem encolhendo; longe de quem joga, não atualiza
        const APlayerController* Viewer = GetWorld()->GetFirstPlayerController();
        const bool bClose = Viewer && Viewer->GetPawn() && FVector::DistSquared(Viewer->GetPawn()->GetActorLocation(), Land) < FMath::Square(4000.f);
        Motes->SetVisibility(bClose && Show01 > 0.f);
        if (bClose)
        {
            const int32 N = Motes->GetInstanceCount();
            TArray<FTransform> Now;
            Now.Reserve(N);
            for (int32 i = 0; i < N; ++i)
            {
                const float Phase = FMath::Frac(Age * .3f + i * .618f);
                const float Angle = i * 2.4f + Age * .8f;
                const float Radius = (22.f + (i * 17 % 34)) * (1.f - Phase * .45f);
                const float Size = .055f * FMath::Sin(Phase * PI) * Show01;
                Now.Add(FTransform(FRotator(Age * 90.f + i * 40.f, Age * 70.f, 0.f).Quaternion(), Land + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 8.f + Phase * 170.f), FVector(FMath::Max(Size, .001f))));
            }
            Motes->BatchUpdateInstancesTransforms(0, Now, true, true, true);
        }
    }
    if (Light) { Light->SetWorldLocation(At + FVector(0, 0, 20.f)); Light->SetIntensity(700.f * Pulse * FMath::Max(.3f, Show01)); }
    if (Label)
    {
        const APlayerController* Pc = GetWorld()->GetFirstPlayerController();
        const APawn* Pawn = Pc ? Pc->GetPawn() : nullptr;
        const bool bNear = Pawn && FVector::Dist2D(Pawn->GetActorLocation(), Land) < 260.f;
        Label->SetVisibility(bNear);
        if (bNear)
        {
            Label->SetWorldLocation(Land + FVector(0, 0, Hover + 70.f));
            if (Pc->PlayerCameraManager) Label->SetWorldRotation(FRotator(0.f, (Pc->PlayerCameraManager->GetCameraLocation() - Land).Rotation().Yaw, 0.f));
        }
    }
}

void AVortexItemDrop::Take(int32 Taken)
{
    if (!HasAuthority()) return;
    Count -= FMath::Max(0, Taken);
    if (Count <= 0) Destroy();
}
