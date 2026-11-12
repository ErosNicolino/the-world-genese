#include "Framework/VortexPauseMenu.h"
#include "Framework/VortexIntroComic.h"
#include "Framework/VortexUiKit.h"
#include "Framework/VortexUiJuice.h"

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/VortexGameInstance.h"
#include "Framework/VortexGameMode.h"
#include "Framework/VortexPlayerSettings.h"
#include "Framework/VortexOptionsPanel.h"
#include "GameFramework/GameUserSettings.h"
#include "Online/VortexOnline.h"
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

// Namespace próprio: no unity build este arquivo divide a unidade com o menu e a janela do herói.
namespace VortexPauseUi
{
    const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor(0xF0, 0xCF, 0x7A));
    const FLinearColor GoldDeep = FLinearColor::FromSRGBColor(FColor(0xC8, 0x92, 0x3A));
    const FLinearColor Cream = FLinearColor::FromSRGBColor(FColor(0xF4, 0xEA, 0xD2));
    const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(0xA9, 0xB4, 0xC2));

    FLinearColor Alpha(const FLinearColor& C, float A) { return FLinearColor(C.R, C.G, C.B, A); }

    FSlateFontInfo Font(int32 Size, const TCHAR* Weight = TEXT("Regular"), int32 Spacing = 0)
    {
        FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(Weight, Size);
        Info.LetterSpacing = Spacing;
        return Info;
    }

    TSharedRef<STextBlock> Label(const FString& Value, int32 Size, const FLinearColor& Color, const TCHAR* Weight = TEXT("Regular"), int32 Spacing = 0)
    {
        // 24/10: títulos ("Black") na letra cartoon das janelas; o resto com contorno, para ler sobre o azul
        const bool bTitle = FCString::Strcmp(Weight, TEXT("Black")) == 0;
        return SNew(STextBlock).Text(FText::FromString(Value)).Font(bTitle ? VortexUiKit::Cartoon::Font(Size, 2) : VortexUiKit::Outlined(Font(Size, Weight, Spacing)))
            .Margin(bTitle ? VortexUiJuice::CartoonNudge(Size) : FMargin(0.f)).ColorAndOpacity(Color)
            .ShadowOffset(FVector2D(0.f, 1.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .6f));
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

    /** 0 = comum, 1 = dourado, 2 = setinha. */
    const FButtonStyle& ButtonStyle(int32 Kind)
    {
        // o kit da HUD: 0 comum, 1 dourado (principal), 2 comum
        return VortexUiKit::Button(Kind == 1 ? 1 : 0);
    }

    const FButtonStyle& OldButtonStyle(int32 Kind)
    {
        static FButtonStyle Styles[3];
        static bool bReady = false;
        if (!bReady)
        {
            bReady = true;
            auto Make = [](FButtonStyle& S, const FLinearColor& Fill, const FLinearColor& Hover, const FLinearColor& Line)
            {
                static TArray<TSharedPtr<FSlateRoundedBoxBrush>> Keep;
                Keep.Add(MakeShared<FSlateRoundedBoxBrush>(Fill, 9.f, Line, 1.f));
                Keep.Add(MakeShared<FSlateRoundedBoxBrush>(Hover, 9.f, Gold, 1.f));
                Keep.Add(MakeShared<FSlateRoundedBoxBrush>(Hover * .85f, 9.f, Gold, 2.f));
                const int32 N = Keep.Num();
                S = FButtonStyle().SetNormal(*Keep[N - 3]).SetHovered(*Keep[N - 2]).SetPressed(*Keep[N - 1])
                    .SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f, 1.f, 0.f, 0.f));
            };
            Make(Styles[0], FLinearColor(.02f, .035f, .06f, .92f), FLinearColor(.04f, .06f, .1f, .96f), Alpha(GoldDeep, .55f));
            Make(Styles[1], Alpha(GoldDeep, .95f), Alpha(Gold, 1.f), Alpha(Gold, 1.f));
            Make(Styles[2], FLinearColor(.03f, .045f, .075f, .95f), FLinearColor(.05f, .075f, .12f, 1.f), FLinearColor(0.f, 0.f, 0.f, 0.f));
        }
        return Styles[FMath::Clamp(Kind, 0, 2)];
    }

    // ---------------------------------------------------------------------------------------------
    class SVortexPauseMenu final : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SVortexPauseMenu) {}
            SLATE_ARGUMENT(TWeakObjectPtr<UVortexGameInstance>, Owner)
        SLATE_END_ARGS()

        void Construct(const FArguments& Args)
        {
            Owner = Args._Owner;
            TSharedPtr<VortexUiJuice::SCartoonFx> Fx;
            ChildSlot
            [
                SNew(SOverlay)
                + SOverlay::Slot()[SNew(SImage).Image(White()).ColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .55f))]
                + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                [
                    SNew(SBox).WidthOverride(1240.f).MaxDesiredHeight(820.f)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()[SNew(SBorder).BorderImage(VortexUiKit::Window())]
                        + SOverlay::Slot().Padding(34.f, 28.f)[SAssignNew(Body, SBox)]
                    ]
                ]
                + SOverlay::Slot()[SAssignNew(Fx, VortexUiJuice::SCartoonFx)]
            ];
            VortexUiJuice::FxLayer = Fx;
            Rebuild();
        }

        virtual bool SupportsKeyboardFocus() const override { return true; }

        virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& Event) override
        {
            if (Event.GetKey() == EKeys::Escape) return FReply::Unhandled(); // confirmação dupla centralizada no PlayerController.
            return FReply::Unhandled();
        }

    private:
        TWeakObjectPtr<UVortexGameInstance> Owner;
        TSharedPtr<SBox> Body;
        FString Status;
        TSharedPtr<SWidget> OptionsPanel;

        void Resume()
        {
            // fecha depois do quadro (o próprio widget está no meio de um evento)
            TWeakObjectPtr<UVortexGameInstance> Weak = Owner;
            FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak](float)
            {
                if (Weak.IsValid()) Weak->TogglePauseMenu();
                return false;
            }));
        }

        TSharedRef<SWidget> Action(const FString& Text, int32 Kind, TFunction<void()> Do)
        {
            return SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&ButtonStyle(Kind)).Id(FName(TEXT("pause_") + Text)).StarColor(Kind == 1 ? Gold : Cream)
                .OnClicked_Lambda([Do]() { Do(); return FReply::Handled(); })
                [SNew(SBox).HeightOverride(52.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
                    [Label(Text, 20, Cream, TEXT("Black"))]];
        }

        /** Linha de opção: nome à esquerda, "<  valor  >" à direita. */
        TSharedRef<SWidget> Option(const FString& Name, const FString& Value, TFunction<void(int32)> Step)
        {
            auto Arrow = [this, Step](const TCHAR* Glyph, int32 Direction)
            {
                return SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&ButtonStyle(2)).Stars(false).OnClicked_Lambda([this, Step, Direction]()
                {
                    Step(Direction);
                    Rebuild();
                    return FReply::Handled();
                })
                [SNew(SBox).WidthOverride(36.f).HeightOverride(36.f).HAlign(HAlign_Center).VAlign(VAlign_Center)[Label(Glyph, 16, Gold, TEXT("Bold"))]];
            };
            return SNew(SBorder).BorderImage(VortexUiKit::Row()).Padding(FMargin(16.f, 6.f, 16.f, 10.f))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[Label(Name, 15, Cream)]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Arrow(TEXT("<"), -1)]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [SNew(SBox).WidthOverride(150.f).HAlign(HAlign_Center)[Label(Value, 15, Gold, TEXT("Bold"))]]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Arrow(TEXT(">"), 1)]
            ];
        }

        TSharedRef<SWidget> Section(const FString& Title)
        {
            return SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 6.f)[Label(FText::FromString(Title).ToUpper().ToString(), 17, Gold, TEXT("Black"))];
        }

        void Rebuild()
        {
            const UVortexOnline* Online = Owner.IsValid() ? Owner->GetSubsystem<UVortexOnline>() : nullptr;

            TSharedRef<SVerticalBox> Left = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[Label(TEXT("PAUSA"), 44, Gold, TEXT("Black"))]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[Label(TEXT("Esc duas vezes para fechar as opções"), 12, Muted)]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 22.f)[Label(TEXT("O mundo segue para os outros"), 14, Cream)]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[Action(TEXT("CONTINUAR"), 1, [this]() { Resume(); })]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[Action(TEXT("A HISTÓRIA"), 0, [this]()
                {
                    // fecha a pausa e abre a HQ de abertura de novo
                    TWeakObjectPtr<UVortexGameInstance> Weak = Owner;
                    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak](float)
                    {
                        if (!Weak.IsValid()) return false;
                        Weak->TogglePauseMenu();
                        if (UWorld* World = Weak->GetWorld()) VortexIntroComic::Open(World->GetFirstPlayerController());
                        return false;
                    }));
                })]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[Action(TEXT("SALVAR MUNDO"), 0, [this]()
                {
                    UWorld* World = Owner.IsValid() ? Owner->GetWorld() : nullptr;
                    AVortexGameMode* Mode = World ? World->GetAuthGameMode<AVortexGameMode>() : nullptr;
                    Status = Mode && Mode->SaveActiveWorld() ? TEXT("Mundo salvo.") : TEXT("Só o anfitrião salva o mundo.");
                    Rebuild();
                })]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[Action(TEXT("VOLTAR AO MENU"), 0, [this]()
                {
                    if (Owner.IsValid()) Owner->ReturnToMenu();
                })]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)[Label(Status, 14, Gold)];
            if (Online && Online->HasRoom())
            {
                Left->AddSlot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
                [
                    SNew(SBorder).BorderImage(Round(12.f)).BorderBackgroundColor(Alpha(GoldDeep, .18f)).Padding(FMargin(16.f, 12.f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()[Label(TEXT("SALA ONLINE"), 12, Gold, TEXT("Bold"), 200)]
                        + SVerticalBox::Slot().AutoHeight()[Label(Online->GetRoomCode(), 34, Cream, TEXT("Black"), 400)]
                        + SVerticalBox::Slot().AutoHeight()[Label(TEXT("Passe este código para os amigos"), 12, Muted)]
                    ]
                ];
            }

            // opções em abas, as mesmas do menu inicial (Framework/VortexOptionsPanel); montadas uma vez
            // só, para a aba e a rolagem não voltarem ao topo quando o lado esquerdo muda
            if (!OptionsPanel) OptionsPanel = MakeVortexOptionsPanel(nullptr, 560.f);
            TSharedRef<SWidget> Right = OptionsPanel.ToSharedRef();
            Body->SetContent(
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(330.f)[Left]]
                + SHorizontalBox::Slot().AutoWidth().Padding(28.f, 0.f)
                [SNew(SBox).WidthOverride(1.f)[SNew(SImage).Image(White()).ColorAndOpacity(Alpha(GoldDeep, .35f))]]
                + SHorizontalBox::Slot().FillWidth(1.f)[Right]);
        }
    };
}

TSharedRef<SWidget> MakeVortexPauseMenu(UVortexGameInstance* Owner)
{
    return SNew(VortexPauseUi::SVortexPauseMenu).Owner(Owner);
}
