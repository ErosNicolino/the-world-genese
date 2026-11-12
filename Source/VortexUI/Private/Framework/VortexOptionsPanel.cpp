#include "Framework/VortexOptionsPanel.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/VortexUiKit.h"
#include "Framework/VortexUiJuice.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/VortexGraphics.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

// Namespace próprio: no unity build este arquivo divide a unidade com os menus.
namespace VortexOptionsUi
{
    const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor(0xF0, 0xCF, 0x7A));
    const FLinearColor GoldDeep = FLinearColor::FromSRGBColor(FColor(0xC8, 0x92, 0x3A));
    const FLinearColor Cream = FLinearColor::FromSRGBColor(FColor(0xF4, 0xEA, 0xD2));
    const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(0xA9, 0xB4, 0xC2));

    FLinearColor Alpha(const FLinearColor& C, float A) { return FLinearColor(C.R, C.G, C.B, A); }
    double Now() { return FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetCurrentTime() : 0.0; }
    float Ease(float T) { T = FMath::Clamp(T, 0.f, 1.f); return 1.f - FMath::Pow(1.f - T, 3.f); }

    FSlateFontInfo Font(int32 Size, const TCHAR* Weight = TEXT("Regular"), int32 Spacing = 0)
    {
        FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(Weight, Size);
        Info.LetterSpacing = Spacing;
        return Info;
    }

    const FSlateBrush* Round(float Radius)
    {
        static TMap<int32, TSharedPtr<FSlateRoundedBoxBrush>> Cache;
        TSharedPtr<FSlateRoundedBoxBrush>& Brush = Cache.FindOrAdd(FMath::RoundToInt(Radius));
        if (!Brush) Brush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor::White, Radius);
        return Brush.Get();
    }

    /** 0 = seta, 1 = valor, 2 = aba, 3 = botão de rodapé. */
    const FButtonStyle& Style(int32 Kind)
    {
        // o kit da HUD: 0 e 1 botões, 2 aba, 3 botão de ação
        return VortexUiKit::Button(Kind == 2 ? 2 : 0);
    }

    const FButtonStyle& OldStyle(int32 Kind)
    {
        static FButtonStyle Styles[4];
        static bool bReady = false;
        if (!bReady)
        {
            bReady = true;
            static TArray<TSharedPtr<FSlateRoundedBoxBrush>> Keep;
            auto Make = [](FButtonStyle& S, const FLinearColor& Fill, const FLinearColor& Hover, const FLinearColor& Line, const FLinearColor& HoverLine)
            {
                Keep.Add(MakeShared<FSlateRoundedBoxBrush>(Fill, 8.f, Line, 1.f));
                Keep.Add(MakeShared<FSlateRoundedBoxBrush>(Hover, 8.f, HoverLine, 1.f));
                Keep.Add(MakeShared<FSlateRoundedBoxBrush>(Hover * .85f, 8.f, HoverLine, 2.f));
                const int32 N = Keep.Num();
                S = FButtonStyle().SetNormal(*Keep[N - 3]).SetHovered(*Keep[N - 2]).SetPressed(*Keep[N - 1])
                    .SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f, 1.f, 0.f, 0.f));
            };
            Make(Styles[0], FLinearColor(.03f, .045f, .075f, .95f), FLinearColor(.07f, .09f, .14f, 1.f), FLinearColor(0.f, 0.f, 0.f, 0.f), Alpha(Gold, .8f));
            Make(Styles[1], FLinearColor(.02f, .035f, .06f, .95f), FLinearColor(.05f, .07f, .11f, 1.f), Alpha(GoldDeep, .55f), Gold);
            Make(Styles[2], FLinearColor(0.f, 0.f, 0.f, 0.f), FLinearColor(1.f, 1.f, 1.f, .05f), FLinearColor(0.f, 0.f, 0.f, 0.f), FLinearColor(0.f, 0.f, 0.f, 0.f));
            Make(Styles[3], FLinearColor(.02f, .035f, .06f, .9f), FLinearColor(.05f, .07f, .11f, 1.f), Alpha(Muted, .35f), Gold);
        }
        return Styles[FMath::Clamp(Kind, 0, 3)];
    }

    class SVortexOptionsPanel final : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SVortexOptionsPanel) {}
            SLATE_ARGUMENT(TFunction<void()>, OnClick)
            SLATE_ARGUMENT(float, BodyHeight)
            SLATE_ARGUMENT(int32, InitialTab)
        SLATE_END_ARGS()

        virtual ~SVortexOptionsPanel() override { VortexGraphics::EndEditing(); }

        void Construct(const FArguments& Args)
        {
            OnClick = Args._OnClick;
            VortexGraphics::BeginEditing();
            const TArray<FString>& Tabs = VortexGraphics::Tabs();
            TSharedRef<SHorizontalBox> TabBar = SNew(SHorizontalBox);
            for (int32 i = 0; i < Tabs.Num(); ++i)
            {
                TabBar->AddSlot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
                [
                    SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&Style(2)).ContentPadding(FMargin(16.f, 8.f, 16.f, 0.f))
                    .OnClicked_Lambda([this, i]() { SelectTab(i); return FReply::Handled(); })
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                        [
                            SNew(STextBlock).Text(FText::FromString(FText::FromString(Tabs[i]).ToUpper().ToString())).Font(Font(14, TEXT("Bold"), 140))
                            .ColorAndOpacity_Lambda([this, i]() { return FSlateColor(i == Tab ? Gold : Muted); })
                        ]
                        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 7.f, 0.f, 0.f)
                        [
                            // sublinhado que cresce na aba escolhida
                            SNew(SBox).HeightOverride(3.f)
                            .WidthOverride_Lambda([this, i]() { return i == Tab ? FOptionalSize(18.f + 70.f * Ease(float(Now() - TabAt) / .25f)) : FOptionalSize(0.f); })
                            [SNew(SImage).Image(Round(2.f)).ColorAndOpacity(Gold)]
                        ]
                    ]
                ];
            }
            ChildSlot
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[TabBar]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
                [SNew(SBox).HeightOverride(1.f)[SNew(SImage).Image(Round(0.f)).ColorAndOpacity(Alpha(GoldDeep, .35f))]]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SBox).HeightOverride(Args._BodyHeight)
                    [
                        SAssignNew(Scroll, SScrollBox)
                        .RenderOpacity(1.f)
                        + SScrollBox::Slot().Padding(0.f, 0.f, 12.f, 0.f)
                        [
                            SAssignNew(Rows, SVerticalBox)
                            .RenderTransform_Lambda([this]() { return FSlateRenderTransform(FVector2f(24.f * (1.f - Ease(float(Now() - TabAt) / .3f)), 0.f)); })
                        ]
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth()
                    [
                        SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&Style(3)).ContentPadding(FMargin(16.f, 8.f))
                        .OnClicked_Lambda([this]()
                        {
                            VortexGraphics::ResetTab(VortexGraphics::Tabs()[Tab]);
                            Click();
                            Refresh();
                            return FReply::Handled();
                        })
                        [SNew(STextBlock).Text(FText::FromString(TEXT("Restaurar padrões desta aba"))).Font(Font(13, TEXT("Bold"))).ColorAndOpacity(Cream)]
                    ]
                    + SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
                    [
                        SNew(STextBlock).Font(Font(12)).ColorAndOpacity(Muted)
                        .Text_Lambda([]() { return FText::FromString(VortexGraphics::HasPending() ? TEXT("Mudanças esperando o Aplicar") : TEXT("Escolha e depois aplique")); })
                    ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
                    [
                        SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&VortexUiKit::Button(0)).ContentPadding(FMargin(18.f, 8.f))
                        .IsEnabled_Lambda([]() { return VortexGraphics::HasPending(); })
                        .OnClicked_Lambda([this]()
                        {
                            VortexGraphics::DiscardPending();
                            Click();
                            Refresh();
                            return FReply::Handled();
                        })
                        [SNew(STextBlock).Text(FText::FromString(TEXT("DESCARTAR"))).Font(Font(13, TEXT("Bold"), 80)).ColorAndOpacity(Cream)]
                    ]
                    + SHorizontalBox::Slot().AutoWidth()
                    [
                        SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&VortexUiKit::Button(1)).ContentPadding(FMargin(26.f, 8.f))
                        .IsEnabled_Lambda([]() { return VortexGraphics::HasPending(); })
                        .OnClicked_Lambda([this]()
                        {
                            VortexGraphics::ApplyPending();
                            Click();
                            Refresh();
                            return FReply::Handled();
                        })
                        [SNew(STextBlock).Text(FText::FromString(TEXT("APLICAR"))).Font(Font(14, TEXT("Bold"), 120))
                            .ColorAndOpacity_Lambda([]() { return FSlateColor(VortexGraphics::HasPending() ? FLinearColor(.08f, .05f, .02f) : Alpha(Muted, .6f)); })]
                    ]
                ]
                ]
                // 11/11 (o usuário: "tem que ter o loading para carregar tudo certinho sem aparecer travando"): do Aplicar
                // até o mundo remontar, o painel fica coberto e não aceita cliques; o motor só troca tudo depois que isto
                // já está na tela (VortexGraphics::ApplyPending espera 2 quadros)
                + SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
                [
                    SNew(SBorder).BorderImage(Round(12.f)).BorderBackgroundColor(FLinearColor(.01f, .015f, .03f, .88f))
                    .Visibility_Lambda([]() { return VortexGraphics::IsApplying() ? EVisibility::Visible : EVisibility::Collapsed; })
                    .HAlign(HAlign_Center).VAlign(VAlign_Center)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                        [
                            SNew(STextBlock).Font(Font(22, TEXT("Bold"), 160)).ColorAndOpacity(Gold)
                            .Text_Lambda([]() { return FText::FromString(FString(TEXT("APLICANDO")) + FString::ChrN(1 + int32(FPlatformTime::Seconds() * 2.0) % 3, TEXT('.'))); })
                        ]
                        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 10.f, 0.f, 0.f)
                        [
                            SNew(STextBlock).Font(Font(13)).ColorAndOpacity(Cream)
                            .Text(FText::FromString(TEXT("Montando o mundo de novo com as opções novas. Leva alguns segundos.")))
                        ]
                    ]
                ]
            ];
            SelectTab(FMath::Clamp(Args._InitialTab, 0, Tabs.Num() - 1), false);
        }

    private:
        TFunction<void()> OnClick;
        TSharedPtr<SScrollBox> Scroll;
        TSharedPtr<SVerticalBox> Rows;
        TArray<VortexGraphics::FOption> Cache;   // a lista mostrada (as linhas leem dela)
        int32 Tab = 0;
        double TabAt = -10.0, ChangedAt = -10.0;
        FString ChangedName;

        void Click() { if (OnClick) OnClick(); }

        void SelectTab(int32 NewTab, bool bSound = true)
        {
            if (bSound && NewTab != Tab) Click();
            Tab = NewTab;
            TabAt = Now();
            Rebuild(true);
        }

        void Change(int32 Index, int32 Direction)
        {
            if (!Cache.IsValidIndex(Index)) return;
            if (Cache[Index].Step) Cache[Index].Step(Direction);
            ChangedName = Cache[Index].Name;
            ChangedAt = Now();
            Click();
            Refresh();
        }

        /**
         * Lê os valores de novo sem refazer as linhas (elas mostram a lista guardada): a rolagem, o mouse e
         * as animações ficam. Só refaz quando a lista muda de forma (uma opção aparece ou some).
         */
        void Refresh()
        {
            TArray<VortexGraphics::FOption> Fresh = VortexGraphics::Options(VortexGraphics::Tabs()[Tab]);
            bool bSameShape = Fresh.Num() == Cache.Num();
            for (int32 i = 0; bSameShape && i < Fresh.Num(); ++i) bSameShape = Fresh[i].Name == Cache[i].Name && Fresh[i].Count == Cache[i].Count;
            if (!bSameShape) { Rebuild(false); return; }
            Cache = MoveTemp(Fresh);
        }

        FString ValueOf(int32 Index) const { return Cache.IsValidIndex(Index) ? Cache[Index].Value : FString(); }
        int32 IndexOf(int32 Index) const { return Cache.IsValidIndex(Index) ? Cache[Index].Index : 0; }

        void Rebuild(bool bTop)
        {
            const float Offset = Scroll.IsValid() ? Scroll->GetScrollOffset() : 0.f;
            Rows->ClearChildren();
            Cache = VortexGraphics::Options(VortexGraphics::Tabs()[Tab]);
            FString Section;
            for (int32 Index = 0; Index < Cache.Num(); ++Index)
            {
                const VortexGraphics::FOption& Item = Cache[Index];
                if (Item.Section != Section)
                {
                    Section = Item.Section;
                    // a seção na faixa da HUD
                    Rows->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0.f, Rows->NumSlots() ? 16.f : 2.f, 0.f, 6.f)
                    [
                        SNew(SBorder).BorderImage(VortexUiKit::Banner()).Padding(FMargin(26.f, 5.f)).HAlign(HAlign_Center)
                        [SNew(STextBlock).Text(FText::FromString(FText::FromString(Section).ToUpper().ToString())).Font(VortexUiKit::Outlined(Font(13, TEXT("Bold"), 220))).ColorAndOpacity(Gold)]
                    ];
                }
                Rows->AddSlot().AutoHeight().Padding(0.f, 3.f)[Row(Item, Index)];
            }
            if (Scroll.IsValid()) Scroll->SetScrollOffset(bTop ? 0.f : Offset);
        }

        TSharedRef<SWidget> Arrow(const TCHAR* Glyph, int32 Index, int32 Direction)
        {
            return SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&Style(0)).OnClicked_Lambda([this, Index, Direction]() { Change(Index, Direction); return FReply::Handled(); })
                [SNew(SBox).WidthOverride(38.f).HeightOverride(38.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
                    [SNew(STextBlock).Text(FText::FromString(Glyph)).Font(VortexUiKit::Outlined(Font(18, TEXT("Bold")))).ColorAndOpacity(Cream)]];
        }

        TSharedRef<SWidget> Pips(const VortexGraphics::FOption& Item, int32 Index)
        {
            if (Item.Count < 2 || Item.Count > 11) return SNullWidget::NullWidget;
            TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);
            const int32 Count = Item.Count;
            const float Width = FMath::Min(22.f, 150.f / Count - 3.f);
            for (int32 i = 0; i < Count; ++i)
            {
                Box->AddSlot().AutoWidth().Padding(1.5f, 0.f)
                [
                    SNew(SBox).WidthOverride(Width).HeightOverride(4.f)
                    [
                        SNew(SImage).Image(Round(2.f)).ColorAndOpacity_Lambda([this, Index, Count, i]()
                        {
                            const int32 Current = IndexOf(Index);
                            const bool bOn = Count == 2 ? i == 1 && Current == 1 : i <= Current;
                            return FSlateColor(bOn ? Gold : Alpha(Muted, .25f));
                        })
                    ]
                ];
            }
            return Box;
        }

        TSharedRef<SWidget> Row(const VortexGraphics::FOption& Item, int32 Index)
        {
            TSharedPtr<SBorder> Border;
            const FString Name = Item.Name;
            // a linha com relevo do kit; acesa (borda dourada) sob o mouse
            SAssignNew(Border, SBorder).Padding(FMargin(0.f));
            TWeakPtr<SBorder> Weak = Border;
            Border->SetBorderImage(TAttribute<const FSlateBrush*>::CreateLambda([Weak]()
            {
                const TSharedPtr<SBorder> B = Weak.Pin();
                return VortexUiKit::Row(B && B->IsHovered());
            }));
            Border->SetContent(
                SNew(SHorizontalBox)
                // filete dourado à esquerda da linha sob o mouse
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SBox).WidthOverride(3.f).Padding(FMargin(0.f, 10.f))
                    [SNew(SImage).Image(Round(1.5f)).ColorAndOpacity_Lambda([Weak]()
                    {
                        const TSharedPtr<SBorder> B = Weak.Pin();
                        return FSlateColor(B && B->IsHovered() ? Gold : FLinearColor::Transparent);
                    })]
                ]
                + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(14.f, 10.f, 12.f, 10.f)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Item.Name)).Font(VortexUiKit::Outlined(Font(16, TEXT("Bold")))).ColorAndOpacity(Cream)]
                    + SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 0.f)
                    [SNew(STextBlock).Text(FText::FromString(Item.Description)).Font(Font(12)).ColorAndOpacity(FLinearColor::FromSRGBColor(FColor(0xC8, 0xD8, 0xF6))).AutoWrapText(true)]
                ]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 6.f, 10.f, 6.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Arrow(TEXT("‹"), Index, -1)]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f)
                    [
                        SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&Style(1)).OnClicked_Lambda([this, Index]() { Change(Index, 1); return FReply::Handled(); })
                        [
                            SNew(SBox).WidthOverride(190.f).HeightOverride(44.f).VAlign(VAlign_Center)
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                                [
                                    SNew(STextBlock).Text_Lambda([this, Index]() { return FText::FromString(ValueOf(Index)); }).Font(VortexUiKit::Outlined(Font(15, TEXT("Bold"))))
                                    .ColorAndOpacity_Lambda([this, Name]()
                                    {
                                        // o valor que acabou de mudar acende e volta ao creme
                                        const float T = Name == ChangedName ? FMath::Clamp(float(Now() - ChangedAt) / .6f, 0.f, 1.f) : 1.f;
                                        return FSlateColor(FMath::Lerp(Gold, Cream, Ease(T)));
                                    })
                                    .RenderTransformPivot(FVector2D(.5f, .5f))
                                    .RenderTransform_Lambda([this, Name]()
                                    {
                                        const float T = Name == ChangedName ? FMath::Clamp(float(Now() - ChangedAt) / .25f, 0.f, 1.f) : 1.f;
                                        return FSlateRenderTransform(1.f + .12f * (1.f - Ease(T)));
                                    })
                                ]
                                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 0.f)[Pips(Item, Index)]
                            ]
                        ]
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Arrow(TEXT("›"), Index, 1)]
                ]);
            return Border.ToSharedRef();
        }
    };
}

TSharedRef<SWidget> MakeVortexOptionsPanel(TFunction<void()> OnClick, float BodyHeight, int32 InitialTab)
{
    return SNew(VortexOptionsUi::SVortexOptionsPanel).OnClick(OnClick).BodyHeight(BodyHeight).InitialTab(InitialTab);
}
