#include "Creatures/VortexMonster.h"
#include "Rpg/VortexRpg.h"

#include "Framework/VortexProbe.h"
#include "AbilitySystemComponent.h"
#include "Animation/VortexMonsterAnimInstance.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexWeaponFx.h"
#include "Combat/VortexMonsterFx.h"
#include "Combat/VortexLobbed.h"
#include "Combat/VortexTelegraph.h"
#include "Components/CapsuleComponent.h"
#include "Creatures/VortexMonsterSpecies.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexTown.h"
#include "World/VortexCaldera.h"
#include "World/VortexTerrainData.h"
#include "Rpg/VortexHeroSheet.h"
#include "Creatures/VortexBlockBody.h"
#include "Creatures/VortexDress.h"
#include "Framework/VortexLook.h"
#include "Combat/VortexProjectile.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Creatures/VortexSlimeLook.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Creatures/VortexMist.h"
#include "Creatures/VortexVolcanicBoss.h"
#include "Creatures/VortexGroupCoordinator.h"
#include "Creatures/VortexPathGrid.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    float Smooth(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); }

    /** 10/11: os ajustes da IA (Data/Creatures/ia_monstros.json), lidos uma vez. */
    float Ai(const TCHAR* Key, float Default)
    {
        static TMap<FString, float> Values;
        static bool bLoaded = false;
        if (!bLoaded)
        {
            bLoaded = true;
            FString Text;
            TSharedPtr<FJsonObject> Json;
            if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Creatures/ia_monstros.json")))
                && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) && Json.IsValid())
                for (const auto& Pair : Json->Values)
                {
                    double Number = 0.0;
                    if (Pair.Value.IsValid() && Pair.Value->TryGetNumber(Number)) Values.Add(FString(Pair.Key), float(Number));
                }
        }
        const float* V = Values.Find(Key);
        return V ? *V : Default;
    }

    /** O grupo do bando no coordenador (separado dos números das matilhas de lobo). */
    int32 PackGroup(int32 PackId) { return PackId >= 0 ? 1000000 + PackId : -1; }

    // 10/11 (IA, fase 3): o caminho em volta dos obstáculos de cada monstro (VortexPathGrid), guardado aqui para não
    // mexer no cabeçalho (muito incluído)
    struct FPathState
    {
        TArray<FVector> Points;
        int32 Next = 0;
        FVector Goal = FVector::ZeroVector;
        double ComputedAt = -10.0, CheckedAt = -10.0;
        bool bBlocked = false;
    };
    TMap<TWeakObjectPtr<const AActor>, FPathState>& Paths()
    {
        static TMap<TWeakObjectPtr<const AActor>, FPathState> Map;
        return Map;
    }
    // 10/11: o rosto do Cogumelo Corrompido (M_CogumeloCorrompido): o material de cada um e o relógio da piscada
    struct FFaceState { TWeakObjectPtr<UMaterialInstanceDynamic> Mat; float NextBlink = 2.f, Blink = 0.f, Glow = 1.5f; };
    TMap<TWeakObjectPtr<const AActor>, FFaceState>& Faces()
    {
        static TMap<TWeakObjectPtr<const AActor>, FFaceState> Map;
        return Map;
    }
    // no máximo 3 buscas por quadro (todas as criaturas juntas)
    uint64 PathFrame = 0;
    int32 PathBudget = 0;
    int32 PathCount = 0, PathFound = 0;
    double PathSeconds = 0.0;
}

AVortexMonster::AVortexMonster()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(20.f);
    NetCullDistanceSquared = FMath::Square(12000.f);
    SetActorHiddenInGame(true);
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->bRunPhysicsWithNoController = true;
    Movement->bOrientRotationToMovement = true;
    Movement->RotationRate = FRotator(0.f, 360.f, 0.f);
    Movement->BrakingDecelerationWalking = 1600.f;
    Movement->GroundFriction = 6.f;
    Movement->MaxStepHeight = 45.f;
    Movement->SetWalkableFloorAngle(48.f);
    bUseControllerRotationYaw = false;
    AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    AbilitySystem->SetIsReplicated(true);
    AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
    Vitals = CreateDefaultSubobject<UVortexAttributes>(TEXT("Vitals"));
    Combat = CreateDefaultSubobject<UVortexCombatComponent>(TEXT("Combat"));
    Combat->Faction = EVortexFaction::Hostile;
    Dress = CreateDefaultSubobject<UVortexDressComponent>(TEXT("Dress"));
    GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    GetMesh()->SetAnimInstanceClass(UVortexMonsterAnimInstance::StaticClass());
    GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    // de longe a animação atualiza menos vezes por segundo (a otimização de taxa de atualização da Unreal)
    GetMesh()->bEnableUpdateRateOptimizations = true;
}

const FVortexSpeciesDef* AVortexMonster::Def() const { return FVortexSpeciesDef::Find(Species); }
FName AVortexMonster::GetModelName() const
{
    const FVortexSpeciesDef* D = Def();
    return D && !D->Model.IsNone() ? D->Model : Species;
}

FName AVortexMonster::GetSpecies() const
{
    if (IsBandit()) return TEXT("Bandit");
    if (Species == TEXT("GoblinBrute") || Species == TEXT("GoblinScout")) return TEXT("Goblin");   // XP, saque e missões
    if (IsSlime()) return TEXT("Slime");   // as gotas de brasa, peçonhenta e de ferro contam como gota (XP, saque, missões)
    return Species.ToString().StartsWith(TEXT("Horse_")) ? FName(TEXT("Horse")) : Species;
}
bool AVortexMonster::IsPassive() const { const FVortexSpeciesDef* D = Def(); return D && D->bPassive; }
bool AVortexMonster::IsBoss() const { const FVortexSpeciesDef* D = Def(); return D && D->bBoss; }

FString AVortexMonster::GetCreatureTitle() const
{
    const FVortexSpeciesDef* D = Def();
    FString Title = D ? D->Title : Species.ToString();
    // 03/11: o nome que o dono deu (ao domar)
    if (Tamer && Tamer->GetSheet() && !Tamer->GetSheet()->GetMountName().IsEmpty()) return Tamer->GetSheet()->GetMountName().ToUpper();
    if (Tamer) return Tamer->IsLocallyControlled() ? TEXT("SEU CAVALO") : TEXT("CAVALO DOMADO");
    if (PackSize > 1 && PackSlot == 0 && GetSpecies() == TEXT("Goblin")) Title = TEXT("GOBLIN CHEFE");
    if (PackSize > 1 && PackSlot == 0 && Species == TEXT("Bandit")) Title = TEXT("CAPITÃO BANDIDO");
    if (bElite) Title += TEXT(" ANCESTRAL");
    return Title;
}

float AVortexMonster::GetHealthFraction() const
{
    const float Max = AbilitySystem->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute());
    return Max > 0.f ? FMath::Clamp(AbilitySystem->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) / Max, 0.f, 1.f) : 0.f;
}

void AVortexMonster::Setup(FName InSpecies, int32 InLevel, int32 InPackId, int32 InPackSlot, int32 InPackSize)
{
    Species = InSpecies;
    Level = FMath::Clamp(InLevel, 1, 100);
    PackId = InPackId;
    PackSlot = InPackSlot;
    PackSize = FMath::Max(1, InPackSize);
    ApplySpecies();
}

void AVortexMonster::OnRep_Species() { ApplySpecies(); }

void AVortexMonster::ApplySpecies()
{
    const FVortexSpeciesDef* D = Def();
    if (!D) return;
    // a cápsula da espécie (chefes grandes ficam com a cápsula larga e alta)
    const float Grow = bElite ? EliteScale : 1.f;
    const float Radius = D->Radius * Grow;
    const float Half = FMath::Max(Radius, D->Height * .5f * Grow);
    GetCapsuleComponent()->SetCapsuleSize(Radius, Half);
    // 02/11: o Azhûr ganha o cérebro de golem vulcânico (em todas as máquinas, com o mesmo nome: os efeitos dele vão
    // por multicast do componente)
    if (Species == TEXT("RuneColossus") && !Volcanic)
    {
        Volcanic = NewObject<UVortexVolcanicBoss>(this, TEXT("VolcanicBoss"));
        Volcanic->SetNetAddressable();
        Volcanic->SetIsReplicated(true);
        Volcanic->RegisterComponent();
    }
    Combat->bStunImmune = D->bBoss || bElite;
    Combat->bMagicAttacks = D->bMagic;
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->MaxAcceleration = D->Accel;
    Movement->MaxWalkSpeed = D->Speed;
    Movement->RotationRate = FRotator(0.f, D->bBoss ? 160.f : 420.f, 0.f);
    if (D->bFlying) { Movement->SetMovementMode(MOVE_Flying); Movement->BrakingDecelerationFlying = 1200.f; }
    if (GetNetMode() == NM_DedicatedServer) return;
    {
        VORTEX_PROBE("Nascer.Nevoa");
        ApplyMist(*D, Half, Grow);
    }
    if (D->bHumanoid) { VORTEX_PROBE("Nascer.Roupa"); ApplyHumanoid(*D); return; }
    if (IsSlime())
    {
        // o slime novo: gelatina deformada pelo material (VortexSlimeLook), sem esqueleto
        GetMesh()->SetSkeletalMesh(nullptr);
        MeshBottom = 0.f;
        if (!SlimeLook && (HasActorBegunPlay() || IsActorBeginningPlay()))
        {
            SlimeLook = NewObject<UVortexSlimeLook>(this, TEXT("SlimeLook"));
            SlimeLook->RegisterComponent();
        }
        if (SlimeLook) SlimeLook->Build(this, Half, 1.4f * Grow);   // a malha tem ~68 cm; a gota fica com ~95
        return;
    }
    VORTEX_PROBE("Nascer.Malha");
    const FString Name = GetModelName().ToString();
    USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Creatures/%s/%s_Skinned/SkeletalMeshes/%s_Skinned.%s_Skinned"),
        *Name, *Name, *Name, *Name));
    if (!Body)
    {
        UE_LOG(LogTemp, Error, TEXT("VORTEX_MONSTER sem malha %s"), *Name);
        return;
    }
    GetMesh()->SetSkeletalMesh(Body);
    // o modelo do Godot olha para -Z: na Unreal fica de lado, gira 90° (igual ao lobo)
    GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    const FBoxSphereBounds Bounds = Body->GetImportedBounds();
    MeshBottom = Bounds.Origin.Z - Bounds.BoxExtent.Z;
    // goblin chefe e alfas um pouco maiores; o semi-chefe, bem maior (os pés no chão: o fundo da malha também cresce)
    const float Size = (PackSize > 1 && PackSlot == 0 && GetSpecies() == TEXT("Goblin") ? 1.15f : 1.f) * D->BodyScale * Grow;
    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -Half - MeshBottom * Size));
    GetMesh()->SetRelativeScale3D(FVector(Size));
    BaseMeshRotation = GetMesh()->GetRelativeRotation().Quaternion();
    BaseMeshLocation = GetMesh()->GetRelativeLocation();
}

namespace
{
    /**
     * 11/11 (o usuário: "crie os visuais dos bandidos: o pequeno, o gordo, os normais e os cavaleiros"): o bandido no corpo
     * de blocos do herói, com a roupa dele (Tools/Blender/MakeBlockOutfit.py: bandido, bandido_capitao, bandido_gordo,
     * bandido_pequeno); o cavaleiro (BanditShield) com a cota de malha por cima. A pele, o cabelo e o rosto pela semente
     * do posto. Shape: o gordo mais largo que alto, o batedor menor.
     */
    FVortexLook BanditBlockLook(FName Species, int32 Seed, bool bCaptain, FName& OutArmor, FVector& OutShape)
    {
        FRandomStream Rng(Seed * 7919 + 13);
        FVortexLook Look;
        Look.Race = TEXT("humano");
        Look.Appearance = Species != TEXT("BanditBrute") && Rng.FRand() < .2f ? TEXT("female") : TEXT("male");
        static const TCHAR* Skins[] = { TEXT("C98E6A"), TEXT("AA7558"), TEXT("D8A07E"), TEXT("8A5A40"), TEXT("E0B090"), TEXT("6E4630") };
        static const TCHAR* HairColors[] = { TEXT("2E2019"), TEXT("1E1A1C"), TEXT("5A3A26"), TEXT("141012"), TEXT("7A5030") };
        static const TCHAR* Hairs[] = { TEXT("raspado"), TEXT("undercut"), TEXT("liso_curto") };
        const FName Outfit = bCaptain ? TEXT("bandido_capitao") : Species == TEXT("BanditBrute") ? TEXT("bandido_gordo")
            : Species == TEXT("BanditArcher") ? TEXT("bandido_pequeno") : TEXT("bandido");
        Look.Pieces.Add(TEXT("npc_outfit"), Outfit);
        // de capuz, o cabelo curto (o comprido atravessava o capuz)
        const bool bHood = Outfit == TEXT("bandido") || Outfit == TEXT("bandido_pequeno");
        Look.Pieces.Add(TEXT("block_hair"), FName(*(FString(TEXT("block_hair_")) + (bHood ? TEXT("raspado") : Hairs[Rng.RandRange(0, 2)]))));
        Look.Pieces.Add(TEXT("block_face"), Rng.FRand() < .6f ? TEXT("determinado") : TEXT("padrao"));
        Look.Colors.Add(TEXT("skin_color"), FColor::FromHex(Skins[Rng.RandRange(0, UE_ARRAY_COUNT(Skins) - 1)]));
        Look.Colors.Add(TEXT("hair_color"), FColor::FromHex(HairColors[Rng.RandRange(0, UE_ARRAY_COUNT(HairColors) - 1)]));
        OutArmor = Species == TEXT("BanditShield") ? FName(TEXT("malha_ferro")) : NAME_None;
        OutShape = Species == TEXT("BanditBrute") ? FVector(1.38f, 1.38f, 1.08f) : Species == TEXT("BanditArcher") ? FVector(.84f)
            : Species == TEXT("BanditShield") ? FVector(1.06f) : FVector(1.f);
        return Look;
    }
}

