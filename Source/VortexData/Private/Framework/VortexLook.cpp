#include "Framework/VortexLook.h"
#include "Framework/VortexFaceStyles.h"
#include "Framework/VortexPieces.h"

#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    /** As entradas de uma lista de Data/Looks/<arquivo> (o campo dado), na ordem do arquivo. */
    TArray<TSharedPtr<FJsonObject>> LookEntries(const TCHAR* File, const TCHAR* Field)
    {
        TArray<TSharedPtr<FJsonObject>> Out;
        FString Text;
        TSharedPtr<FJsonObject> Root;
        if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Looks") / File)) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("VORTEX_LOOK sem Data/Looks/%s"), File);
            return Out;
        }
        const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
        if (Root->TryGetArrayField(Field, Items))
            for (const TSharedPtr<FJsonValue>& Item : *Items)
                if (Item.IsValid() && Item->AsObject().IsValid()) Out.Add(Item->AsObject());
        return Out;
    }

    FColor ParseHex(const FString& Hex)
    {
        return FColor::FromHex(Hex);
    }

    TArray<FString> Strings(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
    {
        TArray<FString> Out;
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (Object->TryGetArrayField(Field, Values))
        {
            for (const TSharedPtr<FJsonValue>& Value : *Values) Out.Add(Value->AsString());
        }
        return Out;
    }

    TArray<FName> Names(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
    {
        TArray<FName> Out;
        for (const FString& Value : Strings(Object, Field)) Out.Add(FName(*Value));
        return Out;
    }
}

const FVortexLookCatalog& FVortexLookCatalog::Get()
{
    static FVortexLookCatalog Instance;
    return Instance;
}

FVortexLookCatalog::FVortexLookCatalog()
{
    const FString Dir = FPaths::ProjectDir() / TEXT("Data/GodotCharacter");
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *(Dir / TEXT("look_catalog.json"))))
    {
        UE_LOG(LogTemp, Error, TEXT("VORTEX_LOOK sem look_catalog.json"));
        return;
    }
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root) return;
    Attributes = Strings(Root, TEXT("attributes"));
    for (const TSharedPtr<FJsonValue>& Value : Root->GetArrayField(TEXT("races")))
    {
        const TSharedPtr<FJsonObject> R = Value->AsObject();
        FVortexRaceInfo Race;
        Race.Id = FName(*R->GetStringField(TEXT("id")));
        Race.Name = R->GetStringField(TEXT("name"));
        Race.Tagline = R->GetStringField(TEXT("tagline"));
        Race.Epithet = R->GetStringField(TEXT("epithet"));
        Race.TraitLabel = R->GetStringField(TEXT("traitLabel"));
        for (const TSharedPtr<FJsonValue>& N : R->GetArrayField(TEXT("start"))) Race.Start.Add(int32(N->AsNumber()));
        Race.Scale = float(R->GetNumberField(TEXT("scale")));
        Race.Traits = Strings(R, TEXT("traits"));
        for (const FString& Hex : Strings(R, TEXT("skin"))) Race.Skin.Add(ParseHex(Hex));
        Race.Features = Strings(R, TEXT("features"));
        Race.Accent = FLinearColor(ParseHex(R->GetStringField(TEXT("accent"))));
        Race.NameStart = Strings(R, TEXT("nameStart"));
        Race.NameEnd = Strings(R, TEXT("nameEnd"));
        Race.Own = Names(R, TEXT("own"));
        Race.Hide = Names(R, TEXT("hide"));
        Race.bFaceless = R->GetBoolField(TEXT("faceless"));
        const TSharedPtr<FJsonObject>* Shape = nullptr;
        if (R->TryGetObjectField(TEXT("shape"), Shape))
        {
            auto Field = [&Shape](const TCHAR* Name, float Default)
            {
                double Value = Default;
                (*Shape)->TryGetNumberField(Name, Value);
                return float(Value);
            };
            Race.Shape.Height = Field(TEXT("height"), 1.f);
            Race.Shape.Width = Field(TEXT("width"), 1.f);
            Race.Shape.Legs = Field(TEXT("legs"), 1.f);
            Race.Shape.LegThick = Field(TEXT("legThick"), 1.f);
            Race.Shape.Torso = Field(TEXT("torso"), 1.f);
            Race.Shape.Arms = Field(TEXT("arms"), 1.f);
            Race.Shape.Head = Field(TEXT("head"), 1.f);
            Race.Shape.Meters = Field(TEXT("meters"), 1.4f);
        }
        Races.Add(Race);
    }
    for (const TSharedPtr<FJsonValue>& Value : Root->GetArrayField(TEXT("colorSlots")))
    {
        const TSharedPtr<FJsonObject> S = Value->AsObject();
        FVortexColorSlot Slot;
        Slot.Key = FName(*S->GetStringField(TEXT("key")));
        Slot.Label = S->GetStringField(TEXT("label"));
        Slot.Default = ParseHex(S->GetStringField(TEXT("default")));
        for (const FString& Hex : Strings(S, TEXT("palette"))) Slot.Palette.Add(ParseHex(Hex));
        Slots.Add(Slot);
    }
    for (const TSharedPtr<FJsonValue>& Value : Root->GetArrayField(TEXT("categories")))
    {
        const TSharedPtr<FJsonObject> C = Value->AsObject();
        Categories.Emplace(FName(*C->GetStringField(TEXT("id"))), C->GetStringField(TEXT("label")));
    }
    for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Root->GetObjectField(TEXT("pieces"))->Values)
    {
        const TSharedPtr<FJsonObject> P = Pair.Value->AsObject();
        FVortexPieceInfo Piece;
        Piece.Id = FName(*Pair.Key);
        Piece.Name = P->GetStringField(TEXT("name"));
        Piece.Category = FName(*P->GetStringField(TEXT("category")));
        for (const TPair<FString, TSharedPtr<FJsonValue>> D : P->GetObjectField(TEXT("defaults"))->Values)
        {
            Piece.Defaults.Add(FName(*D.Key), ParseHex(D.Value->AsString()));
        }
        Piece.Races = Names(P, TEXT("races"));
        Pieces.Add(Piece.Id, Piece);
    }
    for (const TPair<FString, TSharedPtr<FJsonValue>> Who : Root->GetObjectField(TEXT("catalog"))->Values)
    {
        TMap<FName, TArray<FName>>& Lists = Catalog.Add(Who.Key);
        for (const TPair<FString, TSharedPtr<FJsonValue>> Category : Who.Value->AsObject()->Values)
        {
            TArray<FName>& List = Lists.Add(FName(*Category.Key));
            for (const TSharedPtr<FJsonValue>& Id : Category.Value->AsArray()) List.Add(FName(*Id->AsString()));
        }
    }
    // peças só do Vortex (Tools/BuildJuliaPieces.py: cabelo com pontas azuis, óculos redondos...):
    // entram no catálogo e na lista de opções de cada aparência
    FString Extra;
    TSharedPtr<FJsonObject> ExtraRoot;
    if (FFileHelper::LoadFileToString(Extra, *(Dir / TEXT("look_catalog_extra.json"))) &&
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Extra), ExtraRoot) && ExtraRoot)
    {
        for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : ExtraRoot->GetObjectField(TEXT("pieces"))->Values)
        {
            const TSharedPtr<FJsonObject> P = Pair.Value->AsObject();
            FVortexPieceInfo Piece;
            Piece.Id = FName(*Pair.Key);
            Piece.Name = P->GetStringField(TEXT("name"));
            Piece.Category = FName(*P->GetStringField(TEXT("category")));
            Piece.Races = Names(P, TEXT("races"));
            Pieces.Add(Piece.Id, Piece);
            for (TPair<FString, TMap<FName, TArray<FName>>>& Lists : Catalog)
            {
                if (TArray<FName>* List = Lists.Value.Find(Piece.Category)) List->AddUnique(Piece.Id);
            }
        }
    }

    // paleta-base e mapa dos tons
    FImage Image;
    if (FImageUtils::LoadImage(*(Dir / TEXT("palette.png")), Image))
    {
        Image.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
        PaletteWidth = Image.SizeX;
        PaletteHeight = Image.SizeY;
        BasePixels = TArray<FColor>(Image.AsBGRA8().GetData(), Image.AsBGRA8().Num());
    }
    if (FFileHelper::LoadFileToString(Text, *(Dir / TEXT("palette_map.json"))))
    {
        TSharedPtr<FJsonObject> Map;
        if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Map) && Map)
        {
            Columns = int32(Map->GetNumberField(TEXT("cols")));
            BlockPx = int32(Map->GetNumberField(TEXT("px")));
            for (const TSharedPtr<FJsonValue>& Value : Map->GetArrayField(TEXT("entries")))
            {
                const TSharedPtr<FJsonObject> E = Value->AsObject();
                FString Slot;
                const TArray<TSharedPtr<FJsonValue>>* Base = nullptr;
                if (!E->TryGetStringField(TEXT("slot"), Slot) || !E->TryGetArrayField(TEXT("base"), Base) || Base->Num() < 3) continue;
                Entries.Add({ int32(E->GetNumberField(TEXT("i"))), FName(*Slot),
                    FColor(uint8((*Base)[0]->AsNumber()), uint8((*Base)[1]->AsNumber()), uint8((*Base)[2]->AsNumber())) });
            }
        }
    }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_LOOK racas=%d pecas=%d cores=%d tons=%d paleta=%dx%d"),
        Races.Num(), Pieces.Num(), Slots.Num(), Entries.Num(), PaletteWidth, PaletteHeight);
}

