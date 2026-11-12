#pragma once

#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Framework/VortexLook.h"
#include "VortexCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UAbilitySystemComponent;
class UVortexAttributes;
class UVortexCombatComponent;
class UVortexWeaponComponent;

/** Visual do herói pela rede (raça, peças e cores), para todos verem a mesma aparência. */
USTRUCT()
struct FVortexNetLook
{
    GENERATED_BODY()
    UPROPERTY() FName Race;
    UPROPERTY() FName Appearance;
    UPROPERTY() TArray<FName> Categories;
    UPROPERTY() TArray<FName> Pieces;
    UPROPERTY() TArray<FName> ColorSlots;
    UPROPERTY() TArray<FColor> Colors;
};

class AVortexPortal;
UCLASS()
class VORTEX_API AVortexCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    /** O inimigo para onde o próximo golpe de perto vai (o mesmo que o golpe escolhe); a HUD marca ele. */
    AActor* PreviewMeleeTarget() const;
    /** Virada rápida do mouse: o foco passa para o inimigo que ficou na frente da câmera (outro que não o atual). */
    void FlickFocus();
    explicit AVortexCharacter(const FObjectInitializer& ObjectInitializer);

    class UVortexCharacterMovementComponent* GetVortexMovement() const;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    const UVortexAttributes* GetVitals() const { return Vitals; }
    UVortexCombatComponent* GetCombat() const { return Combat; }
    UVortexWeaponComponent* GetWeapon() const { return Weapon; }
    class UVortexHeroSheet* GetSheet() const { return Sheet; }
    class UVortexQuestLog* GetQuestLog() const { return QuestLog; }
    class UVortexClassSkills* GetClassSkills() const { return ClassSkills; }
    class UVortexSocial* GetSocial() const { return Social; }
    UStaticMeshComponent* GetWeaponVisual() const { return WeaponVisual; }
    /** A arma da primeira pessoa (na mão do corpo-cópia), ou nada. */
    UStaticMeshComponent* GetViewWeapon() const { return ViewRight; }
    /** Cavalo em que está montado (todas as máquinas), ou nada. */
    class AVortexMonster* GetMount() const { return Mount; }
    /** Sem vida (todas as máquinas). */
    bool IsDead() const;
    /** Há quanto tempo está caído nesta máquina (a tela de morte). */
    float GetDeadTime() const { return DeadTime; }
    bool IsMounted() const { return Mount != nullptr; }
    /** Todas as máquinas: o corpo sobe para a sela (0 = no chão). */
    void SetMountLift(float Lift);
    /** O cavalo selvagem ou domado mais perto (até Range cm), ou nada. */
    class AVortexMonster* FindHorseNear(float Range) const;
    UStaticMeshComponent* GetWeaponVisualLeft() const { return WeaponVisualLeft; }

    // Câmera igual à do Godot no teste de passeio (7 m, alvo 1,35 m acima dos pés, ombro 1,15 m).
    void UseGodotBenchCamera(float ArmLength = 700.f, float Shoulder = 115.f);
    bool IsGodotBenchCamera() const { return bGodotBenchCamera; }
    /** On-demand aim ray; no trace runs during ordinary camera updates. */
    bool TraceAim(FHitResult& OutHit, float MaxDistance = 10000.f) const;
    void StepZoom(float Steps);
    void SwapCameraShoulder();
    void SetAimHeld(bool bHeld);
    void BeginPrimaryAttack();

    // Giro do modelo importado para ficar de frente para +X (a frente do personagem na Unreal).
    static constexpr float MeshYawOffset = -90.f;
    static constexpr float HeroCapsuleHalfHeight = 70.f;
    static constexpr float HeroCapsuleRadius = 25.f;
    /** Visual compacto para a rede (também usado pela sala de espera). */
    static FVortexNetLook LookToNet(const FVortexLook& Look);
    static FVortexLook LookFromNet(const FVortexNetLook& Net);

    virtual void BeginPlay() override;
    virtual void NotifyControllerChanged() override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void Landed(const FHitResult& Hit) override;
    virtual void OnJumped_Implementation() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Troca uma peça importada com o mesmo esqueleto; por ora é uma prévia local, sem persistência.
    UFUNCTION(BlueprintCallable, Category = "Vortex|Appearance")
    bool SetWardrobePiece(FName Category, FName PieceId);

    /** Veste um visual completo (FVortexLook): peças de todas as categorias, paleta recolorida e o
     *  tamanho da raça (só o modelo cresce ou encolhe; a cápsula de colisão continua a mesma). */
    void ApplyLook(const FVortexLook& Look);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    /** Proporção da raça vestida agora. */
    const FVortexBodyShape& GetBodyShape() const { return BodyShape; }
    const FVortexLook& GetLook() const { return CurrentLook; }

    // Até quando o HUD mostra a sensibilidade do mouse (tempo do mundo), depois de [ ou ]
    float GetSensitivityNoticeUntil() const { return SensitivityNoticeUntil; }
    float GetMinimapNoticeUntil() const { return MinimapNoticeUntil; }
    bool IsAimHeld() const { return bAimHeld; }
    /** Arma de perto na mão (espada, manoplas, machado...): sem mira, bate no que está à frente. */
    bool IsMeleeWeapon() const;
    /** C: câmera em primeira pessoa (nos olhos do herói; o corpo continua aparecendo, sem a cabeça). */
    bool IsFirstPerson() const { return bFirstPerson; }
    void ToggleFirstPerson();

    // ---- emotes e danças (segurar B abre a roda; o mouse escolhe; soltar toca). Todos veem
    struct FEmote { const TCHAR* Clip; const TCHAR* Label; };
    /** As páginas da roda (0 danças, 1 gestos); a rodinha do mouse troca de página. */
    static const TArray<FEmote>& EmotePage(int32 Page);
    static constexpr int32 EmotePages = 2;
    /** O emote tocando agora (todas as máquinas), ou nenhum. */
    FName GetEmote() const { return Emote; }
    bool IsEmoteWheelOpen() const { return bEmoteWheel; }
    int32 GetEmotePage() const { return EmotePageIndex; }
    /** A fatia apontada pelo mouse (-1: o centro, nenhuma). */
    int32 GetEmoteChoice() const;
    /** Só os emotes que o herói tem (os clipes MX_<raça>_* do Mixamo). */
    bool HasEmote(FName Clip) const;
    /** Toca (ou para, com NAME_None) o emote: do dono passa pelo servidor. */
    void PlayEmote(FName Clip);
