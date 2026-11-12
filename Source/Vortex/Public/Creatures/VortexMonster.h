#pragma once

#include "AbilitySystemInterface.h"
#include "Creatures/VortexCreature.h"
#include "Creatures/VortexSteering.h"
#include "GameFramework/Character.h"
#include "VortexMonster.generated.h"

class UAbilitySystemComponent;
class UVortexAttributes;
class UVortexCombatComponent;
class AVortexCharacter;
struct FVortexSpeciesDef;
struct FVortexMonsterMove;
struct FOnAttributeChangeData;

/**
 * As criaturas do Godot que não são o lobo: gota de jade, cogumelo, golem, morcego, sapo, goblin,
 * coelho, cervo, cavalos e os chefes (Seryth, Thorun e Azhûr). Uma classe só, guiada pela ficha da
 * espécie (FVortexSpeciesDef): tamanho, ritmo, vida e dano pelo nível, golpes e o que acontece ao
 * vencê-la. Tudo é decidido no servidor (10 vezes por segundo); os clientes recebem o clipe a tocar.
 *
 * - Hostis: veem o herói até a distância da espécie (mais longe se foram provocadas), escolhem o
 *   golpe pela distância e pelas recargas, desviam de árvores e pedras (VortexSteering) e voltam
 *   para casa se o herói foge para longe.
 * - Animais: pastam em volta do posto e fogem do herói (o cavalo só foge se apanhar).
 * - Chefes: acordam quando o herói entra na arena, marcam o chão antes de cada golpe, ficam mais
 *   perigosos a 2/3 e a 1/3 da vida e voltam ao começo se todos fogem.
 */
UCLASS()
class VORTEX_API AVortexMonster : public ACharacter, public IAbilitySystemInterface, public IVortexCreature
{
    GENERATED_BODY()
public:
    float GetMaxHealthValue() const { return MaxHealthValue; }
    virtual int32 GetBossPhase() const override { return IsBoss() && bAwake ? Phase : 0; }
    AVortexMonster();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

    /** Servidor, antes do FinishSpawning: espécie ("Slime", "Horse_Baio"...), nível e grupo. */
    void Setup(FName InSpecies, int32 InLevel, int32 InPackId = -1, int32 InPackSlot = 0, int32 InPackSize = 1);
    /** Posto de onde veio (a população guarda para renascer). */
    int32 PostId = -1;
    /** Servidor, antes do FinishSpawning: semente da roupa dos humanoides (a mesma em todas as máquinas). */
    void SetLookSeed(int32 Seed) { LookSeed = Seed; }
    /** Bandidos: todos os tipos (espada, escudo, marreta, arco) contam como "Bandit" (saque, XP e missões). */
    bool IsBandit() const { return Species.ToString().StartsWith(TEXT("Bandit")); }
    /** As gotas (Slime, SlimeFire, SlimePoison, SlimeIron): o corpo de gelatina, sem esqueleto. */
    bool IsSlime() const { return Species.ToString().StartsWith(TEXT("Slime")); }

    // IVortexCreature
    virtual FName GetSpecies() const override;
    virtual int32 GetCreatureLevel() const override { return Level; }
    virtual FString GetCreatureTitle() const override;
    virtual FString GetCreatureAction() const override { return ActionLabel; }
    virtual float GetHealthFraction() const override;
    virtual bool IsCreatureDead() const override { return bDead; }
    virtual bool IsPassive() const override;
    virtual bool IsBoss() const override;
    virtual bool IsElite() const override { return bElite; }
    /** Antes do Setup: vira o semi-chefe da região (6x vida, 1,5x dano, 1,6x tamanho, não atordoa). */
    void MakeElite() { bElite = true; }
    /** Aranha: antes do FinishSpawning, nasce pendurada no fio (no alto) e cai quando o herói passa embaixo. */
    void SetHanging(bool bOn) { ClimbPose = bOn ? 3 : 0; }
    static constexpr float EliteScale = 1.6f;
    /** Dungeon dos portais, antes do FinishSpawning: não espera o terreno (fica fora do mapa), o teto para as aranhas
     *  penduradas e os multiplicadores das regras (fúria: dano; rápidos: velocidade). */
    void SetDungeon(float CeilingZ, float DamageScale, float SpeedScale) { bDungeon = true; DungeonCeiling = CeilingZ; DungeonDamage = DamageScale; DungeonSpeed = SpeedScale; }
    bool IsInDungeon() const { return bDungeon; }
    virtual bool IsEngaged() const override { return bEngaged; }

    const FVortexSpeciesDef* Def() const;
    /** Nome dos arquivos da espécie ("Slime", "Horse_Baio"...). */
    FName GetAssetName() const { return Species; }
    /** A pasta da malha e das animações (Def()->Model, ou a própria espécie). */
    FName GetModelName() const;
    /** Clipe que deve estar tocando (animação) e o número da ação (muda a cada golpe/reação). */
    FName GetActClip() const { return ActClip; }
    int32 GetActSerial() const { return ActSerial; }
    float GetActRate() const { return ActRate; }
    bool IsGrazing() const { return bGrazing; }
    bool IsFleeing() const { return bFleeing; }
    int32 GetPhase() const { return Phase; }
    /** O cérebro de golem vulcânico (só o Azhûr), ou nada. */
    class UVortexVolcanicBoss* GetVolcanic() const { return Volcanic; }
    float GetMeshBottom() const { return MeshBottom; }