// Bandidos: o corpo de blocos do herói (11/11; antes o aventureiro), a roupa pela semente do posto e a arma do tipo dele no
// osso da mão (como no herói); o escudo no antebraço esquerdo. Mortos-vivos e esqueletos continuam no aventureiro.
void AVortexMonster::ApplyHumanoid(const FVortexSpeciesDef& D)
{
    // a roupa e a arma são componentes: só depois de o ator entrar no mundo (o BeginPlay chama de novo)
    if (!HasActorBegunPlay() && !IsActorBeginningPlay()) return;
    const float Half = FMath::Max(D.Radius, D.Height * .5f);
    MeshBottom = 0.f;
    bool bBlock = false;
    if (IsBandit())
    {
        FName Armor;
        FVector Shape;
        const FVortexLook Look = BanditBlockLook(Species, LookSeed, PackSlot == 0 && PackSize > 1, Armor, Shape);
        bBlock = VortexBlockBody::Apply(this, Look, Armor);
        if (bBlock)
        {
            GetMesh()->SetRelativeRotation(FRotator(0.f, AVortexCharacter::MeshYawOffset, 0.f));
            GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -Half));
            GetMesh()->SetRelativeScale3D(Shape * D.BodyScale * (bElite ? EliteScale : 1.f));
        }
    }
    if (!bBlock)
    {
        USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned.Adventurer_Skinned"));
        if (!Body) return;
        GetMesh()->SetSkeletalMesh(Body);
        GetMesh()->SetRelativeRotation(FRotator(0.f, AVortexCharacter::MeshYawOffset, 0.f));
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -Half));
        GetMesh()->SetRelativeScale3D(FVector(D.BodyScale * (bElite ? EliteScale : 1.f)));
        // mortos-vivos (cemitérios): a raça morto-vivo em trapos; os esqueletos
        if (Dress) Dress->Apply(GetMesh(), Species.ToString().StartsWith(TEXT("Undead")) ? UVortexDressComponent::UndeadLook(LookSeed) :
            Species.ToString().StartsWith(TEXT("Skeleton")) ? UVortexDressComponent::SkeletonLook(LookSeed) : UVortexDressComponent::BanditLook(LookSeed, PackSlot == 0 && PackSize > 1));
    }
    const UVortexWeaponDefinition* Weapon = UVortexWeaponComponent::LoadType(D.Weapon);
    if (!Weapon) return;
    if (!WeaponVisual)
    {
        WeaponVisual = NewObject<UStaticMeshComponent>(this, TEXT("BanditWeapon"));
        WeaponVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WeaponVisual->SetupAttachment(GetMesh(), UVortexWeaponComponent::HandBoneFor(Weapon, GetMesh()));
        WeaponVisual->SetCastShadow(false);   // a sombra da arma sobre ela mesma pintava triângulos pretos
        WeaponVisual->RegisterComponent();
    }
    WeaponVisual->SetStaticMesh(Weapon->VisualMesh);
    if (Weapon->LeftMesh)
    {
        if (!ShieldVisual)
        {
            ShieldVisual = NewObject<UStaticMeshComponent>(this, TEXT("BanditShield"));
            ShieldVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            ShieldVisual->SetupAttachment(GetMesh(), TEXT("Hand_L"));
            ShieldVisual->RegisterComponent();
        }
        ShieldVisual->SetStaticMesh(Weapon->LeftMesh);
    }
}

void AVortexMonster::ApplyMist(const FVortexSpeciesDef& D, float HalfHeight, float Scale)
{
    // névoa roxa de corrupção nos monstros hostis (pedido de 30/10): baixa, saindo dos pés, do tamanho do bicho
    if (D.bPassive || !(HasActorBegunPlay() || IsActorBeginningPlay())) return;
    UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Soul.NS_Free_Spells_Aura_Soul"));
    if (!System) return;
    if (!Mist)
    {
        Mist = UNiagaraFunctionLibrary::SpawnSystemAttached(System, GetCapsuleComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset, false, true, ENCPoolMethod::None);
        if (!Mist) return;
        VortexMist::MakePurpleMist(Mist);
    }
    const float Size = FMath::Clamp(D.Radius / 45.f, .6f, 4.f) * Scale;
    Mist->SetRelativeLocation(FVector(0.f, 0.f, -HalfHeight + 5.f));
    Mist->SetRelativeScale3D(FVector(.35f * Size, .35f * Size, .25f * Size));
}

void AVortexMonster::BeginPlay()
{
    {
        VORTEX_PROBE("Nascer.SuperBeginPlay");
        Super::BeginPlay();
    }
    ApplySpecies();
    AbilitySystem->InitAbilityActorInfo(this, this);
    bool bStreamed = false;
    if (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It) { bStreamed = true; }
    // as dungeons ficam fora do mapa: o chão é delas, não do terreno
    if (bDungeon) bStreamed = false;
    bLocalTerrainReady = !bStreamed;
    if (bStreamed) GetWorldTimerManager().SetTimer(TerrainHandle, this, &AVortexMonster::TryActivateTerrain, .1f, true, .05f);
    if (HasAuthority())
    {
        const FVortexSpeciesDef* D = Def();
        if (!D) { Destroy(); return; }
        Home = GetActorLocation();
        // vida e dano pelo nível (ValleyMonster.Level.cs): ~8-16 golpes de um herói do mesmo nível
        // (os números em Data/Rpg/balance.json; o nível vem da região, VortexTerrainData::AreaLevel)
        const FVortexRpgCatalog& Rpg = FVortexRpgCatalog::Get();
        const float Hit = Rpg.MonsterHitBase + Rpg.MonsterHitPerLevel * (Level - 1);
        float MaxHealth = D->bBoss ? D->BossHealth : FMath::RoundToFloat(Hit * D->Hits * 2.7f);
        AttackDamage = FMath::RoundToFloat(D->BaseDamage * (1.f + Rpg.MonsterDamagePerLevel * (Level - 1)));
        if (PackSize > 1 && PackSlot == 0 && GetSpecies() == TEXT("Goblin")) { MaxHealth *= 1.8f; AttackDamage *= 1.2f; }
        else if (PackSize > 1 && PackSlot == 0 && IsBandit()) { MaxHealth *= 1.6f; AttackDamage *= 1.15f; }
        else if (PackSlot > 0 && !D->bPassive) { MaxHealth = FMath::Max(2.f, MaxHealth * .75f); AttackDamage = FMath::Max(4.f, AttackDamage * .65f); }
        if (bElite) { MaxHealth *= 6.f; AttackDamage *= 1.5f; }
        AttackDamage *= DungeonDamage;
        if (bDungeon && ClimbPose == 3) { GetCharacterMovement()->SetMovementMode(MOVE_Flying); ThreadTopZ = DungeonCeiling; }
        MaxHealthValue = FMath::Max(1.f, FMath::RoundToFloat(MaxHealth));
        AbilitySystem->SetNumericAttributeBase(UVortexAttributes::GetMaxHealthAttribute(), MaxHealthValue);
        AbilitySystem->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), MaxHealthValue);
        LastHealth = MaxHealthValue;
        AbilitySystem->GetGameplayAttributeValueChangeDelegate(UVortexAttributes::GetHealthAttribute()).AddUObject(this, &AVortexMonster::OnHealthChanged);
        Combat->OnStunned.AddWeakLambda(this, [this](float Seconds)
        {
            if (bDead) return;
            const double Now = GetWorld()->GetTimeSeconds();
            Acting = INDEX_NONE;
            LungeStart = -1.f;
            StunUntil = Now + Seconds;
            NextMoveTime = FMath::Max(NextMoveTime, Now + Seconds + .3);
            StopMoving();
            const FVortexSpeciesDef* Spec = Def();
            Act(Spec ? Spec->HitClip : FName(TEXT("Hit")));
        });
        Act(NAME_None);
        if (!bStreamed) bTerrainReady = true;
        WanderAt = GetWorld()->GetTimeSeconds() + FMath::FRandRange(1.f, 4.f);
    }
    RefreshPresentation();
    if (HasAuthority() && bTerrainReady) GetWorldTimerManager().SetTimer(ThinkHandle, this, &AVortexMonster::Think, .1f, true, FMath::FRandRange(.05f, .15f));
    // 10/11: o bando reveza quem ataca de perto (no máximo 2 por vez; os outros cercam esperando)
    if (HasAuthority() && PackId >= 0 && PackSize > 1)
        if (const FVortexSpeciesDef* D = Def(); D && !D->bPassive && !D->bBoss)
            if (UVortexGroupCoordinator* Group = GetWorld()->GetSubsystem<UVortexGroupCoordinator>())
            {
                Group->RegisterMember(PackGroup(PackId), this, EVortexGroupRole::Anchor);
                Group->SetMaxAttackers(PackGroup(PackId), int32(Ai(TEXT("atacantesMax"), 2.f)));
                bInPackGroup = true;
            }
}

void AVortexMonster::ReleaseToken()
{
    if (!bAttackToken) return;
    bAttackToken = false;
    if (UVortexGroupCoordinator* Group = GetWorld() ? GetWorld()->GetSubsystem<UVortexGroupCoordinator>() : nullptr)
        Group->ReleaseAttack(PackGroup(PackId), this, GetWorld()->GetTimeSeconds());
}

bool AVortexMonster::CanSee(const AVortexCharacter* Hero) const
{
    if (!Hero) return false;
    const FVortexSpeciesDef* D = Def();
    const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, D ? D->Height * .3f : 50.f);
    const FVector At = Hero->GetActorLocation() + FVector(0.f, 0.f, 40.f);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexMonsterSight), false, this);
    Query.AddIgnoredActor(Hero);
    FHitResult Hit;
    // só o que é parado (terreno, árvores, casas, pedras): os outros bichos não tapam a vista
    return !GetWorld()->LineTraceSingleByChannel(Hit, Eye, At, ECC_WorldStatic, Query);
}

void AVortexMonster::EndPlay(const EEndPlayReason::Type Reason)
{
    Paths().Remove(this);
    Faces().Remove(this);
    if (bInPackGroup)
        if (UVortexGroupCoordinator* Group = GetWorld() ? GetWorld()->GetSubsystem<UVortexGroupCoordinator>() : nullptr)
            Group->UnregisterMember(PackGroup(PackId), this);
    GetWorldTimerManager().ClearTimer(ThinkHandle);
    GetWorldTimerManager().ClearTimer(TerrainHandle);
    Super::EndPlay(Reason);
}

// ---- chão do streaming: só aparece e anda quando o terreno de colisão do posto existe ------------

void AVortexMonster::TryActivateTerrain()
{
    AVortexTerrainStreamer* Streamer = nullptr;
    if (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It) { Streamer = *It; }
    if (!Streamer) return;
    float GroundZ = 0.f;
    const FVector Here = GetActorLocation();
    if (!Streamer->GroundReadyAt(Here.X, Here.Y, GroundZ)) return;
    const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVortexSpeciesDef* D = Def();
    const float Lift = D && D->bFlying ? 90.f : 2.f;
    if (!HasAuthority())
    {
        if (!bTerrainReady) return;
        bLocalTerrainReady = true;
        RefreshPresentation();
        GetWorldTimerManager().ClearTimer(TerrainHandle);
        return;
    }
    GetWorldTimerManager().ClearTimer(TerrainHandle);
    SetActorLocation(FVector(Here.X, Here.Y, GroundZ + Half + Lift + (ClimbPose == 3 ? 520.f : 0.f)), false, nullptr, ETeleportType::TeleportPhysics);
    if (ClimbPose == 3) { GetCharacterMovement()->SetMovementMode(MOVE_Flying); ThreadTopZ = GetActorLocation().Z + 650.f; }
    Home = GetActorLocation();
    bTerrainReady = true;
    bLocalTerrainReady = true;
    RefreshPresentation();
    GetWorldTimerManager().SetTimer(ThinkHandle, this, &AVortexMonster::Think, .1f, true, FMath::FRandRange(.05f, .15f));
    ForceNetUpdate();
}

