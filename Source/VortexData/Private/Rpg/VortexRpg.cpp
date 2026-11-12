#include "Rpg/VortexRpg.h"

#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace VortexRpg
{
    static const TCHAR* EquipKeys[NumEquip] = { TEXT("weapon"), TEXT("head"), TEXT("chest"), TEXT("legs"), TEXT("boots"), TEXT("accessory") };
    static const TCHAR* EquipLabels[NumEquip] = { TEXT("Arma"), TEXT("Cabeça"), TEXT("Peito"), TEXT("Pernas"), TEXT("Botas"), TEXT("Joia") };

    FName EquipKey(int32 Slot) { return Slot >= 0 && Slot < NumEquip ? FName(EquipKeys[Slot]) : NAME_None; }
    FString EquipLabel(int32 Slot) { return Slot >= 0 && Slot < NumEquip ? EquipLabels[Slot] : TEXT(""); }
    int32 EquipSlotOf(FName ItemSlot)
    {
        for (int32 i = 0; i < NumEquip; ++i)
        {
            if (ItemSlot == EquipKeys[i]) return i;
        }
        return INDEX_NONE;
    }

    // Os 12 oficiais do Compêndio v1.2 (ProgressionSystem.Official no Godot), na mesma ordem
    static const TCHAR* OfficialKeys[NumOfficial] = { TEXT("attack"), TEXT("defense"), TEXT("resistance"), TEXT("max_health"),
        TEXT("max_mana"), TEXT("crit"), TEXT("crit_dmg"), TEXT("speed"), TEXT("haste"), TEXT("skill"), TEXT("lifesteal"), TEXT("hp_regen") };
    static const TCHAR* OfficialLabels[NumOfficial] = { TEXT("Ataque"), TEXT("Defesa"), TEXT("Resist. mágica"), TEXT("Vida máx."),
        TEXT("Mana máx."), TEXT("% Crítico"), TEXT("% Dano crítico"), TEXT("% Velocidade de ataque"), TEXT("% Recarga"),
        TEXT("Poder de magia"), TEXT("% Roubo de vida"), TEXT("Regeneração") };

    FName OfficialKey(int32 Official) { return Official >= 0 && Official < NumOfficial ? FName(OfficialKeys[Official]) : NAME_None; }

    // 10/11: os 8 atributos de pontos (VortexRpg.h, EPointGroup) e os oficiais de cada um
    static const int32 GroupOfOfficial[NumOfficial] = { GStrength, GDefense, GMagicResist, GVitality, GMagic, GPrecision, GPrecision,
        GAgility, GAgility, GMagic, GVampirism, GVitality };
    static const TCHAR* GroupLabels[NumGroups] = { TEXT("Força"), TEXT("Magia"), TEXT("Vitalidade"), TEXT("Defesa"),
        TEXT("Resistência Mágica"), TEXT("Precisão"), TEXT("Agilidade"), TEXT("Vampirismo") };
    static const TCHAR* GroupHelps[NumGroups] = {
        TEXT("Mais dano com espadas, machados, lanças, arcos e outras armas."),
        TEXT("Mais dano das magias, cajados, varinhas e orbes, e mais mana."),
        TEXT("Mais vida máxima, e a vida volta mais rápido sozinha."),
        TEXT("Corta o dano dos golpes físicos (garras, espadas, flechas). Nunca deixa imune."),
        TEXT("Corta o dano mágico (veneno, cristais, runas, fogo dos chefes). Nunca deixa imune."),
        TEXT("Mais chance de crítico, e o crítico bate mais forte."),
        TEXT("Golpes mais rápidos e técnicas e especial com recarga mais curta."),
        TEXT("Parte do dano que você causa volta como vida (até 25%).") };
    int32 GroupOf(int32 Official) { return Official >= 0 && Official < NumOfficial ? GroupOfOfficial[Official] : INDEX_NONE; }
    FString GroupLabel(int32 Group) { return Group >= 0 && Group < NumGroups ? GroupLabels[Group] : TEXT(""); }
    FString GroupHelp(int32 Group) { return Group >= 0 && Group < NumGroups ? GroupHelps[Group] : TEXT(""); }
    TArray<int32> GroupStats(int32 Group)
    {
        // o principal primeiro (o número grande na tela)
        static const int32 Primary[NumGroups] = { Attack, Skill, MaxHealthStat, Defense, Resistance, Crit, Speed, Lifesteal };
        TArray<int32> Out;
        if (Group < 0 || Group >= NumGroups) return Out;
        Out.Add(Primary[Group]);
        for (int32 i = 0; i < NumOfficial; ++i) if (GroupOfOfficial[i] == Group && i != Primary[Group]) Out.Add(i);
        return Out;
    }
    FString OfficialLabel(int32 Official) { return Official >= 0 && Official < NumOfficial ? OfficialLabels[Official] : TEXT(""); }
    int32 OfficialOf(FName Key)
    {
        for (int32 i = 0; i < NumOfficial; ++i)
        {
            if (Key == OfficialKeys[i]) return i;
        }
        return INDEX_NONE;
    }
    FString OfficialHelp(int32 Official)
    {
        switch (Official)
        {
        case Attack: return TEXT("Soma ao dano de todo golpe de arma.");
        case Defense: return TEXT("Divide o dano dos golpes físicos (quanto mais, menos passa; nunca deixa imune).");
        case Resistance: return TEXT("Divide o dano mágico: veneno, cristais, runas (nunca deixa imune).");
        case MaxHealthStat: return TEXT("Mais vida máxima.");
        case MaxManaStat: return TEXT("Mais mana máxima.");
        case Crit: return TEXT("Mais chance de acerto crítico.");
        case CritDmg: return TEXT("O crítico bate ainda mais forte.");
        case Speed: return TEXT("Golpes e animações de ataque mais rápidos (até +100%).");
        case Haste: return TEXT("Recargas das técnicas e do especial mais curtas.");
        case Skill: return TEXT("Mais dano das magias, cajados, varinhas e orbes.");
        case Lifesteal: return TEXT("Parte do dano que você causa volta como vida (até 25%).");
        case HpRegenStat: return TEXT("A vida volta sozinha mais rápido (décimos de vida por segundo).");
        default: return TEXT("");
        }
    }
}

