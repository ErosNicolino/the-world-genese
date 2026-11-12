"""O efeito do poder de cada espada e de cada arco (07/11, o usuário: "criar um efeito para cada do zero, não
reutilizar"), montado pelo AVortexPowerFx (Source/Vortex/Private/Combat/VortexPowerFx.cpp) com peças feitas em código:
o corte no ar (slash), o anel de blocos (ring), o círculo de runas no chão (glyph), os pilares (pillar), os espinhos
que saem do chão (spikes), os estilhaços (shards), os orbes (orbs), a chuva (rain), a espiral (spiral), o feixe
(beam), as penas (feathers), o raio em zigue-zague (bolt) e o rastro na lâmina (trail). Nenhum efeito pronto do
projeto (Niagara) entra.

Cada arma tem a sua composição: o tipo do poder dá a base (o golpe no chão racha o chão em anel; a chuva de flechas
cai no ponto da mira), o nome dá o tema (a fênix solta penas de fogo; o gelo, cristais; a raiz, espinhos de madeira;
o trovão, raios; a estrela, orbes...), as cores saem do elemento e do próprio desenho da arma, e a raridade dá o
tamanho e a quantidade. Os números são sorteados pela chave da arma (sempre os mesmos para ela).

python Tools/Weapons/BuildPowerFx.py   ->   Data/Rpg/power_fx.json
"""
import json
import os
import random
import zlib

import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "Data", "Rpg", "power_fx.json")
RANKS = ["common", "uncommon", "rare", "epic", "legendary", "unique", "divine"]
ELEMENT = {"fire": [(1, .45, .1), (1, .8, .25), (.9, .15, .05)], "ice": [(.55, .85, 1), (.9, .98, 1), (.3, .6, 1)],
           "lightning": [(.45, .7, 1), (.9, .95, 1), (.6, .35, 1)], "nature": [(.35, .95, .3), (.8, 1, .4), (.1, .6, .2)],
           "poison": [(.5, 1, .2), (.75, .3, 1), (.2, .7, .1)], "water": [(.2, .7, 1), (.6, .95, 1), (.05, .4, .8)],
           "holy": [(1, .85, .4), (1, 1, .8), (1, .65, .2)], "shadow": [(.55, .25, 1), (.9, .5, 1), (.25, .05, .5)]}
THEMES = [  # palavras do nome / da chave -> peças a mais (o tema)
    (("fenix", "asa", "grifo"), [("feathers", {})]),
    (("gel", "gelo", "geada", "inverno", "neve", "safira", "cristal", "ametista"), [("shards", {"style": 1})]),
    (("raiz", "arvore", "bosque", "salgueiro", "espinheiro", "folha", "verde", "jade", "esmeralda", "galho"), [("spikes", {"style": 1})]),
    (("estrela", "constela", "sol", "aurora", "lua", "celeste", "juramento", "santuario"), [("orbs", {"style": 1})]),
    (("trovao", "relampago", "fragmento", "vortice", "bracos", "sucata"), [("bolt", {})]),
    (("sangu", "rubro", "sangrenta", "presa", "ferrao"), [("shards", {"style": 2})]),
    (("mare", "correnteza", "rio", "tridente", "nevoa"), [("ring", {"style": 1})]),
    (("magma", "brasa", "cobre", "cinzas", "azhur"), [("pillar", {"style": 1})]),
    (("eclipse", "noite", "vazio", "sombra", "crepusculo", "escritura"), [("glyph", {"style": 1}), ("orbs", {"style": 2})]),
    (("espelho", "silencio", "sem_sombra", "branco"), [("shards", {"style": 3})]),
    (("vendaval", "vento", "sopro", "valdora"), [("spiral", {})]),
    (("monolito", "marco", "guardiao", "lenhador"), [("spikes", {"style": 2})]),
    (("genese",), [("ring", {"style": 2}), ("pillar", {"style": 2}), ("orbs", {"style": 1})]),
]
BASE = {  # o tipo do poder -> as peças da forma
    "dash": [("trail", {}), ("shards", {"at": "front"}), ("ring", {"small": 1})],
    "flurry": [("slash", {"count": 5, "small": 1}), ("trail", {})],
    "sweep": [("slash", {"sweep": 220}), ("ring", {})],
    "groundslam": [("ring", {}), ("spikes", {"at": "ring"}), ("shards", {})],
    "reap": [("slash", {"sweep": 360, "low": 1}), ("glyph", {})],
    "shockwave": [("spikes", {"at": "line"}), ("ring", {"small": 1}), ("pillar", {"at": "end"})],
    "whirl": [("slash", {"sweep": 360, "count": 3}), ("spiral", {})],
    "runeseal": [("glyph", {"at": "aim"}), ("pillar", {"at": "aim"}), ("shards", {"at": "aim"})],
    "overload": [("orbs", {"converge": 1}), ("beam", {})],
    "orbit": [("orbs", {"follow": 1}), ("spiral", {})],
    "firewall": [("pillar", {"at": "wall"}), ("rain", {"up": 1})],
    "holyslam": [("pillar", {}), ("ring", {}), ("rain", {})],
    "heal": [("spiral", {}), ("glyph", {}), ("feathers", {"soft": 1})],
    "volley": [("beam", {"fan": 1}), ("shards", {"small": 1})],
    "pierce": [("beam", {"long": 1}), ("ring", {"small": 1, "at": "front"})],
    "explosive": [("shards", {"at": "aim"}), ("ring", {"at": "aim"})],
    "arrowrain": [("rain", {"at": "aim"}), ("glyph", {"at": "aim"})],
    "burst3": [("beam", {"count": 3}), ("trail", {})],
    "fireball": [("orbs", {"converge": 1}), ("shards", {"at": "aim"})],
}


