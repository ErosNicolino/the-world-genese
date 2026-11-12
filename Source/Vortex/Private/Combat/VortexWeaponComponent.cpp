#include "Combat/VortexWeaponComponent.h"

#include "AbilitySystemComponent.h"
#include "Combat/VortexSwordAbility.h"
#include "Combat/VortexGuardAbility.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Combat/VortexWeaponKit.h"
#include "Combat/VortexSwords.h"
#include "Engine/StaticMesh.h"
#include "Framework/VortexCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexSlashWave.h"
#include "Combat/VortexProjectile.h"
#include "Combat/VortexSeriousPunch.h"
#include "Rpg/VortexHeroSheet.h"
#include "Combat/VortexCombatComponent.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

UVortexWeaponComponent::UVortexWeaponComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
}

void UVortexWeaponComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner()->HasAuthority())
    {
        UVortexWeaponDefinition* Default = LoadObject<UVortexWeaponDefinition>(nullptr,
            TEXT("/Game/Vortex/Weapons/Sword/DA_DefaultSword.DA_DefaultSword"));
        if (Default) Equip(Default);
        else UE_LOG(LogTemp, Error, TEXT("VORTEX_WEAPON missing DA_DefaultSword"));
    }
}

void UVortexWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UVortexWeaponComponent, EquippedDefinition);
    DOREPLIFETIME(UVortexWeaponComponent, AttackState);
    DOREPLIFETIME(UVortexWeaponComponent, bGuarding);
    DOREPLIFETIME(UVortexWeaponComponent, GuardStartServerTime);
    DOREPLIFETIME(UVortexWeaponComponent, SpecialReadyServerTime);
    DOREPLIFETIME(UVortexWeaponComponent, bSpecialActive);
    DOREPLIFETIME(UVortexWeaponComponent, ChargeStartServerTime);
    DOREPLIFETIME(UVortexWeaponComponent, bCharging);
    DOREPLIFETIME(UVortexWeaponComponent, AttackSpeed);
    DOREPLIFETIME(UVortexWeaponComponent, ThrowMask);
    DOREPLIFETIME(UVortexWeaponComponent, DrawStartServerTime);
    DOREPLIFETIME_CONDITION(UVortexWeaponComponent, LockTarget, COND_OwnerOnly);
    DOREPLIFETIME(UVortexWeaponComponent, bDrawing);
}

void UVortexWeaponComponent::SetCombatPace(float Speed, float Cooldowns)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    CooldownScale = Cooldowns;
    if (!FMath::IsNearlyEqual(AttackSpeed, Speed))
    {
        AttackSpeed = Speed;
        GetOwner()->ForceNetUpdate();
    }
}

bool UVortexWeaponComponent::Equip(UVortexWeaponDefinition* NewDefinition)
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    if (!Owner || !Owner->HasAuthority() || !Owner->GetAbilitySystemComponent() ||
        (NewDefinition && (!NewDefinition->VisualMesh || !NewDefinition->PrimaryAbility || NewDefinition->Swings.IsEmpty()))) return false;
    UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent();
    if (GrantedAbility.IsValid()) ASC->ClearAbility(GrantedAbility);
    if (GrantedGuardAbility.IsValid()) ASC->ClearAbility(GrantedGuardAbility);
    ActiveAbility = nullptr;
    ActiveGuardAbility = nullptr;
    bGuarding = false;
    EquippedDefinition = NewDefinition;
    GrantedAbility = NewDefinition
        ? ASC->GiveAbility(FGameplayAbilitySpec(NewDefinition->PrimaryAbility, 1))
        : FGameplayAbilitySpecHandle();
    GrantedGuardAbility = NewDefinition && NewDefinition->SecondaryAbility
        ? ASC->GiveAbility(FGameplayAbilitySpec(NewDefinition->SecondaryAbility, 1))
        : FGameplayAbilitySpecHandle();
    LastSwingIndex = INDEX_NONE;
    LastSwingEndTime = -1000.f;
    ApplyVisual();
    Owner->ForceNetUpdate();
    return true;
}

void UVortexWeaponComponent::OnRep_EquippedDefinition() { ApplyVisual(); }

float UVortexWeaponComponent::MeleeReach(const FVortexWeaponSwing& Swing) const
{
    // Os alcances vieram do Godot em metros do Godot (a espada: 3 m * 1,3 + 0,4 = 4,3 m) e não passaram pela
    // escala do herói (0,367, a mesma das armas em Data/VortexWeapons/frames.json): os golpes acertavam
    // quem estava a ~2,7 vezes a distância em que a lâmina chega. Aqui o alcance sai do próprio boneco
    const AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
    if (!Hero || !EquippedDefinition) return Swing.ReachCm;
    // Chicote e correntes estendem partes procedurais; a malha da empunhadura não representa o alcance.
    const VortexWeaponKit::FKit* Kit = VortexWeaponKit::Find(EquippedDefinition->WeaponId);
    if (Kit && (Kit->Family == TEXT("Whip") || Kit->Family == TEXT("Chain")))
        return FMath::Clamp(Swing.ReachCm, 1.f, 600.f);
    const float Body = Hero->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float Arm = 55.f * Hero->GetActorScale3D().Z;
    float Length = 0.f;
    // o comprimento da arma: o canto do modelo mais longe da pegada (a origem, no soquete da mão)
    for (const UStaticMesh* Mesh : { EquippedDefinition->VisualMesh.Get(), EquippedDefinition->LeftMesh.Get() })
    {
        if (!Mesh) continue;
        const FBox Box = Mesh->GetBoundingBox();
        for (int32 i = 0; i < 8; ++i)
        {
            const FVector Corner((i & 1) ? Box.Max.X : Box.Min.X, (i & 2) ? Box.Max.Y : Box.Min.Y, (i & 4) ? Box.Max.Z : Box.Min.Z);
            Length = FMath::Max(Length, float(Corner.Size()));
        }
    }
    // folga: o golpe varre um arco e o alvo se mexe; a pancada no chão (slam) espalha mais; na estocada da lança e
    // da alabarda o braço inteiro vai para a frente (a pegada anda ~60 cm), e só contar o modelo parado deixava a
    // lança alcançando quase o mesmo que a espada
    const float Slack = Swing.Mode == TEXT("slam") ? 70.f : IsPolearm(Kit) ? 95.f : 35.f;
    return FMath::Min(Swing.ReachCm, Body + Arm + Length + Slack);
}

