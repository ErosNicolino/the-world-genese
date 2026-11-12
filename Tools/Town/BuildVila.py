"""A vila nova (05/11): a planta inteira, refeita do zero pelas referências de `modelando ainda/Futuras modelagens/Cidade`.

python Tools/Town/BuildVila.py            -> Data/GodotTown/vila.json e Docs/Images/vila/planta_vila.png

O AVortexTown lê vila.json no lugar de town.json + town_extra.json (o mesmo formato: models, pieces, trees, cobbles,
spinners, cloth, smoke, glows, breakables, bounds) e o FVortexTerrainData aplaina o chão dentro do "flatten".
Coordenadas do Godot em metros (Unreal: X = x, Y = z). Cada peça: a frente (a porta) no -z do modelo; yaw em graus.

- Muralha de 8 lados (cerca de 160 x 160 m), torres nos cantos e no meio dos lados, portões no norte (a entrada, com
  o nome da cidade), no sul e no leste (a estrada do Estádio).
- Ruas em grade: a avenida norte-sul e a leste-oeste (8 m), as ruas (6 m) e o anel por dentro da muralha. Tudo em
  placas de paralelepípedo de 3 x 3 m (vila_rua_*: as pedras com alturas diferentes), as praças em placas de laje.
- Praça central (fonte), praça do mercado (barracas), praça do poço, campo de treino de terra.
- Quarteirões com casas variadas (vila_casa_1..8, viradas para a rua) e os prédios de cada ofício, com as placas das
  lojas de antes (Forja de Bram, Ervas de Ilsa, Mapas de Nicolly, Taverna do Javali Dourado...).
- Fora do portão norte: o moinho e as hortas.
Os modelos são feitos por Tools/Blender/BuildVilaKit.py; as caixas de colisão deles vêm de Data/GodotTown/vila_models.json
(enquanto não existe, cada modelo usa uma caixa do tamanho do alicerce).
"""
import json
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import terrain_peek  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "Data", "GodotTown", "vila.json")
KIT = os.path.join(ROOT, "Data", "GodotTown", "vila_models.json")
PREVIEW = os.path.join(ROOT, "Docs", "Images", "vila", "planta_vila.png")
rng = random.Random(1105)

# ---- o tamanho -----------------------------------------------------------------------------------------
CX, CZ = 18.0, 21.0          # o centro (a fonte); múltiplos de 3: as ruas caem certinho na grade das placas
HALF = 71.0                  # do centro à muralha (11/11: 62 -> 71, o usuário: "muita casa colada, pode aumentar a vila")
CUT = 23.0                   # o corte dos cantos (a muralha de 8 lados)
RING = HALF - 5.0            # o anel de rua por dentro da muralha (6 m, até 2 m da muralha)
STREETS = [-27.0, 0.0, 27.0]  # as ruas (relativas ao centro); 0 = as avenidas. Poucas ruas: quarteirões grandes, cheios
AVENUE, STREET = 6.0, 6.0
TILE = 3.0

# ---- os modelos: alicerce (largura x fundo, m) e o que são -----------------------------------------------
# (as casas de 1 a 8 e os prédios; "enter" = dá para entrar)
MODELS = {
    # 11/11: as casas do kit da Quaternius (Tools/Blender/BuildCasasQuaternius.py), todas de entrar: a planta + 60 cm
    "vila_casa_1": (8.6, 6.6, "casa térrea de pedra"),
    "vila_casa_2": (8.6, 6.6, "sobrado: pedra embaixo, enxaimel em cima"),
    "vila_casa_3": (6.6, 6.6, "chalé de reboco, a empena para a rua"),
    "vila_casa_4": (10.6, 6.6, "sobrado comprido de reboco"),
    "vila_casa_5": (6.6, 8.6, "casa estreita e funda, a empena para a rua"),
    "vila_casa_6": (10.6, 8.6, "casa grande de dois andares"),
    "vila_casa_7": (8.6, 8.6, "casa térrea grande de pedra"),
    "vila_casa_8": (8.6, 6.6, "casa com frente de loja e toldo"),
    "vila_prefeitura": (17, 12, "prefeitura com torre do sino"),
    "vila_taverna": (16, 12, "taverna (dá para entrar)"),
    "vila_capela": (10, 18, "capela de pedra"),
    "vila_forja": (12, 10, "forja aberta"),
    "vila_carpintaria": (12, 9, "carpintaria com toras"),
    "vila_quartel": (16, 11, "quartel da guarda"),
    "vila_estabulo": (16, 9, "estábulo"),
    "vila_loja": (8.6, 8.6, "loja com placa (padaria, alfaiate, armaria, mapas, ervas)"),
    "vila_moinho": (8, 8, "moinho de pedra"),
    "vila_horta": (12, 7, "canteiro da horta"),
    "vila_muralha": (6, 2.4, "trecho de muralha com ameias"),
    "vila_torre": (7, 7, "torre da muralha"),
    "vila_portao": (16, 5, "portão com duas torres"),
}
TILES = ["vila_rua_a", "vila_rua_b", "vila_rua_c", "vila_rua_d"]      # paralelepípedo
SLABS = ["vila_laje_a", "vila_laje_b"]                                # as praças
DIRT = ["vila_terra_a", "vila_terra_b"]                               # o campo de treino e as estradas de fora
BORDERS = ["vila_borda_a", "vila_borda_b", "vila_borda_c"]           # 07/11: a transição da pedra para a grama
for t in TILES + SLABS + DIRT + BORDERS:
    MODELS[t] = (TILE, TILE, "chão")
MODELS["vila_rua_p"] = (1.2, 1.5, "chão")      # 11/11: o caminho de pedra da porta de cada casa até a rua
MODELS["vila_moinho_pas"] = (0, 0, "as pás do moinho")
# 06/11: os enfeites do kit (no lugar dos do Godot)
for name, size in (("vila_banco", (2, .6)), ("vila_floreira", (1.5, 1)), ("vila_poste", (.5, .5)), ("vila_barraca_a", (3.4, 2.6)),
                   ("vila_barraca_b", (3.4, 2.6)), ("vila_caixotes", (2.2, 1.2)), ("vila_poco", (3, 2.6)), ("vila_quadro", (2.7, .9)),
                   ("vila_cerca", (5.5, .2)), ("vila_carroca", (4.8, 2.2))):
    MODELS[name] = (size[0], size[1], "enfeite")

pieces, trees, cobbles, glows, smoke, breakables, spinners, cloth = [], [], [], [], [], [], [], []
lots = []          # (x0, z0, x1, z1) ocupados por construções (para não sobrepor)


def yaw_facing(fx, fz):
    """O giro (graus) que vira a frente do modelo (o -z) para a direção (fx, fz)."""
    return -math.degrees(math.atan2(fx, -fz))


def yaw_along(dx, dz):
    """O giro que deita o x do modelo na direção (dx, dz)."""
    return -math.degrees(math.atan2(dz, dx))


def add(model, x, z, yaw=0.0, label=""):
    pieces.append({"model": model, "x": round(x, 3), "z": round(z, 3), "yaw": round(yaw, 2), "label": label})


