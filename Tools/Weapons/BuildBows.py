"""Os 35 arcos do jogo (07/11, das pranchas do usuário, do Normal ao Divino): nome, raridade, elemento, o modelo de
blocos sem corda (Tools/Blender/MakePixelBows.py; a corda é da Unreal), a flecha própria de cada um e o poder do botão
direito. Igual às espadas (BuildSwords.py): o jogo lê tudo pelo VortexSwords (Data/Rpg/bows.json).

python Tools/Weapons/BuildBows.py   ->   Data/Rpg/bows.json
                                         Data/UI/SkillIcons/Novos/weapon_arco_<chave>.png   (o ícone na mochila)
Depois: python Tools/BuildArsenal.py (os itens) e o Blender (MakePixelBows.py: os arcos e as flechas).

O usuário pediu: o mesmo estilo do arco de blocos do jogo, a flecha de cada arco combinando com ele e um poder para
cada um (pelo nome e pela raridade). Os poderes: a chuva de flechas do arco, a rajada da besta, o selo de runa da
pistola, a sobrecarga da varinha, a muralha de fogo, a órbita, a cura e a bola de fogo, mais três novos só de arco
(VortexWeaponSpecials.cpp): o leque de flechas (volley), a flecha que atravessa tudo (pierce) e a que explode (explosive).
O Arco de Bétula (o inicial) fica com a chuva de flechas. Os arcos do Godot que já existiam ganham o modelo que combina
(o id do item fica: os saves continuam valendo).
"""
import importlib.util
import json
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BOWS = os.path.join(ROOT, "Data", "VortexWeapons", "Bows")
ICONS = os.path.join(ROOT, "Data", "UI", "SkillIcons", "Novos")
OUT = os.path.join(ROOT, "Data", "Rpg", "bows.json")

spec = importlib.util.spec_from_file_location("build_swords", os.path.join(os.path.dirname(os.path.abspath(__file__)), "BuildSwords.py"))
swords = importlib.util.module_from_spec(spec)
spec.loader.exec_module(swords)
RANK, PRICE = swords.RANK, swords.PRICE
# o arco vale pela destreza e pela velocidade de soltar (como os do Godot)
STATS = {"common": {"attack": 1}, "uncommon": {"attack": 2, "crit": 3, "speed": 4}, "rare": {"attack": 3, "crit": 6, "speed": 6},
         "epic": {"attack": 5, "crit": 9, "speed": 8}, "legendary": {"attack": 8, "crit": 13, "speed": 10},
         "unique": {"attack": 10, "crit": 12, "speed": 10}, "divine": {"attack": 13, "crit": 15, "speed": 12}}

# tipo -> (mana, recarga, valores, ícone). Os números das outras armas (VortexWeaponKit.cpp); os três novos são do arco.
BASE = {
    "arrowrain": (0, 16, {"reach": 2500, "radius": 400, "life": 1.8, "arrows": 20, "scale": .2}, "special_arco_de_betula"),
    "burst3": (0, 9, {"reach": 4000, "count": 3, "time": .45, "scale": 1.8}, "special_besta"),
    "volley": (0, 9, {"reach": 2600, "count": 5, "spread": 40, "scale": 1.3}, "special_chakrams"),
    "pierce": (0, 10, {"reach": 4000, "size": 2.0, "scale": 3.5}, "special_lanca_do_patrulheiro"),
    "explosive": (10, 11, {"reach": 3000, "radius": 320, "scale": 3.0}, "special_catalisador"),
    "runeseal": (15, 12, {"delay": 1.5, "radius": 300, "scale": 4.5, "mark": .25, "mark_life": 5, "reach": 2600}, "special_pistola_runica"),
    "overload": (12, 6, {"charge": .6, "scale": 4.0}, "special_varinha"),
    "orbit": (20, 25, {"life": 12, "every": 1.5, "scale": .5, "undead": .5, "reach": 1800}, "special_orbe"),
    "firewall": (25, 18, {"tornado_life": 2.5, "tornado_radius": 250, "tornado_cooldown": 3, "reach": 2200, "width": 800, "life": 6, "scale": .6},
                 "special_livro_da_aurora"),
    "fireball": (15, 10, {"reach": 2600}, "special_cajado_de_ilsa"),
    "heal": (20, 16, {"heal_reach": 900}, "special_cajado_de_ilsa"),
}

