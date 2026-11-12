#include "Framework/VortexFrontEnd.h"
#include "Misc/ConfigCacheIni.h"
#include "Framework/VortexFaceStyles.h"
#include "Framework/VortexPieces.h"
#include "Framework/VortexUiKit.h"
#include "Framework/VortexUiJuice.h"
#include "Online/VortexOnline.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/CompositeFont.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/VortexGameInstance.h"
#include "Framework/VortexMenuStage.h"
#include "Framework/VortexPortrait.h"
#include "Framework/VortexPlayerSettings.h"
#include "Framework/VortexGraphics.h"
#include "Framework/VortexOptionsPanel.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Framework/VortexPlayerState.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Persistence/VortexPlayerSave.h"
#include "Persistence/VortexSaveSubsystem.h"
#include "Rpg/VortexQuestLog.h"
#include "Rpg/VortexRpg.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "String/LexFromString.h"
#include "Styling/CoreStyle.h"
#include "UnrealClient.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Algo/AllOf.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

// ---------------------------------------------------------------------------------------------
// estilo
// ---------------------------------------------------------------------------------------------
namespace VortexUi
{
    const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor(0xF0, 0xCF, 0x7A));
    const FLinearColor GoldDeep = FLinearColor::FromSRGBColor(FColor(0xC8, 0x92, 0x3A));
    const FLinearColor Cream = FLinearColor::FromSRGBColor(FColor(0xF4, 0xEA, 0xD2));
    const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(0xA9, 0xB4, 0xC2));
    const FLinearColor Good = FLinearColor::FromSRGBColor(FColor(0x8F, 0xE3, 0x8A));
    const FLinearColor Bad = FLinearColor::FromSRGBColor(FColor(0xF0, 0x86, 0x76));
    // a esmeralda do brasão: a luz da Gênese (destaques, brilho dos ícones, foco)
    const FLinearColor Emerald = FLinearColor::FromSRGBColor(FColor(0x3C, 0xE6, 0x8E));
    const FLinearColor EmeraldDeep = FLinearColor::FromSRGBColor(FColor(0x0F, 0x6B, 0x45));
    const FLinearColor Night = FLinearColor::FromSRGBColor(FColor(0x07, 0x0C, 0x16));

    // por enquanto só o humano pode ser escolhido (01/11, pedido seu); as outras raças ficam com cadeado
    bool IsRaceOpen(FName Race)
    {
        return Race == TEXT("humano") || Race == TEXT("elfo");
    }

    FSlateFontInfo Font(int32 Size, const TCHAR* Weight = TEXT("Regular"), int32 Spacing = 0)
    {
        FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(Weight, Size);
        Info.LetterSpacing = Spacing;
        return Info;
    }

    /**
     * A letra dos títulos e botões: Cinzel (Content/Vortex/UI/Fonts, licença OFL), lida direto do .ttf.
     * Sem o arquivo, cai na letra comum em negrito. Só Latin-1: setas e marcadores ficam na letra comum.
     */
    FSlateFontInfo Display(int32 Size, int32 Weight = 700, int32 Spacing = 0)
    {
        // 24/10: a letra de desenho animado das janelas do jogo também no menu (a Cinzel brigava com o kit cartoon)
        if (FPaths::FileExists(FPaths::ProjectContentDir() / TEXT("Vortex/UI/Fonts/LuckiestGuy-Regular.ttf")))
        {
            FSlateFontInfo Info = VortexUiKit::Cartoon::Font(FMath::RoundToInt(Size * .95f), Size >= 22 ? 2 : 1);
            Info.LetterSpacing = Spacing / 4;
            return Info;
        }
        static TMap<int32, TSharedPtr<const FCompositeFont>> Fonts;
        TSharedPtr<const FCompositeFont>& Composite = Fonts.FindOrAdd(Weight);
        if (!Composite.IsValid())
        {
            const FString Path = FPaths::ProjectContentDir() / FString::Printf(TEXT("Vortex/UI/Fonts/Cinzel-%d.ttf"), Weight);
            if (FPaths::FileExists(Path))
            {
                Composite = MakeShared<FStandaloneCompositeFont>(NAME_None, Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
            }
        }
        if (!Composite.IsValid()) return Font(Size, Weight >= 900 ? TEXT("Black") : TEXT("Bold"), Spacing);
        FSlateFontInfo Info(Composite, Size);
        Info.LetterSpacing = Spacing;
        return Info;
    }

    TSharedRef<STextBlock> Text(const FString& Value, int32 Size, const FLinearColor& Color, const TCHAR* Weight = TEXT("Regular"),
        int32 Spacing = 0, bool bWrap = false)
    {
        // títulos ("Black") de todas as telas na letra de fantasia do jogo
        const bool bTitle = FCString::Strcmp(Weight, TEXT("Black")) == 0;
        return SNew(STextBlock).Text(FText::FromString(Value)).Font(bTitle ? Display(FMath::RoundToInt(Size * .92f), 900, Spacing) : Font(Size, Weight, Spacing))
            .Margin(bTitle ? VortexUiJuice::CartoonNudge(Size) : FMargin(0.f))
            .ColorAndOpacity(Color).ShadowOffset(FVector2D(0.f, 2.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .6f)).AutoWrapText(bWrap);
    }

    const FSlateBrush* White() { return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")); }
    const FSlateBrush* Round(float Radius)
    {
        static TMap<int32, TSharedPtr<FSlateRoundedBoxBrush>> Cache;
        TSharedPtr<FSlateRoundedBoxBrush>& Brush = Cache.FindOrAdd(FMath::RoundToInt(Radius));
        if (!Brush) Brush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor::White, Radius);
        return Brush.Get();
    }
    const FSlateBrush* Outline(float Radius, float Width)
    {
        static TMap<int32, TSharedPtr<FSlateRoundedBoxBrush>> Cache;
        TSharedPtr<FSlateRoundedBoxBrush>& Brush = Cache.FindOrAdd(FMath::RoundToInt(Radius) * 100 + FMath::RoundToInt(Width * 10.f));
        if (!Brush) Brush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor::Transparent, Radius, FLinearColor::White, Width);
        return Brush.Get();
    }
    const FEditableTextBoxStyle& FieldStyle()
    {
        static FEditableTextBoxStyle Style = []()
        {
            FEditableTextBoxStyle S = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>(TEXT("NormalEditableTextBox"));
            static FSlateRoundedBoxBrush Normal(FLinearColor(.015f, .025f, .045f, .92f), 8.f, FLinearColor(GoldDeep.R, GoldDeep.G, GoldDeep.B, .55f), 1.f);
            static FSlateRoundedBoxBrush Hover(FLinearColor(.02f, .035f, .06f, .95f), 8.f, Gold, 1.f);
            static FSlateRoundedBoxBrush Focus(FLinearColor(.025f, .04f, .07f, .98f), 8.f, Gold, 2.f);
            S.SetBackgroundImageNormal(Normal);
            S.SetBackgroundImageHovered(Hover);
            S.SetBackgroundImageFocused(Focus);
            S.SetBackgroundImageReadOnly(Normal);
            S.SetForegroundColor(FSlateColor(Cream));
            S.SetPadding(FMargin(14.f, 10.f));
            return S;
        }();
        return Style;
    }
    float Ease(float T) { T = FMath::Clamp(T, 0.f, 1.f); return 1.f - FMath::Pow(1.f - T, 3.f); }
    // maiúsculas com acento (FString::ToUpper só troca as letras ASCII)
    FString Upper(const FString& Value) { return FText::FromString(Value).ToUpper().ToString(); }
}
using namespace VortexUi;

// ---------------------------------------------------------------------------------------------
// fundo vivo: sombra para a leitura, brasas quadradas subindo e clarões
// ---------------------------------------------------------------------------------------------
class SVortexAmbience final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SVortexAmbience) {}
        SLATE_ATTRIBUTE(float, LeftShade)
        SLATE_ATTRIBUTE(float, FullShade)
        SLATE_ATTRIBUTE(FLinearColor, Flash)
        SLATE_ATTRIBUTE(FLinearColor, EmberTint)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        LeftShade = Args._LeftShade;
        FullShade = Args._FullShade;
        Flash = Args._Flash;
        EmberTint = Args._EmberTint;
    }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8.f, 8.f); }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Out,
        int32 Layer, const FWidgetStyle&, bool) const override
    {
        const FVector2f Size = FVector2f(Geometry.GetLocalSize());
        const double T = FSlateApplication::Get().GetCurrentTime();
        const float Left = LeftShade.Get(), Full = FullShade.Get();
        if (Full > 0.f)
        {
            FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform()), White(),
                ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, Full));
        }
        if (Left > 0.f)
        {
            TArray<FSlateGradientStop> Stops;
            Stops.Add(FSlateGradientStop(FVector2D(0.f, 0.f), FLinearColor(.005f, .01f, .02f, .88f * Left)));
            Stops.Add(FSlateGradientStop(FVector2D(Size.X * .30f, 0.f), FLinearColor(.005f, .01f, .02f, .62f * Left)));
            Stops.Add(FSlateGradientStop(FVector2D(Size.X * .55f, 0.f), FLinearColor(.005f, .01f, .02f, 0.f)));
            FSlateDrawElement::MakeGradient(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform()), Stops, Orient_Vertical);
        }
        // pé da tela: escurece para os textos e botões de baixo
        TArray<FSlateGradientStop> Bottom;
        Bottom.Add(FSlateGradientStop(FVector2D(0.f, Size.Y * .72f), FLinearColor(0.f, 0.f, 0.f, 0.f)));
        Bottom.Add(FSlateGradientStop(FVector2D(0.f, Size.Y), FLinearColor(0.f, 0.f, 0.f, .55f)));
        FSlateDrawElement::MakeGradient(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform()), Bottom, Orient_Horizontal);
        // brasas: quadradinhos (o mundo é de blocos) subindo e piscando
        const FLinearColor Tint = EmberTint.Get();
        for (int32 i = 0; i < 70; ++i)
        {
            const float Seed = FMath::Frac(i * .6180339f);
            const float Speed = .035f + .05f * FMath::Frac(i * .371f);
            const float Life = FMath::Frac(float(T) * Speed + Seed);
            const float X = FMath::Frac(i * .1337f + .07f * FMath::Sin(float(T) * .25f + i)) * Size.X + FMath::Sin(float(T) * (.6f + Seed) + i) * 24.f;
            const float Y = Size.Y * (1.08f - Life * 1.2f);
            const float S = 2.f + 4.f * FMath::Frac(i * .913f);
            const float A = FMath::Sin(Life * PI) * (.35f + .45f * FMath::Frac(i * .277f)) * (.7f + .3f * FMath::Sin(float(T) * 3.f + i));
            FLinearColor C = FMath::Lerp(Tint, FLinearColor(1.f, .45f, .12f), FMath::Frac(i * .53f) * .6f);
            C.A = A;
            FSlateDrawElement::MakeBox(Out, Layer + 1, Geometry.ToPaintGeometry(FVector2f(S, S), FSlateLayoutTransform(FVector2f(X, Y))),
                White(), ESlateDrawEffect::None, C);
        }
        const FLinearColor F = Flash.Get();
        if (F.A > 0.001f)
        {
            FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform()), White(), ESlateDrawEffect::None, F);
        }
        return Layer + 2;
    }

private:
    TAttribute<float> LeftShade, FullShade;
    TAttribute<FLinearColor> Flash, EmberTint;
};

// ---------------------------------------------------------------------------------------------
// botão: vidro escuro com borda dourada; acende, desliza e mostra uma barra ao passar o mouse
// ---------------------------------------------------------------------------------------------
class SVortexButton final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SVortexButton) : _Kind(0), _Width(0.f), _FontSize(17), _Icon(nullptr) {}
        SLATE_ATTRIBUTE(FText, Label)
        SLATE_ARGUMENT(int32, Kind)        // 0 normal, 1 principal (dourado), 2 discreto, 3 perigo
        SLATE_ARGUMENT(float, Width)
        SLATE_ARGUMENT(int32, FontSize)
        SLATE_EVENT(FSimpleDelegate, OnPressed)
        SLATE_ARGUMENT(SVortexFrontEnd*, Menu)
        SLATE_ARGUMENT(const FSlateBrush*, Icon)   // opcional, à esquerda do texto
    SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        Kind = Args._Kind;
        Menu = Args._Menu;
        OnPressed = Args._OnPressed;
        // 24/10: botão do kit cartoon (azul; amarelo no principal; vermelho no de apagar; o discreto mais escuro),
        // letra branca com contorno, e de borracha: cresce e balança com o mouse, achata ao apertar e quica ao soltar
        const FLinearColor TextColor = Cream;
        ChildSlot
        [
            SNew(SBox).WidthOverride(Args._Width > 0.f ? FOptionalSize(Args._Width) : FOptionalSize()).HeightOverride(Kind == 2 ? 46.f : 58.f)
            .RenderTransformPivot(FVector2D(.5f, .5f))
            .RenderTransform_Lambda([this]() { return VortexUiJuice::JuiceTransform(Juice); })
            [
                SAssignNew(Button, SButton)
                .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
                .ContentPadding(0.f)
                .OnPressed_Lambda([this]() { Juice.bDown = true; Juice.PressAt = VortexUiJuice::Now(); })
                .OnReleased_Lambda([this]() { Juice.bDown = false; Juice.ReleaseAt = VortexUiJuice::Now(); })
                .OnClicked_Lambda([this]()
                {
                    if (Menu) Menu->Sound(TEXT("ui_click"), .8f);
                    VortexUiJuice::Burst(Kind == 1 ? Gold : Cream, 5, .8f);
                    OnPressed.ExecuteIfBound();
                    return FReply::Handled();
                })
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()
                    [
                        SNew(SBorder).BorderImage_Lambda([this]()
                        {
                            const bool bHot = Hover > .5f;
                            const FButtonStyle& Style = VortexUiKit::Cartoon::Button(Kind == 1 ? 1 : Kind == 3 ? 5 : 0);
                            if (Juice.bDown) return &Style.Pressed;
                            return bHot ? &Style.Hovered : &Style.Normal;
                        })
                        .BorderBackgroundColor(Kind == 2 ? FLinearColor(.62f, .7f, .9f) : FLinearColor::White)
                    ]
                    + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(Args._Icon ? 14.f : 24.f, 0.f, 18.f, 0.f)
                    [
                        SNew(SHorizontalBox)
                        // o ícone (quando tem) cresce um pouco com o mouse em cima
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, Args._Icon ? 8.f : 0.f, 0.f)
                        [
                            SNew(SBox).WidthOverride(Args._Icon ? (Kind == 2 ? 30.f : 38.f) : 0.f).HeightOverride(Args._Icon ? (Kind == 2 ? 30.f : 38.f) : 0.f)
                            .Visibility(Args._Icon ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
                            [
                                SNew(SImage).Image(Args._Icon)
                                .RenderTransformPivot(FVector2D(.5f, .5f))
                                .RenderTransform_Lambda([this]() { return TOptional<FSlateRenderTransform>(FSlateRenderTransform(1.f + .14f * Hover)); })
                            ]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [
                            SAssignNew(Caption, STextBlock).Text(Args._Label).Font(Display(Args._FontSize, Kind == 2 ? 600 : 700, 40))
                            .Margin(VortexUiJuice::CartoonNudge(Args._FontSize)).ColorAndOpacity(TextColor)
                        ]
                    ]
                ]
            ]
        ];
    }

    virtual void Tick(const FGeometry& Geometry, const double CurrentTime, const float DeltaTime) override
    {
        SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
        const bool bHot = Button.IsValid() && Button->IsHovered();
        Hover = FMath::FInterpTo(Hover, bHot ? 1.f : 0.f, DeltaTime, 14.f);
        if (bHot != Juice.bHover)
        {
            Juice.bHover = bHot;
            (bHot ? Juice.HoverAt : Juice.LeaveAt) = VortexUiJuice::Now();
            if (!bHot) Juice.bDown = false;
        }
        if (Caption.IsValid()) Caption->SetRenderTransform(FSlateRenderTransform(FVector2f(6.f * Hover, 0.f)));
    }

private:
    FLinearColor Fill() const
    {
        if (Kind == 1)
        {
            // principal: dourado que brilha mais no hover e respira de leve
            const float Breath = .04f * FMath::Sin(float(FSlateApplication::Get().GetCurrentTime()) * 2.4f);
            return FMath::Lerp(GoldDeep, Gold, .35f + .45f * Hover + Breath);
        }
        const FLinearColor Base = Kind == 3 ? FLinearColor(.12f, .025f, .02f, .85f) : FLinearColor(.015f, .028f, .05f, Kind == 2 ? .55f : .82f);
        return FMath::Lerp(Base, FLinearColor(.06f, .085f, .13f, .92f), Hover);
    }
    int32 Kind = 0;
    float Hover = 0.f;
    VortexUiJuice::FJuice Juice;
    SVortexFrontEnd* Menu = nullptr;
    FSimpleDelegate OnPressed;
    TSharedPtr<SButton> Button;
    TSharedPtr<STextBlock> Caption;
};

// ---------------------------------------------------------------------------------------------
// entrada grande do início: ícone voxel 3D (feito no Blender), título na Cinzel e o que ela faz.
// No mouse: desliza, a barra dourada cresce, o ícone sobe e a esmeralda acende atrás dele.
// ---------------------------------------------------------------------------------------------
class SVortexMenuEntry final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SVortexMenuEntry) : _bPrimary(false), _Index(0) {}
        SLATE_ARGUMENT(FString, Title)
        SLATE_ARGUMENT(FString, Subtitle)
        SLATE_ARGUMENT(const FSlateBrush*, Icon)
        SLATE_ARGUMENT(bool, bPrimary)
        SLATE_ARGUMENT(int32, Index)
        SLATE_ARGUMENT(SVortexFrontEnd*, Menu)
        SLATE_EVENT(FSimpleDelegate, OnPressed)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        Menu = Args._Menu;
        OnPressed = Args._OnPressed;
        bPrimary = Args._bPrimary;
        Index = Args._Index;
        Born = FSlateApplication::Get().GetCurrentTime();
        ChildSlot
        [
            SNew(SBox).WidthOverride(560.f).HeightOverride(104.f)
            [
                SAssignNew(Button, SButton)
                .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
                .ContentPadding(0.f)
                .OnPressed_Lambda([this]() { Juice.bDown = true; Juice.PressAt = VortexUiJuice::Now(); })
                .OnReleased_Lambda([this]() { Juice.bDown = false; Juice.ReleaseAt = VortexUiJuice::Now(); })
                .OnClicked_Lambda([this]()
                {
                    if (Menu) Menu->Sound(TEXT("ui_open"), .9f);
                    VortexUiJuice::Burst(bPrimary ? Gold : Cream, 7, 1.f);
                    OnPressed.ExecuteIfBound();
                    return FReply::Handled();
                })
                [
                    SNew(SOverlay)
                    // 24/10: placa do kit cartoon (cartão azul; a principal, amarela), que clareia com o mouse
                    + SOverlay::Slot()
                    [
                        SNew(SBorder).BorderImage(bPrimary ? &VortexUiKit::Cartoon::Button(1).Normal : VortexUiKit::Cartoon::Card())
                        .BorderBackgroundColor_Lambda([this]() { return FSlateColor(FLinearColor(1.f + .18f * Hover, 1.f + .18f * Hover, 1.f + .18f * Hover)); })
                    ]
                    + SOverlay::Slot().Padding(16.f, 0.f, 18.f, 0.f)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [
                            SNew(SBox).WidthOverride(92.f).HeightOverride(92.f)
                            [
                                SNew(SOverlay)
                                // brilho da esmeralda atrás do ícone
                                + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                                [
                                    SNew(SBox).WidthOverride(70.f).HeightOverride(70.f)
                                    [
                                        SNew(SImage).Image(Round(35.f)).ColorAndOpacity_Lambda([this]()
                                        {
                                            FLinearColor C = Emerald; C.A = .06f + .22f * Hover + (bPrimary ? .06f : 0.f);
                                            return FSlateColor(C);
                                        })
                                    ]
                                ]
                                + SOverlay::Slot()[SAssignNew(Icon, SImage).Image(Args._Icon)]
                            ]
                        ]
                        + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(14.f, 0.f, 0.f, 0.f)
                        [
                            SAssignNew(Words, SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()
                            [
                                SNew(STextBlock).Text(FText::FromString(Upper(Args._Title))).Font(Display(bPrimary ? 30 : 25, 900, 60))
                                .Margin(VortexUiJuice::CartoonNudge(bPrimary ? 30 : 25))
                                .ShadowOffset(FVector2D(0.f, 2.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, .1f, .7f))
                                .ColorAndOpacity_Lambda([this]() { return FSlateColor(FMath::Lerp(bPrimary ? Cream : Gold, FLinearColor::White, Hover)); })
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(1.f, 3.f, 0.f, 0.f)
                            [Text(Args._Subtitle, 14, bPrimary ? FLinearColor::FromSRGBColor(FColor(0x5A, 0x34, 0x08)) : Cream, TEXT("Bold"), 0, true)]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [
                            // seta que aparece com o mouse
                            SNew(STextBlock).Text(FText::FromString(TEXT("›"))).Font(Font(34, TEXT("Bold")))
                            .ColorAndOpacity_Lambda([this]() { FLinearColor C = Cream; C.A = Hover; return FSlateColor(C); })
                        ]
                    ]
                ]
            ]
        ];
    }

    virtual void Tick(const FGeometry& Geometry, const double CurrentTime, const float DeltaTime) override
    {
        SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
        const bool bHot = Button.IsValid() && Button->IsHovered();
        if (bHot && !bWasHot && Menu) Menu->Sound(TEXT("ui_click"), .18f);
        if (bHot != Juice.bHover)
        {
            Juice.bHover = bHot;
            (bHot ? Juice.HoverAt : Juice.LeaveAt) = VortexUiJuice::Now();
            if (!bHot) Juice.bDown = false;
        }
        bWasHot = bHot;
        Hover = FMath::FInterpTo(Hover, bHot ? 1.f : 0.f, DeltaTime, 12.f);
        // entrada em cascata: cada uma chega um pouco depois, vindo da esquerda; e de borracha (balança, achata, quica)
        const float In = Ease(float(CurrentTime - Born) / .5f - Index * .12f);
        SetRenderOpacity(In);
        SetRenderTransformPivot(FVector2D(.5f, .5f));
        const TOptional<FSlateRenderTransform> Rubber = VortexUiJuice::JuiceTransform(Juice);
        SetRenderTransform(FSlateRenderTransform(Rubber ? Rubber->GetMatrix() : FMatrix2x2f(), FVector2f(-40.f * (1.f - In) + 12.f * Hover, 0.f)));
        if (Icon.IsValid())
        {
            Icon->SetRenderTransformPivot(FVector2D(.5f, .5f));
            const float Bob = FMath::Sin(float(CurrentTime) * 2.2f + Index) * 2.f * (.3f + Hover);
            Icon->SetRenderTransform(FSlateRenderTransform(FScale2f(1.f + .1f * Hover), FVector2f(0.f, Bob - 4.f * Hover)));
        }
    }

private:
    SVortexFrontEnd* Menu = nullptr;
    FSimpleDelegate OnPressed;
    bool bPrimary = false, bWasHot = false;
    int32 Index = 0;
    float Hover = 0.f;
    double Born = 0.0;
    VortexUiJuice::FJuice Juice;
    TSharedPtr<SButton> Button;
    TSharedPtr<SImage> Icon;
    TSharedPtr<SVerticalBox> Words;
};

