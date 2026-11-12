"""As peças dos lugares marcantes e as raízes da Árvore do Mundo (07/11), em blocos como o resto do Vortex.

Cada peça é uma grade de blocos (voxels) com a cor de cada um; só as faces de fora viram malha (leve, e a Unreal usa a
própria malha como colisão). Três materiais pelo nome do espaço: MB_block (pedra, madeira, casca: com luz), MB_glow
(os cristais e os veios de seiva das raízes: acesos) e MB_water (a cachoeira: a água que corre).

blender -b --factory-startup -P Tools/Blender/MakeMarvels.py -- <raiz> [peça ...]
Sai Data/World/Marvels/SM_<peça>.glb (Tools/ImportMarvels.py leva para a Unreal; o AVortexLandmarks monta).
Medidas em metros; a origem no chão, no meio (a cachoeira: no alto, caindo para baixo, 10 m de altura).
"""
import math
import os
import random
import sys

import bmesh
import bpy

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
ROOT = ARGS[0] if ARGS else r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
ONLY = ARGS[1:]
OUT = os.path.join(ROOT, "Data", "World", "Marvels")


def hexc(h):
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) / 255 for i in (0, 2, 4))


STONE = [hexc(c) for c in ("8b8a86", "7a7975", "9a9893", "6d6c68", "a6a39c")]
SAND = [hexc(c) for c in ("b89a72", "a8885f", "c7aa82", "9a7b55")]
MOSS = [hexc(c) for c in ("5f8a3c", "4f7a32", "6f9a48")]
BARK = [hexc(c) for c in ("5a3d26", "4b321f", "6a4a2e", "3f2a1a")]
BARK_TOP = [hexc(c) for c in ("74593a", "6b5235")]
SAP = [hexc(c) for c in ("c27a12", "b06a0c", "d88f1f")]   # 07/11: mais escuro e saturado (aceso, o claro virava branco)
CRYSTAL = [hexc(c) for c in ("38f07a", "22d866", "6dffa2", "17b852")]
WATER = [hexc(c) for c in ("5fb8ec", "4aa6e0", "8fd0f6", "3d97d6", "d8f0ff")]   # 07/11: azul (o quase branco sumia na névoa)
FOAM = [hexc(c) for c in ("ffffff", "eef8ff", "dff2ff")]


class Grid:
    """Blocos de lado s (m): {(i, j, k): (cor, material)}."""

    def __init__(self, s, seed=1):
        self.s = s
        self.v = {}
        self.rng = random.Random(seed)

    def put(self, x, y, z, pal, mat=0):
        key = (int(math.floor(x / self.s)), int(math.floor(y / self.s)), int(math.floor(z / self.s)))
        self.v[key] = (self.rng.choice(pal), mat)

    def box(self, x0, x1, y0, y1, z0, z1, pal, mat=0):
        s = self.s
        for i in range(int(math.floor(x0 / s)), int(math.ceil(x1 / s))):
            for j in range(int(math.floor(y0 / s)), int(math.ceil(y1 / s))):
                for k in range(int(math.floor(z0 / s)), int(math.ceil(z1 / s))):
                    self.v[(i, j, k)] = (self.rng.choice(pal), mat)

    def ball(self, cx, cy, cz, r, pal, mat=0, squash=1.0):
        s = self.s
        n = int(math.ceil(r / s)) + 1
        bi, bj, bk = int(round(cx / s)), int(round(cy / s)), int(round(cz / s))
        for i in range(-n, n + 1):
            for j in range(-n, n + 1):
                for k in range(-n, n + 1):
                    if (i * i + j * j + (k / squash) ** 2) * s * s <= r * r:
                        self.v[(bi + i, bj + j, bk + k)] = (self.rng.choice(pal), mat)

    def mesh(self, name):
        bm = bmesh.new()
        col = bm.loops.layers.color.new("Col")
        s = self.s
        dirs = [((1, 0, 0), [(1, 0, 0), (1, 1, 0), (1, 1, 1), (1, 0, 1)]), ((-1, 0, 0), [(0, 0, 0), (0, 0, 1), (0, 1, 1), (0, 1, 0)]),
                ((0, 1, 0), [(0, 1, 0), (0, 1, 1), (1, 1, 1), (1, 1, 0)]), ((0, -1, 0), [(0, 0, 0), (1, 0, 0), (1, 0, 1), (0, 0, 1)]),
                ((0, 0, 1), [(0, 0, 1), (1, 0, 1), (1, 1, 1), (0, 1, 1)]), ((0, 0, -1), [(0, 0, 0), (0, 1, 0), (1, 1, 0), (1, 0, 0)])]
        cache = {}

        def vert(p):
            if p not in cache:
                cache[p] = bm.verts.new((p[0] * s, p[1] * s, p[2] * s))
            return cache[p]

        for (i, j, k), (c, m) in self.v.items():
            for (dx, dy, dz), corners in dirs:
                if (i + dx, j + dy, k + dz) in self.v:
                    continue
                shade = 1.0 if dz == 1 else .86 if dz == 0 else .7
                f = bm.faces.new([vert((i + a, j + b, k + cc)) for a, b, cc in corners])
                f.material_index = m
                for loop in f.loops:
                    loop[col] = (c[0] * shade, c[1] * shade, c[2] * shade, 1.0)
        me = bpy.data.meshes.new(name)
        bm.to_mesh(me)
        bm.free()
        # a cor tem que ser a camada ativa: senão o glTF manda um COLOR_0 branco (a Unreal lê esse)
        me.color_attributes.active_color = me.color_attributes["Col"]
        me.color_attributes.render_color_index = me.color_attributes.active_color_index
        obj = bpy.data.objects.new(name, me)
        bpy.context.collection.objects.link(obj)
        for mname in ("MB_block", "MB_glow", "MB_water"):
            mat = bpy.data.materials.get(mname) or bpy.data.materials.new(mname)
            obj.data.materials.append(mat)
        return obj