void AVortexMonster::OnRep_TerrainReady() { RefreshPresentation(); }

void AVortexMonster::RefreshPresentation()
{
    const bool bReady = bTerrainReady && bLocalTerrainReady;
    SetActorHiddenInGame(!bReady);
    // 02/11: montado, o cavalo segue sem colisão (antes isto a religava nos outros jogos e o cavalo empurrava o próprio
    // cavaleiro para cima: "o personagem voa"); e o cavalo nunca bloqueia heróis, em nenhuma máquina (o ajuste de
    // AVortexMonster::SpawnHorse só valia no servidor)
    SetActorEnableCollision(bReady && !bDead && !Rider);
    if (IsHorse()) GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    const FVortexSpeciesDef* D = Def();
    if (!bReady) GetCharacterMovement()->DisableMovement();
    else if (GetCharacterMovement()->MovementMode == MOVE_None) GetCharacterMovement()->SetMovementMode(D && (D->bFlying || ClimbPose == 3) ? MOVE_Flying : MOVE_Walking);
}

// ---- ações ---------------------------------------------------------------------------------------

void AVortexMonster::Act(FName Clip, float Rate)
{
    ActClip = Clip;
    ActRate = Rate;
    ++ActSerial;
    ForceNetUpdate();
}

void AVortexMonster::MoveTo(const FVector& Where, float Speed)
{
    // 10/11 (IA, fase 3): com a linha reta bloqueada (casa, muro, pedra, barranco), segue um caminho em volta;
    // livre, vai direto como antes. Sem terreno (dungeon) ou voando/escalando, só o desvio de sempre
    FVector Toward = Where - GetActorLocation();
    const FVortexSpeciesDef* D = Def();
    if (D && !D->bFlying && !D->bClimber && !bDungeon && Ai(TEXT("caminho"), 1.f) > 0.f && Toward.Size2D() > 180.f && AVortexTerrainStreamer::SharedData())
    {
        FPathState& P = Paths().FindOrAdd(this);
        const double Now = GetWorld()->GetTimeSeconds();
        // a linha reta é conferida de vez em quando (até 15 m à frente)
        if (Now - P.CheckedAt > .4)
        {
            P.CheckedAt = Now;
            const FVector Look = GetActorLocation() + Toward.GetClampedToMaxSize2D(1500.f);
            P.bBlocked = VortexPathGrid::LineBlocked(this, Look);
            if (!P.bBlocked) P.Points.Reset();
        }
        if (P.bBlocked)
        {
            const bool bStale = P.Points.IsEmpty() || FVector::Dist2D(P.Goal, Where) > 250.f || Now - P.ComputedAt > 3.0;
            if (GFrameCounter != PathFrame) { PathFrame = GFrameCounter; PathBudget = 3; }
            if (bStale && Now - P.ComputedAt > .75 && PathBudget > 0)
            {
                --PathBudget;
                P.ComputedAt = Now;
                P.Goal = Where;
                P.Next = 0;
                const double Started = FPlatformTime::Seconds();
                const bool bOk = VortexPathGrid::Find(this, Where, P.Points);
                PathSeconds += FPlatformTime::Seconds() - Started;
                ++PathCount;
                PathFound += bOk;
                if (PathCount <= 5) UE_LOG(LogTemp, Display, TEXT("VORTEX_PATH %s até %.0f m: %s, %d pontos, %.2f ms"), *GetSpecies().ToString(), FVector::Dist2D(Where, GetActorLocation()) / 100.f, bOk ? TEXT("achou") : TEXT("não achou"), P.Points.Num(), (FPlatformTime::Seconds() - Started) * 1000.0);
                if (PathCount % 50 == 0)
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_PATH %d buscas, %d acharam, %.2f ms em média"), PathCount, PathFound, PathSeconds * 1000.0 / PathCount);
            }
            while (P.Points.IsValidIndex(P.Next) && FVector::Dist2D(P.Points[P.Next], GetActorLocation()) < 90.f) ++P.Next;
            if (P.Points.IsValidIndex(P.Next)) Toward = P.Points[P.Next] - GetActorLocation();
        }
    }
    MoveAlong(Toward, Speed);
}

void AVortexMonster::MoveAlong(const FVector& Direction, float Speed)
{
    FVector Want = Direction.GetSafeNormal2D();
    if (Want.IsNearlyZero() || Speed <= 0.f) { StopMoving(); return; }
    // desvio de árvores, pedras e casas; preso há mais de 1,2 s: passo firme de lado
    FVector Forced;
    if (VortexSteering::Unstick(this, Want, .1f, Steer, Forced)) Want = Forced;
    else Want = VortexSteering::Avoid(this, Want, FMath::Clamp(Speed * .45f, 150.f, 320.f) + GetCapsuleComponent()->GetScaledCapsuleRadius(), Steer);
    Want = (Want + VortexSteering::Separation(this, GetCapsuleComponent()->GetScaledCapsuleRadius() * 2.5f + 60.f) * .6f).GetSafeNormal2D();
    MoveDirection = Want;
    MoveSpeed = Speed;
}

void AVortexMonster::StopMoving()
{
    MoveDirection = FVector::ZeroVector;
    MoveSpeed = 0.f;
}

void AVortexMonster::FaceToward(const FVector& Where, float DeltaSeconds, float Rate)
{
    const FVector Flat = (Where - GetActorLocation()).GetSafeNormal2D();
    if (Flat.IsNearlyZero()) return;
    const FRotator Want = Flat.Rotation();
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Want.Yaw, 0.f), DeltaSeconds, Rate));
}

AVortexCharacter* AVortexMonster::FindTarget(float Range) const
{
    AVortexCharacter* Best = nullptr;
    float BestDistance = Range;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AVortexCharacter* Hero = It->Get() ? Cast<AVortexCharacter>(It->Get()->GetPawn()) : nullptr;
        if (!Hero || !Hero->GetAbilitySystemComponent() ||
            Hero->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) <= 0.f) continue;
        // dentro da cidade ninguém é atacado (VillageBounds do Godot)
        if (AVortexTown::Contains(Hero->GetActorLocation())) continue;
        const float Distance = FVector::Dist2D(Hero->GetActorLocation(), GetActorLocation());
        if (Distance < BestDistance && FMath::Abs(Hero->GetActorLocation().Z - GetActorLocation().Z) < 1500.f)
        {
            Best = Hero;
            BestDistance = Distance;
        }
    }
    return Best;
}

void AVortexMonster::Think()
{
    VORTEX_PROBE("Criaturas.Pensar");
    if (bDead) return;
    const FVortexSpeciesDef* D = Def();
    if (!D) return;
    const double Now = GetWorld()->GetTimeSeconds();
    bGrazing = false;
    // guerreiro de escudo: em guarda (frente protegida) quando não está golpeando nem atordoado
    Combat->bShieldFront = D->bShield && Acting == INDEX_NONE && Now >= StunUntil && Now >= HitUntil;
    if (Now < StunUntil) { StopMoving(); return; }
    if (D->bBoss) { ThinkBoss(Now); return; }
    if (D->bPassive) { ThinkPassive(Now); return; }
    ThinkHostile(Now);
}

void AVortexMonster::ThinkPassive(double Now)
{
    const FVortexSpeciesDef& D = *Def();
    if (Tamer || Rider) { ThinkTamed(Now); return; }
    // comendo a fruta que o herói deu: fica parado
    if (Now < TameStopUntil) { StopMoving(); return; }
    AVortexCharacter* Hero = FindTarget(FMath::Max(D.Scare * 1.8f, 2500.f));
    const float Distance = Hero ? FVector::Dist2D(Hero->GetActorLocation(), GetActorLocation()) : 1e9f;
    const bool bProvoked = Now < ProvokedUntil;
    if (Hero && (bProvoked || (D.Scare > 0.f && Distance < D.Scare)))
    {
        FleeUntil = FMath::Max(FleeUntil, Now + (GetSpecies() == TEXT("Deer") ? 3.5 : 2.5));
        Target = Hero;
    }
    bFleeing = Now < FleeUntil && Target.IsValid();
    if (bFleeing)
    {
        MoveAlong(GetActorLocation() - Target->GetActorLocation(), D.Speed * 1.1f);
        return;
    }
    // o cervo levanta a cabeça quando o herói ainda está longe
    if (Hero && !D.AlertClip.IsNone() && GetSpecies() == TEXT("Deer") && Distance < D.Scare * 1.7f)
    {
        StopMoving();
        if (ActClip != D.AlertClip) Act(D.AlertClip);
        return;
    }
    if (ActClip == D.AlertClip && !D.AlertClip.IsNone()) Act(NAME_None);
    // pasta em volta do posto: anda até um ponto, para e come
    if (Now >= WanderAt)
    {
        WanderAt = Now + FMath::FRandRange(4.f, 9.f);
        const float Angle = FMath::FRandRange(0.f, 2.f * UE_PI);
        WanderTo = Home + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * FMath::FRandRange(150.f, 700.f);
    }
    if (FVector::Dist2D(WanderTo, GetActorLocation()) > 80.f && !WanderTo.IsZero()) MoveTo(WanderTo, D.Speed * .3f);
    else
    {
        StopMoving();
        bGrazing = !D.GrazeClip.IsNone();
    }
}

// ---- cavalo domado ------------------------------------------------------------------------------

bool AVortexMonster::Feed(AVortexCharacter* By)
{
    if (!HasAuthority() || !IsHorse() || Tamer || bDead || !By) return false;
    const double Now = GetWorld()->GetTimeSeconds();
    // 2 a 4 frutas, pelo posto (o mesmo cavalo pede sempre o mesmo)
    if (TameFed == 0) TameNeed = uint8(2 + FMath::Abs(PostId >= 0 ? PostId : int32(GetUniqueID())) % 3);
    ++TameFed;
    FleeUntil = 0.0;
    ProvokedUntil = 0.0;
    Target = nullptr;
    bFleeing = false;
    TameStopUntil = Now + 2.5;
    StopMoving();
    FaceToward(By->GetActorLocation(), 1.f, 100.f);
    Act(TEXT("Eat"));
    MulticastSound(TEXT("heal"), .5f);
    ForceNetUpdate();
    return TameFed >= TameNeed;
}