const FVortexRaceInfo* FVortexLookCatalog::Race(FName Id) const
{
    const FVortexRaceInfo* Found = Races.FindByPredicate([Id](const FVortexRaceInfo& R) { return R.Id == Id; });
    return Found ? Found : (Races.Num() ? &Races[0] : nullptr);
}

const FVortexColorSlot* FVortexLookCatalog::Slot(FName Key) const
{
    return Slots.FindByPredicate([Key](const FVortexColorSlot& S) { return S.Key == Key; });
}

FString FVortexLookCatalog::CategoryLabel(FName Category, FName RaceId) const
{
    if (Category == TEXT("race"))
    {
        if (const FVortexRaceInfo* R = Race(RaceId)) return R->TraitLabel;
    }
    const TPair<FName, FString>* Found = Categories.FindByPredicate([Category](const TPair<FName, FString>& C) { return C.Key == Category; });
    return Found ? Found->Value : Category.ToString();
}

bool FVortexLookCatalog::IsHidden(FName RaceId, FName Category) const
{
    const FVortexRaceInfo* R = Race(RaceId);
    return R && R->Hide.Contains(Category);
}

TArray<FName> FVortexLookCatalog::Options(const FVortexLook& Look, FName Category) const
{
    TArray<FName> Out;
    const FVortexRaceInfo* R = Race(Look.Race);
    if (!R || R->Hide.Contains(Category)) return Out;
    const TMap<FName, TArray<FName>>* Lists = Catalog.Find(Look.Appearance == TEXT("female") ? TEXT("Female") : TEXT("Male"));
    const TArray<FName>* All = Lists ? Lists->Find(Category) : nullptr;
    if (!All) return Out;
    TArray<FName> Mine, Common;
    for (const FName& Id : *All)
    {
        const FVortexPieceInfo* P = Pieces.Find(Id);
        if (!P) continue;
        if (P->Races.Num() > 0)
        {
            if (P->Races.Contains(R->Id)) Mine.Add(Id);
            continue;
        }
        if (Category == TEXT("race"))
        {
            // traços: só os desta raça (pelo prefixo, como no Godot)
            const FString IdText = Id.ToString();
            const bool bAllowed = R->Features.ContainsByPredicate([&IdText](const FString& F)
            {
                return F.EndsWith(TEXT("_")) ? IdText.StartsWith(F) : IdText == F;
            });
            if (bAllowed) Mine.Add(Id);
            continue;
        }
        Common.Add(Id);
    }
    // traços próprios primeiro; nas categorias "own" da raça, só eles
    if (Category == TEXT("race"))
    {
        // o "nenhum" vai para o fim: raças com traço começam com ele aparecendo
        Mine.Sort([](const FName& A, const FName& B) { return (A == TEXT("race_none")) < (B == TEXT("race_none")); });
        return Mine.Num() ? Mine : TArray<FName>{ TEXT("race_none") };
    }
    Out = Mine;
    if (!R->Own.Contains(Category) || Mine.IsEmpty()) Out.Append(Common);
    // cosméticos: "nenhum" sempre primeiro (o herói começa sem chapéu, óculos ou capa)
    const FName None(*(Category.ToString() + TEXT("_none")));
    if (Out.Remove(None) > 0) Out.Insert(None, 0);
    return Out;
}

