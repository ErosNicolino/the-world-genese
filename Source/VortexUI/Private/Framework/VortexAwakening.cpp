#include "Framework/VortexAwakening.h"
#include "VortexUiImpl.h"

#include "Combat/VortexClassSkills.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Framework/VortexGameInstance.h"
#include "Persistence/VortexPlayerSave.h"
#include "Rendering/DrawElements.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexRpg.h"
#include "Styling/CoreStyle.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SLeafWidget.h"

namespace
{
    // raridade -> (moldura, nome do metal, cor, ordem da revelação, espera depois de virar)
    struct FTier { const TCHAR* Frame; const TCHAR* Metal; FLinearColor Color; int32 Rank; float Hold; const TCHAR* Sound; };
    FTier TierOf(const FString& Rarity)
    {
        if (Rarity == TEXT("legendary")) return { TEXT("T_Card_Diamond"), TEXT("DIAMANTE"), FLinearColor(.85f, .7f, 1.f), 3, 1.7f, TEXT("level_up") };
        if (Rarity == TEXT("epic")) return { TEXT("T_Card_Gold"), TEXT("OURO"), FLinearColor(1.f, .78f, .25f), 2, 1.f, TEXT("magic_burst") };
        if (Rarity == TEXT("rare")) return { TEXT("T_Card_Silver"), TEXT("PRATA"), FLinearColor(.75f, .85f, 1.f), 1, .7f, TEXT("coin") };
        return { TEXT("T_Card_Bronze"), TEXT("BRONZE"), FLinearColor(.95f, .6f, .35f), 0, .5f, TEXT("pickup") };
    }
    FString RarityName(const FString& Rarity)
    {
        return Rarity == TEXT("legendary") ? TEXT("LENDÁRIA") : Rarity == TEXT("epic") ? TEXT("ÉPICA") : Rarity == TEXT("rare") ? TEXT("RARA") : TEXT("COMUM");
    }
    float EaseOutBack(float T) { T = FMath::Clamp(T, 0.f, 1.f); const float C = 1.7f; return 1.f + (C + 1.f) * FMath::Pow(T - 1.f, 3.f) + C * FMath::Pow(T - 1.f, 2.f); }
    float CardEase(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); }

    class SVortexAwakening : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SVortexAwakening) {}
            SLATE_ARGUMENT(TWeakObjectPtr<AVortexCharacter>, Hero)
            SLATE_ARGUMENT(bool, Revealed)
        SLATE_END_ARGS()

        void Construct(const FArguments& Args)
        {
            Hero = Args._Hero;
            SetCanTick(true);
            const AVortexCharacter* H = Hero.Get();
            const UVortexHeroSheet* Sheet = H ? H->GetSheet() : nullptr;
            HeroName = H && H->GetLook().Appearance == TEXT("female") ? TEXT("a Aventureira") : TEXT("o Aventureiro");
            const UVortexGameInstance* Game = H ? Cast<UVortexGameInstance>(H->GetGameInstance()) : nullptr;
            if (const UVortexPlayerSave* Saved = Game ? Game->GetSelectedHero() : nullptr) { if (!Saved->HeroName.IsEmpty()) HeroName = Saved->HeroName; }
            WeaponType = Sheet ? Sheet->GetWeaponType() : NAME_None;
            const FVortexRpgCatalog& Rpg = FVortexRpgCatalog::Get();
            if (Sheet) for (const FName& Id : Sheet->GetClassCards()) if (const FVortexClassDef* C = Rpg.Class(Id)) Cards.Add({ C });
            // revela da mais comum para a mais rara (a lendária por último)
            for (int32 i = 0; i < Cards.Num(); ++i) Order.Add(i);
            Order.StableSort([this](int32 A, int32 B) { return TierOf(Cards[A].Def->Rarity).Rank < TierOf(Cards[B].Def->Rarity).Rank; });
            float T = 1.75f;
            for (int32 i : Order)
            {
                FCard& C = Cards[i];
                const FTier Tier = TierOf(C.Def->Rarity);
                C.Suspense = Tier.Rank == 3 ? .9f : Tier.Rank == 2 ? .35f : 0.f;
                C.RevealAt = T;
                T += C.Suspense + .25f + .45f + Tier.Hold;
            }
            AllRevealedAt = T;
            if (Args._Revealed) Time = AllRevealedAt + 1.f;
            auto Load = [this](const TCHAR* Name) -> FSlateBrush*
            {
                UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/UI/Cards/%s.%s"), Name, Name));
                if (!Texture) return nullptr;
                Textures.Add(TStrongObjectPtr<UTexture2D>(Texture));
                TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
                Brush->SetResourceObject(Texture);
                Brush->ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
                Brush->DrawAs = ESlateBrushDrawType::Image;
                Brushes.Add(Name, Brush);
                return Brush.Get();
            };
            for (const TCHAR* Name : { TEXT("T_Card_Back"), TEXT("T_Card_Bronze"), TEXT("T_Card_Silver"), TEXT("T_Card_Gold"), TEXT("T_Card_Diamond"), TEXT("T_Card_Glow"), TEXT("T_Card_Rays") }) Load(Name);
            White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
            RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SVortexAwakening::Animate));
        }

        virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1920.f, 1080.f); }
        virtual bool SupportsKeyboardFocus() const override { return true; }

        virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& Key) override
        {
            if (Key.GetKey() == EKeys::Escape && Chosen == INDEX_NONE) { VortexAwakening::Close(); return FReply::Handled(); }
            if (Key.GetKey() == EKeys::SpaceBar && Time < AllRevealedAt) { Time = AllRevealedAt; return FReply::Handled(); }
            return FReply::Unhandled();
        }

        virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override
        {
            Mouse = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
            return FReply::Unhandled();
        }

        virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
        {
            Mouse = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
            if (Chosen != INDEX_NONE) return FReply::Handled();
            if (Time < AllRevealedAt) { Time = AllRevealedAt; return FReply::Handled(); }   // clique pula a revelação
            if (Selected != INDEX_NONE && ConfirmBox.ContainsPoint(FVector2f(Mouse)))
            {
                Chosen = Selected;
                ChosenAt = Time;
                if (AVortexCharacter* H = Hero.Get(); H && H->GetSheet()) H->GetSheet()->ServerChooseClass(Cards[Chosen].Def->Id);
                VortexAudio::Play2D(Hero.Get(), TEXT("holy_cast"), 1.f);
                VortexAudio::Play2D(Hero.Get(), TEXT("level_up"), .8f);
                return FReply::Handled();
            }
            const int32 Hit = CardAt(Mouse);
            if (Hit != INDEX_NONE) { Selected = Hit; VortexAudio::Play2D(Hero.Get(), TEXT("ui_click"), .6f); }
            return FReply::Handled();
        }

        virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer,
            const FWidgetStyle&, bool) const override
        {
            const FVector2f Size = FVector2f(Geometry.GetLocalSize());
            const float W = Size.X, H = Size.Y;
            const float Scale = H / 1080.f;
            // fundo: escurece e acende um brilho no meio
            const float In = CardEase(Time / .6f) * (Chosen != INDEX_NONE ? 1.f - CardEase((Time - ChosenAt - 2.2f) / .6f) : 1.f);
            Box(Out, Layer, Geometry, FVector2f(0, 0), Size, White, FLinearColor(.01f, .005f, .02f, .86f * In));
            if (const FSlateBrush* Glow = Brush(TEXT("T_Card_Glow")))
                Box(Out, Layer + 1, Geometry, FVector2f(W * .5f - H * .6f, H * .42f - H * .6f), FVector2f(H * 1.2f, H * 1.2f), Glow, FLinearColor(.45f, .3f, .9f, .22f * In));
            // título
            const float TitleA = CardEase((Time - .2f) / .6f) * In;
            Text(Out, Layer + 2, Geometry, TEXT("O DESPERTAR"), FVector2f(W * .5f, H * .085f), 54.f * Scale, FLinearColor(1.f, .86f, .5f, TitleA), true);
            Text(Out, Layer + 2, Geometry, Chosen != INDEX_NONE ? FString::Printf(TEXT("%s desperta como %s"), *HeroName, *Cards[Chosen].Def->Name)
                : FString::Printf(TEXT("A seiva do Vórtice mostra seis caminhos para %s"), *HeroName),
                FVector2f(W * .5f, H * .145f), 22.f * Scale, FLinearColor(.85f, .82f, .95f, TitleA), true);
            // as cartas
            const float CardH = FMath::Min(H * .4f, W * .2f), CardW = CardH * 2.f / 3.f;
            const float Gap = CardW * .14f;
            const float Total = Cards.Num() * CardW + (Cards.Num() - 1) * Gap;
            const float X0 = (W - Total) * .5f, CY = H * .44f;
            const int32 Hovered = Time >= AllRevealedAt && Chosen == INDEX_NONE ? CardAt(Mouse) : INDEX_NONE;
            CardRects.SetNum(Cards.Num());
            float Flash = 0.f;
            for (int32 i = 0; i < Cards.Num(); ++i)
            {
                const FCard& C = Cards[i];
                const FTier Tier = TierOf(C.Def->Rarity);
                // chegada: do pé da tela até o lugar, girando
                const float Fly = EaseOutBack((Time - (.45f + i * .09f)) / .6f);
                const float Slot = i - (Cards.Num() - 1) * .5f;
                FVector2f Center(X0 + i * (CardW + Gap) + CardW * .5f, CY + Slot * Slot * 6.f * Scale);
                FVector2f From(W * .5f, H + CardH);
                FVector2f At = From + (Center - From) * Fly;
                float Angle = FMath::Lerp(25.f * (Slot < 0 ? -1.f : 1.f), Slot * 2.f, Fly);
                float ScaleX = 1.f, ScaleY = 1.f;
                bool bFront = Time >= C.RevealAt + C.Suspense + .25f + .225f;
                float Lift = 0.f;
                // suspense (épica/lendária): treme e brilha antes de virar
                if (Time >= C.RevealAt && Time < C.RevealAt + C.Suspense)
                {
                    const float K = (Time - C.RevealAt) / C.Suspense;
                    At += FVector2f(FMath::Sin(Time * 60.f) * 4.f * K, FMath::Cos(Time * 47.f) * 3.f * K) * Scale;
                }
                // levanta e vira (escala horizontal passa por zero e mostra a frente)
                const float FlipStart = C.RevealAt + C.Suspense;
                if (Time >= FlipStart)
                {
                    Lift = CardEase((Time - FlipStart) / .25f) * (1.f - CardEase((Time - FlipStart - .9f) / .4f));
                    const float P = FMath::Clamp((Time - FlipStart - .25f) / .45f, 0.f, 1.f);
                    ScaleX = P < 1.f ? FMath::Max(.02f, FMath::Abs(FMath::Cos(P * UE_PI))) : 1.f;
                    ScaleY = 1.f + .06f * Lift;
                    if (P > .5f && !C.bSounded) { const_cast<FCard&>(C).bSounded = true; VortexAudio::Play2D(Hero.Get(), Tier.Sound, Tier.Rank == 3 ? 1.f : .8f); if (Tier.Rank == 3) VortexAudio::Play2D(Hero.Get(), TEXT("holy_cast"), .9f); }
                    if (Tier.Rank == 3) Flash = FMath::Max(Flash, 1.f - FMath::Clamp((Time - FlipStart - .48f) / .9f, 0.f, 1.f));
                }
                const bool bHover = Hovered == i, bSel = Selected == i;
                float Pop = (bHover ? .06f : 0.f) + (bSel ? .1f : 0.f);
                At.Y -= (Lift * 26.f + Pop * 120.f) * Scale;
                // escolhida: vai para o meio e cresce; as outras caem e somem
                float Alpha = Fly > 0.f ? 1.f : 0.f;
                if (Chosen != INDEX_NONE)
                {
                    const float K = CardEase((Time - ChosenAt) / .7f);
                    if (i == Chosen) { At = At + (FVector2f(W * .5f, H * .47f) - At) * K; Angle *= 1.f - K; Pop = .35f * K; }
                    else { At.Y += K * H * .5f; Alpha = 1.f - K; }
                    Alpha *= In;
                }
                const float S = (1.f + Pop) * Alpha;
                if (Alpha <= 0.01f) continue;
                const FVector2f CardSize(CardW, CardH);
                CardRects[i] = FSlateRect(At.X - CardW * .5f, At.Y - CardH * .5f, At.X + CardW * .5f, At.Y + CardH * .5f);
                const FSlateRenderTransform Transform(Concatenate(FScale2f(ScaleX * S, ScaleY * S), FQuat2f(FMath::DegreesToRadians(Angle))));
                // atrás da carta virada: o brilho da raridade (e os raios girando na lendária)
                if (bFront)
                {
                    const float Pulse = .75f + .25f * FMath::Sin(Time * 2.5f + i);
                    if (Tier.Rank == 3)
                        if (const FSlateBrush* Rays = Brush(TEXT("T_Card_Rays")))
                            Box(Out, Layer + 3, Geometry, At - FVector2f(CardH, CardH) * 1.1f, FVector2f(CardH, CardH) * 2.2f, Rays, FLinearColor(Tier.Color.R, Tier.Color.G, Tier.Color.B, .55f * Alpha),
                                FSlateRenderTransform(FQuat2f(Time * .35f)));
                    if (const FSlateBrush* Glow = Brush(TEXT("T_Card_Glow")))
                        Box(Out, Layer + 4, Geometry, At - FVector2f(CardW, CardH) * .95f, FVector2f(CardW, CardH) * 1.9f, Glow,
                            FLinearColor(Tier.Color.R, Tier.Color.G, Tier.Color.B, (bSel ? .9f : .45f + .25f * (Tier.Rank / 3.f)) * Pulse * Alpha));
                }
                const FSlateBrush* Face = Brush(bFront ? Tier.Frame : TEXT("T_Card_Back"));
                if (Face) Box(Out, Layer + 5, Geometry, At - CardSize * .5f, CardSize, Face, FLinearColor(1, 1, 1, Alpha), Transform);
                if (bFront && ScaleX > .97f) PaintFace(Out, Layer + 6, Geometry, C, Tier, At, CardSize, Transform, Alpha, Scale);
                if (bSel && bFront)
                {
                    // contorno dourado da carta escolhida
                    Box(Out, Layer + 7, Geometry, At - CardSize * .5f - FVector2f(4, 4), CardSize + FVector2f(8, 8), FCoreStyle::Get().GetBrush(TEXT("Border")),
                        FLinearColor(1.f, .85f, .4f, Alpha), Transform);
                }
            }
            // clarão da lendária
            if (Flash > 0.f) Box(Out, Layer + 20, Geometry, FVector2f(0, 0), Size, White, FLinearColor(1.f, .95f, 1.f, .65f * Flash * Flash));
            // painel de detalhes e o botão
            if (Time >= AllRevealedAt && Chosen == INDEX_NONE) PaintDetails(Out, Layer + 10, Geometry, W, H, Scale, Hovered != INDEX_NONE ? Hovered : Selected);
            else if (Time < AllRevealedAt)
                Text(Out, Layer + 2, Geometry, TEXT("Clique ou Espaço para revelar tudo"), FVector2f(W * .5f, H * .93f), 16.f * Scale, FLinearColor(.7f, .68f, .8f, .7f * In), true);
            return Layer + 21;
        }

    public:
        void Pick(int32 Index) { if (Cards.IsValidIndex(Index)) { Time = FMath::Max(Time, AllRevealedAt); Selected = Index; } }
        void Confirm()
        {
            if (!Cards.IsValidIndex(Selected) || Chosen != INDEX_NONE) return;
            Chosen = Selected;
            ChosenAt = Time;
            if (AVortexCharacter* H = Hero.Get(); H && H->GetSheet()) H->GetSheet()->ServerChooseClass(Cards[Chosen].Def->Id);
        }
        int32 Rarest() const { return Order.Num() ? Order.Last() : INDEX_NONE; }

    private:
        struct FCard { const FVortexClassDef* Def = nullptr; float RevealAt = 0.f, Suspense = 0.f; bool bSounded = false; };
        TWeakObjectPtr<AVortexCharacter> Hero;
        FString HeroName;
        FName WeaponType;
        TArray<FCard> Cards;
        TArray<int32> Order;
        float Time = 0.f, AllRevealedAt = 5.f, ChosenAt = 0.f;
        int32 Selected = INDEX_NONE, Chosen = INDEX_NONE;
        FVector2D Mouse = FVector2D::ZeroVector;
        mutable TArray<FSlateRect> CardRects;
        mutable FSlateRect ConfirmBox;
        TArray<TStrongObjectPtr<UTexture2D>> Textures;
        TMap<FString, TSharedPtr<FSlateBrush>> Brushes;
        mutable TMap<UTexture2D*, TSharedPtr<FSlateBrush>> IconBrushes;
        const FSlateBrush* White = nullptr;

        EActiveTimerReturnType Animate(double, float Delta)
        {
            Time += Delta;
            if (Chosen != INDEX_NONE && Time > ChosenAt + 3.f) { VortexAwakening::Close(); return EActiveTimerReturnType::Stop; }
            Invalidate(EInvalidateWidgetReason::Paint);
            return EActiveTimerReturnType::Continue;
        }

        int32 CardAt(const FVector2D& P) const
        {
            for (int32 i = CardRects.Num() - 1; i >= 0; --i) if (CardRects[i].ContainsPoint(FVector2f(P))) return i;
            return INDEX_NONE;
        }

        const FSlateBrush* Brush(const TCHAR* Name) const
        {
            const TSharedPtr<FSlateBrush>* Found = Brushes.Find(Name);
            return Found ? Found->Get() : nullptr;
        }

        const FSlateBrush* IconBrush(FName Icon) const
        {
            UTexture2D* Texture = FVortexRpgCatalog::Icon(Icon);
            if (!Texture) return nullptr;
            TSharedPtr<FSlateBrush>& B = IconBrushes.FindOrAdd(Texture);
            if (!B)
            {
                B = MakeShared<FSlateBrush>();
                B->SetResourceObject(Texture);
                B->ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
                B->DrawAs = ESlateBrushDrawType::Image;
            }
            return B.Get();
        }

        static void Box(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Pos, const FVector2f& Size, const FSlateBrush* B,
            const FLinearColor& Tint, const FSlateRenderTransform& Transform = FSlateRenderTransform())
        {
            if (!B) return;
            FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos), Transform, FVector2f(.5f, .5f)), B, ESlateDrawEffect::None, Tint);
        }

        static FVector2f Measure(const FString& Value, const FSlateFontInfo& Font)
        {
            return FVector2f(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Value, Font));
        }

        static void Text(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FString& Value, const FVector2f& At, float SizePt, const FLinearColor& Color,
            bool bCenter, bool bBold = true, float MaxWidth = 0.f)
        {
            FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", FMath::Max(6, FMath::RoundToInt(SizePt)));
            FVector2f Sz = Measure(Value, Font);
            if (MaxWidth > 0.f && Sz.X > MaxWidth)
            {
                Font.Size = FMath::Max(6, FMath::FloorToInt(Font.Size * MaxWidth / Sz.X));
                Sz = Measure(Value, Font);
            }
            const FVector2f Pos = bCenter ? At - FVector2f(Sz.X * .5f, Sz.Y * .5f) : At;
            // sombra e texto
            FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(Sz, FSlateLayoutTransform(Pos + FVector2f(1.5f, 1.5f))), Value, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, Color.A * .8f));
            FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(Sz, FSlateLayoutTransform(Pos)), Value, Font, ESlateDrawEffect::None, Color);
        }

        void PaintFace(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FCard& C, const FTier& Tier, const FVector2f& At, const FVector2f& CardSize,
            const FSlateRenderTransform& Transform, float Alpha, float Scale) const
        {
            // posições na arte da carta (512 x 768)
            auto P = [&](float X, float Y) { return At + FVector2f((X / 512.f - .5f) * CardSize.X, (Y / 768.f - .5f) * CardSize.Y); };
            const float Unit = CardSize.Y / 768.f;
            if (const FSlateBrush* Icon = IconBrush(C.Def->Icon))
            {
                const float S = 170.f * Unit;
                Box(Out, Layer, G, P(256.f, 250.f) - FVector2f(S, S) * .5f, FVector2f(S, S), Icon, FLinearColor(1, 1, 1, Alpha));
            }
            Text(Out, Layer + 1, G, C.Def->Name.ToUpper(), P(256.f, 467.f), 44.f * Unit, FLinearColor(1.f, .95f, .85f, Alpha), true, true, 360.f * Unit);
            Text(Out, Layer + 1, G, FString::Printf(TEXT("%s  •  %s"), *RarityName(C.Def->Rarity), Tier.Metal), P(256.f, 524.f), 26.f * Unit, FLinearColor(Tier.Color.R, Tier.Color.G, Tier.Color.B, Alpha), true, true, 400.f * Unit);
            Text(Out, Layer + 1, G, C.Def->Role, P(256.f, 568.f), 22.f * Unit, FLinearColor(.82f, .8f, .9f, Alpha), true, false, 410.f * Unit);
            int32 Line = 0;
            for (const TPair<FName, float>& Buff : C.Def->Buffs)
            {
                if (Line >= 3) break;
                Text(Out, Layer + 1, G, FVortexRpgCatalog::StatLine(Buff.Key, Buff.Value), P(256.f, 616.f + Line * 38.f), 25.f * Unit, FLinearColor(.55f, 1.f, .6f, Alpha), true, true, 400.f * Unit);
                ++Line;
            }
            if (!WeaponType.IsNone() && C.Def->Weapons.Contains(WeaponType))
                Text(Out, Layer + 1, G, TEXT("★ LUTA COM A SUA ARMA"), P(256.f, 78.f), 22.f * Unit, FLinearColor(1.f, .86f, .4f, Alpha), true, true, 380.f * Unit);
        }

        void PaintDetails(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, float W, float H, float Scale, int32 Index) const
        {
            const float Top = H * .7f, PanelH = H * .27f;
            const float Left = W * .08f, PanelW = W * .84f;
            Box(Out, Layer, G, FVector2f(Left, Top), FVector2f(PanelW, PanelH), White, FLinearColor(.03f, .025f, .06f, .88f));
            Box(Out, Layer, G, FVector2f(Left, Top), FVector2f(PanelW, 2.f), White, FLinearColor(1.f, .8f, .4f, .6f));
            ConfirmBox = FSlateRect();
            if (!Cards.IsValidIndex(Index))
            {
                Text(Out, Layer + 1, G, TEXT("Escolha uma carta para ver a classe inteira  •  Esc fecha (a aba Classes, na janela do herói, abre de novo)"),
                    FVector2f(W * .5f, Top + PanelH * .5f), 19.f * Scale, FLinearColor(.85f, .82f, .95f, 1.f), true, false);
                return;
            }
            const FVortexClassDef& C = *Cards[Index].Def;
            const FTier Tier = TierOf(C.Rarity);
            const float X = Left + 28.f * Scale;
            float Y = Top + 18.f * Scale;
            Text(Out, Layer + 1, G, C.Name, FVector2f(X, Y), 32.f * Scale, FLinearColor(1.f, .93f, .8f, 1.f), false);
            Text(Out, Layer + 1, G, FString::Printf(TEXT("%s  •  carta de %s  •  %s"), *RarityName(C.Rarity), *FString(Tier.Metal).ToLower(), *C.Role),
                FVector2f(X, Y + 44.f * Scale), 17.f * Scale, Tier.Color, false, false);
            Text(Out, Layer + 1, G, FString::Printf(TEXT("Passiva — %s: %s"), *C.PassiveName, *C.PassiveText), FVector2f(X, Y + 74.f * Scale), 16.f * Scale, FLinearColor(.85f, .82f, .92f), false, false);
            TArray<FString> Buffs;
            for (const TPair<FName, float>& B : C.Buffs) Buffs.Add(FVortexRpgCatalog::StatLine(B.Key, B.Value));
            TArray<FString> Weapons;
            for (const FName& Wpn : C.Weapons) Weapons.Add((Wpn == WeaponType ? TEXT("★ ") : TEXT("")) + Wpn.ToString().Replace(TEXT("_"), TEXT(" ")));
            Text(Out, Layer + 1, G, TEXT("Bônus: ") + FString::Join(Buffs, TEXT("   ")), FVector2f(X, Y + 102.f * Scale), 16.f * Scale, FLinearColor(.55f, 1.f, .6f), false, false);
            Text(Out, Layer + 1, G, TEXT("Armas: ") + FString::Join(Weapons, TEXT(", ")), FVector2f(X, Y + 128.f * Scale), 15.f * Scale, FLinearColor(.75f, .72f, .82f), false, false);
            // as 4 habilidades
            if (const TArray<FVortexClassSkill>* Skills = FVortexRpgCatalog::Get().SkillsOf(C.Id))
            {
                const float SkillW = PanelW * .2f;
                for (int32 k = 0; k < Skills->Num() && k < 4; ++k)
                {
                    const FVortexClassSkill& S = (*Skills)[k];
                    const float SX = X + k * (SkillW + 12.f * Scale), SY = Y + 160.f * Scale;
                    Box(Out, Layer + 1, G, FVector2f(SX, SY), FVector2f(SkillW, 66.f * Scale), White, FLinearColor(.08f, .07f, .12f, .9f));
                    if (const FSlateBrush* Icon = IconBrush(S.Icon)) Box(Out, Layer + 2, G, FVector2f(SX + 6.f * Scale, SY + 7.f * Scale), FVector2f(52.f, 52.f) * Scale, Icon, FLinearColor::White);
                    Text(Out, Layer + 2, G, FString::Printf(TEXT("%d  %s"), k + 1, *S.Name), FVector2f(SX + 66.f * Scale, SY + 8.f * Scale), 15.f * Scale,
                        UVortexClassSkills::ElementColor(S.Element), false, true, SkillW - 74.f * Scale);
                    Text(Out, Layer + 2, G, FString::Printf(TEXT("%s  •  %d mana  •  %.0f s"), *S.Category(), S.Mana, S.Cooldown), FVector2f(SX + 66.f * Scale, SY + 34.f * Scale),
                        12.f * Scale, FLinearColor(.7f, .7f, .8f), false, false, SkillW - 74.f * Scale);
                }
            }
            // o botão (só com uma carta escolhida)
            if (Selected == Index)
            {
                const FVector2f BtnSize(470.f * Scale, 58.f * Scale);
                const FVector2f BtnPos(Left + PanelW - BtnSize.X - 26.f * Scale, Top + 22.f * Scale);
                const bool bHover = FSlateRect(BtnPos.X, BtnPos.Y, BtnPos.X + BtnSize.X, BtnPos.Y + BtnSize.Y).ContainsPoint(FVector2f(Mouse));
                Box(Out, Layer + 1, G, BtnPos, BtnSize, White, bHover ? FLinearColor(1.f, .82f, .4f) : FLinearColor(.86f, .66f, .28f));
                Text(Out, Layer + 2, G, FString::Printf(TEXT("DESPERTAR COMO %s"), *C.Name.ToUpper()), BtnPos + BtnSize * .5f, 19.f * Scale, FLinearColor(.08f, .05f, .02f), true, true, BtnSize.X - 24.f * Scale);
                Text(Out, Layer + 1, G, TEXT("É para sempre."), FVector2f(BtnPos.X + BtnSize.X * .5f, BtnPos.Y + BtnSize.Y + 16.f * Scale), 13.f * Scale, FLinearColor(.75f, .7f, .8f), true, false);
                ConfirmBox = FSlateRect(BtnPos.X, BtnPos.Y, BtnPos.X + BtnSize.X, BtnPos.Y + BtnSize.Y);
            }
            else Text(Out, Layer + 1, G, TEXT("Clique para escolher esta carta"), FVector2f(Left + PanelW - 220.f * Scale, Top + 50.f * Scale), 16.f * Scale, FLinearColor(1.f, .85f, .5f), true, false);
        }
    };

    TSharedPtr<SVortexAwakening> Window;
    TWeakObjectPtr<APlayerController> WindowOwner;
}

