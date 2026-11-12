#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexUiKit.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

/**
 * O "suco" das janelas cartoon, usado por todas as telas (herói, loja, menu, pausa, opções, conversa, social):
 * sons da interface (UiSound), a camada de estrelinhas (SCartoonFx + Burst/FloatUp), o botão de borracha
 * (SJuicyButton: cresce e balança com o mouse, achata ao apertar, quica ao soltar, estrelas e som no clique),
 * o soco dos números (Punch) e o acerto da letra cartoon no meio das caixas (CartoonNudge).
 * Cada tela põe um SCartoonFx por cima de tudo e guarda em FxLayer.
 */
namespace VortexUiJuice
{
    namespace Av = VortexUiKit::Cartoon;
    const FLinearColor JuiceGold = FLinearColor::FromSRGBColor(FColor(0xFF, 0xD2, 0x3C));
    const FLinearColor JuiceCream = FLinearColor::FromSRGBColor(FColor(0xFF, 0xFB, 0xF0));

    /** A Luckiest Guy desenha as letras no alto da linha (sobra espaço embaixo): desce o texto sem mudar a altura. */
    inline FMargin CartoonNudge(int32 Size) { return FMargin(0.f, Size * .17f, 0.f, -Size * .17f); }

    inline double Now() { return FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetCurrentTime() : FPlatformTime::Seconds(); }

    inline const UObject* SoundContext() { return GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr; }

    /** Toca um som da interface e anota a hora (o botão só toca o "pop" comum se o clique não tocou outro). */
    inline void UiSound(FName Sound, float Volume = .6f)
    {
        VortexAudio::LastUiSoundAt() = Now();
        VortexAudio::Play2D(SoundContext(), Sound, Volume);
    }