FName FVortexLookCatalog::FacePiece(const FVortexLook& Look) const
{
    const FVortexRaceInfo* R = Race(Look.Race);
    if (R && R->bFaceless) return NAME_None;
    return Look.Appearance == TEXT("female") ? FName(TEXT("face_female")) : FName(TEXT("face_male"));
}

void FVortexLookCatalog::ChoosePiece(FVortexLook& Look, FName Category, FName PieceId) const
{
    Look.Pieces.Add(Category, PieceId);
    if (const FVortexPieceInfo* P = Pieces.Find(PieceId))
    {
        for (const TPair<FName, FColor>& D : P->Defaults)
        {
            if (Slot(D.Key)) Look.Colors.Add(D.Key, D.Value);
        }
    }
}

FVortexLook FVortexLookCatalog::Normalized(const FVortexLook& In) const
{
    FVortexLook Look = In;
    if (!Races.ContainsByPredicate([&Look](const FVortexRaceInfo& R) { return R.Id == Look.Race; })) Look.Race = TEXT("humano");
    if (Look.Appearance != TEXT("female")) Look.Appearance = TEXT("male");
    // saves antigos usavam "Hair", "Shirt"...: passa para minúsculas
    TMap<FName, FName> Lower;
    for (const TPair<FName, FName>& Pair : Look.Pieces) Lower.Add(FName(*Pair.Key.ToString().ToLower()), Pair.Value);
    // o "acessório" único de antes vai para o lugar dele (chapéu, rosto ou costas)
    if (const FName* Old = Lower.Find(TEXT("accessory")))
    {
        if (const FVortexPieceInfo* P = Pieces.Find(*Old)) Lower.Add(P->Category, *Old);
    }
    Look.Pieces.Reset();
    for (const TPair<FName, FString>& Category : Categories)
    {
        const TArray<FName> Options_ = Options(Look, Category.Key);
        if (Options_.IsEmpty()) continue;
        const FName* Chosen = Lower.Find(Category.Key);
        if (Chosen && Options_.Contains(*Chosen)) Look.Pieces.Add(Category.Key, *Chosen);
        else ChoosePiece(Look, Category.Key, Options_[0]);
    }
    // o cabelo do humano de blocos (fora das categorias do Godot)
    if (HasBlockHair(Look.Race))
    {
        const FName* Hair = Lower.Find(TEXT("block_hair"));
        const bool bKnown = Hair && BlockHairs().ContainsByPredicate([Hair, &Look](const FVortexBlockHair& H) { return H.Id == *Hair && H.Fits(Look.Appearance); });
        // padrão: o liso curto no masculino, o rabo de cavalo no feminino
        Look.Pieces.Add(TEXT("block_hair"), bKnown ? *Hair : Look.Appearance == TEXT("female") ? FName(TEXT("block_hair_rabo_cavalo_alto")) : BlockHairs()[0].Id);
        // a cor é a escolhida no menu: a peça "hair" do Godot escolhida de novo acima (a troca de corpo a apaga) trazia a
        // cor padrão dela por cima
        if (const FColor* Chosen = In.Colors.Find(TEXT("hair_color"))) Look.Colors.Add(TEXT("hair_color"), *Chosen);
        // a roupa: a escolhida, se for deste corpo; senão a primeira dele
        const FName* Outfit = Lower.Find(TEXT("block_outfit"));
        const bool bElf = Look.Race == TEXT("elfo");
        const bool bOutfit = Outfit && BlockOutfits().ContainsByPredicate([Outfit, &Look](const FVortexBlockOutfit& O) { return O.Id == *Outfit && O.Fits(Look.Appearance) && O.FitsRace(Look.Race); });
        // o elfo começa com o traje élfico do corpo dele
        const FVortexBlockOutfit* First = BlockOutfits().FindByPredicate([&Look, bElf](const FVortexBlockOutfit& O) { return !O.Body.IsNone() && O.Fits(Look.Appearance) && (bElf ? O.Race == Look.Race : O.FitsRace(Look.Race)); });
        Look.Pieces.Add(TEXT("block_outfit"), bOutfit ? *Outfit : First ? First->Id : FName(TEXT("block_outfit_nenhuma")));
        // o rosto: o escolhido, se existir (VortexFaceStyles); senão o padrão
        const FName* Face = Lower.Find(TEXT("block_face"));
        // 09/11: o elfo escolhe a pele (2) e a orelha (4 tipos); os olhos dependem da pele. "peca_orelha" é a malha que o personagem veste
        FName Pele;
        if (bElf)
        {
            const FName* Tone = Lower.Find(TEXT("elfo_pele"));
            Pele = Tone && *Tone == TEXT("escura") ? FName(TEXT("escura")) : FName(TEXT("clara"));
            Look.Pieces.Add(TEXT("elfo_pele"), Pele);
            const FName* Ear = Lower.Find(TEXT("elfo_orelha"));
            const FName EarId = Ear && (*Ear == TEXT("curta") || *Ear == TEXT("longa") || *Ear == TEXT("tras") || *Ear == TEXT("caida")) ? *Ear : FName(TEXT("longa"));
            Look.Pieces.Add(TEXT("elfo_orelha"), EarId);
            Look.Pieces.Add(TEXT("peca_orelha"), FName(*FString::Printf(TEXT("P_orelha_%s_%s"), *EarId.ToString(), *Pele.ToString())));
        }
        const bool bFace = Face && VortexFaceStyles().ContainsByPredicate([Face, Pele](const FVortexFaceStyle& F) { return F.Id == *Face && F.Pele == Pele; });
        Look.Pieces.Add(TEXT("block_face"), bFace ? *Face : !bElf ? FName(TEXT("padrao")) : Pele == TEXT("escura") ? FName(TEXT("dark_vermelho")) : FName(TEXT("elfo_verde")));
        // as roupas separadas: só as que existem, neste espaço e deste corpo
        for (const FVortexWearSlot& Wear : VortexWearSlots())
            if (const FName* Id = Lower.Find(Wear.Key))
                if (const FVortexWearPiece* P = VortexWearPiece(*Id))
                    if (P->Slot == Wear.Key && P->Body == Look.Appearance && (P->Race.IsNone() || P->Race == Look.Race)) Look.Pieces.Add(Wear.Key, *Id);
    }
    for (const FVortexColorSlot& S : Slots)
    {
        if (!Look.Colors.Contains(S.Key))
        {
            const FVortexRaceInfo* R = Race(Look.Race);
            Look.Colors.Add(S.Key, S.Key == TEXT("skin_color") && R && R->Skin.Num() ? R->Skin[0] : S.Default);
        }
    }
    return Look;
}

