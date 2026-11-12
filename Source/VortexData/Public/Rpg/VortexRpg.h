#pragma once

#include "CoreMinimal.h"
#include "VortexRpg.generated.h"

class UTexture2D;

/** Uma pilha na mochila (ou um espaço vazio: Item nenhum). */
USTRUCT(BlueprintType)
struct VORTEXDATA_API FVortexItemStack
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FName Item;
    UPROPERTY(SaveGame) int32 Count = 0;
    bool IsEmpty() const { return Item.IsNone() || Count <= 0; }
    bool operator==(const FVortexItemStack& Other) const { return Item == Other.Item && Count == Other.Count; }
};

namespace VortexRpg
{
    /** Dificuldade de uma criatura para um herói, pela diferença de nível (ela menos ele):
     *  0 cinza (bem abaixo), 1 branco (abaixo), 2 verde (mesmo nível), 3 amarelo, 4 laranja,
     *  5 vermelho (muito difícil), 6 preto (mata com um golpe). */
    inline int32 Difficulty(int32 HeroLevel, int32 CreatureLevel)
    {
        const int32 Gap = CreatureLevel - HeroLevel;
        return Gap >= 15 ? 6 : Gap >= 8 ? 5 : Gap >= 4 ? 4 : Gap >= 2 ? 3 : Gap >= -2 ? 2 : Gap >= -7 ? 1 : 0;
    }
    inline FLinearColor DifficultyColor(int32 Tier)
    {
        static const FLinearColor Colors[7] = { { .55f, .55f, .55f }, { .95f, .95f, .92f }, { .42f, 1.f, .38f }, { 1.f, .9f, .25f },
            { 1.f, .55f, .15f }, { 1.f, .16f, .12f }, { .08f, .05f, .07f } };
        return Colors[FMath::Clamp(Tier, 0, 6)];
    }
    /** Dano pela diferença de nível: a criatura muito acima bate muito mais forte (15+ níveis: um golpe
     *  mata) e apanha menos; a de baixo, o contrário. */
    inline float GapScale(int32 AttackerLevel, int32 DefenderLevel, bool bCreatureAttacking)
    {
        const int32 Gap = AttackerLevel - DefenderLevel;
        if (bCreatureAttacking)
            return Gap > 0 ? 1.f + .09f * Gap + (Gap >= 15 ? .6f * (Gap - 14) : 0.f) : FMath::Max(.45f, 1.f + .04f * Gap);
        return Gap < 0 ? FMath::Max(.15f, 1.f + .045f * Gap) : FMath::Min(1.5f, 1.f + .02f * Gap);
    }
    /** Força, Destreza, Vitalidade, Inteligência, Mana, Espírito, Recuperação, Sorte (a ordem das raças).
     *  Como no Godot (Compêndio v1.2), estes oito são o vetor da raça: não recebem pontos. */
    constexpr int32 NumAttributes = 8;
    enum EAttribute : uint8 { Strength, Agility, Vitality, Intelligence, ManaAttr, Spirit, Recovery, Luck };
    /** Os 12 atributos oficiais: os pontos de nível vão para eles, com as mesmas chaves do
     *  equipamento (ponto e item somam igual). */
    constexpr int32 NumOfficial = 12;
    enum EOfficial : uint8 { Attack, Defense, Resistance, MaxHealthStat, MaxManaStat, Crit, CritDmg, Speed, Haste, Skill, Lifesteal, HpRegenStat };
    VORTEXDATA_API FName OfficialKey(int32 Official);       // "attack", "defense"...
    VORTEXDATA_API FString OfficialLabel(int32 Official);   // "Ataque", "Defesa"...
    VORTEXDATA_API FString OfficialHelp(int32 Official);
    VORTEXDATA_API int32 OfficialOf(FName Key);              // INDEX_NONE se não é oficial
    /** 10/11 (o usuário: "junta em 8"): os pontos de nível vão para 8 atributos; cada um sobe 1 ou 2 dos oficiais
     *  (que continuam sendo as chaves do equipamento). Força: Ataque. Magia: Poder de magia e Mana. Vitalidade: Vida e
     *  Regeneração. Defesa. Resistência Mágica. Precisão: Crítico e Dano crítico. Agilidade: Velocidade de ataque e
     *  Recarga. Vampirismo: Roubo de vida. */
    constexpr int32 NumGroups = 8;
    enum EPointGroup : uint8 { GStrength, GMagic, GVitality, GDefense, GMagicResist, GPrecision, GAgility, GVampirism };
    VORTEXDATA_API int32 GroupOf(int32 Official);            // o atributo de pontos que sobe este oficial
    VORTEXDATA_API FString GroupLabel(int32 Group);          // "Força", "Magia"...
    VORTEXDATA_API FString GroupHelp(int32 Group);
    VORTEXDATA_API TArray<int32> GroupStats(int32 Group);    // os oficiais que o atributo sobe (o principal primeiro)
    /** Roubo de vida máximo (arma + pontos), em %. */
    constexpr float MaxLifesteal = 25.f;
    constexpr int32 BagSize = 30;
    constexpr int32 PointsPerLevel = 7;
    /** Lugares do corpo, na ordem da tela de equipamento. */
    enum EEquip : uint8 { Weapon, Head, Chest, Legs, Boots, Jewel, NumEquip };
    VORTEXDATA_API FName EquipKey(int32 Slot);          // "weapon", "head"... (o "slot" dos itens)
    VORTEXDATA_API FString EquipLabel(int32 Slot);      // "Arma", "Cabeça"...
    VORTEXDATA_API int32 EquipSlotOf(FName ItemSlot);   // INDEX_NONE se não veste
}

