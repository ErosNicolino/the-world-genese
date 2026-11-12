"""A fada (10/11): criatura passiva pequenina (~9 cm), feita de blocos de pixel como os heróis.

No Blender aberto (MCP):  FADA_VARIANTE = 0; exec(open(r"<raiz>/Tools/Blender/MakeFairy.py", encoding="utf-8").read())
Sem janela:               blender -b --factory-startup -P Tools/Blender/MakeFairy.py -- <variante> [jogo]

Cada bloco (voxel) tem uma cor só: o rosto, a flor, as nervuras das asas e as pétalas são pintados bloco a bloco. As
faces escondidas saem. Duas cores de material: o corpo (opaco) e as asas (translúcidas). Cada bloco é de um osso só
(corpo, cabeça, braços, pernas, 4 asas): rígido como os heróis. Variantes (a folha do usuário): 0 cabelo curto com a
flor, 1 duas tranças, 2 orelha pontuda, 3 coroa de flores, 4 pontas loiras, 5 pontas lilás com a borboleta.
"""
import math
import os
import sys

import bpy
from mathutils import Vector

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
VARIANT = int(next((a for a in ARGS if a.isdigit()), globals().get("FADA_VARIANTE", 0)))
GAME = "jogo" in ARGS or globals().get("FADA_JOGO", False)
HEIGHT_M = .09                      # a fada inteira, do pé ao alto do cabelo
OUT = os.path.join(ROOT, "Data", "Creatures", "Fada")

# ---- as cores (RGBA 0-255); as das asas têm alfa
COL = {
    "pele": (250, 206, 170, 255), "pele_s": (232, 176, 142, 255), "bochecha": (240, 150, 160, 255),
    "olho": (58, 32, 30, 255), "brilho": (255, 255, 255, 255), "boca": (150, 70, 70, 255),
    "verde": (150, 196, 92, 255), "verde_c": (184, 220, 120, 255), "verde_e": (96, 150, 60, 255),
    "rosa": (246, 170, 196, 255), "rosa_c": (252, 206, 220, 255), "rosa_e": (222, 128, 162, 255),
    "marrom": (110, 72, 44, 255), "sapato": (104, 160, 58, 255), "sapato_e": (70, 120, 40, 255),
    "petala": (252, 252, 246, 255), "miolo": (250, 200, 50, 255), "folha": (96, 160, 64, 255),
    "asa": (170, 240, 240, 110), "asa_v": (90, 200, 210, 200), "asa_b": (210, 255, 255, 170),
    "azul_b": (110, 160, 255, 255), "azul_bc": (170, 210, 255, 255),
}
# o cabelo: (claro, escuro) por variante; as pontas (4 e 5)
HAIR = [((240, 130, 172), (212, 96, 146))] * 5 + [((232, 124, 178), (200, 92, 152))]
TIPS = {4: ((252, 226, 150), (240, 206, 120)), 5: ((186, 160, 236), (156, 128, 214))}
for n, (a, b) in enumerate(HAIR):
    COL["cabelo%d" % n] = a + (255,)
    COL["cabelo%d_e" % n] = b + (255,)
for n, (a, b) in TIPS.items():
    COL["ponta%d" % n] = a + (255,)
    COL["ponta%d_e" % n] = b + (255,)

V = {}   # (x, y, z) -> (cor, osso)


def h(*k):
    """Um número fixo de 0 a 1 por posição (o desenho não muda de uma geração para outra)."""
    return (math.sin(sum(v * m for v, m in zip(k, (12.9898, 78.233, 37.719)))) * 43758.5453) % 1.0


def put(x, y, z, c, bone, over=True):
    if over or (x, y, z) not in V:
        V[(x, y, z)] = (c, bone)


def box(x0, x1, y0, y1, z0, z1, c, bone, over=True):
    """De x0 a x1 (inclusive) etc."""
    for x in range(x0, x1 + 1):
        for y in range(y0, y1 + 1):
            for z in range(z0, z1 + 1):
                put(x, y, z, c(x, y, z) if callable(c) else c, bone, over)


