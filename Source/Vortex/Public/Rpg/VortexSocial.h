#pragma once

#include "Components/ActorComponent.h"
#include "Rpg/VortexRpg.h"
#include "VortexSocial.generated.h"

class AVortexCharacter;

/** O que um herói põe na mesa da troca: itens da mochila (nada equipado) e moedas. */
USTRUCT()
struct VORTEX_API FVortexTradeOffer
{
    GENERATED_BODY()
    UPROPERTY() TArray<FVortexItemStack> Items;
    UPROPERTY() int32 Coins = 0;
    UPROPERTY() bool bReady = false;
};

/** Pedido recebido de outro jogador (grupo, amizade, duelo, troca), válido por um minuto. */
USTRUCT()
struct VORTEX_API FVortexSocialPending
{
    GENERATED_BODY()
    UPROPERTY() TObjectPtr<AVortexCharacter> From;
    UPROPERTY() FName Kind;
    UPROPERTY() float Until = 0.f;
};

/**
 * Jogador com jogador (MainPvp, MainParty e MainTrade do Godot). Perto de outro herói (3,2 m) e
 * olhando para ele, E abre o menu de interação:
 * - Grupo (até 8): convidar / aceitar / sair. O grupo aparece na HUD embaixo do seu quadro.
 * - Amizade: pedir / aceitar / desfazer; os amigos ficam no save do herói.
 * - Duelo (não mata): aceito o desafio, contagem 3, 2, 1 e a luta; quem chegar a 15% da vida perde,
 *   e os dois voltam à vida de antes. Afastar-se 45 m cancela. 02/11: um domo de energia (AVortexDuelDome) fecha os
 *   dois: eles não saem, ninguém entra e de fora ninguém fere quem está dentro.
 * - Troca: itens e moedas, cada um monta a sua oferta; qualquer mudança desfaz o "pronto" dos dois;
 *   com os dois prontos o servidor confere as mochilas e troca.
 * Tudo é decidido no servidor; cada herói recebe só os pedidos e a troca dele.
 */
UCLASS(ClassGroup=(Vortex), meta=(BlueprintSpawnableComponent))
class VORTEX_API UVortexSocial : public UActorComponent
{
    GENERATED_BODY()
public:
    UVortexSocial();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    static constexpr int32 PartyLimit = 8;
    static constexpr float InteractRange = 320.f;
    static constexpr float DuelLoseAt = .15f;

    // ---- leitura (todas as máquinas)
    int32 GetPartyId() const { return PartyId; }
    FString GetHeroName() const;
    /** 03/11: a cor do jogador (o ping, o minimapa e o mapa), para saber quem é quem. Cada um escolhe a sua com Shift+Z
     *  (fica salva); sem escolha, uma automática diferente para cada jogador. */
    static constexpr int32 PaletteCount = 8;
    static FLinearColor PaletteColor(int32 Index);
    static const TCHAR* PaletteName(int32 Index);
    int32 GetPlayerColorIndex() const;
    FLinearColor GetPlayerColor() const { return PaletteColor(GetPlayerColorIndex()); }
    /** Dono: a próxima cor (salva no GameUserSettings e enviada ao servidor). */
    void CyclePlayerColor();
    bool SameParty(const AActor* Other) const;
    /** Os outros heróis do meu grupo. */
    TArray<AVortexCharacter*> PartyMates() const;
    AVortexCharacter* GetDuelOpponent() const { return DuelOpponent; }
    /** 0 nada, 1 contagem, 2 luta. */
    uint8 GetDuelPhase() const { return DuelPhase; }
    /** Segundos até a luta começar (contagem). */
    float DuelCountdown() const;
    bool IsDuelFighting(const AActor* Other) const;
    AVortexCharacter* GetTradePartner() const { return TradePartner; }
    const FVortexTradeOffer& GetMyOffer() const { return MyOffer; }
    const FVortexTradeOffer& GetTheirOffer() const { return TheirOffer; }
    bool HasPending(const AActor* From, FName Kind) const;
    /** Amigo (save deste computador). */
    static bool IsFriend(const UObject* WorldContext, const FString& Name);

    struct FMenuItem { FName Action; FString Label; bool bEnabled = true; };
    /** As opções do menu de interação com aquele jogador. */
    TArray<FMenuItem> MenuFor(const AVortexCharacter* Other) const;
    /** Ação do menu (a amizade desfeita é só local; o resto vai ao servidor). */
    void Act(AVortexCharacter* Other, FName Action);

    // ---- pedidos ao servidor
    UFUNCTION(Server, Reliable) void ServerAct(AVortexCharacter* Target, FName Action);
    UFUNCTION(Server, Reliable, WithValidation) void ServerSetHeroName(const FString& Name);
    UFUNCTION(Server, Reliable, WithValidation) void ServerTradeOffer(const TArray<FVortexItemStack>& Items, int32 Coins);
    UFUNCTION(Server, Reliable) void ServerTradeReady(bool bReady);
    UFUNCTION(Client, Reliable) void ClientFriend(const FString& Name);
    // (testes chamam direto no servidor)
    void ServerAct_Implementation(AVortexCharacter* Target, FName Action);
    bool ServerSetHeroName_Validate(const FString& Name);
    void ServerSetHeroName_Implementation(const FString& Name);
    bool ServerTradeOffer_Validate(const TArray<FVortexItemStack>& Items, int32 Coins);
    void ServerTradeOffer_Implementation(const TArray<FVortexItemStack>& Items, int32 Coins);
    void ServerTradeReady_Implementation(bool bReady);

    // ---- servidor (combate): o golpe levou este herói a 15% no duelo
    void DuelLost(AVortexCharacter* Winner);

private:
    UPROPERTY(Replicated) int32 PartyId = 0;
    UPROPERTY(Replicated) FString HeroName;
    UPROPERTY(Replicated) uint8 PlayerColor = 255;
    UFUNCTION(Server, Reliable) void ServerSetPlayerColor(uint8 Index);
    UPROPERTY(Replicated) TObjectPtr<AVortexCharacter> DuelOpponent;
    UPROPERTY(Replicated) uint8 DuelPhase = 0;
    UPROPERTY(Replicated) float DuelFightAt = 0.f;
    UPROPERTY(Replicated) TObjectPtr<AVortexCharacter> TradePartner;
    UPROPERTY(Replicated) FVortexTradeOffer MyOffer;
    UPROPERTY(Replicated) FVortexTradeOffer TheirOffer;
    UPROPERTY(Replicated) TArray<FVortexSocialPending> Pending;
    float DuelHealthBefore = -1.f;
    /** O domo de energia do duelo (AVortexDuelDome, só no servidor; os dois lados apontam para o mesmo). */
    TWeakObjectPtr<class AVortexDuelDome> DuelDome;
    bool bNameSent = false;

    AVortexCharacter* Hero() const;
    static UVortexSocial* Of(const AActor* Actor);
    float Now() const;
    void Notice(const FString& Text, uint8 Kind = 0) const;
    void AddPending(AVortexCharacter* From, FName Kind);
    bool TakePending(AVortexCharacter* From, FName Kind);
    void JoinParty(AVortexCharacter* Inviter);
    void LeaveParty();
    void BeginDuel(AVortexCharacter* Other);
    void EndDuel(const FString& Message, AVortexCharacter* Winner);
    void FinishDuelSide(const FString& Message);
    void KeepDome();
    void BeginTrade(AVortexCharacter* Other);
    void EndTrade(const FString& Message);
    void TryCompleteTrade();
};
