"""As texturas dos materiais das armas em pixel art (02/11), no estilo da Espada de Cinzas repintada
(Tools/Blender/RestyleWeaponTexture.py): paletas de 6 tons e um desenho por material, 64x64 pixels que se repetem
(as armas usam a projeção em caixa, 1 repetição a cada 50 cm: o pixel tem ~0,8 cm, como o da espada), ampliados 8x
sem filtro.

python Tools/MakePixelWeaponMaterials.py   ->   SourceArt/Blender_Personagem_Armas/04_Texturas/T_<material>{,_normal,_rough}.png
(a fonte do Tools/ImportVortexWeapons.py; as antigas estão em Backups/armas_texturas_2026-11-02)
"""
import json
import os

import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "SourceArt", "Blender_Personagem_Armas", "04_Texturas")
N = 64
UP = 8
NUMBERS = json.load(open(os.path.join(OUT, "materials.json"), encoding="utf-8"))
yy, xx = np.mgrid[0:N, 0:N]


def ramp(*colors):
    return np.array(colors, np.float32)


def noise(rng, cells, octaves=2):
    """Ruído de valor que se repete na borda (a textura emenda), 0..1."""
    out = np.zeros((N, N), np.float32)
    amp, total = 1.0, 0.0
    for o in range(octaves):
        c = cells * 2 ** o
        g = rng.random((c, c)).astype(np.float32)
        fy, fx = yy * c / N, xx * c / N
        y0, x0 = fy.astype(int), fx.astype(int)
        ty, tx = fy - y0, fx - x0
        ty, tx = ty * ty * (3 - 2 * ty), tx * tx * (3 - 2 * tx)
        y1, x1 = (y0 + 1) % c, (x0 + 1) % c
        v = (g[y0, x0] * (1 - tx) + g[y0, x1] * tx) * (1 - ty) + (g[y1, x0] * (1 - tx) + g[y1, x1] * tx) * ty
        out += v * amp
        total += amp
        amp *= .5
    return out / total


def voronoi(rng, count):
    """As facetas (gemas) e as rachaduras (brasa): a distância ao ponto mais perto, à borda e o índice da célula."""
    pts = rng.random((count, 2)) * N
    best = np.full((N, N), 1e9, np.float32)
    second = np.full((N, N), 1e9, np.float32)
    index = np.zeros((N, N), int)
    for i, (py, px) in enumerate(pts):
        dy = np.abs(yy - py)
        dx = np.abs(xx - px)
        dy = np.minimum(dy, N - dy)
        dx = np.minimum(dx, N - dx)
        d = np.hypot(dy, dx)
        closer = d < best
        second = np.where(closer, best, np.minimum(second, d))
        index = np.where(closer, i, index)
        best = np.where(closer, d, best)
    return best, second - best, index


def paint(pal, t):
    return pal[np.clip((t * len(pal)).astype(int), 0, len(pal) - 1)]


