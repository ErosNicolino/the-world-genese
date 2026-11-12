#include "Framework/VortexHeroWindow.h"
#include "VortexUiImpl.h"
#include "Combat/VortexClassSkills.h"
#include "Framework/VortexAwakening.h"

#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexProjectile.h"
#include "Combat/VortexSwords.h"
#include "Combat/VortexWeaponKit.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Containers/Ticker.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexGameInstance.h"
#include "Framework/VortexLook.h"
#include "Framework/VortexShopWindow.h"
#include "Rpg/VortexShop.h"
#include "GameFramework/PlayerController.h"
#include "Persistence/VortexPlayerSave.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexQuestLog.h"
#include "Rpg/VortexStory.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/VortexUiKit.h"
#include "Framework/VortexPortrait.h"
#include "Creatures/VortexNpc.h"
#include "EngineUtils.h"
#include "Framework/VortexUiJuice.h"

// Tudo dentro de um namespace próprio: no unity build este arquivo divide a unidade com o menu
// (que tem os próprios Gold, Cream, Text...).
//
// Visual (23/10, "refaça todas as janelas, algo realmente de jogo"): o kit "Aventura" em pixel-art
// (madeira com rebites sobre pedra em blocos, espaços de item afundados com o brilho da raridade, botões e abas
// de bloco, títulos na faixa vermelha em letra de desenho animado), as abas como placas na lateral, o herói 3D
// ao vivo na mochila e na aba Herói (gira arrastando) e letras maiores.
//
// 23/10, "refaça a janela, não combina com o cartoon": o kit "Cartoon" no lugar do Aventura (azul vivo, cantos
// redondos, contorno grosso, botões coloridos) e, "efeito e animações nos cliques tipo cartoon e sons":
// botões de borracha (crescem e balançam com o mouse, achatam ao apertar e quicam ao soltar), estrelinhas no
// clique, a janela abre pulando e fecha encolhendo, a página entra deslizando, o número do atributo dá um
// soco com "+1" subindo, as moedas rolam, o brilho dos itens raros pulsa, e um som de desenho animado para
// cada coisa (Tools/MakeUiSoundsCartoon.py). Para voltar ao visual antigo: Av = VortexUiKit::Aventura.
namespace VortexHeroUi
{
    namespace Av = VortexUiKit::Cartoon;
    // sons, estrelinhas e botões de borracha: Framework/VortexUiJuice.h (todas as telas usam)
    using namespace VortexUiJuice;

