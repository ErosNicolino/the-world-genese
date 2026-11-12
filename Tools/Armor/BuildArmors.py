"""As 21 armaduras do jogo (07/11, das pranchas do usuário: 3 por raridade, do Normal ao Divino), 84 peças.

python Tools/Armor/BuildArmors.py   ->   Data/Rpg/armors.json   (o jogo: as passivas, o visual e o cabelo de cada peça)
                                         Data/UI/SkillIcons/Novos/armor_<conjunto>_<peça>.png   (o ícone na mochila)
Depois: python Tools/BuildArsenal.py (os itens e os conjuntos no arsenal.json).

Cada peça: defesa, vida e resistência pela raridade e pelo lugar (o peitoral protege mais, as botas menos), 1 ou 2
atributos do tema e uma passiva própria. Cada conjunto: 2 peças dão atributos; 4 peças, uma passiva forte e exclusiva.
As passivas são efeitos do código (Source/Vortex/Private/Rpg/VortexArmor.cpp) com os números daqui: ajustar não precisa
compilar. Peças com a mesma passiva somam (duas peças com 3% de bloqueio = 6%); tempos de espera usam o menor.

O cabelo (hair): "hide" = o capacete fecha a cabeça e o cabelo some; "fringe" = capuz ou gorro: o cabelo some e o
capuz tem uma franja na cor do cabelo do herói (Tools/Blender/MakeBlockArmor.py).
"""
import json
import os

from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PIECES = os.path.join(ROOT, "Data", "VortexArmor", "Pieces")
ICONS = os.path.join(ROOT, "Data", "UI", "SkillIcons", "Novos")
OUT = os.path.join(ROOT, "Data", "Rpg", "armors.json")

RANKS = ["common", "uncommon", "rare", "epic", "legendary", "unique", "divine"]
TIER_KEY = {"common": "comum", "uncommon": "incomum", "rare": "raro", "epic": "epico", "legendary": "lendario", "unique": "unico", "divine": "divino"}
RARITY_COLOR = {"common": (201, 194, 178), "uncommon": (111, 208, 106), "rare": (85, 156, 240), "epic": (176, 108, 242),
                "legendary": (242, 162, 58), "unique": (62, 214, 196), "divine": (255, 236, 160)}
PRICE = {"common": 25, "uncommon": 70, "rare": 180, "epic": 460, "legendary": 1150, "unique": 2900, "divine": 7000}
# o conjunto inteiro (as 4 peças): defesa, resistência e vida; cada lugar leva uma parte
SET_DEF = {"common": 4, "uncommon": 7, "rare": 11, "epic": 16, "legendary": 22, "unique": 26, "divine": 32}
SET_RES = {"common": 2, "uncommon": 4, "rare": 7, "epic": 11, "legendary": 15, "unique": 18, "divine": 22}
SET_HP = {"common": 10, "uncommon": 25, "rare": 45, "epic": 75, "legendary": 115, "unique": 140, "divine": 180}
SHARE = {"head": .22, "chest": .36, "legs": .26, "boots": .16}
SLOT_LABEL = {"head": "Capacete", "chest": "Peitoral", "legs": "Calças", "boots": "Botas"}

# ---- as passivas: tipo -> (nome, texto com os números). Os textos aparecem no item.
def pct(v):
    return ("%.1f" % (v * 100)).rstrip("0").rstrip(".").replace(".", ",") + "%"