AVortexMonster* AVortexMonster::SpawnTamed(UWorld* World, FName Coat, const FTransform& Pose, AVortexCharacter* Master)
{
    if (!World || !Master || !Coat.ToString().StartsWith(TEXT("Horse_"))) return nullptr;
    AVortexMonster* Horse = World->SpawnActorDeferred<AVortexMonster>(AVortexMonster::StaticClass(), Pose, nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (!Horse) return nullptr;
    Horse->Setup(Coat, Master->GetSheet() ? Master->GetSheet()->GetLevel() : 1);
    Horse->Tamer = Master;
    Horse->TameFed = Horse->TameNeed;
    Horse->Combat->Faction = EVortexFaction::Neutral;   // ninguém bate no cavalo de alguém
    // não empurra nem trava ninguém: heróis e monstros passam através dele (antes, seguindo o dono, empurrava todo mundo)
    Horse->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Horse->FinishSpawning(Pose);
    return Horse;
}

void AVortexMonster::ThinkTamed(double Now)
{
    const FVortexSpeciesDef& D = *Def();
    AVortexCharacter* Master = Tamer;
    if (!IsValid(Master)) { Destroy(); return; }   // o dono saiu do mundo
    if (Rider) { bGrazing = false; bComing = false; return; }   // montado: vai com o herói
    const float Distance = FVector::Dist2D(Master->GetActorLocation(), GetActorLocation());
    // como no Red Dead: o cavalo fica onde você deixou; longe dele (70 m) vai embora, e o assobio (X) chama de novo
    if (Distance > 7000.f) { Destroy(); return; }
    if (bComing)
    {
        // chamado: galopa até o dono e para do lado
        if (Distance > 500.f)
        {
            const FVector Beside = Master->GetActorLocation() + (GetActorLocation() - Master->GetActorLocation()).GetSafeNormal2D() * 300.f;
            MoveTo(Beside, Distance > 1100.f ? D.Speed : D.Speed * .45f);
            return;
        }
        bComing = false;
        FaceToward(Master->GetActorLocation(), 1.f, 200.f);
    }
    StopMoving();
    bGrazing = !D.GrazeClip.IsNone() && Master->GetVelocity().Size2D() < 50.f && Distance > 300.f;
}

void AVortexMonster::Whistled()
{
    if (!HasAuthority() || !Tamer || Rider) return;
    const FVector Hero = Tamer->GetActorLocation();
    if (FVector::Dist2D(Hero, GetActorLocation()) > 4000.f)
    {
        // de longe: surge a 35 m atrás da câmera do dono (fora da vista) e vem galopando
        FVector At = Hero - FRotator(0.f, Tamer->GetControlRotation().Yaw, 0.f).Vector() * 3500.f;
        if (const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData()) At.Z = Data->GroundAt(At.X, At.Y) + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10.f;
        TeleportTo(At, FRotator(0.f, (Hero - At).Rotation().Yaw, 0.f), false, true);
    }
    bComing = true;
    bGrazing = false;
    // o cavalo responde ao assobio com um bufo (antes, um som de vento)
    MulticastSound(TEXT("horse_snort"), .8f);
}

namespace
{
    /** A altura (cm, já na escala da malha) do osso Bone na pose de descanso, acima da origem da malha. */
    /** O osso Bone na pose de descanso, no espaço do ator (cm); false se a malha não tem o osso. */
    bool RefBoneInActor(const USkeletalMeshComponent* Mesh, FName Bone, FVector& Out)
    {
        const USkeletalMesh* Asset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
        if (!Asset || !Mesh->GetOwner()) return false;
        const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
        int32 Index = Ref.FindBoneIndex(Bone);
        if (Index == INDEX_NONE) return false;
        FTransform Pose = FTransform::Identity;
        for (; Index != INDEX_NONE; Index = Ref.GetParentIndex(Index)) Pose = Pose * Ref.GetRefBonePose()[Index];
        Out = Mesh->GetOwner()->GetActorTransform().InverseTransformPosition(Mesh->GetComponentTransform().TransformPosition(Pose.GetLocation()));
        return true;
    }

    float RefBoneHeight(const USkeletalMeshComponent* Mesh, FName Bone)
    {
        const USkeletalMesh* Asset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
        if (!Asset) return -1.f;
        const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
        int32 Index = Ref.FindBoneIndex(Bone);
        if (Index == INDEX_NONE) return -1.f;
        FTransform Pose = FTransform::Identity;
        for (; Index != INDEX_NONE; Index = Ref.GetParentIndex(Index)) Pose = Pose * Ref.GetRefBonePose()[Index];
        return Mesh->GetComponentTransform().TransformPosition(Pose.GetLocation()).Z - Mesh->GetComponentLocation().Z;
    }
}

float AVortexMonster::SaddleHeight() const
{
    // a sela do modelo (osso "Saddle"), medida do chão (o pé da cápsula)
    const float Feet = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float Saddle = RefBoneHeight(GetMesh(), TEXT("Saddle"));
    if (Saddle > 0.f) return GetMesh()->GetComponentLocation().Z + Saddle - Feet;
    const FVortexSpeciesDef* D = Def();
    return (D ? D->Height : 170.f) * .62f;
}

float AVortexMonster::RideLift(const AVortexCharacter* By) const
{
    // o quadril do herói (no descanso, acima dos pés) na sela; o clipe Ride dobra as pernas em volta do cavalo.
    // Antes era 62% da altura do cavalo: o herói ficava flutuando acima dele
    const float Hips = By ? RefBoneHeight(By->GetMesh(), TEXT("Hips")) : -1.f;
    return FMath::Max(0.f, SaddleHeight() - (Hips > 0.f ? Hips : 50.f) + 4.f);
}

FVector AVortexMonster::GetVelocity() const
{
    // montado, a velocidade é a do herói (o andar/galope acompanha)
    if (Rider) return Rider->GetVelocity();
    if (ClimbPose == 1 || ClimbPose == 2) return FVector(Super::GetVelocity().Size(), 0.f, 0.f);
    return Super::GetVelocity();
}

void AVortexMonster::SetRider(AVortexCharacter* By)
{
    if (!HasAuthority() || bDead) return;
    Rider = By;
    ApplyRiding();
    ForceNetUpdate();
}

void AVortexMonster::OnRep_Rider() { ApplyRiding(); }

void AVortexMonster::ApplyRiding()
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (AVortexCharacter* By = Rider)
    {
        // preso ao herói: os pés do cavalo nos pés dele, virado para onde ele vai; o herói sobe na sela
        Movement->DisableMovement();
        SetActorEnableCollision(false);
        // o movimento do cavaleiro ignora o cavalo (em todas as máquinas)
        By->GetCapsuleComponent()->IgnoreActorWhenMoving(this, true);
        AttachToActor(By, FAttachmentTransformRules::KeepWorldTransform);
        const float Feet = -By->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        // a sela embaixo do quadril do herói (02/11: com o cavalo só 12 cm para trás, o herói sentava na cabeça): a
        // distância da sela até o meio do cavalo, no comprimento, e a do quadril até o meio do herói
        FVector Saddle, Hips;
        const float SaddleX = RefBoneInActor(GetMesh(), TEXT("Saddle"), Saddle) ? Saddle.X : 12.f;
        const float HipsX = RefBoneInActor(By->GetMesh(), TEXT("Hips"), Hips) ? Hips.X : 0.f;
        SetActorRelativeLocation(FVector(HipsX - SaddleX, 0.f, Feet));
        UE_LOG(LogTemp, Display, TEXT("VORTEX_MOUNT sela x=%.0f quadril x=%.0f"), SaddleX, HipsX);
        SetActorRelativeRotation(FRotator::ZeroRotator);
        By->SetMountLift(RideLift(By));
        UE_LOG(LogTemp, Display, TEXT("VORTEX_MOUNT sela %.0f cm, herói sobe %.0f cm"), SaddleHeight(), RideLift(By));
        return;
    }
    if (GetAttachParentActor())
    {
        if (AVortexCharacter* Was = Cast<AVortexCharacter>(GetAttachParentActor()))
        {
            Was->SetMountLift(0.f);
            Was->GetCapsuleComponent()->IgnoreActorWhenMoving(this, false);
        }
        DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    }
    SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
    SetActorEnableCollision(bTerrainReady && !bDead);
    if (Movement->MovementMode == MOVE_None && bTerrainReady) Movement->SetMovementMode(MOVE_Walking);
}

