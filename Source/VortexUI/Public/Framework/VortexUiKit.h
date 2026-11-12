#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateBoxBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateTypes.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"

/**
 * Kit "Cartoon" das janelas do mundo (herói, mochila, técnicas, classe, alquimia, missões, loja), no lugar do
 * "Aventura" em pixel-art, que brigava com a letra de desenho animado (Tools/MakeUiKitCartoon.py,
 * /Game/Vortex/UI/KitCartoon). Azul vivo, cantos redondos, contorno escuro grosso, botões coloridos com a parte de
 * baixo mais escura. Mesmas funções do Aventura (trocar o apelido no .cpp volta o visual antigo) e mais: botões
 * vermelho e verde, a fita tingida e as peças dos efeitos (estrela, faísca, anel).
 */
namespace VortexUiKit::Cartoon
{
    inline UTexture2D* Texture(const TCHAR* Name)
    {
        static TMap<FString, UTexture2D*> Loaded;
        UTexture2D*& Found = Loaded.FindOrAdd(Name);
        if (!Found)
        {
            Found = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/UI/KitCartoon/%s.%s"), Name, Name));
            if (Found) Found->AddToRoot();
        }
        return Found;
    }

    /** Peça de nove fatias (Margin em fração da textura), opcionalmente tingida. */
    inline const FSlateBrush* Box(const TCHAR* Name, const FMargin& Margin, const FLinearColor& Tint = FLinearColor::White)
    {
        static TMap<FString, TSharedPtr<FSlateBoxBrush>> Cache;
        const FString Key = FString::Printf(TEXT("%s|%.3f|%.3f|%s"), Name, Margin.Left, Margin.Top, *Tint.ToString());
        TSharedPtr<FSlateBoxBrush>& Brush = Cache.FindOrAdd(Key);
        if (!Brush) Brush = MakeShared<FSlateBoxBrush>(Texture(Name), Margin, FSlateColor(Tint));
        return Brush.Get();
    }

    inline const FSlateBrush* Image(const TCHAR* Name, const FVector2D& Size)
    {
        static TMap<FString, TSharedPtr<FSlateImageBrush>> Cache;
        TSharedPtr<FSlateImageBrush>& Brush = Cache.FindOrAdd(FString::Printf(TEXT("%s|%.0f|%.0f"), Name, Size.X, Size.Y));
        if (!Brush) Brush = MakeShared<FSlateImageBrush>(Texture(Name), Size);
        return Brush.Get();
    }

    // 07/11: a fita dos títulos no vermelho-vinho com ouro das referências do usuário (era laranja)
    const FLinearColor Orange = FLinearColor::FromSRGBColor(FColor(150, 36, 42));
    const FLinearColor Violet = FLinearColor::FromSRGBColor(FColor(150, 92, 255));
    const FLinearColor Teal = FLinearColor::FromSRGBColor(FColor(36, 178, 166));

    /** A janela inteira (azul, cheia, com faixa clara em cima). */
    inline const FSlateBrush* Frame() { return Box(TEXT("W_Frame"), FMargin(.3f)); }
    /** Nada: a moldura do kit cartoon já é cheia (fica para o código igual ao do Aventura). */
    inline const FSlateBrush* Stone() { return Box(TEXT("W_StoneTile"), FMargin(0.f)); }
    /** Painel afundado azul-escuro (colunas, listas). */
    inline const FSlateBrush* Panel() { return Box(TEXT("W_Panel"), FMargin(.3f)); }
    /** Espaço de item: 0 normal, 1 com o mouse, 2 escolhido (aro amarelo por cima). */
    inline const FSlateBrush* Slot(int32 State = 0) { return Box(State == 2 ? TEXT("W_SlotActive") : State == 1 ? TEXT("W_SlotHover") : TEXT("W_Slot"), FMargin(.3f)); }
    inline const FSlateBrush* SlotGlow(const FLinearColor& Color) { return Box(TEXT("W_SlotGlow"), FMargin(.3f), Color); }
    /** Cartão em relevo; trancado: cinza. */
    inline const FSlateBrush* Card(bool bLocked = false) { return Box(bLocked ? TEXT("W_CardLocked") : TEXT("W_Card"), FMargin(.3f)); }
    /** A fita dos títulos, em pílula, na cor pedida (laranja por padrão). */
    inline const FSlateBrush* Ribbon(const FLinearColor& Tint = Orange) { return Box(TEXT("W_Ribbon"), FMargin(.25f, .45f), Tint); }
    inline const FSlateBrush* Key() { return Box(TEXT("W_Key"), FMargin(.3f, .35f)); }
    /** 07/11: a folha de pergaminho (o diário de missões, a fala do vendedor): letra escura por cima. */
    inline const FSlateBrush* Parchment() { return Box(TEXT("W_Parchment"), FMargin(.3f)); }
    inline const FSlateBrush* BarBack() { return Box(TEXT("W_BarBack"), FMargin(.25f, .45f)); }
    inline const FSlateBrush* BarFill() { return Box(TEXT("W_BarFill"), FMargin(.25f, .45f)); }
    /** A placa da vitrine do herói (elipse azul). */
    inline const FSlateBrush* Stage() { return Image(TEXT("W_Stage"), FVector2D(192.f, 72.f)); }
    /** Os efeitos dos cliques (brancos, tingidos ao desenhar). */
    inline const FSlateBrush* Star() { return Image(TEXT("W_Star"), FVector2D(64.f, 64.f)); }
    inline const FSlateBrush* Spark() { return Image(TEXT("W_Spark"), FVector2D(32.f, 32.f)); }
    inline const FSlateBrush* Ring() { return Image(TEXT("W_Ring"), FVector2D(64.f, 64.f)); }

