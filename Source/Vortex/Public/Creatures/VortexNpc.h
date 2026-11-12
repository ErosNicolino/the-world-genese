#pragma once

#include "GameFramework/Character.h"
#include "Framework/VortexCharacter.h"
#include "Creatures/VortexSteering.h"
#include "Creatures/VortexNpcRoutine.h"
#include "VortexNpc.generated.h"

/**
 * Morador do vale (Fase 14). Só a "aparição" de um posto da população: o ID e o estado moram em
 * UVortexPopulationSubsystem, que cria este ator perto dos jogadores e o destrói longe deles.
 * No servidor anda pela patrulha do posto (se houver); no cliente fica oculto até o chão local
 * daquele ponto carregar. Usa o aventureiro e a animação do herói (andar/parar) e veste as peças e
 * cores do posto (o mesmo guarda-roupa e paleta do herói); um bichinho (AVortexPet) pode seguir o
 * NPC. Falas e diálogo ficam para as fases próprias.
 */
UCLASS()
class VORTEX_API AVortexNpc : public ACharacter
{
    GENERATED_BODY()

public:
    AVortexNpc();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Servidor, antes do BeginPlay (SpawnActorDeferred)
    void InitNpc(FName Id, const FString& Name, const TArray<FVector2D>& InPatrol, int32 InPatrolIndex, float InRestYaw);
    /** Servidor, antes do BeginPlay: aparência e bichinho (vão pela rede uma vez, na criação). */
    void InitLook(const FVortexLook& Look, FName Pet);

    FName GetNpcId() const { return NpcId; }
    const FString& GetDisplayName() const { return DisplayName; }
    int32 GetPatrolIndex() const { return PatrolIndex; }

    static constexpr float WalkSpeed = 160.f;   // cm/s, passo de quem passeia na praça

    /** Servidor: alguém começou a conversar; o NPC para a patrulha e olha para ele por uns segundos. */
    void TalkTo(AActor* Listener);

    /** 08/11, servidor, antes do BeginPlay: o posto (para a rotina, VortexNpcRoutine), se é criança e se já nasce onde
     *  a rotina manda (a população o cria lá: chega fazendo o que a hora pede, sem andar). */
    void InitRoutine(const FVector2D& InPost, float InPostYaw, bool bInChild, bool bInAtGoal);
    /** 08/11: o que ele faz parado (o clipe em laço), o gesto da vez (muda o número: toca de novo), sentado, dormindo. */
    FName GetActivity() const { return Activity; }
    FName GetGesture() const { return Gesture; }
    uint8 GetGestureSeq() const { return GestureSeq; }
    bool IsSitting() const { return bSitting; }
    bool IsAsleep() const { return bAsleep; }
    bool IsChild() const { return bChild; }
    /** O que ele está dizendo a outro morador agora (o balão em cima da cabeça; vazio: nada). */
    const FString& GetBubble() const { return BubbleText; }
    /** Testes: para onde vai agora (sem rotina: false). */
    bool GetRoutineGoal(FVector2D& OutAt, bool& bOutArrived) const { OutAt = Goal.At; bOutArrived = bArrived; return bHasGoal; }

private:
    // 08/11: a rotina (servidor) e o que vai pela rede para a animação
    UPROPERTY(Replicated) FName Activity;
    UPROPERTY(Replicated) FName Gesture;
    UPROPERTY(ReplicatedUsing = OnRep_Gesture) uint8 GestureSeq = 0;
    UPROPERTY(Replicated) bool bSitting = false;
    UPROPERTY(ReplicatedUsing = OnRep_Asleep) bool bAsleep = false;
    UPROPERTY(Replicated) bool bChild = false;
    UPROPERTY(Replicated) FString BubbleText;
    TWeakObjectPtr<AVortexNpc> ChatPartner;
    TArray<FString> ChatLines;
    int32 ChatIndex = 0, GreetCount = 0;
    float ChatNext = 0.f, NextChatAt = 0.f, GreetGap = 60.f;
    bool bChatLead = false;
    FName ChatSaved;
    bool CanChat() const;
    void TickChat(float DeltaSeconds);
    void EndChat();
    void JoinChat(AVortexNpc* Partner, bool bLead);
    UFUNCTION() void OnRep_Gesture();
    UFUNCTION() void OnRep_Asleep();
    VortexNpcRoutine::FGoal Goal;
    FVector2D Post = FVector2D::ZeroVector;
    float PostYaw = 0.f;
    bool bAtGoal = false, bHasGoal = false, bArrived = false;
    TArray<FVector2D> Route;
    int32 RouteIndex = 0, Replans = 0, WanderCount = 0;
    float RoutineCheck = 0.f, WanderLeft = 0.f, NoProgressFor = 0.f, LastGreet = -1000.f, WatchFor = 0.f;
    FVector2D ProgressFrom = FVector2D::ZeroVector;
    FName PausedActivity;
    void SetGoal(const VortexNpcRoutine::FGoal& Next);
    void FollowRoute(float DeltaSeconds);
    void Arrive();
    void SetAsleep(bool bSleep);
    void StandUp();
    void React(float DeltaSeconds);
    void PlayGesture(FName Clip);
    void Say();

    UPROPERTY(Replicated)
    FName NpcId;

    UPROPERTY(Replicated)
    FString DisplayName;

    UPROPERTY(Replicated)
    FVortexNetLook NpcLook;

    UPROPERTY(Replicated)
    FName PetKind;

    // guarda-roupa (cada máquina monta o seu)
    UPROPERTY(Transient) TArray<TObjectPtr<USkeletalMeshComponent>> Parts;
    UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> LookMaterial;
    UPROPERTY(Transient) TMap<FName, TObjectPtr<class UMaterialInstanceDynamic>> Tints;
    UPROPERTY(Transient) TObjectPtr<class UTexture2D> Palette;
    TWeakObjectPtr<class AVortexPet> Pet;
    void ApplyLook();
    void SetPart(FName PartName, FName PieceId);

    TArray<FVector2D> Patrol;
    int32 PatrolIndex = 0;
    float RestYaw = 0.f;
    float PauseLeft = 0.f;
    TWeakObjectPtr<AActor> TalkingWith;
    VortexSteering::FMemory Steer;
    // preso (banco, caixote, outro NPC): sem avançar por um tempo, desvia e segue para o próximo ponto
    float StuckFor = 0.f, DodgeLeft = 0.f;
    FVector2D LastProgressAt = FVector2D::ZeroVector, DodgeDir = FVector2D::ZeroVector;
    FTimerHandle ShowHandle;
    bool bShown = false;

    void TryShowLocally();
};
