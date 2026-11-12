"""A malha de caminhos dos moradores da vila (08/11): onde dá para andar e os lugares da rotina.

Lê Data/GodotTown/vila.json (a vila de Tools/Town/BuildVila.py) e grava Data/Population/vila_nav.json:
- uma grade de 0,5 m ('1' anda, '0' parede, móvel, cerca, árvore): as caixas de colisão de cada peça que ocupam a altura
  de uma pessoa (de 0,45 m a 1,7 m do chão), engordadas meio metro (o raio do personagem); as portas das casas fechadas
  contam como parede (as das lojas, da taverna e da capela ficam abertas, como na vila);
- os lugares: a porta de fora de cada casa (onde o morador "entra para dormir"), a porta e o salão das construções com
  nome (taverna, capela, forja...), os bancos (onde sentar e para que lado) e a roda em volta da fonte da praça.
Cada ponto é puxado para a casa andável mais perto. AVortexNpc acha o caminho nesta grade (A*).

    python Tools/Town/BuildNpcNav.py
"""
import json
import math
import os
from collections import deque

import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
VILA = os.path.join(ROOT, "Data", "GodotTown", "vila.json")
OUT = os.path.join(ROOT, "Data", "Population", "vila_nav.json")
CELL = .5
GROUND = ("vila_rua", "vila_laje", "vila_terra", "vila_borda")


def rot(yaw, lx, lz):
    """O giro da vila (AVortexTown: FRotator(0, -yaw, 0) aplicado em (x, z) do Godot)."""
    t = math.radians(-yaw)
    return lx * math.cos(t) - lz * math.sin(t), lx * math.sin(t) + lz * math.cos(t)


