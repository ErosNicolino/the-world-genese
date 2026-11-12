#include "Combat/VortexSwords.h"
#include "Rpg/VortexRpg.h"
#include "Combat/VortexPowerFx.h"
#include "Misc/PackageName.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

#include "Combat/VortexWeaponDefinition.h"
#include "Dom/JsonObject.h"
#include "Framework/VortexCharacter.h"
#include "Rpg/VortexHeroSheet.h"
#include "Animation/AnimSequence.h"
#include "Combat/VortexWeaponComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Framework/VortexAudio.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    /** As duas tabelas (swords.json e bows.json), lidas uma vez. */
    const TMap<FName, VortexSwords::FSword>& Table()
    {
        static TMap<FName, VortexSwords::FSword> All = []()
        {
            TMap<FName, VortexSwords::FSword> Out;
            for (const TCHAR* File : { TEXT("swords"), TEXT("bows") })
            {
                FString Text;
                TSharedPtr<FJsonObject> Root;
                if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / FString::Printf(TEXT("Data/Rpg/%s.json"), File))) ||
                    !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root) continue;
                const bool bBow = FCString::Strcmp(File, TEXT("bows")) == 0;
                for (const auto& Pair : Root->GetObjectField(File)->Values)
                {
                    const TSharedPtr<FJsonObject> O = Pair.Value->AsObject();
                    const TSharedPtr<FJsonObject> P = O->GetObjectField(TEXT("power"));
                    VortexSwords::FSword S;
                    S.Model = O->GetStringField(TEXT("model"));
                    S.Key = O->GetStringField(TEXT("key"));
                    S.Element = FName(*O->GetStringField(TEXT("element")));
                    S.Weapon = bBow ? FName(TEXT("arco_de_betula")) : FName(TEXT("espada_de_cinzas"));
                    O->TryGetStringField(TEXT("arrow"), S.Arrow);
                    FString Anim;
                    S.bSelfAnim = O->TryGetStringField(TEXT("anim"), Anim) && Anim == TEXT("self");
                    S.PowerName = P->GetStringField(TEXT("name"));
                    P->TryGetStringField(TEXT("text"), S.PowerText);
                    FString Kind;
                    if (P->TryGetStringField(TEXT("kind"), Kind))
                    {
                        S.bHasKit = true;
                        S.Kit.Family = bBow ? TEXT("Bow") : TEXT("Sword");
                        S.Kit.Special = FName(*Kind);
                        S.Kit.SpecialMana = float(P->GetNumberField(TEXT("mana")));
                        S.Kit.SpecialCooldown = float(P->GetNumberField(TEXT("cooldown")));
                        S.PowerIcon = FName(*P->GetStringField(TEXT("icon")));
                        for (const auto& V : P->GetObjectField(TEXT("values"))->Values)
                            S.Kit.Values.Add(FName(*V.Key), float(V.Value->AsNumber()));
                    }
                    Out.Add(FName(*Pair.Key), MoveTemp(S));
                }
            }
            UE_LOG(LogTemp, Display, TEXT("VORTEX_SWORDS %d espadas e arcos"), Out.Num());
            return Out;
        }();
        return All;
    }
}

const VortexSwords::FSword* VortexSwords::Find(FName Item)
{
    return Table().Find(FVortexRpgCatalog::BaseOf(Item));   // 07/11: "<id>@<nível>" é a mesma espada
}

const VortexSwords::FSword* VortexSwords::ForVisual(FName Item)
{
    if (const FSword* Own = Find(Item)) return Own;
    const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Item);
    if (!Def) return nullptr;
    static const TMap<FString, FString> Tier = { { TEXT("common"), TEXT("comum") }, { TEXT("uncommon"), TEXT("incomum") },
        { TEXT("rare"), TEXT("raro") }, { TEXT("epic"), TEXT("epico") }, { TEXT("legendary"), TEXT("lendario") }, { TEXT("unique"), TEXT("unico") } };
    const FString* Name = Tier.Find(Def->Rarity);
    const FName Family(*Def->Family);
    if (!Name || (Family != TEXT("espada_de_cinzas") && Family != TEXT("arco_de_betula"))) return nullptr;
    // a mesma conta de antes (as espadas do arsenal não trocam de modelo)
    const FString Key = FString::Printf(TEXT("%s_%d"), **Name, 1 + int32(GetTypeHash(Item.ToString()) % 5));
    for (const TPair<FName, FSword>& Pair : Table())
        if (Pair.Value.Key == Key && Pair.Value.Weapon == Family) return &Pair.Value;
    return nullptr;
}

