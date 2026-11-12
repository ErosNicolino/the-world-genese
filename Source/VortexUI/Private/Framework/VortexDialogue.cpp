#include "Framework/VortexDialogue.h"
#include "VortexUiImpl.h"
#include "Framework/VortexShopWindow.h"
#include "Containers/Ticker.h"

#include "Containers/Ticker.h"
#include "Creatures/VortexNpc.h"
#include "Creatures/VortexNpcRoutine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Rpg/VortexQuestLog.h"
#include "Rpg/VortexStory.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Framework/VortexUiKit.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/VortexUiJuice.h"

namespace VortexDialogueUi
{
    const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor(0xF0, 0xCF, 0x7A));
    const FLinearColor GoldDeep = FLinearColor::FromSRGBColor(FColor(0xC8, 0x92, 0x3A));
    const FLinearColor Cream = FLinearColor::FromSRGBColor(FColor(0xF4, 0xEA, 0xD2));
    const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(0xA9, 0xB4, 0xC2));
    const FLinearColor QuestColor = FLinearColor::FromSRGBColor(FColor(0xFF, 0xD8, 0x6B));
    const FLinearColor ReadyColor = FLinearColor::FromSRGBColor(FColor(0x8F, 0xE3, 0x8A));

    FSlateFontInfo Font(int32 Size, const TCHAR* Weight = TEXT("Regular"))
    {
        return FCoreStyle::GetDefaultFontStyle(Weight, Size);
    }

    const FSlateBrush* Rounded(float Radius, const FLinearColor& Fill, const FLinearColor& Line, float Width)
    {
        static TArray<TSharedPtr<FSlateRoundedBoxBrush>> Keep;
        Keep.Add(MakeShared<FSlateRoundedBoxBrush>(Fill, Radius, Line, Width));
        return Keep.Last().Get();
    }

    const FButtonStyle& ChoiceStyle()
    {
        static FButtonStyle Style = []()
        {
            FButtonStyle S;
            S.SetNormal(*Rounded(8.f, FLinearColor(.02f, .035f, .06f, .9f), FLinearColor(GoldDeep.R, GoldDeep.G, GoldDeep.B, .35f), 1.f));
            S.SetHovered(*Rounded(8.f, FLinearColor(.05f, .07f, .11f, .96f), Gold, 1.f));
            S.SetPressed(*Rounded(8.f, FLinearColor(.04f, .06f, .09f, .96f), Gold, 2.f));
            S.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f, 1.f, 0.f, 0.f));
            return S;
        }();
        return Style;
    }

    struct FChoice
    {
        FString Text;
        FLinearColor Color = Cream;
        TFunction<void()> Run;
    };

    class SVortexDialogueWindow final : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SVortexDialogueWindow) {}
            SLATE_ARGUMENT(TWeakObjectPtr<AVortexCharacter>, Hero)
            SLATE_ARGUMENT(TWeakObjectPtr<AVortexNpc>, Npc)
        SLATE_END_ARGS()

        void Construct(const FArguments& Args)
        {
            Hero = Args._Hero;
            Npc = Args._Npc;
            NpcId = Npc.IsValid() ? Npc->GetNpcId() : NAME_None;
            const FVortexStoryCatalog& Story = FVortexStoryCatalog::Get();
            Dialogue = Story.DialogueOf(NpcId);
            const FVortexNpcInfo* Info = Story.Npc(NpcId);
            NpcName = Info ? Info->Name : (Npc.IsValid() ? Npc->GetDisplayName() : FString(TEXT("?")));
            NpcTitle = Info ? Info->Title : FString();
            TSharedPtr<VortexUiJuice::SCartoonFx> Fx;
            ChildSlot
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                [
                SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(0.f, 0.f, 0.f, 70.f))
                [
                    SNew(SBox).WidthOverride(980.f)
                    [
                        // 24/10: a caixa do kit cartoon (a mesma das janelas), com o nome numa fita e as respostas de borracha
                        SNew(SBorder).BorderImage(VortexUiKit::Cartoon::Frame())
                        .Padding(FMargin(30.f, 22.f, 30.f, 26.f))
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
                                [
                                    SNew(SBorder).BorderImage(VortexUiKit::Cartoon::Ribbon()).Padding(FMargin(22.f, 4.f, 22.f, 10.f))
                                    [SNew(STextBlock).Font(VortexUiKit::Cartoon::Font(22, 2)).Margin(VortexUiJuice::CartoonNudge(22)).ColorAndOpacity(Cream).Text_Lambda([this]() { return FText::FromString(Speaker); })]
                                ]
                                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(12.f, 0.f, 0.f, 3.f)
                                [SNew(STextBlock).Font(Font(13, TEXT("Bold"))).ColorAndOpacity(Cream).Text(FText::FromString(NpcTitle))]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 16.f)
                            [
                                SNew(SBox).MinDesiredHeight(70.f)
                                [
                                    SNew(STextBlock).Font(Font(16)).ColorAndOpacity(Cream).AutoWrapText(true).LineHeightPercentage(1.15f)
                                    .ShadowOffset(FVector2D(0.f, 1.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, .1f, .7f))
                                    .Text_Lambda([this]() { return FText::FromString(Body.Left(FMath::FloorToInt(Shown))); })
                                ]
                            ]
                            + SVerticalBox::Slot().AutoHeight()[SAssignNew(ChoiceBox, SVerticalBox)]
                        ]
                    ]
                ]
                ]
                + SOverlay::Slot()[SAssignNew(Fx, VortexUiJuice::SCartoonFx)]
            ];
            VortexUiJuice::FxLayer = Fx;
            if (UVortexQuestLog* Log = QuestLog()) Log->ServerTalk(NpcId, NAME_None);
            GoTo(Dialogue ? Dialogue->Start : NAME_None);
        }

        virtual bool SupportsKeyboardFocus() const override { return true; }

        virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& Event) override
        {
            const FKey Key = Event.GetKey();
            if (Key == EKeys::Escape) return FReply::Unhandled(); // o PlayerController fecha o modal sem empilhar a pausa.
            if (Key == EKeys::E && Choices.Num() <= 1 && Shown >= Body.Len()) { RequestClose(); return FReply::Handled(); }
            if (Key == EKeys::SpaceBar || Key == EKeys::Enter || Key == EKeys::E)
            {
                if (Shown < Body.Len()) Shown = float(Body.Len());
                else if (Choices.Num() == 1) Pick(0);
                return FReply::Handled();
            }
            static const FKey Numbers[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
            for (int32 i = 0; i < 9; ++i)
            {
                if (Key == Numbers[i] && Choices.IsValidIndex(i)) { Pick(i); return FReply::Handled(); }
            }
            return FReply::Unhandled();
        }

        virtual void Tick(const FGeometry& Geometry, const double CurrentTime, const float DeltaTime) override
        {
            SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
            // fala escrita aos poucos (~60 letras por segundo), com o bipe do Godot de vez em quando
            if (Shown < Body.Len())
            {
                const int32 Before = FMath::FloorToInt(Shown);
                Shown = FMath::Min(float(Body.Len()), Shown + DeltaTime * 60.f);
                if (FMath::FloorToInt(Shown) / 6 != Before / 6) VortexAudio::Play2D(Hero.Get(), VortexNpcRoutine::Voice(NpcId, Before / 6), .32f);   // 08/11: a voz do morador (Tools/MakeNpcVoices.py)
            }
            // longe demais (o herói andou) ou o NPC sumiu: a conversa acaba
            if (!Hero.IsValid() || !Npc.IsValid() || FVector::Dist(Hero->GetActorLocation(), Npc->GetActorLocation()) > 800.f) RequestClose();
        }

    private:
        TWeakObjectPtr<AVortexCharacter> Hero;
        TWeakObjectPtr<AVortexNpc> Npc;
        FName NpcId;
        const FVortexDialogue* Dialogue = nullptr;
        FString NpcName, NpcTitle, Speaker, Body;
        float Shown = 0.f;
        TArray<FChoice> Choices;
        TSharedPtr<SVerticalBox> ChoiceBox;

        UVortexQuestLog* QuestLog() const { return Hero.IsValid() ? Hero->FindComponentByClass<UVortexQuestLog>() : nullptr; }

        void RequestClose()
        {
            FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float) { VortexDialogue::Close(); return false; }));
        }

        void Show(const FString& Who, const FString& Text)
        {
            Speaker = Who.IsEmpty() ? NpcName : Who;
            Body = Text;
            Shown = 0.f;
        }

        void Pick(int32 Index)
        {
            if (!Choices.IsValidIndex(Index)) return;
            VortexUiJuice::UiSound(TEXT("ui_c_select"), .5f);
            TFunction<void()> Run = Choices[Index].Run;
            if (Run) Run();
        }

        void BuildChoices()
        {
            ChoiceBox->ClearChildren();
            for (int32 i = 0; i < Choices.Num(); ++i)
            {
                const FChoice& C = Choices[i];
                ChoiceBox->AddSlot().AutoHeight().Padding(0.f, 3.f)
                [
                    SNew(VortexUiJuice::SJuicyButton).ButtonStyle(&VortexUiKit::Cartoon::Button(0)).Sound(NAME_None).Stars(true)
                    .OnClicked_Lambda([this, i]() { Pick(i); return FReply::Handled(); })
                    [
                        SNew(SBox).Padding(FMargin(14.f, 7.f))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
                            [SNew(STextBlock).Font(VortexUiKit::Cartoon::Font(16, 1)).Margin(VortexUiJuice::CartoonNudge(16)).ColorAndOpacity(Gold).Text(FText::FromString(FString::FromInt(i + 1)))]
                            + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                            [SNew(STextBlock).Font(VortexUiKit::Outlined(Font(15, TEXT("Bold")))).ColorAndOpacity(C.Color == Muted ? Cream : C.Color).Text(FText::FromString(C.Text))]
                        ]
                    ]
                ];
            }
        }

        /** As escolhas das missões deste NPC (entram no começo da lista). */
        void AddQuestChoices()
        {
            const UVortexQuestLog* Log = QuestLog();
            if (!Log) return;
            for (const FVortexQuestDef& Q : FVortexStoryCatalog::Get().Quests)
            {
                if (Q.Giver != NpcId) continue;
                const FName Id = Q.Id;
                if (Log->IsReady(Q)) Choices.Add({ FString::Printf(TEXT("[Entregar] %s"), *Q.Title), ReadyColor, [this, Id]() { QuestDone(Id); } });
                else if (Log->IsActive(Q.Id)) Choices.Add({ FString::Printf(TEXT("Sobre: %s"), *Q.Title), Muted, [this, Id]() { QuestAbout(Id); } });
                else if (Log->IsAvailable(Q) && !Q.bAuto) Choices.Add({ FString::Printf(TEXT("[Missão] %s"), *Q.Title), QuestColor, [this, Id]() { QuestOffer(Id); } });
            }
        }

        void GoTo(FName NodeId)
        {
            Choices.Reset();
            const FVortexDialogueNode* Node = Dialogue && !NodeId.IsNone() ? Dialogue->Nodes.Find(NodeId) : nullptr;
            if (!Node)
            {
                // NPC sem diálogo: uma fala curta (bark) e as missões
                const FVortexNpcInfo* Info = FVortexStoryCatalog::Get().Npc(NpcId);
                // 08/11: mais as falas da hora: quem não tinha fala própria respondia só "..."
                TArray<FString> Lines = Info ? Info->Barks : TArray<FString>();
                if (Hero.IsValid()) Lines.Append(VortexNpcRoutine::Barks(NpcId, VortexNpcRoutine::HourOf(Hero->GetWorld())));
                Show(NpcName, Lines.Num() ? Lines[FMath::RandRange(0, Lines.Num() - 1)] : FString(TEXT("Bom te ver por aqui.")));
                AddQuestChoices();
                Choices.Add({ TEXT("Até logo."), Muted, [this]() { RequestClose(); } });
                BuildChoices();
                return;
            }
            Show(Node->Speaker, Node->Text);
            if (!Node->Actions.IsEmpty())
            {
                if (UVortexQuestLog* Log = QuestLog()) Log->ServerTalk(NpcId, NodeId);
                for (const TArray<FString>& Action : Node->Actions)
                {
                    if (Action.Num() >= 1 && Action[0] == TEXT("open_shop"))
                    {
                        // fecha a conversa e abre a loja no quadro seguinte (não dá para fechar esta
                        // janela de dentro dela mesma); sem nome é o Ducci
                        const FName Vendor = Action.Num() >= 2 ? FName(*Action[1]) : FName(TEXT("ducci"));
                        TWeakObjectPtr<AVortexCharacter> Who = Hero;
                        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Who, Vendor](float) -> bool
                        {
                            VortexDialogue::Close();
                            if (Who.IsValid()) VortexShopWindow::Open(Who.Get(), Vendor);
                            return false;
                        }));
                    }
                }
            }
            if (Node->Choices.Num())
            {
                if (NodeId == Dialogue->Start) AddQuestChoices();
                const UVortexQuestLog* Log = QuestLog();
                for (const FVortexDialogueChoice& C : Node->Choices)
                {
                    if (!C.IfFlag.IsNone() && !(Log && Log->HasFlag(C.IfFlag))) continue;
                    if (!C.IfNotFlag.IsNone() && Log && Log->HasFlag(C.IfNotFlag)) continue;
                    const FName Next = C.Next;
                    Choices.Add({ C.Text, Cream, [this, Next]() { if (Next.IsNone()) RequestClose(); else GoTo(Next); } });
                }
            }
            else
            {
                const FName Next = Node->Next;
                Choices.Add({ Next.IsNone() ? TEXT("Até logo.") : TEXT("Continuar"), Cream, [this, Next]() { if (Next.IsNone()) RequestClose(); else GoTo(Next); } });
            }
            BuildChoices();
        }

        FName Root() const { return Dialogue ? Dialogue->Start : NAME_None; }

        void QuestOffer(FName Id)
        {
            const FVortexQuestDef* Q = FVortexStoryCatalog::Get().Quest(Id);
            if (!Q) return;
            Show(NpcName, Q->Offer);
            Choices.Reset();
            Choices.Add({ TEXT("Aceitar"), QuestColor, [this, Id]()
            {
                if (UVortexQuestLog* Log = QuestLog()) Log->ServerAccept(Id);
                VortexAudio::Play2D(Hero.Get(), TEXT("quest_accept"), .7f);
                const FVortexQuestDef* Accepted = FVortexStoryCatalog::Get().Quest(Id);
                Show(NpcName, Accepted ? Accepted->Progress : TEXT(""));
                Choices.Reset();
                Choices.Add({ TEXT("Vou cuidar disso."), Cream, [this]() { RequestClose(); } });
                BuildChoices();
            } });
            Choices.Add({ TEXT("Agora não."), Muted, [this]() { GoTo(Root()); } });
            BuildChoices();
        }

        void QuestAbout(FName Id)
        {
            const FVortexQuestDef* Q = FVortexStoryCatalog::Get().Quest(Id);
            const UVortexQuestLog* Log = QuestLog();
            if (!Q || !Log) return;
            Show(NpcName, Q->Progress + TEXT("\n\n") + Log->ObjectiveLines(*Q));
            Choices.Reset();
            Choices.Add({ TEXT("Voltar"), Muted, [this]() { GoTo(Root()); } });
            BuildChoices();
        }

        void QuestDone(FName Id)
        {
            const FVortexQuestDef* Q = FVortexStoryCatalog::Get().Quest(Id);
            if (!Q) return;
            if (UVortexQuestLog* Log = QuestLog()) Log->ServerTurnIn(Id);
            TArray<FString> Rewards;
            if (Q->Xp > 0) Rewards.Add(FString::Printf(TEXT("+%lld XP"), Q->Xp));
            if (Q->Coins > 0) Rewards.Add(FString::Printf(TEXT("+%d moedas"), Q->Coins));
            Show(NpcName, Q->Done + (Rewards.Num() ? FString(TEXT("\n\n")) + FString::Join(Rewards, TEXT("   ")) : FString()));
            Choices.Reset();
            Choices.Add({ TEXT("Obrigado."), Cream, [this]() { GoTo(Root()); } });
            BuildChoices();
        }
    };

    TSharedPtr<SVortexDialogueWindow> Window;
    TWeakObjectPtr<APlayerController> Owner;
}

