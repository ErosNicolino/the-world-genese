#pragma once

#include "GameFramework/Actor.h"
#include "Rpg/VortexRpg.h"
#include "VortexWeaponFx.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UPointLightComponent;

/**
 * Os efeitos dos golpes e secundários novos das armas (REVISAO_ARMAS), um ator por efeito:
 *   aura       canto do instrumento (anel e notas em volta de quem toca, segue o herói)
 *   orbit      2 orbes de luz girando em volta do herói (atiram no inimigo mais perto)
 *   tornado    círculo mágico e tornado de fogo no ponto (dano em área por alguns segundos)
 *   firewall   parede de fogo (dano a quem encosta; as criaturas não atravessam, menos pedra e fogo)
 *   vines      vinhas prendendo uma criatura (segue o alvo)
 *   lob        garrafa arremessada em arco que estilhaça no ponto (dano, ácido e poça)
 *   potion     poção que estoura: fortalece todos no raio (heróis e criaturas)
 *   holytrail  rastro sagrado no chão para a frente (dano e atordoamento)
 *   boomerang  machado/chakram que vai até o alcance e volta para a mão (acerta na ida e na volta)
 *   rain       chuva de flechas num raio
 *   seal       runa grudada (no inimigo ou no chão) que explode depois de um tempo
 *   beam       chicote esticado até o ponto (gancho)
 *   slam       pancada no chão (onda de choque e poeira)
 *   sweep      rastro largo de corte em volta (giro, varrida)
 * O servidor decide o dano; os números de criação vão uma vez pela rede (como AVortexProjectile).
 */
UCLASS()
class VORTEX_API AVortexWeaponFx : public AActor
{
    GENERATED_BODY()
public:
    AVortexWeaponFx();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Servidor: cria o efeito em At (girado por Rot), deixa Setup ajustar e termina a criação. */
    static AVortexWeaponFx* Spawn(AActor* Hero, FName Kind, const FVector& At, const FRotator& Rot, TFunctionRef<void(AVortexWeaponFx&)> Setup);
    /** Só desenho, nesta máquina (soco, estocada): sai depois de Delay na frente de Hero. */
    static void SpawnLocal(AActor* Hero, FName Kind, float Delay, float Reach, const FLinearColor& Tint);
    /** Criaturas: o passo que elas querem dar, desviando das paredes de fogo (Golem e Colosso passam). */
    static FVector SteerAroundWalls(const class ACharacter* Creature, const FVector& Move);

    // ---- números de criação (vão pela rede uma vez)
    UPROPERTY(Replicated) FName Kind;
    UPROPERTY(Replicated) FLinearColor Color = FLinearColor::White;
    UPROPERTY(Replicated) float Life = 1.f;
    UPROPERTY(Replicated) float Radius = 100.f;
    UPROPERTY(Replicated) float Length = 0.f;
    UPROPERTY(Replicated) float Size = 1.f;
    UPROPERTY(Replicated) float Speed = 0.f;
    UPROPERTY(Replicated) float Delay = 0.f;
    UPROPERTY(Replicated) int32 Count = 1;
    UPROPERTY(Replicated) FVector_NetQuantize Target;
    UPROPERTY(Replicated) TObjectPtr<AActor> Follow;
    UPROPERTY(Replicated) TObjectPtr<UStaticMesh> Mesh;
    /** Elemento da arma (grimório de água, de raio...): a aparência do tornado e da parede. */
    UPROPERTY(Replicated) FName Element;
    /** Garrafa e poção: o inimigo mirado; a garrafa cai onde ele estiver quando chegar. */
    UPROPERTY(Replicated) TObjectPtr<AActor> Homing;

    // ---- só no servidor
    float Damage = 0.f;
    float Every = .5f;
    float Stun = 0.f;
    float Expose = 0.f, ExposeTime = 0.f;
    EVortexDamageKind DamageKind = EVortexDamageKind::Magic;
    /** Boomerang: avisa a arma que voltou (ela libera os golpes e mostra a arma na mão). */
    TFunction<void()> OnReturn;

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
    UPROPERTY(Transient) TObjectPtr<UPointLightComponent> Light;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Glows;
    UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraComponent>> Systems;
    float Age = 0.f, NextTick = 0.f, LookTick = 0.f;
    UPROPERTY(Transient) TObjectPtr<USceneComponent> Spinner;
    UPROPERTY(Transient) TObjectPtr<class UProceduralMeshComponent> Blades;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BladeLook;
    TSet<TWeakObjectPtr<AActor>> Hits;
    bool bReturning = false, bDone = false;
    FVector Start = FVector::ZeroVector, Out = FVector::ForwardVector;

    UStaticMeshComponent* AddPart(const TCHAR* Shape, bool bGlow, float Opacity = .6f);
    UStaticMeshComponent* AddMeshPart(UStaticMesh* InMesh);
    UNiagaraComponent* AddSystem(const TCHAR* Path, const FVector& At, const FVector& Scale);
    void BuildLook();
    void TickLook(float DeltaSeconds);
    void TickServer(float DeltaSeconds);
    void HitArea(const FVector& Center, float RadiusCm, float Amount, bool bOnce);
    bool HitOne(AActor* Victim, float Amount);
    TArray<AActor*> Enemies(const FVector& Center, float RadiusCm) const;
    /** A chuva de flechas: onde cai a próxima (puxando para os inimigos dentro do círculo). */
    FVector RainLanding() const;
    class UVortexCombatComponent* SourceCombat() const;
    FVector BoomerangAt(float Time, bool& bBack) const;
};