void FVortexRpgCatalog::LoadBalance()
{
    // o balanceamento dos pontos e das criaturas (02/11), à mão: o rpg.json sai do Godot (Tools/BuildRpgData.py)
    FString Text;
    TSharedPtr<FJsonObject> Root;
    if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Rpg/balance.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root) return;
    auto Number = [](const TSharedPtr<FJsonObject>& O, const TCHAR* Key, float& Out) { double V = 0.0; if (O->TryGetNumberField(Key, V)) Out = float(V); };
    Number(Root, TEXT("levelDamage"), LevelDamage);
    // 07/11: a escala do golpe de cada tipo de arma e os números do nível do item
    const TSharedPtr<FJsonObject>* Scales = nullptr;
    if (Root->TryGetObjectField(TEXT("weaponDamageScale"), Scales))
        for (const auto& P : (*Scales)->Values)
            if (!FString(*P.Key).StartsWith(TEXT("_"))) WeaponDamageScale.Add(FName(*P.Key), float(P.Value->AsNumber()));
    const TSharedPtr<FJsonObject>* Levels = nullptr;
    if (Root->TryGetObjectField(TEXT("itemLevel"), Levels))
    {
        auto List = [Levels](const TCHAR* Key, TArray<float>& Out)
        {
            const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
            if ((*Levels)->TryGetArrayField(Key, A) && A->Num() == 7) { Out.Reset(); for (const TSharedPtr<FJsonValue>& V : *A) Out.Add(float(V->AsNumber())); }
        };
        List(TEXT("natural"), ItemNatural);
        List(TEXT("attackBase"), ItemAttackBase);
        List(TEXT("attackPerLevel"), ItemAttackPerLevel);
        List(TEXT("attackDefault"), ItemAttackDefault);
        Number(*Levels, TEXT("growthPerLevel"), ItemGrowthPerLevel);
        Number(*Levels, TEXT("pricePerLevel"), ItemPricePerLevel);
        double Max = 0.0;
        if ((*Levels)->TryGetNumberField(TEXT("max"), Max)) ItemMaxLevel = FMath::Clamp(int32(Max), 1, 999);
    }
    Number(Root, TEXT("defenseK"), DefenseK);
    Number(Root, TEXT("defenseKPerLevel"), DefenseKPerLevel);
    Number(Root, TEXT("monsterHitBase"), MonsterHitBase);
    Number(Root, TEXT("monsterHitPerLevel"), MonsterHitPerLevel);
    Number(Root, TEXT("monsterDamagePerLevel"), MonsterDamagePerLevel);
    Number(Root, TEXT("groupRange"), GroupRange);
    Number(Root, TEXT("groupXpBonus"), GroupXpBonus);
    Number(Root, TEXT("groupHealthPerHero"), GroupHealthPerHero);
    Number(Root, TEXT("pvpDamage"), PvpDamage);
    const TSharedPtr<FJsonObject>* Points = nullptr;
    if (!Root->TryGetObjectField(TEXT("points"), Points)) return;
    for (int32 i = 0; i < VortexRpg::NumOfficial; ++i)
    {
        const TSharedPtr<FJsonObject>* Curve = nullptr;
        if (!(*Points)->TryGetObjectField(VortexRpg::OfficialKey(i).ToString(), Curve)) continue;
        FPointCurve& C = PointCurve[i];
        C.bOn = true;
        Number(*Curve, TEXT("max"), C.Max);
        Number(*Curve, TEXT("maxPerLevel"), C.MaxPerLevel);
        Number(*Curve, TEXT("mid"), C.Mid);
    }
}

float FVortexRpgCatalog::PointValue(int32 Official, int32 Points, int32 Level) const
{
    if (Official < 0 || Official >= VortexRpg::NumOfficial || Points <= 0) return 0.f;
    const FPointCurve& C = PointCurve[Official];
    if (!C.bOn) return Points * PerPoint[Official];
    const float L = float(FMath::Max(1, Level));
    return (C.Max + C.MaxPerLevel * L) * Points / (Points + FMath::Max(.1f, C.Mid) * L);
}