# 07/11 (o usuário: "bem detalhado e bem único, uma por uma"): as receitas feitas à mão, arma por arma, no lugar da
# montada pelo sorteio. Os tempos casam com a animação dela (Tools/Blender/MakePowerAnims.py, HAND) e com o poder
# (os "delay" do swords.json / bows.json)
HAND = {
    # a onda que corre na linha da mira: varre (o corte rente ao chão), o respingo da largada, a onda de blocos de água
    # (funda, clara e a espuma da crista que enrola), os respingos da crista e a espuma no rastro (na própria onda)
    "lamina_da_mare_baixa": {"colors": [[.04, .2, .42], [.12, .45, .75], [.85, .95, 1.0]], "parts": [
        {"type": "trail", "color": 1, "delay": 0.0, "life": .55, "size": 1.0},
        {"type": "slash", "color": 1, "delay": .28, "life": .3, "sweep": 170, "low": 1, "count": 1, "tilt": 6, "radius": 260, "size": 1},
        {"type": "sprites", "tex": "agua", "delay": .45, "life": .6, "count": 9, "radius": 90, "size": 2.4, "rise": 140, "gravity": 450,
         "at": "front", "spread": .1},
        {"type": "wave", "color": 0, "delay": .5, "life": 1.4, "length": 1400, "width": 340, "speed": 1500, "height": 175, "lit": 1},
    ]},
    # a espada cravada: o chão racha em linhas, as pedras sobem em anel, o anel de luz, a coluna dourada onde ela entrou,
    # a poeira que abre e as faíscas
    "marco_do_guardiao": {"colors": [[.32, .27, .22], [1.0, .78, .32], [.55, .5, .44], [.2, .17, .14]], "parts": [
        {"type": "trail", "color": 1, "delay": .15, "life": .5, "size": 1.1},
        {"type": "cracks", "color": 3, "lit": 1, "delay": .62, "life": 1.5, "count": 9, "radius": 340, "size": 1.5, "seed": 11, "front": 95},
        {"type": "spikes", "color": 2, "lit": 1, "delay": .64, "life": 1.15, "count": 10, "radius": 230, "size": 1.3, "style": 2, "at": "ring", "seed": 5, "front": 95},
        {"type": "ring", "color": 1, "delay": .62, "life": .5, "count": 14, "radius": 320, "size": 1.0, "front": 95},
        {"type": "pillar", "color": 1, "delay": .6, "life": .9, "size": .55, "count": 1, "front": 95},
        {"type": "sprites", "tex": "poeira", "delay": .62, "life": .95, "count": 12, "radius": 170, "size": 1.8, "out": 170, "rise": 40,
         "at": "ring", "spread": .08, "front": 95},
        {"type": "sprites", "tex": "faisca", "delay": .62, "life": .55, "count": 5, "radius": 30, "size": 1.0, "rise": 120, "spread": .05, "front": 95},
        {"type": "shards", "color": 2, "lit": 1, "delay": .62, "life": 1.0, "count": 12, "size": .9, "seed": 21, "front": 95},
    ]},
}