// ---------------------------------------------------------------------------------------------
// botão de canto (sair do jogo): o ícone e o nome embaixo
// ---------------------------------------------------------------------------------------------
class SVortexCornerButton final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SVortexCornerButton) {}
        SLATE_ARGUMENT(FString, Label)
        SLATE_ARGUMENT(const FSlateBrush*, Icon)
        SLATE_ARGUMENT(SVortexFrontEnd*, Menu)
        SLATE_EVENT(FSimpleDelegate, OnPressed)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        Menu = Args._Menu;
        OnPressed = Args._OnPressed;
        ChildSlot
        [
            SAssignNew(Button, SButton)
            .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
            .ContentPadding(0.f)
            .OnClicked_Lambda([this]() { if (Menu) Menu->Sound(TEXT("ui_click"), .8f); OnPressed.ExecuteIfBound(); return FReply::Handled(); })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                [
                    SNew(SBox).WidthOverride(64.f).HeightOverride(64.f)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()[SNew(SImage).Image(Round(14.f)).ColorAndOpacity_Lambda([this]() { return FSlateColor(FMath::Lerp(FLinearColor(.012f, .022f, .04f, .6f), FLinearColor(.18f, .05f, .04f, .85f), Hover)); })]
                        + SOverlay::Slot()[SNew(SImage).Image(Outline(14.f, 1.5f)).ColorAndOpacity_Lambda([this]() { return FSlateColor(FMath::Lerp(FLinearColor(GoldDeep.R, GoldDeep.G, GoldDeep.B, .45f), Bad, Hover)); })]
                        + SOverlay::Slot().Padding(6.f)[SNew(SImage).Image(Args._Icon)]
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 0.f)
                [
                    SNew(STextBlock).Text(FText::FromString(Upper(Args._Label))).Font(Display(11, 700, 120))
                    .ColorAndOpacity_Lambda([this]() { return FSlateColor(FMath::Lerp(Muted, Cream, Hover)); })
                ]
            ]
        ];
    }
    virtual void Tick(const FGeometry& Geometry, const double CurrentTime, const float DeltaTime) override
    {
        SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
        Hover = FMath::FInterpTo(Hover, Button.IsValid() && Button->IsHovered() ? 1.f : 0.f, DeltaTime, 12.f);
    }
private:
    SVortexFrontEnd* Menu = nullptr;
    FSimpleDelegate OnPressed;
    TSharedPtr<SButton> Button;
    float Hover = 0.f;
};

// ---------------------------------------------------------------------------------------------
// carta da raça
// ---------------------------------------------------------------------------------------------
class SVortexRaceCard final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SVortexRaceCard) : _Index(0) {}
        SLATE_ARGUMENT(int32, Index)
        SLATE_ARGUMENT(SVortexFrontEnd*, Menu)
        SLATE_ARGUMENT(const FSlateBrush*, Art)
        SLATE_EVENT(FSimpleDelegate, OnChosen)
    SLATE_END_ARGS()

    static constexpr float Width = 276.f, Height = 760.f, Gap = 16.f;

    void Construct(const FArguments& Args)
    {
        Index = Args._Index;
        Menu = Args._Menu;
        OnChosen = Args._OnChosen;
        const FVortexLookCatalog& Catalog = FVortexLookCatalog::Get();
        const FVortexRaceInfo& Race = Catalog.Races[Index];
        const FVortexRaceInfo& Human = Catalog.Races[0];
        Accent = Race.Accent;
        bLocked = !IsRaceOpen(Race.Id);

        TSharedRef<SVerticalBox> Stats = SNew(SVerticalBox);
        static const TCHAR* Short[] = { TEXT("FOR"), TEXT("DES"), TEXT("VIT"), TEXT("INT"), TEXT("MAN"), TEXT("ESP"), TEXT("REC"), TEXT("SOR") };
        for (int32 i = 0; i < Race.Start.Num() && i < 8; ++i)
        {
            const int32 Value = Race.Start[i], Base = Human.Start.IsValidIndex(i) ? Human.Start[i] : Value;
            const FLinearColor BarColor = Value > Base ? Good : Value < Base ? Bad : FLinearColor(Gold.R, Gold.G, Gold.B, .85f);
            Stats->AddSlot().AutoHeight().Padding(0.f, 1.5f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [SNew(SBox).WidthOverride(38.f)[Text(Short[i], 10, Muted, TEXT("Bold"), 60)]]
                + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                [
                    SNew(SBox).HeightOverride(8.f)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()[SNew(SImage).Image(Round(4.f)).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, .08f))]
                        + SOverlay::Slot().HAlign(HAlign_Left)
                        [
                            SNew(SBox).WidthOverride_Lambda([this, Value]()
                            {
                                // as barras enchem quando a carta aparece
                                const float Grow = Ease(float(Menu->Clock() - Born) / .9f - Index * .06f);
                                return FOptionalSize(170.f * Value / 12.f * Grow);
                            })
                            [SNew(SImage).Image(Round(4.f)).ColorAndOpacity(BarColor)]
                        ]
                    ]
                ]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
                [SNew(SBox).WidthOverride(20.f)[Text(FString::FromInt(Value), 12, Cream, TEXT("Bold"))]]
            ];
        }
        TSharedRef<SVerticalBox> Traits = SNew(SVerticalBox);
        for (const FString& Trait : Race.Traits)
        {
            const bool bWeak = Race.IsWeakness(Trait);
            Traits->AddSlot().AutoHeight().Padding(0.f, 2.f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)[Text(bWeak ? TEXT("-") : TEXT("+"), 14, bWeak ? Bad : Good, TEXT("Bold"))]
                + SHorizontalBox::Slot().FillWidth(1.f)[Text(Trait, 11, bWeak ? FLinearColor(Bad.R, Bad.G, Bad.B, .95f) : Cream, TEXT("Regular"), 0, true)]
            ];
        }

        ChildSlot
        [
            SNew(SBox).WidthOverride(Width).HeightOverride(Height)
            [
                SAssignNew(Button, SButton)
                .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
                .ContentPadding(0.f)
                .OnClicked_Lambda([this]()
                {
                    if (bLocked) Menu->Sound(TEXT("ui_click"), .3f);
                    else if (Menu->ChosenRaceIndex() == INDEX_NONE) OnChosen.ExecuteIfBound();
                    return FReply::Handled();
                })
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()[SNew(SBorder).BorderImage(VortexUiKit::Panel())]
                    + SOverlay::Slot()
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()
                        [
                            SNew(SBox).HeightOverride(300.f).Clipping(EWidgetClipping::ClipToBounds)
                            [
                                SNew(SOverlay)
                                + SOverlay::Slot()
                                [
                                    // brilho da raça atrás do retrato
                                    SNew(SBorder).BorderImage(Round(16.f)).BorderBackgroundColor_Lambda([this]()
                                    {
                                        return FLinearColor(Accent.R, Accent.G, Accent.B, .10f + .22f * Hover);
                                    })
                                ]
                                + SOverlay::Slot()
                                [
                                    SAssignNew(Portrait, SImage).Image(Args._Art)
                                ]
                                + SOverlay::Slot().VAlign(VAlign_Bottom)
                                [
                                    SNew(SBox).HeightOverride(110.f)
                                    [
                                        SNew(SImage).Image(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))).ColorAndOpacity(FLinearColor::Transparent)
                                    ]
                                ]
                                + SOverlay::Slot().VAlign(VAlign_Bottom).Padding(20.f, 0.f, 20.f, 14.f)
                                [
                                    SNew(SVerticalBox)
                                    + SVerticalBox::Slot().AutoHeight()[Text(Upper(Race.Name), Race.Name.Len() > 7 ? 24 : 30, Cream, TEXT("Black"), 100)]
                                    + SVerticalBox::Slot().AutoHeight()[Text(Upper(Race.Epithet), 11, Accent, TEXT("Bold"), 200)]
                                ]
                            ]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(20.f, 12.f, 20.f, 6.f)
                        [Text(Race.Tagline, 12, FLinearColor(Cream.R, Cream.G, Cream.B, .85f), TEXT("Italic"), 0, true)]
                        + SVerticalBox::Slot().AutoHeight().Padding(20.f, 4.f)[Stats]
                        + SVerticalBox::Slot().FillHeight(1.f).Padding(20.f, 10.f, 20.f, 0.f)[Traits]
                        + SVerticalBox::Slot().AutoHeight().Padding(20.f, 8.f, 20.f, 18.f).HAlign(HAlign_Center)
                        [
                            SNew(STextBlock).Text(FText::FromString(bLocked ? TEXT("EM BREVE") : TEXT("ESCOLHER"))).Font(Font(13, TEXT("Bold"), 300))
                            .ColorAndOpacity_Lambda([this]() { return FSlateColor(FLinearColor(Accent.R, Accent.G, Accent.B, .25f + .75f * Hover)); })
                        ]
                    ]
                    + SOverlay::Slot()
                    [
                        // trancada: a carta escurece e um cadeado fica sobre o retrato
                        SNew(SOverlay).Visibility(bLocked ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
                        + SOverlay::Slot()[SNew(SImage).Image(Round(16.f)).ColorAndOpacity(FLinearColor(Night.R, Night.G, Night.B, .62f))]
                        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 92.f, 0.f, 0.f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                            [
                                // a alça (contorno arredondado) por trás do corpo
                                SNew(SBox).WidthOverride(46.f).HeightOverride(50.f)
                                [SNew(SImage).Image(Outline(23.f, 8.f)).ColorAndOpacity(Gold)]
                            ]
                            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, -26.f, 0.f, 0.f)
                            [
                                SNew(SBox).WidthOverride(76.f).HeightOverride(62.f)
                                [
                                    SNew(SOverlay)
                                    + SOverlay::Slot()[SNew(SImage).Image(Round(12.f)).ColorAndOpacity(GoldDeep)]
                                    + SOverlay::Slot().Padding(4.f)[SNew(SImage).Image(Round(9.f)).ColorAndOpacity(Gold)]
                                    + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                                    [SNew(SBox).WidthOverride(12.f).HeightOverride(22.f)[SNew(SImage).Image(Round(6.f)).ColorAndOpacity(Night)]]
                                ]
                            ]
                        ]
                    ]
                    + SOverlay::Slot()
                    [
                        // borda: dourada parada, na cor da raça e pulsando no hover
                        SNew(SBorder).BorderImage(Outline(16.f, 2.f)).BorderBackgroundColor_Lambda([this]()
                        {
                            const float Pulse = .75f + .25f * FMath::Sin(float(Menu->Clock()) * 4.f);
                            return FMath::Lerp(FLinearColor(GoldDeep.R, GoldDeep.G, GoldDeep.B, .45f), Accent * Pulse, Hover);
                        })
                    ]
                ]
            ]
        ];
        Born = Menu->Clock();
    }

    virtual void Tick(const FGeometry& Geometry, const double CurrentTime, const float DeltaTime) override
    {
        SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
        const bool bHovered = !bLocked && Button.IsValid() && Button->IsHovered() && Menu->ChosenRaceIndex() == INDEX_NONE;
        if (bHovered && !bWasHovered) Menu->Sound(TEXT("ui_open"), .25f);
        bWasHovered = bHovered;
        Hover = FMath::FInterpTo(Hover, bHovered ? 1.f : 0.f, DeltaTime, 10.f);
        // entrada: cada carta sobe com um pequeno atraso
        const float Enter = Ease(float(Menu->Clock() - Born) / .55f - Index * .08f);
        float X = 0.f, Y = (1.f - Enter) * 80.f - 16.f * Hover, Scale = 1.f + .035f * Hover, Opacity = Enter;
        const int32 Chosen = Menu->ChosenRaceIndex();
        if (Chosen != INDEX_NONE)
        {
            const float T = Ease(float(Menu->ChosenAge()) / .55f);
            if (Chosen == Index)
            {
                // a escolhida vai para o centro e cresce
                X = (2.5f - Index) * (Width + Gap) * T;
                Scale = 1.f + .14f * T;
                Y = -16.f * (1.f - T);
                Opacity = 1.f - Ease(float(Menu->ChosenAge() - .7) / .45f);
            }
            else
            {
                Y += 120.f * T;
                Opacity = 1.f - T;
            }
        }
        SetRenderOpacity(Opacity);
        // o contorno arredondado não respeita a opacidade: carta que sumiu fica escondida
        // (só depois da escolha: escondido, o widget para de atualizar e não volta a aparecer)
        SetVisibility(Chosen != INDEX_NONE && Opacity < .02f ? EVisibility::Hidden : EVisibility::Visible);
        SetRenderTransformPivot(FVector2D(.5f, .5f));
        SetRenderTransform(FSlateRenderTransform(FScale2f(Scale), FVector2f(X, Y)));
        if (Portrait.IsValid())
        {
            Portrait->SetRenderTransformPivot(FVector2D(.5f, .4f));
            Portrait->SetRenderTransform(FSlateRenderTransform(FScale2f(1.f + .07f * Hover)));
        }
    }

private:
    int32 Index = 0;
    float Hover = 0.f;
    double Born = 0.0;
    bool bWasHovered = false;
    bool bLocked = false;
    FLinearColor Accent = FLinearColor::White;
    SVortexFrontEnd* Menu = nullptr;
    FSimpleDelegate OnChosen;
    TSharedPtr<SButton> Button;
    TSharedPtr<SImage> Portrait;
};

// ---------------------------------------------------------------------------------------------
// seletor de cor livre: quadrado de saturação/brilho e faixa de matiz (a cor sai em sRGB)
// ---------------------------------------------------------------------------------------------
class SVortexColorPicker final : public SLeafWidget
{
public:
    DECLARE_DELEGATE_OneParam(FOnColor, FColor);
    SLATE_BEGIN_ARGS(SVortexColorPicker) {}
        SLATE_ATTRIBUTE(FColor, Color)
        SLATE_EVENT(FOnColor, OnColor)
    SLATE_END_ARGS()

    static constexpr float Size = 250.f, HueHeight = 22.f, Gap = 12.f;

    void Construct(const FArguments& Args)
    {
        Color = Args._Color;
        OnColor = Args._OnColor;
        SyncFromColor();
    }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(Size, Size + Gap + HueHeight); }

    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Out,
        int32 Layer, const FWidgetStyle&, bool) const override
    {
        if (!bDragging) const_cast<SVortexColorPicker*>(this)->SyncFromColor();
        const FLinearColor Pure = FLinearColor(Hue, 1.f, 1.f).HSVToLinearRGB();
        const FVector2f Square(Size, Size);
        // saturação: branco -> matiz; brilho: transparente -> preto
        TArray<FSlateGradientStop> Across;
        Across.Add(FSlateGradientStop(FVector2D(0.f, 0.f), FLinearColor::White));
        Across.Add(FSlateGradientStop(FVector2D(Size, 0.f), Pure));
        FSlateDrawElement::MakeGradient(Out, Layer, Geometry.ToPaintGeometry(Square, FSlateLayoutTransform()), Across, Orient_Vertical,
            ESlateDrawEffect::NoGamma);
        TArray<FSlateGradientStop> Down;
        Down.Add(FSlateGradientStop(FVector2D(0.f, 0.f), FLinearColor(0.f, 0.f, 0.f, 0.f)));
        Down.Add(FSlateGradientStop(FVector2D(0.f, Size), FLinearColor::Black));
        FSlateDrawElement::MakeGradient(Out, Layer + 1, Geometry.ToPaintGeometry(Square, FSlateLayoutTransform()), Down, Orient_Horizontal,
            ESlateDrawEffect::NoGamma);
        // faixa do matiz
        TArray<FSlateGradientStop> Hues;
        for (int32 i = 0; i <= 6; ++i)
        {
            Hues.Add(FSlateGradientStop(FVector2D(Size * i / 6.f, 0.f), FLinearColor(360.f * i / 6.f, 1.f, 1.f).HSVToLinearRGB()));
        }
        FSlateDrawElement::MakeGradient(Out, Layer, Geometry.ToPaintGeometry(FVector2f(Size, HueHeight), FSlateLayoutTransform(FVector2f(0.f, Size + Gap))),
            Hues, Orient_Vertical, ESlateDrawEffect::NoGamma);
        // marcadores
        const FSlateBrush* Ring = VortexUi::Outline(8.f, 2.f);
        const FVector2f Dot(Sat * Size - 8.f, (1.f - Val) * Size - 8.f);
        FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2f(16.f, 16.f), FSlateLayoutTransform(Dot)), Ring,
            ESlateDrawEffect::None, Val > .5f ? FLinearColor::Black : FLinearColor::White);
        FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2f(8.f, HueHeight + 6.f),
            FSlateLayoutTransform(FVector2f(Hue / 360.f * Size - 4.f, Size + Gap - 3.f))), VortexUi::Outline(3.f, 2.f), ESlateDrawEffect::None, FLinearColor::White);
        return Layer + 2;
    }

    virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
        const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
        Area = Local.Y <= Size ? 0 : 1;
        bDragging = true;
        Pick(Local);
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }
    virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (!bDragging) return FReply::Unhandled();
        Pick(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
        return FReply::Handled();
    }
    virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent&) override
    {
        bDragging = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    virtual FCursorReply OnCursorQuery(const FGeometry&, const FPointerEvent&) const override { return FCursorReply::Cursor(EMouseCursor::Crosshairs); }

private:
    void SyncFromColor()
    {
        const FColor C = Color.Get();
        const FLinearColor Hsv = FLinearColor(C.R / 255.f, C.G / 255.f, C.B / 255.f).LinearRGBToHSV();
        if (Hsv.G > .01f && Hsv.B > .01f) Hue = Hsv.R;
        Sat = Hsv.G;
        Val = Hsv.B;
    }
    void Pick(const FVector2D& Local)
    {
        if (Area == 0)
        {
            Sat = FMath::Clamp(float(Local.X / Size), 0.f, 1.f);
            Val = 1.f - FMath::Clamp(float(Local.Y / Size), 0.f, 1.f);
        }
        else Hue = FMath::Clamp(float(Local.X / Size), 0.f, 1.f) * 360.f;
        const FLinearColor Rgb = FLinearColor(Hue, Sat, Val).HSVToLinearRGB();
        OnColor.ExecuteIfBound(Rgb.ToFColor(false));
    }
    TAttribute<FColor> Color;
    FOnColor OnColor;
    float Hue = 0.f, Sat = 0.f, Val = 1.f;
    int32 Area = 0;
    bool bDragging = false;
};