    /** Botões: 0 azul, 1 amarelo (principal), 2 aba, 3 aba escolhida, 4 espaço de item, 5 vermelho, 6 verde. */
    inline const FButtonStyle& Button(int32 Kind)
    {
        static FButtonStyle Styles[7];
        static bool bReady = false;
        if (!bReady)
        {
            bReady = true;
            // apertado: o conteúdo desce (a parte escura de baixo some) — parece que o botão afundou
            auto Make = [](FButtonStyle& S, const FSlateBrush* Normal, const FSlateBrush* Hover, const FSlateBrush* Pressed, const FSlateBrush* Disabled)
            {
                S = FButtonStyle().SetNormal(*Normal).SetHovered(*Hover).SetPressed(*Pressed).SetDisabled(*Disabled)
                    .SetNormalPadding(FMargin(0.f, 0.f, 0.f, 4.f)).SetPressedPadding(FMargin(0.f, 4.f, 0.f, 0.f));
            };
            const FMargin M(.25f, .4f);
            const FSlateBrush* Off = Box(TEXT("W_ButtonDisabled"), M);
            Make(Styles[0], Box(TEXT("W_Button"), M), Box(TEXT("W_ButtonHover"), M), Box(TEXT("W_ButtonPressed"), M), Off);
            Make(Styles[1], Box(TEXT("W_ButtonGold"), M), Box(TEXT("W_ButtonGoldHover"), M), Box(TEXT("W_ButtonGold"), M, FLinearColor(.85f, .85f, .85f)), Off);
            const FMargin T(.3f);
            Make(Styles[2], Box(TEXT("W_Tab"), T), Box(TEXT("W_Tab"), T, FLinearColor(1.25f, 1.25f, 1.3f)), Box(TEXT("W_TabActive"), T), Box(TEXT("W_Tab"), T));
            Make(Styles[3], Box(TEXT("W_TabActive"), T), Box(TEXT("W_TabActive"), T), Box(TEXT("W_TabActive"), T), Box(TEXT("W_TabActive"), T));
            Make(Styles[4], Slot(0), Slot(1), Slot(1), Slot(0));
            Styles[4].SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));
            Make(Styles[5], Box(TEXT("W_ButtonRed"), M), Box(TEXT("W_ButtonRedHover"), M), Box(TEXT("W_ButtonRed"), M, FLinearColor(.85f, .85f, .85f)), Off);
            Make(Styles[6], Box(TEXT("W_ButtonGreen"), M), Box(TEXT("W_ButtonGreenHover"), M), Box(TEXT("W_ButtonGreen"), M, FLinearColor(.85f, .85f, .85f)), Off);
        }
        return Styles[FMath::Clamp(Kind, 0, 6)];
    }

    /** A letra de desenho animado com o contorno azul-escuro do kit. */
    inline FSlateFontInfo Font(int32 Size, int32 OutlineSize = 2)
    {
        static TSharedPtr<const FCompositeFont> Composite = []()
        {
            const FString Path = FPaths::ProjectContentDir() / TEXT("Vortex/UI/Fonts/Cinzel-900.ttf");   // 07/11: a letra medieval (combina com as habilidades)
            return FPaths::FileExists(Path) ? TSharedPtr<const FCompositeFont>(MakeShared<FStandaloneCompositeFont>(NAME_None, Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad))
                : TSharedPtr<const FCompositeFont>();
        }();
        FSlateFontInfo Info = Composite.IsValid() ? FSlateFontInfo(Composite, FMath::Max(6, FMath::RoundToInt(Size * .84f))) :   // (a Cinzel é mais larga)
            FCoreStyle::GetDefaultFontStyle("Bold", Size);
        Info.OutlineSettings.OutlineSize = OutlineSize;
        Info.OutlineSettings.OutlineColor = FLinearColor::FromSRGBColor(FColor(20, 20, 50));
        return Info;
    }
    inline FSlateFontInfo Cartoon(int32 Size, int32 OutlineSize = 2) { return Font(Size, OutlineSize); }
}