# ---- o corpo. x: da esquerda da tela (-) para a direita (+); y: frente (-) para trás (+); z: do chão para cima
# pés e pernas
for side, (x0, bone) in ((-1, (-3, "perna_E")), (1, (1, "perna_D"))):
    box(x0, x0 + 1, -2, 1, 0, 1, lambda x, y, z: "sapato_e" if z == 0 or (y == -2 and z == 1 and h(x, z) < .5) else "sapato", bone)
    box(x0, x0 + 1, -1, 0, 2, 7, lambda x, y, z: "pele_s" if y == 0 and h(x, y, z) < .3 else "pele", bone)
    put(x0 + (0 if side < 0 else 1), -2, 2, "sapato", bone)          # a ponta da folha do sapato
# a saia: as pétalas rosa embaixo (a barra recortada) e as folhas verdes por cima (pontas para baixo)
for x in range(-6, 6):
    for y in range(-4, 4):
        edge = x in (-6, 5) or y in (-4, 3)
        for z in range(6, 9):
            if z == 6 and edge and h(x, y) < .45:
                continue
            put(x, y, z, "rosa_e" if z == 6 else ("rosa_c" if (x + y + z) % 3 == 0 else "rosa"), "corpo")
for x in range(-5, 5):
    for y in range(-3, 3):
        for z in range(8, 11):
            if not (x in (-5, 4) or y in (-3, 2) or z >= 10):
                continue
            if z == 8 and (x + y) % 2:
                continue
            put(x, y, z, "verde_e" if z == 8 else ("verde_c" if h(x, y, z) < .25 else "verde"), "corpo")
# cinto e o laço de folha na frente
box(-3, 2, -2, 1, 11, 11, "marrom", "corpo")
for p in ((-1, -3, 11), (0, -3, 11), (-2, -3, 12), (1, -3, 12), (-1, -3, 10), (0, -3, 10)):
    put(*p, "folha", "corpo")
# o corpete e a gola de pétalas verde-claras
box(-3, 2, -2, 1, 12, 15, lambda x, y, z: "verde_c" if h(x, y, z) < .2 else "verde", "corpo")
for x in range(-4, 4):
    for y in range(-3, 3):
        if x in (-4, 3) or y in (-3, 2):
            put(x, y, 16, "verde_c", "corpo")
            if (x + y) % 2 == 0:
                put(x, y, 15, "verde_c", "corpo")
# braços (um pouco afastados do corpo) e a mangaquinha de folha no ombro
for x0, bone in ((-6, "braco_E"), (4, "braco_D")):
    box(x0, x0 + 1, -1, 0, 9, 15, lambda x, y, z: "pele_s" if z == 9 else "pele", bone)
    box(x0, x0 + 1, -1, 0, 14, 15, "verde_c", bone)
    put(x0 + (0 if x0 < 0 else 1), -2, 15, "verde_c", bone)
# a cabeça: 10 x 10 x 10, o rosto na frente (y = -5)
box(-5, 4, -5, 4, 17, 26, lambda x, y, z: "pele_s" if z == 17 else "pele", "cabeca")
FACE = {}
for x, z in ((-3, 21), (-3, 22), (-3, 23), (-2, 21), (-2, 22), (-2, 23), (1, 21), (1, 22), (1, 23), (2, 21), (2, 22), (2, 23)):
    FACE[(x, z)] = "olho"
FACE[(-2, 23)] = FACE[(2, 23)] = "brilho"
for x, z in ((-4, 20), (3, 20)):
    FACE[(x, z)] = "bochecha"
for x, z in ((-1, 19), (0, 19), (-2, 20), (1, 20)):
    FACE[(x, z)] = "boca"
# 10/11 (o usuário: "o rosto animado, como o do herói"): o rosto não é mais pintado nos blocos; é uma placa na frente
# da cabeça com 4 quadros (normal, piscando, feliz, assustada), trocados pelo jogo (M_FadaRosto, parâmetro Quadro)
# orelhas: pequenas e pontudas (a variante 2 maiores, para cima)
for side in (-1, 1):
    xo = -6 if side < 0 else 5
    box(xo, xo, -1, 0, 20, 22, "pele", "cabeca")
    if VARIANT == 2:
        put(xo + side, -1, 22, "pele", "cabeca")
        put(xo + side, -1, 23, "pele", "cabeca")
        put(xo + 2 * side, -1, 24, "pele_s", "cabeca")
    else:
        put(xo, -1, 23, "pele", "cabeca")