// ---------------------------------------------------------------------------------------------
// área do palco na criação: arrastar gira o herói, a roda aproxima
// ---------------------------------------------------------------------------------------------
class SVortexStageInput final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SVortexStageInput) {}
        SLATE_ARGUMENT(SVortexFrontEnd*, Menu)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args) { Menu = Args._Menu; }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8.f, 8.f); }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&, FSlateWindowElementList&, int32 Layer, const FWidgetStyle&, bool) const override
    {
        return Layer;
    }
    virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& Event) override
    {
        const FKey Button = Event.GetEffectingButton();
        if (Button != EKeys::LeftMouseButton && Button != EKeys::RightMouseButton) return FReply::Unhandled();
        bDragging = true;
        bPanning = Button == EKeys::RightMouseButton;
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }
    virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& Event) override
    {
        bDragging = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    virtual FReply OnMouseMove(const FGeometry&, const FPointerEvent& Event) override
    {
        if (!bDragging) return FReply::Unhandled();
        AVortexMenuStage* Stage = Menu->Stage();
        if (!Stage) return FReply::Handled();
        const FVector2D Delta = Event.GetCursorDelta();
        // esquerdo: gira a câmera em volta do herói; direito: sobe e desce
        if (bPanning) Stage->Pan(Delta.Y * .6f);
        else Stage->Orbit(Delta.X * .35f, -Delta.Y * .25f);
        return FReply::Handled();
    }
    virtual FReply OnMouseWheel(const FGeometry&, const FPointerEvent& Event) override
    {
        if (AVortexMenuStage* Stage = Menu->Stage()) Stage->Zoom(Event.GetWheelDelta());
        return FReply::Handled();
    }
    virtual FCursorReply OnCursorQuery(const FGeometry&, const FPointerEvent&) const override
    {
        return FCursorReply::Cursor(bDragging ? EMouseCursor::GrabHandClosed : EMouseCursor::GrabHand);
    }
private:
    SVortexFrontEnd* Menu = nullptr;
    bool bDragging = false, bPanning = false;
};

// ---------------------------------------------------------------------------------------------
// menu
// ---------------------------------------------------------------------------------------------
void SVortexFrontEnd::Construct(const FArguments& Args)
{
    Owner = Args._Owner;
    Rng.Initialize(int32(FDateTime::Now().GetTicks() & 0x7fffffff));
    Now = FSlateApplication::Get().GetCurrentTime();
    auto LoadBrush = [this](FSlateBrush& Brush, const TCHAR* Path, FVector2D Size)
    {
        if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, Path))
        {
            if (Owner.IsValid()) Owner->KeepForMenu(Texture);
            Brush.SetResourceObject(Texture);
            Brush.ImageSize = Size;
            Brush.DrawAs = ESlateBrushDrawType::Image;
        }
    };
    LoadBrush(EmblemBrush, TEXT("/Game/Vortex/UI/Menu/T_Emblem.T_Emblem"), FVector2D(150.f, 150.f));
    LoadBrush(LogoBrush, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuLogo.T_MenuLogo"), FVector2D(672.f, 252.f));
    LoadBrush(IconPlay, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuIcon_Play.T_MenuIcon_Play"), FVector2D(92.f, 92.f));
    LoadBrush(IconJoin, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuIcon_Join.T_MenuIcon_Join"), FVector2D(92.f, 92.f));
    LoadBrush(IconOptions, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuIcon_Options.T_MenuIcon_Options"), FVector2D(92.f, 92.f));
    LoadBrush(IconQuit, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuIcon_Quit.T_MenuIcon_Quit"), FVector2D(52.f, 52.f));
    // troféu das Conquistas (Tools/Blender/MakeTrophyIcon.py + Tools/ImportMenuIcons.py)
    LoadBrush(IconTrophy, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuIcon_Trophy.T_MenuIcon_Trophy"), FVector2D(92.f, 92.f));
    LoadBrush(IconHost, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuIcon_Host.T_MenuIcon_Host"), FVector2D(92.f, 92.f));
    LoadBrush(IconKey, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuIcon_Key.T_MenuIcon_Key"), FVector2D(92.f, 92.f));
    LoadBrush(IconNewWorld, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuIcon_NewWorld.T_MenuIcon_NewWorld"), FVector2D(92.f, 92.f));
    LoadBrush(IconNewHero, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuIcon_NewHero.T_MenuIcon_NewHero"), FVector2D(92.f, 92.f));
    LoadBrush(IconDelete, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuIcon_Delete.T_MenuIcon_Delete"), FVector2D(92.f, 92.f));
    LoadBrush(IconBack, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuIcon_Back.T_MenuIcon_Back"), FVector2D(92.f, 92.f));
    LoadBrush(IconReady, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuIcon_Ready.T_MenuIcon_Ready"), FVector2D(92.f, 92.f));
    LoadBrush(WorldArt, TEXT("/Game/Vortex/UI/Menu/Art/T_MenuWorldArt.T_MenuWorldArt"), FVector2D(288.f, 192.f));
    for (const FVortexRaceInfo& Race : FVortexLookCatalog::Get().Races)
    {
        FSlateBrush& Brush = RaceArt.Add(Race.Id);
        const FString Path = FString::Printf(TEXT("/Game/Vortex/UI/Menu/Races/T_Race_%s.T_Race_%s"), *Race.Id.ToString(), *Race.Id.ToString());
        LoadBrush(Brush, *Path, FVector2D(330.f, 330.f));
    }
    // no início, um herói sorteado de uma raça liberada espera no palco
    const FVortexLookCatalog& Catalog = FVortexLookCatalog::Get();
    if (Catalog.IsValid())
    {
        TArray<const FVortexRaceInfo*> Open;
        for (const FVortexRaceInfo& Each : Catalog.Races) if (IsRaceOpen(Each.Id)) Open.Add(&Each);
        const FVortexRaceInfo& Race = Open.Num() ? *Open[Rng.RandHelper(Open.Num())] : Catalog.Races[0];
        Look = Catalog.Random(Race.Id, Rng.FRand() < .5f ? FName(TEXT("male")) : FName(TEXT("female")), Rng);
    }

    TSharedPtr<VortexUiJuice::SCartoonFx> Fx;
    ChildSlot
    [
        SNew(SOverlay)
        + SOverlay::Slot()
        [
            SNew(SVortexAmbience)
            .LeftShade_Lambda([this]() { return Page == EPage::Creator || Page == EPage::Races ? 0.f : Page == EPage::Heroes ? .6f : 1.f; })
            .FullShade_Lambda([this]() { return Page == EPage::Races ? .25f : 0.f; })
            .EmberTint_Lambda([this]()
            {
                const FVortexRaceInfo* Race = FVortexLookCatalog::Get().Race(Look.Race);
                // fagulhas da Gênese: esmeralda (com ouro), e a cor da raça na criação
                return Page == EPage::Creator && Race ? Race->Accent : FMath::Lerp(Emerald, Gold, .35f);
            })
            .Flash_Lambda([this]()
            {
                // clarão na revelação da raça e ao criar o herói
                float A = 0.f;
                FLinearColor C = FLinearColor::White;
                if (ChosenRace != INDEX_NONE)
                {
                    const float T = float(Now - ChosenAt) - .55f;
                    if (T > 0.f) { A = FMath::Exp(-T * 3.5f) * .55f; C = FVortexLookCatalog::Get().Races[ChosenRace].Accent; }
                }
                const float P = float(Now - CelebrateAt);
                if (P >= 0.f && P < 3.f) { const float B = FMath::Exp(-P * 3.f) * .6f; if (B > A) { A = B; C = Gold; } }
                C.A = A;
                return C;
            })
        ]
        + SOverlay::Slot()
        [
            SAssignNew(PageHost, SBox)
        ]
        // 02/11: na primeira abertura (VortexGraphics::FirstRunSetup), a pergunta dos shaders por cima de tudo
        + SOverlay::Slot()
        [
            SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
            .Visibility_Lambda([]() { return VortexGraphics::NeedsShaderChoice() ? EVisibility::Visible : EVisibility::Collapsed; })
            [
                GlassPanel(TEXT("Ativar os shaders?"),
                    TEXT("A qualidade já foi ajustada para o seu PC. Os shaders deixam o mundo mais bonito (luz rebatida, reflexos, raios de sol e névoa encorpada), mas pesam uns 25% no FPS. Dá para mudar depois em Opções > Shaders."),
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("Sim, ativar"), FSimpleDelegate::CreateLambda([this]() { VortexGraphics::ChooseShaders(true); Sound(TEXT("ui_click"), .5f); }), 3, 220.f)]
                    + SHorizontalBox::Slot().AutoWidth().Padding(12.f, 0.f)[Button(TEXT("Não, mais leve"), FSimpleDelegate::CreateLambda([this]() { VortexGraphics::ChooseShaders(false); Sound(TEXT("ui_click"), .5f); }), 1, 220.f)])
            ]
        ]
        + SOverlay::Slot()
        [
            // cortina da abertura
            SNew(SImage).Image(White())
            .Visibility_Lambda([this]() { return bStageReady && Now - ReadyAt > 1.6 ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
            .ColorAndOpacity_Lambda([this]()
            {
                const float A = bStageReady ? 1.f - Ease(float(Now - ReadyAt) / 1.4f) : 1.f;
                return FSlateColor(FLinearColor(.004f, .008f, .016f, A));
            })
        ]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 26.f)
        [
            // aviso (erros e confirmações)
            SNew(SBorder)
            .BorderImage(Round(20.f))
            .Padding(FMargin(22.f, 10.f))
            .Visibility_Lambda([this]() { return Status.IsEmpty() || Now - StatusAt > (bStatusError ? 12.0 : 5.0) ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
            .BorderBackgroundColor_Lambda([this]() { return bStatusError ? FLinearColor(.25f, .04f, .03f, .92f) : FLinearColor(.03f, .05f, .09f, .92f); })
            [
                SNew(STextBlock).Font(Font(15, TEXT("Bold"))).ColorAndOpacity(Cream)
                .Text_Lambda([this]() { return FText::FromString(Status); })
            ]
        ]
        // estrelinhas dos cliques (as mesmas das janelas do jogo)
        + SOverlay::Slot()[SAssignNew(Fx, VortexUiJuice::SCartoonFx)]
    ];
    VortexUiJuice::FxLayer = Fx;
    // conectado no mapa do menu: é a sala de espera
    Go(Owner.IsValid() && Owner->InLobby() ? EPage::Lobby : EPage::Home);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexFrontEndShot"))) { ShotStep = 0; ShotAt = Now; }
#endif
}

AVortexMenuStage* SVortexFrontEnd::Stage() const
{
    return Owner.IsValid() ? Owner->GetMenuStage() : nullptr;
}

void SVortexFrontEnd::Sound(FName Name, float Volume) const
{
    if (AVortexMenuStage* S = Stage()) S->PlayUi(Name, Volume);
}

void SVortexFrontEnd::SetStatus(const FString& Message, bool bError)
{
    Status = Message;
    bStatusError = bError;
    StatusAt = Now;
    if (bError && !Message.IsEmpty()) Sound(TEXT("ui_error"), .7f);
}

FString SVortexFrontEnd::RaceName(FName RaceId) const
{
    const FVortexRaceInfo* Race = FVortexLookCatalog::Get().Race(RaceId);
    return Race ? Race->Name : RaceId.ToString();
}

FReply SVortexFrontEnd::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() != EKeys::Escape || !PageBack) return FReply::Unhandled();
    Sound(TEXT("ui_close"), .8f);
    const TFunction<void()> Back = PageBack;   // a volta troca de página e refaz o PageBack
    Back();
    return FReply::Handled();
}

void SVortexFrontEnd::Go(EPage NewPage)
{
    Page = NewPage;
    PageBack = nullptr;
    // o Esc precisa do foco no menu, mesmo depois de clicar fora de um campo
    if (DoesSharedInstanceExist() && FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocus(AsShared(), EFocusCause::SetDirectly);
    PageAt = Now;
    TSharedRef<SWidget> Content = SNullWidget::NullWidget;
    switch (Page)
    {
    case EPage::Home: Content = BuildHome(); break;
    case EPage::Worlds: Content = BuildWorlds(); break;
    case EPage::CreateWorld: Content = BuildCreateWorld(); break;
    case EPage::Join: Content = BuildJoin(); break;
    case EPage::Options: Content = BuildOptions(); break;
    case EPage::Heroes: Content = BuildHeroes(); break;
    case EPage::DeleteHero: Content = BuildDeleteHero(); break;
    case EPage::DeleteWorld: Content = BuildDeleteWorld(); break;
    case EPage::Races: Content = BuildRaces(); break;
    case EPage::Creator: Content = BuildCreator(); break;
    case EPage::Lobby: Content = BuildLobby(); break;
    case EPage::Multiplayer: Content = BuildMultiplayer(); break;
    case EPage::Achievements: Content = BuildAchievements(); break;
    }
    PageHost->SetContent(Content);
    if (AVortexMenuStage* S = Stage())
    {
        S->SetMode(Page == EPage::Creator ? EVortexStageMode::Creator : Page == EPage::Races ? EVortexStageMode::Races
            : Page == EPage::Heroes ? EVortexStageMode::Heroes : Page == EPage::Lobby ? EVortexStageMode::Lobby : EVortexStageMode::Home);
        if (Page != EPage::Races && Page != EPage::Lobby) S->ShowLook(Look);
    }
}

void SVortexFrontEnd::Tick(const FGeometry& Geometry, const double CurrentTime, const float DeltaTime)
{
    SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
    Now = CurrentTime;
    // sala da Epic: o que está acontecendo aparece embaixo ("Procurando a sala...")
    if (const UVortexOnline* Online = Owner.IsValid() ? Owner->GetSubsystem<UVortexOnline>() : nullptr)
    {
        if (Online->IsBusy() && Online->GetStatus() != LastOnlineStatus) SetStatus(Online->GetStatus());
        LastOnlineStatus = Online->GetStatus();
    }
    // abertura: a cortina escura se abre quando o palco começa a rodar, e a tela entra de novo
    if (!bStageReady && Stage() && Stage()->IsReady())
    {
        bStageReady = true;
        ReadyAt = Now;
        PageAt = Now;
    }
    // cada tela entra subindo e aparecendo
    const float T = Ease(float(Now - PageAt) / .4f);
    if (PageHost.IsValid())
    {
        PageHost->SetRenderOpacity(T);
        PageHost->SetRenderTransform(FSlateRenderTransform(FVector2f(0.f, 26.f * (1.f - T))));
    }
    // raça escolhida: depois da revelação vai para a criação
    if (Page == EPage::Races && ChosenRace != INDEX_NONE && Now - ChosenAt > 2.3)
    {
        Look.Race = FVortexLookCatalog::Get().Races[ChosenRace].Id;
        Look = FVortexLookCatalog::Get().Normalized(Look);
        ChosenRace = INDEX_NONE;
        Go(EPage::Creator);
    }
    // herói criado: depois da comemoração, a lista de heróis com ele selecionado
    if (Page == EPage::Creator && CelebrateAt > 0.0 && Now - CelebrateAt > 3.2)
    {
        CelebrateAt = -100.0;
        // 02/11: mantém a sala online. Antes, criar um herói no mundo multiplayer refazia a lista sem ela, e a sala do
        // anfitrião abria na rede local (código com L): os amigos de outra casa ficavam presos tentando entrar
        OpenHeroes(SelectedWorldId, bSelectedWorldHost, bSelectedWorldOnline);
        SelectHero(CreatedHeroId);
    }
    if (Page == EPage::Lobby) TickLobby();
#if !UE_BUILD_SHIPPING
    TickShot();
#endif
}

TSharedRef<SWidget> SVortexFrontEnd::Button(const FString& Text, FSimpleDelegate Action, int32 Kind, float Width, const FSlateBrush* Icon)
{
    // botões estreitos usam letra menor para o texto caber
    const int32 Size = Width > 0.f && Width < 200.f ? 14 : Width == 0.f ? 15 : 17;
    return SNew(SVortexButton).Label(FText::FromString(Text)).Kind(Kind).Width(Width).FontSize(Size).Menu(this).OnPressed(Action).Icon(Icon);
}

TSharedRef<SWidget> SVortexFrontEnd::TextField(TSharedPtr<SEditableTextBox>& Out, const FString& Hint, const FString& Initial)
{
    return SAssignNew(Out, SEditableTextBox).Style(&FieldStyle()).Font(Font(18)).HintText(FText::FromString(Hint)).Text(FText::FromString(Initial));
}

TSharedRef<SWidget> SVortexFrontEnd::GlassPanel(const FString& Title, const FString& Subtitle, TSharedRef<SWidget> Body, float Width)
{
    return SNew(SBox).WidthOverride(Width)
    [
        SNew(SOverlay)
        + SOverlay::Slot()[SNew(SBorder).BorderImage(VortexUiKit::Window())]
        + SOverlay::Slot().Padding(FMargin(40.f, 34.f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[Text(Upper(Title), 30, Cream, TEXT("Black"), 120)]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 4.f)
            [SNew(SBox).WidthOverride(90.f).HeightOverride(3.f).HAlign(HAlign_Left)[SNew(SImage).Image(Round(2.f)).ColorAndOpacity(Gold)]]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 22.f)[Text(Subtitle, 15, Muted, TEXT("Regular"), 0, true)]
            + SVerticalBox::Slot().AutoHeight()[Body]
        ]
    ];
}

// ---- início ---------------------------------------------------------------------------------

const FSlateBrush* SVortexFrontEnd::Portrait(const FGuid& PlayerId)
{
    if (FSlateBrush* Found = PortraitBrushes.Find(PlayerId)) return Found->GetResourceObject() ? Found : nullptr;
    FSlateBrush& Brush = PortraitBrushes.Add(PlayerId);
    if (UTexture2D* Texture = VortexPortrait::Load(PlayerId))
    {
        if (Owner.IsValid()) Owner->KeepForMenu(Texture);
        Brush.SetResourceObject(Texture);
        Brush.ImageSize = FVector2D(72.f, 72.f);
        Brush.DrawAs = ESlateBrushDrawType::Image;
        return &Brush;
    }
    return nullptr;
}

TSharedRef<SWidget> SVortexFrontEnd::PageHeader(const FString& Title, const FString& Subtitle, TFunction<void()> Back)
{
    PageBack = Back;
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
        [
            SNew(SButton)
            .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
            .ContentPadding(FMargin(0.f, 4.f))
            .OnClicked_Lambda([this, Back]() { Sound(TEXT("ui_close"), .8f); if (Back) Back(); return FReply::Handled(); })
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SImage).Image(&IconBack).DesiredSizeOverride(FVector2D(36.f, 36.f))]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 3.f, 0.f, 0.f)
                [SNew(STextBlock).Text(FText::FromString(TEXT("VOLTAR"))).Font(Display(14, 700, 180)).ColorAndOpacity(Cream)
                    .ShadowOffset(FVector2D(0.f, 2.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .85f))]
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
        [
            SNew(STextBlock).Text(FText::FromString(Upper(Title))).Font(Display(46, 900, 40)).ColorAndOpacity(Cream)
            .ShadowOffset(FVector2D(0.f, 3.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .75f))
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(2.f, 4.f, 0.f, 0.f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(46.f).HeightOverride(3.f)[SNew(SImage).Image(Round(2.f)).ColorAndOpacity(Emerald)]]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f)[SNew(SBox).WidthOverride(46.f).HeightOverride(3.f)[SNew(SImage).Image(Round(2.f)).ColorAndOpacity(GoldDeep)]]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(2.f, 12.f, 0.f, 0.f)
        [
            SNew(SBox).MaxDesiredWidth(760.f)
            [
                // sombra forte: o palco atrás pode ser claro (grama, céu)
                SNew(STextBlock).Text(FText::FromString(Subtitle)).Font(Font(16)).AutoWrapText(true)
                .ColorAndOpacity(FLinearColor(Cream.R, Cream.G, Cream.B, .88f))
                .ShadowOffset(FVector2D(0.f, 2.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .9f))
            ]
        ];
}

void SVortexFrontEnd::OpenSingle(FGuid WorldId)
{
    // mundo de um jogador: o herói dele (ou a criação, se ainda não tem) e o mundo abre direto, sem sala
    OpenHeroes(WorldId, false);
}

void SVortexFrontEnd::ContinueWith(FGuid WorldId, FGuid PlayerId)
{
    // o mesmo caminho do "Jogar" com aquele herói: o mundo de um jogador abre direto; o de multiplayer abre
    // a sala (online quando dá)
    bool bMulti = false;
    if (Owner.IsValid()) for (const FVortexWorldSummary& World : Owner->GetWorlds()) if (World.WorldId == WorldId) bMulti = World.bMultiplayer;
    if (bMulti) OpenOnline(WorldId);
    else OpenSingle(WorldId);
    if (PlayerId.IsValid()) SelectHero(PlayerId);
    if (SelectedHeroId.IsValid()) PlayHero();
}

TSharedRef<SWidget> SVortexFrontEnd::BuildHome()
{
    // Jogar (mundos de um jogador, 1 herói cada), Multiplayer (mundos com sala para amigos, até 3 heróis, ou
    // entrar na sala de um amigo), Configurações, Conquistas e Sair. "Continuar" leva direto ao último mundo
    // de um jogador
    TSharedRef<SVerticalBox> Entries = SNew(SVerticalBox);
    int32 Index = 0;
    auto Add = [&](const FString& Title, const FString& Sub, const FSlateBrush* Icon, bool bPrimary, FSimpleDelegate Action)
    {
        Entries->AddSlot().AutoHeight().Padding(0.f, 6.f).HAlign(HAlign_Left)
        [
            SNew(SVortexMenuEntry).Title(Title).Subtitle(Sub).Icon(Icon).bPrimary(bPrimary).Index(Index++).Menu(this).OnPressed(Action)
        ];
    };
    Add(TEXT("Jogar"), TEXT("Sozinho: até 5 mundos, um herói em cada."), &IconPlay, true,
        FSimpleDelegate::CreateLambda([this]() { bCreatingWorld = false; bMultiplayerMode = false; Go(EPage::Worlds); }));
    Add(TEXT("Multiplayer"), TEXT("Abra uma sala para os amigos ou entre na de um amigo."), &IconJoin, false,
        FSimpleDelegate::CreateLambda([this]() { Go(EPage::Multiplayer); }));
    // 07/11: o modo de teste: todas as armas e armaduras enfileiradas, com o herói do último mundo (não grava nada).
    // 08/11: só no editor (é uma ferramenta do criador): o jogo empacotado e o instalador não mostram
#if WITH_EDITOR
    Add(TEXT("Arsenal"), TEXT("Teste todas as armas, arcos e armaduras num mapa leve. Não mexe no save."), &IconTrophy, false,
        FSimpleDelegate::CreateLambda([this]()
        {
            const TArray<FVortexWorldSummary> Last = Owner.IsValid() ? Owner->GetWorlds(false) : TArray<FVortexWorldSummary>();
            const TArray<FVortexHeroSummary> With = Owner.IsValid() && Last.Num() ? Owner->GetHeroes(Last[0].WorldId) : TArray<FVortexHeroSummary>();
            Sound(TEXT("ui_open"));
            if (Owner.IsValid()) Owner->OpenArsenal(With.Num() ? With[0].PlayerId : FGuid());
        }));
#endif
    Add(TEXT("Configurações"), TEXT("Tela, gráficos, brilho, áudio e controles."), &IconOptions, false,
        FSimpleDelegate::CreateLambda([this]() { Go(EPage::Options); }));
    Add(TEXT("Conquistas"), TEXT("O que os seus heróis já fizeram."), &IconTrophy, false,
        FSimpleDelegate::CreateLambda([this]() { Go(EPage::Achievements); }));
    Add(TEXT("Sair"), TEXT("Fechar o jogo."), &IconQuit, false,
        FSimpleDelegate::CreateLambda([]() { FPlatformMisc::RequestExit(false); }));

    // cartão "Continuar": o último mundo e o primeiro herói dele, com o retrato
    TSharedRef<SWidget> Continue = SNullWidget::NullWidget;
    const TArray<FVortexWorldSummary> Worlds = Owner.IsValid() ? Owner->GetWorlds(false) : TArray<FVortexWorldSummary>();
    if (Worlds.Num())
    {
        const FVortexWorldSummary Last = Worlds[0];
        const TArray<FVortexHeroSummary> LastHeroes = Owner.IsValid() ? Owner->GetHeroes(Last.WorldId) : TArray<FVortexHeroSummary>();
        const FGuid HeroId = LastHeroes.Num() ? LastHeroes[0].PlayerId : FGuid();
        const FString HeroLine = LastHeroes.Num() ? FString::Printf(TEXT("%s  •  %s"), *LastHeroes[0].HeroName, *RaceName(LastHeroes[0].RaceId)) : FString(TEXT("Escolha um herói"));
        const FSlateBrush* Face = HeroId.IsValid() ? Portrait(HeroId) : nullptr;
        if (!Face && LastHeroes.Num()) Face = RaceArt.Find(LastHeroes[0].RaceId);
        Continue = SNew(SButton)
            .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
            .ContentPadding(0.f)
            .OnClicked_Lambda([this, Last, HeroId]() { Sound(TEXT("ui_open")); ContinueWith(Last.WorldId, HeroId); return FReply::Handled(); })
            [
                SNew(SBox).WidthOverride(560.f).HeightOverride(96.f)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()[SNew(SBorder).BorderImage(VortexUiKit::Panel())]
                    + SOverlay::Slot().Padding(18.f, 0.f)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [
                            SNew(SBox).WidthOverride(66.f).HeightOverride(66.f)
                            [
                                SNew(SOverlay)
                                + SOverlay::Slot()[SNew(SImage).Image(Round(10.f)).ColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .45f))]
                                + SOverlay::Slot().Padding(3.f)[SNew(SImage).Image(Face ? Face : &WorldArt)]
                                + SOverlay::Slot()[SNew(SImage).Image(Outline(10.f, 1.5f)).ColorAndOpacity(Gold)]
                            ]
                        ]
                        + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(16.f, 0.f, 0.f, 0.f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("CONTINUAR"))).Font(Display(12, 700, 260)).ColorAndOpacity(Emerald)]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)[SNew(STextBlock).Text(FText::FromString(Last.DisplayName)).Font(Display(22, 700, 20)).ColorAndOpacity(Cream)]
                            + SVerticalBox::Slot().AutoHeight()[Text(HeroLine + FString::Printf(TEXT("  •  %s"), *Last.LastModified.ToString(TEXT("%d/%m %H:%M"))), 13, Muted)]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [SNew(STextBlock).Text(FText::FromString(TEXT("›"))).Font(Font(36, TEXT("Bold"))).ColorAndOpacity(Gold)]
                    ]
                ]
            ];
    }

    // dicas que se revezam embaixo
    static const TCHAR* Tips[] = {
        TEXT("Na sala, o código fica no alto: é só passar para os amigos (até 8 jogadores)."),
        TEXT("Seu amigo entra sem herói e cria o dele já dentro da sala."),
        TEXT("Na criação, arraste o herói para girar e use a roda do mouse para ver o rosto."),
        TEXT("Achou a imagem escura? Opções › Gráficos › Brilho."),
        TEXT("Sem a Epic, a sala abre na rede local: amigos na mesma rede entram com o código."),
        TEXT("Sozinho: até 5 mundos com um herói cada. Multiplayer: outros 5 mundos, até 3 heróis em cada."),
    };
    return SNew(SOverlay)
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(70.f, 40.f, 0.f, 0.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
            [
                // o logo voxel (SourceArt/Menu/MenuArt.blend) respira devagar; sem ele, o título em texto
                SNew(SImage).Image(&LogoBrush).Visibility(LogoBrush.GetResourceObject() ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
                .RenderTransformPivot(FVector2D(.5f, .5f))
                .RenderTransform_Lambda([this]() { return FSlateRenderTransform(FScale2f(1.f + .008f * FMath::Sin(float(Now) * 1.1f)), FVector2f(0.f, 3.f * FMath::Sin(float(Now) * .9f))); })
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(30.f, -6.f, 0.f, 0.f)
            [
                SNew(STextBlock).Text(FText::FromString(TEXT("A LENDA DO VALE"))).Font(Display(17, 700, 620))
                .ShadowOffset(FVector2D(0.f, 2.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .7f))
                .ColorAndOpacity_Lambda([this]() { return FSlateColor(FMath::Lerp(GoldDeep, Gold, .5f + .5f * FMath::Sin(float(Now) * 1.7f))); })
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(10.f, 34.f, 0.f, 0.f)[Entries]
            + SVerticalBox::Slot().AutoHeight().Padding(10.f, 22.f, 0.f, 0.f)[Continue]
        ]
        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 44.f, 30.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
                [SNew(SBox).WidthOverride(10.f).HeightOverride(10.f)[SNew(SImage).Image(Round(2.f)).ColorAndOpacity(Emerald)]]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [
                    SNew(STextBlock).Font(Font(14)).ColorAndOpacity(Cream)
                    .Text_Lambda([this]() { return FText::FromString(Tips[int32(Now / 7.0) % UE_ARRAY_COUNT(Tips)]); })
                    .ColorAndOpacity_Lambda([this]()
                    {
                        const float Phase = float(FMath::Fmod(Now, 7.0));
                        const float A = FMath::Min(FMath::Clamp(Phase / .6f, 0.f, 1.f), FMath::Clamp((7.f - Phase) / .6f, 0.f, 1.f));
                        return FSlateColor(FLinearColor(Cream.R, Cream.G, Cream.B, .9f * A));
                    })
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 8.f, 0.f, 0.f)
            [Text(TEXT("Alpha 1.0  •  Unreal Engine 5"), 11, FLinearColor(Muted.R, Muted.G, Muted.B, .6f))]
        ];
}

void SVortexFrontEnd::ContinueLast()
{
    const TArray<FVortexWorldSummary> Worlds = Owner.IsValid() ? Owner->GetWorlds() : TArray<FVortexWorldSummary>();
    if (Worlds.IsEmpty()) { bCreatingWorld = true; Go(EPage::Worlds); return; }
    OpenHeroes(Worlds[0].WorldId, false);
}

// ---- mundos ---------------------------------------------------------------------------------

TSharedRef<SWidget> SVortexFrontEnd::BuildWorlds()
{
    // os mundos como cartas (a ilha voxel de cada um ganha um tom pela semente) e, na última, o
    // "Novo mundo" que abre ali mesmo o nome e a semente
    const TArray<FVortexWorldSummary> Worlds = Owner.IsValid() ? Owner->GetWorlds(bMultiplayerMode) : TArray<FVortexWorldSummary>();
    const bool bFull = Worlds.Num() >= UVortexGameInstance::MaxWorlds;
    const float CardW = 286.f, CardH = 452.f;
    TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
    auto Card = [&](TSharedRef<SWidget> Body, const FLinearColor& Edge)
    {
        return SNew(SBox).WidthOverride(CardW).HeightOverride(CardH)
        [
            SNew(SOverlay)
            + SOverlay::Slot()[SNew(SBorder).BorderImage(VortexUiKit::Panel())]
            + SOverlay::Slot()[SNew(SImage).Image(Outline(16.f, 1.f)).ColorAndOpacity(Edge)]
            + SOverlay::Slot().Padding(16.f, 14.f)[Body]
        ];
    };
    for (const FVortexWorldSummary& World : Worlds)
    {
        const TArray<FVortexHeroSummary> WorldHeroes = Owner.IsValid() ? Owner->GetHeroes(World.WorldId) : TArray<FVortexHeroSummary>();
        FString HeroNames;
        for (const FVortexHeroSummary& Hero : WorldHeroes) HeroNames += (HeroNames.IsEmpty() ? TEXT("") : TEXT(", ")) + Hero.HeroName;
        // tom da ilha pela semente: cada mundo tem uma cor própria, sem perder o verde
        const float Hue = float((uint32(World.Seed) * 2654435761u) % 360u);
        const FLinearColor Tint = FMath::Lerp(FLinearColor::White, FLinearColor(Hue, .55f, 1.f).HSVToLinearRGB(), .22f);
        Row->AddSlot().AutoWidth().Padding(0.f, 0.f, 18.f, 0.f)
        [
            Card(
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                [
                    SNew(SBox).WidthOverride(250.f).HeightOverride(168.f)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()[SNew(SImage).Image(Round(12.f)).ColorAndOpacity(FLinearColor(.02f, .05f, .09f, .85f))]
                        + SOverlay::Slot().Padding(4.f)[SNew(SImage).Image(&WorldArt).ColorAndOpacity(Tint)]
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(2.f, 14.f, 0.f, 0.f)
                [SNew(STextBlock).Text(FText::FromString(World.DisplayName)).Font(Display(20, 700, 10)).ColorAndOpacity(Cream).AutoWrapText(true)]
                + SVerticalBox::Slot().AutoHeight().Padding(2.f, 4.f, 0.f, 0.f)
                [Text(FString::Printf(TEXT("Jogado em %s\nSemente %d"), *World.LastModified.ToString(TEXT("%d/%m %H:%M")), World.Seed), 12, Muted, TEXT("Regular"), 0, true)]
                + SVerticalBox::Slot().FillHeight(1.f).Padding(2.f, 8.f, 0.f, 0.f)
                [
                    Text(WorldHeroes.IsEmpty() ? FString(TEXT("Nenhum herói ainda"))
                        : FString::Printf(TEXT("%d %s: %s"), WorldHeroes.Num(), WorldHeroes.Num() == 1 ? TEXT("herói") : TEXT("heróis"), *HeroNames), 13, Gold, TEXT("Regular"), 0, true)
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                [Button(bMultiplayerMode ? TEXT("Abrir sala") : TEXT("Jogar"), bMultiplayerMode ? FSimpleDelegate::CreateSP(this, &SVortexFrontEnd::OpenOnline, World.WorldId)
                    : FSimpleDelegate::CreateSP(this, &SVortexFrontEnd::OpenSingle, World.WorldId), 1, 250.f)]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f, 0.f, 0.f)
                [
                    SNew(SButton)
                    .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
                    .OnClicked_Lambda([this, Id = World.WorldId, Name = World.DisplayName]()
                    {
                        PendingWorldDelete = Id;
                        PendingWorldName = Name;
                        Go(EPage::DeleteWorld);
                        return FReply::Handled();
                    })
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)[SNew(SImage).Image(&IconDelete).DesiredSizeOverride(FVector2D(24.f, 24.f))]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [SNew(STextBlock).Text(FText::FromString(TEXT("EXCLUIR MUNDO"))).Font(Display(11, 700, 160)).ColorAndOpacity(FLinearColor(Bad.R, Bad.G, Bad.B, .8f))]
                    ]
                ],
                FLinearColor(GoldDeep.R, GoldDeep.G, GoldDeep.B, .25f))
        ];
    }
    // a carta do mundo novo
    TSharedRef<SWidget> NewBody = SNullWidget::NullWidget;
    if (bFull)
    {
        NewBody = SNew(SVerticalBox)
            + SVerticalBox::Slot().FillHeight(1.f).VAlign(VAlign_Center).HAlign(HAlign_Center)
            [Text(FString::Printf(TEXT("Limite de %d mundos.\nExclua um para criar outro."), UVortexGameInstance::MaxWorlds), 15, Muted, TEXT("Regular"), 0, true)];
    }
    else if (!bCreatingWorld)
    {
        NewBody = SNew(SButton)
            .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
            .OnClicked_Lambda([this]() { Sound(TEXT("ui_open")); bCreatingWorld = true; Go(EPage::Worlds); return FReply::Handled(); })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().FillHeight(1.f).VAlign(VAlign_Center).HAlign(HAlign_Center)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                    [SNew(SImage).Image(&IconNewWorld).DesiredSizeOverride(FVector2D(150.f, 150.f))]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("NOVO MUNDO"))).Font(Display(18, 900, 160)).ColorAndOpacity(Cream)]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f)[Text(FString::Printf(TEXT("%d de %d"), Worlds.Num(), UVortexGameInstance::MaxWorlds), 12, Muted)]
                ]
            ];
    }
    else
    {
        NewBody = SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("NOVO MUNDO"))).Font(Display(20, 900, 120)).ColorAndOpacity(Emerald)]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 10.f)[Text(TEXT("A semente decide como o continente vai nascer."), 13, Muted, TEXT("Regular"), 0, true)]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[SNew(STextBlock).Text(FText::FromString(TEXT("NOME"))).Font(Display(11, 700, 200)).ColorAndOpacity(Gold)]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[TextField(NameBox, TEXT("Vale de Aurora"))]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[SNew(STextBlock).Text(FText::FromString(TEXT("SEMENTE (OPCIONAL)"))).Font(Display(11, 700, 200)).ColorAndOpacity(Gold)]
            + SVerticalBox::Slot().AutoHeight()[TextField(SeedBox, TEXT("Aleatória"))]
            + SVerticalBox::Slot().FillHeight(1.f)[SNullWidget::NullWidget]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Button(TEXT("Criar mundo"), FSimpleDelegate::CreateSP(this, &SVortexFrontEnd::CreateWorld), 1, 250.f, &IconNewWorld)]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f, 0.f, 0.f)
            [Button(TEXT("Cancelar"), FSimpleDelegate::CreateLambda([this]() { bCreatingWorld = false; Go(EPage::Worlds); }), 2, 250.f, &IconBack)];
    }
    Row->AddSlot().AutoWidth()[Card(NewBody, bCreatingWorld ? Emerald : FLinearColor(Emerald.R, Emerald.G, Emerald.B, .35f))];

    return SNew(SBox).HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(70.f, 46.f, 40.f, 40.f))
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [
            bMultiplayerMode
            ? PageHeader(TEXT("Mundos de multiplayer"),
                Worlds.IsEmpty() ? TEXT("Crie um mundo para jogar com amigos. Ele abre uma sala: os amigos entram com o código dela, criam o herói e marcam pronto.")
                                 : TEXT("Escolha um mundo e o seu herói (até 3 em cada). A sala abre e os amigos entram com o código dela."),
                [this]() { bCreatingWorld = false; Go(EPage::Multiplayer); })
            : PageHeader(TEXT("Seus mundos"),
                Worlds.IsEmpty() ? TEXT("Crie o seu primeiro mundo. Cada mundo de um jogador tem o seu herói.")
                                 : TEXT("Escolha um mundo para continuar a aventura do herói dele."),
                [this]() { bCreatingWorld = false; Go(EPage::Home); })
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 34.f, 0.f, 0.f)
        [
            SNew(SScrollBox).Orientation(Orient_Horizontal).ScrollBarThickness(FVector2D(6.f, 6.f))
            + SScrollBox::Slot()[Row]
        ]
    ];
}

