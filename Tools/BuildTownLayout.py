"""Planta da Cidade do Santuário (a vila inicial do Godot) para a Unreal: Data/GodotTown/town.json.

python Tools/BuildTownLayout.py

Só lê o Godot (Art/World/town_props.json, village_props.json, village_houses.json) e copia as listas de
Game/World/ValleyWorld.Town.cs e ValleyWorld.Village.cs (posições em metros, giro em graus, frente
de cada peça no -Z do Godot). Na Unreal: X = x, Y = z, giro = -giro (como as plantas do terreno).

Saída:
  models: caixas de colisão [cx, cy, cz, sx, sy, sz], placa [x, y, z], lampiões, alicerce (espaço da peça, Godot)
  pieces: modelo, placa, x, z, giro
  trees:  espécie, x, z, escala
  cobbles: [x, z, tom 0..1, musgo 0/1] das pedras da praça e dos caminhos (75 cm)
"""
import json
import math
import random
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GODOT = ROOT.parents[1] / "the-world-geneses" / "Art" / "World"
OUT = ROOT / "Data" / "GodotTown" / "town.json"

TOWN_W, TOWN_E, TOWN_S, TOWN_N = -40.0, 40.0, -24.0, 56.0
VILLAGE = (0.0, 15.0)
PLAZA_R = 8.6
CELL = 0.75


def town_pieces():
    p = [
        ("town_gatehouse", "CIDADE DO SANTUÁRIO", 0, TOWN_S, 0),
        ("town_gatehouse", "PORTÃO NORTE", 0, TOWN_N, 180),
        ("town_inn", "TAVERNA DO JAVALI DOURADO", -29, 36, -90),
        ("town_belltower", "", -12, -12, 0),
        ("town_house_a", "", -12, 44, -90), ("town_house_b", "", 12, 40, 90), ("town_house_c", "", 12, 49, 90),
        ("town_house_b", "", -30, 8, -90), ("town_house_c", "", -30, 18, -90), ("town_house_a", "", 30, 40, 90),
        ("town_house_b", "", 30, 48, 90), ("town_house_a", "", -28, -10, -90), ("town_house_b", "", -28, -18, -90),
        ("town_cottage", "", -31, -1, -90), ("town_house_d", "", -31, 27, -90), ("town_shop", "ALFAIATE", -31, 47, -90),
        ("town_cottage", "", -17, 51, 0), ("town_cottage", "", 19, 52, 0),
        ("town_house_d", "", 31, 27, 90), ("town_shop", "PADARIA", 31, 16, 90), ("town_cottage", "", 31, 5, 90),
        ("town_house_c", "", 34, -9, 90), ("town_shop", "ARMARIA", 34, -19, 90), ("town_cottage", "", -18, -20, 180),
        ("vil_well", "", -20.5, 26, 90), ("vil_planter", "", -26, 12.5, 90), ("vil_planter", "", -26, 31, 90),
        ("vil_planter", "", 26.5, 21.5, -90), ("vil_planter", "", 26.5, 10.5, -90), ("vil_bench", "", -21.5, 40, 90),
        ("vil_bench", "", 21.5, 36, -90), ("vil_bench", "", -21.5, -8, 90), ("vil_crates", "", 36.5, -14, 0),
    ]
    for x in (-36, -28, -20, -12, 12, 20, 28, 36):
        p.append(("town_wall", "", x, TOWN_S, 0))
        p.append(("town_wall", "", x, TOWN_N, 180))
    z = TOWN_S + 4
    while z < TOWN_N:
        p.append(("town_wall", "", TOWN_W, z, 90))
        p.append(("town_wall", "", TOWN_E, z, -90))
        z += 8
    for x, z, yaw in ((TOWN_W, TOWN_S, 0), (TOWN_E, TOWN_S, 0), (TOWN_W, TOWN_N, 180), (TOWN_E, TOWN_N, 180), (TOWN_W, 16, 90), (TOWN_E, 16, -90)):
        p.append(("town_tower", "", x, z, yaw))
    for z in (-18, -10, 30, 38, 46):
        p.append(("vil_lamp", "", -2.8, z, 90))
        p.append(("vil_lamp", "", 2.8, z, -90))
    for z in (-16, -4, 8, 20, 32, 44):
        p.append(("vil_lamp", "", -22.2, z, -90))
    for z in (8, 20, 30, 42):
        p.append(("vil_lamp", "", 22.2, z, 90))
    for z in (-18, -6):
        p.append(("vil_lamp", "", 27.6, z, 90))
    return p