# ---- o cabelo: uma casca em volta da cabeça, dois tons sorteados; a franja recortada na testa
def hair_col(x, y, z):
    tip = VARIANT in TIPS and z <= 20
    base = ("ponta%d" % VARIANT) if tip else ("cabelo%d" % VARIANT)
    return base + ("_e" if h(x, y, z) < .35 else "")


for x in range(-6, 6):
    for y in range(-6, 6):
        for z in range(17, 29):
            inside = -5 <= x <= 4 and -5 <= y <= 4 and 17 <= z <= 26
            if inside:
                continue
            front = y == -6
            side_ = x in (-6, 5)
            top = z >= 27
            back = y == 5
            if top:
                if z == 28 and (h(x, y) < .55 or abs(x) > 4 or abs(y) > 4):
                    continue
            elif front:
                # a franja: cobre a testa até a linha 24, com mechas descendo até 23 e 22
                low = 24 - (1 if h(x, 1) < .45 else 0) - (4 if x in (-5, 4) else 0) - (2 if x in (-6, 5) else 0)
                if z < low or x < -6 or x > 5:
                    continue
            elif side_:
                if (-1 <= y <= 0 and 20 <= z <= 23) or z < (19 if y < 0 else 18):
                    continue   # a orelha aparece
            elif back:
                if z < 17 + (1 if h(x, 3) < .5 else 0):
                    continue
            else:
                continue
            put(x, y, z, hair_col(x, y, z), "cabeca", over=False)
# as tranças (variante 1): duas, na frente dos ombros, em gomos
if VARIANT == 1:
    for x0 in (-8, 6):
        for z in range(11, 21):
            box(x0, x0 + 1, -4, -3, z, z, "cabelo1_e" if z % 3 == 0 else "cabelo1", "cabeca")
        box(x0, x0 + 1, -4, -3, 10, 10, "folha", "cabeca")


def flower(cx, cy, cz, bone="cabeca", petal="petala", core="miolo", leaf=True):
    for dx, dz in ((0, 1), (0, -1), (1, 0), (-1, 0), (1, 1), (-1, -1), (1, -1), (-1, 1)):
        if abs(dx) + abs(dz) == 2 and h(cx + dx, cz + dz) < .5:
            continue
        put(cx + dx, cy, cz + dz, petal, bone)
    put(cx, cy - 1, cz, core, bone)
    put(cx, cy, cz, core, bone)
    if leaf:
        put(cx + 2, cy + 1, cz - 1, "folha", bone)
        put(cx + 2, cy + 1, cz - 2, "folha", bone)


if VARIANT == 3:
    # a coroa de flores em volta do alto da cabeça, com folhas entre elas
    for (x, y) in ((-5, -6), (-1, -7), (3, -6), (6, -2), (-7, -2)):
        flower(x, y, 27, leaf=False)
    for x in range(-6, 6, 2):
        put(x, -7, 26, "folha", "cabeca")
else:
    flower(3, -7, 26)
if VARIANT == 5:
    # a borboleta azul pousada perto da flor
    for x, z, c in ((4, 29, "azul_b"), (5, 29, "azul_bc"), (6, 30, "azul_b"), (4, 30, "azul_bc"), (5, 30, "azul_b"),
                    (2, 29, "azul_b"), (1, 30, "azul_bc"), (2, 30, "azul_b"), (3, 29, "olho")):
        put(x, -6, z, c, "cabeca")


# ---- as asas: 2 pares, finos (1 bloco), translúcidos, com as nervuras e a borda; o par de cima maior
def wing(side, top):
    bone = "asa_%s_%s" % ("cima" if top else "baixo", "E" if side < 0 else "D")
    # uma elipse inclinada: o comprimento (a) e a largura (b), saindo das costas para o lado e para cima
    a, b, tilt = (11.0, 4.6, .55) if top else (8.0, 3.2, -.45)
    root = Vector((side * 1.5, 3, 15 if top else 13))
    ux = Vector((side * math.cos(tilt), 0, math.sin(tilt)))
    uz = Vector((-side * math.sin(tilt) * side * side, 0, math.cos(tilt)))
    for x in range(-18, 19):
        for z in range(0, 32):
            p = Vector((x + .5, 0, z + .5)) - Vector((root.x, 0, root.z))
            u = p.dot(ux)
            w = p.dot(Vector((-ux.z, 0, ux.x)))
            if u < 0:
                continue
            r = ((u - a) / a) ** 2 + (w / b) ** 2
            if r > 1:
                continue
            vein = abs(w) < .6 or abs(w - u * .28) < .5 or abs(w + u * .28) < .5
            c = "asa_b" if r > .78 else "asa_v" if vein else "asa"
            put(x, 3, z, c, bone)