void AVortexMonster::ThinkHostile(double Now)
{
    const FVortexSpeciesDef& D = *Def();
    // um golpe em andamento: espera o impacto e o fim
    if (Acting != INDEX_NONE)
    {
        if (!bResolved && Now >= ActImpact) { bResolved = true; ResolveMove(); }
        if (Now >= ActEnd)
        {
            Acting = INDEX_NONE;
            Act(NAME_None);
            ReleaseToken();
        }
        else { if (LungeStart < 0.f) StopMoving(); return; }
    }
    // 02/11: a vila é proibida: quem entrou (perseguindo alguém até a porta) larga o alvo e volta para casa
    if (AVortexTown::Contains(GetActorLocation()))
    {
        Target = nullptr;
        bEngaged = false;
        MoveTo(Home, D.Speed);
        return;
    }
    const bool bProvoked = Now < ProvokedUntil && Target.IsValid() && !AVortexTown::Contains(Target->GetActorLocation());
    AVortexCharacter* Hero = bProvoked ? Target.Get() : FindTarget(D.Sight);
    // 10/11 (IA, fase 2): um herói novo só é notado com a vista livre (atrás de casa ou pedra passa), ou bem perto (ouviu)
    if (Hero && !Target.IsValid() && !bProvoked && FVector::Dist2D(Hero->GetActorLocation(), GetActorLocation()) > Ai(TEXT("ouvir"), 500.f) && !CanSee(Hero))
        Hero = nullptr;
    // os avisos curtos somem sozinhos
    if (Now >= LabelUntil && (ActionLabel == TEXT("Desviou!") || ActionLabel == TEXT("Chamou ajuda!"))) ActionLabel.Reset();
    // longe demais de casa e ninguém provocando: volta (e se cura no caminho)
    const float FromHome = FVector::Dist2D(GetActorLocation(), Home);
    if (!Hero || (!bProvoked && FromHome > D.Leash))
    {
        // 10/11 (IA, fase 2): perdeu o herói de vista (e não foi longe demais de casa): vai até o último lugar onde o
        // viu e procura em volta por uns segundos ("?"), antes de desistir
        if (Target.IsValid() && FromHome <= D.Leash && !D.bFlying)
        {
            LastSeenAt = Target->GetActorLocation();
            SearchUntil = Now + Ai(TEXT("buscaTempo"), 6.f);
        }
        Target = nullptr;
        bEngaged = false;
        bFleeing = false;
        ReleaseToken();
        if (ActionLabel == TEXT("!")) ActionLabel.Reset();
        if (Now < SearchUntil && FromHome <= D.Leash)
        {
            ActionLabel = TEXT("?");
            if (FVector::Dist2D(LastSeenAt, GetActorLocation()) > 120.f) MoveTo(LastSeenAt, D.Speed * Ai(TEXT("buscaVelocidade"), .7f));
            else
            {
                // chegou: olha em volta (vira de um lado para o outro)
                StopMoving();
                SetActorRotation(FRotator(0.f, GetActorRotation().Yaw + FMath::Sin(Now * 1.6) * 9.f, 0.f));
            }
            return;
        }
        if (ActionLabel == TEXT("?")) ActionLabel.Reset();
        bCalledHelp = false;
        if (FromHome > D.Leash) AbilitySystem->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), MaxHealthValue);
        // 10/11 (IA, fase 1): sem ninguém por perto, passeia em volta de casa com paradas (antes ficava parado no lugar)
        const float Roam = Ai(TEXT("passeioRaio"), 550.f);
        if (FromHome > Roam + 200.f) { MoveTo(Home, D.Speed * .8f); return; }
        if (Now >= WanderAt)
        {
            WanderAt = Now + FMath::FRandRange(Ai(TEXT("passeioPausaMin"), 3.f), Ai(TEXT("passeioPausaMax"), 8.f));
            const float Angle = FMath::FRand() * UE_TWO_PI;
            WanderTo = FMath::FRand() < Ai(TEXT("passeioParado"), .35f) ? GetActorLocation()
                : Home + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * FMath::FRandRange(.3f, 1.f) * Roam;
        }
        if (!WanderTo.IsZero() && FVector::Dist2D(WanderTo, GetActorLocation()) > 80.f) MoveTo(WanderTo, D.Speed * Ai(TEXT("passeioVelocidade"), .3f));
        else StopMoving();
        return;
    }
    SearchUntil = 0.0;
    if (ActionLabel == TEXT("?")) ActionLabel.Reset();
    // 10/11 (IA, fase 1): viu o herói agora (sem ter apanhado): para, encara, "!" e o grito da espécie antes de vir
    const bool bNewTarget = !Target.IsValid();
    if (Target.Get() != Hero) AlertPack(Hero);
    Target = Hero;
    bEngaged = true;
    const FVector HeroAt = Hero->GetActorLocation();
    const float Distance = FVector::Dist2D(HeroAt, GetActorLocation());
    if (bNewTarget && !bProvoked && !D.bFlying)
    {
        AlertUntil = Now + Ai(TEXT("alertaTempo"), .8f);
        ActionLabel = TEXT("!");
        if (!D.AlertClip.IsNone()) Act(D.AlertClip);
        FName Shout = VortexAudio::CreatureVoice(GetSpecies(), TEXT("alert"));
        if (Shout.IsNone()) Shout = VortexAudio::CreatureVoice(GetSpecies(), TEXT("attack"));
        if (!Shout.IsNone()) MulticastSound(Shout, .8f);
        NextMoveTime = FMath::Max(NextMoveTime, AlertUntil);
    }
    if (Now < AlertUntil)
    {
        StopMoving();
        FaceToward(HeroAt, .1f, 8.f);
        return;
    }
    if (ActionLabel == TEXT("!"))
    {
        ActionLabel.Reset();
        if (!D.AlertClip.IsNone()) Act(NAME_None);
    }
    // 10/11 (IA, fase 2): o herói carregando o golpe forte (o especial), virado para ele e perto: chance de pular de lado
    // e para trás antes de o golpe sair (uma vez por recarga). Os pesados (golem, brutamontes) não desviam
    if (Now >= DodgeReadyAt && !D.bFlying && Distance < Ai(TEXT("desvioDistancia"), 520.f) && GetCharacterMovement()->IsMovingOnGround())
        if (const UVortexWeaponComponent* Weapon = Hero->GetWeapon(); Weapon && (Weapon->SpecialCharge() > .3f || Weapon->IsSpecialActive()))
        {
            const FVector ToMe = (GetActorLocation() - HeroAt).GetSafeNormal2D();
            if (FVector::DotProduct(Hero->GetActorForwardVector().GetSafeNormal2D(), ToMe) > .5f)
            {
                DodgeReadyAt = Now + Ai(TEXT("desvioRecarga"), 3.f);
                const float Chance = Ai(*FString::Printf(TEXT("desvio_%s"), *GetSpecies().ToString()), Ai(TEXT("desvioPadrao"), .25f));
                if (FMath::FRand() < Chance)
                {
                    const FVector Side = FVector(-ToMe.Y, ToMe.X, 0.f) * (FMath::RandBool() ? 1.f : -1.f);
                    LaunchCharacter((ToMe * .8f + Side * .6f).GetSafeNormal() * Ai(TEXT("desvioForca"), 650.f) + FVector(0.f, 0.f, 260.f), true, true);
                    StopMoving();
                    NextMoveTime = FMath::Max(NextMoveTime, Now + .5);
                    ActionLabel = TEXT("Desviou!");
                    LabelUntil = Now + 1.2;
                    return;
                }
            }
        }
    // 10/11 (IA, fase 2): goblin apanhando chama os goblins por perto (uma vez por luta)
    if (!bCalledHelp && GetSpecies().ToString().StartsWith(TEXT("Goblin")) && GetHealthFraction() <= Ai(TEXT("ajudaVida"), .5f))
    {
        bCalledHelp = true;
        FName Shout = VortexAudio::CreatureVoice(GetSpecies(), TEXT("alert"));
        if (Shout.IsNone()) Shout = VortexAudio::CreatureVoice(GetSpecies(), TEXT("attack"));
        if (!Shout.IsNone()) MulticastSound(Shout, 1.f);
        ActionLabel = TEXT("Chamou ajuda!");
        LabelUntil = Now + 2.0;
        const float Reach = Ai(TEXT("ajudaRaio"), 3000.f);
        for (TActorIterator<AVortexMonster> It(GetWorld()); It; ++It)
        {
            if (*It == this || It->bDead || !It->GetSpecies().ToString().StartsWith(TEXT("Goblin"))) continue;
            if (FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) > Reach) continue;
            It->Target = Hero;
            It->ProvokedUntil = FMath::Max(It->ProvokedUntil, Now + 10.0);
        }
    }
    // ferida e medrosa: foge um pouco (uma vez por luta)
    if (D.bFearful && !bFledLowHealth && GetHealthFraction() <= .25f)
    {
        bFledLowHealth = true;
        FleeUntil = Now + 1.5;
    }
    bFleeing = Now < FleeUntil;
    if (bFleeing || Now < RetreatUntil)
    {
        const FVector Away = (GetActorLocation() - HeroAt).GetSafeNormal2D();
        const FVector Side(-Away.Y, Away.X, 0.f);
        MoveAlong(Away + Side * ((PostId % 2) ? .7f : -.7f), D.Speed * (bFleeing ? 1.45f : 1.25f));
        return;
    }
    if (Now < HitUntil) { StopMoving(); return; }
    // escolhe o golpe: o primeiro da lista que cabe na distância e está carregado
    if (Now >= NextMoveTime)
    {
        // 10/11 (IA, fase 1): antes era sempre o primeiro da lista que cabia; agora sorteia entre os que cabem, com
        // menos chance de repetir o último. Golpe de perto no bando só com a vez (no máximo 2 atacando juntos);
        // a flecha só sai com a linha livre
        TArray<int32, TInlineAllocator<8>> Options;
        TArray<float, TInlineAllocator<8>> Weights;
        float Total = 0.f;
        bool bChecked = false, bSees = true;
        for (int32 i = 0; i < D.Moves.Num(); ++i)
        {
            const FVortexMonsterMove& M = D.Moves[i];
            const double* Ready = MoveReady.Find(i);
            if (Distance < M.MinRange || Distance > M.MaxRange || (Ready && Now < *Ready)) continue;
            if (FMath::Abs(HeroAt.Z - GetActorLocation().Z) > 260.f && M.Kind != EVortexMoveKind::Lob) continue;
            if (M.Kind == EVortexMoveKind::Shoot)
            {
                if (!bChecked) { bChecked = true; bSees = CanSee(Hero); }
                if (!bSees) continue;
            }
            const float W = i == LastMove ? Ai(TEXT("repetirPeso"), .3f) : 1.f;
            Options.Add(i);
            Weights.Add(W);
            Total += W;
        }
        while (!Options.IsEmpty())
        {
            float Roll = FMath::FRand() * Total;
            int32 Pick = Options.Num() - 1;
            for (int32 k = 0; k < Options.Num(); ++k) { Roll -= Weights[k]; if (Roll <= 0.f) { Pick = k; break; } }
            const int32 i = Options[Pick];
            const EVortexMoveKind Kind = D.Moves[i].Kind;
            const bool bClose = Kind == EVortexMoveKind::Strike || Kind == EVortexMoveKind::Lunge || Kind == EVortexMoveKind::Area;
            bool bMayAttack = true;
            if (bClose && bInPackGroup && !bAttackToken)
                if (UVortexGroupCoordinator* Group = GetWorld()->GetSubsystem<UVortexGroupCoordinator>())
                {
                    bMayAttack = Group->AcquireAttack(PackGroup(PackId), this, float(Now));
                    bAttackToken = bMayAttack;
                }
            if (bMayAttack && StartMove(i, Now)) { LastMove = i; return; }
            Total -= Weights[Pick];
            Options.RemoveAt(Pick);
            Weights.RemoveAt(Pick);
        }
    }
    // aproxima até a distância da espécie (o cogumelo fica longe para cuspir)
    const float Close = D.CloseEnough > 0.f ? D.CloseEnough : D.Radius + 95.f;
    // em bando: cada um chega por um lado (cerca o herói em vez de fazer fila)
    FVector Approach = HeroAt;
    if (PackSize > 1)
    {
        const FVector From = (GetActorLocation() - HeroAt).GetSafeNormal2D();
        const float Spread = (PackSlot - (PackSize - 1) * .5f) * 50.f;
        Approach = HeroAt + From.RotateAngleAxis(Spread, FVector::UpVector) * Close * .8f;
        // 10/11 (IA, fase 2): o herói está de olho em outro (não neste): este dá a volta e chega pelas costas dele
        const FVector HeroFront = Hero->GetActorForwardVector().GetSafeNormal2D();
        if (Ai(TEXT("flanquear"), 1.f) > 0.f && FVector::DotProduct(HeroFront, From) < .2f && D.KeepAway <= 0.f && D.CloseEnough <= 0.f)
        {
            const FVector Behind = HeroAt - HeroFront * Close * .8f;
            // passa por fora (não por dentro do herói): um ponto do lado antes das costas
            const FVector Around = HeroAt + (From + (-HeroFront)).GetSafeNormal2D() * Close * 1.1f;
            Approach = FVector::Dist2D(GetActorLocation(), Behind) > Close * 1.2f ? Around : Behind;
        }
    }
    if (D.KeepAway > 0.f && Distance < D.KeepAway && Distance > 230.f)
    {
        // o batedor recua de lado (guarda distância para atirar)
        const FVector Away = (GetActorLocation() - HeroAt).GetSafeNormal2D();
        MoveAlong(Away + FVector(-Away.Y, Away.X, 0.f) * ((PostId % 2) ? .5f : -.5f), D.Speed);
    }
    else if (Distance > Close) MoveTo(Approach, D.Speed);
    else if (GetSpecies() == TEXT("Mushroom") && Distance < 420.f) MoveAlong(GetActorLocation() - HeroAt, D.Speed * .8f);
    else if (D.bFlying && Distance < 750.f)
    {
        // o morcego ronda em círculo esperando a próxima picada
        const FVector To = (HeroAt - GetActorLocation()).GetSafeNormal2D();
        MoveAlong(FVector(-To.Y, To.X, 0.f) + To * FMath::Clamp((Distance - 500.f) / 300.f, -1.f, 1.f), D.Speed * .9f);
    }
    else
    {
        // esperando a recarga: ronda de lado na distância certa (troca de lado de vez em quando)
        const FVector To = (HeroAt - GetActorLocation()).GetSafeNormal2D();
        const float Side = FMath::Sin(Now * .45 + PostId * 1.3 + PackSlot * 2.1) > 0.f ? 1.f : -1.f;
        // (mais rápido que antes, 0,4: parecia parado esperando)
        MoveAlong(FVector(-To.Y, To.X, 0.f) * Side + To * FMath::Clamp((Distance - Close * .85f) / 150.f, -1.f, 1.f), D.Speed * .6f);
    }
}

bool AVortexMonster::StartMove(int32 Index, double Now)
{
    const FVortexSpeciesDef& D = *Def();
    const FVortexMonsterMove& M = D.Moves[Index];
    AVortexCharacter* Hero = Target.Get();
    if (!Hero) return false;
    Acting = Index;
    ActStart = Now;
    ActImpact = Now + M.Impact;
    ActEnd = Now + FMath::Max(M.Impact + .25f, M.Impact * 1.6f);
    bResolved = false;
    LungeStart = -1.f;
    if (M.OwnCooldown > 0.f) MoveReady.Add(Index, Now + M.OwnCooldown);
    NextMoveTime = Now + M.Impact + M.After + PackSlot * .2f;
    StopMoving();
    const FVector Here = GetActorLocation();
    const FVector HeroAt = Hero->GetActorLocation();
    const FVector Toward = (HeroAt - Here).GetSafeNormal2D();
    // 10/11 (o usuário: "o cogumelo ao virar gira 180 graus"): vira na hora só se falta pouco; se está de lado ou de
    // costas, o Tick vira rápido e suave até o impacto (antes era um estalo de meia-volta)
    if (FMath::Abs(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, Toward.Rotation().Yaw)) < 60.f)
        SetActorRotation(FRotator(0.f, Toward.Rotation().Yaw, 0.f));
    if (M.Kind == EVortexMoveKind::Lunge || M.Kind == EVortexMoveKind::Hop)
    {
        // salto: para onde o herói está (sem passar dele) ou, no pulo para trás, para longe
        const float Distance = FVector::Dist2D(HeroAt, Here);
        LungeFrom = Here;
        LungeTo = M.Kind == EVortexMoveKind::Hop ? Here - Toward * M.LungeDistance
            : Here + Toward * FMath::Min(M.LungeDistance, FMath::Max(0.f, Distance - D.Radius - 40.f));
        LungeStart = M.LungeFrom;
        LungeLength = FMath::Max(.1f, M.Impact - M.LungeFrom);
    }
    if (M.bTelegraph && (M.Kind == EVortexMoveKind::Area || M.Kind == EVortexMoveKind::Lunge))
    {
        // o chão avisa onde vai bater; o dano sai pelo aviso (na hora do impacto)
        const FVector At = M.Kind == EVortexMoveKind::Lunge ? LungeTo
            : M.bAtTarget ? HeroAt - FVector(0.f, 0.f, Hero->GetSimpleCollisionHalfHeight()) : Here;
        AVortexTelegraph::Spawn(this, At, M.Reach, M.Impact, FMath::RoundToFloat(AttackDamage * M.Scale), false, D.Telegraph, M.bMagic);
    }
    Act(M.Clip);
    PlayMoveSound(M);
    // o efeito da habilidade de cada espécie (01/11: antes todas mostravam só o aviso no chão)
    const FVector Feet = Here - FVector(0.f, 0.f, GetSimpleCollisionHalfHeight());
    if (M.Id == TEXT("Spores") || M.Id == TEXT("Burst")) MulticastMoveFx(TEXT("PoisonCloud"), Feet, M.Reach, M.Impact * .8f, false);
    else if (M.Id == TEXT("Roots")) MulticastMoveFx(TEXT("Dust"), HeroAt - FVector(0.f, 0.f, Hero->GetSimpleCollisionHalfHeight()), M.Reach, M.Impact, false);
    else if (M.Id == TEXT("Screech"))
    {
        MulticastMoveFx(TEXT("Tornado"), Here, M.Reach, M.Impact * .4f, true);
        // 08/11 (o usuário: "o dano é só no primeiro momento"): o vento segue ferindo enquanto gira (3 toques fracos, na largura do funil)
        const float Tick = FMath::RoundToFloat(AttackDamage * M.Scale * .3f), Wide = M.Reach * .6f;
        const EVortexDamageKind DamageKind = M.bMagic ? EVortexDamageKind::Magic : EVortexDamageKind::Melee;
        for (int32 Turn = 1; Turn <= 3; ++Turn)
        {
            FTimerHandle Handle;
            TWeakObjectPtr<AVortexMonster> Self(this);
            GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([Self, Tick, Wide, DamageKind]()
            {
                AVortexMonster* Bat = Self.Get();
                UVortexCombatComponent* Combat = Bat && !Bat->IsCreatureDead() ? Bat->FindComponentByClass<UVortexCombatComponent>() : nullptr;
                if (!Combat) return;
                for (TActorIterator<AVortexCharacter> It(Bat->GetWorld()); It; ++It)
                    if (FVector::Dist2D(It->GetActorLocation(), Bat->GetActorLocation()) <= Wide && FMath::Abs(It->GetActorLocation().Z - Bat->GetActorLocation().Z) <= 350.f && Combat->CanHarm(*It))
                        Combat->ApplyHit(*It, Tick, DamageKind);
            }), M.Impact + .4f * Turn, false);
        }
    }
    else if (M.Id == TEXT("Slam") || M.Id == TEXT("Stomp")) MulticastMoveFx(TEXT("Dust"), Feet, M.Reach * .8f, M.Impact, false);
    else if (M.Id == TEXT("Leap")) MulticastMoveFx(TEXT("Splash"), LungeTo - FVector(0.f, 0.f, GetSimpleCollisionHalfHeight()), M.Reach * .7f, M.Impact, false);
    return true;
}

