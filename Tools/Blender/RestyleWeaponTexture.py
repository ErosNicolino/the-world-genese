"""Repinta a textura de uma arma em pixel art, no estilo do jogo (02/11), sem mexer no modelo.

Cada texel da textura é levado ao ponto da arma onde ele fica (os triângulos rasterizados no espaço das UVs: a posição e
a normal). A arma é medida no próprio eixo (o comprimento, a largura e a espessura, pelo PCA dos vértices), e cada parte
ganha a sua pintura: a parte de cada texel (lâmina, guarda, cabo, pomo...) vem da posição ao longo do eixo e da cor de
antes (o couro do cabo). O tom sai de paletas de 6 cores (pixel art), com o sombreado da textura de antes (os detalhes
do modelo, as voltas do couro) e um grão de "pixels" de ~1 cm no espaço da arma (o mesmo desenho em qualquer ilha de UV).

blender -b --factory-startup -P Tools/Blender/RestyleWeaponTexture.py -- <W_arma.glb> <textura de antes> <saída.png> [arma]
(caminhos absolutos)
"""
import math
import os
import sys

import bpy
import numpy as np

args = sys.argv[sys.argv.index("--") + 1:]
GLB, SRC, OUT = args[0], args[1], args[2]
NAME = args[3] if len(args) > 3 else os.path.splitext(os.path.basename(GLB))[0][2:]
RES = 256             # o desenho em 256 (pixel nítido); sai ampliado 2x sem filtro
PIX = .008            # o "pixel" do grão no espaço da arma (m)

for o in list(bpy.data.objects):
    bpy.data.objects.remove(o, do_unlink=True)
# vários modelos separados por ";" (a mesma textura: o arco ametista e a flecha)
for path in GLB.split(";"):
    bpy.ops.import_scene.gltf(filepath=path)
meshes = [o for o in bpy.data.objects if o.type == "MESH"]
if len(meshes) > 1:
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.join()
obj = meshes[0]
me = obj.data
me.calc_loop_triangles()
uv = np.array([d.uv[:] for d in me.uv_layers.active.data], np.float32)
co = np.array([v.co[:] for v in me.vertices], np.float32)
tris = [(t.loops[:], t.vertices[:], t.normal[:]) for t in me.loop_triangles]

# a textura de antes, reduzida (média) para 256
src = bpy.data.images.load(SRC)
sw, sh = src.size
spx = np.array(src.pixels[:], np.float32).reshape(sh, sw, 4)[..., :3]
k = sw // RES
before = spx.reshape(RES, k, RES, k, 3).mean(axis=(1, 3))          # linhas de baixo para cima (o Blender)

# ---- a rasterização: posição e normal de cada texel
pos = np.zeros((RES, RES, 3), np.float32)
nrm = np.zeros((RES, RES, 3), np.float32)
hit = np.zeros((RES, RES), bool)
for loops, verts, n in tris:
    a, b, c = (uv[l] * RES for l in loops)
    x0, x1 = int(max(0, math.floor(min(a[0], b[0], c[0])))), int(min(RES - 1, math.ceil(max(a[0], b[0], c[0]))))
    y0, y1 = int(max(0, math.floor(min(a[1], b[1], c[1])))), int(min(RES - 1, math.ceil(max(a[1], b[1], c[1]))))
    if x1 < x0 or y1 < y0:
        continue
    xs, ys = np.meshgrid(np.arange(x0, x1 + 1) + .5, np.arange(y0, y1 + 1) + .5)
    d = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
    if abs(d) < 1e-9:
        continue
    w0 = ((b[1] - c[1]) * (xs - c[0]) + (c[0] - b[0]) * (ys - c[1])) / d
    w1 = ((c[1] - a[1]) * (xs - c[0]) + (a[0] - c[0]) * (ys - c[1])) / d
    w2 = 1 - w0 - w1
    inside = (w0 >= -.02) & (w1 >= -.02) & (w2 >= -.02)
    if not inside.any():
        continue
    P = w0[..., None] * co[verts[0]] + w1[..., None] * co[verts[1]] + w2[..., None] * co[verts[2]]
    sl = (slice(y0, y1 + 1), slice(x0, x1 + 1))
    pos[sl][inside] = P[inside]
    nrm[sl][inside] = n
    hit[sl] |= inside