/**
 * O kit das janelas no estilo da HUD (Tools/MakeUiKit.py, /Game/Vortex/UI/Kit): azul-noite com moldura
 * dourada em relevo e gemas azuis. Pincéis de nove fatias (os cantos ficam, o meio estica), usados
 * pela janela do herói, pelo menu inicial, pela pausa, pelas opções e pelas janelas de jogador.
 */
namespace VortexUiKit
{
    inline UTexture2D* KitTexture(const TCHAR* Name)
    {
        static TMap<FString, UTexture2D*> Loaded;
        UTexture2D*& Texture = Loaded.FindOrAdd(Name);
        if (!Texture)
        {
            Texture = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/UI/Kit/%s.%s"), Name, Name));
            if (Texture) Texture->AddToRoot();
        }
        return Texture;
    }

    /** Pincel de nove fatias (Margin em fração da textura). */
    inline const FSlateBrush* KitBox(const TCHAR* Name, float Margin, const FLinearColor& Tint = FLinearColor::White)
    {
        static TMap<FString, TSharedPtr<FSlateBoxBrush>> Cache;
        const FString Key = FString::Printf(TEXT("%s|%.3f|%s"), Name, Margin, *Tint.ToString());
        TSharedPtr<FSlateBoxBrush>& Brush = Cache.FindOrAdd(Key);
        if (!Brush)
        {
            Brush = MakeShared<FSlateBoxBrush>(KitTexture(Name), FMargin(Margin), FSlateColor(Tint));
        }
        return Brush.Get();
    }

    /** Pincel de nove fatias com margens diferentes na horizontal e na vertical. */
    inline const FSlateBrush* KitBoxMargin(const TCHAR* Name, const FMargin& Margin, const FLinearColor& Tint = FLinearColor::White)
    {
        static TMap<FString, TSharedPtr<FSlateBoxBrush>> Cache;
        const FString Key = FString::Printf(TEXT("%s|%.3f|%.3f|%s"), Name, Margin.Left, Margin.Top, *Tint.ToString());
        TSharedPtr<FSlateBoxBrush>& Brush = Cache.FindOrAdd(Key);
        if (!Brush) Brush = MakeShared<FSlateBoxBrush>(KitTexture(Name), Margin, FSlateColor(Tint));
        return Brush.Get();
    }

    /** Imagem inteira do kit num tamanho fixo (não estica: gema, divisória). */
    inline const FSlateBrush* KitImage(const TCHAR* Name, const FVector2D& Size)
    {
        static TMap<FString, TSharedPtr<FSlateImageBrush>> Cache;
        const FString Key = FString::Printf(TEXT("%s|%.0fx%.0f"), Name, Size.X, Size.Y);
        TSharedPtr<FSlateImageBrush>& Brush = Cache.FindOrAdd(Key);
        if (!Brush) Brush = MakeShared<FSlateImageBrush>(KitTexture(Name), Size);
        return Brush.Get();
    }

    /** Linha com relevo (atributos, estatísticas, missões); acesa quando o mouse passa. */
    // 24/10 ("faça do 2 ao 4": a mesma cara cartoon no jogo todo): as funções do kit antigo (menu, pausa,
    // opções, janelas de jogador) passam a dar as peças do kit Cartoon. Os nomes ficam, as telas não mudam.
    inline const FSlateBrush* Row(bool bHover = false) { return Cartoon::Box(bHover ? TEXT("W_RowHover") : TEXT("W_Row"), FMargin(.25f, .4f)); }
    /** A faixa dos títulos (a fita laranja em pílula). */
    inline const FSlateBrush* Banner() { return Cartoon::Ribbon(); }
    /** Gema redonda branca (pinte com ColorAndOpacity). */
    inline const FSlateBrush* Gem(float Size) { return KitImage(TEXT("T_UiGemRound"), FVector2D(Size, Size)); }
    /** Fio dourado com losango no meio. */
    inline const FSlateBrush* Divider(float Width) { return KitImage(TEXT("T_UiDivider"), FVector2D(Width, Width * 14.f / 256.f)); }

