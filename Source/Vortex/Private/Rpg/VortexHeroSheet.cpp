#include "Rpg/VortexHeroSheet.h"

#include "Persistence/VortexWorldSave.h"

#include "Engine/NetConnection.h"

#include "Framework/VortexGameMode.h"
#include "Rpg/VortexArmor.h"
#include "GameFramework/GameStateBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "World/VortexAwakeningStadium.h"
#include "World/VortexArsenal.h"
#include "World/VortexFountain.h"
#include "World/VortexTerrainStreamer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"
#include "Creatures/VortexCreature.h"
#include "Rpg/VortexQuestLog.h"
#include "EngineUtils.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexSpellChoice.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexGameInstance.h"
#include "Framework/VortexLook.h"
#include "Net/UnrealNetwork.h"
#include "Persistence/VortexPlayerSave.h"
#include "Persistence/VortexSaveSubsystem.h"
#include "Creatures/VortexNpc.h"
#include "Framework/VortexGameState.h"
#include "World/VortexDungeons.h"
#include "Rpg/VortexShop.h"
#include "World/VortexItemDrop.h"
#include "Framework/VortexHudBase.h"
#include "GameFramework/PlayerController.h"

namespace
{
    bool Rpg_RaceCanMagic(FName Race) { return FVortexRpgCatalog::Get().Race(Race).bCanMagic; }

    UAbilitySystemComponent* AbilitiesOf(const AActor* Actor)
    {
        const IAbilitySystemInterface* Owner = Cast<IAbilitySystemInterface>(Actor);
        return Owner ? Owner->GetAbilitySystemComponent() : nullptr;
    }

    FString ItemName(FName Item)
    {
        const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Item);
        return Def ? Def->Name : Item.ToString();
    }

    // Avisos: 0 comum, 1 XP, 2 nível, 3 saque, 4 recusado
    enum ENoticeKind : uint8 { Plain, Experience, LevelUp, Loot, Refused };
}

void UVortexHeroSheet::ApplyBurn(float Seconds)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) return;
    const AGameStateBase* State = Owner->GetWorld()->GetGameState();
    const double Now = State ? State->GetServerWorldTimeSeconds() : Owner->GetWorld()->GetTimeSeconds();
    if (VortexArmor::Has(this, TEXT("burn_immune"))) return;   // 07/11: o dragão e o magma não queimam
    if (BurnServerUntil < Now) PushNotice(TEXT("Queimando!"));
    BurnServerUntil = FMath::Max(BurnServerUntil, float(Now + Seconds));
    Owner->ForceNetUpdate();
}

float UVortexHeroSheet::BurnRemaining() const
{
    const UWorld* World = GetWorld();
    const AGameStateBase* State = World ? World->GetGameState() : nullptr;
    const double Now = State ? State->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
    return FMath::Max(0.f, float(BurnServerUntil - Now));
}

void UVortexHeroSheet::ApplyPoison(float Seconds)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) return;
    const AGameStateBase* State = Owner->GetWorld()->GetGameState();
    const double Now = State ? State->GetServerWorldTimeSeconds() : Owner->GetWorld()->GetTimeSeconds();
    if (PoisonServerUntil < Now) PushNotice(TEXT("Envenenado!"));
    PoisonServerUntil = FMath::Max(PoisonServerUntil, float(Now + Seconds));
    Owner->ForceNetUpdate();
}

float UVortexHeroSheet::PoisonRemaining() const
{
    const UWorld* World = GetWorld();
    const AGameStateBase* State = World ? World->GetGameState() : nullptr;
    const double Now = State ? State->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
    return FMath::Max(0.f, float(PoisonServerUntil - Now));
}

void UVortexHeroSheet::ApplyBlessing(FName Kind, float Seconds)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) return;
    const AGameStateBase* State = Owner->GetWorld()->GetGameState();
    const double Now = State ? State->GetServerWorldTimeSeconds() : Owner->GetWorld()->GetTimeSeconds();
    Blessing = Kind;
    BlessingServerUntil = float(Now + Seconds);
    bBlessingActive = true;
    Changed();
    Owner->ForceNetUpdate();
}

float UVortexHeroSheet::BlessingRemaining() const
{
    const UWorld* World = GetWorld();
    const AGameStateBase* State = World ? World->GetGameState() : nullptr;
    const double Now = State ? State->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
    return Blessing.IsNone() ? 0.f : FMath::Max(0.f, float(BlessingServerUntil - Now));
}

UVortexHeroSheet::UVortexHeroSheet()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
    EnsureSizes();
}

void UVortexHeroSheet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UVortexHeroSheet, Level);
    DOREPLIFETIME(UVortexHeroSheet, DrinkItem);
    DOREPLIFETIME(UVortexHeroSheet, DrinkStartServer);
    DOREPLIFETIME_CONDITION(UVortexHeroSheet, Xp, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVortexHeroSheet, Points, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVortexHeroSheet, Bag, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVortexHeroSheet, Equipped, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVortexHeroSheet, Coins, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVortexHeroSheet, bServerLoaded, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVortexHeroSheet, bNeedsAwakening, COND_OwnerOnly);
    DOREPLIFETIME(UVortexHeroSheet, ClassId);
    DOREPLIFETIME_CONDITION(UVortexHeroSheet, StaffSpell, COND_OwnerOnly);
    DOREPLIFETIME(UVortexHeroSheet, MountName);
    DOREPLIFETIME_CONDITION(UVortexHeroSheet, AskMountSerial, COND_OwnerOnly);
    DOREPLIFETIME(UVortexHeroSheet, OutlawUntilDay);
    DOREPLIFETIME_CONDITION(UVortexHeroSheet, ClassCards, COND_OwnerOnly);
    DOREPLIFETIME(UVortexHeroSheet, BurnServerUntil);
    DOREPLIFETIME(UVortexHeroSheet, PoisonServerUntil);
    DOREPLIFETIME(UVortexHeroSheet, Blessing);
    DOREPLIFETIME(UVortexHeroSheet, DungeonId);
    DOREPLIFETIME(UVortexHeroSheet, DungeonRules);
    DOREPLIFETIME(UVortexHeroSheet, BlessingServerUntil);
    DOREPLIFETIME_CONDITION(UVortexHeroSheet, ShopDay, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVortexHeroSheet, ShopBought, COND_OwnerOnly);
}

void UVortexHeroSheet::EnsureSizes()
{
    Points.SetNumZeroed(VortexRpg::NumGroups);
    Bag.SetNum(VortexRpg::BagSize);
    Equipped.SetNum(VortexRpg::NumEquip);
}

bool UVortexHeroSheet::IsOwnerLocal() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    return Pawn && Pawn->IsLocallyControlled() && Pawn->IsPlayerControlled();
}

FName UVortexHeroSheet::GetRace() const
{
    const AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
    return Hero ? Hero->GetLook().Race : FName(TEXT("humano"));
}

int32 UVortexHeroSheet::GetPointsFree() const
{
    int32 Spent = 0;
    for (const int32 P : Points) Spent += FMath::Max(0, P);
    return FMath::Max(0, FVortexRpgCatalog::PointsEarned(Level) - Spent);
}

int32 UVortexHeroSheet::CountOf(FName Item) const
{
    int32 Count = 0;
    for (const FVortexItemStack& Stack : Bag)
    {
        if (Stack.Item == Item) Count += Stack.Count;
    }
    return Count;
}

FName UVortexHeroSheet::GetWeaponFamily() const
{
    return FVortexRpgCatalog::Get().WeaponFamily(GetEquipped(VortexRpg::Weapon));
}

