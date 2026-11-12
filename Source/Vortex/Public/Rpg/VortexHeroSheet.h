#pragma once

#include "Components/ActorComponent.h"
#include "Rpg/VortexRpg.h"
#include "VortexHeroSheet.generated.h"

/** O que o save do herói guarda da ficha (e o que o dono manda ao servidor ao entrar). */
USTRUCT()
struct VORTEX_API FVortexSheetSave
{
    GENERATED_BODY()
    UPROPERTY() int64 Xp = 0;
    UPROPERTY() TArray<int32> Points;
    UPROPERTY() TArray<FVortexItemStack> Bag;
    UPROPERTY() TArray<FName> Equipped;
    UPROPERTY() int32 Coins = 0;
    UPROPERTY() bool bNeedsAwakening = false;
    /** Veio de um herói do menu (os testes entram direto no mapa, sem herói salvo). */
    UPROPERTY() bool bFromMenu = false;
    /** Herói novo: o servidor dá a mochila inicial e ele acorda sem arma no Estádio do Despertar. */
    UPROPERTY() bool bFresh = true;
    /** Classe escolhida no Despertar (nível 10) e as 6 cartas tiradas (ficam até escolher). */
    UPROPERTY() FName ClassId;
    UPROPERTY() TArray<FName> ClassCards;
    /** Pelagem do cavalo domado ("Horse_Baio"...), ou nada. */
    UPROPERTY() FName MountCoat;
    /** 03/11: o nome que o jogador deu ao cavalo (ao domar). */
    UPROPERTY() FString MountName;
    /** A magia do cajado escolhida (02/11): "heal", "vines" ou "fireball"; nada até escolher. */
    UPROPERTY() FName StaffSpell;
    /** Caveira de quem matou outro herói fora do grupo (02/11): horas do jogo que faltam. */
    UPROPERTY() float OutlawHours = 0.f;
    /** 09/11: qual herói é (o anfitrião guarda a cópia dos convidados por jogador e herói). */
    UPROPERTY() FGuid HeroId;
    /** Lojas: dia das compras e o que já comprou nesse dia. */
    UPROPERTY() int32 ShopDay = 0;
    UPROPERTY() TArray<FName> ShopBought;
};

/**
 * Ficha de RPG do herói: nível e XP, pontos de atributo, mochila (30 espaços), equipamento e
 * moedas. O servidor é o dono dos números; o dono do herói recebe tudo (e grava no save local, como
 * o resto do herói); os outros jogadores só veem o nível.
 *
 * - Atributos = os oito da raça + os pontos (2 por nível) + o equipamento. Valem no combate: vida e
 *   mana máximas, dano, crítico, defesa, regeneração e as afinidades da raça (UVortexCombatComponent).
 * - XP por criatura vencida (o Godot: 65% da tabela, +8% por nível dela), dividida com quem estiver
 *   perto; subir de nível enche vida e mana.
 * - Saque e moedas vão direto para a mochila de quem venceu.
 */