const TArray<FVortexBlockHair>& FVortexLookCatalog::BlockHairs()
{
    // Data/Looks/cabelos.json (01/11: a lista e a física saíram do código; mudar não precisa compilar): o estilo de
    // Tools/Blender/MakeStrandHair.py, o nome no menu, o corpo e a física (0 = presa, .5 = leve, 1 = solta; só vale nos
    // cabelos com correntes Hair_*); a ordem é a do menu
    static const TArray<FVortexBlockHair> Hairs = []()
    {
        TArray<FVortexBlockHair> Out;
        for (const TSharedPtr<FJsonObject>& Entry : LookEntries(TEXT("cabelos.json"), TEXT("cabelos")))
        {
            const FString Id = Entry->GetStringField(TEXT("id"));
            const bool bMesh = !Entry->HasField(TEXT("malha")) || Entry->GetBoolField(TEXT("malha"));
            Out.Add({ FName(TEXT("block_hair_") + Id), Entry->GetStringField(TEXT("nome")),
                bMesh ? FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/humano/Hair/Hair_%s.Hair_%s"), *Id, *Id) : FString(),
                float(Entry->GetNumberField(TEXT("fisica"))), FName(*Entry->GetStringField(TEXT("corpo"))) });
        }
        if (Out.IsEmpty()) Out.Add({ TEXT("block_hair_careca"), TEXT("Careca"), FString() });
        return Out;
    }();
    return Hairs;
}

