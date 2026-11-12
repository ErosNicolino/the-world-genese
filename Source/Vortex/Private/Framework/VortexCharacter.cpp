#include "Framework/VortexCharacter.h"
#include "Misc/PackageName.h"

#include "Components/WindDirectionalSourceComponent.h"
#include "Engine/WindDirectionalSource.h"
#include "EngineUtils.h"
#include "Rpg/VortexArmor.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Framework/VortexProbe.h"
#include "Creatures/VortexMonster.h"
#include "World/VortexForage.h"
#include "World/VortexArsenal.h"
#include "World/VortexItemDrop.h"
#include "Rpg/VortexSocial.h"
#include "Framework/VortexWetness.h"
#include "Framework/VortexSocialUi.h"
#include "World/VortexWorldFeatures.h"
#include "World/VortexPointsOfInterest.h"
#include "World/VortexPortal.h"
#include "World/VortexDungeons.h"
#include "Rpg/VortexRpg.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexTown.h"
#include "Framework/VortexHeroWindow.h"
#include "Framework/VortexGameInstance.h"
#include "Framework/VortexDevChat.h"
#include "Framework/VortexShopWindow.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexQuestLog.h"
#include "Combat/VortexClassSkills.h"
#include "Framework/VortexAwakening.h"
#include "Creatures/VortexCreature.h"
#include "EngineUtils.h"
#include "Framework/VortexDialogue.h"
#include "Creatures/VortexNpc.h"
#include "World/VortexAwakeningStadium.h"
#include "Framework/VortexGraphics.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexHudBase.h"
#include "Framework/VortexPings.h"
#include "GameFramework/PlayerState.h"
#include "Combat/VortexHitFx.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Net/UnrealNetwork.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Framework/VortexHairComponent.h"
#include "Framework/VortexPieces.h"
#include "Framework/VortexFaceStyles.h"

namespace VortexGraphics { float FirstPersonFieldOfView(); }   // VortexGraphics.cpp (fora do .h: compila menos)
#include "GameFramework/GameStateBase.h"
#include "Engine/Texture2D.h"
#include "Framework/VortexPlayerSettings.h"

#include "Framework/VortexCharacterMovementComponent.h"
#include "Framework/VortexCameraBoom.h"
#include "Animation/VortexAnimInstance.h"
#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexRecoverAbility.h"
#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Combat/VortexWeaponKit.h"
#include "Framework/VortexSpellChoice.h"
#include "Combat/VortexProjectile.h"
#include "Combat/VortexWeaponFx.h"
#include "Combat/VortexWeaponParts.h"
#include "Engine/StaticMesh.h"
#include "Engine/OverlapResult.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "CollisionQueryParams.h"

AVortexCharacter::AVortexCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UVortexCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
    PrimaryActorTick.bCanEverTick = true;
    // sempre ligado: os sons (passos, rolamento, golpes) e a transição da câmera vivem no Tick
    PrimaryActorTick.bStartWithTickEnabled = true;
    GetCapsuleComponent()->InitCapsuleSize(HeroCapsuleRadius, HeroCapsuleHalfHeight);
    bReplicates = true;
    SetReplicateMovement(true);
    bUseControllerRotationYaw = false;
    AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    AbilitySystem->SetIsReplicated(true);
    AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
    Vitals = CreateDefaultSubobject<UVortexAttributes>(TEXT("Vitals"));
    Sheet = CreateDefaultSubobject<UVortexHeroSheet>(TEXT("Sheet"));
    QuestLog = CreateDefaultSubobject<UVortexQuestLog>(TEXT("QuestLog"));
    ClassSkills = CreateDefaultSubobject<UVortexClassSkills>(TEXT("ClassSkills"));
    Social = CreateDefaultSubobject<UVortexSocial>(TEXT("Social"));
    Combat = CreateDefaultSubobject<UVortexCombatComponent>(TEXT("Combat"));
    Weapon = CreateDefaultSubobject<UVortexWeaponComponent>(TEXT("Weapon"));

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = true;
    Movement->RotationRate = FRotator(0.f, 540.f, 0.f);
    Movement->MaxWalkSpeed = 460.f;
    Movement->MaxAcceleration = 4500.f;
    Movement->BrakingDecelerationWalking = 4500.f;
    Movement->GroundFriction = 8.f;
    Movement->MaxStepHeight = 45.f;
    Movement->SetWalkableFloorAngle(45.f);
    Movement->JumpZVelocity = 800.f;
    Movement->GravityScale = 2.45f;
    Movement->AirControl = 0.3f;

    CameraBoom = CreateDefaultSubobject<UVortexCameraBoom>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetCapsuleComponent());
    CameraBoom->TargetArmLength = WantedCameraDistance;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
    CameraBoom->SocketOffset = FVector(0.f, 34.f, 29.f);
    // segue o herói com um atraso curto (suaviza paradas, pulos e degraus); a rotação continua direta no mouse
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 12.f;
    CameraBoom->CameraLagMaxDistance = 60.f;
    CameraBoom->bEnableCameraRotationLag = false;
    CameraBoom->bDoCollisionTest = true;
    CameraBoom->ProbeSize = 24.f;
    CameraBoom->ProbeChannel = ECC_Camera;

    // a câmera de quem joga atravessa personagens (sem isto ela pulava para dentro do herói ao encostar)
    GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom);
    FollowCamera->bUsePawnControlRotation = false;
    FollowCamera->FieldOfView = 75.f;

    DebugBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugBody"));
    DebugBody->SetupAttachment(GetCapsuleComponent());
    DebugBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DebugBody->SetRelativeScale3D(FVector(0.5, 0.5, 1.4));
    WeaponVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponVisual"));
    // Same bone and local geometry as the Godot embedded sword. Its animation can hide the
    // original by scaling the bone to zero; absolute component scale keeps this visual visible.
    WeaponVisual->SetupAttachment(GetMesh(), TEXT("Sword"));
    WeaponVisual->SetAbsolute(false, false, true);
    WeaponVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponVisual->SetReceivesDecals(false);
    // arma pequena e sempre em movimento: fora do campo de distância e da cena do Lumen (custava atualização todo quadro)
    WeaponVisual->bAffectDistanceFieldLighting = false;
    WeaponVisual->bAffectDynamicIndirectLighting = false;
    // 01/11: sem sombra: as armas do Meshy têm milhares de faces pequenas e a sombra dela sobre ela mesma pintava
    // triângulos pretos (sem as sombras a arma fica igual ao Blender)
    WeaponVisual->SetCastShadow(false);
    WeaponVisual->SetVisibility(false);
    WeaponVisualLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponVisualLeft"));
    WeaponVisualLeft->SetupAttachment(GetMesh(), TEXT("Hand_L"));
    WeaponVisualLeft->SetAbsolute(false, false, true);
    WeaponVisualLeft->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponVisualLeft->SetReceivesDecals(false);
    // arma pequena e sempre em movimento: fora do campo de distância e da cena do Lumen (custava atualização todo quadro)
    WeaponVisualLeft->bAffectDistanceFieldLighting = false;
    WeaponVisualLeft->bAffectDynamicIndirectLighting = false;
    WeaponVisualLeft->SetCastShadow(false);
    WeaponVisualLeft->SetVisibility(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        DebugBody->SetStaticMesh(CylinderMesh.Object);
    }
    // o aventureiro do Godot (Tools/ConvertGodotAdventurer.py + ImportAdventurer.py), animado em C++
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> AdventurerMesh(
        TEXT("/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned.Adventurer_Skinned"));
    if (AdventurerMesh.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(AdventurerMesh.Object);
        WeaponVisual->SetRelativeTransform(FTransform::Identity);
        GetMesh()->SetAnimInstanceClass(UVortexAnimInstance::StaticClass());
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -HeroCapsuleHalfHeight));
        GetMesh()->SetRelativeRotation(FRotator(0.f, MeshYawOffset, 0.f));
        // Fora da tela (e no servidor dedicado) a pose não é calculada; o herói controlado aqui
        // volta a animar sempre em NotifyControllerChanged.
        GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
        DebugBody->SetVisibility(false);
    }
}

void AVortexCharacter::NotifyControllerChanged()
{
    Super::NotifyControllerChanged();
    if (AbilitySystem && IsLocallyControlled()) AbilitySystem->InitAbilityActorInfo(this, this);
    ApplyAnimTickOption();
}

void AVortexCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    AbilitySystem->InitAbilityActorInfo(this, this);
}

UAbilitySystemComponent* AVortexCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystem;
}

void AVortexCharacter::ApplyAnimTickOption()
{
    // Com peças modulares a malha base fica fora do desenho e só serve de pose para as peças:
    // ela precisa animar sempre, senão as peças congelam.
    const bool bAlways = (IsLocallyControlled() && IsPlayerControlled()) || !WardrobeParts.IsEmpty();
    GetMesh()->VisibilityBasedAnimTickOption = bAlways
        ? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones
        : EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
}

void AVortexCharacter::CycleWardrobe(int32 CategoryIndex)
{
    static const FName Categories[] = { TEXT("Hair"), TEXT("Eyes"), TEXT("Mouth"), TEXT("Shirt"), TEXT("Pants"), TEXT("Shoes"), TEXT("Gloves") };
    static const FName Options[][2] = {
        { TEXT("hair_male_1"), TEXT("hair_male_2") }, { TEXT("eyes_m_calm"), TEXT("eyes_m_fierce") },
        { TEXT("mouth_small"), TEXT("mouth_smile") }, { TEXT("shirt_tunic"), TEXT("shirt_punk") },
        { TEXT("pants_dark"), TEXT("pants_jeans_punk") }, { TEXT("boots_laced"), TEXT("boots_punk") },
        { TEXT("gloves_leather"), TEXT("gloves_punk") },
    };
    if (CategoryIndex < 0 || CategoryIndex >= UE_ARRAY_COUNT(Categories)) return;
    FName Current = Options[CategoryIndex][0];
    for (USkeletalMeshComponent* Part : WardrobeParts)
    {
        if (Part && Part->GetFName() == Categories[CategoryIndex] && Part->GetSkeletalMeshAsset())
        {
            Current = Part->GetSkeletalMeshAsset()->GetFName();
        }
    }
    // alterna entre as duas opções (sem peças montadas, a atual é a padrão)
    const FName Next = Current == Options[CategoryIndex][1] ? Options[CategoryIndex][0] : Options[CategoryIndex][1];
    const bool bOk = SetWardrobePiece(Categories[CategoryIndex], Next);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_WARDROBE %s -> %s %s"), *Categories[CategoryIndex].ToString(), *Next.ToString(), bOk ? TEXT("ok") : TEXT("falhou"));
}

void AVortexCharacter::LookAtFace()
{
    if (AController* C = GetController())
    {
        C->SetControlRotation(FRotator(-8.f, GetActorRotation().Yaw + 180.f, 0.f));
    }
}

bool AVortexCharacter::EnsureWardrobeParts()
{
    if (!WardrobeParts.IsEmpty()) return true;
    if (!GetMesh()->GetSkeletalMeshAsset()) return false;
    const TPair<FName, FName> Defaults[] = {
        {TEXT("Core"), TEXT("core")}, {TEXT("Face"), TEXT("face_male")},
        {TEXT("Hair"), TEXT("hair_male_1")}, {TEXT("Eyes"), TEXT("eyes_m_calm")},
        {TEXT("Mouth"), TEXT("mouth_small")}, {TEXT("Shirt"), TEXT("shirt_tunic")},
        {TEXT("Pants"), TEXT("pants_dark")}, {TEXT("Shoes"), TEXT("boots_laced")},
        {TEXT("Gloves"), TEXT("gloves_leather")}, {TEXT("GearStrap"), TEXT("gear_strap")},
        {TEXT("GearBelt"), TEXT("gear_belt")}, {TEXT("Race"), NAME_None}, {TEXT("Accessory"), NAME_None},
        {TEXT("AccessoryFace"), NAME_None}, {TEXT("AccessoryBack"), NAME_None},
    };
    TArray<TPair<FName, USkeletalMesh*>> Loaded;
    for (const TPair<FName, FName>& Entry : Defaults)
    {
        if (Entry.Value.IsNone())
        {
            Loaded.Emplace(Entry.Key, nullptr);   // espaço vazio (traço da raça, cosméticos)
            continue;
        }
        const FString Id = Entry.Value.ToString();
        const FString Path = FString::Printf(TEXT("/Game/Vortex/Characters/Adventurer/Wardrobe/%s/SkeletalMeshes/%s.%s"), *Id, *Id, *Id);
        USkeletalMesh* PieceMesh = LoadObject<USkeletalMesh>(nullptr, *Path);
        if (!PieceMesh || PieceMesh->GetSkeleton() != GetMesh()->GetSkeletalMeshAsset()->GetSkeleton()) return false;
        Loaded.Emplace(Entry.Key, PieceMesh);
    }
    for (const TPair<FName, USkeletalMesh*>& Entry : Loaded)
    {
        USkeletalMeshComponent* Part = NewObject<USkeletalMeshComponent>(this, Entry.Key);
        Part->SetupAttachment(GetMesh());
        Part->SetSkeletalMesh(Entry.Value);
        Part->SetLeaderPoseComponent(GetMesh());
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetReceivesDecals(false);
        Part->bUseBoundsFromLeaderPoseComponent = true;
        Part->SetMaterial(0, GetMesh()->GetMaterial(0));
        Part->RegisterComponent();
        WardrobeParts.Add(Part);
    }
    // A malha original avalia a animação; as peças visíveis copiam sua pose.
    // Escondida de verdade: só tirar do passo principal ainda a desenhava na profundidade, e a
    // peça antiga aparecia como silhueta escura por baixo e por cima da nova. Escondida, ela
    // continua animando (AlwaysTickPoseAndRefreshBones em ApplyAnimTickOption).
    GetMesh()->SetVisibility(false, false);
    GetMesh()->SetCastShadow(false);
    ApplyAnimTickOption();
    return true;
}

bool AVortexCharacter::SetPartMesh(FName PartName, FName PieceId)
{
    USkeletalMeshComponent* Part = nullptr;
    for (USkeletalMeshComponent* Candidate : WardrobeParts)
    {
        if (Candidate && Candidate->GetFName() == PartName) Part = Candidate;
    }
    if (!Part) return false;
    USkeletalMesh* PieceMesh = nullptr;
    if (!PieceId.IsNone() && !PieceId.ToString().EndsWith(TEXT("_none")))
    {
        const FString Id = PieceId.ToString();
        const FString Path = FString::Printf(TEXT("/Game/Vortex/Characters/Adventurer/Wardrobe/%s/SkeletalMeshes/%s.%s"), *Id, *Id, *Id);
        PieceMesh = LoadObject<USkeletalMesh>(nullptr, *Path);
        if (!PieceMesh || PieceMesh->GetSkeleton() != GetMesh()->GetSkeletalMeshAsset()->GetSkeleton())
        {
            UE_LOG(LogTemp, Warning, TEXT("VORTEX_LOOK peca ausente %s"), *Id);
            PieceMesh = nullptr;
        }
    }
    Part->SetSkeletalMesh(PieceMesh);
    Part->SetVisibility(PieceMesh != nullptr);
    Part->SetMaterial(0, LookMaterial ? static_cast<UMaterialInterface*>(LookMaterial) : GetMesh()->GetMaterial(0));
    return PieceMesh != nullptr;
}

namespace
{
    /** Onde a câmera de terceira pessoa fica em relação ao pivô (no peito), pela distância do zoom. Andando e
     *  lutando: quase atrás do herói e acima, olhando um pouco para baixo (como os jogos de ação com espada); a
     *  câmera de ombro (bem para o lado, na altura do peito) ficou só para a mira (segurar F). */
    FVector CameraOffsetFor(float Distance, float ShoulderSign, bool bAim)
    {
        const float Alpha = FMath::Clamp((Distance - 150.f) / 650.f, 0.f, 1.f);
        if (bAim) return FVector(0.f, ShoulderSign * FMath::Lerp(70.f, 140.f, Alpha), FMath::Lerp(-10.f, 55.f, Alpha));
        return FVector(0.f, ShoulderSign * FMath::Lerp(28.f, 45.f, Alpha), FMath::Lerp(12.f, 60.f, Alpha));
    }

    FVortexNetLook ToNet(const FVortexLook& Look)
    {
        FVortexNetLook Net;
        Net.Race = Look.Race;
        Net.Appearance = Look.Appearance;
        for (const TPair<FName, FName>& Piece : Look.Pieces) { Net.Categories.Add(Piece.Key); Net.Pieces.Add(Piece.Value); }
        for (const TPair<FName, FColor>& Color : Look.Colors) { Net.ColorSlots.Add(Color.Key); Net.Colors.Add(Color.Value); }
        return Net;
    }

    FVortexLook FromNet(const FVortexNetLook& Net)
    {
        FVortexLook Look;
        Look.Race = Net.Race;
        Look.Appearance = Net.Appearance;
        for (int32 i = 0; i < Net.Categories.Num() && i < Net.Pieces.Num(); ++i) Look.Pieces.Add(Net.Categories[i], Net.Pieces[i]);
        for (int32 i = 0; i < Net.ColorSlots.Num() && i < Net.Colors.Num(); ++i) Look.Colors.Add(Net.ColorSlots[i], Net.Colors[i]);
        return Look;
    }
}

FVortexNetLook AVortexCharacter::LookToNet(const FVortexLook& Look) { return ToNet(Look); }
FVortexLook AVortexCharacter::LookFromNet(const FVortexNetLook& Net) { return FromNet(Net); }

void AVortexCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AVortexCharacter, NetLook);
    DOREPLIFETIME(AVortexCharacter, Mount);
    DOREPLIFETIME(AVortexCharacter, Emote);
}

void AVortexCharacter::ApplyLook(const FVortexLook& InLook)
{
    ApplyLookLocal(InLook);
    // o herói controlado aqui manda o visual para o servidor, que repassa aos outros jogadores
    if (IsLocallyControlled() && IsPlayerControlled())
    {
        const FVortexNetLook Net = ToNet(CurrentLook);
        if (HasAuthority())
        {
            NetLook = Net;
            if (Sheet) Sheet->RefreshVitals();
        }
        else ServerSetLook(Net);
    }
}

bool AVortexCharacter::ServerSetLook_Validate(const FVortexNetLook& Look)
{
    return Look.Categories.Num() <= 32 && Look.Pieces.Num() <= 32 && Look.ColorSlots.Num() <= 32 && Look.Colors.Num() <= 32;   // 09/11: mais 6 espaços de roupa
}

void AVortexCharacter::ServerSetLook_Implementation(const FVortexNetLook& Look)
{
    if (!NetLook.Race.IsNone() && Look.Race != NetLook.Race) return;   // 09/11: a raça dá atributos: só na primeira vez
    NetLook = Look;
    ApplyLookLocal(FromNet(Look));   // o servidor também usa o tamanho da raça na colisão
    if (Sheet) Sheet->RefreshVitals();   // os atributos da raça mudam vida e mana
}

void AVortexCharacter::OnRep_NetLook()
{
    if (!IsLocallyControlled()) ApplyLookLocal(FromNet(NetLook));
}

// Tamanho e proporção da raça: o modelo (altura e largura), os ossos (na animação) e, no jogo, a
// cápsula de colisão e a altura da câmera acompanham a altura do corpo.
void AVortexCharacter::ApplyBodyShape(const FVortexBodyShape& Shape)
{
    BodyShape = Shape;
    GetMesh()->SetRelativeScale3D(FVector(Shape.Width, Shape.Width, Shape.Height));
    if (UVortexAnimInstance* Anim = Cast<UVortexAnimInstance>(GetMesh()->GetAnimInstance())) Anim->Body = Shape;
    const float Stature = Shape.Stature();
    const float HalfHeight = HeroCapsuleHalfHeight * Stature;
    GetCapsuleComponent()->SetCapsuleSize(FMath::Clamp(HeroCapsuleRadius * FMath::Max(Shape.Width, .85f), 18.f, 45.f), HalfHeight);
    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -HalfHeight + MountLift));
    // 02/11: nos outros jogos (cópia simulada) a suavização da rede repõe a malha na posição guardada a cada quadro: sem
    // guardar a nova, o amigo montado aparecia em pé no chão, dentro do cavalo
    CacheInitialMeshOffset(GetMesh()->GetRelativeLocation(), GetBaseRotationOffset().Rotator());
    if (CameraBoom && !bGodotBenchCamera)
    {
        // a câmera mira mais baixo no anão e mais alto no bárbaro (e sobe com ele na sela)
        CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 45.f * Stature + MountLift));
    }
}