    /** Letra no estilo da HUD: contorno escuro (desenho) em volta. */
    inline FSlateFontInfo Outlined(FSlateFontInfo Font, int32 Size = 1)
    {
        Font.OutlineSettings.OutlineSize = Size;
        Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, .01f, .85f);
        return Font;
    }

    /** A janela grande (moldura dourada com as gemas nos cantos). */
    inline const FSlateBrush* Window() { return Cartoon::Frame(); }
    /** Painel de dentro (azul-escuro afundado). */
    inline const FSlateBrush* Panel() { return Cartoon::Panel(); }
    /** Espaço de item: 0 normal, 1 com o mouse, 2 escolhido. */
    inline const FSlateBrush* ItemSlot(int32 State = 0) { return Cartoon::Slot(State); }
    /** A faixa do título. */
    inline const FSlateBrush* Header() { return Cartoon::Ribbon(); }

    /** Botões: 0 comum (azul), 1 dourado (principal), 2 aba, 3 aba escolhida, 4 espaço de item, 5 vermelho, 6 verde. */
    inline const FButtonStyle& Button(int32 Kind) { return Cartoon::Button(Kind); }
}

/**
 * Kit "Aventura" das janelas do mundo (inventário, herói, técnicas, classes, alquimia, missões, loja): pixel-art
 * de jogo de blocos (Tools/MakeUiKitAventura.py, /Game/Vortex/UI/KitAventura). Madeira com aro de ouro e
 * rebites, pedra em blocos ladrilhada, espaços de item afundados, botões e abas de bloco, faixa vermelha.
 */
namespace VortexUiKit::Aventura
{
    inline UTexture2D* Texture(const TCHAR* Name)
    {
        static TMap<FString, UTexture2D*> Loaded;
        UTexture2D*& Found = Loaded.FindOrAdd(Name);
        if (!Found)
        {
            Found = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/UI/KitAventura/%s.%s"), Name, Name));
            if (Found) Found->AddToRoot();
        }
        return Found;
    }

    /** Peça de nove fatias (Margin em fração da textura), opcionalmente tingida. */
    inline const FSlateBrush* Box(const TCHAR* Name, const FMargin& Margin, const FLinearColor& Tint = FLinearColor::White)
    {
        static TMap<FString, TSharedPtr<FSlateBoxBrush>> Cache;
        const FString Key = FString::Printf(TEXT("%s|%.3f|%.3f|%s"), Name, Margin.Left, Margin.Top, *Tint.ToString());
        TSharedPtr<FSlateBoxBrush>& Brush = Cache.FindOrAdd(Key);
        if (!Brush) Brush = MakeShared<FSlateBoxBrush>(Texture(Name), Margin, FSlateColor(Tint));
        return Brush.Get();
    }

    /** A moldura da janela (o meio é vazado: a pedra vem por baixo). */
    inline const FSlateBrush* Frame() { return Box(TEXT("W_Frame"), FMargin(.16f)); }
    /** Pedra em blocos repetida (não estica). */
    inline const FSlateBrush* Stone()
    {
        static TSharedPtr<FSlateImageBrush> Brush;
        if (!Brush)
        {
            Brush = MakeShared<FSlateImageBrush>(Texture(TEXT("W_StoneTile")), FVector2D(96.f, 96.f));
            Brush->Tiling = ESlateBrushTileType::Both;
        }
        return Brush.Get();
    }
    /** Painel afundado na pedra (colunas, listas). */
    inline const FSlateBrush* Panel() { return Box(TEXT("W_Panel"), FMargin(.2f)); }
    /** Espaço de item: 0 normal, 1 com o mouse, 2 escolhido. */
    inline const FSlateBrush* Slot(int32 State = 0) { return Box(State == 2 ? TEXT("W_SlotActive") : State == 1 ? TEXT("W_SlotHover") : TEXT("W_Slot"), FMargin(.25f)); }
    /** O brilho por dentro do espaço, na cor da raridade. */
    inline const FSlateBrush* SlotGlow(const FLinearColor& Color) { return Box(TEXT("W_SlotGlow"), FMargin(.3f), Color); }
    /** Cartão de pedra em relevo (técnicas, habilidades, receitas); trancado: apagado. */
    inline const FSlateBrush* Card(bool bLocked = false) { return Box(bLocked ? TEXT("W_CardLocked") : TEXT("W_Card"), FMargin(.2f)); }
    /** A faixa vermelha dos títulos (as pontas dobradas ficam, o meio estica). */
    inline const FSlateBrush* Ribbon() { return Box(TEXT("W_Ribbon"), FMargin(.17f, .3f)); }
    /** Tecla (as teclas dos comandos). */
    inline const FSlateBrush* Key() { return Box(TEXT("W_Key"), FMargin(.3f)); }
    inline const FSlateBrush* BarBack() { return Box(TEXT("W_BarBack"), FMargin(.3f)); }
    inline const FSlateBrush* BarFill() { return Box(TEXT("W_BarFill"), FMargin(.3f)); }
    /** O chão da vitrine do herói (uma placa em losango). */
    inline const FSlateBrush* Stage()
    {
        static TSharedPtr<FSlateImageBrush> Brush;
        if (!Brush) Brush = MakeShared<FSlateImageBrush>(Texture(TEXT("W_Stage")), FVector2D(192.f, 72.f));
        return Brush.Get();
    }