bool UVortexWeaponComponent::IsPolearm(const VortexWeaponKit::FKit* Kit)
{
    return Kit && (Kit->Family == TEXT("Spear") || Kit->Family == TEXT("Halberd"));
}

float UVortexWeaponComponent::MeleeCone(const FVortexWeaponSwing& Swing) const
{
    // a estocada da lança vinha com o cone mais estreito de todas as armas (0,75: ~40° para cada lado). Sem mira
    // automática (só a marcação de foco), um monstro um pouco de lado já escapava: abre para ~60°
    if (EquippedDefinition && IsPolearm(VortexWeaponKit::Find(EquippedDefinition->WeaponId))) return FMath::Min(Swing.MinForwardDot, .5f);
    return Swing.MinForwardDot;
}

void UVortexWeaponComponent::OnRep_ThrowMask() { ApplyVisual(); }

FName UVortexWeaponComponent::HandBoneFor(const UVortexWeaponDefinition* Definition, const USkeletalMeshComponent* Mesh)
{
    if (Definition && Mesh && Definition->WeaponId.ToString().StartsWith(TEXT("arco")) && Mesh->GetBoneIndex(TEXT("Bow_L")) != INDEX_NONE)
        return TEXT("Bow_L");
    return Definition && !Definition->HandBone.IsNone() ? Definition->HandBone : FName(TEXT("Sword"));
}

FQuat UVortexWeaponComponent::GripRotationFor(FName Bone)
{
    // medido na Unreal no fim do Arco_Draw: o eixo do comprimento do arco (Z) saía ao longo da mira e o da corda (Y) para
    // cima. Meia volta em torno de (0,1,1) troca Y e Z (e vira o X): o arco fica em pé, a corda para a mão direita
    // 01/11 (teste do usuário): o arco estava virado ao contrário (as pontas curvando para longe e a corda do lado de
    // fora). Meia volta em torno do comprimento do arco (o Z do modelo): o cabo fica para o alvo e a corda para o corpo
    return Bone == TEXT("Bow_L") ? FQuat(FVector(0.f, 1.f, 1.f).GetSafeNormal(), PI) * FQuat(FVector::UpVector, PI) : FQuat::Identity;
}

namespace
{
    /** 07/11: as 35 espadas e os 35 arcos das pranchas do usuário (Tools/Blender/MakePixelSwords.py e MakePixelBows.py,
     *  do Normal ao Divino): cada um com o seu modelo (a Lâmina do Recruta e o Arco de Bétula são os iniciais); as
     *  variantes do arsenal, um dos 5 modelos da raridade delas (VortexSwords::ForVisual). */
    UStaticMesh* PixelSwordFor(const AVortexCharacter* Owner)
    {
        const UVortexHeroSheet* Sheet = Owner ? Owner->GetSheet() : nullptr;
        const FName Item = Sheet ? Sheet->GetEquipped(VortexRpg::Weapon) : NAME_None;
        const UVortexWeaponDefinition* Definition = Owner && Owner->GetWeapon() ? Owner->GetWeapon()->GetDefinition() : nullptr;
        const VortexSwords::FSword* Sword = VortexSwords::ForVisual(Item);
        if (!Sword || !Definition || Definition->WeaponId != Sword->Weapon) return nullptr;
        return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Weapons/Models/%s.%s"), *Sword->Model, *Sword->Model));
    }
}

void UVortexWeaponComponent::ApplyVisual()
{
    if (const AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner()))
    {
        if (UStaticMeshComponent* Visual = Owner->GetWeaponVisual())
        {
            // a espada vai no osso "Sword"; as armas do Godot no soquete da mão (já vêm na pegada)
            const FName Bone = HandBoneFor(EquippedDefinition, Owner->GetMesh());
            if (Visual->GetAttachSocketName() != Bone) Visual->AttachToComponent(Owner->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Bone);
            Visual->SetRelativeRotation(GripRotationFor(Bone));
            UStaticMesh* Pixel = EquippedDefinition ? PixelSwordFor(Owner) : nullptr;
            VortexSwords::Preload(Owner);   // 07/11: o poder da arma nova já carregado
            Visual->SetStaticMesh(Pixel ? Pixel : EquippedDefinition ? EquippedDefinition->VisualMesh.Get() : nullptr);
            // arremessada (machado, chakram): a mão fica vazia até a arma voltar
            Visual->SetVisibility(EquippedDefinition != nullptr && !(ThrowMask & 1));
        }
        if (UStaticMeshComponent* Left = Owner->GetWeaponVisualLeft())
        {
            Left->SetStaticMesh(EquippedDefinition ? EquippedDefinition->LeftMesh : nullptr);
            Left->SetVisibility(EquippedDefinition && EquippedDefinition->LeftMesh && !(ThrowMask & 2));
        }
    }
}

void UVortexWeaponComponent::PressPrimary()
{
    if (!EquippedDefinition) return;
    if (GetOwner()->HasAuthority()) PressPrimaryOnServer();
    else ServerPressPrimary();
}

void UVortexWeaponComponent::ServerPressPrimary_Implementation() { PressPrimaryOnServer(); }

