#include "Combat/VortexWeaponKit.h"

#include "Combat/VortexCombatComponent.h"
#include "Combat/VortexWeaponFx.h"
#include "Creatures/VortexCreature.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexRpg.h"

namespace
{
    using VortexWeaponKit::EMuzzle;
    using VortexWeaponKit::FKit;

    FKit Kit(const TCHAR* Family, float PrimaryMana, float SpecialMana, float Cooldown, const TCHAR* Special, EMuzzle Muzzle,
        std::initializer_list<TPair<FName, float>> Values = {})
    {
        FKit K;
        K.Family = Family;
        K.PrimaryMana = PrimaryMana;
        K.SpecialMana = SpecialMana;
        K.SpecialCooldown = Cooldown;
        K.Special = Special;
        K.Muzzle = Muzzle;
        for (const TPair<FName, float>& V : Values) K.Values.Add(V.Key, V.Value);
        return K;
    }

    const TMap<FName, FKit>& Table()
    {
        static TMap<FName, FKit> All;
        if (All.Num()) return All;
        // ---- mágicas: pouco no golpe normal, mais no secundário (G6)
        All.Add(TEXT("instrumento"), Kit(TEXT("Lute"), 2.f, 4.f, 20.f, TEXT("song"), EMuzzle::Body,
            { { TEXT("damage"), .15f }, { TEXT("armor"), .15f }, { TEXT("radius"), 800.f }, { TEXT("self"), 15.f }, { TEXT("allies"), 3.f } }));
        All.Add(TEXT("orbe"), Kit(TEXT("Orb"), 3.f, 20.f, 25.f, TEXT("orbit"), EMuzzle::Tip,
            { { TEXT("life"), 12.f }, { TEXT("every"), 1.5f }, { TEXT("scale"), .9f }, { TEXT("undead"), .5f }, { TEXT("reach"), 1800.f } }));
        All.Add(TEXT("livro_da_aurora"), Kit(TEXT("Book"), 3.f, 25.f, 18.f, TEXT("firewall"), EMuzzle::Tip,
            { { TEXT("tornado_life"), 2.5f }, { TEXT("tornado_radius"), 250.f }, { TEXT("tornado_cooldown"), 3.f }, { TEXT("reach"), 2200.f },
              { TEXT("width"), 800.f }, { TEXT("life"), 6.f }, { TEXT("scale"), 1.2f } }));
        All.Add(TEXT("varinha"), Kit(TEXT("Wand"), 2.f, 12.f, 6.f, TEXT("overload"), EMuzzle::Tip,
            { { TEXT("charge"), .6f }, { TEXT("scale"), 4.f } }));
        All.Add(TEXT("cajado_de_ilsa"), Kit(TEXT("Staff"), 3.f, 20.f, 16.f, TEXT("vines"), EMuzzle::Tip,
            { { TEXT("cast"), 1.2f }, { TEXT("hold"), 3.f }, { TEXT("hold_boss"), 1.5f }, { TEXT("reach"), 2600.f } }));
        All.Add(TEXT("catalisador"), Kit(TEXT("Flask"), 2.f, 20.f, 20.f, TEXT("potion"), EMuzzle::Grip,
            { { TEXT("radius"), 400.f }, { TEXT("buff"), .15f }, { TEXT("life"), 10.f }, { TEXT("acid"), .1f }, { TEXT("acid_life"), 4.f }, { TEXT("reach"), 1600.f } }));
        // ---- de perto (não gastam mana, menos a maça, que é sagrada)
        All.Add(TEXT("maca_sagrada"), Kit(TEXT("Mace"), 0.f, 15.f, 14.f, TEXT("holyslam"), EMuzzle::Tip,
            { { TEXT("length"), 700.f }, { TEXT("stun"), 2.5f }, { TEXT("levels"), 5.f }, { TEXT("heal"), .08f }, { TEXT("scale"), 4.f } }));
        All.Add(TEXT("manoplas_de_ferro"), Kit(TEXT("Fist"), 0.f, 20.f, 9.f, TEXT("seriouspunch"), EMuzzle::Tip));
        All.Add(TEXT("adagas_gemeas"), Kit(TEXT("Dagger"), 0.f, 0.f, 8.f, TEXT("dash"), EMuzzle::Tip,
            { { TEXT("distance"), 600.f }, { TEXT("time"), .25f }, { TEXT("scale"), 3.5f } }));
        All.Add(TEXT("alabarda"), Kit(TEXT("Halberd"), 0.f, 0.f, 12.f, TEXT("shockwave"), EMuzzle::Tip,
            { { TEXT("reach"), 450.f }, { TEXT("length"), 800.f }, { TEXT("scale"), 4.5f } }));
        // (os multiplicadores dos secundários são do dano do golpe normal: bem maiores, pela recarga e pela demora)
        All.Add(TEXT("lanca_do_patrulheiro"), Kit(TEXT("Spear"), 0.f, 0.f, 10.f, TEXT("flurry"), EMuzzle::Tip,
            { { TEXT("count"), 6.f }, { TEXT("time"), 1.2f }, { TEXT("scale"), 1.1f } }));
        All.Add(TEXT("martelo_de_bram"), Kit(TEXT("Hammer"), 0.f, 0.f, 12.f, TEXT("groundslam"), EMuzzle::Tip,
            { { TEXT("radius"), 400.f }, { TEXT("scale"), 4.5f } }));
        All.Add(TEXT("machados_duplos"), Kit(TEXT("DualAxes"), 0.f, 0.f, 10.f, TEXT("throwaxes"), EMuzzle::Grip,
            { { TEXT("reach"), 1200.f }, { TEXT("scale"), 3.f }, { TEXT("speed"), 2600.f } }));
        All.Add(TEXT("machado"), Kit(TEXT("Axe"), 0.f, 0.f, 14.f, TEXT("whirl"), EMuzzle::Tip,
            { { TEXT("time"), 2.f }, { TEXT("hits"), 4.f }, { TEXT("radius"), 260.f }, { TEXT("scale"), 1.8f } }));
        All.Add(TEXT("espadao"), Kit(TEXT("Greatsword"), 0.f, 0.f, 12.f, TEXT("sweep"), EMuzzle::Tip,
            { { TEXT("reach"), 420.f }, { TEXT("push"), 600.f }, { TEXT("scale"), 4.5f } }));
        {
            FKit Shield = Kit(TEXT("SwordShield"), 0.f, 0.f, 0.f, TEXT("shield"), EMuzzle::Tip);
            Shield.GuardKeep = .2f;
            All.Add(TEXT("espada_escudo"), Shield);
        }
        All.Add(TEXT("correntes"), Kit(TEXT("Chain"), 0.f, 0.f, 9.f, TEXT("chainflurry"), EMuzzle::Tip,
            { { TEXT("reach"), 500.f }, { TEXT("count"), 3.f }, { TEXT("time"), .8f }, { TEXT("scale"), 1.7f } }));
        All.Add(TEXT("chicote"), Kit(TEXT("Whip"), 0.f, 0.f, 6.f, TEXT("grapple"), EMuzzle::Tip,
            { { TEXT("reach"), 1500.f }, { TEXT("pull"), 2500.f }, { TEXT("push"), 400.f }, { TEXT("scale"), 2.8f } }));
        All.Add(TEXT("foice"), Kit(TEXT("Scythe"), 0.f, 0.f, 10.f, TEXT("reap"), EMuzzle::Tip,
            { { TEXT("time"), .45f }, { TEXT("radius"), 320.f }, { TEXT("scale"), 4.f } }));
        // ---- distância
        All.Add(TEXT("chakrams"), Kit(TEXT("Chakram"), 0.f, 0.f, 12.f, TEXT("giantchakram"), EMuzzle::Grip,
            { { TEXT("reach"), 1400.f }, { TEXT("return"), .6f }, { TEXT("giant_reach"), 1600.f }, { TEXT("giant_size"), 200.f }, { TEXT("scale"), 1.2f } }));   // acerta na ida e na volta: 2 dava 4x o golpe, em área
        All.Add(TEXT("pistola_runica"), Kit(TEXT("Pistol"), 1.f, 15.f, 12.f, TEXT("runeseal"), EMuzzle::Tip,
            { { TEXT("delay"), 1.5f }, { TEXT("radius"), 300.f }, { TEXT("scale"), 4.5f }, { TEXT("mark"), .25f }, { TEXT("mark_life"), 5.f }, { TEXT("reach"), 2600.f } }));
        All.Add(TEXT("besta"), Kit(TEXT("Crossbow"), 0.f, 0.f, 9.f, TEXT("burst3"), EMuzzle::Tip,
            { { TEXT("reach"), 4000.f }, { TEXT("count"), 3.f }, { TEXT("time"), .45f }, { TEXT("scale"), 1.8f } }));
        All.Add(TEXT("arco_de_betula"), Kit(TEXT("Bow"), 0.f, 20.f, 16.f, TEXT("arrowrain"), EMuzzle::Grip,
            // 03/11: carga cheia em 1,1 s; o tiro rápido dá 0,3x e cai, o cheio 1,7x (antes 0,8 s, 0,5x a 1,5x: dava
            // para metralhar); min_draw e gap: a corda mínima e o intervalo entre flechas
            { { TEXT("reach"), 2500.f }, { TEXT("full"), 1.1f }, { TEXT("min"), .3f }, { TEXT("max"), 1.7f }, { TEXT("min_draw"), .35f }, { TEXT("gap"), .5f },
              { TEXT("radius"), 400.f }, { TEXT("life"), 1.8f }, { TEXT("arrows"), 20.f }, { TEXT("scale"), .3f } }));
        // 02/11: era 1,3 por flecha. Com as flechas puxando para os inimigos (RainLanding) umas 15 das 20 acertam o
        // mesmo bicho: dava ~20 golpes num só. Com ,3 fica ~5 golpes num alvo (e mais, espalhado, num grupo)
        return All;
    }
}