# ---- o contorno (8 lados) -------------------------------------------------------------------------------
def octagon(half, cut):
    h, c = half, cut
    pts = [(-h + c, -h), (h - c, -h), (h, -h + c), (h, h - c), (h - c, h), (-h + c, h), (-h, h - c), (-h, -h + c)]
    return [(CX + x, CZ + z) for x, z in pts]


def inside(poly, x, z):
    n, ok = len(poly), False
    for i in range(n):
        x1, z1 = poly[i]
        x2, z2 = poly[(i + 1) % n]
        if (z1 > z) != (z2 > z) and x < (x2 - x1) * (z - z1) / (z2 - z1) + x1:
            ok = not ok
    return ok


WALL = octagon(HALF, CUT)
INNER = octagon(HALF - 3.0, CUT * .93)          # por dentro da muralha (onde cabe coisa)

# ---- muralha, torres e portões ---------------------------------------------------------------------------
GATES = {(0, -1): "CIDADE DO SANTUÁRIO", (0, 1): "PORTÃO SUL", (1, 0): "PORTÃO DO ESTÁDIO"}
for i in range(8):
    (x1, z1), (x2, z2) = WALL[i], WALL[(i + 1) % 8]
    dx, dz = x2 - x1, z2 - z1
    length = math.hypot(dx, dz)
    ux, uz = dx / length, dz / length
    # a normal para fora (o polígono gira no sentido horário no mapa: x para a direita, z para baixo)
    nx, nz = uz, -ux
    if (CX - (x1 + x2) / 2) * nx + (CZ - (z1 + z2) / 2) * nz > 0:
        nx, nz = -nx, -nz
    mid = ((x1 + x2) / 2, (z1 + z2) / 2)
    side = (round(nx), round(nz)) if abs(nx) > .9 or abs(nz) > .9 else None
    gate = side in GATES
    # 06/11 (o usuário: "a muralha não se conecta nas bordas, tem um vão que dá para passar"): os trechos de 6 m
    # cobrem tudo entre as torres dos cantos (3,5 m de cada ponta) e as torres do portão (8 m do meio), encostando
    # um pouco por dentro delas; o último trecho de cada pedaço sobrepõe o vizinho em vez de deixar vão
    spans = [(3.2, length / 2 - 7.6), (length / 2 + 7.6, length - 3.2)] if gate else [(3.2, length - 3.2)]
    for a, b in spans:
        count = max(1, int(math.ceil((b - a) / 6.0)))
        for n in range(count):
            t = a + 3.0 + (n * (b - a - 6.0) / (count - 1) if count > 1 else (b - a - 6.0) / 2)
            add("vila_muralha", x1 + ux * t, z1 + uz * t, yaw_along(ux, uz))
    add("vila_torre", x1, z1, 0)
    if gate:
        add("vila_portao", mid[0], mid[1], yaw_facing(nx, nz), GATES[side])
    elif side is not None:
        add("vila_torre", mid[0], mid[1], 0)

# ---- as ruas (placas de 3 x 3) ---------------------------------------------------------------------------
covered = {}       # (i, k) da grade de 3 m -> tipo


def fill_rect(x0, z0, x1, z1, kind):
    for i in range(int(math.floor(x0 / TILE)), int(math.ceil(x1 / TILE))):
        for k in range(int(math.floor(z0 / TILE)), int(math.ceil(z1 / TILE))):
            cx, cz = (i + .5) * TILE, (k + .5) * TILE
            if x0 < cx < x1 and z0 < cz < z1 and inside(INNER, cx, cz):
                covered.setdefault((i, k), kind)


def free(x0, z0, x1, z1):
    return all(not (x0 < b[2] and x1 > b[0] and z0 < b[3] and z1 > b[1]) for b in lots)


PLAZA = (CX - 12, CZ - 9, CX + 12, CZ + 9)
MARKET = (CX - 54, CZ - 9, CX - 33, CZ + 9)
WELL = (CX + 40, CZ - 53, CX + 52, CZ - 41)
TRAIN = (CX - 54, CZ - 53, CX - 40, CZ - 41)
for r, slab in ((PLAZA, "laje"), (MARKET, "laje"), (WELL, "laje"), (TRAIN, "terra")):
    fill_rect(*r, slab)
for s in STREETS:
    w = AVENUE if s == 0 else STREET
    fill_rect(CX + s - w / 2, CZ - RING, CX + s + w / 2, CZ + RING, "rua")
    fill_rect(CX - RING, CZ + s - w / 2, CX + RING, CZ + s + w / 2, "rua")
# o anel por dentro da muralha (os lados retos; nos cantos cortados, placas giradas ao longo da diagonal)
for x0, z0, x1, z1 in ((CX - RING - 3, CZ - RING - 3, CX + RING + 3, CZ - RING + 3), (CX - RING - 3, CZ + RING - 3, CX + RING + 3, CZ + RING + 3),
                       (CX - RING - 3, CZ - RING - 3, CX - RING + 3, CZ + RING + 3), (CX + RING - 3, CZ - RING - 3, CX + RING + 3, CZ + RING + 3)):
    fill_rect(x0, z0, x1, z1, "rua")
for i, (gx, gz) in enumerate([(0, -1), (0, 1), (1, 0)]):
    # da avenida até fora do portão
    if gx == 0:
        fill_rect(CX - AVENUE / 2, CZ + gz * RING - 6, CX + AVENUE / 2, CZ + gz * RING + 6, "rua")
    else:
        fill_rect(CX + RING - 6, CZ - AVENUE / 2, CX + HALF + 1, CZ + AVENUE / 2, "rua")
diag_tiles = []    # 11/11: vazio (as placas giradas do anel se sobrepunham às da grade; agora a quina é em escadinha)
for corner in range(4):
    sx, sz = (-1, -1, 1, 1)[corner], (-1, 1, -1, 1)[corner]
    # a diagonal do anel: do fim do lado reto de z ao fim do lado reto de x
    a = (CX + sx * (RING - CUT * .9), CZ + sz * RING)
    b = (CX + sx * RING, CZ + sz * (RING - CUT * .9))
    # 11/11 (o usuário: "nas quinas dentro da muralha o chão não faz curva, só buga"): as placas da grade (sem giro) cujo
    # centro fica a até 3 m da diagonal, em escadinha; o meio-fio acompanha os degraus
    L = math.hypot(b[0] - a[0], b[1] - a[1])
    ux, uz = (b[0] - a[0]) / L, (b[1] - a[1]) / L
    for i in range(int(math.floor(min(a[0], b[0]) / TILE)) - 2, int(math.ceil(max(a[0], b[0]) / TILE)) + 2):
        for k in range(int(math.floor(min(a[1], b[1]) / TILE)) - 2, int(math.ceil(max(a[1], b[1]) / TILE)) + 2):
            cx, cz = (i + .5) * TILE, (k + .5) * TILE
            t = max(0.0, min(L, (cx - a[0]) * ux + (cz - a[1]) * uz))
            if math.hypot(cx - (a[0] + ux * t), cz - (a[1] + uz * t)) <= 3.0 and inside(INNER, cx, cz):
                covered.setdefault((i, k), "rua")