    const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor(0xFF, 0xD2, 0x3C));
    const FLinearColor GoldDeep = FLinearColor::FromSRGBColor(FColor(0xF0, 0x9A, 0x1E));
    const FLinearColor Cream = FLinearColor::FromSRGBColor(FColor(0xFF, 0xFB, 0xF0));
    const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(0xB8, 0xCB, 0xF2));
    const FLinearColor Good = FLinearColor::FromSRGBColor(FColor(0x8F, 0xE8, 0x7A));
    const FLinearColor Bad = FLinearColor::FromSRGBColor(FColor(0xF4, 0x80, 0x70));
    const FLinearColor Ink = FLinearColor(.05f, .03f, .02f);
    const FLinearColor Health = FLinearColor::FromSRGBColor(FColor(0xF4, 0x6A, 0x6A));
    const FLinearColor ManaBlue = FLinearColor::FromSRGBColor(FColor(0x6E, 0xAE, 0xF6));

    FLinearColor Alpha(const FLinearColor& C, float A) { return FLinearColor(C.R, C.G, C.B, A); }
    // 07/11: a letra escura do pergaminho (o diário de missões, a fala do vendedor)
    FLinearColor InkText() { return FLinearColor::FromSRGBColor(FColor(0x3A, 0x26, 0x16)); }
    FLinearColor InkMuted() { return FLinearColor::FromSRGBColor(FColor(0x6E, 0x50, 0x34)); }
    FLinearColor InkTitle() { return FLinearColor::FromSRGBColor(FColor(0x8A, 0x22, 0x1E)); }

    FSlateFontInfo Font(int32 Size, const TCHAR* Weight = TEXT("Regular"), int32 Spacing = 0)
    {
        FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(Weight, Size);
        Info.LetterSpacing = Spacing;
        return Info;
    }

    TSharedRef<STextBlock> Label(const FString& Value, int32 Size, const FLinearColor& Color, const TCHAR* Weight = TEXT("Regular"),
        int32 Spacing = 0, bool bWrap = false)
    {
        // contorno escuro como as letras da HUD (as miúdas ficam só com a sombra, para não embolar)
        return SNew(STextBlock).Text(FText::FromString(Value)).Font(Size >= 14 ? VortexUiKit::Outlined(Font(Size, Weight, Spacing)) : Font(Size, Weight, Spacing)).ColorAndOpacity(Color)
            .ShadowOffset(FVector2D(0.f, 1.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .7f)).AutoWrapText(bWrap);
    }

    /** Título em letra de desenho animado (nomes, números grandes, títulos das seções). */
    TSharedRef<STextBlock> Title(const FString& Value, int32 Size, const FLinearColor& Color, bool bWrap = false)
    {
        return SNew(STextBlock).Text(FText::FromString(Value)).Font(Av::Cartoon(Size, Size >= 22 ? 2 : 1)).ColorAndOpacity(Color)
            .Margin(CartoonNudge(Size))
            .ShadowOffset(FVector2D(0.f, 2.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .6f)).AutoWrapText(bWrap);
    }

    FString Upper(const FString& Value) { return FText::FromString(Value).ToUpper().ToString(); }

    /** O mesmo texto, mas lido a cada quadro (números que mudam sem refazer a janela, como os pontos gastos). */
    TSharedRef<STextBlock> Live(TSharedRef<STextBlock> Block, TFunction<FString()> Value)
    {
        Block->SetText(TAttribute<FText>::CreateLambda([Value]() { return FText::FromString(Value()); }));
        return Block;
    }

    FString Number(float Value, int32 Decimals = 0)
    {
        if (Decimals <= 0) return FText::AsNumber(FMath::RoundToInt(Value)).ToString();
        return FString::Printf(TEXT("%.*f"), Decimals, Value).Replace(TEXT("."), TEXT(","));
    }

    const FSlateBrush* White() { return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")); }

    const FSlateBrush* Round(float Radius)
    {
        static TMap<int32, TSharedPtr<FSlateRoundedBoxBrush>> Cache;
        TSharedPtr<FSlateRoundedBoxBrush>& Brush = Cache.FindOrAdd(FMath::RoundToInt(Radius));
        if (!Brush) Brush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor::White, Radius);
        return Brush.Get();
    }

    /** Pílula redonda com contorno escuro, na cor pedida (etiquetas pequenas, onde a peça de nove fatias não cabe). */
    const FSlateBrush* Pill(const FLinearColor& Fill)
    {
        static TMap<FString, TSharedPtr<FSlateRoundedBoxBrush>> Cache;
        TSharedPtr<FSlateRoundedBoxBrush>& Brush = Cache.FindOrAdd(Fill.ToString());
        if (!Brush) Brush = MakeShared<FSlateRoundedBoxBrush>(Fill, 12.f, FLinearColor::FromSRGBColor(FColor(20, 20, 50)), 3.f);
        return Brush.Get();
    }

    /** Imagem de uma textura que fica carregada (ícones de item/técnica, arte da raça). */
    const FSlateBrush* TextureBrush(UTexture2D* Texture, float Size)
    {
        static TMap<TPair<UTexture2D*, int32>, TSharedPtr<FSlateBrush>> Cache;
        if (!Texture) return nullptr;
        TSharedPtr<FSlateBrush>& Brush = Cache.FindOrAdd({ Texture, FMath::RoundToInt(Size) });
        if (!Brush)
        {
            Brush = MakeShared<FSlateBrush>();
            Brush->SetResourceObject(Texture);
            Brush->ImageSize = FVector2D(Size, Size * Texture->GetSizeY() / FMath::Max(1, Texture->GetSizeX()));
            Brush->DrawAs = ESlateBrushDrawType::Image;
        }
        return Brush.Get();
    }

    const FSlateBrush* IconBrush(FName Icon, float Size) { return TextureBrush(FVortexRpgCatalog::Icon(Icon), Size); }

    const FSlateBrush* RaceBrush(FName Race, float Size = 300.f)
    {
        static TMap<FName, UTexture2D*> Loaded;
        UTexture2D*& Texture = Loaded.FindOrAdd(Race);
        if (!Texture)
        {
            const FString Id = Race.ToString();
            Texture = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/UI/Menu/Races/T_Race_%s.T_Race_%s"), *Id, *Id));
            if (Texture) Texture->AddToRoot();
        }
        return TextureBrush(Texture, Size);
    }

    /** Botões de bloco: 0 = pedra, 1 = dourado, 2 = aba, 3 = espaço de item, 4 = aba escolhida. */
    const FButtonStyle& ButtonStyle(int32 Kind)
    {
        return Av::Button(Kind == 3 ? 4 : Kind == 4 ? 3 : Kind);
    }

    const FProgressBarStyle& BarStyle()
    {
        static FProgressBarStyle Style = FProgressBarStyle().SetBackgroundImage(*Av::BarBack()).SetFillImage(*Av::BarFill()).SetMarqueeImage(*Av::BarFill());
        return Style;
    }

    /** Painel afundado na pedra. */
    TSharedRef<SWidget> Box(const TSharedRef<SWidget>& Content, const FMargin& Padding = FMargin(18.f))
    {
        return SNew(SBorder).BorderImage(Av::Panel()).Padding(Padding)[Content];
    }

    /** O título da seção na faixa vermelha (letra de desenho animado) e, embaixo, uma linha de ajuda. */
    TSharedRef<SWidget> Heading(const FString& Text, const FString& Subtitle = FString(), const FLinearColor& SubColor = Muted)
    {
        TSharedRef<SVerticalBox> V = SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(SBorder).BorderImage(Av::Ribbon()).Padding(FMargin(34.f, 6.f, 34.f, 8.f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
                [Title(Upper(Text), 19, Cream)]
            ];
        if (!Subtitle.IsEmpty()) V->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f, 0.f, 0.f)[Label(Subtitle, 14, SubColor, TEXT("Regular"), 0, true)];
        V->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 8.f);
        return V;
    }

    TSharedRef<SWidget> Bar(TAttribute<TOptional<float>> Value, const FLinearColor& Color, float Height = 16.f)
    {
        return SNew(SBox).HeightOverride(Height)[SNew(SProgressBar).Style(&BarStyle()).Percent(Value).FillColorAndOpacity(Color)];
    }

    /** Ícone num espaço afundado, com o brilho da raridade por dentro e a quantidade no canto. */
    TSharedRef<SWidget> IconTile(FName Icon, float Size, const FLinearColor& Edge, int32 Count = 0, bool bDim = false)
    {
        // cor forte (raro para cima): o brilho pulsa devagar
        const bool bShiny = !bDim && FMath::Max3(Edge.R, Edge.G, Edge.B) - FMath::Min3(Edge.R, Edge.G, Edge.B) > .3f;
        // 07/11 (a referência do usuário): o miolo levemente na cor da raridade e o aro dela forte (comum: discreto)
        const bool bPlain = FMath::Max3(Edge.R, Edge.G, Edge.B) - FMath::Min3(Edge.R, Edge.G, Edge.B) < .15f;
        TSharedRef<SOverlay> O = SNew(SOverlay)
            + SOverlay::Slot()[SNew(SImage).Image(Av::Slot(0))]
            + SOverlay::Slot().Padding(Size * .07f)[SNew(SImage).Image(Round(6.f)).ColorAndOpacity(FLinearColor(Edge.R, Edge.G, Edge.B, bDim || bPlain ? 0.f : .16f))]
            + SOverlay::Slot()[SNew(SImage).Image(Av::SlotGlow(Edge)).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, bDim || bPlain ? 0.f : .9f))]
            + SOverlay::Slot()
            [
                SNew(SImage).Image(Av::SlotGlow(Edge))
                .ColorAndOpacity_Lambda([bDim, bShiny, Phase = float(GetTypeHash(Icon) % 600) * .01f]()
                {
                    const float A = bDim ? .12f : bShiny ? .55f + .45f * (.5f + .5f * FMath::Sin(float(Now()) * 3.4f + Phase)) : .8f;
                    return FSlateColor(FLinearColor(1.f, 1.f, 1.f, A));
                })
            ];
        if (const FSlateBrush* Brush = Icon.IsNone() ? nullptr : IconBrush(Icon, Size * .68f))
        {
            O->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SImage).Image(Brush).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, bDim ? .25f : 1.f))];
        }
        if (Count > 1)
        {
            O->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, Size * .1f, Size * .06f)[Title(FString::FromInt(Count), FMath::Max(14, FMath::RoundToInt(Size * .25f)), Cream)];
        }
        return SNew(SBox).WidthOverride(Size).HeightOverride(Size)[O];
    }

    /** Um número grande com o nome em cima (os blocos de "No combate"). */
    TSharedRef<SWidget> StatTile(const FString& Name, TFunction<FString()> Value, const FLinearColor& Color)
    {
        // bloco colorido (vida vermelha, mana azul...); as cores claras viram cinza-azulado para a letra branca aparecer
        const FLinearColor Tile = Color.GetLuminance() > .6f ? FLinearColor::FromSRGBColor(FColor(0x6A, 0x78, 0x9C)) : FMath::Lerp(Color, FLinearColor::Black, .12f);
        return SNew(SBorder).BorderImage(Av::Ribbon(Tile)).Padding(FMargin(10.f, 6.f, 10.f, 12.f)).HAlign(HAlign_Center)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Title(Upper(Name), 13, Cream)]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Live(Title(Value(), 26, Cream), Value)]
            ];
    }

    TSharedRef<SWidget> StatTile(const FString& Name, const FString& Value, const FLinearColor& Color)
    {
        return StatTile(Name, TFunction<FString()>([Value]() { return Value; }), Color);
    }

    /** Tecla desenhada (os comandos e o número da habilidade). */
    TSharedRef<SWidget> KeyCap(const FString& Key, float MinWidth = 40.f)
    {
        return SNew(SBox).MinDesiredWidth(MinWidth).HeightOverride(38.f)
            [SNew(SBorder).BorderImage(Av::Key()).Padding(FMargin(10.f, 4.f, 10.f, 8.f)).HAlign(HAlign_Center).VAlign(VAlign_Center)[Title(Key, 15, Cream)]];
    }

    /** Botão de bloco com o texto em letra de desenho animado. */
    TSharedRef<SWidget> BlockButton(const FString& Text, int32 Kind, bool bEnabled, float Height, TFunction<void()> OnClick, const FString& Key = FString())
    {
        return SNew(SJuicyButton).ButtonStyle(&ButtonStyle(Kind)).IsEnabled(bEnabled).HAlign(HAlign_Center).VAlign(VAlign_Center)
            .Id(FName(*(TEXT("btn_") + Text + Key))).StarColor(Kind == 1 ? Gold : Cream)   // Key: botões iguais em vários cartões
            .ContentPadding(FMargin(18.f, 4.f, 18.f, 4.f))
            .OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
            [SNew(SBox).MinDesiredHeight(Height - 12.f).VAlign(VAlign_Center)[Title(Upper(Text), 17, bEnabled ? Cream : Alpha(Cream, .6f))]];
    }

    struct FAttr { const TCHAR* Short; FLinearColor Color; };
    const FAttr Attrs[VortexRpg::NumAttributes] = {
        { TEXT("FOR"), FLinearColor::FromSRGBColor(FColor(0xE8, 0x7A, 0x5A)) }, { TEXT("DES"), FLinearColor::FromSRGBColor(FColor(0x8F, 0xE3, 0x8A)) },
        { TEXT("VIT"), FLinearColor::FromSRGBColor(FColor(0xE8, 0x5A, 0x6A)) }, { TEXT("INT"), FLinearColor::FromSRGBColor(FColor(0x8A, 0xB4, 0xF0)) },
        { TEXT("MAN"), FLinearColor::FromSRGBColor(FColor(0x5A, 0x9C, 0xF0)) }, { TEXT("ESP"), FLinearColor::FromSRGBColor(FColor(0xB0, 0x8C, 0xF2)) },
        { TEXT("REC"), FLinearColor::FromSRGBColor(FColor(0x6F, 0xD0, 0xB0)) }, { TEXT("SOR"), FLinearColor::FromSRGBColor(FColor(0xF2, 0xCF, 0x6A)) },
    };

    // ---------------------------------------------------------------------------------------------
    class SVortexHeroWindow final : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SVortexHeroWindow) {}
            SLATE_ARGUMENT(TWeakObjectPtr<AVortexCharacter>, Hero)
            SLATE_ARGUMENT(int32, Page)
        SLATE_END_ARGS()

        virtual ~SVortexHeroWindow() override { StopDoll(); }

        void Construct(const FArguments& Args)
        {
            Hero = Args._Hero;
            Page = Args._Page;
            OpenedAt = Now();
            PageChangedAt = OpenedAt;
            if (const UVortexHeroSheet* Sheet = GetSheet()) ShownCoins = float(Sheet->GetCoins());
            const TCHAR* PageTitles[] = { TEXT("Herói"), TEXT("Mochila"), TEXT("Técnicas"), TEXT("Classe"), TEXT("Alquimia"), TEXT("Missões") };
            TSharedPtr<SCartoonFx> Fx;
            ChildSlot
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                [
                    SNew(SImage).Image(White()).ColorAndOpacity_Lambda([this]()
                    {
                        return FSlateColor(FLinearColor(0.f, 0.f, .05f, .5f * OpenAmount()));
                    })
                ]
                + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                [
                    SNew(SBox).WidthOverride(1360.f).HeightOverride(880.f)
                    .RenderTransformPivot(FVector2D(.5f, .5f))
                    .RenderTransform_Lambda([this]()
                    {
                        // abre pulando (cresce, passa do tamanho e assenta); fecha encolhendo
                        const float T = float(Now() - OpenedAt);
                        float Scale = .72f + .28f * (1.f - FMath::Exp(-8.f * T) * FMath::Cos(T * 15.f));
                        if (ClosingAt > 0.0) Scale *= FMath::Max(.2f, 1.f - float(Now() - ClosingAt) / .14f * .5f);
                        return FSlateRenderTransform(FScale2f(Scale));
                    })
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()[SNew(SBorder).BorderImage(Av::Frame())]
                        + SOverlay::Slot().Padding(FMargin(26.f, 30.f, 30.f, 18.f))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 16.f, 0.f)[SAssignNew(TabBox, SBox).WidthOverride(112.f)]
                            + SHorizontalBox::Slot().FillWidth(1.f)
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot().AutoHeight()[SAssignNew(HeaderBox, SBox)]
                                + SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 12.f, 0.f, 0.f)
                                [
                                    // a página nova entra deslizando da direita, com um quique, e aparecendo
                                    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush(TEXT("NoBorder"))).Padding(0.f)
                                    .ColorAndOpacity_Lambda([this]() { return FLinearColor(1.f, 1.f, 1.f, FMath::Min(1.f, float(Now() - PageChangedAt) / .12f)); })
                                    .RenderTransform_Lambda([this]()
                                    {
                                        const float T = float(Now() - PageChangedAt);
                                        return FSlateRenderTransform(FVector2f(T > .8f ? 0.f : 60.f * FMath::Exp(-9.f * T) * FMath::Cos(T * 13.f), 0.f));
                                    })
                                    [SAssignNew(BodyBox, SBox)]
                                ]
                                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f).HAlign(HAlign_Center)
                                [SNew(STextBlock).Font(Font(14)).ColorAndOpacity(Cream).ShadowOffset(FVector2D(0.f, 1.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, .1f, .8f))
                                    .Text_Lambda([this]() { return FText::FromString(Help()); })]
                            ]
                        ]
                        // o nome da página numa fita grande, saindo pela borda de cima
                        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, -30.f, 0.f, 0.f)
                        [
                            SNew(SBorder).BorderImage(Av::Ribbon()).Padding(FMargin(70.f, 8.f, 70.f, 16.f))
                            .RenderTransformPivot(FVector2D(.5f, .5f))
                            .RenderTransform_Lambda([this]() { return Punch(PageChangedAt); })
                            [
                                SNew(STextBlock).Font(Av::Cartoon(38, 3)).ColorAndOpacity(Cream).Margin(CartoonNudge(38)).ShadowOffset(FVector2D(0.f, 3.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, .1f, .7f))
                                .Text_Lambda([this, PageTitles]() { return FText::FromString(Upper(PageTitles[FMath::Clamp(Page, 0, 5)])); })
                            ]
                        ]
                        // fechar: botão vermelho redondo saindo pelo canto
                        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, -18.f, -18.f, 0.f)
                        [
                            SNew(SJuicyButton).ButtonStyle(&ButtonStyle(5)).Id(TEXT("close")).Sound(NAME_None).StarColor(Health)
                            .ToolTipText(FText::FromString(TEXT("Fechar (I, Tab ou Esc)")))
                            .OnClicked_Lambda([this]() { RequestClose(); return FReply::Handled(); })
                            [SNew(SBox).WidthOverride(66.f).HeightOverride(62.f).HAlign(HAlign_Center).VAlign(VAlign_Center)[Title(TEXT("X"), 32, Cream)]]
                        ]
                    ]
                ]
                + SOverlay::Slot()[SAssignNew(Fx, SCartoonFx)]
            ];
            FxLayer = Fx;
            Rebuild();
        }

        virtual bool SupportsKeyboardFocus() const override { return true; }

        virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& Event) override
        {
            const FKey Key = Event.GetKey();
            if (Key == EKeys::Escape) return FReply::Unhandled(); // Esc é centralizado no PlayerController.
            if (Key == EKeys::I || Key == EKeys::Tab)
            {
                RequestClose();
                return FReply::Handled();
            }
            static const FKey PageKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six };
            for (int32 i = 0; i < PageCount; ++i)
            {
                if (Key == PageKeys[i]) { SetPage(i); return FReply::Handled(); }
            }
            return FReply::Unhandled();
        }

        virtual void Tick(const FGeometry& Geometry, const double CurrentTime, const float DeltaTime) override
        {
            SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
            const UVortexHeroSheet* Sheet = GetSheet();
            if (!Sheet)
            {
                RequestClose();
                return;
            }
            // a vitrine do herói só existe nas abas que a mostram (a câmera custa um quadro a mais)
            if (Page == 0 || Page == 1) { StartDoll(); PlaceDoll(DeltaTime); }
            else StopDoll();
            // as moedas rolam até o valor novo
            const float Coins = float(Sheet->GetCoins());
            if (ShownCoins != Coins)
            {
                const float Step = FMath::Max(1.f, FMath::Abs(Coins - ShownCoins) * DeltaTime * 6.f);
                ShownCoins = FMath::Abs(Coins - ShownCoins) <= Step ? Coins : ShownCoins + FMath::Sign(Coins - ShownCoins) * Step;
                if (Now() - CoinsChangedAt > .5) CoinsChangedAt = Now();   // a moeda pula de novo a cada meio segundo rolando
            }
            const UVortexQuestLog* Log = QuestLog();
            // "o menu continua piscando quando coloco pontos": logo depois de um "+" na página do herói só os pontos
            // mudaram, e eles já aparecem ao vivo; refazer a janela inteira a cada clique é que piscava
            const bool bOnlyPoints = Page == 0 && Now() - LastSpentAt < 2.0 && Sheet->GetLevel() == ShownLevel && Sheet->GetXp() == ShownXp
                && (!Log || Log->GetRevision() == ShownQuestRevision);
            if (bOnlyPoints && Sheet->GetRevision() != ShownRevision) ShownRevision = Sheet->GetRevision();
            if (Sheet->GetRevision() != ShownRevision || (Log && Log->GetRevision() != ShownQuestRevision)) Rebuild();
        }

        /** Testes: o mesmo que o botão "+" do atributo. */
        int32 GetRebuilds() const { return Rebuilds; }
        void SpendForTest(int32 i)
        {
            if (UVortexHeroSheet* Target = GetSheet()) Target->ServerSpendPoint(uint8(i));
            LastSpentAttr = i;
            LastSpentAt = Now();
        }

        /** Testes de foto: escolhe um item da mochila (BagIndex) ou do corpo (EquipSlot) como se fosse clicado. */
        void SelectForTest(int32 BagIndex, int32 EquipSlot)
        {
            Selected = BagIndex;
            SelectedEquip = EquipSlot;
            Rebuild();
        }

        void SetPage(int32 NewPage)
        {
            if (NewPage == Page) return;
            Page = NewPage;
            Selected = INDEX_NONE;
            SelectedEquip = INDEX_NONE;
            HoverBag = INDEX_NONE;
            UiSound(TEXT("ui_c_tab"), .55f);
            PageChangedAt = Now();
            Juices().FindOrAdd(FName(*FString::Printf(TEXT("tab%d"), NewPage))).PopAt = PageChangedAt;
            Rebuild();
        }

    private:
        TWeakObjectPtr<AVortexCharacter> Hero;
        static constexpr int32 PageCount = 6;
        double PageChangedAt = 0.0, ClosingAt = 0.0, CoinsChangedAt = -10.0, LastSpentAt = -10.0;
        float ShownCoins = 0.f;
        int32 LastSpentAttr = INDEX_NONE;
        int32 ShownLevel = -1;
        int64 ShownXp = -1;
        // os números da página do herói, calculados no máximo uma vez por quadro
        FVortexHeroStats LiveStats;
        uint64 LiveFrame = MAX_uint64;
        const FVortexHeroStats& Stats()
        {
            if (LiveFrame != GFrameCounter) if (const UVortexHeroSheet* Sheet = GetSheet()) { LiveStats = Sheet->ComputeStats(); LiveFrame = GFrameCounter; }
            return LiveStats;
        }
        int32 FreePoints() const { const UVortexHeroSheet* Sheet = GetSheet(); return Sheet ? Sheet->GetPointsFree() : 0; }

        /** 0 fechada .. 1 aberta (o escuro do fundo acompanha). */
        float OpenAmount() const
        {
            const float In = FMath::Min(1.f, float(Now() - OpenedAt) / .2f);
            return ClosingAt > 0.0 ? In * FMath::Max(0.f, 1.f - float(Now() - ClosingAt) / .14f) : In;
        }
        int32 Page = 0, ShownRevision = -1, ShownQuestRevision = -1, Selected = INDEX_NONE, SelectedEquip = INDEX_NONE, LastClicked = INDEX_NONE;
        int32 HoverBag = INDEX_NONE, HoverEquip = INDEX_NONE;
        FName SelectedQuest, SelectedClass;
        UVortexQuestLog* QuestLog() const { return Hero.IsValid() ? Hero->GetQuestLog() : nullptr; }
        FString Filter;
        double OpenedAt = 0.0, LastClickTime = 0.0;
        TSharedPtr<SBox> HeaderBox, BodyBox, TabBox, DetailBox;

        // ---- a vitrine: uma câmera só do herói (corpo inteiro), ao vivo; gira arrastando o mouse
        struct FDoll
        {
            TStrongObjectPtr<UTextureRenderTarget2D> Target;
            // a imagem passa por um material de interface (M_UiDoll): a câmera grava a cor com transparência
            // zero, e a imagem direta ficava invisível
            TStrongObjectPtr<UMaterialInstanceDynamic> Material;
            TWeakObjectPtr<USceneCaptureComponent2D> Capture;
            FSlateBrush Brush;
            float Yaw = 0.f, Spin = 0.f;
            bool bDrag = false;
            float LastX = 0.f;
        };
        TSharedPtr<FDoll> Doll;

        void StartDoll()
        {
            AVortexCharacter* H = Hero.Get();
            if (Doll || !H || !H->GetWorld() || H->GetNetMode() == NM_DedicatedServer) return;
            Doll = MakeShared<FDoll>();
            UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
            Target->ClearColor = FLinearColor(0.f, 0.f, 0.f, 1.f);
            Target->InitCustomFormat(480, 640, PF_FloatRGBA, false);   // cor + profundidade (o fundo vazio fica transparente)
            Doll->Target.Reset(Target);
            USceneCaptureComponent2D* Capture = NewObject<USceneCaptureComponent2D>(H);
            Capture->bCaptureEveryFrame = true;
            Capture->bCaptureOnMovement = false;
            Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
            // só a cor do modelo (sem a luz da cena): igual de dia e de noite, como o retrato da HUD; a profundidade
            // vai junto (canal A) e o material da vitrine deixa transparente onde não há nada
            Capture->CaptureSource = ESceneCaptureSource::SCS_SceneColorSceneDepth;
            Capture->ShowFlags.SetLighting(false);
            Capture->ShowFlags.SetAtmosphere(false);
            Capture->ShowFlags.SetFog(false);
            Capture->ShowFlags.SetBloom(false);
            Capture->ShowFlags.SetEyeAdaptation(false);
            Capture->FOVAngle = 30.f;
            Capture->TextureTarget = Target;
            Capture->ShowOnlyActors.Add(H);
            Capture->RegisterComponentWithWorld(H->GetWorld());
            Doll->Capture = Capture;
            if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/UI/KitCartoon/M_UiDoll.M_UiDoll")))
            {
                UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, GetTransientPackage());
                Material->SetTextureParameterValue(TEXT("Doll"), Target);
                Doll->Material.Reset(Material);
                Doll->Brush.SetResourceObject(Material);
            }
            else Doll->Brush.SetResourceObject(Target);
            Doll->Brush.ImageSize = FVector2D(480.f, 640.f);
            Doll->Brush.DrawAs = ESlateBrushDrawType::Image;
            PlaceDoll(0.f);
        }

        void PlaceDoll(float DeltaTime)
        {
            AVortexCharacter* H = Hero.Get();
            USceneCaptureComponent2D* Capture = Doll && Doll->Capture.IsValid() ? Doll->Capture.Get() : nullptr;
            if (!H || !Capture) return;
            // sem arrastar, volta devagar para de frente
            if (!Doll->bDrag) Doll->Yaw = FMath::FInterpTo(Doll->Yaw, 0.f, DeltaTime, 1.2f);
            const float Stature = FMath::Max(.75f, H->GetBodyShape().Stature());
            const FVector Center = H->GetActorLocation() + FVector(0.f, 0.f, 8.f * Stature);
            const FVector Dir = FRotator(0.f, H->GetActorRotation().Yaw + Doll->Yaw, 0.f).Vector();
            const FVector Eye = Center + Dir * 290.f * Stature + FVector(0.f, 0.f, 22.f * Stature);
            Capture->SetWorldLocationAndRotation(Eye, (Center - Eye).Rotation());
        }

        void StopDoll()
        {
            if (Doll && Doll->Capture.IsValid()) Doll->Capture->DestroyComponent();
            Doll.Reset();
        }

        /** A vitrine na tela: o herói ao vivo sobre a placa de pedra; arrastar gira. */
        TSharedRef<SWidget> DollView(float Width, float Height)
        {
            if (!Doll) StartDoll();
            TSharedRef<SOverlay> O = SNew(SOverlay)
                + SOverlay::Slot()[SNew(SBorder).BorderImage(Av::Card())]
                + SOverlay::Slot().Padding(FMargin(10.f, 10.f, 10.f, 18.f))[SNew(SImage).Image(Round(14.f)).ColorAndOpacity(FLinearColor::FromSRGBColor(FColor(0x1E, 0x28, 0x46)))]   // 07/11: azul-noite (era azul-claro)
                + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 24.f)
                [SNew(SBox).WidthOverride(Width * .82f).HeightOverride(Width * .3f)[SNew(SImage).Image(Av::Stage())]];
            if (Doll)
            {
                // 07/11: sem esticar (a câmera grava 3:4; a caixa da mochila é mais estreita): o maior 3:4 que cabe, no meio
                const float InW = Width - 16.f, InH = Height - 16.f;
                const float W = FMath::Min(InW, InH * .75f), H = W / .75f;
                O->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SBox).WidthOverride(W).HeightOverride(H)[SNew(SImage).Image(&Doll->Brush)]];
            }
            O->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 20.f)[Title(TEXT("ARRASTE PARA GIRAR"), 12, Cream)];
            return SNew(SBox).WidthOverride(Width).HeightOverride(Height)
                [
                    SNew(SBorder).BorderImage(White()).BorderBackgroundColor(FLinearColor::Transparent).Padding(0.f)
                    .OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& Event)
                    {
                        if (!Doll) return FReply::Unhandled();
                        Doll->bDrag = true;
                        Doll->LastX = Event.GetScreenSpacePosition().X;
                        return FReply::Handled();
                    })
                    .OnMouseMove_Lambda([this](const FGeometry&, const FPointerEvent& Event)
                    {
                        if (!Doll || !Doll->bDrag) return FReply::Unhandled();
                        if (!Event.IsMouseButtonDown(EKeys::LeftMouseButton)) { Doll->bDrag = false; return FReply::Unhandled(); }
                        Doll->Yaw += (Event.GetScreenSpacePosition().X - Doll->LastX) * .6f;
                        Doll->LastX = Event.GetScreenSpacePosition().X;
                        return FReply::Handled();
                    })
                    .OnMouseButtonUp_Lambda([this](const FGeometry&, const FPointerEvent&)
                    {
                        if (Doll) Doll->bDrag = false;
                        return FReply::Handled();
                    })
                    [O]
                ];
        }

        UVortexHeroSheet* GetSheet() const { return Hero.IsValid() ? Hero->GetSheet() : nullptr; }

        void RequestClose()
        {
            // encolhe um instante e só então sai da tela
            if (ClosingAt > 0.0) return;
            ClosingAt = Now();
            UiSound(TEXT("ui_c_close"), .55f);
            FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float) { VortexHeroWindow::Close(); return false; }), .13f);
        }

        FString Help() const
        {
            switch (Page)
            {
            case 1: return TEXT("Passe o mouse para ver  •  clique duplo ou botão direito usa / equipa  •  I ou Tab fecha");
            case 2: return TEXT("As técnicas mudam com a arma na mão  •  I ou Tab fecha");
            case 3: return TEXT("Habilidades da classe nas teclas 1 a 4: a 1ª no Despertar, depois uma a cada 5 níveis  •  I ou Tab fecha");
            case 4: return TEXT("Ingredientes vêm das criaturas e da coleta  •  I ou Tab fecha");
            case 5: return TEXT("Clique numa missão para ver os detalhes  •  I ou Tab fecha");
            default: return TEXT("2 pontos a cada nível  •  passe o mouse num atributo para ver o que ele faz  •  I ou Tab fecha");
            }
        }

        int32 Rebuilds = 0;
        void Rebuild()
        {
            const UVortexHeroSheet* Sheet = GetSheet();
            if (!Sheet) return;
            ++Rebuilds;
            ShownRevision = Sheet->GetRevision();
            ShownQuestRevision = QuestLog() ? QuestLog()->GetRevision() : -1;
            ShownLevel = Sheet->GetLevel();
            ShownXp = Sheet->GetXp();
            // a mesma página refeita (um item, XP, um ponto gasto): as listas voltam para onde estavam
            TArray<float> Offsets;
            if (Page == BuiltPage) CollectScrolls(BodyBox.ToSharedRef(), [&Offsets](SScrollBox& Box) { Offsets.Add(Box.GetScrollOffset()); });
            TabBox->SetContent(BuildTabs(*Sheet));
            HeaderBox->SetContent(BuildHeader(*Sheet));
            BodyBox->SetContent(Page == 1 ? BuildBag(*Sheet) : Page == 2 ? BuildSkills(*Sheet) : Page == 3 ? BuildClasses(*Sheet)
                : Page == 4 ? BuildAlchemy(*Sheet) : Page == 5 ? BuildQuests(*Sheet) : BuildCharacter(*Sheet));
            int32 Next = 0;
            if (Offsets.Num()) CollectScrolls(BodyBox.ToSharedRef(), [&Offsets, &Next](SScrollBox& Box) { if (Offsets.IsValidIndex(Next)) Box.SetScrollOffset(Offsets[Next]); ++Next; });
            BuiltPage = Page;
        }

        /** Visita as listas com rolagem da página, sempre na mesma ordem. */
        static void CollectScrolls(const TSharedRef<SWidget>& Widget, const TFunctionRef<void(SScrollBox&)>& Visit)
        {
            if (Widget->GetType() == FName(TEXT("SScrollBox"))) Visit(static_cast<SScrollBox&>(Widget.Get()));
            FChildren* Children = Widget->GetChildren();
            for (int32 i = 0; Children && i < Children->Num(); ++i) CollectScrolls(Children->GetChildAt(i), Visit);
        }
        int32 BuiltPage = -1;

        // ---- as abas: placas de madeira na lateral, com o ícone grande e o aviso (pontos, cartas, missões prontas)
        TSharedRef<SWidget> BuildTabs(const UVortexHeroSheet& Sheet)
        {
            const TCHAR* TabNames[] = { TEXT("Herói"), TEXT("Mochila"), TEXT("Técnicas"), TEXT("Classe"), TEXT("Alquimia"), TEXT("Missões") };
            const FName TabIcons[] = { TEXT("helm_leather"), TEXT("ui_backpack"), TEXT("ability_sword_ashen_arc"), TEXT("atlas_C27"), TEXT("pocao_de_vida"), TEXT("mapa_de_nicolly") };
            const UVortexQuestLog* Log = QuestLog();
            int32 ReadyQuests = 0;
            if (Log) for (const FVortexQuestDef& Q : FVortexStoryCatalog::Get().Quests) if (Log->IsReady(Q)) ++ReadyQuests;
            TSharedRef<SVerticalBox> Tabs = SNew(SVerticalBox);
            for (int32 i = 0; i < PageCount; ++i)
            {
                const bool bActive = Page == i;
                const int32 AlertCount = i == 0 ? Sheet.GetPointsFree() : i == 3 && Sheet.CanAwaken() ? 1 : i == 5 ? ReadyQuests : 0;
                Tabs->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()
                    [
                        SNew(SJuicyButton).ButtonStyle(&ButtonStyle(bActive ? 4 : 2)).ContentPadding(FMargin(4.f, 4.f, 4.f, 4.f))
                        .Id(FName(*FString::Printf(TEXT("tab%d"), i))).Sound(NAME_None).StarColor(Gold)
                        .ToolTipText(FText::FromString(FString::Printf(TEXT("%s  (tecla %d)"), TabNames[i], i + 1)))
                        .OnClicked_Lambda([this, i]() { SetPage(i); return FReply::Handled(); })
                        [
                            SNew(SBox).HeightOverride(96.f)
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot().FillHeight(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
                                [SNew(SImage).Image(IconBrush(TabIcons[i], bActive ? 54.f : 46.f))]
                                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Title(Upper(TabNames[i]), 14, Cream)]
                            ]
                        ]
                    ]
                    + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, -6.f, -8.f, 0.f)
                    [
                        // o aviso vermelho pulsa (pontos para gastar, missão pronta)
                        SNew(SBox).WidthOverride(32.f).HeightOverride(32.f)
                        .Visibility_Lambda([this, i, AlertCount]() { return (i == 0 ? FreePoints() : AlertCount) > 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
                        .RenderTransformPivot(FVector2D(.5f, .5f))
                        .RenderTransform_Lambda([]() { return FSlateRenderTransform(FScale2f(1.f + .12f * FMath::Max(0.f, FMath::Sin(float(Now()) * 6.f)))); })
                        [
                            SNew(SOverlay)
                            + SOverlay::Slot()[SNew(SImage).Image(Round(16.f)).ColorAndOpacity(FLinearColor::FromSRGBColor(FColor(20, 20, 50)))]
                            + SOverlay::Slot().Padding(3.f)[SNew(SImage).Image(Round(13.f)).ColorAndOpacity(FLinearColor::FromSRGBColor(FColor(240, 60, 60)))]
                            + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                            [Live(Title(FString::FromInt(AlertCount), 16, Cream), [this, i, AlertCount]() { return FString::FromInt(i == 0 ? FreePoints() : AlertCount); })]
                        ]
                    ]
                ];
            }
            return Tabs;
        }

        // ---- o alto: nome em letra de desenho animado, nível, moedas e fechar
        TSharedRef<SWidget> BuildHeader(const UVortexHeroSheet& Sheet)
        {
            FString Name = TEXT("Herói");
            const UVortexGameInstance* Game = Hero.IsValid() ? Cast<UVortexGameInstance>(Hero->GetGameInstance()) : nullptr;
            if (const UVortexPlayerSave* Save = Game ? Game->GetSelectedHero() : nullptr) Name = Save->HeroName;
            const FVortexRaceInfo* Race = FVortexLookCatalog::Get().Race(Sheet.GetRace());
            return SNew(SHorizontalBox)
                // o nível num selo amarelo
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
                [
                    SNew(SBox).WidthOverride(62.f).HeightOverride(58.f)
                    [
                        SNew(SBorder).BorderImage(&Av::Button(1).Normal).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(0.f, 0.f, 0.f, 6.f))
                        [Title(FString::FromInt(Sheet.GetLevel()), 30, Cream)]
                    ]
                ]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()[Title(Name, 32, Gold)]
                    + SVerticalBox::Slot().AutoHeight()
                    [Label(FString::Printf(TEXT("Nível %d  •  %s  •  %s"), Sheet.GetLevel(), Race ? *Race->Name : TEXT("Humano"), Race ? *Race->Epithet : TEXT("")), 15, Cream)]
                ]
                + SHorizontalBox::Slot().FillWidth(1.f)
                // as moedas numa pílula escura; o número rola e a moeda pula quando muda
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 64.f, 0.f)
                [
                    SNew(SBorder).BorderImage(Av::Panel()).Padding(FMargin(12.f, 6.f, 18.f, 6.f))
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
                        [
                            SNew(SImage).Image(IconBrush(TEXT("ui_coin"), 34.f))
                            .RenderTransformPivot(FVector2D(.5f, .5f))
                            .RenderTransform_Lambda([this]() { return Punch(CoinsChangedAt); })
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [
                            SNew(SBox).MinDesiredWidth(70.f)
                            [
                                SNew(STextBlock).Font(Av::Cartoon(26, 2)).ColorAndOpacity(Gold).Margin(CartoonNudge(26)).ShadowOffset(FVector2D(0.f, 2.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, .1f, .6f))
                                .Text_Lambda([this]() { return FText::FromString(Number(ShownCoins)); })
                            ]
                        ]
                    ]
                ];
        }

        // ---- HERÓI --------------------------------------------------------------------------
        TSharedRef<SWidget> AttributeRow(const UVortexHeroSheet& Sheet, const FVortexHeroStats& S, int32 i, const FLinearColor& Color, int32 Free)
        {
            // 10/11: i é um dos 8 atributos de pontos; o número grande é o oficial principal dele, a linha de baixo os dois
            const FVortexRpgCatalog& Rpg = FVortexRpgCatalog::Get();
            const TArray<int32> Parts = VortexRpg::GroupStats(i);
            const int32 Main = Parts.Num() ? Parts[0] : 0;
            const bool bPercent = VortexRpg::OfficialLabel(Main).StartsWith(TEXT("%"));
            auto Shown = [this, Main, bPercent]()
            {
                const float Total = Stats().Official[Main];
                const FString Text = FMath::IsNearlyEqual(Total, FMath::RoundToFloat(Total)) ? Number(Total) : Number(Total, 1);
                return bPercent ? Text + TEXT("%") : Text;
            };
            auto Detail = [this, Parts]()
            {
                FString Out;
                for (const int32 O : Parts)
                {
                    const bool bPct = VortexRpg::OfficialLabel(O).StartsWith(TEXT("%"));
                    const float V = Stats().Official[O];
                    if (!Out.IsEmpty()) Out += TEXT("  ·  ");
                    Out += VortexRpg::OfficialLabel(O).Replace(TEXT("% "), TEXT("")) + TEXT(" ") + (FMath::IsNearlyEqual(V, FMath::RoundToFloat(V)) ? Number(V) : Number(V, 1)) + (bPct ? TEXT("%") : TEXT(""));
                }
                return Out;
            };
            auto Spent = [this, i]() { const UVortexHeroSheet* Now = GetSheet(); const int32 N = Now ? Now->GetPoints(i) : 0; return N > 0 ? FString::Printf(TEXT("+%d"), N) : FString(); };
            TSharedRef<STextBlock> Value = Live(Title(Shown(), 22, Cream), Shown);
            Value->SetColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([this, i]() { return FSlateColor(LastSpentAttr == i && Now() - LastSpentAt < 1.5 ? Good : Cream); }));
            TSharedRef<STextBlock> Plus = Title(TEXT("+"), 24, Cream);
            Plus->SetColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([this]() { return FSlateColor(FreePoints() > 0 ? Cream : Alpha(Cream, .5f)); }));
            // o próximo ponto pela curva do nível (Data/Rpg/balance.json): rende menos quanto mais pontos já tem
            FString Gains;
            for (const int32 O : Parts)
            {
                const float Next = Rpg.PointValue(O, Sheet.GetPoints(i) + 1, Sheet.GetLevel()) - Rpg.PointValue(O, Sheet.GetPoints(i), Sheet.GetLevel());
                const FString PerPoint = Next >= 10.f ? Number(Next) : Number(Next, Next >= 1.f ? 1 : 2);
                const bool bPct = VortexRpg::OfficialLabel(O).StartsWith(TEXT("%"));
                Gains += FString::Printf(TEXT("\n  %s +%s%s"), *VortexRpg::OfficialLabel(O).Replace(TEXT("% "), TEXT("")), *PerPoint, bPct ? TEXT("%") : TEXT(""));
            }
            const FString Name = VortexRpg::GroupLabel(i);
            return SNew(SBox).HeightOverride(58.f)
                .ToolTipText(FText::FromString(FString::Printf(TEXT("%s\nPróximo ponto:%s"), *VortexRpg::GroupHelp(i), *Gains)))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
                    [SNew(SBox).WidthOverride(16.f).HeightOverride(16.f)[SNew(SImage).Image(White()).ColorAndOpacity(Color)]]
                    + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()[Label(Name, 17, Cream, TEXT("Bold"))]
                        + SVerticalBox::Slot().AutoHeight()[Live(Label(Detail(), 12, Muted), Detail)]
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f)
                    [Live(Label(Spent(), 14, Good, TEXT("Bold")), Spent)]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [
                        SNew(SBox).WidthOverride(76.f).HAlign(HAlign_Right)
                        [
                            SNew(SBox).RenderTransformPivot(FVector2D(.5f, .5f))
                            .RenderTransform_Lambda([this, i]() { return LastSpentAttr == i ? Punch(LastSpentAt) : TOptional<FSlateRenderTransform>(); })
                            [Value]
                        ]
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f, 0.f, 0.f)
                    [
                        SNew(SJuicyButton).ButtonStyle(&ButtonStyle(6)).IsEnabled_Lambda([this]() { return FreePoints() > 0; }).ContentPadding(FMargin(0.f))
                        .Id(FName(*FString::Printf(TEXT("attr%d"), i))).StarColor(Good)
                        .OnClicked_Lambda([this, i, Color]()
                        {
                            if (UVortexHeroSheet* Target = GetSheet()) Target->ServerSpendPoint(uint8(i));
                            UiSound(TEXT("ui_c_point"), .65f);
                            FloatUp(TEXT("+1"), Good);
                            Burst(Color, 7, 1.f);
                            LastSpentAttr = i;
                            LastSpentAt = Now();
                            return FReply::Handled();
                        })
                        [SNew(SBox).WidthOverride(42.f).HeightOverride(36.f).HAlign(HAlign_Center).VAlign(VAlign_Center)[Plus]]
                    ]
                ];
        }

        TSharedRef<SWidget> BuildCharacter(const UVortexHeroSheet& Sheet)
        {
            const FVortexHeroStats S = Sheet.ComputeStats();
            const FVortexRaceInfo* Race = FVortexLookCatalog::Get().Race(Sheet.GetRace());
            const int32 Free = Sheet.GetPointsFree();
            // à esquerda: o herói ao vivo, o nível e a barra de XP
            const int64 Low = FVortexRpgCatalog::XpForLevel(Sheet.GetLevel()), High = FVortexRpgCatalog::XpForLevel(Sheet.GetLevel() + 1);
            const float Into = High > Low ? float(double(Sheet.GetXp() - Low) / double(High - Low)) : 1.f;
            TSharedRef<SVerticalBox> Left = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[DollView(300.f, 360.f)]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1.f)[Title(FString::Printf(TEXT("NÍVEL %d"), Sheet.GetLevel()), 22, Gold)]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [Label(FString::Printf(TEXT("%s / %s XP"), *Number(float(Sheet.GetXp() - Low)), *Number(float(High - Low))), 14, Muted, TEXT("Bold"))]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)[Bar(TOptional<float>(Into), Gold, 18.f)]
                + SVerticalBox::Slot().AutoHeight()[Label(FString::Printf(TEXT("Faltam %s XP para o nível %d"), *Number(float(High - Sheet.GetXp())), Sheet.GetLevel() + 1), 14, Muted)];

            // no meio: os 8 atributos de pontos (10/11) em três grupos
            struct FGroup { const TCHAR* Name; FLinearColor Color; TArray<int32> Items; };
            const FGroup Groups[] = {
                { TEXT("Ataque"), FLinearColor::FromSRGBColor(FColor(0xF0, 0x86, 0x5A)), { VortexRpg::GStrength, VortexRpg::GPrecision, VortexRpg::GAgility } },
                { TEXT("Sobrevivência"), FLinearColor::FromSRGBColor(FColor(0x7C, 0xD0, 0x80)), { VortexRpg::GVitality, VortexRpg::GDefense, VortexRpg::GMagicResist, VortexRpg::GVampirism } },
                { TEXT("Magia"), FLinearColor::FromSRGBColor(FColor(0x8A, 0xA8, 0xF6)), { VortexRpg::GMagic } } };
            TSharedRef<STextBlock> PointsLeft = Live(Title(TEXT(""), 18, Cream), [this]() { const int32 N = FreePoints(); return N > 0 ? FString::Printf(TEXT("%d PONTOS"), N) : FString(TEXT("SEM PONTOS")); });
            PointsLeft->SetColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([this]() { return FSlateColor(FreePoints() > 0 ? Cream : Muted); }));
            TSharedRef<SVerticalBox> Mid = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1.f)[Heading(TEXT("Atributos"))]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(10.f, 0.f, 0.f, 0.f)
                    [
                        SNew(SBorder).BorderImage_Lambda([this]() { return FreePoints() > 0 ? Av::Ribbon(FLinearColor::FromSRGBColor(FColor(0x5C, 0xC8, 0x3C))) : Av::Panel(); }).Padding(FMargin(22.f, 6.f, 22.f, 12.f))
                        .RenderTransformPivot(FVector2D(.5f, .5f))
                        .RenderTransform_Lambda([this]() { return FreePoints() > 0 ? FSlateRenderTransform(FScale2f(1.f + .05f * FMath::Max(0.f, FMath::Sin(float(Now()) * 5.f)))) : FSlateRenderTransform(); })
                        [PointsLeft]
                    ]
                ];
            for (const FGroup& G : Groups)
            {
                TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
                for (const int32 i : G.Items) Rows->AddSlot().AutoHeight()[AttributeRow(Sheet, S, i, G.Color, Free)];
                Mid->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 8.f)
                [
                    SNew(SBorder).BorderImage(Av::Card()).Padding(FMargin(16.f, 10.f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)[Title(Upper(G.Name), 18, G.Color)]
                        + SVerticalBox::Slot().AutoHeight()[Rows]
                    ]
                ];
            }

            // à direita: os números do combate em blocos, o resto em lista curta, e a raça
            auto Mult = [](float M) { return M <= 0.f ? FString(TEXT("não usa")) : FString::Printf(TEXT("%s%d%%"), M >= 1.f ? TEXT("+") : TEXT(""), FMath::RoundToInt((M - 1.f) * 100.f)); };
            auto MultColor = [](float M, bool bLowGood = false) { return FMath::IsNearlyEqual(M, 1.f) ? Cream : ((M > 1.f) != bLowGood ? Good : Bad); };
            TSharedRef<SUniformGridPanel> Tiles = SNew(SUniformGridPanel).SlotPadding(FMargin(4.f));
            Tiles->AddSlot(0, 0)[StatTile(TEXT("Vida"), [this]() { return Number(Stats().MaxHealth); }, Health)];
            Tiles->AddSlot(1, 0)[StatTile(TEXT("Mana"), [this]() { return Number(Stats().MaxMana); }, ManaBlue)];
            Tiles->AddSlot(0, 1)[StatTile(TEXT("Ataque"), [this]() { return FString::Printf(TEXT("%s%s"), Stats().Attack >= 0.f ? TEXT("+") : TEXT(""), *Number(Stats().Attack)); }, Gold)];
            Tiles->AddSlot(1, 1)[StatTile(TEXT("Defesa"), [this]() { return Number(Stats().Defense); }, Cream)];
            Tiles->AddSlot(0, 2)[StatTile(TEXT("Crítico"), [this]() { return FString::Printf(TEXT("%s%%"), *Number(Stats().Crit * 100.f, 1)); }, Cream)];
            Tiles->AddSlot(1, 2)[StatTile(TEXT("Magia"), [this]() { return FString::Printf(TEXT("+%s"), *Number(Stats().SkillPower)); }, ManaBlue)];
            TSharedRef<SVerticalBox> Right = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[Heading(TEXT("No combate"))]
                + SVerticalBox::Slot().AutoHeight()[Tiles];
            auto Row = [this, &Right](const FString& Name, TFunction<FString()> Value, TFunction<bool()> bGood)
            {
                TSharedRef<STextBlock> Shown = Live(Label(Value(), 15, Cream, TEXT("Bold")), Value);
                Shown->SetColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([bGood]() { return FSlateColor(bGood() ? Good : Cream); }));
                Right->AddSlot().AutoHeight().Padding(4.f, 3.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1.f)[Label(Name, 15, Muted)]
                    + SHorizontalBox::Slot().AutoWidth()[Shown]
                ];
            };
            auto Never = []() { return false; };
            Right->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f);
            Row(TEXT("Golpe à distância"), [this]() { return FString::Printf(TEXT("+%s"), *Number(Stats().RangedAttack)); }, Never);
            Row(TEXT("Resistência à magia"), [this]() { return FString::Printf(TEXT("-%s%%"), *Number(Stats().MagicReduction() * 100.f)); }, Never);
            Row(TEXT("Dano crítico"), [this]() { return FString::Printf(TEXT("x%s"), *Number(Stats().CritMult, 2)); }, Never);
            Row(TEXT("Velocidade de ataque"), [this]() { return FString::Printf(TEXT("x%s"), *Number(Stats().AttackSpeed, 2)); }, [this]() { return Stats().AttackSpeed > 1.f; });
            Row(TEXT("Recargas"), [this]() { return FString::Printf(TEXT("%s%%"), *Number(Stats().CooldownScale * 100.f)); }, [this]() { return Stats().CooldownScale < 1.f; });
            Row(TEXT("Roubo de vida"), [this]() { return FString::Printf(TEXT("%s%%"), *Number(Stats().Lifesteal * 100.f, 1)); }, [this]() { return Stats().Lifesteal > 0.f; });
            Row(TEXT("Vida por segundo"), [this]() { return Number(Stats().HpRegen, 2); }, Never);
            Row(TEXT("Mana por segundo"), [this]() { return Number(Stats().MpRegen, 2); }, Never);
            // a raça: a arte, o nome, os traços e o que ela muda no combate
            TSharedRef<SVerticalBox> Traits = SNew(SVerticalBox);
            if (Race)
            {
                for (const FString& Trait : Race->Traits)
                {
                    const bool bBad = Race->IsWeakness(Trait);
                    Traits->AddSlot().AutoHeight().Padding(0.f, 1.f)[Label(FString::Printf(TEXT("%s %s"), bBad ? TEXT("-") : TEXT("+"), *Trait), 14, bBad ? Bad : Good, TEXT("Regular"), 0, true)];
                }
            }
            // fica embaixo do herói, onde sobra espaço (na coluna da direita ela era cortada)
            Left->AddSlot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
            [
                SNew(SBorder).BorderImage(Av::Card()).Padding(FMargin(12.f, 10.f))
                .ToolTipText(FText::FromString(FString::Printf(TEXT("Corpo a corpo %s  •  Distância %s  •  Magia %s  •  Dano recebido %s"),
                    *Mult(S.Race.Melee), *Mult(S.Race.Ranged), *Mult(S.Race.Magic), *Mult(S.Race.Taken))))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 0.f, 10.f, 0.f)
                    [SNew(SBox).WidthOverride(84.f).HeightOverride(84.f)[SNew(SImage).Image(RaceBrush(Sheet.GetRace(), 84.f))]]
                    + SHorizontalBox::Slot().FillWidth(1.f)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()[Title(Race ? Upper(Race->Name) : TEXT("HUMANO"), 20, Race ? Race->Accent : Cream)]
                        + SVerticalBox::Slot().AutoHeight()[Traits]
                    ]
                ]
            ];
            (void)MultColor;

            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(340.f)[Box(Left, FMargin(18.f))]]
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(16.f, 0.f)[Box(SNew(SScrollBox) + SScrollBox::Slot()[Mid])]
                + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(340.f)[Box(SNew(SScrollBox) + SScrollBox::Slot()[Right])]];
        }

        // ---- MOCHILA ------------------------------------------------------------------------
        bool PassesFilter(const FVortexItemDef* Def) const
        {
            if (Filter.IsEmpty()) return true;
            return Def && Def->Type == Filter;
        }

        void ClickBag(int32 Index)
        {
            const double Now = FSlateApplication::Get().GetCurrentTime();
            if (LastClicked == Index && Now - LastClickTime < .35) UseBag(Index);
            LastClicked = Index;
            LastClickTime = Now;
            Selected = Index;
            SelectedEquip = INDEX_NONE;
            UiSound(TEXT("ui_c_select"), .5f);
            // 07/11 (o usuário: "toda vez que clico num item a página recarrega"): escolher só troca a moldura e o painel
            // da direita; usar (o duplo clique) muda a mochila e aí a janela se refaz pela revisão da ficha
            if (const UVortexHeroSheet* Sheet = GetSheet(); Sheet && DetailBox) DetailBox->SetContent(Detail(*Sheet));
        }

        void UseBag(int32 Index)
        {
            UVortexHeroSheet* Sheet = GetSheet();
            const FVortexItemDef* Def = Sheet && Sheet->GetBag().IsValidIndex(Index) ? FVortexRpgCatalog::Get().Item(Sheet->GetBag()[Index].Item) : nullptr;
            if (Sheet) Sheet->ServerUseSlot(uint8(Index));
            // poção: glub glub e bolhas verdes; equipamento: baque e brilho na cor da raridade
            UiSound(Def && Def->IsUsable() ? TEXT("ui_c_use") : TEXT("ui_c_equip"), .65f);
            if (Def) Burst(Def->IsUsable() ? Good : FVortexRpgCatalog::RarityColor(Def->Rarity), 8, 1.1f);
        }

        /** O mouse passou num item: o painel da direita mostra ele (sem refazer a janela inteira). */
        void ShowHover(int32 BagIndex, int32 EquipSlot)
        {
            HoverBag = BagIndex;
            HoverEquip = EquipSlot;
            if (const UVortexHeroSheet* Sheet = GetSheet(); Sheet && DetailBox) DetailBox->SetContent(Detail(*Sheet));
        }

        TSharedRef<SWidget> BagSlot(const UVortexHeroSheet& Sheet, int32 Index)
        {
            const FVortexItemStack& Stack = Sheet.GetBag()[Index];
            const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Stack.Item);
            const bool bShown = Def && PassesFilter(Def);
            const bool bSelected = Selected == Index;
            constexpr float Size = 76.f;
            TSharedRef<SWidget> Content = Def
                ? IconTile(Def->Icon, Size, FVortexRpgCatalog::RarityColor(Def->Rarity), Stack.Count, !bShown)
                : StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(Size).HeightOverride(Size)[SNew(SImage).Image(Av::Slot(0))]);
            return SNew(SBorder).BorderImage(White()).BorderBackgroundColor(FLinearColor::Transparent).Padding(0.f)
                .OnMouseButtonDown_Lambda([this, Index, bHas = Def != nullptr](const FGeometry&, const FPointerEvent& Event)
                {
                    if (Event.GetEffectingButton() == EKeys::RightMouseButton && bHas)
                    {
                        UseBag(Index);
                        return FReply::Handled();
                    }
                    return FReply::Unhandled();
                })
                [
                    SNew(SJuicyButton).ButtonStyle(&ButtonStyle(3)).ContentPadding(0.f).Id(FName(*FString::Printf(TEXT("bag%d"), Index))).Stars(Def != nullptr)
                    .StarColor(Def ? FVortexRpgCatalog::RarityColor(Def->Rarity) : Cream)
                    .OnClicked_Lambda([this, Index]() { ClickBag(Index); return FReply::Handled(); })
                    .OnHovered_Lambda([this, Index, bHas = Def != nullptr]() { if (bHas) ShowHover(Index, INDEX_NONE); })
                    .OnUnhovered_Lambda([this]() { ShowHover(INDEX_NONE, INDEX_NONE); })
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()[Content]
                        + SOverlay::Slot()[SNew(SImage).Image(Av::Slot(2)).Visibility_Lambda([this, Index]() { return Selected == Index ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })]
                    ]
                ];
        }

        TSharedRef<SWidget> EquipSlot(const UVortexHeroSheet& Sheet, int32 Slot)
        {
            const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Sheet.GetEquipped(Slot));
            const bool bSelected = SelectedEquip == Slot;
            static const FName Empty[] = { TEXT("sword_iron"), TEXT("helm_leather"), TEXT("chest_tunic"), TEXT("chest_tunic"), TEXT("chest_tunic"), TEXT("ring_jade") };
            constexpr float Size = 80.f;
            return SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                [
                    SNew(SJuicyButton).ButtonStyle(&ButtonStyle(3)).ContentPadding(0.f).Id(FName(*FString::Printf(TEXT("eq%d"), Slot))).Stars(Def != nullptr)
                    .StarColor(Def ? FVortexRpgCatalog::RarityColor(Def->Rarity) : Cream)
                    .ToolTipText(FText::FromString(Def ? Def->Name : VortexRpg::EquipLabel(Slot) + TEXT(": vazio")))
                    .OnClicked_Lambda([this, Slot]()
                    {
                        SelectedEquip = Slot;
                        Selected = INDEX_NONE;
                        UiSound(TEXT("ui_c_select"), .5f);
                        if (const UVortexHeroSheet* Sheet = GetSheet(); Sheet && DetailBox) DetailBox->SetContent(Detail(*Sheet));
                        return FReply::Handled();
                    })
                    .OnHovered_Lambda([this, Slot, bHas = Def != nullptr]() { if (bHas) ShowHover(INDEX_NONE, Slot); })
                    .OnUnhovered_Lambda([this]() { ShowHover(INDEX_NONE, INDEX_NONE); })
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()[Def ? IconTile(Def->Icon, Size, FVortexRpgCatalog::RarityColor(Def->Rarity)) : IconTile(Empty[Slot], Size, Muted, 0, true)]
                        + SOverlay::Slot()[SNew(SImage).Image(Av::Slot(2)).Visibility_Lambda([this, Slot]() { return SelectedEquip == Slot ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })]
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 2.f, 0.f, 0.f)[Title(Upper(VortexRpg::EquipLabel(Slot)), 12, bSelected ? Gold : Muted)];
        }

        /** O item do painel da direita: o do mouse, senão o escolhido; equipamento compara com o que está no corpo. */
        TSharedRef<SWidget> Detail(const UVortexHeroSheet& Sheet)
        {
            const bool bHover = HoverBag != INDEX_NONE || HoverEquip != INDEX_NONE;
            const int32 BagIndex = bHover ? HoverBag : Selected;
            const int32 EquipIndex = bHover ? HoverEquip : SelectedEquip;
            const bool bEquip = EquipIndex != INDEX_NONE;
            const FName Item = bEquip ? Sheet.GetEquipped(EquipIndex) : Sheet.GetBag().IsValidIndex(BagIndex) ? Sheet.GetBag()[BagIndex].Item : NAME_None;
            const FVortexRpgCatalog& Catalog = FVortexRpgCatalog::Get();
            const FVortexItemDef* Def = Catalog.Item(Item);
            if (!Def)
            {
                return SNew(SVerticalBox)
                    + SVerticalBox::Slot().FillHeight(1.f).VAlign(VAlign_Center).HAlign(HAlign_Center)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[IconTile(TEXT("ui_backpack"), 110.f, Muted, 0, true)]
                        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 12.f, 0.f, 0.f)[Title(TEXT("NENHUM ITEM"), 20, Muted)]
                        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f)[Label(TEXT("Passe o mouse num item para ver o que ele faz."), 15, Muted, TEXT("Regular"), 0, true)]
                    ];
            }
            const FLinearColor Color = FVortexRpgCatalog::RarityColor(Def->Rarity);
            // o que está no corpo no mesmo lugar (para comparar)
            const int32 EquipOf = VortexRpg::EquipSlotOf(Def->Slot);
            const FVortexItemDef* Worn = !bEquip && EquipOf != INDEX_NONE ? Catalog.Item(Sheet.GetEquipped(EquipOf)) : nullptr;
            TSharedRef<SVerticalBox> V = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                [
                    SNew(SBorder).BorderImage(Av::Ribbon(FMath::Lerp(Color, FLinearColor::Black, .1f))).Padding(FMargin(26.f, 4.f, 26.f, 10.f))
                    [Title(Upper(FVortexRpgCatalog::RarityLabel(Def->Rarity)), 15, Cream)]
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 10.f, 0.f, 8.f)[IconTile(Def->Icon, 116.f, Color)]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Title(Def->Name, 22, Color, true)]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 2.f, 0.f, 10.f)[Label(FVortexRpgCatalog::TypeLabel(Def->Type), 14, Muted)]
                // 07/11: o nível do item; em vermelho quando o herói ainda não pode vestir
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 8.f)
                [
                    Def->ReqLevel <= 0 ? SNullWidget::NullWidget : StaticCastSharedRef<SWidget>(Label(Def->ReqLevel > Sheet.GetLevel()
                        ? FString::Printf(TEXT("Nível %d  •  precisa do nível %d (você está no %d)"), Def->Level, Def->ReqLevel, Sheet.GetLevel())
                        : FString::Printf(TEXT("Nível %d"), Def->Level), 15, Def->ReqLevel > Sheet.GetLevel() ? FLinearColor(1.f, .35f, .3f) : Cream))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[Label(Def->Description, 15, Cream, TEXT("Regular"), 0, true)];
            // atributos do item; se for trocar o do corpo, a diferença ao lado (verde ganha, vermelho perde)
            TSet<FName> Keys;
            for (const TPair<FName, float>& Stat : Def->Stats) Keys.Add(Stat.Key);
            if (Worn) for (const TPair<FName, float>& Stat : Worn->Stats) Keys.Add(Stat.Key);
            for (const FName Key : Keys)
            {
                const float Mine = Def->Stats.FindRef(Key);
                const float Old = Worn ? Worn->Stats.FindRef(Key) : 0.f;
                if (FMath::IsNearlyZero(Mine) && FMath::IsNearlyZero(Old)) continue;
                const float Delta = Mine - Old;
                TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1.f)[Label(FVortexRpgCatalog::StatLine(Key, Mine), 15, FMath::IsNearlyZero(Mine) ? Muted : Mine > 0.f ? Good : Bad, TEXT("Bold"))];
                if (Worn && !FMath::IsNearlyZero(Delta))
                {
                    Line->AddSlot().AutoWidth()[Title(FString::Printf(TEXT("%s%s"), Delta > 0.f ? TEXT("+") : TEXT(""), *Number(Delta, FMath::IsNearlyEqual(Delta, FMath::RoundToFloat(Delta)) ? 0 : 1)), 16, Delta > 0.f ? Good : Bad)];
                }
                V->AddSlot().AutoHeight().Padding(0.f, 1.f)[Line];
            }
            if (Worn) V->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[Label(FString::Printf(TEXT("comparando com: %s"), *Worn->Name), 13, Muted, TEXT("Regular"), 0, true)];
            if (Def->Heal > 0.f) V->AddSlot().AutoHeight().Padding(0.f, 1.f)[Label(FString::Printf(TEXT("Recupera %s de vida"), *Number(Def->Heal)), 15, Good, TEXT("Bold"))];
            if (Def->Mana > 0.f) V->AddSlot().AutoHeight().Padding(0.f, 1.f)[Label(FString::Printf(TEXT("Recupera %s de mana"), *Number(Def->Mana)), 15, Good, TEXT("Bold"))];
            // arsenal: elemento e peculiaridade da arma, conjunto da armadura (quantas peças estão vestidas)
            if (!Def->Element.IsNone() || !Def->Trait.IsNone())
            {
                TArray<FString> Parts;
                if (const FString* E = Catalog.ElementNames.Find(Def->Element)) Parts.Add(*E);
                if (const FString* T = Catalog.TraitNames.Find(Def->Trait)) Parts.Add(*T);
                const FLinearColor Tint = Def->Element.IsNone() ? Gold : AVortexProjectile::ElementColor(Def->Element);
                V->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 1.f)[Title(Upper(FString::Join(Parts, TEXT("  •  "))), 15, Tint, true)];
                if (!Def->EffectText.IsEmpty() && !Def->Description.Contains(Def->EffectText)) V->AddSlot().AutoHeight().Padding(0.f, 1.f)[Label(Def->EffectText, 14, Cream, TEXT("Regular"), 0, true)];
            }
            if (const FVortexItemSet* Set = Catalog.Sets.Find(Def->Set))
            {
                int32 WornCount = 0;
                for (const FName Piece : Set->Pieces)
                    for (int32 Slot = 0; Slot < VortexRpg::NumEquip; ++Slot) WornCount += Sheet.GetEquipped(Slot) == Piece;
                V->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 1.f)
                [Label(FString::Printf(TEXT("Conjunto %s  (%d/%d)"), *Set->Name, WornCount, Set->Pieces.Num()), 15, WornCount >= 2 ? Good : Gold, TEXT("Bold"), 0, true)];
                for (const TPair<FName, float>& B : Set->Bonus2)
                    V->AddSlot().AutoHeight()[Label(TEXT("2 peças: ") + FVortexRpgCatalog::StatLine(B.Key, B.Value), 14, WornCount >= 2 ? Good : Muted, TEXT("Regular"), 0, true)];
                for (const TPair<FName, float>& B : Set->Bonus4)
                    V->AddSlot().AutoHeight()[Label(TEXT("4 peças: ") + FVortexRpgCatalog::StatLine(B.Key, B.Value), 14, WornCount >= 4 ? Good : Muted, TEXT("Regular"), 0, true)];
            }
            if (Def->Price > 0)
            {
                V->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)[SNew(SImage).Image(IconBrush(TEXT("ui_coin"), 22.f))]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Title(FString::FromInt(Def->Price), 18, Gold)]
                ];
            }
            // o botão: só para o escolhido (passando o mouse é só para ver)
            FString Action;
            if (!bHover)
            {
                if (bEquip) Action = TEXT("Desequipar");
                else if (Def->IsUsable()) Action = TEXT("Usar");
                else if (Def->IsEquipment()) Action = TEXT("Equipar");
            }
            if (!Action.IsEmpty())
            {
                const int32 EquipChosen = SelectedEquip, BagChosen = Selected;
                V->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
                [
                    BlockButton(Action, bEquip ? 5 : Def->IsUsable() ? 6 : 1, true, 56.f, [this, bEquip, EquipChosen, BagChosen]()
                    {
                        if (UVortexHeroSheet* Target = GetSheet())
                        {
                            if (bEquip) Target->ServerUnequip(uint8(EquipChosen));
                            else Target->ServerUseSlot(uint8(BagChosen));
                        }
                        const FVortexItemDef* Chosen = bEquip ? nullptr : GetSheet() && GetSheet()->GetBag().IsValidIndex(BagChosen) ? FVortexRpgCatalog::Get().Item(GetSheet()->GetBag()[BagChosen].Item) : nullptr;
                        UiSound(Chosen && Chosen->IsUsable() ? TEXT("ui_c_use") : TEXT("ui_c_equip"), .65f);
                        Burst(Chosen && Chosen->IsUsable() ? Good : Gold, 8, 1.1f);
                        SelectedEquip = INDEX_NONE;
                    })
                ];
            }
            // 07/11: jogar no chão (o item fica rodando lá: AVortexItemDrop)
            if (!bHover && !bEquip && Selected != INDEX_NONE)
            {
                const int32 BagChosen = Selected;
                V->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
                [
                    BlockButton(TEXT("Jogar no chão"), 5, true, 44.f, [this, BagChosen]()
                    {
                        if (UVortexHeroSheet* Target = GetSheet()) Target->ServerDropSlot(uint8(BagChosen));
                        UiSound(TEXT("ui_c_equip"), .5f);
                        Selected = INDEX_NONE;
                    })
                ];
            }
            return SNew(SScrollBox) + SScrollBox::Slot()[V];
        }

        TSharedRef<SWidget> BuildBag(const UVortexHeroSheet& Sheet)
        {
            // à esquerda: o herói ao vivo com o que veste em volta (cabeça, peito, pernas, botas | arma, joia)
            TSharedRef<SVerticalBox> LeftSlots = SNew(SVerticalBox);
            for (const int32 Slot : { int32(VortexRpg::Head), int32(VortexRpg::Chest), int32(VortexRpg::Legs), int32(VortexRpg::Boots) })
                LeftSlots->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[EquipSlot(Sheet, Slot)];
            TSharedRef<SVerticalBox> RightSlots = SNew(SVerticalBox);
            for (const int32 Slot : { int32(VortexRpg::Weapon), int32(VortexRpg::Jewel) })
                RightSlots->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[EquipSlot(Sheet, Slot)];
            const FVortexHeroStats S = Sheet.ComputeStats();
            TSharedRef<SUniformGridPanel> Quick = SNew(SUniformGridPanel).SlotPadding(FMargin(3.f));
            Quick->AddSlot(0, 0)[StatTile(TEXT("Vida"), Number(S.MaxHealth), Health)];
            Quick->AddSlot(1, 0)[StatTile(TEXT("Ataque"), FString::Printf(TEXT("+%s"), *Number(S.Attack)), Gold)];
            Quick->AddSlot(2, 0)[StatTile(TEXT("Defesa"), Number(S.Defense), Cream)];
            TSharedRef<SVerticalBox> Paper = SNew(SVerticalBox)
                + SVerticalBox::Slot().FillHeight(1.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[LeftSlots]
                    + SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(8.f, 0.f)[DollView(220.f, 370.f)]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[RightSlots]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)[Quick];

            // no meio: a mochila (filtros, contagem, a grade de 6x5)
            TSharedRef<SWrapBox> Filters = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6.f, 6.f));
            const TPair<const TCHAR*, const TCHAR*> Kinds[] = { { TEXT("Tudo"), TEXT("") }, { TEXT("Armadura"), TEXT("armor") },
                { TEXT("Armas"), TEXT("weapon") }, { TEXT("Uso"), TEXT("consumable") }, { TEXT("Materiais"), TEXT("material") }, { TEXT("Joias"), TEXT("accessory") } };
            for (const TPair<const TCHAR*, const TCHAR*>& Kind : Kinds)
            {
                const FString Type = Kind.Value;
                const bool bOn = Filter == Type;
                Filters->AddSlot()
                [
                    SNew(SJuicyButton).ButtonStyle(&ButtonStyle(bOn ? 1 : 0)).ContentPadding(FMargin(12.f, 6.f, 12.f, 6.f)).Id(FName(TEXT("filter_") + Type)).Stars(false)
                    .OnClicked_Lambda([this, Type]() { Filter = Type; Rebuild(); return FReply::Handled(); })
                    [Title(Upper(Kind.Key), 14, Cream)]
                ];
            }
            int32 Used = 0;
            TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(3.f));
            for (int32 i = 0; i < Sheet.GetBag().Num(); ++i)
            {
                if (!Sheet.GetBag()[i].IsEmpty()) ++Used;
                Grid->AddSlot(i % 5, i / 5)[BagSlot(Sheet, i)];
            }
            TSharedRef<SVerticalBox> BagCol = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1.f)[Filters]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(10.f, 2.f, 0.f, 0.f)[Title(FString::Printf(TEXT("%d/%d"), Used, Sheet.GetBag().Num()), 20, Used >= Sheet.GetBag().Num() ? Bad : Muted)]
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Grid];

            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(430.f)[Box(Paper, FMargin(14.f))]]
                + SHorizontalBox::Slot().AutoWidth().Padding(14.f, 0.f)[SNew(SBox).WidthOverride(440.f)[Box(BagCol, FMargin(14.f))]]
                + SHorizontalBox::Slot().FillWidth(1.f)[Box(SAssignNew(DetailBox, SBox)[Detail(Sheet)], FMargin(16.f))];
        }

        // ---- TÉCNICAS -----------------------------------------------------------------------
        TSharedRef<SWidget> BuildSkills(const UVortexHeroSheet& Sheet)
        {
            const FVortexRpgCatalog& Rpg = FVortexRpgCatalog::Get();
            const FName WeaponItem = Sheet.GetEquipped(VortexRpg::Weapon);
            const FVortexItemDef* Weapon = Rpg.Item(WeaponItem);
            // 07/11 (o usuário: "a espada só tem o golpe normal e o voador, que outros são esses?"): as quatro técnicas do
            // Godot por nível (rpg.json "skills") nunca foram feitas aqui, e a defesa no F não existe. A página mostra o que
            // a arma faz de verdade: o combo e o poder do botão direito (o da espada ou do arco, Data/Rpg/swords.json e
            // bows.json; senão o da arma, VortexWeaponKit)
            TSharedRef<SUniformGridPanel> Cards = SNew(SUniformGridPanel).SlotPadding(FMargin(8.f));
            const FName Type = Sheet.GetWeaponType();
            const bool bBow = Type.ToString().StartsWith(TEXT("arco"));
            const VortexSwords::FSword* Own = VortexSwords::Find(WeaponItem);
            const VortexWeaponKit::FKit* Kit = Own && Own->bHasKit ? &Own->Kit : VortexWeaponKit::Find(Type);
            auto Card = [&](int32 Column, const FString& Name, const TCHAR* Category, const FString& Text, FName Icon, const FString& Cost, const TCHAR* Key)
            {
                Cards->AddSlot(Column, 0)
                [
                    SNew(SBorder).BorderImage(Av::Card(false)).Padding(FMargin(16.f, 14.f))
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 0.f, 14.f, 0.f)[IconTile(Icon, 84.f, Gold, 0, false)]
                        + SHorizontalBox::Slot().FillWidth(1.f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[Title(Name, 19, Cream, true)]
                            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.f, 3.f, 0.f, 0.f)
                            [SNew(SBorder).BorderImage(Av::Panel()).Padding(FMargin(8.f, 2.f))[Label(Category, 12, Gold, TEXT("Bold"), 80)]]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[Label(Text, 15, Cream, TEXT("Regular"), 0, true)]
                            + SVerticalBox::Slot().AutoHeight()[Label(Cost, 14, ManaBlue, TEXT("Bold"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[Title(Key, 15, Good, true)]
                        ]
                    ]
                ];
            };
            Card(0, bBow ? TEXT("Disparo") : TEXT("Combo"), TEXT("GOLPE"),
                bBow ? TEXT("Segure para puxar a corda e solte: quanto mais puxa, mais forte e mais reta vai a flecha.")
                     : TEXT("Clique em sequência: a série de golpes da arma; o último desequilibra."),
                Weapon ? Weapon->Icon : FName(TEXT("weapon_") + Type.ToString()), TEXT("Sem custo"), bBow ? TEXT("SEGURE E SOLTE  •  CLIQUE ESQUERDO") : TEXT("CLIQUE ESQUERDO"));
            const FString PowerName = Own && !Own->PowerName.IsEmpty() ? Own->PowerName : FString(TEXT("Golpe especial"));
            const FString PowerText = Own && !Own->PowerText.IsEmpty() ? Own->PowerText : FString(TEXT("O golpe especial desta arma."));
            const FName PowerIcon = Own && Own->bHasKit ? Own->PowerIcon : FName(TEXT("special_") + Type.ToString());
            const FString Cost = Kit ? FString::Printf(TEXT("Mana %d  •  Recarga %s s"), FMath::RoundToInt(Kit->SpecialMana),
                *Number(Kit->SpecialCooldown, Kit->SpecialCooldown == FMath::RoundToFloat(Kit->SpecialCooldown) ? 0 : 1)) : FString();
            Card(1, PowerName, TEXT("PODER"), PowerText, PowerIcon, Cost, TEXT("BOTÃO DIREITO"));
            TSharedRef<SVerticalBox> Keys = SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[Heading(TEXT("Comandos"))];
            const TPair<const TCHAR*, const TCHAR*> Commands[] = {
                { TEXT("CLIQUE E"), TEXT("Combo de golpes") }, { TEXT("CLIQUE D"), TEXT("Golpe especial da arma") },
                { TEXT("CTRL"), TEXT("Esquiva") },
                { TEXT("SHIFT"), TEXT("Correr") }, { TEXT("Q  R"), TEXT("Poção de vida / de mana") }, { TEXT("H"), TEXT("Recuperar o fôlego") },
                { TEXT("1-4"), TEXT("Habilidades da classe") }, { TEXT("I"), TEXT("Esta janela") }, { TEXT("M"), TEXT("Mapa") }, { TEXT("E"), TEXT("Falar, colher, montar") },
            };
            for (const TPair<const TCHAR*, const TCHAR*>& Command : Commands)
            {
                Keys->AddSlot().AutoHeight().Padding(0.f, 4.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(120.f).HAlign(HAlign_Left)[KeyCap(Command.Key)]]
                    + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(10.f, 0.f, 0.f, 0.f)[Label(Command.Value, 15, Cream, TEXT("Regular"), 0, true)]
                ];
            }
            TSharedRef<SVerticalBox> Main = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 14.f, 0.f)
                    [IconTile(Weapon ? Weapon->Icon : FName(TEXT("sword_iron")), 76.f, Weapon ? FVortexRpgCatalog::RarityColor(Weapon->Rarity) : Muted)]
                    + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()[Title(Weapon ? Weapon->Name : TEXT("Sem arma"), 28, Weapon ? FVortexRpgCatalog::RarityColor(Weapon->Rarity) : Muted)]
                        + SVerticalBox::Slot().AutoHeight()[Label(TEXT("O que esta arma faz: o golpe e o poder do botão direito"), 15, Muted)]
                    ]
                ]
                + SVerticalBox::Slot().FillHeight(1.f)[Cards];
            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 16.f, 0.f)[Box(Main)]
                + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(400.f)[Box(SNew(SScrollBox) + SScrollBox::Slot()[Keys])]];
        }

        // ---- CLASSES ------------------------------------------------------------------------
        static FLinearColor ClassColor(const FString& Rarity)
        {
            if (Rarity == TEXT("rare")) return FLinearColor::FromSRGBColor(FColor(0x6F, 0xA8, 0xF0));
            if (Rarity == TEXT("epic")) return FLinearColor::FromSRGBColor(FColor(0xB0, 0x7C, 0xF2));
            if (Rarity == TEXT("legendary")) return FLinearColor::FromSRGBColor(FColor(0xF2, 0xB0, 0x40));
            if (Rarity == TEXT("unique") || Rarity == TEXT("divine")) return FLinearColor::FromSRGBColor(FColor(0xF0, 0x70, 0x70));
            return FLinearColor::FromSRGBColor(FColor(0xC8, 0xC8, 0xC8));
        }

        static FString ClassRarityName(const FString& Rarity)
        {
            if (Rarity == TEXT("rare")) return TEXT("Rara");
            if (Rarity == TEXT("epic")) return TEXT("Épica");
            if (Rarity == TEXT("legendary")) return TEXT("Lendária");
            if (Rarity == TEXT("unique")) return TEXT("Única");
            if (Rarity == TEXT("divine")) return TEXT("Divina");
            return TEXT("Comum");
        }

        static FString BuffLine(const FVortexClassDef& C)
        {
            TArray<FString> Parts;
            for (const TPair<FName, float>& Buff : C.Buffs)
            {
                const int32 Index = VortexRpg::OfficialOf(Buff.Key);
                const FString Name = Index != INDEX_NONE ? VortexRpg::OfficialLabel(Index) : Buff.Key.ToString();
                Parts.Add(FString::Printf(TEXT("+%s %s"), *Number(Buff.Value), *Name));
            }
            return FString::Join(Parts, TEXT("   "));
        }

        TSharedRef<SWidget> ClassCard(const FVortexClassDef& C, bool bPick, FName Race, bool bBig = false)
        {
            const FLinearColor Edge = ClassColor(C.Rarity);
            const float Affinity = C.AffinityFor(Race);
            TSharedRef<SVerticalBox> Body = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[Title(Upper(ClassRarityName(C.Rarity)), 14, Edge)]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)[Title(C.Name, bBig ? 34 : 22, Cream, true)]
                + SVerticalBox::Slot().AutoHeight()[Label(C.Role, 15, Muted, TEXT("Regular"), 0, true)]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)[Label(BuffLine(C), 15, Good, TEXT("Bold"), 0, true)];
            if (!C.PassiveName.IsEmpty())
            {
                Body->AddSlot().AutoHeight()[Label(FString::Printf(TEXT("%s: %s"), *C.PassiveName, *C.PassiveText), 14, Cream, TEXT("Regular"), 0, true)];
            }
            if (Affinity <= 0.f || Affinity > 1.05f)
            {
                Body->AddSlot().AutoHeight().Padding(0.f, 4.f)
                    [Label(Affinity <= 0.f ? TEXT("Sua raça não combina com esta classe") : TEXT("Sua raça tem afinidade com esta classe"), 13, Affinity <= 0.f ? Bad : Good)];
            }
            if (bPick)
            {
                const FName Id = C.Id;
                Body->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
                [
                    BlockButton(TEXT("Despertar como esta"), 1, true, 44.f, [this, Id]()
                    {
                        if (UVortexHeroSheet* Target = GetSheet()) Target->ServerChooseClass(Id);
                        UiSound(TEXT("ui_c_point"), .7f);
                        Burst(Gold, 12, 1.6f);
                    }, Id.ToString())
                ];
            }
            return SNew(SOverlay)
                + SOverlay::Slot()[SNew(SBorder).BorderImage(Av::Card()).Padding(FMargin(18.f, 14.f))[Body]]
                + SOverlay::Slot().VAlign(VAlign_Top).Padding(6.f, 6.f)[SNew(SBox).HeightOverride(5.f)[SNew(SImage).Image(White()).ColorAndOpacity(Alpha(Edge, .9f))]];
        }

        /** Uma habilidade da classe em cartão: a tecla, o ícone e, trancada, o nível em que libera. */
        TSharedRef<SWidget> ClassSkillCard(const FVortexClassSkill& S, int32 Slot, int32 Level)
        {
            const int32 Unlock = UVortexClassSkills::UnlockLevel(Slot);
            const bool bOpen = Level >= Unlock;
            return SNew(SBorder).BorderImage(Av::Card(!bOpen)).Padding(FMargin(12.f, 12.f))
                .ToolTipText(FText::FromString(S.Description))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth()[KeyCap(FString::FromInt(Slot + 1))]
                        + SHorizontalBox::Slot().FillWidth(1.f)
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [Title(bOpen ? FString::Printf(TEXT("%d MANA"), S.Mana) : FString::Printf(TEXT("NÍVEL %d"), Unlock), 15, bOpen ? ManaBlue : Gold)]
                    ]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 10.f)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()[IconTile(S.Icon, 92.f, bOpen ? UVortexClassSkills::ElementColor(S.Element) : Muted, 0, !bOpen)]
                        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                        [SNew(SBox).Visibility(bOpen ? EVisibility::Collapsed : EVisibility::HitTestInvisible)[Title(TEXT("TRANCADA"), 15, Gold)]]
                    ]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Title(S.Name, 17, bOpen ? Cream : Muted, true)]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 2.f, 0.f, 0.f)
                    [Label(bOpen ? FString::Printf(TEXT("%s  •  %.0f s"), *S.Category(), S.Cooldown) : FString::Printf(TEXT("Libera no nível %d"), Unlock), 13, Muted)]
                ];
        }

        TSharedRef<SWidget> BuildClasses(const UVortexHeroSheet& Sheet)
        {
            const FVortexRpgCatalog& Rpg = FVortexRpgCatalog::Get();
            const FName Race = Sheet.GetRace();
            TSharedRef<SVerticalBox> Main = SNew(SVerticalBox);
            if (const FVortexClassDef* Mine = Rpg.Class(Sheet.GetClassId()))
            {
                Main->AddSlot().AutoHeight()[ClassCard(*Mine, false, Race, true)];
                Main->AddSlot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)[Heading(TEXT("Habilidades"), TEXT("Teclas 1 a 4  •  a 1ª no Despertar, depois uma a cada 5 níveis"))];
                if (const TArray<FVortexClassSkill>* Skills = Rpg.SkillsOf(Mine->Id))
                {
                    TSharedRef<SUniformGridPanel> Row = SNew(SUniformGridPanel).SlotPadding(FMargin(6.f));
                    for (int32 i = 0; i < Skills->Num() && i < 4; ++i) Row->AddSlot(i, 0)[ClassSkillCard((*Skills)[i], i, Sheet.GetLevel())];
                    Main->AddSlot().AutoHeight()[Row];
                }
                Main->AddSlot().AutoHeight().Padding(0.f, 14.f, 0.f, 4.f)[Title(FString::Printf(TEXT("ASCENSÕES  •  NÍVEL %d"), Rpg.AscensionLevel), 17, Gold)];
                for (const TPair<FString, FString>& Ascension : Mine->Ascensions)
                {
                    Main->AddSlot().AutoHeight().Padding(0.f, 3.f)
                        [Label(FString::Printf(TEXT("%s — %s"), *Ascension.Key, *Ascension.Value), 15, Cream, TEXT("Regular"), 0, true)];
                }
            }
            else if (Sheet.GetClassCards().Num())
            {
                Main->AddSlot().AutoHeight()[Heading(TEXT("O Despertar"), TEXT("Seis cartas foram tiradas para você: escolha uma (é para sempre)"))];
                Main->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f, 0.f, 12.f)
                [
                    BlockButton(TEXT("Ver as seis cartas"), 1, true, 50.f, [this]()
                    {
                        AVortexCharacter* H = Hero.Get();
                        VortexHeroWindow::Close();
                        if (H) VortexAwakening::Open(H, true);
                    })
                ];
                TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(6.f));
                int32 i = 0;
                for (const FName& Card : Sheet.GetClassCards())
                {
                    if (const FVortexClassDef* C = Rpg.Class(Card)) { Grid->AddSlot(i % 3, i / 3)[ClassCard(*C, true, Race)]; ++i; }
                }
                Main->AddSlot().AutoHeight()[Grid];
            }
            else
            {
                const bool bReady = Sheet.CanAwaken();
                Main->AddSlot().AutoHeight()[Heading(TEXT("O Despertar"),
                    bReady ? TEXT("Você chegou ao nível do Despertar: tire as seis cartas") : FString::Printf(TEXT("Acontece no nível %d"), Rpg.AwakeningLevel))];
                Main->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 12.f)
                [
                    Label(TEXT("No Despertar a seiva do Vórtice mostra seis caminhos. A chance de cada raridade é a mesma para todos (comum 58%, rara 27%, épica 11%, lendária 4%), mas a sua raça pesa no sorteio: algumas classes combinam mais, outras nem aparecem. Pelo menos uma carta é rara ou melhor."),
                        16, Cream, TEXT("Regular"), 0, true)
                ];
                if (bReady)
                {
                    Main->AddSlot().AutoHeight().HAlign(HAlign_Center)
                    [
                        BlockButton(TEXT("Despertar: tirar as seis cartas"), 1, true, 54.f, [this]()
                        {
                            if (UVortexHeroSheet* Target = GetSheet()) Target->ServerDrawClasses();
                        })
                    ];
                }
                else
                {
                    const int64 Need = FVortexRpgCatalog::XpForLevel(Rpg.AwakeningLevel);
                    Main->AddSlot().AutoHeight()[Bar(TOptional<float>(FMath::Clamp(float(double(Sheet.GetXp()) / double(FMath::Max<int64>(1, Need))), 0.f, 1.f)), Gold, 18.f)];
                    Main->AddSlot().AutoHeight().Padding(0.f, 6.f)
                        [Label(FString::Printf(TEXT("%s / %s XP até o Despertar"), *Number(float(Sheet.GetXp())), *Number(float(Need))), 15, Muted)];
                }
            }
            // catálogo: todas as classes sorteáveis, por raridade, com a afinidade da raça
            TSharedRef<SVerticalBox> Catalog = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[Heading(TEXT("Classes do mundo"), TEXT("Passe o mouse para ver os bônus e as armas"))];
            for (const TCHAR* Rarity : { TEXT("legendary"), TEXT("epic"), TEXT("rare"), TEXT("common") })
            {
                Catalog->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 6.f)[Title(Upper(ClassRarityName(Rarity)), 16, ClassColor(Rarity))];
                TSharedRef<SWrapBox> Wrap = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6.f, 6.f));
                for (const FVortexClassDef& C : Rpg.Classes)
                {
                    if (!C.bDrawable || C.Rarity != Rarity) continue;
                    const float Affinity = C.AffinityFor(Race);
                    TArray<FString> Weapons;
                    for (const FName& W : C.Weapons) Weapons.Add(W.ToString().Replace(TEXT("_"), TEXT(" ")));
                    const FString Tip = FString::Printf(TEXT("%s\n%s\n%s\nArmas: %s"), *C.Name, *C.Role, *BuffLine(C), *FString::Join(Weapons, TEXT(", ")));
                    Wrap->AddSlot()
                    [
                        SNew(SBorder).BorderImage(Pill(Affinity <= 0.f ? FLinearColor::FromSRGBColor(FColor(0x55, 0x5E, 0x78)) : FLinearColor::FromSRGBColor(FColor(0x3A, 0x66, 0xC4))))
                        .Padding(FMargin(12.f, 5.f, 12.f, 7.f)).ToolTipText(FText::FromString(Tip))
                        [Label(C.Name, 14, Affinity <= 0.f ? Alpha(Cream, .55f) : Affinity > 1.05f ? Good : Cream, TEXT("Bold"))]
                    ];
                }
                Catalog->AddSlot().AutoHeight()[Wrap];
            }
            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 16.f, 0.f)[Box(SNew(SScrollBox) + SScrollBox::Slot()[Main])]
                + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(390.f)[Box(SNew(SScrollBox) + SScrollBox::Slot()[Catalog])]];
        }

        // ---- ALQUIMIA -----------------------------------------------------------------------
        TSharedRef<SWidget> BuildAlchemy(const UVortexHeroSheet& Sheet)
        {
            const FVortexRpgCatalog& Rpg = FVortexRpgCatalog::Get();
            TMap<FString, TSharedRef<SVerticalBox>> Columns;
            TArray<FString> Order;
            for (const FVortexRecipe& R : Rpg.Recipes)
            {
                if (!Columns.Contains(R.Station))
                {
                    Order.Add(R.Station);
                    Columns.Add(R.Station, SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[Heading(R.Station,
                        R.Station.Contains(TEXT("alquimia")) ? TEXT("Poções a partir do que as criaturas e a mata dão") : TEXT("Equipamento de couro, madeira e pedra"))]);
                }
                const FVortexItemDef* Output = Rpg.Item(R.Output);
                const bool bCan = Sheet.CanCraft(R.Id);
                TSharedRef<SHorizontalBox> Needs = SNew(SHorizontalBox);
                for (const TPair<FName, int32>& Ingredient : R.Ingredients)
                {
                    const FVortexItemDef* Def = Rpg.Item(Ingredient.Key);
                    const int32 Have = Sheet.CountOf(Ingredient.Key);
                    Needs->AddSlot().AutoWidth().Padding(0.f, 0.f, 10.f, 0.f)
                    [
                        SNew(SHorizontalBox).ToolTipText(FText::FromString(Def ? Def->Name : Ingredient.Key.ToString()))
                        + SHorizontalBox::Slot().AutoWidth()[IconTile(Def ? Def->Icon : NAME_None, 46.f, Have >= Ingredient.Value ? Good : Bad)]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 0.f, 0.f)
                        [Title(FString::Printf(TEXT("%d/%d"), Have, Ingredient.Value), 15, Have >= Ingredient.Value ? Good : Bad)]
                    ];
                }
                const FName Id = R.Id;
                Columns[R.Station]->AddSlot().AutoHeight().Padding(0.f, 5.f)
                [
                    SNew(SBorder).BorderImage(Av::Card(!bCan)).Padding(FMargin(14.f, 10.f))
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
                        [IconTile(Output ? Output->Icon : NAME_None, 70.f, Output ? FVortexRpgCatalog::RarityColor(Output->Rarity) : Muted, R.Count)]
                        + SHorizontalBox::Slot().FillWidth(1.f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[Title(R.Name, 19, Cream, true)]
                            + SVerticalBox::Slot().AutoHeight()[Label(R.Description, 14, Muted, TEXT("Regular"), 0, true)]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[Needs]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f, 0.f, 0.f)
                        [
                            BlockButton(TEXT("Preparar"), bCan ? 1 : 0, bCan, 46.f, [this, Id]()
                            {
                                if (UVortexHeroSheet* Target = GetSheet()) Target->ServerCraft(Id);
                            }, Id.ToString())
                        ]
                    ]
                ];
            }
            TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
            for (int32 i = 0; i < Order.Num(); ++i)
            {
                Row->AddSlot().FillWidth(1.f).Padding(i ? 16.f : 0.f, 0.f, 0.f, 0.f)[Box(SNew(SScrollBox) + SScrollBox::Slot()[Columns[Order[i]]])];
            }
            return Row;
        }

        // ---- MISSÕES ------------------------------------------------------------------------
        TSharedRef<SWidget> QuestRow(const FVortexQuestDef& Q, const FString& State, const FLinearColor& Color)
        {
            const FName Id = Q.Id;
            const bool bSelected = SelectedQuest == Id;
            // escolhida: amarela (as letras ficam brancas com contorno)
            return SNew(SJuicyButton).ButtonStyle(&ButtonStyle(bSelected ? 4 : 0)).ContentPadding(FMargin(12.f, 9.f, 12.f, 7.f)).VAlign(VAlign_Center)
                .Id(FName(TEXT("q_") + Id.ToString())).Sound(NAME_None).Stars(false)
                .OnClicked_Lambda([this, Id]()
            {
                SelectedQuest = Id;
                UiSound(TEXT("ui_c_select"), .5f);
                Rebuild();
                return FReply::Handled();
            })
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
                [Title(Q.bMain ? TEXT("!") : TEXT("?"), 26, bSelected ? Cream : Q.bMain ? Gold : FLinearColor(.5f, .72f, 1.f))]
                + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()[Title(Q.Title, 16, Cream)]
                    + SVerticalBox::Slot().AutoHeight()[Label(State, 13, bSelected ? Cream : Color, TEXT("Bold"))]
                ]
            ];
        }

        TSharedRef<SWidget> BuildQuests(const UVortexHeroSheet& Sheet)
        {
            const FVortexStoryCatalog& Story = FVortexStoryCatalog::Get();
            const UVortexQuestLog* Log = QuestLog();
            TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
            // capítulo atual da jornada
            int32 Chapter = 1;
            for (const FVortexQuestDef& Q : Story.Quests) if (Q.bMain && Log && Log->IsDone(Q.Id) && !Q.IsLocked()) Chapter = FMath::Max(Chapter, Q.Chapter);
            if (const FVortexChapter* C = Story.Chapters.FindByPredicate([Chapter](const FVortexChapter& X) { return X.Id == Chapter; }))
            {
                List->AddSlot().AutoHeight()[Heading(FString::Printf(TEXT("Capítulo %d"), C->Id), C->Title + TEXT("  •  ") + C->Region, InkMuted())];
                List->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[Label(C->Text, 14, InkText(), TEXT("Regular"), 0, true)];
            }
            auto Section = [&List](const FString& Name) { List->AddSlot().AutoHeight().Padding(0.f, 12.f, 0.f, 6.f)[Title(Upper(Name), 17, InkTitle())]; };
            int32 Count = 0;
            Section(TEXT("Em andamento"));
            for (const FVortexQuestDef& Q : Story.Quests)
            {
                if (!Log || !Log->IsActive(Q.Id)) continue;
                const bool bReady = Log->IsReady(Q);
                List->AddSlot().AutoHeight().Padding(0.f, 2.f)[QuestRow(Q, bReady ? FString::Printf(TEXT("Pronta: fale com %s"), *Story.NpcCallName(Q.Giver)) : TEXT("Em andamento"), bReady ? Good : Muted)];
                if (SelectedQuest.IsNone()) SelectedQuest = Q.Id;
                ++Count;
            }
            if (!Count) List->AddSlot().AutoHeight()[Label(TEXT("Nenhuma. Os moradores com ! sobre a cabeça têm trabalho para você."), 14, InkMuted(), TEXT("Regular"), 0, true)];
            Section(TEXT("Disponíveis"));
            Count = 0;
            for (const FVortexQuestDef& Q : Story.Quests)
            {
                if (!Log || !Log->IsAvailable(Q) || Q.bAuto) continue;
                List->AddSlot().AutoHeight().Padding(0.f, 2.f)[QuestRow(Q, FString::Printf(TEXT("Fale com %s"), *Story.NpcCallName(Q.Giver)), HudQuest())];
                ++Count;
            }
            if (!Count) List->AddSlot().AutoHeight()[Label(TEXT("Nenhuma agora: termine as que estão em andamento."), 14, InkMuted())];
            if (SelectedQuest.IsNone() && Log)
            {
                for (const FVortexQuestDef& Q : Story.Quests) if (Log->IsAvailable(Q) && !Q.bAuto) { SelectedQuest = Q.Id; break; }
            }
            Section(TEXT("Concluídas"));
            Count = 0;
            for (const FVortexQuestDef& Q : Story.Quests)
            {
                if (!Log || !Log->IsDone(Q.Id)) continue;
                List->AddSlot().AutoHeight().Padding(0.f, 2.f)[QuestRow(Q, TEXT("Concluída"), Alpha(Good, .7f))];
                ++Count;
            }
            if (!Count) List->AddSlot().AutoHeight()[Label(TEXT("Ainda nenhuma."), 14, InkMuted())];
            Section(TEXT("Além do vale"));
            for (const FVortexQuestDef& Q : Story.Quests)
            {
                if (Q.IsLocked()) List->AddSlot().AutoHeight().Padding(0.f, 2.f)[QuestRow(Q, FString::Printf(TEXT("Capítulo %d  •  %s"), Q.Chapter, *Q.Region), Alpha(Muted, .6f))];
            }

            // detalhes da missão escolhida
            TSharedRef<SVerticalBox> Detail = SNew(SVerticalBox);
            if (const FVortexQuestDef* Q = Story.Quest(SelectedQuest))
            {
                const bool bActive = Log && Log->IsActive(Q->Id);
                const bool bDone = Log && Log->IsDone(Q->Id);
                Detail->AddSlot().AutoHeight()[Heading(Q->Title, FString::Printf(TEXT("%s  •  dada por %s"), Q->bMain ? TEXT("Jornada") : TEXT("Missão"),
                    *Story.NpcCallName(Q->Giver)), InkMuted())];
                Detail->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
                    [Label(bDone ? Q->Done : bActive ? Q->Progress : Q->Offer, 17, InkText(), TEXT("Regular"), 0, true)];
                if (Log)
                {
                    Detail->AddSlot().AutoHeight()[Title(TEXT("OBJETIVOS"), 18, InkTitle())];
                    Detail->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 14.f)
                    [
                        SNew(SBorder).BorderImage(Av::Card()).Padding(FMargin(14.f, 10.f))[Label(Log->ObjectiveLines(*Q), 16, Cream, TEXT("Regular"), 0, true)]
                    ];
                }
                // recompensas em blocos (XP, moedas e os itens com ícone)
                TSharedRef<SWrapBox> Rewards = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8.f, 8.f));
                if (Q->Xp > 0) Rewards->AddSlot()[StatTile(TEXT("XP"), Number(float(Q->Xp)), Gold)];
                if (Q->Coins > 0) Rewards->AddSlot()[StatTile(TEXT("Moedas"), FString::FromInt(Q->Coins), Gold)];
                for (const TPair<FName, int32>& Item : Q->Items)
                {
                    const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Item.Key);
                    Rewards->AddSlot()
                    [
                        SNew(SBox).ToolTipText(FText::FromString(Def ? Def->Name : Item.Key.ToString()))
                        [IconTile(Def ? Def->Icon : NAME_None, 70.f, Def ? FVortexRpgCatalog::RarityColor(Def->Rarity) : Muted, Item.Value)]
                    ];
                }
                Detail->AddSlot().AutoHeight()[Title(TEXT("RECOMPENSA"), 18, InkTitle())];
                Detail->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 16.f)[Rewards];
                if (bActive)
                {
                    const FName Id = Q->Id;
                    Detail->AddSlot().AutoHeight().HAlign(HAlign_Left)
                    [
                        BlockButton(TEXT("Acompanhar na tela"), 1, true, 50.f, [this, Id]()
                        {
                            if (UVortexQuestLog* Target = QuestLog()) Target->ServerTrack(Id);
                            UiSound(TEXT("ui_c_select"), .6f);
                        })
                    ];
                }
            }
            else Detail->AddSlot().AutoHeight()[Label(TEXT("Escolha uma missão à esquerda."), 16, InkMuted())];
            return SNew(SHorizontalBox)
                // 07/11 (a referência do usuário): o diário aberto, as duas páginas de pergaminho com a letra escura
                + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(480.f)[SNew(SBorder).BorderImage(Av::Parchment()).Padding(FMargin(22.f, 20.f))[SNew(SScrollBox) + SScrollBox::Slot()[List]]]]
                + SHorizontalBox::Slot().AutoWidth().Padding(2.f, 10.f)[SNew(SBox).WidthOverride(10.f)[SNew(SImage).Image(Round(3.f)).ColorAndOpacity(FLinearColor::FromSRGBColor(FColor(0x5A, 0x3A, 0x22)))]]
                + SHorizontalBox::Slot().FillWidth(1.f)[SNew(SBorder).BorderImage(Av::Parchment()).Padding(FMargin(28.f, 22.f))[SNew(SScrollBox) + SScrollBox::Slot()[Detail]]];
        }

        static FLinearColor HudQuest() { return FLinearColor::FromSRGBColor(FColor(0xFF, 0xD8, 0x6B)); }
    };

    TSharedPtr<SVortexHeroWindow> Window;
    TWeakObjectPtr<APlayerController> WindowOwner;
}