const VortexWeaponKit::FKit* VortexSwords::SpecialKit(const AActor* Owner, const UVortexWeaponDefinition* Definition)
{
    if (!Definition) return nullptr;
    const AVortexCharacter* Hero = Cast<AVortexCharacter>(Owner);
    const UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
    const FName Item = Sheet ? Sheet->GetEquipped(VortexRpg::Weapon) : NAME_None;
    const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Item);
    // só quando a arma na mão é mesmo a espada da ficha (os testes trocam o tipo direto)
    if (const FSword* Sword = Find(Item); Sword && Sword->bHasKit && Def && (Definition->WeaponId.IsNone() || Sword->Weapon == Definition->WeaponId)) return &Sword->Kit;
    return VortexWeaponKit::Find(Definition->WeaponId);
}

namespace
{
    /** A espada equipada com poder próprio, se a arma na mão é ela. */
    const VortexSwords::FSword* PoweredSword(const AActor* Owner, const UVortexWeaponDefinition* Definition)
    {
        const AVortexCharacter* Hero = Cast<AVortexCharacter>(Owner);
        const UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
        const FName Item = Sheet ? Sheet->GetEquipped(VortexRpg::Weapon) : NAME_None;
        const FVortexItemDef* Def = FVortexRpgCatalog::Get().Item(Item);
        const VortexSwords::FSword* Sword = VortexSwords::Find(Item);
        if (!Sword || !Sword->bHasKit || !Def || !Definition || !(Definition->WeaponId.IsNone() || Sword->Weapon == Definition->WeaponId)) return nullptr;
        return Sword;
    }

    UNiagaraSystem* Fx(const TCHAR* Path)
    {
        static TMap<FString, UNiagaraSystem*> Cache;
        UNiagaraSystem*& Found = Cache.FindOrAdd(Path);
        if (!Found)
        {
            Found = LoadObject<UNiagaraSystem>(nullptr, Path);
            if (Found) Found->AddToRoot();
        }
        return Found;
    }
}

UAnimSequence* VortexSwords::SpecialClip(const AActor* Owner, const UVortexWeaponDefinition* Definition, bool bHold)
{
    const FSword* Sword = PoweredSword(Owner, Definition);
    if (!Sword) return nullptr;
    // 07/11 (o usuário: "todas giram igual; animação própria para cada uma"): a animação feita para esta arma no
    // esqueleto do herói (Tools/Blender/MakePowerAnims.py -> MX_<raça>_Poder_<espada|arco>_<chave>_Full; a de carregar,
    // _Carga_Full). As do secundário das outras armas eram do esqueleto antigo e não tocavam no herói de blocos
    const ACharacter* Hero = Cast<ACharacter>(Owner);
    const USkeletalMesh* Mesh = Hero && Hero->GetMesh() ? Hero->GetMesh()->GetSkeletalMeshAsset() : nullptr;
    if (!Mesh || !Mesh->GetName().StartsWith(TEXT("Hero_"))) return nullptr;
    const FString Race = Mesh->GetName().RightChop(5).Replace(TEXT("_FP"), TEXT(""));
    const FString Name = FString::Printf(TEXT("MX_%s_Poder_%s_%s%s_Full"), *Race, Sword->Weapon == TEXT("arco_de_betula") ? TEXT("arco") : TEXT("espada"),
        *Sword->Key, bHold ? TEXT("_Carga") : TEXT(""));
    static TMap<FString, UAnimSequence*> Cache;
    if (UAnimSequence** Found = Cache.Find(Name)) return *Found;
    const FString Package = FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/Anims/%s/%s"), *Race, *Name);
    UAnimSequence* Clip = FPackageName::DoesPackageExist(Package) ? LoadObject<UAnimSequence>(nullptr, *(Package + TEXT(".") + Name)) : nullptr;
    if (Clip) Clip->AddToRoot();
    Cache.Add(Name, Clip);
    return Clip;
}

void VortexSwords::OnSpecialPose(const ACharacter* Hero, int32 Serial)
{
    static TMap<TWeakObjectPtr<const ACharacter>, int32> Shown;
    const AVortexCharacter* Vortex = Cast<AVortexCharacter>(Hero);
    if (!Vortex || !Vortex->GetWeapon() || Shown.FindRef(Hero) == Serial) return;
    Shown.Add(Hero, Serial);
    const UVortexWeaponDefinition* Definition = Vortex->GetWeapon()->GetDefinition();
    const FSword* Sword = PoweredSword(Hero, Definition);
    if (!Sword) return;
    // 07/11 (o usuário: "os efeitos são sempre o mesmo; criar um para cada do zero"): o efeito montado para esta arma
    // (AVortexPowerFx, Data/Rpg/power_fx.json), no lugar dos Niagara prontos do projeto
    const UVortexHeroSheet* Sheet = Vortex->GetSheet();
    // segurando (a carga): o efeito da carga ("<item>_carga"); soltando, o do poder
    const FName Item = Sheet ? Sheet->GetEquipped(VortexRpg::Weapon) : NAME_None;
    const bool bHold = Vortex->GetWeapon()->GetAttackState().SwingIndex == VortexSpecialPose::Hold;
    AVortexPowerFx::Spawn(const_cast<ACharacter*>(Hero), bHold ? FName(*(Item.ToString() + TEXT("_carga"))) : Item);
    const TCHAR* Prefix = Sword->Weapon == TEXT("arco_de_betula") ? TEXT("bw_") : TEXT("sw_");
    VortexAudio::Play(Hero, FName(*(Prefix + Sword->Key)), Hero->GetActorLocation(), .9f);
}