FVortexHeroStats UVortexHeroSheet::ComputeStats() const
{
    const FVortexRpgCatalog& Rpg = FVortexRpgCatalog::Get();
    FVortexHeroStats S;
    using namespace VortexRpg;
    // os oito da raça (o Godot: base + raça + classe); os pontos de nível não vão mais para eles
    const FVortexRaceInfo* RaceInfo = FVortexLookCatalog::Get().Race(GetRace());
    for (int32 i = 0; i < NumAttributes; ++i)
    {
        S.Attr[i] = RaceInfo && RaceInfo->Start.IsValidIndex(i) ? RaceInfo->Start[i] : Rpg.BaseAttr[i];
    }
    // os 12 oficiais: pontos de nível + equipamento (mesmas chaves) + o que a classe der
    // os pontos pela curva do nível (Data/Rpg/balance.json): rendem cada vez menos no mesmo atributo
    // 10/11: os pontos são dos 8 atributos; cada oficial pega os pontos do atributo dele
    for (int32 i = 0; i < NumOfficial; ++i) S.Official[i] = Rpg.PointValue(i, GetPoints(GroupOf(i)), GetLevel());
    S.ArmorK = Rpg.DefenseKAt(GetLevel());
    for (const FName& Item : Equipped)
    {
        if (const FVortexItemDef* Def = Rpg.Item(Item))
        {
            for (const TPair<FName, float>& Stat : Def->Stats)
            {
                const int32 Index = OfficialOf(Stat.Key);
                if (Index != INDEX_NONE) S.Official[Index] += Stat.Value;
            }
        }
    }
    // conjuntos de armadura: 2 e 4 peças do mesmo conjunto vestidas
    TMap<FName, int32> SetCount;
    for (const FName& Item : Equipped)
        if (const FVortexItemDef* Def = Rpg.Item(Item); Def && !Def->Set.IsNone()) SetCount.FindOrAdd(Def->Set)++;
    for (const TPair<FName, int32>& Count : SetCount)
    {
        const FVortexItemSet* Set = Rpg.Sets.Find(Count.Key);
        if (!Set) continue;
        auto Add = [&S](const TMap<FName, float>& Bonus)
        {
            for (const TPair<FName, float>& Stat : Bonus)
            {
                const int32 Index = OfficialOf(Stat.Key);
                if (Index != INDEX_NONE) S.Official[Index] += Stat.Value;
            }
        };
        if (Count.Value >= 2) Add(Set->Bonus2);
        if (Count.Value >= 4) Add(Set->Bonus4);
    }
    AddClassBonus(S);
    auto Over = [&S, &Rpg](int32 Attr) { return float(S.Attr[Attr] - Rpg.BaseAttr[Attr]); };
    auto E = [&Rpg](const TCHAR* Key, float Default) { return Rpg.Effect(Key, Default); };
    S.Race = Rpg.Race(GetRace());
    // vida 100 e mana 60 de base (PlayerController.BaseMaxHp/BaseMaxMp), mais Vitalidade/Mana da raça
    S.MaxHealth = FMath::Max(20.f, 100.f + Over(Vitality) * E(TEXT("health_per_vitality"), 6.f) + S.Official[MaxHealthStat]);
    S.MaxMana = FMath::Max(10.f, 60.f + Over(ManaAttr) * E(TEXT("mana_per_mana"), 5.f) + S.Official[MaxManaStat]);
    // golpe: Força (corpo a corpo), Destreza (flechas), Inteligência e Mana (magia) + o Ataque / Poder de magia
    S.Attack = 1.f + Over(Strength) * E(TEXT("damage_per_strength"), 1.f) + S.Official[Attack];
    S.RangedAttack = 1.f + FMath::Max(0.f, Over(Agility)) * E(TEXT("damage_per_strength"), 1.f) + S.Official[Attack];
    S.SkillPower = 1.f + FMath::Max(0.f, Over(Intelligence)) + FMath::RoundToFloat(FMath::Max(0.f, Over(ManaAttr)) * .35f) + S.Official[Skill];
    S.Defense = S.Official[Defense];
    S.Resistance = S.Official[Resistance];
    const float LuckPoints = FMath::Max(0.f, Over(Luck));
    S.Crit = FMath::Min(.6f, .02f + LuckPoints * E(TEXT("crit_chance_per_luck"), .005f) +
        Over(Agility) * E(TEXT("crit_chance_per_agility"), .0075f) + S.Official[Crit] / 100.f);
    S.CritMult = E(TEXT("crit_multiplier"), 1.8f) + S.Official[CritDmg] / 100.f;
    S.AttackSpeed = 1.f + FMath::Clamp(S.Official[Speed], 0.f, 100.f) / 100.f;
    S.CooldownScale = 1.f / (1.f + FMath::Max(0.f, S.Official[Haste]) / 100.f);
    S.Lifesteal = FMath::Clamp(S.Official[Lifesteal], 0.f, MaxLifesteal) / 100.f;
    S.HealPotency = 1.f + S.Attr[Intelligence] * E(TEXT("heal_percent_per_intelligence"), .03f);
    S.HpRegen = S.Race.HpRegen * (E(TEXT("health_regen_base"), .35f) + FMath::Max(0.f, Over(Vitality)) * E(TEXT("health_regen_per_vitality"), .03f) +
        FMath::Max(0.f, Over(Recovery)) * E(TEXT("health_regen_per_recovery"), .1f) + S.Official[HpRegenStat] / 10.f);
    // 03/11: os pontos em Mana também regeneram: +0,04 de mana por segundo para cada ponto de mana máxima que eles dão
    S.MpRegen = (1.1f + S.Attr[Spirit] * E(TEXT("mana_regen_per_spirit"), .025f) + S.Official[MaxManaStat] * E(TEXT("mana_regen_per_mana"), .04f)) * S.Race.MpRegen;
    S.LootBonus = LuckPoints * E(TEXT("loot_chance_per_luck"), .06f);
    S.CoinBonus = LuckPoints * E(TEXT("coins_per_luck"), .05f) + VortexArmor::Sum(this, TEXT("coins"), TEXT("amount"));
    // bênção do santuário (pontos de interesse), enquanto durar
    if (const FName Blessed = GetBlessing(); !Blessed.IsNone())
    {
        if (Blessed == TEXT("forca")) S.Attack += FMath::Max(3.f, S.Attack * .25f);
        else if (Blessed == TEXT("vida")) S.MaxHealth = FMath::RoundToFloat(S.MaxHealth * 1.25f);
        else if (Blessed == TEXT("sorte")) { S.Crit += .1f; S.LootBonus += .25f; S.CoinBonus += .25f; }
    }
    // regras da dungeon do portal
    if (HasDungeonRule(VortexDungeonRules::Fragile)) S.MaxHealth = FMath::RoundToFloat(S.MaxHealth * .75f);
    if (HasDungeonRule(VortexDungeonRules::Luck)) { S.LootBonus += .5f; S.CoinBonus += .5f; }
    return S;
}

void UVortexHeroSheet::AddClassBonus(FVortexHeroStats& Stats) const
{
    // bônus permanente da classe nos oficiais (ClassCatalog.OfficialBonus do Godot)
    if (const FVortexClassDef* Class = FVortexRpgCatalog::Get().Class(ClassId))
    {
        for (const TPair<FName, float>& Buff : Class->Buffs)
        {
            const int32 Index = VortexRpg::OfficialOf(Buff.Key);
            if (Index != INDEX_NONE) Stats.Official[Index] += Buff.Value;
        }
    }
}

bool UVortexHeroSheet::CanAwaken() const
{
    return ClassId.IsNone() && Level >= FVortexRpgCatalog::Get().AwakeningLevel;
}

bool UVortexHeroSheet::CanCraft(FName RecipeId) const
{
    const FVortexRecipe* Recipe = FVortexRpgCatalog::Get().Recipe(RecipeId);
    if (!Recipe) return false;
    for (const TPair<FName, int32>& Ingredient : Recipe->Ingredients) if (CountOf(Ingredient.Key) < Ingredient.Value) return false;
    return true;
}

void UVortexHeroSheet::ServerDrawClasses_Implementation()
{
    if (!CanAwaken() || ClassCards.Num()) return;
    // a semente vem do herói e do momento: as cartas ficam guardadas (fechar e abrir não troca)
    FRandomStream Rng(int32(GetTypeHash(GetOwner()->GetName()) ^ uint32(FPlatformTime::Cycles())));
    ClassCards = FVortexRpgCatalog::Get().DrawClasses(Rng, GetRace(), GetWeaponType(), &Points);
    Notice(TEXT("O Despertar: escolha uma das seis classes"), 5);
    VortexAudio::Play(this, TEXT("level_up"), GetOwner()->GetActorLocation(), .8f, 0.f);
    Changed();
}

void UVortexHeroSheet::ServerChooseClass_Implementation(FName Class)
{
    const FVortexClassDef* Def = FVortexRpgCatalog::Get().Class(Class);
    if (!Def || !ClassId.IsNone() || !ClassCards.Contains(Class)) return;
    ClassId = Class;
    ClassCards.Reset();
    Notice(FString::Printf(TEXT("Você despertou como %s"), *Def->Name), 5);
    VortexAudio::Play(this, TEXT("holy_cast"), GetOwner()->GetActorLocation(), 1.f, 0.f);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_CLASS %s escolheu %s"), *GetNameSafe(GetOwner()), *Class.ToString());
    Changed(true);
}

void UVortexHeroSheet::AskMountName()
{
    MountName.Reset();
    ++AskMountSerial;
    // o anfitrião não recebe a réplica: abre aqui mesmo
    if (const APawn* Pawn = Cast<APawn>(GetOwner()); Pawn && Pawn->IsLocallyControlled()) OnRep_AskMountName();
}

void UVortexHeroSheet::OnRep_AskMountName()
{
    AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
    if (Hero && Hero->IsLocallyControlled() && AskMountSerial > 0) VortexSpellChoice::OpenHorseName(Hero);
}

void UVortexHeroSheet::ServerSetMountName_Implementation(const FString& Name)
{
    // só letras, números e espaço; até 20
    FString Clean;
    for (const TCHAR C : Name.TrimStartAndEnd()) if (FChar::IsAlnum(C) || C == TEXT(' ') || C == TEXT('-')) Clean.AppendChar(C);
    Clean = Clean.Left(20).TrimEnd();
    if (MountCoat.IsNone() || Clean.IsEmpty()) return;
    MountName = Clean;
    Notice(FString::Printf(TEXT("Seu cavalo agora se chama %s"), *Clean), 3);
    Changed(true);
}

void UVortexHeroSheet::ServerChooseStaffSpell_Implementation(FName Spell)
{
    // 02/11: a magia do cajado é escolhida uma vez (o usuário: "só pode ficar com uma")
    if (!StaffSpell.IsNone() || !(Spell == TEXT("heal") || Spell == TEXT("vines") || Spell == TEXT("fireball"))) return;
    StaffSpell = Spell;
    Notice(FString::Printf(TEXT("Magia do cajado: %s"), Spell == TEXT("heal") ? TEXT("Cura") : Spell == TEXT("vines") ? TEXT("Prisão de Vinhas") : TEXT("Bola de Fogo")), 5);
    VortexAudio::Play(this, TEXT("holy_cast"), GetOwner()->GetActorLocation(), .8f, 0.f);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_STAFF_SPELL %s escolheu %s"), *GetNameSafe(GetOwner()), *Spell.ToString());
    Changed(true);
}

bool UVortexHeroSheet::IsOutlaw() const
{
    const AVortexGameState* State = GetWorld() ? GetWorld()->GetGameState<AVortexGameState>() : nullptr;
    return State && OutlawUntilDay > State->GetDayNumber() + State->GetHour() / 24.f;
}

