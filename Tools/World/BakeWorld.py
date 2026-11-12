"""O continente do Vortex: a planta do mundo do Godot (data/world/_src/bake_world.py) com rios
completos, vales e montanhas mais bonitas. Lê a arte do Godot (só leitura) e grava em
Data/GodotWorld (o que o terreno da Unreal lê, FVortexTerrainData::LoadWorld).

Mudanças em relação ao Godot:
  - rios: a rede de drenagem é a mesma (priority flood + D8 + acúmulo), mas o riacho começa com
    uma bacia ~3x menor (nasce perto das cabeceiras, nas montanhas) e engrossa com os afluentes
    (o leito vai de 3 a 9 m de meia largura conforme a água que passa);
  - vales: em volta de cada rio o chão desce em U largo (22% até 70 m do rio) antes da encosta
    mais íngreme; nas montanhas isso vira cânion;
  - montanhas: cristas finas por cima das grandes em Raikor, Nivaluz e Brasafenda.

    python Tools/World/BakeWorld.py

---- texto original do Godot ----
Planta do mundo a partir do mapa conceitual (mapa_conceito.png, a arte do continente).

A imagem vira os dados que o jogo lê (Game/World/WorldMap.cs), 1 pixel = 6 m (o continente tem
~7 km de ponta a ponta):
  ../world_height.bin   float16  altura do terreno em metros (relevo de cada região, costa, rios)
  ../world_region.bin   uint8    região de cada ponto (0 = mar; 1..9 abaixo)
  ../world_tint.bin     uint8×3  tom do chão, misturado nas bordas entre regiões
  ../world_flags.bin    uint8    1 = lava (Brasafenda), 2 = gelo (lagos e rios congelados de Nivaluz)
  ../world_meta.json            tamanho, escala, onde fica a vila (0,0) na imagem e as regiões
  previa_*.png                  para conferir

Terra, mar, rios e lagos saem das cores da própria arte (azul = água). As regiões usam os
rótulos da arte como centros, puxados pelas cores (lava, areia, neve, roxo...), então as bordas
seguem o desenho. O relevo de cada região é gerado aqui (montanhas de Raikor, o vulcão, as
mesas de Solcânion, o pântano...), e as bordas se misturam suave.

    python data/world/_src/bake_world.py
"""
import json
import math
import os
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HERE = os.path.normpath(os.path.join(ROOT, "..", "..", "the-world-geneses", "data", "world", "_src"))   # a arte (só leitura)
OUT = os.path.join(ROOT, "Data", "GodotWorld")
PREVIEW = os.path.join(ROOT, "Saved", "WorldBake")
os.makedirs(PREVIEW, exist_ok=True)
MPP = 6.0                                   # metros por pixel
WATER_LEVEL = 2.05                          # mesma linha d'água do lago (ValleyWorld.WaterLevel)
HOME = (560, 640)                           # pixel onde fica a vila (mundo 0, 0), nas Planícies de Valdora

# id, nome, centro (pixel da arte), cor típica na arte, tom do chão no jogo
REGIONS = [
    (1, "Planícies de Valdora", (640, 590), (150, 185, 70), (1.00, 1.00, 1.00)),
    (2, "Brasafenda", (315, 190), (70, 50, 48), (.62, .55, .55)),
    (3, "Picos de Raikor", (690, 215), (115, 125, 130), (.9, .95, .98)),
    (4, "Nivaluz", (1060, 190), (225, 235, 245), (1.00, 1.00, 1.00)),
    (5, "Solcânion", (215, 470), (215, 145, 90), (1.00, .96, .92)),
    (6, "Aurivale", (1000, 430), (220, 185, 75), (1.02, 1.00, .92)),
    (7, "Lodonegro", (230, 800), (55, 135, 105), (.85, .95, .9)),
    (8, "Morthúria", (640, 925), (125, 100, 155), (.95, .9, 1.02)),
    (9, "Verdalume", (1040, 760), (45, 140, 55), (1.00, 1.02, .98)),
]
VOLCANO = (360, 85)                         # o cone fumegante de Brasafenda
YGGDRASIL = (1045, 705)                     # a árvore do mundo em Verdalume
# Lugares construídos pelo jogo em cima do terreno (mundo, metros): ficam planos, sem morro nem rio
# (vila e cidade inicial, vilas regionais, covis dos chefes regionais).
FLAT_SITES = [((0, 0), 700, 1100), ((-2188, -1006), 160, 320), ((2870, 614), 160, 320),
              ((-2180, -1090), 120, 260), ((2960, 705), 120, 260)]