def street_hi(c, e):
    """Onde terminam as placas da rua e (relativa ao centro c) do lado de cima (a borda da placa)."""
    w = 6.0 if abs(e) == RING else AVENUE if e == 0 else STREET
    return math.ceil((c + e + w / 2) / TILE - 1e-6) * TILE


def street_lo(c, e):
    w = 6.0 if abs(e) == RING else AVENUE if e == 0 else STREET
    return math.floor((c + e - w / 2) / TILE + 1e-6) * TILE


def on_tiles(x0, z0, x1, z1):
    """A caixa pisa em alguma placa de chão (as da grade ou as do anel em diagonal)?"""
    for i in range(int(math.floor(x0 / TILE)), int(math.ceil(x1 / TILE))):
        for k in range(int(math.floor(z0 / TILE)), int(math.ceil(z1 / TILE))):
            if (i, k) in covered and x0 < (i + 1) * TILE - .05 and x1 > i * TILE + .05 and z0 < (k + 1) * TILE - .05 and z1 > k * TILE + .05:
                return True
    return any(x0 - 2.1 < x < x1 + 2.1 and z0 - 2.1 < z < z1 + 2.1 for x, z, _ in diag_tiles)


# ---- os prédios de cada ofício ---------------------------------------------------------------------------
placed = {}        # placa -> (modelo, x, z, frente): os NPCs vão para os prédios deles


GAP = 2.5        # 11/11: o mínimo entre duas construções (antes 1 m: "muita casa colada")
SETBACK = 1.5    # o recuo da casa em relação à rua
BELT = 6.0       # 11/11 (o usuário: "casas muito perto da muralha"): faixa verde, só com árvores, entre a rua da muralha e as casas


def place(model, x, z, face, label=""):
    w, d, _ = MODELS[model]
    fx, fz = face
    rot = abs(fx) > .5          # de lado: largura e fundo trocam no mapa
    hw, hd = (d / 2, w / 2) if rot else (w / 2, d / 2)
    lots.append((x - hw - GAP / 2, z - hd - GAP / 2, x + hw + GAP / 2, z + hd + GAP / 2))
    add(model, x, z, yaw_facing(fx, fz), label)
    if label:
        placed[label] = (model, x, z, face)
    # a luz das janelas à noite, na frente
    glows.append([round(x + fx * (d / 2 + .6), 2), 2.4, round(z + fz * (d / 2 + .6), 2)])


# (x, z relativos ao centro, frente para onde, placa)
SPECIAL = [
    # 07/11: cada prédio no canto do quarteirão (encostado nas ruas): o resto do quarteirão cabe casas
    ("vila_prefeitura", 13.5, -14.9, (0, 1), "PREFEITURA"),
    ("vila_taverna", -13.5, -14.9, (0, 1), "TAVERNA DO JAVALI DOURADO"),
    ("vila_quartel", -38.3, -18.3, (0, -1), "QUARTEL DA GUARDA"),
    ("vila_carpintaria", -36.3, 19.2, (0, 1), "CARPINTARIA"),
    ("vila_forja", -36.3, 35.3, (0, -1), "FORJA DE BRAM"),
    ("vila_capela", -8.3, 39.3, (0, -1), "CAPELA"),
    ("vila_estabulo", 34.8, -11.3, (-1, 0), "ESTÁBULO"),
    ("vila_loja", 34.6, 8, (-1, 0), "PADARIA"),
    ("vila_loja", -7.6, 13.6, (0, -1), "ERVAS DE ILSA"),
    ("vila_loja", 7.6, 13.6, (0, -1), "MAPAS DE NICOLLY"),
    ("vila_loja", 7.6, 34.6, (0, -1), "ALFAIATE"),
    ("vila_loja", 34.6, 34.6, (-1, 0), "ARMARIA"),
]
for model, x, z, face, label in SPECIAL:
    place(model, CX + x, CZ + z, face, label)
# 07/11: a forja nova tem a chaminé no modelo (chimneys)
# smoke.append([CX - 33.3 - 3.8, 7.5, CZ + 32.3 - .3, 130, 950, .5, 14])      # a chaminé da forja

# ---- as casas: nos quarteirões, viradas para a rua mais perto ---------------------------------------------
HOUSES = ["vila_casa_%d" % n for n in range(1, 9)]
edges = [-RING] + STREETS + [RING]
for a in range(len(edges) - 1):
    for b in range(len(edges) - 1):
        # 06/11 (o usuário: casas "em cima do caminho"): a borda do quarteirão é a borda das placas de 3 m da rua
        x0, x1 = street_hi(CX, edges[a]) + .3, street_lo(CX, edges[a + 1]) - .3
        z0, z1 = street_hi(CZ, edges[b]) + .3, street_lo(CZ, edges[b + 1]) - .3
        if x1 - x0 < 6 or z1 - z0 < 6:
            continue
        yard = (x0, z0, x1, z1)       # o quarteirão inteiro: as árvores também nascem na faixa verde
        x0, x1 = x0 + BELT * (a == 0), x1 - BELT * (a == len(edges) - 2)
        z0, z1 = z0 + BELT * (b == 0), z1 - BELT * (b == len(edges) - 2)
        # fileiras de casas (11/11: afastadas e recuadas da rua): primeiro as das bordas de cima e de baixo (de costas uma para a outra, como na
        # referência), depois as dos lados no que sobrar; sem entrar nas praças nem nos prédios
        for side in ((0, -1), (0, 1), (-1, 0), (1, 0)):
            fx, fz = side
            along = (x0, x1) if fz else (z0, z1)
            t = along[0]
            while t < along[1] - 5.5:
                # 06/11: tenta todos os modelos (embaralhados) antes de andar 75 cm: o quarteirão fica cheio
                done = False
                for model in rng.sample(HOUSES, len(HOUSES)):
                    w, d, _ = MODELS[model]
                    if t + w > along[1] + .01:
                        continue
                    c = t + w / 2
                    if fz:
                        x, z = c, (z0 + SETBACK + d / 2) if fz < 0 else (z1 - SETBACK - d / 2)
                        box = (x - w / 2, z - d / 2, x + w / 2, z + d / 2)
                    else:
                        x, z = (x0 + SETBACK + d / 2) if fx < 0 else (x1 - SETBACK - d / 2), c
                        box = (x - d / 2, z - w / 2, x + d / 2, z + w / 2)
                    blocked = any(box[0] < r[2] and box[2] > r[0] and box[1] < r[3] and box[3] > r[1] for r in (PLAZA, MARKET, WELL, TRAIN))
                    if (not blocked and free(*box) and not on_tiles(*box) and all(inside(INNER, px, pz) for px in (box[0], box[2]) for pz in (box[1], box[3]))):
                        place(model, x, z, (fx, fz))
                        t += w + rng.choice((2.5, 3.0, 3.5))
                        done = True
                        break
                if not done:
                    t += .75
        # o miolo do quarteirão: quintais com árvores, canteiros e um poço aqui e ali
        for n in range(80):
            tx, tz = rng.uniform(yard[0] + 2, yard[2] - 2), rng.uniform(yard[1] + 2, yard[3] - 2)
            if free(tx - 2.2, tz - 2.2, tx + 2.2, tz + 2.2) and not on_tiles(tx - 2, tz - 2, tx + 2, tz + 2) and inside(INNER, tx, tz)                     and all(math.hypot(tx - o["x"], tz - o["z"]) > 6.5 for o in trees):
                trees.append({"kind": rng.choice(("tree_oak", "tree_cherry", "tree_birch")), "x": round(tx, 2), "z": round(tz, 2), "scale": round(rng.uniform(.75, 1.05), 2)})
                lots.append((tx - 1.5, tz - 1.5, tx + 1.5, tz + 1.5))
        for n in range(12):
            tx, tz = rng.uniform(x0 + 1.5, x1 - 1.5), rng.uniform(z0 + 1.5, z1 - 1.5)
            if free(tx - 1.3, tz - 1.3, tx + 1.3, tz + 1.3) and not on_tiles(tx - 1.3, tz - 1.3, tx + 1.3, tz + 1.3) and inside(INNER, tx, tz):
                add("vila_floreira", tx, tz, rng.choice((0, 90)))
                lots.append((tx - 1.2, tz - 1.2, tx + 1.2, tz + 1.2))