namespace VortexAwakeningImpl
{
    bool IsOpen() { return Window.IsValid(); }

    void Open(AVortexCharacter* Hero, bool bAlreadyRevealed)
    {
        if (IsOpen() || !Hero || !Hero->GetSheet() || Hero->GetSheet()->GetClassCards().IsEmpty() || !GEngine || !GEngine->GameViewport) return;
        APlayerController* Controller = Cast<APlayerController>(Hero->GetController());
        if (!Controller) return;
        Window = SNew(SVortexAwakening).Hero(Hero).Revealed(bAlreadyRevealed);
        WindowOwner = Controller;
        GEngine->GameViewport->AddViewportWidgetContent(Window.ToSharedRef(), 60);
        Controller->SetShowMouseCursor(true);
        FInputModeUIOnly Mode;
        Mode.SetWidgetToFocus(Window);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Controller->SetInputMode(Mode);
        VortexAudio::Play2D(Hero, TEXT("boss_awaken"), .5f);
        UE_LOG(LogTemp, Display, TEXT("VORTEX_AWAKENING aberto (%d cartas)"), Hero->GetSheet()->GetClassCards().Num());
    }

    void DebugPick(int32 Index) { if (Window.IsValid()) Window->Pick(Index); }
    void DebugConfirm() { if (Window.IsValid()) Window->Confirm(); }
    int32 DebugRarest() { return Window.IsValid() ? Window->Rarest() : INDEX_NONE; }

    void Close()
    {
        if (!IsOpen()) return;
        if (GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Window.ToSharedRef());
        Window.Reset();
        if (APlayerController* Controller = WindowOwner.Get())
        {
            Controller->SetShowMouseCursor(false);
            Controller->SetInputMode(FInputModeGameOnly());
        }
        WindowOwner.Reset();
    }
}