void UVortexHeroSheet::MarkOutlaw()
{
    const AVortexGameState* State = GetWorld() ? GetWorld()->GetGameState<AVortexGameState>() : nullptr;
    if (!State || !GetOwner()->HasAuthority()) return;
    OutlawUntilDay = State->GetDayNumber() + State->GetHour() / 24.f + 3.f;
    Notice(TEXT("Você matou um herói fora do seu grupo: caveira por 3 dias"), 4);
    GetOwner()->ForceNetUpdate();
    Changed(true);
}

void UVortexHeroSheet::ServerCraft_Implementation(FName RecipeId)
{
    const FVortexRecipe* Recipe = FVortexRpgCatalog::Get().Recipe(RecipeId);
    if (!Recipe || !CanCraft(RecipeId)) { Notice(TEXT("Faltam ingredientes"), Refused); return; }
    for (const TPair<FName, int32>& Ingredient : Recipe->Ingredients) TakeItem(Ingredient.Key, Ingredient.Value);
    const int32 Left = GiveItem(Recipe->Output, Recipe->Count, false);
    if (Left > 0) Notice(TEXT("Mochila cheia: parte se perdeu"), Refused);
    Notice(FString::Printf(TEXT("Preparou %s  x%d"), *Recipe->Name, Recipe->Count - Left), Loot);
    VortexAudio::Play(this, Recipe->Station.Contains(TEXT("alquimia")) ? TEXT("drink") : TEXT("hammer_hit"), GetOwner()->GetActorLocation(), .7f);
    Changed();
}

// ---- servidor --------------------------------------------------------------------------------

void UVortexHeroSheet::AddXp(int64 Amount)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0) return;
    if (const float More = VortexArmor::Sum(this, TEXT("xp"), TEXT("amount")); More > 0.f) Amount = FMath::RoundToInt64(Amount * (1.0 + More));
    Xp = FMath::Clamp<int64>(Xp + Amount, 0, 2000000000);
    Notice(FString::Printf(TEXT("+%lld XP"), Amount), Experience);
    const int32 NewLevel = FVortexRpgCatalog::LevelFromXp(Xp);
    if (NewLevel > Level)
    {
        Level = NewLevel;
        Notice(FString::Printf(TEXT("Nível %d!  +%d pontos de atributo"), Level, VortexRpg::PointsPerLevel), LevelUp);
        MulticastLevelUp(Level);
        // nível do Despertar: as seis cartas saem na hora (a cena abre quando não houver luta por perto)
        if (CanAwaken() && ClassCards.IsEmpty()) ServerDrawClasses_Implementation();
        Changed(true);
        return;
    }
    Changed();
}

int32 UVortexHeroSheet::GiveItem(FName Item, int32 Count, bool bNotify)
{
    const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Item);
    if (!Def || Count <= 0 || !GetOwner() || !GetOwner()->HasAuthority()) return Count;
    EnsureSizes();
    int32 Left = Count;
    for (FVortexItemStack& Stack : Bag)
    {
        if (Left <= 0) break;
        if (Stack.Item != Item || Stack.Count >= Def->Stack) continue;
        const int32 Add = FMath::Min(Left, Def->Stack - Stack.Count);
        Stack.Count += Add;
        Left -= Add;
    }
    for (FVortexItemStack& Stack : Bag)
    {
        if (Left <= 0) break;
        if (!Stack.IsEmpty()) continue;
        Stack.Item = Item;
        Stack.Count = FMath::Min(Left, Def->Stack);
        Left -= Stack.Count;
    }
    if (bNotify && Left < Count) Notice(FString::Printf(TEXT("%s  x%d"), *Def->Name, Count - Left), Loot);
    if (Left > 0 && bNotify) Notice(TEXT("Mochila cheia"), Refused);
    Changed();
    return Left;
}

int32 UVortexHeroSheet::TakeItem(FName Item, int32 Count)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Count <= 0) return 0;
    int32 Taken = 0;
    // das últimas pilhas para as primeiras (as da frente da mochila ficam)
    for (int32 i = Bag.Num() - 1; i >= 0 && Taken < Count; --i)
    {
        FVortexItemStack& Stack = Bag[i];
        if (Stack.Item != Item) continue;
        const int32 Take = FMath::Min(Stack.Count, Count - Taken);
        Stack.Count -= Take;
        Taken += Take;
        if (Stack.Count <= 0) Stack = FVortexItemStack();
    }
    if (Taken > 0) Changed();
    return Taken;
}

bool UVortexHeroSheet::SpendCoins(int32 Amount)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0 || Coins < Amount) return false;
    Coins -= Amount;
    Notice(FString::Printf(TEXT("-%d moedas"), Amount), Loot);
    Changed();
    return true;
}

void UVortexHeroSheet::AddCoins(int32 Amount)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0) return;
    Coins = FMath::Clamp(Coins + Amount, 0, 999999999);
    Notice(FString::Printf(TEXT("+%d moedas"), Amount), Loot);
    Changed();
}

float UVortexHeroSheet::OutgoingDamage(float Base, EVortexDamageKind Kind, bool& bOutCrit, bool bForceCrit) const
{
    const FVortexHeroStats S = ComputeStats();
    // o Godot: corpo a corpo soma Força+Ataque, flechas Destreza+Ataque, magia Inteligência+Poder de magia
    const float Bonus = Kind == EVortexDamageKind::Magic ? S.SkillPower : Kind == EVortexDamageKind::Ranged ? S.RangedAttack : S.Attack;
    const float Race = Kind == EVortexDamageKind::Magic ? S.Race.Magic : Kind == EVortexDamageKind::Ranged ? S.Race.Ranged : S.Race.Melee;
    // o dano do nível (balance.json: levelDamage) soma ao da arma, como o golpe de referência das criaturas
    float Damage = (Base + Bonus + FVortexRpgCatalog::Get().LevelDamage * (GetLevel() - 1)) * FMath::Max(.2f, Race);
    bOutCrit = bForceCrit || FMath::FRand() < S.Crit;
    if (bOutCrit) Damage *= S.CritMult;
    return FMath::Max(1.f, FMath::RoundToFloat(Damage));
}

float UVortexHeroSheet::IncomingDamage(float Base, bool bMagic)
{
    SinceDamaged = 0.f;
    const FVortexHeroStats S = ComputeStats();
    // 02/11: a Defesa e a Resistência dividem o golpe (passa K / (K + defesa), K pelo nível: balance.json), como a
    // armadura do LoL: nunca fica imune e nunca vira um golpe só. (Antes a Defesa tirava direto do golpe.)
    const float Mitigated = Base * (1.f - (bMagic ? S.MagicReduction() : S.PhysicalReduction()));
    return FMath::Max(1.f, FMath::RoundToFloat(FMath::Max(1.f, Mitigated) * S.Race.Taken));
}

void UVortexHeroSheet::Leech(float DamageDealt)
{
    UAbilitySystemComponent* Abilities = AbilitiesOf(GetOwner());
    if (!Abilities || !GetOwner()->HasAuthority() || DamageDealt <= 0.f) return;
    const FVortexHeroStats S = ComputeStats();
    const float Health = Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
    if (S.Lifesteal <= 0.f || Health <= 0.f) return;
    Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), FMath::Min(S.MaxHealth, Health + DamageDealt * S.Lifesteal));
}

