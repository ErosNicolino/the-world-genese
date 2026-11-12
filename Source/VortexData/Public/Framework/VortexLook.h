#pragma once

#include "CoreMinimal.h"

class UTexture2D;
class UObject;

/**
 * Aparência do herói: raça, visual (aventureiro/aventureira), uma peça por categoria e as cores.
 * Cosméticos em três lugares que se combinam: "head" (chapéu e cabeça), "face_gear" (rosto) e
 * "back" (pescoço e costas), cada um com "<lugar>_none". Saves com o "accessory" antigo migram.
 * Dados em Data/GodotCharacter/look_catalog.json (Tools/BuildLookCatalog.py, a partir do Godot).
 *
 * Cada raça tem peças próprias e regras próprias (o que o Vortex acrescenta ao Godot):
 * - "own": nessas categorias só aparecem as peças da raça (o elfo só tem cabelos e olhos élficos);
 * - "hide": categorias que a raça não tem (o morto-vivo é sempre careca, sem olhos nem boca comuns);
 * - "faceless": o rosto-base some, e o crânio escolhido ocupa a cabeça.
 *
 * Cores: a textura do herói é uma paleta (blocos de 4x4 px, um por tom). Cada tom sabe a que cor
 * pertence (pele, camisa, botas...) e a cor-base da peça; recolorir = cor escolhida x (tom / base),
 * então sombras e detalhes continuam. BuildPalette monta a textura de um herói.
 */
struct VORTEXDATA_API FVortexLook
{
    FName Race = TEXT("humano");
    FName Appearance = TEXT("male");
    TMap<FName, FName> Pieces;        // categoria (race, hair, eyes...) -> id da peça
    TMap<FName, FColor> Colors;       // cor (skin_color...) -> sRGB
};

/** Proporção do corpo: tamanho geral e de cada parte (1 = humano). */
struct VORTEXDATA_API FVortexBodyShape
{
    float Height = 1.f, Width = 1.f;       // corpo inteiro (altura e largura/profundidade)
    float Legs = 1.f, LegThick = 1.f;      // comprimento e grossura das pernas
    float Torso = 1.f, Arms = 1.f;         // largura do tronco, grossura dos braços
    float Head = 1.f;                      // tamanho da cabeça
    float Meters = 1.4f;                   // altura que a criação mostra
    float ArmSpread = 0.f;                 // graus: os braços abertos para fora (heróis largos: não entram no corpo)
    float ShoulderOut = 0.f;               // cm: o ombro sai do tronco (a junta ficava funda dentro da roupa)
    /** Altura do corpo em relação ao humano (para a cápsula e a câmera). */
    float Stature() const { return Meters / 1.4f; }
};

struct VORTEXDATA_API FVortexRaceInfo
{
    FName Id;
    FString Name, Tagline, Epithet, TraitLabel;
    TArray<int32> Start;              // 8 atributos iniciais (Força ... Sorte)
    float Scale = 1.f;
    TArray<FString> Traits;
    TArray<FColor> Skin;
    TArray<FString> Features;         // prefixos dos traços da raça ("elf_"...) ou ids
    FLinearColor Accent = FLinearColor::White;
    TArray<FString> NameStart, NameEnd;
    TArray<FName> Own, Hide;
    bool bFaceless = false;
    FVortexBodyShape Shape;
    bool IsWeakness(const FString& Trait) const
    {
        return Trait.StartsWith(TEXT("-")) || Trait.StartsWith(TEXT("Recebe +")) || Trait.StartsWith(TEXT("Não "));
    }
};

struct VORTEXDATA_API FVortexPieceInfo
{
    FName Id, Category;
    FString Name;
    TMap<FName, FColor> Defaults;
    TArray<FName> Races;
};

/** Cabelo do humano de blocos (01/11, Tools/Blender/MakeBlockHair.py e Tools/ImportBlockHair.py): uma malha à parte
 *  que o herói veste por cima do corpo, na cor "hair_color". A escolha fica em Look.Pieces["block_hair"]. */
struct VORTEXDATA_API FVortexBlockHair
{
    FName Id;
    FString Name;
    FString Mesh;   // a malha (vazio = careca)
    float Looseness = 0.f;   // física: 0 = preso à cabeça, .5 = leve, 1 = solto (UVortexHairComponent)
    FName Body;              // o corpo em que aparece no menu ("male", "female"; vazio = nos dois)
    bool Fits(FName Appearance) const { return Body.IsNone() || Body == Appearance; }
};