def village_pieces():
    p = [
        ("house_herbalist", "ERVAS DE ILSA", 16.5, 9.5, 90.0),
        ("house_smithy", "FORJA DE BRAM", -17, 14.5, -90.0),
        ("house_mapmaker", "MAPAS DE NICOLLY", 16.5, 23.5, 90.0),
    ]
    nan = None
    props = [
        ("vil_fountain", "", 0, 15, 0), ("vil_tower", "", -8.5, -4.5, 0), ("vil_wagon", "BAZAR DO DUCCI", -14.5, 27.5, nan),
        ("vil_stall_a", "", 6.5, 30, nan), ("vil_stall_b", "", -4.5, 31, nan), ("vil_well", "", -10, 4.5, 0),
        ("vil_board", "MISSÕES", 5.2, 5.2, nan), ("vil_crates", "", -12.5, 9, 30), ("vil_crates", "", -19.5, 31, 110),
        ("vil_crates", "", 11.5, 30.5, 200),
        ("vil_planter", "", 11.2, 5.5, 90), ("vil_planter", "", 11.2, 13.6, 90), ("vil_planter", "", 11.2, 19.5, 90),
        ("vil_planter", "", 11.2, 27.6, 90), ("vil_planter", "", -3.8, 4.6, 0), ("vil_planter", "", 3.8, 26.8, 180),
    ]
    for deg in (45, 135, 225, 315):
        a = math.radians(deg)
        props.append(("vil_bench", "", math.cos(a) * 5.6, 15 + math.sin(a) * 5.6, nan))
    for deg in (20, 70, 110, 160, 200, 250, 290, 340):
        a = math.radians(deg)
        props.append(("vil_lamp", "", math.cos(a) * (PLAZA_R + .8), 15 + math.sin(a) * (PLAZA_R + .8), -deg + 90))
    for z in (1, -4):
        props.append(("vil_lamp", "", -3.4, z, 90))
        props.append(("vil_lamp", "", 2.2, z, -90))
    for k in range(3):
        props.append(("vil_fence", "", 21.5, 4 + k * 2.1, 90))
    for model, label, x, z, yaw in props:
        if yaw is None:   # de frente para a praça
            yaw = math.degrees(math.atan2(-(VILLAGE[0] - x), -(VILLAGE[1] - z)))
        p.append((model, label, x, z, yaw))
    return p


TREES = [
    ("tree_cherry", -6, 50, .9), ("tree_cherry", 6, 53, .85), ("tree_cherry", -19, 40.5, .8), ("tree_cherry", 21, 44.5, .8),
    ("tree_oak", -36.5, 52.5, .9), ("tree_oak", 36.5, 53, .85), ("tree_oak", -36, -21, .9), ("tree_oak", 21, -21, .85),
    ("tree_birch", -36, 22.5, .9), ("tree_autumn", 36.5, 34, .85), ("tree_cherry", -8, -19, .8),
]

PATHS = [
    ((0, 15), (0, TOWN_S - 3), 3), ((0, 15), (0, TOWN_N + 3), 3),
    ((0, 36), (-24, 36), 2), ((0, 44), (-8, 44), 1.8), ((0, 40), (8, 40), 1.8), ((0, 49), (8, 49), 1.8),
    ((-24, -21), (-24, 50), 2.4), ((-24, 0), (0, 0), 2.2), ((-18, -17.5), (-24, -17.5), 1.6),
    ((-17, 47.5), (-17, 44), 1.6), ((-17, 44), (-24, 44), 1.8),
    ((24, 2), (24, 50), 2.4), ((0, 33), (24, 33), 2.2), ((0, 44), (24, 44), 2),
    ((24, 2), (29.5, -2), 1.8), ((29.5, -2), (29.5, -21), 1.8), ((19, 48), (19, 44), 1.6),
    ((0, 15), (11.5, 9.5), 2), ((0, 15), (11.5, 23.5), 2),
    ((0, 15), (-12.5, 14.5), 2), ((0, 15), (-11.5, 25.5), 2),
    ((0, 15), (1, 29.5), 2.2), ((-1.2, -1), (-7, -2), 1.6), ((-3, 5), (-8.5, 5), 1.4),
]


def on_cobble(x, z):
    if math.hypot(x - VILLAGE[0], z - VILLAGE[1]) <= PLAZA_R:
        return True
    for (ax, az), (bx, bz), w in PATHS:
        sx, sz = bx - ax, bz - az
        t = max(0.0, min(1.0, ((x - ax) * sx + (z - az) * sz) / (sx * sx + sz * sz)))
        if math.hypot(x - (ax + sx * t), z - (az + sz * t)) <= w * .5:
            return True
    return False


def cobbles():
    rng = random.Random(77)
    out = []
    x = TOWN_W - 2
    while x <= TOWN_E + 2:
        z = TOWN_S - 4
        while z <= TOWN_N + 4:
            cx = (math.floor(x / CELL) + .5) * CELL
            cz = (math.floor(z / CELL) + .5) * CELL
            if on_cobble(cx, cz):
                t = rng.random()
                moss = 1 if rng.random() < .08 else 0
                out.append([round(cx, 3), round(cz, 3), round(t, 3), moss])
            z += CELL
        x += CELL
    return out


def main():
    models = {}
    for name in ("town_props.json", "village_props.json", "village_houses.json"):
        data = json.loads((GODOT / name).read_text(encoding="utf-8"))
        for key, info in data.items():
            if isinstance(info, dict) and "boxes" in info:
                models[key] = {k: info.get(k) for k in ("boxes", "sign", "lamps", "footing")}
    pieces = [{"model": m, "label": l, "x": round(x, 3), "z": round(z, 3), "yaw": round(y, 3)} for m, l, x, z, y in town_pieces() + village_pieces()]
    missing = sorted({p["model"] for p in pieces} - set(models))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps({
        "_doc": "Gerado por Tools/BuildTownLayout.py a partir do Godot (somente leitura). Metros, coordenadas do Godot.",
        "bounds": [TOWN_W, TOWN_S, TOWN_E, TOWN_N],
        "models": models, "pieces": pieces,
        "trees": [{"kind": k, "x": x, "z": z, "scale": s} for k, x, z, s in TREES],
        "cobbles": cobbles(),
    }, ensure_ascii=False) + "\n", encoding="utf-8")
    print("TOWN_LAYOUT pecas=%d modelos=%d pedras=%d faltando=%s" % (len(pieces), len(models), len(cobbles()), missing))


if __name__ == "__main__":
    main()