# ---- 11/11 (o usuário: "bichos na cidade"): onde mora cada bicho (AVortexTownLife; modelos do Cube World Kit) ------
# galinhas e gatos nos quintais (lugar livre, fora das ruas), cachorros nas ruas, ovelhas e porcos num cercado perto do
# estábulo. [tipo, x, z, raio em que anda (m)]
animals = []
life_rng = random.Random(2511)


def yard_spot(half, tries=400, far_from=(), gap=14.0):
    for _ in range(tries):
        x, z = life_rng.uniform(CX - RING + 6, CX + RING - 6), life_rng.uniform(CZ - RING + 6, CZ + RING - 6)
        if (inside(INNER, x - half, z - half) and inside(INNER, x + half, z + half) and free(x - half, z - half, x + half, z + half)
                and not on_tiles(x - half, z - half, x + half, z + half) and all(math.hypot(x - a, z - b) > gap for a, b in far_from)):
            return x, z
    return None


used = []
yard_rects = []   # os terreiros das galinhas (terra batida, sem grama: a grama alta escondia as galinhas)
for n in range(5):
    at = yard_spot(3.0, tries=3000, far_from=used, gap=12.0)
    if at:
        used.append(at)
        for dx in (-1.5, 1.5):
            for dz in (-1.5, 1.5):
                add(rng.choice(DIRT), at[0] + dx, at[1] + dz, rng.choice((0, 90, 180, 270)))
        yard_rects.append((at[0] - 3, at[1] - 3, at[0] + 3, at[1] + 3))
        lots.append((at[0] - 3.1, at[1] - 3.1, at[0] + 3.1, at[1] + 3.1))
        for k in range(4):
            animals.append(["Chick" if k == 3 else "Chicken", round(at[0] + life_rng.uniform(-1, 1), 2), round(at[1] + life_rng.uniform(-1, 1), 2), 3.5])
for n in range(4):
    at = yard_spot(1.2, far_from=used, gap=10.0)
    if at:
        used.append(at)
        animals.append(["Cat", round(at[0], 2), round(at[1], 2), 4.0])
streets = [((i + .5) * TILE, (k + .5) * TILE) for (i, k), kind in covered.items() if kind == "rua"]
for n in range(3):
    x, z = streets[life_rng.randrange(len(streets))]
    animals.append(["Dog", round(x, 2), round(z, 2), 12.0])
# o cercado: 5,6 x 5,6 m (4 trechos de cerca), no lugar livre mais perto do estábulo
stable = placed.get("ESTÁBULO")
pen = None
if stable:
    best = None
    for _ in range(3000):
        x, z = life_rng.uniform(CX - RING + 8, CX + RING - 8), life_rng.uniform(CZ - RING + 8, CZ + RING - 8)
        # só as construções contam (os lotes maiores que 3,2 m); árvores e floreiras no caminho saem
        big = [b for b in lots if b[2] - b[0] > 3.2 or b[3] - b[1] > 3.2]
        if (all(not (x - 3.6 < b[2] and x + 3.6 > b[0] and z - 3.6 < b[3] and z + 3.6 > b[1]) for b in big)
                and not on_tiles(x - 3.4, z - 3.4, x + 3.4, z + 3.4) and inside(INNER, x - 3.6, z - 3.6) and inside(INNER, x + 3.6, z + 3.6)):
            dist = math.hypot(x - stable[1], z - stable[2])
            if best is None or dist < best[0]:
                best = (dist, x, z)
    if best:
        _, x, z = best
        pen = (x, z)
        trees[:] = [t for t in trees if not (abs(t["x"] - x) < 4.6 and abs(t["z"] - z) < 4.6)]
        pieces[:] = [q for q in pieces if not (q["model"] == "vila_floreira" and abs(q["x"] - x) < 4.2 and abs(q["z"] - z) < 4.2)]
        for dx, dz, yaw in ((0, -2.8, 0), (0, 2.8, 0), (-2.8, 0, 90), (2.8, 0, 90)):
            add("vila_cerca", x + dx, z + dz, yaw)
        lots.append((x - 3.2, z - 3.2, x + 3.2, z + 3.2))
        for k in range(5):
            animals.append(["Sheep" if k < 3 else "Pig", round(x + life_rng.uniform(-1.5, 1.5), 2), round(z + life_rng.uniform(-1.5, 1.5), 2), 1.9])
print("VILA bichos: %d (%s), cercado %s" % (len(animals), ", ".join("%s %d" % (k, sum(a[0] == k for a in animals)) for k in ("Chicken", "Chick", "Cat", "Dog", "Sheep", "Pig")),
      "a %.0f m do estábulo" % best[0] if stable and best else "não coube"))

# ---- as placas de chão (06/11: depois dos prédios; a que fica embaixo de um prédio sai) ---------------------
def under_building(x0, z0, x1, z1):
    m = GAP / 2 + .4          # os lotes têm GAP / 2 de folga em volta: só a placa de baixo da construção sai
    return any(x0 < b[2] - m and x1 > b[0] + m and z0 < b[3] - m and z1 > b[1] + m for b in lots)


tiles_out = []
for (i, k), kind in sorted(covered.items()):
    x, z = (i + .5) * TILE, (k + .5) * TILE
    if under_building(x - 1.5, z - 1.5, x + 1.5, z + 1.5):
        continue
    pool = TILES if kind == "rua" else SLABS if kind == "laje" else DIRT
    # paralelepípedo e laje só giram 0 ou 180 (as fileiras continuam de uma placa para a outra); a terra à vontade
    # 07/11: paralelepípedo e laje sem giro (as pedras da beira emendam com as da placa vizinha)
    add(rng.choice(pool), x, z, 0 if kind != "terra" else rng.choice((0, 90, 180, 270)))
    tiles_out.append((x, z, 0.0))