#if !UE_BUILD_SHIPPING
    /** Testes (VortexHeroShots): abre a roda apontando Aim, ou fecha (tocando o escolhido). */
    void DebugEmoteWheel(bool bOpen, FVector2D Aim) { if (bOpen) { OpenEmoteWheel(); EmoteAim = Aim; } else CloseEmoteWheel(); }
#endif

private:
    UPROPERTY(Replicated) FName Emote;
    double EmoteEnds = 0.0;
    bool bEmoteWheel = false;
    int32 EmotePageIndex = 0;
    FVector2D EmoteAim = FVector2D::ZeroVector;
    void OpenEmoteWheel();
    void CloseEmoteWheel();
    UFUNCTION(Server, Reliable) void ServerEmote(FName Clip);
    void ServerEmote_Implementation(FName Clip);
    /** Servidor: o emote acaba ao andar, pular, nadar, montar, cair, esquivar ou atacar (e os gestos, no fim). */
    void TickEmote();

private:
    // Giro da câmera pelo mouse, com a sensibilidade de UVortexPlayerSettings
    void LookYaw(float Value);
    void LookPitch(float Value);
    void SlowerMouse();
    void CycleMinimap();
    void FasterMouse();
    float SensitivityNoticeUntil = -1.f;
    float MinimapNoticeUntil = -1.f;
    bool EnsureWardrobeParts();
    /** Herói novo (Tools/ImportHeroes.py): o modelo fixo da raça, ou nada se a raça ainda não tem modelo. */
    /** O modelo da raça; no corpo feminino, o Feminino/Hero_<raça> quando existe (o humano de blocos: busto e cintura). */
    static USkeletalMesh* HeroModelFor(FName Race, FName Appearance = NAME_None);
    /** Põe o modelo fixo da raça no lugar das peças de roupa (sem personalização). */
    void ApplyHeroModel(USkeletalMesh* Model, const FVortexLook& InLookRaw);
    /** true: o corpo é o modelo fixo da raça (as peças de roupa ficam vazias). */
    bool bHeroModel = false;
    float RaceSpeed = 1.f;
    /** Primeira pessoa dos heróis novos (como no Fortnite): o corpo sem a cabeça (Hero_<raça>_FP), só para quem joga,
     *  copiando a pose do corpo de verdade. */
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> FirstPersonBody;
    /** Rosto animado (estilo Hytale, Tools/ImportHeroFace.py): o material com os quadros dos olhos e da boca, trocados
     *  em TickFace (piscar, olhar para onde a câmera vira, expressões no golpe, na dor, nas danças e na morte). */
    UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> FaceMaterial;
    /** Cabelo do humano de blocos (FVortexLookCatalog::BlockHairs): malha à parte com a pose do corpo, na cor "hair_color";
     *  os soltos com física (UVortexHairComponent). */
    UPROPERTY(Transient) TObjectPtr<class UVortexHairComponent> HairPart;
    void ApplyBlockHair();
    /** Roupa do humano de blocos (FVortexLookCatalog::BlockOutfits): malha à parte que segue a pose do corpo. */
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> OutfitPart;
    void ApplyBlockOutfit();
    float FaceBlinkIn = 3.f, FaceBlinkAge = 99.f, FaceHitAge = 99.f, FaceLastHealth = -1.f, FaceClock = 0.f;