namespace VortexHeroWindowImpl
{
    bool IsOpen() { return VortexHeroUi::Window.IsValid(); }

    void Open(AVortexCharacter* Hero, int32 Page)
    {
        if (IsOpen() || !Hero || !Hero->GetSheet() || !GEngine || !GEngine->GameViewport) return;
        APlayerController* Controller = Cast<APlayerController>(Hero->GetController());
        if (!Controller) return;
        VortexHeroUi::Window = SNew(VortexHeroUi::SVortexHeroWindow).Hero(Hero).Page(Page);
        VortexHeroUi::WindowOwner = Controller;
        GEngine->GameViewport->AddViewportWidgetContent(VortexHeroUi::Window.ToSharedRef(), 40);
        Controller->SetShowMouseCursor(true);
        FInputModeGameAndUI Mode;
        Mode.SetWidgetToFocus(VortexHeroUi::Window);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        Controller->SetInputMode(Mode);
        VortexHeroUi::UiSound(TEXT("ui_c_open"), .6f);
    }

    void SelectForTest(int32 BagIndex, int32 EquipSlot)
    {
        if (IsOpen()) VortexHeroUi::Window->SelectForTest(BagIndex, EquipSlot);
    }

    void SpendForTest(int32 Attribute)
    {
        if (IsOpen()) VortexHeroUi::Window->SpendForTest(Attribute);
    }