for x, z, yaw in diag_tiles:
    if (int(math.floor(x / TILE)), int(math.floor(z / TILE))) not in covered and not under_building(x - 1.5, z - 1.5, x + 1.5, z + 1.5):
        add(rng.choice(TILES), x, z, yaw)
        tiles_out.append((x, z, yaw))
# 07/11 (o usuário: "pedra e grama direto fica esquisito"): a faixa de transição na beira das placas de pedra que dão
# para a grama (não na beira das construções, da muralha nem de outra placa)
borders = 0
border_rects = []   # 11/11 (o usuário: "grama atravessando"): o meio-fio também entra na máscara da grama
for (i, k), kind in sorted(covered.items()):
    if kind == "terra":
        continue
    x, z = (i + .5) * TILE, (k + .5) * TILE
    if under_building(x - 1.5, z - 1.5, x + 1.5, z + 1.5):
        continue
    for fx, fz in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        if (i + fx, k + fz) in covered:
            continue
        ex, ez = x + fx * 1.5, z + fz * 1.5
        sx0, sx1 = sorted((ex, ex + fx * 1.1)) if fx else (x - 1.5, x + 1.5)
        sz0, sz1 = sorted((ez, ez + fz * 1.1)) if fz else (z - 1.5, z + 1.5)
        if not inside(INNER, ex + fx * 1.2, ez + fz * 1.2) or not free(sx0, sz0, sx1, sz1) or on_tiles(sx0 + .1, sz0 + .1, sx1 - .1, sz1 - .1):
            continue
        add(rng.choice(BORDERS), ex, ez, yaw_facing(fx, fz))
        border_rects.append((sx0, sz0, sx1, sz1))
        borders += 1
print("VILA %d faixas de transição pedra/grama" % borders)
# a grama não nasce no chão de pedra (células de 75 cm, só a máscara: 2 números)
mask = set()
for x, z, yaw in tiles_out:
    a = math.radians(-yaw)
    for u in (-1.125, -.375, .375, 1.125):
        for v in (-1.125, -.375, .375, 1.125):
            px, pz = x + u * math.cos(a) - v * math.sin(a), z + u * math.sin(a) + v * math.cos(a)
            mask.add((math.floor(px / .75) * .75 + .375, math.floor(pz / .75) * .75 + .375))


def mask_rect(x0, z0, x1, z1):
    for i in range(int(math.floor(x0 / .75)), int(math.ceil(x1 / .75))):
        for k in range(int(math.floor(z0 / .75)), int(math.ceil(z1 / .75))):
            mask.add((i * .75 + .375, k * .75 + .375))


for r in border_rects + yard_rects:
    mask_rect(*r)
cobbles = [[round(x, 3), round(z, 3)] for x, z in sorted(mask)]

# ---- enfeites --------------------------------------------------------------------------------------------
add("vil_fountain", CX, CZ)
add("vila_quadro", CX + 9, CZ + 8.5, yaw_facing(0, -1), "MISSÕES")
for dx, dz in ((-10, -7), (10, -7), (-10, 7), (10, 7)):
    add("vila_banco", CX + dx, CZ + dz, yaw_facing(-math.copysign(1, dx), 0))
    add("vila_floreira", CX + dx * 1.35, CZ + dz * 1.4, 0)
add("vila_poco", WELL[0] + 10.5, WELL[1] + 10.5)
for dx, dz in ((-6, 0), (6, 0)):
    add("vila_banco", WELL[0] + 10.5 + dx, WELL[1] + 10.5 + dz, yaw_facing(-math.copysign(1, dx), 0))
# o mercado: barracas em fileiras e a carroça do Ducci
mx0, mz0 = MARKET[0] + 3, MARKET[1] + 3
for r in range(2):
    for c in range(3):
        add(rng.choice(("vila_barraca_a", "vila_barraca_b")), mx0 + 2.5 + c * 6.5, mz0 + 2 + r * 8, yaw_facing(0, -1 if r % 2 == 0 else 1))
add("vila_carroca", MARKET[2] - 3, MARKET[3] - 3, yaw_facing(1, 0), "BAZAR DO DUCCI")
for n in range(6):
    add("vila_caixotes", MARKET[0] + rng.uniform(1, 15), MARKET[1] + rng.choice((.8, 17.2)), rng.choice((0, 90)))
# o campo de treino: bonecos e cerca
for n in range(3):
    add("vila_cerca", TRAIN[0] + 2 + n * 5.5, TRAIN[1] + .8, 0)
# lampiões nas avenidas
for t in range(-60, 61, 12):
    for (x, z, f) in ((CX + 2.6, CZ + t, (-1, 0)), (CX + t, CZ + 2.6, (0, -1)), (CX - 2.6, CZ + t + 6, (1, 0)), (CX + t + 6, CZ - 2.6, (0, 1))):
        # 07/11: virados para o meio da rua (antes giravam ao acaso: o braço apontava para a parede)
        if not (PLAZA[0] < x < PLAZA[2] and PLAZA[1] < z < PLAZA[3]) and free(x - .6, z - .6, x + .6, z + .6):
            add("vila_poste", x, z, yaw_facing(*f))
# barris e caixotes que quebram (na frente da taverna, no mercado e na forja)
for x, z, kind in ((CX - 19.3, CZ - 8.3, "wine"), (CX - 18.1, CZ - 8.3, "wine"), (CX - 4.8, CZ - 8.3, "water"),
                   (CX - 49.5, CZ - 7.5, "crate"), (CX - 48, CZ - 7.5, "water"), (CX - 34.5, CZ + 7.5, "crate"),
                   (CX - 43.6, CZ + 31.2, "crate"), (CX - 44.9, CZ + 31.2, "water"), (CX + 40.5, CZ - 2.2, "crate")):
    breakables.append({"kind": kind, "x": round(x, 2), "z": round(z, 2), "yaw": round(rng.uniform(0, 360), 1)})
# bandeiras nos portões
for gx, gz in ((0, -1), (0, 1), (1, 0)):
    px, pz = CX + gx * HALF, CZ + gz * HALF
    for s in (-1, 1):
        ox, oz = (s * 9, 0) if gx == 0 else (0, s * 9)
        cloth.append({"model": "town_banner_red", "x": round(px + ox, 2), "z": round(pz + oz + gz * -2.2, 2), "yaw": round(yaw_facing(gx, gz), 1), "y": 7.5, "dx": .12})