TSharedRef<SWidget> SVortexFrontEnd::BuildCreateWorld()
{
    PageBack = [this]() { bCreatingWorld = false; Go(EPage::Worlds); };
    // o mundo novo agora é criado na própria lista de mundos
    bCreatingWorld = true;
    return BuildWorlds();
}

void SVortexFrontEnd::CreateWorld()
{
    if (!Owner.IsValid() || !NameBox.IsValid() || !SeedBox.IsValid()) return;
    if (Owner->GetWorlds(bMultiplayerMode).Num() >= UVortexGameInstance::MaxWorlds)
    {
        SetStatus(FString::Printf(TEXT("Até %d mundos. Exclua um para criar outro."), UVortexGameInstance::MaxWorlds), true);
        return;
    }
    const FString SeedText = SeedBox->GetText().ToString().TrimStartAndEnd();
    int32 Seed = FMath::Rand();
    if (!SeedText.IsEmpty() && !LexTryParseString(Seed, *SeedText)) { SetStatus(TEXT("A semente precisa ser um número inteiro."), true); return; }
    FString Name = NameBox->GetText().ToString().TrimStartAndEnd();
    if (Name.IsEmpty()) Name = TEXT("Vale de Aurora");
    FString Error;
    FGuid WorldId;
    if (!Owner->CreateWorldOnly(Name, Seed, WorldId, Error, bMultiplayerMode)) SetStatus(Error, true);
    else
    {
        bCreatingWorld = false;
        SetStatus(FString::Printf(TEXT("Mundo \"%s\" criado."), *Name));
        if (bMultiplayerMode) OpenOnline(WorldId);
        else OpenSingle(WorldId);
    }
}

TSharedRef<SWidget> SVortexFrontEnd::BuildJoin()
{
    // um campo grande na letra do jogo (5 letras da sala online, ou o código "L..." da rede local); Enter entra
    TSharedRef<SWidget> Steps = SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[Text(TEXT("1.  Peça o código ao seu amigo: ele aparece no alto da sala dele."), 14, Muted, TEXT("Regular"), 0, true)]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[Text(TEXT("2.  Entre e escolha (ou crie) o seu herói lá dentro."), 14, Muted, TEXT("Regular"), 0, true)]
        + SVerticalBox::Slot().AutoHeight()[Text(TEXT("3.  Marque \"pronto\" e espere ele começar a aventura."), 14, Muted, TEXT("Regular"), 0, true)];
    return SNew(SBox).HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(70.f, 46.f, 40.f, 40.f))
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [PageHeader(TEXT("Entrar na sala"), TEXT("Jogue no mundo de um amigo, onde quer que ele esteja."), [this]() { Go(EPage::Home); })]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 34.f, 0.f, 0.f)
        [
            SNew(SBox).WidthOverride(620.f)
            [
                SNew(SOverlay)
                + SOverlay::Slot()[SNew(SBorder).BorderImage(VortexUiKit::Window())]
                + SOverlay::Slot().Padding(FMargin(40.f, 34.f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                    [SNew(SImage).Image(&IconKey).DesiredSizeOverride(FVector2D(110.f, 110.f))]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 8.f, 0.f, 10.f)
                    [SNew(STextBlock).Text(FText::FromString(TEXT("CÓDIGO DA SALA"))).Font(Display(13, 700, 300)).ColorAndOpacity(Gold)]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SAssignNew(RoomCodeBox, SEditableTextBox).Style(&FieldStyle()).Font(Display(40, 900, 500))
                        .Justification(ETextJustify::Center).HintText(FText::FromString(TEXT("ABCDE")))
                        .OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type How) { if (How == ETextCommit::OnEnter) JoinOnlineRoom(); })
                    ]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 12.f, 0.f, 0.f)
                    [Button(TEXT("Colar código"), FSimpleDelegate::CreateLambda([this]()
                    {
                        FString Pasted, Clean;
                        FPlatformApplicationMisc::ClipboardPaste(Pasted);
                        for (const TCHAR C : Pasted.ToUpper()) if (FChar::IsAlnum(C)) Clean.AppendChar(C);
                        if (RoomCodeBox.IsValid()) RoomCodeBox->SetText(FText::FromString(Clean));
                    }), 2, 220.f)]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 14.f, 0.f, 26.f)
                    [Button(TEXT("Entrar na sala"), FSimpleDelegate::CreateSP(this, &SVortexFrontEnd::JoinOnlineRoom), 1, 340.f, &IconKey)]
                    + SVerticalBox::Slot().AutoHeight()[SNew(SImage).Image(VortexUiKit::Divider(540.f))]
                    + SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)[Steps]
                ]
            ]
        ]
    ];
}

