#include "Framework/VortexSocialUi.h"
#include "VortexUiImpl.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexRpg.h"
#include "Rpg/VortexSocial.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/VortexUiKit.h"
#include "Framework/VortexUiJuice.h"

namespace VortexSocialStyle
{
    const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor(0xF0, 0xCF, 0x7A));
    const FLinearColor GoldDeep = FLinearColor::FromSRGBColor(FColor(0xC8, 0x92, 0x3A));
    const FLinearColor Cream = FLinearColor::FromSRGBColor(FColor(0xF4, 0xEA, 0xD2));
    const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(0xA9, 0xB4, 0xC2));
    const FLinearColor Green = FLinearColor::FromSRGBColor(FColor(0x8F, 0xE3, 0x8A));

    // 24/10: o kit cartoon (as letras grandes em desenho animado, as outras com contorno para ler sobre o azul)
    FSlateFontInfo SocialFont(int32 Size, const TCHAR* Weight = TEXT("Regular"))
    {
        return Size >= 20 ? VortexUiKit::Cartoon::Font(Size, 2) : VortexUiKit::Outlined(FCoreStyle::GetDefaultFontStyle(Weight, Size));
    }

    const FSlateBrush* SocialRounded(float Radius, const FLinearColor& Fill, const FLinearColor& Line, float Width)
    {
        static TArray<TSharedPtr<FSlateRoundedBoxBrush>> Keep;
        Keep.Add(MakeShared<FSlateRoundedBoxBrush>(Fill, Radius, Line, Width));
        return Keep.Last().Get();
    }

    const FButtonStyle& SocialButton()
    {
        return VortexUiKit::Cartoon::Button(0);
    }

    const FButtonStyle& OldSocialButton()
    {
        static FButtonStyle Style = []()
        {
            FButtonStyle S;
            S.SetNormal(*SocialRounded(6.f, FLinearColor(.02f, .035f, .06f, .92f), FLinearColor(GoldDeep.R, GoldDeep.G, GoldDeep.B, .4f), 1.f));
            S.SetHovered(*SocialRounded(6.f, FLinearColor(.06f, .08f, .12f, .97f), Gold, 1.f));
            S.SetPressed(*SocialRounded(6.f, FLinearColor(.04f, .06f, .09f, .97f), Gold, 2.f));
            S.SetDisabled(*SocialRounded(6.f, FLinearColor(.02f, .02f, .03f, .7f), FLinearColor(.3f, .3f, .3f, .3f), 1.f));
            S.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f, 1.f, 0.f, 0.f));
            return S;
        }();
        return Style;
    }

    const FSlateBrush* SocialIcon(FName Icon, float Size)
    {
        static TMap<TPair<FName, int32>, TSharedPtr<FSlateBrush>> Cache;
        TSharedPtr<FSlateBrush>& Brush = Cache.FindOrAdd({ Icon, FMath::RoundToInt(Size) });
        if (!Brush)
        {
            Brush = MakeShared<FSlateBrush>();
            if (UTexture2D* Texture = FVortexRpgCatalog::Icon(Icon)) Brush->SetResourceObject(Texture);
            Brush->ImageSize = FVector2D(Size, Size);
            Brush->DrawAs = ESlateBrushDrawType::Image;
        }
        return Brush.Get();
    }

    TSharedRef<SWidget> Panel(const TSharedRef<SWidget>& Content)
    {
        return SNew(SBorder).BorderImage(VortexUiKit::Cartoon::Frame()).Padding(FMargin(24.f, 22.f))[Content];
    }
}

namespace VortexSocialUiImpl
{
    using namespace VortexSocialStyle;
    TSharedPtr<SWidget> Window;
    bool bTradeWindow = false;
    TWeakObjectPtr<APlayerController> Owner;

    void Show(AVortexCharacter* Hero, const TSharedRef<SWidget>& Content)
    {
        Close();
        APlayerController* Controller = Hero ? Cast<APlayerController>(Hero->GetController()) : nullptr;
        if (!Controller || !GEngine || !GEngine->GameViewport) return;
        Window = Content;
        Owner = Controller;
        GEngine->GameViewport->AddViewportWidgetContent(Window.ToSharedRef(), 45);
        Controller->SetShowMouseCursor(true);
        FInputModeGameAndUI Mode;
        Mode.SetWidgetToFocus(Window);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        Controller->SetInputMode(Mode);
        VortexAudio::Play2D(Hero, TEXT("ui_open"), .5f);
    }

