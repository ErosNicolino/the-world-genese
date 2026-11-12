#include "Framework/VortexDevChat.h"
#include "VortexUiImpl.h"
#include "World/VortexPortal.h"
#include "World/VortexPointsOfInterest.h"

#include "Framework/VortexCharacter.h"
#include "Framework/VortexGameState.h"
#include "Framework/VortexHeroWindow.h"
#include "Framework/VortexUiKit.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexCombatComponent.h"
#include "Creatures/VortexCreature.h"
#include "Creatures/VortexMonsterSpecies.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexRpg.h"
#include "World/VortexCreatureSubsystem.h"
#include "World/VortexFairies.h"
#include "World/VortexWeatherSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "World/VortexLandmarks.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace VortexDevChatUi
{
    struct FLine { FString Text; FLinearColor Color; double Time = 0.0; };
    TArray<FLine> Lines;
    TArray<FString> History;
    constexpr int32 ShownLines = 12;
    constexpr double LineSeconds = 8.0;

    double Now() { return FPlatformTime::Seconds(); }

    class SVortexDevChat : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SVortexDevChat) {}
        SLATE_END_ARGS()

        TWeakObjectPtr<AVortexCharacter> Hero;
        TSharedPtr<SEditableTextBox> Input;
        bool bTyping = false;
        double OpenedAt = 0.0;
        int32 HistoryIndex = INDEX_NONE;

        void Construct(const FArguments&)
        {
            // fica na tela o jogo todo, por cima das janelas: invisível ao mouse (só a caixa aberta pega clique).
            // Antes cobria a tela inteira e engolia os cliques do inventário e do diálogo (que viravam ataque)
            SetVisibility(EVisibility::SelfHitTestInvisible);
            const FSlateFontInfo Font = VortexUiKit::Outlined(FCoreStyle::GetDefaultFontStyle("Bold", 15));
            TSharedRef<SVerticalBox> Log = SNew(SVerticalBox);
            for (int32 Row = 0; Row < ShownLines; ++Row)
            {
                // a linha Row conta de cima: a de baixo é a mais nova
                Log->AddSlot().AutoHeight().Padding(0.f, 1.f)
                [
                    SNew(STextBlock).Font(Font).AutoWrapText(true)
                        .Text_Lambda([this, Row]() { const FLine* Line = LineAt(Row); return Line ? FText::FromString(Line->Text) : FText::GetEmpty(); })
                        .ColorAndOpacity_Lambda([this, Row]() { const FLine* Line = LineAt(Row); return FSlateColor(Line ? Line->Color.CopyWithNewOpacity(Line->Color.A * Fade(*Line)) : FLinearColor::Transparent); })
                        .Visibility_Lambda([this, Row]() { return LineAt(Row) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
                ];
            }
            ChildSlot
            .HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(24.f, 0.f, 0.f, 210.f)
            [
                SNew(SBox).WidthOverride(620.f).Visibility(EVisibility::SelfHitTestInvisible)
                [
                    SNew(SVerticalBox).Visibility(EVisibility::SelfHitTestInvisible)
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(SBorder)
                        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .BorderBackgroundColor_Lambda([this]() { return FLinearColor(0.f, 0.f, .02f, bTyping ? .55f : 0.f); })
                        .Padding(10.f, 6.f)
                        .Visibility(EVisibility::HitTestInvisible)
                        [ Log ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
                    [
                        SAssignNew(Input, SEditableTextBox)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                        .BackgroundColor(FLinearColor(.02f, .02f, .05f, .9f))
                        .ForegroundColor(FLinearColor::White)
                        .HintText(FText::FromString(TEXT("comando (/help)")))
                        .ClearKeyboardFocusOnCommit(false)
                        .RevertTextOnEscape(false)
                        .OnTextChanged(this, &SVortexDevChat::Changed)
                        .OnTextCommitted(this, &SVortexDevChat::Committed)
                        .Visibility_Lambda([this]() { return bTyping ? EVisibility::Visible : EVisibility::Collapsed; })
                    ]
                ]
            ];
        }

        // com a caixa aberta, todas as últimas linhas; fechada, só as recentes (somem devagar)
        const FLine* LineAt(int32 Row) const
        {
            const int32 Index = Lines.Num() - ShownLines + Row;
            if (!Lines.IsValidIndex(Index)) return nullptr;
            const FLine& Line = Lines[Index];
            return bTyping || Now() - Line.Time < LineSeconds ? &Line : nullptr;
        }

        float Fade(const FLine& Line) const
        {
            return bTyping ? 1.f : FMath::Clamp(float(LineSeconds - (Now() - Line.Time)) / 1.5f, 0.f, 1.f);
        }

        void Begin()
        {
            bTyping = true;
            OpenedAt = Now();
            HistoryIndex = INDEX_NONE;
            SetInput(TEXT("/"));
        }

        void SetInput(const FString& Text)
        {
            Input->SetText(FText::FromString(Text));
            Input->GoTo(ETextLocation::EndOfDocument);
        }

        void Changed(const FText& Text)
        {
            // o "t" que abriu o chat chega depois, como letra digitada: não entra no comando
            if (Now() - OpenedAt < .25 && Text.ToString().Equals(TEXT("/t"), ESearchCase::IgnoreCase)) SetInput(TEXT("/"));
        }

        void Committed(const FText& Text, ETextCommit::Type How)
        {
            if (!bTyping) return;
            if (How == ETextCommit::OnEnter) Submit(Text.ToString().TrimStartAndEnd());
            if (How == ETextCommit::OnEnter || How == ETextCommit::OnCleared || How == ETextCommit::OnUserMovedFocus) VortexDevChat::Close();
        }

        virtual FReply OnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override
        {
            // setas: volta aos comandos já usados
            if (bTyping && History.Num() > 0 && (Event.GetKey() == EKeys::Up || Event.GetKey() == EKeys::Down))
            {
                const int32 Step = Event.GetKey() == EKeys::Up ? -1 : 1;
                HistoryIndex = HistoryIndex == INDEX_NONE ? (Step < 0 ? History.Num() - 1 : INDEX_NONE) : HistoryIndex + Step;
                if (!History.IsValidIndex(HistoryIndex)) HistoryIndex = INDEX_NONE;
                SetInput(HistoryIndex == INDEX_NONE ? TEXT("/") : History[HistoryIndex]);
                return FReply::Handled();
            }
            return SCompoundWidget::OnPreviewKeyDown(Geometry, Event);
        }

        void Submit(const FString& Line)
        {
            if (Line.IsEmpty() || Line == TEXT("/")) return;
            History.Remove(Line);
            History.Add(Line);
            if (History.Num() > 30) History.RemoveAt(0);
            VortexDevChat::AddLine(TEXT("> ") + Line);
            AVortexCharacter* Owner = Hero.Get();
            if (!Owner) return;
            TArray<FString> Args;
            Line.ParseIntoArrayWS(Args);
            FString Command = Args[0].ToLower();
            Command.RemoveFromStart(TEXT("/"));
            APlayerController* Controller = Cast<APlayerController>(Owner->GetController());
            // estes rodam aqui mesmo; o resto é do servidor
            if (Command == TEXT("help") || Command == TEXT("ajuda"))
            {
                for (const TCHAR* Help : {
                    TEXT("/deus  liga/desliga: a vida não desce            /voar  voa para onde a câmera olha (W)"),
                    TEXT("/curar  vida e mana cheias          /nivel N   /xp N   /moedas N"),
                    TEXT("/item id [qtd]  ex.: /item pocao_de_vida 5     /itens [palavra]  procura o id"),
                    TEXT("/hora H  (6 nascer, 18 pôr do sol)   /manha  /meiodia  /tarde  /noite"),
                    TEXT("/relogio N  velocidade do dia (1 normal, 0 parado, 60 rápido)"),
                    TEXT("/dia N  vai para o dia N        /estacao primavera|verao|outono|inverno"),
                    TEXT("/clima chuva|tempestade|neve|nevasca|neblina|nublado|limpo|normal"),
                    TEXT("/velocidade N  o jogo todo mais lento ou rápido (0,1 a 5; 1 normal)"),
                    TEXT("/ir X Y (metros)  /ir 1..9  /ir deserto  /ir yggdrasil  (/ir lista)     /onde"),
                    TEXT("/spawn nome [nível] [qtd]  ex.: /spawn gota de brasa 5 3   /spawn fada   (/spawn lista)"),
                    TEXT("/matar [metros]  vence os hostis em volta (30 m)"),
                    TEXT("/portal [E-S]  abre um portal de dungeon     /poi [ruinas torre santuario mina cemiterio caravana cais]"),
                    TEXT("/fps   /hud   /limpar (o chat)") })
                {
                    VortexDevChat::AddLine(Help);
                }
                return;
            }
            if (Command == TEXT("clear") || Command == TEXT("limpar")) { Lines.Reset(); return; }
            // o clima é de cada máquina (cada uma desenha o seu): roda aqui mesmo
            if (Command == TEXT("clima") || Command == TEXT("weather"))
            {
                UVortexWeatherSubsystem* Weather = Owner->GetWorld()->GetSubsystem<UVortexWeatherSubsystem>();
                const FString Want = Args.IsValidIndex(1) ? Args[1].ToLower() : FString();
                static const TPair<const TCHAR*, EVortexWeather> Names[] = { { TEXT("chuva"), EVortexWeather::Rain }, { TEXT("tempestade"), EVortexWeather::Storm },
                    { TEXT("neve"), EVortexWeather::Snow }, { TEXT("nevasca"), EVortexWeather::Blizzard }, { TEXT("neblina"), EVortexWeather::Fog },
                    { TEXT("nublado"), EVortexWeather::Cloudy }, { TEXT("limpo"), EVortexWeather::Clear } };
                if (!Weather) { VortexDevChat::AddLine(TEXT("Sem clima neste mapa."), true); return; }
                if (Want == TEXT("normal")) { Weather->ClearForcedWeather(); VortexDevChat::AddLine(TEXT("Clima de volta ao normal (muda sozinho).")); return; }
                // "neve" e "nevasca" começam igual: o nome inteiro primeiro, depois as 4 primeiras letras
                for (int32 Pass = 0; Pass < 2; ++Pass)
                    for (const auto& Entry : Names)
                        if (Pass == 0 ? Want == Entry.Key : (Want.Len() >= 4 && FString(Entry.Key).StartsWith(Want.Left(4))))
                        {
                            Weather->ForceWeather(Entry.Value);
                            VortexDevChat::AddLine(FString::Printf(TEXT("Clima: %s (até /clima normal)."), UVortexWeatherSubsystem::WeatherName(Entry.Value)));
                            return;
                        }
                VortexDevChat::AddLine(TEXT("Uso: /clima chuva|tempestade|neve|nevasca|neblina|nublado|limpo|normal"), true);
                return;
            }
            if (Command == TEXT("fps")) { if (Controller) Controller->ConsoleCommand(TEXT("stat fps")); return; }
            if (Command == TEXT("hud")) { if (Controller && Controller->GetHUD()) Controller->GetHUD()->ShowHUD(); return; }
            if (Command == TEXT("pos") || Command == TEXT("onde"))
            {
                const FVector At = Owner->GetActorLocation() / 100.f;
                const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
                const FString Region = Terrain ? Terrain->RegionName(Terrain->RegionAt(At.X * 100.f, At.Y * 100.f)) : FString();
                VortexDevChat::AddLine(FString::Printf(TEXT("X %.0f  Y %.0f  altura %.0f (metros)  %s"), At.X, At.Y, At.Z, *Region));
                return;
            }
            Owner->ServerDevCommand(Line);
        }
    };

    TSharedPtr<SVortexDevChat> Widget;
    TWeakObjectPtr<APlayerController> Owner;

    // o widget fica na tela o jogo todo (mostra as respostas mesmo com a caixa fechada); a troca de mapa o tira
    SVortexDevChat* Ensure()
    {
        if (!GEngine || !GEngine->GameViewport) return nullptr;
        if (!Widget.IsValid() || !Widget->IsParentValid())
        {
            Widget = SNew(SVortexDevChat);
            GEngine->GameViewport->AddViewportWidgetContent(Widget.ToSharedRef(), 35);   // atrás das janelas (inventário, diálogo: 40)
        }
        return Widget.Get();
    }
}

namespace VortexDevChatPlaces
{
    // minúsculas e sem acento: "Solcânion" e "solcanion" são o mesmo lugar
    FString Fold(const FString& In)
    {
        FString Out = In.ToLower();
        static const TPair<const TCHAR*, const TCHAR*> Marks[] = {
            { TEXT("á"), TEXT("a") }, { TEXT("à"), TEXT("a") }, { TEXT("â"), TEXT("a") }, { TEXT("ã"), TEXT("a") },
            { TEXT("é"), TEXT("e") }, { TEXT("ê"), TEXT("e") }, { TEXT("í"), TEXT("i") }, { TEXT("ó"), TEXT("o") },
            { TEXT("ô"), TEXT("o") }, { TEXT("õ"), TEXT("o") }, { TEXT("ú"), TEXT("u") }, { TEXT("ç"), TEXT("c") } };
        for (const TPair<const TCHAR*, const TCHAR*>& Mark : Marks) Out.ReplaceInline(Mark.Key, Mark.Value);
        return Out;
    }

    // apelidos das regiões pelo tipo de terra (1 Valdora ... 9 Verdalume)
    int32 RegionAlias(const FString& Word)
    {
        static const TPair<const TCHAR*, int32> Aliases[] = {
            { TEXT("inicio"), 1 }, { TEXT("vila"), 1 }, { TEXT("campos"), 1 },
            { TEXT("vulcao"), 2 }, { TEXT("lava"), 2 }, { TEXT("fogo"), 2 }, { TEXT("cinzas"), 2 },
            { TEXT("montanha"), 3 }, { TEXT("montanhas"), 3 }, { TEXT("tempestade"), 3 },
            { TEXT("neve"), 4 }, { TEXT("gelo"), 4 }, { TEXT("frio"), 4 },
            { TEXT("deserto"), 5 }, { TEXT("areia"), 5 }, { TEXT("canion"), 5 },
            { TEXT("dourado"), 6 }, { TEXT("ouro"), 6 }, { TEXT("outono"), 6 },
            { TEXT("pantano"), 7 }, { TEXT("lama"), 7 }, { TEXT("brejo"), 7 },
            { TEXT("sombrio"), 8 }, { TEXT("roxo"), 8 }, { TEXT("morte"), 8 },
            { TEXT("floresta"), 9 }, { TEXT("elfos"), 9 }, { TEXT("elfica"), 9 } };
        for (const TPair<const TCHAR*, int32>& Alias : Aliases) if (Word == Alias.Key) return Alias.Value;
        return 0;
    }

    /** O lugar pedido: número da região (1-9), nome ou apelido da região, lugar marcado ou chefe. */
    bool Find(const FVortexTerrainData* Terrain, const FString& Raw, FVector2D& OutAt, FString& OutName)
    {
        const FString Word = Fold(Raw);
        int32 Region = Word.IsNumeric() ? FCString::Atoi(*Word) : RegionAlias(Word);
        if (Terrain && Terrain->HasWorld())
        {
            for (int32 Id = 1; Region == 0 && Id < 10; ++Id) if (Fold(Terrain->RegionName(Id)).StartsWith(Word)) Region = Id;
            if (Region >= 1 && Region <= 9)
            {
                OutAt = Terrain->RegionCenter(Region);
                OutName = Terrain->RegionName(Region);
                return true;
            }
        }
        for (const AVortexLandmarks::FSite& Site : AVortexLandmarks::Sites())
        {
            if (Fold(Site.Id).StartsWith(Word) || Fold(Site.Name).StartsWith(Word))
            {
                // na borda do lugar, não no meio da árvore/vila
                OutAt = Site.Center + FVector2D(0.f, Site.Radius * .6f);
                OutName = Site.Name;
                return true;
            }
        }
        static const TPair<const TCHAR*, FVector2D> Bosses[] = {
            { TEXT("escorpiao"), FVector2D(-218000.f, -109000.f) }, { TEXT("titan"), FVector2D(296000.f, 70500.f) } };
        for (const TPair<const TCHAR*, FVector2D>& Boss : Bosses)
        {
            if (FString(Boss.Key).StartsWith(Word))
            {
                OutAt = Boss.Value + FVector2D(2500.f, 0.f);
                OutName = FString::Printf(TEXT("covil do chefe (%s)"), Boss.Key);
                return true;
            }
        }
        return false;
    }

    FString List(const FVortexTerrainData* Terrain)
    {
        FString Out = TEXT("Lugares:");
        if (Terrain && Terrain->HasWorld()) for (int32 Id = 1; Id < 10; ++Id) Out += FString::Printf(TEXT(" %d %s,"), Id, *Terrain->RegionName(Id));
        Out += TEXT(" yggdrasil, dunas, cacadores, seryth, thorun, escorpiao, titan. Também: deserto, neve, vulcao, pantano, floresta...");
        return Out;
    }
}

namespace VortexDevChatImpl
{
    bool IsTyping() { return VortexDevChatUi::Widget.IsValid() && VortexDevChatUi::Widget->bTyping; }

    void Open(AVortexCharacter* Hero)
    {
        if (IsTyping() || !Hero) return;
        APlayerController* Controller = Cast<APlayerController>(Hero->GetController());
        VortexDevChatUi::SVortexDevChat* Chat = Controller ? VortexDevChatUi::Ensure() : nullptr;
        if (!Chat) return;
        Chat->Hero = Hero;
        Chat->Begin();
        VortexDevChatUi::Owner = Controller;
        FInputModeUIOnly Mode;
        Mode.SetWidgetToFocus(Chat->Input);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
        Controller->SetInputMode(Mode);
        if (VortexDevChatUi::Lines.Num() == 0) AddLine(TEXT("Comandos de teste. /help mostra a lista."));
    }

    void Close()
    {
        if (!IsTyping()) return;
        VortexDevChatUi::Widget->bTyping = false;
        if (APlayerController* Controller = VortexDevChatUi::Owner.Get()) Controller->SetInputMode(FInputModeGameOnly());
        VortexDevChatUi::Owner.Reset();
    }

    void AddLine(const FString& Text, bool bError)
    {
        VortexDevChatUi::Lines.Add({ Text, bError ? FLinearColor(1.f, .45f, .35f) : FLinearColor(.95f, .93f, .85f), VortexDevChatUi::Now() });
        if (VortexDevChatUi::Lines.Num() > 60) VortexDevChatUi::Lines.RemoveAt(0);
        VortexDevChatUi::Ensure();
    }

    // /spawn em português: o nome do bicho no jogo (o título: "gota de brasa", "lobo da bruma", "azhur") ou o id do
    // Godot ("SlimeFire"), sem acento, maiúscula nem espaço; um pedaço basta quando só um bicho tem aquele pedaço
    namespace Spawnable
    {
        FString Plain(const FString& In)
        {
            static const TCHAR* From = TEXT("áàâãäéèêëíìîïóòôõöúùûüçñ");
            static const TCHAR* To = TEXT("aaaaaeeeeiiiiooooouuuucn");
            FString Out;
            for (TCHAR C : In.ToLower())
            {
                if (const TCHAR* Hit = FCString::Strchr(From, C)) C = To[Hit - From];
                if (FChar::IsAlnum(C)) Out.AppendChar(C);
            }
            return Out;
        }
        struct FEntry { FName Id; FString Title; };
        const TArray<FEntry>& All()
        {
            static TArray<FEntry> List;
            if (List.IsEmpty())
            {
                for (const FName& Id : FVortexSpeciesDef::All())
                    if (const FVortexSpeciesDef* Def = FVortexSpeciesDef::Find(Id)) List.Add({ Id, Def->Title.ToLower() });
                List.Add({ TEXT("Wolf"), TEXT("lobo da bruma") });   // o lobo tem classe própria (AVortexWolf)
                List.Add({ TEXT("Fada"), TEXT("fada") });            // enfeite (AVortexFairies), não é criatura
            }
            return List;
        }
        /** Os que combinam com o nome: o exato primeiro; senão todos que contêm o pedaço. */
        TArray<const FEntry*> Find(const FString& Name)
        {
            const FString Want = Plain(Name);
            TArray<const FEntry*> Exact, Partial;
            for (const FEntry& E : All())
            {
                const FString Id = Plain(E.Id.ToString()), Title = Plain(E.Title);
                if (Want == Id || Want == Title) Exact.Add(&E);
                else if (!Want.IsEmpty() && (Title.Contains(Want) || Id.Contains(Want))) Partial.Add(&E);
            }
            return Exact.IsEmpty() ? Partial : Exact;
        }
    }

    FString RunOnServer(AVortexCharacter* Hero, const FString& Line, bool& bOutError)
    {
        bOutError = false;
        TArray<FString> Args;
        Line.TrimStartAndEnd().ParseIntoArrayWS(Args);
        UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
        UAbilitySystemComponent* ASC = Hero ? Hero->GetAbilitySystemComponent() : nullptr;
        if (!Sheet || !ASC || Args.Num() == 0) return FString();
        FString Command = Args[0].ToLower();
        Command.RemoveFromStart(TEXT("/"));
        auto Fail = [&bOutError](const FString& Text) { bOutError = true; return Text; };
        auto Number = [&Args](int32 Index, float Default) { return Args.IsValidIndex(Index) && Args[Index].IsNumeric() ? FCString::Atof(*Args[Index]) : Default; };
        auto FillVitals = [ASC]()
        {
            ASC->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), ASC->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute()));
            ASC->SetNumericAttributeBase(UVortexAttributes::GetManaAttribute(), ASC->GetNumericAttribute(UVortexAttributes::GetMaxManaAttribute()));
        };

        if (Command == TEXT("god") || Command == TEXT("deus"))
        {
            UVortexAttributes* Vitals = const_cast<UVortexAttributes*>(Hero->GetVitals());
            if (!Vitals) return Fail(TEXT("Sem atributos."));
            Vitals->bGodMode = !Vitals->bGodMode;
            if (Vitals->bGodMode) FillVitals();
            return Vitals->bGodMode ? TEXT("Modo deus LIGADO: a vida não desce.") : TEXT("Modo deus desligado.");
        }
        if (Command == TEXT("fly") || Command == TEXT("voar"))
        {
            const bool bOn = !Hero->IsDevFlying();
            Hero->SetDevFly(bOn);
            Hero->ClientDevFly(bOn);
            return bOn ? TEXT("Voo LIGADO: W vai para onde a câmera olha. /fly de novo para cair.") : TEXT("Voo desligado.");
        }
        if (Command == TEXT("level") || Command == TEXT("nivel"))
        {
            if (!Args.IsValidIndex(1)) return Fail(TEXT("Uso: /level N (1 a 100)"));
            const int32 Want = FMath::Clamp(FMath::RoundToInt(Number(1, 1.f)), 1, 100);
            const int64 Missing = FVortexRpgCatalog::XpForLevel(Want) - Sheet->GetXp();
            if (Missing <= 0) return Fail(FString::Printf(TEXT("Já está no nível %d (o nível só sobe)."), Sheet->GetLevel()));
            Sheet->AddXp(Missing);
            return FString::Printf(TEXT("Nível %d."), Sheet->GetLevel());
        }
        if (Command == TEXT("xp"))
        {
            const int64 Amount = FMath::Max<int64>(1, int64(Number(1, 1000.f)));
            Sheet->AddXp(Amount);
            return FString::Printf(TEXT("+%lld XP (nível %d)."), Amount, Sheet->GetLevel());
        }
        if (Command == TEXT("coins") || Command == TEXT("moedas"))
        {
            const int32 Amount = FMath::Clamp(FMath::RoundToInt(Number(1, 1000.f)), 1, 1000000);
            Sheet->AddCoins(Amount);
            return FString::Printf(TEXT("+%d moedas."), Amount);
        }
        if (Command == TEXT("give") || Command == TEXT("item"))
        {
            if (!Args.IsValidIndex(1)) return Fail(TEXT("Uso: /give item [quantidade]"));
            const FName Item(*Args[1]);
            if (!FVortexRpgCatalog::Get().Item(Item)) return Fail(FString::Printf(TEXT("Item \"%s\" não existe (use o id do catálogo)."), *Args[1]));
            const int32 Count = FMath::Clamp(FMath::RoundToInt(Number(2, 1.f)), 1, 999);
            const int32 Left = Sheet->GiveItem(Item, Count);
            return Left > 0 ? FString::Printf(TEXT("%s x%d (%d não couberam na mochila)."), *Item.ToString(), Count - Left, Left)
                : FString::Printf(TEXT("%s x%d."), *Item.ToString(), Count);
        }
        if (Command == TEXT("heal") || Command == TEXT("curar"))
        {
            FillVitals();
            return TEXT("Vida e mana cheias.");
        }
        // 12/11: atalhos do dia, o relógio, o dia e a estação (a estação vem do dia: 3 dias cada; o dia 1 começa a primavera)
        {
            AVortexGameState* State = Hero->GetWorld()->GetGameState<AVortexGameState>();
            static const TPair<const TCHAR*, float> Moments[] = { { TEXT("manha"), 8.f }, { TEXT("meiodia"), 12.f }, { TEXT("tarde"), 17.f }, { TEXT("noite"), 22.f } };
            for (const auto& Moment : Moments)
                if (Command == Moment.Key && State) { State->SetDayHour(Moment.Value); return FString::Printf(TEXT("Hora: %.0f."), Moment.Value); }
            if (Command == TEXT("relogio") && State)
            {
                State->SetDayTimeScale(FMath::Clamp(Number(1, 1.f), 0.f, 600.f));
                return FString::Printf(TEXT("Relógio do dia %.1fx."), State->GetDayTimeScale());
            }
            if ((Command == TEXT("dia") || Command == TEXT("estacao")) && State)
            {
                int32 Day = State->GetDayNumber();
                if (Command == TEXT("dia")) Day = FMath::Clamp(FMath::RoundToInt(Number(1, float(Day))), 1, 9999);
                else
                {
                    static const TCHAR* Seasons[] = { TEXT("prim"), TEXT("ver"), TEXT("out"), TEXT("inv") };
                    int32 Want = -1;
                    for (int32 i = 0; i < 4; ++i) if (Args.IsValidIndex(1) && Args[1].ToLower().StartsWith(Seasons[i])) Want = i;
                    if (Want < 0) return Fail(TEXT("Uso: /estacao primavera|verao|outono|inverno"));
                    Day = Want * 3 + 1;
                }
                State->SetDayClock(AVortexGameState::ClockOfHour(State->GetHour()), Day);
                return FString::Printf(TEXT("Dia %d (%s)."), Day, UVortexWeatherSubsystem::SeasonName(((Day - 1) / 3) % 4));
            }
        }
        if (Command == TEXT("velocidade") || Command == TEXT("speed"))
        {
            const float Speed = FMath::Clamp(Number(1, 1.f), .1f, 5.f);
            UGameplayStatics::SetGlobalTimeDilation(Hero->GetWorld(), Speed);
            return FString::Printf(TEXT("Jogo a %.1fx."), Speed);
        }
        if (Command == TEXT("itens") || Command == TEXT("items"))
        {
            const FString Want = Args.IsValidIndex(1) ? Spawnable::Plain(Args[1]) : FString();
            TArray<FString> Found;
            for (const TPair<FName, FVortexItemDef>& Pair : FVortexRpgCatalog::Get().Items)
                if (Want.IsEmpty() || Spawnable::Plain(Pair.Key.ToString()).Contains(Want) || Spawnable::Plain(Pair.Value.Name).Contains(Want))
                    Found.Add(Pair.Key.ToString());
            if (Found.IsEmpty()) return Fail(TEXT("Nenhum item com isso. /itens sozinho lista todos."));
            Found.Sort();
            const int32 Total = Found.Num();
            if (Total > 40) Found.SetNum(40);
            return FString::Printf(TEXT("%d itens%s: %s"), Total, Total > 40 ? TEXT(" (os 40 primeiros)") : TEXT(""), *FString::Join(Found, TEXT(", ")));
        }
        if (Command == TEXT("time") || Command == TEXT("hora"))
        {
            AVortexGameState* State = Hero->GetWorld()->GetGameState<AVortexGameState>();
            if (!State || !Args.IsValidIndex(1)) return Fail(TEXT("Uso: /time H (0 a 24)"));
            const float Hour = FMath::Clamp(Number(1, 12.f), 0.f, 24.f);
            State->SetDayHour(Hour);
            return FString::Printf(TEXT("Hora: %.1f."), Hour);
        }
        if (Command == TEXT("tp") || Command == TEXT("ir"))
        {
            const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
            if (!Args.IsValidIndex(1)) return VortexDevChatPlaces::List(Terrain.Get());
            FVector2D At;
            FString Name;
            if (Args.IsValidIndex(2) && Args[1].IsNumeric() && Args[2].IsNumeric()) At = FVector2D(Number(1, 0.f), Number(2, 0.f)) * 100.f;
            else if (!VortexDevChatPlaces::Find(Terrain.Get(), Args[1], At, Name))
                return Fail(FString::Printf(TEXT("Lugar \"%s\" não existe. /tp sozinho lista os lugares."), *Args[1]));
            FVector To(At.X, At.Y, Hero->GetActorLocation().Z);
            if (Terrain)
            {
                // o chão de lá (ou a superfície da água), mesmo que o pedaço ainda não tenha sido montado
                To.Z = FMath::Max(Terrain->GroundAt(To.X, To.Y), Terrain->QuickGround(To.X, To.Y));
                float Surface = 0.f;
                if (Terrain->IsWaterAt(To.X, To.Y, Surface)) To.Z = FMath::Max(To.Z, Surface);
            }
            To.Z += Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 150.f;
            Hero->Dismount();
            if (!Hero->TeleportTo(To, Hero->GetActorRotation())) return Fail(TEXT("Não coube lá (algo no caminho)."));
            // o chão de lá leva um instante para ganhar colisão: o herói flutua 4 s antes de cair
            if (!Hero->IsDevFlying())
            {
                Hero->SetDevFly(true);
                Hero->ClientDevFly(true);
                FTimerHandle Land;
                TWeakObjectPtr<AVortexCharacter> Weak(Hero);
                Hero->GetWorldTimerManager().SetTimer(Land, FTimerDelegate::CreateLambda([Weak]()
                {
                    if (AVortexCharacter* Landed = Weak.Get()) { Landed->SetDevFly(false); Landed->ClientDevFly(false); }
                }), 4.f, false);
            }
            return FString::Printf(TEXT("Foi para %sX %.0f  Y %.0f."), Name.IsEmpty() ? TEXT("") : *(Name + TEXT(": ")), To.X / 100.f, To.Y / 100.f);
        }
        if (Command == TEXT("spawn") || Command == TEXT("criar"))
        {
            // o nome pode ter várias palavras ("gota de brasa"); os números do fim são o nível e a quantidade
            int32 NameEnd = 1;
            while (Args.IsValidIndex(NameEnd) && !Args[NameEnd].IsNumeric()) ++NameEnd;
            const FString Name = FString::Join(TArrayView<const FString>(Args).Slice(1, NameEnd - 1), TEXT(" "));
            if (Name.IsEmpty() || Spawnable::Plain(Name) == TEXT("lista"))
            {
                TArray<FString> Names;
                for (const Spawnable::FEntry& E : Spawnable::All()) Names.Add(E.Title);
                return TEXT("Uso: /spawn nome [nível] [quantidade]. Nomes: ") + FString::Join(Names, TEXT(", "));
            }
            const TArray<const Spawnable::FEntry*> Found = Spawnable::Find(Name);
            if (Found.IsEmpty()) return Fail(FString::Printf(TEXT("\"%s\" não existe. /spawn lista mostra os nomes."), *Name));
            if (Found.Num() > 1)
            {
                TArray<FString> Names;
                for (const Spawnable::FEntry* E : Found) Names.Add(E->Title);
                return Fail(TEXT("Qual deles? ") + FString::Join(Names, TEXT(", ")));
            }
            const FName Species = Found[0]->Id;
            const int32 Level = FMath::Clamp(FMath::RoundToInt(Number(NameEnd, float(Sheet->GetLevel()))), 1, 100);
            const int32 Count = FMath::Clamp(FMath::RoundToInt(Number(NameEnd + 1, 1.f)), 1, 10);
            const FVector Forward = Hero->GetActorForwardVector().GetSafeNormal2D();
            if (Species == TEXT("Fada"))
            {
                // a fada só existe em quem desenha a tela (no servidor dedicado não há)
                TActorIterator<AVortexFairies> It(Hero->GetWorld());
                AVortexFairies* Fairies = It ? *It : nullptr;
                int32 Made = 0;
                for (int32 i = 0; Fairies && i < Count; ++i)
                    Made += Fairies->Summon(Hero->GetActorLocation() + Forward.RotateAngleAxis((i - (Count - 1) * .5f) * 30.f, FVector::UpVector) * 250.f + FVector(0.f, 0.f, 60.f)) ? 1 : 0;
                return Made > 0 ? FString::Printf(TEXT("%d fada(s) na sua frente (fogem se você chegar perto)."), Made) : Fail(TEXT("Sem fadas aqui (-VortexNoFairies ou servidor sem tela)."));
            }
            UVortexCreatureSubsystem* Creatures = Hero->GetWorld()->GetSubsystem<UVortexCreatureSubsystem>();
            if (!Creatures) return Fail(TEXT("Sem criaturas neste mapa."));
            int32 Made = 0;
            for (int32 i = 0; i < Count; ++i)
            {
                // em leque, a ~5 m na frente do herói
                const FVector At = Hero->GetActorLocation() + Forward.RotateAngleAxis((i - (Count - 1) * .5f) * 25.f, FVector::UpVector) * 500.f;
                Made += Creatures->DebugSpawnNear(Species, At, Level);
            }
            return Made > 0 ? FString::Printf(TEXT("%d x %s nível %d."), Made, *Found[0]->Title, Level) : Fail(TEXT("Não nasceu (chão ainda não carregou?)."));
        }
        if (Command == TEXT("portal"))
        {
            const FString Rank = Args.IsValidIndex(1) ? Args[1].ToUpper().Left(1) : FString(TEXT("E"));
            if (!FString(TEXT("EDCBAS")).Contains(Rank)) return Fail(TEXT("Ranque: E, D, C, B, A ou S."));
            const FVector Forward = Hero->GetActorForwardVector().GetSafeNormal2D();
            const FVector At = Hero->GetActorLocation() + Forward * 700.f;
            const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
            const float Ground = Data ? Data->GroundAt(At.X, At.Y) : At.Z - 90.f;
            AVortexPortal::Open(Hero->GetWorld(), FVector(At.X, At.Y, Ground), Forward.Rotation().Yaw + 180.f, Rank);
            return FString::Printf(TEXT("Portal de ranque %s aberto."), *Rank);
        }
        if (Command == TEXT("poi"))
        {
            static const TPair<const TCHAR*, EVortexPoi> Kinds[] = { { TEXT("ruinas"), EVortexPoi::Ruins }, { TEXT("torre"), EVortexPoi::Tower },
                { TEXT("santuario"), EVortexPoi::Shrine }, { TEXT("mina"), EVortexPoi::Mine }, { TEXT("cemiterio"), EVortexPoi::Graveyard },
                { TEXT("caravana"), EVortexPoi::Caravan }, { TEXT("cais"), EVortexPoi::Dock } };
            int32 Want = -1;
            if (Args.IsValidIndex(1)) for (const auto& K : Kinds) if (Args[1].ToLower().StartsWith(FString(K.Key).Left(4))) Want = int32(K.Value);
            const FVector Here = Hero->GetActorLocation();
            const FIntPoint Home(FMath::FloorToInt(Here.X / AVortexPointsOfInterest::CellSize), FMath::FloorToInt(Here.Y / AVortexPointsOfInterest::CellSize));
            FVortexPoi Best;
            float BestDistance = TNumericLimits<float>::Max();
            for (int32 DX = -40; DX <= 40; ++DX) for (int32 DY = -40; DY <= 40; ++DY)
            {
                FVortexPoi Poi;
                if (!AVortexPointsOfInterest::PoiAt(Home + FIntPoint(DX, DY), Poi) || (Want >= 0 && int32(Poi.Kind) != Want)) continue;
                const float D = FVector2D::Distance(Poi.At, FVector2D(Here));
                if (D < BestDistance) { BestDistance = D; Best = Poi; }
            }
            if (BestDistance == TNumericLimits<float>::Max()) return Fail(TEXT("Nenhum por perto (até ~3 km)."));
            const FVector2D Stand = AVortexPointsOfInterest::LocalToWorld(Best, 0.f, -9.f);
            const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
            const float Ground = Data ? Data->GroundAt(Stand.X, Stand.Y) : Here.Z;
            Hero->TeleportTo(FVector(Stand, Ground + 250.f), FRotator(0.f, (FVector(Best.At, 0.f) - FVector(Stand, 0.f)).Rotation().Yaw, 0.f));
            return FString::Printf(TEXT("%s a %.0f m (região %d)."), *AVortexPointsOfInterest::KindName(Best.Kind), BestDistance / 100.f, Best.Region);
        }
        if (Command == TEXT("kill") || Command == TEXT("matar"))
        {
            const float Radius = FMath::Clamp(Number(1, 30.f), 1.f, 200.f) * 100.f;
            UVortexCombatComponent* Combat = Hero->GetCombat();
            int32 Killed = 0;
            for (TActorIterator<ACharacter> It(Hero->GetWorld()); It; ++It)
            {
                ACharacter* Target = *It;
                const IVortexCreature* Creature = Cast<IVortexCreature>(Target);
                if (!Creature || Creature->IsPassive() || !Combat || FVector::Dist(Target->GetActorLocation(), Hero->GetActorLocation()) > Radius) continue;
                // o dano passa pela defesa: bate até cair (chefes têm muita vida)
                for (int32 Hit = 0; Hit < 40 && Combat->CanHarm(Target); ++Hit) Combat->ApplyHit(Target, 9999.f);
                if (!Combat->CanHarm(Target)) ++Killed;
            }
            return FString::Printf(TEXT("%d hostis vencidos em %.0f m."), Killed, Radius / 100.f);
        }
        return Fail(FString::Printf(TEXT("Comando \"%s\" não existe. /help mostra a lista."), *Args[0]));
    }
}

#if !UE_BUILD_SHIPPING
namespace
{
    // o chat de testes pelo console (o MCP da Unreal e os testes usam): Vortex.Cmd spawn SlimeFire 5 2
    FAutoConsoleCommandWithWorldAndArgs DevCommand(TEXT("Vortex.Cmd"),
        TEXT("Roda um comando do chat de testes (sem a barra) no herói do servidor. Ex.: Vortex.Cmd spawn SlimeFire 5 2"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
        {
            AVortexCharacter* Hero = nullptr;
            if (World)
                for (TActorIterator<AVortexCharacter> It(World); It; ++It)
                    if (It->IsPlayerControlled()) { Hero = *It; break; }
            if (!Hero || !Hero->HasAuthority()) { UE_LOG(LogTemp, Warning, TEXT("VORTEX_CMD sem herói no servidor")); return; }
            bool bError = false;
            const FString Answer = VortexDevChat::RunOnServer(Hero, TEXT("/") + FString::Join(Args, TEXT(" ")), bError);
            UE_LOG(LogTemp, Display, TEXT("VORTEX_CMD %s%s"), bError ? TEXT("ERRO ") : TEXT(""), *Answer);
        }));
}
#endif