void UVortexHeroSheet::AwardKill(AActor* Killer, AActor* Victim)
{
    if (!Killer || !Victim || !Victim->HasAuthority() || !Killer->IsA<AVortexCharacter>()) return;
    const IVortexCreature* Creature = Cast<IVortexCreature>(Victim);
    if (!Creature) return;
    const FVortexRpgCatalog& Rpg = FVortexRpgCatalog::Get();
    // as tabelas do Godot usam a espécie em minúsculas ("wolf", "slime", "runecolossus")
    const FName Species = FName(*Creature->GetSpecies().ToString().ToLower());
    const int32 MonsterLevel = Creature->GetCreatureLevel();
    // semi-chefe da região: 5x o XP, o saque sorteado 3 vezes e 5x as moedas
    const bool bElite = Creature->IsElite();
    // missões "derrote": conta para todos os heróis perto (o grupo inteiro avança)
    for (TActorIterator<AVortexCharacter> It(Victim->GetWorld()); It; ++It)
    {
        if (*It == Killer || FVector::Dist(It->GetActorLocation(), Victim->GetActorLocation()) < 4000.f)
        {
            if (UVortexQuestLog* Quests = It->FindComponentByClass<UVortexQuestLog>()) Quests->OnKill(Creature->GetSpecies(), Creature->IsBoss());
        }
    }
    // XP para todos os heróis vivos perto (cooperativo: quem ajudou também aprende). 02/11: o XP inteiro para cada um
    // (sem dividir), mais um bônus por companheiro (balance.json: groupRange, groupXpBonus)
    const float Range = Rpg.GroupRange;
    auto Alive = [](AVortexCharacter* Hero)
    {
        const UAbilitySystemComponent* A = AbilitiesOf(Hero);
        return A && A->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) > 0.f;
    };
    int32 Group = 0;
    for (TActorIterator<AVortexCharacter> It(Victim->GetWorld()); It; ++It)
        if (Alive(*It) && FVector::Dist(It->GetActorLocation(), Victim->GetActorLocation()) <= Range) ++Group;
    const double GroupBonus = 1.0 + Rpg.GroupXpBonus * FMath::Max(0, Group - 1);
    for (TActorIterator<AVortexCharacter> It(Victim->GetWorld()); It; ++It)
    {
        UVortexHeroSheet* Sheet = It->FindComponentByClass<UVortexHeroSheet>();
        const UAbilitySystemComponent* Abilities = AbilitiesOf(*It);
        if (!Sheet || !Abilities || Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()) <= 0.f) continue;
        if (*It != Killer && FVector::Dist(It->GetActorLocation(), Victim->GetActorLocation()) > Range) continue;
        // diferença de nível (o Godot): até 3 abaixo vale cheio, depois cai 8% por nível; acima, +4%
        const int32 Diff = MonsterLevel - Sheet->GetLevel();
        const double Factor = Diff >= 0 ? 1.0 + .04 * FMath::Min(Diff, 10) : Diff >= -3 ? 1.0 : FMath::Max(.1, 1.0 + .08 * (Diff + 3));
        // Progressao inicial mais rapida: +25% no nivel 1, reduzindo ate o normal no nivel 10.
        const double EarlyXpBonus = FMath::Lerp(1.25, 1.0, FMath::Clamp((Sheet->GetLevel() - 1) / 9.0, 0.0, 1.0));
        const double Blessed = Sheet->HasDungeonRule(VortexDungeonRules::Blessing) ? 1.5 : 1.0;
        Sheet->AddXp(FMath::Max<int64>(1, FMath::RoundToInt64(Rpg.XpForKill(Species, MonsterLevel) * Factor * EarlyXpBonus * (bElite ? 5.0 : 1.0) * Blessed * GroupBonus)));
        // eco dos heróis: cada vitória devolve 8% da vida
        if (Sheet->HasDungeonRule(VortexDungeonRules::Echo)) Sheet->HealBy(Sheet->ComputeStats().MaxHealth * .08f);
    }
    // saque e moedas: para quem venceu (a Sorte aumenta as chances)
    if (UVortexHeroSheet* Sheet = Killer->FindComponentByClass<UVortexHeroSheet>())
    {
        const FVortexHeroStats S = Sheet->ComputeStats();
        if (const TArray<FVortexLootEntry>* Table = Rpg.Loot.Find(Species))
        {
            for (int32 Roll = 0; Roll < (bElite ? 3 : 1); ++Roll)
            for (const FVortexLootEntry& Entry : *Table)
            {
                if (FMath::FRand() < FMath::Min(1.f, Entry.Chance * (1.f + S.LootBonus)))
                {
                    // 07/11: o equipamento cai no nível do bicho (um pouco para cima ou para baixo), como nos jogos de saque;
                    // e cai no chão, rodando (AVortexItemDrop): o E pega
                    const FVortexItemDef* Drop = Rpg.Item(Entry.Item);
                    const FName Item = Drop && Drop->IsEquipment() ? FVortexRpgCatalog::LevelledId(Entry.Item, MonsterLevel + FMath::RandRange(-2, 1)) : Entry.Item;
                    AVortexItemDrop::Spawn(Victim->GetWorld(), Item, FMath::RandRange(Entry.Min, FMath::Max(Entry.Min, Entry.Max)), Victim->GetActorLocation());
                }
            }
        }
        Sheet->AddCoins(FMath::RoundToInt(FMath::RandRange(2, 5 + MonsterLevel) * (1.f + S.CoinBonus) * (bElite ? 5.f : 1.f)));
    }
}

void UVortexHeroSheet::ServerPickup_Implementation(AActor* Target)
{
    AVortexItemDrop* Drop = Cast<AVortexItemDrop>(Target);
    const AActor* Owner = GetOwner();
    if (!bLoaded || !Drop || !Owner || FVector::Dist2D(Drop->GetActorLocation(), Owner->GetActorLocation()) > 6000.f) return;
    if (!Drop->IsNear(Owner->GetActorLocation(), 400.f)) return;   // este item, perto (com folga da rede)
    const int32 Want = Drop->GetCount();
    const int32 Left = GiveItem(Drop->GetItem(), Want, true);
    if (Left >= Want) { Notice(TEXT("Mochila cheia."), Refused); return; }
    VortexAudio::Play(this, TEXT("equip"), Owner->GetActorLocation(), .6f);
    Drop->Take(Want - Left);
}

void UVortexHeroSheet::ServerDropSlot_Implementation(uint8 BagIndex)
{
    AActor* Owner = GetOwner();
    if (!bLoaded || !Owner || !Bag.IsValidIndex(BagIndex) || Bag[BagIndex].IsEmpty()) return;
    const FVortexItemStack Stack = Bag[BagIndex];
    if (!AVortexItemDrop::Spawn(Owner->GetWorld(), Stack.Item, Stack.Count, Owner->GetActorLocation() + Owner->GetActorForwardVector() * 90.f, .6f)) return;
    Bag[BagIndex] = FVortexItemStack();
    Changed();
}

void UVortexHeroSheet::MulticastLevelUp_Implementation(int32 NewLevel)
{
    if (const AActor* Owner = GetOwner()) VortexAudio::Play(this, TEXT("level_up"), Owner->GetActorLocation(), 1.f, 0.f);
}

void UVortexHeroSheet::Notice(const FString& Text, uint8 Kind)
{
    if (IsOwnerLocal())
    {
        Notices.Add({ Text, GetWorld() ? float(GetWorld()->GetTimeSeconds()) : 0.f, Kind });
        if (Notices.Num() > 8) Notices.RemoveAt(0);
        if (Kind == Loot) VortexAudio::Play2D(this, TEXT("pickup"), .6f);
    }
    else if (GetOwner() && GetOwner()->HasAuthority()) ClientNotice(Text, Kind);
}

void UVortexHeroSheet::ClientNotice_Implementation(const FString& Text, uint8 Kind)
{
    Notices.Add({ Text, GetWorld() ? float(GetWorld()->GetTimeSeconds()) : 0.f, Kind });
    if (Notices.Num() > 8) Notices.RemoveAt(0);
    if (Kind == Loot) VortexAudio::Play2D(this, TEXT("pickup"), .6f);
}

void UVortexHeroSheet::SetDungeon(FName Id, int32 Rules)
{
    if (DungeonId == Id && DungeonRules == Rules) return;
    DungeonId = Id;
    DungeonRules = Id.IsNone() ? 0 : Rules;
    Changed();
    GetOwner()->ForceNetUpdate();
}

void UVortexHeroSheet::Changed(bool bRefill)
{
    ++Revision;
    ApplyVitals(bRefill);
    EquipWeapon();
    if (const AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner()); Hero && Hero->HasAuthority() && Hero->GetWeapon())
    {
        const FVortexHeroStats S = ComputeStats();
        Hero->GetWeapon()->SetCombatPace(S.AttackSpeed, S.CooldownScale);
    }
    if (IsOwnerLocal()) bSaveDirty = true;
}

void UVortexHeroSheet::EquipWeapon()
{
    // servidor: a arma da ficha vira a arma na mão (golpes, modelo e postura do tipo dela)
    AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner());
    UVortexWeaponComponent* Weapon = Hero ? Hero->GetWeapon() : nullptr;
    if (!Weapon || !Hero->HasAuthority()) return;
    const FName Type = GetWeaponType();
    // 07/11: o mesmo tipo (uma espada por outra): o golpe não muda, mas o modelo sim (cada raridade tem os seus)
    if (Type == EquippedType) { Weapon->RefreshVisual(); return; }
    if (Type.IsNone())
    {
        // sem arma (no Estádio, antes de escolher)
        Weapon->Equip(nullptr);
        EquippedType = Type;
        return;
    }
    UVortexWeaponDefinition* Definition = UVortexWeaponComponent::LoadType(Type);
    if (Definition && Weapon->Equip(Definition))
    {
        EquippedType = Type;
        UE_LOG(LogTemp, Display, TEXT("VORTEX_WEAPON equipada %s"), *Type.ToString());
    }
}

FName UVortexHeroSheet::GetWeaponType() const
{
    const FName Item = GetEquipped(VortexRpg::Weapon);
    if (Item.IsNone()) return bNeedsAwakening ? NAME_None : FName(TEXT("espada_de_cinzas"));
    return FVortexRpgCatalog::Get().WeaponType(Item);
}

// ---- Estádio do Despertar ----------------------------------------------------------------------

namespace
{
    /** A arma comum de cada tipo (a do pedestal): Data/Rpg/weapons.json. */
    FName StarterItem(FName Type)
    {
        static TMap<FName, FName> Items;
        if (Items.IsEmpty())
        {
            FString Text;
            TSharedPtr<FJsonObject> Json;
            if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Rpg/weapons.json"))) &&
                FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) && Json)
            {
                for (const TSharedPtr<FJsonValue>& V : Json->GetArrayField(TEXT("types")))
                {
                    const TSharedPtr<FJsonObject> T = V->AsObject();
                    Items.Add(FName(*T->GetStringField(TEXT("id"))), FName(*T->GetStringField(TEXT("item"))));
                }
            }
        }
        const FName* Found = Items.Find(Type);
        return Found ? *Found : Type;
    }
}

void UVortexHeroSheet::EnterStadium()
{
    AActor* Owner = GetOwner();
    AVortexAwakeningStadium* Stadium = Owner ? AVortexAwakeningStadium::Ensure(Owner->GetWorld()) : nullptr;
    if (!Stadium || AVortexAwakeningStadium::Contains(Owner->GetActorLocation())) return;
    ReturnLocation = Owner->GetActorLocation();
    // 08/11: cada um num ponto da faixa de chegada (todos nasciam um dentro do outro)
    Owner->SetActorLocation(Stadium->SpawnPoint() + FVector(FMath::FRandRange(-280.f, 280.f), FMath::FRandRange(-90.f, 90.f), 0.f), false, nullptr, ETeleportType::TeleportPhysics);
    // olhando para o norte (o título do estádio e a Ala Marcial)
    if (APawn* Pawn = Cast<APawn>(Owner))
    {
        if (AController* Controller = Pawn->GetController()) Controller->SetControlRotation(FRotator(-8.f, 90.f, 0.f));
    }
    Notice(TEXT("Estádio do Despertar: escolha uma arma nos pedestais (E) e teste nos bonecos"), Plain);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_STADIUM %s entrou"), *GetNameSafe(Owner));
}