    bool IsOpen() { return Window.IsValid(); }

    void Close()
    {
        if (!Window.IsValid()) return;
        if (GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Window.ToSharedRef());
        Window.Reset();
        bTradeWindow = false;
        if (APlayerController* Controller = Owner.Get())
        {
            Controller->SetShowMouseCursor(false);
            Controller->SetInputMode(FInputModeGameOnly());
        }
        Owner.Reset();
    }

    AVortexCharacter* FindLookedAt(const AVortexCharacter* Hero)
    {
        const APlayerController* Controller = Hero ? Cast<APlayerController>(Hero->GetController()) : nullptr;
        if (!Controller || !Controller->PlayerCameraManager) return nullptr;
        const FVector Eye = Controller->PlayerCameraManager->GetCameraLocation();
        const FVector Look = Controller->PlayerCameraManager->GetCameraRotation().Vector();
        AVortexCharacter* Best = nullptr;
        // 03/11: só colado (1,9 m) e de frente para ele (antes 3,2 m e quase de lado: perto de um amigo o E sempre abria
        // a conversa com ele, e não dava para falar com ninguém nem pegar nada)
        float BestDot = .97f;
        for (TActorIterator<AVortexCharacter> It(Hero->GetWorld()); It; ++It)
        {
            AVortexCharacter* Other = *It;
            if (Other == Hero || !Other->IsPlayerControlled() || FVector::Dist(Other->GetActorLocation(), Hero->GetActorLocation()) > 190.f) continue;
            if (FVector::DotProduct(Hero->GetActorForwardVector().GetSafeNormal2D(), (Other->GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D()) < .6f) continue;
            // o corpo inteiro conta: pés, peito ou cabeça
            float Dot = -1.f;
            for (const float H : { -60.f, 0.f, 60.f })
                Dot = FMath::Max(Dot, FVector::DotProduct((Other->GetActorLocation() + FVector(0, 0, H) - Eye).GetSafeNormal(), Look));
            if (Dot > BestDot) { BestDot = Dot; Best = Other; }
        }
        return Best;
    }

    // ---- menu de interação -------------------------------------------------------------------------
    class SSocialMenu final : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SSocialMenu) {}
            SLATE_ARGUMENT(TWeakObjectPtr<AVortexCharacter>, Hero)
            SLATE_ARGUMENT(TWeakObjectPtr<AVortexCharacter>, Other)
        SLATE_END_ARGS()

