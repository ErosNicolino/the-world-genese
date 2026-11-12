#pragma once

#include "Components/ActorComponent.h"
#include "GameplayAbilitySpec.h"
#include "VortexWeaponComponent.generated.h"

namespace VortexWeaponKit { struct FKit; }

class UVortexWeaponDefinition;
class AVortexProjectile;
struct FVortexWeaponSwing;
class UVortexSwordAbility;
class UVortexGuardAbility;

/** Golpes do especial na animação (AttackState.SwingIndex): segurando a carga e soltando. */
namespace VortexSpecialPose
{
    constexpr int32 Hold = -2;
    constexpr int32 Release = -3;
}

USTRUCT()
struct FVortexAttackState
{
    GENERATED_BODY()
    UPROPERTY() int32 Serial = 0;
    UPROPERTY() int32 SwingIndex = INDEX_NONE;
    UPROPERTY() float ServerStartTime = 0.f;
};

/** Equipped data and runtime combo state; the ability is granted/removed on the server. */
UCLASS(ClassGroup=(Vortex), meta=(BlueprintSpawnableComponent))
class VORTEX_API UVortexWeaponComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UVortexWeaponComponent();
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category="Vortex|Weapon")
    void PressPrimary();
    /** Especial da arma (botão direito): com recarga; a espada solta o corte à distância. */
    void PressSpecial();
    /** Botão direito solto: o especial carregado (charge) dispara com a força acumulada. */
    void ReleaseSpecialInput();
    /** Carga do especial agora (0..1), para o HUD. */
    float SpecialCharge() const;
    /** Arma do Godot por tipo (DA em /Game/Vortex/Weapons/Types; a espada é o DA_DefaultSword). */
    static UVortexWeaponDefinition* LoadType(FName TypeId);
    /** O osso onde a arma vai: o da definição, menos o arco nos heróis do Mixamo (Bow_L, na mão esquerda: lá a esquerda
     *  segura o arco e a direita puxa a corda; Tools/Blender/MixamoToHero.py add_bow_grip). */
    static FName HandBoneFor(const UVortexWeaponDefinition* Definition, const class USkeletalMeshComponent* Mesh);
    /** O giro da arma no osso: no Bow_L a Unreal lê os eixos do osso com o do comprimento trocado pelo da corda (o
     *  arco deitava como um rifle); o resto vai sem giro. */
    static FQuat GripRotationFor(FName Bone);
    /** Refaz a arma na mão (o herói trocou de corpo: o osso da pegada pode ser outro). */
    void RefreshVisual() { ApplyVisual(); }
    /** Projétil de um golpe à distância, saindo da arma para o ponto da mira. */
    AVortexProjectile* LaunchProjectile(FName Mode, FName Element, float Damage, float SpeedCm, float RangeCm, float RadiusCm,
        float Size = 1.f, bool bPierce = false, FVector* AimPoint = nullptr, float ArrowCharge = -1.f);
    /** G8: onde a arma na mão dispara (ponta, mão ou corpo da arma, pelo VortexWeaponKit). */
    FVector MuzzleLocation() const;
    /** G9: o ponto que a mira enxerga (raio da câmera até onde bate, ou o fim do raio). */
    FVector AimPoint(float MaxDistance) const;
    /** O inimigo em que a mira está (ou passa raspando, ~70 cm), até MaxDistance do herói; senão nada. */
    AActor* AimTarget(float MaxDistance) const;
    bool IsSpecialActive() const { return bSpecialActive; }
    /** Segundos até o especial voltar (em qualquer máquina, pelo relógio do servidor). */
    float SpecialCooldownRemaining() const;
    float SpecialCooldownTotal() const;
    void SetGuardPressed(bool bPressed);
    float ResolveGuardDamage(const AActor* Attacker, float Incoming, bool& bPerfectParry) const;
    bool IsGuarding() const { return bGuarding; }
    /** Velocidade de ataque (golpes e animações) e escala das recargas (Pressa), da ficha do herói. */
    void SetCombatPace(float Speed, float Cooldowns);
    float GetAttackSpeed() const { return FMath::Clamp(AttackSpeed, .5f, 2.f); }
    /** Alcance de verdade de um golpe de perto (cm, do centro do herói até a borda do alvo): o corpo, o braço
     *  e o comprimento do modelo da arma, com uma folga. Nunca passa do alcance do golpe (ReachCm). */
    float MeleeReach(const struct FVortexWeaponSwing& Swing) const;
    /** O cone à frente em que o golpe acerta (o MinForwardDot do golpe; a estocada da lança, mais largo). */
    float MeleeCone(const struct FVortexWeaponSwing& Swing) const;
    /** Lança e alabarda: a estocada estica o braço e acerta com a ponta. */
    static bool IsPolearm(const VortexWeaponKit::FKit* Kit);
    float GetCooldownScale() const { return FMath::Clamp(CooldownScale, .2f, 1.f); }
    void RegisterGuardStart(UVortexGuardAbility* Ability);
    void RegisterGuardEnd(UVortexGuardAbility* Ability);
    bool Equip(UVortexWeaponDefinition* NewDefinition);
    const UVortexWeaponDefinition* GetDefinition() const { return EquippedDefinition; }
    const FVortexAttackState& GetAttackState() const { return AttackState; }
    /** Manoplas: o especial é o Soco Sério (punho gigante à distância), não o contra-golpe do Godot. */
    bool HasSeriousPunch() const;
    static constexpr float SeriousPunchMana = 20.f;
    static constexpr float SeriousPunchCooldown = 9.f;
    int32 NextSwingIndex() const;

    // ---- revisão das armas (VortexWeaponSpecials.cpp) ----------------------------------------
    /** Dano do primeiro golpe normal (os secundários são múltiplos dele). */
    float PrimaryDamage() const;
    /** Botão esquerdo solto (o arco carrega enquanto segura). */
    void ReleasePrimary();
    /** Golpes normais com jeito próprio (tornado do grimório, garrafa, chakram que volta). true = já saiu. */
    bool FireKitPrimary(int32 SwingIndex);
    /** Machados arremessados / chakram fora da mão: bits 1 = direita, 2 = esquerda (todos veem a mão vazia). */
    uint8 GetThrowMask() const { return ThrowMask; }
    /** Arco: segurando a corda (0..1 da carga), para a HUD. */
    float DrawCharge() const;
    /** 03/11: o arco fica em pé na mão quando não está atirando (a mão do Arco_Draw é em pé; parado e andando ela deita). */
    void TickBowGrip(float DeltaSeconds);
    /** 03/11: cajado: o Caps Lock trava a mira no inimigo da mira (os tiros e as magias vão nele) e destrava. */
    bool UsesLock() const;
    void ToggleLock();
    AActor* GetLockTarget() const;
    /** 03/11: a cura do cajado vai no aliado (outro herói) da mira, até Range; nenhum = o próprio herói e o grupo. */
    class AVortexCharacter* HealTarget(float Range) const;
    /** Testes: secundário pronto de novo e as armas arremessadas de volta na mão. */
    void DebugResetSpecial() { SpecialReadyServerTime = 0.f; ThrowMask = 0; }
    void RegisterAttackStart(UVortexSwordAbility* Ability, int32 Index);
    void RegisterAttackEnd(UVortexSwordAbility* Ability, int32 Index);