def weapon_colors(path):
    """As duas cores mais marcantes do desenho da arma (a mais comum e a mais saturada)."""
    if not os.path.exists(path):
        return [(.8, .8, .9), (1, 1, 1)]
    a = np.asarray(Image.open(path).convert("RGBA")).astype(int)
    px = a[a[..., 3] > 0][:, :3]
    q = (px // 32) * 32 + 16
    keys, counts = np.unique(q, axis=0, return_counts=True)
    order = np.argsort(-counts)
    sat = lambda c: int(max(c)) - int(min(c))
    common = keys[order[0]]
    vivid = max((keys[i] for i in order[:10]), key=sat)
    return [tuple(round(float(x) / 255, 3) for x in common), tuple(round(float(x) / 255, 3) for x in vivid)]


def compose(kind, item, name, key, rarity, element, grid):
    rnd = random.Random(zlib.crc32(("fx_" + key + item).encode()))
    rank = RANKS.index(rarity)
    scale = 1 + .12 * rank
    base = ELEMENT.get(element)
    wc = weapon_colors(grid)
    colors = [list(c) for c in (base if base else [wc[1], wc[0], (1, 1, 1)])]
    if base:
        colors.append(list(wc[1]))   # a cor da arma entra como a quarta
    parts = [(t, dict(o)) for t, o in BASE.get(kind, [("ring", {})])]
    text = (item + " " + name).lower().replace("ç", "c").replace("ã", "a").replace("é", "e").replace("ê", "e").replace("ú", "u").replace("ó", "o").replace("â", "a").replace("í", "i")
    for words, extra in THEMES:
        if any(w in text for w in words):
            parts += [(t, dict(o)) for t, o in extra]
    if rank >= 4 and rnd.random() < .7:
        parts.append((rnd.choice(["ring", "rain", "spiral", "glyph", "shards"]), {"style": 3}))
    out = []
    for i, (t, o) in enumerate(parts):
        p = {"type": t, "color": rnd.randrange(len(colors)) if i else 0, "delay": round(rnd.uniform(0, .15) + .08 * i, 3),
             "life": round(rnd.uniform(.7, 1.3) * scale ** .5, 3), "size": round(rnd.uniform(.8, 1.25) * scale, 3),
             "radius": round(rnd.uniform(160, 320) * scale * (.5 if o.get("small") else 1), 1),
             "count": int(rnd.randint(8, 18) * (1 + .15 * rank) * (.6 if o.get("small") else 1)),
             "spin": round(rnd.uniform(-1, 1) * 360, 1), "tilt": round(rnd.uniform(-35, 35), 1), "seed": rnd.randrange(1 << 20)}
        p.update({k: v for k, v in o.items()})
        if t == "slash":
            p["sweep"] = o.get("sweep", rnd.choice((120, 160, 200)))
            p["count"] = o.get("count", rnd.randint(1, 2 + rank // 3))
        if t == "beam":
            p["count"] = o.get("count", 5 if o.get("fan") else 1)
        out.append(p)
    return {"colors": colors, "parts": out}


LIFE = {"slash": .35, "ring": .55, "glyph": 1.2, "pillar": .9, "spikes": 1.0, "shards": 1.0, "orbs": 1.0, "rain": 1.4, "spiral": 1.0,
        "beam": .5, "feathers": 1.4, "bolt": .4, "trail": .6, "sprites": .7, "wave": 1.4, "cracks": 1.4, "streak": .6, "vanish": .3}
SHORT = {"f": "front", "r": "radius", "n": "count", "sz": "size", "l": "life", "c": "color"}


def class_recipes():
    """As linhas do class_fx_hand.py -> receitas do AVortexPowerFx (as cores do elemento, os tempos a partir do golpe)."""
    import sys
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from class_fx_hand import FX as CFX, PALETTE
    skills = {s["id"]: s for v in json.load(open(os.path.join(ROOT, "Data", "Rpg", "rpg.json"), encoding="utf-8"))["classSkills"]["classes"].values() for s in v}
    hits_path = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "classes.json")
    hits = {v["item"]: v["hit"] for v in json.load(open(hits_path, encoding="utf-8")).values()} if os.path.exists(hits_path) else {}
    missing = [k for k in skills if k not in CFX]
    assert not missing, "sem efeito: %s" % missing
    out = {}
    for sid, line in CFX.items():
        sk = skills[sid]
        env = {"R": float(sk.get("radius") or 3) * 100, "D": float(sk.get("duration") or 2), "G": abs(float(sk.get("range") or 8)) * 100,
               "L": float(sk.get("delay") or 0)}
        hit = float(hits.get(sid, .3))
        parts = []
        for i, chunk in enumerate(x.strip() for x in line.split(";") if x.strip()):
            name, _, args = chunk.partition(":")
            part = {"type": "sprites", "tex": name[2:]} if name.startswith("s.") else {"type": name}
            rel, absolute = 0.0, None
            for a in (x.strip() for x in args.split(",") if x.strip()):
                if a == "@z":
                    if sk.get("range"): part["at_aim"] = 1
                    continue
                if "=" not in a:
                    part[a] = 1
                    continue
                k, v = a.split("=", 1)
                if k == "at":
                    part["at_" + v] = 1
                    continue
                val = float(eval(v, {"__builtins__": {}}, env))
                if k == "d": rel = val
                elif k == "t": absolute = val
                else: part[SHORT.get(k, k)] = round(val, 3)
            part["delay"] = round(max(0.0, absolute if absolute is not None else hit + rel), 3)
            part.setdefault("life", LIFE.get(part["type"], .8))
            part.setdefault("seed", zlib.crc32(("%s%d" % (sid, i)).encode()) % 100000)
            parts.append(part)
        out["classe_" + sid] = {"colors": PALETTE.get(sk.get("element"), PALETTE["arcane"]), "parts": parts, "hit": hit}
    return out


if __name__ == "__main__":
    data = {"_doc": "Gerado por Tools/Weapons/BuildPowerFx.py: o efeito do poder de cada arma (AVortexPowerFx).", "fx": {}}
    # 07/11: as outras 66, uma por uma (Tools/Weapons/power_fx_hand.py, pela chave "espada_<chave>" / "arco_<chave>")
    import sys
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from power_fx_hand import FX
    made = set()
    for table, folder, kindname in (("swords", "Swords", "espada"), ("bows", "Bows", "arco")):
        entries = json.load(open(os.path.join(ROOT, "Data", "Rpg", table + ".json"), encoding="utf-8"))[table]
        for item, w in entries.items():
            kind = w["power"].get("kind")
            if not kind:
                continue
            key = "%s_%s" % (kindname, w["key"])
            grid = os.path.join(ROOT, "Data", "VortexWeapons", folder, w["key"] + ".png")
            data["fx"][item] = HAND.get(item) or FX.get(key) or compose(kind, item, w["name"], w["key"], w["rarity"], w["element"], grid)
            made.add(key)
            if key + "_carga" in FX:
                data["fx"][item + "_carga"] = FX[key + "_carga"]
                made.add(key + "_carga")
    # 07/11: os 264 poderes de classe (Tools/Weapons/class_fx_hand.py), "classe_<id>", com o instante do golpe da animação
    data["fx"].update(class_recipes())
    lost = set(FX) - made
    assert not lost, "receitas sem arma: %s" % sorted(lost)
    sigs = {json.dumps(v["parts"], sort_keys=True) for v in data["fx"].values()}
    assert len(sigs) == len(data["fx"]), "dois efeitos iguais"
    json.dump(data, open(OUT, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    kinds = {}
    for v in data["fx"].values():
        for p in v["parts"]:
            kinds[p["type"]] = kinds.get(p["type"], 0) + 1
    print("efeitos: %d armas, peças: %s" % (len(data["fx"]), kinds))
