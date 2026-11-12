// A HUD no estilo cartoon das janelas ("as huds não combinam mais com o jogo"), o minimapa nítido e o mapa do mundo em
// tela cheia ("o mini mapa e o mapa em si não têm qualidade nem foco real"). As peças vêm de Tools/MakeHudCartoon.py;
// as medidas são as do esboço aprovado (Docs/Images/hud_esboco), em pixels de uma tela 1920x1080.
#include "Framework/VortexAimHUD.h"
#include "Combat/VortexSwords.h"
#include "Creatures/VortexMonster.h"
#include "Creatures/VortexVolcanicBoss.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "CanvasItem.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexClassSkills.h"
#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Creatures/VortexCreature.h"
#include "Creatures/VortexNpc.h"
#include "Creatures/VortexWolf.h"
#include "Dom/JsonObject.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Fonts/CompositeFont.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "Rpg/VortexSocial.h"
#include "Framework/VortexCharacterMovementComponent.h"
#include "Framework/VortexGameInstance.h"
#include "Framework/VortexGameMode.h"
#include "Framework/VortexGameState.h"
#include "Framework/VortexPlayerSettings.h"
#include "Framework/VortexProbe.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Online/VortexOnline.h"
#include "Persistence/VortexPlayerSave.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexQuestLog.h"
#include "Rpg/VortexStory.h"
#include "Serialization/JsonReader.h"
#include "World/VortexWeatherSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "Styling/CoreStyle.h"
#include "World/VortexDungeons.h"
#include "World/VortexFountain.h"
#include "World/VortexLandmarks.h"
#include "World/VortexPointsOfInterest.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexTown.h"

namespace
{
    FLinearColor Srgb(uint8 R, uint8 G, uint8 B, float A = 1.f) { FLinearColor C = FLinearColor::FromSRGBColor(FColor(R, G, B)); C.A = A; return C; }
    const FLinearColor Ink = Srgb(20, 20, 50);
    const FLinearColor Cream = Srgb(255, 248, 230);
    const FLinearColor Gold = Srgb(255, 198, 46);
    const FLinearColor Sky = Srgb(160, 190, 255);
    const FLinearColor HpRed = Srgb(232, 64, 64);
    const FLinearColor MpBlue = Srgb(70, 140, 255);
    const FLinearColor Green = Srgb(98, 200, 72);
    const FLinearColor Faded = Srgb(150, 160, 200);

    FLinearColor WithA(FLinearColor C, float A) { C.A *= A; return C; }

    // a letra de desenho animado (Content/Vortex/UI/Fonts, licença Apache 2.0)
    FSlateFontInfo ToonFont(float Size)
    {
        static TSharedPtr<const FCompositeFont> Cartoon = []()
        {
            const FString Path = FPaths::ProjectContentDir() / TEXT("Vortex/UI/Fonts/Cinzel-900.ttf");   // 07/11: a letra medieval (combina com as habilidades)
            return FPaths::FileExists(Path) ? TSharedPtr<const FCompositeFont>(MakeShared<FStandaloneCompositeFont>(NAME_None, Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad))
                : TSharedPtr<const FCompositeFont>();
        }();
        const int32 Points = FMath::Max(6, FMath::RoundToInt(Size * .63f));   // 07/11: a Cinzel é mais larga que a Luckiest Guy (o texto saía da caixa)
        return Cartoon.IsValid() ? FSlateFontInfo(Cartoon, Points) : FCoreStyle::GetDefaultFontStyle("Bold", Points);
    }

    FString Thousands(int32 Value)
    {
        FString Digits = FString::FromInt(FMath::Abs(Value)), Out;
        for (int32 i = 0; i < Digits.Len(); ++i)
        {
            if (i > 0 && (Digits.Len() - i) % 3 == 0) Out.AppendChar(TEXT('.'));
            Out.AppendChar(Digits[i]);
        }
        return Value < 0 ? TEXT("-") + Out : Out;
    }

    FName WeaponHudIcon(FName WeaponType)
    {
        static TMap<FName, FName> Icons;
        static bool bLoaded = false;
        if (!bLoaded)
        {
            bLoaded = true;
            FString Json;
            TSharedPtr<FJsonObject> Root;
            if (FFileHelper::LoadFileToString(Json, *(FPaths::ProjectDir() / TEXT("Data/Rpg/weapons.json"))) &&
                FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) && Root)
            {
                for (const TSharedPtr<FJsonValue>& Value : Root->GetArrayField(TEXT("types")))
                {
                    const TSharedPtr<FJsonObject> Type = Value->AsObject();
                    FString Id, Icon;
                    if (Type->TryGetStringField(TEXT("id"), Id) && Type->TryGetStringField(TEXT("icon"), Icon))
                        Icons.Add(FName(*Id), FName(*Icon.Replace(TEXT("/"), TEXT("_"))));
                }
            }
        }
        const FName* Icon = Icons.Find(WeaponType);
        return Icon ? *Icon : NAME_None;
    }

    // ---- o pintor do mapa local (1 ponto por bloco) ------------------------------------------------------------------
    enum class EPaintProp : uint8 { Tree, House, Rock };
    struct FPaintProp { FVector2f At; float Scale = 1.f; EPaintProp Kind = EPaintProp::Tree; FColor Color; };

    /** As árvores, casas e pedras da área (lidas aqui, na thread do jogo: os pedaços do continente podem sumir da memória). */
    TArray<FPaintProp> GatherProps(const FVortexTerrainData& Data, const FIntPoint& Origin, int32 Size)
    {
        TArray<FPaintProp> Out;
        const int32 C0X = FMath::FloorToInt(float(Origin.X) / Data.CellsPerChunk), C0Y = FMath::FloorToInt(float(Origin.Y) / Data.CellsPerChunk);
        const int32 C1X = FMath::FloorToInt(float(Origin.X + Size) / Data.CellsPerChunk), C1Y = FMath::FloorToInt(float(Origin.Y + Size) / Data.CellsPerChunk);
        TMap<uint16, TOptional<FPaintProp>> Style;
        for (int32 CY = C0Y; CY <= C1Y; ++CY)
            for (int32 CX = C0X; CX <= C1X; ++CX)
            {
                if (!Data.IsScattered(FIntPoint(CX, CY))) continue;
                const TArray<FVortexTerrainProp>* Props = Data.PropsInChunk(FIntPoint(CX, CY));
                if (!Props) continue;
                for (const FVortexTerrainProp& Prop : *Props)
                {
                    TOptional<FPaintProp>& Kind = Style.FindOrAdd(Prop.Kind);
                    if (!Kind.IsSet())
                    {
                        // a cor de cada espécie no mapa (copa de outono laranja, cerejeira rosa, pinheiro com neve...)
                        const FString Name = Data.Kinds.IsValidIndex(Prop.Kind) ? Data.Kinds[Prop.Kind] : FString();
                        FPaintProp P;
                        if (Name.StartsWith(TEXT("tree")))
                        {
                            P.Kind = EPaintProp::Tree;
                            P.Color = Name.Contains(TEXT("snow")) || Name.Contains(TEXT("ice")) ? FColor(214, 228, 238)
                                : Name.Contains(TEXT("autumn")) || Name.Contains(TEXT("ember")) ? FColor(226, 150, 70)
                                : Name.Contains(TEXT("golden")) ? FColor(226, 182, 54)
                                : Name.Contains(TEXT("cherry")) ? FColor(236, 150, 186)
                                : Name.Contains(TEXT("dead")) || Name.Contains(TEXT("bone")) || Name.Contains(TEXT("haunted")) ? FColor(120, 104, 96)
                                : Name.Contains(TEXT("elven")) || Name.Contains(TEXT("moon")) ? FColor(62, 176, 150)
                                : Name.Contains(TEXT("cactus")) ? FColor(96, 160, 70)
                                : FColor(62, 138, 58);
                        }
                        else if (Name.StartsWith(TEXT("town_")) || Name.StartsWith(TEXT("house_")) || Name.StartsWith(TEXT("elf_house")) || Name == TEXT("vil_stall_a") || Name == TEXT("vil_stall_b"))
                        {
                            P.Kind = EPaintProp::House;
                            P.Color = Name.Contains(TEXT("tower")) || Name.Contains(TEXT("wall")) || Name.Contains(TEXT("gate")) ? FColor(150, 146, 140) : FColor(178, 82, 58);
                        }
                        else if (Name.StartsWith(TEXT("rock")) || Name.Contains(TEXT("boulder")) || Name.Contains(TEXT("spire")) || Name.Contains(TEXT("spike")))
                        {
                            P.Kind = EPaintProp::Rock;
                            P.Color = FColor(132, 130, 128);
                        }
                        else
                        {
                            Kind = FPaintProp();
                            Kind->Scale = -1.f;   // não aparece no mapa
                            continue;
                        }
                        Kind = P;
                    }
                    if (Kind->Scale < 0.f) continue;
                    FPaintProp P = Kind.GetValue();
                    P.At = FVector2f(Prop.Location.X / Data.CellSize - Origin.X, Prop.Location.Y / Data.CellSize - Origin.Y);
                    P.Scale = Prop.Scale;
                    if (P.At.X < -8.f || P.At.Y < -8.f || P.At.X > Size + 8.f || P.At.Y > Size + 8.f) continue;
                    Out.Add(P);
                }
            }
        return Out;
    }

    struct FFogInput
    {
        TArray<uint8> Seen;
        FIntPoint SeenOrigin = FIntPoint::ZeroValue, SeenSize = FIntPoint::ZeroValue, BaseCell = FIntPoint::ZeroValue;
        int32 Step = 4;
    };

    /** Um pedaço do mundo, 1 ponto por bloco, no estilo do mapa: chão em patamares com contorno, luz em 3 faixas, água em
     *  dois tons com espuma e tinta na costa, copas das árvores, telhados e pedras; com névoa (nuvens) se Fog vier. */
    TArray<FColor> PaintCells(const TSharedPtr<FVortexTerrainData>& Data, const FIntPoint& Origin, int32 Size, const TArray<FPaintProp>& Props, const TSharedPtr<FFogInput>& Fog)
    {
        const int32 N = Size + 2;
        TArray<float> H;
        TArray<FColor> G;
        H.SetNumUninitialized(N * N);
        G.SetNumUninitialized(N * N);
        ParallelFor(N, [&](int32 Y)
        {
            for (int32 X = 0; X < N; ++X) Data->SampleCell(Origin.X - 1 + X, Origin.Y - 1 + Y, H[Y * N + X], G[Y * N + X]);
        }, EParallelForFlags::BackgroundPriority);
        const float WaterLevel = Data->WaterLevel, Surface = Data->WaterSurface;
        auto At = [&](int32 X, int32 Y) { return H[FMath::Clamp(Y + 1, 0, N - 1) * N + FMath::Clamp(X + 1, 0, N - 1)]; };
        auto Wet = [&](int32 X, int32 Y) { return At(X, Y) < WaterLevel; };
        TArray<FLinearColor> Px;
        Px.SetNumUninitialized(Size * Size);
        const FLinearColor Deep = Srgb(48, 116, 182), Shallow = Srgb(96, 196, 214), Foam = Srgb(236, 248, 248), Ocean = Srgb(40, 92, 150), Coast = Srgb(38, 30, 24);
        ParallelFor(Size, [&](int32 Y)
        {
            for (int32 X = 0; X < Size; ++X)
            {
                const float Height = At(X, Y);
                FLinearColor C;
                if (Height <= -299.f) C = Ocean;
                else if (Height < WaterLevel)
                {
                    const bool bShore = !Wet(X - 1, Y) || !Wet(X + 1, Y) || !Wet(X, Y - 1) || !Wet(X, Y + 1);
                    C = bShore ? Foam : (Surface - Height < 110.f) ? Shallow : Deep;
                }
                else
                {
                    C = FLinearColor::FromSRGBColor(G[(Y + 1) * N + X + 1]);
                    const float Slope = At(X + 1, Y + 1) - At(X - 1, Y - 1);
                    C *= Slope < -30.f ? .8f : Slope > 30.f ? 1.12f : 1.f;
                    // degrau de bloco (meio metro ou mais): um fio escuro
                    if (FMath::Abs(Height - At(X + 1, Y)) > 40.f || FMath::Abs(Height - At(X, Y + 1)) > 40.f) C *= .8f;
                    if (Wet(X - 1, Y) || Wet(X + 1, Y) || Wet(X, Y - 1) || Wet(X, Y + 1)) C = FMath::Lerp(C, Coast, .6f);
                }
                Px[Y * Size + X] = C;
            }
        }, EParallelForFlags::BackgroundPriority);
        // 02/11: a Cidade do Santuário (o mapa não mostrava a vila): o chão de pedra e as construções, com o contorno
        {
            TArray<uint8> Town;
            Town.SetNumZeroed(Size * Size);
            const float CS = Data->CellSize;
            bool bAny = false;
            for (int32 Y = 0; Y < Size; ++Y)
                for (int32 X = 0; X < Size; ++X)
                {
                    const uint8 K = uint8(AVortexTown::MapKindAt((Origin.X + X + .5f) * CS, (Origin.Y + Y + .5f) * CS));
                    Town[Y * Size + X] = K;
                    bAny |= K != 0;
                }
            if (bAny)
            {
                const FLinearColor Stone = Srgb(206, 192, 162), Roof = Srgb(150, 74, 52), Edge = Srgb(58, 40, 32);
                auto KindAt = [&](int32 X, int32 Y) { return X < 0 || Y < 0 || X >= Size || Y >= Size ? 0 : Town[Y * Size + X]; };
                for (int32 Y = 0; Y < Size; ++Y)
                    for (int32 X = 0; X < Size; ++X)
                    {
                        const uint8 K = Town[Y * Size + X];
                        if (K == 1) Px[Y * Size + X] = FMath::Lerp(Px[Y * Size + X], Stone, .85f);
                        else if (K == 2)
                        {
                            const bool bEdge = KindAt(X - 1, Y) != 2 || KindAt(X + 1, Y) != 2 || KindAt(X, Y - 1) != 2 || KindAt(X, Y + 1) != 2;
                            Px[Y * Size + X] = bEdge ? Edge : Roof * ((X + Y) % 4 == 0 ? .86f : 1.f);
                        }
                    }
            }
        }
        // pedras, telhados e copas (as copas por último: ficam por cima)
        auto Disc = [&](float CX, float CY, float R, const FLinearColor& Color, float Mix = 1.f)
        {
            const int32 X0 = FMath::FloorToInt(CX - R), X1 = FMath::CeilToInt(CX + R), Y0 = FMath::FloorToInt(CY - R), Y1 = FMath::CeilToInt(CY + R);
            for (int32 Y = FMath::Max(0, Y0); Y <= FMath::Min(Size - 1, Y1); ++Y)
                for (int32 X = FMath::Max(0, X0); X <= FMath::Min(Size - 1, X1); ++X)
                    if (FMath::Square(X + .5f - CX) + FMath::Square(Y + .5f - CY) <= R * R) Px[Y * Size + X] = FMath::Lerp(Px[Y * Size + X], Color, Mix);
        };
        for (const FPaintProp& P : Props)
        {
            if (P.Kind == EPaintProp::Rock) Disc(P.At.X, P.At.Y, FMath::Clamp(P.Scale * .9f, .8f, 2.2f), FLinearColor::FromSRGBColor(P.Color));
            else if (P.Kind == EPaintProp::House)
            {
                const int32 R = FMath::Clamp(FMath::RoundToInt(P.Scale * 4.f), 3, 7);
                const FLinearColor Roof = FLinearColor::FromSRGBColor(P.Color);
                for (int32 Y = FMath::RoundToInt(P.At.Y) - R; Y <= FMath::RoundToInt(P.At.Y) + R; ++Y)
                    for (int32 X = FMath::RoundToInt(P.At.X) - R; X <= FMath::RoundToInt(P.At.X) + R; ++X)
                    {
                        if (X < 0 || Y < 0 || X >= Size || Y >= Size) continue;
                        const bool bEdge = FMath::Abs(X - FMath::RoundToInt(P.At.X)) == R || FMath::Abs(Y - FMath::RoundToInt(P.At.Y)) == R;
                        const bool bRidge = Y == FMath::RoundToInt(P.At.Y);
                        Px[Y * Size + X] = bEdge ? Coast : bRidge ? Roof * .78f : Roof;
                    }
            }
        }
        for (const FPaintProp& P : Props)
        {
            if (P.Kind != EPaintProp::Tree) continue;
            // copa: um anel um pouco mais escuro, a cor da espécie e um brilho em cima (pouco contorno: mata fechada
            // com contorno forte virava uma mancha escura no minimapa)
            const float R = FMath::Clamp(1.3f + P.Scale * .8f, 1.5f, 3.f);
            const FLinearColor Leaf = FLinearColor::FromSRGBColor(P.Color);
            Disc(P.At.X, P.At.Y, R + .7f, Leaf * .6f);
            Disc(P.At.X, P.At.Y, R, Leaf * 1.15f);
            Disc(P.At.X - R * .3f, P.At.Y - R * .35f, R * .5f, FMath::Lerp(Leaf, FLinearColor::White, .3f));
        }
        TArray<FColor> Out;
        Out.SetNumUninitialized(Size * Size);
        ParallelFor(Size, [&](int32 Y)
        {
            for (int32 X = 0; X < Size; ++X)
            {
                FColor C = Px[Y * Size + X].ToFColor(true);
                if (Fog.IsValid())
                {
                    // a névoa do mapa grande, no mesmo desenho de nuvens da textura do continente
                    const int32 CellX = Origin.X + X, CellY = Origin.Y + Y;
                    const float Per = float(Data->CellsPerChunk);
                    const float Fx = (CellX + .5f) / Per - .5f - Fog->SeenOrigin.X, Fy = (CellY + .5f) / Per - .5f - Fog->SeenOrigin.Y;
                    const int32 Cx = FMath::FloorToInt(Fx), Cy = FMath::FloorToInt(Fy);
                    auto SeenAt = [&](int32 A, int32 B) { return Fog->Seen[FMath::Clamp(B, 0, Fog->SeenSize.Y - 1) * Fog->SeenSize.X + FMath::Clamp(A, 0, Fog->SeenSize.X - 1)] / 255.f; };
                    const float F = FMath::Lerp(FMath::Lerp(SeenAt(Cx, Cy), SeenAt(Cx + 1, Cy), Fx - Cx), FMath::Lerp(SeenAt(Cx, Cy + 1), SeenAt(Cx + 1, Cy + 1), Fx - Cx), Fy - Cy);
                    C = VortexMapPaint::CloudOver(C, F, float(CellX - Fog->BaseCell.X) / Fog->Step, float(CellY - Fog->BaseCell.Y) / Fog->Step, Per / Fog->Step);
                }
                Out[Y * Size + X] = C;
            }
        }, EParallelForFlags::BackgroundPriority);
        return Out;
    }
}