def smooth(x, sigma):
    return ndimage.gaussian_filter(x.astype(np.float32), sigma)


def fbm(shape, seed, scale, octaves=4):
    """Ruído fractal (0..1) com a feição principal de `scale` pixels."""
    rng = np.random.default_rng(seed)
    out = np.zeros(shape, np.float32)
    amp, total = 1.0, 0.0
    for o in range(octaves):
        cells = max(2, int(shape[0] / scale * 2 ** o) + 2)
        grid = rng.random((cells, cells)).astype(np.float32)
        layer = ndimage.zoom(grid, shape[0] / (cells - 1), order=3)[:shape[0], :shape[1]]
        out += layer * amp
        total += amp
        amp *= .5
    out /= total
    return (out - out.min()) / (out.max() - out.min() + 1e-9)


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def terrace(h, step, steep=.3):
    """Degraus de `step` m: o patamar plano em (1 - steep) e a subida no resto (as mesas e os basaltos)."""
    k = h / step
    f = k - np.floor(k)
    return (np.floor(k) + smoothstep(1 - steep, 1, f)) * step


def hillshade(h):
    """Relevo sombreado (luz do noroeste) para as prévias."""
    gy, gx = np.gradient(h.astype(np.float32), MPP)
    n = np.dstack([-gx, -gy, np.ones_like(gx)])
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    light = np.array([-.5, -.5, .7]) / np.linalg.norm([-.5, -.5, .7])
    return np.clip((n @ light) * 255, 0, 255).astype(np.uint8)