void SVortexFrontEnd::OpenTravellers()
{
    const FGuid Keep = SelectedHeroId;
    OpenHeroes(UVortexGameInstance::TravellerWorld(), false);
    for (const FVortexHeroSummary& Hero : Heroes) if (Hero.PlayerId == Keep) SelectHero(Keep);
}

void SVortexFrontEnd::JoinOnlineRoom()
{
    UVortexOnline* Online = Owner.IsValid() ? Owner->GetSubsystem<UVortexOnline>() : nullptr;
    if (!Online || !RoomCodeBox.IsValid() || Online->IsBusy()) return;
    // o herói é escolhido (ou criado) já dentro da sala. 02/11: começa no último herói viajante usado num mundo de
    // amigo (se ele ainda existe): quem sai e volta para um mundo em andamento entra com ele, e não com um herói novo
    FGuid Last;
    FString LastText;
    if (GConfig->GetString(TEXT("Vortex.Multiplayer"), TEXT("LastTravellerHero"), LastText, GGameUserSettingsIni)) FGuid::Parse(LastText, Last);
    const bool bKnown = Last.IsValid() && Owner->GetHeroes(UVortexGameInstance::TravellerWorld()).ContainsByPredicate([&](const FVortexHeroSummary& H) { return H.PlayerId == Last; });
    Owner->SetSelectedHero(bKnown ? Last : FGuid());
    // sala da rede local: o código traz o endereço
    FString Address;
    if (UVortexGameInstance::AddressFromLanCode(RoomCodeBox->GetText().ToString(), Address))
    {
        FString Error;
        if (!Owner->JoinByAddress(Address, Error)) SetStatus(Error, true);
        else SetStatus(TEXT("Conectando na sala da rede local... (código com L: só funciona no mesmo Wi-Fi)"), false);
        return;
    }
    if (!UVortexOnline::HasCredentials())
    {
        SetStatus(TEXT("Este código é de uma sala da Epic, e faltam as credenciais da Epic neste computador."), true);
        return;
    }
    const FString TravellerName = TEXT("Aventureiro");
    Sound(TEXT("ui_open"));
    TWeakPtr<SVortexFrontEnd> Weak = SharedThis(this);
    Online->JoinRoom(TravellerName, RoomCodeBox->GetText().ToString(), FVortexOnlineDone::CreateLambda([Weak](bool bOk, const FString& Result)
    {
        TSharedPtr<SVortexFrontEnd> Self = Weak.Pin();
        if (!Self) return;
        FString Error;
        if (!bOk) { Self->SetStatus(Result, true); return; }
        // a sala foi achada: o menu fica com o aviso enquanto a conexão com o anfitrião fecha (pode levar alguns segundos)
        Self->SetStatus(TEXT("Sala encontrada! Conectando ao anfitrião... (pode levar até um minuto)"), false);
        if (Self->Owner.IsValid() && !Self->Owner->JoinOnline(Result, Error)) Self->SetStatus(Error, true);
    }));
}

void SVortexFrontEnd::OpenOnline(FGuid WorldId)
{
    // a sala é criada sozinha ao escolher o herói: pela Epic (amigos em qualquer lugar) quando há as
    // credenciais; senão, na rede local (o código esconde o IP)
    OpenHeroes(WorldId, true, UVortexOnline::HasCredentials());
}

void SVortexFrontEnd::Connect()
{
    if (!SelectedHeroId.IsValid()) { OpenTravellers(); SetStatus(TEXT("Escolha ou crie um herói viajante primeiro."), true); return; }
    if (Owner.IsValid()) Owner->SetSelectedHero(SelectedHeroId);
    FString Error;
    if (Owner.IsValid() && AddressBox.IsValid() && !Owner->JoinByAddress(AddressBox->GetText().ToString(), Error)) SetStatus(Error, true);
    else SetStatus(TEXT("Conectando no anfitrião..."), false);
}

// ---- opções ---------------------------------------------------------------------------------

TSharedRef<SWidget> SVortexFrontEnd::BuildOptions()
{
    // o painel em abas (Framework/VortexOptionsPanel), o mesmo do menu de pausa
    TWeakPtr<SVortexFrontEnd> Weak = SharedThis(this);
    return SNew(SBox).HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(70.f, 46.f, 40.f, 30.f))
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [PageHeader(TEXT("Opções"), TEXT("Tela, gráficos, brilho, áudio, controles e interface. Cada opção diz o que muda e quanto pesa."), [this]() { Go(EPage::Home); })]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 26.f, 0.f, 0.f)
        [
            SNew(SBox).WidthOverride(1000.f)
            [
                SNew(SOverlay)
                + SOverlay::Slot()[SNew(SBorder).BorderImage(VortexUiKit::Window())]
                + SOverlay::Slot().Padding(FMargin(30.f, 26.f))
                [MakeVortexOptionsPanel([Weak]() { if (TSharedPtr<SVortexFrontEnd> Self = Weak.Pin()) Self->Sound(TEXT("ui_click"), .7f); }, 520.f, OptionsTab)]
            ]
        ]
    ];
}

// ---- heróis do mundo ------------------------------------------------------------------------

void SVortexFrontEnd::OpenHeroes(FGuid WorldId, bool bHost, bool bOnline)
{
    SelectedWorldId = WorldId;
    bSelectedWorldHost = bHost;
    bSelectedWorldOnline = bOnline;
    bTravellerMode = WorldId == UVortexGameInstance::TravellerWorld();
    Heroes = Owner.IsValid() ? Owner->GetHeroes(WorldId) : TArray<FVortexHeroSummary>();
    SelectedWorldName.Empty();
    bSelectedWorldMulti = bTravellerMode;
    if (Owner.IsValid())
    {
        for (const FVortexWorldSummary& World : Owner->GetWorlds())
            if (World.WorldId == WorldId) { SelectedWorldName = World.DisplayName; bSelectedWorldMulti = World.bMultiplayer; }
    }
    SelectedHeroId.Invalidate();
    if (Heroes.Num())
    {
        SelectHero(Heroes[0].PlayerId);
        return;
    }
    Go(EPage::Heroes);
}

void SVortexFrontEnd::SelectHero(FGuid PlayerId)
{
    SelectedHeroId = PlayerId;
    FVortexLook Saved;
    if (Owner.IsValid() && Owner->GetHeroLook(PlayerId, Saved)) Look = Saved;
    if (Page != EPage::Heroes) Go(EPage::Heroes);
    else if (AVortexMenuStage* S = Stage()) { S->ShowLook(Look); S->Pulse(); }
}

TSharedRef<SWidget> SVortexFrontEnd::BuildHeroes()
{
    TSharedRef<SHorizontalBox> Cards = SNew(SHorizontalBox);
    // um herói no mundo de um jogador; três no de multiplayer e três viajantes
    const int32 Slots = Owner.IsValid() ? Owner->MaxHeroes(SelectedWorldId) : 3;
    for (int32 Slot = 0; Slot < Slots; ++Slot)
    {
        TSharedRef<SWidget> Card = SNullWidget::NullWidget;
        if (Heroes.IsValidIndex(Slot))
        {
            const FVortexHeroSummary Hero = Heroes[Slot];
            const FVortexRaceInfo* Race = FVortexLookCatalog::Get().Race(Hero.RaceId);
            const FLinearColor Accent = Race ? Race->Accent : Gold;
            const FSlateBrush* Art = RaceArt.Find(Hero.RaceId);
            Card = SNew(SButton)
                .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
                .ContentPadding(0.f)
                .OnClicked_Lambda([this, Hero]() { Sound(TEXT("ui_click"), .8f); SelectHero(Hero.PlayerId); return FReply::Handled(); })
                [
                    SNew(SBox).WidthOverride(230.f).HeightOverride(300.f)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()[SNew(SBorder).BorderImage(VortexUiKit::Panel())]
                        + SOverlay::Slot().VAlign(VAlign_Top)
                        [SNew(SBox).HeightOverride(190.f).Clipping(EWidgetClipping::ClipToBounds)[SNew(SImage).Image(Art)]]
                        + SOverlay::Slot().VAlign(VAlign_Bottom).Padding(18.f, 0.f, 18.f, 18.f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[Text(Hero.HeroName, 22, Cream, TEXT("Bold"))]
                            + SVerticalBox::Slot().AutoHeight()[Text(Upper(Race ? Race->Name : FString()), 11, Accent, TEXT("Bold"), 220)]
                        ]
                        + SOverlay::Slot()
                        [
                            SNew(SBorder).BorderImage(Outline(14.f, 2.f)).BorderBackgroundColor_Lambda([this, Hero, Accent]()
                            {
                                const bool bOn = Hero.PlayerId == SelectedHeroId;
                                const float Pulse = .8f + .2f * FMath::Sin(float(Now) * 4.f);
                                return bOn ? Accent * Pulse : FLinearColor(GoldDeep.R, GoldDeep.G, GoldDeep.B, .35f);
                            })
                        ]
                    ]
                ];
        }
        else
        {
            Card = SNew(SButton)
                .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
                .ContentPadding(0.f)
                .OnClicked_Lambda([this]() { Sound(TEXT("ui_open")); StartCreation(); return FReply::Handled(); })
                [
                    SNew(SBox).WidthOverride(230.f).HeightOverride(300.f)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()[SNew(SBorder).BorderImage(VortexUiKit::Panel())]
                        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 8.f)[SNew(SImage).Image(&IconNewHero).DesiredSizeOverride(FVector2D(120.f, 120.f))]
                            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Text(TEXT("NOVO HERÓI"), 13, Gold, TEXT("Bold"), 260)]
                        ]
                    ]
                ];
        }
        Cards->AddSlot().AutoWidth().Padding(0.f, 0.f, 16.f, 0.f)[Card];
    }
    // dentro da sala de um amigo: escolher aqui só troca o herói e volta para a sala
    const bool bInLobby = Owner.IsValid() && Owner->InLobby();
    const FString Sub = bTravellerMode ? FString(TEXT("Seus heróis para jogar no mundo de um amigo"))
        : SelectedWorldName.IsEmpty() ? FString() : FString::Printf(TEXT("Mundo: %s%s"), *SelectedWorldName,
        bSelectedWorldOnline ? TEXT("  •  sala online, até 8 jogadores") : bSelectedWorldHost ? TEXT("  •  sala na rede local, até 8 jogadores") : TEXT(""));
    const FString PlayText = bInLobby ? TEXT("Usar este herói") : bTravellerMode ? TEXT("Escolher e conectar") : bSelectedWorldHost ? TEXT("Jogar com este herói") : TEXT("Entrar no mundo");
    return SNew(SOverlay)
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(70.f, 46.f, 0.f, 0.f)
        [
            PageHeader(bInLobby || bTravellerMode ? TEXT("Seu herói") : TEXT("Seus heróis"), Sub.IsEmpty() ? FString(TEXT("Escolha quem vai para a aventura, ou crie um novo.")) : Sub,
                [this, bInLobby]() { Go(bInLobby ? EPage::Lobby : bTravellerMode ? EPage::Home : EPage::Worlds); })
        ]
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(80.f, 0.f, 0.f, 70.f)[Cards]
        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 80.f, 70.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
            [
                SNew(SBox).Visibility_Lambda([this]() { return SelectedHeroId.IsValid() ? EVisibility::Visible : EVisibility::Collapsed; })
                [Button(PlayText, FSimpleDelegate::CreateSP(this, &SVortexFrontEnd::PlayHero), 1, 320.f)]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
            [
                SNew(SBox).Visibility_Lambda([this]() { return SelectedHeroId.IsValid() ? EVisibility::Visible : EVisibility::Collapsed; })
                [Button(TEXT("Excluir herói"), FSimpleDelegate::CreateLambda([this]() { PendingDelete = SelectedHeroId; Go(EPage::DeleteHero); }), 3, 320.f, &IconDelete)]
            ]
        ];
}

namespace
{
    /** 10/11: o herói selecionado foi salvo num formato antigo (antes dos 8 atributos): a página de excluir mostra o aviso. */
    bool GOutdatedHero = false;
}

void SVortexFrontEnd::PlayHero()
{
    FString Error;
    Sound(TEXT("ui_open"));
    // 10/11: save de outra versão (os pontos dos 12 atributos antigos): avisa e pede para apagar
    if (UVortexSaveSubsystem* Saves = Owner.IsValid() ? Owner->GetSubsystem<UVortexSaveSubsystem>() : nullptr)
        if (const UVortexPlayerSave* Save = Saves->LoadLocalPlayer(SelectedHeroId); Save && Save->bHasSheet && Save->SaveSchemaVersion < UVortexPlayerSave::CurrentSchemaVersion)
        {
            GOutdatedHero = true;
            PendingDelete = SelectedHeroId;
            Go(EPage::DeleteHero);
            return;
        }
    // na sala de um amigo: este é o herói que vai junto (a sala fica sabendo sozinha) e volta para ela
    if (Owner.IsValid() && Owner->InLobby())
    {
        Owner->SetSelectedHero(SelectedHeroId);
        Go(EPage::Lobby);
        return;
    }
    // viajante: escolhido, agora o código da sala ou o IP do amigo
    if (bTravellerMode)
    {
        if (Owner.IsValid()) Owner->SetSelectedHero(SelectedHeroId);
        Go(EPage::Join);
        return;
    }
    UVortexOnline* Online = Owner.IsValid() ? Owner->GetSubsystem<UVortexOnline>() : nullptr;
    if (bSelectedWorldOnline && Online)
    {
        // primeiro a sala da Epic (com o nome do herói), depois o mundo abre escutando por ela
        if (Online->IsBusy()) return;
        FString RoomName = TEXT("Aventureiro");
        for (const FVortexHeroSummary& Hero : Heroes) if (Hero.PlayerId == SelectedHeroId) RoomName = Hero.HeroName;
        TWeakPtr<SVortexFrontEnd> Weak = SharedThis(this);
        Online->HostRoom(RoomName, FVortexOnlineDone::CreateLambda([Weak](bool bOk, const FString& Result)
        {
            TSharedPtr<SVortexFrontEnd> Self = Weak.Pin();
            if (!Self) return;
            FString OpenError;
            if (!bOk)
            {
                // sem a Epic agora (internet, serviço): a sala abre na rede local, também com código
                if (Self->Owner.IsValid() && Self->Owner->OpenLobby(Self->SelectedWorldId, Self->SelectedHeroId, false, OpenError))
                {
                    Self->Owner->SetMenuNotice(TEXT("A Epic não respondeu: a sala abriu na rede local (só amigos na mesma rede)."));
                }
                else Self->SetStatus(Result, true);
            }
            else if (Self->Owner.IsValid() && !Self->Owner->OpenLobby(Self->SelectedWorldId, Self->SelectedHeroId, true, OpenError)) Self->SetStatus(OpenError, true);
        }));
        return;
    }
    // rede local: primeiro a sala de espera, depois todos vão juntos ao mundo
    if (bSelectedWorldHost)
    {
        if (Owner.IsValid() && !Owner->OpenLobby(SelectedWorldId, SelectedHeroId, false, Error)) SetStatus(Error, true);
        return;
    }
    if (Owner.IsValid() && !Owner->OpenWorldWithHero(SelectedWorldId, SelectedHeroId, bSelectedWorldHost, Error)) SetStatus(Error, true);
}

TSharedRef<SWidget> SVortexFrontEnd::BuildDeleteHero()
{
    PageBack = [this]() { GOutdatedHero = false; Go(EPage::Heroes); };
    FString Name;
    for (const FVortexHeroSummary& Hero : Heroes) if (Hero.PlayerId == PendingDelete) Name = Hero.HeroName;
    const FString Title = GOutdatedHero ? FString::Printf(TEXT("%s é de uma versão antiga"), *Name) : FString::Printf(TEXT("Excluir %s?"), *Name);
    const FString Text = GOutdatedHero
        ? FString(TEXT("Este save é de uma versão anterior do jogo: os atributos mudaram (agora são 8) e ele não pode ser usado. Apagar este herói para criar um novo? Não dá para desfazer."))
        : FString(TEXT("O save deste herói será apagado deste computador. Não dá para desfazer."));
    return SNew(SBox).HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(FMargin(110.f, 0.f, 0.f, 0.f))
    [
        GlassPanel(Title, Text,
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()[Button(GOutdatedHero ? TEXT("Sim, apagar") : TEXT("Sim, excluir"), FSimpleDelegate::CreateSP(this, &SVortexFrontEnd::ConfirmDeleteHero), 3, 220.f, &IconDelete)]
            + SHorizontalBox::Slot().AutoWidth().Padding(12.f, 0.f)[Button(GOutdatedHero ? TEXT("Agora não") : TEXT("Cancelar"), FSimpleDelegate::CreateLambda([this]() { GOutdatedHero = false; Go(EPage::Heroes); }), 1, 200.f)])
    ];
}

TSharedRef<SWidget> SVortexFrontEnd::BuildDeleteWorld()
{
    PageBack = [this]() { PendingWorldDelete.Invalidate(); Go(EPage::Worlds); };
    const int32 HeroCount = Owner.IsValid() ? Owner->GetHeroes(PendingWorldDelete).Num() : 0;
    return SNew(SBox).HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(FMargin(110.f, 0.f, 0.f, 0.f))
    [
        GlassPanel(FString::Printf(TEXT("Excluir o mundo \"%s\"?"), *PendingWorldName),
            HeroCount > 0
                ? FString::Printf(TEXT("O mundo e %s (%d) serão apagados deste computador. Não dá para desfazer."), HeroCount == 1 ? TEXT("o herói dele") : TEXT("os heróis dele"), HeroCount)
                : FString(TEXT("O mundo será apagado deste computador. Não dá para desfazer.")),
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("Sim, excluir"), FSimpleDelegate::CreateSP(this, &SVortexFrontEnd::ConfirmDeleteWorld), 3, 220.f, &IconDelete)]
            + SHorizontalBox::Slot().AutoWidth().Padding(12.f, 0.f)[Button(TEXT("Cancelar"), FSimpleDelegate::CreateLambda([this]() { PendingWorldDelete.Invalidate(); Go(EPage::Worlds); }), 2, 180.f)])
    ];
}

void SVortexFrontEnd::ConfirmDeleteWorld()
{
    FString Error;
    const bool bDeleted = Owner.IsValid() && Owner->DeleteWorld(PendingWorldDelete, Error);
    const FString Name = PendingWorldName;
    PendingWorldDelete.Invalidate();
    Go(EPage::Worlds);
    if (bDeleted) SetStatus(FString::Printf(TEXT("Mundo \"%s\" excluído."), *Name));
    else SetStatus(Error, true);
}

void SVortexFrontEnd::ConfirmDeleteHero()
{
    GOutdatedHero = false;
    FString Error;
    const bool bDeleted = Owner.IsValid() && Owner->DeleteHero(SelectedWorldId, PendingDelete, Error);
    PendingDelete.Invalidate();
    OpenHeroes(SelectedWorldId, bSelectedWorldHost, bSelectedWorldOnline);
    if (!bDeleted) SetStatus(Error, true);
    else SetStatus(TEXT("Herói excluído."));
}

// ---- seleção de raça ------------------------------------------------------------------------

void SVortexFrontEnd::StartCreation()
{
    // vindo da criação ("Trocar raça"), dá para voltar e continuar com o que já estava feito
    bRaceFromCreator = Page == EPage::Creator;
    if (bRaceFromCreator && HeroNameBox.IsValid()) HeroName = HeroNameBox->GetText().ToString();
    if (!bRaceFromCreator) HeroName.Empty();
    ChosenRace = INDEX_NONE;
    Go(EPage::Races);
}

void SVortexFrontEnd::ChooseRace(int32 Index)
{
    if (!FVortexLookCatalog::Get().Races.IsValidIndex(Index) || !IsRaceOpen(FVortexLookCatalog::Get().Races[Index].Id)) return;
    ChosenRace = Index;
    ChosenAt = Now;
    Sound(TEXT("equip"));
    const FVortexLookCatalog& Catalog = FVortexLookCatalog::Get();
    if (bRaceFromCreator)
    {
        // troca só a raça: as cores e as peças que a nova raça aceita continuam; a pele vai para a da raça
        Look.Race = Catalog.Races[Index].Id;
        Look.Colors.Remove(TEXT("skin_color"));
        Look = Catalog.Normalized(Look);
    }
    else
    {
        Look = Catalog.Random(Catalog.Races[Index].Id, Look.Appearance, Rng);
    }
    if (!bRaceFromCreator || HeroName.IsEmpty()) HeroName = Catalog.RandomName(Look.Race, Rng);
    if (AVortexMenuStage* S = Stage()) S->Pulse();
}