def main():
    vila = json.load(open(VILA, encoding="utf-8"))
    x0, z0, x1, z1 = vila["bounds"]
    x0, z0, x1, z1 = x0 - 4, z0 - 4, x1 + 4, z1 + 4
    W, H = int(math.ceil((x1 - x0) / CELL)), int(math.ceil((z1 - z0) / CELL))
    blocked = np.zeros((H, W), bool)
    cx = x0 + (np.arange(W) + .5) * CELL
    cz = z0 + (np.arange(H) + .5) * CELL
    gx, gz = np.meshgrid(cx, cz)

    def box(px, pz, yaw, bx, bz, sx, sz):
        """Marca a caixa (centro e tamanho no espaço da peça) na grade."""
        wx, wz = rot(yaw, bx, bz)
        wx, wz = px + wx, pz + wz
        r = math.hypot(sx, sz) * .5 + CELL
        i0, i1 = max(0, int((wz - r - z0) / CELL)), min(H, int((wz + r - z0) / CELL) + 1)
        j0, j1 = max(0, int((wx - r - x0) / CELL)), min(W, int((wx + r - x0) / CELL) + 1)
        if i0 >= i1 or j0 >= j1:
            return
        dx, dz = gx[i0:i1, j0:j1] - wx, gz[i0:i1, j0:j1] - wz
        t = math.radians(yaw)       # volta para o espaço da peça (o giro inverso)
        lx = dx * math.cos(t) - dz * math.sin(t)
        lz = dx * math.sin(t) + dz * math.cos(t)
        blocked[i0:i1, j0:j1] |= (np.abs(lx) <= sx * .5) & (np.abs(lz) <= sz * .5)

    places = {"houses": [], "buildings": {}, "benches": [], "plaza": [], "board": None}
    for p in vila["pieces"]:
        model, label = p["model"], p.get("label", "")
        if model.startswith(GROUND):
            continue
        info = vila["models"].get(model, {})
        px, pz, yaw = p["x"], p["z"], p["yaw"]
        for b in info.get("boxes", []):
            if len(b) < 6:
                continue
            bottom, top = b[1] - b[4] * .5, b[1] + b[4] * .5
            if top > .45 and bottom < 1.7:
                box(px, pz, yaw, b[0], b[2], b[3], b[5])
        doors = info.get("doors", []) or info.get("closed_door", [])
        for d in info.get("doors", []):
            if not label:           # a porta fechada da casa (a dobradiça em x, a folha para +x)
                box(px, pz, yaw, d[0] + d[3] * .5, d[2], d[3], .2)
        if doors:
            d = doors[0]
            ox, oz = rot(yaw, d[0] + d[3] * .5, d[2] - 1.0)        # fora (a frente da peça é -z)
            ix, iz = rot(yaw, d[0] + d[3] * .5, d[2] + 1.6)        # dentro
            entry = {"model": model, "door": [px + ox, pz + oz], "inside": [px + ix, pz + iz], "center": [px, pz], "yaw": yaw}
            if label:
                places["buildings"][label] = entry
            elif model.startswith("vila_casa"):
                places["houses"].append(entry)
        if model == "vila_banco":
            fx, fz = rot(yaw, 0, -.55)
            places["benches"].append({"seat": [px + fx * .3 / .55, pz + fz * .3 / .55], "stand": [px + fx, pz + fz], "yaw": yaw})
        if model == "vil_fountain":
            for k in range(8):
                a = k / 8 * 2 * math.pi
                places["plaza"].append([px + math.cos(a) * 5.5, pz + math.sin(a) * 5.5])
        if model == "vila_quadro":
            fx, fz = rot(yaw, 0, -1.2)
            places["board"] = [px + fx, pz + fz]
    for t in vila.get("trees", []):
        box(t["x"], t["z"], 0, 0, 0, .9, .9)

    # engorda meio metro (o raio do personagem), mas a cela de um banco continua sendo onde se senta
    walk = ~blocked
    grown = blocked.copy()
    grown[1:, :] |= blocked[:-1, :]
    grown[:-1, :] |= blocked[1:, :]
    grown[:, 1:] |= blocked[:, :-1]
    grown[:, :-1] |= blocked[:, 1:]
    walk = ~grown

    def snap(pt):
        """O ponto andável mais perto (busca em largura na grade)."""
        j, i = int((pt[0] - x0) / CELL), int((pt[1] - z0) / CELL)
        i, j = min(max(i, 0), H - 1), min(max(j, 0), W - 1)
        if walk[i, j]:
            return [round(pt[0], 2), round(pt[1], 2)]
        seen = {(i, j)}
        q = deque([(i, j)])
        while q:
            a, b = q.popleft()
            for da, db in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                n = (a + da, b + db)
                if 0 <= n[0] < H and 0 <= n[1] < W and n not in seen:
                    if walk[n]:
                        return [round(x0 + (n[1] + .5) * CELL, 2), round(z0 + (n[0] + .5) * CELL, 2)]
                    seen.add(n)
                    q.append(n)
        return [round(pt[0], 2), round(pt[1], 2)]

    for h in places["houses"]:
        h["door"] = snap(h["door"])
    for b in places["buildings"].values():
        b["door"], b["inside"], b["center"] = snap(b["door"]), snap(b["inside"]), snap(b["center"])
    for b in places["benches"]:
        b["stand"] = snap(b["stand"])
    places["plaza"] = [snap(p) for p in places["plaza"]]
    if places["board"]:
        places["board"] = snap(places["board"])

    # a maior região ligada (o resto é canto fechado: dentro de casa, atrás do muro)
    lab = np.zeros((H, W), np.int32)
    best, best_n, cur = 0, 0, 0
    for i in range(H):
        for j in range(W):
            if walk[i, j] and not lab[i, j]:
                cur += 1
                n = 0
                q = deque([(i, j)])
                lab[i, j] = cur
                while q:
                    a, b = q.popleft()
                    n += 1
                    for da, db in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        c, d = a + da, b + db
                        if 0 <= c < H and 0 <= d < W and walk[c, d] and not lab[c, d]:
                            lab[c, d] = cur
                            q.append((c, d))
                if n > best_n:
                    best, best_n = cur, n
    main_area = lab == best
    out = {
        "_doc": "Gerado por Tools/Town/BuildNpcNav.py (08/11). Metros do Godot (Unreal: X = x*100, Y = z*100). walk: H linhas de W "
                "caracteres, '1' anda. A grade começa em origin (canto de baixo) e cada casa tem cell metros.",
        "origin": [x0, z0], "cell": CELL, "w": W, "h": H,
        "walk": "".join("".join("1" if v else "0" for v in row) for row in main_area),
        "places": places,
    }
    json.dump(out, open(OUT, "w", encoding="utf-8"), ensure_ascii=False)
    print("grade %dx%d, andável %.0f%%, casas %d, construções %d (%s), bancos %d, praça %d" % (
        W, H, main_area.mean() * 100, len(places["houses"]), len(places["buildings"]), ", ".join(places["buildings"]),
        len(places["benches"]), len(places["plaza"])))
    # conferência: cada lugar cai na área ligada
    def ok(pt):
        j, i = int((pt[0] - x0) / CELL), int((pt[1] - z0) / CELL)
        return 0 <= i < H and 0 <= j < W and main_area[i, j]
    bad = [h["door"] for h in places["houses"] if not ok(h["door"])] + [b["door"] for b in places["buildings"].values() if not ok(b["door"])]
    print("portas fora da área ligada:", len(bad), bad[:5])
    from PIL import Image
    img = np.zeros((H, W, 3), np.uint8)
    img[walk] = (90, 90, 90)
    img[main_area] = (200, 200, 200)
    for h in places["houses"]:
        j, i = int((h["door"][0] - x0) / CELL), int((h["door"][1] - z0) / CELL)
        img[max(0, i - 1):i + 2, max(0, j - 1):j + 2] = (220, 60, 60)
    for b in places["buildings"].values():
        for k, col in (("door", (60, 160, 255)), ("inside", (60, 220, 90))):
            j, i = int((b[k][0] - x0) / CELL), int((b[k][1] - z0) / CELL)
            img[max(0, i - 1):i + 2, max(0, j - 1):j + 2] = col
    for b in places["benches"]:
        j, i = int((b["stand"][0] - x0) / CELL), int((b["stand"][1] - z0) / CELL)
        img[max(0, i - 1):i + 2, max(0, j - 1):j + 2] = (240, 200, 40)
    os.makedirs(os.path.join(ROOT, "Saved", "WorldBake"), exist_ok=True)
    Image.fromarray(img).resize((W * 3, H * 3), Image.NEAREST).save(os.path.join(ROOT, "Saved", "WorldBake", "previa_npc_nav.png"))


if __name__ == "__main__":
    main()