# ---- a arma no próprio eixo: t (comprimento), w (largura), h (espessura)
center = co.mean(0)
_, _, axes = np.linalg.svd(co - center, full_matrices=False)
rel = pos - center
T, W, H = rel @ axes[0], rel @ axes[1], rel @ axes[2]
NT, NW, NH = nrm @ axes[0], nrm @ axes[1], nrm @ axes[2]

# a cor de antes: o material e o sombreado
r, g, b = before[..., 0], before[..., 1], before[..., 2]
mx, mn = before.max(-1), before.min(-1)
sat = np.where(mx > 1e-4, (mx - mn) / np.maximum(mx, 1e-4), 0)
lum = .2126 * r + .7152 * g + .0722 * b
warm = (sat > .28) & (r > b)

PALETTES = {
    # 6 tons cada, do escuro ao claro
    "aco": [(40, 44, 56), (66, 74, 90), (100, 110, 128), (138, 150, 168), (180, 192, 208), (222, 230, 240)],
    "fio": [(150, 162, 180), (186, 198, 214), (214, 224, 236), (234, 240, 248), (246, 250, 255), (255, 255, 255)],
    "fuligem": [(22, 20, 22), (36, 32, 34), (54, 48, 48), (74, 66, 64), (98, 88, 84), (124, 114, 108)],
    "ferro": [(20, 18, 22), (34, 30, 36), (50, 44, 50), (68, 60, 66), (90, 80, 86), (116, 104, 108)],
    "ouro": [(88, 48, 12), (132, 80, 20), (178, 118, 34), (220, 162, 56), (246, 204, 96), (255, 236, 160)],
    "couro": [(36, 14, 12), (58, 24, 18), (84, 36, 26), (112, 52, 34), (140, 72, 46), (168, 96, 62)],
    "brasa": [(110, 18, 6), (170, 40, 8), (226, 82, 14), (252, 136, 30), (255, 192, 76), (255, 238, 164)],
}


def parts_espada(m):
    """A espada: a lâmina (t < guarda), a guarda (o alargamento), o cabo (o couro), o pomo (o fim)."""
    grip = hit & warm
    g0, g1 = np.quantile(T[grip], .02), np.quantile(T[grip], .98)
    tip_side = -1 if abs(T[hit].min() - g0) > abs(T[hit].max() - g1) else 1       # a ponta fica do lado longe do cabo
    if tip_side < 0:                                                                 # lâmina em t < g0
        guard = hit & ~grip & (T < g0) & (T > g0 - .095)
        blade = hit & ~grip & (T <= g0 - .095)
        pommel = hit & ~grip & (T > g1)
        along = np.clip((g0 - .095 - T) / max(1e-4, g0 - .095 - T[hit].min()), 0, 1)    # 0 na guarda, 1 na ponta
    else:
        guard = hit & ~grip & (T > g1) & (T < g1 + .095)
        blade = hit & ~grip & (T >= g1 + .095)
        pommel = hit & ~grip & (T < g0)
        along = np.clip((T - g1 - .095) / max(1e-4, T[hit].max() - g1 - .095), 0, 1)
    half = np.quantile(np.abs(W[blade]), .97)
    across = np.abs(W) / max(half, 1e-4)                                             # 0 no meio da lâmina, 1 no fio
    m[blade] = "aco"
    m[blade & (across > .78)] = "fio"
    # o sulco de brasa: o fundo rebaixado no meio da lâmina (mais fino que a face) e as paredes dele
    surface = np.quantile(np.abs(H[blade & (across < .6)]), .85)
    groove = blade & (across < .4) & (np.abs(H) < surface - .003) & (along < .8)
    m[groove] = "brasa"
    GLOW[groove] = 1 - np.abs(W[groove]) / max(1e-4, np.abs(W[groove]).max())      # 1 no meio do sulco
    # a fuligem: o aço escurece perto da guarda (as cinzas), com a borda quebrada em pixels
    soot = blade & (m == "aco") & (noise(.035) * .35 + along * 2.2 < .62)
    m[soot] = "fuligem"
    m[guard | pommel] = "ferro"
    # o friso dourado: as bordas da guarda e o anel do pomo
    gw = np.quantile(np.abs(W[guard]), .97) if guard.any() else 1
    m[guard & ((np.abs(W) > gw * .82) | (np.abs(T - (g0 - .0475 if tip_side < 0 else g1 + .0475)) > .038))] = "ouro"
    if pommel.any():
        pend = T[pommel].max() if tip_side < 0 else T[pommel].min()
        dist = np.abs(T - pend)
        m[pommel & (dist > .055)] = "ouro"
        gem = pommel & (dist < .012) & (np.abs(NT) > .7) & (np.hypot(W, H) < .026)     # a gema na ponta
        m[gem] = "brasa"
        GLOW[gem] = 1 - np.hypot(W, H)[gem] / .026
    m[grip] = "couro"
    return m