    /** Botões: 0 comum (pedra), 1 dourado (principal), 2 aba de madeira, 3 aba escolhida, 4 espaço de item. */
    inline const FButtonStyle& Button(int32 Kind)
    {
        static FButtonStyle Styles[5];
        static bool bReady = false;
        if (!bReady)
        {
            bReady = true;
            auto Make = [](FButtonStyle& S, const FSlateBrush* Normal, const FSlateBrush* Hover, const FSlateBrush* Pressed, const FSlateBrush* Disabled)
            {
                S = FButtonStyle().SetNormal(*Normal).SetHovered(*Hover).SetPressed(*Pressed).SetDisabled(*Disabled)
                    .SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f, 2.f, 0.f, 0.f));
            };
            const FMargin M(.25f, .3f);
            Make(Styles[0], Box(TEXT("W_Button"), M), Box(TEXT("W_ButtonHover"), M), Box(TEXT("W_ButtonPressed"), M), Box(TEXT("W_ButtonDisabled"), M));
            Make(Styles[1], Box(TEXT("W_ButtonGold"), M), Box(TEXT("W_ButtonGoldHover"), M), Box(TEXT("W_ButtonGold"), M, FLinearColor(.85f, .85f, .85f)), Box(TEXT("W_ButtonDisabled"), M));
            const FMargin T(.2f);
            Make(Styles[2], Box(TEXT("W_Tab"), T), Box(TEXT("W_Tab"), T, FLinearColor(1.25f, 1.2f, 1.1f)), Box(TEXT("W_TabActive"), T), Box(TEXT("W_Tab"), T));
            Make(Styles[3], Box(TEXT("W_TabActive"), T), Box(TEXT("W_TabActive"), T), Box(TEXT("W_TabActive"), T), Box(TEXT("W_TabActive"), T));
            Make(Styles[4], Slot(0), Slot(1), Slot(2), Slot(0));
        }
        return Styles[FMath::Clamp(Kind, 0, 4)];
    }

    /** A letra de desenho animado (Luckiest Guy, a mesma dos números de dano e do letreiro), com contorno escuro. */
    inline FSlateFontInfo Cartoon(int32 Size, int32 OutlineSize = 2)
    {
        static TSharedPtr<const FCompositeFont> Font = []()
        {
            const FString Path = FPaths::ProjectContentDir() / TEXT("Vortex/UI/Fonts/Cinzel-900.ttf");   // 07/11: a letra medieval (combina com as habilidades)
            return FPaths::FileExists(Path) ? TSharedPtr<const FCompositeFont>(MakeShared<FStandaloneCompositeFont>(NAME_None, Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad))
                : TSharedPtr<const FCompositeFont>();
        }();
        FSlateFontInfo Info = Font.IsValid() ? FSlateFontInfo(Font, FMath::Max(6, FMath::RoundToInt(Size * .84f))) :   // (a Cinzel é mais larga)
            FCoreStyle::GetDefaultFontStyle("Bold", Size);
        Info.OutlineSettings.OutlineSize = OutlineSize;
        Info.OutlineSettings.OutlineColor = FLinearColor(.05f, .03f, .02f, .95f);
        return Info;
    }
}