TSharedRef<SWidget> SVortexFrontEnd::BuildRaces()
{
    PageBack = [this]()
    {
            if (ChosenRace != INDEX_NONE) return;
            Go(bRaceFromCreator ? EPage::Creator : EPage::Heroes);
        };
    const FVortexLookCatalog& Catalog = FVortexLookCatalog::Get();
    TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
    for (int32 i = 0; i < Catalog.Races.Num(); ++i)
    {
        Row->AddSlot().AutoWidth().Padding(SVortexRaceCard::Gap * .5f, 0.f)
        [
            SNew(SVortexRaceCard).Index(i).Menu(this).Art(RaceArt.Find(Catalog.Races[i].Id))
            .OnChosen(FSimpleDelegate::CreateSP(this, &SVortexFrontEnd::ChooseRace, i))
        ];
    }
    return SNew(SOverlay)
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 30.f, 0.f, 0.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Text(TEXT("ESCOLHA SUA RAÇA"), 36, Cream, TEXT("Black"), 220)]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f)
            [Text(TEXT("Cada raça tem corpo, forças, fraquezas e peças próprias."), 15, Muted)]
        ]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 96.f)[Row]
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(60.f, 0.f, 0.f, 40.f)
        [Button(bRaceFromCreator ? TEXT("Voltar à criação") : TEXT("Voltar"), FSimpleDelegate::CreateLambda([this]()
        { if (PageBack) PageBack(); }), 2, 220.f)]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            // revelação: o nome da raça cresce no centro, na cor dela
            SNew(SVerticalBox)
            .Visibility_Lambda([this]() { return ChosenRace != INDEX_NONE && Now - ChosenAt > .55 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
            .RenderTransformPivot(FVector2D(.5f, .5f))
            .RenderTransform_Lambda([this]()
            {
                const float T = Ease(float(Now - ChosenAt - .55) / .5f);
                return FSlateRenderTransform(FScale2f(.7f + .3f * T));
            })
            .RenderOpacity(1.f)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(STextBlock).Font(Font(18, TEXT("Bold"), 600)).ShadowOffset(FVector2D(0.f, 3.f)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, .7f))
                .Text(FText::FromString(TEXT("VOCÊ ESCOLHEU")))
                .ColorAndOpacity_Lambda([this]() { return FSlateColor(FLinearColor(Cream.R, Cream.G, Cream.B, Ease(float(Now - ChosenAt - .6) / .4f))); })
            ]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(STextBlock).Font(Font(110, TEXT("Black"), 300)).ShadowOffset(FVector2D(0.f, 5.f)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, .8f))
                .Text_Lambda([this]() { return FText::FromString(ChosenRace != INDEX_NONE ? Upper(FVortexLookCatalog::Get().Races[ChosenRace].Name) : FString()); })
                .ColorAndOpacity_Lambda([this]()
                {
                    FLinearColor C = ChosenRace != INDEX_NONE ? FVortexLookCatalog::Get().Races[ChosenRace].Accent : Gold;
                    C.A = Ease(float(Now - ChosenAt - .55) / .35f);
                    return FSlateColor(C);
                })
            ]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(STextBlock).Font(Font(20, TEXT("Bold"), 420)).ShadowOffset(FVector2D(0.f, 3.f)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, .7f))
                .Text_Lambda([this]() { return FText::FromString(ChosenRace != INDEX_NONE ? Upper(FVortexLookCatalog::Get().Races[ChosenRace].Epithet) : FString()); })
                .ColorAndOpacity_Lambda([this]() { return FSlateColor(FLinearColor(Cream.R, Cream.G, Cream.B, Ease(float(Now - ChosenAt - .9) / .4f))); })
            ]
        ];
}

// ---- criação --------------------------------------------------------------------------------

void SVortexFrontEnd::ShowLookOnStage()
{
    if (AVortexMenuStage* S = Stage()) S->ShowLook(Look);
}

void SVortexFrontEnd::CyclePiece(FName Category, int32 Direction)
{
    const FVortexLookCatalog& Catalog = FVortexLookCatalog::Get();
    const TArray<FName> Options = Catalog.Options(Look, Category);
    if (Options.Num() < 2) return;
    const int32 Current = FMath::Max(0, Options.IndexOfByKey(Look.Pieces.FindRef(Category)));
    Catalog.ChoosePiece(Look, Category, Options[(Current + Direction + Options.Num()) % Options.Num()]);
    Sound(TEXT("equip"), .45f);
    ShowLookOnStage();
}

void SVortexFrontEnd::SetColor(FName Slot, FColor Color)
{
    Look.Colors.Add(Slot, Color);
    if (Slot == TEXT("shirt_color")) Look.Colors.Add(TEXT("sleeve_color"), Color);   // as mangas acompanham a camisa
    Sound(TEXT("ui_click"), .4f);
    ShowLookOnStage();
}

void SVortexFrontEnd::SetHairStyle(FName Id)
{
    if (Look.Pieces.FindRef(TEXT("block_hair")) == Id) return;
    Look.Pieces.Add(TEXT("block_hair"), Id);
    Sound(TEXT("equip"), .45f);
    ShowLookOnStage();
}

void SVortexFrontEnd::SetHairColor(FColor Color, bool bFromPicker)
{
    Color.A = 255;
    Look.Colors.Add(TEXT("hair_color"), Color);
    // a cor livre guarda a matiz dela; as cores prontas e o código acertam a cor livre
    if (!bFromPicker)
    {
        const FLinearColor Hsv = FLinearColor(Color.R / 255.f, Color.G / 255.f, Color.B / 255.f).LinearRGBToHSV();
        HairHsv = FLinearColor(Hsv.G > .001f && Hsv.B > .001f ? Hsv.R : HairHsv.R, Hsv.G, Hsv.B, 1.f);
        Sound(TEXT("ui_click"), .4f);
    }
    if (PickerHex.IsValid() && !PickerHex->HasKeyboardFocus()) PickerHex->SetText(FText::FromString(FVortexLookCatalog::ColorHex(Color)));
    ShowLookOnStage();
}

TSharedRef<SWidget> SVortexFrontEnd::BuildHairPanel()
{
    const FVortexLookCatalog& Catalog = FVortexLookCatalog::Get();
    const FColor Current = Look.Colors.FindRef(TEXT("hair_color"));
    {
        const FLinearColor Hsv = FLinearColor(Current.R / 255.f, Current.G / 255.f, Current.B / 255.f).LinearRGBToHSV();
        HairHsv = FLinearColor(Hsv.G > .001f && Hsv.B > .001f ? Hsv.R : HairHsv.R, Hsv.G, Hsv.B, 1.f);
    }
    const FButtonStyle* Plain = &FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder"));
    auto Title = [](const FString& Value) { return Text(Upper(Value), 13, Gold, TEXT("Bold"), 160); };

    // um botão de escolha (aceso quando é o atual), 2 por linha
    auto Choice = [this, Plain](const FString& Label, TFunction<bool()> Chosen, TFunction<void()> Pick) -> TSharedRef<SWidget>
    {
        return SNew(SButton).ButtonStyle(Plain).ContentPadding(0.f).OnClicked_Lambda([Pick]() { Pick(); return FReply::Handled(); })
        [
            SNew(SBox).WidthOverride(158.f).HeightOverride(40.f)
            [
                SNew(SOverlay)
                + SOverlay::Slot()[SNew(SImage).Image(Round(10.f)).ColorAndOpacity_Lambda([Chosen]() { return FSlateColor(Chosen() ? FLinearColor(GoldDeep.R, GoldDeep.G, GoldDeep.B, .55f) : FLinearColor(.02f, .03f, .05f, .7f)); })]
                + SOverlay::Slot()[SNew(SImage).Image(Outline(10.f, 1.5f)).ColorAndOpacity_Lambda([Chosen]() { return FSlateColor(Chosen() ? Gold : FLinearColor(1.f, 1.f, 1.f, .18f)); })]
                + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[Text(Label, 15, Cream, TEXT("Bold"))]
            ]
        ];
    };
    auto Pairs = [](const TArray<TSharedRef<SWidget>>& Items)
    {
        TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
        for (int32 i = 0; i < Items.Num(); i += 2)
        {
            TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
            Row->AddSlot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)[Items[i]];
            if (Items.IsValidIndex(i + 1)) Row->AddSlot().AutoWidth()[Items[i + 1]];
            Rows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[Row];
        }
        return Rows;
    };
    // o corpo (a mesma malha e as mesmas animações; o feminino com o top e os cílios)
    TArray<TSharedRef<SWidget>> Bodies;
    for (const TPair<FName, FString>& Body : { TPair<FName, FString>(TEXT("male"), TEXT("Masculino")), TPair<FName, FString>(TEXT("female"), TEXT("Feminino")) })
    {
        const FName Id = Body.Key;
        Bodies.Add(Choice(Body.Value, [this, Id]() { return Look.Appearance == Id; }, [this, Id]() { SetAppearance(Id); }));
    }
    // os estilos: um botão por cabelo, o escolhido aceso
    TArray<TSharedRef<SWidget>> StyleButtons;
    for (const FVortexBlockHair& Hair : FVortexLookCatalog::BlockHairs())
    {
        if (!Hair.Fits(Look.Appearance)) continue;          // os cabelos do corpo escolhido (a troca de corpo refaz a tela)
        const FName Id = Hair.Id;
        StyleButtons.Add(Choice(Hair.Name, [this, Id]() { return Look.Pieces.FindRef(TEXT("block_hair")) == Id; }, [this, Id]() { SetHairStyle(Id); }));
    }
    TSharedRef<SVerticalBox> Styles = Pairs(StyleButtons);
    // os rostos (Data/Looks/rostos.json): os olhos e as sobrancelhas de cada estilo, nos dois corpos
    TArray<TSharedRef<SWidget>> FaceButtons;
    const FName Tone = Look.Pieces.FindRef(TEXT("elfo_pele"));   // vazio no humano
    for (const FVortexFaceStyle& Face : VortexFaceStyles())
    {
        if (Face.Pele != Tone) continue;                    // os olhos de cada pele
        const FName Id = Face.Id;
        auto ChosenFace = [this]() { const FName F = Look.Pieces.FindRef(TEXT("block_face")); return F.IsNone() ? FName(TEXT("padrao")) : F; };
        FaceButtons.Add(Choice(Face.Name, [ChosenFace, Id]() { return ChosenFace() == Id; }, [this, Id, ChosenFace]()
        {
            if (ChosenFace() == Id) return;
            Look.Pieces.Add(TEXT("block_face"), Id);
            Sound(TEXT("equip"), .45f);
            ShowLookOnStage();
        }));
    }
    // as roupas do corpo escolhido (Tools/Blender/MakeBlockOutfit.py) e a sem roupa
    TArray<TSharedRef<SWidget>> OutfitButtons;
    for (const FVortexBlockOutfit& Outfit : FVortexLookCatalog::BlockOutfits())
    {
        if (!Outfit.Fits(Look.Appearance) || !Outfit.FitsRace(Look.Race)) continue;
        const FName Id = Outfit.Id;
        OutfitButtons.Add(Choice(Outfit.Name, [this, Id]() { return Look.Pieces.FindRef(TEXT("block_outfit")) == Id; }, [this, Id]()
        {
            if (Look.Pieces.FindRef(TEXT("block_outfit")) == Id) return;
            Look.Pieces.Add(TEXT("block_outfit"), Id);
            Sound(TEXT("equip"), .45f);
            ShowLookOnStage();
        }));
    }

    // as roupas separadas (Data/Looks/pecas.json): uma linha por espaço, com setas; a primeira opção é nenhuma
    TSharedRef<SVerticalBox> WearRows = SNew(SVerticalBox);
    for (const FVortexWearSlot& Wear : VortexWearSlots())
    {
        const FName Key = Wear.Key;
        TArray<FName> Options = { NAME_None };
        for (const FVortexWearPiece& Piece : VortexWearPieces()) if (Piece.Slot == Key && Piece.Body == Look.Appearance && (Piece.Race.IsNone() || Piece.Race == Look.Race)) Options.Add(Piece.Id);
        auto Arrow = [this, Plain, Key, Options](const TCHAR* Label, int32 Direction) -> TSharedRef<SWidget>
        {
            return SNew(SButton).ButtonStyle(Plain).ContentPadding(0.f).OnClicked_Lambda([this, Key, Options, Direction]()
            {
                const int32 Current = FMath::Max(0, Options.IndexOfByKey(Look.Pieces.FindRef(Key)));
                const FName Next = Options[(Current + Direction + Options.Num()) % Options.Num()];
                if (Next.IsNone()) Look.Pieces.Remove(Key); else Look.Pieces.Add(Key, Next);
                Sound(TEXT("equip"), .45f);
                ShowLookOnStage();
                return FReply::Handled();
            })
            [
                SNew(SBox).WidthOverride(40.f).HeightOverride(44.f)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()[SNew(SImage).Image(Round(10.f)).ColorAndOpacity(FLinearColor(.02f, .03f, .05f, .7f))]
                    + SOverlay::Slot()[SNew(SImage).Image(Outline(10.f, 1.5f)).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, .18f))]
                    + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[Text(Label, 18, Cream, TEXT("Bold"))]
                ]
            ];
        };
        WearRows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()[Arrow(TEXT("<"), -1)]
            + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).HAlign(HAlign_Center)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Text(Wear.Label, 12, Muted, TEXT("Bold"))]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                [
                    SNew(STextBlock).Font(Font(15)).ColorAndOpacity(Cream).Text_Lambda([this, Key]()
                    {
                        const FVortexWearPiece* Piece = VortexWearPiece(Look.Pieces.FindRef(Key));
                        return FText::FromString(Piece ? Piece->Name : TEXT("Nenhum"));
                    })
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth()[Arrow(TEXT(">"), 1)]
        ];
    }

    // as cores prontas (a paleta do "hair_color"): 7 por linha, a escolhida com o contorno dourado
    TSharedRef<SVerticalBox> Presets = SNew(SVerticalBox);
    TSharedPtr<SHorizontalBox> Line;
    const FVortexColorSlot* Slot = Catalog.Slot(TEXT("hair_color"));
    const TArray<FColor> Palette = Slot ? Slot->Palette : TArray<FColor>();
    for (int32 i = 0; i < Palette.Num(); ++i)
    {
        if (i % 7 == 0) Presets->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[SAssignNew(Line, SHorizontalBox)];
        const FColor Color = Palette[i];
        Line->AddSlot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
        [
            SNew(SButton).ButtonStyle(Plain).ContentPadding(0.f).OnClicked_Lambda([this, Color]() { SetHairColor(Color, false); return FReply::Handled(); })
            [
                SNew(SBox).WidthOverride(36.f).HeightOverride(36.f)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()[SNew(SImage).Image(Round(9.f)).ColorAndOpacity(FLinearColor::FromSRGBColor(Color))]
                    + SOverlay::Slot()[SNew(SImage).Image(Outline(9.f, 2.5f)).ColorAndOpacity_Lambda([this, Color]()
                    {
                        const FColor Chosen = Look.Colors.FindRef(TEXT("hair_color"));
                        return FSlateColor(Chosen.R == Color.R && Chosen.G == Color.G && Chosen.B == Color.B ? Gold : FLinearColor(0.f, 0.f, 0.f, .35f));
                    })]
                ]
            ]
        ];
    }

    // a cor livre: matiz (com o arco-íris por trás), saturação e brilho (com a faixa da cor atual por trás) e o código
    auto Picked = [this]() { return HairHsv.HSVToLinearRGB().ToFColor(false); };
    auto Slider = [this, Picked](const FString& Label, int32 Channel, float Max, TSharedRef<SWidget> Track) -> TSharedRef<SWidget>
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(84.f)[Text(Label, 13, Muted, TEXT("Bold"))]]
            + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
            [
                SNew(SOverlay)
                + SOverlay::Slot().VAlign(VAlign_Center)[SNew(SBox).HeightOverride(10.f)[Track]]
                + SOverlay::Slot()
                [
                    SNew(SSlider).SliderBarColor(FLinearColor::Transparent).SliderHandleColor(Cream)
                    .Value_Lambda([this, Channel, Max]() { return (Channel == 0 ? HairHsv.R : Channel == 1 ? HairHsv.G : HairHsv.B) / Max; })
                    .OnValueChanged_Lambda([this, Channel, Max, Picked](float Value)
                    {
                        (Channel == 0 ? HairHsv.R : Channel == 1 ? HairHsv.G : HairHsv.B) = Value * Max;
                        SetHairColor(Picked(), true);
                    })
                ]
            ];
    };
    TSharedRef<SHorizontalBox> Rainbow = SNew(SHorizontalBox);
    for (int32 i = 0; i < 24; ++i)
        Rainbow->AddSlot().FillWidth(1.f)[SNew(SImage).Image(White()).ColorAndOpacity(FLinearColor((i + .5f) * 15.f, .8f, .9f).HSVToLinearRGB())];
    auto Ramp = [this](int32 Channel)
    {
        // a faixa da saturação ou do brilho com a matiz atual, em 12 passos
        TSharedRef<SHorizontalBox> Steps = SNew(SHorizontalBox);
        for (int32 i = 0; i < 12; ++i)
        {
            const float T = (i + .5f) / 12.f;
            Steps->AddSlot().FillWidth(1.f)[SNew(SImage).Image(White()).ColorAndOpacity_Lambda([this, Channel, T]()
            {
                const FLinearColor Hsv(HairHsv.R, Channel == 1 ? T : HairHsv.G, Channel == 2 ? T : HairHsv.B);
                return FSlateColor(FLinearColor::FromSRGBColor(Hsv.HSVToLinearRGB().ToFColor(false)));
            })];
        }
        return Steps;
    };

    // 09/11: o elfo: as 2 peles (trocar refaz a tela, porque os olhos dependem dela) e os 4 tipos de orelha
    TSharedRef<SWidget> ElfBox = SNullWidget::NullWidget;
    if (Look.Race == TEXT("elfo"))
    {
        TArray<TSharedRef<SWidget>> Skins, Ears;
        for (const TPair<FName, FString>& Skin : { TPair<FName, FString>(TEXT("clara"), TEXT("Clara")), TPair<FName, FString>(TEXT("escura"), TEXT("Dark elfo")) })
        {
            const FName Id = Skin.Key;
            Skins.Add(Choice(Skin.Value, [this, Id]() { return Look.Pieces.FindRef(TEXT("elfo_pele")) == Id; }, [this, Id]()
            {
                if (Look.Pieces.FindRef(TEXT("elfo_pele")) == Id) return;
                Look.Pieces.Add(TEXT("elfo_pele"), Id);
                Look = FVortexLookCatalog::Get().Normalized(Look);
                Sound(TEXT("equip"), .45f);
                ShowLookOnStage();
            const double Keep = PageAt;
            Go(EPage::Creator);
            PageAt = Keep;
            }));
        }
        for (const TPair<FName, FString>& Ear : { TPair<FName, FString>(TEXT("curta"), TEXT("Curta")), TPair<FName, FString>(TEXT("longa"), TEXT("Longa")), TPair<FName, FString>(TEXT("tras"), TEXT("Para trás")), TPair<FName, FString>(TEXT("caida"), TEXT("Caída")) })
        {
            const FName Id = Ear.Key;
            Ears.Add(Choice(Ear.Value, [this, Id]() { return Look.Pieces.FindRef(TEXT("elfo_orelha")) == Id; }, [this, Id]()
            {
                if (Look.Pieces.FindRef(TEXT("elfo_orelha")) == Id) return;
                Look.Pieces.Add(TEXT("elfo_orelha"), Id);
                Look = FVortexLookCatalog::Get().Normalized(Look);
                Sound(TEXT("equip"), .45f);
                ShowLookOnStage();
            }));
        }
        ElfBox = SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[Title(TEXT("Pele"))]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[Pairs(Skins)]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[Title(TEXT("Orelhas"))]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[Pairs(Ears)];
    }

    TSharedRef<SVerticalBox> Body = SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[Title(TEXT("Corpo"))]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[Pairs(Bodies)]
        + SVerticalBox::Slot().AutoHeight()[ElfBox]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[Title(TEXT("Rosto"))]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[Pairs(FaceButtons)]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[Title(TEXT("Roupa"))]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[Pairs(OutfitButtons)]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[Title(TEXT("Cabelo"))]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[Styles]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[Title(TEXT("Cor"))]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[Presets]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[Title(TEXT("Cor livre"))]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[Slider(TEXT("Matiz"), 0, 360.f, Rainbow)]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[Slider(TEXT("Saturação"), 1, 1.f, Ramp(1))]
        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[Slider(TEXT("Brilho"), 2, 1.f, Ramp(2))]
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
            [
                SNew(SBox).WidthOverride(44.f).HeightOverride(44.f)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()[SNew(SImage).Image(Round(10.f)).ColorAndOpacity_Lambda([this]() { return FSlateColor(FLinearColor::FromSRGBColor(Look.Colors.FindRef(TEXT("hair_color")))); })]
                    + SOverlay::Slot()[SNew(SImage).Image(Outline(10.f, 2.f)).ColorAndOpacity(Gold)]
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
            [
                // o código (#RRGGBB): vale ao apertar Enter ou sair da caixa; inválido volta para a cor atual
                SAssignNew(PickerHex, SEditableTextBox).Style(&FieldStyle()).Font(Font(16))
                .Text(FText::FromString(FVortexLookCatalog::ColorHex(Current)))
                .OnTextCommitted_Lambda([this](const FText& Value, ETextCommit::Type)
                {
                    FString Hex = Value.ToString().TrimStartAndEnd();
                    Hex.RemoveFromStart(TEXT("#"));
                    const bool bValid = Hex.Len() == 6 && Algo::AllOf(Hex, [](TCHAR C) { return FChar::IsHexDigit(C); });
                    if (bValid) SetHairColor(FColor::FromHex(Hex), false);
                    else if (PickerHex.IsValid()) PickerHex->SetText(FText::FromString(FVortexLookCatalog::ColorHex(Look.Colors.FindRef(TEXT("hair_color")))));
                })
            ]
        ];
    return SNew(SBox).WidthOverride(380.f)
    [
        SNew(SOverlay)
        + SOverlay::Slot()[SNew(SBorder).BorderImage(VortexUiKit::Window())]
        + SOverlay::Slot().Padding(FMargin(28.f, 24.f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[Text(Upper(TEXT("Aparência")), 24, Cream, TEXT("Black"), 120)]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 16.f)
            [SNew(SBox).WidthOverride(70.f).HeightOverride(3.f).HAlign(HAlign_Left)[SNew(SImage).Image(Round(2.f)).ColorAndOpacity(Gold)]]
            // até 16 cabelos por corpo: o painel rola se a tela for baixa
            + SVerticalBox::Slot().AutoHeight()[SNew(SBox).MaxDesiredHeight(600.f)[SNew(SScrollBox) + SScrollBox::Slot()[Body]]]
        ]
    ];
}