FVortexLook AVortexCharacter::WithArmor(const FVortexLook& InLook) const
{
    // a armadura vestida troca as peças (chapéu/elmo, camisa, calça, botas) e as cores do conjunto
    FVortexLook Out = InLook;
    if (!Sheet) return Out;
    for (int32 Slot = 0; Slot < VortexRpg::NumEquip; ++Slot)
    {
        if (Slot == VortexRpg::Weapon) continue;
        const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Sheet->GetEquipped(Slot));
        if (!Def) continue;
        for (const TPair<FName, FName>& Piece : Def->Look) Out.Pieces.Add(Piece.Key, Piece.Value);
        for (const TPair<FName, FColor>& Color : Def->LookColors)
        {
            // cada peça pinta só a sua cor (a camisa não muda a cor da calça)
            const FName Category = Color.Key == TEXT("shirt_color") ? FName(TEXT("shirt")) : Color.Key == TEXT("pants_color") ? FName(TEXT("pants"))
                : Color.Key == TEXT("boots_color") ? FName(TEXT("shoes")) : NAME_None;
            if (Category.IsNone() || Def->Look.Contains(Category)) Out.Colors.Add(Color.Key, Color.Value);
        }
    }
    return Out;
}

void AVortexCharacter::RefreshArmorLook()
{
    // trocou a armadura: refaz o visual a partir do visual do herói (sem armadura)
    if (!Sheet) return;
    uint32 Signature = 0;
    for (int32 Slot = 0; Slot < VortexRpg::NumEquip; ++Slot) if (Slot != VortexRpg::Weapon) Signature = HashCombine(Signature, GetTypeHash(Sheet->GetEquipped(Slot)));
    if (Signature == ArmorSignature) return;
    ArmorSignature = Signature;
    if (bHasBaseLook) ApplyLookLocal(BaseLook);
}

namespace
{
    // 07/11: as peças de armadura de blocos (Data/Rpg/armors.json, Tools/ImportBlockArmor.py): um componente por lugar
    const TPair<int32, const TCHAR*> ArmorSlots[] = { { VortexRpg::Head, TEXT("BlockArmor_head") }, { VortexRpg::Chest, TEXT("BlockArmor_chest") },
        { VortexRpg::Legs, TEXT("BlockArmor_legs") }, { VortexRpg::Boots, TEXT("BlockArmor_boots") } };

    USkeletalMeshComponent* ArmorPart(AActor* Hero, const TCHAR* Name)
    {
        TArray<USkeletalMeshComponent*> Parts;
        Hero->GetComponents(Parts);
        for (USkeletalMeshComponent* Part : Parts) if (Part->GetFName() == Name) return Part;
        return nullptr;
    }

    // 09/11: as roupas separadas, um componente por espaço; Armor = o lugar da armadura que as cobre (-1 = nenhum)
    struct FWearSlot { const TCHAR* Look; const TCHAR* Part; int32 Armor; bool bHead; };
    const FWearSlot WearSlots[] = {
        { TEXT("peca_camisa"), TEXT("WearPiece_camisa"), VortexRpg::Chest, false }, { TEXT("peca_cima"), TEXT("WearPiece_cima"), VortexRpg::Chest, false },
        { TEXT("peca_calca"), TEXT("WearPiece_calca"), VortexRpg::Legs, false }, { TEXT("peca_calcado"), TEXT("WearPiece_calcado"), VortexRpg::Boots, false },
        { TEXT("peca_chapeu"), TEXT("WearPiece_chapeu"), VortexRpg::Head, true }, { TEXT("peca_acessorio"), TEXT("WearPiece_acessorio"), -1, true },
        { TEXT("peca_orelha"), TEXT("WearPiece_orelha"), VortexRpg::Head, true },
        { TEXT("peca_cintura"), TEXT("WearPiece_cintura"), -1, false }, { TEXT("peca_bracos"), TEXT("WearPiece_bracos"), -1, false } };   // 09/11: as orelhas do elfo

    /** A malha do pano simulado, ao lado da malha da peça: "<pasta>/X.X" -> "<pasta>/X_tecido.X_tecido". */
    USkeletalMesh* ClothMeshOf(const FString& Mesh)
    {
        FString Package, Name;
        return Mesh.Split(TEXT("."), &Package, &Name) ? LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%s_tecido.%s_tecido"), *Package, *Name)) : nullptr;
    }

    /** O pano só balança com vento se o mundo tiver uma fonte; uma por mundo. */
    void EnsureClothWind(UWorld* World)
    {
        if (!World || World->GetNetMode() == NM_DedicatedServer || TActorIterator<AWindDirectionalSource>(World)) return;
        if (AWindDirectionalSource* Wind = World->SpawnActor<AWindDirectionalSource>(FVector::ZeroVector, FRotator(0.f, 35.f, 0.f)))
        {
            Wind->GetComponent()->SetStrength(.35f);
            Wind->GetComponent()->SetSpeed(.5f);
        }
    }

    bool WearsHelmet(const UVortexHeroSheet* Sheet)
    {
        return Sheet && VortexArmor::VisualPiece(Sheet->GetEquipped(VortexRpg::Head)) != nullptr;
    }

    /** Quem joga: o capacete some na primeira pessoa (a câmera fica dentro da cabeça); tudo some com a câmera dentro do corpo. */
    void SyncArmorView(AActor* Hero, bool bHideHead, bool bHideAll)
    {
        for (const TPair<int32, const TCHAR*>& Slot : ArmorSlots)
            if (USkeletalMeshComponent* Part = ArmorPart(Hero, Slot.Value))
                Part->SetOwnerNoSee(bHideAll || (bHideHead && Slot.Key == VortexRpg::Head));
        for (const FWearSlot& Slot : WearSlots)
            if (USkeletalMeshComponent* Part = ArmorPart(Hero, Slot.Part))
                Part->SetOwnerNoSee(bHideAll || (bHideHead && Slot.bHead));
        for (const FWearSlot& Slot : WearSlots)
            if (USkeletalMeshComponent* Part = ArmorPart(Hero, *(FString(Slot.Part) + TEXT("_tecido"))))
                Part->SetOwnerNoSee(bHideAll || (bHideHead && Slot.bHead));
        if (USkeletalMeshComponent* Part = ArmorPart(Hero, TEXT("BlockOutfit_tecido"))) Part->SetOwnerNoSee(bHideAll);
        if (USkeletalMeshComponent* Part = ArmorPart(Hero, TEXT("ArmorPiece_tecido"))) Part->SetOwnerNoSee(bHideAll);
    }

    /** Veste uma malha num componente nomeado do herói, seguindo a pose do corpo (Mesh nulo = o componente some). */
    void DressPart(AActor* Hero, const TCHAR* Name, USkeletalMeshComponent* Body, USkeletalMesh* Mesh, const FLinearColor& Hair, bool bNoSee)
    {
        USkeletalMeshComponent* Part = ArmorPart(Hero, Name);
        if (!Part && Mesh)
        {
            Part = NewObject<USkeletalMeshComponent>(Hero, Name);
            Part->SetupAttachment(Body);
            Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Part->SetGenerateOverlapEvents(false);
            Part->bUseBoundsFromLeaderPoseComponent = true;
            Part->bCastHiddenShadow = true;
            Part->RegisterComponent();
        }
        if (!Part) return;
        if (Part->GetSkeletalMeshAsset() != Mesh)
        {
            Part->SetSkeletalMesh(Mesh);
            Part->EmptyOverrideMaterials();   // o material da peça anterior ficava preso (a mesma pintura em todas)
        }
        Part->SetLeaderPoseComponent(Body);
        Part->SetVisibility(Mesh != nullptr);
        Part->SetOwnerNoSee(Body->bOwnerNoSee || bNoSee);
        // a franja dos capuzes pega a cor do cabelo do herói (M_ArmorBlock: HairTint): um material novo da peça desta
        if (Mesh && Mesh->GetMaterials().Num())
            if (UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Mesh->GetMaterials()[0].MaterialInterface, Part))
            {
                Material->SetVectorParameterValue(TEXT("HairTint"), Hair);
                for (int32 m = 0; m < Part->GetNumMaterials(); ++m) Part->SetMaterial(m, Material);
            }
    }

    void ApplyArmorParts(AActor* Hero, const UVortexHeroSheet* Sheet, USkeletalMeshComponent* Body, const FLinearColor& Hair, bool bHideHead)
    {
        for (const TPair<int32, const TCHAR*>& Slot : ArmorSlots)
        {
            const VortexArmor::FPiece* Piece = Sheet ? VortexArmor::VisualPiece(Sheet->GetEquipped(Slot.Key)) : nullptr;
            USkeletalMesh* Mesh = Piece ? LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/humano/Armaduras/%s.%s"),
                *Piece->Mesh, *Piece->Mesh)) : nullptr;
            DressPart(Hero, Slot.Value, Body, Mesh, Hair, bHideHead && Slot.Key == VortexRpg::Head);
            if (Slot.Key == VortexRpg::Chest)
            {
                // 09/11: o tabardo em tecido real, quando a armadura tem (A_<...>_tecido); nem toda tem, então pergunta antes de carregar
                const FString Package = Piece ? FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/humano/Armaduras/%s_tecido"), *Piece->Mesh) : FString();
                USkeletalMesh* Cloth = Piece && FPackageName::DoesPackageExist(Package) ? LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%s.%s_tecido"), *Package, *Piece->Mesh)) : nullptr;
                DressPart(Hero, TEXT("ArmorPiece_tecido"), Body, Cloth, Hair, false);
                if (Cloth) EnsureClothWind(Hero->GetWorld());
            }
        }
    }

    /** As roupas separadas (Data/Looks/pecas.json); a armadura vestida no mesmo lugar fica no lugar delas. */
    void ApplyWearParts(AActor* Hero, const UVortexHeroSheet* Sheet, USkeletalMeshComponent* Body, const FVortexLook& Look, const FLinearColor& Hair, bool bHideHead)
    {
        for (const FWearSlot& Slot : WearSlots)
        {
            const FVortexWearPiece* Piece = VortexWearPiece(Look.Pieces.FindRef(Slot.Look));
            const bool bCovered = Sheet && Slot.Armor >= 0 && VortexArmor::VisualPiece(Sheet->GetEquipped(Slot.Armor));
            // 09/11: cada peça com pano simulado (saia, capa, véu, pingente, bolsa) tem a malha dela ao lado. As orelhas ficam à mostra nos chapéus
            DressPart(Hero, Slot.Part, Body, Piece && !bCovered ? LoadObject<USkeletalMesh>(nullptr, *Piece->Mesh) : nullptr, Hair, bHideHead && Slot.bHead);
            USkeletalMesh* Cloth = Piece && !bCovered && Piece->bCloth ? ClothMeshOf(Piece->Mesh) : nullptr;
            DressPart(Hero, *(FString(Slot.Part) + TEXT("_tecido")), Body, Cloth, Hair, bHideHead && Slot.bHead);
            if (Cloth) EnsureClothWind(Hero->GetWorld());
        }
    }

    /** A armadura ou a roupa separada esconde a região do traje que cobre (MakeBlockOutfit.py: M_Roupa_cabeca/_tronco/_pernas/_pes). */
    void HideCoveredOutfit(USkeletalMeshComponent* Outfit, const UVortexHeroSheet* Sheet, const FVortexLook& Look)
    {
        USkeletalMesh* Mesh = Outfit ? Outfit->GetSkeletalMeshAsset() : nullptr;
        const FSkeletalMeshRenderData* Render = Mesh ? Mesh->GetResourceForRendering() : nullptr;
        if (!Render || Render->LODRenderData.IsEmpty()) return;
        struct FRegion { const TCHAR* Key; int32 Armor; const TCHAR* Wear; };
        static const FRegion Regions[] = { { TEXT("_cabeca"), VortexRpg::Head, TEXT("peca_chapeu") }, { TEXT("_tronco"), VortexRpg::Chest, TEXT("peca_camisa") },
            { TEXT("_pernas"), VortexRpg::Legs, TEXT("peca_calca") }, { TEXT("_pes"), VortexRpg::Boots, TEXT("peca_calcado") } };
        const TArray<FSkeletalMaterial>& Materials = Mesh->GetMaterials();
        const TArray<FSkelMeshRenderSection>& Sections = Render->LODRenderData[0].RenderSections;
        for (int32 Section = 0; Section < Sections.Num(); ++Section)
        {
            const int32 Index = Sections[Section].MaterialIndex;
            if (!Materials.IsValidIndex(Index)) continue;
            const FString Slot = Materials[Index].MaterialSlotName.ToString();
            for (const FRegion& Region : Regions)
                if (Slot.EndsWith(Region.Key))
                    Outfit->ShowMaterialSection(Index, Section, !(Look.Pieces.Contains(Region.Wear) || (Sheet && VortexArmor::VisualPiece(Sheet->GetEquipped(Region.Armor)))), 0);
        }
    }
}