PASSIVES = {
    "coins": ("Bolso Fundo", lambda v: "+%s de moedas" % pct(v["amount"])),
    "xp": ("Aprendiz", lambda v: "+%s de experiência" % pct(v["amount"])),
    "regen": ("Fôlego", lambda v: "recupera %s da vida a cada 5 s" % pct(v["amount"])),
    "mana_regen": ("Mente Calma", lambda v: "recupera %s da mana a cada 5 s" % pct(v["amount"])),
    "dodge_cd": ("Pés Leves", lambda v: "a esquiva volta %s mais rápido" % pct(v["amount"])),
    "move": ("Passo Firme", lambda v: "+%s de velocidade ao andar" % pct(v["amount"])),
    "block": ("Bloqueio", lambda v: "%s de chance de anular um golpe" % pct(v["chance"])),
    "first_hit": ("Guarda Alta", lambda v: "o primeiro golpe a cada %d s tira %s menos" % (v["every"], pct(v["amount"]))),
    "last_stand": ("Última Defesa", lambda v: "com menos de 30%% da vida, leva %s menos de dano" % pct(v["amount"])),
    "kill_heal": ("Vitória", lambda v: "cada inimigo vencido devolve %s da vida" % pct(v["amount"])),
    "beast": ("Caçador", lambda v: "+%s de dano contra feras" % pct(v["amount"])),
    "thorns": ("Espinhos", lambda v: "devolve %s do dano recebido a quem bateu" % pct(v["amount"])),
    "dodge_crit": ("Golpe Oportuno", lambda v: "depois de esquivar, o próximo golpe em %d s é crítico" % v["window"]),
    "chill": ("Toque Gelado", lambda v: "%s de chance de deixar lento quem te acerta" % pct(v["chance"])),
    "freeze": ("Coração Gelado", lambda v: "%s de chance de congelar quem te acerta por %s s" % (pct(v["chance"]), str(v["time"]).replace(".", ","))),
    "proc_poison": ("Veneno", lambda v: "%s de chance de envenenar o alvo" % pct(v["chance"])),
    "proc_burn": ("Brasa", lambda v: "%s de chance de queimar o alvo" % pct(v["chance"])),
    "burn_attacker": ("Pele de Brasa", lambda v: "%s de chance de queimar quem te acerta" % pct(v["chance"])),
    "burn_immune": ("Sangue Quente", lambda v: "não queima"),
    "magma_burst": ("Erupção", lambda v: "%s de chance de explodir em lava em volta ao ser atingido" % pct(v["chance"])),
    "leech": ("Fome da Noite", lambda v: "rouba %s do dano como vida (%s à noite)" % (pct(v["day"]), pct(v["night"]))),
    "revive": ("Renascer", lambda v: "ao cair, levanta com %s da vida (a cada %d min)" % (pct(v["amount"]), v["every"] // 60)),
    "ally_heal": ("Abrigo", lambda v: "cura você e os aliados a até %d m em %s da vida por segundo" % (v["radius"] // 100, pct(v["amount"]))),
    "starfall": ("Chuva de Estrelas", lambda v: "a cada %d s uma estrela cai num inimigo a até %d m" % (v["every"], v["radius"] // 100)),
    "full_power": ("Plenitude", lambda v: "com a vida cheia, +%s de dano" % pct(v["amount"])),
}


def P(kind, **values):
    return {"kind": kind, "values": values}


# conjunto: chave, nome, raridade, elemento, [(lugar, nome, cabelo, atributos extras, passiva)], bônus de 2, (nome, passiva de 4)
S = [
    # ---- Normal
    ("viajante", "Couro do Viajante", "common", "", [
        ("head", "Capuz do Viajante", "fringe", {"haste": 1}, P("coins", amount=.04)),
        ("chest", "Colete do Viajante", None, {}, P("regen", amount=.01)),
        ("legs", "Calças do Viajante", None, {}, P("dodge_cd", amount=.05)),
        ("boots", "Botas do Viajante", None, {}, P("move", amount=.03))],
     {"haste": 3}, ("Pé na Estrada", P("move", amount=.06))),
    ("recruta", "Gibão do Recruta", "common", "", [
        ("head", "Chapéu de Ferro do Recruta", "fringe", {}, P("block", chance=.03)),
        ("chest", "Gibão Acolchoado", None, {"max_health": 5}, P("first_hit", amount=.25, every=12)),
        ("legs", "Calças Acolchoadas", None, {}, P("last_stand", amount=.08)),
        ("boots", "Botas Enfaixadas", None, {}, P("kill_heal", amount=.01))],
     {"defense": 1, "max_health": 10}, ("Disciplina", P("first_hit", amount=.5, every=10))),
    ("cacador", "Peles do Caçador", "common", "", [
        ("head", "Gorro de Pele", "fringe", {"crit": 1}, P("beast", amount=.04)),
        ("chest", "Casaco de Peles", None, {}, P("beast", amount=.05)),
        ("legs", "Calças de Peles", None, {}, P("move", amount=.03)),
        ("boots", "Botas de Pele", None, {}, P("kill_heal", amount=.01))],
     {"crit": 2}, ("Instinto do Caçador", P("beast", amount=.12))),
    # ---- Incomum
    ("malha_ferro", "Malha de Ferro", "uncommon", "", [
        ("head", "Elmo de Nasal", "hide", {}, P("block", chance=.03)),
        ("chest", "Cota de Malha Reforçada", None, {}, P("block", chance=.04)),
        ("legs", "Perneiras de Malha", None, {}, P("block", chance=.03)),
        ("boots", "Botas de Ferro", None, {}, P("last_stand", amount=.08))],
     {"defense": 2, "resistance": 2}, ("Muralha de Ferro", P("block", chance=.08))),
    ("batedor", "Couro Batido do Batedor", "uncommon", "", [
        ("head", "Capuz do Batedor", "fringe", {"crit": 2}, P("dodge_cd", amount=.06)),
        ("chest", "Gibão Tachonado", None, {"speed": 3}, P("dodge_crit", window=2)),
        ("legs", "Calças Tachonadas", None, {}, P("dodge_cd", amount=.06)),
        ("boots", "Botas do Batedor", None, {}, P("move", amount=.04))],
     {"crit": 3, "speed": 3}, ("Pés Leves", P("dodge_cd", amount=.2))),
    ("bronze", "Escamas de Bronze", "uncommon", "", [
        ("head", "Elmo de Escamas", "hide", {}, P("thorns", amount=.03)),
        ("chest", "Couraça de Escamas", None, {}, P("thorns", amount=.05)),
        ("legs", "Saiote de Escamas", None, {}, P("thorns", amount=.04)),
        ("boots", "Botas de Bronze", None, {}, P("thorns", amount=.03))],
     {"defense": 2, "max_health": 15}, ("Escamas Afiadas", P("thorns", amount=.12))),
    # ---- Raro
    ("guarda_real", "Placas do Guarda Real", "rare", "", [
        ("head", "Elmo de Penacho Azul", "hide", {}, P("block", chance=.04)),
        ("chest", "Peitoral com Tabardo", None, {"max_health": 15}, P("first_hit", amount=.3, every=10)),
        ("legs", "Coxotes do Guarda", None, {}, P("last_stand", amount=.1)),
        ("boots", "Grevas do Guarda", None, {}, P("block", chance=.03))],
     {"defense": 3, "resistance": 3}, ("Juramento do Guarda", P("last_stand", amount=.25))),
    ("geada", "Couraça de Gelo", "rare", "ice", [
        ("head", "Elmo de Cristal de Gelo", "hide", {"max_mana": 10}, P("chill", chance=.08)),
        ("chest", "Couraça de Gelo", None, {}, P("chill", chance=.1)),
        ("legs", "Calças de Gelo e Pele", None, {}, P("chill", chance=.08)),
        ("boots", "Botas de Neve", None, {}, P("move", amount=.04))],
     {"resistance": 5}, ("Coração Gelado", P("freeze", chance=.12, time=1.2))),
    ("mare", "Armadura da Maré", "rare", "water", [
        ("head", "Elmo de Conchas", "hide", {"max_mana": 12}, P("mana_regen", amount=.02)),
        ("chest", "Couraça de Coral", None, {}, P("regen", amount=.012)),
        ("legs", "Calças de Escama Marinha", None, {}, P("regen", amount=.01)),
        ("boots", "Botas da Maré", None, {}, P("mana_regen", amount=.02))],
     {"hp_regen": 2, "max_mana": 10}, ("Maré Cheia", P("regen", amount=.025))),
    # ---- Épico
    ("noite_violeta", "Placas da Noite Violeta", "epic", "shadow", [
        ("head", "Elmo Chifrudo da Noite", "hide", {"attack": 2}, P("leech", day=.01, night=.02)),
        ("chest", "Peitoral Rúnico da Noite", None, {}, P("leech", day=.015, night=.03)),
        ("legs", "Perneiras Rúnicas", None, {}, P("leech", day=.01, night=.02)),
        ("boots", "Botas Rúnicas", None, {}, P("move", amount=.04))],
     {"attack": 3, "lifesteal": 2}, ("Fome da Noite", P("leech", day=.05, night=.1))),
    ("escorpiao", "Carapaça do Escorpião", "epic", "poison", [
        ("head", "Elmo do Ferrão", "hide", {"crit": 3}, P("proc_poison", chance=.08)),
        ("chest", "Carapaça de Quitina", None, {}, P("proc_poison", chance=.1)),
        ("legs", "Perneiras de Quitina", None, {}, P("thorns", amount=.05)),
        ("boots", "Botas de Garra", None, {}, P("proc_poison", chance=.08))],
     {"crit": 4, "attack": 2}, ("Veneno do Deserto", P("proc_poison", chance=.35))),
    ("dragao_rubro", "Escamas do Dragão Rubro", "epic", "fire", [
        ("head", "Elmo do Dragão", "hide", {"attack": 2}, P("burn_immune")),
        ("chest", "Peitoral de Escamas Rubras", None, {}, P("burn_attacker", chance=.12)),
        ("legs", "Perneiras Rubras", None, {}, P("proc_burn", chance=.08)),
        ("boots", "Garras do Dragão", None, {}, P("proc_burn", chance=.08))],
     {"attack": 3, "max_health": 20}, ("Sangue de Dragão", P("proc_burn", chance=.35))),
    # ---- Lendário
    ("grifo_dourado", "Placas do Grifo Dourado", "legendary", "", [
        ("head", "Elmo Alado do Grifo", "hide", {"crit": 4}, P("dodge_cd", amount=.08)),
        ("chest", "Peitoral do Grifo", None, {"speed": 5}, P("full_power", amount=.08)),
        ("legs", "Coxotes Dourados", None, {}, P("move", amount=.05)),
        ("boots", "Garras do Grifo", None, {}, P("move", amount=.05))],
     {"crit": 5, "speed": 6}, ("Voo do Grifo", P("dodge_cd", amount=.25))),
    ("magma", "Armadura de Magma", "legendary", "fire", [
        ("head", "Elmo de Obsidiana", "hide", {"attack": 3}, P("burn_immune")),
        ("chest", "Couraça de Magma", None, {}, P("magma_burst", chance=.08, radius=380, scale=3.0)),
        ("legs", "Perneiras de Magma", None, {}, P("burn_attacker", chance=.12)),
        ("boots", "Botas de Lava", None, {}, P("magma_burst", chance=.06, radius=380, scale=3.0))],
     {"attack": 4, "resistance": 6}, ("Erupção", P("magma_burst", chance=.2, radius=480, scale=4.0))),
    ("bosque_ancestral", "Couraça do Bosque Ancestral", "legendary", "nature", [
        ("head", "Elmo de Galhos", "hide", {"hp_regen": 2}, P("regen", amount=.012)),
        ("chest", "Couraça de Casca", None, {"max_health": 25}, P("regen", amount=.015)),
        ("legs", "Saiote de Folhas", None, {}, P("thorns", amount=.06)),
        ("boots", "Botas de Raiz", None, {}, P("last_stand", amount=.12))],
     {"hp_regen": 3, "max_health": 30}, ("Seiva Ancestral", P("regen", amount=.035))),
    # ---- Único
    ("espelho", "Armadura Espelhada", "unique", "shadow", [
        ("head", "Elmo Espelhado", "hide", {"skill": 4}, P("thorns", amount=.06)),
        ("chest", "Peitoral Espelhado", None, {}, P("thorns", amount=.08)),
        ("legs", "Coxotes Espelhados", None, {}, P("block", chance=.05)),
        ("boots", "Botas Espelhadas", None, {}, P("thorns", amount=.06))],
     {"resistance": 8, "skill": 4}, ("Reflexo Perfeito", P("thorns", amount=.25))),
    ("sem_sombra", "Armadura Sem Sombra", "unique", "holy", [
        ("head", "Elmo Sem Sombra", "hide", {"crit": 5}, P("dodge_cd", amount=.1)),
        ("chest", "Peitoral Sem Sombra", None, {"crit_dmg": 15}, P("full_power", amount=.1)),
        ("legs", "Calças Sem Sombra", None, {}, P("dodge_cd", amount=.1)),
        ("boots", "Botas Sem Sombra", None, {}, P("move", amount=.06))],
     {"crit": 6, "crit_dmg": 20}, ("Golpe Sem Sombra", P("dodge_crit", window=3))),
    ("andarilho", "Vestes do Andarilho Perdido", "unique", "", [
        ("head", "Gorro Remendado", "fringe", {"haste": 4}, P("coins", amount=.08)),
        ("chest", "Colete Remendado", None, {}, P("xp", amount=.06)),
        ("legs", "Calças Remendadas", None, {}, P("coins", amount=.08)),
        ("boots", "Botas Desparelhadas", None, {}, P("move", amount=.06))],
     {"haste": 6, "max_health": 30}, ("Sorte de Viajante", P("xp", amount=.15))),
    # ---- Divino
    ("aurora", "Armadura da Aurora", "divine", "holy", [
        ("head", "Elmo Alado da Aurora", "hide", {"skill": 5}, P("regen", amount=.015)),
        ("chest", "Peitoral da Aurora", None, {"max_health": 40}, P("last_stand", amount=.15)),
        ("legs", "Saiote de Penas", None, {}, P("regen", amount=.015)),
        ("boots", "Botas Aladas", None, {}, P("move", amount=.07))],
     {"max_health": 50, "resistance": 8}, ("Renascer na Aurora", P("revive", amount=.3, every=300))),
    ("arvore_mundo", "Couraça da Árvore do Mundo", "divine", "nature", [
        ("head", "Elmo de Madeira Viva", "hide", {"hp_regen": 3}, P("regen", amount=.015)),
        ("chest", "Couraça da Árvore", None, {"max_health": 40}, P("ally_heal", amount=.004, radius=900)),
        ("legs", "Perneiras de Raízes", None, {}, P("thorns", amount=.08)),
        ("boots", "Botas de Raiz Antiga", None, {}, P("last_stand", amount=.15))],
     {"hp_regen": 4, "max_health": 50}, ("Abrigo da Árvore", P("ally_heal", amount=.012, radius=1000))),
    ("estrelas", "Manto das Estrelas", "divine", "holy", [
        ("head", "Elmo Estelar", "hide", {"skill": 6}, P("mana_regen", amount=.03)),
        ("chest", "Peitoral das Constelações", None, {"crit": 6}, P("full_power", amount=.12)),
        ("legs", "Coxotes Estelares", None, {}, P("dodge_cd", amount=.1)),
        ("boots", "Botas Estelares", None, {}, P("move", amount=.07))],
     {"crit": 8, "skill": 8}, ("Chuva de Estrelas", P("starfall", every=8, radius=1600, damage=12))),
]


# onde se acha: as comuns e incomuns nas lojas (o Garrick vende as de metal; a Rosa, as de couro e pano); das raras para
# cima, só no saque dos bichos (cada peça, a chance por bicho vencido; os semi-chefes sorteiam 3 vezes)
SHOP = {"viajante": "rosa", "recruta": "rosa", "cacador": "rosa", "malha_ferro": "garrick", "batedor": "rosa", "bronze": "garrick"}
LOOT = {"rare": {"bandit": .015, "goblin": .015, "golem": .01}, "epic": {"golem": .015, "bandit": .005, "runecolossus": .01},
        "legendary": {"runecolossus": .012, "golem": .008}, "unique": {"runecolossus": .008, "golem": .006},
        "divine": {"runecolossus": .004, "golem": .003}}


def describe(p):
    name, text = PASSIVES[p["kind"]]
    return name, text(p["values"])


def icon(src, rarity):
    """O ícone: o recorte da prancha num fundo azul-noite com o brilho da raridade (como os das espadas)."""
    art = Image.open(src).convert("RGBA")
    k = 160 / max(art.size)
    art = art.resize((max(1, int(art.width * k)), max(1, int(art.height * k))), Image.LANCZOS)
    S = 192
    c = RARITY_COLOR[rarity]
    bg = Image.new("RGB", (S, S))
    d = ImageDraw.Draw(bg)
    for r in range(S, 0, -2):
        t = r / S
        col = tuple(int((22 + 30 * (1 - t)) * (1 - .35 * (1 - t)) + c[i] * .22 * (1 - t) ** 2) for i in range(3))
        d.ellipse((S / 2 - r * .75, S / 2 - r * .75, S / 2 + r * .75, S / 2 + r * .75), fill=col)
    out = bg.convert("RGBA")
    alpha = Image.new("L", (S, S), 0)
    alpha.paste(art.split()[3], ((S - art.width) // 2, (S - art.height) // 2))
    halo = alpha.filter(ImageFilter.MaxFilter(9)).filter(ImageFilter.GaussianBlur(6))
    glow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    glow.paste(Image.new("RGBA", (S, S), c + (255,)), (0, 0), halo.point(lambda v: v * 3 // 5))
    out.alpha_composite(glow)
    out.alpha_composite(art, ((S - art.width) // 2, (S - art.height) // 2))
    ImageDraw.Draw(out).rectangle((1, 1, S - 2, S - 2), outline=tuple(int(v * .7) for v in c) + (255,), width=3)
    return out.convert("RGB")


if __name__ == "__main__":
    os.makedirs(ICONS, exist_ok=True)
    data = {"_doc": "Gerado por Tools/Armor/BuildArmors.py. As 21 armaduras: peças, passivas, visual e cabelo.", "sets": {}, "pieces": {}}
    per_tier = {}
    for key, name, rarity, element, pieces, bonus2, (set_name, passive4) in S:
        n = per_tier[rarity] = per_tier.get(rarity, 0) + 1
        source = "%s_%d" % (TIER_KEY[rarity], n)
        ids = {}
        for slot, piece_name, hair, extra, passive in pieces:
            item = "arm_%s_%s" % (key, slot)
            ids[slot] = item
            stats = {"defense": max(1, round(SET_DEF[rarity] * SHARE[slot])), "resistance": round(SET_RES[rarity] * SHARE[slot]),
                     "max_health": round(SET_HP[rarity] * SHARE[slot])}
            for k, v in extra.items():
                stats[k] = stats.get(k, 0) + v
            stats = {k: v for k, v in stats.items() if v}
            pname, ptext = describe(passive)
            entry = {"set": key, "slot": slot, "name": piece_name, "rarity": rarity, "element": element, "stats": stats,
                     "price": int(PRICE[rarity] * {"head": 1, "chest": 2, "legs": 1.5, "boots": 1}[slot]),
                     "icon": "armor_%s_%s" % (key, slot), "source": "%s_%s" % (source, slot),
                     "mesh": "A_%s_%s" % (key, slot), "passive": dict(passive, name=pname, text=ptext)}
            if slot == "head":
                entry["hair"] = hair
            if key in SHOP:
                entry["shop"] = SHOP[key]
            data["pieces"][item] = entry
            icon(os.path.join(PIECES, "%s_%s.png" % (source, slot)), rarity).save(os.path.join(ICONS, "armor_%s_%s.png" % (key, slot)))
        p4name, p4text = describe(passive4)
        data["sets"][key] = {"name": name, "rarity": rarity, "element": element, "source": source, "pieces": ids, "bonus2": bonus2,
                             "passive4": dict(passive4, name=set_name, text=p4text)}
    # as armaduras antigas (rpg.json e os conjuntos do arsenal) vestem o visual do conjunto novo mais parecido: só o
    # visual (os atributos e o bônus delas continuam os delas; não contam para a passiva das 4 peças dos novos)
    VISUAL = {"helm_leather": "viajante_head", "helm_iron": "malha_ferro_head", "chest_tunic": "viajante_chest", "chest_mail": "malha_ferro_chest",
              "chest_punk": "batedor_chest", "legs_cloth": "recruta_legs", "legs_leather": "viajante_legs", "boots_leather": "viajante_boots",
              "boots_iron": "malha_ferro_boots"}
    for old, new in (("wolf", "cacador"), ("guard", "guarda_real"), ("bark", "bosque_ancestral"), ("crater", "magma"), ("dunas", "escorpiao"),
                     ("sombra", "noite_violeta"), ("sabio", "espelho"), ("azhur", "dragao_rubro"), ("yggdrasil", "arvore_mundo")):
        for slot in ("head", "chest", "legs", "boots"):
            VISUAL["%s_%s" % (old, slot)] = "%s_%s" % (new, slot)
    data["visual"] = {old: "arm_" + new for old, new in VISUAL.items()}
    assert all(v in data["pieces"] for v in data["visual"].values())
    data["loot"] = {}
    for item, p in data["pieces"].items():
        for species, chance in LOOT.get(p["rarity"], {}).items():
            data["loot"].setdefault(species, []).append({"item": item, "min": 1, "max": 1, "chance": chance})
    assert len(data["sets"]) == 21 and len(data["pieces"]) == 84
    for p in data["pieces"].values():
        assert p["passive"]["kind"] in PASSIVES
    json.dump(data, open(OUT, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    kinds = sorted({p["passive"]["kind"] for p in data["pieces"].values()} | {s["passive4"]["kind"] for s in data["sets"].values()})
    print("armaduras: %d conjuntos, %d peças; passivas: %s" % (len(data["sets"]), len(data["pieces"]), ", ".join(kinds)))