public:
#if !UE_BUILD_SHIPPING
    /** Testes (VortexHeroShots): força o quadro dos olhos e da boca (-1 = o rosto vivo). */
    static inline int32 DebugFace = -1;
#endif
private:
    int32 FaceEye = -1, FaceMouth = -1;
    void TickFace(float DeltaSeconds);
    void ApplyAnimTickOption();
    // Teste no editor (fora da versão final): teclas 1 a 7 trocam a peça de cada categoria, F gira a
    // câmera para ver o rosto.
    void CycleWardrobe(int32 CategoryIndex);
    void LookAtFace();
    void ZoomIn();
    void ZoomOut();
    void UpdateAimFacing();
    void FinishAttackFacing();
    // golpe de perto: vira para o inimigo mais próximo à frente (ou para onde anda) e dá um passo curto
    AActor* FindMeleeTarget(const FVector& Want, const AActor* Skip = nullptr) const;
    /** O inimigo em foco fica preso até morrer, sair de perto ou o jogador virar o mouse para outro. */
    mutable TWeakObjectPtr<AActor> FocusLock;
    void BeginMeleeFacing();
    void TickMeleeFacing(float DeltaSeconds);

    UFUNCTION(Server, Reliable)
    void ServerBeginMeleeFacing(AActor* InTarget, float Yaw);
    TWeakObjectPtr<AActor> MeleeTarget;
    float MeleeYaw = 0.f, MeleeFacingAge = 0.f;
    bool bMeleeFacing = false;
    bool bFirstPerson = false;
    /** O Despertar já apareceu nesta sessão (fechar com Esc não reabre sozinho; a aba Classes reabre). */
    bool bAwakeningShown = false;
    float AwakeningCheck = 0.f;
    float FirstPersonBlend = 0.f, BobPhase = 0.f;
    void TickFirstPerson(float DeltaSeconds);
    // primeira pessoa (G5): uma cópia do corpo presa à câmera (só quem joga vê), com a mesma pose do
    // corpo (leader pose) e a arma nos ossos da mão; cabeça e pernas ficam de fora
    void TickViewModel(float DeltaSeconds);
    void SetViewModelVisible(bool bVisible);
    UPROPERTY(Transient) TObjectPtr<USceneComponent> ViewRoot;
    UPROPERTY(Transient) TArray<TObjectPtr<USkeletalMeshComponent>> ViewBody;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> ViewRight;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> ViewLeft;
    float ViewBob = 0.f;
    /** Primeira pessoa: o quanto o corpo-cópia saiu do lugar para deixar o meio da tela livre (cm). */
    FVector2D ViewAvoid = FVector2D::ZeroVector;
    /** Terceira pessoa: a câmera está dentro do herói (o corpo some para quem joga). */
    bool bCameraInsideBody = false;