const TArray<FVortexBlockOutfit>& FVortexLookCatalog::BlockOutfits()
{
    // Data/Looks/roupas.json (a lista saiu do código; mudar não precisa compilar): as roupas da ficha
    // (modelando ainda/Ficha de Trajes RPG em Pixel Art.png), o nome no menu e o corpo; a ordem é a do menu
    static const TArray<FVortexBlockOutfit> Outfits = []()
    {
        TArray<FVortexBlockOutfit> Out;
        for (const TSharedPtr<FJsonObject>& Entry : LookEntries(TEXT("roupas.json"), TEXT("roupas")))
        {
            const FString Id = Entry->GetStringField(TEXT("id"));
            const bool bMesh = !Entry->HasField(TEXT("malha")) || Entry->GetBoolField(TEXT("malha"));
            Out.Add({ FName(TEXT("block_outfit_") + Id), Entry->GetStringField(TEXT("nome")),
                bMesh ? FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/humano/Roupas/Roupa_%s.Roupa_%s"), *Id, *Id) : FString(),
                FName(*Entry->GetStringField(TEXT("corpo"))),
                Entry->HasField(TEXT("raca")) ? FName(*Entry->GetStringField(TEXT("raca"))) : NAME_None,
                Entry->HasField(TEXT("tecido")) && Entry->GetBoolField(TEXT("tecido")) });
        }
        if (Out.IsEmpty()) Out.Add({ TEXT("block_outfit_nenhuma"), TEXT("Sem roupa"), FString() });
        return Out;
    }();
    return Outfits;
}