void AVortexMonster::ResolveMove()
{
    const FVortexSpeciesDef& D = *Def();
    if (!D.Moves.IsValidIndex(Acting)) return;
    const FVortexMonsterMove& M = D.Moves[Acting];
    AVortexCharacter* Hero = Target.Get();
    const float Amount = FMath::Max(1.f, FMath::RoundToFloat(AttackDamage * M.Scale));
    const EVortexDamageKind Kind = M.bMagic ? EVortexDamageKind::Magic : EVortexDamageKind::Melee;
    switch (M.Kind)
    {
    case EVortexMoveKind::Strike:
        if (Hero && Combat->TryMeleeHit(Hero, Amount, M.Reach + D.Radius * .5f, M.FrontDot, 0.f, M.bMagic) && Hero->GetSheet())
        {
            // gota de brasa queima, gota peçonhenta envenena
            if (D.OnHit == TEXT("burn")) Hero->GetSheet()->ApplyBurn(3.f);
            else if (D.OnHit == TEXT("poison")) Hero->GetSheet()->ApplyPoison(5.f);
        }
        break;
    case EVortexMoveKind::Area:
    case EVortexMoveKind::Lunge:
        if (M.bTelegraph) break;   // o aviso no chão já fere
        for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
        {
            if (FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) > M.Reach + D.Radius * .5f) continue;
            if (FMath::Abs(It->GetActorLocation().Z - GetActorLocation().Z) > 260.f) continue;
            if (Combat->CanHarm(*It)) Combat->ApplyHit(*It, Amount, Kind);
        }
        break;
    case EVortexMoveKind::Lob:
        if (Hero)
        {
            const FVector From = GetActorLocation() + FVector(0.f, 0.f, D.Height * .4f) + GetActorForwardVector() * D.Radius;
            const float Distance = FVector::Dist2D(Hero->GetActorLocation(), GetActorLocation());
            const float Flight = M.Flight + Distance * .00025f;
            // 10/11 (IA, fase 1): mira para onde o herói vai estar quando cair (não onde ele está)
            FVector Land = Hero->GetActorLocation() + Hero->GetVelocity().GetSafeNormal2D() * Hero->GetVelocity().Size2D() * Flight * Ai(TEXT("miraAdiantada"), .75f);
            Land.Z = Hero->GetActorLocation().Z - Hero->GetSimpleCollisionHalfHeight();
            AVortexLobbed::Throw(this, From, Land, Flight, Amount, M.Reach, !M.bMagic);
        }
        break;
    case EVortexMoveKind::Shoot:
        if (Hero)
        {
            // flecha de verdade (cai com a gravidade, crava onde bate) saindo da mão do arco
            const FVector From = GetMesh()->DoesSocketExist(TEXT("Sword")) ? GetMesh()->GetSocketLocation(TEXT("Sword"))
                : GetActorLocation() + FVector(0.f, 0.f, D.Height * .3f) + GetActorForwardVector() * D.Radius;
            const float Speed = 2600.f;
            // 10/11 (IA, fase 1): a flecha mira adiantada (o herói correndo de lado não escapa só andando)
            const float Lead = FVector::Dist(From, Hero->GetActorLocation()) / Speed * Ai(TEXT("miraAdiantada"), .75f);
            const FVector Aim = Hero->GetActorLocation() + FVector(0.f, 0.f, 20.f) + FVector(Hero->GetVelocity().X, Hero->GetVelocity().Y, 0.f) * Lead;
            const FVector Direction = AVortexProjectile::BallisticDirection(From, Aim, Speed, (Aim - From).GetSafeNormal());
            const FTransform Where(Direction.Rotation(), From);
            if (AVortexProjectile* Shot = GetWorld()->SpawnActorDeferred<AVortexProjectile>(AVortexProjectile::StaticClass(), Where, this, this,
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
            {
                Shot->Mode = TEXT("arrow");
                Shot->Damage = Amount;
                Shot->Speed = Speed;
                Shot->Range = M.Reach;
                Shot->FinishSpawning(Where);
            }
        }
        break;
    default:
        break;
    }
    if (M.Retreat > 0.f) RetreatUntil = GetWorld()->GetTimeSeconds() + M.Retreat;
}

// ---- chefes ----------------------------------------------------------------------------------------

void AVortexMonster::ThinkBoss(double Now)
{
    if (Volcanic) { Volcanic->Think(Now); return; }
    const FVortexSpeciesDef& D = *Def();
    AVortexCharacter* Hero = nullptr;
    float Best = D.Leash;
    for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
    {
        const float FromHome = FVector::Dist2D(It->GetActorLocation(), Home);
        if (FromHome < Best && It->GetAbilitySystemComponent() &&
            It->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) > 0.f) { Hero = *It; Best = FromHome; }
    }
    if (!Hero)
    {
        // todos fugiram da arena: depois de 5 s, o chefe volta ao começo
        if (bAwake)
        {
            if (AbsentSince < 0.0) AbsentSince = Now;
            if (Now - AbsentSince > 5.0) ResetEncounter();
        }
        if (FVector::Dist2D(GetActorLocation(), Home) > 200.f && !bAwake) MoveTo(Home, D.Speed);
        else StopMoving();
        return;
    }
    AbsentSince = -1.0;
    Target = Hero;
    if (!bAwake)
    {
        bAwake = true;
        bEngaged = true;
        ActionLabel = TEXT("O guardião desperta");
        Act(D.AwakenClip.IsNone() ? FName(TEXT("Idle")) : D.AwakenClip);
        MulticastSound(TEXT("boss_awaken"), 1.f);
        RecoverUntil = Now + 1.6;
        return;
    }
    // fases: a 2/3 e a 1/3 da vida (mais marcas e golpes mais fortes)
    const float Fraction = GetHealthFraction();
    const uint8 NewPhase = Fraction > .667f ? 1 : Fraction > .334f ? 2 : 3;
    if (NewPhase > Phase)
    {
        Phase = NewPhase;
        Acting = INDEX_NONE;
        ActionLabel = Phase == 2 ? TEXT("A fúria desperta") : TEXT("ÚLTIMA RESISTÊNCIA");
        Act(D.AwakenClip.IsNone() ? FName(TEXT("Idle")) : D.AwakenClip);
        AVortexTelegraph::Spawn(this, GetActorLocation(), 600.f, 1.6f, 18.f + 5.f * Phase, false, D.Telegraph);
        MulticastSound(TEXT("boss_phase"), 1.f);
        // a Caldeira responde: a lava sobe em volta dele (mais poças e mais duradouras na última fase)
        if (Def()->Telegraph.R > .9f && Def()->Telegraph.G < .5f)
        {
            const int32 Count = Phase == 2 ? 5 : 8;
            for (int32 k = 0; k < Count; ++k)
            {
                const float A = 2.f * UE_PI * k / Count + Phase;
                FVector P = GetActorLocation() + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * (Phase == 2 ? 750.f : 1050.f);
                if (const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData()) P.Z = Terrain->GroundAt(P.X, P.Y);
                MulticastLavaPool(P, Phase == 2 ? 230.f : 280.f, Phase == 2 ? 30.f : 60.f);
            }
        }
        RecoverUntil = Now + 2.1;
        return;
    }
    if (Acting != INDEX_NONE)
    {
        if (Now < ActEnd) { if (LungeStart < 0.f) StopMoving(); return; }
        Acting = INDEX_NONE;
        RecoverUntil = Now + (Phase == 3 ? .45 : .7);
        ActionLabel = TEXT("Brecha após o ataque");
        Act(NAME_None);
        return;
    }
    if (Now < RecoverUntil) { StopMoving(); return; }
    if (Now >= NextMoveTime) { BeginPattern(Now); return; }
    const float Distance = FVector::Dist2D(Hero->GetActorLocation(), GetActorLocation());
    if (Distance > D.Radius + 340.f) MoveTo(Hero->GetActorLocation(), D.Speed * (Phase == 3 ? 1.2f : 1.f));
    else StopMoving();
}

void AVortexMonster::BeginPattern(double Now)
{
    const FVortexSpeciesDef& D = *Def();
    AVortexCharacter* Hero = Target.Get();
    if (!Hero || D.Moves.IsEmpty()) return;
    const FVector Here = GetActorLocation();
    const FVector HeroAt = Hero->GetActorLocation();
    const float Distance = FVector::Dist2D(HeroAt, Here);
    // na ordem, como o Godot; perto demais troca pelo golpe em volta, longe demais pela investida
    int32 Index = PatternIndex++ % D.Moves.Num();
    if (Distance > D.Moves[Index].MaxRange)
    {
        for (int32 i = 0; i < D.Moves.Num(); ++i) if (D.Moves[i].Rush > 0.f) { Index = i; break; }
    }
    const FVortexMonsterMove& M = D.Moves[Index];
    const FVector Forward = (HeroAt - Here).GetSafeNormal2D().IsNearlyZero() ? GetActorForwardVector() : (HeroAt - Here).GetSafeNormal2D();
    SetActorRotation(FRotator(0.f, Forward.Rotation().Yaw, 0.f));
    Acting = Index;
    ActStart = Now;
    ActEnd = Now + M.Duration;
    bResolved = true;
    NextMoveTime = Now + M.Duration + (Phase == 3 ? .35 : .7);
    ActionLabel = M.Label;
    const float Strength = Phase == 3 ? 1.4f : Phase == 2 ? 1.2f : 1.f;
    for (const FVortexMark& K : M.Marks)
    {
        if (K.MinPhase > Phase) continue;
        FVector At = K.Anchor == 2 ? HeroAt : Here;
        if (K.Anchor == 1) At = Here + Forward * K.Ahead;
        else if (K.Anchor == 2 && K.Ahead > 0.f) At = HeroAt + Forward.RotateAngleAxis(K.SideAngle, FVector::UpVector) * K.Ahead;
        AVortexTelegraph::Spawn(this, At, K.Radius, K.Delay, FMath::RoundToFloat(AttackDamage * K.Scale * Strength), K.bRing, D.Telegraph, M.bMagic);
        if (M.bLeavesLava)
        {
            // um meteoro cai na marca; onde ele bate a lava fica um tempo
            float Ground = At.Z - Def()->Height * .5f;
            if (const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData())
            {
                const float Z = Terrain->GroundAt(At.X, At.Y);
                if (Z > -299.f) Ground = Z;
            }
            const FVector Land(At.X, At.Y, Ground);
            MulticastMeteor(Land, K.Delay);
            FTimerHandle Later;
            GetWorldTimerManager().SetTimer(Later, FTimerDelegate::CreateWeakLambda(this, [this, Land, R = K.Radius]()
            {
                MulticastLavaPool(Land, R * .75f, 12.f);
            }), K.Delay, false);
        }
    }
    if (M.Rush > 0.f)
    {
        LungeFrom = Here;
        LungeTo = Here + Forward * M.Rush;
        LungeStart = .45f;
        LungeLength = .72f;
    }
    else LungeStart = -1.f;
    Act(M.Clip, M.Duration > 0.f ? 1.f : 1.f);
    PlayMoveSound(M);
}

void AVortexMonster::ResetEncounter()
{
    bAwake = false;
    bEngaged = false;
    Phase = 1;
    Acting = INDEX_NONE;
    PatternIndex = 0;
    AbsentSince = -1.0;
    ActionLabel = TEXT("Adormecido");
    AbilitySystem->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), MaxHealthValue);
    LastHealth = MaxHealthValue;
    Act(NAME_None);
}

// ---- dano, morte e provocação -------------------------------------------------------------------