USkeletalMesh* AVortexCharacter::HeroModelFor(FName Race, FName Appearance)
{
    if (Race.IsNone() || Race == TEXT("elfo")) Race = TEXT("humano");   // 09/11: o elfo é o corpo do humano; muda a pele, os olhos e as orelhas
    const FString Id = Race.ToString();
    // 05/11: o corpo feminino é a MESMA malha do masculino (o usuário: "o corpo é idêntico"); muda só a textura (o top,
    // T_<raça>_Pixel_F) e os olhos com cílios, no ApplyHeroModel. A malha à parte (Feminino/Hero_<raça>, com busto e
    // cintura, juntas em outro lugar) quebrava as animações e a pegada do arco.
    // 10/11: agora o Feminino/Hero_<raça> é CÓPIA do masculino (mesmas juntas e pesos), só a cintura mais fina
    // (Tools/Blender/MakeFemaleBody.py), com o mesmo nome (os clipes e o rosto são achados pelo nome da malha)
    if (Appearance == TEXT("female"))
        if (USkeletalMesh* Female = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/%s/Feminino/Hero_%s.Hero_%s"), *Id, *Id, *Id)))
            return Female;
    return LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/%s/Hero_%s.Hero_%s"), *Id, *Id, *Id));
}

void AVortexCharacter::ApplyHeroModel(USkeletalMesh* Model, const FVortexLook& InLookRaw)
{
    const FVortexLookCatalog& Catalog = FVortexLookCatalog::Get();
    BaseLook = InLookRaw;
    bHasBaseLook = true;
    CurrentLook = Catalog.Normalized(InLookRaw);
    for (USkeletalMeshComponent* Part : WardrobeParts)
    {
        if (!Part) continue;
        Part->SetSkeletalMesh(nullptr);
        Part->SetVisibility(false);
    }
    if (GetMesh()->GetSkeletalMeshAsset() != Model)
    {
        GetMesh()->SetSkeletalMesh(Model);
        // o material da paleta do aventureiro (posto no componente) ficava por cima da textura do modelo
        GetMesh()->EmptyOverrideMaterials();
        // a translação dos ossos vem da malha (cada herói com as próprias proporções); a raiz e o quadril, da
        // animação (o pulo, o agachar). Com o aventureiro dá na mesma: as animações dele têm as juntas dele
        if (USkeleton* Skeleton = Model->GetSkeleton())
        {
            const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();
            for (int32 Bone = 0; Bone < Ref.GetNum(); ++Bone)
            {
                const FName Name = Ref.GetBoneName(Bone);
                // a raiz fica parada (as animações do Manny avançam a raiz; aqui quem anda é a cápsula). O quadril do
                // Mixamo (mixamorig:Hips) também vem da animação: o pulo, o agachar e a queda da morte
                const bool bHips = Name == TEXT("Hips") || Name.ToString().EndsWith(TEXT(":Hips")) || Name.ToString().EndsWith(TEXT("_Hips"));
                const EBoneTranslationRetargetingMode::Type Want = Name == TEXT("Root") ? EBoneTranslationRetargetingMode::Skeleton
                    : bHips ? EBoneTranslationRetargetingMode::AnimationScaled : EBoneTranslationRetargetingMode::Skeleton;
                if (Skeleton->GetBoneTranslationRetargetingMode(Bone) != Want) Skeleton->SetBoneTranslationRetargetingMode(Bone, Want);
            }
        }
    }
    // o material do herói (Tools/ImportHeroes.py): só a cor da textura, fosco (o do glTF trazia o mapa de metal, que
    // sem o Lumen deixava o corpo escuro)
    const FString Id = Model->GetName().RightChop(5);   // Hero_<raça>
    // o rosto animado (MI_HeroFace_<raça>, Tools/ImportHeroFace.py), quando a raça tem; senão só a textura
    FaceMaterial = nullptr;
    FaceEye = FaceMouth = -1;
    if (UMaterialInterface* Face = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/%s/MI_HeroFace_%s.MI_HeroFace_%s"), *Id, *Id, *Id)))
    {
        FaceMaterial = UMaterialInstanceDynamic::Create(Face, this);
        for (int32 Slot = 0; Slot < GetMesh()->GetNumMaterials(); ++Slot) GetMesh()->SetMaterial(Slot, FaceMaterial);
        // a versão feminina (o humano de blocos: Tools/MakeBlockFemale.py e MakeBlockFace.py --feminino): a mesma malha e
        // as mesmas animações, com o top na cor do corpo e os olhos com cílios
        if (CurrentLook.Appearance == TEXT("female"))
        {
            if (UTexture* Skin = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/%s/T_%s_Pixel_F.T_%s_Pixel_F"), *Id, *Id, *Id)))
                FaceMaterial->SetTextureParameterValue(TEXT("Base"), Skin);
            if (UTexture* Eyes = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/%s/T_%s_Eyes_F.T_%s_Eyes_F"), *Id, *Id, *Id)))
                FaceMaterial->SetTextureParameterValue(TEXT("Eyes"), Eyes);
        }
        if (CurrentLook.Race == TEXT("elfo"))
        {
            const FString Tone = FString::Printf(TEXT("T_elfo_%s%s"), *CurrentLook.Pieces.FindRef(TEXT("elfo_pele")).ToString(), CurrentLook.Appearance == TEXT("female") ? TEXT("_F") : TEXT(""));
            if (UTexture* Skin = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/humano/Elfo/%s.%s"), *Tone, *Tone)))
                FaceMaterial->SetTextureParameterValue(TEXT("Base"), Skin);
        }
        // o estilo do rosto (Data/Looks/rostos.json, Tools/ImportBlockFaces.py): os olhos do estilo, no corpo escolhido
        const FName Style = CurrentLook.Pieces.FindRef(TEXT("block_face"));
        if (!Style.IsNone() && Style != TEXT("padrao") && VortexFaceStyles().ContainsByPredicate([Style](const FVortexFaceStyle& F) { return F.Id == Style; }))
        {
            const FString Name = FString::Printf(TEXT("T_%s_Eyes_%s%s"), *Id, CurrentLook.Appearance == TEXT("female") ? TEXT("F_") : TEXT(""), *Style.ToString());
            if (UTexture* Eyes = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/%s/%s.%s"), *Id, *Name, *Name)))
                FaceMaterial->SetTextureParameterValue(TEXT("Eyes"), Eyes);
        }
    }
    else if (UMaterialInterface* Skin = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/%s/MI_Hero_%s.MI_Hero_%s"), *Id, *Id, *Id)))
        for (int32 Slot = 0; Slot < GetMesh()->GetNumMaterials(); ++Slot) GetMesh()->SetMaterial(Slot, Skin);
    GetMesh()->SetVisibility(true);
    GetMesh()->SetCastShadow(true);
    ApplyBlockHair();
    ApplyBlockOutfit();
    ApplyArmorParts(this, Sheet, GetMesh(), FLinearColor(CurrentLook.Colors.FindRef(TEXT("hair_color"))), bFirstPerson && IsLocallyControlled());
    ApplyWearParts(this, Sheet, GetMesh(), CurrentLook, FLinearColor(CurrentLook.Colors.FindRef(TEXT("hair_color"))), bFirstPerson && IsLocallyControlled());
    bHeroModel = true;
    if (bFirstPerson) SetViewModelVisible(FirstPersonBlend > .5f);   // trocou de raça em primeira pessoa
    // o modelo já tem as proporções da raça; só o tamanho (o anão menor, o bárbaro maior) vem da raça
    const FVortexRaceInfo* Race = Catalog.Race(CurrentLook.Race);
    FVortexBodyShape Shape;
    if (Race) Shape.Meters = Race->Shape.Meters;
    Shape.Height = Shape.Width = Shape.Stature();
    // braços abertos para fora em quem tem o corpo largo (o braço entrava no corpo; as adagas se juntavam na frente).
    // Medido no Blender com a animação parada do aventureiro
    // e o ombro um pouco para fora do tronco (a junta ficava funda dentro do colete). O anão e o humano têm as juntas do
    // Mixamo (RigHero.py --mixamo), com o ombro no lugar: só abre um pouco o braço nos clipes do aventureiro
    static const TMap<FName, FVector2D> Arms = { { TEXT("humano"), { 5.f, 0.f } }, { TEXT("anao"), { 6.f, 0.f } }, { TEXT("elfo"), { 4.f, 1.5f } },
        { TEXT("barbaro"), { 6.f, 3.5f } }, { TEXT("diabo"), { 4.f, 2.f } }, { TEXT("morto_vivo"), { 6.f, 7.f } } };
    // 31/10: o herói do Mixamo como veio (esqueleto completo, Tools/ImportMixamoHeroes.py) toca os clipes do próprio
    // esqueleto: nada de abrir o braço nem empurrar o ombro
    const bool bMixamo = Model->GetRefSkeleton().FindBoneIndex(TEXT("mixamorig:Hips")) != INDEX_NONE ||
        Model->GetRefSkeleton().FindBoneIndex(TEXT("mixamorig_Hips")) != INDEX_NONE;
    const FVector2D Arm = bMixamo ? FVector2D::ZeroVector : Arms.FindRef(CurrentLook.Race.IsNone() ? FName(TEXT("humano")) : CurrentLook.Race);
    static const TMap<FName, float> Speeds = { { TEXT("anao"), .8f } };
    RaceSpeed = (Speeds.Contains(CurrentLook.Race) ? Speeds[CurrentLook.Race] : 1.f) * VortexArmor::MoveScale(Sheet);   // 07/11: as botas
    Shape.ArmSpread = Arm.X;
    Shape.ShoulderOut = Arm.Y;
    ApplyBodyShape(Shape);
    ApplyAnimTickOption();
    if (Weapon) Weapon->RefreshVisual();
}

void AVortexCharacter::ApplyBlockHair()
{
    const FVortexBlockHair* Hair = FVortexLookCatalog::BlockHair(CurrentLook);
    USkeletalMesh* HairMesh = Hair && !Hair->Mesh.IsEmpty() ? LoadObject<USkeletalMesh>(nullptr, *Hair->Mesh) : nullptr;
    if (HairMesh && !HairPart)
    {
        HairPart = NewObject<UVortexHairComponent>(this, TEXT("BlockHair"));
        HairPart->SetupAttachment(GetMesh());
        HairPart->RegisterComponent();
    }
    if (!HairPart) return;
    // o cabelo tem o próprio esqueleto com os nomes de osso do herói: copia a pose do corpo osso a osso (e simula os soltos)
    HairPart->SetBody(GetMesh());
    HairPart->SetHair(HairMesh, Hair ? Hair->Looseness : 0.f);
    // com armadura no peito ou capacete o cabelo solto passa mais longe do corpo
    const bool bArmored = Sheet && (!Sheet->GetEquipped(VortexRpg::Chest).IsNone() || !Sheet->GetEquipped(VortexRpg::Head).IsNone());
    HairPart->CollisionMargin = bArmored ? 4.f : 1.5f;
    // 07/11: o capacete (e o capuz) troca o cabelo: o cabelo some enquanto ele estiver na cabeça
    const FVortexWearPiece* Hat = VortexWearPiece(CurrentLook.Pieces.FindRef(TEXT("peca_chapeu")));
    HairPart->SetVisibility(HairMesh != nullptr && !WearsHelmet(Sheet) && !(Hat && Hat->bHidesHair));
    HairPart->SetOwnerNoSee(GetMesh()->bOwnerNoSee);
    if (!HairMesh) return;
    // M_BlockHair (Tools/ImportBlockHair.py): a textura guarda só os tons, a cor vem do herói. Dado direto, e não o da
    // malha: a importação deixou os cabelos com o WorldGridMaterial (cinza, sem cor; corrigido em 01/11)
    static const TCHAR* BlockHairMaterial = TEXT("/Game/Vortex/Characters/Heroes/humano/Hair/M_BlockHair.M_BlockHair");
    if (UMaterialInstanceDynamic* Material = HairPart->CreateDynamicMaterialInstance(0, LoadObject<UMaterialInterface>(nullptr, BlockHairMaterial)))
    {
        Material->SetVectorParameterValue(TEXT("Tint"), FLinearColor(CurrentLook.Colors.FindRef(TEXT("hair_color"))));
        // 09/11: chapéu "franja": só o cabelo acima da aba some; o que cai abaixo dela continua
        Material->SetScalarParameterValue(TEXT("ClipZ"), Hat && Hat->HairMode == TEXT("franja") && !WearsHelmet(Sheet) ? VortexWearFranjaZ() : 100000.f);
    }
}

void AVortexCharacter::ApplyBlockOutfit()
{
    const FVortexBlockOutfit* Outfit = FVortexLookCatalog::BlockOutfit(CurrentLook);
    USkeletalMesh* OutfitMesh = Outfit && !Outfit->Mesh.IsEmpty() ? LoadObject<USkeletalMesh>(nullptr, *Outfit->Mesh) : nullptr;
    if (OutfitMesh && !OutfitPart)
    {
        OutfitPart = NewObject<USkeletalMeshComponent>(this, TEXT("BlockOutfit"));
        OutfitPart->SetupAttachment(GetMesh());
        OutfitPart->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        OutfitPart->SetGenerateOverlapEvents(false);
        OutfitPart->bUseBoundsFromLeaderPoseComponent = true;
        OutfitPart->bCastHiddenShadow = true;
        OutfitPart->RegisterComponent();
    }
    if (!OutfitPart) return;
    // a roupa tem o próprio esqueleto com os nomes de osso do herói: segue a pose do corpo como as peças antigas
    if (OutfitPart->GetSkeletalMeshAsset() != OutfitMesh) OutfitPart->SetSkeletalMesh(OutfitMesh);
    OutfitPart->SetLeaderPoseComponent(GetMesh());
    OutfitPart->SetVisibility(OutfitMesh != nullptr);
    OutfitPart->SetOwnerNoSee(GetMesh()->bOwnerNoSee);
    HideCoveredOutfit(OutfitPart, Sheet, CurrentLook);
    // 09/11: a saia do traje é pano simulado, numa malha à parte; calça ou armadura nas pernas ficam no lugar dela
    const bool bLegsCovered = CurrentLook.Pieces.Contains(TEXT("peca_calca")) || (Sheet && VortexArmor::VisualPiece(Sheet->GetEquipped(VortexRpg::Legs)));
    USkeletalMesh* Cloth = Outfit && Outfit->bCloth && !bLegsCovered ? ClothMeshOf(Outfit->Mesh) : nullptr;
    DressPart(this, TEXT("BlockOutfit_tecido"), GetMesh(), Cloth, FLinearColor::White, false);
    if (Cloth) EnsureClothWind(GetWorld());
}

void AVortexCharacter::ApplyLookLocal(const FVortexLook& InLookRaw)
{
    const FVortexLookCatalog& Catalog = FVortexLookCatalog::Get();
    // heróis novos: um modelo fixo por raça (30/10/2026: sem personalização por enquanto)
    if (Catalog.IsValid())
        if (USkeletalMesh* Model = HeroModelFor(InLookRaw.Race, InLookRaw.Appearance)) { ApplyHeroModel(Model, InLookRaw); return; }
    if (!Catalog.IsValid() || !EnsureWardrobeParts()) return;
    BaseLook = InLookRaw;
    bHasBaseLook = true;
    const FVortexLook InLook = WithArmor(InLookRaw);
    CurrentLook = Catalog.Normalized(InLook);
    // paleta deste herói (cópia própria do material)
    if (!LookMaterial)
    {
        LookMaterial = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);
    }
    if (UTexture2D* Palette = Catalog.BuildPalette(CurrentLook, this))
    {
        LookPalette = Palette;
        // o material do boneco veio da importação do glTF: a paleta é a "BaseColorTexture"
        LookMaterial->SetTextureParameterValue(TEXT("BaseColorTexture"), LookPalette);
        LookMaterial->SetTextureParameterValue(TEXT("Palette"), LookPalette);
    }
    static const TPair<FName, FName> PartOf[] = {
        {TEXT("race"), TEXT("Race")}, {TEXT("hair"), TEXT("Hair")}, {TEXT("eyes"), TEXT("Eyes")}, {TEXT("mouth"), TEXT("Mouth")},
        {TEXT("head"), TEXT("Accessory")}, {TEXT("face_gear"), TEXT("AccessoryFace")}, {TEXT("back"), TEXT("AccessoryBack")},
        {TEXT("shirt"), TEXT("Shirt")}, {TEXT("pants"), TEXT("Pants")},
        {TEXT("shoes"), TEXT("Shoes")}, {TEXT("gloves"), TEXT("Gloves")},
    };
    for (const TPair<FName, FName>& Entry : PartOf)
    {
        const FName* Piece = CurrentLook.Pieces.Find(Entry.Key);
        SetPartMesh(Entry.Value, Piece ? *Piece : NAME_None);
        // peças próprias da raça têm cores fixas na paleta: são tingidas pela cor escolhida
        // dividida pela cor original da peça (o mesmo jeito do Godot)
        const FVortexPieceInfo* Info = Piece ? Catalog.Piece(*Piece) : nullptr;
        static const TMap<FName, FName> SlotOf = { {TEXT("hair"), TEXT("hair_color")}, {TEXT("shirt"), TEXT("shirt_color")},
            {TEXT("pants"), TEXT("pants_color")}, {TEXT("shoes"), TEXT("boots_color")}, {TEXT("gloves"), TEXT("glove_color")} };
        const FName* Slot = SlotOf.Find(Entry.Key);
        const FColor* Base = Info && Slot ? Info->Defaults.Find(*Slot) : nullptr;
        const FColor* Chosen = Slot ? CurrentLook.Colors.Find(*Slot) : nullptr;
        if (Info && Info->Races.Num() > 0 && Base && Chosen)
        {
            TObjectPtr<UMaterialInstanceDynamic>& Tint = TintMaterials.FindOrAdd(Entry.Value);
            if (!Tint) Tint = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);
            Tint->SetTextureParameterValue(TEXT("BaseColorTexture"), LookPalette);
            const FLinearColor C = FLinearColor::FromSRGBColor(*Chosen), B = FLinearColor::FromSRGBColor(*Base);
            Tint->SetVectorParameterValue(TEXT("BaseColorFactor"), FLinearColor(
                FMath::Clamp(C.R / FMath::Max(B.R, .02f), 0.f, 2.5f), FMath::Clamp(C.G / FMath::Max(B.G, .02f), 0.f, 2.5f),
                FMath::Clamp(C.B / FMath::Max(B.B, .02f), 0.f, 2.5f), 1.f));
            for (USkeletalMeshComponent* Part : WardrobeParts)
            {
                if (Part && Part->GetFName() == Entry.Value) Part->SetMaterial(0, Tint);
            }
        }
    }
    SetPartMesh(TEXT("Face"), Catalog.FacePiece(CurrentLook));
    SetPartMesh(TEXT("Core"), TEXT("core"));
    // alça e cinto só com a túnica (nas outras roupas pareciam o traje antigo por cima)
    const bool bTunic = CurrentLook.Pieces.FindRef(TEXT("shirt")) == TEXT("shirt_tunic");
    SetPartMesh(TEXT("GearStrap"), bTunic ? FName(TEXT("gear_strap")) : NAME_None);
    SetPartMesh(TEXT("GearBelt"), bTunic ? FName(TEXT("gear_belt")) : NAME_None);
    const FVortexRaceInfo* Race = Catalog.Race(CurrentLook.Race);
    ApplyBodyShape(Race ? Race->Shape : FVortexBodyShape());
}

bool AVortexCharacter::SetWardrobePiece(FName Category, FName PieceId)
{
    // Opções do primeiro conjunto modular; IDs de outras categorias não podem ocupar este slot.
    static const TMap<FName, TArray<FName>> Allowed = {
        {TEXT("Face"), {TEXT("face_male"), TEXT("face_female")}},
        {TEXT("Hair"), {TEXT("hair_male_1"), TEXT("hair_male_2"), TEXT("hair_female_1"), TEXT("hair_female_2")}},
        {TEXT("Eyes"), {TEXT("eyes_m_calm"), TEXT("eyes_m_fierce")}},
        {TEXT("Mouth"), {TEXT("mouth_small"), TEXT("mouth_smile")}},
        {TEXT("Shirt"), {TEXT("shirt_tunic"), TEXT("shirt_punk")}},
        {TEXT("Pants"), {TEXT("pants_dark"), TEXT("pants_jeans_punk")}},
        {TEXT("Shoes"), {TEXT("boots_laced"), TEXT("boots_punk")}},
        {TEXT("Gloves"), {TEXT("gloves_leather"), TEXT("gloves_punk")}},
    };
    const TArray<FName>* Options = Allowed.Find(Category);
    if (!Options || !Options->Contains(PieceId)) return false;
    const FString Id = PieceId.ToString();
    const FString Path = FString::Printf(TEXT("/Game/Vortex/Characters/Adventurer/Wardrobe/%s/SkeletalMeshes/%s.%s"), *Id, *Id, *Id);
    USkeletalMesh* PieceMesh = LoadObject<USkeletalMesh>(nullptr, *Path);
    if (!PieceMesh || PieceMesh->GetSkeleton() != GetMesh()->GetSkeletalMeshAsset()->GetSkeleton()) return false;
    if (!EnsureWardrobeParts()) return false;
    for (USkeletalMeshComponent* Part : WardrobeParts)
    {
        if (Part && Part->GetFName() == Category)
        {
            Part->SetSkeletalMesh(PieceMesh);
            Part->SetMaterial(0, GetMesh()->GetMaterial(0));
            if (Category == TEXT("Shirt"))
            {
                for (USkeletalMeshComponent* Other : WardrobeParts)
                {
                    if (Other && (Other->GetFName() == TEXT("GearStrap") || Other->GetFName() == TEXT("GearBelt")))
                    {
                        Other->SetVisibility(PieceId == TEXT("shirt_tunic"));
                    }
                }
            }
            return true;
        }
    }
    return false;
}

void AVortexCharacter::BeginPlay()
{
    Super::BeginPlay();
    // peças da arma com movimento próprio (capa do grimório, chicote e corrente), só desenho
    if (GetNetMode() != NM_DedicatedServer)
    {
        // 03/11: o herói molhado (escurece e brilha onde a água tocou, seca aos poucos)
        NewObject<UVortexWetness>(this, TEXT("Wetness"))->RegisterComponent();
        UVortexWeaponParts* Parts = NewObject<UVortexWeaponParts>(this, TEXT("WeaponParts"));
        Parts->RegisterComponent();
    }
    AbilitySystem->InitAbilityActorInfo(this, this);
    // herói novo: começa no modelo do humano até chegar o visual salvo (ou o da rede); antes nascia o aventureiro
    if (!bHasBaseLook && GetNetMode() != NM_DedicatedServer)
        if (USkeletalMesh* Model = HeroModelFor(TEXT("humano"))) { FVortexLook Default; Default.Race = TEXT("humano"); ApplyHeroModel(Model, Default); }
    FollowCamera->SetFieldOfView(VortexGraphics::FieldOfView());
    if (HasAuthority()) AbilitySystem->GiveAbility(FGameplayAbilitySpec(UVortexRecoverAbility::StaticClass(), 1));
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexWardrobePreview")))
    {
        const TPair<FName, FName> Preview[] = {
            {TEXT("Hair"), TEXT("hair_male_2")}, {TEXT("Eyes"), TEXT("eyes_m_fierce")},
            {TEXT("Mouth"), TEXT("mouth_smile")}, {TEXT("Shirt"), TEXT("shirt_punk")},
            {TEXT("Pants"), TEXT("pants_jeans_punk")}, {TEXT("Shoes"), TEXT("boots_punk")},
            {TEXT("Gloves"), TEXT("gloves_punk")},
        };
        bool bSuccess = true;
        for (const TPair<FName, FName>& Entry : Preview)
        {
            bSuccess &= SetWardrobePiece(Entry.Key, Entry.Value);
        }
        UE_LOG(LogTemp, Display, TEXT("VORTEX_WARDROBE_PREVIEW %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"));
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexWardrobeProbe")))
    {
        bool bSuccess = EnsureWardrobeParts() && WardrobeParts.Num() == 15;
        for (USkeletalMeshComponent* Part : WardrobeParts)
        {
            // traço da raça e cosméticos começam vazios
            if (Part && (Part->GetFName() == TEXT("Race") || Part->GetFName().ToString().StartsWith(TEXT("Accessory")))) continue;
            bSuccess &= Part && Part->GetSkeletalMeshAsset() && Part->LeaderPoseComponent.Get() == GetMesh() &&
                Part->GetSkeletalMeshAsset()->GetSkeleton() == GetMesh()->GetSkeletalMeshAsset()->GetSkeleton();
        }
        UE_LOG(LogTemp, Display, TEXT("VORTEX_WARDROBE_PROBE %s parts=%d"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), WardrobeParts.Num());
    }
    if (GetNetMode() == NM_Client && FParse::Param(FCommandLine::Get(), TEXT("VortexMovementObserve")))
    {
        FTimerDelegate Observe = FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            if (GetActorLocation().X > 900.f)
            {
                UE_LOG(LogTemp, Display, TEXT("Vortex movement: client observed replicated x=%.1f z=%.1f"),
                    GetActorLocation().X, GetActorLocation().Z);
                GetWorldTimerManager().ClearTimer(MovementObserveTimer);
            }
        });
        GetWorldTimerManager().SetTimer(MovementObserveTimer, Observe, 0.2f, true, 0.2f);
    }
    if (GetNetMode() == NM_Client && FParse::Param(FCommandLine::Get(), TEXT("VortexClientInputSmoke")))
    {
        bClientInputSmokeActive = true;
        SetActorTickEnabled(true);
    }
#endif
}

void AVortexCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    // caiu de um pulo dentro da água rasa: tchibum (o tamanho pela velocidade da queda), no lugar do som de chão
    float Wade = 0.f;
    if (GetNetMode() != NM_DedicatedServer && GetVortexMovement()->WaterDepth(Wade) && Wade > 8.f)
    {
        const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
        VortexHitFx::WaterSplash(GetWorld(), FVector(Feet.X, Feet.Y, Feet.Z + Wade), FMath::Clamp((FallSpeed - 150.f) / 900.f + .25f, .25f, 1.f));
        return;
    }
    if (FallSpeed > 350.f) VortexAudio::Play(this, TEXT("land"), GetActorLocation(), FMath::Clamp(FallSpeed / 900.f, .4f, 1.f));
}

void AVortexCharacter::OnJumped_Implementation()
{
    Super::OnJumped_Implementation();
    VortexAudio::Play(this, TEXT("jump"), GetActorLocation(), .7f);
}

// Passos (pelo chão embaixo), esquiva, golpes da espada, dor, cura, morte e nado. Tudo pelo que
// já é replicado, então os outros jogadores ouvem igual.
void AVortexCharacter::UpdateSounds(float DeltaSeconds)
{
    if (GetNetMode() == NM_DedicatedServer || IsHidden()) return;
    UVortexCharacterMovementComponent* Movement = GetVortexMovement();
    const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    const float Speed = GetVelocity().Size2D();
    if (Movement->IsFalling()) FallSpeed = FMath::Max(FallSpeed, -GetVelocity().Z);
    else if (!Movement->IsFalling()) FallSpeed = Movement->IsFalling() ? FallSpeed : 0.f;
    StepTimer -= DeltaSeconds;
    const bool bSwimming = Movement->IsSwimmingInWater();
    if (bSwimming && !bWasSwimming)
    {
        // caiu na água: o tamanho do respingo vem da velocidade da queda (pulo de cima de um morro = tchibum grande)
        const float Drop = FMath::Max(FallSpeed, -GetVelocity().Z);
        VortexHitFx::WaterSplash(GetWorld(), FVector(GetActorLocation().X, GetActorLocation().Y, Movement->WaterSurfaceZ()), FMath::Clamp((Drop - 150.f) / 900.f + Speed / 1600.f, .15f, 1.f));
        SwimFxTimer = .4f;
    }
    if (!bSwimming && bWasSwimming && Speed > 120.f)
    {
        // saindo da água: escorre um pouco
        VortexHitFx::Splash(GetWorld(), Feet, FLinearColor(.72f, .9f, 1.f), 6);
    }
    bWasSwimming = bSwimming;
    {
        float Wade = 0.f;
        bWasWadingNext = !bSwimming && Movement->IsMovingOnGround() && Movement->WaterDepth(Wade) && Wade > 8.f;
    }
    if (bSwimming)
    {
        if (Speed > 60.f && StepTimer <= 0.f) { StepTimer = .62f; VortexAudio::Play(this, TEXT("swim_stroke"), Feet, .6f); }
        // braçadas: ondinha e gotas na mão que entra na água; parado, um anel de vez em quando
        SwimFxTimer -= DeltaSeconds;
        if (SwimFxTimer <= 0.f)
        {
            const FVector Surface(GetActorLocation().X, GetActorLocation().Y, Movement->WaterSurfaceZ());
            if (Speed > 60.f)
            {
                SwimFxTimer = .31f;
                const FVector Side = GetActorRightVector() * (bSwimStrokeSide ? 32.f : -32.f);
                bSwimStrokeSide = !bSwimStrokeSide;
                VortexHitFx::Ripple(GetWorld(), Surface + GetActorForwardVector() * 70.f + Side, 55.f, .7f);
                VortexHitFx::Splash(GetWorld(), Surface + GetActorForwardVector() * 70.f + Side, FLinearColor(.85f, .95f, 1.f), 4);
                if (bSwimStrokeSide) VortexHitFx::Ripple(GetWorld(), Surface - GetActorForwardVector() * 80.f, 80.f, 1.3f);   // rastro
            }
            else
            {
                SwimFxTimer = 1.1f;
                VortexHitFx::Ripple(GetWorld(), Surface, 110.f, 1.4f);
            }
        }
    }
    else if (float Wade = 0.f; Movement->IsMovingOnGround() && Movement->WaterDepth(Wade) && Wade > 8.f)
    {
        // andando na água rasa (antes do nado): onda e gotas a cada passo, o som da água; parado, um anel de vez em quando
        const FVector Surface(Feet.X, Feet.Y, Feet.Z + Wade);
        if (!bWasWading) VortexHitFx::Splash(GetWorld(), Surface, FLinearColor(.8f, .93f, 1.f), 6);
        SwimFxTimer -= DeltaSeconds;
        if (Speed > 60.f && StepTimer <= 0.f)
        {
            StepTimer = Speed > 420.f ? .3f : .44f;
            VortexAudio::Play(this, TEXT("swim_stroke"), Feet, Speed > 420.f ? .45f : .3f, .1f);
            VortexHitFx::Ripple(GetWorld(), Surface + GetActorForwardVector() * 25.f, 45.f + Wade * .4f, .8f);
            VortexHitFx::Splash(GetWorld(), Surface + GetActorForwardVector() * 25.f, FLinearColor(.85f, .95f, 1.f), Speed > 420.f ? 5 : 3);
        }
        else if (Speed <= 60.f && SwimFxTimer <= 0.f)
        {
            SwimFxTimer = 1.2f;
            VortexHitFx::Ripple(GetWorld(), Surface, 70.f, 1.3f);
        }
    }
    else if (Movement->IsMovingOnGround() && Speed > 60.f && StepTimer <= 0.f)
    {
        // passadas mais curtas correndo; anão dá passos mais curtos, bárbaro mais longos
        StepTimer = (Speed > 420.f ? .3f : .44f) * FMath::Clamp(BodyShape.Stature(), .75f, 1.3f);
        VortexAudio::Play(this, VortexAudio::StepSound(Feet, StepVariant++), Feet, Speed > 420.f ? .55f : .35f, .08f);
    }
    bWasWading = bWasWadingNext;
    if (Movement->DodgesStarted != LastDodges)
    {
        if (LastDodges != 0 || Movement->DodgesStarted == 1) VortexAudio::Play(this, TEXT("roll"), Feet, .8f);
        LastDodges = Movement->DodgesStarted;
    }
    if (Weapon)
    {
        const FVortexAttackState& Attack = Weapon->GetAttackState();
        if (Attack.Serial != LastAttackSerial)
        {
            LastAttackSerial = Attack.Serial;
            const UVortexWeaponDefinition* Definition = Weapon->GetDefinition();
            if (Attack.SwingIndex != INDEX_NONE && Definition && Definition->Swings.IsValidIndex(Attack.SwingIndex))
            {
                const FVortexWeaponSwing& Swing = Definition->Swings[Attack.SwingIndex];
                const float Pace = FMath::Max(.1f, Weapon->GetAttackSpeed());
                const VortexWeaponKit::FKit* Kit = VortexWeaponKit::Find(Definition->WeaponId);
                const FName Family = Kit ? Kit->Family : NAME_None;
                const FName SwordSwing(*FString::Printf(TEXT("sword_swing_%d"), 1 + Attack.SwingIndex % 3));
                if (Swing.Mode == TEXT("melee") || Swing.Mode == TEXT("slam"))
                {
                    VortexAudio::Play(this, VortexAudio::WeaponSound(this, TEXT("swing"), Attack.SwingIndex, SwordSwing), GetActorLocation(), .8f);
                    // G3: o rastro com o jeito da arma (soco curto, estocada reta, arco na cor da arma)
                    const float Delay = Swing.ActiveStart / Pace;
                    // 01/11: o rastro no alcance de verdade do golpe (o mesmo do acerto, MeleeReach: o corpo, o braço e a
                    // arma); o ReachCm do Godot é bem maior e o corte aparecia longe de quem ele acerta
                    const float HitReach = Weapon ? Weapon->MeleeReach(Swing) : Swing.ReachCm;
                    if (Swing.Mode == TEXT("slam")) VortexHitFx::GroundSlam(this, FMath::Max(Swing.RadiusCm, 200.f), Delay);
                    else if (Family == TEXT("Fist")) AVortexWeaponFx::SpawnLocal(this, TEXT("punch"), Delay, HitReach * .9f, FLinearColor(1.f, .85f, .6f));
                    else if (Family == TEXT("Spear") || (Family == TEXT("Halberd") && Attack.SwingIndex == 2))
                        AVortexWeaponFx::SpawnLocal(this, TEXT("thrust"), Delay, HitReach, FLinearColor(.85f, .9f, 1.f));
                    else
                    {
                        const FName Element = VortexWeaponKit::HeldElement(this);
                        const FLinearColor Tint = !Element.IsNone() ? AVortexProjectile::ElementColor(Element == TEXT("lightning") ? FName(TEXT("arcane")) : Element)
                            : Kit ? VortexWeaponKit::TrailColor(Kit->Family) : FLinearColor::White;
                        VortexHitFx::SwingTrail(this, HitReach, Attack.SwingIndex, Delay, Tint);
                    }
                }
                else
                {
                    // tiro, magia, arremesso: o som no instante em que sai
                    const FName Shot = VortexAudio::WeaponSound(this, TEXT("shot"), Attack.SwingIndex, Swing.Sound);
                    FTimerHandle Handle;
                    GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Shot]()
                    {
                        VortexAudio::Play(this, Shot, GetActorLocation(), .85f);
                    }), FMath::Max(.01f, Swing.ActiveStart / Pace), false);
                }
            }
        }
    }
    if (Vitals)
    {
        const float Health = Vitals->GetHealth();
        if (LastHealth >= 0.f)
        {
            if (Health <= 0.f && LastHealth > 0.f) VortexAudio::Play(this, TEXT("death"), GetActorLocation(), 1.f, 0.f);
            else if (Health < LastHealth - .5f)
            {
                VortexAudio::Play(this, Weapon && Weapon->IsGuarding() ? TEXT("guard_armor") : TEXT("hurt"), GetActorLocation(), .85f);
            }
            else if (Health > LastHealth + 5.f) VortexAudio::Play(this, TEXT("heal"), GetActorLocation(), .8f, 0.f);
        }
        LastHealth = Health;
    }
}