void UVortexWeaponComponent::PressPrimaryOnServer()
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    if (!Owner || !EquippedDefinition || !Owner->GetAbilitySystemComponent() || bGuarding || bSpecialActive || Owner->IsDrinking()) return;
    // machados duplos no ar: sem golpe até voltarem
    if (ThrowMask == 3 && EquippedDefinition->WeaponId == TEXT("machados_duplos")) return;
    // arco: segurar o clique estica a corda (soltar dispara, ReleasePrimary)
    if (const VortexWeaponKit::FKit* Kit = VortexWeaponKit::Find(EquippedDefinition->WeaponId); Kit && Kit->Family == TEXT("Bow"))
    {
        if (bDrawing || ActiveAbility || GetWorld()->GetTimeSeconds() < BowReadyTime) return;
        const AGameStateBase* State = GetWorld()->GetGameState();
        bDrawing = true;
        DrawLocalStart = GetWorld()->GetTimeSeconds();
        DrawStartServerTime = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
        ++AttackState.Serial;
        AttackState.SwingIndex = VortexSpecialPose::Hold;
        AttackState.ServerStartTime = GetWorld()->GetTimeSeconds();
        Owner->ForceNetUpdate();
        KitSound(TEXT("wfx_bow_draw"), Owner->GetActorLocation(), .8f);
        GetWorld()->GetTimerManager().SetTimer(DrawTimer, this, &UVortexWeaponComponent::BowFire, 3.f, false);
        return;
    }
    if (ActiveAbility)
    {
        ActiveAbility->BufferNext();
    }
    else
    {
        Owner->GetAbilitySystemComponent()->TryActivateAbilityByClass(EquippedDefinition->PrimaryAbility);
    }
}

void UVortexWeaponComponent::SetGuardPressed(bool bPressed)
{
    if (GetOwner()->HasAuthority()) SetGuardPressedOnServer(bPressed);
    else ServerSetGuardPressed(bPressed);
}

void UVortexWeaponComponent::ServerSetGuardPressed_Implementation(bool bPressed) { SetGuardPressedOnServer(bPressed); }

void UVortexWeaponComponent::SetGuardPressedOnServer(bool bPressed)
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    if (!Owner || !EquippedDefinition || !Owner->GetAbilitySystemComponent()) return;
    if (!bPressed)
    {
        if (ActiveGuardAbility) ActiveGuardAbility->Release();
        return;
    }
    if (!EquippedDefinition->SecondaryAbility || bGuarding) return;
    if (ActiveAbility)
    {
        const FGameplayTagContainer AttackTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Weapon.Sword.Primary")));
        Owner->GetAbilitySystemComponent()->CancelAbilities(&AttackTag);
    }
    Owner->GetAbilitySystemComponent()->TryActivateAbilityByClass(EquippedDefinition->SecondaryAbility);
}

void UVortexWeaponComponent::RegisterGuardStart(UVortexGuardAbility* Ability)
{
    if (!GetOwner()->HasAuthority()) return;
    ActiveGuardAbility = Ability;
    bGuarding = true;
    GuardStartServerTime = GetWorld()->GetTimeSeconds();
    GetOwner()->ForceNetUpdate();
}

void UVortexWeaponComponent::RegisterGuardEnd(UVortexGuardAbility* Ability)
{
    if (!GetOwner()->HasAuthority() || ActiveGuardAbility != Ability) return;
    ActiveGuardAbility = nullptr;
    bGuarding = false;
    GetOwner()->ForceNetUpdate();
}