// ---- peças cartoon ---------------------------------------------------------------------------------------------------

UTexture2D* AVortexAimHUD::ToonTex(FName Name)
{
    if (TObjectPtr<UTexture2D>* Found = ToonTextures.Find(Name)) return *Found;
    UTexture2D* Loaded = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/UI/HudCartoon/T_%s.T_%s"), *Name.ToString(), *Name.ToString()));
    ToonTextures.Add(Name, Loaded);
    return Loaded;
}

void AVortexAimHUD::Slice(FName Name, float X, float Y, float W, float H, float Corner, const FLinearColor& Tint)
{
    UTexture2D* Texture = ToonTex(Name);
    if (!Texture || W <= 0.f || H <= 0.f || Tint.A <= .01f) return;
    // o canto da textura é metade do lado menor (cobre a curva toda); o da tela, o pedido sem passar da metade
    const float TW = Texture->GetSizeX(), TH = Texture->GetSizeY();
    const float CU = FMath::Min(TW, TH) * .5f / TW, CV = FMath::Min(TW, TH) * .5f / TH;
    const float C = FMath::Min(Corner, FMath::Min(W, H) * .5f);
    // pílula (o canto é a metade da altura): 3 pedaços na horizontal bastam (barras, teclas, chips)
    if (C >= H * .5f - .5f)
    {
        const float Cap = H * .5f;
        const float PXs[4] = { X, X + Cap, X + W - Cap, X + W };
        const float PUs[4] = { 0.f, CU, 1.f - CU, 1.f };
        for (int32 i = 0; i < 3; ++i)
        {
            if (PXs[i + 1] - PXs[i] <= 0.f) continue;
            FCanvasTileItem Tile(FVector2D(PXs[i], Y), Texture->GetResource(), FVector2D(PXs[i + 1] - PXs[i], H), FVector2D(PUs[i], 0.f), FVector2D(PUs[i + 1], 1.f), Tint);
            Tile.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(Tile);
        }
        return;
    }
    const float Xs[4] = { X, X + C, X + W - C, X + W }, Ys[4] = { Y, Y + C, Y + H - C, Y + H };
    const float Us[4] = { 0.f, CU, 1.f - CU, 1.f }, Vs[4] = { 0.f, CV, 1.f - CV, 1.f };
    for (int32 j = 0; j < 3; ++j)
        for (int32 i = 0; i < 3; ++i)
        {
            if (Xs[i + 1] - Xs[i] <= 0.f || Ys[j + 1] - Ys[j] <= 0.f) continue;
            FCanvasTileItem Tile(FVector2D(Xs[i], Ys[j]), Texture->GetResource(), FVector2D(Xs[i + 1] - Xs[i], Ys[j + 1] - Ys[j]),
                FVector2D(Us[i], Vs[j]), FVector2D(Us[i + 1], Vs[j + 1]), Tint);
            Tile.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(Tile);
        }
}

void AVortexAimHUD::Sprite(FName Name, float CX, float CY, float Size, float Degrees, const FLinearColor& Tint)
{
    UTexture2D* Texture = ToonTex(Name);
    if (!Texture || Tint.A <= .01f) return;
    FCanvasTileItem Tile(FVector2D(CX - Size * .5f, CY - Size * .5f), Texture->GetResource(), FVector2D(Size, Size), Tint);
    Tile.BlendMode = SE_BLEND_Translucent;
    if (Degrees != 0.f)
    {
        Tile.Rotation = FRotator(0.f, Degrees, 0.f);
        Tile.PivotPoint = FVector2D(.5f, .5f);
    }
    Canvas->DrawItem(Tile);
}

void AVortexAimHUD::ToonBar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Color)
{
    Slice("H_BarBack", X, Y, W, H, H * .5f);
    const float Inset = FMath::Max(2.f, H * .12f);
    const float Inner = H - 2.f * Inset;
    const float Fill = FMath::Clamp(Fraction, 0.f, 1.f);
    if (Fill <= 0.f) return;
    // a ponta é sempre redonda: abaixo da largura de uma altura, a barra só fica mais curta e mais fina por dentro
    Slice("H_BarFill", X + Inset, Y + Inset, FMath::Max(Inner, (W - 2.f * Inset) * Fill), Inner, Inner * .5f, Color);
}

FVector2D AVortexAimHUD::ToonSize(const FString& Text, float Size) const
{
    FSlateFontInfo Font = ToonFont(Size);
    Font.OutlineSettings.OutlineSize = FMath::Max(2, FMath::RoundToInt(Size / 8.f));
    return Measure(Text, Font);
}

void AVortexAimHUD::Toon(const FString& Text, float X, float Y, float W, float H, float Size, const FLinearColor& Color, int32 Align)
{
    if (Text.IsEmpty() || Color.A <= .01f) return;
    FSlateFontInfo Font = ToonFont(Size);
    Font.OutlineSettings.OutlineSize = FMath::Max(2, FMath::RoundToInt(Size / 8.f));
    Font.OutlineSettings.OutlineColor = WithA(Ink, Color.A);
    FVector2D Extent = Measure(Text, Font);
    // não cabe: diminui a letra (nomes compridos de missões e criaturas)
    if (W > 0.f && Extent.X > W && Size > 10.f)
    {
        const float Fit = FMath::Max(10.f, Size * W / Extent.X);
        Font = ToonFont(Fit);
        Font.OutlineSettings.OutlineSize = FMath::Max(2, FMath::RoundToInt(Fit / 8.f));
        Font.OutlineSettings.OutlineColor = WithA(Ink, Color.A);
        Extent = Measure(Text, Font);
    }
    const float TX = Align == 1 ? X + (W - Extent.X) * .5f : Align == 2 ? X + W - Extent.X : X;
    // a Luckiest Guy desenha um pouco acima do meio da caixa: desce 6% da altura da letra
    const float TY = Y + (H - Extent.Y) * .5f + Size * .06f;
    FCanvasTextItem Item(FVector2D(FMath::RoundToFloat(TX), FMath::RoundToFloat(TY)), FText::FromString(Text), Font, Color);
    if (!TextFont) TextFont = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    Item.Font = TextFont;
    PendingToon.Add(Item);
}

void AVortexAimHUD::FlushToon()
{
    for (FCanvasTextItem& Item : PendingToon) Canvas->DrawItem(Item);
    PendingToon.Reset();
}

// ---- herói --------------------------------------------------------------------------------------------------------------

void AVortexAimHUD::DrawPlayer(AVortexCharacter* Hero, float DeltaSeconds)
{
    const UVortexAttributes* Vitals = Hero->GetVitals();
    if (!Vitals) return;
    const float S = Scale;
    const float Health = Vitals->GetHealth(), MaxHealth = FMath::Max(1.f, Vitals->GetMaxHealth());
    const float Target = FMath::Clamp(Health / MaxHealth, 0.f, 1.f);
    // a vida escorre: cai rápido e sobe devagar (como no Godot)
    HealthShown = FMath::FInterpConstantTo(HealthShown, Target, DeltaSeconds, HealthShown > Target ? 2.2f : .9f);
    if (LastHeroHealth >= 0.f && Health < LastHeroHealth - .5f)
    {
        const FVector Head = Hero->GetActorLocation() + FVector(0.f, 0.f, 130.f);
        DamageNumbers.Add({ Head, LastHeroHealth - Health, float(GetWorld()->GetTimeSeconds()), true });
    }
    LastHeroHealth = Health;

    // o cartão, o retrato redondo e o selo do nível
    Slice("H_PanelBlue", 24.f * S, 22.f * S, 536.f * S, 150.f * S, 30.f * S);
    if (HeroPortrait) DrawMaterial(HeroPortrait, 17.f * S, 17.f * S, 136.f * S, 136.f * S, 0, 0, 1, 1);
    Sprite("H_Ring", 85.f * S, 85.f * S, 150.f * S);
    const UVortexHeroSheet* Sheet = Hero->GetSheet();
    const int32 HeroLevel = Sheet ? Sheet->GetLevel() : 1;
    Sprite("H_Disc", 136.f * S, 146.f * S, 54.f * S, 0.f, Gold);
    Toon(FString::FromInt(HeroLevel), 109.f * S, 119.f * S, 54.f * S, 54.f * S, 28.f * S, Cream, 1);

    // nome do herói escolhido no menu; raça e visual vêm do que ele veste
    const bool bFemale = Hero->GetLook().Appearance == TEXT("female");
    FString Name = bFemale ? TEXT("Aventureira") : TEXT("Aventureiro");
    const UVortexGameInstance* Game = Cast<UVortexGameInstance>(GetGameInstance());
    if (const UVortexPlayerSave* Saved = Game ? Game->GetSelectedHero() : nullptr) Name = Saved->HeroName;
    else if (const APlayerState* State = Hero->GetPlayerState())
    {
        const FString Player = State->GetPlayerName();
        if (!Player.IsEmpty() && !Player.StartsWith(TEXT("DESKTOP")) && Player.Len() <= 20) Name = Player;
    }
    const FVortexRaceInfo* Race = FVortexLookCatalog::Get().Race(Hero->GetLook().Race);
    Toon(Name, 176.f * S, 28.f * S, 230.f * S, 44.f * S, 34.f * S, Cream);
    const float NameW = FMath::Min(ToonSize(Name, 34.f * S).X, 230.f * S);
    Toon(FString::Printf(TEXT("%s  •  %s"), Race ? *Race->Name : TEXT("Humano"), bFemale ? TEXT("Aventureira") : TEXT("Aventureiro")),
        176.f * S + NameW + 14.f * S, 34.f * S, 540.f * S - (176.f * S + NameW + 14.f * S), 30.f * S, 17.f * S, Sky);

    // vida (pulsa quando baixa), mana e XP
    const float Pulse = Target < .3f ? .85f + .15f * FMath::Sin(GetWorld()->GetTimeSeconds() * 8.f) : 1.f;
    ToonBar(172.f * S, 80.f * S, 368.f * S, 36.f * S, HealthShown, FLinearColor(HpRed.R, HpRed.G * Pulse, HpRed.B * Pulse));
    Toon(FString::Printf(TEXT("%s / %s"), *Thousands(FMath::CeilToInt(Health)), *Thousands(FMath::RoundToInt(MaxHealth))), 172.f * S, 80.f * S, 368.f * S, 36.f * S, 22.f * S, Cream, 1);
    ToonBar(172.f * S, 118.f * S, 298.f * S, 28.f * S, Vitals->GetMana() / FMath::Max(1.f, Vitals->GetMaxMana()), MpBlue);
    Toon(FString::Printf(TEXT("%s / %s"), *Thousands(FMath::FloorToInt(Vitals->GetMana())), *Thousands(FMath::RoundToInt(Vitals->GetMaxMana()))), 172.f * S, 118.f * S, 298.f * S, 28.f * S, 18.f * S, Cream, 1);
    const int64 XpLow = FVortexRpgCatalog::XpForLevel(HeroLevel), XpHigh = FVortexRpgCatalog::XpForLevel(HeroLevel + 1);
    const int64 XpInto = Sheet ? Sheet->GetXp() - XpLow : 0;
    ToonBar(172.f * S, 148.f * S, 368.f * S, 18.f * S, XpHigh > XpLow ? float(double(XpInto) / double(XpHigh - XpLow)) : 0.f, Gold);
    Toon(FString::Printf(TEXT("XP %s / %s"), *Thousands(int32(FMath::Min<int64>(XpInto, MAX_int32))), *Thousands(int32(FMath::Min<int64>(XpHigh - XpLow, MAX_int32)))),
        472.f * S, 116.f * S, 70.f * S, 32.f * S, 12.f * S, Gold, 1);
}