struct VORTEXDATA_API FVortexItemDef
{
    FName Id;
    FString Name, Description, Rarity, Type, Family;
    FName Slot, Icon;
    int32 Stack = 1;
    TMap<FName, float> Stats;   // attack, defense, max_health, max_mana, crit, crit_dmg, hp_regen...
    float Heal = 0.f, Mana = 0.f;
    /** Mapas das caravanas: centenas de metros de rota que o mapa copia para a carta do herói. */
    int32 RevealMap = 0;
    // ---- arsenal (Data/Rpg/arsenal.json, Tools/BuildArsenal.py)
    /** Elemento da arma (fire, water, lightning, ice, nature, poison, holy, shadow) e peculiaridade (bleed, vampiric, heavy, keen). */
    FName Element, Trait;
    /** Texto do efeito do elemento/peculiaridade (a janela do herói mostra). */
    FString EffectText;
    /** Preço na loja (vender rende uma parte). */
    int32 Price = 0;
    /** Conjunto de armadura (bônus de 2 e 4 peças). */
    FName Set;
    /** Visual no herói: categoria do guarda-roupa (head, shirt, pants, shoes) -> peça, e as cores. */
    TMap<FName, FName> Look;
    TMap<FName, FColor> LookColors;
    /** 07/11: o nível do item (como nos jogos de saque: a mesma arma em vários níveis, "<id>@<nível>") e o nível que o
     *  herói precisa para vestir. Os itens sem "@" ficam no nível natural da raridade (Data/Rpg/balance.json, itemLevel). */
    int32 Level = 0, ReqLevel = 0;
    /** O item de onde ele saiu (o id sem o "@nível"); o mesmo id para os itens de nível natural. */
    FName BaseId;
    /** Quanto o Ataque (ou o Poder de magia) dele rende perto do da espada da mesma raridade (o martelo um pouco mais). */
    float AttackFactor = 1.f;
    bool IsConsumable() const { return Type == TEXT("consumable") && (Heal > 0.f || Mana > 0.f); }
    bool IsEquipment() const { return VortexRpg::EquipSlotOf(Slot) != INDEX_NONE; }
    /** Dá para usar da mochila (beber, comer ou abrir um mapa). */
    bool IsUsable() const { return IsConsumable() || RevealMap > 0; }
};

struct VORTEXDATA_API FVortexSkillDef
{
    FName Id, Icon;
    FString Name, Description, Category;
    int32 Level = 1, Mana = 0;
    float Cooldown = 0.f;
};

struct VORTEXDATA_API FVortexLootEntry
{
    FName Item;
    int32 Min = 1, Max = 1;
    float Chance = 1.f;
};

/** Uma classe do Compêndio v1.2 (o Despertar no nível 10 tira 6 cartas). */
struct VORTEXDATA_API FVortexClassDef
{
    FName Id, Icon;
    FString Name, Rarity, Role, Attribute, Description, PassiveName, PassiveText;
    TArray<FName> Weapons;
    TMap<FName, float> Buffs;          // nos 12 oficiais
    TMap<FName, float> Affinity;       // raça -> peso no sorteio (0 = bloqueada)
    bool bDrawable = true;
    TArray<TPair<FString, FString>> Ascensions;   // nome, descrição (nível 50)
    float AffinityFor(FName Race) const { const float* A = Affinity.Find(Race); return A ? *A : 1.f; }
};