    int32 RebuildsForTest() { return IsOpen() ? VortexHeroUi::Window->GetRebuilds() : -1; }

    void Close()
    {
        if (!IsOpen()) return;
        if (GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(VortexHeroUi::Window.ToSharedRef());
        VortexHeroUi::Window.Reset();
        if (APlayerController* Controller = VortexHeroUi::WindowOwner.Get())
        {
            Controller->SetShowMouseCursor(false);
            Controller->SetInputMode(FInputModeGameOnly());
        }
        VortexHeroUi::WindowOwner.Reset();
        // fechou pelo botão/tecla: o som já tocou quando começou a encolher
        if (VortexHeroUi::Now() - VortexAudio::LastUiSoundAt() > .3) VortexHeroUi::UiSound(TEXT("ui_c_close"), .55f);
    }

    void Toggle(AVortexCharacter* Hero)
    {
        if (IsOpen()) Close();
        else Open(Hero);
    }
}

// ---------------------------------------------------------------------------------------------
// Loja (VortexShopWindow): mesmo estilo da janela do herói.
namespace VortexHeroUi
{
    class SVortexShopWindow final : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SVortexShopWindow) {}
            SLATE_ARGUMENT(TWeakObjectPtr<AVortexCharacter>, Hero)
            SLATE_ARGUMENT(FName, Vendor)
        SLATE_END_ARGS()

