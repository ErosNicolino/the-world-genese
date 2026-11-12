"""A vila maior (03/11): Data/GodotTown/town_extra.json, que o AVortexTown soma à planta do Godot (town.json).

python Tools/BuildTownExpansion.py

O que muda (tudo no espaço do Godot: x, y para cima, z; metros; a frente das peças no -Z, giro em graus como no Godot):
- a muralha sai de x -40..40, z -24..56 para x -62..62, z -34..80 (o platô plano em volta, sem os lagos);
  os portões norte e sul vão para a muralha nova e entram os do leste e do oeste;
- bairros novos: dos artesãos (oeste), o quartel com o campo de treino e o estábulo (leste), o mercado e a capela
  (norte), e fora da muralha, ao sul, o moinho e as hortas;
- ruas de pedra ligando os portões, postes de luz, bancos, jardineiras, árvores e mastros com bandeiras;
- barris de água e de vinho e caixotes que quebram (AVortexBreakable), as pás do moinho que giram, a luz das janelas
  à noite e a fumaça da chaminé do quartel.
As construções novas: Tools/Blender/BuildTownBuildings.py (as caixas delas em town_models_extra.json).
"""
import json
import math
import os
import random

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOWN = os.path.join(ROOT, "Data", "GodotTown")
X0, Z0, X1, Z1 = -62.0, -34.0, 62.0, 80.0
CELL = 0.75
rng = random.Random(311)

pieces, trees, spinners, cloth, smoke, glows, breakables = [], [], [], [], [], [], []


def piece(model, x, z, yaw=0.0, label=""):
    pieces.append({"model": model, "label": label, "x": round(x, 3), "z": round(z, 3), "yaw": yaw})


def run(a, b, gaps=()):
    """Centros de muros de 8 m de a até b, sem cobrir os vãos (gaps: (início, fim))."""
    out = []
    spans = [(a, b)]
    for g0, g1 in gaps:
        nxt = []
        for s0, s1 in spans:
            if g1 <= s0 or g0 >= s1:
                nxt.append((s0, s1))
                continue
            if g0 > s0:
                nxt.append((s0, g0))
            if g1 < s1:
                nxt.append((g1, s1))
        spans = nxt
    for s0, s1 in spans:
        n = max(1, math.ceil((s1 - s0) / 8.0))
        step = (s1 - s0) / n
        out.extend(s0 + step * (i + .5) for i in range(n))
    return out


# ---- muralha, torres e portões -----------------------------------------------------------------
GATE_W, GATE_E = 13.0, 10.0   # z das ruas que entram pelo oeste e pelo leste (vãos entre as casas antigas)
for x in run(X0, X1, [(-8, 8)]):
    piece("town_wall", x, Z0, 0)
    piece("town_wall", x, Z1, 180)
for z in run(Z0, Z1, [(GATE_W - 8, GATE_W + 8)]):
    piece("town_wall", X0, z, 90)
for z in run(Z0, Z1, [(GATE_E - 8, GATE_E + 8)]):
    piece("town_wall", X1, z, -90)
for x, z, yaw in ((X0, Z0, 0), (X1, Z0, 0), (X0, Z1, 180), (X1, Z1, 180), (-31, Z0, 0), (31, Z0, 0), (-31, Z1, 180), (31, Z1, 180),
                  (X0, -12, 90), (X0, 46, 90), (X1, -12, -90), (X1, 46, -90)):
    piece("town_tower", x, z, yaw)
piece("town_gatehouse", 0, Z0, 0, "CIDADE DO SANTUÁRIO")
piece("town_gatehouse", 0, Z1, 180, "PORTÃO NORTE")
piece("town_gatehouse", X0, GATE_W, 90, "PORTÃO OESTE")
piece("town_gatehouse", X1, GATE_E, -90, "PORTÃO LESTE")
# bandeiras dos dois lados de cada portão (por dentro)
for x, z, yaw, color in ((-10, Z0 + 3, 0, "red"), (10, Z0 + 3, 0, "red"), (-10, Z1 - 3, 180, "blue"), (10, Z1 - 3, 180, "blue"),
                         (X0 + 3, GATE_W - 10, 90, "green"), (X0 + 3, GATE_W + 10, 90, "green"),
                         (X1 - 3, GATE_E - 10, -90, "blue"), (X1 - 3, GATE_E + 10, -90, "blue")):
    piece("town_banner_pole", x, z, yaw)
    cloth.append({"model": f"town_banner_{color}", "x": x, "z": z, "yaw": yaw, "y": 6.0, "dx": .12})