# ---- as peças --------------------------------------------------------------------------------------

def waterfall():
    # a água que cai (10 m, a Unreal estica até o lago): colunas de blocos de alturas e fundos diferentes
    g = Grid(.75, 3)
    for x in [i * .75 for i in range(-10, 11)]:
        depth = .75 + abs(math.sin(x * 1.7)) * 1.5
        for z in [k * .75 for k in range(-14, 0)]:
            wob = math.sin(z * .9 + x * 2.1) * .6
            g.box(x, x + .75, -depth + wob, wob, z, z + .75, WATER, 2)
    for x in [i * .75 for i in range(-11, 12)]:   # a borda de cima, que se curva
        g.box(x, x + .75, -1.5, 2.0, -.75, 0, FOAM, 2)
    return g


def waterfall_foam():
    g = Grid(.75, 4)
    for k in range(60):
        a = g.rng.uniform(0, 2 * math.pi)
        r = g.rng.uniform(0, 9)
        g.ball(math.cos(a) * r, math.sin(a) * r * .6 + 3, g.rng.uniform(0, 1.2), g.rng.uniform(.8, 1.8), FOAM, 2, .6)
    return g


def lookout():
    g = Grid(.5, 5)
    g.box(-5, 5, -5, 5, -1.5, .5, STONE)
    for x, y in ((-4, -4), (4, -4), (-4, 4), (4, 4)):
        h = 5.5 if (x, y) != (4, 4) else 3.0                     # uma coluna quebrada
        g.box(x - .75, x + .75, y - .75, y + .75, .5, .5 + h, STONE)
        g.box(x - 1, x + 1, y - 1, y + 1, .5 + h, 1.0 + h, MOSS)
    g.box(-4.5, 4.5, -4.5, -3.5, 6.0, 6.5, STONE)                  # a viga que sobrou
    g.box(-5, 5, 4, 4.5, .5, 1.5, STONE)                           # a mureta
    g.box(-1.5, 1.5, -1, -.25, .5, 1.25, SAND)                     # o banco
    return g


