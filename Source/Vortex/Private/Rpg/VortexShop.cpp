#include "Rpg/VortexShop.h"

#include "Rpg/VortexRpg.h"

namespace
{
    const TCHAR* Rarities[] = { TEXT("common"), TEXT("uncommon"), TEXT("rare"), TEXT("epic"), TEXT("legendary"), TEXT("unique") };
    const int32 GearPrice[] = { 40, 110, 300, 700, 1600, 3000 };
    const int32 MaterialPrice[] = { 4, 10, 28, 70, 160, 300 };

    int32 RarityIndex(const FString& Rarity)
    {
        for (int32 i = 0; i < UE_ARRAY_COUNT(Rarities); ++i) if (Rarity == Rarities[i]) return i;
        return 0;
    }

    // Semente estável (FNV-1a, como o WorldHash do Godot).
    uint32 Hash(const FString& Text)
    {
        uint32 H = 2166136261u;
        for (TCHAR C : Text) { H ^= uint32(C); H *= 16777619u; }
        return H & 0x7fffffffu;
    }

    // Gerador simples e igual em todas as máquinas (o FRandomStream também serviria; este deixa o
    // sorteio explícito e independente da versão da Unreal).
    struct FRoll
    {
        uint32 State;
        explicit FRoll(uint32 Seed) : State(Seed ? Seed : 1u) {}
        int32 Next(int32 Max) { State ^= State << 13; State ^= State >> 17; State ^= State << 5; return Max > 0 ? int32(State % uint32(Max)) : 0; }
    };

    const TArray<FName>& Names(std::initializer_list<const TCHAR*> List, TArray<FName>& Out)
    {
        if (Out.IsEmpty()) for (const TCHAR* N : List) Out.Add(FName(N));
        return Out;
    }

    const TArray<FName>& ForgedFamilies() { static TArray<FName> L; return Names({ TEXT("espada_de_cinzas"), TEXT("lanca_do_patrulheiro"), TEXT("martelo_de_bram"), TEXT("manoplas_de_ferro"), TEXT("adagas_gemeas"), TEXT("katana_do_vento") }, L); }
    const TArray<FName>& MagicFamilies() { static TArray<FName> L; return Names({ TEXT("cajado_de_ilsa"), TEXT("livro_da_aurora") }, L); }
    const TArray<FName>& MetalArmor() { static TArray<FName> L; return Names({ TEXT("helm_iron"), TEXT("chest_mail"), TEXT("boots_iron"), TEXT("guard_head"), TEXT("guard_chest"), TEXT("guard_legs"), TEXT("guard_boots"),
        TEXT("crater_head"), TEXT("crater_chest"), TEXT("crater_legs"), TEXT("crater_boots") }, L); }
    const TArray<FName>& ClothArmor() { static TArray<FName> L; return Names({ TEXT("helm_leather"), TEXT("chest_tunic"), TEXT("chest_punk"), TEXT("legs_cloth"), TEXT("legs_leather"), TEXT("boots_leather"),
        TEXT("wolf_head"), TEXT("wolf_chest"), TEXT("wolf_legs"), TEXT("wolf_boots"), TEXT("bark_head"), TEXT("bark_chest"), TEXT("bark_legs"), TEXT("bark_boots") }, L); }
    const TArray<FName>& Metals() { static TArray<FName> L; return Names({ TEXT("iron_scrap"), TEXT("obsidian_fragment"), TEXT("colossus_shard") }, L); }
    const TArray<FName>& Hides() { static TArray<FName> L; return Names({ TEXT("wolf_pelt"), TEXT("rabbit_pelt"), TEXT("bark_fiber"), TEXT("bandit_bandana"), TEXT("deer_antler") }, L); }
    const TArray<FName>& Herbs() { static TArray<FName> L; return Names({ TEXT("healing_herb"), TEXT("mushroom_cap"), TEXT("gel_jade"), TEXT("moss_core"), TEXT("bat_wing"), TEXT("frutas_silvestres") }, L); }
    const TArray<FName>& Food() { static TArray<FName> L; return Names({ TEXT("meat"), TEXT("bread"), TEXT("frutas_silvestres"), TEXT("racao_de_viagem") }, L); }
    const TArray<FName>& CaravanGoods() { static TArray<FName> L; return Names({ TEXT("bread"), TEXT("frutas_silvestres"), TEXT("racao_de_viagem"), TEXT("meat"), TEXT("potion_red"), TEXT("potion_blue"), TEXT("antidoto"), TEXT("healing_herb"),
        TEXT("mapa_de_viajante"), TEXT("carta_de_terras_distantes"), TEXT("arco_curto"), TEXT("espada_de_ferro"), TEXT("adagas_gemeas"), TEXT("manoplas_de_ferro"), TEXT("livro_da_aurora") }, L); }
    const TArray<FName>& DuneGoods() { static TArray<FName> L; return Names({ TEXT("bread"), TEXT("racao_de_viagem"), TEXT("potion_red"), TEXT("potion_blue"), TEXT("antidoto"), TEXT("mapa_de_viajante"),
        TEXT("espada_de_ferro"), TEXT("lanca_do_patrulheiro"), TEXT("marreta_de_ferreiro"), TEXT("katana_do_vento") }, L); }
    const TArray<FName>& HunterGoods() { static TArray<FName> L; return Names({ TEXT("bread"), TEXT("frutas_silvestres"), TEXT("meat"), TEXT("potion_red"), TEXT("potion_blue"), TEXT("antidoto"), TEXT("healing_herb"),
        TEXT("arco_curto"), TEXT("arco_do_cacador"), TEXT("lanca_de_caca"), TEXT("adagas_gemeas") }, L); }