# ---- oeste: rua dos artesãos (as casas olham para o leste, para a rua x = -46) ----------------------
for z, model, label in ((-24, "town_shop", "CARPINTARIA"), (-12, "town_house_d", ""), (0, "town_shop", "CURTUME"),
                        (27, "town_house_a", ""), (38, "town_shop", "ALQUIMIA DE VERA"), (50, "town_cottage", ""),
                        (61, "town_house_b", ""), (72, "town_cottage", "")):
    piece(model, -53, z, -90, label)
# quintais atrás das casas antigas do oeste: hortas, feno e árvores
for z in (-14, 2, 30, 44):
    piece("town_crops", -38.5, z, 90)
for z in (-22, 22, 56):
    piece("town_hay", -38, z, rng.uniform(0, 180))

# ---- leste: quartel, campo de treino e estábulo (olham para o oeste, para a rua x = 46) ---------------
piece("town_barracks", 53, -19, 90, "QUARTEL DA GUARDA")
for x, z in ((45.5, -6), (48.5, -6), (45.5, -1), (48.5, -1), (51.5, -3.5)):
    piece("town_dummy", x, z, -90 + rng.uniform(-10, 10))
piece("vil_fence", 43.5, -8, 0)
piece("vil_fence", 43.5, 1.5, 0)
piece("town_stable", 53, 27, 90, "ESTÁBULO")
for x, z in ((46.5, 33), (47.5, 35)):
    piece("town_hay", x, z, rng.uniform(0, 180))
for z, model in ((42, "town_house_c"), (54, "town_cottage"), (66, "town_house_a"), (75, "town_cottage")):
    piece(model, 53, z, 90)
for x, z, yaw, color in ((44, -24, -90, "blue"), (44, -14, -90, "blue")):
    piece("town_banner_pole", x, z, yaw)
    cloth.append({"model": f"town_banner_{color}", "x": x, "z": z, "yaw": yaw, "y": 6.0, "dx": .12})
smoke.append([53 + 1.5, 8.6, -19 - 4.0])   # a chaminé do quartel

# ---- norte: o mercado e a capela ---------------------------------------------------------------------
piece("town_chapel", -27, 69, 0, "CAPELA DA AURORA")
for x, z, model, yaw in ((-10, 64, "vil_stall_a", 10), (-4, 70, "vil_stall_b", -8), (4, 64, "vil_stall_a", -12),
                         (10, 70, "vil_stall_b", 6), (16, 64, "vil_stall_a", 0), (-16, 70, "vil_stall_b", 15)):
    piece(model, x, z, yaw)
piece("vil_well", 0, 74, 0)
piece("town_shop", 27, 70, 0, "MERCEARIA")
piece("town_house_b", 37, 71, 0)
piece("town_house_c", -40, 71, 0)
for x, z in ((-14, 61), (14, 61), (-14, 76), (14, 76)):
    piece("town_banner_pole", x, z, 0)
    cloth.append({"model": "town_banner_red", "x": x, "z": z, "yaw": 0, "y": 6.0, "dx": .12})

# ---- fora, ao sul: o moinho e as hortas ----------------------------------------------------------------
piece("town_mill", 30, -47, 0, "MOINHO")
spinners.append({"model": "town_mill_blades", "x": 30, "z": -47, "yaw": 0, "local": [0, 7.5, -3.25], "speed": 28})
for x, z in ((16, -42), (16, -48), (16, -54), (44, -42), (44, -48)):
    piece("town_crops", x, z, 0)