    // ---- cavalo (MainMount do Godot): frutas silvestres domam; o domado segue o dono, é montado com E
    bool IsHorse() const { return Species.ToString().StartsWith(TEXT("Horse_")); }
    AVortexCharacter* GetTamer() const { return Tamer; }
    AVortexCharacter* GetRider() const { return Rider; }
    int32 GetTameFed() const { return TameFed; }
    int32 GetTameNeed() const { return TameNeed; }
    /** Servidor: o herói dá uma fruta (o cavalo para e come); true quando já comeu o bastante. */
    bool Feed(AVortexCharacter* By);
    /** Servidor: cria o cavalo domado (pelagem Coat) de Master no lugar dado. */
    static AVortexMonster* SpawnTamed(UWorld* World, FName Coat, const FTransform& Pose, AVortexCharacter* Master);
    /** Servidor: monta (By) ou desmonta (nullptr). */
    void SetRider(AVortexCharacter* By);
    /** Servidor: o assobio do dono: vem galopando até ele (de longe, surge fora da vista, a 35 m atrás). */
    void Whistled();
    /** Altura da sela acima do chão (cm), para o herói sentar (pelo osso "Saddle" do modelo). */
    float SaddleHeight() const;
    /** Quanto o herói By sobe para o quadril dele ficar na sela. */
    float RideLift(const AVortexCharacter* By) const;
    virtual FVector GetVelocity() const override;

private:
    /** O cérebro do Azhûr, o golem vulcânico (02/11): decide os golpes dele no lugar do ThinkBoss. */
    friend class UVortexVolcanicBoss;
    UPROPERTY(Transient) TObjectPtr<class UVortexVolcanicBoss> Volcanic;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UAbilitySystemComponent> AbilitySystem;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UVortexAttributes> Vitals;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UVortexCombatComponent> Combat;
    /** Humanoides: roupa (peças do guarda-roupa do herói) e a arma/escudo nas mãos. */
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UVortexDressComponent> Dress;
    UPROPERTY(Transient) TObjectPtr<class UStaticMeshComponent> WeaponVisual;
    UPROPERTY(Transient) TObjectPtr<class UStaticMeshComponent> ShieldVisual;
    UPROPERTY(Transient) TObjectPtr<class UVortexSlimeLook> SlimeLook;
    /** Névoa roxa de corrupção saindo dos pés dos monstros hostis (os animais não têm). */
    UPROPERTY(Transient) TObjectPtr<class UNiagaraComponent> Mist;
    void ApplyMist(const FVortexSpeciesDef& D, float HalfHeight, float Scale);
    UPROPERTY(Replicated) int32 LookSeed = 0;
    void ApplyHumanoid(const FVortexSpeciesDef& D);

    UPROPERTY(ReplicatedUsing = OnRep_Species) FName Species;
    UPROPERTY(Replicated) int32 Level = 1;
    UPROPERTY(Replicated) bool bElite = false;
    UPROPERTY(Replicated) int32 PackSlot = 0;
    UPROPERTY(Replicated) int32 PackSize = 1;
    UPROPERTY(Replicated) FName ActClip;
    UPROPERTY(Replicated) int32 ActSerial = 0;
    UPROPERTY(Replicated) float ActRate = 1.f;
    UPROPERTY(Replicated) bool bDead = false;
    UPROPERTY(Replicated) bool bEngaged = false;
    UPROPERTY(Replicated) bool bGrazing = false;
    UPROPERTY(Replicated) bool bFleeing = false;
    UPROPERTY(Replicated) uint8 Phase = 1;
    UPROPERTY(Replicated) FString ActionLabel;
    // aranha: 0 no chão, 1 na parede, 2 no teto, 3 pendurada no fio; o fio vai do corpo até ThreadTopZ (0: sem fio)
    UPROPERTY(Replicated) uint8 ClimbPose = 0;
    UPROPERTY(Replicated) float ThreadTopZ = 0.f;
    UPROPERTY(Replicated) bool bDungeon = false;
    float DungeonCeiling = 0.f, DungeonDamage = 1.f, DungeonSpeed = 1.f;
    UPROPERTY(Transient) TObjectPtr<class UStaticMeshComponent> Thread;
    FQuat BaseMeshRotation = FQuat::Identity;
    FVector BaseMeshLocation = FVector::ZeroVector;
    double ThreadDropAt = -10.0;
    /** Servidor: sobe parede, anda no teto, cai do fio; true quando cuidou do movimento neste quadro. */
    bool UpdateClimb(float DeltaSeconds, double Now);
    /** Todas as máquinas: o corpo vira com a parede/teto e o fio estica. */
    void UpdateClimbLook(float DeltaSeconds);
    UPROPERTY(ReplicatedUsing = OnRep_TerrainReady) bool bTerrainReady = false;
    UPROPERTY(Replicated) TObjectPtr<AVortexCharacter> Tamer;
    UPROPERTY(ReplicatedUsing = OnRep_Rider) TObjectPtr<AVortexCharacter> Rider;
    UPROPERTY(Replicated) uint8 TameFed = 0;
    UPROPERTY(Replicated) uint8 TameNeed = 3;
    double TameStopUntil = 0.0;
    /** Servidor: chamado no assobio, galopando até o dono (depois fica parado onde chegou). */
    bool bComing = false;
    UFUNCTION() void OnRep_Rider();
    void ApplyRiding();
    void ThinkTamed(double Now);

