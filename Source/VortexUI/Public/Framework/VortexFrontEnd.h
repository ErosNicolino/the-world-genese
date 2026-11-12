#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "Framework/VortexLook.h"
#include "Persistence/VortexSaveSubsystem.h"

class UVortexGameInstance;
class AVortexMenuStage;
class SEditableTextBox;
class SBox;

/**
 * Menu do Vortex (Slate nativo, sem asset de widget): fica transparente por cima do palco 3D
 * (AVortexMenuStage), onde o herói aparece de verdade.
 * Telas: início, mundos, novo mundo, LAN, opções, heróis do mundo (cartas com a arte da raça; a
 * escolhida aparece no palco), seleção de raça (cartas com atributos e traços, animação de escolha e
 * revelação) e criação (estilo por categoria com as peças da raça, cores por paleta, nome com
 * sorteio por raça, giro/zoom/rosto, comemoração ao criar).
 * Brasas sobem pela tela, os botões acendem e deslizam, e cada tela entra com um fade.
 */
class SVortexFrontEnd final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SVortexFrontEnd) {}
        SLATE_ARGUMENT(UVortexGameInstance*, Owner)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
    void SetNotice(const FString& Message) { SetStatus(Message, true); }

    // para os widgets internos
    AVortexMenuStage* Stage() const;
    void Sound(FName Name, float Volume = 1.f) const;
    double Clock() const { return Now; }
    int32 ChosenRaceIndex() const { return ChosenRace; }
    double ChosenAge() const { return ChosenRace == INDEX_NONE ? -1.0 : Now - ChosenAt; }
    const FVortexLook& CurrentLook() const { return Look; }