# chave (o modelo), item (o id no jogo), nome, raridade, elemento, poder, nome do poder, o que faz, ajustes
B = [
    ("comum_1", "arco_curto", "Arco Curto", "common", "", "burst3", "Três Seguidas",
     "solta três flechas rápidas, uma atrás da outra", {}),
    ("comum_2", "arco_do_batedor", "Arco do Batedor", "common", "", "volley", "Leque do Batedor",
     "abre um leque de três flechas", {"count": 3, "spread": 24}),
    ("comum_3", "arco_de_galho_seco", "Arco de Galho Seco", "common", "", "pierce", "Flecha Seca",
     "uma flecha forte que atravessa os inimigos em linha", {"size": 1.6}),
    ("comum_4", "arco_de_betula", "Arco de Bétula", "common", "", None, "Chuva de Flechas",
     "uma chuva de flechas cai onde você mira", {}),
    ("comum_5", "arco_remendado", "Arco Remendado", "common", "", "arrowrain", "Chuvisco de Flechas",
     "uma chuva de flechas pequena onde você mira", {"arrows": 12, "radius": 320}),
    ("incomum_1", "arco_do_salgueiro", "Arco do Salgueiro", "uncommon", "nature", "volley", "Folhas ao Vento",
     "um leque de quatro flechas de folha", {"count": 4, "spread": 30}),
    ("incomum_2", "arco_de_espinheiro", "Arco de Espinheiro", "uncommon", "poison", "burst3", "Espinhos em Fila",
     "três flechas de espinho envenenadas, uma atrás da outra", {}),
    ("incomum_3", "arco_de_cobre_rubro", "Arco de Cobre Rubro", "uncommon", "fire", "explosive", "Flecha de Brasa",
     "uma flecha que explode em fogo onde acerta", {"radius": 260}),
    ("incomum_4", "arco_da_correnteza", "Arco da Correnteza", "uncommon", "water", "pierce", "Flecha da Correnteza",
     "atravessa os inimigos em linha, como a água", {}),
    ("incomum_5", "arco_do_cacador", "Arco do Caçador", "uncommon", "", "arrowrain", "Chuva do Caçador",
     "uma chuva de flechas onde você mira", {"radius": 380}),
    ("raro_1", "arco_da_nevoa", "Arco da Névoa Azul", "rare", "water", "runeseal", "Selo da Névoa",
     "marca o chão com uma runa de água que explode logo depois", {}),
    ("raro_2", "arco_da_lua_prateada", "Arco da Lua Prateada", "rare", "holy", "volley", "Crescente",
     "um leque de cinco flechas de luz em meia-lua", {"count": 5, "spread": 50}),
    ("raro_3", "arco_do_relampago", "Arco do Relâmpago", "rare", "lightning", "overload", "Flecha-Trovão",
     "segura e solta: um raio carregado que explode no alvo", {"radius": 260}),
    ("raro_4", "arco_da_geada", "Arco da Geada", "rare", "ice", "pierce", "Lança de Gelo",
     "uma flecha de gelo que atravessa todos em linha", {"size": 2.2}),
    ("raro_5", "arco_de_jade", "Arco de Jade", "rare", "nature", "burst3", "Três Jades",
     "três flechas de jade seguidas", {}),
    ("epico_1", "arco_ametista", "Arco Ametista", "epic", "shadow", "orbit", "Estilhaços Flutuantes",
     "cristais de ametista giram em volta e atiram nos inimigos", {}),
    ("epico_2", "arco_noite_violeta", "Arco da Noite Violeta", "epic", "shadow", "runeseal", "Runa da Noite",
     "uma runa de sombra que explode e marca os inimigos", {"radius": 340}),
    ("epico_3", "arco_lua_sangrenta", "Sussurro da Lua Sangrenta", "unique", "shadow", "explosive", "Estrela Sangrenta",
     "uma flecha que explode em estrelas de sangue", {"radius": 380}),
    ("epico_4", "arco_fragmentado", "Arco Fragmentado", "epic", "lightning", "burst3", "Fragmentos",
     "cinco disparos rápidos de fragmentos do Vórtice", {"count": 5, "time": .6}),
    ("epico_5", "arco_do_eclipse", "Arco do Eclipse", "epic", "shadow", "pierce", "Flecha do Eclipse",
     "uma flecha escura enorme que atravessa tudo", {"size": 3.0}),
    ("lendario_1", "arco_de_magma", "Arco de Magma Rúnico", "legendary", "fire", "firewall", "Muralha de Magma",
     "ergue uma muralha de magma que queima quem atravessa", {}),
    ("lendario_2", "arco_grifo_dourado", "Arco do Grifo Dourado", "legendary", "", "volley", "Asas do Grifo",
     "um leque de sete flechas douradas", {"count": 7, "spread": 60}),
    ("lendario_3", "arco_da_fenix", "Arco da Fênix", "legendary", "fire", "fireball", "Sopro da Fênix",
     "lança uma bola de fogo que explode em 3 m", {}),
    ("lendario_4", "arco_do_sol_nascente", "Arco do Sol Nascente", "legendary", "holy", "explosive", "Explosão Solar",
     "uma flecha de sol que explode em luz", {"radius": 420}),
    ("lendario_5", "arco_do_bosque_ancestral", "Arco do Bosque Ancestral", "legendary", "nature", "arrowrain", "Chuva de Galhos",
     "uma chuva de flechas enorme onde você mira", {"arrows": 30, "radius": 520}),
    ("unico_1", "arco_de_bracos_gemeos", "Arco de Braços Gêmeos", "unique", "lightning", "burst3", "Disparo Gêmeo",
     "seis flechas em pares, bem rápidas", {"count": 6, "time": .6}),
    ("unico_2", "arco_do_espelho", "Arco do Espelho", "unique", "shadow", "volley", "Reflexo",
     "um leque largo de seis flechas, para os dois lados da mira", {"count": 6, "spread": 70}),
    ("unico_3", "arco_da_raiz_primeva", "Arco da Raiz Primeva", "unique", "nature", "runeseal", "Raiz que Explode",
     "uma raiz rompe o chão onde você mira e explode", {"radius": 420}),
    ("unico_4", "arco_sem_sombra", "Arco Sem Sombra", "unique", "holy", "pierce", "Flecha Sem Sombra",
     "uma flecha de luz que atravessa tudo e vai muito longe", {"reach": 6000}),
    ("unico_5", "arco_do_viajante_perdido", "Arco do Viajante Perdido", "unique", "ice", "heal", "Fôlego do Viajante",
     "cura você e os aliados em volta", {}),
    ("divino_1", "asas_da_aurora", "Asas da Aurora", "divine", "holy", "volley", "Aurora",
     "nove flechas de luz em leque", {"count": 9, "spread": 70}),
    ("divino_2", "arco_da_arvore_do_mundo", "Arco da Árvore do Mundo", "divine", "nature", "arrowrain", "Chuva da Árvore do Mundo",
     "uma chuva de flechas imensa onde você mira", {"arrows": 40, "radius": 600}),
    ("divino_3", "arco_das_estrelas", "Arco das Estrelas", "divine", "holy", "orbit", "Constelação Viva",
     "estrelas giram em volta e atiram nos inimigos sem parar", {"life": 16, "every": 1.0}),
    ("divino_4", "arco_do_primeiro_rio", "Arco do Primeiro Rio", "divine", "water", "pierce", "Rio Primordial",
     "uma flecha de água imensa que atravessa tudo à frente", {"size": 3.2}),
    ("divino_5", "arco_do_vendaval_eterno", "Arco do Vendaval Eterno", "divine", "nature", "explosive", "Vendaval Eterno",
     "uma flecha que estoura num vendaval enorme", {"radius": 560}),
]