void AVortexCharacter::Tick(float DeltaSeconds)
{
    // presa numa teia (o dono e o servidor calculam igual: a velocidade é a mesma nos dois)
    if (const AVortexPointsOfInterest* Pois = AVortexPointsOfInterest::Find(GetWorld()))
    {
        const bool bInWeb = Pois->IsInWeb(GetActorLocation());
        if (bInWeb && !bWebbed && IsLocallyControlled() && Sheet) Sheet->PushNotice(TEXT("Preso na teia! Ataque para cortar"), 1);
        bWebbed = bInWeb;
    }
    VORTEX_PROBE("Heroi.Tick");
    Super::Tick(DeltaSeconds);
    if (Weapon && GetNetMode() != NM_DedicatedServer) { Weapon->TickBowGrip(DeltaSeconds); }
    // 03/11: mira travada (cajado): a câmera vira sozinha para o inimigo travado (o jogador ainda mexe um pouco)
    if (Weapon && IsLocallyControlled())
        if (const AActor* Locked = Weapon->GetLockTarget())
            if (APlayerController* Player = Cast<APlayerController>(GetController()))
            {
                FVector Eye;
                FRotator View;
                Player->GetPlayerViewPoint(Eye, View);
                const FRotator Want = (Locked->GetActorLocation() + FVector(0.f, 0.f, 30.f) - Eye).Rotation();
                const FRotator Now = Player->GetControlRotation();
                FRotator Delta = (Want - Now).GetNormalized();
                Delta.Pitch *= .5f;
                Delta.Roll = 0.f;
                Player->SetControlRotation(Now + Delta * FMath::Min(1.f, DeltaSeconds * 7.f));
            }
    // caiu sem vida montado: desce do cavalo
    if (Mount && HasAuthority() && AbilitySystem && AbilitySystem->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) <= 0.f) Dismount();
    // 02/11: nos outros jogos, desceu do cavalo e o cavalo já soltou: o herói volta ao chão (antes ficava para sempre na
    // altura da sela, porque o cavalo às vezes chegava solto antes de saber que não tinha mais cavaleiro)
    if (!Mount && MountLift > 0.f)
    {
        TArray<AActor*> Riding;
        GetAttachedActors(Riding);
        const bool bHorse = Riding.ContainsByPredicate([this](const AActor* A) { const AVortexMonster* H = Cast<AVortexMonster>(A); return H && H->GetRider() == this; });
        if (!bHorse) SetMountLift(0.f);
    }
    // e para de ignorar o cavalo de antes (o cavalo pode ter chegado solto antes de saber que não tinha mais cavaleiro)
    if (!Mount && GetCapsuleComponent()->GetMoveIgnoreActors().Num())
    {
        const TArray<AActor*> Ignored = GetCapsuleComponent()->GetMoveIgnoreActors();
        for (AActor* Each : Ignored)
            if (const AVortexMonster* Horse = Cast<AVortexMonster>(Each); Horse && Horse->GetRider() != this) GetCapsuleComponent()->IgnoreActorWhenMoving(Each, false);
    }
    // montado, nos outros jogos: sem a suavização da rede (o cavalo vai preso à cápsula, que não é suavizada; com a malha
    // suavizada os dois se separavam e o cavaleiro "tremia" em volta do cavalo)
    if (GetLocalRole() == ROLE_SimulatedProxy)
    {
        const ENetworkSmoothingMode Want = Mount || MountLift > 0.f ? ENetworkSmoothingMode::Disabled : ENetworkSmoothingMode::Exponential;
        if (GetCharacterMovement()->NetworkSmoothingMode != Want) GetCharacterMovement()->NetworkSmoothingMode = Want;
    }
    TickDeath(DeltaSeconds);
    TickFace(DeltaSeconds);
    if (HasAuthority()) TickEmote();
    if (IsLocallyControlled()) VortexSocialUi::TickTrade(this);
#if !UE_BUILD_SHIPPING
    // travadas no log (medir o primeiro poder, a entrada na cidade...)
    if (IsLocallyControlled() && DeltaSeconds > .15f && GetWorld()->GetTimeSeconds() > 8.0)
    {
        UE_LOG(LogTemp, Display, TEXT("VORTEX_HITCH %.0f ms em %.1f s"), DeltaSeconds * 1000.f, GetWorld()->GetTimeSeconds());
    }
#endif
    // tranco da câmera: treme e volta em 0,2 s (tempo real: a pausa de impacto não segura a câmera)
    if (KickAge < .25f && IsLocallyControlled())
    {
        KickAge += GetWorld()->GetDeltaSeconds();
        const float Left = FMath::Max(0.f, 1.f - KickAge / .2f);
        const float A = KickStrength * Left * Left * 4.f;
        FollowCamera->SetRelativeLocation(Left > 0.f ? FVector(0.f, FMath::Sin(KickAge * 95.f) * A, FMath::Cos(KickAge * 120.f) * A * .7f) : FVector::ZeroVector);
    }
    UpdateSounds(DeltaSeconds);
    TickPotion();
    if ((ArmorCheck -= DeltaSeconds) <= 0.f) { ArmorCheck = .4f; RefreshArmorLook(); }
    // queimando (lava): chamas no corpo enquanto durar (todos veem)
    if (GetNetMode() != NM_DedicatedServer && Sheet)
    {
        const bool bBurning = Sheet->BurnRemaining() > 0.f;
        if (bBurning && !BurnFx)
        {
            if (UNiagaraSystem* Fire = VortexHitFx::System(TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Small.NS_Fire_Small")))
                BurnFx = UNiagaraFunctionLibrary::SpawnSystemAttached(Fire, GetCapsuleComponent(), NAME_None, FVector(0.f, 0.f, -20.f), FRotator::ZeroRotator,
                    FVector(.9f), EAttachLocation::KeepRelativeOffset, false, ENCPoolMethod::None);
        }
        else if (!bBurning && BurnFx) { BurnFx->DestroyComponent(); BurnFx = nullptr; }
    }
    if (bMeleeFacing) TickMeleeFacing(DeltaSeconds);
    // nível 10 com as cartas tiradas: a cena do Despertar abre quando a área está calma
    if (IsLocallyControlled() && !bAwakeningShown && Sheet && (AwakeningCheck -= DeltaSeconds) <= 0.f)
    {
        AwakeningCheck = .5f;
        if (Sheet->GetClassId().IsNone() && Sheet->GetClassCards().Num() && !VortexHeroWindow::IsOpen() && !VortexDialogue::IsOpen() && !VortexAwakening::IsOpen())
        {
            bool bCalm = Vitals && Vitals->GetHealth() > 0.f;
            for (TActorIterator<APawn> It(GetWorld()); It && bCalm; ++It)
            {
                const IVortexCreature* Creature = Cast<IVortexCreature>(*It);
                bCalm = !(Creature && Creature->IsEngaged() && !Creature->IsCreatureDead() && FVector::Dist(It->GetActorLocation(), GetActorLocation()) < 3000.f);
            }
            if (bCalm) { bAwakeningShown = true; VortexAwakening::Open(this); }
        }
    }
    if (IsLocallyControlled() && (bFirstPerson || FirstPersonBlend > 0.f)) TickFirstPerson(DeltaSeconds);
    else if (IsLocallyControlled() && FollowCamera)
    {
        // terceira pessoa: uma parede atrás puxa a câmera para dentro do herói; aí ele some só para
        // quem joga (a sombra fica) em vez de encher a tela por dentro
        const FVector Head = GetActorLocation() + FVector(0.f, 0.f, 35.f * BodyShape.Stature());
        const FVector Cam = FollowCamera->GetComponentLocation();
        const FVector Local = GetActorTransform().InverseTransformPosition(Cam);
        const bool bInside = FVector::Dist(Cam, Head) < 75.f * BodyShape.Stature() ||
            (FVector2D(Local.X, Local.Y).Size() < GetCapsuleComponent()->GetScaledCapsuleRadius() + 30.f && FMath::Abs(Local.Z) < GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 40.f);
        if (bInside != bCameraInsideBody)
        {
            bCameraInsideBody = bInside;
            for (USkeletalMeshComponent* Part : WardrobeParts) if (Part) { Part->SetOwnerNoSee(bInside); Part->bCastHiddenShadow = true; }
            GetMesh()->SetOwnerNoSee(bInside && WardrobeParts.IsEmpty());
            if (HairPart) HairPart->SetOwnerNoSee(bInside);
            if (OutfitPart) OutfitPart->SetOwnerNoSee(GetMesh()->bOwnerNoSee);
            SyncArmorView(this, bInside, bInside);
            if (WeaponVisual) WeaponVisual->SetOwnerNoSee(bInside);
            if (WeaponVisualLeft) WeaponVisualLeft->SetOwnerNoSee(bInside);
        }
    }
    if (bCameraTransitionActive && IsLocallyControlled() && !bGodotBenchCamera && !bFirstPerson && FirstPersonBlend <= 0.f)
    {
        const float Distance = bAimHeld ? FMath::Min(WantedCameraDistance, 210.f) : WantedCameraDistance;
        const FVector WantedOffset = CameraOffsetFor(Distance, CameraShoulderSign, bAimHeld);
        // campo de visão das opções (75° no padrão); mirando fecha 7°
        const float WantedFov = VortexGraphics::FieldOfView() - (bAimHeld ? 7.f : 0.f);
        CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, Distance, DeltaSeconds, 8.f);
        CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, WantedOffset, DeltaSeconds, 8.f);
        FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, WantedFov, DeltaSeconds, 8.f));
        if (FMath::Abs(CameraBoom->TargetArmLength - Distance) < .25f &&
            FVector::DistSquared(CameraBoom->SocketOffset, WantedOffset) < .1f &&
            FMath::Abs(FollowCamera->FieldOfView - WantedFov) < .05f)
        {
            CameraBoom->TargetArmLength = Distance;
            CameraBoom->SocketOffset = WantedOffset;
            FollowCamera->SetFieldOfView(WantedFov);
            bCameraTransitionActive = false;
        }
    }
#if !UE_BUILD_SHIPPING
    if (!bClientInputSmokeActive || !IsLocallyControlled())
    {
        return;
    }
    if (!bClientInputSmokeStarted)
    {
        ClientInputSmokeStartX = GetActorLocation().X;
        bClientInputSmokeStarted = true;
    }
    ClientInputSmokeElapsed += DeltaSeconds;
    AddMovementInput(FVector::ForwardVector, 1.f);
    // ordem: esquiva no chão logo no começo, depois corrida e salto (correndo, o personagem sai
    // da borda do mapa de teste em ~2 s e no ar a esquiva não vale)
    if (ClientInputSmokeElapsed >= 0.2f && !bClientInputSmokeDodged && GetCharacterMovement()->IsMovingOnGround())
    {
        BeginDodge();
        bClientInputSmokeDodged = true;
    }
    if (ClientInputSmokeElapsed >= 1.f && !bClientInputSmokeSprinting)
    {
        BeginSprint();
        bClientInputSmokeSprinting = true;
    }
    if (ClientInputSmokeElapsed >= 1.4f && !bClientInputSmokeJumped)
    {
        Jump();
        bClientInputSmokeJumped = true;
    }
    bClientInputSmokeSawAirborne |= GetCharacterMovement()->IsFalling();
    if (ClientInputSmokeElapsed >= 3.f)
    {
        EndSprint();
        bClientInputSmokeActive = false;
        const float DeltaX = GetActorLocation().X - ClientInputSmokeStartX;
        const int32 Dodges = GetVortexMovement()->DodgesStarted;
        const bool bPassed = DeltaX > 300.f && bClientInputSmokeSawAirborne && bClientInputSmokeSprinting && Dodges == 1;
        UE_LOG(LogTemp, Display,
            TEXT("Vortex movement: CLIENT_INPUT_SMOKE_%s deltaX=%.1f airborne=%d dodges=%d"),
            bPassed ? TEXT("PASS") : TEXT("FAIL"), DeltaX, bClientInputSmokeSawAirborne ? 1 : 0, Dodges);
    }
#endif
}

void AVortexCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    check(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AVortexCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AVortexCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AVortexCharacter::LookYaw);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AVortexCharacter::LookPitch);
    PlayerInputComponent->BindKey(EKeys::LeftBracket, IE_Pressed, this, &AVortexCharacter::SlowerMouse);
    PlayerInputComponent->BindKey(EKeys::RightBracket, IE_Pressed, this, &AVortexCharacter::FasterMouse);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AVortexCharacter::BeginJump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AVortexCharacter::BeginSprint);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AVortexCharacter::EndSprint);
    PlayerInputComponent->BindAction(TEXT("Dodge"), IE_Pressed, this, &AVortexCharacter::BeginDodge);
    PlayerInputComponent->BindKey(EKeys::H, IE_Pressed, this, &AVortexCharacter::BeginRecover);
    // 01/11: andar é o normal; o Caps Lock troca para correr sempre (o Shift corre só enquanto segura)
    PlayerInputComponent->BindKey(EKeys::CapsLock, IE_Pressed, this, &AVortexCharacter::ToggleWalk);
    PlayerInputComponent->BindKey(EKeys::M, IE_Pressed, this, &AVortexCharacter::CycleMinimap);
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AVortexCharacter::BeginPrimaryAttack);
    // arco: segurar estica a corda, soltar dispara
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AVortexCharacter::EndPrimaryAttack);
    // toda arma: combo no botão esquerdo, especial no direito; a defesa fica em segurar F
    PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AVortexCharacter::BeginSpecialAttack);
    // especiais carregados (arco, martelo, lança...): soltar o botão dispara
    PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AVortexCharacter::EndSpecialAttack);
    PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &AVortexCharacter::BeginGuard);
    PlayerInputComponent->BindKey(EKeys::F, IE_Released, this, &AVortexCharacter::EndGuard);
    // poções (Q vida, R mana) e a janela do herói (I ou Tab)
    PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AVortexCharacter::QuickHealth);
    PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &AVortexCharacter::QuickMana);
    PlayerInputComponent->BindKey(EKeys::I, IE_Pressed, this, &AVortexCharacter::ToggleHeroWindow);
    PlayerInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AVortexCharacter::ToggleHeroWindow);
    PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AVortexCharacter::Interact);
    PlayerInputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AVortexCharacter::ZoomIn);
    PlayerInputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AVortexCharacter::ZoomOut);
    PlayerInputComponent->BindKey(EKeys::V, IE_Pressed, this, &AVortexCharacter::SwapCameraShoulder);
    PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &AVortexCharacter::ToggleFirstPerson);
    PlayerInputComponent->BindKey(EKeys::Z, IE_Pressed, this, &AVortexCharacter::Ping);
    PlayerInputComponent->BindKey(EKeys::X, IE_Pressed, this, &AVortexCharacter::Whistle);
    PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, this, &AVortexCharacter::OpenEmoteWheel);
    PlayerInputComponent->BindKey(EKeys::B, IE_Released, this, &AVortexCharacter::CloseEmoteWheel);
    PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AVortexCharacter::RespawnNow);
    // habilidades da classe nas teclas 1 a 4
    static const FKey SkillKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
    for (int32 i = 0; i < UE_ARRAY_COUNT(SkillKeys); ++i)
    {
        FInputKeyBinding Binding(FInputChord(SkillKeys[i]), IE_Pressed);
        Binding.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this, [this, i]() { CastClassSkill(i); });
        PlayerInputComponent->KeyBindings.Add(Binding);
    }