    const FVortexItemDef* Def(FName Id) { return FVortexRpgCatalog::Get().Item(Id); }

    // O que o Ducci paga: 30% (materiais pelo valor de mercado).
    int32 SellPrice(const FVortexItemDef& D)
    {
        return D.Type == TEXT("material") ? MaterialPrice[RarityIndex(D.Rarity)] : FMath::Max(1, VortexShop::BuyPrice(D) * 3 / 10);
    }

    int32 Rate(const FVortexItemDef& D, double Gear, double Material)
    {
        return FMath::Max(1, FMath::RoundToInt(D.Type == TEXT("material") ? MaterialPrice[RarityIndex(D.Rarity)] * Material : VortexShop::BuyPrice(D) * Gear));
    }

    VortexShop::FOffer Fixed(FName Id, int32 Qty)
    {
        VortexShop::FOffer O;
        O.Item = Id; O.Qty = Qty;
        if (const FVortexItemDef* D = Def(Id)) O.Price = VortexShop::BuyPrice(*D);
        return O;
    }

    // Sorteia N itens da lista pesando pela raridade (as comuns aparecem mais), sem repetir.
    void Roll(FRoll& Rng, TArray<const FVortexItemDef*> Pool, int32 N, int32 Qty, TArray<VortexShop::FOffer>& Out)
    {
        static const int32 Weight[] = { 40, 30, 18, 9, 3, 0 };
        Pool.RemoveAll([](const FVortexItemDef* D) { return Weight[RarityIndex(D->Rarity)] <= 0; });
        Pool.Sort([](const FVortexItemDef& A, const FVortexItemDef& B) { return A.Id.LexicalLess(B.Id); });
        for (int32 Picked = 0; Picked < N && Pool.Num() > 0; ++Picked)
        {
            int32 Total = 0;
            for (const FVortexItemDef* D : Pool) Total += Weight[RarityIndex(D->Rarity)];
            int32 R = Rng.Next(Total), Index = 0;
            for (; Index < Pool.Num(); ++Index) { R -= Weight[RarityIndex(Pool[Index]->Rarity)]; if (R < 0) break; }
            Index = FMath::Min(Index, Pool.Num() - 1);
            Out.Add(Fixed(Pool[Index]->Id, Qty));
            Pool.RemoveAt(Index);
        }
    }