private:
    enum class EPage : uint8 { Home, Worlds, CreateWorld, Join, Options, Heroes, DeleteHero, Races, Creator, Lobby, DeleteWorld, Multiplayer, Achievements };
    void Go(EPage NewPage);
    TSharedRef<SWidget> BuildHome();
    TSharedRef<SWidget> BuildWorlds();
    TSharedRef<SWidget> BuildCreateWorld();
    TSharedRef<SWidget> BuildJoin();
    TSharedRef<SWidget> BuildOptions();
    TSharedRef<SWidget> BuildHeroes();
    TSharedRef<SWidget> BuildDeleteHero();
    TSharedRef<SWidget> BuildDeleteWorld();
    void ConfirmDeleteWorld();
    FGuid PendingWorldDelete;
    FString PendingWorldName;
    TSharedRef<SWidget> BuildRaces();
    TSharedRef<SWidget> BuildCreator();
    TSharedRef<SWidget> BuildLobby();
    /** Multiplayer: criar a sala (mundos de multiplayer) ou entrar na de um amigo. */
    TSharedRef<SWidget> BuildMultiplayer();
    /** Conquistas, tiradas dos heróis salvos neste computador. */
    TSharedRef<SWidget> BuildAchievements();
    /** Jogar = mundos de um jogador (1 herói); Multiplayer = mundos com sala para amigos (até 3 heróis). */
    bool bMultiplayerMode = false;
    /** O mundo escolhido é de multiplayer (a tela de heróis mostra 3 vagas; o de um jogador, 1). */
    bool bSelectedWorldMulti = false;
    void OpenSingle(FGuid WorldId);
    /** Jogadores da sala: o anfitrião primeiro, depois na ordem de chegada. */
    TArray<class AVortexPlayerState*> LobbyPlayers() const;
    class AVortexPlayerState* LocalLobbyState() const;
    void TickLobby();
    void OpenTravellers();
    bool bTravellerMode = false;   // heróis viajantes (os que entram no mundo de um amigo)
    bool bLobbyInput = false;
    double LobbyRefreshAt = 0.0;
    TSharedRef<SWidget> GlassPanel(const FString& Title, const FString& Subtitle, TSharedRef<SWidget> Body, float Width = 640.f);
    TSharedRef<SWidget> Button(const FString& Text, FSimpleDelegate Action, int32 Kind = 0, float Width = 0.f, const FSlateBrush* Icon = nullptr);
    TSharedRef<SWidget> TextField(TSharedPtr<SEditableTextBox>& Out, const FString& Hint, const FString& Initial = FString());

    void SetStatus(const FString& Message, bool bError = false);
    void ContinueLast();
    void CreateWorld();
    void Connect();
    /** bOnline: a sala abre pela Epic (amigos de qualquer casa); sem ele, na rede local. */
    void OpenHeroes(FGuid WorldId, bool bHost, bool bOnline = false);
    void SelectHero(FGuid PlayerId);
    void PlayHero();
    void ConfirmDeleteHero();
    int32 OptionsTab = 0;
    void StartCreation();
    void ChooseRace(int32 Index);
    void ShowLookOnStage();
    void CyclePiece(FName Category, int32 Direction);
    void SetColor(FName Slot, FColor Color);
    /** O cabelo do humano de blocos na criação (01/11): o estilo, as cores prontas e a cor livre. */
    TSharedRef<SWidget> BuildHairPanel();
    void SetHairStyle(FName Id);
    void SetHairColor(FColor Color, bool bFromPicker);
    /** A cor livre em matiz (0-360), saturação e brilho, guardada aqui: ida e volta pelo RGB perdia a matiz no cinza e no
     *  preto, e o seletor pulava. */
    FLinearColor HairHsv = FLinearColor(18.f, .68f, .47f, 1.f);
    void SetAppearance(FName Appearance);
    void Randomize(bool bOnlyColors);
    void RandomName();
    void SaveHero();
    FString RaceName(FName RaceId) const;

    TWeakObjectPtr<UVortexGameInstance> Owner;
    TSharedPtr<SBox> PageHost;
    EPage Page = EPage::Home;
    TFunction<void()> PageBack;   // o que o Esc faz nesta página (vazio: nada)
    double Now = 0.0, PageAt = 0.0, ReadyAt = 0.0;
    bool bStageReady = false;
    FString Status;
    bool bStatusError = false;
    double StatusAt = -100.0;

    FGuid SelectedWorldId, SelectedHeroId, PendingDelete;
    bool bSelectedWorldHost = false;
    /** "Online" nos mundos: ao jogar, abre a sala da Epic antes do mundo. */
    bool bSelectedWorldOnline = false;
    void JoinOnlineRoom();
    void OpenOnline(FGuid WorldId);
    FString SelectedWorldName;
    TArray<FVortexHeroSummary> Heroes;

    FVortexLook Look;
    FString HeroName;
    int32 ChosenRace = INDEX_NONE;
    bool bRaceFromCreator = false;
    double ChosenAt = -100.0;
    double CelebrateAt = -100.0;
    FString CelebrateName;
    FGuid CreatedHeroId;
    FRandomStream Rng;

    TSharedPtr<SEditableTextBox> NameBox, SeedBox, AddressBox, HeroNameBox, PickerHex, RoomCodeBox;
    FString LastOnlineStatus;
    FName PickerSlot;   // cor aberta no seletor livre (nenhuma = fechado)
    FSlateBrush EmblemBrush;
    // arte do menu (SourceArt/Menu/MenuArt.blend -> /Game/Vortex/UI/Menu/Art)
    FSlateBrush LogoBrush, IconPlay, IconJoin, IconOptions, IconQuit, IconTrophy, WorldArt;
    // ícones dos botões (Data/UI/Menu, gerados no ChatGPT; Tools/ImportMenuIcons.py)
    FSlateBrush IconHost, IconKey, IconNewWorld, IconNewHero, IconDelete, IconBack, IconReady;
    TMap<FGuid, FSlateBrush> PortraitBrushes;
    const FSlateBrush* Portrait(const FGuid& PlayerId);
    /** Cabeçalho comum: "‹ Voltar", o título na Cinzel e a linha de baixo. */
    TSharedRef<SWidget> PageHeader(const FString& Title, const FString& Subtitle, TFunction<void()> Back);
    /** "Novo mundo" aberto dentro da própria lista de mundos. */
    bool bCreatingWorld = false;
    /** Último mundo jogado e o herói dele (o cartão "Continuar" do início). */
    void ContinueWith(FGuid WorldId, FGuid PlayerId);
    TMap<FName, FSlateBrush> RaceArt;

#if !UE_BUILD_SHIPPING
    int32 ShotStep = -1;
    double ShotAt = 0.0;
    void TickShot();
#endif
};