void FVortexRpgCatalog::LoadArsenal()
{
    // o arsenal por cima dos itens do Godot: variantes com elemento, preços, conjuntos de armadura
    FString Text;
    TSharedPtr<FJsonObject> Root;
    if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Rpg/arsenal.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root) return;
    auto Fill = [](FVortexItemDef& Def, const TSharedPtr<FJsonObject>& O)
    {
        FString S;
        double N = 0.0;
        if (O->TryGetStringField(TEXT("name"), S)) Def.Name = S;
        if (O->TryGetStringField(TEXT("description"), S)) Def.Description = S;
        if (O->TryGetStringField(TEXT("rarity"), S)) Def.Rarity = S;
        if (O->TryGetStringField(TEXT("type"), S)) Def.Type = S;
        if (O->TryGetStringField(TEXT("family"), S)) Def.Family = S;
        if (O->TryGetStringField(TEXT("slot"), S)) Def.Slot = FName(*S);
        if (O->TryGetStringField(TEXT("icon"), S)) Def.Icon = FName(*S);
        if (O->TryGetNumberField(TEXT("stack"), N)) Def.Stack = FMath::Max(1, int32(N));
        if (O->TryGetStringField(TEXT("element"), S)) Def.Element = S.IsEmpty() ? NAME_None : FName(*S);
        if (O->TryGetStringField(TEXT("trait"), S)) Def.Trait = S.IsEmpty() ? NAME_None : FName(*S);
        if (O->TryGetStringField(TEXT("effect_text"), S)) Def.EffectText = S;
        if (O->TryGetStringField(TEXT("set"), S)) Def.Set = FName(*S);
        if (O->TryGetNumberField(TEXT("price"), N)) Def.Price = int32(N);
        const TSharedPtr<FJsonObject>* Obj = nullptr;
        if (O->TryGetObjectField(TEXT("stats"), Obj))
        {
            Def.Stats.Reset();
            for (const TPair<FString, TSharedPtr<FJsonValue>> Stat : (*Obj)->Values) Def.Stats.Add(FName(*Stat.Key), float(Stat.Value->AsNumber()));
        }
        if (O->TryGetObjectField(TEXT("use"), Obj))
        {
            (*Obj)->TryGetNumberField(TEXT("heal"), Def.Heal);
            (*Obj)->TryGetNumberField(TEXT("mana"), Def.Mana);
            (*Obj)->TryGetNumberField(TEXT("reveal_map"), Def.RevealMap);
        }
        if (O->TryGetObjectField(TEXT("look"), Obj))
            for (const TPair<FString, TSharedPtr<FJsonValue>> P : (*Obj)->Values) Def.Look.Add(FName(*P.Key), FName(*P.Value->AsString()));
        if (O->TryGetObjectField(TEXT("look_colors"), Obj))
            for (const TPair<FString, TSharedPtr<FJsonValue>> P : (*Obj)->Values) Def.LookColors.Add(FName(*P.Key), FColor::FromHex(P.Value->AsString()));
    };
    for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Root->GetObjectField(TEXT("patch"))->Values)
        if (FVortexItemDef* Def = Items.Find(FName(*Pair.Key))) Fill(*Def, Pair.Value->AsObject());
    for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Root->GetObjectField(TEXT("items"))->Values)
    {
        FVortexItemDef Def;
        Def.Id = FName(*Pair.Key);
        Fill(Def, Pair.Value->AsObject());
        Items.Add(Def.Id, Def);
    }
    for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Root->GetObjectField(TEXT("sets"))->Values)
    {
        const TSharedPtr<FJsonObject> O = Pair.Value->AsObject();
        FVortexItemSet Set;
        Set.Id = FName(*Pair.Key);
        Set.Name = O->GetStringField(TEXT("name"));
        Set.Text = O->GetStringField(TEXT("text"));
        for (const TSharedPtr<FJsonValue>& V : O->GetArrayField(TEXT("pieces"))) Set.Pieces.Add(FName(*V->AsString()));
        for (const TPair<FString, TSharedPtr<FJsonValue>> B : O->GetObjectField(TEXT("bonus2"))->Values) Set.Bonus2.Add(FName(*B.Key), float(B.Value->AsNumber()));
        for (const TPair<FString, TSharedPtr<FJsonValue>> B : O->GetObjectField(TEXT("bonus4"))->Values) Set.Bonus4.Add(FName(*B.Key), float(B.Value->AsNumber()));
        Sets.Add(Set.Id, Set);
    }
    for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Root->GetObjectField(TEXT("elements"))->Values)
        ElementNames.Add(FName(*Pair.Key), Pair.Value->AsObject()->GetStringField(TEXT("name")));
    if (const TSharedPtr<FJsonObject>* Traits = nullptr; Root->TryGetObjectField(TEXT("traits"), Traits))
        for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : (*Traits)->Values)
            TraitNames.Add(FName(*Pair.Key), Pair.Value->AsObject()->GetStringField(TEXT("name")));
    // 07/11: saques a mais (as espadas divinas no Colosso de Runas e no Golem), somados aos do rpg.json
    if (const TSharedPtr<FJsonObject>* Extra = nullptr; Root->TryGetObjectField(TEXT("loot"), Extra))
        for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : (*Extra)->Values)
            for (const TSharedPtr<FJsonValue>& Value : Pair.Value->AsArray())
            {
                const TSharedPtr<FJsonObject> O = Value->AsObject();
                Loot.FindOrAdd(FName(*Pair.Key)).Add({ FName(*O->GetStringField(TEXT("item"))), int32(O->GetNumberField(TEXT("min"))),
                    int32(O->GetNumberField(TEXT("max"))), float(O->GetNumberField(TEXT("chance"))) });
            }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_ARSENAL itens=%d conjuntos=%d"), Items.Num(), Sets.Num());
}

int32 FVortexRpgCatalog::NaturalLevel(const FString& Rarity) const
{
    static const TCHAR* const Ranks[] = { TEXT("common"), TEXT("uncommon"), TEXT("rare"), TEXT("epic"), TEXT("legendary"), TEXT("unique"), TEXT("divine") };
    for (int32 i = 0; i < UE_ARRAY_COUNT(Ranks); ++i)
        if (Rarity == Ranks[i] && ItemNatural.IsValidIndex(i)) return FMath::Max(1, int32(ItemNatural[i]));
    return 1;
}

void FVortexRpgCatalog::SetItemLevel(FVortexItemDef& Def, int32 Level) const
{
    // como nos jogos de saque (Diablo, WoW): o Ataque cresce em linha com o nível do item e a raridade multiplica quanto
    // ele cresce; os de porcentagem (crítico, velocidade, roubo de vida) não crescem; a defesa, a vida e a mana crescem
    // 10% por nível. Def.Level ainda é o nível dos números atuais (de onde a conta parte)
    static const TCHAR* const Ranks[] = { TEXT("common"), TEXT("uncommon"), TEXT("rare"), TEXT("epic"), TEXT("legendary"), TEXT("unique"), TEXT("divine") };
    int32 Rank = 0;
    for (int32 i = 0; i < UE_ARRAY_COUNT(Ranks); ++i) if (Def.Rarity == Ranks[i]) Rank = i;
    const int32 From = FMath::Max(1, Def.Level);
    Level = FMath::Clamp(Level, 1, ItemMaxLevel);
    const float Grow = (1.f + ItemGrowthPerLevel * Level) / (1.f + ItemGrowthPerLevel * From);
    for (TPair<FName, float>& Stat : Def.Stats)
    {
        if (Stat.Key == TEXT("attack") || Stat.Key == TEXT("skill"))
            Stat.Value = FMath::RoundToFloat((ItemAttackBase[Rank] + ItemAttackPerLevel[Rank] * Level) * Def.AttackFactor);
        else if (Stat.Key == TEXT("defense") || Stat.Key == TEXT("resistance") || Stat.Key == TEXT("max_health") || Stat.Key == TEXT("max_mana") || Stat.Key == TEXT("hp_regen"))
            Stat.Value = FMath::RoundToFloat(Stat.Value * Grow);
    }
    Def.Price = FMath::RoundToInt(Def.Price * (1.f + ItemPricePerLevel * Level) / (1.f + ItemPricePerLevel * From));
    Def.Level = Level;
    Def.ReqLevel = Level;
}

const FVortexItemDef* FVortexRpgCatalog::Item(FName Id) const
{
    if (const FVortexItemDef* Found = Items.Find(Id)) return Found;
    if (const TSharedPtr<FVortexItemDef>* Made = Levelled.Find(Id)) return Made->Get();
    // "<id>@<nível>": o mesmo equipamento em outro nível (montado uma vez e guardado)
    FString Base, LevelText;
    if (!Id.ToString().Split(TEXT("@"), &Base, &LevelText)) return nullptr;
    const FVortexItemDef* Source = Items.Find(FName(*Base));
    const int32 Level = FCString::Atoi(*LevelText);
    if (!Source || !Source->IsEquipment() || Level < 1 || Level > ItemMaxLevel) return nullptr;
    TSharedPtr<FVortexItemDef> Def = MakeShared<FVortexItemDef>(*Source);
    SetItemLevel(*Def, Level);
    Def->Id = Id;
    return Levelled.Add(Id, Def).Get();
}