void VortexSwords::Preload(const ACharacter* Hero)
{
    const AVortexCharacter* Vortex = Cast<AVortexCharacter>(Hero);
    const UVortexWeaponDefinition* Definition = Vortex && Vortex->GetWeapon() ? Vortex->GetWeapon()->GetDefinition() : nullptr;
    const FSword* Sword = PoweredSword(Hero, Definition);
    if (!Sword) return;
    SpecialClip(Hero, Definition, false);
    SpecialClip(Hero, Definition, true);
    const FString Sound = (Sword->Weapon == TEXT("arco_de_betula") ? TEXT("bw_") : TEXT("sw_")) + Sword->Key;
    const FString Package = TEXT("/Game/Vortex/Audio/Sfx/") + Sound;
    if (FPackageName::DoesPackageExist(Package))
        if (UObject* Loaded = LoadObject<UObject>(nullptr, *(Package + TEXT(".") + Sound))) Loaded->AddToRoot();
    for (const TCHAR* Path : { TEXT("/Game/Vortex/Combat/M_PowerFx.M_PowerFx"), TEXT("/Game/Vortex/Combat/M_SlashArc.M_SlashArc"),
             TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("/Engine/BasicShapes/Sphere.Sphere"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
             TEXT("/Engine/BasicShapes/Cone.Cone") })
        if (UObject* Loaded = LoadObject<UObject>(nullptr, Path)) Loaded->AddToRoot();
}

UNiagaraSystem* VortexSwords::ElementBurst(FName Element)
{
    static const TMap<FName, const TCHAR*> Bursts = {
        { TEXT("fire"), TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Fire.NS_Free_Spells_Buff_Fire") },
        { TEXT("ice"), TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Air.NS_Free_Spells_Buff_Air") },
        { TEXT("lightning"), TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Lightning.NS_Free_Spells_Buff_Lightning") },
        { TEXT("arcane"), TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Lightning.NS_Free_Spells_Buff_Lightning") },
        { TEXT("nature"), TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Healing.NS_Free_Spells_Buff_Healing") },
        { TEXT("poison"), TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Soul.NS_Free_Spells_Buff_Soul") },
        { TEXT("water"), TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Mana.NS_Free_Spells_Buff_Mana") },
        { TEXT("holy"), TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_LevelUp.NS_Free_Spells_LevelUp") },
        { TEXT("shadow"), TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Soul.NS_Free_Spells_Aura_Soul") },
    };
    const TCHAR* const* Path = Bursts.Find(Element);
    return Fx(Path ? *Path : TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave"));
}

namespace
{
    /** A arma da tabela na mão (o item equipado, se o tipo da arma bate); bVisual: as variantes do arsenal também. */
    const VortexSwords::FSword* Held(const AActor* Owner, bool bVisual)
    {
        const AVortexCharacter* Hero = Cast<AVortexCharacter>(Owner);
        const UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
        const UVortexWeaponDefinition* Definition = Hero && Hero->GetWeapon() ? Hero->GetWeapon()->GetDefinition() : nullptr;
        if (!Sheet || !Definition) return nullptr;
        const FName Item = Sheet->GetEquipped(VortexRpg::Weapon);
        const VortexSwords::FSword* Sword = bVisual ? VortexSwords::ForVisual(Item) : VortexSwords::Find(Item);
        return Sword && Sword->Weapon == Definition->WeaponId ? Sword : nullptr;
    }
}

UStaticMesh* VortexSwords::ArrowMesh(const AActor* Owner)
{
    const FSword* Bow = Held(Owner, true);
    // 08/11: sem flecha própria (o Arco de Bétula, o arco inicial), a de bétula
    const FString Name = Bow && !Bow->Arrow.IsEmpty() ? Bow->Arrow : FString(TEXT("W_flecha_betula_blocos"));
    static TMap<FString, UStaticMesh*> Cache;
    UStaticMesh*& Found = Cache.FindOrAdd(Name);
    if (!Found)
    {
        Found = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Weapons/Models/%s.%s"), *Name, *Name));
        if (Found) Found->AddToRoot();
    }
    return Found;
}

uint8 VortexSwords::PowerKind(const AActor* Owner, uint8 Kind)
{
    const FSword* Sword = Held(Owner, false);
    if (!Sword) return Kind;
    return uint8(Sword->Weapon == TEXT("arco_de_betula") ? EVortexDamageKind::Ranged : EVortexDamageKind::Melee);
}

FName VortexSwords::PowerElement(const AActor* Owner)
{
    const FSword* Sword = Held(Owner, false);
    return Sword ? Sword->Element : NAME_None;
}