// ---- efeitos ativos: pílulas embaixo do cartão --------------------------------------------------------------------------

void AVortexAimHUD::DrawBuffs(AVortexCharacter* Hero)
{
    struct FChip { FString Text; FLinearColor Color; float Left = -1.f; };
    TArray<FChip> Chips;
    const UVortexCombatComponent* Combat = Hero->GetCombat();
    if (Combat && Combat->CounterReadyFor() > 0.f) Chips.Add({ TEXT("Contra-ataque"), Srgb(255, 190, 80), Combat->CounterReadyFor() });
    if (const UVortexHeroSheet* Blessed = Hero->GetSheet(); Blessed && !Blessed->GetBlessing().IsNone())
    {
        const FName Kind = Blessed->GetBlessing();
        const uint8 Variant = Kind == TEXT("forca") ? 0 : Kind == TEXT("vida") ? 1 : 2;
        Chips.Add({ AVortexPointsOfInterest::BlessingName(Variant), Variant == 0 ? Srgb(255, 120, 90) : Variant == 1 ? Srgb(120, 230, 130) : Gold, Blessed->BlessingRemaining() });
    }
    // regras do portal (vermelho: desafio; verde: bônus)
    if (const UVortexHeroSheet* Sheet = Hero->GetSheet(); Sheet && !Sheet->GetDungeonId().IsNone())
        for (int32 Rule = 0; Rule < VortexDungeonRules::Count; ++Rule)
            if (Sheet->HasDungeonRule(Rule))
                Chips.Add({ VortexDungeonRules::Name(Rule), VortexDungeonRules::IsBonus(Rule) ? Green : HpRed });
    if (Hero->GetSheet() && Hero->GetSheet()->BurnRemaining() > 0.f) Chips.Add({ TEXT("Queimando"), Srgb(255, 120, 40), Hero->GetSheet()->BurnRemaining() });
    if (Hero->GetSheet() && Hero->GetSheet()->PoisonRemaining() > 0.f) Chips.Add({ TEXT("Envenenado"), Srgb(175, 90, 235), Hero->GetSheet()->PoisonRemaining() });
    if (const UVortexClassSkills* Skills = Hero->GetClassSkills())
        for (const UVortexClassSkills::FActive& Buff : Skills->ActiveBuffs()) Chips.Add({ Buff.Label, Buff.Color, Buff.Left });
    if (const AVortexFountain* Fountain = AVortexFountain::Find(GetWorld()); Fountain && Fountain->IsNear(Hero->GetActorLocation()))
        Chips.Add({ TEXT("Água da Fonte: cura"), Srgb(110, 240, 210) });
    else if (AVortexTown::Contains(Hero->GetActorLocation())) Chips.Add({ TEXT("Santuário: protegido"), Sky });
    float X = 30.f * Scale;
    const float Y = 186.f * Scale, H = 36.f * Scale;
    for (const FChip& Chip : Chips)
    {
        const FString Text = Chip.Left > 0.f ? FString::Printf(TEXT("%s %.0f s"), *Chip.Text, FMath::CeilToFloat(Chip.Left)) : Chip.Text;
        const float W = ToonSize(Text, 15.f * Scale).X + 56.f * Scale;
        if (X + W > Canvas->ClipX * .45f) break;
        Slice("H_PanelNavy", X, Y, W, H, 18.f * Scale);
        Sprite("H_Disc", X + 20.f * Scale, Y + H * .5f, 24.f * Scale, 0.f, Chip.Color);
        Toon(Text, X + 38.f * Scale, Y, W - 44.f * Scale, H, 15.f * Scale, Cream);
        X += W + 10.f * Scale;
    }
}

// ---- bússola em fita -----------------------------------------------------------------------------------------------------

void AVortexAimHUD::DrawCompass(AVortexCharacter* Hero)
{
    const float S = Scale;
    const FRotator View = PlayerOwner && PlayerOwner->PlayerCameraManager ? PlayerOwner->PlayerCameraManager->GetCameraRotation() : Hero->GetActorRotation();
    // 10/11 (o usuário: "melhorar muito a bússola"): mais larga, riscos a cada 5°, o rumo em graus no centro, a
    // distância embaixo de cada marca e os lugares descobertos por perto
    const float W = 760.f * S, H = 52.f * S, X0 = (Canvas->ClipX - W) * .5f, Y0 = 18.f * S;
    const float CX = X0 + W * .5f, Span = 105.f;
    Slice("H_PanelNavy", X0, Y0, W, H, 26.f * S);
    auto ScreenX = [&](float WorldYaw, float& OutX)
    {
        const float Rel = FRotator::NormalizeAxis(WorldYaw - View.Yaw);
        if (FMath::Abs(Rel) > Span) return false;
        OutX = CX + Rel / Span * (W * .5f - 26.f * S);
        return true;
    };
    // o norte é -Y (o alto do mapa); leste +X
    static const TCHAR* Names[] = { TEXT("L"), TEXT("SE"), TEXT("S"), TEXT("SO"), TEXT("O"), TEXT("NO"), TEXT("N"), TEXT("NE") };
    for (int32 d = 0; d < 360; d += 5)
    {
        float X = 0.f;
        if (!ScreenX(float(d), X)) continue;
        const float Edge = FMath::Clamp((1.f - FMath::Abs(X - CX) / (W * .5f)) * 3.f, 0.f, 1.f);
        if (d % 45 == 0)
        {
            const bool bNorth = d == 270, bMain = d % 90 == 0;
            Toon(Names[(d / 45) % 8], X - 30.f * S, Y0 + 3.f * S, 60.f * S, H - 16.f * S, (bMain ? 26.f : 19.f) * S, WithA(bNorth ? HpRed : Cream, Edge), 1);
            DrawLine(X, Y0 + H - 9.f * S, X, Y0 + H - 3.f * S, WithA(bNorth ? HpRed : Cream, Edge), 2.5f * S);
        }
        else if (d % 15 == 0) DrawLine(X, Y0 + H - 14.f * S, X, Y0 + H - 4.f * S, WithA(Faded, Edge), 2.f * S);
        else DrawLine(X, Y0 + H - 9.f * S, X, Y0 + H - 4.f * S, WithA(Faded, Edge * .6f), 1.5f * S);
    }
    // marcas em cima da fita, cada uma com a distância embaixo (as que ficariam uma em cima da outra mostram só a mais perto)
    struct FCompassMark { float X; float Meters; FName Icon; FLinearColor Tint; float Size; int32 Priority; };
    TArray<FCompassMark> Marks;
    auto Mark = [&](const FVector& At, FName Icon, const FLinearColor& Tint, float Size, int32 Priority = 1)
    {
        const FVector To = At - Hero->GetActorLocation();
        float X = 0.f;
        if (To.Size2D() < 100.f || !ScreenX(To.Rotation().Yaw, X)) return;
        Marks.Add({ X, float(To.Size2D() / 100.0), Icon, Tint, Size, Priority });
    };
    // os lugares descobertos a até 600 m (ruínas, torres, santuários...) entram fracos
    for (const FMapPoi& Poi : MapPois)
        if (FVector2D::Distance(Poi.At, FVector2D(Hero->GetActorLocation())) < 60000.f && IsDiscovered(FVector(Poi.At, 0.f)))
            Mark(FVector(Poi.At, 0.f), Poi.Name.StartsWith(TEXT("Portal")) ? FName("M_Portal") : FName("M_Poi"), WithA(FLinearColor::White, .75f), 18.f, 0);
    Mark(FVector(0.f, 1500.f, 0.f), "M_Town", FLinearColor::White, 22.f);
    Mark(FVector(17000.f, 11000.f, 0.f), "M_Poi", Srgb(255, 150, 90), 20.f);
    if (bHasMarker) Mark(FVector(Marker, 0.f), "M_Flag", FLinearColor::White, 26.f);
    if (const UVortexQuestLog* Log = Hero->GetQuestLog())
        if (const FVortexQuestDef* Quest = Log->Tracked())
            for (int32 i = 0; i < Quest->Objectives.Num(); ++i)
                if (Quest->Objectives[i].Type == TEXT("visit") && Log->ObjectiveValue(*Quest, i) < 1)
                    Mark(FVector(Quest->Objectives[i].Point, 0.f), "M_Quest", FLinearColor::White, 26.f);
    for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
        if (*It != Hero) Mark(It->GetActorLocation(), "M_Party", FLinearColor::White, 18.f);
    const float Now = GetWorld()->GetTimeSeconds();
    for (const FVortexPing& Ping : VortexPings::All())
    {
        if (Now - Ping.Time > VortexPings::Life || Now < Ping.Time) continue;
        const FVector At = Ping.Target.IsValid() ? Ping.Target->GetActorLocation() : Ping.At;
        Mark(At, Ping.bDanger ? FName("M_Enemy") : FName("M_Npc"), Ping.bDanger ? FLinearColor::White : Srgb(110, 230, 255), 20.f);
    }
    // a mais importante (e a mais perto) primeiro; a que encosta numa já posta não repete a distância
    Marks.Sort([](const FCompassMark& A, const FCompassMark& B) { return A.Priority != B.Priority ? A.Priority > B.Priority : A.Meters < B.Meters; });
    TArray<float> Labeled = { CX };   // o centro é da plaquinha do rumo
    FString CenterDist;
    for (const FCompassMark& M : Marks)
    {
        const float Edge = FMath::Clamp((1.f - FMath::Abs(M.X - CX) / (W * .5f)) * 3.f, 0.f, 1.f);
        Sprite(M.Icon, M.X, Y0 + 2.f * S, M.Size * S, 0.f, WithA(M.Tint, M.Tint.A * Edge));
        const FString Dist = M.Meters >= 1000.f ? FString::Printf(TEXT("%.1f km"), M.Meters / 1000.f).Replace(TEXT("."), TEXT(",")) : FString::Printf(TEXT("%.0f m"), M.Meters);
        // a marca bem na mira leva a distância para dentro da plaquinha
        if (CenterDist.IsEmpty() && M.Priority > 0 && FMath::Abs(M.X - CX) < 80.f * S) { CenterDist = Dist; continue; }
        bool bFree = true;
        for (const float L : Labeled) if (FMath::Abs(L - M.X) < 80.f * S) bFree = false;
        if (!bFree) continue;
        Labeled.Add(M.X);
        Toon(Dist, M.X - 50.f * S, Y0 + H + 2.f * S, 100.f * S, 22.f * S, (M.Priority > 0 ? 16.f : 14.f) * S, WithA(M.Priority > 0 ? Cream : Faded, Edge), 1);
    }
    // o rumo no centro: a seta dourada e os graus
    const float Heading = FMath::Fmod(FRotator::NormalizeAxis(View.Yaw - 270.f) + 360.f, 360.f);   // 0 = norte, sentido horário
    DrawLine(CX, Y0 + 4.f * S, CX, Y0 + H - 4.f * S, WithA(Gold, .35f), 2.f * S);
    static const TCHAR* Short[] = { TEXT("N"), TEXT("NE"), TEXT("L"), TEXT("SE"), TEXT("S"), TEXT("SO"), TEXT("O"), TEXT("NO") };
    FString Plate = FString::Printf(TEXT("%s %d°"), Short[FMath::RoundToInt(Heading / 45.f) % 8], FMath::RoundToInt(Heading) % 360);
    if (!CenterDist.IsEmpty()) Plate += TEXT("  ·  ") + CenterDist;
    const float PW = (CenterDist.IsEmpty() ? 96.f : 170.f) * S;
    Slice("H_PanelNavy", CX - PW * .5f, Y0 + H - 2.f * S, PW, 28.f * S, 10.f * S);
    Toon(Plate, CX - PW * .5f, Y0 + H - 1.f * S, PW, 26.f * S, 17.f * S, Gold, 1);
}

// ---- alvo / chefe ---------------------------------------------------------------------------------------------------