void UVortexHeroSheet::ServerStadiumTake_Implementation(FName Type)
{
    AActor* Owner = GetOwner();
    const AVortexAwakeningStadium* Stadium = Owner ? AVortexAwakeningStadium::Find(Owner->GetWorld()) : nullptr;
    // a escolha vem do pedestal mais perto no computador do jogador; aqui só confere que ele está
    // perto daquele pedestal (com folga: a posição no servidor pode estar um pouco atrás)
    const AVortexAwakeningStadium::FPedestal* Pedestal = Stadium ? Stadium->FindPedestal(Type) : nullptr;
    if (!bNeedsAwakening || !Pedestal || FVector::Dist2D(Owner->GetActorLocation(), Pedestal->Where) > 400.f)
    {
        UE_LOG(LogTemp, Display, TEXT("VORTEX_STADIUM pegar %s recusado (longe ou sem pedestal)"), *Type.ToString());
        return;
    }
    EnsureSizes();
    if (GetWeaponType() == Type)
    {
        Equipped[VortexRpg::Weapon] = NAME_None;
        Notice(FString::Printf(TEXT("%s devolvida ao pedestal"), Pedestal->Definition.IsValid() ? *Pedestal->Definition->DisplayName : *Type.ToString()), Plain);
        Changed();
        return;
    }
    const UVortexWeaponDefinition* Definition = Pedestal->Definition.Get();
    if (!Definition || !Definition->bPlayable)
    {
        Notice(TEXT("Esta arma ainda não chegou ao Vortex"), Refused);
        return;
    }
    if (Definition->bMagic && !Rpg_RaceCanMagic(GetRace()))
    {
        Notice(TEXT("Anões não conseguem usar armas de magia"), Refused);
        return;
    }
    Equipped[VortexRpg::Weapon] = StarterItem(Type);
    Notice(FString::Printf(TEXT("%s: clique esquerdo = %s  •  botão direito = %s"), *Definition->DisplayName,
        *Definition->PrimaryText.TrimChar(TEXT('.')), *Definition->SecondaryName), Loot);
    VortexAudio::Play(this, TEXT("equip"), Owner->GetActorLocation(), .8f);
    Changed();
}

void UVortexHeroSheet::ServerLeaveStadium_Implementation()
{
    AActor* Owner = GetOwner();
    const AVortexAwakeningStadium* Stadium = Owner ? AVortexAwakeningStadium::Find(Owner->GetWorld()) : nullptr;
    if (!bNeedsAwakening || !Stadium || !Stadium->IsAtExit(Owner->GetActorLocation())) return;
    if (GetWeaponType().IsNone())
    {
        Notice(TEXT("Escolha uma arma antes de sair do estádio"), Refused);
        return;
    }
    bNeedsAwakening = false;
    Notice(TEXT("Que comece a jornada"), 5);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_STADIUM %s saiu com %s"), *GetNameSafe(Owner), *GetWeaponType().ToString());
    // a jornada começa na Cidade do Santuário, na praça, de frente para a Fonte de Jade (sem cidade,
    // volta para onde estava); parado no ar até o chão de lá carregar (senão cairia pelo terreno vazio)
    if (ACharacter* Hero = Cast<ACharacter>(Owner))
    {
        Hero->GetCharacterMovement()->DisableMovement();
        FVector Arrive = ReturnLocation.IsZero() ? FVector(0.f, 1500.f, 2000.f) : ReturnLocation + FVector(0.f, 0.f, 100.f);
        FRotator Facing = Hero->GetActorRotation();
        if (const AVortexFountain* Fountain = AVortexFountain::Find(Owner->GetWorld()))
        {
            const FTransform Spot = Fountain->RespawnPoint();
            Arrive = Spot.GetLocation() + Spot.GetUnitAxis(EAxis::Y) * FMath::FRandRange(-240.f, 240.f) + FVector(0.f, 0.f, 100.f);   // lado a lado, de frente para a fonte
            Facing = Spot.Rotator();
        }
        Hero->SetActorLocationAndRotation(Arrive, Facing, false, nullptr, ETeleportType::TeleportPhysics);
        if (AController* Controller = Hero->GetController())
        {
            Controller->SetControlRotation(Facing);
            if (APlayerController* Player = Cast<APlayerController>(Controller)) Player->ClientSetRotation(Facing);
        }
    }
    GetWorld()->GetTimerManager().SetTimer(LandTimer, this, &UVortexHeroSheet::LandInValley, .2f, true);
    Changed();
}

void UVortexHeroSheet::ServerRespawnNow_Implementation()
{
    if (DeadFor > 2.f) DeadFor = 100.f;
}

void UVortexHeroSheet::LandInValley()
{
    ACharacter* Hero = Cast<ACharacter>(GetOwner());
    AVortexTerrainStreamer* Streamer = nullptr;
    for (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It; ++It) Streamer = *It;
    if (!Hero) { GetWorld()->GetTimerManager().ClearTimer(LandTimer); return; }
    float Ground = 0.f;
    const FVector At = Hero->GetActorLocation();
    if (Streamer && !Streamer->GroundReadyAt(At.X, At.Y, Ground)) return;
    if (Streamer) Hero->SetActorLocation(FVector(At.X, At.Y, Ground + 120.f), false, nullptr, ETeleportType::TeleportPhysics);
    Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetWorld()->GetTimerManager().ClearTimer(LandTimer);
}

void UVortexHeroSheet::ApplyVitals(bool bRefill)
{
    UAbilitySystemComponent* Abilities = AbilitiesOf(GetOwner());
    if (!Abilities || !GetOwner()->HasAuthority()) return;
    const FVortexHeroStats S = ComputeStats();
    Abilities->SetNumericAttributeBase(UVortexAttributes::GetMaxHealthAttribute(), S.MaxHealth);
    Abilities->SetNumericAttributeBase(UVortexAttributes::GetMaxManaAttribute(), S.MaxMana);
    const float Health = Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
    const float Mana = Abilities->GetNumericAttribute(UVortexAttributes::GetManaAttribute());
    if (Health <= 0.f) return;   // morto continua morto
    Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), bRefill ? S.MaxHealth : FMath::Min(Health, S.MaxHealth));
    Abilities->SetNumericAttributeBase(UVortexAttributes::GetManaAttribute(), bRefill ? S.MaxMana : FMath::Min(Mana, S.MaxMana));
}

void UVortexHeroSheet::OnRep_Sheet()
{
    ++Revision;
    // 07/11: nos outros computadores, trocar de espada (mesmo tipo) também troca o modelo na mão
    if (const AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner())) if (Hero->GetWeapon()) Hero->GetWeapon()->RefreshVisual();
    if (IsOwnerLocal() && bLoaded) bSaveDirty = true;
}

// ---- carregar e gravar (o save é do dono, como o visual) ------------------------------------

void UVortexHeroSheet::TryLoadFromSave()
{
    if (bSentLoad || !IsOwnerLocal()) return;
    bSentLoad = true;
    FVortexSheetSave Save;
    const UVortexGameInstance* Game = GetWorld() ? Cast<UVortexGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
    if (const UVortexPlayerSave* Hero = Game ? Game->GetSelectedHero() : nullptr)
    {
        Save.HeroId = Hero->Identity.PlayerId;
        Save.bFromMenu = true;
        if (Hero->bHasSheet)
        {
            Save.bFresh = false;
            Save.Xp = Hero->Experience;
            // saves de antes dos 12 oficiais (schema 3): os pontos eram dos oito antigos e voltam a ficar livres
            // 10/11: os pontos dos 12 antigos (schema 4) não valem nos 8 novos: ficam livres (o menu pede para apagar o save antigo)
            if (Hero->SaveSchemaVersion >= 5) Save.Points = Hero->AttributePoints;
            Save.Bag = Hero->Bag;
            Save.Equipped = Hero->Equipped;
            Save.Coins = Hero->Coins;
            Save.bNeedsAwakening = Hero->bNeedsAwakening;
            Save.ClassId = Hero->ClassId;
            Save.ClassCards = Hero->ClassCards;
            Save.MountCoat = Hero->MountCoat;
            Save.MountName = Hero->MountName;
            Save.StaffSpell = Hero->StaffSpell;
            Save.OutlawHours = Hero->OutlawHours;
            Save.ShopDay = Hero->ShopDay;
            Save.ShopBought = Hero->ShopBought;
        }
    }
    bLoaded = true;   // daqui em diante as mudanças vão para o save
    ServerLoad(Save);
}

bool UVortexHeroSheet::ServerLoad_Validate(const FVortexSheetSave& Save)
{
    return Save.ShopBought.Num() <= 4096 && Save.Points.Num() <= 16 && Save.Bag.Num() <= 64 && Save.Equipped.Num() <= 16;
}

