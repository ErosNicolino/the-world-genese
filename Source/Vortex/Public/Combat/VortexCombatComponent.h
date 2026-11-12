#pragma once

#include "Components/ActorComponent.h"
#include "Rpg/VortexRpg.h"
#include "VortexCombatComponent.generated.h"

UENUM(BlueprintType)
enum class EVortexFaction : uint8 { Player, Hostile, Neutral };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FVortexCombatResult, AActor*, InstigatorActor, AActor*, TargetActor, float, Damage);
/** Servidor: a criatura foi atordoada (aparo perfeito ou golpe que atordoa) por tantos segundos. */
DECLARE_MULTICAST_DELEGATE_OneParam(FVortexStunned, float);

/** Server-side validation and damage. Attack abilities own timing and call this component. */
UCLASS(ClassGroup=(Vortex), meta=(BlueprintSpawnableComponent))
class VORTEX_API UVortexCombatComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UVortexCombatComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vortex|Combat")
    EVortexFaction Faction = EVortexFaction::Player;

    UPROPERTY(BlueprintAssignable, Category="Vortex|Combat")
    FVortexCombatResult OnConfirmedHit;

    /** Último acerto crítico confirmado (hora do mundo), para o HUD destacar o número. */
    float LastCritTime = -10.f;
    TWeakObjectPtr<AActor> LastCritTarget;

    /** Does not trust client-reported hits; callable only by an authoritative ability. */
    bool TryMeleeHit(AActor* Target, float Damage, float ReachCm, float MinForwardDot, float StunSeconds = 0.f, bool bMagic = false, float DamageScale = 1.f);
    int32 StrikeInArc(float Damage, float ReachCm, float MinForwardDot, TSet<TWeakObjectPtr<AActor>>& AlreadyHit, float StunSeconds = 0.f, float DamageScale = 1.f);
    /** Facção diferente, viva e sem esquivar. */
    bool CanHarm(const AActor* Target) const;
    /**
     * Só no servidor, com o alcance já checado por quem chama (golpe à distância): defesa, dano e avisos.
     * Kind: o atributo do herói que soma ao golpe e, numa criatura, se o golpe é mágico (Resistência).
     * StunSeconds: a criatura acertada fica parada esse tempo (chefes não).
     */
    bool ApplyHit(AActor* Target, float Damage, EVortexDamageKind Kind = EVortexDamageKind::Melee, float StunSeconds = 0.f, float DamageScale = 1.f);

    // ---- estados das habilidades de classe (servidor): lentidão/prisão, enfraquecer, expor, dano contínuo
    /** Velocidade vira Factor (0 = preso no lugar) por Seconds. */
    void ApplySlow(float Factor, float Seconds);
    /** Causa Factor menos dano (0,2 = -20%) por Seconds. */
    void ApplyWeaken(float Factor, float Seconds);
    /** Leva Factor mais dano (0,2 = +20%) por Seconds. */
    void ApplyExpose(float Factor, float Seconds);
    /** Poção do catalisador: causa Factor mais dano e anda Factor mais rápido por Seconds (vale para inimigos também). */
    void ApplyEmpower(float Factor, float Seconds);
    /** Perde Dps de vida por segundo por Seconds (quem aplicou ganha o crédito se matar). */
    void ApplyDot(float Dps, float Seconds, AActor* Source);
    /** Multiplicador da velocidade agora (lentidão). */
    float MoveScale() const;
    /** Multiplicador do dano que este causa (enfraquecido) e do que leva (exposto). */
    float OutgoingScale() const;
    float IncomingScale() const;
    bool IsRooted() const { return MoveScale() <= .05f; }

    // ---- atordoamento (servidor; criaturas leem no Think)
    void Stun(float Seconds);
    bool IsStunned() const;
    float StunRemaining() const;
    FVortexStunned OnStunned;
    /** Chefes não ficam atordoados (o Godot zera o stun no Azhûr e nos chefes regionais). */
    bool bStunImmune = false;
    /** Escudo em guarda (bandido de escudo): golpes pela frente passam só 40%; pelas costas, inteiros. */
    bool bShieldFront = false;
    /** Criatura cujo golpe é mágico (veneno, cristal, runas): a Resistência do herói corta, a Defesa não. */
    bool bMagicAttacks = false;

    /** Servidor: quem acertou este por último (as criaturas perseguem quem as provocou). */
    TWeakObjectPtr<AActor> LastAttacker;
    double LastHurtTime = -100.0;

    /** Contra-ataque aberto: depois de um aparo perfeito, o próximo golpe é crítico e bate 50% mais. */
    bool HasCounterReady() const;
    /** Segundos que o contra-ataque ainda fica aberto (o dono vê na HUD). */
    float CounterReadyFor() const;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    static constexpr float ParryStunSeconds = .65f;
    /** Depois de um atordoamento, quanto tempo os golpes comuns não atordoam de novo (o aparo perfeito sempre atordoa). */
    static constexpr float StunGuardSeconds = 3.f;
    double StunGuardUntil = 0.0;
    static constexpr float CounterWindowSeconds = 1.6f;

private:
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastConfirmedHit(AActor* Target, float Damage, bool bCrit);
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastParry(AActor* Attacker, bool bStunned);
    double StunnedUntil = 0.0;
    double SlowUntil = 0.0, WeakenUntil = 0.0, ExposeUntil = 0.0, DotUntil = 0.0;
    float SlowFactor = 1.f, WeakenFactor = 0.f, ExposeFactor = 0.f, DotDps = 0.f;
    double EmpowerUntil = 0.0;
    float EmpowerFactor = 0.f;
    TWeakObjectPtr<AActor> DotSource;
    FTimerHandle DotTimer;
    void TickDot();
    double CounterUntil = 0.0;
    /** O mesmo prazo, no relógio do servidor que os clientes conhecem (só para o dono). */
    UPROPERTY(Replicated) float CounterServerUntil = 0.f;
};