public:
    /** Primeira pessoa: onde ficam os pés do corpo-cópia, à frente e abaixo dos olhos (cm). */
    float ViewForward = 36.f, ViewDrop = 82.f;
private:
    FRotator ViewLastCamera = FRotator::ZeroRotator;
    /** Primeira pessoa: onde a mão de segurar fica no corpo-cópia na pose de descanso (cm, espaço do ViewRoot). */
    FVector ViewRest = FVector::ZeroVector;
    bool bViewRestValid = false;
    TWeakObjectPtr<const class UVortexWeaponDefinition> ViewRestWeapon;
    FVector2D ViewSway = FVector2D::ZeroVector;
    void ApplyFirstPersonVisibility();
    UFUNCTION(Server, Reliable)
    void ServerSetAimHeld(bool bHeld);
    UFUNCTION(Server, Reliable)
    void ServerBeginAttackFacing();
#if !UE_BUILD_SHIPPING
    FTimerHandle MovementObserveTimer;
    bool bClientInputSmokeActive = false;
    bool bClientInputSmokeStarted = false;
    bool bClientInputSmokeSprinting = false;
    bool bClientInputSmokeJumped = false;
    bool bClientInputSmokeDodged = false;
    bool bClientInputSmokeSawAirborne = false;
    float ClientInputSmokeElapsed = 0.f;
    float ClientInputSmokeStartX = 0.f;
#endif

    void MoveForward(float Value);
    void MoveRight(float Value);
    void ToggleWalk();
    void BeginSprint();
    void EndSprint();

    void BeginDodge();
    void BeginRecover();
    void BeginSpecialAttack();
    void EndPrimaryAttack();
    void BeginJump();
    /** Bebendo: a ação vira "parar de beber" (a poção não é gasta). true = cancelou. */
    bool CancelDrinkInstead();
    /** Visual escolhido na criação (sem armadura) e a mesma coisa com a armadura vestida por cima. */
    FVortexLook BaseLook;
    bool bHasBaseLook = false;
    uint32 ArmorSignature = 0;
    float ArmorCheck = 0.f;
    FVortexLook WithArmor(const FVortexLook& InLook) const;
    void RefreshArmorLook();
    /** A garrafinha na mão esquerda e o som do gole (todas as máquinas que desenham). */
    void TickPotion();
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> PotionBody;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> PotionView;
    bool bWasDrinking = false;
public:
    /** Bebendo uma poção (todas as máquinas). */
    bool IsDrinking() const;
private:
    void EndSpecialAttack();
    void BeginGuard();
    void EndGuard();
    void QuickHealth();
    void QuickMana();
    void ToggleHeroWindow();
public:
    /** Opções mudaram (campo de visão): a câmera vai até o novo valor. */
    void RefreshCameraSettings() { bCameraTransitionActive = true; }
    /** Tranco curto na câmera (acerto dado ou levado). */
    void KickCamera(float Strength) { KickStrength = FMath::Max(KickStrength * FMath::Max(0.f, 1.f - KickAge / .2f), Strength); KickAge = 0.f; }
