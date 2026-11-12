"""As 35 espadas do jogo (07/11, das pranchas do usuário, do Normal ao Divino): nome, raridade, elemento, o modelo de
blocos (Tools/Blender/MakePixelSwords.py) e o poder do botão direito, diferente do corte à distância da espada inicial.

python Tools/Weapons/BuildSwords.py   ->   Data/Rpg/swords.json            (o jogo: o modelo e o poder de cada espada)
                                           Data/UI/SkillIcons/Novos/weapon_espada_<chave>.png   (o ícone na mochila)
Depois: python Tools/BuildArsenal.py (põe os itens no arsenal.json) e, com o editor fechado, Tools/ImportItemIcons.py.

O usuário escolheu os nomes que sugeri e pediu os poderes "pelo rank": cada poder é um dos secundários que o jogo já
tem nas outras armas (VortexWeaponSpecials.cpp), com o nome, o elemento e os números da espada; a força sobe e a
recarga cai com a raridade (RANK). A Lâmina do Recruta é a espada inicial (o item espada_de_cinzas): fica com o
corte à distância. As do Godot que já existiam ganham o modelo e o nome que combinam (o id do item fica: os saves
continuam valendo).
"""
import json
import math
import os

from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SWORDS = os.path.join(ROOT, "Data", "VortexWeapons", "Swords")
ICONS = os.path.join(ROOT, "Data", "UI", "SkillIcons", "Novos")
OUT = os.path.join(ROOT, "Data", "Rpg", "swords.json")

# a força (multiplica o dano do poder), a recarga e a cor da raridade
RANK = {"common": (.75, 1.15), "uncommon": (.9, 1.05), "rare": (1.0, 1.0), "epic": (1.15, .95), "legendary": (1.3, .9),
        "unique": (1.45, .85), "divine": (1.6, .8)}
RARITY_COLOR = {"common": (201, 194, 178), "uncommon": (111, 208, 106), "rare": (85, 156, 240), "epic": (176, 108, 242),
                "legendary": (242, 162, 58), "unique": (62, 214, 196), "divine": (255, 236, 160)}
STATS = {"common": {"attack": 2}, "uncommon": {"attack": 3, "crit": 2}, "rare": {"attack": 4, "crit": 5, "crit_dmg": 10},
         "epic": {"attack": 6, "crit": 8, "crit_dmg": 20}, "legendary": {"attack": 9, "crit": 12, "crit_dmg": 35},
         "unique": {"attack": 11, "crit": 10, "crit_dmg": 30}, "divine": {"attack": 14, "crit": 14, "crit_dmg": 45}}
PRICE = {"common": 30, "uncommon": 80, "rare": 200, "epic": 520, "legendary": 1300, "unique": 3200, "divine": 8000}

# os poderes base (os números dos secundários das outras armas, VortexWeaponKit.cpp): tipo -> (mana, recarga, valores, ícone)
# 07/11: o "scale" multiplica o golpe inteiro do herói (Docs/02_Combate/BALANCEAMENTO_ARMAS.md): os de muitos acertos
# (as estrelas, a muralha, a chuva) desceram para casar com a recarga
BASE = {
    "dash": (0, 8, {"distance": 600, "time": .25, "scale": 3.5}, "special_adagas_gemeas"),
    "flurry": (0, 10, {"count": 6, "time": 1.2, "scale": 1.1}, "special_lanca_do_patrulheiro"),
    "sweep": (0, 12, {"reach": 420, "push": 600, "scale": 4.5}, "special_espadao"),
    "groundslam": (0, 12, {"radius": 400, "scale": 4.5}, "special_martelo_de_bram"),
    "reap": (0, 10, {"time": .45, "radius": 320, "scale": 4.0}, "special_foice"),
    "shockwave": (0, 12, {"reach": 450, "length": 800, "scale": 4.5}, "special_alabarda"),
    "whirl": (0, 14, {"time": 2.0, "hits": 4, "radius": 260, "scale": 1.8}, "special_machado"),
    "runeseal": (15, 12, {"delay": 1.5, "radius": 300, "scale": 4.5, "mark": .25, "mark_life": 5, "reach": 2600}, "special_pistola_runica"),
    "overload": (12, 6, {"charge": .6, "scale": 4.0}, "special_varinha"),
    "orbit": (20, 25, {"life": 12, "every": 1.5, "scale": .5, "undead": .5, "reach": 1800}, "special_orbe"),
    "firewall": (25, 18, {"tornado_life": 2.5, "tornado_radius": 250, "tornado_cooldown": 3, "reach": 2200, "width": 800, "life": 6, "scale": .6},
                 "special_livro_da_aurora"),
    "holyslam": (15, 14, {"length": 700, "stun": 2.5, "levels": 5, "heal": .08, "scale": 4.0}, "special_maca_sagrada"),
    "heal": (20, 16, {"heal_reach": 900}, "special_cajado_de_ilsa"),
    # 07/11 (o usuário: "uma maré na linha que mirei empurrando os inimigos"): a onda que corre na linha da mira
    "tidewave": (0, 12, {"length": 1400, "width": 320, "speed": 1500, "push": 900, "delay": .5, "scale": 3.4}, "special_espadao"),
}