const TArray<FVortexFaceStyle>& VortexFaceStyles()
{
    // Data/Looks/rostos.json (mudar não precisa compilar)
    static const TArray<FVortexFaceStyle> Styles = []()
    {
        TArray<FVortexFaceStyle> Out;
        for (const TSharedPtr<FJsonObject>& Entry : LookEntries(TEXT("rostos.json"), TEXT("rostos")))
            Out.Add({ FName(*Entry->GetStringField(TEXT("id"))), Entry->GetStringField(TEXT("nome")), Entry->HasField(TEXT("pele")) ? FName(*Entry->GetStringField(TEXT("pele"))) : NAME_None });
        if (Out.IsEmpty()) Out.Add({ TEXT("padrao"), TEXT("Padrão") });
        return Out;
    }();
    return Styles;
}

const TArray<FVortexWearSlot>& VortexWearSlots()
{
    // 10/11 (o usuário: "em vez de peça por peça, presets"): sem espaços de roupa separada no menu
    static const TArray<FVortexWearSlot> Slots;
    static const TArray<FVortexWearSlot> Unused = {
        { TEXT("peca_camisa"), TEXT("Camisa") }, { TEXT("peca_cima"), TEXT("Parte de cima") }, { TEXT("peca_calca"), TEXT("Calça") },
        { TEXT("peca_calcado"), TEXT("Calçado") }, { TEXT("peca_acessorio"), TEXT("Rosto e pescoço") },
        { TEXT("peca_cintura"), TEXT("Cintura") }, { TEXT("peca_bracos"), TEXT("Braços") } };
    return Slots;
}