FName FVortexRpgCatalog::LevelledId(FName Base, int32 Level)
{
    return FName(*FString::Printf(TEXT("%s@%d"), *BaseOf(Base).ToString(), FMath::Clamp(Level, 1, 100)));
}

FName FVortexRpgCatalog::BaseOf(FName Id)
{
    FString Base;
    return Id.ToString().Split(TEXT("@"), &Base, nullptr) ? FName(*Base) : Id;
}

const FVortexRpgCatalog& FVortexRpgCatalog::Get()
{
    static FVortexRpgCatalog Instance;
    return Instance;
}

FVortexRpgCatalog::FVortexRpgCatalog()
{
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Rpg/rpg.json"))))
    {
        UE_LOG(LogTemp, Error, TEXT("VORTEX_RPG sem Data/Rpg/rpg.json"));
        return;
    }
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root) return;

    for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Root->GetObjectField(TEXT("items"))->Values)
    {
        const TSharedPtr<FJsonObject> O = Pair.Value->AsObject();
        FVortexItemDef Def;
        Def.Id = FName(*Pair.Key);
        Def.Name = O->GetStringField(TEXT("name"));
        Def.Description = O->GetStringField(TEXT("description"));
        Def.Rarity = O->GetStringField(TEXT("rarity"));
        Def.Type = O->GetStringField(TEXT("type"));
        Def.Family = O->GetStringField(TEXT("family"));
        Def.Slot = FName(*O->GetStringField(TEXT("slot")));
        Def.Icon = FName(*O->GetStringField(TEXT("icon")));
        Def.Stack = FMath::Max(1, int32(O->GetNumberField(TEXT("stack"))));
        for (const TPair<FString, TSharedPtr<FJsonValue>> Stat : O->GetObjectField(TEXT("stats"))->Values)
        {
            Def.Stats.Add(FName(*Stat.Key), float(Stat.Value->AsNumber()));
        }
        const TSharedPtr<FJsonObject> Use = O->GetObjectField(TEXT("use"));
        Use->TryGetNumberField(TEXT("heal"), Def.Heal);
        Use->TryGetNumberField(TEXT("mana"), Def.Mana);
        Use->TryGetNumberField(TEXT("reveal_map"), Def.RevealMap);
        O->TryGetNumberField(TEXT("price"), Def.Price);
        Items.Add(Def.Id, Def);
    }
    LoadArsenal();
    LoadBalance();
    // 07/11: cada equipamento no nível natural da raridade, com os números pela mesma conta dos outros níveis
    for (TPair<FName, FVortexItemDef>& It : Items)
    {
        FVortexItemDef& Def = It.Value;
        Def.BaseId = Def.Id;
        if (!Def.IsEquipment()) continue;
        static const TCHAR* const Ranks[] = { TEXT("common"), TEXT("uncommon"), TEXT("rare"), TEXT("epic"), TEXT("legendary"), TEXT("unique"), TEXT("divine") };
        int32 Rank = 0;
        for (int32 i = 0; i < UE_ARRAY_COUNT(Ranks); ++i) if (Def.Rarity == Ranks[i]) Rank = i;
        const float* Skill = Def.Stats.Find(TEXT("skill"));
        const float* Attack = Def.Stats.Find(TEXT("attack"));
        const float Authored = Skill ? *Skill : Attack ? *Attack : 0.f;
        if (Skill) Def.Stats.Remove(TEXT("attack"));   // a magia usa o Poder de magia: o Ataque do cajado não fazia nada
        Def.AttackFactor = ItemAttackDefault.IsValidIndex(Rank) && ItemAttackDefault[Rank] > 0.f ? Authored / ItemAttackDefault[Rank] : 1.f;
        // 07/11 (a economia): sem preço escrito, o da raridade (o mesmo da loja), para crescer com o nível como os outros
        static const int32 GearPrice[] = { 40, 110, 300, 700, 1600, 3000, 8000 };
        if (Def.Price <= 0) Def.Price = GearPrice[Rank] * (Def.Slot == TEXT("accessory") ? 6 : 5) / 5;
        const int32 Natural = NaturalLevel(Def.Rarity);
        Def.Level = Natural;   // a referência dos números escritos (o SetItemLevel cresce a partir dela)
        SetItemLevel(Def, Natural);
    }
    for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Root->GetObjectField(TEXT("loot"))->Values)
    {
        TArray<FVortexLootEntry>& Entries = Loot.FindOrAdd(FName(*Pair.Key));   // (o arsenal já pode ter posto: as espadas divinas)
        for (const TSharedPtr<FJsonValue>& Value : Pair.Value->AsArray())
        {
            const TSharedPtr<FJsonObject> O = Value->AsObject();
            Entries.Add({ FName(*O->GetStringField(TEXT("item"))), int32(O->GetNumberField(TEXT("min"))),
                int32(O->GetNumberField(TEXT("max"))), float(O->GetNumberField(TEXT("chance"))) });
        }
    }
    for (const TSharedPtr<FJsonValue>& Value : Root->GetObjectField(TEXT("starting"))->GetArrayField(TEXT("bag")))
    {
        const TSharedPtr<FJsonObject> O = Value->AsObject();
        StartingBag.Add({ FName(*O->GetStringField(TEXT("item"))), int32(O->GetNumberField(TEXT("count"))) });
    }
    for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Root->GetObjectField(TEXT("xpSources"))->Values)
    {
        XpSources.Add(FName(*Pair.Key), int32(Pair.Value->AsNumber()));
    }
    for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Root->GetObjectField(TEXT("effects"))->Values)
    {
        Effects.Add(FName(*Pair.Key), float(Pair.Value->AsNumber()));
    }
    const TSharedPtr<FJsonObject>* PerPointJson = nullptr;
    if (Root->TryGetObjectField(TEXT("officialPerPoint"), PerPointJson))
    {
        for (int32 i = 0; i < VortexRpg::NumOfficial; ++i)
        {
            double Value = 0.0;
            if ((*PerPointJson)->TryGetNumberField(VortexRpg::OfficialKey(i).ToString(), Value)) PerPoint[i] = float(Value);
        }
    }
    const TSharedPtr<FJsonObject>* BaseJson = nullptr;
    if (Root->TryGetObjectField(TEXT("attributeBase"), BaseJson))
    {
        static const TCHAR* Keys[VortexRpg::NumAttributes] = { TEXT("strength"), TEXT("agility"), TEXT("vitality"), TEXT("intelligence"),
            TEXT("mana"), TEXT("spirit"), TEXT("recovery"), TEXT("luck") };
        for (int32 i = 0; i < VortexRpg::NumAttributes; ++i)
        {
            int32 Value = 0;
            if ((*BaseJson)->TryGetNumberField(Keys[i], Value)) BaseAttr[i] = Value;
        }
    }
    const TSharedPtr<FJsonObject>* ClassesJson = nullptr;
    if (Root->TryGetObjectField(TEXT("classes"), ClassesJson))
    {
        (*ClassesJson)->TryGetNumberField(TEXT("awakeningLevel"), AwakeningLevel);
        (*ClassesJson)->TryGetNumberField(TEXT("ascensionLevel"), AscensionLevel);
        for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : (*ClassesJson)->GetObjectField(TEXT("rarities"))->Values)
        {
            ClassRarityChance.Add(Pair.Key, int32(Pair.Value->AsNumber()));
        }
        for (const TSharedPtr<FJsonValue>& Value : (*ClassesJson)->GetArrayField(TEXT("list")))
        {
            const TSharedPtr<FJsonObject> O = Value->AsObject();
            FVortexClassDef C;
            C.Id = FName(*O->GetStringField(TEXT("id")));
            C.Name = O->GetStringField(TEXT("name"));
            C.Rarity = O->GetStringField(TEXT("rarity"));
            O->TryGetStringField(TEXT("role"), C.Role);
            O->TryGetStringField(TEXT("attribute"), C.Attribute);
            O->TryGetStringField(TEXT("description"), C.Description);
            O->TryGetBoolField(TEXT("drawable"), C.bDrawable);
            FString IconName;
            if (O->TryGetStringField(TEXT("icon"), IconName) && !IconName.IsEmpty()) C.Icon = FName(*IconName);
            for (const TSharedPtr<FJsonValue>& W : O->GetArrayField(TEXT("weapons"))) C.Weapons.Add(FName(*W->AsString()));
            for (const TPair<FString, TSharedPtr<FJsonValue>> B : O->GetObjectField(TEXT("buffs"))->Values) C.Buffs.Add(FName(*B.Key), float(B.Value->AsNumber()));
            for (const TPair<FString, TSharedPtr<FJsonValue>> A : O->GetObjectField(TEXT("affinity"))->Values) C.Affinity.Add(FName(*A.Key), float(A.Value->AsNumber()));
            const TSharedPtr<FJsonObject>* Passive = nullptr;
            if (O->TryGetObjectField(TEXT("passive"), Passive))
            {
                (*Passive)->TryGetStringField(TEXT("name"), C.PassiveName);
                (*Passive)->TryGetStringField(TEXT("text"), C.PassiveText);
            }
            for (const TSharedPtr<FJsonValue>& A : O->GetArrayField(TEXT("ascensions")))
            {
                const TSharedPtr<FJsonObject> J = A->AsObject();
                C.Ascensions.Add({ J->GetStringField(TEXT("name")), J->GetStringField(TEXT("description")) });
            }
            Classes.Add(MoveTemp(C));
        }
    }
    const TSharedPtr<FJsonObject>* SkillsJson = nullptr;
    if (Root->TryGetObjectField(TEXT("classSkills"), SkillsJson))
    {
        const TSharedPtr<FJsonObject>* PowerJson = nullptr;
        if ((*SkillsJson)->TryGetObjectField(TEXT("rarityPower"), PowerJson))
            for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : (*PowerJson)->Values) RarityPower.Add(Pair.Key, float(Pair.Value->AsNumber()));
        for (const TPair<FString, TSharedPtr<FJsonValue>> Owner : (*SkillsJson)->GetObjectField(TEXT("classes"))->Values)
        {
            TArray<FVortexClassSkill>& List = ClassSkills.Add(FName(*Owner.Key));
            int32 SlotIndex = 0;
            for (const TSharedPtr<FJsonValue>& Value : Owner.Value->AsArray())
            {
                const TSharedPtr<FJsonObject> O = Value->AsObject();
                FVortexClassSkill S;
                S.Owner = FName(*Owner.Key);
                S.Slot = SlotIndex++;
                S.Id = FName(*O->GetStringField(TEXT("id")));
                S.Name = O->GetStringField(TEXT("name"));
                O->TryGetStringField(TEXT("description"), S.Description);
                S.Kind = FName(*O->GetStringField(TEXT("kind")));
                FString Field;
                if (O->TryGetStringField(TEXT("element"), Field)) S.Element = FName(*Field);
                if (O->TryGetStringField(TEXT("icon"), Field)) S.Icon = FName(*Field);
                if (O->TryGetStringField(TEXT("anim"), Field)) S.Anim = FName(*Field);
                auto Num = [&O](const TCHAR* Key, float Fallback) { double V = Fallback; O->TryGetNumberField(Key, V); return float(V); };
                auto Pair = [&O](const TCHAR* Key, float& A, float& B)
                {
                    const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
                    if (O->TryGetArrayField(Key, Arr) && Arr->Num() >= 2) { A = float((*Arr)[0]->AsNumber()); B = float((*Arr)[1]->AsNumber()); }
                };
                const FString K = S.Kind.ToString();
                // os padrões de cada tipo (ClassSkillCatalog.Parse do Godot), para o JSON só dizer o que foge do comum
                // bênção, cura e escudo sem raio valem só para o herói (com raio, para o grupo perto)
                S.Radius = Num(TEXT("radius"), K == TEXT("buff") || K == TEXT("heal") || K == TEXT("shield") ? 0.f : K == TEXT("cleave") ? 3.2f : K == TEXT("nova") ? 4.f : K == TEXT("zone") ? 3.f : K == TEXT("strike") ? 2.2f
                    : K == TEXT("summon") ? 4.f : K == TEXT("beam") ? .9f : K == TEXT("wave") ? 1.f : 1.2f);
                S.Range = Num(TEXT("range"), K == TEXT("projectile") ? 13.f : K == TEXT("wave") ? 7.f : K == TEXT("strike") ? 6.f : K == TEXT("zone") ? 4.f
                    : K == TEXT("dash") ? 6.f : K == TEXT("chain") ? 10.f : K == TEXT("beam") ? 10.f : 0.f);
                S.Duration = Num(TEXT("duration"), K == TEXT("zone") ? 4.f : K == TEXT("buff") || K == TEXT("shield") || K == TEXT("summon") ? 8.f : 0.f);
                S.Mana = int32(Num(TEXT("mana"), 15.f));
                S.Cooldown = Num(TEXT("cooldown"), 6.f);
                S.Power = Num(TEXT("power"), 0.f);
                S.Delay = Num(TEXT("delay"), K == TEXT("strike") ? .6f : 0.f);
                S.Stun = Num(TEXT("stun"), 0.f);
                S.Speed = Num(TEXT("speed"), K == TEXT("wave") ? 11.f : 17.f);
                S.Count = int32(Num(TEXT("count"), K == TEXT("summon") ? 3.f : K == TEXT("chain") ? 4.f : 1.f));
                S.Spread = Num(TEXT("spread"), 0.f);
                S.Explode = Num(TEXT("explode"), 0.f);
                O->TryGetBoolField(TEXT("pierce"), S.bPierce);
                O->TryGetBoolField(TEXT("lob"), S.bLob);
                O->TryGetBoolField(TEXT("follow"), S.bFollow);
                S.Pulses = int32(Num(TEXT("pulses"), 1.f));
                S.Heal = Num(TEXT("heal"), 0.f);
                S.Shield = Num(TEXT("shield"), 0.f);
                S.Lifesteal = Num(TEXT("lifesteal"), 0.f);
                S.Land = Num(TEXT("land"), 0.f);
                const TSharedPtr<FJsonObject>* Buff = nullptr;
                if (O->TryGetObjectField(TEXT("buff"), Buff))
                {
                    double V = 0.0;
                    if ((*Buff)->TryGetNumberField(TEXT("damage"), V)) S.BuffDamage = float(V);
                    if ((*Buff)->TryGetNumberField(TEXT("speed"), V)) S.BuffSpeed = float(V);
                    if ((*Buff)->TryGetNumberField(TEXT("armor"), V)) S.BuffArmor = float(V);
                    if ((*Buff)->TryGetNumberField(TEXT("regen"), V)) S.BuffRegen = float(V);
                }
                S.BuffDuration = Num(TEXT("buff_duration"), S.Duration);
                Pair(TEXT("slow"), S.SlowFactor, S.SlowTime);
                Pair(TEXT("weaken"), S.WeakenFactor, S.WeakenTime);
                Pair(TEXT("expose"), S.ExposeFactor, S.ExposeTime);
                Pair(TEXT("dot"), S.DotDps, S.DotTime);
                S.Knock = Num(TEXT("knock"), 0.f);
                S.Taunt = Num(TEXT("taunt"), 0.f);
                S.Execute = Num(TEXT("execute"), 0.f);
                S.Backstab = Num(TEXT("backstab"), 0.f);
                S.Haste = Num(TEXT("haste"), 0.f) / 100.f;   // no JSON em % (como o Godot): 30 = recargas 30% mais rápidas
                S.CritBonus = Num(TEXT("crit_bonus"), 0.f);
                S.ManaGain = Num(TEXT("mana_gain"), 0.f);
                S.Deathward = Num(TEXT("deathward"), 0.f);
                List.Add(MoveTemp(S));
            }
        }
    }
    const TArray<TSharedPtr<FJsonValue>>* RecipesJson = nullptr;
    if (Root->TryGetArrayField(TEXT("recipes"), RecipesJson))
    {
        for (const TSharedPtr<FJsonValue>& Value : *RecipesJson)
        {
            const TSharedPtr<FJsonObject> O = Value->AsObject();
            FVortexRecipe R;
            R.Id = FName(*O->GetStringField(TEXT("id")));
            R.Name = O->GetStringField(TEXT("name"));
            O->TryGetStringField(TEXT("description"), R.Description);
            O->TryGetStringField(TEXT("station"), R.Station);
            R.Output = FName(*O->GetStringField(TEXT("output")));
            O->TryGetNumberField(TEXT("count"), R.Count);
            for (const TPair<FString, TSharedPtr<FJsonValue>> I : O->GetObjectField(TEXT("ingredients"))->Values)
            {
                R.Ingredients.Add({ FName(*I.Key), int32(I.Value->AsNumber()) });
            }
            Recipes.Add(MoveTemp(R));
        }
    }
    for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Root->GetObjectField(TEXT("races"))->Values)
    {
        const TSharedPtr<FJsonObject> O = Pair.Value->AsObject();
        FVortexRaceCombat R;
        R.Melee = float(O->GetNumberField(TEXT("melee")));
        R.Ranged = float(O->GetNumberField(TEXT("ranged")));
        R.Magic = float(O->GetNumberField(TEXT("magic")));
        R.Taken = float(O->GetNumberField(TEXT("taken")));
        R.HpRegen = float(O->GetNumberField(TEXT("hpRegen")));
        R.MpRegen = float(O->GetNumberField(TEXT("mpRegen")));
        R.bCanMagic = O->GetBoolField(TEXT("canMagic"));
        Races.Add(FName(*Pair.Key), R);
    }
    for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Root->GetObjectField(TEXT("skills"))->Values)
    {
        TArray<FVortexSkillDef>& List = Skills.Add(FName(*Pair.Key));
        for (const TSharedPtr<FJsonValue>& Value : Pair.Value->AsArray())
        {
            const TSharedPtr<FJsonObject> O = Value->AsObject();
            FVortexSkillDef S;
            S.Id = FName(*O->GetStringField(TEXT("id")));
            S.Icon = FName(*O->GetStringField(TEXT("icon")));
            S.Name = O->GetStringField(TEXT("name"));
            S.Description = O->GetStringField(TEXT("description"));
            S.Category = O->GetStringField(TEXT("category"));
            S.Level = int32(O->GetNumberField(TEXT("level")));
            S.Mana = int32(O->GetNumberField(TEXT("mana")));
            S.Cooldown = float(O->GetNumberField(TEXT("cooldown")));
            List.Add(S);
        }
    }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_RPG itens=%d saques=%d tecnicas=%d"), Items.Num(), Loot.Num(), Skills.Num());
}