# 10/11 (o usuário: "asas realistas, batendo"): as asas não são mais de blocos; são lâminas finas com a textura
# pintada em alta resolução (WINGS, mais abaixo). A função wing() fica para a versão em blocos.
# os brilhos (pó de fada) ficam no jogo, em partículas

# ---- a malha: só as faces à mostra; cada cor um pixel da paleta
names = sorted({c for c, _ in V.values()})
PAL_W = 8
PAL_H = 4   # 10/11: sempre 8 x 4 (potência de 2): com 3 linhas a Unreal esticava a textura e as cores saíam trocadas
assert len(names) <= PAL_W * PAL_H, len(names)
PAL_NAME = "T_Fada_%d" % VARIANT   # uma paleta por variante (as cores mudam de lugar)
pal = bpy.data.images.get(PAL_NAME) or bpy.data.images.new(PAL_NAME, PAL_W, max(1, PAL_H), alpha=True)
pal.scale(PAL_W, max(1, PAL_H))
px = [0.0] * (PAL_W * max(1, PAL_H) * 4)
uv_of = {}
for i, n in enumerate(names):
    cx, cy = i % PAL_W, i // PAL_W
    r, g, b, a = COL[n]
    px[(cy * PAL_W + cx) * 4:(cy * PAL_W + cx) * 4 + 4] = [r / 255, g / 255, b / 255, a / 255]
    uv_of[n] = ((cx + .5) / PAL_W, (cy + .5) / max(1, PAL_H))
pal.pixels[:] = px
translucent = lambda c: COL[c][3] < 255

for o in [o for o in bpy.data.objects if o.name.startswith("Fada")]:
    bpy.data.objects.remove(o, do_unlink=True)
U = HEIGHT_M / 30.0                 # metros por bloco (30 blocos de altura)
verts, faces, fuv, fmat, vbone = [], [], [], [], []
DIRS = [((1, 0, 0), [(1, 0, 0), (1, 1, 0), (1, 1, 1), (1, 0, 1)]), ((-1, 0, 0), [(0, 0, 0), (0, 0, 1), (0, 1, 1), (0, 1, 0)]),
        ((0, 1, 0), [(0, 1, 0), (0, 1, 1), (1, 1, 1), (1, 1, 0)]), ((0, -1, 0), [(0, 0, 0), (1, 0, 0), (1, 0, 1), (0, 0, 1)]),
        ((0, 0, 1), [(0, 0, 1), (1, 0, 1), (1, 1, 1), (0, 1, 1)]), ((0, 0, -1), [(0, 0, 0), (0, 1, 0), (1, 1, 0), (1, 0, 0)])]
for (x, y, z), (c, bone) in V.items():
    for (dx, dy, dz), corners in DIRS:
        n = V.get((x + dx, y + dy, z + dz))
        if n and n[1] == bone and translucent(n[0]) == translucent(c):
            continue
        base = len(verts)
        for cx, cy, cz in corners:
            verts.append(((x + cx - .5) * U, (y + cy - .5) * U, (z + cz) * U))
            vbone.append(bone)
        faces.append(tuple(range(base, base + 4)))
        # 10/11: um quadradinho dentro do pixel da cor (não um ponto): com área zero a Unreal não acha as tangentes e
        # algumas faces saíam escuras
        u0, v0 = uv_of[c]
        du, dv = .3 / PAL_W, .3 / PAL_H
        fuv.append(("ASA", [(u0 - du, v0 - dv), (u0 + du, v0 - dv), (u0 + du, v0 + dv), (u0 - du, v0 + dv)]))
        fmat.append(1 if translucent(c) else 0)