float UVortexWeaponComponent::ResolveGuardDamage(const AActor* Attacker, float Incoming, bool& bPerfectParry) const
{
    bPerfectParry = false;
    if (!GetOwner()->HasAuthority() || !bGuarding || !Attacker) return Incoming;
    const FVector TowardAttacker = (Attacker->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal2D();
    if (FVector::DotProduct(GetOwner()->GetActorForwardVector().GetSafeNormal2D(), TowardAttacker) < .2f) return Incoming;
    if (GetWorld()->GetTimeSeconds() - GuardStartServerTime <= .22f)
    {
        bPerfectParry = true;
        return Incoming <= 15.f ? 0.f : FMath::CeilToFloat((Incoming - 15.f) * .5f);
    }
    // defesa comum segura 65% do golpe de frente; o escudo segura 80% (VortexWeaponKit::GuardKeep)
    const VortexWeaponKit::FKit* Kit = EquippedDefinition ? VortexWeaponKit::Find(EquippedDefinition->WeaponId) : nullptr;
    return FMath::Max(1.f, FMath::RoundToFloat(Incoming * (Kit ? Kit->GuardKeep : .35f)));
}

int32 UVortexWeaponComponent::NextSwingIndex() const
{
    if (!EquippedDefinition || EquippedDefinition->Swings.IsEmpty()) return INDEX_NONE;
    const float Now = GetWorld()->GetTimeSeconds();
    return LastSwingIndex >= 0 && LastSwingIndex + 1 < EquippedDefinition->Swings.Num() &&
        Now - LastSwingEndTime <= EquippedDefinition->ChainGraceSeconds ? LastSwingIndex + 1 : 0;
}

void UVortexWeaponComponent::RegisterAttackStart(UVortexSwordAbility* Ability, int32 Index)
{
    if (!GetOwner()->HasAuthority()) return;
    ActiveAbility = Ability;
    ++AttackState.Serial;
    AttackState.SwingIndex = Index;
    AttackState.ServerStartTime = GetWorld()->GetTimeSeconds();
    GetOwner()->ForceNetUpdate();
}

void UVortexWeaponComponent::RegisterAttackEnd(UVortexSwordAbility* Ability, int32 Index)
{
    if (!GetOwner()->HasAuthority() || ActiveAbility != Ability) return;
    ActiveAbility = nullptr;
    LastSwingIndex = Index;
    LastSwingEndTime = GetWorld()->GetTimeSeconds();
    AttackState.SwingIndex = INDEX_NONE;
    ++AttackState.Serial;
    GetOwner()->ForceNetUpdate();
}

// ---- especial ---------------------------------------------------------------------------------

void UVortexWeaponComponent::PressSpecial()
{
    if (!EquippedDefinition) return;
    if (GetOwner()->HasAuthority()) PressSpecialOnServer();
    else ServerPressSpecial();
}

void UVortexWeaponComponent::ServerPressSpecial_Implementation() { PressSpecialOnServer(); }

float UVortexWeaponComponent::SpecialCooldownTotal() const
{
    if (!EquippedDefinition) return 1.f;
    if (HasSeriousPunch()) return FMath::Max(.1f, SeriousPunchCooldown * GetCooldownScale());
    if (const VortexWeaponKit::FKit* Kit = VortexSwords::SpecialKit(GetOwner(), EquippedDefinition); Kit && Kit->SpecialCooldown > 0.f)
        return FMath::Max(.1f, Kit->SpecialCooldown * GetCooldownScale());
    const float Base = EquippedDefinition->SecondaryKind == TEXT("charge") ? EquippedDefinition->Secondary(TEXT("cooldown"), 1.f) : EquippedDefinition->SpecialCooldown;
    return FMath::Max(.1f, Base * GetCooldownScale());
}

float UVortexWeaponComponent::SpecialCooldownRemaining() const
{
    const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    const double Now = State ? State->GetServerWorldTimeSeconds() : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
    return FMath::Max(0.f, float(SpecialReadyServerTime - Now));
}

void UVortexWeaponComponent::PressSpecialOnServer()
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    const VortexWeaponKit::FKit* Kit = EquippedDefinition ? VortexSwords::SpecialKit(GetOwner(), EquippedDefinition) : nullptr;
    // espada e escudo: o botão direito levanta o escudo (sem recarga)
    if (Owner && Kit && Kit->Special == TEXT("shield")) { SetGuardPressedOnServer(true); return; }
    // conjurando a prisão de vinhas: apertar de novo cancela (a mana volta)
    if (bCasting) { ReleaseKitSpecial(); return; }
    if (!Owner || !EquippedDefinition || bGuarding || bSpecialActive || ActiveAbility || SpecialCooldownRemaining() > 0.f || Owner->IsDrinking()) return;
    const UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent();
    if (!ASC || ASC->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) <= 0.f ||
        Owner->GetVortexMovement()->IsDodging() || Owner->GetVortexMovement()->IsSwimmingInWater()) return;
    if (HasSeriousPunch()) { BeginSeriousPunch(); return; }
    if (BeginKitSpecial()) return;
    if (EquippedDefinition->SecondaryKind == TEXT("charge"))
    {
        // segurar: a pose de carga (o clipe próprio, ou o golpe mais forte parado no começo)
        bSpecialActive = true;
        bCharging = true;
        ChargeStartServerTime = GetWorld()->GetTimeSeconds();
        ++AttackState.Serial;
        AttackState.SwingIndex = VortexSpecialPose::Hold;
        AttackState.ServerStartTime = ChargeStartServerTime;
        Owner->ForceNetUpdate();
        // segurando demais: solta sozinho
        GetWorld()->GetTimerManager().SetTimer(SpecialReleaseTimer, this, &UVortexWeaponComponent::ReleaseSpecialOnServer, 3.f, false);
        return;
    }
    const int32 Count = EquippedDefinition->Swings.Num();
    SpecialSwing = EquippedDefinition->SpecialSwingIndex >= 0 && EquippedDefinition->SpecialSwingIndex < Count
        ? EquippedDefinition->SpecialSwingIndex : Count - 1;
    if (SpecialSwing < 0) return;
    const AGameStateBase* State = GetWorld()->GetGameState();
    const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    SpecialReadyServerTime = float(Now + EquippedDefinition->SpecialCooldown * GetCooldownScale());
    bSpecialActive = true;
    // a mesma animação do golpe do combo, vista por todos pelo AttackState
    ++AttackState.Serial;
    AttackState.SwingIndex = SpecialSwing;
    AttackState.ServerStartTime = GetWorld()->GetTimeSeconds();
    Owner->ForceNetUpdate();
    const FVortexWeaponSwing& Swing = EquippedDefinition->Swings[SpecialSwing];
    FTimerManager& Timers = GetWorld()->GetTimerManager();
    Timers.SetTimer(SpecialReleaseTimer, this, &UVortexWeaponComponent::ReleaseSpecial, FMath::Max(.01f, EquippedDefinition->SpecialRelease), false);
    Timers.SetTimer(SpecialFinishTimer, this, &UVortexWeaponComponent::FinishSpecial,
        FMath::Max(EquippedDefinition->SpecialRelease + .05f, Swing.Duration), false);
}

// ---- especial carregado (Godot: kind "charge") ------------------------------------------------

void UVortexWeaponComponent::ReleaseSpecialInput()
{
    if (!EquippedDefinition) return;
    if (const VortexWeaponKit::FKit* Kit = VortexSwords::SpecialKit(GetOwner(), EquippedDefinition); Kit && !Kit->Special.IsNone() && !HasSeriousPunch())
    {
        if (GetOwner()->HasAuthority()) ReleaseKitSpecial();
        else ServerReleaseSpecial();
        return;
    }
    if (EquippedDefinition->SecondaryKind != TEXT("charge")) return;
    if (GetOwner()->HasAuthority()) ReleaseSpecialOnServer();
    else ServerReleaseSpecial();
}

void UVortexWeaponComponent::ServerReleaseSpecial_Implementation()
{
    const VortexWeaponKit::FKit* Kit = EquippedDefinition ? VortexSwords::SpecialKit(GetOwner(), EquippedDefinition) : nullptr;
    if (Kit && !Kit->Special.IsNone() && !HasSeriousPunch()) ReleaseKitSpecial();
    else ReleaseSpecialOnServer();
}