for i, x in enumerate(range(10, 52, 2)):
    if 24 <= x <= 36:
        continue
    piece("vil_fence", x, -38.5, 0)

# ---- ruas de pedra -----------------------------------------------------------------------------------
streets = [
    (-62, GATE_W - 1.5, -34, GATE_W + 1.5),   # oeste: do portão até as casas antigas
    (36, GATE_E - 1.5, 62, GATE_E + 1.5),     # leste
    (-1.5, 56, 1.5, 80),                       # norte: do portão antigo ao novo
    (-1.5, -34, 1.5, -24),                     # sul
    (-47.5, -30, -44.5, 77),                   # rua dos artesãos
    (44.5, -30, 47.5, 77),                     # rua do quartel
    (-45, 58.5, 45, 61.5),                     # rua do mercado
    (-45, -30.5, 45, -27.5),                   # rua do sul
    (-20, 62, 20, 76),                         # a praça do mercado
]
# dentro da vila antiga, só os dois pedaços que ligam as ruas novas do oeste e do leste às ruas dela
CONNECT = [(-40, GATE_W - 1.5, -23, GATE_W + 1.5), (24, GATE_E - 1.5, 40, GATE_E + 1.5)]


def footprint_blocks():
    """Os retângulos (x, z, meia largura, meia profundidade, giro) das construções, para as pedras não entrarem nelas."""
    models = json.load(open(os.path.join(TOWN, "town.json"), encoding="utf-8"))["models"]
    extra = json.load(open(os.path.join(TOWN, "town_models_extra.json"), encoding="utf-8"))
    models.update(extra)
    rects = []
    town = json.load(open(os.path.join(TOWN, "town.json"), encoding="utf-8"))
    kept = [p for p in town["pieces"] if p["model"] not in ("town_wall", "town_tower", "town_gatehouse")]
    for p in kept + pieces:
        foot = models.get(p["model"], {}).get("footing") or []
        if len(foot) != 4 or p["model"] in ("vil_lamp", "vil_fence", "town_banner_pole"):
            continue
        rects.append((p["x"], p["z"], foot, math.radians(p["yaw"])))
    return rects


blocks = footprint_blocks()


def blocked(x, z):
    for px, pz, f, yaw in blocks:
        dx, dz = x - px, z - pz
        # para o espaço da peça (o giro do Godot em volta de y)
        lx = dx * math.cos(yaw) - dz * math.sin(yaw)
        lz = dx * math.sin(yaw) + dz * math.cos(yaw)
        if f[0] - .3 <= lx <= f[2] + .3 and f[1] - .3 <= lz <= f[3] + .3:
            return True
    return False


old_cells = {(math.floor(c[0] / CELL), math.floor(c[1] / CELL)) for c in json.load(open(os.path.join(TOWN, "town.json"), encoding="utf-8"))["cobbles"]}
cells = set()
for n, (x0, z0, x1, z1) in enumerate(streets + CONNECT):
    for i in range(math.floor(x0 / CELL), math.ceil(x1 / CELL)):
        for j in range(math.floor(z0 / CELL), math.ceil(z1 / CELL)):
            x, z = (i + .5) * CELL, (j + .5) * CELL
            if n < len(streets) and -40 < x < 40 and -24 < z < 56:
                continue   # dentro da vila antiga as ruas são as dela
            if (i, j) not in old_cells and not blocked(x, z):
                cells.add((i, j))
cobbles = [[round((i + .5) * CELL, 3), round((j + .5) * CELL, 3), round(rng.random(), 2), 1 if rng.random() < .07 else 0] for i, j in sorted(cells)]

# ---- postes, bancos, jardineiras e árvores ---------------------------------------------------------------
for z in range(-26, 78, 12):
    piece("vil_lamp", -48.5, z, -90)
    piece("vil_lamp", 48.5, z + 6, 90)
for x in range(-40, 44, 12):
    piece("vil_lamp", x, 57.8, 0)
    piece("vil_lamp", x + 6, -26.5, 180)