# ---- as asas realistas: 2 pares de lâminas (as duas faces), libélula: a forma, as nervuras, a borda clara e o furta-cor
# do ciano no pé ao lilás na ponta. Uma textura (T_FadaAsa, 256 x 128: a de cima à esquerda, a de baixo à direita)
WW, WH = 256, 128
wimg = bpy.data.images.get("T_FadaAsa") or bpy.data.images.new("T_FadaAsa", WW, WH, alpha=True)
wimg.scale(WW, WH)


def wing_pixels():
    import numpy as np
    out = np.zeros((WH, WW, 4), dtype=np.float32)
    ys, xs = np.mgrid[0:WH, 0:WW / 2]
    for half, (narrow, bulge) in enumerate(((.42, .18), (.36, .1))):
        u = (xs + .5) / (WW / 2)                       # 0 no pé, 1 na ponta
        v = (ys + .5) / WH * 2 - 1                     # -1 a 1 na largura
        # a forma: estreita no pé, larga no meio, ponta arredondada; um pouco mais cheia na borda de cima
        width = narrow * np.sqrt(np.clip(u, 0, 1)) * np.sqrt(np.clip(1 - u ** 2.2, 0, 1)) * 2.1 + .02
        vc = v - bulge * u * (1 - u)
        r = np.abs(vc) / np.maximum(width, 1e-3)
        inside = (r < 1) & (u > .02)
        # as nervuras: a principal ao longo, as que abrem em leque e as transversais
        vein = np.abs(vc) < .02
        for k in (-.55, -.25, .3, .62):
            vein |= np.abs(vc - k * width * np.clip(u * 1.6, 0, 1)) < .018
        cross = (np.abs(np.sin(u * 38 + vc * 9)) < .09) & (u > .2)
        edge = (r > .9) & inside
        tint = np.stack([.62 + .3 * u, .93 - .12 * u, .95 + .05 * u], -1)          # ciano -> lilás
        shimmer = .06 * np.sin(u * 20 + vc * 30)
        col = tint + shimmer[..., None]
        a = np.where(inside, .22 + .1 * u, 0)
        a = np.where(inside & cross, .34, a)
        a = np.where(inside & vein, .7, a)
        a = np.where(edge, .85, a)
        col = np.where((inside & vein)[..., None], col * .75, col)
        col = np.where(edge[..., None], np.minimum(1, col + .15), col)
        sl = slice(half * WW // 2, (half + 1) * WW // 2)
        out[:, sl, :3] = np.clip(col, 0, 1)
        out[:, sl, 3] = a
    return out.ravel().tolist()


wimg.pixels[:] = wing_pixels()
WINGS = []   # (osso, raiz, comprimento, largura, inclinação, metade da textura)
for side in (-1, 1):
    sd = "E" if side < 0 else "D"
    WINGS.append(("asa_cima_" + sd, (side * 1.5, 3.2, 15.5), 17.0, 10.0, .5, 0, side))
    WINGS.append(("asa_baixo_" + sd, (side * 1.5, 3.2, 13.0), 12.5, 6.8, -.5, 1, side))
for bone, root, length, width, tilt, half, side in WINGS:
    ax = Vector((side * math.cos(tilt), 0, math.sin(tilt)))
    up = Vector((-ax.z * side, 0, ax.x * side)) if side > 0 else Vector((ax.z, 0, -ax.x))
    R = Vector(root)
    corners = [R - up * width / 2, R + ax * length - up * width / 2, R + ax * length + up * width / 2, R + up * width / 2]
    us = [(half * .5, 0), (half * .5 + .5, 0), (half * .5 + .5, 1), (half * .5, 1)]
    for flip in (False, True):   # as duas faces (a de trás ao contrário)
        base = len(verts)
        order = [0, 1, 2, 3] if not flip else [3, 2, 1, 0]
        for k in order:
            c = corners[k] + Vector((0, .02 if flip else -.02, 0))
            verts.append((c.x * U, c.y * U, c.z * U))
            vbone.append(bone)
        faces.append(tuple(range(base, base + 4)))
        fuv.append(("ASA", [us[k] for k in order]))
        fmat.append(1)
# ---- o rosto: 4 quadros de 10 x 10 lado a lado (T_FadaRosto, 40 x 10), na frente da cabeça (x -5..4, z 17..26)
FW, FH, FRAMES = 10, 10, 4
fimg = bpy.data.images.get("T_FadaRosto") or bpy.data.images.new("T_FadaRosto", FW * FRAMES, FH, alpha=True)
fimg.scale(FW * FRAMES, FH)
fpx = [0.0] * (FW * FRAMES * FH * 4)


def face_put(frame, col, row, c):
    """col 0..9 da esquerda, row 0..9 de cima (z 26 é a linha 0)."""
    r_, g_, b_, a_ = COL[c]
    i = ((FH - 1 - row) * FW * FRAMES + frame * FW + col) * 4
    fpx[i:i + 4] = [r_ / 255, g_ / 255, b_ / 255, 1.0]


for fr in range(FRAMES):
    for col in range(FW):
        for row in range(FH):
            face_put(fr, col, row, "pele_s" if row == 9 else "pele")
    face_put(fr, 1, 6, "bochecha")
    face_put(fr, 8, 6, "bochecha")
    eyes = [(2, 3), (3, 3), (2, 4), (3, 4), (2, 5), (3, 5), (6, 3), (7, 3), (6, 4), (7, 4), (6, 5), (7, 5)]
    if fr in (0, 3):      # aberta (3: assustada, o mesmo olho)
        for c_, r_ in eyes:
            face_put(fr, c_, r_, "olho")
        face_put(fr, 3, 3, "brilho")
        face_put(fr, 7, 3, "brilho")
    elif fr == 1:         # piscando: só a linha dos cílios
        for c_ in (2, 3, 6, 7):
            face_put(fr, c_, 5, "olho")
        face_put(fr, 1, 5, "olho")
        face_put(fr, 8, 5, "olho")
    else:                 # feliz: os olhos em ^
        for c_, r_ in ((2, 4), (3, 4), (1, 5), (4, 5), (6, 4), (7, 4), (5, 5), (8, 5)):
            face_put(fr, c_, r_, "olho")
    if fr == 2:           # a boca aberta sorrindo
        for c_ in (3, 4, 5, 6):
            face_put(fr, c_, 7, "boca")
        face_put(fr, 4, 8, "bochecha")
        face_put(fr, 5, 8, "bochecha")
    elif fr == 3:         # "o"
        for c_, r_ in ((4, 7), (5, 7), (4, 8), (5, 8)):
            face_put(fr, c_, r_, "boca")
    else:                 # o sorrisinho
        for c_, r_ in ((4, 7), (5, 7), (3, 6), (6, 6)):
            face_put(fr, c_, r_, "boca")
fimg.pixels[:] = fpx
base = len(verts)
for x_, z_ in ((-5, 17), (5, 17), (5, 27), (-5, 27)):
    verts.append(((x_ - .5) * U, (-5 - .5 - .02) * U, z_ * U))
    vbone.append("cabeca")
faces.append(tuple(range(base, base + 4)))
fuv.append(("ASA", [(0, 0), (1, 0), (1, 1), (0, 1)]))
fmat.append(2)
mesh = bpy.data.meshes.new("Fada")
mesh.from_pydata(verts, [], faces)
mesh.update()
uvl = mesh.uv_layers.new(name="UV")
for poly, uv in zip(mesh.polygons, fuv):
    poly.material_index = fmat[poly.index]
    for k, li in enumerate(poly.loop_indices):
        uvl.data[li].uv = uv[1][k] if isinstance(uv, tuple) and uv and uv[0] == "ASA" else uv
obj = bpy.data.objects.new("Fada", mesh)
bpy.context.scene.collection.objects.link(obj)


def material(name, blend, image=None):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    tex = next((n for n in nt.nodes if n.type == "TEX_IMAGE"), None) or nt.nodes.new("ShaderNodeTexImage")
    tex.image = image or (wimg if blend else pal)
    tex.interpolation = "Linear" if blend else "Closest"
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = .8
    if blend:
        nt.links.new(tex.outputs["Alpha"], bsdf.inputs["Alpha"])
        bsdf.inputs["Emission Strength"].default_value = .6
        nt.links.new(tex.outputs["Color"], bsdf.inputs["Emission Color"])
        try:
            m.surface_render_method = "BLENDED"
        except (AttributeError, TypeError):
            pass
    return m


mesh.materials.append(material("M_Fada_%d" % VARIANT, False))
mesh.materials.append(material("M_FadaAsa", True))
mesh.materials.append(material("M_FadaRosto", False, fimg))   # no Blender mostra os 4 quadros; no jogo, um por vez

# ---- o esqueleto: cada bloco num osso
JOINTS = {"corpo": ((0, 0, 9), (0, 0, 16), None), "cabeca": ((0, 0, 16.5), (0, 0, 26), "corpo"),
          "braco_E": ((-5, -.5, 15), (-5, -.5, 9), "corpo"), "braco_D": ((5, -.5, 15), (5, -.5, 9), "corpo"),
          "perna_E": ((-2, -.5, 8), (-2, -.5, 0), "corpo"), "perna_D": ((2, -.5, 8), (2, -.5, 0), "corpo"),
          "asa_cima_E": ((-1.5, 3, 15), (-10, 3, 21), "corpo"), "asa_cima_D": ((1.5, 3, 15), (10, 3, 21), "corpo"),
          "asa_baixo_E": ((-1.5, 3, 13), (-8, 3, 9), "corpo"), "asa_baixo_D": ((1.5, 3, 13), (8, 3, 9), "corpo")}
arm_data = bpy.data.armatures.new("Fada_Esqueleto")
rig = bpy.data.objects.new("Fada_Esqueleto", arm_data)
bpy.context.scene.collection.objects.link(rig)
bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode="EDIT")
for name, (hd, tl, parent) in JOINTS.items():
    b = arm_data.edit_bones.new(name)
    b.head = Vector(hd) * U
    b.tail = Vector(tl) * U