    /** A camada dos efeitos por cima da janela: estrelas, faíscas, anel e textos que sobem (posições na tela). */
    class SCartoonFx final : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SCartoonFx) {}
        SLATE_END_ARGS()

        void Construct(const FArguments&) { SetVisibility(EVisibility::HitTestInvisible); }

        enum class EBit : uint8 { Star, Spark, Ring, Text };
        struct FBit
        {
            EBit Kind = EBit::Star;
            FVector2f At = FVector2f::ZeroVector, Vel = FVector2f::ZeroVector;
            double Born = 0.0;
            float Life = .6f, Size = 24.f, Spin = 0.f;
            FLinearColor Color = FLinearColor::White;
            FString Text;
        };

        void Add(const FBit& Bit) { Bits.Add(Bit); }

        virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

        virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Out, int32 LayerId,
            const FWidgetStyle&, bool) const override
        {
            const double T = Now();
            Bits.RemoveAll([T](const FBit& B) { return T - B.Born > B.Life; });
            for (const FBit& B : Bits)
            {
                const float Age = float(T - B.Born), K = Age / B.Life;
                // sobe, abre e cai um pouco (a gravidade puxa as estrelas)
                const FVector2f Abs = B.At + B.Vel * Age + FVector2f(0.f, B.Kind == EBit::Text ? 0.f : 700.f) * (.5f * Age * Age);
                const FVector2f Local = FVector2f(Geometry.AbsoluteToLocal(FVector2D(Abs)));
                const float Fade = K < .65f ? 1.f : 1.f - (K - .65f) / .35f;
                FLinearColor Color = B.Color;
                Color.A *= Fade;
                if (B.Kind == EBit::Text)
                {
                    // o texto dá um pulo (cresce e volta) e sobe devagar
                    const float Pop = 1.f + .6f * FMath::Exp(-9.f * Age) * FMath::Cos(Age * 18.f);
                    const FSlateFontInfo Font = Av::Cartoon(26, 2);
                    const FVector2f Measured = FVector2f(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(B.Text, Font));
                    const FVector2f Pos = Local - FVector2f(0.f, 70.f * (1.f - FMath::Exp(-3.f * Age))) - Measured * .5f * Pop;
                    FSlateDrawElement::MakeText(Out, LayerId, Geometry.ToPaintGeometry(Measured, FSlateLayoutTransform(Pop, Pos)), B.Text, Font, ESlateDrawEffect::None, Color);
                    continue;
                }
                float Size = B.Size;
                const FSlateBrush* Brush = Av::Star();
                if (B.Kind == EBit::Ring) { Brush = Av::Ring(); Size = B.Size * (.3f + 1.4f * FMath::Sqrt(K)); }
                else
                {
                    if (B.Kind == EBit::Spark) Brush = Av::Spark();
                    // nasce pulando e encolhe no fim
                    Size *= K < .15f ? FMath::Sin(K / .15f * PI * .5f) * 1.25f : 1.25f - .9f * (K - .15f) / .85f;
                }
                const FVector2f Box(Size, Size);
                FSlateDrawElement::MakeBox(Out, LayerId, Geometry.ToPaintGeometry(Box, FSlateLayoutTransform(Local - Box * .5f), FSlateRenderTransform(FQuat2f(B.Spin * Age)), FVector2f(.5f, .5f)),
                    Brush, ESlateDrawEffect::None, Color);
            }
            return LayerId + 1;
        }

    private:
        mutable TArray<FBit> Bits;
    };
    inline TWeakPtr<SCartoonFx> FxLayer;

    inline FVector2f CursorAt() { return FVector2f(FSlateApplication::Get().GetCursorPos()); }

    /** Estourinho no mouse: um anel, estrelas coloridas e faíscas. */
    inline void Burst(const FLinearColor& Color, int32 Stars = 6, float Power = 1.f)
    {
        const TSharedPtr<SCartoonFx> Fx = FxLayer.Pin();
        if (!Fx) return;
        const FVector2f At = CursorAt();
        SCartoonFx::FBit Ring;
        Ring.Kind = SCartoonFx::EBit::Ring;
        Ring.At = At;
        Ring.Born = Now();
        Ring.Life = .32f;
        Ring.Size = 70.f * Power;
        Ring.Color = FLinearColor(1.f, 1.f, 1.f, .9f);
        Fx->Add(Ring);
        const FLinearColor Palette[] = { Color, JuiceGold, JuiceCream, FLinearColor::FromSRGBColor(FColor(0x7A, 0xD8, 0xFF)) };
        for (int32 i = 0; i < Stars + Stars / 2; ++i)
        {
            SCartoonFx::FBit Bit;
            Bit.Kind = i < Stars ? SCartoonFx::EBit::Star : SCartoonFx::EBit::Spark;
            const float Angle = (float(i) / Stars + FMath::FRandRange(-.08f, .08f)) * 2.f * PI;
            const float Speed = FMath::FRandRange(160.f, 300.f) * Power;
            Bit.At = At;
            Bit.Vel = FVector2f(FMath::Cos(Angle), FMath::Sin(Angle) - .6f) * Speed;
            Bit.Born = Now();
            Bit.Life = FMath::FRandRange(.45f, .7f);
            Bit.Size = (Bit.Kind == SCartoonFx::EBit::Star ? FMath::FRandRange(16.f, 26.f) : FMath::FRandRange(12.f, 18.f)) * FMath::Sqrt(Power);
            Bit.Spin = FMath::FRandRange(-9.f, 9.f);
            Bit.Color = Palette[i % 4];
            Fx->Add(Bit);
        }
    }

    /** Texto que pula e sobe no mouse ("+1"). */
    inline void FloatUp(const FString& Text, const FLinearColor& Color)
    {
        const TSharedPtr<SCartoonFx> Fx = FxLayer.Pin();
        if (!Fx) return;
        SCartoonFx::FBit Bit;
        Bit.Kind = SCartoonFx::EBit::Text;
        Bit.At = CursorAt() - FVector2f(0.f, 24.f);
        Bit.Born = Now();
        Bit.Life = .9f;
        Bit.Color = Color;
        Bit.Text = Text;
        Fx->Add(Bit);
    }

    /** Estado da animação de um botão. Com Id, sobrevive à janela ser refeita (o clique refaz a página, e o
     *  botão novo continua o quique do antigo). */
    struct FJuice
    {
        double HoverAt = -10.0, LeaveAt = -10.0, PressAt = -10.0, ReleaseAt = -10.0, PopAt = -10.0;
        bool bHover = false, bDown = false;
    };
    inline TMap<FName, FJuice>& Juices()
    {
        static TMap<FName, FJuice> States;
        return States;
    }

    /** Escala e giro do "botão de borracha" num instante. */
    inline TOptional<FSlateRenderTransform> JuiceTransform(const FJuice& S)
    {
        const double T = Now();
        float Sx = 1.f, Sy = 1.f, Rot = 0.f;
        // com o mouse: cresce um pouco e dá uma balançada
        const float H = S.bHover ? FMath::Min(1.f, float(T - S.HoverAt) / .1f) : FMath::Max(0.f, 1.f - float(T - S.LeaveAt) / .12f);
        Sx += .05f * H;
        Sy += .05f * H;
        if (S.bHover)
        {
            const float W = float(T - S.HoverAt);
            Rot = 3.f * FMath::Exp(-7.f * W) * FMath::Sin(W * 32.f);
        }
        // apertado: achata (mais largo e mais baixo); soltou: quica (estica e volta)
        if (S.bDown)
        {
            const float P = FMath::Min(1.f, float(T - S.PressAt) / .05f);
            Sx += .08f * P;
            Sy -= .12f * P;
        }
        else if (const float R = float(T - S.ReleaseAt); R < .7f)
        {
            const float B = FMath::Exp(-7.f * R) * FMath::Sin(R * 28.f);
            Sx -= .09f * B;
            Sy += .14f * B;
        }
        // entrou pulando (a aba escolhida)
        if (const float P = float(T - S.PopAt); P < .6f)
        {
            const float B = FMath::Exp(-7.f * P) * FMath::Cos(P * 22.f);
            Sx += .16f * B;
            Sy += .16f * B;
        }
        if (FMath::IsNearlyEqual(Sx, 1.f, .001f) && FMath::IsNearlyEqual(Sy, 1.f, .001f) && FMath::Abs(Rot) < .01f) return TOptional<FSlateRenderTransform>();
        const float Ang = FMath::DegreesToRadians(Rot), C = FMath::Cos(Ang), Sn = FMath::Sin(Ang);
        return FSlateRenderTransform(FMatrix2x2f(Sx * C, Sx * Sn, -Sy * Sn, Sy * C));
    }

    /** O botão de borracha: o SButton de sempre por dentro, com a animação, o som do clique e as estrelinhas. */
    class SJuicyButton final : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SJuicyButton) : _ButtonStyle(nullptr), _HAlign(HAlign_Fill), _VAlign(VAlign_Fill), _ContentPadding(FMargin(0.f)),
            _Sound(TEXT("ui_c_pop")), _Stars(true), _StarColor(FLinearColor::White) {}
            SLATE_DEFAULT_SLOT(FArguments, Content)
            SLATE_ARGUMENT(const FButtonStyle*, ButtonStyle)
            SLATE_ARGUMENT(EHorizontalAlignment, HAlign)
            SLATE_ARGUMENT(EVerticalAlignment, VAlign)
            SLATE_ATTRIBUTE(FMargin, ContentPadding)
            SLATE_EVENT(FOnClicked, OnClicked)
            SLATE_EVENT(FSimpleDelegate, OnHovered)
            SLATE_EVENT(FSimpleDelegate, OnUnhovered)
            /** O mesmo Id continua a animação quando a janela é refeita. */
            SLATE_ARGUMENT(FName, Id)
            /** O som do clique, se o próprio clique não tocou outro (NAME_None: nenhum). */
            SLATE_ARGUMENT(FName, Sound)
            SLATE_ARGUMENT(bool, Stars)
            SLATE_ARGUMENT(FLinearColor, StarColor)
        SLATE_END_ARGS()

        void Construct(const FArguments& Args)
        {
            Id = Args._Id;
            Sound = Args._Sound;
            bStars = Args._Stars;
            StarColor = Args._StarColor;
            OnClick = Args._OnClicked;
            OnHover = Args._OnHovered;
            OnUnhover = Args._OnUnhovered;
            BornAt = Now();
            BaseStyle = Args._ButtonStyle ? Args._ButtonStyle : &Av::Button(0);
            SetRenderTransformPivot(FVector2D(.5f, .5f));
            SetRenderTransform(TAttribute<TOptional<FSlateRenderTransform>>::CreateSP(this, &SJuicyButton::Transform));
            ChildSlot
            [
                SAssignNew(Button, SButton).ButtonStyle(StyleFor(State().bHover)).HAlign(Args._HAlign).VAlign(Args._VAlign)
                .ContentPadding(Args._ContentPadding)
                .OnClicked(this, &SJuicyButton::Clicked)
                .OnPressed(this, &SJuicyButton::Pressed)
                .OnReleased(this, &SJuicyButton::Released)
                .OnHovered(this, &SJuicyButton::Hovered)
                .OnUnhovered(this, &SJuicyButton::Unhovered)
                [Args._Content.Widget]
            ];
        }

    private:
        FName Id, Sound;
        bool bStars = true;
        FLinearColor StarColor;
        FOnClicked OnClick;
        FSimpleDelegate OnHover, OnUnhover;
        mutable FJuice Own;
        TSharedPtr<SButton> Button;
        const FButtonStyle* BaseStyle = nullptr;
        const FButtonStyle* ShownStyle = nullptr;
        double BornAt = 0.0;

        FJuice& State() const { return Id.IsNone() ? Own : Juices().FindOrAdd(Id); }

        /** O estilo com a cor "com o mouse" já no normal: o botão refeito embaixo do mouse nasce aceso, sem piscar
         *  (a Slate só avisa que o mouse está em cima dele um quadro depois). */
        const FButtonStyle* StyleFor(bool bHover)
        {
            if (!bHover) return ShownStyle = BaseStyle;
            static TMap<const FButtonStyle*, TSharedPtr<FButtonStyle>> Lit;
            TSharedPtr<FButtonStyle>& Style = Lit.FindOrAdd(BaseStyle);
            if (!Style) { Style = MakeShared<FButtonStyle>(*BaseStyle); Style->SetNormal(BaseStyle->Hovered); }
            return ShownStyle = Style.Get();
        }

        virtual void Tick(const FGeometry& Geometry, const double CurrentTime, const float DeltaTime) override
        {
            SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
            FJuice& S = State();
            // a janela foi refeita com o mouse já fora: o botão novo nunca recebe a saída, então solta aqui
            // (só depois de o botão existir um tempo: o novo embaixo do mouse ainda vai receber a entrada)
            if ((S.bHover || S.bDown) && !IsHovered() && Now() - BornAt > .25 && Now() - FMath::Max(S.HoverAt, S.PressAt) > .3)
            {
                S.bHover = S.bDown = false;
                S.LeaveAt = Now();
            }
            const FButtonStyle* Wanted = S.bHover ? StyleFor(true) : BaseStyle;
            if (Button && Wanted != ShownStyle) { ShownStyle = Wanted; Button->SetButtonStyle(Wanted); }
        }

        TOptional<FSlateRenderTransform> Transform() const { return JuiceTransform(State()); }

        FReply Clicked()
        {
            const uint64 Before = VortexAudio::UiSoundCount();
            const FReply Reply = OnClick.IsBound() ? OnClick.Execute() : FReply::Handled();
            if (VortexAudio::UiSoundCount() == Before && !Sound.IsNone()) UiSound(Sound, .55f);
            if (bStars) Burst(StarColor, 5, .8f);
            return Reply;
        }

        void Pressed()
        {
            FJuice& S = State();
            S.bDown = true;
            S.PressAt = Now();
        }

        void Released()
        {
            FJuice& S = State();
            S.bDown = false;
            S.ReleaseAt = Now();
        }

        void Hovered()
        {
            FJuice& S = State();
            if (!S.bHover)
            {
                S.bHover = true;
                // botão que acabou de nascer embaixo do mouse (a janela foi refeita): acende sem balançar nem tocar
                const bool bReborn = Now() - BornAt < .25;
                S.HoverAt = bReborn ? Now() - 1.0 : Now();
                // um tique baixinho (sem metralhar quando o mouse corre pela grade)
                static double LastTick = -1.0;
                if (!bReborn && IsEnabled() && Now() - LastTick > .07)
                {
                    LastTick = Now();
                    VortexAudio::Play2D(SoundContext(), TEXT("ui_c_hover"), .3f);
                }
            }
            OnHover.ExecuteIfBound();
        }

        void Unhovered()
        {
            FJuice& S = State();
            if (S.bHover)
            {
                S.bHover = false;
                S.LeaveAt = Now();
            }
            S.bDown = false;
            OnUnhover.ExecuteIfBound();
        }
    };

    /** Um número que dá um soco (cresce e volta) a partir de At. */
    inline TOptional<FSlateRenderTransform> Punch(double At)
    {
        const float P = float(Now() - At);
        if (P < 0.f || P > .7f) return TOptional<FSlateRenderTransform>();
        return FSlateRenderTransform(FScale2f(1.f + .55f * FMath::Exp(-7.f * P) * FMath::Cos(P * 20.f)));
    }
}