/**
 * Habilidade de classe (data/class_skills.json do Godot): 4 por classe, todas liberam no Despertar.
 * Números como no Godot (metros, segundos, poder); quem usa converte metros em cm (x100).
 * Kind: projectile, wave, cleave, nova, strike, zone, dash, buff, heal, shield, chain, beam, summon.
 */
struct VORTEXDATA_API FVortexClassSkill
{
    FName Id, Owner, Icon, Kind, Element, Anim;
    FString Name, Description;
    int32 Slot = 0, Mana = 15, Count = 1, Pulses = 1;
    float Cooldown = 6.f, Power = 0.f, Radius = 1.2f, Range = 0.f, Duration = 0.f, Delay = 0.f, Stun = 0.f;
    float Speed = 17.f, Spread = 0.f, Explode = 0.f, Heal = 0.f, Shield = 0.f, Lifesteal = 0.f, Land = 0.f;
    bool bPierce = false, bLob = false, bFollow = false;
    float BuffDamage = 0.f, BuffSpeed = 0.f, BuffArmor = 0.f, BuffRegen = 0.f, BuffDuration = 0.f;
    float SlowFactor = 1.f, SlowTime = 0.f, WeakenFactor = 0.f, WeakenTime = 0.f, ExposeFactor = 0.f, ExposeTime = 0.f;
    float DotDps = 0.f, DotTime = 0.f, Knock = 0.f, Taunt = 0.f, Execute = 0.f, Backstab = 0.f;
    float Haste = 0.f, CritBonus = 0.f, ManaGain = 0.f, Deathward = 0.f;
    /** Alcance de verdade (metros), como Main.Reach do Godot: magia de longe. */
    float Reach() const;
    /** Elemento físico (aço, terra, vento, sangue, sombra): técnica de combate, não magia. */
    bool IsPhysical() const;
    /** "OFENSIVA", "ÁREA", "MOBILIDADE"... (o rótulo do Godot). */
    FString Category() const;
};

/** Receita de alquimia ou da bancada (data/crafting.json do Godot). */
struct VORTEXDATA_API FVortexRecipe
{
    FName Id, Output;
    FString Name, Description, Station;
    int32 Count = 1;
    TArray<TPair<FName, int32>> Ingredients;
};

/** Afinidades de combate da raça (1 = humano; Taken < 1 apanha menos). */
struct VORTEXDATA_API FVortexRaceCombat
{
    float Melee = 1.f, Ranged = 1.f, Magic = 1.f, Taken = 1.f, HpRegen = 1.f, MpRegen = 1.f;
    bool bCanMagic = true;
};

/** Como o golpe sai: corpo a corpo (Força), à distância (Destreza) ou magia (Inteligência e Poder de magia). */
enum class EVortexDamageKind : uint8 { Melee, Ranged, Magic };

/** Os números de RPG do herói já somados (raça + pontos nos oficiais + equipamento + classe). */
struct VORTEXDATA_API FVortexHeroStats
{
    int32 Attr[VortexRpg::NumAttributes] = {};
    /** Total de cada atributo oficial (pontos + equipamento + classe), na unidade do item. */
    float Official[VortexRpg::NumOfficial] = {};
    float MaxHealth = 100.f, MaxMana = 60.f;
    float Attack = 0.f, RangedAttack = 0.f, SkillPower = 0.f;
    float Defense = 0.f, Resistance = 0.f;
    float Crit = .02f, CritMult = 1.8f;
    float AttackSpeed = 1.f;                // golpes e animações mais rápidos
    float CooldownScale = 1.f;              // recargas / (1 + pressa%)
    float Lifesteal = 0.f;                  // fração do dano que volta como vida
    float HealPotency = 1.f;                // poções e curas (Inteligência)
    float HpRegen = 0.f, MpRegen = 0.f;     // por segundo
    float LootBonus = 0.f, CoinBonus = 0.f;
    FVortexRaceCombat Race;
    /** O K da defesa no nível do herói (FVortexRpgCatalog::DefenseKAt). */
    float ArmorK = 50.f;
    float MagicReduction() const { return Resistance <= 0.f ? 0.f : Resistance / (Resistance + ArmorK); }
    float PhysicalReduction() const { return Defense <= 0.f ? 0.f : Defense / (Defense + ArmorK); }
};