for name, (_hd, _tl, parent) in JOINTS.items():
    if parent:
        arm_data.edit_bones[name].parent = arm_data.edit_bones[parent]
bpy.ops.object.mode_set(mode="OBJECT")
groups = {n: obj.vertex_groups.new(name=n) for n in JOINTS}
for i, bname in enumerate(vbone):
    groups[bname].add([i], 1.0, "REPLACE")
obj.parent = rig
mod = obj.modifiers.new("Armature", "ARMATURE")
mod.object = rig
print("FADA variante", VARIANT, "blocos", len(V), "faces", len(faces), "cores", len(names))

# ---- para o jogo: a malha com o esqueleto (SK_Fada_<v>.glb), a textura das asas e uma paleta por cor de cabelo
HAIR_COLORS = {"rosa": ((240, 130, 172), (212, 96, 146)), "lilas": ((186, 150, 236), (154, 118, 212)),
               "menta": ((140, 220, 186), (104, 190, 156)), "loiro": ((250, 222, 140), (230, 192, 104)),
               "ruivo": ((236, 128, 72), (204, 96, 52)), "azul": ((140, 190, 250), (104, 156, 226))}
if GAME:
    os.makedirs(OUT, exist_ok=True)
    for cor, (claro, escuro) in HAIR_COLORS.items():
        img = bpy.data.images.new("T_Fada_%d_%s" % (VARIANT, cor), PAL_W, max(1, PAL_H), alpha=True)
        p2 = list(px)
        for i, n in enumerate(names):
            if n in ("cabelo%d" % VARIANT, "cabelo%d_e" % VARIANT):
                r, g, b = claro if not n.endswith("_e") else escuro
                p2[i * 4:i * 4 + 3] = [r / 255, g / 255, b / 255]
        img.pixels[:] = p2
        img.filepath_raw = os.path.join(OUT, "T_Fada_%d_%s.png" % (VARIANT, cor))
        img.file_format = "PNG"
        img.save()
    fimg.filepath_raw = os.path.join(OUT, "T_FadaRosto.png")
    fimg.file_format = "PNG"
    fimg.save()
    wimg.filepath_raw = os.path.join(OUT, "T_FadaAsa.png")
    wimg.file_format = "PNG"
    wimg.save()
    for o in bpy.context.view_layer.objects:
        o.select_set(o in (obj, rig))
    bpy.context.view_layer.objects.active = rig
    bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "SK_Fada_%d.glb" % VARIANT), use_selection=True, export_format="GLB",
                              export_animations=False, export_skins=True, export_apply=False, export_yup=True, export_def_bones=False)
    print("FADA exportada", VARIANT)