float UVortexWeaponComponent::SpecialCharge() const
{
    if (!bCharging || !GetWorld()) return 0.f;
    const AGameStateBase* State = GetWorld()->GetGameState();
    const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    return FMath::Clamp(float(Now - ChargeStartServerTime) / 1.2f, 0.f, 1.f);
}

void UVortexWeaponComponent::ReleaseSpecialOnServer()
{
    if (!bCharging || !EquippedDefinition) return;
    GetWorld()->GetTimerManager().ClearTimer(SpecialReleaseTimer);
    // 1,2 s de carga = força máxima; um toque rápido ainda solta o mínimo
    const float Charge = FMath::Clamp((GetWorld()->GetTimeSeconds() - ChargeStartServerTime) / 1.2f, 0.f, 1.f);
    bCharging = false;
    ++AttackState.Serial;
    AttackState.SwingIndex = VortexSpecialPose::Release;
    AttackState.ServerStartTime = GetWorld()->GetTimeSeconds();
    const AGameStateBase* State = GetWorld()->GetGameState();
    const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    SpecialReadyServerTime = float(Now + EquippedDefinition->Secondary(TEXT("cooldown"), 1.f) * GetCooldownScale());
    GetOwner()->ForceNetUpdate();
    // o golpe sai um instante depois de soltar (o braço desce)
    FTimerDelegate Fire = FTimerDelegate::CreateUObject(this, &UVortexWeaponComponent::ChargeEffect, Charge);
    GetWorld()->GetTimerManager().SetTimer(SpecialFinishTimer, Fire, .12f, false);
}

void UVortexWeaponComponent::ChargeEffect(float Charge)
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    if (!Owner || !EquippedDefinition) return;
    const UVortexWeaponDefinition& D = *EquippedDefinition;
    const float Damage = FMath::Lerp(D.Secondary(TEXT("min"), 4.f), D.Secondary(TEXT("max"), 10.f), Charge);
    const FName Shape = D.SecondaryShape;
    const FName Element = D.Swings.Num() ? D.Swings.Last().Element : NAME_None;
    UE_LOG(LogTemp, Display, TEXT("VORTEX_CHARGE %s forma=%s carga=%.2f dano=%.0f"), *D.WeaponId.ToString(), *Shape.ToString(), Charge, Damage);
    if (Shape == TEXT("shot") || Shape == TEXT("pierce"))
    {
        // flecha/virote/tiro carregado: mais rápido, mais longe e maior
        const FVortexWeaponSwing* Last = D.Swings.Num() ? &D.Swings.Last() : nullptr;
        const FName Mode = Last && Last->Mode != TEXT("melee") ? (Last->Mode == TEXT("orb") ? FName(TEXT("bolt")) : Last->Mode) : FName(TEXT("arrow"));
        LaunchProjectile(Mode, Element, Damage, (Last ? Last->ProjectileSpeed : 2800.f) * (1.2f + .5f * Charge),
            D.Secondary(TEXT("reach"), 2000.f), 0.f, 1.f + .8f * Charge, Shape == TEXT("pierce"));
    }
    else if (Shape == TEXT("slam") || Shape == TEXT("reap"))
    {
        // pancada no chão / giro em volta: todos no raio, em volta do herói
        LaunchProjectile(TEXT("burst"), Element.IsNone() ? FName(TEXT("steel")) : Element, Damage, 0.f, 0.f,
            D.Secondary(TEXT("radius"), 340.f) * (.75f + .25f * Charge), 1.f + Charge);
    }
    else if (Shape == TEXT("circle"))
    {
        // círculo na mira
        FVector Aim;
        LaunchProjectile(TEXT("burst"), Element, Damage, 0.f, D.Secondary(TEXT("reach"), 1400.f), D.Secondary(TEXT("radius"), 340.f), 1.f + Charge, false, &Aim);
    }
    else if (Shape == TEXT("lunge") || Shape == TEXT("cleave"))
    {
        // investida: o herói avança perfurando o caminho (cleave: corte largo parado)
        LungeDamage = Damage;
        LungeHits.Reset();
        LungeTicks = 0;
        if (Shape == TEXT("lunge"))
        {
            const float Distance = FMath::Lerp(300.f, D.Secondary(TEXT("reach"), 700.f), Charge);
            Owner->LaunchCharacter(Owner->GetActorForwardVector().GetSafeNormal2D() * Distance / .3f, true, false);
        }
        GetWorld()->GetTimerManager().SetTimer(LungeTimer, FTimerDelegate::CreateWeakLambda(this, [this, Shape]()
        {
            AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
            if (Hero && Hero->GetCombat())
            {
                const bool bCleave = Shape == TEXT("cleave");
                Hero->GetCombat()->StrikeInArc(1.f, bCleave ? EquippedDefinition->Secondary(TEXT("reach"), 420.f) : 260.f, bCleave ? -.1f : .3f, LungeHits, 0.f, LungeDamage);
            }
            if (++LungeTicks >= 8) GetWorld()->GetTimerManager().ClearTimer(LungeTimer);
        }), .04f, true);
    }
    VortexAudio::Play(this, TEXT("skill_slash"), Owner->GetActorLocation(), .7f + .3f * Charge);
    GetWorld()->GetTimerManager().SetTimer(SpecialReleaseTimer, this, &UVortexWeaponComponent::FinishCharge, .45f, false);
}

void UVortexWeaponComponent::FinishCharge()
{
    FinishSpecial();
}

namespace VortexVolley { float Yaw = 0.f; }   // shortcut: um valor solto (o leque, VortexWeaponSpecials.cpp); um parâmetro no .h recompilaria tudo