private:
public:
    /** E: no Estádio, pega a arma do pedestal perto ou sai pelo portão dourado. */
    void Interact();
private:

    float WantedCameraDistance = 380.f;
    float CameraShoulderSign = 1.f;
    bool bAimHeld = false;
    bool bAttackFacing = false;
    FTimerHandle AttackFacingTimer;
    bool bCameraTransitionActive = false;
    float KickAge = 10.f, KickStrength = 0.f;
    bool bGodotBenchCamera = false;

    UPROPERTY(VisibleAnywhere, Category = "Vortex|Prototype")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, Category = "Vortex|Prototype")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleAnywhere, Category = "Vortex|Prototype")
    TObjectPtr<UStaticMeshComponent> DebugBody;

    UPROPERTY(VisibleAnywhere, Category = "Vortex|Appearance")
    TArray<TObjectPtr<USkeletalMeshComponent>> WardrobeParts;

    UPROPERTY(Transient)
    TObjectPtr<class UMaterialInstanceDynamic> LookMaterial;
    UPROPERTY(Transient)
    TObjectPtr<class UTexture2D> LookPalette;
    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<class UMaterialInstanceDynamic>> TintMaterials;
    FVortexLook CurrentLook;
    FVortexBodyShape BodyShape;
    // sons (em todas as máquinas que desenham)
    void UpdateSounds(float DeltaSeconds);
    float StepTimer = 0.f, LastHealth = -1.f, FallSpeed = 0.f, SwimFxTimer = 0.f;
    bool bWasSwimming = false, bSwimStrokeSide = false, bWasWading = false, bWasWadingNext = false;
    /** Uma janela (inventário, diálogo, loja, social, chat) está com o mouse: sem ataque e sem girar a câmera. */
    bool IsMouseOnWindow() const;
    UPROPERTY(Transient) TObjectPtr<class UNiagaraComponent> BurnFx;
    int32 LastDodges = 0, LastAttackSerial = 0, StepVariant = 0;
    bool SetPartMesh(FName PartName, FName PieceId);
    void ApplyLookLocal(const FVortexLook& Look);
    void ApplyBodyShape(const FVortexBodyShape& Shape);
    UPROPERTY(ReplicatedUsing = OnRep_NetLook)
    FVortexNetLook NetLook;
    UFUNCTION()
    void OnRep_NetLook();
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetLook(const FVortexNetLook& Look);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vortex|Abilities", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UAbilitySystemComponent> AbilitySystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vortex|Abilities", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UVortexAttributes> Vitals;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vortex|Combat", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UVortexCombatComponent> Combat;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vortex|Weapon", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UVortexWeaponComponent> Weapon;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vortex|Weapon", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> WeaponVisual;

    /** Cópia espelhada das armas de par (adagas, manoplas...), no osso Hand_L. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vortex|Weapon", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> WeaponVisualLeft;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vortex|Rpg", meta=(AllowPrivateAccess="true"))
    TObjectPtr<class UVortexHeroSheet> Sheet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vortex|Rpg", meta=(AllowPrivateAccess="true"))
    TObjectPtr<class UVortexQuestLog> QuestLog;

    /** As 4 habilidades da classe (teclas 1 a 4). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vortex|Rpg", meta=(AllowPrivateAccess="true"))
    TObjectPtr<class UVortexClassSkills> ClassSkills;
    /** Grupo, amizade, duelo e troca com outros jogadores (E perto deles). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vortex|Rpg", meta=(AllowPrivateAccess="true"))
    TObjectPtr<class UVortexSocial> Social;
    void CastClassSkill(int32 Slot);
    /** Z: marca o ponto sob a mira para o grupo (numa criatura hostil: "perigo"). */
    void Ping();
    UFUNCTION(Server, Reliable)
    void ServerPing(FVector_NetQuantize At, AActor* Target);
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPing(FVector_NetQuantize At, AActor* Target, bool bDanger);
    float NextPingTime = 0.f;
    // ---- coletáveis e cavalo (E colhe, dá fruta, monta e desmonta; X assobia)
    UPROPERTY(Replicated) TObjectPtr<class AVortexMonster> Mount;
    TWeakObjectPtr<class AVortexMonster> OwnHorse;
    float MountLift = 0.f;
    float DeadTime = 0.f;
    bool bDeadPosed = false;
    void TickDeath(float DeltaSeconds);
    void RespawnNow();
    void Whistle();
    UFUNCTION(Server, Reliable) void ServerHarvest(FName Id);
    UFUNCTION(NetMulticast, Reliable) void MulticastHarvested(FName Id, FVector_NetQuantize At);
    UFUNCTION(Server, Reliable) void ServerHorse(class AVortexMonster* Horse);
    void ServerHorse_Implementation(class AVortexMonster* Horse);
    UFUNCTION(Server, Reliable) void ServerDismount();
    UFUNCTION(Server, Reliable) void ServerWhistle();
    void ServerWhistle_Implementation();
    // baús das cabanas abandonadas: o servidor sorteia o saque; o dono vê a tampa abrir
    UFUNCTION(Server, Reliable) void ServerOpenChest(FName Id);
    void ServerOpenChest_Implementation(FName Id);
    UFUNCTION(Client, Reliable) void ClientChestOpened(FName Id);
    // santuários (pontos de interesse): orar dá a bênção por 5 minutos; de novo no mesmo, só depois de 10
    UFUNCTION(Server, Reliable) void ServerPray(FName Id);
    void ServerPray_Implementation(FName Id);
    UFUNCTION(Client, Reliable) void ClientBlessed(uint8 Variant);
    TMap<FName, double> PrayedAt;
    // portais de dungeon (VortexPortal): entrar leva ao destino; sem destino, a dungeon ainda está sendo feita
    UFUNCTION(Server, Reliable) void ServerEnterPortal(AVortexPortal* Portal);
    void ServerEnterPortal_Implementation(AVortexPortal* Portal);
    UFUNCTION(Client, Reliable) void ClientPortalEntered(bool bTravelled);
    // teias (pontos de interesse): presa anda bem devagar e não pula; o golpe corta (3 golpes)
    UFUNCTION(Server, Unreliable) void ServerSlashWeb();
    void ServerSlashWeb_Implementation();
    bool bWebbed = false;