void AVortexAimHUD::DrawTarget(AVortexCharacter* Hero, float DeltaSeconds)
{
    const float Now = GetWorld()->GetTimeSeconds();
    AActor* Current = TargetCreature.Get();
    const IVortexCreature* Creature = Cast<IVortexCreature>(Current);
    // fica no alvo enquanto ele luta (ou levou golpe há pouco); senão, a criatura alerta mais perto
    const bool bKeep = Creature && Creature->IsEngaged() && FVector::Dist(Current->GetActorLocation(), Hero->GetActorLocation()) < (Creature->IsBoss() ? 8000.f : 4000.f);
    if (Creature && (bKeep || Now - TargetSeenTime < 2.5f)) { if (bKeep && !Creature->IsCreatureDead()) TargetSeenTime = Now; }
    else if (AActor* Found = FindCombatCreature(Hero))
    {
        if (Found != Current) TargetHealthShown = 1.f, WolfCaptureTime = 0.f, bTargetPortraitReady = false;
        TargetCreature = Found;
        Current = Found;
        Creature = Cast<IVortexCreature>(Found);
        TargetSeenTime = Now;
    }
    else { Current = nullptr; Creature = nullptr; }
    // a barra grande do alto é só dos chefes e semi-chefes; os outros têm a placa sobre a cabeça
    const bool bTopBar = Current && Creature && (Creature->IsBoss() || Creature->IsElite());
    TargetAlpha = FMath::FInterpConstantTo(TargetAlpha, bTopBar && Now - TargetSeenTime < 2.5f ? 1.f : 0.f, DeltaSeconds, 4.f);
    if (!bTopBar || TargetAlpha <= 0.f) return;

    const float S = Scale, A = TargetAlpha;
    const UAbilitySystemComponent* Abilities = Cast<IAbilitySystemInterface>(Current) ? Cast<IAbilitySystemInterface>(Current)->GetAbilitySystemComponent() : nullptr;
    const float MaxHealth = Abilities ? FMath::Max(1.f, Abilities->GetNumericAttribute(UVortexAttributes::GetMaxHealthAttribute())) : 1.f;
    const float Health = Creature->GetHealthFraction() * MaxHealth;
    TargetHealthShown = FMath::FInterpConstantTo(TargetHealthShown, Creature->GetHealthFraction(), DeltaSeconds, .8f);
    const bool bBoss = Creature->IsBoss();
    const float W = (bBoss ? 700.f : 540.f) * S, H = 58.f * S;
    const float X0 = (Canvas->ClipX - W) * .5f + 20.f * S, Y0 = 92.f * S;
    Slice("H_PanelRed", X0, Y0, W, H, 28.f * S, WithA(FLinearColor::White, A));
    if (TargetPortrait && A > .5f) DrawMaterial(TargetPortrait, X0 - 36.f * S, Y0 - 8.f * S, 76.f * S, 76.f * S, 0, 0, 1, 1);
    Sprite("H_Ring", X0 + 2.f * S, Y0 + 30.f * S, 84.f * S, 0.f, WithA(FLinearColor::White, A));
    const int32 Tier = VortexRpg::Difficulty(Hero->GetSheet() ? Hero->GetSheet()->GetLevel() : 1, Creature->GetCreatureLevel());
    Sprite("H_Disc", X0 + 36.f * S, Y0 + 60.f * S, 36.f * S, 0.f, WithA(bBoss ? Srgb(170, 70, 230) : VortexRpg::DifficultyColor(Tier), A));
    Toon(FString::FromInt(Creature->GetCreatureLevel()), X0 + 18.f * S, Y0 + 42.f * S, 36.f * S, 36.f * S, 19.f * S, WithA(Cream, A), 1);
    const AVortexWolf* Wolf = Cast<AVortexWolf>(Current);
    const FString Title = FText::FromString(Creature->GetCreatureTitle()).ToUpper().ToString();
    Toon(Wolf && Wolf->bFrenzied ? Title + TEXT("  (FRENESI)") : Title, X0 + 58.f * S, Y0 + 4.f * S, W - 76.f * S, 26.f * S, 22.f * S, WithA(bBoss ? Gold : Cream, A));
    // chefe: a barra em 3 fases (cor de cada fase; marcas nos terços; a fase atual pulsa)
    const int32 BossPhase = bBoss ? FMath::Clamp(Creature->GetBossPhase(), 0, 3) : 0;
    FLinearColor Fill = HpRed;
    if (BossPhase > 0)
    {
        static const FLinearColor PhaseColor[] = { Srgb(255, 200, 90), Srgb(255, 200, 90), Srgb(255, 110, 40), Srgb(230, 120, 255) };
        const float Pulse = BossPhase == 3 ? .85f + .15f * FMath::Sin(Now * 8.f) : 1.f;
        Fill = PhaseColor[BossPhase] * Pulse;
    }
    const float BX = X0 + 56.f * S, BY = Y0 + 32.f * S, BW = W - 70.f * S, BH = 22.f * S;
    ToonBar(BX, BY, BW, BH, TargetHealthShown, WithA(Fill, A));
    Toon(FString::Printf(TEXT("%s / %s"), *Thousands(FMath::CeilToInt(Health)), *Thousands(FMath::RoundToInt(MaxHealth))), BX, BY, BW, BH, 16.f * S, WithA(Cream, A), 1);
    if (bBoss)
    {
        // 02/11: o golem vulcânico (Azhûr) tem 2 fases, a troca em 40% da vida; os outros chefes, 3 (nos terços)
        const AVortexMonster* AsMonster = Cast<AVortexMonster>(Current);
        const bool bTwo = AsMonster && AsMonster->GetVolcanic();
        const int32 Phases = bTwo ? 2 : 3;
        for (int32 k = 1; k < Phases; ++k)
        {
            const float X = BX + BW * (bTwo ? .4f : k / 3.f);
            DrawLine(X, BY + 2.f * S, X, BY + BH - 2.f * S, WithA(Ink, .9f * A), 4.f * S);
        }
        // selos das fases (I, II, III) à direita do painel
        static const TCHAR* Roman[] = { TEXT("I"), TEXT("II"), TEXT("III") };
        for (int32 k = 0; k < Phases; ++k)
        {
            const bool bLit = BossPhase >= k + 1, bNow = BossPhase == k + 1;
            const float R = (bNow ? 30.f : 24.f) * S, CX = X0 + W + 30.f * S + k * 40.f * S, CY = Y0 + H * .5f;
            Sprite("H_Disc", CX, CY, R, 0.f, WithA(bLit ? Srgb(200, 60, 40) : Srgb(70, 70, 90), A));
            Toon(Roman[k], CX - R, CY - R, 2.f * R, 2.f * R, (bNow ? 16.f : 13.f) * S, WithA(bLit ? Cream : Faded, A), 1);
        }
        if (BossPhase > 0)
        {
            static const TCHAR* PhaseName[] = { TEXT(""), TEXT("FASE I · O COLOSSO"), TEXT("FASE II · A FÚRIA DA CALDEIRA"), TEXT("FASE III · O NÚCLEO EXPOSTO") };
            const TCHAR* Name = bTwo ? (BossPhase >= 2 ? TEXT("FASE II · O NÚCLEO DESPERTO") : TEXT("FASE I · O COLOSSO")) : PhaseName[BossPhase];
            Toon(Name, X0, Y0 + H + 4.f * S, W, 26.f * S, 18.f * S, WithA(Srgb(255, 180, 120), A), 1);
        }
    }
    // o que ela está fazendo: em vermelho vivo quando é o golpe que vem
    const FString Action = Creature->GetCreatureAction();
    const bool bStrike = Wolf ? (Wolf->WolfState == EVortexWolfState::Bite || Wolf->WolfState == EVortexWolfState::Pounce || Wolf->bFrenzied)
        : Action.Contains(TEXT("·")) || Action == TEXT("ÚLTIMA RESISTÊNCIA") || Action == TEXT("A fúria desperta");
    if (!bBoss || BossPhase == 0)
    {
        const FString Doing = Wolf ? FString(Wolf->bFrenzied ? TEXT("Frenesi!") : Wolf->WolfState == EVortexWolfState::Pounce ? TEXT("Bote!") : Wolf->WolfState == EVortexWolfState::Bite ? TEXT("Mordida!") : TEXT("")) : Action;
        if (!Doing.IsEmpty()) Toon(Doing, X0, Y0 + H + 4.f * S, W, 24.f * S, (bStrike ? 19.f : 16.f) * S, WithA(bStrike ? Srgb(255, 110, 90) : Faded, A), 1);
    }
}

// ---- minimapa --------------------------------------------------------------------------------------------------------

namespace
{
    // 11/11 (o usuário: "ícones para elas no minimapa e no mapa"): as construções com nome da vila (Data/GodotTown/vila.json,
    // Tools/Town/BuildVila.py), cada uma com o ícone do símbolo da placa (Tools/Town/MakeShopIcons.py: T_M_Loja_<SLUG>)
    struct FTownPlace { FVector2D At; FString Name; FName Icon; };