#if !UE_BUILD_SHIPPING
    // 1-4 são das habilidades: o teste de roupas usa o teclado numérico
    static const FKey Digits[] = { EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour, EKeys::NumPadFive, EKeys::NumPadSix, EKeys::NumPadSeven };
    for (int32 i = 0; i < UE_ARRAY_COUNT(Digits); ++i)
    {
        FInputKeyBinding Binding(FInputChord(Digits[i]), IE_Pressed);
        Binding.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this, [this, i]() { CycleWardrobe(i); });
        PlayerInputComponent->KeyBindings.Add(Binding);
    }
    PlayerInputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AVortexCharacter::LookAtFace);
    PlayerInputComponent->BindKey(EKeys::T, IE_Pressed, this, &AVortexCharacter::OpenDevChat);
#endif
}

void AVortexCharacter::LookYaw(float Value)
{
    if (IsMouseOnWindow()) return;   // janela do herói aberta: o mouse é dela
    if (bEmoteWheel) { EmoteAim.X = FMath::Clamp(EmoteAim.X + Value, -12.f, 12.f); return; }   // a roda de emotes: o mouse aponta a fatia
    const UVortexPlayerSettings* Settings = UVortexPlayerSettings::Get();
    AddControllerYawInput(Value * Settings->MouseSensitivity * (bAimHeld ? Settings->AimSensitivityScale : 1.f));
}

void AVortexCharacter::LookPitch(float Value)
{
    if (IsMouseOnWindow()) return;   // janela do herói aberta: o mouse é dela
    if (bEmoteWheel) { EmoteAim.Y = FMath::Clamp(EmoteAim.Y - Value, -12.f, 12.f); return; }
    const UVortexPlayerSettings* Settings = UVortexPlayerSettings::Get();
    AddControllerPitchInput(Value * Settings->MouseSensitivity * (bAimHeld ? Settings->AimSensitivityScale : 1.f) *
        (Settings->bInvertMouseY ? -1.f : 1.f));
}

void AVortexCharacter::CycleMinimap()
{
    // M: o mapa do vale inteiro (o alcance do minimapa fica nas opções, aba Interface)
    if (APlayerController* Player = Cast<APlayerController>(GetController()))
    {
        // Trocar para o mapa fecha a ficha e a pausa antes de mudar foco e modo de entrada.
        VortexHeroWindow::Close();
        if (UVortexGameInstance* Instance = Cast<UVortexGameInstance>(GetGameInstance()); Instance && Instance->IsPauseMenuOpen())
            Instance->TogglePauseMenu();
        if (AVortexHudBase* Hud = Cast<AVortexHudBase>(Player->GetHUD())) Hud->ToggleWorldMap();
    }
    VortexAudio::Play2D(this, TEXT("map_open"), .6f);
}

void AVortexCharacter::SlowerMouse()
{
    UVortexPlayerSettings::Get()->StepMouseSensitivity(-1);
    SensitivityNoticeUntil = GetWorld()->GetTimeSeconds() + 2.f;
}

void AVortexCharacter::FasterMouse()
{
    UVortexPlayerSettings::Get()->StepMouseSensitivity(1);
    SensitivityNoticeUntil = GetWorld()->GetTimeSeconds() + 2.f;
}

void AVortexCharacter::MoveForward(float Value)
{
    if (Controller && !FMath::IsNearlyZero(Value))
    {
        // voando (/fly), a frente inclui subir e descer com a câmera
        const FRotator Yaw = IsDevFlying() ? Controller->GetControlRotation() : FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f);
        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Value);
    }
}

void AVortexCharacter::OpenDevChat()
{
    if (!VortexHeroWindow::IsOpen() && IsLocallyControlled()) VortexDevChat::Open(this);
}

void AVortexCharacter::ServerDevCommand_Implementation(const FString& Line)
{
#if !UE_BUILD_SHIPPING
    if (Line.Len() > 200) return;
    bool bError = false;
    const FString Reply = VortexDevChat::RunOnServer(this, Line, bError);
    if (!Reply.IsEmpty()) ClientDevReply(Reply, bError);
#endif
}

void AVortexCharacter::ClientDevReply_Implementation(const FString& Text, bool bError) { VortexDevChat::AddLine(Text, bError); }

void AVortexCharacter::ClientDevFly_Implementation(bool bOn) { SetDevFly(bOn); }

void AVortexCharacter::SetDevFly(bool bOn)
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (bOn)
    {
        Movement->MaxFlySpeed = 1800.f;
        Movement->BrakingDecelerationFlying = 4000.f;
        if (Movement->MovementMode != MOVE_Flying) Movement->SetMovementMode(MOVE_Flying);
    }
    else if (Movement->MovementMode == MOVE_Flying) Movement->SetMovementMode(MOVE_Falling);
}

bool AVortexCharacter::IsDevFlying() const { return GetCharacterMovement() && GetCharacterMovement()->MovementMode == MOVE_Flying; }

void AVortexCharacter::MoveRight(float Value)
{
    if (Controller && !FMath::IsNearlyZero(Value))
    {
        const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Value);
    }
}

void AVortexCharacter::UseGodotBenchCamera(float ArmLength, float Shoulder)
{
    bGodotBenchCamera = true;
    bCameraTransitionActive = false;
    CameraBoom->TargetArmLength = ArmLength;
    CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 135.f - HeroCapsuleHalfHeight));
    CameraBoom->SocketOffset = FVector(0.f, Shoulder, 0.f);
    CameraBoom->bEnableCameraLag = false;
    CameraBoom->ProbeSize = 18.f;
    if (UVortexCameraBoom* VortexBoom = Cast<UVortexCameraBoom>(CameraBoom)) VortexBoom->SetSmoothRecoveryEnabled(false);
    FollowCamera->SetFieldOfView(64.f);
}

void AVortexCharacter::ZoomIn()
{
    // mapa aberto: a roda aproxima o mapa, não a câmera
    if (const APlayerController* Player = Cast<APlayerController>(GetController()))
        if (AVortexHudBase* Hud = Cast<AVortexHudBase>(Player->GetHUD()); Hud && Hud->IsWorldMapOpen()) { Hud->ZoomWorldMap(-1.f); return; }
    // roda de emotes aberta: a rodinha troca de página
    if (bEmoteWheel) { EmotePageIndex = (EmotePageIndex + EmotePages - 1) % EmotePages; return; }
    StepZoom(-1.f);
}

void AVortexCharacter::ZoomOut()
{
    if (const APlayerController* Player = Cast<APlayerController>(GetController()))
        if (AVortexHudBase* Hud = Cast<AVortexHudBase>(Player->GetHUD()); Hud && Hud->IsWorldMapOpen()) { Hud->ZoomWorldMap(1.f); return; }
    if (bEmoteWheel) { EmotePageIndex = (EmotePageIndex + 1) % EmotePages; return; }
    StepZoom(1.f);
}

void AVortexCharacter::SwapCameraShoulder()
{
    if (bGodotBenchCamera || !IsLocallyControlled()) return;
    CameraShoulderSign *= -1.f;
    bCameraTransitionActive = true;
    SetActorTickEnabled(true);
}

void AVortexCharacter::StepZoom(float Steps)
{
    if (bGodotBenchCamera || !IsLocallyControlled()) return;
    WantedCameraDistance = FMath::Clamp(WantedCameraDistance + Steps * 60.f, 150.f, 800.f);
    bCameraTransitionActive = true;
    SetActorTickEnabled(true);
}

bool AVortexCharacter::TraceAim(FHitResult& OutHit, float MaxDistance) const
{
    OutHit = FHitResult();
    if (!FollowCamera || !GetWorld() || !FMath::IsFinite(MaxDistance) || MaxDistance <= 0.f) return false;
    const FVector Start = FollowCamera->GetComponentLocation();
    const FVector End = Start + FollowCamera->GetForwardVector() * FMath::Min(MaxDistance, 100000.f);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexAim), false, this);
    const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Query);
    if (!bHit) { OutHit.TraceStart = Start; OutHit.TraceEnd = End; }
    return bHit;
}

UVortexCharacterMovementComponent* AVortexCharacter::GetVortexMovement() const
{
    return Cast<UVortexCharacterMovementComponent>(GetCharacterMovement());
}

// Corrida e esquiva viram bits do movimento salvo (UVortexCharacterMovementComponent): o
// servidor recebe junto com o passo e a previsão do cliente não é corrigida à toa.
void AVortexCharacter::ToggleWalk()
{
    // 03/11: com o cajado, o Caps Lock trava a mira num inimigo (e destrava)
    if (Weapon && Weapon->UsesLock()) { Weapon->ToggleLock(); return; }
    if (UVortexCharacterMovementComponent* Movement = GetVortexMovement()) Movement->bWantsToWalk = !Movement->bWantsToWalk;
}

void AVortexCharacter::BeginSprint()
{
    GetVortexMovement()->bWantsToSprint = true;
}

void AVortexCharacter::EndSprint()
{
    GetVortexMovement()->bWantsToSprint = false;
}

void AVortexCharacter::BeginDodge()
{
    if (CancelDrinkInstead()) return;
    if (Mount) return;
    GetVortexMovement()->bWantsToDodge = true;
}

void AVortexCharacter::BeginRecover()
{
    if (AbilitySystem) AbilitySystem->TryActivateAbilityByClass(UVortexRecoverAbility::StaticClass());
}

bool AVortexCharacter::CancelDrinkInstead()
{
    // bebendo: atacar, esquivar, pular, defender ou lançar habilidade interrompe a bebida (sem gastar a poção)
    if (!Sheet || !Sheet->IsDrinking()) return false;
    Sheet->ServerCancelDrink();
    return true;
}

bool AVortexCharacter::IsMouseOnWindow() const
{
    // inventário, diálogo, loja, janela de outro jogador, chat de comandos, mapa aberto: o mouse é deles
    const APlayerController* Player = Cast<APlayerController>(GetController());
    const AVortexHudBase* Hud = Player ? Cast<AVortexHudBase>(Player->GetHUD()) : nullptr;
    return VortexHeroWindow::IsOpen() || VortexDialogue::IsOpen() || VortexShopWindow::IsOpen() || VortexSocialUi::IsOpen() || VortexDevChat::IsTyping()
        || (Hud && Hud->IsWorldMapOpen());
}

void AVortexCharacter::BeginPrimaryAttack()
{
    if (CancelDrinkInstead()) return;
    if (IsMouseOnWindow() || Mount || IsDead()) return;   // janela do herói aberta: o mouse é dela; montado não luta
    // teia na frente (ou presa nela): o golpe corta (aqui e no servidor)
    if (AVortexPointsOfInterest* Pois = AVortexPointsOfInterest::Find(GetWorld()))
        if (Pois->SlashWeb(GetActorLocation(), GetControlRotation().Vector()) && !HasAuthority()) ServerSlashWeb();
    if (IsMeleeWeapon() && !bAimHeld && !bFirstPerson)
    {
        BeginMeleeFacing();
        if (Weapon) Weapon->PressPrimary();
        return;
    }
    bAttackFacing = true;
    UpdateAimFacing();
    // 06/11: arma de longe (arco, cajado): o corpo fica virado para a mira enquanto o botão está apertado (puxando a
    // corda pode passar de 0,9 s); solta e mais um pouco em EndPrimaryAttack
    GetWorldTimerManager().SetTimer(AttackFacingTimer, this, &AVortexCharacter::FinishAttackFacing, IsMeleeWeapon() ? 0.9f : 30.f, false);
    if (!HasAuthority()) ServerBeginAttackFacing();
    if (Weapon) Weapon->PressPrimary();
}

void AVortexCharacter::BeginJump()
{
    if (CancelDrinkInstead()) return;
    if (bWebbed) return;   // presa na teia: não pula (golpeie para cortar)
    Jump();
}

void AVortexCharacter::EndPrimaryAttack()
{
    // 06/11: soltando a flecha (ou o feitiço), o corpo vira na hora para onde a câmera olha: nunca atira de costas
    if (!IsMeleeWeapon() && !bFirstPerson && Controller && !IsDead() && !Mount)
    {
        bAttackFacing = true;
        UpdateAimFacing();
        SetActorRotation(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
        GetWorldTimerManager().SetTimer(AttackFacingTimer, this, &AVortexCharacter::FinishAttackFacing, 0.6f, false);
        if (!HasAuthority()) ServerBeginAttackFacing();
    }
    if (Weapon) Weapon->ReleasePrimary();
}

void AVortexCharacter::BeginSpecialAttack()
{
    if (CancelDrinkInstead()) return;
    if (IsMouseOnWindow() || Mount || IsDead()) return;   // janela do herói aberta: o mouse é dela
    bAttackFacing = true;
    UpdateAimFacing();
    // 02/11: o cajado sem magia escolhida abre a escolha (Cura, Vinhas ou Bola de Fogo) em vez de conjurar
    if (Weapon && Sheet && Sheet->GetStaffSpell().IsNone() && Weapon->GetDefinition() && IsLocallyControlled())
    {
        const VortexWeaponKit::FKit* Kit = VortexWeaponKit::Find(Weapon->GetDefinition()->WeaponId);
        if (Kit && Kit->Special == TEXT("vines")) { VortexSpellChoice::Open(this); return; }
    }
    GetWorldTimerManager().SetTimer(AttackFacingTimer, this, &AVortexCharacter::FinishAttackFacing, 0.9f, false);
    if (!HasAuthority()) ServerBeginAttackFacing();
    if (Weapon) Weapon->PressSpecial();
}

void AVortexCharacter::EndSpecialAttack()
{
    if (Weapon) Weapon->ReleaseSpecialInput();
}

void AVortexCharacter::BeginGuard()
{
    if (CancelDrinkInstead()) return;
    SetAimHeld(true);
    if (!HasAuthority()) ServerSetAimHeld(true);
    if (Weapon) Weapon->SetGuardPressed(true);
}

void AVortexCharacter::EndGuard()
{
    SetAimHeld(false);
    if (!HasAuthority()) ServerSetAimHeld(false);
    if (Weapon) Weapon->SetGuardPressed(false);
}

void AVortexCharacter::SetAimHeld(bool bHeld)
{
    bAimHeld = bHeld;
    UpdateAimFacing();
    if (IsLocallyControlled() && !bGodotBenchCamera)
    {
        bCameraTransitionActive = true;
        SetActorTickEnabled(true);
    }
}

void AVortexCharacter::ServerSetAimHeld_Implementation(bool bHeld)
{
    SetAimHeld(bHeld);
}

void AVortexCharacter::ServerBeginAttackFacing_Implementation()
{
    bAttackFacing = true;
    UpdateAimFacing();
    GetWorldTimerManager().SetTimer(AttackFacingTimer, this, &AVortexCharacter::FinishAttackFacing, 0.9f, false);
}

void AVortexCharacter::FinishAttackFacing()
{
    bAttackFacing = false;
    bMeleeFacing = false;
    UpdateAimFacing();
}

void AVortexCharacter::TickPotion()
{
    if (GetNetMode() == NM_DedicatedServer) return;
    const bool bDrinking = IsDrinking();
    if (bDrinking != bWasDrinking)
    {
        bWasDrinking = bDrinking;
        if (bDrinking) VortexAudio::Play(this, TEXT("drink"), GetActorLocation(), .9f, .05f);
    }
    // a garrafinha na mão esquerda (no corpo e na primeira pessoa), da cor da poção
    auto Ensure = [this](TObjectPtr<UStaticMeshComponent>& Bottle, USkeletalMeshComponent* Hand, bool bView)
    {
        if (!Hand) return;
        if (!Bottle)
        {
            Bottle = NewObject<UStaticMeshComponent>(this);
            Bottle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Bottle->SetCastShadow(!bView);
            Bottle->SetOnlyOwnerSee(bView);
            Bottle->RegisterComponent();
        }
        if (Bottle->GetAttachParent() != Hand)
        {
            // no "soquete" da mão esquerda: o mesmo afastamento do soquete da arma em relação à mão direita
            FTransform Offset = FTransform::Identity;
            const FReferenceSkeleton& Ref = Hand->GetSkeletalMeshAsset() ? Hand->GetSkeletalMeshAsset()->GetRefSkeleton() : GetMesh()->GetSkeletalMeshAsset()->GetRefSkeleton();
            const int32 Socket = Ref.FindBoneIndex(TEXT("Socket_Weapon_R"));
            if (Socket != INDEX_NONE) Offset = Ref.GetRefBonePose()[Socket];
            Bottle->AttachToComponent(Hand, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("Hand_L"));
            Bottle->SetRelativeTransform(Offset);
        }
    };
    if (bDrinking || PotionBody)
    {
        Ensure(PotionBody, GetMesh(), false);
        if (ViewBody.Num()) Ensure(PotionView, ViewBody[0], true);
        const FName Item = Sheet ? Sheet->GetDrinkItem() : NAME_None;
        const TCHAR* Model = Item == TEXT("potion_blue") ? TEXT("W_pocao_azul") : Item == TEXT("potion_green") || Item == TEXT("bread") ? TEXT("W_pocao_verde") : TEXT("W_pocao_vermelha");
        UStaticMesh* BottleMesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Weapons/Models/%s/StaticMeshes/%s.%s"), Model, Model, Model));
        if (!BottleMesh) BottleMesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Weapons/Models/%s.%s"), Model, Model));
        const bool bFirst = bFirstPerson && FirstPersonBlend > .5f;
        for (UStaticMeshComponent* Bottle : { PotionBody.Get(), PotionView.Get() })
        {
            if (!Bottle) continue;
            if (bDrinking && Bottle->GetStaticMesh() != BottleMesh) Bottle->SetStaticMesh(BottleMesh);
            const bool bView = Bottle == PotionView;
            Bottle->SetVisibility(bDrinking && (bView ? bFirst && !bHeroModel : true));
            if (!bView) Bottle->SetOwnerNoSee(bFirst && !bHeroModel);
        }
    }
}

bool AVortexCharacter::IsDrinking() const
{
    return Sheet && Sheet->IsDrinking();
}

bool AVortexCharacter::IsMeleeWeapon() const
{
    const UVortexWeaponDefinition* Definition = Weapon ? Weapon->GetDefinition() : nullptr;
    return Definition && Definition->IsMelee();
}

AActor* AVortexCharacter::FindMeleeTarget(const FVector& Want, const AActor* Skip) const
{
    // até 4,5 m, num leque de ~80° para cada lado de onde o herói quer bater; o mais perto e mais à frente
    // (até 6 m o herói se virava para um bicho longe e cortava o ar)
    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexMeleeAssist), false, this);
    GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, Objects, FCollisionShape::MakeSphere(450.f), Query);
    AActor* Best = nullptr;
    float BestScore = TNumericLimits<float>::Max();
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Candidate = Overlap.GetActor();
        if (!Candidate || Candidate == Skip || !Combat || !Combat->CanHarm(Candidate)) continue;
        const FVector To = Candidate->GetActorLocation() - GetActorLocation();
        if (FMath::Abs(To.Z) > 300.f) continue;
        const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Candidate->GetRootComponent());
        const float Surface = FMath::Max(0.f, To.Size2D() - (Capsule ? Capsule->GetScaledCapsuleRadius() : 0.f));
        const float Dot = FVector::DotProduct(Want, To.GetSafeNormal2D());
        // colado no herói vale qualquer lado; mais longe, o semicírculo da frente (um bicho ao lado: o herói
        // vira para ele e avança, em vez de cortar o ar para a frente); o mais à frente e mais perto ganha
        if (Surface > 120.f && Dot < -.1f) continue;
        const float Score = Surface * (1.8f - Dot);
        if (Score < BestScore) { BestScore = Score; Best = Candidate; }
    }
    return Best;
}

AActor* AVortexCharacter::PreviewMeleeTarget() const
{
    auto Edge = [this](const AActor* Foe)
    {
        const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Foe->GetRootComponent());
        return FVector::Dist2D(Foe->GetActorLocation(), GetActorLocation()) - (Capsule ? Capsule->GetScaledCapsuleRadius() : 0.f);
    };
    // o alcance do primeiro golpe da arma (o mesmo que decide o acerto)
    const UVortexWeaponDefinition* Def = Weapon ? Weapon->GetDefinition() : nullptr;
    const float Reach = Def && Def->Swings.Num() ? Weapon->MeleeReach(Def->Swings[0]) : 150.f;
    // para onde o herói anda (ou está virado) e, sem nada lá, para onde a câmera olha
    FVector Want = GetLastMovementInputVector().GetSafeNormal2D();
    if (Want.IsNearlyZero()) Want = GetActorForwardVector().GetSafeNormal2D();
    AActor* Found = FindMeleeTarget(Want);
    if (!Found)
    {
        const FVector CameraForward = Controller ? FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f).Vector() : GetActorForwardVector();
        Found = FindMeleeTarget(CameraForward);
    }
    // o foco preso continua enquanto o inimigo estiver vivo e perto (até ~5,5 m da borda), para não pular sozinho de
    // um bicho para outro no meio da luta; mas se ele está fora do alcance e outro está ao alcance da arma, o golpe vai
    // no de perto (antes o herói se virava para o preso lá longe e cortava o ar com um bicho colado nele)
    if (AActor* Locked = FocusLock.Get())
    {
        const bool bAlive = Combat && Combat->CanHarm(Locked);
        const float LockedEdge = bAlive ? Edge(Locked) : 1e9f;
        const bool bCloser = Found && Found != Locked && LockedEdge > Reach && Edge(Found) <= Reach;
        if (bAlive && LockedEdge < 550.f && !bCloser) return Locked;
        FocusLock = nullptr;
    }
    FocusLock = Found;
    return Found;
}