for x, z, yaw in ((-8, 57.5, 0), (8, 57.5, 0), (-43.5, 30, 90), (43.5, 20, -90), (-24, 62, 180)):
    piece("vil_bench", x, z, yaw)
for x, z in ((-43.5, -20), (-43.5, 40), (43.5, 40), (43.5, 60), (-22, 77), (22, 77)):
    piece("vil_planter", x, z, 0)
for x, z, kind in ((-41, -28, "tree_oak"), (-41, 8, "tree_apple"), (-41, 52, "tree_cherry"), (-41, 66, "tree_birch"),
                   (41, 48, "tree_oak"), (41, 30, "tree_apple"), (-56, 77, "tree_birch"), (56, 77, "tree_birch"),
                   (-20, 78, "tree_cherry"), (20, 78, "tree_cherry"), (-34, 62, "tree_autumn"), (34, 62, "tree_autumn"),
                   (6, -40, "tree_oak"), (54, -40, "tree_apple")):
    trees.append({"kind": kind, "x": x, "z": z, "scale": round(rng.uniform(.8, 1.0), 2)})

# ---- barris (água e vinho) e caixotes que quebram -----------------------------------------------------
for x, z, kind in ((-7.5, 66, "wine"), (-7, 67.2, "water"), (6.5, 66.5, "wine"), (12.5, 67, "crate"), (13.3, 67.6, "crate"),
                   (-18.5, 66, "water"), (24, 67.5, "wine"), (24.9, 66.9, "wine"), (29.5, 66.6, "crate"),
                   (47, -12, "water"), (47.9, -12.6, "water"), (49.5, 22, "water"), (48.6, 23, "crate"), (49.6, 31, "crate"),
                   (-45.5, -20.5, "crate"), (-46.2, -21.4, "crate"), (-45.4, 2.8, "water"), (-45.6, 36, "wine"),
                   (-46.3, 36.9, "wine"), (2.8, -30.5, "water"), (-2.8, -30.2, "crate"), (33.5, -42, "water"), (26.4, -42.3, "crate"),
                   (-2.5, 77, "water"), (3, 77.4, "crate"), (-58, 16.5, "crate"), (58, 6.5, "water")):
    breakables.append({"kind": kind, "x": x, "z": z, "yaw": rng.uniform(0, 360)})

# ---- a luz das janelas à noite: na frente das casas (a 2,4 m do chão) -------------------------------------
HOUSES = {"town_house_a": 3.4, "town_house_b": 3.4, "town_house_c": 3.4, "town_house_d": 3.4, "town_cottage": 2.3, "town_shop": 2.9,
          "town_inn": 3.4, "town_barracks": 3.3, "town_chapel": 6.9}
old = json.load(open(os.path.join(TOWN, "town.json"), encoding="utf-8"))["pieces"]
for p in old + pieces:
    depth = HOUSES.get(p["model"])
    if not depth:
        continue
    yaw = math.radians(p["yaw"])
    # a frente (-Z do Godot) girada pelo giro da peça
    fx, fz = -math.sin(yaw), -math.cos(yaw)
    glows.append([round(p["x"] + fx * (depth + .6), 2), 2.4, round(p["z"] + fz * (depth + .6), 2)])

extra_models = json.load(open(os.path.join(TOWN, "town_models_extra.json"), encoding="utf-8"))
out = {
    "_doc": __doc__.strip().splitlines()[0] + " Gerado por Tools/BuildTownExpansion.py; não editar à mão.",
    "bounds": [X0, Z0, X1, Z1],
    "remove": ["town_wall", "town_tower", "town_gatehouse"],
    "models": extra_models,
    "pieces": pieces, "trees": trees, "cobbles": cobbles,
    "spinners": spinners, "cloth": cloth, "smoke": smoke, "glows": glows, "breakables": breakables,
}
json.dump(out, open(os.path.join(TOWN, "town_extra.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print("VILA_MAIOR", len(pieces), "peças,", len(cobbles), "pedras,", len(trees), "árvores,", len(breakables), "barris/caixotes,",
      len(glows), "janelas,", len(cloth), "bandeiras")