const FVortexRaceCombat& FVortexRpgCatalog::Race(FName Id) const
{
    static const FVortexRaceCombat Human;
    const FVortexRaceCombat* Found = Races.Find(Id);
    return Found ? *Found : Human;
}

float FVortexRpgCatalog::Effect(const TCHAR* Key, float Default) const
{
    const float* Found = Effects.Find(FName(Key));
    return Found ? *Found : Default;
}

int64 FVortexRpgCatalog::XpForKill(FName Species, int32 MonsterLevel) const
{
    const int32* Base = XpSources.Find(Species);
    return FMath::RoundToInt64((Base ? *Base : 10) * .65 * (1.0 + .08 * FMath::Max(0, MonsterLevel - 1)));
}

FName FVortexRpgCatalog::WeaponFamily(FName ItemId) const
{
    if (Skills.Contains(ItemId)) return ItemId;
    if (const FVortexItemDef* Def = Item(ItemId))
    {
        if (Skills.Contains(FName(*Def->Family))) return FName(*Def->Family);
    }
    // espadas comuns do mercador e o resto usam as técnicas da espada (como no Godot)
    return TEXT("espada_de_cinzas");
}

FName FVortexRpgCatalog::WeaponType(FName ItemId) const
{
    const FVortexItemDef* Def = Item(ItemId);
    if (Def && Def->Type == TEXT("weapon") && !Def->Family.IsEmpty()) return FName(*Def->Family);
    if (ItemId == TEXT("sword_iron") || ItemId == TEXT("sword_jade")) return TEXT("espada_de_cinzas");
    return ItemId.IsNone() ? FName(TEXT("espada_de_cinzas")) : ItemId;
}