/** Roupa do humano de blocos (01/11, Tools/Blender/MakeBlockOutfit.py e Tools/ImportBlockOutfits.py): uma malha à parte
 *  com a textura própria, vestida por cima do corpo com a pose dele. A escolha fica em Look.Pieces["block_outfit"]. */
struct VORTEXDATA_API FVortexBlockOutfit
{
    FName Id;
    FString Name;
    FString Mesh;   // a malha (vazio = sem roupa)
    FName Body;     // o corpo em que aparece ("male", "female"; vazio = nos dois)
    FName Race;     // vazio = todas as raças; "elfo" = só no menu do elfo
    bool bCloth = false;   // tem a malha <Mesh>_tecido (a saia em Chaos Cloth)
    bool Fits(FName Appearance) const { return Body.IsNone() || Body == Appearance; }
    bool FitsRace(FName InRace) const { return Race.IsNone() || Race == InRace; }
};

struct VORTEXDATA_API FVortexColorSlot
{
    FName Key;
    FString Label;
    FColor Default;
    TArray<FColor> Palette;
};

class VORTEXDATA_API FVortexLookCatalog
{
public:
    static const FVortexLookCatalog& Get();
    bool IsValid() const { return Races.Num() > 0; }

    TArray<FString> Attributes;
    TArray<FVortexRaceInfo> Races;
    TArray<FVortexColorSlot> Slots;
    TArray<TPair<FName, FString>> Categories;   // ordem do criador
    TMap<FName, FVortexPieceInfo> Pieces;

    const FVortexRaceInfo* Race(FName Id) const;
    const FVortexPieceInfo* Piece(FName Id) const { return Pieces.Find(Id); }
    const FVortexColorSlot* Slot(FName Key) const;
    FString CategoryLabel(FName Category, FName RaceId) const;

    /** Peças que esta raça/visual pode usar numa categoria (as da raça primeiro). Vazio = a raça não tem. */
    TArray<FName> Options(const FVortexLook& Look, FName Category) const;
    bool IsHidden(FName RaceId, FName Category) const;
    /** Completa e corrige: peça válida em cada categoria, cores de todos os espaços. */
    FVortexLook Normalized(const FVortexLook& Look) const;
    /** Troca a peça e já aplica as cores padrão dela (a punk vem preta, o manto azul...). */
    void ChoosePiece(FVortexLook& Look, FName Category, FName PieceId) const;
    FVortexLook Random(FName RaceId, FName Appearance, FRandomStream& Rng, bool bOnlyColors = false, const FVortexLook* Keep = nullptr) const;
    FString RandomName(FName RaceId, FRandomStream& Rng) const;
    /** Peças fixas do visual (rosto, alça, cinto) que ficam fora das categorias. */
    FName FacePiece(const FVortexLook& Look) const;

    /** Paleta recolorida deste visual (textura nova, 64x1024, sem filtro). */
    UTexture2D* BuildPalette(const FVortexLook& Look, UObject* Outer) const;

    /** Os cabelos do humano de blocos (o primeiro é o padrão). */
    static const TArray<FVortexBlockHair>& BlockHairs();
    static const FVortexBlockHair* BlockHair(const FVortexLook& Look);
    static bool HasBlockHair(FName RaceId) { return RaceId == TEXT("humano") || RaceId == TEXT("elfo"); }   // o elfo usa o corpo do humano (09/11)
    /** As roupas do humano de blocos (a primeira de cada corpo é a padrão). */
    static const TArray<FVortexBlockOutfit>& BlockOutfits();
    static const FVortexBlockOutfit* BlockOutfit(const FVortexLook& Look);

    static FString ColorHex(const FColor& Color) { return FString::Printf(TEXT("#%02X%02X%02X"), Color.R, Color.G, Color.B); }

private:
    FVortexLookCatalog();
    TMap<FString, TMap<FName, TArray<FName>>> Catalog;   // "Male"/"Female" -> categoria -> peças
    struct FEntry { int32 Index; FName Slot; FColor Base; };
    TArray<FEntry> Entries;
    TArray<FColor> BasePixels;
    int32 PaletteWidth = 0, PaletteHeight = 0, Columns = 16, BlockPx = 4;
};