# a saída dos portões (11/11, o usuário: "muita planta perto das saídas dos portões, atrapalha a visão"): como nos
# castelos, um largo de terra batida na frente do portão (15 x 9 m, plano) e a estrada de 6 m saindo reta por 36 m;
# em volta, até 48 m do portão, a "clareira": sem árvores, arbustos, pedras nem grama alta (AVortexTown::Clears)
clearings, aprons = [], []
for gx, gz in ((0, -1), (0, 1), (1, 0)):
    ox, oz = CX + gx * HALF, CZ + gz * HALF          # o portão, na linha da muralha
    ax, az = -gz, gx                                   # ao longo da muralha
    for depth in range(1, 13):
        for w in ((-6, -3, 0, 3, 6) if depth <= 3 else (-1.5, 1.5)):
            x, z = ox + gx * (depth * 3 - .5) + ax * w, oz + gz * (depth * 3 - .5) + az * w
            add(rng.choice(DIRT), x, z, rng.choice((0, 90, 180, 270)))
            mask_rect(x - 1.5, z - 1.5, x + 1.5, z + 1.5)
    # o largo fica plano (o "flatten"): 16 m ao longo da muralha e 11 m para fora
    aprons.append([[round(ox + ax * u + gx * v, 2), round(oz + az * u + gz * v, 2)] for u, v in ((-8, -1), (8, -1), (8, 11), (-8, 11))])
    clearings.append([round(ox + gx * 24, 2), round(oz + gz * 24, 2), 24.0, 15.0, round(math.degrees(math.atan2(gz, gx)), 1)])
cobbles = [[round(x, 3), round(z, 3)] for x, z in sorted(mask)]

# 11/11: as variantes sem janela lateral (_g) saíram: as casas ficam a 2,5 m ou mais umas das outras
def local_to_world(p, lx, lz):
    a = math.radians(-p["yaw"])
    return p["x"] + lx * math.cos(a) - lz * math.sin(a), p["z"] + lx * math.sin(a) + lz * math.cos(a)


# ---- 11/11 (o usuário: "os caminhos certinhos"): um caminho de pedra de 1,2 m da porta de cada casa até a rua -----
kit = json.load(open(KIT, encoding="utf-8")) if os.path.exists(KIT) else {}
walks = 0
for p in [q for q in pieces if q["model"].startswith("vila_casa")]:
    d = MODELS[p["model"]][1]
    doors = kit.get(p["model"], {}).get("doors") or kit.get(p["model"], {}).get("closed_door") or [[-.62]]
    steps = []
    for k in range(9):
        x, z = local_to_world(p, doors[0][0] + .62, -(d / 2 - .3) - .75 - k * 1.5)
        if on_tiles(x - .2, z - .2, x + .2, z + .2):
            for sx, sz in steps:
                add("vila_rua_p", sx, sz, p["yaw"])
                cobbles.extend([round(math.floor((sx + ox) / .75) * .75 + .375, 3), round(math.floor((sz + oz) / .75) * .75 + .375, 3)] for ox in (-.4, .4) for oz in (-.4, .4))
            walks += bool(steps)
            break
        steps.append((x, z))
print("VILA %d casas com caminho até a rua" % walks)

# ---- os modelos (caixas de colisão do kit, ou o alicerce) ----------------------------------------------------
models = {}
for name, (w, d, _) in MODELS.items():
    if name in kit:
        models[name] = kit[name]
        continue
    floor = name.startswith(("vila_rua", "vila_laje", "vila_terra", "vila_horta")) or name == "vila_moinho_pas"
    tall = 7.0 if name in ("vila_muralha", "vila_torre", "vila_portao") else 5.0
    models[name] = {"boxes": [] if floor else [[0, tall / 2, 0, w, tall, d]],
                    "footing": [] if floor or name == "vila_moinho_pas" else [-w / 2, -d / 2, w / 2, d / 2],
                    "sign": [0, 3.2, -d / 2 - .1] if not floor else None, "lamps": []}
# os enfeites do Godot que continuam (por enquanto)
godot = json.load(open(os.path.join(ROOT, "Data", "GodotTown", "town.json"), encoding="utf-8"))["models"]
for p in pieces:
    if p["model"] not in models and p["model"] in godot:
        models[p["model"]] = godot[p["model"]]

# ---- a fumaça das chaminés (06/11, o usuário: "fumaça saindo das casas que têm chaminé, igual fumaça de chaminé"):
# o alto de cada chaminé do modelo (vila_models.json), girado junto com a peça; fina, clara e meio transparente
for p in pieces:
    for cx, cy, cz in models.get(p["model"], {}).get("chimneys", []):
        a = math.radians(-p["yaw"])
        wx = p["x"] + cx * math.cos(a) - cz * math.sin(a)
        wz = p["z"] + cx * math.sin(a) + cz * math.cos(a)
        # [x, y, z, largura (cm), altura (cm), opacidade, bolas]: 06/11, mais grossa (antes 55 cm e 0,22 quase sumia)
        smoke.append([round(wx, 2), round(cy, 2), round(wz, 2), 130, 950, .45, 14])

# ---- os NPCs da vila nos prédios novos (06/11, o usuário: "arrume os NPCs, estão todos bugados") ------------
# Data/Population/npc_posts.json: o posto (cm, Unreal X = x, Y = z), o giro (graus, Unreal) e a patrulha de cada um.
# Os lojistas ficam atrás do balcão (lojas e taverna de entrar), o Bram na forja, a Rhea na porta do quartel, o Ducci
# na carroça do mercado; a Lary e a Julia dão a volta na fonte.
def at_building(label, lx, lz):
    """Um ponto no espaço do prédio (Godot: x, z; a frente no -z) -> mundo (m), e o giro (Unreal) olhando a rua."""
    model, x, z, (fx, fz) = placed[label]
    a = math.radians(-yaw_facing(fx, fz))
    wx = x + lx * math.cos(a) - lz * math.sin(a)
    wz = z + lx * math.sin(a) + lz * math.cos(a)
    return wx, wz, math.degrees(math.atan2(fz, fx))


SHOP_BACK = 1.1           # atrás do balcão da loja (o balcão vai de -0,4 a 0,2 m no fundo da peça)
npc_spots = {
    "ilsa": at_building("ERVAS DE ILSA", 0.0, SHOP_BACK),
    "nicolly": at_building("MAPAS DE NICOLLY", 0.0, SHOP_BACK),
    "rosa": at_building("ALFAIATE", 0.0, SHOP_BACK),
    "garrick": at_building("ARMARIA", 0.0, SHOP_BACK),
    "marta": at_building("TAVERNA DO JAVALI DOURADO", 5.0, 4.55),
    "bram": at_building("FORJA DE BRAM", -0.7, -3.4),
    "rhea": at_building("QUARTEL DA GUARDA", 2.0, -7.0),
}
wagon = next(p for p in pieces if p["model"] == "vila_carroca")
npc_spots["ducci"] = (wagon["x"] + 2.3, wagon["z"] + .6, 0.0)
npc_spots["lexy"] = (CX + 10.5, CZ + 3.5, 180.0)
npc_spots["bunny"] = (CX - 9.0, CZ + 2.0, -12.5)   # 10/11: a jardineira, no canteiro do lado oeste da fonte, olhando para ela
NPCS = os.path.join(ROOT, "Data", "Population", "npc_posts.json")
npc_doc = json.load(open(NPCS, encoding="utf-8"))
for npc in npc_doc["npcs"]:
    if npc["id"] in npc_spots:
        x, z, yaw = npc_spots[npc["id"]]
        npc["post_cm"] = [round(x * 100, 1), round(z * 100, 1)]
        npc["yaw"] = round(yaw, 1)
        if "patrol_cm" in npc:
            npc["patrol_cm"] = []
    elif npc["id"] in ("lary", "julia"):
        # a volta na fonte (raio de 6,5 m; a Julia começa do outro lado e anda ao contrário)
        sign, start = (1, 0.0) if npc["id"] == "lary" else (-1, math.pi)
        ring = [(CX + 6.5 * math.cos(start + sign * n * math.pi / 5), CZ + 6.5 * math.sin(start + sign * n * math.pi / 5)) for n in range(10)]
        npc["post_cm"] = [round(ring[0][0] * 100, 1), round(ring[0][1] * 100, 1)]
        npc["patrol_cm"] = [[round(x * 100, 1), round(z * 100, 1)] for x, z in ring]
        npc["yaw"] = 90.0 * sign