# chave (o modelo), item (o id no jogo), nome, raridade, elemento, poder, nome do poder, o que faz, ajustes
S = [
    ("comum_1", "espada_de_cinzas", "Lâmina do Recruta", "common", "", None, "Corte à Distância", "lança o corte para a frente", {}),
    ("comum_2", "espada_de_treino", "Espada de Treino", "common", "", "dash", "Estocada de Treino",
     "avança 4,5 m e acerta quem estiver no caminho", {"distance": 450}),
    ("comum_3", "agulha_do_viajante", "Agulha do Viajante", "common", "", "flurry", "Mil Agulhadas",
     "cinco estocadas rápidas seguidas", {"count": 5}),
    ("comum_4", "espada_de_ferro", "Velha Companheira", "common", "", "sweep", "Varrida Cansada",
     "um golpe largo que empurra os inimigos", {"push": 400}),
    ("comum_5", "cutelo_do_lenhador", "Cutelo do Lenhador", "common", "", "groundslam", "Golpe do Lenhador",
     "crava a lâmina no chão e acerta em volta", {"radius": 300, "delay": .7}),
    ("incomum_1", "foice_verde_do_bosque", "Foice-Verde do Bosque", "uncommon", "nature", "reap", "Colheita Verde",
     "um giro rasteiro que corta tudo em volta", {}),
    ("incomum_2", "espada_do_soldado", "Espada de Cobre Batido", "uncommon", "", "shockwave", "Estrondo de Cobre",
     "uma onda de choque em linha para a frente", {"length": 650}),
    ("incomum_3", "lamina_da_mare_baixa", "Lâmina da Maré Baixa", "uncommon", "water", "tidewave", "Maré Rasteira",
     "varre o chão e solta uma onda que corre na linha da mira, arrastando e empurrando os inimigos", {}),
    ("incomum_4", "marco_do_guardiao", "Marco do Guardião", "uncommon", "", "groundslam", "Marco Fincado",
     "crava a espada no chão: o chão racha, as pedras sobem e quem está perto é abalado", {"radius": 380, "delay": .62}),
    ("incomum_5", "ferrao_da_noite", "Ferrão da Noite", "uncommon", "poison", "dash", "Picada Noturna",
     "avança como um ferrão e envenena o alvo", {"distance": 550, "dot": .35, "dot_time": 4}),
    ("raro_1", "espada_aco_frio", "Safira Gélida", "rare", "ice", "runeseal", "Selo de Gelo",
     "marca o chão com uma runa de gelo que explode logo depois", {"radius": 320}),
    ("raro_2", "lua_crescente", "Lua Crescente", "rare", "", "whirl", "Dança da Lua",
     "gira como a lua crescente, acertando várias vezes em volta", {"radius": 300}),
    ("raro_3", "trovao_partido", "Trovão Partido", "rare", "lightning", "overload", "Descarga do Trovão",
     "segura e solta: um raio carregado que explode no alvo", {"radius": 260}),
    ("raro_4", "presa_da_serpente", "Presa da Serpente", "rare", "poison", "flurry", "Bote da Serpente",
     "sete botes rápidos que espalham veneno", {"count": 7, "dot": .2, "dot_time": 4}),
    ("raro_5", "coracao_de_ametista", "Coração de Ametista", "rare", "shadow", "orbit", "Estilhaços de Ametista",
     "cristais roxos giram em volta e atacam quem chega perto", {}),
    ("epico_1", "monolito_desperto", "Monolito Desperto", "epic", "", "groundslam", "Queda do Monolito",
     "a lâmina de pedra cai como um monólito e racha o chão em volta", {"radius": 520, "delay": .75}),
    ("epico_2", "escritura_do_eclipse", "Escritura do Eclipse", "epic", "shadow", "runeseal", "Runa do Eclipse",
     "escreve uma runa de sombra que explode e marca os inimigos", {"radius": 380, "mark": .35}),
    ("epico_3", "lamina_sanguinea", "Lâmina Sanguínea", "epic", "shadow", "reap", "Colheita de Sangue",
     "um corte em círculo que bebe a vida de quem acerta", {"radius": 360, "drain": .25}),
    ("epico_4", "fragmento_do_vortice", "Fragmento do Vórtice", "epic", "lightning", "orbit", "Fragmentos do Vórtice",
     "pedaços do Vórtice orbitam e disparam raios nos inimigos", {"every": 1.2}),
    ("epico_5", "espada_crepusculo", "Ceifadora do Vazio", "epic", "shadow", "whirl", "Redemoinho do Vazio",
     "um redemoinho de sombra que acerta tudo em volta", {"radius": 320, "hits": 5}),
    ("lendario_1", "espada_suspiro_azhur", "Suspiro de Azhûr", "unique", "fire", "firewall", "Suspiro do Colosso",
     "ergue uma muralha de brasa que queima quem atravessa", {"width": 900}),
    ("lendario_2", "espada_aurora", "Aurora Eterna", "legendary", "holy", "holyslam", "Alvorada",
     "um golpe de luz que atordoa os inimigos e cura você", {}),
    ("lendario_3", "asa_da_fenix", "Asa da Fênix", "legendary", "fire", "dash", "Voo da Fênix",
     "voa 9 m envolto em fogo, queimando o caminho", {"distance": 900, "dot": .3, "dot_time": 3}),
    ("lendario_4", "sopro_de_valdora", "Sopro de Valdora", "legendary", "nature", "sweep", "Vendaval de Valdora",
     "o vento do vale varre e arremessa os inimigos para longe", {"push": 1200, "reach": 500}),
    ("lendario_5", "coroa_do_sol", "Coroa do Sol", "legendary", "holy", "shockwave", "Raio de Sol",
     "um raio de sol em linha que atravessa todos os inimigos", {"length": 1200}),
    ("unico_1", "fenda_esmeralda", "Fenda Esmeralda", "unique", "nature", "runeseal", "Fenda Esmeralda",
     "abre uma fenda verde no chão que explode e prende os inimigos", {"radius": 420, "delay": 1.1, "root": 2.5}),
    ("unico_2", "lagrima_do_inverno", "Lágrima do Inverno", "unique", "ice", "whirl", "Nevasca",
     "uma nevasca de cortes em volta que congela aos poucos", {"radius": 380, "hits": 6, "slow": .55, "slow_time": 1.5}),
    ("unico_3", "raiz_da_primeira_arvore", "Raiz da Primeira Árvore", "unique", "nature", "groundslam", "Raízes Ancestrais",
     "as raízes rompem o chão em volta, num raio enorme", {"radius": 600, "delay": .8}),
    ("unico_4", "silencio_branco", "Silêncio Branco", "unique", "holy", "dash", "Corte do Silêncio",
     "some e reaparece 10 m à frente, cortando tudo no caminho", {"distance": 1000}),
    ("unico_5", "sucata_do_andarilho", "Sucata do Andarilho", "unique", "lightning", "overload", "Faísca de Sucata",
     "segura e solta: a lasca de cristal estoura numa explosão de faíscas", {"charge": .45, "radius": 320}),
    ("divino_1", "juramento_celeste", "Juramento Celeste", "divine", "holy", "holyslam", "Juramento",
     "a luz do juramento atordoa os inimigos por mais tempo e cura bem mais", {"stun": 3.2, "heal": .14}),
    ("divino_2", "tridente_do_primeiro_selo", "Tridente do Primeiro Selo", "divine", "water", "runeseal", "Primeiro Selo",
     "o selo das origens: uma runa enorme que explode quase na hora", {"radius": 520, "delay": .9}),
    ("divino_3", "estrela_guia", "Estrela Guia", "divine", "holy", "orbit", "Constelação",
     "estrelas giram em volta e caem nos inimigos sem parar", {"life": 16, "every": 1.0}),
    ("divino_4", "voz_do_santuario", "Voz do Santuário", "divine", "holy", "heal", "Prece do Santuário",
     "a prece do santuário cura você e os aliados em volta", {"heal_reach": 1300}),
    ("divino_5", "genese", "Gênese", "divine", "holy", "shockwave", "Gênese",
     "a luz do começo do mundo: uma onda imensa que atravessa tudo à frente", {"length": 1700, "reach": 600}),
]