/**
 * Dados de RPG (Data/Rpg/rpg.json, Tools/BuildRpgData.py a partir do Godot): itens, saque,
 * mochila inicial, XP por criatura, efeitos dos atributos, afinidades das raças e as técnicas
 * de cada arma. Ícones em /Game/Vortex/UI/Icons (Tools/ImportItemIcons.py).
 */
/** Conjunto de armadura: as peças e os bônus de 2 e 4 peças vestidas. */
struct VORTEXDATA_API FVortexItemSet
{
    FName Id;
    FString Name, Text;
    TArray<FName> Pieces;
    TMap<FName, float> Bonus2, Bonus4;
};

class VORTEXDATA_API FVortexRpgCatalog
{
public:
    static const FVortexRpgCatalog& Get();
    TMap<FName, FVortexItemSet> Sets;
    /** Nome de cada elemento ("Fogo", "Água"...). */
    TMap<FName, FString> ElementNames;
    /** Nome de cada peculiaridade ("Sangramento", "Peso"...). */
    TMap<FName, FString> TraitNames;
    /** Lê Data/Rpg/arsenal.json por cima dos itens (no construtor). */
    void LoadArsenal();

    TMap<FName, FVortexItemDef> Items;
    TMap<FName, TArray<FVortexLootEntry>> Loot;
    TArray<FVortexItemStack> StartingBag;
    TMap<FName, int32> XpSources;
    TMap<FName, float> Effects;
    /** Quanto 1 ponto de nível soma em cada oficial (progression.json: official_per_point). */
    float PerPoint[VortexRpg::NumOfficial] = { 1.f, 1.f, 2.f, 6.f, 5.f, .5f, 3.f, 1.5f, 1.5f, 1.f, .25f, 1.f };
    /** Os oito atributos de partida do herói (progression.json: attributes.base), antes da raça. */
    int32 BaseAttr[VortexRpg::NumAttributes] = { 6, 5, 8, 4, 6, 4, 4, 4 };
    /**
     * O balanceamento dos pontos (Data/Rpg/balance.json, 02/11; ver Docs/02_Combate/BALANCEAMENTO.md). Cada oficial com
     * curva vale (Max + MaxPerLevel x nível) x pontos / (pontos + Mid x nível): os primeiros pontos rendem muito e o
     * resto cada vez menos, e como o meio cresce com o nível, a mesma parte dos pontos vale o mesmo no nível 5 e no 100
     * (como o rating do WoW). Sem curva, o linear antigo (PerPoint).
     */
    struct FPointCurve { bool bOn = false; float Max = 0.f, MaxPerLevel = 0.f, Mid = 4.f; };
    FPointCurve PointCurve[VortexRpg::NumOfficial];
    /** Dano que o herói ganha por nível (somado ao da arma, antes do bônus dos pontos). */
    float LevelDamage = 0.f;
    /** Defesa e Resistência dividem o dano: passa K / (K + defesa), com K = DefenseK + DefenseKPerLevel x nível. */
    float DefenseK = 50.f, DefenseKPerLevel = 0.f;
    /** Criaturas: golpe de referência (a vida é golpes x este) e dano, crescendo por nível (ValleyMonster do Godot). */
    float MonsterHitBase = 3.f, MonsterHitPerLevel = 1.3f, MonsterDamagePerLevel = .14f;
    /**
     * Grupo (02/11): cada herói vivo a até GroupRange cm do bicho vencido ganha o XP inteiro (sem dividir, como no
     * Diablo e no Borderlands), com GroupXpBonus a mais por companheiro; e o bicho leva menos dano com mais heróis em
     * volta (a vida vale 1 + GroupHealthPerHero por herói a mais), para a luta em grupo não virar passeio.
     */
    float GroupRange = 4000.f, GroupXpBonus = .1f, GroupHealthPerHero = .5f;
    /** Herói contra herói (duelo e PvP, 02/11): o golpe da ficha de quem bate, a defesa de quem apanha, vezes isto. */
    float PvpDamage = .5f;
    /** Quanto Points pontos num oficial valem para um herói do nível dado. */
    float PointValue(int32 Official, int32 Points, int32 Level) const;
    /** O K da defesa para um nível. */
    float DefenseKAt(int32 Level) const { return FMath::Max(1.f, DefenseK + DefenseKPerLevel * FMath::Max(1, Level)); }
    void LoadBalance();
    TMap<FName, FVortexRaceCombat> Races;
    TMap<FName, TArray<FVortexSkillDef>> Skills;   // família da arma -> 4 técnicas
    TArray<FVortexClassDef> Classes;
    TMap<FString, int32> ClassRarityChance;        // common 58, rare 27, epic 11, legendary 4
    int32 AwakeningLevel = 10, AscensionLevel = 50;
    TArray<FVortexRecipe> Recipes;
    /** As 4 habilidades de cada classe (pela ordem dos espaços 1 a 4). */
    TMap<FName, TArray<FVortexClassSkill>> ClassSkills;
    /** Força das habilidades pela raridade da classe (common 1, rare 1,1, epic 1,17, legendary 1,25). */
    TMap<FString, float> RarityPower;
    const TArray<FVortexClassSkill>* SkillsOf(FName ClassId) const { return ClassSkills.Find(ClassId); }
    const FVortexClassDef* Class(FName Id) const { return Classes.FindByPredicate([Id](const FVortexClassDef& C) { return C.Id == Id; }); }
    const FVortexRecipe* Recipe(FName Id) const { return Recipes.FindByPredicate([Id](const FVortexRecipe& R) { return R.Id == Id; }); }
    /** O Despertar: 6 classes pela chance da raridade e pela afinidade da raça (ClassCatalog.Draw do Godot). */
    TArray<FName> DrawClasses(FRandomStream& Rng, FName Race, FName WeaponType, const TArray<int32>* Points = nullptr) const;
    /** Peso de uma classe no sorteio deste herói (raça, arma na mão e onde pôs os pontos). */
    float ClassWeight(const FVortexClassDef& C, FName Race, FName WeaponType, const TArray<int32>* Points) const;