void UVortexHeroSheet::ServerLoad_Implementation(const FVortexSheetSave& Incoming)
{
    // 09/11: uma vez por herói: reenviar a ficha antiga duplicava o que já tinha sido trocado ou vendido
    if (bServerLoaded) return;
    bServerLoaded = true;
    // o convidado volta com a cópia do anfitrião, e herói que ele nunca viu entra como novo (AVortexGameMode::DecideGuestSheet)
    FVortexSheetSave Save = Incoming;
    AVortexGameMode* Mode = GetWorld()->GetAuthGameMode<AVortexGameMode>();
    if (const APawn* Pawn = Cast<APawn>(GetOwner()); Pawn && Mode && Pawn->GetNetConnection())
    {
        bGuest = true;
        GuestAccount = AVortexGameMode::GuestAccount(Pawn);
        GuestHero = Incoming.HeroId;
        Save = Mode->DecideGuestSheet(GuestAccount, Incoming);
    }
    const FVortexRpgCatalog& Rpg = FVortexRpgCatalog::Get();
    Xp = FMath::Clamp<int64>(Save.Xp, 0, 2000000000);
    Level = FVortexRpgCatalog::LevelFromXp(Xp);
    Points = Save.Points;
    if (Points.Num() != VortexRpg::NumGroups) Points.Reset();   // 10/11: pontos de outro formato (os 12 antigos): ficam livres
    Bag = Save.Bag;
    Equipped = Save.Equipped;
    Coins = FMath::Clamp(Save.Coins, 0, 999999999);
    EnsureSizes();
    // nada além do que o nível deu, nada de item que não existe ou que não cabe na pilha
    int32 Budget = FVortexRpgCatalog::PointsEarned(Level);
    for (int32& P : Points) { P = FMath::Clamp(P, 0, Budget); Budget -= P; }
    for (FVortexItemStack& Stack : Bag)
    {
        const FVortexItemDef* Def = Rpg.Item(Stack.Item);
        if (!Def || Stack.Count <= 0) Stack = FVortexItemStack();
        else Stack.Count = FMath::Min(Stack.Count, Def->Stack);
    }
    for (int32 i = 0; i < Equipped.Num(); ++i)
    {
        const FVortexItemDef* Def = Rpg.Item(Equipped[i]);
        if (!Def || VortexRpg::EquipSlotOf(Def->Slot) != i) Equipped[i] = NAME_None;
    }
    bNeedsAwakening = Save.bNeedsAwakening;
    ClassId = FVortexRpgCatalog::Get().Class(Save.ClassId) ? Save.ClassId : NAME_None;
    MountCoat = Save.MountCoat.ToString().StartsWith(TEXT("Horse_")) ? Save.MountCoat : NAME_None;
    MountName = MountCoat.IsNone() ? FString() : Save.MountName.Left(20);
    StaffSpell = Save.StaffSpell == TEXT("heal") || Save.StaffSpell == TEXT("vines") || Save.StaffSpell == TEXT("fireball") ? Save.StaffSpell : NAME_None;
    // a caveira guardada em horas que faltam: volta a contar do dia de agora deste mundo
    if (const AVortexGameState* State = GetWorld()->GetGameState<AVortexGameState>(); State && Save.OutlawHours > 0.f)
        OutlawUntilDay = State->GetDayNumber() + State->GetHour() / 24.f + FMath::Min(Save.OutlawHours, 72.f) / 24.f;
    ShopDay = FMath::Max(0, Save.ShopDay);
    ShopBought = Save.ShopBought;
    if (ShopBought.Num() > 512) ShopBought.SetNum(512);
    ClassCards.Reset();
    for (const FName& Card : Save.ClassCards) if (ClassId.IsNone() && FVortexRpgCatalog::Get().Class(Card) && ClassCards.Num() < 6) ClassCards.Add(Card);
    if (Save.bFresh)
    {
        // herói novo: a mochila inicial do Godot; acorda sem arma no Estádio do Despertar (lá
        // escolhe a arma inicial). Heróis de teste (sem dono) ficam com a espada.
        for (FVortexItemStack& Stack : Bag) Stack = FVortexItemStack();
        for (const FVortexItemStack& Start : Rpg.StartingBag) GiveItem(Start.Item, Start.Count, false);
        const APawn* Pawn = Cast<APawn>(GetOwner());
        // heróis do menu (e -VortexAwakening nos testes); os testes que entram direto no mapa ficam com a espada
        const bool bHuman = Pawn && Pawn->IsPlayerControlled() && (Save.bFromMenu || FParse::Param(FCommandLine::Get(), TEXT("VortexAwakening")));
        bNeedsAwakening = bHuman;
        Equipped[VortexRpg::Weapon] = bHuman ? NAME_None : FName(TEXT("espada_de_cinzas"));
    }
    bLoaded = true;
    if (bNeedsAwakening && !AVortexArsenal::IsArsenal(GetWorld())) EnterStadium();
    // já na primeira entrada: se o anfitrião cair antes do autosave, o herói continua conhecido
    if (bGuest && Mode) { StoreGuest(); Mode->SaveGuestsNow(); }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_SHEET carregada %s nivel=%d xp=%lld itens=%d novo=%d"),
        *GetNameSafe(GetOwner()), Level, Xp, Bag.FilterByPredicate([](const FVortexItemStack& S) { return !S.IsEmpty(); }).Num(), Save.bFresh ? 1 : 0);
    Changed(true);
}

FVortexSheetSave UVortexHeroSheet::MakeSave() const
{
    FVortexSheetSave Out;
    Out.Xp = Xp;
    Out.Points = Points;
    Out.Bag = Bag;
    Out.Equipped = Equipped;
    Out.Coins = Coins;
    Out.bNeedsAwakening = bNeedsAwakening;
    Out.bFromMenu = true;
    Out.bFresh = false;
    Out.ClassId = ClassId;
    Out.ClassCards = ClassCards;
    Out.MountCoat = MountCoat;
    Out.MountName = MountName;
    Out.StaffSpell = StaffSpell;
    if (const AVortexGameState* State = GetWorld() ? GetWorld()->GetGameState<AVortexGameState>() : nullptr; State && OutlawUntilDay > 0.f)
        Out.OutlawHours = FMath::Max(0.f, (OutlawUntilDay - (State->GetDayNumber() + State->GetHour() / 24.f)) * 24.f);
    Out.ShopDay = ShopDay;
    Out.ShopBought = ShopBought;
    return Out;
}

void UVortexHeroSheet::StoreGuest()
{
    if (!bServerLoaded || !bGuest || !GetWorld()) return;
    AVortexGameMode* Mode = GetWorld()->GetAuthGameMode<AVortexGameMode>();
    if (FVortexGuestHero* Entry = Mode ? Mode->FindGuest(GuestAccount, GuestHero, true) : nullptr)
    {
        Entry->bHasSheet = true;
        Entry->Sheet = MakeSave();
    }
}

void UVortexHeroSheet::EndPlay(const EEndPlayReason::Type Reason)
{
    // quem sai logo depois de uma mudança não pode perdê-la (nem ficar com o item que acabou de dar)
    if (bSaveDirty && bServerLoaded && bWasLocal) SaveLocal();
    Super::EndPlay(Reason);
}

void UVortexHeroSheet::SaveLocal()
{
    bSaveDirty = false;
    if (AVortexArsenal::IsArsenal(GetWorld())) return;   // 07/11: o Arsenal não mexe no herói gravado (nem na cópia em memória)
    UVortexGameInstance* Game = GetWorld() ? Cast<UVortexGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
    UVortexSaveSubsystem* Saves = Game ? Game->GetSubsystem<UVortexSaveSubsystem>() : nullptr;
    UVortexPlayerSave* Hero = Game ? Game->GetSelectedHero() : nullptr;
    if (!Saves || !Hero) return;
    Hero->bHasSheet = true;
    Hero->Experience = Xp;
    Hero->AttributePoints = Points;
    Hero->Bag = Bag;
    Hero->Equipped = Equipped;
    Hero->Coins = Coins;
    Hero->bNeedsAwakening = bNeedsAwakening;
    Hero->ClassId = ClassId;
    Hero->ClassCards = ClassCards;
    Hero->MountCoat = MountCoat;
    Hero->MountName = MountName;
    Hero->StaffSpell = StaffSpell;
    {
        const AVortexGameState* State = GetWorld() ? GetWorld()->GetGameState<AVortexGameState>() : nullptr;
        Hero->OutlawHours = State && OutlawUntilDay > 0.f ? FMath::Max(0.f, (OutlawUntilDay - (State->GetDayNumber() + State->GetHour() / 24.f)) * 24.f) : 0.f;
    }
    Hero->ShopDay = ShopDay;
    Hero->ShopBought = ShopBought;
    Hero->SaveSchemaVersion = UVortexPlayerSave::CurrentSchemaVersion;
    // falhou (disco cheio, arquivo preso): continua pendente, tenta de novo e avisa uma vez
    const bool bSaved = Saves->SaveLocalPlayer(Hero);
    bSaveDirty = !bSaved;
    if (!bSaved && !bSaveFailWarned)
    {
        bSaveFailWarned = true;
        UE_LOG(LogTemp, Error, TEXT("VORTEX_SAVE não foi possível gravar o herói"));
        Notice(TEXT("Não foi possível salvar o herói. Confira o espaço em disco."), Refused);
    }
    if (bSaved) bSaveFailWarned = false;
}

// ---- mapas das caravanas -------------------------------------------------------------------

void UVortexHeroSheet::ClientRevealMap_Implementation(int32 Hundreds, int32 Seed, const FString& ItemName)
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    const APlayerController* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    AVortexHudBase* Hud = Controller ? Cast<AVortexHudBase>(Controller->GetHUD()) : nullptr;
    const int32 Opened = Hud ? Hud->RevealTravelerRoute(Hundreds, Seed, Pawn->GetActorLocation()) : 0;
    Notice(Opened > 0 ? FString::Printf(TEXT("%s: %d pedaços novos na carta (M para ver)"), *ItemName, Opened)
        : FString::Printf(TEXT("%s: você já conhecia tudo o que ele mostra"), *ItemName), Loot);
    VortexAudio::Play2D(GetOwner(), TEXT("map_open"), .7f);
}

// ---- lojas ---------------------------------------------------------------------------------