int64 FVortexRpgCatalog::XpForLevel(int32 Level)
{
    return Level <= 1 ? 0 : FMath::RoundToInt64(100.0 * FMath::Pow(double(Level - 1), 1.8));
}

int32 FVortexRpgCatalog::LevelFromXp(int64 Xp)
{
    int32 Level = 1;
    while (Level < 999 && XpForLevel(Level + 1) <= Xp) ++Level;
    return Level;
}

UTexture2D* FVortexRpgCatalog::Icon(FName Icon)
{
    static TMap<FName, UTexture2D*> Cache;
    if (UTexture2D** Found = Cache.Find(Icon)) return *Found;
    const FString Name = TEXT("T_") + Icon.ToString();
    UTexture2D* Texture = Icon.IsNone() ? nullptr : LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/UI/Icons/%s.%s"), *Name, *Name));
    if (Texture) Texture->AddToRoot();   // ícones pequenos e poucos: ficam carregados (o Slate não segura)
    Cache.Add(Icon, Texture);
    return Texture;
}

FLinearColor FVortexRpgCatalog::RarityColor(const FString& Rarity)
{
    if (Rarity == TEXT("uncommon")) return FLinearColor::FromSRGBColor(FColor(0x6F, 0xD0, 0x6A));
    if (Rarity == TEXT("rare")) return FLinearColor::FromSRGBColor(FColor(0x55, 0x9C, 0xF0));
    if (Rarity == TEXT("epic")) return FLinearColor::FromSRGBColor(FColor(0xB0, 0x6C, 0xF2));
    if (Rarity == TEXT("legendary")) return FLinearColor::FromSRGBColor(FColor(0xF2, 0xA2, 0x3A));
    // 07/11: a única ganhou cor (saía cinza, como a comum) e a divina é nova (as espadas divinas)
    if (Rarity == TEXT("unique")) return FLinearColor::FromSRGBColor(FColor(0x3E, 0xD6, 0xC4));
    if (Rarity == TEXT("divine")) return FLinearColor::FromSRGBColor(FColor(0xFF, 0xEC, 0xA0));
    return FLinearColor::FromSRGBColor(FColor(0xC9, 0xC2, 0xB2));
}