        void Construct(const FArguments& Args)
        {
            Hero = Args._Hero;
            Vendor = Args._Vendor;
            Info = VortexShop::Info(Vendor);
            OpenedAt = Now();
            TSharedPtr<SCartoonFx> Fx;
            ChildSlot
            [
                SNew(SOverlay)
                + SOverlay::Slot()[SNew(SImage).Image(White()).ColorAndOpacity(FLinearColor(0.f, 0.f, .05f, .5f))]
                + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                [
                    // abre pulando, como a janela do herói
                    SNew(SBox).WidthOverride(1320.f).HeightOverride(820.f)
                    .RenderTransformPivot(FVector2D(.5f, .5f))
                    .RenderTransform_Lambda([this]()
                    {
                        const float T = float(Now() - OpenedAt);
                        return FSlateRenderTransform(FScale2f(.72f + .28f * (1.f - FMath::Exp(-8.f * T) * FMath::Cos(T * 15.f))));
                    })
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()[SNew(SBorder).BorderImage(Av::Frame())]
                        + SOverlay::Slot().Padding(FMargin(36.f, 28.f, 36.f, 22.f))[SAssignNew(BodyBox, SBox)]
                    ]
                ]
                + SOverlay::Slot()[SAssignNew(Fx, SCartoonFx)]
            ];
            FxLayer = Fx;
            Rebuild();
        }

        virtual bool SupportsKeyboardFocus() const override { return true; }

        virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& Event) override
        {
            const FKey Key = Event.GetKey();
            if (Key == EKeys::Escape) return FReply::Unhandled(); // Esc é centralizado no PlayerController.
            if (Key == EKeys::E || Key == EKeys::I || Key == EKeys::Tab) { VortexShopWindow::Close(); return FReply::Handled(); }
            return FReply::Unhandled();
        }

        virtual void Tick(const FGeometry& Geometry, const double CurrentTime, const float DeltaTime) override
        {
            SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
            const UVortexHeroSheet* Sheet = GetSheet();
            if (!Sheet) { VortexShopWindow::Close(); return; }
            // a ficha mudou (comprou, vendeu) ou virou o dia: redesenha
            if (Sheet->GetRevision() != ShownRevision || Sheet->ShopWorldDay() != ShownDay) Rebuild();
            else if (!Face && Now() >= FaceTryAt && SellerFace()) Rebuild();
        }

    private:
        TWeakObjectPtr<AVortexCharacter> Hero;
        FName Vendor;
        VortexShop::FVendorInfo Info;
        TSharedPtr<SBox> BodyBox;
        int32 ShownRevision = -1, ShownDay = -1;
        double OpenedAt = 0.0;

        UVortexHeroSheet* GetSheet() const { return Hero.IsValid() ? Hero->GetSheet() : nullptr; }

        TSharedRef<SWidget> PriceTag(int32 Price, const FLinearColor& Color)
        {
            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)[SNew(SImage).Image(IconBrush(TEXT("ui_coin"), 24.f))]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Title(FString::FromInt(Price), 20, Color)];
        }

        TSharedRef<SWidget> ShopButton(const FString& Text, bool bEnabled, int32 Kind, TFunction<void()> OnClick, FName Id = NAME_None)
        {
            // o Id segura a animação e a cor "com o mouse" quando a loja é refeita (comprou, vendeu)
            return SNew(SJuicyButton).ButtonStyle(&ButtonStyle(Kind)).IsEnabled(bEnabled).HAlign(HAlign_Center).VAlign(VAlign_Center).Id(Id)
                .ContentPadding(FMargin(14.f, 4.f, 14.f, 4.f)).StarColor(Kind == 1 ? Gold : Cream)
                .OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
                [Title(Text, 16, bEnabled ? Cream : Alpha(Cream, .6f))];
        }

        // 07/11 (o usuário, com uma imagem de referência: a loja com o retrato do vendedor, a fala num pergaminho, as
        // grades "à venda" e "seus itens" com o preço embaixo de cada um, e os botões grandes Comprar e Vender): clicar
        // num item escolhe; os botões de baixo compram ou vendem o escolhido
        int32 SelOffer = INDEX_NONE;
        FName SelSell;
        UTexture2D* Face = nullptr;
        double FaceTryAt = 0.0;

        UTexture2D* SellerFace()
        {
            if (Face || Now() < FaceTryAt || !Hero.IsValid()) return Face;
            FaceTryAt = Now() + .5;
            for (TActorIterator<AVortexNpc> It(Hero->GetWorld()); It; ++It)
                if (It->GetNpcId() == Vendor) { Face = VortexPortrait::CaptureFace(*It); break; }
            return Face;
        }

        TSharedRef<SWidget> ItemCard(FName Icon, const FLinearColor& Color, int32 Count, bool bDim, bool bSelected, const FString& Tip,
            TSharedRef<SWidget> Under, FName Id, TFunction<void()> OnClick)
        {
            return SNew(SJuicyButton).ButtonStyle(&ButtonStyle(3)).ContentPadding(0.f).Id(Id).Stars(!bDim).StarColor(Color)
                .ToolTipText(FText::FromString(Tip))
                .OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()[IconTile(Icon, 82.f, Color, Count, bDim)]
                        + SOverlay::Slot()[SNew(SImage).Image(Av::Slot(2)).Visibility(bSelected ? EVisibility::HitTestInvisible : EVisibility::Collapsed)]
                    ]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 3.f, 0.f, 2.f)[Under]
                ];
        }

        TSharedRef<SWidget> SmallPrice(int32 Price, const FLinearColor& Color)
        {
            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[SNew(SImage).Image(IconBrush(TEXT("ui_coin"), 18.f))]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Title(FString::FromInt(Price), 16, Color)];
        }

        void Rebuild()
        {
            UVortexHeroSheet* Sheet = GetSheet();
            if (!Sheet || !BodyBox) return;
            ShownRevision = Sheet->GetRevision();
            ShownDay = Sheet->ShopWorldDay();
            const FVortexRpgCatalog& Catalog = FVortexRpgCatalog::Get();
            const TArray<VortexShop::FOffer> Stock = VortexShop::Stock(Vendor, Sheet->ShopWorldId(), ShownDay, Sheet->GetLevel());
            TWeakObjectPtr<AVortexCharacter> WeakHero = Hero;
            const FName V = Vendor;

            // ---- 07/11: a forja do Bram: subir o nível do que você veste (até o seu), com moedas e o material da raridade
            TSharedRef<SVerticalBox> Forge = SNew(SVerticalBox);
            if (V == TEXT("bram"))
            {
                Forge->AddSlot().AutoHeight().Padding(0.f, 10.f, 0.f, 4.f)[Heading(TEXT("Forja: melhorar o que você veste"))];
                for (const int32 Slot : { int32(VortexRpg::Weapon), int32(VortexRpg::Head), int32(VortexRpg::Chest), int32(VortexRpg::Legs), int32(VortexRpg::Boots) })
                {
                    const FVortexItemDef* Worn = Catalog.Item(Sheet->GetEquipped(Slot));
                    if (!Worn) continue;
                    int32 Price = 0, Count = 0, Next = 0;
                    FName Material;
                    const bool bCan = VortexShop::ForgeCost(*Worn, Sheet->GetLevel(), Price, Material, Count, Next);
                    const FVortexItemDef* Mat = Catalog.Item(Material);
                    const bool bAfford = bCan && Sheet->GetCoins() >= Price && Sheet->CountOf(Material) >= Count;
                    const FString Text = bCan ? FString::Printf(TEXT("Nv. %d: %d moedas + %d %s"), Next, Price, Count, Mat ? *Mat->Name : *Material.ToString())
                        : FString(TEXT("no seu nível"));
                    Forge->AddSlot().AutoHeight().Padding(0.f, 3.f)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                        [Label(FString::Printf(TEXT("%s  (nv. %d)"), *Worn->Name, Worn->Level), 14, FVortexRpgCatalog::RarityColor(Worn->Rarity))]
                        + SHorizontalBox::Slot().AutoWidth()
                        [ShopButton(Text, bAfford, 1, [WeakHero, Slot]()
                        {
                            if (UVortexHeroSheet* Sh = WeakHero.IsValid() ? WeakHero->GetSheet() : nullptr) Sh->ServerForge(uint8(Slot));
                            UiSound(TEXT("ui_c_equip"), .7f);
                            Burst(Gold, 10, 1.2f);
                        }, FName(*FString::Printf(TEXT("forge_%d"), Slot)))]
                    ];
                }
            }

            // ---- à venda: a grade de 5 colunas, o preço embaixo (vermelho se não dá para pagar)
            TSharedRef<SUniformGridPanel> Offers = SNew(SUniformGridPanel).SlotPadding(FMargin(5.f));
            for (int32 i = 0; i < Stock.Num(); ++i)
            {
                const VortexShop::FOffer& O = Stock[i];
                const FVortexItemDef* Def = Catalog.Item(O.Item);
                if (!Def) continue;
                const int32 Left = Sheet->ShopLeft(Vendor, i, O.Qty, O.bUnlimited);
                const FLinearColor Color = FVortexRpgCatalog::RarityColor(Def->Rarity);
                FString Tip = FString::Printf(TEXT("%s\n%s  •  %s"), *Def->Name, *FVortexRpgCatalog::RarityLabel(Def->Rarity), *FVortexRpgCatalog::TypeLabel(Def->Type));
                if (!O.bUnlimited) Tip += Left > 0 ? FString::Printf(TEXT("  •  restam %d"), Left) : FString(TEXT("  •  esgotado"));
                Tip += TEXT("\n") + Def->Description;
                const int32 Index = i;
                TSharedRef<SWidget> Under = Left > 0 ? SmallPrice(O.Price, Sheet->GetCoins() >= O.Price ? Gold : Bad) : StaticCastSharedRef<SWidget>(Title(TEXT("ESGOTADO"), 13, Muted));
                Offers->AddSlot(i % 5, i / 5)
                [
                    ItemCard(Def->Icon, Color, 0, Left <= 0, SelOffer == i, Tip, Under, FName(*FString::Printf(TEXT("shop_offer_%d"), i)),
                        [this, Index]() { SelOffer = Index; SelSell = NAME_None; UiSound(TEXT("ui_c_select"), .5f); Rebuild(); })
                ];
            }

            // ---- seus itens: o que ele compra da mochila (somando as pilhas), grade de 4 colunas
            TMap<FName, int32> Counts;
            for (const FVortexItemStack& Stack : Sheet->GetBag()) if (!Stack.IsEmpty()) Counts.FindOrAdd(Stack.Item) += Stack.Count;
            TArray<TPair<const FVortexItemDef*, int32>> Sellable;
            for (const TPair<FName, int32>& C : Counts)
            {
                const FVortexItemDef* Def = Catalog.Item(C.Key);
                if (Def && VortexShop::Pays(Vendor, *Def) > 0) Sellable.Add({ Def, C.Value });
            }
            Sellable.Sort([](const TPair<const FVortexItemDef*, int32>& A, const TPair<const FVortexItemDef*, int32>& B)
            {
                return A.Key->Type != B.Key->Type ? A.Key->Type < B.Key->Type : A.Key->Name < B.Key->Name;
            });
            if (!SelSell.IsNone() && !Counts.Contains(SelSell)) SelSell = NAME_None;
            TSharedRef<SUniformGridPanel> Sells = SNew(SUniformGridPanel).SlotPadding(FMargin(5.f));
            int32 SellCount = 0, SellEach = 0;
            for (int32 i = 0; i < Sellable.Num(); ++i)
            {
                const FVortexItemDef* Def = Sellable[i].Key;
                const int32 Each = VortexShop::Pays(Vendor, *Def);
                const FName Item = Def->Id;
                if (Item == SelSell) { SellCount = Sellable[i].Value; SellEach = Each; }
                Sells->AddSlot(i % 4, i / 4)
                [
                    ItemCard(Def->Icon, FVortexRpgCatalog::RarityColor(Def->Rarity), Sellable[i].Value, false, SelSell == Item,
                        FString::Printf(TEXT("%s\n%d moedas cada\n%s"), *Def->Name, Each, *Def->Description), SmallPrice(Each, Gold),
                        FName(TEXT("shop_sell_") + Item.ToString()),
                        [this, Item]() { SelSell = Item; SelOffer = INDEX_NONE; UiSound(TEXT("ui_c_select"), .5f); Rebuild(); })
                ];
            }
            // as casas vazias completam a grade (como na referência)
            for (int32 i = Sellable.Num(); i < FMath::Max(12, (Sellable.Num() + 3) / 4 * 4); ++i)
                Sells->AddSlot(i % 4, i / 4)[SNew(SBox).WidthOverride(82.f).HeightOverride(82.f)[SNew(SImage).Image(Av::Slot(0)).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, .6f))]];

            // ---- o escolhido: o que aparece em cima dos botões
            const VortexShop::FOffer* Offer = Stock.IsValidIndex(SelOffer) ? &Stock[SelOffer] : nullptr;
            const FVortexItemDef* OfferDef = Offer ? Catalog.Item(Offer->Item) : nullptr;
            const int32 OfferLeft = Offer ? Sheet->ShopLeft(Vendor, SelOffer, Offer->Qty, Offer->bUnlimited) : 0;
            const bool bCanBuy = OfferDef && OfferLeft > 0 && Sheet->GetCoins() >= Offer->Price;
            const FVortexItemDef* SellDef = SelSell.IsNone() ? nullptr : Catalog.Item(SelSell);
            FString Chosen = TEXT("Escolha um item para comprar ou vender");
            FLinearColor ChosenColor = Muted;
            if (OfferDef) { Chosen = FString::Printf(TEXT("%s  —  %d moedas"), *OfferDef->Name, Offer->Price); ChosenColor = FVortexRpgCatalog::RarityColor(OfferDef->Rarity); }
            else if (SellDef) { Chosen = FString::Printf(TEXT("%s  —  vende por %d cada (você tem %d)"), *SellDef->Name, SellEach, SellCount); ChosenColor = FVortexRpgCatalog::RarityColor(SellDef->Rarity); }
            const int32 BuyIndex = SelOffer;
            const FName SellItem = SelSell;
            auto Sell = [WeakHero, V, SellItem](int32 N)
            {
                if (UVortexHeroSheet* Sh = WeakHero.IsValid() ? WeakHero->GetSheet() : nullptr) Sh->ServerShopSell(V, SellItem, N);
                UiSound(TEXT("ui_c_coin"), .65f);
                Burst(Gold, 8, 1.1f);
            };

            // ---- o vendedor: o retrato, o nome na fita, a fala no pergaminho
            UTexture2D* Portrait = SellerFace();
            TSharedRef<SWidget> PortraitBox = SNew(SBox).WidthOverride(230.f).HeightOverride(230.f)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()[SNew(SBorder).BorderImage(Av::Card())]
                    + SOverlay::Slot().Padding(12.f)
                    [
                        Portrait ? StaticCastSharedRef<SWidget>(SNew(SImage).Image(TextureBrush(Portrait, 206.f)))
                            : StaticCastSharedRef<SWidget>(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[Title(Info.Seller.Left(1), 90, Gold)])
                    ]
                ];
            const FString Restock = Info.bRestocksDaily ? FString::Printf(TEXT("Estoque novo amanhã (dia %d)"), ShownDay + 1) : FString(TEXT("Suprimentos todos os dias"));
            TSharedRef<SWidget> SellerCol = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[PortraitBox]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, -14.f, 0.f, 0.f)
                [
                    SNew(SBorder).BorderImage(Av::Ribbon()).Padding(FMargin(30.f, 5.f, 30.f, 9.f)).HAlign(HAlign_Center)
                    [Title(Info.Seller, 20, Cream)]
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 10.f)[Label(Restock, 13, Muted)]
                + SVerticalBox::Slot().FillHeight(1.f)
                [
                    SNew(SBorder).BorderImage(Av::Parchment()).Padding(FMargin(20.f, 18.f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()[Label(Info.Greeting, 16, InkText(), TEXT("Regular"), 0, true)]
                        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)[Label(Info.SellHelp, 13, InkMuted(), TEXT("Regular"), 0, true)]
                    ]
                ];

            BodyBox->SetContent(
                SNew(SVerticalBox)
                // o título no meio (a fita vermelha) e as moedas e o X à direita
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot().HAlign(HAlign_Center)
                    [
                        SNew(SBorder).BorderImage(Av::Ribbon()).Padding(FMargin(60.f, 6.f, 60.f, 10.f))[Title(Upper(Info.Title), 24, Cream)]
                    ]
                    + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center)[ShopButton(TEXT("X"), true, 5, []() { VortexShopWindow::Close(); }, TEXT("shop_x"))]
                ]
                + SVerticalBox::Slot().FillHeight(1.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 14.f, 0.f)[SNew(SBox).WidthOverride(270.f)[SellerCol]]
                    + SHorizontalBox::Slot().FillWidth(1.3f).Padding(0.f, 0.f, 12.f, 0.f)
                    [
                        Box(SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[Heading(TEXT("Itens à venda"))]
                            + SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot().HAlign(HAlign_Center)[Offers]]
                            + SVerticalBox::Slot().AutoHeight()[Forge], FMargin(14.f))
                    ]
                    + SHorizontalBox::Slot().FillWidth(1.f)
                    [
                        Box(SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[Heading(TEXT("Seus itens (vender)"), Info.BuyLine)]
                            + SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot().HAlign(HAlign_Center)[Sells]], FMargin(14.f))
                    ]
                ]
                // embaixo: suas moedas, o escolhido e os botões grandes
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [
                        SNew(SBorder).BorderImage(Av::Panel()).Padding(FMargin(16.f, 8.f))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)[Label(TEXT("Suas moedas:"), 15, Muted)]
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[PriceTag(Sheet->GetCoins(), Gold)]
                        ]
                    ]
                    + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).HAlign(HAlign_Center).Padding(12.f, 0.f)[Title(Chosen, 17, ChosenColor, true)]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f)
                    [
                        SNew(SBox).MinDesiredWidth(200.f).HeightOverride(58.f)
                        [
                            ShopButton(TEXT("COMPRAR"), bCanBuy, 6, [WeakHero, V, BuyIndex]()
                            {
                                if (UVortexHeroSheet* S = WeakHero.IsValid() ? WeakHero->GetSheet() : nullptr) S->ServerShopBuy(V, BuyIndex);
                                UiSound(TEXT("ui_c_buy"), .7f);
                                Burst(Gold, 10, 1.3f);
                            }, TEXT("shop_buy"))
                        ]
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f)
                    [
                        SNew(SBox).MinDesiredWidth(200.f).HeightOverride(58.f)[ShopButton(TEXT("VENDER"), SellDef != nullptr, 5, [Sell]() { Sell(1); }, TEXT("shop_sell"))]
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f, 0.f, 0.f)
                    [
                        SNew(SBox).HeightOverride(58.f)
                        [ShopButton(SellCount > 1 ? FString::Printf(TEXT("TUDO (%d)"), SellCount * SellEach) : FString(TEXT("TUDO")), SellDef != nullptr && SellCount > 1, 0,
                            [Sell, SellCount]() { Sell(SellCount); }, TEXT("shop_sellall"))]
                    ]
                ]);
        }
    };

    TSharedPtr<SVortexShopWindow> ShopWindow;
    TWeakObjectPtr<APlayerController> ShopOwner;
}