# 07/11 (o usuário: "guardas em todas as entradas" e "mais alguns NPCs para completar a vila"): dois guardas por
# portão, por dentro, olhando para fora; o padeiro, a irmã da capela, o cavalariço e o carpinteiro nos prédios deles
def look(app, hair, outfit, skin, hair_color, face="determinado"):
    return {"race": "humano", "appearance": app, "parts": {"block_hair": "block_hair_" + hair, "block_face": face, "npc_outfit": outfit},
            "colors": {"skin_color": skin, "hair_color": hair_color}}


extra = []
GUARD_LOOKS = [("male", "raspado", "garrick", "#C98E6A", "#2E2019"), ("male", "undercut", "guerreiro", "#AA7558", "#1E1A1C"),
               ("female", "rabo_cavalo_alto", "rhea", "#D8A07E", "#3A2A20"), ("male", "liso_curto", "garrick", "#8A5A40", "#141012"),
               ("male", "raspado", "guerreiro", "#E0B090", "#7A5030"), ("female", "rabo_cavalo_alto", "rhea", "#AA7558", "#1E1A1C")]
gates = [p for p in pieces if p["model"] == "vila_portao"]
for i, g in enumerate(gates):
    for s, (lx, name) in enumerate(((-3.6, "esquerda"), (3.6, "direita"))):
        x, z = local_to_world(g, lx, 4.2)          # por dentro (a frente do portão, o -z, olha para fora)
        # 07/11 (o usuário: "os guardas estão todos de costas, olhando para o muro"): olham para dentro, para a rua
        fx, fz = local_to_world(g, 0, 1)
        app, hair, outfit, skin, hc = GUARD_LOOKS[(2 * i + s) % len(GUARD_LOOKS)]
        extra.append({"id": "guarda_%d_%d" % (i, s), "name": "Guarda", "title": "Guarda do %s" % g["label"].title().replace("Do", "do").replace("Da", "da"),
                      "post_cm": [round(x * 100, 1), round(z * 100, 1)], "yaw": round(math.degrees(math.atan2(fz - g["z"], fx - g["x"])), 1),
                      "patrol_cm": [], "look": look(app, hair, outfit, skin, hc)})
for npc_id, name, title, label, lx, lz, lk in (
        ("tomas", "Tomás", "Padeiro", "PADARIA", 0.0, SHOP_BACK, look("male", "liso_curto", "aventureiro", "#D8A07E", "#5A3A26", "padrao")),
        ("irma_celia", "Irmã Célia", "Cuidadora da capela", "CAPELA", 1.6, 6.2, look("female", "meio_preso", "sacerdotisa", "#E8C0A0", "#2E2019", "padrao")),
        ("joao_cavalarico", "João", "Cavalariço", "ESTÁBULO", 2.0, -5.6, look("male", "undercut", "aventureiro", "#AA7558", "#2E2019", "padrao")),
        ("edda", "Edda", "Carpinteira", "CARPINTARIA", 1.5, -.5, look("female", "rabo_cavalo_alto", "aventureira", "#C98E6A", "#7A3A20"))):
    x, z, yaw = at_building(label, lx, lz)
    extra.append({"id": npc_id, "name": name, "title": title, "post_cm": [round(x * 100, 1), round(z * 100, 1)], "yaw": round(yaw, 1),
                  "patrol_cm": [], "look": lk})
# o andarilho da avenida: vai do portão norte à praça e volta
walk = [(CX + 1.8, CZ - HALF + 9), (CX + 1.8, CZ - 30), (CX + 1.8, CZ - 12), (CX - 1.8, CZ - 12), (CX - 1.8, CZ - 30), (CX - 1.8, CZ - HALF + 9)]
extra.append({"id": "bento", "name": "Bento", "title": "Morador", "post_cm": [round(walk[0][0] * 100, 1), round(walk[0][1] * 100, 1)], "yaw": 90.0,
              "patrol_cm": [[round(x * 100, 1), round(z * 100, 1)] for x, z in walk], "look": look("male", "liso_curto", "aventureiro", "#E0B090", "#7A5030", "padrao")})
# 11/11 (o usuário: "no mundo pode fazer os viajantes, roupas únicas para eles"): saem de um portão pela estrada, vão até
# uns 200 m e voltam (a patrulha do AVortexNpc). As roupas são do Tools/Blender/MakeBlockOutfit.py (viajante_*)
for npc_id, name, title, (gx, gz), lk in (
        ("viajante_mercador", "Osvaldo", "Mercador viajante", (0, -1), look("male", "liso_curto", "viajante_mercador", "#C98E6A", "#5A3A26", "padrao")),
        ("viajante_bardo", "Lírio", "Bardo andarilho", (1, 0), look("male", "undercut", "viajante_bardo", "#E0B090", "#7A3A20", "padrao")),
        ("viajante_campones", "Tião", "Camponês", (0, 1), look("male", "raspado", "viajante_campones", "#AA7558", "#2E2019", "padrao")),
        ("viajante_peregrina", "Irene", "Peregrina", (0, -1), look("female", "meio_preso", "Traje_peregrino_f", "#D8A07E", "#3A2A20", "padrao"))):
    side = 2.0 if npc_id == "viajante_peregrina" else -2.0           # cada um num lado da estrada
    ax, az = -gz, gx
    route = [(CX + gx * (HALF + d) + ax * side, CZ + gz * (HALF + d) + az * side) for d in (-6, 30, 80, 140, 200)]
    extra.append({"id": npc_id, "name": name, "title": title, "post_cm": [round(route[0][0] * 100, 1), round(route[0][1] * 100, 1)],
                  "yaw": round(math.degrees(math.atan2(gz, gx)), 1), "patrol_cm": [[round(x * 100, 1), round(z * 100, 1)] for x, z in route], "look": lk})
ids = {e["id"] for e in extra}
npc_doc["npcs"] = [n for n in npc_doc["npcs"] if n["id"] not in ids] + extra
npc_doc["_doc_vila"] = ("06/11: os postos dos NPCs da cidade são escritos por Tools/Town/BuildVila.py (a vila nova): "
                        "lojistas atrás do balcão, Bram na forja, Rhea no quartel, Ducci na carroça, Lary e Julia na fonte.")