        void Construct(const FArguments& Args)
        {
            Hero = Args._Hero;
            Other = Args._Other;
            UVortexSocial* Social = Hero.IsValid() ? Hero->GetSocial() : nullptr;
            const UVortexSocial* Them = Other.IsValid() ? Other->GetSocial() : nullptr;
            TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
            const int32 Level = Other.IsValid() && Other->GetSheet() ? Other->GetSheet()->GetLevel() : 1;
            List->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
                [SNew(STextBlock).Text(FText::FromString(Them ? Them->GetHeroName() : FString())).Font(SocialFont(22, TEXT("Bold"))).ColorAndOpacity(Gold)];
            List->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
                [SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("Nível %d%s"), Level, Social && Social->SameParty(Other.Get()) ? TEXT("  ·  do seu grupo") : TEXT("")))).Font(SocialFont(14)).ColorAndOpacity(Muted)];
            if (Social)
                for (const UVortexSocial::FMenuItem& Item : Social->MenuFor(Other.Get()))
                {
                    const FName Action = Item.Action;
                    List->AddSlot().AutoHeight().Padding(0.f, 3.f)
                    [
                        SNew(SBox).WidthOverride(320.f).HeightOverride(42.f)
                        [
                            SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&SocialButton()).IsEnabled(Item.bEnabled && !Action.IsNone())
                            .OnClicked_Lambda([this, Action]()
                            {
                                if (Hero.IsValid() && Other.IsValid() && Hero->GetSocial()) Hero->GetSocial()->Act(Other.Get(), Action);
                                Close();
                                return FReply::Handled();
                            })
                            [
                                SNew(SBox).Padding(FMargin(14.f, 0.f)).VAlign(VAlign_Center)
                                [SNew(STextBlock).Text(FText::FromString(Item.Label)).Font(SocialFont(17)).ColorAndOpacity(Item.bEnabled ? Cream : Muted)]
                            ]
                        ]
                    ];
                }
            List->AddSlot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
                [SNew(STextBlock).Text(FText::FromString(TEXT("Esc fecha"))).Font(SocialFont(12)).ColorAndOpacity(Muted)];
            ChildSlot.HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(0.f, 0.f, 120.f, 0.f)[Panel(List)];
        }
        virtual bool SupportsKeyboardFocus() const override { return true; }
        virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Key) override
        {
            if (Key.GetKey() == EKeys::Escape) return FReply::Unhandled(); // Esc é centralizado no PlayerController.
            if (Key.GetKey() == EKeys::E) { Close(); return FReply::Handled(); }
            return FReply::Unhandled();
        }
        virtual void Tick(const FGeometry& Geometry, double Time, float Delta) override
        {
            SCompoundWidget::Tick(Geometry, Time, Delta);
            // longe demais: fecha
            if (!Hero.IsValid() || !Other.IsValid() || FVector::Dist(Hero->GetActorLocation(), Other->GetActorLocation()) > UVortexSocial::InteractRange * 2.f) Close();
        }
    private:
        TWeakObjectPtr<AVortexCharacter> Hero, Other;
    };

    void OpenMenu(AVortexCharacter* Hero, AVortexCharacter* Other)
    {
        if (!Hero || !Other || !Hero->GetSocial()) return;
        Show(Hero, SNew(SSocialMenu).Hero(Hero).Other(Other));
    }

    // ---- troca -------------------------------------------------------------------------------------
    class STradeWindow final : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(STradeWindow) {}
            SLATE_ARGUMENT(TWeakObjectPtr<AVortexCharacter>, Hero)
        SLATE_END_ARGS()

        void Construct(const FArguments& Args)
        {
            Hero = Args._Hero;
            ChildSlot.HAlign(HAlign_Center).VAlign(VAlign_Center)[SAssignNew(Body, SBox)];
            Rebuild();
        }
        virtual bool SupportsKeyboardFocus() const override { return true; }
        virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Key) override
        {
            if (Key.GetKey() == EKeys::Escape) { Cancel(); return FReply::Handled(); }
            return FReply::Unhandled();
        }
        virtual void Tick(const FGeometry& Geometry, double Time, float Delta) override
        {
            SCompoundWidget::Tick(Geometry, Time, Delta);
            const FString Now = Signature();
            if (Now != Seen) Rebuild();
        }
    private:
        TWeakObjectPtr<AVortexCharacter> Hero;
        TSharedPtr<SBox> Body;
        FString Seen;
        // o que o herói está montando (vai inteiro ao servidor a cada mudança)
        TMap<FName, int32> Draft;
        int32 DraftCoins = 0;

        UVortexSocial* Social() const { return Hero.IsValid() ? Hero->GetSocial() : nullptr; }

        FString Signature() const
        {
            const UVortexSocial* S = Social();
            if (!S) return FString();
            auto Offer = [](const FVortexTradeOffer& O)
            {
                FString Out = FString::Printf(TEXT("%d%d|"), O.Coins, O.bReady ? 1 : 0);
                for (const FVortexItemStack& I : O.Items) Out += FString::Printf(TEXT("%s%d,"), *I.Item.ToString(), I.Count);
                return Out;
            };
            FString Bag;
            if (const UVortexHeroSheet* Sheet = Hero->GetSheet()) Bag = FString::FromInt(Sheet->GetRevision());
            return Offer(S->GetMyOffer()) + TEXT("#") + Offer(S->GetTheirOffer()) + TEXT("#") + Bag;
        }

        void Send()
        {
            UVortexSocial* S = Social();
            if (!S) return;
            TArray<FVortexItemStack> Items;
            for (const TPair<FName, int32>& Pair : Draft) if (Pair.Value > 0) { FVortexItemStack I; I.Item = Pair.Key; I.Count = Pair.Value; Items.Add(I); }
            S->ServerTradeOffer(Items, DraftCoins);
        }

        void Cancel()
        {
            if (UVortexSocial* S = Social()) S->ServerAct(S->GetTradePartner(), TEXT("trade_cancel"));
            Close();
        }

        TSharedRef<SWidget> ItemSlot(FName Item, int32 Count, bool bClickable, TFunction<void()> Click)
        {
            const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Item);
            TSharedRef<SWidget> Face = SNew(SOverlay)
                + SOverlay::Slot()[SNew(SImage).Image(SocialIcon(Def ? Def->Icon : Item, 44.f))]
                + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom)
                    [SNew(STextBlock).Text(FText::FromString(Count > 1 ? FString::FromInt(Count) : FString())).Font(SocialFont(12, TEXT("Bold"))).ColorAndOpacity(Cream).ShadowOffset(FVector2D(1, 1))];
            return SNew(SBox).WidthOverride(56.f).HeightOverride(56.f).ToolTipText(FText::FromString(Def ? Def->Name : Item.ToString()))
            [
                SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&SocialButton()).IsEnabled(bClickable).HAlign(HAlign_Center).VAlign(VAlign_Center)
                .OnClicked_Lambda([Click]() { if (Click) Click(); return FReply::Handled(); })[Face]
            ];
        }

        TSharedRef<SWidget> Coins(int32 Amount, const FLinearColor& Color)
        {
            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)[SNew(SImage).Image(SocialIcon(TEXT("ui_coin"), 22.f))]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d moedas"), Amount))).Font(SocialFont(16, TEXT("Bold"))).ColorAndOpacity(Color)];
        }

        TSharedRef<SWidget> TextButton(const FString& Label, float Width, TFunction<void()> Click, bool bEnabled = true)
        {
            return SNew(SBox).WidthOverride(Width).HeightOverride(40.f)
            [
                SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&SocialButton()).IsEnabled(bEnabled).HAlign(HAlign_Center).VAlign(VAlign_Center)
                .OnClicked_Lambda([Click]() { Click(); return FReply::Handled(); })
                [SNew(STextBlock).Text(FText::FromString(Label)).Font(SocialFont(15, TEXT("Bold"))).ColorAndOpacity(Cream)]
            ];
        }

        void Rebuild()
        {
            Seen = Signature();
            UVortexSocial* S = Social();
            const UVortexHeroSheet* Sheet = Hero.IsValid() ? Hero->GetSheet() : nullptr;
            if (!S || !Sheet || !Body.IsValid()) return;
            const UVortexSocial* Them = S->GetTradePartner() ? S->GetTradePartner()->GetSocial() : nullptr;
            // a mochila: o que ainda não está na mesa
            TSharedRef<SWrapBox> Bag = SNew(SWrapBox).UseAllottedSize(true);
            TMap<FName, int32> Left;
            for (const FVortexItemStack& Stack : Sheet->GetBag()) if (!Stack.IsEmpty()) Left.FindOrAdd(Stack.Item) += Stack.Count;
            for (const TPair<FName, int32>& Pair : Draft) if (int32* L = Left.Find(Pair.Key)) *L -= Pair.Value;
            for (const TPair<FName, int32>& Pair : Left)
            {
                if (Pair.Value <= 0) continue;
                const FName Item = Pair.Key;
                Bag->AddSlot().Padding(3.f)[ItemSlot(Item, Pair.Value, true, [this, Item]() { Draft.FindOrAdd(Item) += 1; Send(); Rebuild(); })];
            }
            // a minha oferta (clique tira 1) e a dele
            TSharedRef<SWrapBox> Mine = SNew(SWrapBox).UseAllottedSize(true);
            for (const TPair<FName, int32>& Pair : Draft)
            {
                if (Pair.Value <= 0) continue;
                const FName Item = Pair.Key;
                Mine->AddSlot().Padding(3.f)[ItemSlot(Item, Pair.Value, true, [this, Item]() { if (int32* C = Draft.Find(Item)) *C = FMath::Max(0, *C - 1); Send(); Rebuild(); })];
            }
            TSharedRef<SWrapBox> Theirs = SNew(SWrapBox).UseAllottedSize(true);
            for (const FVortexItemStack& Stack : S->GetTheirOffer().Items) Theirs->AddSlot().Padding(3.f)[ItemSlot(Stack.Item, Stack.Count, false, nullptr)];
            auto CoinStep = [this, Sheet](int32 Step) { DraftCoins = FMath::Clamp(DraftCoins + Step, 0, Sheet->GetCoins()); Send(); Rebuild(); };
            const bool bMineReady = S->GetMyOffer().bReady, bTheirReady = S->GetTheirOffer().bReady;
            auto Column = [&](const FString& Title, const TSharedRef<SWidget>& Content, float Width)
            {
                return SNew(SBox).WidthOverride(Width)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[SNew(STextBlock).Text(FText::FromString(Title)).Font(SocialFont(16, TEXT("Bold"))).ColorAndOpacity(Gold)]
                    + SVerticalBox::Slot().FillHeight(1.f)
                    [
                        SNew(SBorder).BorderImage(SocialRounded(6.f, FLinearColor(0.f, 0.f, 0.f, .35f), FLinearColor(GoldDeep.R, GoldDeep.G, GoldDeep.B, .25f), 1.f)).Padding(6.f)
                        [SNew(SScrollBox) + SScrollBox::Slot()[Content]]
                    ]
                ];
            };
            Body->SetContent(Panel(
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 12.f)
                    [SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("TROCA COM %s"), *(Them ? Them->GetHeroName() : FString(TEXT("?"))).ToUpper()))).Font(SocialFont(22, TEXT("Bold"))).ColorAndOpacity(Gold)]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SBox).HeightOverride(360.f)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 14.f, 0.f)[Column(TEXT("Sua mochila"), Bag, 260.f)]
                        + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 14.f, 0.f)[Column(bMineReady ? TEXT("Sua oferta  ✔ pronto") : TEXT("Sua oferta"), Mine, 260.f)]
                        + SHorizontalBox::Slot().AutoWidth()[Column(bTheirReady ? FString(TEXT("A oferta dele  ✔ pronto")) : FString(TEXT("A oferta dele")), Theirs, 260.f)]
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(274.f, 0.f, 0.f, 0.f).VAlign(VAlign_Center)[Coins(DraftCoins, Cream)]
                    + SHorizontalBox::Slot().AutoWidth().Padding(10.f, 0.f, 0.f, 0.f)[TextButton(TEXT("-10"), 44.f, [CoinStep]() { CoinStep(-10); })]
                    + SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)[TextButton(TEXT("-1"), 40.f, [CoinStep]() { CoinStep(-1); })]
                    + SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)[TextButton(TEXT("+1"), 40.f, [CoinStep]() { CoinStep(1); })]
                    + SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)[TextButton(TEXT("+10"), 44.f, [CoinStep]() { CoinStep(10); })]
                    + SHorizontalBox::Slot().AutoWidth().Padding(30.f, 0.f, 0.f, 0.f).VAlign(VAlign_Center)[Coins(S->GetTheirOffer().Coins, Muted)]
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 16.f, 0.f, 0.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 12.f, 0.f)
                        [TextButton(bMineReady ? TEXT("Desfazer pronto") : TEXT("Pronto"), 200.f, [this, bMineReady]() { if (UVortexSocial* So = Social()) So->ServerTradeReady(!bMineReady); })]
                    + SHorizontalBox::Slot().AutoWidth()[TextButton(TEXT("Cancelar"), 160.f, [this]() { Cancel(); })]
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 10.f, 0.f, 0.f)
                    [SNew(STextBlock).Text(FText::FromString(TEXT("Clique na mochila para pôr na mesa e na sua oferta para tirar. Qualquer mudança desfaz o pronto dos dois.")))
                        .Font(SocialFont(12)).ColorAndOpacity(Muted)]
            ));
        }
    };

    void TickTrade(AVortexCharacter* Hero)
    {
        const UVortexSocial* Social = Hero ? Hero->GetSocial() : nullptr;
        if (!Social || !Hero->IsLocallyControlled()) return;
        const bool bTrading = Social->GetTradePartner() != nullptr;
        if (bTrading && !bTradeWindow)
        {
            Show(Hero, SNew(STradeWindow).Hero(Hero));
            bTradeWindow = true;
        }
        else if (!bTrading && bTradeWindow) Close();
    }
}