namespace VortexShopWindowImpl
{
    bool IsOpen() { return VortexHeroUi::ShopWindow.IsValid(); }

    void Open(AVortexCharacter* Hero, FName Vendor)
    {
        if (IsOpen() || !Hero || !Hero->GetSheet() || !VortexShop::IsVendor(Vendor) || !GEngine || !GEngine->GameViewport) return;
        APlayerController* Controller = Cast<APlayerController>(Hero->GetController());
        if (!Controller) return;
        if (VortexHeroWindow::IsOpen()) VortexHeroWindow::Close();
        VortexHeroUi::ShopWindow = SNew(VortexHeroUi::SVortexShopWindow).Hero(Hero).Vendor(Vendor);
        VortexHeroUi::ShopOwner = Controller;
        GEngine->GameViewport->AddViewportWidgetContent(VortexHeroUi::ShopWindow.ToSharedRef(), 40);
        Controller->SetShowMouseCursor(true);
        FInputModeGameAndUI Mode;
        Mode.SetWidgetToFocus(VortexHeroUi::ShopWindow);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        Controller->SetInputMode(Mode);
        VortexHeroUi::UiSound(TEXT("ui_c_open"), .6f);
    }

    void Close()
    {
        if (!IsOpen()) return;
        if (GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(VortexHeroUi::ShopWindow.ToSharedRef());
        VortexHeroUi::ShopWindow.Reset();
        if (APlayerController* Controller = VortexHeroUi::ShopOwner.Get())
        {
            Controller->SetShowMouseCursor(false);
            Controller->SetInputMode(FInputModeGameOnly());
        }
        VortexHeroUi::ShopOwner.Reset();
        // fechou pelo botão/tecla: o som já tocou quando começou a encolher
        if (VortexHeroUi::Now() - VortexAudio::LastUiSoundAt() > .3) VortexHeroUi::UiSound(TEXT("ui_c_close"), .55f);
    }
}