void SVortexFrontEnd::SetAppearance(FName Appearance)
{
    if (Look.Appearance == Appearance) return;
    Look.Appearance = Appearance;
    // o cabelo de blocos que não é deste corpo volta para o padrão dele (o Normalized escolhe)
    Look.Pieces.Remove(TEXT("hair"));
    Look.Pieces.Remove(TEXT("eyes"));
    Look.Pieces.Remove(TEXT("mouth"));
    Look = FVortexLookCatalog::Get().Normalized(Look);
    Sound(TEXT("equip"), .5f);
    ShowLookOnStage();
    const double Keep = PageAt;
    Go(EPage::Creator);
    PageAt = Keep;
}

void SVortexFrontEnd::Randomize(bool bOnlyColors)
{
    Look = FVortexLookCatalog::Get().Random(Look.Race, Look.Appearance, Rng, bOnlyColors, &Look);
    Sound(TEXT("equip"), .6f);
    if (AVortexMenuStage* S = Stage()) { S->ShowLook(Look); S->Pulse(); }
}

void SVortexFrontEnd::RandomName()
{
    HeroName = FVortexLookCatalog::Get().RandomName(Look.Race, Rng);
    if (HeroNameBox.IsValid()) HeroNameBox->SetText(FText::FromString(HeroName));
    Sound(TEXT("ui_click"), .5f);
}

void SVortexFrontEnd::SaveHero()
{
    if (CelebrateAt > 0.0) return;
    FString Error;
    FGuid PlayerId;
    const FString Name = HeroNameBox.IsValid() ? HeroNameBox->GetText().ToString() : HeroName;
    if (!Owner.IsValid() || !Owner->CreateHero(SelectedWorldId, Name, Look.Race, Look.Appearance, Look.Pieces, PlayerId, Error, Look.Colors))
    {
        SetStatus(Error, true);
        return;
    }
    CreatedHeroId = PlayerId;
    // a foto do rosto para a HUD: tirada agora, com o herói parado de frente na vitrine
    if (AVortexMenuStage* S = Stage(); S && S->GetHero()) VortexPortrait::CaptureAndSave(S->GetHero(), PlayerId);
    CelebrateName = Name.TrimStartAndEnd();
    CelebrateAt = Now;
    if (AVortexMenuStage* S = Stage()) { S->ResetView(); S->Celebrate(); }
}

// ---- multiplayer -------------------------------------------------------------------------------

TSharedRef<SWidget> SVortexFrontEnd::BuildMultiplayer()
{
    // dois caminhos: abrir a sala num mundo de multiplayer (separados dos de um jogador) ou entrar na de um amigo
    TSharedRef<SVerticalBox> Entries = SNew(SVerticalBox);
    int32 Index = 0;
    auto Add = [&](const FString& Title, const FString& Sub, const FSlateBrush* Icon, bool bPrimary, FSimpleDelegate Action)
    {
        Entries->AddSlot().AutoHeight().Padding(0.f, 6.f).HAlign(HAlign_Left)
        [
            SNew(SVortexMenuEntry).Title(Title).Subtitle(Sub).Icon(Icon).bPrimary(bPrimary).Index(Index++).Menu(this).OnPressed(Action)
        ];
    };
    const int32 Count = Owner.IsValid() ? Owner->GetWorlds(true).Num() : 0;
    Add(TEXT("Criar sala"), FString::Printf(TEXT("Seus mundos de multiplayer (%d de %d), até 3 heróis em cada."), Count, UVortexGameInstance::MaxWorlds),
        &IconHost, true, FSimpleDelegate::CreateLambda([this]() { bCreatingWorld = false; bMultiplayerMode = true; Go(EPage::Worlds); }));
    Add(TEXT("Entrar na sala"), TEXT("Jogue no mundo de um amigo com o código que ele passar."), &IconKey, false,
        FSimpleDelegate::CreateLambda([this]() { Go(EPage::Join); }));
    return SNew(SBox).HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(70.f, 46.f, 40.f, 40.f))
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [
            PageHeader(TEXT("Multiplayer"),
                TEXT("Na sala, cada amigo cria o herói dele e marca pronto; quando todos estiverem prontos, o anfitrião começa a aventura."),
                [this]() { Go(EPage::Home); })
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(10.f, 34.f, 0.f, 0.f)[Entries]
    ];
}

// ---- conquistas --------------------------------------------------------------------------------

namespace
{
    struct FAchievementStats
    {
        int32 Heroes = 0, MaxLevel = 0, Quests = 0, Friends = 0, Coins = 0, MultiWorlds = 0;
        bool bAwakened = false, bMount = false;
        float Explored = 0.f;
    };

    /** Os números das conquistas: o melhor de cada herói salvo neste computador (de todos os mundos e os viajantes). */
    FAchievementStats GatherStats(UVortexGameInstance* Owner)
    {
        FAchievementStats Stats;
        UVortexSaveSubsystem* Saves = Owner ? Owner->GetSubsystem<UVortexSaveSubsystem>() : nullptr;
        if (!Saves) return Stats;
        TArray<FGuid> Worlds = { UVortexGameInstance::TravellerWorld() };
        for (const FVortexWorldSummary& World : Owner->GetWorlds())
        {
            Worlds.Add(World.WorldId);
            Stats.MultiWorlds += World.bMultiplayer ? 1 : 0;
        }
        for (const FGuid& WorldId : Worlds)
        {
            for (const FVortexHeroSummary& Hero : Owner->GetHeroes(WorldId))
            {
                ++Stats.Heroes;
                const UVortexPlayerSave* Save = Saves->LoadLocalPlayer(Hero.PlayerId);
                if (!Save) continue;
                Stats.MaxLevel = FMath::Max(Stats.MaxLevel, FVortexRpgCatalog::LevelFromXp(Save->Experience));
                Stats.bAwakened |= !Save->ClassId.IsNone();
                Stats.bMount |= !Save->MountCoat.IsNone();
                int32 Done = 0;
                for (const FVortexQuestState& Quest : Save->QuestLog) Done += Quest.State == 2 ? 1 : 0;
                Stats.Quests = FMath::Max(Stats.Quests, Done);
                Stats.Friends = FMath::Max(Stats.Friends, Save->Friends.Num());
                Stats.Coins = FMath::Max(Stats.Coins, Save->Coins);
                int32 Seen = 0;
                for (const uint8 Cell : Save->MapSeen) Seen += Cell ? 1 : 0;
                // o continente tem ~7,5 km de lado: ~417 pedaços de 18 m por lado
                Stats.Explored = FMath::Max(Stats.Explored, Seen / (417.f * 417.f));
            }
        }
        return Stats;
    }
}

TSharedRef<SWidget> SVortexFrontEnd::BuildAchievements()
{
    const FAchievementStats S = GatherStats(Owner.Get());
    struct FEntry { const TCHAR* Title; const TCHAR* Text; float Have; float Need; };
    const TArray<FEntry> List = {
        { TEXT("Primeiros passos"), TEXT("Crie um herói."), float(S.Heroes), 1.f },
        { TEXT("Aventureiro"), TEXT("Chegue ao nível 5."), float(S.MaxLevel), 5.f },
        { TEXT("Desperto"), TEXT("Escolha a sua classe no Despertar (nível 10)."), S.bAwakened ? 1.f : 0.f, 1.f },
        { TEXT("Veterano"), TEXT("Chegue ao nível 25."), float(S.MaxLevel), 25.f },
        { TEXT("Lenda do vale"), TEXT("Chegue ao nível 50."), float(S.MaxLevel), 50.f },
        { TEXT("Mensageiro"), TEXT("Conclua 5 missões."), float(S.Quests), 5.f },
        { TEXT("Herói do povo"), TEXT("Conclua 15 missões."), float(S.Quests), 15.f },
        { TEXT("A saga inteira"), TEXT("Conclua as 31 missões."), float(S.Quests), 31.f },
        { TEXT("Cavaleiro"), TEXT("Dome um cavalo."), S.bMount ? 1.f : 0.f, 1.f },
        { TEXT("Explorador"), TEXT("Descubra 5% do continente no mapa."), S.Explored * 100.f, 5.f },
        { TEXT("Cartógrafo"), TEXT("Descubra 25% do continente no mapa."), S.Explored * 100.f, 25.f },
        { TEXT("Bons amigos"), TEXT("Faça amizade com outro herói."), float(S.Friends), 1.f },
        { TEXT("Companhia"), TEXT("Crie um mundo de multiplayer."), float(S.MultiWorlds), 1.f },
        { TEXT("Bolso cheio"), TEXT("Junte 1.000 moedas."), float(S.Coins), 1000.f },
    };
    int32 Unlocked = 0;
    for (const FEntry& E : List) Unlocked += E.Have >= E.Need ? 1 : 0;
    TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(8.f));
    for (int32 i = 0; i < List.Num(); ++i)
    {
        const FEntry& E = List[i];
        const bool bDone = E.Have >= E.Need;
        const float Fill = FMath::Clamp(E.Have / E.Need, 0.f, 1.f);
        const FString Progress = bDone ? FString(TEXT("CONQUISTADA"))
            : E.Need <= 1.f ? FString(TEXT("BLOQUEADA")) : FString::Printf(TEXT("%d / %d"), FMath::FloorToInt(E.Have), FMath::RoundToInt(E.Need));
        Grid->AddSlot(i % 2, i / 2)
        [
            SNew(SBox).WidthOverride(430.f).HeightOverride(86.f)
            [
                SNew(SOverlay)
                + SOverlay::Slot()[SNew(SBorder).BorderImage(VortexUiKit::Panel())]
                + SOverlay::Slot()[SNew(SImage).Image(Outline(12.f, 1.f)).ColorAndOpacity(bDone ? Gold : FLinearColor(Muted.R, Muted.G, Muted.B, .3f))]
                + SOverlay::Slot().Padding(14.f, 10.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
                    [
                        SNew(SImage).Image(&IconTrophy).DesiredSizeOverride(FVector2D(46.f, 46.f))
                        .ColorAndOpacity(bDone ? Gold : FLinearColor(1.f, 1.f, 1.f, .18f))
                    ]
                    + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(E.Title)).Font(Display(15, 700, 20)).ColorAndOpacity(bDone ? Cream : Muted)]
                        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)[Text(E.Text, 12, Muted)]
                        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                            [
                                SNew(SBox).HeightOverride(5.f)
                                [
                                    SNew(SOverlay)
                                    + SOverlay::Slot()[SNew(SImage).Image(Round(2.f)).ColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .5f))]
                                    + SOverlay::Slot().HAlign(HAlign_Left)
                                    [
                                        SNew(SBox).WidthOverride(FMath::Max(1.f, 230.f * Fill))
                                        [SNew(SImage).Image(Round(2.f)).ColorAndOpacity(bDone ? Gold : Emerald)]
                                    ]
                                ]
                            ]
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f, 0.f, 0.f)
                            [SNew(STextBlock).Text(FText::FromString(Progress)).Font(Display(10, 700, 120)).ColorAndOpacity(bDone ? Gold : Muted)]
                        ]
                    ]
                ]
            ]
        ];
    }
    return SNew(SBox).HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(70.f, 46.f, 40.f, 40.f))
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [
            PageHeader(TEXT("Conquistas"),
                FString::Printf(TEXT("%d de %d conquistadas pelos heróis deste computador."), Unlocked, List.Num()),
                [this]() { Go(EPage::Home); })
        ]
        + SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 24.f, 0.f, 0.f)
        [
            SNew(SScrollBox) + SScrollBox::Slot()[Grid]
        ]
    ];
}

// ---- sala de espera ---------------------------------------------------------------------------

TArray<AVortexPlayerState*> SVortexFrontEnd::LobbyPlayers() const
{
    TArray<AVortexPlayerState*> Result;
    const UWorld* World = Owner.IsValid() ? Owner->GetWorld() : nullptr;
    const AGameStateBase* State = World ? World->GetGameState() : nullptr;
    if (!State) return Result;
    for (APlayerState* Player : State->PlayerArray)
    {
        if (AVortexPlayerState* Lobby = Cast<AVortexPlayerState>(Player)) Result.Add(Lobby);
    }
    Result.Sort([](const AVortexPlayerState& A, const AVortexPlayerState& B)
    {
        if (A.IsLobbyHost() != B.IsLobbyHost()) return A.IsLobbyHost();
        return A.GetPlayerId() < B.GetPlayerId();
    });
    if (Result.Num() > 8) Result.SetNum(8);
    return Result;
}

AVortexPlayerState* SVortexFrontEnd::LocalLobbyState() const
{
    const APlayerController* Controller = Owner.IsValid() ? Owner->GetFirstLocalPlayerController() : nullptr;
    return Controller ? Controller->GetPlayerState<AVortexPlayerState>() : nullptr;
}

void SVortexFrontEnd::TickLobby()
{
    if (!Owner.IsValid()) return;
    APlayerController* Controller = Owner->GetFirstLocalPlayerController();
    // o convidado chega antes do controle existir: o mouse e o foco vêm quando ele aparece
    if (Controller && !bLobbyInput)
    {
        bLobbyInput = true;
        Controller->bShowMouseCursor = true;
        FInputModeUIOnly Input;
        Input.SetWidgetToFocus(SharedThis(this));
        Controller->SetInputMode(Input);
    }
    if (Now < LobbyRefreshAt) return;
    LobbyRefreshAt = Now + .2;
    TArray<FVortexLook> Looks;
    TArray<bool> Ready;
    for (const AVortexPlayerState* Player : LobbyPlayers())
    {
        if (!Player->HasLobbyHero()) continue;
        Looks.Add(FVortexLookCatalog::Get().Normalized(AVortexCharacter::LookFromNet(Player->GetLobbyLook())));
        Ready.Add(Player->IsLobbyReady() || Player->IsLobbyHost());
    }
    if (AVortexMenuStage* S = Stage()) S->ShowLobby(Looks, Ready);
}