FVector VortexWeaponKit::ViewAnchor(FName Family, FName& OutBone)
{
    // ponto da tela (cm, espaço da câmera: X à frente, Y à direita, Z para cima) onde fica a mão que segura a
    // arma na primeira pessoa, com o herói parado. A tela mostra ~27° para baixo e ~42° para o lado: a mão
    // fica perto da borda de baixo, aparecendo em parte, e a arma sobe dela para dentro da tela.
    // OutBone "Hands" = o meio das duas mãos (armas seguradas com as duas: pela mão direita, a esquerda, que
    // fica bem mais alta no cabo, ia parar flutuando no alto da tela)
    OutBone = TEXT("Hand_R");
    if (Family == TEXT("Greatsword") || Family == TEXT("Hammer") || Family == TEXT("Katana"))
    {
        OutBone = TEXT("Hands");
        return FVector(46.f, 9.f, -23.f);
    }
    // cabo comprido (as mãos bem separadas): o meio delas mais à direita e mais baixo, para a mão da frente
    // não ficar no meio da tela
    if (Family == TEXT("Halberd") || Family == TEXT("Spear") || Family == TEXT("Scythe"))
    {
        OutBone = TEXT("Hands");
        return FVector(50.f, 19.f, -28.f);
    }
    // as duas mãos à vista, uma em cada canto de baixo (luvas grandes: mais longe)
    if (Family == TEXT("Fist")) { OutBone = TEXT("Hands"); return FVector(50.f, 2.f, -24.f); }
    // grimório, orbe e alaúde: na frente do peito, baixos, a magia (ou a música) sai por cima
    if (Family == TEXT("Book") || Family == TEXT("Orb") || Family == TEXT("Lute")) { OutBone = TEXT("Hands"); return FVector(48.f, 6.f, -25.f); }
    // arco (na mão direita neste boneco): mais alto, o arco em pé no lado direito
    if (Family == TEXT("Bow")) return FVector(48.f, 17.f, -11.f);
    // besta e cajado: apoiados, a mão um pouco mais para o meio
    if (Family == TEXT("Crossbow") || Family == TEXT("Staff")) return FVector(44.f, 13.f, -18.f);
    // uma mão (espada, adagas, maça, machados, varinha, pistola, chicote, corrente, chakram, frasco)
    return FVector(42.f, 18.f, -17.f);
}