void AVortexCharacter::FlickFocus()
{
    // depois da virada, a câmera aponta para onde o jogador quer: o inimigo mais à frente dela, que não seja o atual
    const FVector CameraForward = Controller ? FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f).Vector() : GetActorForwardVector();
    if (AActor* Other = FindMeleeTarget(CameraForward, FocusLock.Get())) FocusLock = Other;
}

void AVortexCharacter::BeginMeleeFacing()
{
    // procura o alvo para onde o herói anda (ou está virado) e, sem nada lá, para onde a câmera olha;
    // sem alvo nenhum, o golpe sai para a frente da câmera (andando para trás ele não se vira de costas)
    const FVector CameraForward = Controller ? FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f).Vector() : GetActorForwardVector();
    FVector Want = GetLastMovementInputVector().GetSafeNormal2D();
    if (Want.IsNearlyZero()) Want = GetActorForwardVector().GetSafeNormal2D();
    AActor* Found = PreviewMeleeTarget();
    if (!Found) Want = CameraForward;
    MeleeTarget = Found;
    MeleeYaw = (Found ? (Found->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() : Want).Rotation().Yaw;
    bMeleeFacing = true;
    bAttackFacing = false;
    MeleeFacingAge = 0.f;
    UpdateAimFacing();
    GetWorldTimerManager().SetTimer(AttackFacingTimer, this, &AVortexCharacter::FinishAttackFacing, 0.7f, false);
    if (!HasAuthority()) ServerBeginMeleeFacing(Found, MeleeYaw);
}

void AVortexCharacter::ServerBeginMeleeFacing_Implementation(AActor* InTarget, float Yaw)
{
    MeleeTarget = InTarget && FVector::Dist(InTarget->GetActorLocation(), GetActorLocation()) < 1200.f ? InTarget : nullptr;
    MeleeYaw = FMath::IsFinite(Yaw) ? Yaw : GetActorRotation().Yaw;
    bMeleeFacing = true;
    bAttackFacing = false;
    MeleeFacingAge = 0.f;
    UpdateAimFacing();
    GetWorldTimerManager().SetTimer(AttackFacingTimer, this, &AVortexCharacter::FinishAttackFacing, 0.7f, false);
}

void AVortexCharacter::TickMeleeFacing(float DeltaSeconds)
{
    MeleeFacingAge += DeltaSeconds;
    float Yaw = MeleeYaw;
    if (const AActor* Foe = MeleeTarget.Get())
    {
        const FVector To = Foe->GetActorLocation() - GetActorLocation();
        if (!To.IsNearlyZero()) Yaw = To.Rotation().Yaw;
    }
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Yaw, 0.f), DeltaSeconds, 20.f));
}

void AVortexCharacter::ToggleFirstPerson()
{
    if (!IsLocallyControlled() || bGodotBenchCamera || VortexHeroWindow::IsOpen()) return;
    bFirstPerson = !bFirstPerson;
    bCameraInsideBody = false;
    bMeleeFacing = false;
    UpdateAimFacing();
    ApplyFirstPersonVisibility();
    if (!bFirstPerson) bCameraTransitionActive = true;
    SetActorTickEnabled(true);
}

void AVortexCharacter::SetViewModelVisible(bool bVisible)
{
    // heróis novos: primeira pessoa de corpo inteiro (como os jogos de "full body awareness"): os olhos ficam no próprio
    // corpo, só a cabeça some (bHideHead); olhando para baixo aparecem o tronco, as pernas e as mãos, e no golpe o braço e
    // a arma de verdade passam na frente. Sem a cópia dos braços presa à câmera
    if (bHeroModel)
    {
        // o corpo sem a cabeça (só para quem joga) no lugar do corpo inteiro; os outros e a sombra continuam com o inteiro
        USkeletalMesh* Whole = GetMesh()->GetSkeletalMeshAsset();
        USkeletalMesh* Headless = Whole ? HeroModelFor(FName(*(Whole->GetName().RightChop(5) + TEXT("_FP")))) : nullptr;
        if (Headless && !FirstPersonBody)
        {
            FirstPersonBody = NewObject<USkeletalMeshComponent>(this, TEXT("FirstPersonBody"));
            FirstPersonBody->SetupAttachment(GetMesh());
            FirstPersonBody->SetOnlyOwnerSee(true);
            FirstPersonBody->SetCastShadow(false);
            FirstPersonBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            FirstPersonBody->SetReceivesDecals(false);
            FirstPersonBody->bUseBoundsFromLeaderPoseComponent = true;
            FirstPersonBody->RegisterComponent();
        }
        if (FirstPersonBody && Headless && FirstPersonBody->GetSkeletalMeshAsset() != Headless)
        {
            FirstPersonBody->SetSkeletalMesh(Headless);
            FirstPersonBody->SetLeaderPoseComponent(GetMesh(), true);
        }
        if (FirstPersonBody)
            for (int32 m = 0; m < FirstPersonBody->GetNumMaterials(); ++m)
                if (FirstPersonBody->GetMaterial(m) != GetMesh()->GetMaterial(0)) FirstPersonBody->SetMaterial(m, GetMesh()->GetMaterial(0));
        const bool bHeadless = bVisible && FirstPersonBody && Headless;
        GetMesh()->SetOwnerNoSee(bHeadless);
        GetMesh()->bCastHiddenShadow = true;
        SyncArmorView(this, bVisible, false);   // 07/11: o capacete some na primeira pessoa
        if (FirstPersonBody) FirstPersonBody->SetVisibility(bHeadless);
        // 08/11: com arco, a tela mostra só o arco preso à câmera, como no Minecraft (UVortexWeaponParts::TickBowString); os braços
        // e o arco do corpo somem só para quem joga
        const UVortexWeaponDefinition* Held = Weapon ? Weapon->GetDefinition() : nullptr;
        const VortexWeaponKit::FKit* HeldKit = Held ? VortexWeaponKit::Find(Held->WeaponId) : nullptr;
        const bool bBowView = bVisible && HeldKit && HeldKit->Family == TEXT("Bow");
        if (UVortexAnimInstance* Anim = Cast<UVortexAnimInstance>(GetMesh()->GetAnimInstance())) Anim->bHideArms = bBowView;
        if (WeaponVisual) WeaponVisual->SetOwnerNoSee(bBowView);
        if (WeaponVisualLeft) WeaponVisualLeft->SetOwnerNoSee(bBowView);
        for (USkeletalMeshComponent* Part : ViewBody) if (Part) Part->SetVisibility(false);
        if (ViewRight) ViewRight->SetVisibility(bBowView && ViewRight->GetStaticMesh() != nullptr);
        if (ViewLeft) ViewLeft->SetVisibility(false);
        return;
    }
    // G5: em primeira pessoa o corpo de verdade some só para quem joga (a sombra continua) e a cópia
    // presa à câmera mostra os braços e a arma, com a mesma animação do corpo
    if (WeaponVisual) WeaponVisual->SetOwnerNoSee(bVisible);
    if (WeaponVisualLeft) WeaponVisualLeft->SetOwnerNoSee(bVisible);
    for (USkeletalMeshComponent* Part : WardrobeParts)
    {
        if (!Part) continue;
        Part->SetOwnerNoSee(bVisible);
        Part->bCastHiddenShadow = true;
    }
    if (WardrobeParts.IsEmpty() || bHeroModel) { GetMesh()->SetOwnerNoSee(bVisible); GetMesh()->bCastHiddenShadow = true; }
    if (HairPart) HairPart->SetOwnerNoSee(bVisible);
    if (OutfitPart) OutfitPart->SetOwnerNoSee(GetMesh()->bOwnerNoSee);
    for (int32 i = 0; i < ViewBody.Num(); ++i)
        if (USkeletalMeshComponent* Part = ViewBody[i])
            // o corpo-cópia só aparece se o herói não tiver peças de roupa (senão elas aparecem por cima dele)
            Part->SetVisibility(bVisible && Part->GetSkeletalMeshAsset() != nullptr && (i > 0 || WardrobeParts.IsEmpty() || bHeroModel));
    const uint8 Thrown = Weapon ? Weapon->GetThrowMask() : 0;
    if (ViewRight) ViewRight->SetVisibility(bVisible && ViewRight->GetStaticMesh() != nullptr && !(Thrown & 1));
    if (ViewLeft) ViewLeft->SetVisibility(bVisible && ViewLeft->GetStaticMesh() != nullptr && !(Thrown & 2));
}

void AVortexCharacter::TickViewModel(float DeltaSeconds)
{
    if (!ViewRoot)
    {
        ViewRoot = NewObject<USceneComponent>(this, TEXT("ViewRoot"));
        ViewRoot->SetupAttachment(FollowCamera);
        ViewRoot->RegisterComponent();
        auto Make = [this](const TCHAR* Name)
        {
            UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, Name);
            Part->SetupAttachment(ViewRoot);
            Part->SetOnlyOwnerSee(true);
            Part->SetCastShadow(false);
            Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Part->bAffectDistanceFieldLighting = false;
            Part->RegisterComponent();
            return Part;
        };
        ViewRight = Make(TEXT("ViewRight"));
        ViewLeft = Make(TEXT("ViewLeft"));
    }
    // a cópia das peças do corpo (cabeça, pernas e costas ficam de fora: nunca entram na tela)
    static const FName Skip[] = { TEXT("Face"), TEXT("Hair"), TEXT("Eyes"), TEXT("Mouth"), TEXT("Accessory"), TEXT("AccessoryFace"),
        TEXT("AccessoryBack"), TEXT("Pants"), TEXT("Shoes") };
    // a cópia: um corpo com a própria animação (a mesma do herói, só com os braços) e as peças de
    // roupa seguindo ele; sem peças de roupa, o próprio corpo-cópia aparece
    if (ViewBody.IsEmpty())
    {
        USkeletalMeshComponent* Driver = NewObject<USkeletalMeshComponent>(this, TEXT("ViewDriver"));
        Driver->SetupAttachment(ViewRoot);
        Driver->SetOnlyOwnerSee(true);
        Driver->SetCastShadow(false);
        Driver->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Driver->SetReceivesDecals(false);
        Driver->SetBoundsScale(4.f);
        Driver->bNeverDistanceCull = true;
        Driver->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Driver->SetSkeletalMesh(GetMesh()->GetSkeletalMeshAsset());
        Driver->SetAnimInstanceClass(UVortexAnimInstance::StaticClass());
        Driver->RegisterComponent();
        ViewBody.Add(Driver);
    }
    USkeletalMeshComponent* Driver = ViewBody[0];
    // o corpo trocou depois que a cópia nasceu (o herói novo não usa a cópia: SetViewModelVisible esconde)
    if (Driver->GetSkeletalMeshAsset() != GetMesh()->GetSkeletalMeshAsset()) Driver->SetSkeletalMesh(GetMesh()->GetSkeletalMeshAsset());
    if (UVortexAnimInstance* Anim = Cast<UVortexAnimInstance>(Driver->GetAnimInstance()))
    {
        Anim->bArmsOnly = true;
    }
    for (int32 m = 0; m < GetMesh()->GetNumMaterials(); ++m)
        if (Driver->GetMaterial(m) != GetMesh()->GetMaterial(m)) Driver->SetMaterial(m, GetMesh()->GetMaterial(m));
    // as mãos dos heróis novos são bem maiores que as do aventureiro: na primeira pessoa tomavam metade da tela
    Driver->SetRelativeTransform(FTransform(GetMesh()->GetRelativeRotation(), FVector::ZeroVector, GetMesh()->GetRelativeScale3D() * (bHeroModel ? .7f : 1.f)));
    while (ViewBody.Num() < WardrobeParts.Num() + 1)
    {
        USkeletalMeshComponent* Part = NewObject<USkeletalMeshComponent>(this);
        Part->SetupAttachment(Driver);
        Part->SetOnlyOwnerSee(true);
        Part->SetCastShadow(false);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetReceivesDecals(false);
        Part->bUseBoundsFromLeaderPoseComponent = true;
        Part->RegisterComponent();
        ViewBody.Add(Part);
    }
    for (int32 i = 0; i < WardrobeParts.Num(); ++i)
    {
        USkeletalMeshComponent* From = WardrobeParts[i];
        USkeletalMeshComponent* To = ViewBody[i + 1];
        if (!From || !To) continue;
        USkeletalMesh* Want = From->GetSkeletalMeshAsset();
        if (To->GetSkeletalMeshAsset() != Want)
        {
            To->SetSkeletalMesh(Want);
            if (Want) To->SetLeaderPoseComponent(Driver, true);
        }
        for (int32 m = 0; Want && m < From->GetNumMaterials(); ++m)
            if (To->GetMaterial(m) != From->GetMaterial(m)) To->SetMaterial(m, From->GetMaterial(m));
    }
    USkeletalMeshComponent* Holder = Driver;

    // a arma na mão da cópia, no mesmo osso da arma do corpo
    const UVortexWeaponDefinition* Definition = Weapon ? Weapon->GetDefinition() : nullptr;
    if (Holder && Definition)
    {
        const FName Bone = UVortexWeaponComponent::HandBoneFor(Definition, Holder);
        const VortexWeaponKit::FKit* HeldKit = VortexWeaponKit::Find(Definition->WeaponId);
        if (bHeroModel && HeldKit && HeldKit->Family == TEXT("Bow"))
        {
            if (ViewRight->GetAttachParent() != FollowCamera) ViewRight->AttachToComponent(FollowCamera, FAttachmentTransformRules::KeepRelativeTransform);
        }
        else if (ViewRight->GetAttachParent() != Holder || ViewRight->GetAttachSocketName() != Bone)
        {
            ViewRight->AttachToComponent(Holder, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Bone);
            ViewRight->SetRelativeRotation(UVortexWeaponComponent::GripRotationFor(Bone));
            ViewRight->SetRelativeScale3D(FVector::OneVector);
        }
        if (ViewLeft->GetAttachParent() != Holder)
            ViewLeft->AttachToComponent(Holder, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("Hand_L"));
        if (ViewRight->GetStaticMesh() != Definition->VisualMesh) ViewRight->SetStaticMesh(Definition->VisualMesh);
        if (ViewLeft->GetStaticMesh() != Definition->LeftMesh) ViewLeft->SetStaticMesh(Definition->LeftMesh);
        if (WeaponVisual)
            for (int32 m = 0; m < WeaponVisual->GetNumMaterials(); ++m) if (ViewRight->GetMaterial(m) != WeaponVisual->GetMaterial(m)) ViewRight->SetMaterial(m, WeaponVisual->GetMaterial(m));
        if (WeaponVisualLeft)
            for (int32 m = 0; m < WeaponVisualLeft->GetNumMaterials(); ++m) if (ViewLeft->GetMaterial(m) != WeaponVisualLeft->GetMaterial(m)) ViewLeft->SetMaterial(m, WeaponVisualLeft->GetMaterial(m));
    }
    else
    {
        ViewRight->SetStaticMesh(nullptr);
        ViewLeft->SetStaticMesh(nullptr);
    }
    // 08/11: com a janela do herói aberta, o corpo inteiro (a vitrine filmava o corpo sem cabeça da primeira pessoa)
    SetViewModelVisible(FirstPersonBlend > .5f && !VortexHeroWindow::IsOpen());
#if !UE_BUILD_SHIPPING
    if (FirstPersonBlend > .5f && GFrameCounter % 120 == 0)
    {
        int32 Shown = 0;
        for (USkeletalMeshComponent* Part : ViewBody) Shown += Part && Part->IsVisible() && Part->GetSkeletalMeshAsset();
        const FTransform Cam = FollowCamera->GetComponentTransform();
        auto InCam = [&](const FVector& W) { return Cam.InverseTransformPosition(W).ToCompactString(); };
        UE_LOG(LogTemp, Display, TEXT("VORTEX_VIEWBODY parts=%d shown=%d root=%s handR=%s head=%s torso=%s scale=%s meshrel=%s"), ViewBody.Num(), Shown,
            *InCam(ViewRoot->GetComponentLocation()), Holder ? *InCam(Holder->GetSocketLocation(TEXT("Hand_R"))) : TEXT("-"),
            Holder ? *InCam(Holder->GetSocketLocation(TEXT("Head"))) : TEXT("-"), Holder ? *InCam(Holder->GetSocketLocation(TEXT("Torso"))) : TEXT("-"),
            Holder ? *Holder->GetComponentScale().ToCompactString() : TEXT("-"), *GetMesh()->GetRelativeTransform().ToString());
    }
#endif
    if (FirstPersonBlend <= .5f) return;

    // balanço ao andar e um atraso leve ao girar a câmera
    const float Speed = GetVelocity().Size2D();
    ViewBob += DeltaSeconds * FMath::Clamp(Speed / 460.f, 0.f, 1.6f) * 9.f;
    const float Walk = FMath::Clamp(Speed / 460.f, 0.f, 1.f);
    const FVector Bob(0.f, FMath::Sin(ViewBob * .5f) * 1.2f * Walk, FMath::Abs(FMath::Sin(ViewBob * .5f)) * -1.f * Walk);
    const FRotator Camera = FollowCamera->GetComponentRotation();
    const FRotator Turn = (Camera - ViewLastCamera).GetNormalized();
    ViewLastCamera = Camera;
    ViewSway = FMath::Vector2DInterpTo(ViewSway, FVector2D(FMath::Clamp(Turn.Yaw * -.5f, -3.f, 3.f), FMath::Clamp(Turn.Pitch * .5f, -3.f, 3.f)), DeltaSeconds, 10.f);
    // os pés do corpo-cópia ficam à frente e abaixo dos olhos: as mãos (que no aventureiro ficam ~20 cm
    // à frente do peito, na altura da barriga) entram na metade de baixo da tela
    const float Stature = BodyShape.Stature();
    const VortexWeaponKit::FKit* ViewKit = Definition ? VortexWeaponKit::Find(Definition->WeaponId) : nullptr;
    // âncora: parado ou andando, a mão que segura a arma cai num ponto fixo da tela (embaixo e de lado,
    // aparecendo só em parte), escolhido pela família. O corpo-cópia usa as animações da terceira pessoa, então
    // cada arma deixava a mão num lugar: no meio da tela, fora dela (a espada, ~42° abaixo) ou grande demais.
    // A pose de descanso é medida aos poucos (fora dos golpes, da defesa e da poção) e congela durante eles:
    // o golpe atravessa a tela a partir do ponto de descanso
    FName AnchorBone = TEXT("Hand_R");
    // a katana não tem kit (usa os golpes da espada), mas é segurada com as duas mãos
    const FName ViewFamily = ViewKit ? ViewKit->Family : Definition && Definition->WeaponId == TEXT("katana_do_vento") ? FName(TEXT("Katana")) : NAME_None;
    const FVector AnchorAt = VortexWeaponKit::ViewAnchor(ViewFamily, AnchorBone);
    if (ViewRestWeapon != Definition) { bViewRestValid = false; ViewRestWeapon = Definition; }
    const bool bBothHands = AnchorBone == TEXT("Hands");
    if (Holder && (bBothHands || Holder->DoesSocketExist(AnchorBone)))
    {
        const FVector Hand = bBothHands ? (Holder->GetSocketLocation(TEXT("Hand_R")) + Holder->GetSocketLocation(TEXT("Hand_L"))) * .5f
            : Holder->GetSocketLocation(AnchorBone);
        const FVector HandInRoot = ViewRoot->GetComponentTransform().InverseTransformPosition(Hand);
        const UVortexAnimInstance* ViewAnim = Cast<UVortexAnimInstance>(Holder->GetAnimInstance());
        const bool bBusy = ViewAnim && (ViewAnim->UpperBodyAlpha > .05f || ViewAnim->DrinkAlpha > .05f);
        if (!bViewRestValid) { ViewRest = HandInRoot; bViewRestValid = true; }
        else if (!bBusy) ViewRest = FMath::VInterpTo(ViewRest, HandInRoot, DeltaSeconds, 3.f);
    }
    const FVector Anchor = bViewRestValid ? AnchorAt - ViewRest : FVector(ViewForward, 0.f, -ViewDrop * Stature);
    // o meio da tela fica sempre livre: se a mão ou a arma entram num círculo de ~15° em volta da mira
    // (o soco, o arco puxado, o cabo da alabarda), o corpo-cópia desliza para fora (de lado e para baixo)
    FVector2D Push = FVector2D::ZeroVector;
    if (Holder)
    {
        const FTransform Cam = FollowCamera->GetComponentTransform();
        TArray<FVector, TInlineAllocator<8>> Points = { Holder->GetSocketLocation(TEXT("Hand_R")) };
        // bebendo, a mão esquerda vai ao rosto de propósito
        if (!IsDrinking()) Points.Add(Holder->GetSocketLocation(TEXT("Hand_L")));
        for (UStaticMeshComponent* Held : { ViewRight.Get(), ViewLeft.Get() })
        {
            if (!Held || !Held->GetStaticMesh() || !Held->IsVisible()) continue;
            const FBox Box = Held->GetStaticMesh()->GetBoundingBox();
            for (const float X : { .2f, .45f, .7f, 1.f })
                for (const FVector& Corner : { Box.Min, Box.Max })
                    Points.Add(Held->GetComponentTransform().TransformPosition(FMath::Lerp(Box.GetCenter(), Corner, X)));
        }
        const float Clear = FMath::Tan(FMath::DegreesToRadians(15.f));
        for (const FVector& World : Points)
        {
            const FVector P = Cam.InverseTransformPosition(World);
            if (P.X < 4.f) continue;
            const FVector2D Off(P.Y, P.Z);
            const float Need = P.X * Clear - Off.Size();
            if (Need <= 0.f) continue;
            // para fora pelo lado em que já está (no meio exato: para a direita e para baixo)
            const FVector2D Dir = Off.Size() > 1.f ? Off.GetSafeNormal() : FVector2D(.6f, -.8f);
            const FVector2D Out = (Dir + FVector2D(0.f, -.35f)).GetSafeNormal() * Need;
            if (Out.SizeSquared() > Push.SizeSquared()) Push = Out;
        }
    }
    ViewAvoid = FMath::Vector2DInterpTo(ViewAvoid, ViewAvoid + Push, DeltaSeconds, 18.f);
    ViewAvoid = FMath::Vector2DInterpTo(ViewAvoid, FVector2D::ZeroVector, DeltaSeconds, Push.IsNearlyZero() ? 2.5f : 0.f);
    ViewAvoid = FVector2D(FMath::Clamp(ViewAvoid.X, -45.f, 45.f), FMath::Clamp(ViewAvoid.Y, -45.f, 20.f));
    ViewRoot->SetRelativeLocation(Anchor + FVector(0.f, ViewAvoid.X, ViewAvoid.Y) + Bob + FVector(0.f, ViewSway.X, ViewSway.Y));
}