AVortexProjectile* UVortexWeaponComponent::LaunchProjectile(FName Mode, FName Element, float Damage, float SpeedCm, float RangeCm,
    float RadiusCm, float Size, bool bPierce, FVector* AimPoint, float ArrowCharge)
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    if (!Owner || !Owner->HasAuthority()) return nullptr;
    // a arma da mochila com elemento pinta o tiro (o rastro, o brilho e a luz) com ele
    if (const FName Held = VortexWeaponKit::HeldElement(Owner); !Held.IsNone() && Element != TEXT("rune") && Mode != TEXT("arrow"))
        Element = Held == TEXT("lightning") ? FName(TEXT("arcane")) : Held;
    // G9: primeiro o ponto que a mira enxerga (o raio da câmera até onde bate); G8: o tiro sai da
    // ponta da arma em direção a esse ponto, então sai da arma e chega exatamente na mira
    const FVector Chest = Owner->GetActorLocation() + FVector(0.f, 0.f, 35.f);
    FVector Target = UVortexWeaponComponent::AimPoint(FMath::Max(RangeCm, 500.f) + 600.f);
    // 02/11: magia (bolt e orb) sai da altura da mão, na frente do herói. A ponta do cajado em blocos às vezes era a de
    // baixo: a magia nascia rente ao chão e subia até a mira, e para acertar o bicho era preciso pular
    const bool bSpell = Mode == TEXT("bolt") || Mode == TEXT("orb");
    // 03/11: mira travada (Caps Lock com o cajado): a magia vai no peito do inimigo travado
    if (const AActor* Locked = GetLockTarget(); Locked && bSpell) Target = Locked->GetActorLocation() + FVector(0.f, 0.f, 20.f);
    const FVector Muzzle = bSpell ? Owner->GetActorLocation() + FVector(0.f, 0.f, 55.f) + Owner->GetActorForwardVector() * 55.f : MuzzleLocation();
    FVector Direction = (Target - Muzzle).GetSafeNormal();
    // 08/11: o alvo ficou atrás da arma: vale a direção da mira, não a do corpo (pulando de lado o corpo olha para outro lado, e a flecha
    // saía ao contrário). O alvo muda junto, porque o cálculo da queda da flecha usa ele
    const FVector AimAhead = Owner->GetControlRotation().Vector();
    if (Direction.IsNearlyZero() || FVector::DotProduct(Direction.GetSafeNormal2D(), AimAhead.GetSafeNormal2D()) < -.2f)
    {
        Direction = AimAhead;
        Target = Muzzle + AimAhead * FMath::Max(RangeCm, 500.f);
    }
    float Drop = 1.f;
    if (Mode == TEXT("arrow"))
    {
        // flecha cai com a gravidade: sobe o tanto certo para descer no ponto da mira. 03/11, arco (ArrowCharge 0..1): pouco
        // puxado a flecha cai bem mais e sai reto para a mira (cai antes dela); puxado até o fim, quase reta e certeira
        const float C = ArrowCharge < 0.f ? 1.f : FMath::Clamp(ArrowCharge, 0.f, 1.f);
        Drop = ArrowCharge < 0.f ? 1.f : FMath::Lerp(2.6f, .45f, C);
        const FVector Ballistic = AVortexProjectile::BallisticDirection(Muzzle, Target, FMath::Max(100.f, SpeedCm), Direction, Drop);
        Direction = ArrowCharge < 0.f ? Ballistic : FMath::Lerp(Direction, Ballistic, C * C).GetSafeNormal();
    }
    FVector Spawn = Muzzle;
    // inimigo colado (entre o peito e a ponta da arma): o tiro nascia já depois dele e passava reto; sai do peito
    // direto no inimigo
    if (Mode != TEXT("burst") && Owner->GetCombat())
    {
        TArray<FHitResult> Close;
        FCollisionObjectQueryParams Pawns;
        Pawns.AddObjectTypesToQuery(ECC_Pawn);
        FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexPointBlank), false, Owner);
        const FVector Reach = Muzzle + (Muzzle - Chest).GetSafeNormal() * 40.f;
        GetWorld()->SweepMultiByObjectType(Close, Chest, Reach, FQuat::Identity, Pawns, FCollisionShape::MakeSphere(30.f), Query);
        for (const FHitResult& Hit : Close)
        {
            AActor* Foe = Hit.GetActor();
            if (!Foe || !Owner->GetCombat()->CanHarm(Foe)) continue;
            Spawn = Chest;
            Direction = (Foe->GetActorLocation() - Chest).GetSafeNormal();
            if (Direction.IsNearlyZero()) Direction = Owner->GetActorForwardVector();
            break;
        }
    }
    if (Mode == TEXT("burst"))
    {
        // no ponto da mira (até o alcance) ou em volta do herói (alcance 0)
        Spawn = RangeCm <= 0.f ? Owner->GetActorLocation() - FVector(0.f, 0.f, 60.f) + Owner->GetActorForwardVector() * 60.f
            : Chest + (Target - Chest).GetClampedToMaxSize(RangeCm);
        if (AimPoint) *AimPoint = Spawn;
    }
    if (VortexVolley::Yaw != 0.f) Direction = Direction.RotateAngleAxis(VortexVolley::Yaw, FVector::UpVector);
    const FTransform Where(Direction.Rotation(), Spawn);
    AVortexProjectile* Shot = GetWorld()->SpawnActorDeferred<AVortexProjectile>(AVortexProjectile::StaticClass(), Where, Owner, Owner,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Shot) return nullptr;
    Shot->Mode = Mode;
    Shot->Element = Element;
    Shot->Damage = Damage;
    Shot->Speed = FMath::Max(100.f, SpeedCm);
    Shot->Range = FMath::Max(100.f, RangeCm);
    Shot->Drop = Drop;
    Shot->Radius = Mode == TEXT("burst") ? FMath::Max(RadiusCm, 150.f) : RadiusCm;
    Shot->Size = Size;
    Shot->bPierce = bPierce;
    Shot->Tags.Add(TEXT("arma"));   // 07/11: o golpe da arma multiplica o dano do herói (1 + bônus) x o dano do golpe, como os outros jogos (valor do movimento)
    Shot->FinishSpawning(Where);
    return Shot;
}