def stonering():
    g = Grid(.5, 6)
    g.ball(0, 0, -1.5, 16, MOSS, 0, .12)
    for k in range(12):
        a = k / 12 * 2 * math.pi
        x, y = math.cos(a) * 14, math.sin(a) * 14
        h = g.rng.uniform(4.5, 7.0) if k % 4 else 2.5               # alguns caídos ou partidos
        for z in [i * .5 for i in range(int(h / .5))]:
            t = 1 - z / h * .35
            g.box(x - .9 * t, x + .9 * t, y - .6 * t, y + .6 * t, z, z + .5, STONE)
        g.box(x - .9, x + .9, y - .6, y + .6, h - .5, h, MOSS)
        if k % 4 == 0:
            g.box(x + 1.2, x + 5, y - .6, y + .6, 0, 1, STONE)       # o pedaço caído
    g.box(-2, 2, -1.2, 1.2, 0, 1.5, SAND)                          # o altar
    g.box(-2.3, 2.3, -1.5, 1.5, 1.5, 2.0, STONE)
    for k in (1, 5):                                               # as vigas sobre dois pares
        a0, a1 = k / 12 * 2 * math.pi, (k + 1) / 12 * 2 * math.pi
        x0, y0, x1, y1 = math.cos(a0) * 14, math.sin(a0) * 14, math.cos(a1) * 14, math.sin(a1) * 14
        for t in [i / 10 for i in range(11)]:
            g.box(x0 + (x1 - x0) * t - .6, x0 + (x1 - x0) * t + .6, y0 + (y1 - y0) * t - .6, y0 + (y1 - y0) * t + .6, 6.5, 7.5, STONE)
    return g


def arch():
    # o arco natural: um semicírculo grosso de rocha sobre a passagem (22 m), as pernas afundando na crista
    g = Grid(1.0, 7)
    for t in [i / 80 for i in range(81)]:
        a = math.pi * t
        x, z = math.cos(a) * 14, math.sin(a) * 22 + 4
        th = 4.5 + 2 * math.sin(a * 3) * .5
        g.ball(0, x, z, th, SAND + STONE)
    for x in (-14, 14):
        g.box(-5, 5, x - 5, x + 5, -10, 6, SAND + STONE)
    for t in [i / 30 for i in range(31)]:                          # o musgo por cima
        a = math.pi * t
        g.ball(0, math.cos(a) * 14, math.sin(a) * 22 + 4 + 4.2, 1.6, MOSS)
    return g


def tower():
    g = Grid(.75, 8)
    for k in range(int(20 / .75)):
        z = k * .75
        top = 14 + 5 * abs(math.sin(z))                            # o alto quebrado
        for a in [i / 48 * 2 * math.pi for i in range(48)]:
            if z > 14 and (math.sin(a * 3 + 1) * 4 + 15) < z:
                continue
            if z < 3 and abs(a - math.pi / 2) < .35:               # a porta
                continue
            if 7 < z < 9 and abs(math.sin(a * 2)) < .2:             # as janelas
                continue
            g.put(math.cos(a) * 5, math.sin(a) * 5, z, STONE if z < 12 or g.rng.random() > .2 else MOSS)
    g.box(-5.5, 5.5, -5.5, 5.5, -1, 0, STONE)
    for k in range(10):                                            # as pedras caídas em volta
        a = g.rng.uniform(0, 6.28)
        g.ball(math.cos(a) * 8, math.sin(a) * 8, 0, g.rng.uniform(.6, 1.2), STONE)
    return g


def root_curve(g, p0, p1, p2, r0, r1, steps=90, veins=True):
    """Uma raiz pela curva (bézier quadrática), engrossando nas pontas; os veios de seiva acesos por cima."""
    for i in range(steps + 1):
        t = i / steps
        x = (1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0]
        y = (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1]
        z = (1 - t) ** 2 * p0[2] + 2 * (1 - t) * t * p1[2] + t * t * p2[2]
        r = r0 + (r1 - r0) * (1 - abs(2 * t - 1)) + .4 * math.sin(t * 17)
        g.ball(x, y, z, r, BARK)
        g.ball(x, y, z + r * .55, r * .55, BARK_TOP)
        if veins and i % 6 == 0:
            g.put(x, y + r * .7, z + r * .4, SAP, 1)
            g.put(x, y - r * .7, z + r * .3, SAP, 1)


def rootbridge():
    # a raiz enorme que atravessa o desfiladeiro (44 m), o lombo largo e chato para andar, e as raízes finas em volta
    g = Grid(1.0, 9)
    root_curve(g, (-34, 0, -8), (0, 0, 9), (34, 0, -8), 5.0, 3.6)
    for k in range(6):
        sx = g.rng.choice((-1, 1)) * g.rng.uniform(18, 30)
        root_curve(g, (sx, g.rng.uniform(-3, 3), -3), (sx * .8, g.rng.uniform(-9, 9), 3), (sx * .6, g.rng.uniform(-14, 14), -6), 1.4, .9, 40)
    return g