void AVortexMonster::OnHealthChanged(const FOnAttributeChangeData& Data)
{
    if (!HasAuthority() || bDead) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Data.NewValue <= 0.f) { Die(); return; }
    if (Data.NewValue < LastHealth)
    {
        // quem bateu provoca: persegue por 6 s mesmo de longe (e chama o grupo)
        if (AVortexCharacter* By = Cast<AVortexCharacter>(Combat->LastAttacker.Get()))
        {
            if (!IsBoss()) { Target = By; AlertPack(By); }
        }
        ProvokedUntil = Now + 6.0;
        const FVortexSpeciesDef* D = Def();
        // fúria: num combo o bicho ficava parado apanhando (pausa de impacto, encolher e atordoamento a cada
        // golpe). Depois de 3 a 5 golpes seguidos (mais nos grandes) ele revida na hora; os ágeis às vezes
        // saltam para trás e de lado antes. Por 1,6 s não encolhe nem é atordoado
        HitStreak = Now - LastHitAt < 1.5 ? HitStreak + 1 : 1;
        LastHitAt = Now;
        const int32 Poise = !D ? 3 : D->Radius >= 60.f ? 5 : D->Radius >= 40.f ? 4 : 3;
        if (D && !D->bBoss && !D->bPassive && Now >= ArmorUntil && HitStreak >= Poise)
        {
            HitStreak = 0;
            ArmorUntil = Now + 1.6;
            StunUntil = HitUntil = 0.0;
            Combat->StunGuardUntil = FMath::Max(Combat->StunGuardUntil, ArmorUntil + 1.0);
            if (Acting == INDEX_NONE)
            {
                const bool bAgile = D->bFlying || D->Speed >= 380.f;
                if (bAgile && FMath::FRand() < .5f)
                {
                    RetreatUntil = Now + .5;
                    NextMoveTime = Now + .5;
                }
                else
                {
                    // revide: os golpes de perto ficam prontos já
                    for (int32 i = 0; i < D->Moves.Num(); ++i) if (D->Moves[i].Kind == EVortexMoveKind::Strike) MoveReady.Remove(i);
                    NextMoveTime = Now;
                }
            }
            if (!D->HitSound.IsNone()) MulticastSound(D->HitSound, 1.f);
        }
        // reação: um golpe já começado continua; senão o bicho encolhe um instante, no máximo a cada
        // 2,2 s (encolher a cada golpe prendia a criatura na animação e ela nunca atacava); em fúria, não
        else if (D && !D->bBoss && Acting == INDEX_NONE && Now >= FlinchReadyAt && Now >= ArmorUntil)
        {
            HitUntil = Now + .25;
            FlinchReadyAt = Now + 2.2;
            Act(D->HitClip);
        }
        if (D && !D->HitSound.IsNone()) MulticastSound(D->HitSound, .7f);
    }
    LastHealth = Data.NewValue;
}

void AVortexMonster::AlertPack(AVortexCharacter* By)
{
    if (PackId < 0 || !By) return;
    for (TActorIterator<AVortexMonster> It(GetWorld()); It; ++It)
    {
        if (*It == this || It->PackId != PackId || It->bDead) continue;
        if (FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) > 2500.f) continue;
        It->Target = By;
        It->ProvokedUntil = FMath::Max(It->ProvokedUntil, GetWorld()->GetTimeSeconds() + 5.0);
    }
}

void AVortexMonster::Die()
{
    ReleaseToken();
    bDead = true;
    bEngaged = false;
    Acting = INDEX_NONE;
    LungeStart = -1.f;
    StopMoving();
    GetWorldTimerManager().ClearTimer(ThinkHandle);
    const FVortexSpeciesDef* D = Def();
    Act(D ? D->DeathClip : FName(TEXT("Death")));
    ActionLabel = TEXT("Derrotado");
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    GetCharacterMovement()->StopMovementImmediately();
    if (D && D->bFlying) GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    // o grito da espécie (gravado) e o "puf" de sumir
    const FName Voice = D && !D->bBoss ? VortexAudio::CreatureVoice(GetSpecies(), TEXT("death")) : NAME_None;
    if (!Voice.IsNone()) MulticastSound(Voice, 1.f);
    MulticastSound(D && D->bBoss ? FName(TEXT("boss_collapse")) : FName(TEXT("enemy_poof")), Voice.IsNone() ? 1.f : .6f);
    SetLifeSpan(D && D->bBoss ? 12.f : 5.f);
    ForceNetUpdate();
}

void AVortexMonster::PlayMoveSound(const FVortexMonsterMove& Move) const
{
    // os sons do Godot de cada golpe (Assets/Sfx; Tools/ImportSfx.py)
    static const TMap<FName, FName> Sounds = {
        { TEXT("Leap"), TEXT("slime_hop") }, { TEXT("Tackle"), TEXT("slime_hop") }, { TEXT("Spores"), TEXT("poison_smoke") },
        { TEXT("Spit"), TEXT("poison_spit") }, { TEXT("Slam"), TEXT("golem_slam") }, { TEXT("Punch"), TEXT("golem_slam") },
        { TEXT("Throw"), TEXT("rock_throw") }, { TEXT("Rock"), TEXT("rock_throw") }, { TEXT("Screech"), TEXT("bat_screech") },
        { TEXT("Nip"), TEXT("bat_dive") }, { TEXT("Dive"), TEXT("bat_dive") }, { TEXT("Tongue"), TEXT("slime_land") },
        { TEXT("Club"), TEXT("hammer_swing") }, { TEXT("Pincer"), TEXT("boss_punch") }, { TEXT("TailSlam"), TEXT("boss_stomp") },
        { TEXT("SandBurst"), TEXT("boss_seismic") }, { TEXT("Burrow"), TEXT("boss_dash") }, { TEXT("RootCleave"), TEXT("boss_punch") },
        { TEXT("VineSnare"), TEXT("nature_cast") }, { TEXT("SporeRing"), TEXT("boss_seismic") }, { TEXT("RootRush"), TEXT("boss_charge") },
        { TEXT("Stomp"), TEXT("boss_stomp") }, { TEXT("Seismic"), TEXT("boss_seismic") }, { TEXT("Charge"), TEXT("boss_charge") },
        { TEXT("Cataclysm"), TEXT("boss_cataclysm") }, { TEXT("Meteor"), TEXT("meteor") },
    };
    if (const FName* Sound = Sounds.Find(Move.Id)) const_cast<AVortexMonster*>(this)->MulticastSound(*Sound, .85f);
    // 03/11: o grito do bicho ao começar o golpe (goblin, esqueleto, aranha...; Tools/MakeCombatSfx.py), 6 em cada 10 vezes
    const FVortexSpeciesDef* D = Def();
    if (D && !D->bBoss && FMath::FRand() < .6f)
    {
        const FName Shout = VortexAudio::CreatureVoice(GetSpecies(), TEXT("attack"));
        if (!Shout.IsNone()) const_cast<AVortexMonster*>(this)->MulticastSound(Shout, .7f);
    }
}

void AVortexMonster::MulticastLavaPool_Implementation(FVector_NetQuantize At, float Radius, float Life)
{
    if (UVortexCalderaSubsystem* Caldera = GetWorld()->GetSubsystem<UVortexCalderaSubsystem>()) Caldera->AddPool(At, Radius, Life);
}

void AVortexMonster::MulticastMeteor_Implementation(FVector_NetQuantize At, float Flight)
{
    if (GetNetMode() == NM_DedicatedServer) return;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    if (AVortexMeteor* Rock = GetWorld()->SpawnActor<AVortexMeteor>(At, FRotator::ZeroRotator, Params)) Rock->Start(At, Flight);
}

void AVortexMonster::MulticastMoveFx_Implementation(FName Fx, FVector_NetQuantize At, float Radius, float Delay, bool bFollow)
{
    AVortexMonsterFx::Spawn(GetWorld(), Fx, At, Radius, Delay, bFollow ? this : nullptr);
}

void AVortexMonster::MulticastSound_Implementation(FName Sound, float Volume)
{
    VortexAudio::Play(this, Sound, GetActorLocation(), Volume, .07f);
}

// ---- passo a passo: movimento, saltos, voo e passos -------------------------------------------------

void AVortexMonster::Tick(float DeltaSeconds)
{
    VORTEX_PROBE("Criaturas.Tick");
    Super::Tick(DeltaSeconds);
    if (Mist && bDead && Mist->IsActive()) Mist->Deactivate();   // a névoa vai sumindo com o corpo
    // morto: o corpo não segura mais ninguém (o Die só roda no servidor; o herói de quem entrou pela sala anda no
    // cliente e ficava preso no corpo até ele sumir)
    if (bDead && GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Ignore)
    {
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    }
    const FVortexSpeciesDef* D = Def();
    if (!D) return;
    // 03/11: nos outros jogos o cavaleiro (Rider) às vezes chegava pela rede antes de existir ali, e o OnRep_Rider não
    // prendia o cavalo: ele ficava solto ao lado do amigo montado. Confere a cada quadro e prende (ou solta) de novo
    if (!HasAuthority() && IsHorse())
    {
        const AActor* Parent = GetAttachParentActor();
        if ((Rider && Parent != Rider) || (!Rider && Parent && Parent->IsA<AVortexCharacter>())) ApplyRiding();
    }
    // passos (em todas as máquinas que desenham)
    if (GetNetMode() != NM_DedicatedServer && !bDead)
    {
        const float Speed = GetVelocity().Size2D();
        StepTimer -= DeltaSeconds;
        if (Speed > 120.f && StepTimer <= 0.f && !D->bFlying && GetCharacterMovement()->IsMovingOnGround())
        {
            StepTimer = FMath::Clamp(60.f / FMath::Max(Speed, 1.f) * (D->Radius / 30.f), .18f, .8f);
            const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
            if (!D->StepSound.IsNone()) VortexAudio::Play(this, D->StepSound, Feet, D->bBoss ? .7f : .25f, .12f);
            else VortexAudio::Play(this, VortexAudio::StepSound(Feet, StepVariant++), Feet, FMath::Clamp(D->Radius / 120.f, .12f, .35f), .15f);
        }
    }
    if (D->bClimber && GetNetMode() != NM_DedicatedServer) UpdateClimbLook(DeltaSeconds);
    // 10/11: o Cogumelo Corrompido pisca sozinho (0,15 s a cada 2 a 5 s) e os olhos acendem mais quando ele vai golpear
    if (GetSpecies() == TEXT("Mushroom") && GetNetMode() != NM_DedicatedServer)
    {
        FFaceState& F = Faces().FindOrAdd(this);
        if (!F.Mat.IsValid())
        {
            UMaterialInterface* M = GetMesh()->GetMaterial(0);
            if (M && M->GetName().Contains(TEXT("Cogumelo")))
            {
                F.Mat = GetMesh()->CreateDynamicMaterialInstance(0, M);
                F.NextBlink = FMath::FRandRange(1.f, 4.f);
            }
        }
        if (UMaterialInstanceDynamic* Mat = F.Mat.Get())
        {
            F.NextBlink -= DeltaSeconds;
            if (F.NextBlink <= 0.f) { F.Blink = .15f; F.NextBlink = FMath::FRandRange(2.f, 5.f); }
            F.Blink = FMath::Max(0.f, F.Blink - DeltaSeconds);
            const FName Clip = GetActClip();
            const bool bStrike = !bDead && (Clip == TEXT("Attack") || Clip == TEXT("Spores") || Clip == TEXT("Roots") || Clip == TEXT("Burst"));
            const float Want = bDead ? 0.f : bStrike ? 6.f : bEngaged ? 2.5f : 1.5f;
            F.Glow = FMath::FInterpTo(F.Glow, Want, DeltaSeconds, bDead ? 1.5f : 6.f);
            Mat->SetScalarParameterValue(TEXT("Olhos"), F.Glow * (1.f + .12f * FMath::Sin(GetWorld()->GetTimeSeconds() * 3.1f)));
            Mat->SetScalarParameterValue(TEXT("Piscar"), bDead ? 1.f : F.Blink > 0.f ? 1.f : 0.f);
        }
    }
    if (!HasAuthority() || bDead || !bTerrainReady) return;
    const double Now = GetWorld()->GetTimeSeconds();
    // salto, mergulho ou investida: o corpo vai do ponto de partida ao de chegada em curva suave
    if (LungeStart >= 0.f && Acting != INDEX_NONE)
    {
        const float Elapsed = float(Now - ActStart);
        if (Elapsed < LungeStart)
        {
            if (AVortexCharacter* Hero = Target.Get()) FaceToward(Hero->GetActorLocation(), DeltaSeconds, 10.f);
            return;
        }
        const float T = Smooth((Elapsed - LungeStart) / LungeLength);
        FVector At = FMath::Lerp(LungeFrom, LungeTo, T);
        At.Z = GetActorLocation().Z;
        SetActorLocation(At, true);
        GetCharacterMovement()->Velocity = FVector::ZeroVector;
        if (T >= 1.f) LungeStart = -1.f;
        return;
    }
    if (Acting != INDEX_NONE)
    {
        if (AVortexCharacter* Hero = Target.Get()) FaceToward(Hero->GetActorLocation(), DeltaSeconds, D->bBoss ? 3.f : 9.f);
    }
    // morcego empurrado (o empurrão tira do voo e põe em queda): freia no ar e volta a voar logo. Antes ele
    // ficava fora do modo de voo e deslizava para trás sem parar
    if (D->bFlying && GetCharacterMovement()->MovementMode != MOVE_Flying && GetCharacterMovement()->MovementMode != MOVE_None)
    {
        UCharacterMovementComponent* Wings = GetCharacterMovement();
        const float Drag = FMath::Exp(-5.f * DeltaSeconds);
        Wings->Velocity.X *= Drag;
        Wings->Velocity.Y *= Drag;
        if (Wings->Velocity.Size2D() < 300.f || Wings->IsMovingOnGround()) Wings->SetMovementMode(MOVE_Flying);
    }
    // voo do morcego: paira um pouco acima do chão; brigando, na altura da cabeça do herói (o golpe de espada
    // alcança); no ataque desce até o peito. Antes rondava a ~2,2 m do chão (no limite do alcance e difícil de
    // mirar) e, fora do vale, o chão vinha "sem dado" (-300): ele afundava até o chão e ficava raspando
    if (D->bFlying && GetCharacterMovement()->MovementMode == MOVE_Flying)
    {
        float Ground = GetActorLocation().Z - 150.f;
        if (const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData())
        {
            // o vale (células exportadas) e o continente (o relevo do mapa do mundo)
            const float Z = FMath::Max(Terrain->GroundAt(GetActorLocation().X, GetActorLocation().Y), Terrain->QuickGround(GetActorLocation().X, GetActorLocation().Y));
            if (Z > -299.f) Ground = Z;
        }
        const AVortexCharacter* Hero = Target.Get();
        const bool bDiving = Acting != INDEX_NONE && Hero;
        const float HeroFeet = Hero ? Hero->GetActorLocation().Z - AVortexCharacter::HeroCapsuleHalfHeight : Ground;
        const float Bob = FMath::Sin(Now * 2.0) * 10.f;
        // o centro do morcego: mergulho no peito (~0,9 m), rondando na cabeça (~1,4 m), sozinho a ~1,3 m do chão
        const float Wanted = bDiving ? HeroFeet + 90.f
            : (bEngaged && Hero) ? FMath::Max(Ground + 90.f, HeroFeet + 140.f) + Bob
            : Ground + 130.f + Bob;
        FVector Velocity = GetCharacterMovement()->Velocity;
        Velocity.Z = (Wanted - GetActorLocation().Z) * 4.f;
        GetCharacterMovement()->Velocity = Velocity;
    }
    if (D->bClimber && UpdateClimb(DeltaSeconds, Now)) return;
    // 10/11: brigando, quem recua, ronda ou anda de lado continua olhando para o herói (anda de costas ou de lado);
    // só quem vai para cima dele vira para onde anda. Antes o cogumelo dava as costas para recuar e girava 180° ao cuspir
    {
        const AVortexCharacter* Foe = Target.Get();
        const bool bKiting = bEngaged && Foe && !bFleeing && Now >= RetreatUntil && Acting == INDEX_NONE && MoveSpeed > 0.f && !D->bFlying
            && FVector::DotProduct(MoveDirection, (Foe->GetActorLocation() - GetActorLocation()).GetSafeNormal2D()) < .5f;
        // fase 2: o guerreiro de escudo anda sempre com o escudo virado para o herói
        const bool bGuard = D->bShield && bEngaged && Foe && Acting == INDEX_NONE && !bFleeing;
        GetCharacterMovement()->bOrientRotationToMovement = !bKiting && !bGuard && !bDead;
        if (bKiting || bGuard) FaceToward(Foe->GetActorLocation(), DeltaSeconds, bGuard ? 7.f : 6.f);
    }
    if (MoveSpeed <= 0.f || Combat->IsRooted()) return;
    // lentidão das habilidades de classe
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed * Combat->MoveScale() * DungeonSpeed;
    // parede de fogo do grimório: desvia (pedra e fogo atravessam)
    MoveDirection = AVortexWeaponFx::SteerAroundWalls(this, MoveDirection);
    GetCharacterMovement()->MaxFlySpeed = MoveSpeed * Combat->MoveScale() * DungeonSpeed;
    AddMovementInput(MoveDirection, 1.f);
}

void AVortexMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AVortexMonster, Species, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexMonster, LookSeed, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexMonster, Level, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexMonster, bElite, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexMonster, PackSlot, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexMonster, PackSize, COND_InitialOnly);
    DOREPLIFETIME(AVortexMonster, ActClip);
    DOREPLIFETIME(AVortexMonster, ClimbPose);
    DOREPLIFETIME(AVortexMonster, ThreadTopZ);
    DOREPLIFETIME(AVortexMonster, Tamer);
    DOREPLIFETIME(AVortexMonster, Rider);
    DOREPLIFETIME(AVortexMonster, TameFed);
    DOREPLIFETIME(AVortexMonster, TameNeed);
    DOREPLIFETIME(AVortexMonster, ActSerial);
    DOREPLIFETIME(AVortexMonster, ActRate);
    DOREPLIFETIME(AVortexMonster, bDead);
    DOREPLIFETIME(AVortexMonster, bEngaged);
    DOREPLIFETIME(AVortexMonster, bGrazing);
    DOREPLIFETIME(AVortexMonster, bFleeing);
    DOREPLIFETIME(AVortexMonster, Phase);
    DOREPLIFETIME(AVortexMonster, ActionLabel);
    DOREPLIFETIME(AVortexMonster, bTerrainReady);
    DOREPLIFETIME(AVortexMonster, bDungeon);
}

// ---- aranha: parede, teto e fio ------------------------------------------------------------------
// "faça elas poderem andar no teto e parede das dungeons e caírem se pendurarem nas teias"
bool AVortexMonster::UpdateClimb(float DeltaSeconds, double Now)
{
    UCharacterMovementComponent* Legs = GetCharacterMovement();
    const FVector At = GetActorLocation();
    const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
    AVortexCharacter* Hero = Target.Get();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexSpiderClimb), false, this);
    if (Hero) Query.AddIgnoredActor(Hero);
    auto Trace = [&](const FVector& From, const FVector& To, FHitResult& Hit) { return GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_WorldStatic, Query); };
    auto Let = [&](bool bWithThread, float CeilingZ)
    {
        // solta: cai (com o fio esticado até onde estava presa, por um instante)
        ClimbPose = 0;
        Legs->SetMovementMode(MOVE_Falling);
        ThreadTopZ = bWithThread ? CeilingZ : 0.f;
        ThreadDropAt = Now;
        MulticastSound(TEXT("creature_hit"), .4f);
        ForceNetUpdate();
    };
    // o fio some um pouco depois de cair
    if (ClimbPose != 3 && ThreadTopZ > 0.f && Now - ThreadDropAt > 1.4) { ThreadTopZ = 0.f; ForceNetUpdate(); }
    switch (ClimbPose)
    {
    case 3:   // pendurada no fio, no alto: espera alguém passar embaixo
    {
        Legs->Velocity = FVector(0.f, 0.f, 6.f * FMath::Sin(Now * 1.3));
        AVortexCharacter* Near = Hero ? Hero : FindTarget(900.f);
        if (Near && FVector::Dist2D(Near->GetActorLocation(), At) < 600.f) { Target = Near; Let(true, ThreadTopZ); }
        return true;
    }
    case 2:   // no teto (dungeons): anda de cabeça para baixo atrás do herói e desce no fio em cima dele
    {
        FHitResult Up;
        if (!Trace(At, At + FVector(0.f, 0.f, Half + 250.f), Up)) { Let(false, 0.f); return true; }
        FVector Velocity = MoveDirection.GetSafeNormal2D() * MoveSpeed * .85f;
        Velocity.Z = (Up.ImpactPoint.Z - Half - 4.f - At.Z) * 6.f;
        Legs->Velocity = Velocity;
        if (!MoveDirection.IsNearlyZero()) FaceToward(At + MoveDirection * 100.f, DeltaSeconds, 8.f);
        if (Hero && FVector::Dist2D(Hero->GetActorLocation(), At) < 260.f && Hero->GetActorLocation().Z < At.Z - 160.f) Let(true, Up.ImpactPoint.Z);
        return true;
    }
    case 1:   // na parede (ou tronco): sobe; no alto passa por cima; no teto, gruda nele
    {
        const FVector Forward = GetActorForwardVector();
        FHitResult Wall, Up;
        if (Trace(At, At + FVector(0.f, 0.f, Half + 45.f), Up) && Up.ImpactNormal.Z < -.6f) { ClimbPose = 2; ForceNetUpdate(); return true; }
        const bool bWall = Trace(At, At + Forward * (Radius + 70.f), Wall) && Wall.ImpactNormal.Z < .45f;
        if (!bWall)
        {
            Legs->SetMovementMode(MOVE_Falling);
            Legs->Velocity = Forward * 320.f + FVector(0.f, 0.f, 260.f);
            ClimbPose = 0;
            ForceNetUpdate();
            return true;
        }
        // o herói ficou para trás, lá embaixo: solta
        if (Hero && Hero->GetActorLocation().Z < At.Z - 350.f && FVector::DotProduct((Hero->GetActorLocation() - At).GetSafeNormal2D(), Forward) < -.2f) { Let(false, 0.f); return true; }
        Legs->Velocity = FVector(0.f, 0.f, FMath::Max(MoveSpeed, 260.f) * .75f) - Wall.ImpactNormal * 60.f;
        return true;
    }
    default:  // no chão: parede alta na frente durante a perseguição, sobe
    {
        if (MoveSpeed <= 0.f || MoveDirection.IsNearlyZero() || !Legs->IsMovingOnGround() || Acting != INDEX_NONE) return false;
        const FVector Direction = MoveDirection.GetSafeNormal2D();
        FHitResult Low, High;
        if (!Trace(At, At + Direction * (Radius + 45.f), Low) || Low.ImpactNormal.Z > .3f) return false;
        if (!Trace(At + FVector(0.f, 0.f, Half + 60.f), At + FVector(0.f, 0.f, Half + 60.f) + Direction * (Radius + 70.f), High)) return false;
        SetActorRotation(FRotator(0.f, (-Low.ImpactNormal).GetSafeNormal2D().Rotation().Yaw, 0.f));
        Legs->SetMovementMode(MOVE_Flying);
        Legs->Velocity = FVector::ZeroVector;
        ClimbPose = 1;
        ForceNetUpdate();
        return true;
    }
    }
}

void AVortexMonster::UpdateClimbLook(float DeltaSeconds)
{
    // o corpo: na parede de barriga para ela (empinado 90°), no teto de cabeça para baixo
    USkeletalMeshComponent* Body = GetMesh();
    if (!Body) return;
    const FQuat Turn = ClimbPose == 1 ? FQuat(FRotator(90.f, 0.f, 0.f)) : ClimbPose == 2 ? FQuat(FRotator(0.f, 0.f, 180.f)) : FQuat::Identity;
    const FQuat Wanted = Turn * BaseMeshRotation;
    const FQuat Now = FQuat::Slerp(Body->GetRelativeRotation().Quaternion(), Wanted, FMath::Clamp(DeltaSeconds * 10.f, 0.f, 1.f));
    Body->SetRelativeRotation(Now);
    Body->SetRelativeLocation((Now * BaseMeshRotation.Inverse()).RotateVector(BaseMeshLocation));
    // o fio: do corpo até o ponto onde ela estava presa
    const bool bThread = ThreadTopZ > GetActorLocation().Z + 30.f && !bDead;
    if (bThread && !Thread)
    {
        Thread = NewObject<UStaticMeshComponent>(this, TEXT("SpiderThread"));
        Thread->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
        Thread->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Thread->SetCastShadow(false);
        Thread->SetUsingAbsoluteRotation(true);
        Thread->SetupAttachment(RootComponent);
        Thread->RegisterComponent();
        if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/World/Portal/M_PortalGlow.M_PortalGlow")))
        {
            UMaterialInstanceDynamic* Silk = UMaterialInstanceDynamic::Create(Glow, this);
            Silk->SetVectorParameterValue(TEXT("Color"), FLinearColor(.8f, .82f, .9f));
            Silk->SetScalarParameterValue(TEXT("Intensity"), .6f);
            Thread->SetMaterial(0, Silk);
        }
    }
    if (Thread)
    {
        Thread->SetVisibility(bThread);
        if (bThread)
        {
            const float Length = ThreadTopZ - GetActorLocation().Z;
            Thread->SetWorldLocation(GetActorLocation() + FVector(0.f, 0.f, Length * .5f));
            Thread->SetWorldScale3D(FVector(.025f, .025f, Length / 100.f));
        }
    }
}