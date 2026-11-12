"""Os lugares marcantes em volta da Cidade do Santuário (07/11, o usuário: "uns 6 espalhados no inicial, bem grandes,
marcantes e lindos", seguindo o Docs/03_Mundo/REFERENCIA_REMODELAGEM_TERRENO_CONTINENTE.pdf, item 5): carimbos de relevo
no mapa do mundo e os caminhos que ligam a cidade a eles.

Cada lugar molda o chão (o mapa de alturas de 6 m por ponto, Data/GodotWorld/world_height.bin) com a forma dele, e as
peças por cima (a cachoeira, o círculo de pedras, o arco, a torre, a raiz, os cristais: Tools/Blender/MakeMarvels.py)
são montadas pelo AVortexLandmarks nos pontos que este script grava em world_features.json ("marvels").
Os caminhos: o trecho menos íngreme da borda da cidade até a entrada de cada lugar (A* no mapa), alisado e marcado
como caminho (world_flags.bin = 4: o chão de terra batida e sem plantas, FVortexTerrainData::RegionLayer).

Sempre parte do mapa original (world_height_base.bin e world_flags_base.bin, gravados na primeira vez e pelo
BakeWorld.py), então rodar de novo não empilha relevo.

    python Tools/World/StampMarvels.py
"""
import heapq
import json
import math
import os
import shutil

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DATA = os.path.join(ROOT, "Data", "GodotWorld")
PREVIEW = os.path.join(ROOT, "Saved", "WorldBake")
MPP, WL = 6.0, 2.05
PATH_FLAG = 4

# id, nome, tipo, centro (m, x/z do mundo), raio (m)
MARVELS = [
    ("mirante", "Mirante da Cachoeira", "waterfall", (930, 30)),
    ("anel", "Colina do Anel de Pedras", "stonering", (470, -800)),
    ("arco", "Arco do Vento", "arch", (-470, -820)),
    ("espelho", "Lago Espelho", "lake", (-1180, 120)),
    ("raiz", "Ponte de Raiz", "rootbridge", (800, 470)),
    ("cristais", "Ninho dos Cristais", "crystals", (-20, -950)),
]
# o recorte do vale exportado do Godot (cm -> m): lá o chão vem das células exportadas, não do mapa
EXPORTED = (-900.0, 198.0, -540.0, 558.0)


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)


def fbm(shape, seed, scale, octaves=3):
    rng = np.random.default_rng(seed)
    out = np.zeros(shape, np.float32)
    amp, tot = 1.0, 0.0
    for o in range(octaves):
        s = max(1.0, scale / (2 ** o))
        g = rng.standard_normal((int(shape[0] / s) + 3, int(shape[1] / s) + 3)).astype(np.float32)
        out += amp * ndimage.zoom(g, s, order=3)[:shape[0], :shape[1]]
        tot += amp
        amp *= .5
    return out / tot