UCLASS(ClassGroup=(Vortex), meta=(BlueprintSpawnableComponent))
class VORTEX_API UVortexHeroSheet : public UActorComponent
{
    GENERATED_BODY()
public:
    UVortexHeroSheet();
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    /** Anfitrião: copia a ficha do convidado para o save do mundo (Key: conta da Epic + herói). */
    void StoreGuest();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ---- leitura (qualquer máquina; mochila e XP só no dono)
    int32 GetLevel() const { return Level; }
    int64 GetXp() const { return Xp; }
    int32 GetPointsFree() const;
    int32 GetPoints(int32 Attribute) const { return Points.IsValidIndex(Attribute) ? Points[Attribute] : 0; }
    const TArray<FVortexItemStack>& GetBag() const { return Bag; }
    FName GetEquipped(int32 Slot) const { return Equipped.IsValidIndex(Slot) ? Equipped[Slot] : NAME_None; }
    int32 GetCoins() const { return Coins; }
    int32 CountOf(FName Item) const;
    FName GetRace() const;
    FVortexHeroStats ComputeStats() const;
    /** Família da arma na mão (as técnicas da aba Habilidades). */
    FName GetWeaponFamily() const;
    bool IsLoaded() const { return bLoaded; }
    /** Ainda no Estádio do Despertar (sem a arma inicial escolhida). */
    bool NeedsAwakening() const { return bNeedsAwakening; }
    /** Tipo da arma na mão (um dos 25 do Godot), ou nenhum. */
    FName GetWeaponType() const;
    FName GetClassId() const { return ClassId; }
    /** Pelagem do cavalo domado (o assobio chama), ou nada. */
    FName GetMountCoat() const { return MountCoat; }
    const FString& GetMountName() const { return MountName; }
    /** 03/11: o dono dá o nome ao cavalo (a janela abre sozinha ao domar). */
    UFUNCTION(Server, Reliable) void ServerSetMountName(const FString& Name);
    /** Servidor: acabou de domar; o dono escolhe o nome (abre a janela no computador dele). */
    void AskMountName();
    /** A magia secundária do cajado (02/11): "heal", "vines", "fireball", ou nada (ainda não escolheu). */
    FName GetStaffSpell() const { return StaffSpell; }
    /** Caveira (02/11): matou um herói fora do grupo; fica 3 dias do jogo. Todos veem. */
    bool IsOutlaw() const;
    /** Servidor: marca a caveira por 3 dias do jogo a partir de agora. */
    void MarkOutlaw();
    /** Servidor: domou um cavalo (fica no save). */
    void SetMountCoat(FName Coat) { MountCoat = Coat; Changed(); }
    const TArray<FName>& GetClassCards() const { return ClassCards; }
    /** Já pode fazer o Despertar de classe (nível 10, sem classe). */
    bool CanAwaken() const;
    /** Receita que dá para fazer agora (ingredientes na mochila). */
    bool CanCraft(FName Recipe) const;
    /** Servidor: a raça mudou (visual novo), recalcula vida e mana máximas. */
    void RefreshVitals() { Changed(); }

    /** Avisos para o HUD do dono: "+29 XP", "Pele de lobo x1"... (texto, hora do mundo, tipo). */
    struct FNotice { FString Text; float Time = 0.f; uint8 Kind = 0; };
    const TArray<FNotice>& GetNotices() const { return Notices; }
    /** Muda quando qualquer coisa da ficha muda (para a janela se redesenhar). */
    int32 GetRevision() const { return Revision; }

    // ---- servidor
    void AddXp(int64 Amount);
    /** Põe na mochila (junta com pilhas iguais); devolve quanto não coube. */
    int32 GiveItem(FName Item, int32 Count, bool bNotify = true);
    void AddCoins(int32 Amount);
    /** Servidor: gasta moedas (troca, lojas); false se não tem. */
    bool SpendCoins(int32 Amount);
    /** Tira da mochila (entregas de missão, receitas); devolve quanto tirou. */
    int32 TakeItem(FName Item, int32 Count);
    /** Golpe saindo deste herói: atributo do tipo de golpe, Ataque/Poder de magia, raça e crítico. */
    float OutgoingDamage(float Base, EVortexDamageKind Kind, bool& bOutCrit, bool bForceCrit = false) const;
    /** Golpe chegando: Defesa (físico) ou Resistência (mágico) e a afinidade da raça. */
    float IncomingDamage(float Base, bool bMagic = false);
    /** Testes (servidor): troca a classe sem o Despertar. */
    void DebugSetClass(FName Class) { ClassId = Class; ClassCards.Reset(); Changed(true); }
    /** Servidor: pegou fogo (lava): perde vida aos poucos por tantos segundos (renova, não soma). */
    void ApplyBurn(float Seconds);
    /** Segundos de queimadura que faltam (todas as máquinas: chamas no corpo e o aviso da HUD). */
    float BurnRemaining() const;
    /** Servidor: envenenado por Seconds (a gota peçonhenta): perde vida aos poucos, sem acumular o tempo. */
    void ApplyPoison(float Seconds);
    float PoisonRemaining() const;
    /** Servidor: bênção de um santuário (pontos de interesse): "forca" (ataque), "vida" (vida máxima) ou "sorte"
     *  (crítico e saque), por tantos segundos (renova, não soma). */
    void ApplyBlessing(FName Kind, float Seconds);
    float BlessingRemaining() const;
    FName GetBlessing() const { return BlessingRemaining() > 0.f ? Blessing : NAME_None; }
    /** Servidor: entrou numa dungeon de portal (id e regras, VortexDungeonRules) ou saiu (NAME_None). */
    void SetDungeon(FName Id, int32 Rules);
    FName GetDungeonId() const { return DungeonId; }
    int32 GetDungeonRules() const { return DungeonRules; }
    bool HasDungeonRule(int32 Rule) const { return !DungeonId.IsNone() && ((DungeonRules >> Rule) & 1); }
    /** Roubo de vida: parte do dano causado volta como vida. */
    void Leech(float DamageDealt);
    /** Servidor: cura direto (arma de luz). */
    void HealBy(float Amount);
    /** Aviso para o dono (no servidor manda para o cliente dele). */
    void PushNotice(const FString& Text, uint8 Kind = 0) { Notice(Text, Kind); }
    /** Criatura vencida perto: XP para quem estava por perto; saque e moedas para quem venceu. */
    static void AwardKill(AActor* Killer, AActor* Victim);