public:
    bool IsWebbed() const { return bWebbed; }
    /** Velocidade a pé da raça (o anão de perna curta anda a 80%: o passo dos clipes casa com o chão). Posta em
     *  ApplyHeroModel, no servidor e nos clientes (a raça é replicada). */
    float GetRaceSpeed() const { return RaceSpeed; }
private:
public:
    /** Servidor: desce do cavalo (morte, teste). */
    void Dismount();
    /** Testes: colhe sem tecla (o mais perto). */
    bool DebugHarvestNearest();
    /** T: chat de comandos de teste (/god, /fly, /level...; ver VortexDevChat). */
    void OpenDevChat();
    UFUNCTION(Server, Reliable) void ServerDevCommand(const FString& Line);
    UFUNCTION(Client, Reliable) void ClientDevReply(const FString& Text, bool bError);
    UFUNCTION(Client, Reliable) void ClientDevFly(bool bOn);
    /** Comando /fly: voa (servidor e dono); W vai para onde a câmera olha. */
    void SetDevFly(bool bOn);
    bool IsDevFlying() const;
    /** Testes: o E perto de um cavalo (fruta ou montar) e o assobio, sem tecla. */
    void DebugHorse(class AVortexMonster* Horse) { ServerHorse_Implementation(Horse); }
    void DebugWhistle() { ServerWhistle_Implementation(); }
    void DebugOpenChest(FName Id) { ServerOpenChest_Implementation(Id); }
    void DebugPray(FName Id) { ServerPray_Implementation(Id); }
    void DebugEnterPortal(AVortexPortal* Portal) { ServerEnterPortal_Implementation(Portal); }
};