FVector UVortexWeaponComponent::AimPoint(float MaxDistance) const
{
    const AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    if (!Owner) return FVector::ZeroVector;
    FHitResult Aim;
    return Owner->TraceAim(Aim, MaxDistance) ? FVector(Aim.ImpactPoint) : FVector(Aim.TraceEnd);
}

AActor* UVortexWeaponComponent::AimTarget(float MaxDistance) const
{
    const AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    if (!Owner || !Owner->GetCombat()) return nullptr;
    // 03/11: mira travada: o inimigo travado (as vinhas e o resto que procuram o alvo vão nele)
    if (AActor* Locked = GetLockTarget(); Locked && FVector::Dist(Locked->GetActorLocation(), Owner->GetActorLocation()) <= MaxDistance + 100.f) return Locked;
    FHitResult Aim;
    Owner->TraceAim(Aim, MaxDistance + 900.f);
    const FVector From = Aim.TraceStart, To = Aim.bBlockingHit ? FVector(Aim.ImpactPoint) : FVector(Aim.TraceEnd);
    // o raio da mira não para nos bichos (o canal de visão passa por eles): uma esfera pelo mesmo caminho
    TArray<FHitResult> Hits;
    FCollisionObjectQueryParams Pawns;
    Pawns.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexAimTarget), false, Owner);
    GetWorld()->SweepMultiByObjectType(Hits, From, To + (To - From).GetSafeNormal() * 150.f, FQuat::Identity, Pawns, FCollisionShape::MakeSphere(70.f), Query);
    for (const FHitResult& Hit : Hits)
    {
        AActor* Actor = Hit.GetActor();
        if (Actor && Owner->GetCombat()->CanHarm(Actor) && FVector::Dist(Actor->GetActorLocation(), Owner->GetActorLocation()) <= MaxDistance + 100.f) return Actor;
    }
    return nullptr;
}

FVector UVortexWeaponComponent::MuzzleLocation() const
{
    const AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    if (!Owner) return FVector::ZeroVector;
    const FVector Chest = Owner->GetActorLocation() + FVector(0.f, 0.f, 35.f) + Owner->GetActorForwardVector() * 40.f;
    const UStaticMeshComponent* Visual = Owner->GetWeaponVisual();
    const UStaticMesh* Mesh = Visual ? Visual->GetStaticMesh() : nullptr;
    if (!Mesh || !EquippedDefinition) return Chest;
    const VortexWeaponKit::FKit* Kit = VortexWeaponKit::Find(EquippedDefinition->WeaponId);
    const VortexWeaponKit::EMuzzle Kind = Kit ? Kit->Muzzle : VortexWeaponKit::EMuzzle::Tip;
    // a malha foi feita na mão (a pegada é a origem): a ponta é o lado do eixo mais comprido que vai mais longe
    const FBox Box = Mesh->GetBoundingBox();
    FVector Local = FVector::ZeroVector;
    if (Kind == VortexWeaponKit::EMuzzle::Body) Local = Box.GetCenter();
    else if (Kind == VortexWeaponKit::EMuzzle::Tip)
    {
        int32 Axis = 0;
        float Best = -1.f;
        for (int32 A = 0; A < 3; ++A)
        {
            const float Far = FMath::Max(FMath::Abs(Box.Min[A]), FMath::Abs(Box.Max[A]));
            if (Far > Best) { Best = Far; Axis = A; }
        }
        Local = Box.GetCenter();
        Local[Axis] = FMath::Abs(Box.Max[Axis]) >= FMath::Abs(Box.Min[Axis]) ? Box.Max[Axis] : Box.Min[Axis];
    }
    const FVector World = Visual->GetComponentTransform().TransformPosition(Local);
    // pose ainda não avaliada (ou a arma longe da mão): do peito
    return FVector::Dist(World, Owner->GetActorLocation()) > 260.f ? Chest : World;
}

UVortexWeaponDefinition* UVortexWeaponComponent::LoadType(FName TypeId)
{
    if (TypeId.IsNone() || TypeId == TEXT("espada_de_cinzas"))
    {
        return LoadObject<UVortexWeaponDefinition>(nullptr, TEXT("/Game/Vortex/Weapons/Sword/DA_DefaultSword.DA_DefaultSword"));
    }
    const FString Name = TEXT("DA_") + TypeId.ToString();
    return LoadObject<UVortexWeaponDefinition>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Weapons/Types/%s.%s"), *Name, *Name));
}