def main():
    art = np.asarray(Image.open(os.path.join(HERE, "mapa_conceito.png")).convert("RGB")).astype(np.float32)
    H, W = art.shape[:2]
    r, g, b = art[..., 0], art[..., 1], art[..., 2]
    yy, xx = np.mgrid[0:H, 0:W]

    # ---- água: azul saturado (mar, rios, lagos); nuvens dos cantos contam como mar
    # azul de água tem o verde acima do vermelho (o roxo de Morthúria e as sombras azuladas não)
    water = (b > r + 60) & (g > r + 15) & (b > g) & (r < 130)          # gelo é claro (vermelho alto)
    cloudy = (art.min(axis=2) > 150) & (art.max(axis=2) - art.min(axis=2) < 70)
    far = np.hypot(xx - W / 2, yy - H / 2) > 520
    water |= cloudy & far
    land = ~water
    lab, n = ndimage.label(land)
    sizes = ndimage.sum(land, lab, range(1, n + 1))
    land = np.isin(lab, np.nonzero(sizes >= 40)[0] + 1)                 # sem cisquinhos de terra
    lab, n = ndimage.label(~land)
    sizes = ndimage.sum(~land, lab, range(1, n + 1))
    small_water = np.isin(lab, np.nonzero(sizes < 90)[0] + 1)          # poças e as letras dos rótulos
    land |= small_water                                                   # sem poças de 1 pixel
    home_zone = np.hypot(xx - HOME[0], yy - HOME[1]) < 75                 # a vila e o vale atual: terra firme
    land |= home_zone
    water = ~land

    # ---- regiões: perto do rótulo + parecido com a cor típica (arte suavizada)
    soft = np.stack([smooth(art[..., k], 5) for k in range(3)], axis=-1)
    scores = []
    for rid, name, (cx, cy), col, _ in REGIONS:
        dist = np.hypot(xx - cx, yy - cy) / 330.0
        color = np.linalg.norm(soft - np.array(col, np.float32), axis=-1) / 160.0
        scores.append(dist + .55 * color)
    region = np.argmin(np.stack(scores), axis=0).astype(np.uint8) + 1
    # bordas limpas: maioria numa janela e depois a vila garantida em Valdora
    onehot = np.stack([smooth(region == k, 6) for k in range(1, 10)])
    region = (np.argmax(onehot, axis=0) + 1).astype(np.uint8)
    region[home_zone] = 1
    weights = np.stack([smooth(region == k, 9) for k in range(1, 10)])
    weights /= weights.sum(axis=0, keepdims=True) + 1e-9

    # ---- relevo de cada região (metros), misturado pelos pesos
    # 07/11 (o usuário: "os relevos estão bem simples, não está igual às imagens"): cada região com a sua cara, as
    # feições torcidas por um ruído largo (domain warp: nada de bolhas redondas nem linhas retas)
    wx = (fbm((H, W), 101, 70) - .5) * 2 * 14
    wy = (fbm((H, W), 102, 70) - .5) * 2 * 14

    def warped(f, k=1.0):
        return ndimage.map_coordinates(f, [yy + wy * k, xx + wx * k], order=1, mode="nearest").astype(np.float32)

    def ridges(seed, scale, octaves=4, sharp=2.0):
        """Cristas (ridged multifractal): cada oitava mais fina pesa onde a anterior já é crista."""
        out = np.zeros((H, W), np.float32)
        weight, amp, total = np.ones((H, W), np.float32), 1.0, 0.0
        for o in range(octaves):
            sig = (1 - np.abs(fbm((H, W), seed + o, scale / 2 ** o, 1) * 2 - 1)) ** sharp
            out += sig * weight * amp
            total += amp
            weight = np.clip(sig * 1.6, 0, 1)
            amp *= .5
        return warped(out / total)

    n1, n2, n3 = fbm((H, W), 1, 60), fbm((H, W), 2, 25), fbm((H, W), 3, 120)
    ridged = 1 - np.abs(fbm((H, W), 4, 45) * 2 - 1)
    fine = (1 - np.abs(fbm((H, W), 31, 14) * 2 - 1)) ** 2               # Vortex: cristas finas por cima
    alpine = ridges(41, 90, 5, 2.2)                                       # Raikor: picos e arestas
    alpine = (alpine - alpine.min()) / (alpine.max() - alpine.min())      # 0..1 (o multifractal fica baixo)
    basalt = ridges(51, 55, 4, 1.6)
    mesa = warped(fbm((H, W), 5, 38))
    butte = warped(fbm((H, W), 52, 11), .5)
    glacier = warped(fbm((H, W), 61, 85) ** 1.4)
    spires = fbm((H, W), 81, 7, 2)
    heights = {
        # Valdora: campos ondulados e alguns platôs de borda de pedra
        1: 5 + 7 * n1 + 2 * n2 + smoothstep(.62, .67, warped(fbm((H, W), 111, 75))) * (9 + 7 * n2),
        # Brasafenda: basalto em degraus e arestas quebradas
        2: 12 + terrace(34 * basalt ** 1.4 + 8 * n2, 7, .35) + 10 * fine * basalt,
        # Raikor: picos altos e arestas afiadas (até ~240 m)
        3: 28 + 200 * alpine ** 1.6 + 18 * n3 + 30 * fine * alpine,
        # Nivaluz: colinas largas e redondas (gelo antigo) e alguns picos
        4: 12 + 46 * glacier + 60 * smoothstep(.6, .9, alpine) * alpine + 10 * fine * glacier,
        # Solcânion: mesas de paredão reto em camadas, morros-testemunho e o chão de areia
        5: 7 + 3 * n2 + terrace(smoothstep(.5, .58, mesa) * (28 + 20 * n3), 6, .22)
           + terrace(smoothstep(.78, .84, butte) * (18 + 14 * n1), 6, .25),
        # Aurivale: pradaria de colinas suaves e compridas
        6: 4 + 3 * n1 + 9 * warped(fbm((H, W), 62, 32), 1.5) ** 1.6,
        # Lodonegro: pântano baixo, com montinhos e ilhas
        7: 2.25 + 1.6 * (n2 - .45) + 2.6 * smoothstep(.62, .78, fbm((H, W), 71, 10)),
        # Morthúria: arestas, agulhas de pedra e dolinas
        8: 8 + 18 * ridged + 4 * n2 + 30 * smoothstep(.74, .84, spires) - 6 * smoothstep(.76, .86, fbm((H, W), 82, 20)),
        # Verdalume: morros grandes e redondos de floresta
        9: 7 + 9 * n1 + 3 * n2 + 20 * warped(fbm((H, W), 91, 42)) ** 2.2,
    }
    height = np.zeros((H, W), np.float32)
    for k in range(1, 10):
        height += weights[k - 1] * heights[k]

    # ---- morros de verdade nas planícies e florestas (antes o relevo variava só ~7 m e as
    # subidas pareciam degrauzinhos): colinas largas de até ~35 m e cristas, longe dos lugares
    # construídos, que continuam planos
    def to_px(wx, wz):
        return wx / MPP + HOME[0], wz / MPP + HOME[1]
    protect = np.ones((H, W), np.float32)
    for (wx, wz), r0, r1 in FLAT_SITES:
        px, py = to_px(wx, wz)
        dm = np.hypot(xx - px, yy - py) * MPP
        protect = np.minimum(protect, np.clip((dm - r0) / (r1 - r0), 0, 1))
    protect = np.minimum(protect, np.clip((np.hypot(xx - YGGDRASIL[0], yy - YGGDRASIL[1]) - 40) / 40, 0, 1))
    hills = fbm((H, W), 11, 45) ** 1.35 * 30 + (1 - np.abs(fbm((H, W), 12, 30) * 2 - 1)) ** 2 * 9
    hilly = weights[0] * 1.0 + weights[5] * .8 + weights[8] * 1.0 + weights[3] * .6 + weights[7] * .5
    height += hills * hilly * protect
    vd = np.hypot(xx - VOLCANO[0], yy - VOLCANO[1])
    cone = np.clip(1 - vd / 125, 0, 1) ** 1.6 * 175
    crater = np.clip(1 - vd / 16, 0, 1) * 45
    height += cone - crater
    yd = np.hypot(xx - YGGDRASIL[0], yy - YGGDRASIL[1])
    height = np.where(yd < 30, height * (yd / 30) + 8 * (1 - yd / 30), height)   # clareira plana da Yggdrasil

    # ---- costa, rios e fundo do mar
    shore = ndimage.distance_transform_edt(land) * MPP                    # metros até a água

    # ---- rios em rede: preenche as depressões (priority flood) e calcula para onde a água de cada
    # ponto escorre (D8); a soma do que passa por cada ponto (acúmulo) desenha rios que nascem nas
    # partes altas, recebem afluentes, engrossam e sempre chegam ao mar. Os lugares construídos
    # viram "morros" só para esta conta, então a água passa em volta deles.
    import heapq
    flow_h = smooth(np.where(land, height, WATER_LEVEL - 2), 3) + fbm((H, W), 22, 10) * 5 + fbm((H, W), 23, 30) * 9 + (1 - protect) * 400
    filled = np.full((H, W), np.inf, np.float32)
    down = np.full((H, W), -1, np.int64)
    heap, order = [], []
    edge = water & (ndimage.binary_dilation(land) )
    for iy, ix in np.argwhere(edge):
        filled[iy, ix] = flow_h[iy, ix]
        heapq.heappush(heap, (float(flow_h[iy, ix]), int(iy * W + ix)))
    nbr = [(-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (-1, 1), (1, -1), (1, 1)]
    done = np.zeros(H * W, bool)
    flat_land = land.ravel()
    fh = flow_h.ravel(); fl = filled.ravel(); dn = down.ravel()
    while heap:
        hcur, idx = heapq.heappop(heap)
        if done[idx]:
            continue
        done[idx] = True
        order.append(idx)
        cy, cx = divmod(idx, W)
        for dy, dx in nbr:
            ny_, nx_ = cy + dy, cx + dx
            if not (0 <= ny_ < H and 0 <= nx_ < W):
                continue
            j = ny_ * W + nx_
            if done[j] or not flat_land[j]:
                continue
            val = max(float(fh[j]), hcur + 1e-3)
            if val < fl[j]:
                fl[j] = val
                dn[j] = idx
                heapq.heappush(heap, (val, j))
    acc = np.ones(H * W, np.float32)
    for idx in reversed(order):
        d = dn[idx]
        if d >= 0:
            acc[d] += acc[idx]
    acc = acc.reshape(H, W)
    T = 1400.0                                         # Vortex: riacho já com bacia ~3x menor (o Godot: 3800)
    # traçado suave: o D8 anda em linhas retas de 45°; borrar e cortar arredonda as curvas e emenda
    # 07/11: e serpenteia (o traçado é torcido por dois ruídos: curvas largas de ~60 px, até 7 px para cada lado, e
    # voltas menores de ~18 px; o D8 sozinho anda em retas de 45°)
    mx = (fbm((H, W), 121, 60, 2) - .5) * 2 * 7 + (fbm((H, W), 123, 18, 1) - .5) * 2 * 2.5
    my = (fbm((H, W), 122, 60, 2) - .5) * 2 * 7 + (fbm((H, W), 124, 18, 1) - .5) * 2 * 2.5

    # torce a distância até o rio (não a linha): o traçado torcido nunca se parte
    def meander(f):
        return ndimage.map_coordinates(f, [yy + my, xx + mx], order=1, mode="nearest")
    river = (meander(ndimage.distance_transform_edt(~(smooth(ndimage.binary_dilation(acc > T, iterations=1).astype(np.float32), 1.4) > .42))) < 1.0) & land & (protect > .5)
    big = (meander(ndimage.distance_transform_edt(~(smooth(ndimage.binary_dilation(acc > T * 12, iterations=1).astype(np.float32), 1.4) > .42))) < 1.0) & land
    huge = (meander(ndimage.distance_transform_edt(~(smooth(ndimage.binary_dilation(acc > T * 60, iterations=2).astype(np.float32), 1.6) > .42))) < 1.0) & land
    made = int(ndimage.label(river)[1])
    rd = ndimage.distance_transform_edt(~river) * MPP                     # metros até o meio do rio
    rd = np.minimum(rd, ndimage.distance_transform_edt(~big) * MPP - 3)   # rios grandes: mais largos
    rd = np.minimum(rd, ndimage.distance_transform_edt(~huge) * MPP - 6)  # Vortex: os maiores, mais ainda
    bed_r = WATER_LEVEL - 1.7
    # Vortex: vale em U largo (22% até 70 m do rio) e depois a encosta (90%); nas montanhas, cânion
    # 07/11: a largura do vale muda ao longo do rio (35 a 95 m) e a borda é torta e arredondada (antes todo vale tinha
    # 70 m e uma quina: de cima pareciam pílulas)
    vw = 35 + 60 * fbm((H, W), 131, 50)
    rdn = rd + (fbm((H, W), 132, 14) - .5) * 2 * 12
    x = np.maximum(rdn - 3, 0)
    carve = bed_r + .2 * x + .7 * 10 * np.logaddexp(0, (x - vw) / 10)     # fundo a 20%, encosta a 90%, sem quina
    carve = np.where(rd < 3, bed_r, carve)
    carve = np.where(protect < 1, np.maximum(carve, height * (1 - protect) + carve * protect), carve)   # lugares construídos: sem vale
    height = np.where(land, np.minimum(height, carve), height)
    # 07/11: a erosão: os córregos pequenos (o mesmo acúmulo, abaixo do tamanho de rio) cavam sulcos em V que descem
    # das montanhas até os rios; mais fundos onde o morro é alto (até ~20% da altura), nada nos lugares construídos
    small = np.clip(np.log(np.maximum(acc, 1) / 25) / np.log(T / 25), 0, 1) * (acc > 25)
    gully = meander(smooth(small.astype(np.float32), .9)) * protect
    tall = smoothstep(12, 60, height)
    eroded = height - gully * tall * (2 + .2 * np.maximum(height - WATER_LEVEL, 0))
    # o sulco não vira poça: para 1 m acima da água (ou no chão, se já for mais baixo)
    height = np.where(land, np.maximum(eroded, np.minimum(height, WATER_LEVEL + 1.0)), height)
    land_r = land & (height >= WATER_LEVEL - .05)                          # o rio vira água
    river_water = land & ~land_r & (rd < 12)          # só a faixa do rio (os pântanos baixos não)
    land_r = land_r | (land & ~land_r & (rd >= 9))
    land = land_r
    water = ~land
    shore = ndimage.distance_transform_edt(land) * MPP
    depth = ndimage.distance_transform_edt(water) * MPP                   # metros até a terra
    beach = WATER_LEVEL + .35
    land_h = beach + (height - beach) * np.clip(shore / 70.0, 0, 1) ** .8
    bed = WATER_LEVEL - np.minimum(6.0, .8 + depth * .08)
    bed = np.where(river_water, np.minimum(height, WATER_LEVEL - .6), bed)   # rio: leito raso
    height = np.where(land, np.where(rd < 120, height, land_h), bed)

    # ---- lava (laranja vivo dentro de Brasafenda) e gelo (água de Nivaluz congela)
    flags = np.zeros((H, W), np.uint8)
    lava = (region == 2) & land & (r > 190) & (g > 50) & (g < 175) & (b < 90)
    lava = ndimage.binary_opening(lava, iterations=1)
    lava = ndimage.binary_dilation(lava, iterations=1) & land
    flags[lava] = 1
    ice_region = smooth(region == 4, 3) > .5
    ice = water & ice_region & (depth < 160)                              # canais entre os blocos de gelo congelam
    flags[ice] = 2
    height = np.where(ice, WATER_LEVEL + .15, height)                    # gelo: dá para andar por cima
    region = np.where(water & ~ice & ~river_water, 0, region).astype(np.uint8)   # rio fica na região dele

    # ---- tom do chão
    tint = np.zeros((H, W, 3), np.float32)
    for k, (_, _, _, _, t) in enumerate(REGIONS):
        tint += weights[k][..., None] * np.array(t, np.float32)
    tint8 = np.clip(tint * 200, 0, 255).astype(np.uint8)                  # 200 = 1.0

    height.astype(np.float16).tofile(os.path.join(OUT, "world_height.bin"))
    region.tofile(os.path.join(OUT, "world_region.bin"))
    tint8.tofile(os.path.join(OUT, "world_tint.bin"))
    flags.tofile(os.path.join(OUT, "world_flags.bin"))
    # 07/11: o mapa sem os lugares marcantes (Tools/World/StampMarvels.py carimba a partir destas cópias, no fim)
    height.astype(np.float16).tofile(os.path.join(OUT, "world_height_base.bin"))
    flags.tofile(os.path.join(OUT, "world_flags_base.bin"))

    def world(px, py):
        return [round((px - HOME[0]) * MPP, 1), round((py - HOME[1]) * MPP, 1)]

    # ---- pontes e cabanas abandonadas (o jogo constrói quando o pedaço carrega)
    frng = np.random.default_rng(57)
    bridges = []
    centre = river & (ndimage.distance_transform_edt(river) >= 1)
    pts = np.argwhere(centre & (protect > .99))
    frng.shuffle(pts)
    for iy, ix in pts:
        if len(bridges) >= 60:
            break
        if any(np.hypot(ix - b[0], iy - b[1]) < 55 for b in bridges):           # ~330 m entre pontes
            continue
        # direção do rio: maior eixo dos pixels de rio em volta; a ponte atravessa
        win = np.argwhere(river[max(0, iy - 5):iy + 6, max(0, ix - 5):ix + 6])
        if len(win) < 6:
            continue
        c = win - win.mean(axis=0)
        w, v = np.linalg.eigh(c.T @ c)
        dy, dx = v[:, 1]                                                      # ao longo do rio
        across = math.atan2(dx, dy)                                           # giro (y) da ponte no jogo
        # largura da água nesse ponto (m)
        width = 0
        for k in range(1, 12):
            qy, qx = int(round(iy + dx * k)), int(round(ix - dy * k))
            ry, rx_ = int(round(iy - dx * k)), int(round(ix + dy * k))
            inside = lambda y, x: 0 <= y < H and 0 <= x < W and height[y, x] < WATER_LEVEL
            if not (inside(qy, qx) or inside(ry, rx_)):
                break
            width = k
        if width == 0 or width > 6:
            continue
        bridges.append((ix, iy, across, (width * 2 + 2) * MPP))
    cabins = []
    near_river = (rd > 22) & (rd < 70)
    cand = np.argwhere(land & (protect > .99) & (region != 2) & ~ice & (height > WATER_LEVEL + 1) & (height < 60))
    frng.shuffle(cand)
    for iy, ix in cand:
        if len(cabins) >= 70:
            break
        if not near_river[iy, ix] and frng.random() < .6:                     # a maioria perto dos rios
            continue
        if any(np.hypot(ix - c[0], iy - c[1]) < 45 for c in cabins):
            continue
        patch_ = height[max(0, iy - 2):iy + 3, max(0, ix - 2):ix + 3]
        if patch_.max() - patch_.min() > 4:                                    # terreno quase plano
            continue
        cabins.append((ix, iy, float(frng.random() * 6.283), int(region[iy, ix])))
    features = {
        "_doc": "Gerado por bake_world.py: pontes sobre os rios (centro, giro, comprimento em m) e cabanas abandonadas (centro, giro, região).",
        "bridges": [{"at": world(x, y), "yaw": round(a, 3), "length": round(l, 1)} for x, y, a, l in bridges],
        "cabins": [{"at": world(x, y), "yaw": round(a, 3), "region": r} for x, y, a, r in cabins],
    }
    json.dump(features, open(os.path.join(OUT, "world_features.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    print("pontes", len(bridges), "cabanas", len(cabins))
    meta = {
        "size": W, "meters_per_pixel": MPP, "home_pixel": list(HOME), "water_level": WATER_LEVEL,
        "extent": round(max(W, H) * MPP / 2 + 60),
        "regions": [{"id": rid, "name": name, "center": world(*c)} for rid, name, c, _, _ in REGIONS],
        "volcano": world(*VOLCANO), "yggdrasil": world(*YGGDRASIL),
    }
    json.dump(meta, open(os.path.join(OUT, "world_meta.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)

    # ---- prévias
    palette = np.array([(40, 90, 170)] + [c for _, _, _, c, _ in REGIONS], np.uint8)
    Image.fromarray(palette[region]).save(os.path.join(PREVIEW, "previa_regioes.png"))
    shade = np.clip((height - height.min()) / (np.percentile(height, 99.5) - height.min()) * 255, 0, 255).astype(np.uint8)
    Image.fromarray(shade).save(os.path.join(PREVIEW, "previa_altura.png"))
    Image.fromarray(hillshade(height)).save(os.path.join(PREVIEW, "previa_relevo.png"))   # 07/11: sombreado
    print(f"terra {land.mean() * 100:.0f}%  altura {height.min():.1f}..{height.max():.1f} m  lava {lava.sum()} px  gelo {ice.sum()} px  rios {made} ({river.sum()} px)")
    prev = np.stack([shade] * 3, axis=-1)
    prev[river_water] = (60, 140, 230)
    Image.fromarray(prev).save(os.path.join(PREVIEW, "previa_rios.png"))
    print("regiões:", {name: int((region == rid).sum()) for rid, name, *_ in REGIONS})
    # 07/11: os lugares marcantes, os caminhos e as raízes por cima do mapa novo
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import StampMarvels
    StampMarvels.main()


if __name__ == "__main__":
    main()