    TArray<const FVortexItemDef*> AllWhere(TFunctionRef<bool(const FVortexItemDef&)> Test)
    {
        TArray<const FVortexItemDef*> Out;
        for (const TPair<FName, FVortexItemDef>& Pair : FVortexRpgCatalog::Get().Items) if (Test(Pair.Value)) Out.Add(&Pair.Value);
        return Out;
    }

    TArray<const FVortexItemDef*> Defs(const TArray<FName>& Ids)
    {
        TArray<const FVortexItemDef*> Out;
        for (const FName Id : Ids) if (const FVortexItemDef* D = Def(Id)) Out.Add(D);
        return Out;
    }

    // Ducci: 8 armas (raridade sorteada), 3 armaduras/acessórios e as poções de sempre.
    void DucciStock(FRoll& Rng, TArray<VortexShop::FOffer>& Out)
    {
        static const int32 RarityWeight[] = { 38, 28, 18, 10, 5, 1 };
        // (as divinas não se vendem: só caem do Colosso de Runas e do Golem)
        TArray<const FVortexItemDef*> Weapons = AllWhere([](const FVortexItemDef& D) { return D.Type == TEXT("weapon") && !D.Family.IsEmpty() && D.Rarity != TEXT("divine"); });
        TArray<const FVortexItemDef*> Gear = AllWhere([](const FVortexItemDef& D) { return D.Type == TEXT("armor") || D.Type == TEXT("accessory") || D.Type == TEXT("offhand"); });
        auto ById = [](const FVortexItemDef& A, const FVortexItemDef& B) { return A.Id.LexicalLess(B.Id); };
        Weapons.Sort(ById);
        Gear.Sort(ById);
        TSet<FName> Taken;
        for (int32 i = 0; i < 8 && Weapons.Num(); ++i)
        {
            int32 R = Rng.Next(100), Rarity = 0;
            for (; Rarity < 5; ++Rarity) { R -= RarityWeight[Rarity]; if (R < 0) break; }
            TArray<const FVortexItemDef*> Pool = Weapons.FilterByPredicate([&](const FVortexItemDef* D) { return RarityIndex(D->Rarity) == Rarity && !Taken.Contains(D->Id); });
            if (Pool.IsEmpty()) Pool = Weapons.FilterByPredicate([&](const FVortexItemDef* D) { return !Taken.Contains(D->Id); });
            if (Pool.IsEmpty()) break;
            const FVortexItemDef* Pick = Pool[Rng.Next(Pool.Num())];
            Taken.Add(Pick->Id);
            Out.Add(Fixed(Pick->Id, 1));
        }
        for (int32 i = 0; i < 3; ++i)
        {
            TArray<const FVortexItemDef*> Pool = Gear.FilterByPredicate([&](const FVortexItemDef* D) { return !Taken.Contains(D->Id); });
            if (Pool.IsEmpty()) break;
            const FVortexItemDef* Pick = Pool[Rng.Next(Pool.Num())];
            Taken.Add(Pick->Id);
            Out.Add(Fixed(Pick->Id, 1));
        }
        for (const TCHAR* Potion : { TEXT("potion_red"), TEXT("potion_blue"), TEXT("potion_green") })
        {
            if (!Def(Potion)) continue;
            VortexShop::FOffer O = Fixed(Potion, 1);
            O.bUnlimited = true;
            Out.Add(O);
        }
    }
}

int32 VortexShop::BuyPrice(const FVortexItemDef& D)
{
    if (D.Price > 0) return D.Price;
    const int32 R = RarityIndex(D.Rarity);
    if (D.Type == TEXT("weapon") || D.Type == TEXT("armor") || D.Type == TEXT("offhand")) return GearPrice[R];
    if (D.Type == TEXT("accessory")) return GearPrice[R] * 6 / 5;
    if (D.Type == TEXT("consumable")) return D.Mana > 0.f ? 30 : D.Rarity == TEXT("common") ? 25 : 60;
    return MaterialPrice[R] * 3;
}

