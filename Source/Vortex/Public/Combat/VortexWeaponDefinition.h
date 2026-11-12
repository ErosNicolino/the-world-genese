#pragma once

#include "Engine/DataAsset.h"
#include "VortexWeaponDefinition.generated.h"

class UAnimSequence;
class UStaticMesh;
class UGameplayAbility;

USTRUCT(BlueprintType)
struct FVortexWeaponSwing
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Animation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Duration = 0.62f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float ActiveStart = 0.2f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float ActiveEnd = 0.32f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Damage = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float ReachCm = 260.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float MinForwardDot = 0.3f;

    // Armas do Godot (WeaponStyles / weapon_types.json): como o golpe acerta.
    //   melee: no alcance e no cone à frente durante a janela; slam: pancada no chão, todos no raio;
    //   arrow, bolt, orb: projétil que voa até ReachCm (orb explode no raio); burst: explosão num
    //   ponto ReachCm à frente.
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Mode = TEXT("melee");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float RadiusCm = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float ProjectileSpeed = 2400.f;   // cm/s
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float PlayRate = 1.f;             // velocidade do clipe
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Stun = 0.f;                 // segundos
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Element;                    // cor da magia/flecha
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Sound;
};

/** Authorable weapon data: appearance, ability set and animation/timing set. */
UCLASS(BlueprintType)
class VORTEX_API UVortexWeaponDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FName WeaponId;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UStaticMesh> VisualMesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TSubclassOf<UGameplayAbility> PrimaryAbility;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TSubclassOf<UGameplayAbility> SecondaryAbility;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UAnimSequence> GuardAnimation;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TArray<FVortexWeaponSwing> Swings;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float ChainGraceSeconds = 0.48f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float BufferOpenFraction = 0.24f;

    // Especial (botão direito; toda arma tem um): usa a animação de um golpe do combo (-1 = o
    // último, o mais forte) e no instante SpecialRelease solta o projétil (espada: o corte à distância).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Special") int32 SpecialSwingIndex = -1;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Special") float SpecialRelease = 0.24f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Special") float SpecialDamage = 16.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Special") float SpecialCooldown = 3.5f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Special") TSubclassOf<AActor> SpecialProjectile;

    // ---- armas do Godot (Tools/ImportWeapons.py, Data/Rpg/weapons.json)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") FString DisplayName;
    /** Ala do Estádio do Despertar: marcial, distancia, arcana, exotica. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") FName Wing;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") bool bMagic = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") FString PrimaryText;
    /** Cópia espelhada na mão esquerda (armas de par), presa no osso Hand_L. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") TObjectPtr<UStaticMesh> LeftMesh;
    /** Osso onde a arma vai (o da espada é "Sword"; as outras, "Socket_Weapon_R"). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") FName HandBone = TEXT("Sword");
    /** Postura da família (vazios = os do aventureiro sem arma). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") TObjectPtr<UAnimSequence> StanceIdle;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") TObjectPtr<UAnimSequence> StanceWalk;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") TObjectPtr<UAnimSequence> StanceRun;

    // Especial do botão direito vindo do Godot. Kind: charge (segurar e soltar), e os outros que
    // chegam arma por arma. Shape do charge: shot, pierce, slam, lunge, cleave, circle, reap.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") FName SecondaryKind;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") FName SecondaryShape;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") FString SecondaryName;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") FString SecondaryText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") FName SecondaryIcon;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") TObjectPtr<UAnimSequence> SecondaryClip;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") TObjectPtr<UAnimSequence> SecondaryHoldClip;
    /** Números do especial (min, max, reach, radius, stun, cooldown, mana...) em unidades do jogo (cm, dano). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") TMap<FName, float> SecondaryValues;
    /** Os golpes e o especial já funcionam no Vortex (os outros ficam "em breve" no Estádio). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Godot") bool bPlayable = false;

    float Secondary(FName Key, float Default = 0.f) const { const float* V = SecondaryValues.Find(Key); return V ? *V : Default; }
    /** Arma de perto (todos os golpes do combo são corte ou pancada): bate no que está à frente, sem mira. */
    bool IsMelee() const
    {
        for (const FVortexWeaponSwing& Swing : Swings) if (Swing.Mode != TEXT("melee") && Swing.Mode != TEXT("slam")) return false;
        return Swings.Num() > 0;
    }
};