json.dump(npc_doc, open(NPCS, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print("VILA NPCs:", ", ".join("%s (%.1f, %.1f)" % (k, v[0], v[1]) for k, v in npc_spots.items()))

# ---- o chão plano -----------------------------------------------------------------------------------------
T = terrain_peek.load()
heights = sorted(float(terrain_peek.at(T, x, z)) for x in range(int(CX - HALF), int(CX + HALF), 4) for z in range(int(CZ - HALF), int(CZ + HALF), 4) if inside(WALL, x, z))
level = round(heights[len(heights) // 2] * 4) / 4
level = max(level, T["water"] + .75)
FLAT = octagon(HALF + 8.0, CUT + 3.0)
# o moinho e as hortas também ficam planos
wet = [(x, z) for x in range(int(CX - HALF - 40), int(CX + HALF + 40), 3) for z in range(int(CZ - HALF - 40), int(CZ + HALF + 40), 3)
       if not inside(FLAT, x, z) and terrain_peek.at(T, x, z) <= T["water"] and min(abs(x - CX), abs(z - CZ)) < HALF + 20 and max(abs(x - CX), abs(z - CZ)) < HALF + 14]

plan = {
    "_doc": "Gerado por Tools/Town/BuildVila.py (05/11): a vila nova. Não editar à mão; mudar o script e rodar de novo.",
    "bounds": [CX - HALF - 3, CZ - HALF - 3, CX + HALF + 3, CZ + HALF + 3],
    "flatten": {"polys": [[[round(x, 2), round(z, 2)] for x, z in FLAT]] + aprons, "level": level, "blend": 16.0},
    # 11/11: [x, z, meio comprimento (para fora do portão), meia largura, giro do eixo x em graus] (m)
    "clearings": clearings,
    "models": models, "pieces": pieces, "trees": trees, "cobbles": cobbles,
    "spinners": spinners, "cloth": cloth, "smoke": smoke, "glows": glows, "breakables": breakables, "animals": animals,
}
os.makedirs(os.path.dirname(OUT), exist_ok=True)
json.dump(plan, open(OUT, "w", encoding="utf-8"), ensure_ascii=False, separators=(",", ":"))
counts = {}
for p in pieces:
    key = "rua/laje/terra" if p["model"].startswith(("vila_rua", "vila_laje", "vila_terra")) else "casas" if p["model"].startswith("vila_casa") else p["model"]
    counts[key] = counts.get(key, 0) + 1
print("VILA nível %.2f m (água %.2f), %d peças: %s" % (level, T["water"], len(pieces), counts))
print("VILA %d árvores, %d células sem grama, %d janelas, %d barris; lago perto da muralha: %d pontos" % (len(trees), len(cobbles), len(glows), len(breakables), len(wet)))

# ---- a planta vista de cima ---------------------------------------------------------------------------------
if "--sem-foto" not in sys.argv:
    from PIL import Image, ImageDraw, ImageFont
    S = 6                                     # pixels por metro
    X0, Z0 = CX - HALF - 45, CZ - HALF - 45
    W = int((2 * HALF + 90) * S)
    img = Image.new("RGB", (W, W), (112, 140, 82))
    d = ImageDraw.Draw(img)

    def P(x, z):
        return ((x - X0) * S, (z - Z0) * S)
    # o relevo e a água em volta
    for x in range(int(X0), int(X0 + W / S), 2):
        for z in range(int(Z0), int(Z0 + W / S), 2):
            hgt = terrain_peek.at(T, x, z)
            if hgt <= T["water"] and not inside(FLAT, x, z):
                d.rectangle([P(x, z), P(x + 2, z + 2)], fill=(64, 110, 170))
            elif not inside(FLAT, x, z):
                g = max(-1, min(1, (hgt - level) / 6))
                d.rectangle([P(x, z), P(x + 2, z + 2)], fill=(int(112 + 40 * g), int(140 + 30 * g), int(82 + 20 * g)))
    colors = {"rua": (150, 146, 138), "laje": (176, 170, 158), "terra": (170, 132, 92)}
    for p in pieces:
        m = p["model"]
        if m.startswith(("vila_rua", "vila_laje", "vila_terra")):
            c = colors["rua" if "rua" in m else "laje" if "laje" in m else "terra"]
            h = .75 if m == "vila_rua_p" else 1.5
            d.polygon([P(p["x"] + ox, p["z"] + oz) for ox, oz in ((-h, -h), (h, -h), (h, h), (-h, h))], fill=c)
    font = ImageFont.load_default()
    for p in pieces:
        m = p["model"]
        if m.startswith(("vila_rua", "vila_laje", "vila_terra", "vila_borda")) or m not in MODELS or m == "vila_moinho_pas" or MODELS[m][2] == "enfeite":
            continue
        w, dd, _ = MODELS[m]
        a = math.radians(-p["yaw"])
        corners = []
        for ox, oz in ((-w / 2, -dd / 2), (w / 2, -dd / 2), (w / 2, dd / 2), (-w / 2, dd / 2)):
            # giro do Unreal (yaw = -Yaw) no plano x/z
            corners.append(P(p["x"] + ox * math.cos(a) - oz * math.sin(a), p["z"] + ox * math.sin(a) + oz * math.cos(a)))
        if m in ("vila_muralha", "vila_torre", "vila_portao"):
            fill = (96, 96, 104)
        elif m.startswith("vila_casa"):
            fill = (188, 104, 62) if m[-1] in "1268" else (120, 84, 60) if m[-1] in "47" else (200, 176, 96)
        elif m == "vila_horta":
            fill = (92, 120, 52)
        else:
            fill = (150, 70, 120)
        d.polygon(corners, fill=fill, outline=(40, 30, 24))
        # a porta (a frente)
        fx, fz = -math.sin(a) * -1, math.cos(a) * -1
        door = (p["x"] + (-dd / 2) * -math.sin(a) * -1, p["z"] + (-dd / 2) * math.cos(a))
        d.ellipse([P(door[0] - .6, door[1] - .6), P(door[0] + .6, door[1] + .6)], fill=(250, 230, 120))
        if p.get("label") and m != "vila_portao":
            tx, ty = P(p["x"], p["z"])
            d.text((tx - len(p["label"]) * 3, ty - 5), p["label"], fill=(255, 255, 255), font=font)
    for t in trees:
        d.ellipse([P(t["x"] - 2, t["z"] - 2), P(t["x"] + 2, t["z"] + 2)], fill=(58, 96, 46))
    for p in pieces:
        if p["model"] == "vil_fountain":
            d.ellipse([P(p["x"] - 3.2, p["z"] - 3.2), P(p["x"] + 3.2, p["z"] + 3.2)], fill=(90, 160, 200), outline=(220, 220, 220))
    for g, name in ((WALL[0], ""),):
        pass
    for p in pieces:
        if p["model"] == "vila_portao":
            tx, ty = P(p["x"], p["z"])
            d.text((tx - len(p["label"]) * 3, ty + 14), p["label"], fill=(255, 250, 200), font=font)
    os.makedirs(os.path.dirname(PREVIEW), exist_ok=True)
    img.save(PREVIEW)
    print("VILA planta:", PREVIEW)