bool VortexShop::IsVendor(FName V)
{
    static const TSet<FName> Known = { TEXT("ducci"), TEXT("bram"), TEXT("garrick"), TEXT("rosa"), TEXT("ilsa"), TEXT("marta"), TEXT("caravana"), TEXT("solcanion"), TEXT("verdalume") };
    return Known.Contains(V);
}

FName VortexShop::NpcOf(FName V)
{
    if (V == TEXT("solcanion")) return TEXT("sahil");
    if (V == TEXT("verdalume")) return TEXT("tavian");
    return V;
}

FName VortexShop::BoughtKey(FName Vendor, int32 Index)
{
    return FName(*FString::Printf(TEXT("%s:%d"), *Vendor.ToString(), Index));
}

VortexShop::FVendorInfo VortexShop::Info(FName V)
{
    FVendorInfo I;
    I.Id = V;
    const FString Craft = TEXT(" só compra o que é do ofício.");
    if (V == TEXT("bram"))
    {
        I.Seller = TEXT("Bram"); I.Title = TEXT("BRAM  ·  FORJA DO SANTUÁRIO");
        I.Greeting = TEXT("Bram: \"Aço bom não se compra com pressa. Escolha uma e eu digo se presta.\"");
        I.SellHelp = TEXT("Bram compra armas (45% do preço) e metal: sucata de ferro, obsidiana e fragmentos do colosso pagam bem. Ervas, comida e roupa ele não quer.");
        I.BuyLine = I.Seller + Craft;
    }
    else if (V == TEXT("garrick"))
    {
        I.Seller = TEXT("Garrick"); I.Title = TEXT("GARRICK  ·  ARMARIA");
        I.Greeting = TEXT("Garrick: \"Ferro bem batido salva vidas. Experimente o peso antes de levar.\"");
        I.SellHelp = TEXT("Garrick compra armaduras de metal e escudos (45%) e sucata de ferro e obsidiana para remendar. Couro e pano: fale com a Rosa.");
        I.BuyLine = I.Seller + Craft;
    }
    else if (V == TEXT("rosa"))
    {
        I.Seller = TEXT("Rosa"); I.Title = TEXT("ROSA  ·  ALFAIATARIA");
        I.Greeting = TEXT("Rosa: \"Couro curtido, linho bom e pele de lobo. Roupa que aguenta estrada.\"");
        I.SellHelp = TEXT("Rosa compra peles, fibras, galhadas e bandanas (140%) para os moldes, e roupas de pano e couro usadas (40%).");
        I.BuyLine = I.Seller + Craft;
    }
    else if (V == TEXT("ilsa"))
    {
        I.Seller = TEXT("Ilsa"); I.Title = TEXT("ILSA  ·  ERVAS E POÇÕES");
        I.Greeting = TEXT("Ilsa: \"Cada frasco foi destilado com calma. Use com sabedoria.\"");
        I.SellHelp = TEXT("Ilsa compra ervas, cogumelos, gel de jade, núcleos de musgo, asas de morcego e frutas (150%). Armas e armaduras não interessam a ela.");
        I.BuyLine = I.Seller + Craft;
    }
    else if (V == TEXT("marta"))
    {
        I.Seller = TEXT("Marta"); I.Title = TEXT("MARTA  ·  TAVERNA DO JAVALI DOURADO");
        I.Greeting = TEXT("Marta: \"Ensopado no fogo e pão saindo do forno. Sente-se, aventureiro!\"");
        I.SellHelp = TEXT("Marta compra carne, pão, frutas e rações para a cozinha (130%). O resto, leve ao Ducci.");
        I.BuyLine = I.Seller + Craft;
    }
    else
    {
        const bool bCaravan = V == TEXT("caravana"), bDune = V == TEXT("solcanion"), bWood = V == TEXT("verdalume");
        I.bRestocksDaily = !(bCaravan || bDune || bWood);
        I.Seller = bDune ? TEXT("Sahil") : bWood ? TEXT("Tavian") : bCaravan ? TEXT("A caravana") : TEXT("Ducci");
        I.Title = bDune ? TEXT("SAHIL  ·  MERCADO DAS DUNAS") : bWood ? TEXT("TAVIAN  ·  SUPRIMENTOS DA MATA")
            : bCaravan ? TEXT("CARAVANA  ·  MERCADORES DAS TRILHAS") : TEXT("DUCCI  ·  O MERCADOR MAGO AMBULANTE");
        I.Greeting = bDune ? TEXT("Sahil: \"Leve água, poções e uma arma firme para cruzar as dunas.\"")
            : bWood ? TEXT("Tavian: \"Antídotos e equipamento de caça. A mata não perdoa distração.\"")
            : bCaravan ? TEXT("Mercador: \"Pão, rações, poções e mapas de onde passamos. Nada encantado, tudo honesto.\"")
            : TEXT("Ducci: \"Olhe à vontade! Tudo encantado, quase tudo funciona.\"");
        I.BuyLine = TEXT("Ele compra os seus achados por um preço justo (quase).");
        I.SellHelp = I.Seller + TEXT(" compra tudo que está na mochila: peles, gel, asas, cogumelos, ervas, núcleos e até equipamento velho.\n\nMateriais raros valem bem mais: o núcleo do golem e os fragmentos da cratera pagam muito.");
    }
    return I;
}

