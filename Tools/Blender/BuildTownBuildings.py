"""As construções novas da vila maior (03/11), em blocos de 12,5 cm como as do Godot (town_*.glb).

blender -b --factory-startup -P Tools/Blender/BuildTownBuildings.py -- <raiz do projeto> [nome ...]

Cada modelo é uma grade de blocos (numpy) pintada por padrões (tijolo, reboco, tábua, telha, palha, pano) e vira uma
malha só com as faces de fora, a cor de cada bloco na cor do vértice (o M_TerrainVertexColor da Unreal desenha).
Sai em Data/GodotTown/Models/<nome>.glb e as informações (caixas de colisão, alicerce e placa, no espaço do Godot:
x, y para cima, z; a frente no -Z) em Data/GodotTown/town_models_extra.json.

No Blender a frente é +Y e o chão é z = 0 (o Godot: x = x, y = z, z = -y).
Modelos: moinho (e as pás, que giram), capela, estábulo, quartel, mastro e três bandeiras (pano que balança),
barril, caixote, fardo de feno, boneco de treino e a horta.
"""
import json
import math
import os
import sys

import bpy
import numpy as np

ROOT = sys.argv[-1] if len(sys.argv) > 1 and os.path.isdir(sys.argv[-1]) else os.getcwd()
ONLY = [a for a in sys.argv[sys.argv.index("--") + 2:]] if "--" in sys.argv and len(sys.argv) > sys.argv.index("--") + 2 else []
OUT = os.path.join(ROOT, "Data", "GodotTown", "Models")
V = 0.125   # o bloco (m)


# ---- cores -----------------------------------------------------------------------------------------

def h(*a):
    """Ruído fixo de 0 a 1 por posição (sempre o mesmo para os mesmos números)."""
    x = 0
    for v in a:
        x = (x * 73856093) ^ (int(v) * 19349663 + 0x9E3779B9)
        x &= 0xFFFFFFFF
    x ^= x >> 13
    x = (x * 1274126177) & 0xFFFFFFFF
    return (x & 0xFFFF) / 65535.0


def tone(base, k, amount=.08):
    return tuple(max(0, min(255, int(c * (1 - amount + 2 * amount * k)))) for c in base)