void UVortexWeaponComponent::ReleaseSpecial()
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    if (!Owner || !EquippedDefinition || !bSpecialActive) return;
    // armas do Godot cujo especial ainda não chegou: um golpe forte da própria arma (o mais forte
    // do combo, maior e com o dobro do dano)
    if (!EquippedDefinition->SecondaryKind.IsNone() && EquippedDefinition->SecondaryKind != TEXT("parry") &&
        EquippedDefinition->Swings.IsValidIndex(SpecialSwing))
    {
        const FVortexWeaponSwing& Strong = EquippedDefinition->Swings[SpecialSwing];
        if (Strong.Mode == TEXT("melee"))
        {
            TSet<TWeakObjectPtr<AActor>> Struck;
            Owner->GetCombat()->StrikeInArc(1.f, Strong.ReachCm * 1.25f, Strong.MinForwardDot - .2f, Struck, 0.f, EquippedDefinition->SpecialDamage);
        }
        else if (Strong.Mode == TEXT("slam"))
        {
            LaunchProjectile(TEXT("burst"), TEXT("steel"), EquippedDefinition->SpecialDamage, 0.f, 0.f, Strong.RadiusCm * 1.3f, 1.4f);
        }
        else
        {
            LaunchProjectile(Strong.Mode, Strong.Element, EquippedDefinition->SpecialDamage, Strong.ProjectileSpeed * 1.2f, Strong.ReachCm,
                Strong.RadiusCm * 1.3f, 1.5f);
        }
        VortexAudio::Play(this, TEXT("skill_slash"), Owner->GetActorLocation(), .8f);
        return;
    }
    // para onde a mira aponta (no plano), saindo da altura do peito
    FVector Forward = Owner->GetActorForwardVector().GetSafeNormal2D();
    FHitResult Aim;
    const bool bAim = Owner->TraceAim(Aim);
    const FVector From = Owner->GetActorLocation() + FVector(0, 0, 20.f);
    const FVector AimDirection = ((bAim ? Aim.ImpactPoint : Aim.TraceEnd) - From).GetSafeNormal2D();
    if (!AimDirection.IsNearlyZero() && FVector::DotProduct(AimDirection, Forward) > -.2f) Forward = AimDirection;
    UClass* Projectile = EquippedDefinition->SpecialProjectile ? EquippedDefinition->SpecialProjectile.Get() : AVortexSlashWave::StaticClass();
    FActorSpawnParameters Params;
    Params.Owner = Owner;
    Params.Instigator = Owner;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* Spawned = GetWorld()->SpawnActor<AActor>(Projectile, From + Forward * 80.f, Forward.Rotation(), Params);
    if (AVortexSlashWave* Wave = Cast<AVortexSlashWave>(Spawned)) Wave->Damage = EquippedDefinition->SpecialDamage;
    if (Spawned) Spawned->Tags.Add(TEXT("arma"));
    UE_LOG(LogTemp, Display, TEXT("VORTEX_SPECIAL_RELEASE %s dano=%.0f"), *GetNameSafe(Spawned), EquippedDefinition->SpecialDamage);
}

bool UVortexWeaponComponent::HasSeriousPunch() const
{
    return EquippedDefinition && EquippedDefinition->WeaponId == TEXT("manoplas_de_ferro");
}

void UVortexWeaponComponent::BeginSeriousPunch()
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    UAbilitySystemComponent* ASC = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
    if (!ASC) return;
    if (ASC->GetNumericAttribute(UVortexAttributes::GetManaAttribute()) < SeriousPunchMana)
    {
        if (UVortexHeroSheet* Sheet = Owner->GetSheet()) Sheet->PushNotice(TEXT("Mana insuficiente para o Soco Sério"));
        return;
    }
    ASC->ApplyModToAttribute(UVortexAttributes::GetManaAttribute(), EGameplayModOp::Additive, -SeriousPunchMana);
    // pose de carga: o punho recolhido junta força por meio segundo (todos veem pelo AttackState)
    bSpecialActive = true;
    bCharging = true;
    ChargeStartServerTime = GetWorld()->GetTimeSeconds();
    ++AttackState.Serial;
    AttackState.SwingIndex = VortexSpecialPose::Hold;
    AttackState.ServerStartTime = ChargeStartServerTime;
    const AGameStateBase* State = GetWorld()->GetGameState();
    const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    SpecialReadyServerTime = float(Now + SeriousPunchCooldown * GetCooldownScale());
    Owner->ForceNetUpdate();
    VortexAudio::Play(this, TEXT("wind_cast"), Owner->GetActorLocation(), .9f);
    GetWorld()->GetTimerManager().SetTimer(SpecialReleaseTimer, this, &UVortexWeaponComponent::ReleaseSeriousPunch, .55f, false);
}

void UVortexWeaponComponent::ReleaseSeriousPunch()
{
    if (!bSpecialActive) return;
    bCharging = false;
    ++AttackState.Serial;
    AttackState.SwingIndex = VortexSpecialPose::Release;
    AttackState.ServerStartTime = GetWorld()->GetTimeSeconds();
    GetOwner()->ForceNetUpdate();
    FTimerManager& Timers = GetWorld()->GetTimerManager();
    Timers.SetTimer(LungeTimer, this, &UVortexWeaponComponent::FireSeriousPunch, .1f, false);
    Timers.SetTimer(SpecialFinishTimer, this, &UVortexWeaponComponent::FinishSpecial, .7f, false);
}

void UVortexWeaponComponent::FireSeriousPunch()
{
    AVortexCharacter* Owner = Cast<AVortexCharacter>(GetOwner());
    if (!Owner || !EquippedDefinition) return;
    // para onde a mira aponta (no plano); de costas para a mira, sai para a frente do herói
    FVector Forward = Owner->GetActorForwardVector().GetSafeNormal2D();
    FHitResult Aim;
    const bool bAim = Owner->TraceAim(Aim, 4000.f);
    const FVector From = Owner->GetActorLocation() + FVector(0.f, 0.f, 30.f);
    const FVector AimDirection = ((bAim ? Aim.ImpactPoint : Aim.TraceEnd) - From).GetSafeNormal2D();
    if (!AimDirection.IsNearlyZero() && FVector::DotProduct(AimDirection, Forward) > -.3f) Forward = AimDirection;
    Owner->SetActorRotation(Forward.Rotation());
    const FTransform Where(Forward.Rotation(), From + Forward * 90.f);
    AVortexSeriousPunch* Punch = GetWorld()->SpawnActorDeferred<AVortexSeriousPunch>(AVortexSeriousPunch::StaticClass(), Where, Owner, Owner,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Punch) return;
    Punch->Damage = 18.f;
    Punch->FistMesh = EquippedDefinition->VisualMesh;
    Punch->FinishSpawning(Where);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_SERIOUS_PUNCH dir=%s"), *Forward.ToCompactString());
}

void UVortexWeaponComponent::FinishSpecial()
{
    bSpecialActive = false;
    bCharging = false;
    AttackState.SwingIndex = INDEX_NONE;
    ++AttackState.Serial;
    LastSwingIndex = INDEX_NONE;   // o próximo clique esquerdo começa o combo do início
    if (GetOwner()) GetOwner()->ForceNetUpdate();
}