bool VortexShop::ForgeCost(const FVortexItemDef& Item, int32 HeroLevel, int32& OutCoins, FName& OutMaterial, int32& OutCount, int32& OutLevel)
{
    // como a melhoria dos MMOs: cada nível custa um quarto do preço do item no nível novo, e o material sobe a cada 30
    OutLevel = FMath::Max(1, Item.Level) + 1;
    if (!Item.IsEquipment() || OutLevel > FMath::Min(100, HeroLevel)) return false;
    const FVortexItemDef* Next = FVortexRpgCatalog::Get().Item(FVortexRpgCatalog::LevelledId(Item.BaseId.IsNone() ? Item.Id : Item.BaseId, OutLevel));
    OutCoins = FMath::Max(10, FMath::RoundToInt((Next ? Next->Price : Item.Price) * .25f));
    // as 7 raridades (o RarityIndex da loja vai só até a Única: a Divina caía na sucata de ferro)
    static const TCHAR* const Ranks[] = { TEXT("common"), TEXT("uncommon"), TEXT("rare"), TEXT("epic"), TEXT("legendary"), TEXT("unique"), TEXT("divine") };
    int32 R = 0;
    for (int32 i = 0; i < UE_ARRAY_COUNT(Ranks); ++i) if (Item.Rarity == Ranks[i]) R = i;
    OutMaterial = R <= 2 ? FName(TEXT("iron_scrap")) : R <= 4 ? FName(TEXT("obsidian_fragment")) : FName(TEXT("colossus_shard"));
    OutCount = 1 + OutLevel / 30;
    return true;
}

TArray<VortexShop::FOffer> VortexShop::Stock(FName V, const FString& WorldId, int32 Day, int32 HeroLevel)
{
    TArray<FOffer> Out = StockBase(V, WorldId, Day);
    if (HeroLevel <= 0) return Out;
    for (FOffer& O : Out)
    {
        const FVortexItemDef* D = Def(O.Item);
        if (!D) continue;
        if (D->IsEquipment())
        {
            O.Item = FVortexRpgCatalog::LevelledId(O.Item, HeroLevel);
            if (const FVortexItemDef* L = Def(O.Item)) O.Price = BuyPrice(*L);
        }
        else if (D->IsUsable()) O.Price = FMath::RoundToInt(O.Price * (1.f + (HeroLevel - 1) / 10.f));   // no 1, o preço de sempre
    }
    return Out;
}