FLinearColor VortexWeaponKit::TrailColor(FName Family)
{
    if (Family == TEXT("Mace")) return FLinearColor(1.f, .85f, .4f);
    if (Family == TEXT("Axe") || Family == TEXT("DualAxes")) return FLinearColor(1.f, .62f, .35f);
    if (Family == TEXT("Greatsword")) return FLinearColor(.75f, .85f, 1.f);
    if (Family == TEXT("Halberd")) return FLinearColor(.9f, .9f, .8f);
    if (Family == TEXT("Scythe")) return FLinearColor(.72f, .5f, 1.f);
    if (Family == TEXT("Dagger")) return FLinearColor(.5f, 1.f, .95f);
    if (Family == TEXT("Chain")) return FLinearColor(.75f, .75f, .8f);
    if (Family == TEXT("Whip")) return FLinearColor(.85f, .6f, .4f);
    return FLinearColor::White;
}

const FVortexItemDef* VortexWeaponKit::HeldItem(const AActor* Hero)
{
    const UVortexHeroSheet* Sheet = Hero ? Hero->FindComponentByClass<UVortexHeroSheet>() : nullptr;
    return Sheet ? FVortexRpgCatalog::Get().Item(Sheet->GetEquipped(VortexRpg::Weapon)) : nullptr;
}