    const TArray<FTownPlace>& TownPlaces()
    {
        static TArray<FTownPlace> Places;
        static bool bRead = false;
        if (bRead) return Places;
        bRead = true;
        FString Text;
        TSharedPtr<FJsonObject> Json;
        const TArray<TSharedPtr<FJsonValue>>* Pieces = nullptr;
        if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/GodotTown/vila.json"))) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid() || !Json->TryGetArrayField(TEXT("pieces"), Pieces))
            return Places;
        for (const TSharedPtr<FJsonValue>& Value : *Pieces)
        {
            const TSharedPtr<FJsonObject> P = Value->AsObject();
            FString Label;
            if (!P || !P->TryGetStringField(TEXT("label"), Label) || Label.IsEmpty()) continue;
            // o mesmo nome de arquivo da placa (AVortexTown e Tools/Town/MakeSigns.py: slug)
            FString Slug;
            for (const TCHAR Ch : Label.ToUpper())
            {
                const TCHAR* From = TEXT("ÁÀÂÃÉÊÍÓÔÕÚÇ");
                const TCHAR* To = TEXT("AAAAEEIOOOUC");
                const TCHAR* Hit = FCString::Strchr(From, Ch);
                const TCHAR Plain = Hit ? To[Hit - From] : Ch;
                if (FChar::IsAlnum(Plain) && Plain < 128) Slug.AppendChar(Plain);
                else if (!Slug.EndsWith(TEXT("_"))) Slug.AppendChar(TEXT('_'));
            }
            Slug.RemoveFromEnd(TEXT("_"));
            const FString Icon = TEXT("M_Loja_") + Slug;
            if (!LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/UI/HudCartoon/T_%s.T_%s"), *Icon, *Icon))) continue;   // portões e placas sem ícone
            // "FORJA DE BRAM" -> "Forja de Bram"
            TArray<FString> Words;
            Label.ToLower().ParseIntoArray(Words, TEXT(" "));
            for (int32 i = 0; i < Words.Num(); ++i)
                if (i == 0 || !(Words[i] == TEXT("de") || Words[i] == TEXT("do") || Words[i] == TEXT("da"))) Words[i][0] = FChar::ToUpper(Words[i][0]);
            Places.Add({ FVector2D(P->GetNumberField(TEXT("x")), P->GetNumberField(TEXT("z"))) * 100.f, FString::Join(Words, TEXT(" ")), FName(*Icon) });
        }
        UE_LOG(LogTemp, Display, TEXT("VORTEX_HUD_LOJAS %d lugares com ícone no mapa"), Places.Num());
        return Places;
    }
}

void AVortexAimHUD::DrawMinimap(AVortexCharacter* Hero)
{
    const float S = Scale, D = 270.f * S, R = D * .5f;
    const float X0 = Canvas->ClipX - 320.f * S, Y0 = 36.f * S, CX = X0 + R, CY = Y0 + R;
    UVortexPlayerSettings* Settings = UVortexPlayerSettings::Get();
    // 10/11: os alcances agora são 60, 100 e 160 m; o de 40 m antigo (perto demais) vira 100
    if (Settings->MinimapRadius < 60.f) { Settings->MinimapRadius = 100.f; Settings->SaveConfig(); }
    const float RadiusCm = Settings->MinimapRadius * 100.f;
    const FVector Here = Hero->GetActorLocation();
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    const FRotator Cam = PlayerOwner && PlayerOwner->PlayerCameraManager ? PlayerOwner->PlayerCameraManager->GetCameraRotation() : Hero->GetActorRotation();
    // girando: o que está à frente da câmera fica para cima
    const float Angle = Settings->bMinimapRotates ? FMath::DegreesToRadians(Cam.Yaw + 90.f) : 0.f;
    const float Cs = FMath::Cos(Angle), Sn = FMath::Sin(Angle);
    if (!MinimapToon)
        if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/UI/HudCartoon/M_HudMinimapCartoon.M_HudMinimapCartoon")))
            MinimapToon = UMaterialInstanceDynamic::Create(Base, this);
    if (MinimapToon && Data)
    {
        if (LocalMap && bLocalReady)
        {
            const float Cells = float(LocalCells), CS = Data->CellSize;
            MinimapToon->SetTextureParameterValue(TEXT("Map"), LocalMap);
            MinimapToon->SetVectorParameterValue(TEXT("View"), FLinearColor((Here.X / CS - LocalOrigin.X) / Cells, (Here.Y / CS - LocalOrigin.Y) / Cells, RadiusCm / (Cells * CS), RadiusCm / (Cells * CS)));
        }
        else if (MapTexture)
        {
            const FVector2D Extent = MapBounds.GetSize();
            MinimapToon->SetTextureParameterValue(TEXT("Map"), MapTexture);
            MinimapToon->SetVectorParameterValue(TEXT("View"), FLinearColor((Here.X - MapBounds.Min.X) / Extent.X, (Here.Y - MapBounds.Min.Y) / Extent.Y, RadiusCm / Extent.X, RadiusCm / Extent.Y));
        }
        MinimapToon->SetScalarParameterValue(TEXT("Angle"), Angle);
        DrawMaterial(MinimapToon, CX - R, CY - R, D, D, 0, 0, 1, 1);
    }
    // do mundo para a tela: desfaz o giro do mapa
    auto ToScreen = [&](const FVector2D& World, FVector2D& Out, bool bClamp)
    {
        const FVector2D Q((World.X - Here.X) / RadiusCm, (World.Y - Here.Y) / RadiusCm);
        FVector2D P(Cs * Q.X + Sn * Q.Y, -Sn * Q.X + Cs * Q.Y);
        if (P.Size() > .9f)
        {
            if (!bClamp) return false;
            P = P.GetSafeNormal() * .9f;
        }
        Out = FVector2D(CX, CY) + P * R;
        return true;
    };
    FVector2D At;
    // lugares descobertos (pontos de interesse, portais) e a cidade
    for (const FMapPoi& Poi : MapPois)
    {
        if (FVector2D::DistSquared(Poi.At, FVector2D(Here)) > FMath::Square(RadiusCm * 1.3f) || !IsDiscovered(FVector(Poi.At, 0.f)) || !ToScreen(Poi.At, At, false)) continue;
        const bool bPortal = Poi.Name.StartsWith(TEXT("Portal"));
        Sprite(bPortal ? FName("M_Portal") : FName("M_Poi"), At.X, At.Y, (bPortal ? 24.f : 18.f) * S, 0.f, bPortal && Poi.Name.Contains(TEXT("VERMELHO")) ? Srgb(255, 120, 120) : FLinearColor::White);
    }
    if (ToScreen(FVector2D(0.f, 1500.f), At, false) && RadiusCm > 12000.f) Sprite("M_Town", At.X, At.Y, 26.f * S);
    for (const FTownPlace& Place : TownPlaces())
        if (ToScreen(Place.At, At, false)) Sprite(Place.Icon, At.X, At.Y, 24.f * S);
    // moradores (com ! ou ? quando têm missão)
    const UVortexQuestLog* Log = Hero->GetQuestLog();
    for (TActorIterator<AVortexNpc> It(GetWorld()); It; ++It)
    {
        if (!ToScreen(FVector2D(It->GetActorLocation()), At, false)) continue;
        const FString Mark = Log ? Log->MarkFor(It->GetNpcId()) : FString();
        if (Mark.IsEmpty()) Sprite("M_Npc", At.X, At.Y, 14.f * S);
        else Sprite("M_Quest", At.X, At.Y, 22.f * S, 0.f, Mark == TEXT("?") ? Srgb(150, 255, 150) : FLinearColor::White);
    }
    // criaturas: só as que o herói viu (chefes maiores; bichos calmos claros)
    for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
    {
        const IVortexCreature* Creature = Cast<IVortexCreature>(*It);
        if (!Creature || Creature->IsCreatureDead() || !WasSeen(*It) || !ToScreen(FVector2D(It->GetActorLocation()), At, false)) continue;
        if (Creature->IsBoss()) Sprite("M_Boss", At.X, At.Y, 26.f * S);
        else if (Creature->IsPassive()) Sprite("M_Npc", At.X, At.Y, 12.f * S, 0.f, Srgb(220, 190, 140));
        else Sprite("M_Enemy", At.X, At.Y, (*It == TargetCreature.Get() ? 20.f : 15.f) * S);
    }
    // 03/11: os outros heróis na cor de cada um (Shift+Z), e os pings que estão valendo
    for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
        if (*It != Hero && ToScreen(FVector2D(It->GetActorLocation()), At, true))
            Sprite("M_Party", At.X, At.Y, 18.f * S, 0.f, It->GetSocial() ? It->GetSocial()->GetPlayerColor() : FLinearColor::White);
    for (const FVortexPing& Ping : VortexPings::All())
        if (GetWorld()->GetTimeSeconds() - Ping.Time < VortexPings::Life && ToScreen(FVector2D(Ping.At), At, true))
            Sprite("M_Flag", At.X, At.Y - 5.f * S, 20.f * S, 0.f, Ping.bDanger ? FLinearColor(1.f, .25f, .2f) : Ping.Color);
    // objetivo da missão acompanhada e o marcador do jogador: ficam na borda quando estão longe
    if (Log)
        if (const FVortexQuestDef* Quest = Log->Tracked())
            for (int32 i = 0; i < Quest->Objectives.Num(); ++i)
                if (Quest->Objectives[i].Type == TEXT("visit") && Log->ObjectiveValue(*Quest, i) < 1 && ToScreen(Quest->Objectives[i].Point, At, true))
                {
                    Sprite("M_Quest", At.X, At.Y, 26.f * S);
                    break;
                }
    if (bHasMarker && ToScreen(Marker, At, true)) Sprite("M_Flag", At.X, At.Y - 6.f * S, 28.f * S);
    // o herói: seta para onde ele olha
    const float HeroDeg = Settings->bMinimapRotates ? FRotator::NormalizeAxis(Hero->GetActorRotation().Yaw - Cam.Yaw) : Cam.Yaw + 90.f;
    Sprite("M_Arrow", CX, CY, 30.f * S, HeroDeg);
    // o aro, o N na borda e o alcance
    Sprite("H_MapRing", CX, CY, D + 28.f * S);
    const FVector2D North(-Sn, -Cs);
    Sprite("H_Disc", CX + North.X * (R + 4.f * S), CY + North.Y * (R + 4.f * S), 36.f * S, 0.f, HpRed);
    Toon(TEXT("N"), CX + North.X * (R + 4.f * S) - 18.f * S, CY + North.Y * (R + 4.f * S) - 18.f * S, 36.f * S, 36.f * S, 20.f * S, Cream, 1);
    Slice("H_PanelNavy", X0 - 10.f * S, Y0 + D - 30.f * S, 70.f * S, 32.f * S, 16.f * S);
    Toon(FString::Printf(TEXT("%d m"), FMath::RoundToInt(Settings->MinimapRadius)), X0 - 10.f * S, Y0 + D - 30.f * S, 70.f * S, 32.f * S, 15.f * S, Cream, 1);
    // a região, o dia e a hora
    FString Place = TEXT("Vale do Santuário");
    if (AVortexTown::Contains(Here)) Place = TEXT("Cidade do Santuário");
    else if (Data && Data->HasWorld()) Place = Data->RegionName(Data->RegionAt(Here.X, Here.Y));
    Slice("H_PanelNavy", X0 - 30.f * S, Y0 + D + 20.f * S, D + 60.f * S, 46.f * S, 18.f * S);
    Toon(Place, X0 - 24.f * S, Y0 + D + 20.f * S, D + 48.f * S, 46.f * S, 20.f * S, Gold, 1);
    if (const AVortexGameState* State = GetWorld()->GetGameState<AVortexGameState>())
    {
        const int32 Minutes = FMath::FloorToInt(State->GetHour() * 60.f) % (24 * 60);
        Slice("H_PanelBlue", CX - 85.f * S, Y0 + D + 70.f * S, 170.f * S, 36.f * S, 18.f * S);
        Toon(FString::Printf(TEXT("DIA %d  •  %02d:%02d"), State->GetDayNumber(), Minutes / 60, Minutes % 60), CX - 85.f * S, Y0 + D + 70.f * S, 170.f * S, 36.f * S, 18.f * S, Cream, 1);
        // 11/11 (o usuário: "embaixo do minimapa o horário e a estação"): a estação e o tempo que está fazendo
        if (const UVortexWeatherSubsystem* Weather = GetWorld()->GetSubsystem<UVortexWeatherSubsystem>())
        {
            const FString Line = FString::Printf(TEXT("%s  •  %s"), UVortexWeatherSubsystem::SeasonName(Weather->GetSeason()),
                UVortexWeatherSubsystem::WeatherName(Weather->GetWeather())).ToUpper();
            Slice("H_PanelNavy", CX - 115.f * S, Y0 + D + 110.f * S, 230.f * S, 32.f * S, 16.f * S);
            Toon(Line, CX - 115.f * S, Y0 + D + 110.f * S, 230.f * S, 32.f * S, 15.f * S, Cream, 1);
        }
    }
}

// ---- missão acompanhada --------------------------------------------------------------------------------------------

void AVortexAimHUD::DrawQuestTracker(AVortexCharacter* Hero)
{
    const UVortexQuestLog* Log = Hero->GetQuestLog();
    const FVortexQuestDef* Quest = Log ? Log->Tracked() : nullptr;
    if (!Quest) return;
    const float S = Scale, W = 344.f * S, X = Canvas->ClipX - W - 20.f * S, Y0 = 466.f * S;   // 11/11: 430 -> 466 (a linha da estação e do tempo embaixo do minimapa)
    TArray<FString> Lines;
    Log->ObjectiveLines(*Quest).ParseIntoArrayLines(Lines);
    const float H = (70.f + Lines.Num() * 30.f) * S;
    Slice("H_PanelNavy", X, Y0, W, H, 22.f * S);
    Toon(Quest->bMain ? TEXT("JORNADA") : TEXT("MISSÃO"), X + 20.f * S, Y0 + 10.f * S, W - 40.f * S, 20.f * S, 15.f * S, Quest->bMain ? Sky : Srgb(160, 230, 160));
    Toon(FText::FromString(Quest->Title).ToUpper().ToString(), X + 20.f * S, Y0 + 30.f * S, W - 40.f * S, 30.f * S, 25.f * S, Gold);
    float Y = Y0 + 66.f * S;
    for (const FString& Line : Lines)
    {
        const bool bDone = Line.StartsWith(TEXT("✓"));
        const bool bReturn = Line.StartsWith(TEXT("↟"));
        // a caixinha: vazia, marcada (feito) ou com a seta de voltar
        const float BX = X + 22.f * S, BY = Y + 5.f * S, B = 20.f * S;
        Slice("H_BarFill", BX, BY, B, B, 6.f * S, bDone ? Green : bReturn ? Gold : Cream);
        if (bDone)
        {
            DrawLine(BX + 4.f * S, BY + 10.f * S, BX + 8.f * S, BY + 15.f * S, Ink, 3.f * S);
            DrawLine(BX + 8.f * S, BY + 15.f * S, BX + 16.f * S, BY + 5.f * S, Ink, 3.f * S);
        }
        // o símbolo do começo da linha (✓, ↟, ◇, •) vira a caixinha: a letra cartoon nem tem esses desenhos
        FString Text = Line.TrimStart();
        while (!Text.IsEmpty() && !FChar::IsAlnum(Text[0]) && Text[0] != TEXT('"') && Text[0] != TEXT('(')) Text = Text.RightChop(1).TrimStart();
        Toon(Text, X + 52.f * S, Y, W - 70.f * S, 30.f * S, 16.f * S, bReturn ? Srgb(170, 255, 150) : bDone ? Faded : Cream);
        Y += 30.f * S;
    }
}

// ---- avisos: saque, XP e os grandes (nível, saída do estádio) -----------------------------------------------------

void AVortexAimHUD::DrawNotices(AVortexCharacter* Hero)
{
    const float S = Scale;
    const float Now = GetWorld()->GetTimeSeconds();
    if (Now < Hero->GetSensitivityNoticeUntil())
    {
        const FString Text = FString::Printf(TEXT("Sensibilidade do mouse: %.1f   ([ diminui, ] aumenta)"), UVortexPlayerSettings::Get()->MouseSensitivity).Replace(TEXT("."), TEXT(","));
        Toon(Text, 0.f, Canvas->ClipY * .72f, Canvas->ClipX, 40.f * S, 24.f * S, Cream, 1);
    }
    // save automático: um aviso discreto no canto
    const float SinceSave = Now - AVortexGameMode::LastAutosaveTime;
    if (SinceSave >= 0.f && SinceSave < 2.5f)
        Toon(TEXT("Jogo salvo"), Canvas->ClipX - 260.f * S, Canvas->ClipY - 150.f * S, 240.f * S, 28.f * S, 18.f * S, WithA(Faded, SinceSave < 2.f ? 1.f : (2.5f - SinceSave) * 2.f), 2);
    // sala da Epic aberta: o código fica no alto, para passar aos amigos
    if (const UVortexOnline* Online = GetGameInstance() ? GetGameInstance()->GetSubsystem<UVortexOnline>() : nullptr)
        if (Online->HasRoom())
            Toon(FString::Printf(TEXT("SALA ONLINE  %s"), *Online->GetRoomCode()), 0.f, 74.f * S, Canvas->ClipX, 26.f * S, 20.f * S, Gold, 1);

    const UVortexHeroSheet* Sheet = Hero->GetSheet();
    if (!Sheet) return;
    float Y = 600.f * S;
    const TArray<UVortexHeroSheet::FNotice>& Notices = Sheet->GetNotices();
    for (int32 i = Notices.Num() - 1; i >= 0; --i)
    {
        const UVortexHeroSheet::FNotice& Notice = Notices[i];
        const float Age = Now - Notice.Time;
        if (Age > 4.f) continue;
        if (Notice.Kind == 5 && Age < 3.f)
        {
            const float A = Age < 2.2f ? 1.f : 1.f - (Age - 2.2f) / .8f;
            CartoonText(FText::FromString(Notice.Text).ToUpper().ToString(), Canvas->ClipX * .5f, Canvas->ClipY * .28f, 64.f * S, Gold, Ink, A);
            continue;
        }
        // o tipo 2 é o aviso em destaque: o baú, a inscrição, o santuário, o grupo... Só o "Nível N!" é o de subir de nível
        if (Notice.Kind == 2 && Age < 3.5f && !Notice.Text.StartsWith(TEXT("Nível ")))
        {
            const float A = Age < 2.7f ? 1.f : 1.f - (Age - 2.7f) / .8f;
            const float Size = 24.f * S;
            const float TW = FMath::Min(ToonSize(Notice.Text, Size).X, Canvas->ClipX - 120.f * S);
            const float W = TW + 70.f * S, H = 56.f * S, X = (Canvas->ClipX - W) * .5f, Top = Canvas->ClipY * .22f;
            Slice("H_PanelNavy", X, Top, W, H, 24.f * S, WithA(FLinearColor::White, A * .95f));
            Toon(Notice.Text, X + 35.f * S, Top, TW, H, Size, WithA(Gold, A), 1);
            continue;
        }
        if (Notice.Kind == 2 && Age < 3.f)
        {
            // "NÍVEL N" grande, dourado, que cresce e some
            const float Grow = 1.f + .25f * FMath::Max(0.f, 1.f - Age * 4.f);
            const float A = Age < 2.2f ? 1.f : 1.f - (Age - 2.2f) / .8f;
            CartoonText(FString::Printf(TEXT("NÍVEL %d"), Sheet->GetLevel()), Canvas->ClipX * .5f, Canvas->ClipY * .26f, 84.f * S * Grow, Gold, Ink, A);
            Toon(TEXT("+2 PONTOS DE ATRIBUTO  •  I PARA DISTRIBUIR"), 0.f, Canvas->ClipY * .26f + 56.f * S, Canvas->ClipX, 34.f * S, 24.f * S, WithA(Cream, A), 1);
            continue;
        }
        const FLinearColor Color = Notice.Kind == 1 ? Srgb(200, 170, 255) : Notice.Kind == 3 ? Gold : Notice.Kind == 4 ? Srgb(255, 140, 120) : Notice.Kind == 6 ? Srgb(255, 220, 130) : Cream;
        const float A = Age < 3.f ? 1.f : 1.f - (Age - 3.f);
        const float Slide = FMath::Max(0.f, .2f - Age) / .2f * 60.f * S;
        const float TW = FMath::Min(ToonSize(Notice.Text, 18.f * S).X, 520.f * S);
        const float W = TW + 62.f * S, H = 42.f * S, X = Canvas->ClipX - W - 20.f * S + Slide;
        Slice("H_PanelNavy", X, Y, W, H, 21.f * S, WithA(FLinearColor::White, A * .95f));
        const bool bGain = Notice.Text.StartsWith(TEXT("+"));
        Sprite("H_Disc", X + 24.f * S, Y + H * .5f, 28.f * S, 0.f, WithA(bGain ? Green : Notice.Kind == 4 ? HpRed : Srgb(110, 150, 255), A));
        Toon(bGain ? TEXT("+") : TEXT("!"), X + 10.f * S, Y, 28.f * S, H, 17.f * S, WithA(Cream, A), 1);
        Toon(Notice.Text, X + 46.f * S, Y, TW, H, 18.f * S, WithA(Color, A));
        Y += 50.f * S;
    }
}

// ---- barra de habilidades e poções ----------------------------------------------------------------------------------

void AVortexAimHUD::DrawHotbar(AVortexCharacter* Hero)
{
    const float S = Scale;
    const float SlotW = 92.f * S, SlotH = 88.f * S, Step = 106.f * S, PotW = 84.f * S;
    const float TotalW = 6.f * Step - 14.f * S + 26.f * S + 98.f * S + PotW;
    const float X0 = (Canvas->ClipX - TotalW) * .5f, SlotY = Canvas->ClipY - 112.f * S;
    Slice("H_PanelNavy", X0 - 18.f * S, SlotY - 10.f * S, 6.f * Step - 14.f * S + 36.f * S, SlotH + 20.f * S, 30.f * S, FLinearColor(1.f, 1.f, 1.f, .92f));
    const UVortexCharacterMovementComponent* Movement = Hero->GetVortexMovement();
    const UVortexWeaponComponent* Weapon = Hero->GetWeapon();
    auto Down = [this](const FKey& Key) { return PlayerOwner && PlayerOwner->IsInputKeyDown(Key); };
    auto Veil = [&](float X, float Y, float W, float H, float Fraction)
    {
        if (Fraction <= 0.f) return;
        // a recarga escurece o botão de cima para baixo (o que falta)
        FCanvasTileItem Dark(FVector2D(X + 6.f * S, Y + 6.f * S), FVector2D(W - 12.f * S, (H - 14.f * S) * FMath::Min(Fraction, 1.f)), Srgb(10, 14, 40, .66f));
        Dark.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Dark);
    };
    auto Icon = [&](UTexture2D* Texture, float X, float Y, const FLinearColor& Tint)
    {
        // 07/11 (o usuário: "um quadrado arredondado e dentro dele outro PNG quadrado"): o ícone ocupa o miolo inteiro
        // do encaixe, rente à moldura de ouro (uma moldura só, como nos jogos)
        if (Texture) DrawTexture(Texture, X + 7.f * S, Y + 6.f * S, SlotW - 14.f * S, SlotH - 12.f * S, 0.f, 0.f, 1.f, 1.f, Tint);
    };
    static const TCHAR* KeyNames[] = { TEXT(""), TEXT(""), TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4") };
    const FKey KeyCodes[] = { EKeys::LeftMouseButton, EKeys::RightMouseButton, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
    const float Remaining = Weapon ? Weapon->SpecialCooldownRemaining() : 0.f;
    // três passadas (os botões, depois os ícones, depois as teclas): as peças iguais seguidas saem num lote só
    for (int32 Pass = 0; Pass < 3; ++Pass)
    for (int32 i = 0; i < 6; ++i)
    {
        const float X = X0 + i * Step, Y = SlotY;
        bool bActive = Down(KeyCodes[i]);
        bool bEmpty = false, bLocked = false;
        const FVortexClassSkill* Skill = i >= 2 && Hero->GetClassSkills() ? Hero->GetClassSkills()->Skill(i - 2) : nullptr;
        if (i == 0) bActive |= Weapon && Weapon->GetAttackState().SwingIndex != INDEX_NONE && !Weapon->IsSpecialActive();
        if (i == 1) bActive = Weapon && Weapon->IsSpecialActive();
        if (i >= 2) { bEmpty = !Skill; bLocked = Skill && !Hero->GetClassSkills()->IsUnlocked(i - 2); }
        if (Pass == 0) { Slice(bEmpty || bLocked ? FName("H_SlotLocked") : FName("H_Slot"), X, Y, SlotW, SlotH, 14.f * S, bActive ? Srgb(255, 230, 140) : FLinearColor::White); continue; }
        if (Pass == 2)
        {
            // a tecla no canto; o golpe e o especial são os botões do mouse: um mouse com o lado dele aceso (as letras
            // "E" e "D" de esquerdo/direito pareciam as teclas E e D)
            const float KX = X + SlotW - 34.f * S, KY = Y + SlotH - 20.f * S;
            Slice("H_Key", KX, KY, 34.f * S, 30.f * S, 10.f * S);
            if (i < 2)
            {
                auto Tile = [&](float TX, float TY, float TW, float TH, const FLinearColor& C)
                {
                    FCanvasTileItem T(FVector2D(TX, TY), FVector2D(TW, TH), C);
                    T.BlendMode = SE_BLEND_Translucent;
                    Canvas->DrawItem(T);
                };
                const float MX = KX + 11.f * S, MY = KY + 5.f * S, MW = 12.f * S, MH = 19.f * S;
                Tile(MX, MY, MW, MH, Cream);
                Tile(i == 0 ? MX : MX + MW * .5f, MY, MW * .5f, MH * .42f, Srgb(255, 200, 60));
                Tile(MX + MW * .5f - .75f * S, MY, 1.5f * S, MH * .42f, Srgb(20, 20, 50));
                Tile(MX, MY + MH * .42f, MW, 1.5f * S, Srgb(20, 20, 50));
            }
            else Toon(KeyNames[i], KX, KY, 34.f * S, 30.f * S, 15.f * S, Cream, 1);
            continue;
        }
        if (i == 0)
        {
            // o ícone pintado do golpe (folhas G01..G06); sem ele, o do tipo da arma
            const FName Type = Hero->GetSheet() ? Hero->GetSheet()->GetWeaponType() : NAME_None;
            // 07/11: as 35 espadas (Data/Rpg/swords.json): o ícone da própria espada
            const FName Item = Hero->GetSheet() ? Hero->GetSheet()->GetEquipped(VortexRpg::Weapon) : NAME_None;
            const FVortexItemDef* ItemDef = VortexSwords::Find(Item) ? FVortexRpgCatalog::Get().Item(Item) : nullptr;
            UTexture2D* Texture = ItemDef ? FVortexRpgCatalog::Icon(ItemDef->Icon) : nullptr;
            if (!Texture) Texture = Type.IsNone() ? nullptr : FVortexRpgCatalog::Icon(FName(TEXT("weapon_") + Type.ToString()));
            if (!Texture) Texture = FVortexRpgCatalog::Icon(WeaponHudIcon(Type));
            Icon(Texture ? Texture : Tex("ability_sword_ashen_arc"), X, Y, FLinearColor::White);
        }
        else if (i == 1)
        {
            const float Gray = Remaining > 0.f ? .45f : 1.f;
            const UVortexWeaponDefinition* Definition = Weapon ? Weapon->GetDefinition() : nullptr;
            const FName Type = Hero->GetSheet() ? Hero->GetSheet()->GetWeaponType() : NAME_None;
            // 07/11: a espada com poder próprio mostra o ícone dele
            const VortexSwords::FSword* Sword = Hero->GetSheet() ? VortexSwords::Find(Hero->GetSheet()->GetEquipped(VortexRpg::Weapon)) : nullptr;
            UTexture2D* Special = Sword && Sword->bHasKit ? FVortexRpgCatalog::Icon(Sword->PowerIcon) : nullptr;
            if (!Special) Special = Type.IsNone() ? nullptr : FVortexRpgCatalog::Icon(FName(TEXT("special_") + Type.ToString()));
            if (!Special && Definition) Special = FVortexRpgCatalog::Icon(Definition->SecondaryIcon);
            Icon(Special ? Special : Tex("ability_flying_slash"), X, Y, FLinearColor(Gray, Gray, Gray, 1.f));
            if (Weapon) Veil(X, Y, SlotW, SlotH, Remaining / FMath::Max(.1f, Weapon->SpecialCooldownTotal()));
            if (Remaining > 0.f) Toon(FString::FromInt(FMath::CeilToInt(Remaining)), X, Y, SlotW, SlotH - 10.f * S, 32.f * S, Cream, 1);
        }
        else if (Skill)
        {
            const UVortexClassSkills* Skills = Hero->GetClassSkills();
            if (bLocked)
            {
                // ainda trancada: ícone bem apagado, cadeado e o nível em que libera
                Icon(FVortexRpgCatalog::Icon(Skill->Icon), X, Y, Srgb(60, 64, 90));
                const float LX = X + SlotW * .5f, LY = Y + SlotH * .4f, U = SlotW / 92.f;
                Slice("H_BarBack", LX - 10.f * U, LY - 18.f * U, 20.f * U, 22.f * U, 10.f * U, Srgb(230, 220, 190));
                Slice("H_PanelGold", LX - 14.f * U, LY - 6.f * U, 28.f * U, 22.f * U, 7.f * U);
                Toon(FString::Printf(TEXT("NV %d"), UVortexClassSkills::UnlockLevel(i - 2)), X, Y + SlotH * .62f, SlotW, SlotH * .3f, 15.f * S, Cream, 1);
            }
            else
            {
                const float Left = Skills->CooldownLeft(i - 2);
                const bool bMana = Hero->GetVitals() && Hero->GetVitals()->GetMana() >= Skill->Mana;
                const float Gray = Left > 0.f || !bMana ? .45f : 1.f;
                if (UTexture2D* Texture = FVortexRpgCatalog::Icon(Skill->Icon)) Icon(Texture, X, Y, FLinearColor(Gray, Gray, Gray, 1.f));
                else Toon(Skill->Name.Left(3), X, Y, SlotW, SlotH, 24.f * S, UVortexClassSkills::ElementColor(Skill->Element), 1);
                Veil(X, Y, SlotW, SlotH, Left / FMath::Max(.1f, Skills->CooldownTotal(i - 2)));
                if (Left > 0.f) Toon(FString::FromInt(FMath::CeilToInt(Left)), X, Y, SlotW, SlotH - 10.f * S, 32.f * S, Cream, 1);
                Toon(FString::FromInt(Skill->Mana), X + 8.f * S, Y + SlotH - 30.f * S, 40.f * S, 22.f * S, 15.f * S, bMana ? Srgb(140, 200, 255) : HpRed);
            }
        }
    }
    // poções: Q (cura) e R (mana), botões redondos com a quantidade
    struct FPotion { FName Icon; const TCHAR* Key; FKey Code; int32 Count; };
    const UVortexHeroSheet* Sheet = Hero->GetSheet();
    const FPotion Potions[] = { { "potion_red", TEXT("Q"), EKeys::Q, Sheet ? Sheet->CountOf(TEXT("potion_red")) : 0 },
        { "potion_blue", TEXT("R"), EKeys::R, Sheet ? Sheet->CountOf(TEXT("potion_blue")) : 0 } };
    for (int32 i = 0; i < 2; ++i)
    {
        const float X = X0 + 6.f * Step - 14.f * S + 26.f * S + i * 98.f * S, Y = SlotY + 6.f * S;
        const bool bHave = Potions[i].Count > 0;
        Sprite("H_Round", X + PotW * .5f, Y + PotW * .5f, PotW, 0.f, Down(Potions[i].Code) ? Srgb(255, 240, 170) : bHave ? Srgb(255, 225, 140) : Srgb(150, 156, 180));
        const float Gray = bHave ? 1.f : .45f;
        if (UTexture2D* Texture = Tex(Potions[i].Icon)) DrawTexture(Texture, X + 14.f * S, Y + 12.f * S, 56.f * S, 56.f * S, 0.f, 0.f, 1.f, 1.f, FLinearColor(Gray, Gray, Gray, 1.f));
        Sprite("H_Disc", X + PotW - 8.f * S, Y + 6.f * S, 30.f * S, 0.f, bHave ? HpRed : Srgb(90, 96, 130));
        Toon(FString::FromInt(Potions[i].Count), X + PotW - 23.f * S, Y - 9.f * S, 30.f * S, 30.f * S, 16.f * S, Cream, 1);
        Slice("H_Key", X + 2.f * S, Y + PotW - 24.f * S, 30.f * S, 28.f * S, 10.f * S);
        Toon(Potions[i].Key, X + 2.f * S, Y + PotW - 24.f * S, 30.f * S, 28.f * S, 15.f * S, Cream, 1);
    }
    // cargas de esquiva (Ctrl): bolinhas em cima da barra; a que está recarregando enche devagar
    if (Movement)
    {
        const int32 Max = FMath::Max(1, Movement->MaxDodgeCharges);
        const float Size = 16.f * S, Gap = 22.f * S, CX0 = X0 + 4.f * S, CY = SlotY - 24.f * S;
        const float Refill = Movement->DodgeChargeRegen > 0.f ? Movement->DodgeChargeProgress / Movement->DodgeChargeRegen : 0.f;
        for (int32 i = 0; i < Max; ++i)
        {
            const float C = CX0 + i * Gap;
            if (i < Movement->DodgeCharges) Sprite("H_Disc", C, CY, Size, 0.f, Srgb(120, 200, 255));
            else if (i == Movement->DodgeCharges) Sprite("H_Disc", C, CY, Size * FMath::Clamp(.4f + Refill * .6f, .4f, 1.f), 0.f, Srgb(70, 100, 160));
            else Sprite("H_Disc", C, CY, Size * .6f, 0.f, Srgb(40, 50, 90));
        }
        Toon(TEXT("CTRL"), CX0 + Max * Gap - 6.f * S, CY - 12.f * S, 60.f * S, 24.f * S, 13.f * S, Faded);
    }
}

// ---- mapa local (minimapa e detalhe do mapa grande) -----------------------------------------------------------------

void AVortexAimHUD::UploadWhole(UTexture2D*& Texture, int32 Size, const TArray<FColor>& Pixels)
{
    if (Pixels.Num() != Size * Size) return;
    if (!Texture)
    {
        Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
        if (!Texture) return;
        Texture->SRGB = true;
        Texture->Filter = TF_Nearest;   // blocos nítidos, como o mundo
        Texture->AddressX = Texture->AddressY = TA_Clamp;
        Texture->LODGroup = TEXTUREGROUP_UI;
        FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
        FMemory::Memcpy(Mip.BulkData.Lock(LOCK_READ_WRITE), Pixels.GetData(), Pixels.Num() * sizeof(FColor));
        Mip.BulkData.Unlock();
        Texture->UpdateResource();
        return;
    }
    uint8* Copy = static_cast<uint8*>(FMemory::Malloc(Pixels.Num() * sizeof(FColor)));
    FMemory::Memcpy(Copy, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
    FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, Size, Size);
    Texture->UpdateTextureRegions(0, 1, Region, Size * sizeof(FColor), sizeof(FColor), Copy,
        [](uint8* Data, const FUpdateTextureRegion2D* Regions) { FMemory::Free(Data); delete Regions; });
}

void AVortexAimHUD::UpdateLocalMaps(AVortexCharacter* Hero)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return;
    const float CS = Data->CellSize;
    // o minimapa: 512 x 512 blocos (384 m) em volta do herói, refeito quando ele chega perto da borda
    if (LocalJob.IsValid() && LocalJob.IsReady())
    {
        const TArray<FColor> Pixels = LocalJob.Consume();
        UTexture2D* Texture = LocalMap;
        UploadWhole(Texture, LocalCells, Pixels);
        LocalMap = Texture;
        LocalOrigin = LocalJobOrigin;
        bLocalReady = LocalMap != nullptr;
        LocalJob = TFuture<TArray<FColor>>();
    }
    const FIntPoint HeroCell(FMath::FloorToInt(Hero->GetActorLocation().X / CS), FMath::FloorToInt(Hero->GetActorLocation().Y / CS));
    const float Reach = UVortexPlayerSettings::Get()->MinimapRadius * 100.f / CS;
    const FIntPoint Center = LocalOrigin + FIntPoint(LocalCells / 2, LocalCells / 2);
    const bool bFar = FMath::Max(FMath::Abs(HeroCell.X - Center.X), FMath::Abs(HeroCell.Y - Center.Y)) > LocalCells / 2 - Reach * 1.05f - 12.f;
    if ((!bLocalReady || bFar) && !LocalJob.IsValid())
    {
        AVortexTown::LoadGrassMask();   // a planta da vila (para pintar no mapa), na thread do jogo
        LocalJobOrigin = HeroCell - FIntPoint(LocalCells / 2, LocalCells / 2);
        TArray<FPaintProp> Props = GatherProps(*Data, LocalJobOrigin, LocalCells);
        LocalJob = Async(EAsyncExecution::ThreadPool, [Data, Origin = LocalJobOrigin, Props = MoveTemp(Props)]()
        {
            return PaintCells(Data, Origin, LocalCells, Props, nullptr);
        });
    }
    // o detalhe do mapa grande: 1024 x 1024 blocos (768 m) onde o mapa está olhando, com a névoa
    if (DetailJob.IsValid() && DetailJob.IsReady())
    {
        const TArray<FColor> Pixels = DetailJob.Consume();
        UTexture2D* Texture = DetailMap;
        UploadWhole(Texture, DetailCells, Pixels);
        DetailMap = Texture;
        DetailOrigin = DetailJobOrigin;
        bDetailReady = DetailMap != nullptr;
        DetailJob = TFuture<TArray<FColor>>();
    }
    if (!bWorldMap || MapZoom < 6.f || DetailJob.IsValid() || !MapBounds.bIsValid) return;
    const FIntPoint ViewCell(FMath::FloorToInt(MapCenter.X / CS), FMath::FloorToInt(MapCenter.Y / CS));
    const FIntPoint DetailCenter = DetailOrigin + FIntPoint(DetailCells / 2, DetailCells / 2);
    const float HalfView = MapBounds.GetSize().Y / MapZoom * .5f / CS;
    const bool bMoved = FMath::Max(FMath::Abs(ViewCell.X - DetailCenter.X), FMath::Abs(ViewCell.Y - DetailCenter.Y)) > DetailCells / 2 - HalfView * 1.8f;
    if (bDetailReady && !bMoved && DetailSeenCount == float(SeenCount)) return;
    DetailJobOrigin = ViewCell - FIntPoint(DetailCells / 2, DetailCells / 2);
    DetailSeenCount = float(SeenCount);
    TSharedPtr<FFogInput> Fog;
    if (bSeenReady)
    {
        Fog = MakeShared<FFogInput>();
        Fog->Seen = Seen;
        Fog->SeenOrigin = SeenOrigin;
        Fog->SeenSize = SeenSize;
        Fog->BaseCell = Data->MapChunkMin() * Data->CellsPerChunk;
        Fog->Step = FMath::Max(1, MapStep);
    }
    TArray<FPaintProp> Props = GatherProps(*Data, DetailJobOrigin, DetailCells);
    DetailJob = Async(EAsyncExecution::ThreadPool, [Data, Origin = DetailJobOrigin, Props = MoveTemp(Props), Fog]()
    {
        return PaintCells(Data, Origin, DetailCells, Props, Fog);
    });
}

// ---- mapa do mundo em tela cheia ------------------------------------------------------------------------------------

void AVortexAimHUD::ToggleWorldMap()
{
    bWorldMap = !bWorldMap;
    bDragging = false;
    if (bWorldMap)
    {
        // 10/11 (o usuário: "inicia muito longe, sempre tenho que dar zoom"): abre perto do herói, já no detalhe por
        // bloco; a roda afasta até o continente inteiro
        if (const APawn* Pawn = PlayerOwner ? PlayerOwner->GetPawn() : nullptr) MapCenter = FVector2D(Pawn->GetActorLocation());
        MapZoom = 10.f;
    }
    if (APlayerController* Controller = PlayerOwner)
    {
        // com o mapa aberto o mouse é do mapa: aparece, e a câmera e o herói param
        if (bWorldMap)
        {
            Controller->SetShowMouseCursor(true);
            FInputModeGameAndUI Mode;
            Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            Mode.SetHideCursorDuringCapture(false);
            Controller->SetInputMode(Mode);
            Controller->SetIgnoreLookInput(true);
            Controller->SetIgnoreMoveInput(true);
        }
        else
        {
            Controller->SetShowMouseCursor(false);
            Controller->SetInputMode(FInputModeGameOnly());
            Controller->ResetIgnoreLookInput();
            Controller->ResetIgnoreMoveInput();
        }
    }
}

void AVortexAimHUD::ZoomWorldMap(float Steps)
{
    const float Old = MapZoom;
    MapZoom = FMath::Clamp(MapZoom * (Steps < 0.f ? 1.5f : 1.f / 1.5f), 1.f, 24.f);
    // o ponto embaixo do mouse fica no lugar (como nos mapas de celular)
    float MX = 0.f, MY = 0.f;
    if (bWorldMap && PlayerOwner && MapView.bIsValid && MapBounds.bIsValid && PlayerOwner->GetMousePosition(MX, MY) && MapView.IsInside(FVector2D(MX, MY)))
    {
        const FVector2D Offset = FVector2D(MX, MY) - MapView.GetCenter();
        const float Base = MapBounds.GetSize().Y / MapView.GetSize().Y;
        const FVector2D Under = MapCenter + Offset * (Base / Old);
        MapCenter = Under - Offset * (Base / MapZoom);
    }
}

void AVortexAimHUD::DrawWorldMap(AVortexCharacter* Hero)
{
    if (!MapTexture || !MapBounds.bIsValid) return;
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    const float S = Scale, OX = (Canvas->ClipX - 1920.f * S) * .5f;
    auto PX = [&](float V) { return OX + V * S; };
    const float Now = GetWorld()->GetTimeSeconds();
    // fundo escurecido e a moldura grande
    FCanvasTileItem Dim(FVector2D(0.f, 0.f), FVector2D(Canvas->ClipX, Canvas->ClipY), Srgb(8, 14, 36, .78f));
    Dim.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(Dim);
    Slice("H_PanelBlue", PX(60.f), 70.f * S, 1800.f * S, 940.f * S, 40.f * S);
    const float VX0 = PX(350.f), VY0 = 100.f * S, VW = 1480.f * S, VH = 880.f * S;
    MapView = FBox2D(FVector2D(VX0, VY0), FVector2D(VX0 + VW, VY0 + VH));
    FCanvasTileItem Frame(FVector2D(VX0 - 6.f * S, VY0 - 6.f * S), FVector2D(VW + 12.f * S, VH + 12.f * S), Ink);
    Canvas->DrawItem(Frame);
    // a vista: centímetros do mundo por pixel da tela (zoom 1 = a altura do mapa inteiro)
    const FVector2D Extent = MapBounds.GetSize();
    const float CmPx = Extent.Y / VH / MapZoom;
    const FVector2D Half(VW * .5f * CmPx, VH * .5f * CmPx);
    MapCenter.X = Half.X * 2.f >= Extent.X ? MapBounds.GetCenter().X : FMath::Clamp(MapCenter.X, MapBounds.Min.X + Half.X, MapBounds.Max.X - Half.X);
    MapCenter.Y = Half.Y * 2.f >= Extent.Y ? MapBounds.GetCenter().Y : FMath::Clamp(MapCenter.Y, MapBounds.Min.Y + Half.Y, MapBounds.Max.Y - Half.Y);
    auto ToScreen = [&](const FVector2D& W) { return FVector2D(VX0 + VW * .5f + (W.X - MapCenter.X) / CmPx, VY0 + VH * .5f + (W.Y - MapCenter.Y) / CmPx); };
    auto ToWorld = [&](const FVector2D& P) { return MapCenter + (P - FVector2D(VX0 + VW * .5f, VY0 + VH * .5f)) * CmPx; };
    auto Inside = [&](const FVector2D& P, float Pad = 0.f) { return P.X >= VX0 + Pad && P.Y >= VY0 + Pad && P.X <= VX0 + VW - Pad && P.Y <= VY0 + VH - Pad; };
    // o mar em volta e o continente por cima (com as nuvens onde ainda não foi visto); só a parte que existe
    // (a vista é mais larga que o mapa: esticar a borda da textura fazia faixas nos lados)
    {
        FCanvasTileItem Sea(FVector2D(VX0, VY0), FVector2D(VW, VH), Srgb(40, 92, 150));
        Canvas->DrawItem(Sea);
        const FVector2D W0(FMath::Max(MapCenter.X - Half.X, MapBounds.Min.X), FMath::Max(MapCenter.Y - Half.Y, MapBounds.Min.Y));
        const FVector2D W1(FMath::Min(MapCenter.X + Half.X, MapBounds.Max.X), FMath::Min(MapCenter.Y + Half.Y, MapBounds.Max.Y));
        if (W1.X > W0.X && W1.Y > W0.Y)
        {
            const FVector2D P0 = ToScreen(W0), P1 = ToScreen(W1);
            FCanvasTileItem Map(P0, MapTexture->GetResource(), P1 - P0, (W0 - MapBounds.Min) / Extent, (W1 - MapBounds.Min) / Extent, FLinearColor::White);
            Canvas->DrawItem(Map);
        }
    }
    // de perto: o detalhe por bloco por cima (onde ele cobre a vista)
    if (Terrain && DetailMap && bDetailReady && MapZoom >= 6.f)
    {
        const float CS = Terrain->CellSize;
        const FVector2D D0 = FVector2D(DetailOrigin) * CS, D1 = D0 + FVector2D(DetailCells * CS);
        const FVector2D W0(FMath::Max(D0.X, MapCenter.X - Half.X), FMath::Max(D0.Y, MapCenter.Y - Half.Y));
        const FVector2D W1(FMath::Min(D1.X, MapCenter.X + Half.X), FMath::Min(D1.Y, MapCenter.Y + Half.Y));
        if (W1.X > W0.X && W1.Y > W0.Y)
        {
            const FVector2D P0 = ToScreen(W0), P1 = ToScreen(W1);
            FCanvasTileItem Detail(P0, DetailMap->GetResource(), P1 - P0, (W0 - D0) / (DetailCells * CS), (W1 - D0) / (DetailCells * CS), FLinearColor::White);
            Canvas->DrawItem(Detail);
        }
    }
    // marcas: cada uma guarda o nome para o mouse mostrar
    struct FHover { FVector2D At; FString Name; };
    TArray<FHover> Hovers;
    auto Pin = [&](const FVector2D& World, FName Icon, float Size, const FString& Name, bool bShowName, const FLinearColor& Tint = FLinearColor::White)
    {
        const FVector2D P = ToScreen(World);
        if (!Inside(P, 6.f * S)) return;
        Sprite(Icon, P.X, P.Y, Size * S, 0.f, Tint);
        if (bShowName && !Name.IsEmpty()) Toon(Name, P.X - 150.f * S, P.Y + Size * .5f * S, 300.f * S, 24.f * S, 17.f * S, Cream, 1);
        if (!Name.IsEmpty()) Hovers.Add({ P, Name });
    };
    // nomes das regiões (de longe): claros onde já se foi, apagados onde não
    if (Terrain && Terrain->HasWorld() && MapZoom < 7.f)
        for (int32 Region = 1; Region < 10; ++Region)
        {
            const FVector2D Center = Terrain->RegionCenter(Region);
            const FVector2D P = ToScreen(Center);
            if (!Inside(P, 20.f * S)) continue;
            const bool bKnown = IsDiscovered(FVector(Center, 0.f));
            Toon(FText::FromString(Terrain->RegionName(Region)).ToUpper().ToString(), P.X - 220.f * S, P.Y - 18.f * S, 440.f * S, 36.f * S, (bKnown ? 30.f : 24.f) * S,
                bKnown ? Cream : Srgb(120, 130, 170, .9f), 1);
        }
    // lugares (ruínas, torres, santuários, minas, cemitérios, caravanas, cais, pontes) e portais
    for (const FMapPoi& Poi : MapPois)
    {
        if (!IsDiscovered(FVector(Poi.At, 0.f))) continue;
        const bool bPortal = Poi.Name.StartsWith(TEXT("Portal"));
        if (!(MapShow & (bPortal ? 8 : 4))) continue;
        Pin(Poi.At, bPortal ? FName("M_Portal") : FName("M_Poi"), bPortal ? 28.f : 22.f, Poi.Name, MapZoom >= 5.f, bPortal && Poi.Name.Contains(TEXT("VERMELHO")) ? Srgb(255, 120, 120) : FLinearColor::White);
    }
    if (MapShow & 4)
    {
        if (IsDiscovered(FVector(17000.f, 11000.f, 0.f))) Pin(FVector2D(17000.f, 11000.f), "M_Poi", 26.f, TEXT("Caldeira do Meteoro"), MapZoom >= 2.f, Srgb(255, 170, 110));
        if (Terrain && Terrain->HasWorld())
            for (const AVortexLandmarks::FSite& Site : AVortexLandmarks::Sites())
            {
                if (!IsDiscovered(FVector(Site.Center, 0.f))) continue;
                const bool bBoss = Site.Id == TEXT("seryth") || Site.Id == TEXT("thorun");
                Pin(Site.Center, bBoss ? FName("M_Boss") : FName("M_Poi"), bBoss ? 30.f : 26.f, Site.Name, MapZoom >= 2.f, Site.Id == TEXT("yggdrasil") ? Srgb(140, 255, 190) : FLinearColor::White);
            }
    }
    Pin(FVector2D(0.f, 1500.f), "M_Town", 36.f, TEXT("Cidade do Santuário"), MapZoom >= 1.5f && MapZoom < 6.f);
    if (MapZoom >= 4.f)
        for (const FTownPlace& Place : TownPlaces()) Pin(Place.At, Place.Icon, MapZoom >= 8.f ? 30.f : 24.f, Place.Name, MapZoom >= 8.f);
    // missões: os lugares a visitar (pulsando) e os moradores com ! ou ?
    const UVortexQuestLog* Log = Hero->GetQuestLog();
    const float Pulse = 1.f + .12f * FMath::Sin(Now * 4.f);
    if (Log && (MapShow & 1))
        for (const FVortexQuestState& State : Log->GetQuests())
        {
            const FVortexQuestDef* Q = State.State == 1 ? FVortexStoryCatalog::Get().Quest(State.Id) : nullptr;
            if (!Q) continue;
            for (int32 i = 0; i < Q->Objectives.Num(); ++i)
                if (Q->Objectives[i].Type == TEXT("visit") && Log->ObjectiveValue(*Q, i) < 1) Pin(Q->Objectives[i].Point, "M_Quest", 32.f * Pulse, Q->Title, false);
        }
    for (TActorIterator<AVortexNpc> It(GetWorld()); It; ++It)
    {
        const FString Mark = Log ? Log->MarkFor(It->GetNpcId()) : FString();
        if (!Mark.IsEmpty() && (MapShow & 1)) Pin(FVector2D(It->GetActorLocation()), "M_Quest", 28.f, It->GetDisplayName(), MapZoom >= 6.f, Mark == TEXT("?") ? Srgb(150, 255, 150) : FLinearColor::White);
        else if (Mark.IsEmpty() && (MapShow & 2) && MapZoom >= 7.f) Pin(FVector2D(It->GetActorLocation()), "M_Npc", 18.f, It->GetDisplayName(), MapZoom >= 10.f);
    }
    // chefes vistos, o grupo e o marcador
    if (MapShow & 16)
        for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
        {
            const IVortexCreature* Creature = Cast<IVortexCreature>(*It);
            if (Creature && Creature->IsBoss() && !Creature->IsCreatureDead() && WasSeen(*It) && IsDiscovered(It->GetActorLocation()))
                Pin(FVector2D(It->GetActorLocation()), "M_Boss", 32.f, Creature->GetCreatureTitle(), false);
        }
    if (MapShow & 32)
        for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
            if (*It != Hero)
                Pin(FVector2D(It->GetActorLocation()), "M_Party", 24.f, It->GetSocial() ? It->GetSocial()->GetHeroName() : FString(TEXT("Companheiro")), true,
                    It->GetSocial() ? It->GetSocial()->GetPlayerColor() : FLinearColor::White);
    for (const FVortexPing& Ping : VortexPings::All())
        if (GetWorld()->GetTimeSeconds() - Ping.Time < VortexPings::Life)
            Pin(FVector2D(Ping.At), "M_Flag", 26.f, Ping.Who.IsEmpty() ? FString(TEXT("Marcação")) : TEXT("Marcação de ") + Ping.Who, false,
                Ping.bDanger ? FLinearColor(1.f, .25f, .2f) : Ping.Color);
    if (bHasMarker && (MapShow & 64)) Pin(Marker, "M_Flag", 38.f, TEXT("Meu marcador"), MapZoom >= 3.f);
    // o herói
    {
        const FVector2D P = ToScreen(FVector2D(Hero->GetActorLocation()));
        const float Yaw = PlayerOwner && PlayerOwner->PlayerCameraManager ? PlayerOwner->PlayerCameraManager->GetCameraRotation().Yaw : Hero->GetActorRotation().Yaw;
        if (Inside(P)) Sprite("M_Arrow", P.X, P.Y, 38.f * S, Yaw + 90.f);
    }
    // régua: um número redondo perto de 140 px
    {
        static const float Nice[] = { 10.f, 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f };
        float Meters = Nice[0];
        for (const float N : Nice) if (N * 100.f / CmPx <= 160.f * S) Meters = N;
        const float L = Meters * 100.f / CmPx;
        const float RX = VX0 + VW - 40.f * S - L, RY = VY0 + VH - 26.f * S;
        Slice("H_BarFill", RX, RY, L, 10.f * S, 5.f * S, Cream);
        Toon(Meters >= 1000.f ? FString::Printf(TEXT("%.0f km"), Meters / 1000.f) : FString::Printf(TEXT("%.0f m"), Meters), RX, RY - 30.f * S, L, 26.f * S, 18.f * S, Cream, 1);
    }
    // título
    Slice("H_PanelGold", 960.f * S + OX - 260.f * S, 36.f * S, 520.f * S, 64.f * S, 30.f * S);
    Toon(TEXT("MAPA DO MUNDO"), 960.f * S + OX - 260.f * S, 36.f * S, 520.f * S, 64.f * S, 36.f * S, Cream, 1);

    // ---- coluna da esquerda: legenda (com filtros), zoom, explorado e centralizar
    float MX = -1.f, MY = -1.f;
    if (PlayerOwner) PlayerOwner->GetMousePosition(MX, MY);
    const FVector2D Mouse(MX, MY);
    const bool bClick = PlayerOwner && PlayerOwner->WasInputKeyJustPressed(EKeys::LeftMouseButton);
    auto Hit = [&](float X, float Y, float W, float H) { return MX >= X && MY >= Y && MX <= X + W && MY <= Y + H; };
    const float LX = PX(86.f), LW = 240.f * S;
    Slice("H_PanelNavy", LX, 110.f * S, LW, 470.f * S, 24.f * S);
    Toon(TEXT("LEGENDA"), LX, 118.f * S, LW, 40.f * S, 24.f * S, Gold, 1);
    struct FRow { FName Icon; const TCHAR* Name; uint8 Bit; };
    static const FRow Rows[] = { { "M_Quest", TEXT("Missões"), 1 }, { "M_Npc", TEXT("Moradores"), 2 }, { "M_Poi", TEXT("Lugares"), 4 }, { "M_Portal", TEXT("Portais"), 8 },
        { "M_Boss", TEXT("Chefes vistos"), 16 }, { "M_Party", TEXT("Grupo"), 32 }, { "M_Flag", TEXT("Meu marcador"), 64 } };
    for (int32 i = 0; i < UE_ARRAY_COUNT(Rows); ++i)
    {
        const float Y = 170.f * S + i * 56.f * S;
        const bool bOn = (MapShow & Rows[i].Bit) != 0;
        if (Hit(LX, Y - 22.f * S, LW, 44.f * S))
        {
            Slice("H_BarBack", LX + 8.f * S, Y - 22.f * S, LW - 16.f * S, 44.f * S, 16.f * S, Srgb(120, 150, 255, .5f));
            if (bClick) { MapShow ^= Rows[i].Bit; VortexAudio::Play2D(this, TEXT("ui_click"), .5f); }
        }
        Sprite(Rows[i].Icon, LX + 34.f * S, Y, 26.f * S);
        Toon(Rows[i].Name, LX + 58.f * S, Y - 18.f * S, 130.f * S, 36.f * S, 17.f * S, bOn ? Cream : Faded);
        Slice("H_BarFill", LX + LW - 46.f * S, Y - 15.f * S, 30.f * S, 30.f * S, 9.f * S, bOn ? Green : Srgb(90, 100, 130));
        if (bOn)
        {
            DrawLine(LX + LW - 39.f * S, Y, LX + LW - 33.f * S, Y + 7.f * S, Cream, 4.f * S);
            DrawLine(LX + LW - 33.f * S, Y + 7.f * S, LX + LW - 22.f * S, Y - 7.f * S, Cream, 4.f * S);
        }
    }
    Slice("H_PanelNavy", LX, 600.f * S, LW, 150.f * S, 24.f * S);
    Toon(TEXT("ZOOM"), LX, 606.f * S, LW, 36.f * S, 22.f * S, Gold, 1);
    static const TCHAR* Levels[] = { TEXT("Mundo"), TEXT("Região"), TEXT("Perto") };
    static const float LevelZoom[] = { 1.f, 3.f, 12.f };
    const int32 Level = MapZoom >= 8.f ? 2 : MapZoom >= 2.f ? 1 : 0;
    for (int32 i = 0; i < 3; ++i)
    {
        const float Y = 660.f * S + i * 28.f * S;
        if (Hit(LX, Y - 14.f * S, LW, 28.f * S) && bClick) { MapZoom = LevelZoom[i]; VortexAudio::Play2D(this, TEXT("ui_click"), .5f); }
        Sprite("H_Disc", LX + 36.f * S, Y, 18.f * S, 0.f, i == Level ? Gold : Srgb(80, 90, 130));
        Toon(Levels[i], LX + 56.f * S, Y - 13.f * S, 150.f * S, 26.f * S, 17.f * S, i == Level ? Cream : Faded);
    }
    Slice("H_PanelBlue", LX, 770.f * S, LW, 56.f * S, 22.f * S);
    Toon(FString::Printf(TEXT("EXPLORADO %d%%"), FMath::RoundToInt(ExploredFraction() * 100.f)), LX, 770.f * S, LW, 56.f * S, 19.f * S, Cream, 1);
    const bool bCenterHover = Hit(LX, 846.f * S, LW, 56.f * S);
    Slice("H_PanelGold", LX, 846.f * S, LW, 56.f * S, 22.f * S, bCenterHover ? Srgb(255, 245, 200) : FLinearColor::White);
    Toon(TEXT("CENTRALIZAR"), LX, 846.f * S, LW, 56.f * S, 19.f * S, Cream, 1);
    if (bCenterHover && bClick) { MapCenter = FVector2D(Hero->GetActorLocation()); VortexAudio::Play2D(this, TEXT("ui_click"), .5f); }
    // dicas
    Slice("H_PanelNavy", 960.f * S + OX - 430.f * S, 1022.f * S, 860.f * S, 44.f * S, 22.f * S);
    Toon(TEXT("ARRASTAR: MOVER  •  RODA: ZOOM  •  CLIQUE DIREITO: MARCAR  •  M: FECHAR"), 960.f * S + OX - 430.f * S, 1022.f * S, 860.f * S, 44.f * S, 17.f * S, Cream, 1);

    // ---- o mouse no mapa: arrastar, marcar (clique direito; de novo em cima tira) e o nome do que está embaixo
    const bool bInView = Inside(Mouse);
    if (bClick && bInView) { bDragging = true; DragMouse = Mouse; DragCenter = MapCenter; }
    if (bDragging)
    {
        if (!PlayerOwner || !PlayerOwner->IsInputKeyDown(EKeys::LeftMouseButton)) bDragging = false;
        else MapCenter = DragCenter - (Mouse - DragMouse) * CmPx;
    }
    if (bInView && PlayerOwner && PlayerOwner->WasInputKeyJustPressed(EKeys::RightMouseButton))
    {
        if (bHasMarker && FVector2D::Distance(ToScreen(Marker), Mouse) < 24.f * S) bHasMarker = false;
        else SetMapMarker(ToWorld(Mouse));
        VortexAudio::Play2D(this, TEXT("ui_click"), .6f);
    }
    if (bInView && !bDragging)
    {
        const FHover* Best = nullptr;
        float BestD = 22.f * S;
        for (const FHover& H : Hovers) if (FVector2D::Distance(H.At, Mouse) < BestD) { BestD = FVector2D::Distance(H.At, Mouse); Best = &H; }
        if (Best)
        {
            const float TW = ToonSize(Best->Name, 18.f * S).X + 30.f * S;
            Slice("H_PanelNavy", Mouse.X + 18.f * S, Mouse.Y - 44.f * S, TW, 38.f * S, 18.f * S);
            Toon(Best->Name, Mouse.X + 18.f * S, Mouse.Y - 44.f * S, TW, 38.f * S, 18.f * S, Cream, 1);
        }
    }
}