FString FVortexRpgCatalog::RarityLabel(const FString& Rarity)
{
    if (Rarity == TEXT("uncommon")) return TEXT("Incomum");
    if (Rarity == TEXT("rare")) return TEXT("Raro");
    if (Rarity == TEXT("epic")) return TEXT("Épico");
    if (Rarity == TEXT("legendary")) return TEXT("Lendário");
    if (Rarity == TEXT("unique")) return TEXT("Único");
    if (Rarity == TEXT("divine")) return TEXT("Divino");
    return TEXT("Comum");
}

FString FVortexRpgCatalog::TypeLabel(const FString& Type)
{
    if (Type == TEXT("weapon")) return TEXT("Arma");
    if (Type == TEXT("armor")) return TEXT("Equipamento");
    if (Type == TEXT("consumable")) return TEXT("Uso");
    if (Type == TEXT("accessory")) return TEXT("Joia");
    if (Type == TEXT("offhand")) return TEXT("Mão secundária");
    return TEXT("Material");
}

FString FVortexRpgCatalog::StatLine(FName Stat, float Value)
{
    static const TMap<FName, FString> Names = {
        { TEXT("attack"), TEXT("Ataque") }, { TEXT("defense"), TEXT("Defesa") }, { TEXT("resistance"), TEXT("Resist. mágica") },
        { TEXT("max_health"), TEXT("Vida máx.") }, { TEXT("max_mana"), TEXT("Mana máx.") }, { TEXT("crit"), TEXT("% Crítico") },
        { TEXT("crit_dmg"), TEXT("% Dano crítico") }, { TEXT("speed"), TEXT("% Velocidade de ataque") }, { TEXT("haste"), TEXT("% Recarga") },
        { TEXT("skill"), TEXT("Poder de magia") }, { TEXT("lifesteal"), TEXT("% Roubo de vida") }, { TEXT("hp_regen"), TEXT("Regeneração") },
    };
    const FString* Name = Names.Find(Stat);
    const FString Number = FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value)) ? FString::FromInt(FMath::RoundToInt(Value))
        : FString::Printf(TEXT("%.1f"), Value).Replace(TEXT("."), TEXT(","));
    return FString::Printf(TEXT("%s %s%s"), Name ? **Name : *Stat.ToString(), Value >= 0.f ? TEXT("+") : TEXT(""), *Number);
}

float FVortexClassSkill::Reach() const
{
    // o alcance é o que a descrição diz (o Godot multiplicava por 1,3 a 3: "estocada de 6 m" acertava a 20 m)
    return FMath::Abs(Range);
}

bool FVortexClassSkill::IsPhysical() const
{
    return Element == TEXT("steel") || Element == TEXT("earth") || Element == TEXT("wind") || Element == TEXT("blood") || Element == TEXT("shadow");
}