FName VortexWeaponKit::HeldElement(const AActor* Hero)
{
    const FVortexItemDef* Item = HeldItem(Hero);
    return Item ? Item->Element : NAME_None;
}

void VortexWeaponKit::OnWeaponHit(AActor* Hero, AActor* Target, float Damage)
{
    // o raio em cadeia acerta de novo: os saltos não saltam outra vez
    static bool bInside = false;
    if (bInside || !Hero || !Target || Damage <= 0.f) return;
    const FVortexItemDef* Item = HeldItem(Hero);
    if (!Item || (Item->Element.IsNone() && Item->Trait.IsNone())) return;
    UVortexCombatComponent* Mine = Hero->FindComponentByClass<UVortexCombatComponent>();
    UVortexCombatComponent* Their = Target->FindComponentByClass<UVortexCombatComponent>();
    if (!Mine || !Their) return;
    const IVortexCreature* Creature = Cast<IVortexCreature>(Target);
    const bool bBoss = Creature && Creature->IsBoss();
    const FName E = Item->Element, T = Item->Trait;
    if (E == TEXT("fire")) Their->ApplyDot(FMath::Max(1.f, Damage * .3f), 3.f, Hero);
    else if (E == TEXT("poison")) Their->ApplyDot(FMath::Max(1.f, Damage * .22f), 4.f, Hero);
    else if (E == TEXT("water")) Their->ApplySlow(.75f, 2.f);
    else if (E == TEXT("ice")) Their->ApplySlow(.65f, 2.f);
    else if (E == TEXT("nature") && FMath::FRand() < .25f) Their->ApplySlow(0.f, .5f);
    else if (E == TEXT("holy"))
    {
        // cura 2% do dano causado
        if (UVortexHeroSheet* Sheet = Hero->FindComponentByClass<UVortexHeroSheet>()) Sheet->HealBy(Damage * .02f);
    }
    else if (E == TEXT("lightning"))
    {
        // salta para até 2 inimigos perto do alvo, com 40% do dano
        TArray<FOverlapResult> Overlaps;
        FCollisionObjectQueryParams Pawns;
        Pawns.AddObjectTypesToQuery(ECC_Pawn);
        FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexChain), false, Target);
        Hero->GetWorld()->OverlapMultiByObjectType(Overlaps, Target->GetActorLocation(), FQuat::Identity, Pawns, FCollisionShape::MakeSphere(480.f), Query);
        int32 Jumps = 0;
        bInside = true;
        for (const FOverlapResult& O : Overlaps)
        {
            AActor* Next = O.GetActor();
            if (!Next || Next == Hero || !Mine->CanHarm(Next) || Jumps >= 2) continue;
            ++Jumps;
            Mine->ApplyHit(Next, FMath::Max(1.f, Damage * .4f), EVortexDamageKind::Magic, 0.f);
            AVortexWeaponFx::Spawn(Hero, TEXT("beam"), Target->GetActorLocation(), FRotator::ZeroRotator, [&](AVortexWeaponFx& F)
            {
                F.Color = FLinearColor(.6f, .8f, 1.f);
                F.Follow = Target;
                F.Target = Next->GetActorLocation();
                F.Life = .25f;
                F.Count = 1;   // raio: fino e brilhante
            });
        }
        bInside = false;
    }
    if (T == TEXT("bleed")) Their->ApplyDot(FMath::Max(1.f, Damage * .25f), 3.f, Hero);
    else if (T == TEXT("heavy") && !bBoss && FMath::FRand() < .2f) Their->Stun(.5f);
}