void AVortexCharacter::ApplyFirstPersonVisibility()
{
    // a cabeça some só para quem está dentro dela (a sombra continua); o resto do corpo e a arma
    // aparecem ao olhar para baixo
    static const FName Head[] = { TEXT("Face"), TEXT("Hair"), TEXT("Eyes"), TEXT("Mouth"), TEXT("Accessory"), TEXT("AccessoryFace"), TEXT("Race") };
    const bool bHide = bFirstPerson && FirstPersonBlend > .5f && !VortexHeroWindow::IsOpen();
    for (USkeletalMeshComponent* Part : WardrobeParts)
    {
        if (!Part) continue;
        bool bIsHead = false;
        for (const FName& Name : Head) bIsHead |= Part->GetFName() == Name;
        if (!bIsHead) continue;
        Part->SetOwnerNoSee(bHide);
        Part->bCastHiddenShadow = true;
    }
}

void AVortexCharacter::TickFirstPerson(float DeltaSeconds)
{
    const float Before = FirstPersonBlend;
    FirstPersonBlend = FMath::FInterpConstantTo(FirstPersonBlend, bFirstPerson ? 1.f : 0.f, DeltaSeconds, 4.f);
    static bool bWindowWasOpen = false;
    const bool bWindowOpen = VortexHeroWindow::IsOpen();
    if ((Before <= .5f) != (FirstPersonBlend <= .5f) || bWindowOpen != bWindowWasOpen) ApplyFirstPersonVisibility();
    bWindowWasOpen = bWindowOpen;
    const float A = FMath::SmoothStep(0.f, 1.f, FirstPersonBlend);
    const float Stature = BodyShape.Stature();
    // olhos: um pouco à frente do rosto, na altura da raça; balanço leve ao andar
    const float Speed = GetVelocity().Size2D();
    const bool bGround = GetCharacterMovement()->IsMovingOnGround();
    BobPhase += DeltaSeconds * (bGround ? FMath::Clamp(Speed / 460.f, 0.f, 1.6f) : 0.f) * 9.f;
    const float Bob = bGround ? FMath::Sin(BobPhase) * FMath::Clamp(Speed / 460.f, 0.f, 1.f) * 2.2f : 0.f;
    // heróis novos (cabeça grande): os olhos mais baixos e quase no rosto, para ver o próprio corpo olhando para baixo
    // olhando para baixo os olhos avançam até a frente do peito (como no Fortnite): o corpo aparece por fora, não pelo buraco
    // do pescoço do corpo sem a cabeça
    const float Down = Controller ? FMath::Clamp(-FRotator::NormalizeAxis(Controller->GetControlRotation().Pitch) / 75.f, 0.f, 1.f) : 0.f;
    // correndo, a animação inclina o corpo para a frente: os olhos vão junto (olhando para baixo o corpo aparecia muito à
    // frente da câmera)
    const float RunAhead = bHeroModel && bGround ? FMath::Clamp((Speed - 160.f) / 300.f, 0.f, 1.f) * (5.f + 13.f * Down) * Stature : 0.f;
    FVector Eye = bHeroModel ? FVector((8.f + 14.f * Down) * Stature + RunAhead, 0.f, (44.f - 4.f * Down) * Stature + Bob + MountLift)
        : FVector(22.f, 0.f, 56.f * Stature + Bob + MountLift);
    // 03/11 (o usuário: "correndo e olhando para baixo ainda vejo dentro do tronco"): os olhos nunca ficam atrás da frente
    // do peito de verdade (o osso Spine2 da pose de agora, que a corrida inclina), olhando para baixo
    if (bHeroModel && Down > .05f)
        for (const TCHAR* Name : { TEXT("mixamorig_Spine2"), TEXT("mixamorig:Spine2") })
            if (GetMesh()->GetBoneIndex(Name) != INDEX_NONE)
            {
                const float ChestX = GetActorTransform().InverseTransformPosition(GetMesh()->GetBoneLocation(Name)).X;
                const float Front = ChestX + (15.f + 9.f * Down) * Stature;
                Eye.X = FMath::Lerp(Eye.X, FMath::Max(Eye.X, Front), FMath::Clamp(Down * 3.f, 0.f, 1.f));
                break;
            }
    // a cabeça encolhida na pose (o corpo do aventureiro e os heróis do Mixamo); os heróis com corpo sem a cabeça
    // (Hero_<raça>_FP) usam ele
    const bool bHeadlessBody = bHeroModel && FirstPersonBody && FirstPersonBody->IsVisible();
    if (UVortexAnimInstance* Anim = Cast<UVortexAnimInstance>(GetMesh()->GetAnimInstance())) Anim->bHideHead = FirstPersonBlend > .5f && !bHeadlessBody && !bWindowOpen;
    TickViewModel(DeltaSeconds);
    const FVector Third(0.f, 0.f, 45.f * Stature);
    const float Distance = bAimHeld ? FMath::Min(WantedCameraDistance, 210.f) : WantedCameraDistance;
    const FVector ShoulderOffset = CameraOffsetFor(Distance, CameraShoulderSign, bAimHeld);
    CameraBoom->SetRelativeLocation(FMath::Lerp(Third, Eye, A));
    CameraBoom->TargetArmLength = FMath::Lerp(Distance, 0.f, A);
    CameraBoom->SocketOffset = FMath::Lerp(ShoulderOffset, FVector::ZeroVector, A);
    CameraBoom->bDoCollisionTest = A < .9f;
    // o atraso de seguir só fora da primeira pessoa (dentro da cabeça ele balançaria a visão)
    CameraBoom->bEnableCameraLag = A < .05f && !bGodotBenchCamera;
    FollowCamera->SetFieldOfView(FMath::Lerp(VortexGraphics::FieldOfView(), VortexGraphics::FirstPersonFieldOfView(), A) - (bAimHeld ? 7.f : 0.f));
    if (!bFirstPerson && FirstPersonBlend <= 0.f)
    {
        CameraBoom->bDoCollisionTest = true;
        bCameraTransitionActive = true;
    }
}

void AVortexCharacter::UpdateAimFacing()
{
    if (bFirstPerson)
    {
        // primeira pessoa: o corpo vira junto com o olhar
        bUseControllerRotationYaw = true;
        GetCharacterMovement()->bOrientRotationToMovement = false;
        return;
    }
    const bool bFacingAim = bAimHeld || (bAttackFacing && !bMeleeFacing);
    bUseControllerRotationYaw = bFacingAim;
    GetCharacterMovement()->bOrientRotationToMovement = !bFacingAim && !bMeleeFacing;
    if (bFacingAim && Controller)
    {
        SetActorRotation(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
    }
}

void AVortexCharacter::CastClassSkill(int32 Slot)
{
    if (CancelDrinkInstead()) return;
    if (VortexHeroWindow::IsOpen() || VortexDialogue::IsOpen() || !ClassSkills || Mount) return;
    ClassSkills->Press(Slot);
}

void AVortexCharacter::Ping()
{
    if (!IsLocallyControlled() || VortexHeroWindow::IsOpen() || GetWorld()->GetTimeSeconds() < NextPingTime) return;
    // 03/11: Shift+Z troca a cor do jogador (o ping, o minimapa e o mapa)
    if (const APlayerController* Player = Cast<APlayerController>(GetController());
        Player && (Player->IsInputKeyDown(EKeys::LeftShift) || Player->IsInputKeyDown(EKeys::RightShift)) && Social)
    {
        Social->CyclePlayerColor();
        NextPingTime = GetWorld()->GetTimeSeconds() + .25f;
        return;
    }
    NextPingTime = GetWorld()->GetTimeSeconds() + .6f;
    // o que está sob a mira (até 150 m); criatura perto do ponto também conta (mirar num bicho é difícil)
    FHitResult Hit;
    const bool bHit = TraceAim(Hit, 15000.f);
    const FVector At = bHit ? Hit.ImpactPoint : Hit.TraceEnd;
    AActor* Target = nullptr;
    float Best = 250.f;
    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        const IVortexCreature* Creature = Cast<IVortexCreature>(*It);
        if (!Creature || Creature->IsCreatureDead()) continue;
        const float D = FMath::PointDistToLine(It->GetActorLocation(), (At - FollowCamera->GetComponentLocation()).GetSafeNormal(), FollowCamera->GetComponentLocation());
        if (D < Best + It->GetSimpleCollisionRadius() && FVector::Dist(It->GetActorLocation(), GetActorLocation()) < 15000.f) { Best = D; Target = *It; }
    }
    if (bHit && Hit.GetActor() && Cast<IVortexCreature>(Hit.GetActor())) Target = Hit.GetActor();
    ServerPing(At, Target);
}

void AVortexCharacter::ServerPing_Implementation(FVector_NetQuantize At, AActor* Target)
{
    const IVortexCreature* Creature = Cast<IVortexCreature>(Target);
    const bool bDanger = Creature && !Creature->IsPassive();
    MulticastPing(At, Target, bDanger);
}

void AVortexCharacter::MulticastPing_Implementation(FVector_NetQuantize At, AActor* Target, bool bDanger)
{
    if (GetNetMode() == NM_DedicatedServer) return;
    // 03/11: o nome do herói (antes ia o nome do computador) e a cor de quem marcou
    FString Who = Social ? Social->GetHeroName() : FString();
    if (Who.Len() > 24) Who.Empty();
    VortexPings::Add(GetWorld(), At, Target, bDanger, Who, Social ? Social->GetPlayerColor() : FLinearColor(.35f, .9f, 1.f));
}

void AVortexCharacter::QuickHealth()
{
    if (Sheet && !VortexHeroWindow::IsOpen()) Sheet->ServerQuickUse(TEXT("potion_red"));
}

void AVortexCharacter::QuickMana()
{
    if (Sheet && !VortexHeroWindow::IsOpen()) Sheet->ServerQuickUse(TEXT("potion_blue"));
}

void AVortexCharacter::Interact()
{
    if (!Sheet || VortexHeroWindow::IsOpen() || VortexDialogue::IsOpen()) return;
    // 07/11: no Arsenal, E perto de um pedestal equipa o que está nele
    if (AVortexArsenal* Arsenal = AVortexArsenal::Find(GetWorld()); Arsenal && Arsenal->TryUse(this)) return;
    // 07/11: o item no chão perto (caiu de um bicho, de um baú ou foi jogado): pega
    if (AVortexItemDrop* Drop = AVortexItemDrop::FindNear(GetWorld(), GetActorLocation(), 220.f)) { Sheet->ServerPickup(Drop); return; }
    // montado: E desce do cavalo
    if (Mount) { ServerDismount(); return; }
    // perto de um morador: conversa (falas, missões, entregas); depois o cavalo e os coletáveis
    if (!Sheet->NeedsAwakening())
    {
        // 03/11: os moradores, baús, portais e o cavalo vêm antes; o menu do amigo só colado e de frente para ele
        if (AVortexNpc* Npc = VortexDialogue::FindTalkable(this)) { VortexDialogue::Open(this, Npc); return; }
        if (const AVortexWorldFeatures* Features = AVortexWorldFeatures::Find(GetWorld()))
            if (const AVortexWorldFeatures::FChest* Chest = Features->NearestChest(GetActorLocation(), 240.f, QuestLog)) { ServerOpenChest(Chest->Id); return; }
        // portal de dungeon perto: entrar
        if (AVortexPortal* Portal = AVortexPortal::FindNear(GetWorld(), GetActorLocation(), 320.f)) { ServerEnterPortal(Portal); return; }
        // pontos de interesse: orar no santuário, ler a inscrição das ruínas
        if (const AVortexPointsOfInterest* Pois = AVortexPointsOfInterest::Find(GetWorld()))
        {
            if (const FVortexPoi* Shrine = Pois->NearestShrine(GetActorLocation(), 350.f)) { ServerPray(Shrine->Id); return; }
            if (const FVortexPoi* Lore = Pois->NearestLore(GetActorLocation(), 260.f)) { Sheet->PushNotice(AVortexPointsOfInterest::LoreText(*Lore), 2); return; }
        }
        if (AVortexMonster* Horse = FindHorseNear(340.f)) { ServerHorse(Horse); return; }
        if (AVortexCharacter* Other = VortexSocialUi::FindLookedAt(this)) { VortexSocialUi::OpenMenu(this, Other); return; }
        // 07/11: as portas da vila (abre e fecha)
        bool bDoorWasOpen = false;
        if (AVortexTown::FindDoor(GetWorld(), GetActorLocation(), 230.f, bDoorWasOpen) && AVortexTown::ToggleDoor(GetWorld(), GetActorLocation(), 230.f))
        {
            VortexAudio::Play(this, bDoorWasOpen ? TEXT("door_close") : TEXT("door_open"), GetActorLocation(), .7f);   // Tools/MakeDoorSounds.py
            return;
        }
        if (UVortexForageSubsystem* Forage = GetWorld()->GetSubsystem<UVortexForageSubsystem>())
            if (const FVortexForageSpot* Spot = Forage->Nearest(GetActorLocation(), UVortexForageSubsystem::Reach)) ServerHarvest(Spot->Id);
        return;
    }
    const AVortexAwakeningStadium* Stadium = AVortexAwakeningStadium::Find(GetWorld());
    if (!Stadium) return;
    if (Stadium->IsAtExit(GetActorLocation())) Sheet->ServerLeaveStadium();
    else if (const AVortexAwakeningStadium::FPedestal* Pedestal = Stadium->NearestPedestal(GetActorLocation())) Sheet->ServerStadiumTake(Pedestal->Type);
}

// ---- coletáveis e cavalo ---------------------------------------------------------------------------

AVortexMonster* AVortexCharacter::FindHorseNear(float Range) const
{
    AVortexMonster* Best = nullptr;
    float BestDistance = Range;
    for (TActorIterator<AVortexMonster> It(GetWorld()); It; ++It)
    {
        AVortexMonster* Horse = *It;
        if (!Horse->IsHorse() || Horse->IsCreatureDead() || Horse->GetRider() || (Horse->GetTamer() && Horse->GetTamer() != this)) continue;
        // a distância conta da borda do cavalo (ele é comprido)
        const float D = FVector::Dist2D(Horse->GetActorLocation(), GetActorLocation()) - Horse->GetCapsuleComponent()->GetScaledCapsuleRadius();
        if (D < BestDistance) { Best = Horse; BestDistance = D; }
    }
    return Best;
}

void AVortexCharacter::SetMountLift(float Lift)
{
    MountLift = Lift;
    ApplyBodyShape(BodyShape);
}

void AVortexCharacter::ServerHarvest_Implementation(FName Id)
{
    UVortexForageSubsystem* Forage = GetWorld()->GetSubsystem<UVortexForageSubsystem>();
    const FVortexForageSpot* Spot = Forage ? Forage->Find(Id, GetActorLocation()) : nullptr;
    if (!Spot || !Sheet || !Forage->IsAvailable(Id) || FVector::Dist2D(Spot->At, GetActorLocation()) > UVortexForageSubsystem::Reach + 120.f)
    {
        UE_LOG(LogTemp, Display, TEXT("VORTEX_HARVEST recusado %s spot=%d disponivel=%d distancia=%.0f"), *Id.ToString(), Spot != nullptr,
            Forage && Forage->IsAvailable(Id), Spot ? FVector::Dist2D(Spot->At, GetActorLocation()) : -1.f);
        return;
    }
    // moita de frutas dá 2; ervas e cogumelos, 1 (Main.Forage do Godot)
    const int32 Count = Spot->Item == TEXT("frutas_silvestres") ? 2 : 1;
    if (Sheet->GiveItem(Spot->Item, Count, false) >= Count)
    {
        UE_LOG(LogTemp, Display, TEXT("VORTEX_HARVEST mochila cheia %s"), *Spot->Item.ToString());
        Sheet->PushNotice(FString::Printf(TEXT("Mochila cheia: não deu para colher %s (venda ou jogue fora algo na mochila, I)"), *Spot->Name), 4);
        return;
    }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_HARVEST %s x%d"), *Spot->Item.ToString(), Count);
    if (const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Spot->Item)) Sheet->PushNotice(FString::Printf(TEXT("%s  x%d"), *Def->Name, Count), 3);
    MulticastHarvested(Id, Spot->At);
    // o herói se abaixa e pega (o clipe "Pegar" da raça, se tiver; acaba sozinho ou ao andar)
    if (HasEmote(TEXT("Pegar"))) ServerEmote_Implementation(TEXT("Pegar"));
}

void AVortexCharacter::MulticastHarvested_Implementation(FName Id, FVector_NetQuantize At)
{
    if (UVortexForageSubsystem* Forage = GetWorld()->GetSubsystem<UVortexForageSubsystem>()) Forage->MarkTaken(Id);
    if (GetNetMode() == NM_DedicatedServer) return;
    VortexAudio::Play(this, TEXT("pickup"), At, .8f);
    VortexHitFx::Splash(GetWorld(), At + FVector(0.f, 0.f, 40.f), FLinearColor(.45f, .9f, .35f), 10);
}

bool AVortexCharacter::DebugHarvestNearest()
{
    UVortexForageSubsystem* Forage = GetWorld()->GetSubsystem<UVortexForageSubsystem>();
    const FVortexForageSpot* Spot = Forage ? Forage->Nearest(GetActorLocation(), UVortexForageSubsystem::Reach) : nullptr;
    if (!Spot) return false;
    ServerHarvest_Implementation(Spot->Id);
    return true;
}

void AVortexCharacter::ServerHorse_Implementation(AVortexMonster* Horse)
{
    if (!Horse || !Sheet || Mount || !Horse->IsHorse() || Horse->IsCreatureDead() || Horse->GetRider()) return;
    if (FVector::Dist2D(Horse->GetActorLocation(), GetActorLocation()) > 340.f + Horse->GetCapsuleComponent()->GetScaledCapsuleRadius() + 80.f) return;
    // o seu cavalo: monta
    if (Horse->GetTamer() == this)
    {
        Mount = Horse;
        Horse->SetRider(this);
        SetActorRotation(FRotator(0.f, Horse->GetActorRotation().Yaw, 0.f));
        ForceNetUpdate();
        return;
    }
    if (Horse->GetTamer()) return;
    // selvagem: uma fruta silvestre por vez (TryTame do Godot)
    if (Sheet->TakeItem(TEXT("frutas_silvestres"), 1) < 1)
    {
        Sheet->PushNotice(TEXT("Precisa de frutas silvestres para domar o cavalo"), 4);
        return;
    }
    if (!Horse->Feed(this))
    {
        Sheet->PushNotice(FString::Printf(TEXT("O cavalo comeu (%d de %d)"), Horse->GetTameFed(), Horse->GetTameNeed()), 3);
        return;
    }
    // domado: troca o selvagem (que é da população do mundo) pelo cavalo deste herói, no mesmo lugar
    const FName Coat = Horse->GetAssetName();
    const FTransform Pose = Horse->GetActorTransform();
    Horse->Destroy();
    if (AVortexMonster* Old = OwnHorse.Get()) Old->Destroy();   // um cavalo por herói
    OwnHorse = AVortexMonster::SpawnTamed(GetWorld(), Coat, Pose, this);
    Sheet->SetMountCoat(Coat);
    Sheet->PushNotice(TEXT("Cavalo domado!  [E] montar  ·  [X] assobiar"), 2);
    Sheet->AskMountName();   // 03/11: o dono dá um nome a ele
    VortexAudio::Play(this, TEXT("level_up"), GetActorLocation(), .6f);
}