    UFUNCTION() void OnRep_Species();
    UFUNCTION() void OnRep_TerrainReady();
    void ApplySpecies();
    void TryActivateTerrain();
    void RefreshPresentation();

    // ---- servidor
    int32 PackId = -1;
    FVector Home = FVector::ZeroVector;
    TWeakObjectPtr<AVortexCharacter> Target;
    FTimerHandle ThinkHandle, TerrainHandle;
    float AttackDamage = 10.f, MaxHealthValue = 30.f, LastHealth = 0.f, MeshBottom = 0.f;
    bool bLocalTerrainReady = false;
    int32 Acting = INDEX_NONE;        // golpe em andamento (índice em Moves)
    double ActStart = 0.0, ActImpact = 0.0, ActEnd = 0.0;
    bool bResolved = false;
    TMap<int32, double> MoveReady;    // recarga de cada golpe
    double NextMoveTime = 0.0, RetreatUntil = 0.0, FleeUntil = 0.0, ProvokedUntil = 0.0, HitUntil = 0.0, StunUntil = 0.0, FlinchReadyAt = 0.0;
    // fúria (poise): golpes seguidos em pouco tempo; ao passar do limite da espécie o bicho revida (ou salta
    // para trás) e por um instante não encolhe nem é atordoado
    int32 HitStreak = 0;
    double LastHitAt = -10.0, ArmorUntil = 0.0;
    double WanderAt = 0.0, AbsentSince = -1.0, RecoverUntil = 0.0;
    FVector WanderTo = FVector::ZeroVector;
    bool bFledLowHealth = false, bAwake = false;
    // 10/11 (IA, fase 1; ajustes em Data/Creatures/ia_monstros.json): o "!" ao ver o herói, o último golpe (para variar)
    // e a vez de atacar no bando (UVortexGroupCoordinator, grupo 1000000 + PackId)
    double AlertUntil = 0.0;
    int32 LastMove = INDEX_NONE;
    bool bAttackToken = false, bInPackGroup = false;
    // fase 2: o desvio do golpe forte, a busca no último lugar onde viu o herói e o pedido de ajuda (goblins)
    double DodgeReadyAt = 0.0, SearchUntil = 0.0, LabelUntil = 0.0;   // LabelUntil: até quando o aviso curto ("Desviou!") fica
    FVector LastSeenAt = FVector::ZeroVector;
    bool bCalledHelp = false;
    void ReleaseToken();
    /** A linha do monstro até o herói está livre (sem parede, árvore ou pedra no meio)? */
    bool CanSee(const AVortexCharacter* Hero) const;
    int32 PatternIndex = 0;
    // investida/salto em andamento
    FVector LungeFrom = FVector::ZeroVector, LungeTo = FVector::ZeroVector;
    float LungeStart = -1.f, LungeLength = 0.f;
    // decisão aplicada a cada quadro como entrada de movimento
    FVector MoveDirection = FVector::ZeroVector;
    float MoveSpeed = 0.f;
    VortexSteering::FMemory Steer;
    float StepTimer = 0.f;
    int32 StepVariant = 0;

    void Think();
    void ThinkPassive(double Now);
    void ThinkHostile(double Now);
    void ThinkBoss(double Now);
    AVortexCharacter* FindTarget(float Range) const;
    bool StartMove(int32 Index, double Now);
    void ResolveMove();
    void BeginPattern(double Now);
    void Act(FName Clip, float Rate = 1.f);
    void MoveTo(const FVector& Where, float Speed);
    void MoveAlong(const FVector& Direction, float Speed);
    void StopMoving();
    void FaceToward(const FVector& Where, float DeltaSeconds, float Rate = 8.f);
    void OnHealthChanged(const FOnAttributeChangeData& Data);
    void Die();
    void ResetEncounter();
    void AlertPack(AVortexCharacter* By);
    void PlayMoveSound(const FVortexMonsterMove& Move) const;
    float Damage() const { return AttackDamage; }
    UFUNCTION(NetMulticast, Reliable)
    void MulticastLavaPool(FVector_NetQuantize At, float Radius, float Life);
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastMeteor(FVector_NetQuantize At, float Flight);
    UFUNCTION(NetMulticast, Unreliable) void MulticastSound(FName Sound, float Volume);
    /** O efeito da habilidade (AVortexMonsterFx: veneno, tornado, poeira, lama) em At; bFollow: segue o monstro. */
    UFUNCTION(NetMulticast, Unreliable) void MulticastMoveFx(FName Fx, FVector_NetQuantize At, float Radius, float Delay, bool bFollow);
};
