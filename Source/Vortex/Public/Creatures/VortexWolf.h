#pragma once

#include "AbilitySystemInterface.h"
#include "Creatures/VortexCreature.h"
#include "Creatures/VortexSteering.h"
#include "GameFramework/Character.h"
#include "VortexWolf.generated.h"

class UAbilitySystemComponent;
class UVortexAttributes;
class UVortexCombatComponent;
class AVortexCharacter;
class UVortexGroupCoordinator;
class UBoxComponent;
struct FOnAttributeChangeData;

UENUM(BlueprintType)
enum class EVortexWolfState : uint8
{
    Idle, Observe, Howl, Chase, Circle, Bite, Pounce, Retreat, ReturnHome, Hit, Dead,
    // 10/11: os especiais do Lobo Ametista (Salto de Ametista: empina e salta; Mordida Dupla: a segunda mordida)
    Rear, Combo
};

/** First hostile creature. Decisions and damage run only on the server at 10 Hz. */
UCLASS()
class VORTEX_API AVortexWolf : public ACharacter, public IAbilitySystemInterface, public IVortexCreature
{
    GENERATED_BODY()
public:
    // IVortexCreature
    virtual FName GetSpecies() const override { return TEXT("Wolf"); }
    virtual int32 GetCreatureLevel() const override { return Level; }
    virtual FString GetCreatureTitle() const override { return bAlpha ? TEXT("ALFA · LOBO DA BRUMA") : TEXT("LOBO DA BRUMA"); }
    virtual bool IsElite() const override { return bAlpha; }
    virtual FString GetCreatureAction() const override;
    virtual float GetHealthFraction() const override;
    virtual bool IsCreatureDead() const override { return WolfState == EVortexWolfState::Dead; }
    virtual bool IsEngaged() const override;
    AVortexWolf();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

    UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category="Vortex|Wolf", meta=(ClampMin="1", ClampMax="100"))
    int32 Level = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vortex|Pack")
    int32 PackId = -1;
    /** O alfa da matilha de 3: 1,35 vez maior, 2,2 vezes a vida e mordida 1,5 vez mais forte. */
    UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category="Vortex|Pack")
    bool bAlpha = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vortex|Pack", meta=(ClampMin="0"))
    int32 PackSlot = 0;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Vortex|Wolf")
    EVortexWolfState WolfState = EVortexWolfState::Idle;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Vortex|Wolf")
    float ActionServerTime = 0.f;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Vortex|Wolf")
    int32 ActionSerial = 0;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Vortex|Wolf")
    bool bFrenzied = false;
    UPROPERTY(ReplicatedUsing=OnRep_TerrainReady)
    bool bTerrainReady = false;

    float Health() const;
    float Damage() const { return AttackDamage; }
    const FVector& Home() const { return HomeLocation; }
    void SetHome(const FVector& Location) { HomeLocation = Location; }
    void SetPack(int32 NewId, int32 NewSlot);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    /** Névoa roxa de corrupção nas patas (só desenho). */
    TWeakObjectPtr<class UNiagaraComponent> WolfMist;
    UPROPERTY(VisibleAnywhere, Category="Vortex|Wolf")
    TObjectPtr<UAbilitySystemComponent> AbilitySystem;
    UPROPERTY(VisibleAnywhere, Category="Vortex|Wolf")
    TObjectPtr<UVortexAttributes> Vitals;
    UPROPERTY(VisibleAnywhere, Category="Vortex|Wolf")
    TObjectPtr<UVortexCombatComponent> Combat;
    UPROPERTY(VisibleAnywhere, Category="Vortex|Wolf")
    TObjectPtr<UBoxComponent> BodyHitbox;
    UPROPERTY(Transient)
    TWeakObjectPtr<AVortexCharacter> Target;
    FVector HomeLocation = FVector::ZeroVector;
    FTimerHandle ThinkHandle, ImpactHandle;
    FTimerHandle TerrainReadyHandle;
    float AttackDamage = 14.f;
    float NextAttackTime = 0.f;
    float RetreatUntil = 0.f;
    float FrenzyUntil = 0.f;
    float LastHealth = 0.f;
    // sons (em todas as máquinas que desenham): uivo, rosnado, bote e passos
    void UpdateSounds(float DeltaSeconds);
    EVortexWolfState HeardState = EVortexWolfState::Idle;
    float WolfStepTimer = 0.f, GrowlCooldown = 0.f;
    int32 WolfStepVariant = 0;
    bool bHasHowled = false;
    // 10/11: os especiais (Salto de Ametista com recarga, a Explosão de Cristais do alfa ferido, uma vez)
    float NextLeapTime = 0.f;
    bool bCrystalBurst = false;
    FVector LeapTo = FVector::ZeroVector;
    void StartLeap(AVortexCharacter* Victim);
    // matilha (Think): esquiva do golpe do herói, tempo esperando a vez, fuga da matilha quebrada
    float NextEvadeTime = 0.f, LastAttackEnd = 0.f, BrokenFleeUntil = 0.f, FlinchReadyAt = 0.f, NextFeintTime = 0.f;
    // fúria: golpes seguidos (ver AVortexMonster::HitStreak)
    int32 HitStreak = 0;
    float LastHitAt = -10.f;
    int32 SeenHeroSwing = 0;
    bool bBrokenFled = false;
    bool bRetreatedLowHealth = false;
    bool bUsesTerrainStreamer = false;
    bool bLocalTerrainReady = false;
    bool bPresentationReadyLogged = false;
    VortexSteering::FMemory Steer;
    // Decisão do Think (10 Hz) aplicada a cada quadro como entrada de movimento: o CharacterMovement
    // acelera e freia suave, e a velocidade real escolhe Walk/Run na animação.
    FVector MoveDirection = FVector::ZeroVector;
    float MoveSpeed = 0.f;

    void Think();
    void OnHealthChanged(const FOnAttributeChangeData& Data);
    void SetState(EVortexWolfState NewState);
    bool StartAttack(EVortexWolfState Attack);
    void ResolveAttack(EVortexWolfState Attack, int32 Serial);
    bool CanSee(const AVortexCharacter* Candidate) const;
    void MoveToward(const FVector& Location, float Speed);
    void StopMoving();
    UVortexGroupCoordinator* Coordinator() const;
    void TryActivateTerrain();
    void RefreshTerrainPresentation();
    UFUNCTION()
    void OnRep_TerrainReady();
};