    // ---- pedidos do dono
    UFUNCTION(Server, Reliable) void ServerSpendPoint(uint8 Attribute);
    /** Usa (poção, comida) ou veste o item daquele espaço da mochila. */
    UFUNCTION(Server, Reliable) void ServerUseSlot(uint8 BagIndex);
    UFUNCTION(Server, Reliable) void ServerUnequip(uint8 Slot);
    UFUNCTION(Server, Reliable) void ServerMoveSlot(uint8 From, uint8 To);
    /** 07/11: pega o item no chão (AVortexItemDrop) e joga no chão o que está naquele espaço da mochila. */
    UFUNCTION(Server, Reliable) void ServerPickup(AActor* Drop);
    UFUNCTION(Server, Reliable) void ServerDropSlot(uint8 BagIndex);
    /** 07/11: a forja do Bram sobe 1 nível do item vestido naquele lugar (VortexShop::ForgeCost). */
    UFUNCTION(Server, Reliable) void ServerForge(uint8 Slot);
    /** Q e R: a primeira poção daquele tipo. */
    UFUNCTION(Server, Reliable) void ServerQuickUse(FName Item);
    /** Para de beber (atacar, esquivar, pular...): a poção não é gasta. */
    UFUNCTION(Server, Reliable) void ServerCancelDrink();
    /** Bebendo (todas as máquinas): a garrafinha na mão esquerda, anda devagar e não luta. */
    bool IsDrinking() const { return !DrinkItem.IsNone(); }
    FName GetDrinkItem() const { return DrinkItem; }
    /** Segundos desde que começou a beber (relógio do servidor). */
    float DrinkElapsed() const;
    /** Quanto tempo leva para beber (o efeito entra no fim dos goles). */
    static constexpr float DrinkSeconds = 1.25f;
    /** Despertar de classe: tira as 6 cartas (uma vez; ficam guardadas até escolher). */
    UFUNCTION(Server, Reliable) void ServerDrawClasses();
    UFUNCTION(Server, Reliable) void ServerChooseClass(FName Class);
    /** A magia do cajado (só uma vez: escolhida, fica). */
    UFUNCTION(Server, Reliable) void ServerChooseStaffSpell(FName Spell);
    /** Alquimia e bancada: gasta os ingredientes e põe o resultado na mochila. */
    UFUNCTION(Server, Reliable) void ServerCraft(FName Recipe);
    UFUNCTION(Server, Reliable, WithValidation) void ServerLoad(const FVortexSheetSave& Save);
    /** Lojas (VortexShop): compra a oferta Index do comerciante, ou vende Count unidades de Item.
     *  O servidor confere a distância do comerciante, o estoque do dia, as moedas e a mochila. */
    UFUNCTION(Server, Reliable) void ServerShopBuy(FName Vendor, int32 Index);
    UFUNCTION(Server, Reliable) void ServerShopSell(FName Vendor, FName Item, int32 Count);
    /** Quantas desta oferta ainda restam hoje para este herói (as sem limite: muitas). */
    int32 ShopLeft(FName Vendor, int32 Index, int32 Qty, bool bUnlimited) const;
    /** Dia do mundo e mundo das lojas (o mesmo sorteio para todos). */
    int32 ShopWorldDay() const;
    FString ShopWorldId() const;
    /** Testes (servidor): ignora a distância até o comerciante. */
    bool bShopAnywhere = false;
    /** Estádio: pegar (ou devolver) a arma do pedestal perto do herói. */
    UFUNCTION(Server, Reliable) void ServerStadiumTake(FName Type);
    /** Estádio: sair pelo portão dourado com a arma escolhida. */
    UFUNCTION(Server, Reliable) void ServerLeaveStadium();
    /** Caído: Espaço depois de 2 s renasce na hora (sem esperar os 5 s). */
    UFUNCTION(Server, Reliable) void ServerRespawnNow();
    /** Há quanto tempo está caído (servidor). */
    float GetDeadFor() const { return DeadFor; }
    /** Servidor: leva o herói ao estádio (herói novo). */
    void EnterStadium();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Sheet) int32 Level = 1;
    UPROPERTY(ReplicatedUsing = OnRep_Sheet) int64 Xp = 0;
    UPROPERTY(ReplicatedUsing = OnRep_Sheet) TArray<int32> Points;
    UPROPERTY(ReplicatedUsing = OnRep_Sheet) TArray<FVortexItemStack> Bag;
    UPROPERTY(ReplicatedUsing = OnRep_Sheet) TArray<FName> Equipped;
    UPROPERTY(ReplicatedUsing = OnRep_Sheet) int32 Coins = 0;
    UPROPERTY(Replicated) FName DrinkItem;
    UPROPERTY(Replicated) float DrinkStartServer = 0.f;
    FTimerHandle DrinkTimer;
    void FinishDrink();
    UPROPERTY(ReplicatedUsing = OnRep_Sheet) bool bNeedsAwakening = false;
    UPROPERTY(ReplicatedUsing = OnRep_Sheet) FName ClassId;
    UPROPERTY(ReplicatedUsing = OnRep_Sheet) TArray<FName> ClassCards;
    UPROPERTY(ReplicatedUsing = OnRep_Sheet) FName MountCoat;
    UPROPERTY(Replicated) FString MountName;
    UPROPERTY(ReplicatedUsing = OnRep_AskMountName) uint8 AskMountSerial = 0;
    UFUNCTION() void OnRep_AskMountName();
    UPROPERTY(ReplicatedUsing = OnRep_Sheet) FName StaffSpell;
    /** O dia do jogo (dia + hora/24, AVortexGameState) em que a caveira some; 0 sem caveira. */
    UPROPERTY(Replicated) float OutlawUntilDay = 0.f;
    UPROPERTY(ReplicatedUsing = OnRep_Sheet) int32 ShopDay = 0;
    UPROPERTY(ReplicatedUsing = OnRep_Sheet) TArray<FName> ShopBought;
    /** Até quando queima (relógio do servidor que os clientes conhecem). */
    UPROPERTY(Replicated) float BurnServerUntil = 0.f;
    UPROPERTY(Replicated) float PoisonServerUntil = 0.f;
    UPROPERTY(Replicated) FName Blessing;
    UPROPERTY(Replicated) float BlessingServerUntil = 0.f;
    bool bBlessingActive = false;
    UPROPERTY(Replicated) FName DungeonId;
    UPROPERTY(Replicated) int32 DungeonRules = 0;
    float BurnTick = 0.f, PoisonTick = 0.f;
    FVector ReturnLocation = FVector::ZeroVector;
    FTimerHandle LandTimer;
    void LandInValley();

    UFUNCTION() void OnRep_Sheet();
    UFUNCTION(Client, Unreliable) void ClientNotice(const FString& Text, uint8 Kind);
    /** Mapa de viajante usado: o dono copia a rota (Hundreds x 100 m) para a carta (a névoa é dele). */
    UFUNCTION(Client, Reliable) void ClientRevealMap(int32 Hundreds, int32 Seed, const FString& ItemName);
    UFUNCTION(NetMulticast, Unreliable) void MulticastLevelUp(int32 NewLevel);

    void Notice(const FString& Text, uint8 Kind);
    /** Servidor: algo mudou. Recalcula vida/mana máximas e, no dono local, grava. */
    void Changed(bool bRefill = false);
    void ApplyVitals(bool bRefill);
    void EquipWeapon();
    FName EquippedType;
    void TryLoadFromSave();
    void SaveLocal();
    bool IsOwnerLocal() const;
    void EnsureSizes();
    /** Bônus permanente da classe nos oficiais (aba Classes). */
    void AddClassBonus(FVortexHeroStats& Stats) const;

    TArray<FNotice> Notices;
    bool bLoaded = false, bSentLoad = false, bSaveDirty = false;
    /** 09/11: o servidor já aplicou a ficha: só uma vez por herói, e o dono só grava depois disso (antes a mochila ainda está vazia). */
    UPROPERTY(Replicated) bool bServerLoaded = false;
    bool bWasLocal = false, bSaveFailWarned = false;
    bool bGuest = false;
    FString GuestAccount;
    FGuid GuestHero;
    FVortexSheetSave MakeSave() const;
    float SinceDamaged = 100.f, SaveTimer = 0.f, LoadWait = 0.f;
    float DeadFor = 0.f;   // caído: renasce na Fonte do Santuário depois de alguns segundos
    int32 Revision = 0;
};