int32 UVortexHeroSheet::ShopWorldDay() const
{
    const AVortexGameState* State = GetWorld() ? GetWorld()->GetGameState<AVortexGameState>() : nullptr;
    return State ? FMath::Max(1, State->GetDayNumber()) : 1;
}

FString UVortexHeroSheet::ShopWorldId() const
{
    const AVortexGameState* State = GetWorld() ? GetWorld()->GetGameState<AVortexGameState>() : nullptr;
    return State ? State->GetActiveWorldId().ToString() : FString();
}

int32 UVortexHeroSheet::ShopLeft(FName Vendor, int32 Index, int32 Qty, bool bUnlimited) const
{
    if (bUnlimited) return 999;
    if (ShopDay != ShopWorldDay()) return Qty;   // dia novo: tudo volta
    const FName Key = VortexShop::BoughtKey(Vendor, Index);
    int32 Bought = 0;
    for (const FName& B : ShopBought) Bought += B == Key;
    return FMath::Max(0, Qty - Bought);
}

namespace
{
    // A loja só funciona perto do comerciante (8 m), como no Godot, onde ela abre pela conversa.
    bool NearVendor(const AActor* Hero, FName Vendor)
    {
        if (!Hero || !Hero->GetWorld()) return false;
        const FName Npc = VortexShop::NpcOf(Vendor);
        for (TActorIterator<AVortexNpc> It(Hero->GetWorld()); It; ++It)
        {
            if (It->GetNpcId() == Npc && FVector::DistSquared(It->GetActorLocation(), Hero->GetActorLocation()) < FMath::Square(800.f)) return true;
        }
        return false;
    }
}

void UVortexHeroSheet::ServerForge_Implementation(uint8 Slot)
{
    if (!bLoaded || !Equipped.IsValidIndex(Slot) || (!bShopAnywhere && !NearVendor(GetOwner(), TEXT("bram"))))
    {
        UE_LOG(LogTemp, Display, TEXT("VORTEX_FORGE longe do Bram ou sem ficha (carregada=%d)"), bLoaded ? 1 : 0);
        return;
    }
    const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Equipped[Slot]);
    int32 Price = 0, Count = 0, Next = 0;
    FName Material;
    if (!Def || !VortexShop::ForgeCost(*Def, Level, Price, Material, Count, Next))
    {
        UE_LOG(LogTemp, Display, TEXT("VORTEX_FORGE recusado %s nivel=%d heroi=%d"), *Equipped[Slot].ToString(), Def ? Def->Level : -1, Level);
        Notice(TEXT("Esse já está no seu nível: suba de nível para melhorar mais."), Refused);
        return;
    }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_FORGE %s -> nivel %d: %d moedas (tem %d), %d %s (tem %d)"), *Equipped[Slot].ToString(), Next, Price, Coins, Count, *Material.ToString(), CountOf(Material));
    if (Coins < Price) { Notice(FString::Printf(TEXT("Faltam %d moedas."), Price - Coins), Refused); return; }
    const FVortexItemDef* Mat = FVortexRpgCatalog::Get().Item(Material);
    if (CountOf(Material) < Count) { Notice(FString::Printf(TEXT("Falta %s (precisa de %d)."), Mat ? *Mat->Name : *Material.ToString(), Count), Refused); return; }
    Coins -= Price;
    TakeItem(Material, Count);
    Equipped[Slot] = FVortexRpgCatalog::LevelledId(Def->BaseId.IsNone() ? Def->Id : Def->BaseId, Next);
    Notice(FString::Printf(TEXT("%s agora é nível %d."), *Def->Name, Next), Loot);
    VortexAudio::Play(this, TEXT("wfx_hammer_slam"), GetOwner()->GetActorLocation(), .5f);
    Changed();
}

void UVortexHeroSheet::ServerShopBuy_Implementation(FName Vendor, int32 Index)
{
    if (!bLoaded || !VortexShop::IsVendor(Vendor) || (!bShopAnywhere && !NearVendor(GetOwner(), Vendor))) return;
    const int32 Day = ShopWorldDay();
    if (ShopDay != Day) { ShopDay = Day; ShopBought.Reset(); }
    const TArray<VortexShop::FOffer> Stock = VortexShop::Stock(Vendor, ShopWorldId(), Day, Level);
    if (!Stock.IsValidIndex(Index)) return;
    const VortexShop::FOffer& Offer = Stock[Index];
    const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Offer.Item);
    const FString Seller = VortexShop::Info(Vendor).Seller;
    if (!Def) return;
    if (ShopLeft(Vendor, Index, Offer.Qty, Offer.bUnlimited) <= 0) { PushNotice(FString::Printf(TEXT("%s já vendeu tudo dessa hoje. Volte amanhã."), *Seller)); return; }
    if (Coins < Offer.Price) { PushNotice(FString::Printf(TEXT("Faltam %d moedas."), Offer.Price - Coins)); return; }
    // cabe na mochila? (pilha igual com espaço ou um espaço vazio)
    const bool bFits = Bag.ContainsByPredicate([&](const FVortexItemStack& S) { return S.IsEmpty() || (S.Item == Offer.Item && S.Count < Def->Stack); });
    if (!bFits) { PushNotice(TEXT("Mochila cheia.")); return; }
    Coins -= Offer.Price;
    GiveItem(Offer.Item, 1, false);
    if (!Offer.bUnlimited) ShopBought.Add(VortexShop::BoughtKey(Vendor, Index));
    Notice(FString::Printf(TEXT("Comprou %s por %d moedas."), *Def->Name, Offer.Price), Loot);
    Changed();
}

void UVortexHeroSheet::ServerShopSell_Implementation(FName Vendor, FName Item, int32 Count)
{
    if (!bLoaded || !VortexShop::IsVendor(Vendor) || (!bShopAnywhere && !NearVendor(GetOwner(), Vendor))) return;
    const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Item);
    if (!Def) return;
    // o que está vestido não aparece na lista; só a mochila é vendida
    const int32 Each = VortexShop::Pays(Vendor, *Def);
    if (Each <= 0) { PushNotice(TEXT("Esse comerciante não compra isso.")); return; }
    Count = FMath::Clamp(Count, 1, CountOf(Item));
    if (Count <= 0) return;
    const int32 Sold = TakeItem(Item, Count);
    if (Sold <= 0) return;
    Coins = FMath::Min(Coins + Each * Sold, 999999999);
    Notice(FString::Printf(TEXT("Vendeu %dx %s por %d moedas."), Sold, *Def->Name, Each * Sold), Loot);
    Changed();
}

// ---- pedidos do dono ----------------------------------------------------------------------

void UVortexHeroSheet::ServerSpendPoint_Implementation(uint8 Attribute)
{
    if (Attribute >= VortexRpg::NumGroups || GetPointsFree() <= 0) return;
    EnsureSizes();
    ++Points[Attribute];
    Changed();
}

void UVortexHeroSheet::ServerUseSlot_Implementation(uint8 BagIndex)
{
    EnsureSizes();
    if (!Bag.IsValidIndex(BagIndex) || Bag[BagIndex].IsEmpty()) return;
    const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Bag[BagIndex].Item);
    UAbilitySystemComponent* Abilities = AbilitiesOf(GetOwner());
    if (!Def || !Abilities) return;
    if (Def->RevealMap > 0)
    {
        const FName Item = Def->Id;
        Bag[BagIndex].Count -= 1;
        if (Bag[BagIndex].Count <= 0) Bag[BagIndex] = FVortexItemStack();
        Changed();
        ClientRevealMap(Def->RevealMap, FMath::Rand(), Def->Name);
        return;
    }
    if (Def->IsConsumable())
    {
        const float Health = Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
        if (Health <= 0.f || IsDrinking()) return;
        if (HasDungeonRule(VortexDungeonRules::NoPotions)) { Notice(TEXT("Este portal não deixa usar poções nem comida."), 1); return; }
        // beber leva um tempo: a garrafinha sobe, três goles; o efeito e o gasto só no fim (cancelar não gasta)
        const AGameStateBase* State = GetWorld()->GetGameState();
        DrinkItem = Def->Id;
        DrinkStartServer = State ? float(State->GetServerWorldTimeSeconds()) : GetWorld()->GetTimeSeconds();
        GetWorld()->GetTimerManager().SetTimer(DrinkTimer, this, &UVortexHeroSheet::FinishDrink, DrinkSeconds, false);
        if (AVortexCharacter* Hero = Cast<AVortexCharacter>(GetOwner()))
        {
            // largar o golpe ou a defesa que estava fazendo
            if (Hero->GetWeapon()) Hero->GetWeapon()->SetGuardPressed(false);
        }
        GetOwner()->ForceNetUpdate();
        return;
    }
    const int32 Slot = VortexRpg::EquipSlotOf(Def->Slot);
    if (Slot == INDEX_NONE) return;
    // 07/11: o nível do item trava (como nos jogos de saque): só veste com o nível pedido
    if (Def->ReqLevel > Level)
    {
        Notice(FString::Printf(TEXT("%s: precisa do nível %d (você está no %d)"), *Def->Name, Def->ReqLevel, Level), Refused);
        return;
    }
    if (Slot == VortexRpg::Weapon)
    {
        const UVortexWeaponDefinition* Type = UVortexWeaponComponent::LoadType(FVortexRpgCatalog::Get().WeaponType(Def->Id));
        if (!Type || !Type->bPlayable)
        {
            Notice(FString::Printf(TEXT("%s: os golpes desta arma ainda não chegaram ao Vortex"), *Def->Name), Refused);
            return;
        }
        if (Type->bMagic && !Rpg_RaceCanMagic(GetRace()))
        {
            Notice(TEXT("Anões não conseguem usar armas de magia"), Refused);
            return;
        }
    }
    const FName Old = Equipped[Slot];
    Equipped[Slot] = Def->Id;
    Bag[BagIndex] = Old.IsNone() ? FVortexItemStack() : FVortexItemStack{ Old, 1 };
    Notice(FString::Printf(TEXT("Equipou %s"), *Def->Name), Plain);
    Changed();
}