if __name__ == "__main__":
    os.makedirs(ICONS, exist_ok=True)
    data = {"_doc": "Gerado por Tools/Weapons/BuildBows.py. O modelo, a flecha e o poder do botão direito de cada arco.", "bows": {}}
    kinds = set()
    for key, item, name, rarity, element, kind, power, text, tweak in B:
        force, slow = RANK[rarity]
        entry = {"key": key, "weapon": "arco_de_betula", "model": "W_arco_%s_blocos" % key, "arrow": "W_flecha_%s_blocos" % key,
                 "name": name, "rarity": rarity, "element": element, "icon": "weapon_arco_%s" % key, "stats": STATS[rarity],
                 "price": PRICE[rarity], "anim": "self", "power": {"name": power, "text": text[0].upper() + text[1:] + "."}}
        if kind:
            mana, cooldown, values, special_icon = BASE[kind]
            values = dict(values, **tweak)
            if "scale" in values:
                values["scale"] = round(values["scale"] * force, 2)
            entry["power"].update({"kind": kind, "mana": mana, "cooldown": round(cooldown * slow, 1), "values": values, "icon": special_icon})
            kinds.add(kind)
        data["bows"][item] = entry
        # o mesmo ícone das espadas (inclinado 45°, o fundo com o brilho da raridade), da grade do arco
        swords.SWORDS = BOWS
        swords.icon(key, rarity).save(os.path.join(ICONS, "weapon_arco_%s.png" % key))
    assert len(data["bows"]) == 35 and len({e["key"] for e in data["bows"].values()}) == 35
    json.dump(data, open(OUT, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    print("arcos: %d (poderes: %s) -> %s" % (len(data["bows"]), ", ".join(sorted(kinds)), OUT))