TSharedRef<SWidget> SVortexFrontEnd::BuildLobby()
{
    const bool bHost = Owner.IsValid() && Owner->IsLobbyHost();
    TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
    for (int32 Index = 0; Index < 8; ++Index)
    {
        auto Player = [this, Index]() -> const AVortexPlayerState*
        {
            const TArray<AVortexPlayerState*> Players = LobbyPlayers();
            return Players.IsValidIndex(Index) ? Players[Index] : nullptr;
        };
        Rows->AddSlot().AutoHeight().Padding(0.f, 3.f)
        [
            SNew(SBox).HeightOverride(44.f)
            [
                SNew(SBorder).BorderImage(Round(10.f)).Padding(FMargin(14.f, 0.f))
                .BorderBackgroundColor_Lambda([this, Player]()
                {
                    const AVortexPlayerState* P = Player();
                    if (!P) return FLinearColor(.02f, .03f, .05f, .35f);
                    return P == LocalLobbyState() ? FLinearColor(.09f, .07f, .03f, .85f) : FLinearColor(.015f, .025f, .045f, .85f);
                })
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                    [
                        SNew(STextBlock).Font(Font(16, TEXT("Bold")))
                        .ColorAndOpacity_Lambda([Player]() { return FSlateColor(Player() ? Cream : FLinearColor(Muted.R, Muted.G, Muted.B, .45f)); })
                        .Text_Lambda([Player, Index]()
                        {
                            const AVortexPlayerState* P = Player();
                            if (!P) return FText::FromString(FString::Printf(TEXT("%d.  vaga livre"), Index + 1));
                            return FText::FromString(FString::Printf(TEXT("%d.  %s"), Index + 1, P->HasLobbyHero() ? *P->GetLobbyHero() : TEXT("chegando...")));
                        })
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f)
                    [
                        SNew(STextBlock).Font(Font(13)).ColorAndOpacity(Muted)
                        .Text_Lambda([this, Player]()
                        {
                            const AVortexPlayerState* P = Player();
                            if (!P || !P->HasLobbyHero()) return FText::GetEmpty();
                            return FText::FromString(FString::Printf(TEXT("%s  •  nível %d"), *RaceName(P->GetLobbyLook().Race), P->GetLobbyLevel()));
                        })
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [
                        SNew(SBox).WidthOverride(110.f).HAlign(HAlign_Right)
                        [
                            SNew(STextBlock).Font(Font(12, TEXT("Bold"), 160))
                            .ColorAndOpacity_Lambda([Player]()
                            {
                                const AVortexPlayerState* P = Player();
                                return FSlateColor(P && P->IsLobbyHost() ? Gold : P && P->IsLobbyReady() ? Good : Muted);
                            })
                            .Text_Lambda([Player]()
                            {
                                const AVortexPlayerState* P = Player();
                                if (!P) return FText::GetEmpty();
                                return FText::FromString(P->IsLobbyHost() ? TEXT("ANFITRIÃO") : P->IsLobbyReady() ? TEXT("PRONTO") : TEXT("ESPERANDO"));
                            })
                        ]
                    ]
                ]
            ]
        ];
    }
    const FString Code = Owner.IsValid() && bHost ? Owner->GetLobbyCode() : FString();
    const FString Where = !Owner.IsValid() ? FString() : !bHost ? FString(TEXT("Conectado ao anfitrião"))
        : Owner->IsLobbyOnline() ? FString(TEXT("Sala online (Epic): amigos de qualquer lugar entram com o código"))
        : FString(TEXT("Sala na rede local: amigos na mesma rede entram com o código"));
    TSharedRef<SVerticalBox> Actions = SNew(SVerticalBox);
    if (bHost)
    {
        Actions->AddSlot().AutoHeight().Padding(0.f, 4.f)[Button(TEXT("Pronto! Começar aventura"), FSimpleDelegate::CreateLambda([this]()
        {
            FString Error;
            if (Owner.IsValid() && !Owner->StartAdventure(Error)) SetStatus(Error, true);
            else Sound(TEXT("level_up"));
        }), 1, 330.f, &IconReady)];
    }
    else
    {
        // o convidado entra sem herói: escolhe um dos seus ou cria um aqui mesmo, conectado
        auto HasHero = [this]() { return Owner.IsValid() && Owner->GetSelectedHeroId().IsValid(); };
        Actions->AddSlot().AutoHeight().Padding(0.f, 4.f)
        [
            SNew(SBox).Visibility_Lambda([this, HasHero]() { const AVortexPlayerState* P = LocalLobbyState(); return P && P->IsLobbyReady() ? EVisibility::Collapsed : EVisibility::Visible; })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Font(Font(14, TEXT("Bold"))).ColorAndOpacity(Gold).AutoWrapText(true)
                    .Visibility_Lambda([HasHero]() { return HasHero() ? EVisibility::Collapsed : EVisibility::Visible; })
                    .Text(FText::FromString(TEXT("Primeiro escolha ou crie o seu herói.")))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
                [Button(TEXT("Escolher ou criar meu herói"), FSimpleDelegate::CreateLambda([this]() { Sound(TEXT("ui_open")); OpenTravellers(); }), 0, 330.f)]
            ]
        ];
        Actions->AddSlot().AutoHeight().Padding(0.f, 4.f)
        [
            SNew(SBox).Visibility_Lambda([this, HasHero]()
            {
                const AVortexPlayerState* P = LocalLobbyState();
                return P && !P->IsLobbyReady() && HasHero() && P->HasLobbyHero() ? EVisibility::Visible : EVisibility::Collapsed;
            })
            [Button(TEXT("Estou pronto"), FSimpleDelegate::CreateLambda([this]() { if (AVortexPlayerState* P = LocalLobbyState()) P->ServerSetLobbyReady(true); }), 1, 330.f, &IconReady)]
        ];
        Actions->AddSlot().AutoHeight().Padding(0.f, 4.f)
        [
            SNew(SBox).Visibility_Lambda([this]() { const AVortexPlayerState* P = LocalLobbyState(); return P && P->IsLobbyReady() ? EVisibility::Visible : EVisibility::Collapsed; })
            [Button(TEXT("Ainda não estou pronto"), FSimpleDelegate::CreateLambda([this]() { if (AVortexPlayerState* P = LocalLobbyState()) P->ServerSetLobbyReady(false); }), 0, 330.f)]
        ];
    }
    Actions->AddSlot().AutoHeight().Padding(0.f, 4.f)[Button(TEXT("Sair da sala"), FSimpleDelegate::CreateLambda([this]()
    {
        Sound(TEXT("ui_close"));
        if (Owner.IsValid()) Owner->ReturnToMenu();
    }), 2, 330.f)];
    return SNew(SBox).HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(FMargin(80.f, 0.f, 0.f, 0.f))
    [
        GlassPanel(TEXT("Sala de espera"),
            bHost ? TEXT("Passe o código para os amigos: eles entram em \"Entrar na sala de um amigo\". Quando todos estiverem prontos (ou se for jogar sozinho), comece a aventura.")
                  : TEXT("Escolha ou crie o seu herói, marque \"pronto\" e espere o anfitrião começar a aventura."),
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)[Text(Where, 14, Muted, TEXT("Regular"), 0, true)]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
            [
                SNew(SBox).Visibility(Code.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
                    [
                        // uma letra por caixa: fácil de ditar e de conferir
                        ([this, &Code]()
                        {
                            TSharedRef<SHorizontalBox> Letters = SNew(SHorizontalBox);
                            const bool bShort = Code.Len() <= 6;
                            for (int32 i = 0; i < Code.Len(); ++i)
                            {
                                Letters->AddSlot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
                                [
                                    SNew(SBox).WidthOverride(bShort ? 46.f : 26.f).HeightOverride(bShort ? 56.f : 40.f)
                                    [
                                        SNew(SOverlay)
                                        + SOverlay::Slot()[SNew(SImage).Image(Round(8.f)).ColorAndOpacity(FLinearColor(.01f, .02f, .035f, .9f))]
                                        + SOverlay::Slot()[SNew(SImage).Image(Outline(8.f, 1.5f)).ColorAndOpacity(Emerald)]
                                        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                                        [SNew(STextBlock).Text(FText::FromString(Code.Mid(i, 1))).Font(Display(bShort ? 28 : 18, 900)).ColorAndOpacity(Cream)]
                                    ]
                                ];
                            }
                            return Letters;
                        })()
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [Button(TEXT("Copiar"), FSimpleDelegate::CreateLambda([this, Code]()
                    {
                        FPlatformApplicationMisc::ClipboardCopy(*Code);
                        SetStatus(TEXT("Código copiado: é só colar para os amigos."));
                    }), 0, 130.f)]
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
            [
                SNew(STextBlock).Font(Font(13)).ColorAndOpacity(Muted)
                .Text_Lambda([this]()
                {
                    const TArray<AVortexPlayerState*> Players = LobbyPlayers();
                    int32 ReadyCount = 0;
                    for (const AVortexPlayerState* P : Players) ReadyCount += P->IsLobbyReady() || P->IsLobbyHost();
                    return FText::FromString(FString::Printf(TEXT("%d de 8 jogadores  •  %d prontos"), Players.Num(), ReadyCount));
                })
            ]
            + SVerticalBox::Slot().AutoHeight()[Rows]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[Actions],
            560.f)
    ];
}

TSharedRef<SWidget> SVortexFrontEnd::BuildCreator()
{
    PageBack = [this]() { if (CelebrateAt <= 0.0) Go(EPage::Heroes); };
    const FVortexLookCatalog& Catalog = FVortexLookCatalog::Get();
    const FVortexRaceInfo* Race = Catalog.Race(Look.Race);
    const FLinearColor Accent = Race ? Race->Accent : Gold;

    // a criação: o nome e, no humano de blocos, o cabelo (estilo e cor); o resto sai sorteado da raça
    const float Height = Race ? Race->Shape.Meters : 1.40f;
    return SNew(SOverlay)
        + SOverlay::Slot()[SNew(SVortexStageInput).Menu(this)]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 34.f, 0.f, 0.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [Text(FString::Printf(TEXT("%s"), Race ? *Upper(Race->Name) : TEXT("")), 46, Accent, TEXT("Black"), 260)]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [Text(FString::Printf(TEXT("%s  •  %.2f m"), Race ? *Upper(Race->Epithet) : TEXT(""), Height).Replace(TEXT("."), TEXT(",")), 13, Cream, TEXT("Bold"), 300)]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 10.f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)[Button(TEXT("Trocar raça"), FSimpleDelegate::CreateSP(this, &SVortexFrontEnd::StartCreation), 2, 170.f)]
                + SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)
                [Button(TEXT("Centralizar"), FSimpleDelegate::CreateLambda([this]() { if (AVortexMenuStage* S = Stage()) S->ResetView(); }), 2, 160.f)]
            ]
        ]
        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(0.f, 0.f, 60.f, 40.f)
        [
            FVortexLookCatalog::HasBlockHair(Look.Race) ? BuildHairPanel() : SNullWidget::NullWidget
        ]
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(60.f, 0.f, 0.f, 40.f)
        [
            // desistir da criação: volta aos heróis sem salvar nada
            Button(TEXT("Cancelar"), FSimpleDelegate::CreateLambda([this]() { if (CelebrateAt <= 0.0) Go(EPage::Heroes); }), 2, 180.f)
        ]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 40.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 10.f)
            [Text(TEXT("Arraste para girar a câmera  •  botão direito sobe e desce  •  roda aproxima"), 12, FLinearColor(Cream.R, Cream.G, Cream.B, .7f))]
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [SNew(SBox).WidthOverride(300.f)[TextField(HeroNameBox, TEXT("Nome do herói"), HeroName)]]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f)
                [Button(TEXT("Sortear nome"), FSimpleDelegate::CreateSP(this, &SVortexFrontEnd::RandomName), 2, 170.f)]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [Button(TEXT("Criar herói"), FSimpleDelegate::CreateSP(this, &SVortexFrontEnd::SaveHero), 1, 230.f, &IconNewHero)]
            ]
        ]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 200.f, 0.f, 0.f)
        [
            // comemoração: "nasce um herói" e o nome, acima da cabeça do herói
            SNew(SVerticalBox)
            .Visibility_Lambda([this]() { return Now - CelebrateAt < 3.2 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
            .RenderTransformPivot(FVector2D(.5f, .5f))
            .RenderTransform_Lambda([this]() { return FSlateRenderTransform(FScale2f(.8f + .2f * Ease(float(Now - CelebrateAt) / .6f))); })
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(STextBlock).Font(Font(20, TEXT("Bold"), 600)).Text(FText::FromString(TEXT("NASCE UM HERÓI")))
                .ShadowOffset(FVector2D(0.f, 3.f)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, .8f))
                .ColorAndOpacity_Lambda([this]() { return FSlateColor(FLinearColor(Gold.R, Gold.G, Gold.B, Ease(float(Now - CelebrateAt) / .4f))); })
            ]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(STextBlock).Font(Font(96, TEXT("Black"), 200)).ShadowOffset(FVector2D(0.f, 5.f)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, .85f))
                .Text_Lambda([this]() { return FText::FromString(CelebrateName); })
                .ColorAndOpacity_Lambda([this]() { return FSlateColor(FLinearColor(Cream.R, Cream.G, Cream.B, Ease(float(Now - CelebrateAt - .2) / .4f))); })
            ]
        ];
}

// ---- fotos de teste -------------------------------------------------------------------------
#if !UE_BUILD_SHIPPING
void SVortexFrontEnd::TickShot()
{
    if (!Stage()) return;
    // -VortexAutoContinue: o caminho do jogador sozinho (Continuar, o primeiro herói, Entrar no mundo), para medir o jogo
    // de verdade com o save dele
    static const bool bAutoContinue = FParse::Param(FCommandLine::Get(), TEXT("VortexAutoContinue"));
    if (bAutoContinue)
    {
        static int32 AutoStep = 0;
        static double AutoAt = 0.0;
        if (AutoStep == 0 && bStageReady && Now - ReadyAt > 2.0) { ContinueLast(); AutoStep = 1; AutoAt = Now; }
        else if (AutoStep == 1 && Now - AutoAt > 2.0) { PlayHero(); AutoStep = 2; }
    }
    // -VortexOptionsShot: uma foto de cada aba das opções (frontend_opcoes_<aba>.png)
    static const bool bOptionsShot = FParse::Param(FCommandLine::Get(), TEXT("VortexOptionsShot"));
    if (bOptionsShot)
    {
        static int32 Step = -1;
        static double At = 0.0;
        if (Step < 0) { if (bStageReady && Now - ReadyAt > 2.0) { Step = 0; At = Now; OptionsTab = 0; Go(EPage::Options); } return; }
        const int32 TabCount = VortexGraphics::Tabs().Num();
        if (Step < TabCount && OptionsTab == Step && Now - At > 1.2)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/frontend_opcoes_%d.png"), Step), true, false);
            ++Step;
            At = Now;
        }
        // a próxima aba só depois do quadro da foto
        else if (Step < TabCount && OptionsTab != Step && Now - At > .3) { OptionsTab = Step; Go(EPage::Options); At = Now; }
        else if (Step >= TabCount && Now - At > 1.0) { Step = 99; FPlatformMisc::RequestExit(false); }
        return;
    }
    // -VortexLookProbe=elfo: veste a raça com cada peça dela (uma foto por peça, em frontend_probe_*)
    static FString ProbeRace;
    static const bool bProbe = FParse::Value(FCommandLine::Get(), TEXT("VortexLookProbe="), ProbeRace);
    if (bProbe)
    {
        static TArray<TPair<FName, FName>> Queue;
        static int32 ProbeIndex = -1;
        static double ProbeAt = 0.0;
        if (ProbeIndex < 0)
        {
            if (!bStageReady || Now - ReadyAt < 2.0) return;
            FVortexLook Base;
            Base.Race = FName(*ProbeRace);
            Base.Appearance = TEXT("male");
            Base = FVortexLookCatalog::Get().Normalized(Base);
            for (const FName Category : { FName(TEXT("shirt")), FName(TEXT("hair")), FName(TEXT("eyes")), FName(TEXT("head")), FName(TEXT("face_gear")), FName(TEXT("back")), FName(TEXT("race")) })
            {
                for (const FName& Piece : FVortexLookCatalog::Get().Options(Base, Category))
                {
                    const FVortexPieceInfo* Info = FVortexLookCatalog::Get().Piece(Piece);
                    if (Info && Info->Races.Num() || Category == TEXT("race")) Queue.Emplace(Category, Piece);
                }
            }
            Look = Base;
            Go(EPage::Creator);
            ProbeIndex = 0;
            ProbeAt = Now;
            return;
        }
        if (ProbeIndex >= Queue.Num() * 2)
        {
            UE_LOG(LogTemp, Display, TEXT("VORTEX_FRONTEND_SHOT_DONE"));
            FPlatformMisc::RequestExit(false);
            return;
        }
        if (Now - ProbeAt < .7) return;
        ProbeAt = Now;
        const TPair<FName, FName>& Item = Queue[ProbeIndex / 2];
        if (ProbeIndex % 2 == 0)
        {
            FVortexLookCatalog::Get().ChoosePiece(Look, Item.Key, Item.Value);
            ShowLookOnStage();
        }
        else
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/probe/%s.png"), *Item.Value.ToString()), false, false);
        }
        ++ProbeIndex;
        return;
    }
    // -VortexElfMenu (09/11, Tools/RunElfChecks.ps1): o painel do elfo com cada pele, orelha e corpo (frontend_elfo_<n>.png)
    static const bool bElfMenu = FParse::Param(FCommandLine::Get(), TEXT("VortexElfMenu"));
    if (bElfMenu)
    {
        struct FCase { const TCHAR* Skin; const TCHAR* Ear; const TCHAR* Body; };
        static const FCase Cases[] = { { TEXT("clara"), TEXT("longa"), TEXT("female") }, { TEXT("escura"), TEXT("curta"), TEXT("female") },
            { TEXT("escura"), TEXT("tras"), TEXT("male") }, { TEXT("clara"), TEXT("caida"), TEXT("male") } };
        static int32 Step = -1;
        static double At = 0.0;
        if (Step < 0) { if (bStageReady && Now - ReadyAt > 2.0) { Step = 0; At = Now - 10.0; } return; }
        if (Now - At < 2.5) return;
        At = Now;
        const int32 Index = Step / 2;
        if (Index >= UE_ARRAY_COUNT(Cases)) { UE_LOG(LogTemp, Display, TEXT("VORTEX_ELF_MENU_DONE")); FPlatformMisc::RequestExit(false); Step = 99999; return; }
        if (Step % 2 == 0)
        {
            FVortexLook Elf;
            Elf.Race = TEXT("elfo");
            Elf.Appearance = Cases[Index].Body;
            Elf.Pieces.Add(TEXT("elfo_pele"), Cases[Index].Skin);
            Elf.Pieces.Add(TEXT("elfo_orelha"), Cases[Index].Ear); Elf.Pieces.Add(TEXT("peca_chapeu"), Index == 0 ? TEXT("P_elfo_tiara_azul") : Index == 1 ? TEXT("P_chapeu_bruxa") : Index == 2 ? TEXT("P_chapeu_cacador") : TEXT("P_chapeu_capuz_marrom")); Elf.Pieces.Add(TEXT("peca_acessorio"), Index == 0 ? TEXT("P_elfo_colar_verde") : Index == 2 ? TEXT("P_acess_corrente") : TEXT("P_acess_lenco")); Elf.Pieces.Add(TEXT("peca_cintura"), Index == 0 ? TEXT("P_acess_bolsa_redonda") : Index == 1 ? TEXT("P_acess_cinto_ervas") : Index == 2 ? TEXT("P_acess_cinto_bolsas") : TEXT("P_acess_bolsa_lateral"));
            Look = FVortexLookCatalog::Get().Normalized(Elf);
            UE_LOG(LogTemp, Display, TEXT("VORTEX_ELF_MENU %d pele %s orelha %s olhos %s traje %s"), Index, *Look.Pieces.FindRef(TEXT("elfo_pele")).ToString(), *Look.Pieces.FindRef(TEXT("peca_orelha")).ToString(),
                *Look.Pieces.FindRef(TEXT("block_face")).ToString(), *Look.Pieces.FindRef(TEXT("block_outfit")).ToString());
            Go(EPage::Creator);
            ShowLookOnStage();
        }
        else FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/frontend_elfo_%d.png"), Index), true, false);
        ++Step;
        return;
    }
    // -VortexMenuTour: uma foto de cada tela do menu novo (menu_<tela>.png), sem criar nada
    static const bool bMenuShot = FParse::Param(FCommandLine::Get(), TEXT("VortexMenuTour"));
    if (bMenuShot)
    {
        static int32 Step = -1;
        static double At = 0.0;
        auto Snap = [](const TCHAR* Name)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/menu_%s.png"), Name), true, false);
        };
        if (Step < 0) { if (bStageReady && Now - ReadyAt > 3.0) { Step = 0; At = Now; } return; }
        if (Now - At < 1.6) return;
        At = Now;
        switch (Step++)
        {
        case 0: Snap(TEXT("inicio")); break;
        case 1: bCreatingWorld = false; Go(EPage::Worlds); break;
        case 2: Snap(TEXT("mundos")); break;
        case 3: bCreatingWorld = true; Go(EPage::Worlds); break;
        case 4: Snap(TEXT("novo_mundo")); break;
        case 5: bCreatingWorld = false; Go(EPage::Join); break;
        case 6: Snap(TEXT("entrar")); break;
        case 7: OptionsTab = 1; Go(EPage::Options); break;
        case 8: Snap(TEXT("opcoes")); break;
        case 9:
        {
            const TArray<FVortexWorldSummary> Worlds = Owner.IsValid() ? Owner->GetWorlds() : TArray<FVortexWorldSummary>();
            if (Worlds.Num()) OpenHeroes(Worlds[0].WorldId, true);
            break;
        }
        case 10: Snap(TEXT("herois")); break;
        case 11: Go(EPage::Multiplayer); break;
        case 12: Snap(TEXT("multiplayer")); break;
        case 13: Go(EPage::Achievements); break;
        case 14: Snap(TEXT("conquistas")); break;
        default: FPlatformMisc::RequestExit(false); break;
        }
        return;
    }
    if (ShotStep < 0) return;
    auto Shot = [](const TCHAR* Name)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/frontend_%s.png"), Name), true, false);
        UE_LOG(LogTemp, Display, TEXT("VORTEX_FRONTEND_SHOT %s"), Name);
    };
    const double T = Now - ShotAt;
    auto Next = [this]() { ++ShotStep; ShotAt = Now; };
    switch (ShotStep)
    {
    case 0: if (bStageReady && Now - ReadyAt > 3.5) { Shot(TEXT("inicio")); Next(); } break;
    case 1:
        if (T > .5)
        {
            if (!Owner.IsValid()) return;
            FString Error;
            FGuid WorldId;
            Owner->CreateWorldOnly(TEXT("Mundo da foto"), 42, WorldId, Error);
            SelectedWorldId = WorldId;
            StartCreation();
            Next();
        }
        break;
    case 2: if (T > 2.0) { Shot(TEXT("racas")); Next(); } break;
    // 01/11: só o humano liberado (as outras cartas com cadeado) e a criação é só o nome
    case 3: if (T > .3) { ChooseRace(0); Next(); } break;
    case 4: if (T > 1.15) { Shot(TEXT("revelacao")); Next(); } break;
    case 5: if (Page == EPage::Creator && T > 2.5) { Shot(TEXT("criacao_humano_escolhido")); Next(); } break;
    case 6:
        if (T > .5)
        {
            Look = FVortexLookCatalog::Get().Random(TEXT("humano"), TEXT("male"), Rng);
            Go(EPage::Creator);
            if (AVortexMenuStage* S = Stage()) S->Orbit(40.f, -12.f);
            Next();
        }
        break;
    case 7: if (T > 2.0) { Shot(TEXT("criacao_humano")); Next(); } break;
    case 8: case 9: case 10: case 11: case 12: Next(); break;
    case 13: if (T > .5) { if (HeroNameBox.IsValid()) HeroNameBox->SetText(FText::FromString(TEXT("Thorgrim"))); SaveHero(); Next(); } break;
    case 14: if (T > 1.0) { Shot(TEXT("comemoracao")); Next(); } break;
    case 15: if (Page == EPage::Heroes && T > 3.5) { Shot(TEXT("herois")); Next(); } break;
    // as páginas do multiplayer, da sala por código e das conquistas (antes ficavam sem foto)
    // (a foto sai no fim do quadro: trocar de página no mesmo quadro fotografava a página seguinte ainda invisível)
    case 16: if (T > .5) { Go(EPage::Multiplayer); Next(); } break;
    case 17: if (T > 1.5) { Shot(TEXT("multiplayer")); Next(); } break;
    case 18: if (T > .3) { bMultiplayerMode = true; bCreatingWorld = false; Go(EPage::Worlds); Next(); } break;
    case 19: if (T > 1.5) { Shot(TEXT("salas")); Next(); } break;
    case 20: if (T > .3) { Go(EPage::Join); Next(); } break;
    case 21: if (T > 1.5) { Shot(TEXT("entrar_codigo")); Next(); } break;
    case 22: if (T > .3) { Go(EPage::Achievements); Next(); } break;
    case 23: if (T > 1.8) { Shot(TEXT("conquistas")); Next(); } break;
    case 24: if (T > .3) { bMultiplayerMode = false; Go(EPage::Worlds); Next(); } break;
    case 25: if (T > 1.5) { Shot(TEXT("mundos")); Next(); } break;
    case 26:
        if (T > .5)
        {
            // limpa o mundo e os heróis da foto (antes o mundo ficava: aparecia "Mundo da foto" no Continuar do jogador)
            for (const FVortexHeroSummary& Hero : Owner->GetHeroes(SelectedWorldId)) { FString E; Owner->DeleteHero(SelectedWorldId, Hero.PlayerId, E); }
            { FString E; Owner->DeleteWorld(SelectedWorldId, E); }
            UE_LOG(LogTemp, Display, TEXT("VORTEX_FRONTEND_SHOT_DONE"));
            FPlatformMisc::RequestExit(false);
            ShotStep = -1;
        }
        break;
    default: break;
    }
}
#endif