def noise(cell_size, seed=7):
    """Ruído de valor em 3D, suave por células (as manchas de fuligem), 0..1."""
    p = pos / cell_size
    i = np.floor(p).astype(np.int64)
    f = p - i
    f = f * f * (3 - 2 * f)

    def h(x, y, z):
        v = (x * 73856093) ^ (y * 19349663) ^ (z * 83492791) ^ seed * 2654435761
        return ((v % 1009) / 1009.0).astype(np.float32)

    out = 0
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                w = (f[..., 0] if dx else 1 - f[..., 0]) * (f[..., 1] if dy else 1 - f[..., 1]) * (f[..., 2] if dz else 1 - f[..., 2])
                out = out + w * h(i[..., 0] + dx, i[..., 1] + dy, i[..., 2] + dz)
    return out


def parts_by_color(m):
    """As outras armas: o material pela cor de antes (aço cinza, ouro, madeira/couro marrons, gemas), com a madeira
    separada do couro pelo brilho (a madeira do Meshy é mais clara)."""
    hue = np.where(mx == r, ((g - b) / np.maximum(mx - mn, 1e-5)) % 6,
                   np.where(mx == g, (b - r) / np.maximum(mx - mn, 1e-5) + 2, (r - g) / np.maximum(mx - mn, 1e-5) + 4)) * 60
    m[hit] = "aco"
    m[hit & (sat < .25) & (lum < .1)] = "ferro"
    brown = hit & (sat >= .25) & (hue >= 10) & (hue < 45)
    m[brown & (lum >= .22)] = "madeira"
    m[brown & (lum < .22)] = "couro"
    m[hit & (sat >= .4) & (hue >= 38) & (hue < 65) & (lum >= .3)] = "ouro"
    m[hit & (sat >= .35) & ((hue < 10) | (hue >= 300))] = "rubi"
    m[hit & (sat >= .3) & (hue >= 65) & (hue < 170)] = "jade"
    m[hit & (sat >= .3) & (hue >= 170) & (hue < 255)] = "safira"
    m[hit & (sat >= .3) & (hue >= 255) & (hue < 300)] = "ametista"
    # as armas de madeira clara: o cinza (e a madeira manchada) é a madeira, não aço
    wood = {"arco_de_betula": "betula", "cajado_de_ilsa": "madeira_clara"}.get(NAME)
    if wood:
        m[hit & ((m == "aco") | (m == "madeira"))] = wood
    return m


PALETTES.update({
    "madeira": [(70, 38, 18), (98, 56, 28), (126, 76, 40), (152, 98, 56), (178, 122, 74), (202, 148, 96)],
    "madeira_clara": [(140, 98, 58), (166, 122, 78), (190, 146, 98), (210, 170, 120), (228, 194, 146), (244, 218, 176)],
    "betula": [(96, 88, 78), (150, 140, 124), (196, 188, 172), (224, 218, 204), (240, 236, 226), (252, 250, 244)],
    "rubi": [(90, 8, 24), (136, 16, 36), (180, 30, 52), (220, 54, 74), (246, 100, 116), (255, 190, 196)],
    "jade": [(16, 80, 60), (28, 114, 86), (44, 150, 112), (72, 186, 140), (120, 220, 176), (200, 250, 226)],
    "safira": [(14, 30, 110), (24, 52, 160), (40, 84, 206), (70, 124, 236), (120, 170, 252), (200, 226, 255)],
    "ametista": [(58, 22, 96), (88, 38, 140), (122, 60, 184), (160, 92, 220), (200, 140, 246), (244, 214, 255)],
})
PARTS = {"espada_de_cinzas": parts_espada}
MAT = np.full((RES, RES), "", object)
GLOW = np.zeros((RES, RES), np.float32)               # a brasa: 1 no meio (amarela), 0 na borda (vermelha)
MAT = PARTS.get(NAME, parts_by_color)(MAT)