void AVortexCharacter::ServerOpenChest_Implementation(FName Id)
{
    AVortexWorldFeatures* Features = AVortexWorldFeatures::Find(GetWorld());
    const AVortexWorldFeatures::FChest* Chest = Features ? Features->NearestChest(GetActorLocation(), 340.f, QuestLog) : nullptr;
    if (!Chest || Chest->Id != Id || !Sheet || !QuestLog)
    {
        UE_LOG(LogTemp, Display, TEXT("VORTEX_CHEST recusado %s: perto=%s"), *Id.ToString(), Chest ? *Chest->Id.ToString() : TEXT("nenhum"));
        return;
    }
    // baú de acampamento: o bando em volta ainda guarda
    if (Chest->bGuarded)
    {
        if (const int32 Guards = AVortexWorldFeatures::GuardsAlive(GetWorld(), Chest->Location))
        {
            Sheet->PushNotice(FString::Printf(TEXT("O baú ainda está guardado! (%d de pé)"), Guards), 1);
            UE_LOG(LogTemp, Display, TEXT("VORTEX_CHEST guardado %s: %d de pé"), *Id.ToString(), Guards);
            return;
        }
    }
    QuestLog->SetFlag(Id);
    // 07/11: o baú volta a dar equipamento (agora os números do nível do item batem), que pula para o chão rodando: o
    // raro, do Raro ao Lendário; o do acampamento, do Incomum ao Épico; o abandonado, às vezes um Normal ou Incomum
    {
        const float Roll = FMath::FRand();
        const TCHAR* Rarity = Chest->bRare ? (Roll < .6f ? TEXT("rare") : Roll < .9f ? TEXT("epic") : TEXT("legendary"))
            : Chest->bGuarded ? (Roll < .5f ? TEXT("uncommon") : Roll < .9f ? TEXT("rare") : TEXT("epic"))
            : (Roll < .25f ? TEXT("common") : Roll < .4f ? TEXT("uncommon") : nullptr);
        TArray<FName> Pool;
        if (Rarity)
            for (const TPair<FName, FVortexItemDef>& It : FVortexRpgCatalog::Get().Items)
                if (It.Value.IsEquipment() && It.Value.Rarity == Rarity && It.Value.Price > 0) Pool.Add(It.Key);
        if (Pool.Num())
            AVortexItemDrop::Spawn(GetWorld(), FVortexRpgCatalog::LevelledId(Pool[FMath::RandHelper(Pool.Num())], FMath::Max(1, Sheet->GetLevel() + FMath::RandRange(-1, 2))), 1, Chest->Location + FVector(0, 0, 60.f));
        else if (!Chest->bRare && !Chest->bGuarded && FMath::FRand() < .5f)
            AVortexItemDrop::Spawn(GetWorld(), TEXT("potion_red"), 1, Chest->Location + FVector(0, 0, 60.f));
    }
    // Mais moedas que antes, para compensar o saque que saiu (o raro e o do acampamento continuam valendo mais).
    Sheet->AddCoins((Chest->bRare ? 140 : Chest->bGuarded ? 100 : 45) + FMath::RandRange(0, Chest->bRare ? 120 : 50) + Sheet->GetLevel() * (Chest->bGuarded ? 8 : 5));
    Sheet->PushNotice(Chest->bGuarded ? TEXT("Saque conquistado!") : Chest->bRare ? TEXT("Baú raro aberto!") : TEXT("Baú abandonado aberto!"), 2);
    ClientChestOpened(Id);
}

void AVortexCharacter::ServerPray_Implementation(FName Id)
{
    const AVortexPointsOfInterest* Pois = AVortexPointsOfInterest::Find(GetWorld());
    const FVortexPoi* Shrine = Pois ? Pois->NearestShrine(GetActorLocation(), 450.f) : nullptr;
    if (!Shrine || Shrine->Id != Id || !Sheet) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (const double* Last = PrayedAt.Find(Id); Last && Now - *Last < 600.0)
    {
        Sheet->PushNotice(FString::Printf(TEXT("O santuário ainda está se recarregando (%d min)"), FMath::CeilToInt((600.0 - (Now - *Last)) / 60.0)), 1);
        return;
    }
    PrayedAt.Add(Id, Now);
    Sheet->ApplyBlessing(AVortexPointsOfInterest::BlessingKind(Shrine->Variant), 300.f);
    Sheet->PushNotice(AVortexPointsOfInterest::BlessingName(Shrine->Variant) + TEXT("! (5 minutos)"), 2);
    ClientBlessed(Shrine->Variant);
}

void AVortexCharacter::ServerSlashWeb_Implementation()
{
    if (AVortexPointsOfInterest* Pois = AVortexPointsOfInterest::Find(GetWorld())) Pois->SlashWeb(GetActorLocation(), GetControlRotation().Vector());
}

void AVortexCharacter::ServerEnterPortal_Implementation(AVortexPortal* Portal)
{
    if (!Portal || !Sheet || FVector::Dist2D(Portal->GetActorLocation(), GetActorLocation()) > 450.f * AVortexPortal::RankSize(Portal->Rank)) return;
    if (Portal->Destination.IsNearlyZero())
    {
        // as dungeons dos portais ainda estão em construção (AVortexDungeons fica guardado para depois)
        Sheet->PushNotice(Portal->IsNoEscape() ? FString(TEXT("Portal VERMELHO (ranque S): em construção! A dungeon ainda não está aberta."))
            : FString::Printf(TEXT("Portal de ranque %s: em construção! A dungeon ainda não está aberta."), *Portal->Rank), 1);
        ClientPortalEntered(false);
        return;
    }
    // portal de volta da dungeon
    TeleportTo(Portal->Destination, GetActorRotation());
    AVortexDungeons::Leave(this);
    Sheet->PushNotice(TEXT("Você voltou pelo portal."), 2);
    ClientPortalEntered(true);
}

void AVortexCharacter::ClientPortalEntered_Implementation(bool bTravelled)
{
    VortexAudio::Play(this, TEXT("portal_enter"), GetActorLocation(), bTravelled ? 1.f : .6f);
}

void AVortexCharacter::ClientBlessed_Implementation(uint8 Variant)
{
    const FLinearColor Color = Variant == 0 ? FLinearColor(1.f, .3f, .2f) : Variant == 1 ? FLinearColor(.3f, 1.f, .45f) : FLinearColor(1.f, .85f, .3f);
    VortexHitFx::Splash(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 60.f), Color, 26);
    VortexAudio::Play(this, TEXT("level_up"), GetActorLocation(), .7f);
}

void AVortexCharacter::ClientChestOpened_Implementation(FName Id)
{
    AVortexWorldFeatures* Features = AVortexWorldFeatures::Find(GetWorld());
    if (!Features) return;
    Features->PlayOpen(Id);
    // brilho dourado em cima do baú (o mais perto, que é o que abriu)
    if (const AVortexWorldFeatures::FChest* Chest = Features->NearestChest(GetActorLocation(), 1e9f, nullptr))
        VortexHitFx::Splash(GetWorld(), Chest->Location + FVector(0.f, 0.f, 40.f), FLinearColor(1.f, .85f, .4f), Chest->bRare ? 30 : 16);
    VortexAudio::Play(this, TEXT("coin"), GetActorLocation(), .8f);
}

void AVortexCharacter::ServerDismount_Implementation() { Dismount(); }

void AVortexCharacter::Dismount()
{
    if (!HasAuthority() || !Mount) return;
    AVortexMonster* Horse = Mount;
    Mount = nullptr;
    Horse->SetRider(nullptr);
    // o cavalo fica ao lado, o herói desce para o outro
    Horse->SetActorLocation(GetActorLocation() + GetActorRightVector() * 120.f, false, nullptr, ETeleportType::TeleportPhysics);
    SetActorLocation(GetActorLocation() - GetActorRightVector() * 60.f + FVector(0.f, 0.f, 10.f), true);
    SetMountLift(0.f);
    ForceNetUpdate();
}

void AVortexCharacter::Whistle()
{
    if (!IsLocallyControlled() || VortexHeroWindow::IsOpen() || VortexDialogue::IsOpen() || Mount) return;
    // 02/11: o assobio de chamar o cavalo (Tools/MakeWhistle.py; antes só o cavalo tocava um som de vento)
    VortexAudio::Play(this, TEXT("horse_whistle"), GetActorLocation() + FVector(0.f, 0.f, 60.f), 1.f, .03f);
    ServerWhistle();
}

void AVortexCharacter::ServerWhistle_Implementation()
{
    if (!Sheet || Mount) return;
    if (AVortexMonster* Horse = OwnHorse.Get(); Horse && !Horse->IsCreatureDead()) { Horse->Whistled(); return; }
    const FName Coat = Sheet->GetMountCoat();
    if (Coat.IsNone()) { Sheet->PushNotice(TEXT("Você ainda não tem cavalo (dome um com frutas silvestres)"), 4); return; }
    // o cavalo salvo surge longe, atrás da câmera, e chega galopando (AVortexMonster::Whistled o põe no lugar)
    FVector At = GetActorLocation() - FRotator(0.f, GetControlRotation().Yaw, 0.f).Vector() * 3500.f;
    if (const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData()) At.Z = Data->GroundAt(At.X, At.Y) + 120.f;
    OwnHorse = AVortexMonster::SpawnTamed(GetWorld(), Coat, FTransform(FRotator(0.f, (GetActorLocation() - At).Rotation().Yaw, 0.f), At), this);
    if (OwnHorse.IsValid()) OwnHorse->Whistled();
}

// ---- morte ------------------------------------------------------------------------------------------

bool AVortexCharacter::IsDead() const
{
    return AbilitySystem && AbilitySystem->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute()) > 0.f
        && AbilitySystem->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) <= 0.f;
}

void AVortexCharacter::TickDeath(float DeltaSeconds)
{
    const bool bDead = IsDead();
    DeadTime = bDead ? DeadTime + DeltaSeconds : 0.f;
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (bDead && !bDeadPosed)
    {
        // caído: não anda, não pula, não luta (a animação de morte segura o último quadro)
        bDeadPosed = true;
        Movement->StopMovementImmediately();
        Movement->DisableMovement();
    }
    else if (!bDead && bDeadPosed)
    {
        bDeadPosed = false;
        Movement->SetMovementMode(MOVE_Walking);
    }
    if (!IsLocallyControlled() || !FollowCamera) return;
    // a câmera perde a cor e escurece devagar enquanto ele está caído
    const float A = FMath::Clamp(DeadTime / 1.5f, 0.f, 1.f);
    FPostProcessSettings& Post = FollowCamera->PostProcessSettings;
    Post.bOverride_ColorSaturation = A > 0.f;
    Post.ColorSaturation = FVector4(1.f - .85f * A, 1.f - .85f * A, 1.f - .85f * A, 1.f);
    Post.bOverride_VignetteIntensity = A > 0.f;
    Post.VignetteIntensity = .4f + .8f * A;
    FollowCamera->PostProcessBlendWeight = 1.f;
}

void AVortexCharacter::RespawnNow()
{
    if (IsDead() && DeadTime > 2.f && Sheet) Sheet->ServerRespawnNow();
}

void AVortexCharacter::ToggleHeroWindow()
{
    if (!IsLocallyControlled()) return;
    if (APlayerController* Player = Cast<APlayerController>(GetController()))
    {
        // I/Tab troca para a ficha em vez de empilhá-la sobre o mapa ou a pausa.
        if (AVortexHudBase* Hud = Cast<AVortexHudBase>(Player->GetHUD()); Hud && Hud->IsWorldMapOpen()) Hud->ToggleWorldMap();
        if (UVortexGameInstance* Instance = Cast<UVortexGameInstance>(GetGameInstance()); Instance && Instance->IsPauseMenuOpen())
            Instance->TogglePauseMenu();
    }
    VortexHeroWindow::Toggle(this);
}

// ---- emotes e danças -------------------------------------------------------------------------------

const TArray<AVortexCharacter::FEmote>& AVortexCharacter::EmotePage(int32 Page)
{
    // os nomes são os dos clipes (Tools/Blender/MixamoToHero.py: MX_<raça>_<clipe>)
    static const TArray<FEmote> Dances = {
        { TEXT("Danca_HipHop"), TEXT("Hip-hop") }, { TEXT("Danca_Samba"), TEXT("Samba") }, { TEXT("Danca_Boba"), TEXT("Dança boba") },
        { TEXT("Danca_Robo"), TEXT("Robô") }, { TEXT("Danca_Thriller"), TEXT("Thriller") }, { TEXT("Danca_Macarena"), TEXT("Macarena") },
        { TEXT("Danca_Galinha"), TEXT("Galinha") }, { TEXT("Danca_Twist"), TEXT("Twist") }, { TEXT("Danca_Swing"), TEXT("Swing") },
        { TEXT("Danca_Salsa"), TEXT("Salsa") }, { TEXT("Danca_Breakdance"), TEXT("Breakdance") }, { TEXT("Danca_Gangnam"), TEXT("Gangnam") } };
    static const TArray<FEmote> Gestures = {
        { TEXT("Emote_Acenar"), TEXT("Acenar") }, { TEXT("Emote_Vitoria"), TEXT("Vitória") }, { TEXT("Emote_Comemorar"), TEXT("Comemorar") },
        { TEXT("Emote_Palmas"), TEXT("Palmas") }, { TEXT("Emote_Reverencia"), TEXT("Reverência") }, { TEXT("Emote_Rir"), TEXT("Rir") },
        { TEXT("Emote_Pensar"), TEXT("Pensar") }, { TEXT("Emote_Provocar"), TEXT("Provocar") }, { TEXT("Emote_Saudar"), TEXT("Saudar") },
        { TEXT("Emote_Apontar"), TEXT("Apontar") }, { TEXT("GuitarPlaying"), TEXT("Guitarra") }, { TEXT("SentadoBanco"), TEXT("Sentar") } };
    return Page == 0 ? Dances : Gestures;
}

bool AVortexCharacter::HasEmote(FName Clip) const
{
    UVortexAnimInstance* Anim = GetMesh() ? Cast<UVortexAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;
    return Anim && Anim->HasClip(Clip);
}

int32 AVortexCharacter::GetEmoteChoice() const
{
    if (EmoteAim.Size() < 3.f) return -1;
    const int32 Count = EmotePage(EmotePageIndex).Num();
    // fatia 0 em cima, no sentido do relógio
    const float Degrees = FMath::RadiansToDegrees(FMath::Atan2(EmoteAim.X, EmoteAim.Y));
    return FMath::RoundToInt(FMath::Fmod(Degrees + 360.f, 360.f) / (360.f / Count)) % Count;
}

void AVortexCharacter::OpenEmoteWheel()
{
    if (IsMouseOnWindow() || IsDead()) return;
    bEmoteWheel = true;
    EmoteAim = FVector2D::ZeroVector;
}

void AVortexCharacter::CloseEmoteWheel()
{
    if (!bEmoteWheel) return;
    bEmoteWheel = false;
    const int32 Choice = GetEmoteChoice();
    if (Choice < 0) return;
    const FName Clip = EmotePage(EmotePageIndex)[Choice].Clip;
    if (HasEmote(Clip)) PlayEmote(Clip);
}

void AVortexCharacter::PlayEmote(FName Clip)
{
    if (!HasAuthority()) Emote = Clip;   // o dono já começa (o servidor confirma)
    ServerEmote(Clip);
}

void AVortexCharacter::ServerEmote_Implementation(FName Clip)
{
    if (!Clip.IsNone() && !HasEmote(Clip)) return;
    Emote = Clip;
    UVortexAnimInstance* Anim = GetMesh() ? Cast<UVortexAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;
    // gesto: acaba no fim do clipe; dança: até mexer
    EmoteEnds = Clip.IsNone() || Clip.ToString().StartsWith(TEXT("Danca")) || !Anim ? 0.0 : GetWorld()->GetTimeSeconds() + Anim->ClipLength(Clip);
    ForceNetUpdate();
}

void AVortexCharacter::TickFace(float DeltaSeconds)
{
    // os quadros (Tools/MakeHeroFaces.py): olhos 0 aberto, 1 meio, 2 fechado, 3/4 olhando para um lado e o outro,
    // 5 bravo, 6 feliz, 7 dor; boca 0 neutra, 1 A, 2 O, 3 E, 4 sorriso, 5 dentes, 6 M, 7 grito
    if (!FaceMaterial || GetNetMode() == NM_DedicatedServer) return;
    FaceClock += DeltaSeconds;
    const float Health = Vitals ? Vitals->GetHealth() : 0.f;
    if (FaceLastHealth >= 0.f && Health < FaceLastHealth - .5f && Health > 0.f) FaceHitAge = 0.f;
    FaceLastHealth = Health;
    FaceHitAge += DeltaSeconds;
    const FName EmoteNow = GetEmote();
    const FString EmoteName = EmoteNow.ToString();
    bool bAttacking = false, bSpecial = false;
    if (const UVortexWeaponComponent* Held = GetWeapon())
    {
        const FVortexAttackState& Attack = Held->GetAttackState();
        const float Now = GetWorld()->GetGameState() ? GetWorld()->GetGameState()->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
        bSpecial = Attack.SwingIndex == VortexSpecialPose::Hold || Attack.SwingIndex == VortexSpecialPose::Release;
        bAttacking = bSpecial || (Attack.SwingIndex >= 0 && Now - Attack.ServerStartTime < .9f) || Held->IsGuarding();
    }
    int32 Eye = 0, Mouth = 0;
    bool bCanBlink = true;
    if (IsDead()) { Eye = 2; Mouth = 2; bCanBlink = false; }
    else if (FaceHitAge < .55f) { Eye = 7; Mouth = 7; bCanBlink = false; }
    else if (bAttacking) { Eye = 5; Mouth = bSpecial ? 7 : 5; }
    else if (EmoteName.StartsWith(TEXT("Danca")) || EmoteName == TEXT("Emote_Comemorar") || EmoteName == TEXT("Emote_Vitoria"))
    {
        Eye = FMath::Fmod(FaceClock, 3.f) < 1.6f ? 6 : 0;
        Mouth = 4;
    }
    else if (EmoteName == TEXT("Emote_Rir")) { Eye = 6; Mouth = FMath::Fmod(FaceClock, .3f) < .15f ? 1 : 3; bCanBlink = false; }
    else if (EmoteName == TEXT("Grito")) { Eye = 5; Mouth = 7; }
    else if (EmoteName == TEXT("Emote_Provocar")) { Eye = 5; Mouth = 4; }
    else if (!EmoteNow.IsNone()) { Mouth = 4; }
    else if (IsLocallyControlled() && Controller)
    {
        // olha para onde a câmera vira (o quadro 3 leva a íris para o lado direito do herói)
        const float Turn = FRotator::NormalizeAxis(Controller->GetControlRotation().Yaw - GetActorRotation().Yaw);
        if (Turn > 30.f && Turn < 150.f) Eye = 3;
        else if (Turn < -30.f && Turn > -150.f) Eye = 4;
    }
    // piscar sozinho a cada 2,5-5,5 s: meio, fechado, meio (0,17 s)
    FaceBlinkIn -= DeltaSeconds;
    if (FaceBlinkIn <= 0.f) { FaceBlinkAge = 0.f; FaceBlinkIn = FMath::FRandRange(2.5f, 5.5f); }
    FaceBlinkAge += DeltaSeconds;
    if (bCanBlink && (Eye == 0 || Eye == 3 || Eye == 4 || Eye == 5) && FaceBlinkAge < .17f) Eye = FaceBlinkAge < .05f || FaceBlinkAge > .12f ? 1 : 2;
#if !UE_BUILD_SHIPPING
    if (DebugFace >= 0) Eye = Mouth = DebugFace;
#endif
    if (Eye != FaceEye) { FaceEye = Eye; FaceMaterial->SetScalarParameterValue(TEXT("EyeFrame"), Eye); }
    if (Mouth != FaceMouth) { FaceMouth = Mouth; FaceMaterial->SetScalarParameterValue(TEXT("MouthFrame"), Mouth); }
}

void AVortexCharacter::TickEmote()
{
    if (Emote.IsNone()) return;
    const UVortexCharacterMovementComponent* Move = GetVortexMovement();
    const bool bBusy = !Move || IsDead() || IsMounted() || Move->IsFalling() || Move->IsSwimmingInWater() || Move->IsDodging()
        || FVector(Move->Velocity.X, Move->Velocity.Y, 0.f).SizeSquared() > 60.f * 60.f
        || (Weapon && Weapon->GetDefinition() && Weapon->GetDefinition()->Swings.IsValidIndex(Weapon->GetAttackState().SwingIndex));
    if (bBusy || (EmoteEnds > 0.0 && GetWorld()->GetTimeSeconds() > EmoteEnds))
    {
        Emote = NAME_None;
        ForceNetUpdate();
    }
}
