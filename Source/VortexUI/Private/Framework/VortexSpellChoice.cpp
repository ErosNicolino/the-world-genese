#include "Framework/VortexSpellChoice.h"
#include "VortexUiImpl.h"

#include "Engine/GameViewportClient.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexUiJuice.h"
#include "Framework/VortexUiKit.h"
#include "GameFramework/PlayerController.h"
#include "Rpg/VortexHeroSheet.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// A janela da escolha da magia do cajado (02/11): três cartões, um clique escolhe (para sempre) e fecha.
namespace VortexSpellChoiceImpl
{
    TSharedPtr<SWidget> Window;
    TWeakObjectPtr<APlayerController> Owner;

    const FLinearColor Cream = FLinearColor::FromSRGBColor(FColor(0xF4, 0xEA, 0xD2));
    const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor(0xF0, 0xCF, 0x7A));

    FSlateFontInfo Font(int32 Size)
    {
        return Size >= 20 ? VortexUiKit::Cartoon::Font(Size, 2) : VortexUiKit::Outlined(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size));
    }

    bool IsOpen() { return Window.IsValid(); }

    void Close()
    {
        if (!Window.IsValid()) return;
        if (GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Window.ToSharedRef());
        Window.Reset();
        if (APlayerController* Controller = Owner.Get())
        {
            Controller->SetShowMouseCursor(false);
            Controller->SetInputMode(FInputModeGameOnly());
        }
        Owner.Reset();
    }

    TSharedRef<SWidget> Choice(TWeakObjectPtr<AVortexCharacter> Hero, FName Spell, const TCHAR* Title, const TCHAR* Text, const FLinearColor& Color)
    {
        return SNew(SBox).WidthOverride(250.f).HeightOverride(210.f)
        [
            SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&VortexUiKit::Cartoon::Button(0)).HAlign(HAlign_Fill).VAlign(VAlign_Fill)
            .OnClicked_Lambda([Hero, Spell]()
            {
                if (AVortexCharacter* H = Hero.Get(); H && H->GetSheet()) H->GetSheet()->ServerChooseStaffSpell(Spell);
                if (AVortexCharacter* H = Hero.Get()) VortexAudio::Play2D(H, TEXT("ui_click"), .6f);
                Close();
                return FReply::Handled();
            })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(14.f, 16.f, 14.f, 8.f).HAlign(HAlign_Center)
                [SNew(STextBlock).Text(FText::FromString(Title)).Font(Font(22)).ColorAndOpacity(Color)]
                + SVerticalBox::Slot().FillHeight(1.f).Padding(16.f, 4.f, 16.f, 14.f)
                [SNew(STextBlock).Text(FText::FromString(Text)).Font(Font(13)).ColorAndOpacity(Cream).AutoWrapText(true).Justification(ETextJustify::Center)]
            ]
        ];
    }

    void Open(AVortexCharacter* Hero)
    {
        Close();
        APlayerController* Controller = Hero ? Cast<APlayerController>(Hero->GetController()) : nullptr;
        if (!Controller || !GEngine || !GEngine->GameViewport) return;
        const TWeakObjectPtr<AVortexCharacter> Weak(Hero);
        Window = SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(SBorder).BorderImage(VortexUiKit::Window()).Padding(FMargin(30.f, 24.f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                [SNew(STextBlock).Text(FText::FromString(TEXT("Escolha a magia do cajado"))).Font(Font(28)).ColorAndOpacity(Gold)]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f, 0.f, 18.f)
                [SNew(STextBlock).Text(FText::FromString(TEXT("Só uma fica: ela vira o botão direito do cajado."))).Font(Font(14)).ColorAndOpacity(Cream)]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f)
                    [Choice(Weak, TEXT("heal"), TEXT("Cura"), TEXT("Cura o aliado que você mirar (o nome dele aparece na mira); sem ninguém na mira, você e o grupo perto."), FLinearColor(.55f, 1.f, .6f))]
                    + SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f)
                    [Choice(Weak, TEXT("vines"), TEXT("Vinhas"), TEXT("Prende o inimigo que você mirar por 3 segundos (chefes, 1,5 s)."), FLinearColor(.75f, .95f, .4f))]
                    + SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f)
                    [Choice(Weak, TEXT("fireball"), TEXT("Bola de Fogo"), TEXT("Uma esfera de fogo grande que explode onde bate (3 m): quatro vezes o golpe."), FLinearColor(1.f, .6f, .3f))]
                ]
            ]
        ];
        Owner = Controller;
        GEngine->GameViewport->AddViewportWidgetContent(Window.ToSharedRef(), 46);
        Controller->SetShowMouseCursor(true);
        FInputModeGameAndUI Mode;
        Mode.SetWidgetToFocus(Window);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        Controller->SetInputMode(Mode);
        VortexAudio::Play2D(Hero, TEXT("ui_open"), .5f);
    }

    void OpenHorseName(AVortexCharacter* Hero)
    {
        Close();
        APlayerController* Controller = Hero ? Cast<APlayerController>(Hero->GetController()) : nullptr;
        if (!Controller || !GEngine || !GEngine->GameViewport) return;
        const TWeakObjectPtr<AVortexCharacter> Weak(Hero);
        TSharedPtr<SEditableTextBox> Input;
        auto Confirm = [Weak](const FString& Name)
        {
            if (AVortexCharacter* H = Weak.Get(); H && H->GetSheet() && !Name.TrimStartAndEnd().IsEmpty()) H->GetSheet()->ServerSetMountName(Name);
            if (AVortexCharacter* H = Weak.Get()) VortexAudio::Play2D(H, TEXT("ui_click"), .6f);
            Close();
        };
        Window = SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(SBorder).BorderImage(VortexUiKit::Window()).Padding(FMargin(34.f, 26.f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                [SNew(STextBlock).Text(FText::FromString(TEXT("Dê um nome ao seu cavalo"))).Font(Font(28)).ColorAndOpacity(Gold)]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f, 0.f, 16.f)
                [SNew(STextBlock).Text(FText::FromString(TEXT("Aparece em cima dele para todos. Enter confirma."))).Font(Font(14)).ColorAndOpacity(Cream)]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                [
                    SNew(SBox).WidthOverride(380.f)
                    [
                        SAssignNew(Input, SEditableTextBox).Font(Font(20)).HintText(FText::FromString(TEXT("Ventania, Trovão, Pipoca...")))
                        .OnTextCommitted_Lambda([Confirm](const FText& Text, ETextCommit::Type How)
                        {
                            if (How == ETextCommit::OnEnter) Confirm(Text.ToString());
                        })
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 16.f, 0.f, 0.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f)
                    [
                        SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&VortexUiKit::Cartoon::Button(0)).ContentPadding(FMargin(22.f, 8.f))
                        .OnClicked_Lambda([Confirm, Input]() { Confirm(Input.IsValid() ? Input->GetText().ToString() : FString()); return FReply::Handled(); })
                        [SNew(STextBlock).Text(FText::FromString(TEXT("Pronto"))).Font(Font(18)).ColorAndOpacity(Cream)]
                    ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f)
                    [
                        SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&VortexUiKit::Cartoon::Button(0)).ContentPadding(FMargin(22.f, 8.f))
                        .OnClicked_Lambda([]() { Close(); return FReply::Handled(); })
                        [SNew(STextBlock).Text(FText::FromString(TEXT("Depois"))).Font(Font(18)).ColorAndOpacity(Cream)]
                    ]
                ]
            ]
        ];
        Owner = Controller;
        GEngine->GameViewport->AddViewportWidgetContent(Window.ToSharedRef(), 46);
        Controller->SetShowMouseCursor(true);
        FInputModeUIOnly Mode;
        Mode.SetWidgetToFocus(Input);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Controller->SetInputMode(Mode);
        VortexAudio::Play2D(Hero, TEXT("ui_open"), .5f);
    }
}