FString FVortexClassSkill::Category() const
{
    const FString K = Kind.ToString();
    if (K == TEXT("projectile") || K == TEXT("wave") || K == TEXT("beam") || K == TEXT("chain")) return TEXT("OFENSIVA");
    if (K == TEXT("cleave") || K == TEXT("nova") || K == TEXT("strike") || K == TEXT("zone") || K == TEXT("summon")) return TEXT("ÁREA");
    if (K == TEXT("dash")) return TEXT("MOBILIDADE");
    if (K == TEXT("shield")) return TEXT("DEFESA");
    if (K == TEXT("heal")) return TEXT("CURA");
    return TEXT("SUPORTE");
}

float FVortexRpgCatalog::ClassWeight(const FVortexClassDef& C, FName Race, FName WeaponType, const TArray<int32>* Points) const
{
    // a raça (0 = bloqueada), a arma do Despertar (classe que luta com ela pesa 3x; as outras ficam raras)
    // e onde o herói pôs os pontos (classe cujos bônus e atributo combinam com os pontos pesa até 3,5x)
    float W = C.AffinityFor(Race);
    if (W <= 0.f) return 0.f;
    if (!WeaponType.IsNone()) W *= C.Weapons.Contains(WeaponType) ? 3.f : .4f;
    if (Points && Points->Num() == VortexRpg::NumGroups)
    {
        int32 Total = 0;
        for (int32 P : *Points) Total += P;
        if (Total > 0)
        {
            using namespace VortexRpg;
            TSet<int32> Likes;
            for (const TPair<FName, float>& Buff : C.Buffs) { const int32 O = OfficialOf(Buff.Key); if (O != INDEX_NONE) Likes.Add(O); }
            static const TMap<FString, TArray<int32>> ByAttribute = {
                { TEXT("strength"), { Attack, Defense } }, { TEXT("intelligence"), { Skill, MaxManaStat } },
                { TEXT("agility"), { Crit, Speed, CritDmg } }, { TEXT("vitality"), { MaxHealthStat, Defense, HpRegenStat } },
                { TEXT("spirit"), { HpRegenStat, MaxManaStat, Resistance } }, { TEXT("mana"), { MaxManaStat, Skill, Haste } },
            };
            if (const TArray<int32>* Extra = ByAttribute.Find(C.Attribute)) for (int32 O : *Extra) Likes.Add(O);
            // 10/11: os pontos são dos 8 atributos: conta cada atributo uma vez
            TSet<int32> LikedGroups;
            for (int32 O : Likes) LikedGroups.Add(GroupOf(O));
            int32 Matching = 0;
            for (int32 G : LikedGroups) if (Points->IsValidIndex(G)) Matching += (*Points)[G];
            W *= 1.f + 2.5f * float(Matching) / float(Total);
        }
    }
    return W;
}

TArray<FName> FVortexRpgCatalog::DrawClasses(FRandomStream& Rng, FName Race, FName WeaponType, const TArray<int32>* Points) const
{
    // Para cada carta: o rank sai pela chance; dentro do rank, cada classe pesa pela raça, pela arma
    // do Despertar e pelos pontos investidos (ClassWeight); sem repetir; ao menos uma Rara ou melhor.
    static const TCHAR* Ranks[] = { TEXT("common"), TEXT("rare"), TEXT("epic"), TEXT("legendary") };
    TArray<const FVortexClassDef*> Picked;
    auto Weight = [this, Race, WeaponType, Points](const FVortexClassDef& C) { return ClassWeight(C, Race, WeaponType, Points); };
    auto RollRank = [this, &Rng](bool bRareOrBetter)
    {
        const int32 First = bRareOrBetter ? 1 : 0;
        int32 Total = 0;
        for (int32 i = First; i < 4; ++i) Total += ClassRarityChance.FindRef(Ranks[i]);
        int32 Roll = Rng.RandRange(0, FMath::Max(1, Total) - 1);
        for (int32 i = First; i < 4; ++i) { Roll -= ClassRarityChance.FindRef(Ranks[i]); if (Roll < 0) return FString(Ranks[i]); }
        return FString(Ranks[3]);
    };
    auto Take = [this, &Picked, &Rng, &Weight](const FString& Rarity)
    {
        TArray<const FVortexClassDef*> Open;
        float Total = 0.f;
        for (const FVortexClassDef& C : Classes)
        {
            if (C.bDrawable && C.Rarity == Rarity && !Picked.Contains(&C) && Weight(C) > 0.f) { Open.Add(&C); Total += Weight(C); }
        }
        if (Open.IsEmpty()) return false;
        float Roll = Rng.FRand() * Total;
        for (const FVortexClassDef* C : Open) { Roll -= Weight(*C); if (Roll <= 0.f) { Picked.Add(C); return true; } }
        Picked.Add(Open.Last());
        return true;
    };
    for (int32 i = 0; i < 6 && Picked.Num() < 6; ++i)
    {
        const bool bNeedRare = i == 5 && !Picked.ContainsByPredicate([](const FVortexClassDef* C) { return C->Rarity != TEXT("common"); });
        if (Take(RollRank(bNeedRare))) continue;
        bool bAny = false;
        for (int32 R = bNeedRare ? 1 : 0; R < 4 && !bAny; ++R) bAny = Take(Ranks[R]);
        if (!bAny) Take(TEXT("common"));
    }
    TArray<FName> Out;
    for (const FVortexClassDef* C : Picked) Out.Add(C->Id);
    return Out;
}

FString FVortexRpgCatalog::AttributeHelp(int32 Attribute)
{
    switch (Attribute)
    {
    case VortexRpg::Strength: return TEXT("Da raça e da classe: dano dos golpes corpo a corpo.");
    case VortexRpg::Agility: return TEXT("Da raça e da classe: dano à distância e chance de crítico.");
    case VortexRpg::Vitality: return TEXT("Da raça e da classe: vida máxima (6 por ponto) e vida que volta.");
    case VortexRpg::Intelligence: return TEXT("Da raça e da classe: magia e curas mais fortes (3% por ponto).");
    case VortexRpg::ManaAttr: return TEXT("Da raça e da classe: mana máxima (5 por ponto) e um pouco de magia.");
    case VortexRpg::Spirit: return TEXT("Da raça e da classe: a mana volta mais rápido.");
    case VortexRpg::Recovery: return TEXT("Da raça: fora de combate (6 s sem apanhar), a vida volta mais rápido.");
    case VortexRpg::Luck: return TEXT("Da raça: mais saque e moedas, e um pouco mais de crítico.");
    default: return TEXT("");
    }
}
