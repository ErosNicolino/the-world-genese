#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VortexVolcanicBoss.generated.h"

class AVortexMonster;
class AVortexCharacter;
class UNiagaraComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** Os estados de combate do golem vulcânico (o documento de design do usuário, 02/11). */
UENUM()
enum class EVolcanicState : uint8
{
    Idle, Detection, Chase, Attack, Ability, Recovery, Stunned, PhaseTransition, Death
};

/**
 * Azhûr, o golem vulcânico (02/11): o cérebro do chefe da Caldeira. O AVortexMonster continua sendo o corpo (vida,
 * movimento, animação replicada pelo Act, morte e saque); este componente decide tudo no servidor:
 * - a máquina de estados (Idle, Detection, Chase, Attack, Ability, Recovery, Stunned, PhaseTransition, Death);
 * - as 12 habilidades (5 físicas, 6 vulcânicas e a final), com os números do Data/Creatures/azhur.json;
 * - a escolha do golpe por distância, recarga, fase e o que acabou de usar (sem repetir), e as emendas da fase 2;
 * - as duas fases (a 2ª com 40% da vida: núcleo mais quente, mais rápido, menos pausa, combinações).
 * Cada golpe só fere no instante do impacto, pelas marcas no chão (AVortexTelegraph, que o servidor resolve) ou pelo
 * feixe (o servidor confere a linha a cada tique). Os efeitos (os Niagara de /Game/Vortex/Bosses/Azhur/FX) vão a
 * todos por multicast, e o calor do núcleo segue a fase replicada do monstro.
 */
UCLASS(ClassGroup=(Vortex))
class VORTEX_API UVortexVolcanicBoss : public UActorComponent
{
    GENERATED_BODY()
public:
    UVortexVolcanicBoss();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Servidor: chamado pelo AVortexMonster::ThinkBoss (a cada 0,1 s). */
    void Think(double Now);
    /** Servidor: o encontro recomeça (todos saíram da arena). */
    void Reset();
    EVolcanicState GetState() const { return State; }
    /** Para os testes: começa uma habilidade agora, sem a escolha. */
    bool DebugUse(FName Ability);
    /** Os nomes das habilidades, na ordem do arquivo. */
    TArray<FName> AbilityNames() const;
    /** Um número de uma habilidade do arquivo (para os testes: a duração, o impacto). */
    float AbilityValue(FName Ability, const TCHAR* Key, float Default) const;
    /** Multiplica o dano que ele leva (atordoado leva mais). */
    float IncomingScale() const;

private:
    struct FAbility
    {
        FName Id, Clip;
        TMap<FName, float> Num;
        TArray<float> Hits;
        FString Label;
        bool bMagic = false;
        float Get(const TCHAR* Key, float Default = 0.f) const { const float* V = Num.Find(Key); return V ? *V : Default; }
    };
    TArray<FAbility> Abilities;
    TMap<FName, FName> Combos;
    TMap<FName, float> Stats;
    TMap<int32, TMap<FName, float>> PhaseStats;
    float Stat(const TCHAR* Key, float Default) const { const float* V = Stats.Find(Key); return V ? *V : Default; }
    float PhaseStat(const TCHAR* Key, float Default) const;
    void Load();

    UPROPERTY(Replicated) EVolcanicState State = EVolcanicState::Idle;
    double StateUntil = 0.0, LastNow = 0.0;
    int32 Current = INDEX_NONE;
    TArray<FName> Recent;
    TMap<FName, double> ReadyAt;
    bool bUltimateUsed = false, bHealthSet = false, bComboNext = false;
    float StaggerFrom = -1.f;
    double StaggerReadyAt = 0.0;
    TArray<FTimerHandle> Pending;

    AVortexMonster* Body() const;
    AVortexCharacter* Target() const;
    void Enter(EVolcanicState NewState, double Now, float Seconds, const FString& Label);
    int32 Choose(double Now, float Distance) const;
    void Use(int32 Index, double Now);
    void ClearPending();
    float Hit(const FAbility& A, const TCHAR* Key = TEXT("damage"), float Default = 1.f) const;
    FVector Ground(const FVector& At) const;
    void Mark(const FVector& At, float Radius, float Delay, float Damage, bool bRing, bool bMagic, float Stun = 0.f);
    void Later(float Seconds, TFunction<void()> Do);
    void Fx(FName System, const FVector& At, float Scale, float Life, const FRotator& Rotation = FRotator::ZeroRotator, bool bFollow = false);
    /** Um som do Azhûr (Data/Audio/azhur_<nome>_v1..3, Tools/MakeAzhurSfx.py) em At, agora ou daqui a Delay s. */
    void Sfx(FName Sound, const FVector& At, float Volume = 1.f, float Delay = 0.f);

    // as habilidades
    void DoHook(const FAbility& A);
    void DoHammer(const FAbility& A);
    void DoStomp(const FAbility& A);
    void DoSweep(const FAbility& A);
    void DoCombo(const FAbility& A);
    void DoCoreBurst(const FAbility& A);
    void DoFissure(const FAbility& A, const FVector& From, const FVector& Direction, float Delay, int32 Segments);
    void DoMeteor(const FAbility& A);
    void DoBeam(const FAbility& A);
    void DoEruption(const FAbility& A);
    void DoShed(const FAbility& A);
    void DoUltimate(const FAbility& A);

    // ---- todos: efeitos e o calor do núcleo
    UFUNCTION(NetMulticast, Unreliable) void MulticastFx(FName System, FVector_NetQuantize At, FRotator Rotation, float Scale, float Life, bool bFollow);
    UFUNCTION(NetMulticast, Reliable) void MulticastBeam(float Seconds, float Length);
    UFUNCTION(NetMulticast, Unreliable) void MulticastSfx(FName Sound, FVector_NetQuantize At, float Volume);
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> BeamMesh;
    UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> BeamFire;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BeamMaterial;
    double BeamUntil = 0.0;
    float BeamLength = 2400.f;
    TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> Heat;
    float HeatNow = 1.f;
    void UpdateBeam();
    void UpdateHeat(float DeltaTime);
};