# ---- a pintura
light = np.array([.3, .5, .81], np.float32)
light /= np.linalg.norm(light)
lit = np.clip(np.abs(nrm @ np.array([0, 0, 1], np.float32)) * .4 + (nrm @ light) * .3 + .45, 0, 1)
cell = np.floor(pos / PIX).astype(np.int64)
grain = ((cell[..., 0] * 73856093) ^ (cell[..., 1] * 19349663) ^ (cell[..., 2] * 83492791)) % 9
out = np.zeros((RES, RES, 3), np.float32)
for name, pal in PALETTES.items():
    m = hit & (MAT == name)
    if not m.any():
        continue
    # o sombreado de antes, normalizado na parte (as voltas do couro, os chanfros), mais um pouco de volume
    lo, hi = np.quantile(lum[m], .05), np.quantile(lum[m], .95)
    shade = np.clip((lum[m] - lo) / max(1e-4, hi - lo), 0, 1)
    if name in ("aco", "fio", "ferro") and NAME in PARTS:
        # metal: limpo (pixel art), o volume vem da luz e só um pouco dos detalhes de antes
        t = .05 + .2 * shade + .5 * lit[m]
        t = t + np.where(grain[m] == 0, .1, 0)
    else:
        t = .05 + .5 * shade + .3 * lit[m]
        t = t + np.where(grain[m] == 0, .12, np.where(grain[m] == 8, -.12, 0))
    if name == "brasa":
        # a brasa: mais quente no meio do sulco, pulsando ao longo da lâmina
        t = .15 + .75 * GLOW[m] + .25 * (noise(.025, 3)[m] - .5)
    idx = np.clip((t * len(pal)).astype(int), 0, len(pal) - 1)
    out[m] = np.array(pal, np.float32)[idx] / 255.0
# o contorno escuro do sulco e do friso (pixel art: a linha que separa as partes)
for name, dark in (("brasa", (.16, .05, .03)), ("ouro", (.2, .11, .04))):
    m = hit & (MAT == name)
    ring = np.zeros_like(m)
    for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        ring |= np.roll(np.roll(m, dy, 0), dx, 1)
    ring &= hit & ~m & ((MAT == "aco") | (MAT == "ferro") | (MAT == "fuligem"))
    # só onde o vizinho está perto no espaço da arma (não do outro lado de uma costura de UV)
    out[ring] = out[ring] * .45 + np.array(dark, np.float32) * .55
# fora das ilhas: a cor vizinha (sem costura escura no filtro)
for _ in range(3):
    miss = ~hit
    grown = np.zeros_like(out)
    cnt = np.zeros((RES, RES), np.float32)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            m2 = np.roll(np.roll(hit, dy, 0), dx, 1)
            grown += np.roll(np.roll(out, dy, 0), dx, 1) * m2[..., None]
            cnt += m2
    fill = miss & (cnt > 0)
    out[fill] = grown[fill] / cnt[fill][..., None]
    hit = hit | fill

img = bpy.data.images.new("restyle", RES * 2, RES * 2, alpha=False)
big = np.repeat(np.repeat(out, 2, axis=0), 2, axis=1)
rgba = np.concatenate([big, np.ones(big.shape[:2] + (1,), np.float32)], -1)
img.pixels.foreach_set(rgba.ravel())
img.filepath_raw = OUT
img.file_format = "PNG"
img.save()
print("RESTYLE", NAME, {k: int((MAT == k).sum()) for k in PALETTES}, "->", OUT)