def metal(rng, pal, scratches=10):
    # escovado: listras finas no comprimento, umas manchas largas e riscos claros
    streak = noise(rng, 2, 1)[:, :1] * .6 + rng.random((N, 1)).astype(np.float32) * .4
    t = .35 + .3 * (np.repeat(streak, N, 1) - .5) + .35 * (noise(rng, 4) - .5) + .3
    t = t.copy()
    for _ in range(scratches):
        y, x, ln = rng.integers(N), rng.integers(N), rng.integers(4, 12)
        for i in range(ln):
            t[(y + i // 3) % N, (x + i) % N] += .35
    t[rng.random((N, N)) < .012] += .4                                          # brilhinhos
    return paint(pal, t), t


def wood(rng, pal, rings=True):
    # veios no comprimento (ao longo de y), ondulados, e um nó
    warp = noise(rng, 3) * 6
    line = np.sin((xx + warp) / N * 2 * np.pi * 7) * .5 + .5
    t = .25 + .45 * line + .3 * noise(rng, 8, 1)
    if rings:
        ky, kx = rng.integers(N), rng.integers(N)
        d = np.hypot(np.minimum(np.abs(yy - ky), N - np.abs(yy - ky)) * .6, np.minimum(np.abs(xx - kx), N - np.abs(xx - kx)))
        t = np.where(d < 7, .15 + .25 * (np.sin(d * 1.6) * .5 + .5), t)
    return paint(pal, t), t


def leather(rng, pal):
    # couro: manchado, poros escuros e uma faixa de costura a cada 32 px
    t = .3 + .45 * noise(rng, 6) + .2 * noise(rng, 16, 1)
    t[rng.random((N, N)) < .05] -= .2
    stitch = ((yy % 32) == 2) & ((xx % 6) < 3)
    t = np.where(stitch, .95, t)
    t = np.where((yy % 32) == 3, t - .25, t)
    return paint(pal, t), t


def cloth(rng, pal):
    # trama: xadrez fino de fios
    weave = ((xx + yy) % 4 < 2).astype(np.float32)
    t = .35 + .25 * weave + .25 * noise(rng, 4) + .1 * ((xx % 2) == 0)
    return paint(pal, t), t


def gem(rng, pal, cells=14):
    # facetas: cada uma com um tom (a luz bate diferente em cada face), a borda clara
    d, edge, idx = voronoi(rng, cells)
    shade = rng.random(cells).astype(np.float32)[idx]
    t = .2 + .55 * shade + .2 * (1 - d / max(1, d.max()))
    t = np.where(edge < 1.1, .97, t)
    t[rng.random((N, N)) < .01] = 1.0
    return paint(pal, t), t


def embers(rng, pal):
    # brasa: pedra escura com rachaduras acesas
    d, edge, idx = voronoi(rng, 12)
    t = .1 + .2 * noise(rng, 6)
    crack = edge < 1.6
    t = np.where(crack, .65 + .35 * (1 - edge / 1.6), t)
    return paint(pal, t), t


def rope(rng, pal):
    # corda: fios em diagonal
    band = ((xx + yy) % 8) / 8.0
    t = .2 + .7 * np.sin(band * np.pi) + .1 * noise(rng, 8, 1)
    return paint(pal, t), t


def leaf(rng, pal):
    # folha: a nervura do meio e as laterais
    t = .35 + .3 * noise(rng, 5)
    mid = np.abs(xx - N // 2) < 1
    side = (np.abs(((yy - np.abs(xx - N // 2) * .8) % 12)) < 1)
    t = np.where(mid | side, .85, t)
    return paint(pal, t), t


def paper(rng, pal):
    t = .45 + .3 * noise(rng, 5) + .1 * noise(rng, 16, 1)
    t = np.where((yy % 8) == 5, t - .18, t)                                       # as linhas da página
    return paint(pal, t), t


def rune(rng, pal):
    # pedra cinza com marcas de runa acesas
    t = .25 + .35 * noise(rng, 6) + .1 * noise(rng, 16, 1)
    glyph = np.zeros((N, N), bool)
    for _ in range(5):
        y, x = rng.integers(4, N - 12), rng.integers(4, N - 12)
        for _ in range(4):
            dy, dx = [(0, 1), (1, 0), (1, 1), (-1, 1)][rng.integers(4)]
            for i in range(rng.integers(3, 7)):
                glyph[(y + dy * i) % N, (x + dx * i) % N] = True
            y, x = (y + dy * 3) % N, (x + dx * 3) % N
    t = np.where(glyph, 1.0, t)
    return paint(pal, t), t


STEEL = ramp((48, 54, 68), (78, 88, 106), (112, 124, 144), (150, 162, 182), (192, 202, 218), (232, 238, 248))
STYLES = {
    "aco": (metal, STEEL),
    "aco_azul": (metal, ramp((30, 46, 86), (48, 74, 130), (72, 108, 176), (108, 150, 214), (156, 194, 240), (220, 236, 255))),
    "aco_escuro": (metal, ramp((24, 24, 32), (40, 42, 54), (60, 64, 80), (86, 92, 110), (120, 126, 146), (166, 172, 190))),
    "ferro": (metal, ramp((34, 32, 34), (56, 54, 56), (80, 78, 80), (108, 106, 106), (140, 138, 136), (178, 176, 172))),
    "prata": (metal, ramp((104, 112, 128), (140, 150, 166), (176, 186, 200), (206, 214, 226), (230, 236, 244), (252, 254, 255))),
    "bronze": (metal, ramp((76, 40, 16), (116, 66, 28), (156, 98, 44), (194, 134, 66), (224, 172, 102), (246, 214, 156))),
    "ouro": (metal, ramp((96, 56, 10), (148, 98, 20), (198, 144, 36), (236, 190, 70), (252, 224, 120), (255, 246, 196))),
    "madeira": (wood, ramp((70, 38, 18), (98, 56, 28), (126, 76, 40), (152, 98, 56), (178, 122, 74), (202, 148, 96))),
    "madeira_clara": (wood, ramp((140, 98, 58), (166, 122, 78), (190, 146, 98), (210, 170, 120), (228, 194, 146), (244, 218, 176))),
    "madeira_escura": (wood, ramp((36, 18, 12), (56, 30, 18), (78, 44, 26), (100, 60, 36), (124, 78, 48), (150, 100, 64))),
    "betula": (wood, ramp((150, 140, 124), (186, 178, 162), (214, 208, 194), (232, 228, 216), (244, 242, 234), (252, 252, 248))),
    "couro": (leather, ramp((54, 28, 14), (82, 44, 22), (112, 62, 32), (140, 82, 44), (168, 106, 60), (210, 170, 120))),
    "couro_escuro": (leather, ramp((26, 14, 10), (42, 24, 16), (60, 36, 24), (80, 50, 34), (102, 66, 46), (160, 130, 96))),
    "couro_vermelho": (leather, ramp((60, 14, 12), (92, 24, 18), (124, 36, 26), (156, 52, 36), (186, 72, 50), (226, 170, 120))),
    "tecido_azul": (cloth, ramp((20, 34, 80), (32, 52, 116), (46, 74, 152), (66, 100, 184), (96, 134, 212), (140, 176, 236))),
    "tecido_vermelho": (cloth, ramp((70, 10, 12), (102, 18, 20), (134, 28, 28), (164, 42, 38), (192, 62, 52), (222, 96, 80))),
    "ametista": (gem, ramp((58, 22, 96), (88, 38, 140), (122, 60, 184), (160, 92, 220), (200, 140, 246), (244, 214, 255))),
    "cristal": (gem, ramp((70, 120, 160), (96, 156, 196), (130, 190, 224), (170, 216, 240), (208, 236, 252), (248, 254, 255))),
    "jade": (gem, ramp((16, 80, 60), (28, 114, 86), (44, 150, 112), (72, 186, 140), (120, 220, 176), (200, 250, 226))),
    "rubi": (gem, ramp((90, 8, 24), (136, 16, 36), (180, 30, 52), (220, 54, 74), (246, 100, 116), (255, 190, 196))),
    "safira": (gem, ramp((14, 30, 110), (24, 52, 160), (40, 84, 206), (70, 124, 236), (120, 170, 252), (200, 226, 255))),
    "topazio": (gem, ramp((120, 74, 8), (170, 112, 16), (214, 156, 30), (240, 196, 60), (252, 228, 120), (255, 250, 210))),
    "vidro": (gem, ramp((110, 150, 150), (140, 180, 178), (172, 206, 202), (200, 226, 222), (226, 242, 240), (252, 255, 255))),
    "brasa": (embers, ramp((26, 12, 10), (48, 20, 14), (150, 34, 10), (226, 86, 16), (254, 158, 40), (255, 230, 140))),
    "corda": (rope, ramp((96, 70, 40), (130, 98, 58), (160, 126, 80), (188, 154, 104), (212, 182, 132), (232, 210, 166))),
    "folha": (leaf, ramp((24, 70, 24), (36, 98, 32), (52, 128, 42), (76, 156, 56), (110, 186, 76), (170, 220, 120))),
    "papel": (paper, ramp((150, 122, 82), (182, 154, 110), (208, 182, 138), (226, 204, 162), (240, 224, 188), (252, 244, 220))),
    "runa": (rune, ramp((54, 58, 66), (76, 82, 92), (100, 108, 118), (128, 136, 146), (156, 164, 172), (120, 240, 255))),
}


def normal_from(t, strength=1.5):
    gx = (np.roll(t, -1, 1) - np.roll(t, 1, 1)) * strength
    gy = (np.roll(t, -1, 0) - np.roll(t, 1, 0)) * strength
    n = np.dstack([-gx, gy, np.ones_like(t)])
    n /= np.linalg.norm(n, axis=-1, keepdims=True)
    return ((n * .5 + .5) * 255).astype(np.uint8)


def big(a):
    return np.repeat(np.repeat(a, UP, 0), UP, 1)


for i, (name, (style, pal)) in enumerate(STYLES.items()):
    rng = np.random.default_rng(1000 + i)
    color, height = style(rng, pal)
    Image.fromarray(big(color.astype(np.uint8))).save(os.path.join(OUT, "T_%s.png" % name))
    Image.fromarray(big(normal_from(np.clip(height, 0, 1)))).save(os.path.join(OUT, "T_%s_normal.png" % name))
    rough = NUMBERS.get(name, {}).get("roughness", .6)
    r = np.clip(rough + (height - .5) * -.15, 0, 1)
    Image.fromarray(big((r * 255).astype(np.uint8))).save(os.path.join(OUT, "T_%s_rough.png" % name))
    print("material:", name)