def main():
    meta = json.load(open(os.path.join(DATA, "world_meta.json"), encoding="utf-8"))
    S = meta["size"]
    HU, HV = meta["home_pixel"]
    for name in ("world_height", "world_flags"):
        base = os.path.join(DATA, name + "_base.bin")
        if not os.path.exists(base):
            shutil.copy(os.path.join(DATA, name + ".bin"), base)
    height = np.fromfile(os.path.join(DATA, "world_height_base.bin"), np.float16).astype(np.float32).reshape(S, S)
    flags = np.fromfile(os.path.join(DATA, "world_flags_base.bin"), np.uint8).reshape(S, S).copy()
    region = np.fromfile(os.path.join(DATA, "world_region.bin"), np.uint8).reshape(S, S)
    yy, xx = np.mgrid[0:S, 0:S].astype(np.float32)
    rough = fbm((S, S), 77, 9)
    crag = fbm((S, S), 78, 4, 2)

    def px(x, z):
        return x / MPP + HU, z / MPP + HV

    def world(ix, iy):
        return [round((ix - HU) * MPP, 1), round((iy - HV) * MPP, 1)]

    marvels = []
    entries = []
    for mid, name, kind, (cx_m, cz_m) in MARVELS:
        cx, cy = px(cx_m, cz_m)
        # o eixo do lugar: virado para a cidade (u = para a cidade)
        a = math.atan2(-cz_m, -cx_m)
        du, dv = (xx - cx) * MPP, (yy - cy) * MPP
        u = du * math.cos(a) + dv * math.sin(a)
        v = -du * math.sin(a) + dv * math.cos(a)
        r = np.hypot(du, dv)
        ring = (r > 160) & (r < 260) & (height > WL)
        b = float(np.median(height[ring])) if ring.any() else 8.0
        T = np.full((S, S), b, np.float32)
        w = np.zeros((S, S), np.float32)
        path = np.zeros((S, S), bool)
        props = []
        L = lambda uu, vv: [round(cx_m + uu * math.cos(a) - vv * math.sin(a), 1), round(cz_m + uu * math.sin(a) + vv * math.cos(a), 1)]
        yaw = round(math.degrees(a), 1)

        if kind == "waterfall":
            # o platô atrás do paredão (u < 0), o lago na frente, a rampa pelo lado até o topo
            top = b + 38
            # a borda irregular (o ruído mexe o contorno até ~25 m), não um retângulo
            inside = smoothstep(0, 1, np.clip(1 - np.maximum(np.abs(v) - 70 - 22 * rough, 0) / 45, 0, 1)) * smoothstep(-215, -170, u + 30 * rough)
            face = 1 - smoothstep(0, 7, u + 8 * np.abs(rough) * (np.abs(v) > 12))   # o paredão: cai em 1 ponto do mapa
            plateau = np.minimum(inside, face)
            T = b + (top - b) * plateau + rough * 2.5 * plateau
            w = np.maximum(w, smoothstep(0, .35, plateau + (r < 260) * .0))
            w = np.where(r < 260, np.maximum(w, smoothstep(260, 200, r)), w)
            lake = ((u - 48) / 50) ** 2 + (v / 44) ** 2
            T = np.where(lake < 1.35, np.minimum(T, WL - 3 + (WL + 1.2 - (WL - 3)) * smoothstep(.75, 1.35, lake)), T)
            ramp = (np.abs(v - 98) < 7) & (u > -150) & (u < 40)
            T = np.where(ramp, b + (top - b) * np.clip((40 - u) / 190, 0, 1), T)
            w = np.where(ramp | (lake < 1.4), 1, w)
            path |= ramp | ((np.abs(v - 98) < 4) & (u >= -175) & (u <= -150)) | ((np.abs(u + 40) < 3) & (v > 0) & (v < 98))
            props += [{"prop": "waterfall", "at": L(1, 0), "yaw": yaw, "top": round(top, 1), "bottom": WL},
                      {"prop": "lookout", "at": L(-28, -40), "yaw": yaw}]
            entry = L(48, 98)
        elif kind == "stonering":
            top = b + 44
            t = np.clip(1 - r / 150, 0, 1) ** 1.15 * 44
            s = t / 11
            step = 11 * (np.floor(s) + smoothstep(.72, 1, s - np.floor(s)))
            T = b + np.where(r < 28, 44, step) + rough * 1.2 * (r > 28)
            w = smoothstep(175, 140, r)
            trail = (np.abs(v) < 5) & (u > 24) & (u < 160)
            T = np.where(trail, b + 44 * np.clip((160 - u) / 136, 0, 1) ** 1.05, T)
            path |= trail
            props += [{"prop": "stonering", "at": L(0, 0), "yaw": yaw}]
            entry = L(165, 0)
        elif kind == "arch":
            along = np.clip(1 - (np.abs(v) / 145) ** 3, 0, 1)
            across = np.clip(1 - (np.abs(u) / 42) ** 2, 0, 1)
            ridge = along * across
            hgt = 34 + rough * 6 + np.maximum(crag, 0) * 9
            T = b + hgt * ridge
            gap = np.abs(v) < 11
            T = np.where(gap, np.minimum(T, b + 1.5 + np.abs(v) * 0.2), T)
            w = smoothstep(.02, .25, ridge) + 0
            w = np.where(gap & (np.abs(u) < 60), 1, w)
            path |= (np.abs(v) < 4) & (np.abs(u) < 70)
            props += [{"prop": "arch", "at": L(0, 0), "yaw": yaw, "top": round(b + 28, 1)}]
            entry = L(75, 0)
        elif kind == "lake":
            e = (u / 125) ** 2 + (v / 90) ** 2
            T = np.where(e < 1.25, WL - 3 + (b - (WL - 3)) * smoothstep(.8, 1.25, e), b)
            island = r < 30
            T = np.where(island, WL + 5.5 - (r / 30) ** 2 * 3.5, T)
            stones = np.zeros((S, S), bool)
            for k in range(6):
                su = 34 + k * 9
                stones |= np.hypot(u - su, v) < 3.4
            T = np.where(stones, WL + .6, T)
            w = smoothstep(1.45, 1.15, e)
            path |= stones
            props += [{"prop": "tower", "at": L(0, 0), "yaw": yaw}]
            entry = L(132, 0)
        elif kind == "rootbridge":
            along = smoothstep(125, 95, np.abs(v))
            depth = np.where(np.abs(u) < 7, WL - 1.4, b - 26 + 30 * smoothstep(7, 19, np.abs(u)) + crag * 3)
            shoulder = b + 5 * smoothstep(40, 19, np.abs(u)) * smoothstep(19, 22, np.abs(u))
            T = np.where(np.abs(u) < 20, depth, shoulder)
            T = b + (T - b) * along
            w = smoothstep(60, 35, np.abs(u)) * smoothstep(150, 110, np.abs(v))
            props += [{"prop": "rootbridge", "at": L(0, 0), "yaw": yaw, "top": round(b + 4, 1)}]
            path |= (np.abs(v) < 4) & (np.abs(u) > 22) & (np.abs(u) < 60)
            entry = L(62, 0)
        elif kind == "crystals":
            rim = smoothstep(52, 74, r) * smoothstep(112, 76, r)
            T = b - 3 + 17 * rim + np.maximum(crag, 0) * 6 * rim
            T = np.where(r < 52, b - 3 + rough, T)
            notch = (np.abs(v) < 7) & (u > 40) & (u < 115)
            T = np.where(notch, b - 2 + np.clip((u - 40) / 75, 0, 1) * 2, T)
            w = smoothstep(125, 100, r)
            path |= notch
            props += [{"prop": "crystals", "at": L(0, 0), "yaw": yaw}]
            for k in range(7):
                ang = k / 7 * 2 * math.pi + .4
                rr = 24 + (k % 3) * 9
                props.append({"prop": "crystal_small", "at": L(rr * math.cos(ang), rr * math.sin(ang)), "yaw": round(k * 47.0, 1)})
            entry = L(120, 0)
        w = np.clip(w, 0, 1)
        height = height * (1 - w) + T * w
        flags[(w > .5) & (flags == 1)] = 0
        flags[path & (height > WL)] = PATH_FLAG
        marvels.append({"id": mid, "name": name, "kind": kind, "at": [cx_m, cz_m], "yaw": yaw, "base": round(b, 1), "props": props})
        entries.append((mid, entry))
        print("%-9s base %.1f m  área %d px" % (mid, b, int((w > .5).sum())))

    # ---- as raízes da Árvore do Mundo pelo continente (o usuário: "essas raízes espalhadas pelo mapa todo"; o PDF:
    # "em lugares selecionados, com função visual": atravessando um rio estreito como ponte natural, ou saindo do chão
    # numa clareira ou encosta). Longe da cidade, dos lugares, do mar, da lava e uma da outra
    rng = np.random.default_rng(2026)
    river = (height < WL) & (region > 0)
    landish = (height > WL + .6) & (region > 0) & (flags == 0)
    near_water = ndimage.distance_transform_edt(~river) * MPP
    taken = [(m["at"][0], m["at"][1]) for m in marvels] + [(0.0, 0.0)]
    roots = []
    def free(x, z, gap):
        return math.hypot(x, z) > 650 and all(math.hypot(x - tx, z - tz) > gap for tx, tz in taken)
    # pontes: o meio de um rio estreito (até ~20 m), a raiz atravessando
    cand = np.argwhere(river & (ndimage.distance_transform_edt(river) <= 2) & (ndimage.distance_transform_edt(river) >= 1))
    rng.shuffle(cand)
    for iy, ix in cand:
        if sum(1 for q in roots if q["kind"] == "root_arch") >= 14:
            break
        if not free(*world(ix, iy), 420):
            continue
        win = np.argwhere(river[max(0, iy - 4):iy + 5, max(0, ix - 4):ix + 5])
        if len(win) < 5:
            continue
        cc = win - win.mean(axis=0)
        _, vec = np.linalg.eigh(cc.T @ cc)
        dy, dx = vec[:, 1]
        # as duas margens no sentido de atravessar (-dy, dx): as duas a no máximo 5 pontos (30 m) uma da outra; a raiz
        # fica no meio (07/11: sem isso ela caía na beira de lagos largos)
        def bank(sign):
            for k in range(1, 6):
                jx, jy = int(round(ix - dy * k * sign)), int(round(iy + dx * k * sign))
                if not (0 <= jx < S and 0 <= jy < S) or region[jy, jx] == 0:
                    return None
                if height[jy, jx] > WL + .3:
                    return k
            return None
        ka, kb = bank(1), bank(-1)
        if ka is None or kb is None or ka + kb > 5:
            continue
        mid = (ka - kb) / 2
        x, z = world(ix - dy * mid, iy + dx * mid)
        if not free(x, z, 420):
            continue
        yaw = math.degrees(math.atan2(dx, -dy))          # atravessa o rio
        roots.append({"kind": "root_arch", "at": [x, z], "yaw": round(yaw, 1), "scale": round(float(rng.uniform(.8, 1.2)), 2)})
        taken.append((x, z))
    # as que saem do chão: em todas as regiões, algumas por região
    for reg in range(1, 10):
        cand = np.argwhere(landish & (region == reg) & (near_water > 40))
        rng.shuffle(cand)
        made = 0
        for iy, ix in cand:
            if made >= 4:
                break
            x, z = world(ix, iy)
            if not free(x, z, 380):
                continue
            roots.append({"kind": "root_emerge", "at": [x, z], "yaw": round(float(rng.uniform(0, 360)), 1),
                          "scale": round(float(rng.uniform(.8, 1.4)), 2), "variant": int(rng.integers(0, 3))})
            taken.append((x, z))
            made += 1
    print("raízes: %d (pontes %d)" % (len(roots), sum(1 for q in roots if q["kind"] == "root_arch")))

    # ---- os caminhos: da borda da cidade (fora do recorte exportado) até a entrada de cada lugar
    water = height < WL + .2
    slope = np.hypot(*np.gradient(height)) / MPP
    def start_for(ex, ez):
        d = math.hypot(ex, ez)
        dx, dz = ex / d, ez / d
        for s in range(300, 1400, 10):
            x, z = dx * s, dz * s
            if not (EXPORTED[0] - 20 < x < EXPORTED[1] + 20 and EXPORTED[2] - 20 < z < EXPORTED[3] + 20):
                return x, z
        return dx * 600, dz * 600
    roads = []
    for mid, (ex, ez) in entries:
        sx, sz = start_for(ex, ez)
        a0 = tuple(int(round(c)) for c in px(sx, sz))
        a1 = tuple(int(round(c)) for c in px(ex, ez))
        x0, x1 = sorted((a0[0], a1[0]))
        y0, y1 = sorted((a0[1], a1[1]))
        x0, y0, x1, y1 = max(0, x0 - 60), max(0, y0 - 60), min(S - 1, x1 + 60), min(S - 1, y1 + 60)
        best = {a0: 0.0}
        prev = {}
        heap = [(0.0, 0.0, a0)]
        while heap:
            _, c, p = heapq.heappop(heap)
            if p == a1:
                break
            if c > best.get(p, 1e18):
                continue
            for dx_ in (-1, 0, 1):
                for dy_ in (-1, 0, 1):
                    if not (dx_ or dy_):
                        continue
                    q = (p[0] + dx_, p[1] + dy_)
                    if not (x0 <= q[0] <= x1 and y0 <= q[1] <= y1):
                        continue
                    step = math.hypot(dx_, dy_)
                    dh = abs(height[q[1], q[0]] - height[p[1], p[0]]) / (step * MPP)
                    cost = step * (1 + 25 * dh + (60 if dh > .45 else 0) + (400 if water[q[1], q[0]] else 0))
                    nc = c + cost
                    if nc < best.get(q, 1e18):
                        best[q] = nc
                        prev[q] = p
                        heapq.heappush(heap, (nc + math.hypot(q[0] - a1[0], q[1] - a1[1]), nc, q))
        route = [a1]
        while route[-1] in prev:
            route.append(prev[route[-1]])
        route.reverse()
        # alisa a altura ao longo do caminho (média de 7 pontos) e marca o caminho
        hs = np.array([height[y, x] for x, y in route], np.float32)
        if len(hs) > 7:
            k = np.ones(7) / 7
            hs = np.convolve(np.pad(hs, 3, mode="edge"), k, mode="valid")
        for (x, y), hv in zip(route, hs):
            if height[y, x] > WL:
                height[y, x] = max(WL + .3, hv)
                if flags[y, x] == 0:
                    flags[y, x] = PATH_FLAG
        roads.append({"to": mid, "points": [world(x, y) for x, y in route[::4]] + [world(*route[-1])]})
        print("caminho até %-9s %d pontos (%.0f m)" % (mid, len(route), len(route) * MPP))

    height.astype(np.float16).tofile(os.path.join(DATA, "world_height.bin"))
    flags.tofile(os.path.join(DATA, "world_flags.bin"))
    feat_path = os.path.join(DATA, "world_features.json")
    feats = json.load(open(feat_path, encoding="utf-8"))
    feats["marvels"] = marvels
    feats["roads"] = roads
    feats["roots"] = roots
    json.dump(feats, open(feat_path, "w", encoding="utf-8"), ensure_ascii=False, indent=1)

    # prévia: a área em volta da cidade com os lugares e os caminhos
    os.makedirs(PREVIEW, exist_ok=True)
    c = slice(HV - 260, HV + 260), slice(HU - 260, HU + 260)
    hh = height[c]
    img = np.stack([np.clip(hh * 3 + 40, 0, 255)] * 3, -1).astype(np.uint8)
    img[..., 1] = np.clip(hh * 3 + 70, 0, 255)
    img[hh < WL] = (40, 90, 170)
    img[flags[c] == PATH_FLAG] = (200, 150, 90)
    Image.fromarray(img).resize((1040, 1040), Image.NEAREST).save(os.path.join(PREVIEW, "previa_lugares.png"))
    print("ok: %d lugares, %d caminhos" % (len(marvels), len(roads)))


if __name__ == "__main__":
    main()