    /** O item do id; "<id>@<nível>" monta (e guarda) o mesmo item naquele nível. */
    const FVortexItemDef* Item(FName Id) const;
    /** "<id>@<nível>" (só equipamento; o nível de 1 a itemLevel.max). */
    static FName LevelledId(FName Base, int32 Level);
    /** O id sem o "@nível". */
    static FName BaseOf(FName Id);
    /** 07/11: a escala do golpe de cada tipo de arma (balance.json, weaponDamageScale): o golpe multiplica o dano. */
    TMap<FName, float> WeaponDamageScale;
    const FVortexRaceCombat& Race(FName Id) const;
    float Effect(const TCHAR* Key, float Default = 0.f) const;
    /** XP de uma criatura do nível dado (o Godot: 65% do valor da tabela, +8% por nível acima do 1). */
    int64 XpForKill(FName Species, int32 MonsterLevel) const;
    /** Família de arma (as técnicas): a própria arma inicial ou o "family" do item. */
    FName WeaponFamily(FName ItemId) const;
    /** Tipo de arma (um dos 25 do Godot) de um item: o próprio id, ou o "family" do item. */
    FName WeaponType(FName ItemId) const;

    /** XP total para chegar a um nível: 100 x (nível - 1)^1,8 (como no Godot). */
    static int64 XpForLevel(int32 Level);
    static int32 LevelFromXp(int64 Xp);
    static int32 PointsEarned(int32 Level) { return FMath::Max(0, Level - 1) * VortexRpg::PointsPerLevel; }

    /** Ícone de item ou técnica (fica carregado). */
    static UTexture2D* Icon(FName Icon);
    static FLinearColor RarityColor(const FString& Rarity);
    static FString RarityLabel(const FString& Rarity);
    static FString TypeLabel(const FString& Type);
    /** "Ataque +2", "% Crítico +3"... */
    static FString StatLine(FName Stat, float Value);
    static FString AttributeHelp(int32 Attribute);

private:
    FVortexRpgCatalog();
    /** Os números do nível do item (balance.json, itemLevel) e a conta que põe um item num nível. */
    void SetItemLevel(FVortexItemDef& Def, int32 Level) const;
    int32 NaturalLevel(const FString& Rarity) const;
    TArray<float> ItemNatural = { 1, 5, 12, 20, 30, 40, 50 }, ItemAttackBase = { 1, 2, 3, 4, 5, 6, 8 },
        ItemAttackPerLevel = { .25f, .4f, .56f, .71f, .88f, 1.06f, 1.25f }, ItemAttackDefault = { 2, 3, 4, 6, 9, 11, 14 };
    float ItemGrowthPerLevel = .1f, ItemPricePerLevel = .125f;
    int32 ItemMaxLevel = 100;
    mutable TMap<FName, TSharedPtr<FVortexItemDef>> Levelled;   // (compartilhado: a classe exportada precisa ser copiável)
};
