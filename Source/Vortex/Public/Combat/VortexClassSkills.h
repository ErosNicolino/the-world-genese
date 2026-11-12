#pragma once

#include "Components/ActorComponent.h"
#include "Rpg/VortexRpg.h"
#include "VortexClassSkills.generated.h"

class AVortexCharacter;

/** Bênçãos e proteções ativas do herói (relógio do servidor; todos veem: auras, HUD, velocidade). */
USTRUCT()
struct FVortexSkillBuffs
{
    GENERATED_BODY()
    UPROPERTY() float DamageUntil = 0.f;
    UPROPERTY() float Damage = 0.f;
    UPROPERTY() float ArmorUntil = 0.f;
    UPROPERTY() float Armor = 0.f;
    UPROPERTY() float SpeedUntil = 0.f;
    UPROPERTY() float Speed = 0.f;
    UPROPERTY() float RegenUntil = 0.f;
    UPROPERTY() float Regen = 0.f;
    UPROPERTY() float ShieldUntil = 0.f;
    UPROPERTY() float Shield = 0.f;
    UPROPERTY() float DeathwardUntil = 0.f;
    UPROPERTY() float HasteUntil = 0.f;
    UPROPERTY() float Haste = 0.f;
    /** Elemento da última bênção (cor da aura). */
    UPROPERTY() FName Element;
};

/**
 * As 4 habilidades da classe do herói (teclas 1 a 4), como Main.ClassSkills.cs do Godot:
 * projétil, onda, corte, explosão em volta, golpe do céu, zona, investida, bênção, cura, escudo,
 * corrente, raio e invocação. O dono aperta; o servidor confere (classe, mana, recarga), aplica dano,
 * estados (lentidão, enfraquecer, expor, dano contínuo, empurrão) e bênçãos, e avisa todas as
 * máquinas para desenharem o efeito na cor do elemento.
 */
UCLASS(ClassGroup=(Vortex), meta=(BlueprintSpawnableComponent))
class VORTEX_API UVortexClassSkills : public UActorComponent
{
    GENERATED_BODY()
public:
    UVortexClassSkills();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Dono: tecla 1 a 4 (Slot 0 a 3). */
    void Press(int32 Slot);
    /** A habilidade do espaço (nenhuma antes do Despertar). */
    const FVortexClassSkill* Skill(int32 Slot) const;
    float CooldownLeft(int32 Slot) const;
    float CooldownTotal(int32 Slot) const;
    /** Mana que a habilidade custa e se dá para lançar agora (mana e recarga). */
    bool CanCast(int32 Slot) const;
    /** Nível em que o espaço libera: 10, 15, 20, 25 (a 1ª no Despertar, depois uma a cada 5 níveis). */
    static int32 UnlockLevel(int32 Slot) { return 10 + 5 * FMath::Max(0, Slot); }
    bool IsUnlocked(int32 Slot) const;

    // ---- bênçãos (qualquer máquina)
    float DamageBonus() const;
    float ArmorBonus() const;
    float SpeedBonus() const;
    float ShieldPoints() const;
    bool HasDeathward() const;
    float HasteBonus() const;
    /** Para a HUD: nome, cor e segundos que faltam de cada bênção ativa. */
    struct FActive { FString Label; FLinearColor Color; float Left; };
    TArray<FActive> ActiveBuffs() const;
    /** Servidor: o golpe que chega passa pelo escudo e pelo Último Fôlego. */
    float AbsorbIncoming(float Damage, float CurrentHealth);
    /** Servidor: aplica a bênção (classe do herói ou de um aliado perto). */
    void GiveBuff(const FVortexClassSkill& S, float Scale);

    /** Clipe de braço para a animação de lançar (por ~0,7 s depois do lançamento). */
    class UAnimSequence* CastAnimation(float& OutTime) const;
    /** O gesto do Godot (Cast, Shout, Pray, Throw, Stomp, Summon, Guard, Point) na variante da arma, se houver. */
    static class UAnimSequence* GestureClip(const FString& Gesture, FName WeaponType);
    static FLinearColor ElementColor(FName Element);
    /** Testes: todas prontas de novo. */
    void DebugResetCooldowns() { for (float& R : ReadyAt) R = 0.f; }

private:
    UFUNCTION(Server, Reliable)
    void ServerCast(uint8 Slot, FVector_NetQuantize Aim);
    UFUNCTION(NetMulticast, Reliable)
    void MulticastFx(uint8 Slot, FVector_NetQuantize Origin, FVector_NetQuantize Target, const TArray<FVector_NetQuantize>& Points);

    UPROPERTY(Replicated) TArray<float> ReadyAt;
    UPROPERTY(Replicated) FVortexSkillBuffs Buffs;
    UPROPERTY(Replicated) uint8 CastSlot = 255;
    UPROPERTY(Replicated) float CastServerTime = -100.f;

    struct FZone { FVortexClassSkill Skill; FVector Center; double Until = 0.0, NextTick = 0.0; float Power = 0.f, Scale = 1.f; };
    struct FSummon { FVortexClassSkill Skill; double Until = 0.0, NextTick = 0.0; float Power = 0.f, Scale = 1.f; };
    TArray<FZone> Zones;
    TArray<FSummon> Summons;
    float RegenTick = 0.f;

    AVortexCharacter* Hero() const;
    double ServerNow() const;
    float PowerOf(const FVortexClassSkill& S) const;
    float ScaleOf(const FVortexClassSkill& S) const;
    void Execute(int32 Slot, const FVortexClassSkill& S, const FVector& Aim);
    void Hit(const FVortexClassSkill& S, AActor* Target, float Power);
    void Area(const FVortexClassSkill& S, const FVector& Center, float RadiusCm, float Power, TSet<TWeakObjectPtr<AActor>>* Skip = nullptr);
    TArray<AActor*> Enemies(const FVector& Center, float RadiusCm) const;
    void Extras(const FVortexClassSkill& S);
    void Later(float Seconds, TFunction<void()> Run);
};

/** Desenho de uma habilidade (só visual, em cada máquina): raio, corrente, escudo, zona e espíritos. */
UENUM()
enum class EVortexSkillFxKind : uint8 { Beam, Chain, Shield, Zone, Spirits };

UCLASS()
class VORTEX_API AVortexSkillFx : public AActor
{
    GENERATED_BODY()
public:
    AVortexSkillFx();
    virtual void Tick(float DeltaSeconds) override;
    void Start(EVortexSkillFxKind InKind, const FLinearColor& InColor, float InLife, const TArray<FVector>& InPoints, AActor* InFollow = nullptr, float InSize = 100.f, int32 InCount = 1);

private:
    UPROPERTY(Transient) TArray<TObjectPtr<class UStaticMeshComponent>> Parts;
    UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> Glow;
    EVortexSkillFxKind Kind = EVortexSkillFxKind::Beam;
    TArray<FVector> Points;
    TWeakObjectPtr<AActor> Follow;
    FLinearColor Color = FLinearColor::White;
    float Life = 1.f, Age = 0.f, Size = 100.f;
    int32 Count = 1;
    class UStaticMeshComponent* AddPart(const TCHAR* Shape);
};