ELEMENT_TEXT = {"fire": "Fogo", "water": "Água", "lightning": "Raio", "ice": "Gelo", "nature": "Natureza", "poison": "Veneno",
                "holy": "Luz", "shadow": "Sombra"}


def icon(key, rarity):
    """O ícone: a espada inclinada (a ponta para cima e à direita) num fundo azul-noite com o brilho da raridade."""
    grid = Image.open(os.path.join(SWORDS, key + ".png")).convert("RGBA")
    big = grid.resize((grid.width * 8, grid.height * 8), Image.NEAREST).rotate(-45, resample=Image.NEAREST, expand=True)
    box = big.getbbox()
    big = big.crop(box)
    k = 168 / max(big.size)
    big = big.resize((max(1, int(big.width * k)), max(1, int(big.height * k))), Image.NEAREST)
    S = 192
    c = RARITY_COLOR[rarity]
    bg = Image.new("RGB", (S, S))
    d = ImageDraw.Draw(bg)
    for r in range(S, 0, -2):
        t = r / S
        col = tuple(int((22 + 30 * (1 - t)) * (1 - .35 * (1 - t)) + c[i] * .22 * (1 - t) ** 2) for i in range(3))
        d.ellipse((S / 2 - r * .75, S / 2 - r * .75, S / 2 + r * .75, S / 2 + r * .75), fill=col)
    out = bg.convert("RGBA")
    glow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    alpha = Image.new("L", (S, S), 0)
    alpha.paste(big.split()[3], ((S - big.width) // 2, (S - big.height) // 2))
    halo = alpha.filter(ImageFilter.MaxFilter(9)).filter(ImageFilter.GaussianBlur(6))
    glow.paste(Image.new("RGBA", (S, S), c + (255,)), (0, 0), halo.point(lambda v: v * 3 // 5))
    out.alpha_composite(glow)
    edge = Image.new("RGBA", (S, S), (10, 8, 14, 0))
    edge.putalpha(alpha.filter(ImageFilter.MaxFilter(5)))
    out.alpha_composite(edge)
    out.alpha_composite(big, ((S - big.width) // 2, (S - big.height) // 2))
    ImageDraw.Draw(out).rectangle((1, 1, S - 2, S - 2), outline=tuple(int(v * .7) for v in c) + (255,), width=3)
    return out.convert("RGB")


if __name__ == "__main__":
    os.makedirs(ICONS, exist_ok=True)
    data = {"_doc": "Gerado por Tools/Weapons/BuildSwords.py. O modelo e o poder do botão direito de cada espada.", "swords": {}}
    kinds = set()
    for key, item, name, rarity, element, kind, power, text, tweak in S:
        force, slow = RANK[rarity]
        entry = {"key": key, "model": "W_espada_%s_blocos" % key, "name": name, "rarity": rarity, "element": element,
                 "icon": "weapon_espada_%s" % key, "stats": STATS[rarity], "price": PRICE[rarity],
                 "power": {"name": power, "text": text[0].upper() + text[1:] + "."}}
        if kind:
            mana, cooldown, values, special_icon = BASE[kind]
            values = dict(values, **tweak)
            if "scale" in values:
                values["scale"] = round(values["scale"] * force, 2)
            entry["power"].update({"kind": kind, "mana": mana, "cooldown": round(cooldown * slow, 1), "values": values, "icon": special_icon})
            kinds.add(kind)
        data["swords"][item] = entry
        icon(key, rarity).save(os.path.join(ICONS, "weapon_espada_%s.png" % key))
    assert len(data["swords"]) == 35 and len({e["key"] for e in data["swords"].values()}) == 35
    json.dump(data, open(OUT, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    print("espadas: %d (poderes: %s) -> %s" % (len(data["swords"]), ", ".join(sorted(kinds)), OUT))