def root_arch():
    # a raiz que atravessa um rio (26 m): mais fina, torcida
    g = Grid(.75, 10)
    root_curve(g, (-18, -2, -4), (0, 3, 6), (18, 2, -4), 2.8, 2.0)
    root_curve(g, (-15, 3, -3), (0, -2, 4), (16, -3, -3), 1.4, 1.0, 60)
    return g


def root_emerge(variant):
    g = Grid(.75, 20 + variant)
    if variant == 0:     # o laço: sai do chão, faz um arco e entra de novo
        root_curve(g, (-9, 0, -3), (0, 0, 9), (9, 0, -3), 2.4, 1.8)
    elif variant == 1:   # a serpente: corre rente ao chão e levanta a ponta
        root_curve(g, (-14, -4, -2), (-2, 6, 1), (8, -2, 2), 2.2, 1.6)
        root_curve(g, (8, -2, 2), (12, -4, 5), (13, -6, 9), 1.6, .8, 30)
    else:                # o feixe: três raízes saindo juntas, pontas para cima
        for k in range(3):
            a = k / 3 * 2 * math.pi
            root_curve(g, (math.cos(a) * 2, math.sin(a) * 2, -3), (math.cos(a) * 6, math.sin(a) * 6, 3),
                       (math.cos(a) * 7, math.sin(a) * 7, 8 + k * 2), 2.0, 1.0, 50)
    return g


def crystals(small=False):
    g = Grid(.5 if small else .75, 30 + small)
    g.ball(0, 0, -1, 3 if small else 9, STONE + MOSS, 0, .35)
    n = 3 if small else 9
    for k in range(n):
        a = k / n * 2 * math.pi + g.rng.uniform(-.3, .3)
        d = 0 if k == 0 else g.rng.uniform(1, 2.5) if small else g.rng.uniform(2.5, 7)
        h = g.rng.uniform(1.5, 4) if small else g.rng.uniform(4, 12) * (1.3 if k == 0 else 1)
        rad = (.6 if small else 1.4) * (1.3 if k == 0 else 1)
        lean = g.rng.uniform(-.25, .25)
        x0, y0 = math.cos(a) * d, math.sin(a) * d
        steps = int(h / g.s)
        for i in range(steps):
            z = i * g.s
            t = 1 - max(0, (z - h * .75) / (h * .25))              # a ponta afina
            cx, cy = x0 + lean * z * math.cos(a), y0 + lean * z * math.sin(a)
            for hx in range(6):                                    # o prisma de seis lados
                ang = hx / 6 * 2 * math.pi
                for rr in (rad * t * .5, rad * t):
                    g.put(cx + math.cos(ang) * rr, cy + math.sin(ang) * rr, z, CRYSTAL, 1)
            g.put(cx, cy, z, CRYSTAL, 1)
    return g


PIECES = {
    "waterfall": waterfall, "waterfall_foam": waterfall_foam, "lookout": lookout, "stonering": stonering, "arch": arch,
    "tower": tower, "rootbridge": rootbridge, "root_arch": root_arch, "root_emerge_0": lambda: root_emerge(0),
    "root_emerge_1": lambda: root_emerge(1), "root_emerge_2": lambda: root_emerge(2), "crystals": crystals,
    "crystal_small": lambda: crystals(True),
}

if __name__ == "__main__":
    bpy.ops.wm.read_factory_settings(use_empty=True)
    os.makedirs(OUT, exist_ok=True)
    for name, make in PIECES.items():
        if ONLY and name not in ONLY:
            continue
        g = make()
        obj = g.mesh("SM_" + name)
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "SM_%s.glb" % name), use_selection=True, export_format="GLB",
                                  export_attributes=True, export_vertex_color="ACTIVE", export_all_vertex_colors=False,
                                  export_materials="EXPORT", export_yup=True)
        print("MARVEL %-16s %6d blocos  %6d faces" % (name, len(g.v), len(obj.data.polygons)))
        bpy.data.objects.remove(obj, do_unlink=True)