TArray<VortexShop::FOffer> VortexShop::StockBase(FName V, const FString& WorldId, int32 Day)
{
    TArray<FOffer> Out;
    FRoll Rng(Hash(WorldId + (V == TEXT("ducci") ? FString() : V.ToString())) ^ uint32(Day * 7919));
    if (V == TEXT("bram"))
    {
        Roll(Rng, AllWhere([](const FVortexItemDef& D) { return D.Type == TEXT("weapon") && ForgedFamilies().Contains(FName(*D.Family)) && D.Rarity != TEXT("divine"); }), 6, 1, Out);
        if (Def(TEXT("shield_wood"))) Out.Add(Fixed(TEXT("shield_wood"), 2));
    }
    else if (V == TEXT("garrick"))
    {
        Roll(Rng, Defs(MetalArmor()), 5, 1, Out);
        if (Def(TEXT("shield_wood"))) Out.Add(Fixed(TEXT("shield_wood"), 3));
    }
    else if (V == TEXT("rosa")) Roll(Rng, Defs(ClothArmor()), 5, 1, Out);
    else if (V == TEXT("ilsa"))
    {
        for (const TPair<const TCHAR*, int32>& P : { TPair<const TCHAR*, int32>(TEXT("potion_red"), 5), { TEXT("potion_blue"), 4 }, { TEXT("potion_green"), 2 }, { TEXT("antidoto"), 3 }, { TEXT("healing_herb"), 5 } })
            if (Def(P.Key)) Out.Add(Fixed(P.Key, P.Value));
        Roll(Rng, AllWhere([](const FVortexItemDef& D) { return D.Type == TEXT("weapon") && MagicFamilies().Contains(FName(*D.Family)); }), 2, 1, Out);
    }
    else if (V == TEXT("marta"))
    {
        for (const TPair<const TCHAR*, int32>& P : { TPair<const TCHAR*, int32>(TEXT("bread"), 8), { TEXT("meat"), 5 }, { TEXT("racao_de_viagem"), 4 }, { TEXT("frutas_silvestres"), 6 } })
            if (Def(P.Key)) Out.Add(Fixed(P.Key, P.Value));
    }
    else if (V == TEXT("caravana") || V == TEXT("solcanion") || V == TEXT("verdalume"))
    {
        const TArray<FName>& Goods = V == TEXT("caravana") ? CaravanGoods() : V == TEXT("solcanion") ? DuneGoods() : HunterGoods();
        for (const FName Id : Goods)
        {
            if (!Def(Id)) continue;
            FOffer O = Fixed(Id, 1);
            O.bUnlimited = true;
            Out.Add(O);
        }
    }
    else DucciStock(Rng, Out);
    return Out;
}

int32 VortexShop::Pays(FName V, const FVortexItemDef& D)
{
    const FName Id = D.Id;
    if (V == TEXT("bram")) return D.Type == TEXT("weapon") ? Rate(D, .45, 0) : Metals().Contains(Id) ? Rate(D, 0, 1.5) : 0;
    if (V == TEXT("garrick"))
        return MetalArmor().Contains(Id) || D.Type == TEXT("offhand") ? Rate(D, .45, 0)
            : Id == TEXT("iron_scrap") || Id == TEXT("obsidian_fragment") ? Rate(D, 0, 1.2) : 0;
    if (V == TEXT("rosa")) return ClothArmor().Contains(Id) ? Rate(D, .40, 0) : Hides().Contains(Id) ? Rate(D, 0, 1.4) : 0;
    if (V == TEXT("ilsa")) return Herbs().Contains(Id) ? Rate(D, 0, 1.5) : 0;
    if (V == TEXT("marta")) return Food().Contains(Id) ? Rate(D, 1.3, 1.3) : 0;
    return SellPrice(D);
}