void UVortexHeroSheet::ServerUnequip_Implementation(uint8 Slot)
{
    EnsureSizes();
    if (Slot >= VortexRpg::NumEquip || Equipped[Slot].IsNone()) return;
    const int32 Free = Bag.IndexOfByPredicate([](const FVortexItemStack& S) { return S.IsEmpty(); });
    if (Free == INDEX_NONE)
    {
        Notice(TEXT("Mochila cheia"), Refused);
        return;
    }
    Bag[Free] = { Equipped[Slot], 1 };
    Equipped[Slot] = NAME_None;
    Changed();
}

void UVortexHeroSheet::ServerMoveSlot_Implementation(uint8 From, uint8 To)
{
    EnsureSizes();
    if (!Bag.IsValidIndex(From) || !Bag.IsValidIndex(To) || From == To) return;
    FVortexItemStack& A = Bag[From];
    FVortexItemStack& B = Bag[To];
    const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(A.Item);
    if (Def && A.Item == B.Item && B.Count < Def->Stack)
    {
        const int32 Move = FMath::Min(A.Count, Def->Stack - B.Count);
        B.Count += Move;
        A.Count -= Move;
        if (A.Count <= 0) A = FVortexItemStack();
    }
    else Swap(A, B);
    Changed();
}

void UVortexHeroSheet::HealBy(float Amount)
{
    UAbilitySystemComponent* Abilities = AbilitiesOf(GetOwner());
    if (!Abilities || Amount <= 0.f) return;
    const float Health = Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
    if (Health <= 0.f) return;
    Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), FMath::Min(ComputeStats().MaxHealth, Health + Amount));
}

float UVortexHeroSheet::DrinkElapsed() const
{
    if (!IsDrinking() || !GetWorld()) return 0.f;
    const AGameStateBase* State = GetWorld()->GetGameState();
    const float Now = State ? float(State->GetServerWorldTimeSeconds()) : GetWorld()->GetTimeSeconds();
    return FMath::Max(0.f, Now - DrinkStartServer);
}

void UVortexHeroSheet::ServerCancelDrink_Implementation()
{
    if (!IsDrinking()) return;
    GetWorld()->GetTimerManager().ClearTimer(DrinkTimer);
    DrinkItem = NAME_None;
    if (GetOwner()) GetOwner()->ForceNetUpdate();
}

void UVortexHeroSheet::FinishDrink()
{
    const FName Item = DrinkItem;
    DrinkItem = NAME_None;
    if (GetOwner()) GetOwner()->ForceNetUpdate();
    const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Item);
    UAbilitySystemComponent* Abilities = AbilitiesOf(GetOwner());
    const int32 Index = Bag.IndexOfByPredicate([Item](const FVortexItemStack& S) { return S.Item == Item && S.Count > 0; });
    if (!Def || !Abilities || Index == INDEX_NONE) return;
    const float Health = Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
    if (Health <= 0.f) return;
    const FVortexHeroStats S = ComputeStats();
    // Inteligência: curas mais fortes (3% por ponto, como no Godot)
    // 07/11 (a economia): cura uma parte da vida (a poção vermelha, 40, cura 30%), para valer no nível alto como no 1
    if (Def->Heal > 0.f) Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(),
        FMath::Min(S.MaxHealth, Health + FMath::Max(Def->Heal, S.MaxHealth * Def->Heal / 133.f) * S.HealPotency));
    if (Def->Mana > 0.f)
    {
        const float Mana = Abilities->GetNumericAttribute(UVortexAttributes::GetManaAttribute());
        Abilities->SetNumericAttributeBase(UVortexAttributes::GetManaAttribute(), FMath::Min(S.MaxMana, Mana + FMath::Max(Def->Mana, S.MaxMana * Def->Mana / 100.f)));
    }
    if (--Bag[Index].Count <= 0) Bag[Index] = FVortexItemStack();
    Notice(FString::Printf(TEXT("Bebeu %s"), *Def->Name), Plain);
    Changed();
}

void UVortexHeroSheet::ServerQuickUse_Implementation(FName Item)
{
    const int32 Index = Bag.IndexOfByPredicate([Item](const FVortexItemStack& S) { return S.Item == Item && S.Count > 0; });
    if (Index == INDEX_NONE)
    {
        Notice(FString::Printf(TEXT("Sem %s na mochila"), *ItemName(Item)), Refused);
        return;
    }
    ServerUseSlot_Implementation(uint8(Index));
}

// ---- passo -----------------------------------------------------------------------------------

void UVortexHeroSheet::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    AActor* Owner = GetOwner();
    if (!Owner) return;
    VortexArmor::Tick(this, DeltaTime);   // 07/11: as passivas da armadura (só no servidor)
    if (IsOwnerLocal()) { bWasLocal = true; TryLoadFromSave(); }
    // no Estádio: a cópia local do coliseu existe e a arma na mão sai do pedestal
    if (bNeedsAwakening && Owner->GetNetMode() != NM_DedicatedServer && (IsOwnerLocal() || Owner->HasAuthority()))
    {
        if (AVortexAwakeningStadium* Stadium = AVortexAwakeningStadium::Ensure(Owner->GetWorld()))
        {
            if (IsOwnerLocal()) Stadium->SetHeldType(GetWeaponType());
        }
    }
    if (bSaveDirty && bServerLoaded && IsOwnerLocal() && (SaveTimer += DeltaTime) > 1.5f)
    {
        SaveTimer = 0.f;
        SaveLocal();
    }
    if (!Owner->HasAuthority()) return;
    // herói sem dono humano (testes, bonecos): começa como herói novo
    LoadWait += DeltaTime;
    if (!bLoaded && LoadWait > 4.f && !Cast<APawn>(Owner)->IsPlayerControlled()) ServerLoad_Implementation(FVortexSheetSave());
    // vida (fora de combate: 6 s sem apanhar) e mana voltam sozinhas
    SinceDamaged += DeltaTime;
    UAbilitySystemComponent* Abilities = AbilitiesOf(Owner);
    if (!Abilities || !bLoaded) return;
    const float Health = Abilities->GetNumericAttribute(UVortexAttributes::GetHealthAttribute());
    // a bênção acabou: os atributos voltam ao normal
    if (bBlessingActive && BlessingRemaining() <= 0.f) { bBlessingActive = false; Changed(); }
    const FVortexHeroStats S = ComputeStats();
    // queimando: perde ~1,2% da vida a cada meio segundo enquanto durar
    if (Health > 0.f && BurnRemaining() > 0.f && (BurnTick += DeltaTime) >= .5f)
    {
        BurnTick = 0.f;
        SinceDamaged = 0.f;
        Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), FMath::Max(0.f, Health - FMath::Max(2.f, FMath::RoundToFloat(S.MaxHealth * .012f))));
    }
    // envenenado: ~0,8% da vida a cada meio segundo (não mata: para em 1)
    if (Health > 1.f && PoisonRemaining() > 0.f && (PoisonTick += DeltaTime) >= .5f)
    {
        PoisonTick = 0.f;
        SinceDamaged = 0.f;
        Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), FMath::Max(1.f, Health - FMath::Max(1.f, FMath::RoundToFloat(S.MaxHealth * .008f))));
    }
    const AVortexFountain* Fountain = AVortexFountain::Find(Owner->GetWorld());
    if (Health <= 0.f)
    {
        // caiu: depois de 5 s renasce na Fonte do Santuário, com vida e mana cheias (fora da cidade
        // e no Estádio, onde não há fonte, fica como antes)
        DeadFor += DeltaTime;
        if (Fountain && !bNeedsAwakening && DeadFor > 5.f)
        {
            DeadFor = 0.f;
            const FTransform Spot = Fountain->RespawnPoint();
            Owner->TeleportTo(Spot.GetLocation(), Spot.Rotator(), false, true);
            Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), S.MaxHealth);
            Abilities->SetNumericAttributeBase(UVortexAttributes::GetManaAttribute(), S.MaxMana);
            SinceDamaged = 100.f;
            Notice(TEXT("Você renasceu na Fonte do Santuário"), LevelUp);
            VortexAudio::Play(this, TEXT("heal"), Spot.GetLocation(), 1.f);
            UE_LOG(LogTemp, Display, TEXT("VORTEX_SANCTUARY %s renasceu na fonte"), *GetNameSafe(Owner));
        }
        return;
    }
    DeadFor = 0.f;
    // a água do santuário: perto da fonte, vida e mana voltam depressa (12% por segundo)
    const bool bSanctuary = Fountain && Fountain->IsNear(Owner->GetActorLocation());
    if ((SinceDamaged > 6.f || bSanctuary) && Health < S.MaxHealth && !HasDungeonRule(VortexDungeonRules::NoRegen))
    {
        const float Regen = bSanctuary ? FMath::Max(S.HpRegen, S.MaxHealth * .12f) : S.HpRegen;
        Abilities->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), FMath::Min(S.MaxHealth, Health + Regen * DeltaTime));
    }
    const float Mana = Abilities->GetNumericAttribute(UVortexAttributes::GetManaAttribute());
    if (Mana < S.MaxMana)
    {
        const float Regen = bSanctuary ? FMath::Max(S.MpRegen, S.MaxMana * .12f) : S.MpRegen;
        Abilities->SetNumericAttributeBase(UVortexAttributes::GetManaAttribute(), FMath::Min(S.MaxMana, Mana + Regen * DeltaTime));
    }
}