const TArray<FVortexWearPiece>& VortexWearPieces()
{
    static const TArray<FVortexWearPiece> Pieces = []()
    {
        TArray<FVortexWearPiece> Out;
        FString Text;
        TSharedPtr<FJsonObject> Root;
        const TSharedPtr<FJsonObject>* List = nullptr;
        if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Looks/pecas.json"))) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid() || !Root->TryGetObjectField(TEXT("pieces"), List))
        {
            UE_LOG(LogTemp, Warning, TEXT("VORTEX_LOOK sem Data/Looks/pecas.json"));
            return Out;
        }
        for (const auto& Pair : (*List)->Values)   // 10/11: auto (o clang do Linux recusava a cópia temporária do par)
        {
            const TSharedPtr<FJsonObject> P = Pair.Value->AsObject();
            FVortexWearPiece Piece;
            Piece.Id = FName(*Pair.Key);
            Piece.Slot = FName(TEXT("peca_") + P->GetStringField(TEXT("slot")));
            Piece.Body = P->GetStringField(TEXT("audience")) == TEXT("F") ? FName(TEXT("female")) : FName(TEXT("male"));
            Piece.Name = P->GetStringField(TEXT("name"));
            Piece.Mesh = FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/humano/Pecas/%s.%s"), *Pair.Key, *Pair.Key);
            Piece.bHidesHair = P->GetBoolField(TEXT("hidesHair"));
            if (P->HasField(TEXT("race"))) Piece.Race = FName(*P->GetStringField(TEXT("race")));
            Piece.bCloth = P->HasField(TEXT("tecido")) && P->GetBoolField(TEXT("tecido"));
            if (P->HasField(TEXT("hair"))) Piece.HairMode = FName(*P->GetStringField(TEXT("hair")));
            Out.Add(Piece);
        }
        return Out;
    }();
    return Pieces;
}

float VortexWearFranjaZ()
{
    static const float Z = []()
    {
        FString Text;
        TSharedPtr<FJsonObject> Root;
        double Value = 133.0;
        if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Looks/pecas.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
            Root->TryGetNumberField(TEXT("franjaZ"), Value);
        return float(Value);
    }();
    return Z;
}

const FVortexWearPiece* VortexWearPiece(FName Id)
{
    return Id.IsNone() ? nullptr : VortexWearPieces().FindByPredicate([Id](const FVortexWearPiece& P) { return P.Id == Id; });
}

const FVortexBlockOutfit* FVortexLookCatalog::BlockOutfit(const FVortexLook& Look)
{
    if (!HasBlockHair(Look.Race)) return nullptr;
    const FName Id = Look.Pieces.FindRef(TEXT("block_outfit"));
    return BlockOutfits().FindByPredicate([Id](const FVortexBlockOutfit& O) { return O.Id == Id; });
}

const FVortexBlockHair* FVortexLookCatalog::BlockHair(const FVortexLook& Look)
{
    if (!HasBlockHair(Look.Race)) return nullptr;
    const FName Id = Look.Pieces.FindRef(TEXT("block_hair"));
    const FVortexBlockHair* Found = BlockHairs().FindByPredicate([Id](const FVortexBlockHair& H) { return H.Id == Id; });
    return Found ? Found : &BlockHairs()[0];
}