namespace VortexDialogueImpl
{
    bool IsOpen() { return VortexDialogueUi::Window.IsValid(); }

    AVortexNpc* FindTalkable(const AVortexCharacter* Hero)
    {
        if (!Hero || !Hero->GetWorld()) return nullptr;
        AVortexNpc* Best = nullptr;
        float BestScore = 1e9f;
        const FVector Forward = Hero->GetControlRotation().Vector().GetSafeNormal2D();
        for (TActorIterator<AVortexNpc> It(Hero->GetWorld()); It; ++It)
        {
            if (It->IsHidden()) continue;
            const FVector To = It->GetActorLocation() - Hero->GetActorLocation();
            const float Distance = To.Size2D();
            if (Distance > 350.f || FMath::Abs(To.Z) > 200.f) continue;
            // de frente conta mais; colado conta mesmo de lado
            const float Facing = FVector::DotProduct(Forward, To.GetSafeNormal2D());
            if (Facing < -.2f && Distance > 150.f) continue;
            const float Score = Distance * (1.5f - Facing * .5f);
            if (Score < BestScore) { BestScore = Score; Best = *It; }
        }
        return Best;
    }

    void Open(AVortexCharacter* Hero, AVortexNpc* Npc)
    {
        if (!Hero || !Npc || IsOpen() || !GEngine || !GEngine->GameViewport) return;
        APlayerController* Controller = Cast<APlayerController>(Hero->GetController());
        if (!Controller) return;
        VortexDialogueUi::Window = SNew(VortexDialogueUi::SVortexDialogueWindow).Hero(Hero).Npc(Npc);
        VortexDialogueUi::Owner = Controller;
        GEngine->GameViewport->AddViewportWidgetContent(VortexDialogueUi::Window.ToSharedRef(), 40);
        Controller->SetShowMouseCursor(true);
        FInputModeGameAndUI Mode;
        Mode.SetWidgetToFocus(VortexDialogueUi::Window);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        Controller->SetInputMode(Mode);
        VortexAudio::Play2D(Hero, TEXT("ui_open"), .5f);
    }

    void Close()
    {
        if (!IsOpen()) return;
        if (GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(VortexDialogueUi::Window.ToSharedRef());
        VortexDialogueUi::Window.Reset();
        if (APlayerController* Controller = VortexDialogueUi::Owner.Get())
        {
            Controller->SetShowMouseCursor(false);
            Controller->SetInputMode(FInputModeGameOnly());
        }
        VortexDialogueUi::Owner.Reset();
    }
}