const VortexWeaponKit::FKit* VortexWeaponKit::Find(FName WeaponId)
{
    return Table().Find(WeaponId);
}

float VortexWeaponKit::PrimaryDamageScale(FName WeaponId)
{
    // Dano esperado por segundo do combo, usando a espada de cinzas como referÃªncia.
    // Armas de alcance e ataques que cobrem Ã¡rea ficam um pouco abaixo da referÃªncia.
    static const TMap<FName, float> Scales = []
    {
        TMap<FName, float> Values;
        Values.Add(TEXT("espadao"), .65f);
        Values.Add(TEXT("machado"), .58f);
        Values.Add(TEXT("machados_duplos"), .63f);
        Values.Add(TEXT("martelo_de_bram"), .62f);
        Values.Add(TEXT("lanca_do_patrulheiro"), .66f);
        Values.Add(TEXT("alabarda"), .65f);
        Values.Add(TEXT("adagas_gemeas"), .61f);
        Values.Add(TEXT("manoplas_de_ferro"), .60f);
        Values.Add(TEXT("maca_sagrada"), .60f);
        Values.Add(TEXT("katana_do_vento"), .51f);
        // 02/11: era .75 (a flecha carregada dava ~1,1 golpe de espada a cada 1,25 s; menos da metade do dano por
        // segundo). Agora a carregada vale ~2 golpes: um pouco abaixo da espada, pela distância
        Values.Add(TEXT("arco_de_betula"), 1.5f);
        Values.Add(TEXT("besta"), .52f);
        Values.Add(TEXT("pistola_runica"), .53f);
        Values.Add(TEXT("chakrams"), .25f);
        // 02/11: era ,73 (o usuário: "o cajado dá muito pouco dano"); a magia dele é de longe, mas lenta de sair
        // 03/11: 1,15 ainda era pouco (cada bolinha custa mana e voa até o alvo): 1,4
        Values.Add(TEXT("cajado_de_ilsa"), 1.4f);
        Values.Add(TEXT("varinha"), .49f);
        Values.Add(TEXT("livro_da_aurora"), .60f);
        Values.Add(TEXT("orbe"), .68f);
        Values.Add(TEXT("instrumento"), .85f);
        Values.Add(TEXT("foice"), .78f);
        Values.Add(TEXT("chicote"), .82f);
        Values.Add(TEXT("correntes"), .86f);
        Values.Add(TEXT("catalisador"), .80f);
        return Values;
    }();
    // 07/11: o balance.json manda (weaponDamageScale): o golpe multiplica o dano, e a escala iguala os tipos
    if (const float* Data = FVortexRpgCatalog::Get().WeaponDamageScale.Find(WeaponId)) return *Data;
    const float* Scale = Scales.Find(WeaponId);
    return Scale ? *Scale : 1.f;
}