def stone(i, j, k):
    # tijolos de pedra 4x2 blocos (deslocados a cada fileira) com junta escura e um pouco de musgo embaixo
    row = k // 2
    off = 2 if row % 2 else 0
    if k % 2 == 1 and h(i, j, k, 7) < .5:
        return tone((92, 92, 96), h(i, j, k), .05)
    brick = ((i + j + off) // 4, row)
    base = (150, 150, 152) if h(*brick, 3) < .7 else (128, 132, 128)
    if k < 6 and h(i, j, k, 9) < .18:
        base = (104, 128, 86)
    return tone(base, h(*brick, 5), .1)


def plaster(i, j, k):
    return tone((236, 222, 188), h(i, j, k), .04)


def beam(i, j, k):
    return tone((92, 60, 38), h(i // 2, j // 2, k), .08)


def planks(i, j, k):
    # tábuas em pé de 2 blocos, cada uma num tom
    board = (i + j) // 2
    base = (138, 96, 58)
    c = tone(base, h(board, 11), .14)
    if (k + board * 3) % 16 == 0:
        c = tone((100, 68, 42), 0.5)
    return c


def roof_dark(i, j, k):
    return tone((96, 76, 64) if (i // 2 + k) % 2 else (84, 66, 56), h(i // 2, j, k), .08)


def roof_red(i, j, k):
    return tone((172, 58, 46) if (i // 2 + k) % 2 else (150, 48, 40), h(i // 2, j, k), .08)


def roof_blue(i, j, k):
    return tone((70, 92, 140) if (i // 2 + k) % 2 else (60, 80, 124), h(i // 2, j, k), .08)


def hay(i, j, k):
    return tone((214, 182, 92) if h(i, j, k, 2) < .6 else (190, 154, 70), h(i, j, k), .06)


def dark(i, j, k):
    return (40, 30, 26)


def glass(i, j, k):
    return tone((70, 100, 130), h(i, j, k), .1)


def iron(i, j, k):
    return tone((60, 62, 66), h(i, j, k), .06)


def gold(i, j, k):
    return tone((222, 180, 70), h(i, j, k), .06)


def cloth(color):
    return lambda i, j, k: tone(color, h(i, j, k), .05)


def flat(color):
    return lambda i, j, k: color


# ---- a grade ---------------------------------------------------------------------------------------

class Grid:
    def __init__(self, sx, sy, sz, ox=None, oy=None):
        """sx, sy, sz em metros; a origem no meio da base (x, y) e no chão (z)."""
        self.n = (int(round(sx / V)), int(round(sy / V)), int(round(sz / V)))
        self.solid = np.zeros(self.n, bool)
        self.color = np.zeros(self.n + (3,), np.uint8)
        self.ox = -sx / 2 if ox is None else ox
        self.oy = -sy / 2 if oy is None else oy
        self.boxes = []
        self.shift = (0.0, 0.0, 0.0)   # move a malha pronta (as pás: o eixo na origem; a bandeira: o alto na origem)

    def idx(self, x, y, z):
        return (int(round((x - self.ox) / V)), int(round((y - self.oy) / V)), int(round(z / V)))

    def box(self, a, b, paint, collide=False, carve=False):
        """De a (x0, y0, z0) até b (x1, y1, z1), em metros."""
        i0, j0, k0 = self.idx(*a)
        i1, j1, k1 = self.idx(*b)
        i0, i1 = sorted((max(0, i0), min(self.n[0], i1)))
        j0, j1 = sorted((max(0, j0), min(self.n[1], j1)))
        k0, k1 = sorted((max(0, k0), min(self.n[2], k1)))
        for i in range(i0, i1):
            for j in range(j0, j1):
                for k in range(k0, k1):
                    if carve:
                        self.solid[i, j, k] = False
                    else:
                        self.solid[i, j, k] = True
                        self.color[i, j, k] = paint(i, j, k)
        if collide:
            self.boxes.append((a, b))

    def cylinder(self, cx, cy, r, z0, z1, paint, r1=None):
        """Um tronco de cone (ou cilindro) em blocos."""
        k0, k1 = int(round(z0 / V)), int(round(z1 / V))
        for k in range(max(0, k0), min(self.n[2], k1)):
            t = (k - k0) / max(1, k1 - k0 - 1)
            rr = r if r1 is None else r + (r1 - r) * t
            for i in range(self.n[0]):
                for j in range(self.n[1]):
                    x = self.ox + (i + .5) * V - cx
                    y = self.oy + (j + .5) * V - cy
                    if x * x + y * y <= rr * rr:
                        self.solid[i, j, k] = True
                        self.color[i, j, k] = paint(i, j, k)

    def gable(self, x0, x1, y0, y1, z0, rise, paint, ridge_paint=None):
        """Telhado de duas águas em degraus: a cumeeira ao longo de y, sobe rise metros do meio para as bordas em x."""
        half = (x1 - x0) / 2
        steps = int(round(half / V))
        for s in range(steps + 1):
            z = z0 + rise * s / max(1, steps)
            xa, xb = x0 + s * V, x1 - s * V
            if xa >= xb:
                break
            self.box((xa, y0, z), (xb, y1, z + V * 2), paint)
        if ridge_paint:
            self.box(((x0 + x1) / 2 - V, y0, z0 + rise), ((x0 + x1) / 2 + V, y1, z0 + rise + V * 2), ridge_paint)

    def mesh(self, name):
        s = self.solid
        verts, faces, cols = [], [], []
        dirs = [((1, 0, 0), 0, .86), ((-1, 0, 0), 0, .86), ((0, 1, 0), 1, .93), ((0, -1, 0), 1, .80),
                ((0, 0, 1), 2, 1.0), ((0, 0, -1), 2, .6)]
        padded = np.pad(s, 1)
        for (dx, dy, dz), axis, shade in dirs:
            neighbour = padded[1 + dx:1 + dx + s.shape[0], 1 + dy:1 + dy + s.shape[1], 1 + dz:1 + dz + s.shape[2]]
            exposed = s & ~neighbour
            for i, j, k in zip(*np.nonzero(exposed)):
                x0, y0, z0 = self.ox + i * V, self.oy + j * V, k * V
                c = self.color[i, j, k] / 255.0 * shade
                if axis == 0:
                    x = x0 + (V if dx > 0 else 0)
                    q = [(x, y0, z0), (x, y0 + V, z0), (x, y0 + V, z0 + V), (x, y0, z0 + V)]
                elif axis == 1:
                    y = y0 + (V if dy > 0 else 0)
                    q = [(x0, y, z0), (x0 + V, y, z0), (x0 + V, y, z0 + V), (x0, y, z0 + V)]
                else:
                    z = z0 + (V if dz > 0 else 0)
                    q = [(x0, y0, z), (x0 + V, y0, z), (x0 + V, y0 + V, z), (x0, y0 + V, z)]
                # a ordem dos cantos pela normal (para fora)
                normal = (dx, dy, dz)
                e1 = np.subtract(q[1], q[0])
                e2 = np.subtract(q[2], q[0])
                if np.dot(np.cross(e1, e2), normal) < 0:
                    q = q[::-1]
                base = len(verts)
                verts.extend(q)
                faces.append((base, base + 1, base + 2, base + 3))
                cols.append((c[0], c[1], c[2], 1.0))
        me = bpy.data.meshes.new(name)
        verts = [(v[0] + self.shift[0], v[1] + self.shift[1], v[2] + self.shift[2]) for v in verts]
        me.from_pydata(verts, [], faces)
        attr = me.color_attributes.new("Col", "BYTE_COLOR", "CORNER")
        flat_cols = np.repeat(np.array(cols, dtype=np.float32), 4, axis=0)
        # BYTE_COLOR guarda em sRGB: as cores acima já estão em sRGB, passam para linear para gravar certo
        lin = np.where(flat_cols[:, :3] <= .04045, flat_cols[:, :3] / 12.92, ((flat_cols[:, :3] + .055) / 1.055) ** 2.4)
        flat_cols[:, :3] = lin
        attr.data.foreach_set("color", flat_cols.ravel())
        # 03/11: a Unreal não importa a cor dos vértices destes glb (fica tudo escuro). A cor vai numa paleta: uma textura
        # pequena com cada cor num pixel, e cada face com o UV no centro do pixel dela (o mesmo visual de blocos)
        q = np.clip(np.round(np.array(cols)[:, :3] * 255), 0, 255).astype(np.int32)
        if len(np.unique(q, axis=0)) > 2048:
            q = (q // 6) * 6 + 3
        uniq, inv = np.unique(q, axis=0, return_inverse=True)
        # quadrada (até 64 x 64): numa linha só os UVs (que a Unreal guarda em meia precisão) caíam na cor vizinha. Cada
        # cor vai nos quatro cantos espelhados (em U e em V): vale com a textura virada ou não pela importação
        size = 4
        while (size // 2) * (size // 2) < len(uniq):
            size *= 2
        img = bpy.data.images.new(name + "_pal", size, size, alpha=False)
        pixels = np.ones((size, size, 4), np.float32)
        index = np.arange(len(uniq))
        half = size // 2
        col, row = index % half, index // half
        for r, c in ((row, col), (size - 1 - row, col), (row, size - 1 - col), (size - 1 - row, size - 1 - col)):
            pixels[r, c, :3] = uniq / 255.0
        img.pixels.foreach_set(pixels.ravel())
        img.file_format = "PNG"
        img.pack()
        uv = me.uv_layers.new(name="UVMap")
        texel = np.stack([(col + .5) / size, (row + .5) / size], axis=1)
        uv.data.foreach_set("uv", np.repeat(texel[inv.ravel()], 4, axis=0).astype(np.float32).ravel())
        me.validate()
        mat = bpy.data.materials.new("TownVoxel_" + name)
        mat.use_nodes = True
        nodes = mat.node_tree.nodes
        bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
        tex = nodes.new("ShaderNodeTexImage")
        tex.image = img
        tex.interpolation = "Closest"
        mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
        bsdf.inputs["Roughness"].default_value = .9
        me.materials.append(mat)
        obj = bpy.data.objects.new(name, me)
        bpy.context.scene.collection.objects.link(obj)
        return obj, len(faces)

    def info(self, sign=None):
        """Caixas, alicerce e placa no espaço do Godot (x, y para cima, z = -y do Blender)."""
        boxes = []
        for a, b in self.boxes:
            cx, cy, cz = (a[0] + b[0]) / 2, (a[1] + b[1]) / 2, (a[2] + b[2]) / 2
            sx, sy, sz = abs(b[0] - a[0]), abs(b[1] - a[1]), abs(b[2] - a[2])
            boxes.append([round(cx, 3), round(cz, 3), round(-cy, 3), round(sx, 3), round(sz, 3), round(sy, 3)])
        if boxes:
            x0 = min(bx[0] - bx[3] / 2 for bx in boxes)
            x1 = max(bx[0] + bx[3] / 2 for bx in boxes)
            z0 = min(bx[2] - bx[5] / 2 for bx in boxes)
            z1 = max(bx[2] + bx[5] / 2 for bx in boxes)
            footing = [round(x0, 3), round(z0, 3), round(x1, 3), round(z1, 3)]
        else:
            footing = []
        return {"boxes": boxes, "footing": footing, "sign": [round(sign[0], 3), round(sign[2], 3), round(-sign[1], 3)] if sign else None,
                "lamps": []}


# ---- os modelos -------------------------------------------------------------------------------------

def window(g, x, y_face, z, w=0.75, hgt=1.0, axis="y"):
    """Janela com moldura de madeira e vidro escuro, rente à parede (y_face: a face da parede)."""
    d = V
    if axis == "y":
        g.box((x - w / 2 - V, y_face - d, z - V), (x + w / 2 + V, y_face + d, z + hgt + V), beam)
        g.box((x - w / 2, y_face - d, z), (x + w / 2, y_face + d * 1.01, z + hgt), glass)
        g.box((x - V / 2, y_face - d, z), (x + V / 2, y_face + d * 1.02, z + hgt), beam)
    else:
        g.box((y_face - d, x - w / 2 - V, z - V), (y_face + d, x + w / 2 + V, z + hgt + V), beam)
        g.box((y_face - d * 1.01, x - w / 2, z), (y_face + d, x + w / 2, z + hgt), glass)


def door(g, x, y_face, w=1.25, hgt=2.25):
    g.box((x - w / 2 - V, y_face - V, 0), (x + w / 2 + V, y_face + V, hgt + V), beam)
    g.box((x - w / 2, y_face - V, 0), (x + w / 2, y_face + V * 1.01, hgt), planks)
    g.box((x + w / 2 - V * 2, y_face, hgt * .45), (x + w / 2 - V, y_face + V * 1.5, hgt * .5), iron)


def mill():
    g = Grid(7, 7, 11)
    g.cylinder(0, 0, 2.7, 0, 6.5, stone, r1=2.3)
    g.box((-1.6, -1.6, 0), (1.6, 1.6, 6.5), stone, collide=True)
    g.cylinder(0, 0, 2.5, 6.5, 8.5, planks, r1=2.3)
    g.cylinder(0, 0, 2.6, 8.5, 10.6, roof_dark, r1=.2)
    door(g, 0, 2.62, 1.25, 2.25)
    window(g, 0, 2.45, 4.0, .6, .8)
    # o eixo das pás na frente
    g.box((-.25, 2.2, 7.25), (.25, 3.1, 7.75), beam)
    return g, None


def mill_blades():
    # quatro pás de treliça e pano, em volta do eixo (origem), no plano x-z; giram em volta de y
    g = Grid(10, 1, 10, oy=-.5)
    g.ox = -5
    g.box((-.3, -.25, 4.7), (.3, .25, 5.3), beam)   # o cubo no meio (z = 5 é o eixo)
    for a in range(4):
        ang = a * math.pi / 2
        for s in np.arange(.3, 4.6, V):
            for w in np.arange(-.05, .85, V):
                x = math.cos(ang) * s - math.sin(ang) * w
                z = 5 + math.sin(ang) * s + math.cos(ang) * w
                frame = w < .1 or w > .7 or int(s / V) % 6 == 0
                p = beam if frame else cloth((236, 230, 214))
                g.box((x - V / 2, -.12 if frame else -.07, z - V / 2), (x + V / 2, .12 if frame else .07, z + V / 2), p)
    g.shift = (0.0, 0.0, -5.0)
    return g, None


def chapel():
    g = Grid(8, 14, 15)
    g.box((-3, -5, 0), (3, 4, 5), stone, collide=True)
    g.gable(-3.4, 3.4, -5.4, 4.4, 5, 3.6, roof_red, beam)
    # a torre do sino na frente
    g.box((-1.5, 3.6, 0), (1.5, 6.6, 9), stone, collide=True)
    g.box((-1.1, 3.9, 7), (1.1, 6.6, 8.5), dark, carve=True)
    g.box((-.4, 4.9, 7.2), (.4, 5.7, 8.1), gold)
    g.cylinder(0, 5.1, 2.0, 9, 13.5, roof_red, r1=.15)
    g.box((-.08, 5.0, 13.4), (.08, 5.2, 14.6), gold)
    g.box((-.5, 5.0, 14.0), (.5, 5.2, 14.15), gold)
    door(g, 0, 6.62, 1.5, 2.75)
    for y in (-3.5, -1, 1.5):
        window(g, y, 3.02, 2.2, .75, 1.6, axis="x")
        window(g, y, -3.02, 2.2, .75, 1.6, axis="x")
    return g, (0, 6.7, 3.4)


def stable():
    g = Grid(10, 7, 6)
    for x in (-4.5, -1.5, 1.5, 4.5):
        for y in (-2.75, 2.75):
            g.box((x - .2, y - .2, 0), (x + .2, y + .2, 3.2), beam, collide=True)
    g.box((-4.7, -2.95, 0), (4.7, -2.55, 1.6), planks, collide=True)   # o fundo e os lados em meia altura
    g.box((-4.7, -2.95, 0), (-4.3, 2.95, 1.6), planks, collide=True)
    g.box((4.3, -2.95, 0), (4.7, 2.95, 1.6), planks, collide=True)
    for x in (-3, 0, 3):
        g.box((x - .1, -2.55, 0), (x + .1, 1.0, 1.3), planks)   # baias
    g.gable(-5.0, 5.0, -3.3, 3.3, 3.2, 2.0, roof_dark, beam)
    for x, y in ((-3.6, -1.6), (-3.6, -0.6), (3.4, -1.8), (1.4, -1.9)):
        g.box((x - .6, y - .4, 0), (x + .6, y + .4, .7), hay)
    g.box((-.6, -2.4, 0), (.6, -1.6, .5), planks)   # cocho
    g.box((-.5, -2.3, .4), (.5, -1.7, .5), flat((80, 120, 170)))
    return g, (0, 3.0, 3.6)


def barracks():
    g = Grid(14, 8, 8)
    g.box((-6, -3, 0), (6, 3, 1.75), stone, collide=True)
    g.box((-6, -3, 1.75), (6, 3, 4.5), plaster, collide=True)
    for x in np.arange(-6, 6.01, 2):
        g.box((x - .15, -3.1, 1.75), (x + .15, 3.1, 4.5), beam)
    g.box((-6.1, -3.1, 1.75), (6.1, 3.1, 1.95), beam)
    g.box((-6.1, -3.1, 4.4), (6.1, 3.1, 4.6), beam)
    g.gable(-6.6, 6.6, -3.5, 3.5, 4.5, 2.5, roof_blue, beam)
    door(g, 0, 3.02, 1.5, 2.5)
    for x in (-4, -2, 2, 4):
        window(g, x, 3.02, 2.6, .75, 1.0)
        window(g, x, -3.02, 2.6, .75, 1.0)
    # o escudo de armas em cima da porta
    g.box((-.5, 3.05, 2.8), (.5, 3.3, 3.8), flat((60, 80, 150)))
    g.box((-.2, 3.1, 3.05), (.2, 3.35, 3.55), gold)
    return g, (0, 3.2, 4.0)


def banner_pole():
    g = Grid(2, 1, 7)
    g.box((-.12, -.12, 0), (.12, .12, 6.5), beam, collide=True)
    g.box((-.18, -.18, 6.5), (.18, .18, 6.85), gold)
    g.box((-.12, -.08, 6.0), (1.5, .08, 6.12), beam)   # a travessa de onde o pano pende
    return g, None


def banner(color, emblem):
    # o pano: pende de z = 0 para baixo (a origem no alto); o material balança mais embaixo
    g = Grid(1.25, .25, 2.5, ox=0, oy=-.125)
    g.box((0, -.0625, 0), (1.25, .0625, 2.5), cloth(color))
    g.box((0, -.0625, 0), (.125, .0625, 2.5), gold)
    g.box((1.125, -.0625, 0), (1.25, .0625, 2.5), gold)
    g.box((.375, -.0625, .9), (.875, .0625, 1.6), cloth(emblem))
    g.shift = (0.0, 0.0, -2.5)
    return g, None


def barrel():
    g = Grid(1, 1, 1.125)
    g.cylinder(0, 0, .42, 0, 1.0, lambda i, j, k: tone((136, 88, 50), h(i * 3 + j, 4), .14), r1=.42)
    g.cylinder(0, 0, .46, .375, .625, lambda i, j, k: tone((128, 82, 46), h(i * 3 + j, 4), .14))
    for z in (.125, .75):
        g.cylinder(0, 0, .44, z, z + .125, iron)
    g.cylinder(0, 0, .36, 1.0, 1.06, lambda i, j, k: tone((120, 80, 46), h(i, j, 2), .1))
    return g, None


def crate():
    g = Grid(.75, .75, .75)
    g.box((-.375, -.375, 0), (.375, .375, .75), planks)
    for a, b in (((-.375, -.39, 0), (.375, -.36, .125)), ((-.375, -.39, .625), (.375, -.36, .75))):
        g.box(a, b, beam)
    g.box((-.375, -.375, 0), (-.25, .375, .75), beam)
    g.box((.25, -.375, 0), (.375, .375, .75), beam)
    return g, None


def hay_bale():
    g = Grid(1.25, .75, .625)
    g.box((-.625, -.375, 0), (.625, .375, .625), hay)
    for x in (-.3, .3):
        g.box((x - .0625, -.38, 0), (x + .0625, .38, .63), flat((150, 110, 60)))
    return g, None


def dummy():
    g = Grid(1.5, .75, 2.25)
    g.box((-.08, -.08, 0), (.08, .08, 2.0), beam, collide=True)
    g.box((-.25, -.18, .9), (.25, .18, 1.65), hay)
    g.box((-.18, -.15, 1.65), (.18, .15, 2.05), cloth((220, 200, 160)))
    g.box((-.7, -.07, 1.35), (.7, .07, 1.47), beam)
    g.box((-.07, .13, 1.8), (.07, .2, 1.9), dark)
    return g, None


def crops():
    # uma horta de 6 x 4 m: fileiras de trigo e de verduras com terra no meio
    g = Grid(6, 4, .75)
    g.box((-3, -2, 0), (3, 2, .125), flat((96, 70, 46)))
    for r, y in enumerate(np.arange(-1.75, 1.8, .5)):
        for x in np.arange(-2.875, 2.9, .25):
            tall = .5 + .2 * h(int(x * 8), int(y * 8))
            if r % 2:
                g.box((x - .07, y - .1, .125), (x + .07, y + .1, .125 + tall), lambda i, j, k: tone((214, 186, 90), h(i, j, k), .1))
            else:
                g.box((x - .1, y - .12, .125), (x + .1, y + .12, .375), lambda i, j, k: tone((80, 150, 60), h(i, j, k), .15))
    return g, None


MODELS = {
    "town_mill": mill, "town_mill_blades": mill_blades, "town_chapel": chapel, "town_stable": stable,
    "town_barracks": barracks, "town_banner_pole": banner_pole,
    "town_banner_red": lambda: banner((170, 40, 40), (240, 200, 80)),
    "town_banner_blue": lambda: banner((50, 70, 150), (230, 230, 230)),
    "town_banner_green": lambda: banner((50, 120, 60), (240, 200, 80)),
    "town_barrel": barrel, "town_crate": crate, "town_hay": hay_bale, "town_dummy": dummy, "town_crops": crops,
}


def main():
    os.makedirs(OUT, exist_ok=True)
    info_path = os.path.join(ROOT, "Data", "GodotTown", "town_models_extra.json")
    infos = json.load(open(info_path, encoding="utf-8")) if os.path.exists(info_path) else {}
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for name, build in MODELS.items():
        if ONLY and name not in ONLY:
            continue
        g, sign = build()
        obj, faces = g.mesh(name)
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, name + ".glb"), use_selection=True, export_format="GLB",
                                  export_vertex_color="ACTIVE", export_all_vertex_colors=True)
        infos[name] = g.info(sign)
        print("TOWN_MODEL", name, faces, "faces", [round(d, 2) for d in obj.dimensions])
        bpy.data.objects.remove(obj, do_unlink=True)
    json.dump(infos, open(info_path, "w", encoding="utf-8"), indent=1)


main()