private:
    UFUNCTION(Server, Reliable)
    void ServerReleasePrimary();
    void ReleasePrimaryOnServer();
    bool BeginKitSpecial();
    /** Som de um secundário para todos (as máquinas que desenham). */
    void KitSound(FName Sound, const FVector& At, float Volume = 1.f, float Jitter = .06f);
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastKitSound(FName Sound, FVector_NetQuantize At, float Volume, float Jitter);
    void ReleaseKitSpecial();
    /** O clipe do secundário (Release) ou o de segurar (Hold) para todos, e o fim do especial depois de Seconds. */
    void KitPose(int32 Pose, float Seconds);
    void StartKitCooldown(float Seconds);
    bool SpendMana(float Amount, const TCHAR* What);
    void Later(float Seconds, TFunction<void()> Run);
    void TickSong();
    void EndSong();
    void BowFire();
    UFUNCTION(Server, Reliable)
    void ServerSetLock(AActor* Target);
    UPROPERTY(Replicated)
    TObjectPtr<AActor> LockTarget;
    float BowUpright = 0.f;
    UFUNCTION()
    void OnRep_ThrowMask();
    UPROPERTY(ReplicatedUsing=OnRep_ThrowMask)
    uint8 ThrowMask = 0;
    UPROPERTY(Replicated)
    float DrawStartServerTime = 0.f;
    UPROPERTY(Replicated)
    bool bDrawing = false;
    /** Arco: a corda precisa esticar pelo menos isso, e entre uma flecha e a outra há um intervalo. */
    static constexpr float BowMinDraw = .3f;
    static constexpr float BowGap = .45f;
    float DrawLocalStart = 0.f, BowReadyTime = 0.f;
    bool bSinging = false;
    float SongStart = 0.f;
    int32 ChakramHand = 0;
    float BookTornadoReadyTime = 0.f;
    TWeakObjectPtr<class AVortexWeaponFx> SongAura;
    FTimerHandle SongTimer, DrawTimer;
    TArray<FTimerHandle> KitTimers;
    TWeakObjectPtr<AActor> VineTarget;
    bool bCasting = false;
    UFUNCTION(Server, Reliable)
    void ServerPressPrimary();
    UFUNCTION(Server, Reliable)
    void ServerSetGuardPressed(bool bPressed);
    UFUNCTION(Server, Reliable)
    void ServerPressSpecial();
    UFUNCTION(Server, Reliable)
    void ServerReleaseSpecial();
    void ReleaseSpecialOnServer();
    void FinishCharge();
    void ChargeEffect(float Charge);
    UPROPERTY(Replicated)
    float ChargeStartServerTime = 0.f;
    UPROPERTY(Replicated)
    float AttackSpeed = 1.f;
    float CooldownScale = 1.f;
    UPROPERTY(Replicated)
    bool bCharging = false;
    FTimerHandle LungeTimer;
    int32 LungeTicks = 0;
    float LungeDamage = 0.f;
    TSet<TWeakObjectPtr<AActor>> LungeHits;
    void PressSpecialOnServer();
    void ReleaseSpecial();
    void FinishSpecial();
    UFUNCTION()
    void OnRep_EquippedDefinition();
    void ApplyVisual();
    void PressPrimaryOnServer();
    void SetGuardPressedOnServer(bool bPressed);

    UPROPERTY(ReplicatedUsing=OnRep_EquippedDefinition)
    TObjectPtr<UVortexWeaponDefinition> EquippedDefinition;
    UPROPERTY(Replicated)
    FVortexAttackState AttackState;
    UPROPERTY(Replicated)
    bool bGuarding = false;
    UPROPERTY(Replicated)
    float GuardStartServerTime = 0.f;
    UPROPERTY(Replicated)
    float SpecialReadyServerTime = 0.f;
    UPROPERTY(Replicated)
    bool bSpecialActive = false;
    int32 SpecialSwing = INDEX_NONE;
    FTimerHandle SpecialReleaseTimer, SpecialFinishTimer;
    UPROPERTY(Transient)
    TObjectPtr<UVortexSwordAbility> ActiveAbility;
    UPROPERTY(Transient)
    TObjectPtr<UVortexGuardAbility> ActiveGuardAbility;
    FGameplayAbilitySpecHandle GrantedAbility;
    FGameplayAbilitySpecHandle GrantedGuardAbility;
    int32 LastSwingIndex = INDEX_NONE;
    float LastSwingEndTime = -1000.f;
    // Soco Sério: junta força (pose de carga), solta e lança o punho
    void BeginSeriousPunch();
    void ReleaseSeriousPunch();
    void FireSeriousPunch();
};