FVortexLook FVortexLookCatalog::Random(FName RaceId, FName Appearance, FRandomStream& Rng, bool bOnlyColors, const FVortexLook* Keep) const
{
    FVortexLook Look;
    Look.Race = RaceId;
    Look.Appearance = Appearance;
    if (bOnlyColors && Keep) Look.Pieces = Keep->Pieces;
    else
    {
        for (const TPair<FName, FString>& Category : Categories)
        {
            const TArray<FName> Options_ = Options(Look, Category.Key);
            if (Options_.IsEmpty()) continue;
            // cosméticos aparecem às vezes (sem virar uma árvore de Natal)
            const bool bCosmetic = Options_[0].ToString().EndsWith(TEXT("_none")) && Category.Key != TEXT("race");
            ChoosePiece(Look, Category.Key, bCosmetic && Rng.FRand() < .55f ? Options_[0] : Options_[Rng.RandHelper(Options_.Num())]);
        }
    }
    for (const FVortexColorSlot& S : Slots)
    {
        const FVortexRaceInfo* R = Race(RaceId);
        const TArray<FColor>& Palette = S.Key == TEXT("skin_color") && R ? R->Skin : S.Palette;
        if (Palette.Num()) Look.Colors.Add(S.Key, Palette[Rng.RandHelper(Palette.Num())]);
    }
    // mangas iguais à camisa na maior parte das vezes
    if (Rng.FRand() < .7f && Look.Colors.Contains(TEXT("shirt_color"))) Look.Colors.Add(TEXT("sleeve_color"), Look.Colors[TEXT("shirt_color")]);
    return Normalized(Look);
}

FString FVortexLookCatalog::RandomName(FName RaceId, FRandomStream& Rng) const
{
    const FVortexRaceInfo* R = Race(RaceId);
    if (!R || R->NameStart.IsEmpty() || R->NameEnd.IsEmpty()) return TEXT("Aventureiro");
    FString Name = R->NameStart[Rng.RandHelper(R->NameStart.Num())] + R->NameEnd[Rng.RandHelper(R->NameEnd.Num())];
    if (Rng.FRand() < .25f) Name += R->NameEnd[Rng.RandHelper(R->NameEnd.Num())];
    return Name.Left(1).ToUpper() + Name.Mid(1).ToLower();
}

UTexture2D* FVortexLookCatalog::BuildPalette(const FVortexLook& Look, UObject* Outer) const
{
    if (BasePixels.IsEmpty()) return nullptr;
    TArray<FColor> Pixels = BasePixels;
    for (const FEntry& Entry : Entries)
    {
        const FColor* Chosen = Look.Colors.Find(Entry.Slot);
        if (!Chosen) continue;
        const int32 X0 = (Entry.Index % Columns) * BlockPx, Y0 = (Entry.Index / Columns) * BlockPx;
        for (int32 Y = Y0; Y < Y0 + BlockPx && Y < PaletteHeight; ++Y)
        for (int32 X = X0; X < X0 + BlockPx && X < PaletteWidth; ++X)
        {
            FColor& Pixel = Pixels[Y * PaletteWidth + X];
            auto Channel = [](uint8 Pick, uint8 Tone, uint8 Base)
            {
                return uint8(FMath::Clamp(FMath::RoundToInt(Pick * (float(Tone) / FMath::Max<float>(Base, 1.f))), 0, 255));
            };
            Pixel = FColor(Channel(Chosen->R, Pixel.R, Entry.Base.R), Channel(Chosen->G, Pixel.G, Entry.Base.G),
                Channel(Chosen->B, Pixel.B, Entry.Base.B), Pixel.A);
        }
    }
    UTexture2D* Texture = UTexture2D::CreateTransient(PaletteWidth, PaletteHeight, PF_B8G8R8A8, NAME_None);
    if (!Texture) return nullptr;
    Texture->SRGB = true;
    Texture->Filter = TF_Nearest;
    Texture->AddressX = Texture->AddressY = TA_Clamp;
    FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
    FMemory::Memcpy(Mip.BulkData.Lock(LOCK_READ_WRITE), Pixels.GetData(), Pixels.Num() * sizeof(FColor));
    Mip.BulkData.Unlock();
    Texture->UpdateResource();
    return Texture;
}
