"""O kit da vila nova (05/11): casas, prédios, muralha, portão, ruas de paralelepípedo, lajes, terra, moinho e horta,
pelas referências de `modelando ainda/Futuras modelagens/Cidade`, com as texturas de Tools/Town/MakeVilaTextures.py.

Sem janela:   blender -b --factory-startup -P Tools/Blender/BuildVilaKit.py -- <raiz> [modelo ...]
No Blender aberto (MCP, para ver): ONLY = ["vila_casa_2"]; exec(open(".../BuildVilaKit.py").read())
  (monta na cena, em fila, sem exportar)

Sai Data/GodotTown/VilaKit/<modelo>.fbx (um material por textura, MV_<nome>, e a cor de cada peça na cor do vértice)
e Data/GodotTown/vila_models.json: as caixas de colisão, o alicerce, a placa e os lampiões de cada modelo no espaço do
Godot (x, y para cima, z; a frente no -z), que o Tools/Town/BuildVila.py junta na planta.
No Blender a frente (a porta) é +Y e o chão é z = 0 (Godot: x = x, y = z, z = -y).

As peças são blocos (caixas e prismas) com volume, no estilo das referências: alicerce de pedra, paredes de reboco com
o enxaimel de madeira escura (postes, travessas e mãos-francesas), janelas com moldura, vidro, peitoril, venezianas e
jardineiras, portas de tábua com dobradiças, telhados com fileiras de telha em degrau, cumeeira, chaminés de tijolo,
sacadas, águas-furtadas, toldos, placas e lampiões. As de entrar (vila_casa_2, vila_casa_8, vila_taverna) têm a porta
aberta, o piso de tábua, móveis e as caixas de colisão só nas paredes (com o vão da porta).
"""
import json
import math
import os
import random
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
ROOT = ARGS[0] if ARGS and os.path.isdir(ARGS[0]) else r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
HEADLESS = bpy.app.background
ONLY = (ARGS[1:] if HEADLESS else globals().get("ONLY", [])) or []
TEX = os.path.join(ROOT, "Data", "GodotTown", "VilaTex")
OUT = os.path.join(ROOT, "Data", "GodotTown", "VilaKit")
INFO = os.path.join(ROOT, "Data", "GodotTown", "vila_models.json")

# a textura e quantos metros ela cobre no mundo
MATS = {
    # 06/11: as do Poly Haven (Tools/Town/MakeVilaTextures.py), no tamanho real delas
    "PedraClara": 2.0, "PedraMuralha": 2.5, "PedraBruta": 1.2, "PedraRua": 1.2, "Calcamento": 1.5, "Reboco": 2.0,
    "MadeiraEscura": 1.5, "MadeiraClara": 2.0, "Madeira": 1.5, "TelhaBarro": 2.0, "TelhaMadeira": 2.0, "Palha": 2.0,
    "Tijolo": 1.5, "Terra": 2.5, "Ferro": 1.0, "Tecido": 1.2, "Vidro": 1.0, "Folha": 1.2, "Brasa": 1.0, "Pano": 2.0,
    "Lanterna": 1.0, "Agua": 2.0,
    # 07/11: os dos móveis (Poly Haven: Tools/Town/MakeVilaTextures.py Piso Movel ...; Tools/Town/FixInteriorTextures.py)
    "Piso": 2.0, "Movel": 1.2, "Colcha": 1.0, "Tapete": 1.0, "Lareira": 1.4, "Couro": 1.0, "Livros": .45, "PedraPiso": 2.0,
    "RuaGrama": 2.0, "Placa": 1.0, "TelhaLisa": 1.0,
}


# o brilho de cada textura (multiplica a cor; a Unreal faz igual, o "Bright" do MI_Vila_<nome>): o reboco do Poly Haven
# é marrom-escuro, e as referências pedem creme
BRIGHT = {"Reboco": 1.75, "PedraBruta": 1.6, "PedraRua": 1.15, "MadeiraEscura": 1.0, "PedraClara": 1.1, "Terra": 1.1}


def log(*a):
    print("VILA_KIT", *a)


# ==================================================================================================
# a malha: caixas e sólidos de 8 pontos, cada um com material, cor e (se quiser) colisão
# ==================================================================================================
class Kit:
    def __init__(self, name, seed=0):
        self.name = name
        self.bm = bmesh.new()
        self.mats = []
        self.part = self.bm.faces.layers.int.new("part")
        self.parts = 0
        self.tints = {}
        self.boxes = []          # colisão (Blender: x0, y0, z0, x1, y1, z1)
        self.lamps = []          # lampiões (Blender)
        self.sign = None         # a placa (Blender)
        self.chimneys = []       # o alto de cada chaminé (Blender): a vila põe a fumaça ali
        self.inner = []          # 07/11: as luzes de dentro (sempre acesas, fracas): o interior se vê pela janela
        self.doors = []          # 07/11: as portas de abrir (E): (x, y da dobradiça, z do piso, largura, altura), Blender
        self.rng = random.Random(hash(name) & 0xFFFF ^ seed)

    def mat(self, name):
        if name not in self.mats:
            self.mats.append(name)
        return self.mats.index(name)

    def solid(self, pts, mat, tint=(1, 1, 1)):
        """8 pontos: os 4 de baixo e os 4 de cima, na mesma ordem."""
        v = [self.bm.verts.new(p) for p in pts]
        idx = self.mat(mat)
        self.parts += 1
        self.tints[self.parts] = tint
        for q in ((0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1), (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0)):
            try:
                f = self.bm.faces.new([v[i] for i in q])
            except ValueError:
                continue
            f.material_index = idx
            f[self.part] = self.parts

    def box(self, x0, y0, z0, x1, y1, z1, mat, tint=(1, 1, 1), collide=False, rot=None, pivot=None):
        if x0 > x1: x0, x1 = x1, x0
        if y0 > y1: y0, y1 = y1, y0
        if z0 > z1: z0, z1 = z1, z0
        pts = [Vector(p) for p in ((x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0), (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1))]
        if rot is not None:
            pv = Vector(pivot) if pivot is not None else sum(pts, Vector()) / 8
            pts = [rot @ (p - pv) + pv for p in pts]
        self.solid(pts, mat, tint)
        if collide and rot is None:
            self.boxes.append((x0, y0, z0, x1, y1, z1))

    def collide(self, x0, y0, z0, x1, y1, z1):
        self.boxes.append((min(x0, x1), min(y0, y1), min(z0, z1), max(x0, x1), max(y0, y1), max(z0, z1)))

    def finish(self):
        """Normais para fora, UV pela posição (cada material no tamanho dele, cada peça com um deslocamento) e a cor."""
        bm = self.bm
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        uv = bm.loops.layers.uv.new("UV")
        col = bm.loops.layers.color.new("Col")
        Z = Vector((0, 0, 1))
        for f in bm.faces:
            n = f.normal
            if abs(n.z) > .98:
                u_ax, v_ax = Vector((1, 0, 0)), Vector((0, 1, 0))
            else:
                u_ax = Z.cross(n).normalized()
                v_ax = n.cross(u_ax).normalized()
            tile = MATS[self.mats[f.material_index]]
            p = f[self.part]
            r = random.Random(p * 7919 + 13)
            off = (r.random(), r.random()) if self.mats[f.material_index] != "Livros" else (0.0, 0.0)
            t = self.tints.get(p, (1, 1, 1))
            for loop in f.loops:
                co = loop.vert.co
                loop[uv].uv = (co.dot(u_ax) / tile + off[0], co.dot(v_ax) / tile + off[1])
                loop[col] = (t[0], t[1], t[2], 1.0)
        mesh = bpy.data.meshes.new(self.name)
        bm.to_mesh(mesh)
        bm.free()
        for m in self.mats:
            mesh.materials.append(material(m))
        obj = bpy.data.objects.new(self.name, mesh)
        return obj


_mat_cache = {}


def material(name):
    key = "MV_" + name
    if key in bpy.data.materials:
        return bpy.data.materials[key]
    m = bpy.data.materials.new(key)
    m.use_nodes = True
    nt = m.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    tex = nt.nodes.new("ShaderNodeTexImage")
    path = os.path.join(TEX, "T_Vila_%s.png" % name)
    if os.path.exists(path):
        tex.image = bpy.data.images.load(path, check_existing=True)
    vc = nt.nodes.new("ShaderNodeVertexColor")
    vc.layer_name = "Col"
    mix = nt.nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.blend_type = "MULTIPLY"
    mix.inputs[0].default_value = 1.0
    nt.links.new(tex.outputs["Color"], mix.inputs[6])
    nt.links.new(vc.outputs["Color"], mix.inputs[7])
    scale = nt.nodes.new("ShaderNodeVectorMath")
    scale.operation = "SCALE"
    scale.inputs["Scale"].default_value = BRIGHT.get(name, 1.0)
    nt.links.new(mix.outputs[2], scale.inputs[0])
    nt.links.new(scale.outputs[0], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = .85
    if name == "Brasa":
        nt.links.new(tex.outputs["Color"], bsdf.inputs["Emission Color"])
        bsdf.inputs["Emission Strength"].default_value = 3.0
    npath = os.path.join(TEX, "T_Vila_%s_N.png" % name)
    if os.path.exists(npath):
        ntex = nt.nodes.new("ShaderNodeTexImage")
        ntex.image = bpy.data.images.load(npath, check_existing=True)
        ntex.image.colorspace_settings.name = "Non-Color"
        nmap = nt.nodes.new("ShaderNodeNormalMap")
        nt.links.new(ntex.outputs["Color"], nmap.inputs["Color"])
        nt.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
    return m


# ==================================================================================================
# peças de construção (no espaço da casa: a frente em +Y, largura em X, fundo em Y, chão z = 0)
# ==================================================================================================
def rgb(r, g, b):
    return (r / 255, g / 255, b / 255)


class House:
    """Uma construção: guarda largura (w), fundo (d) e as aberturas de cada fachada (para o enxaimel não passar por cima)."""

    def __init__(self, k, w, d):
        self.k, self.w, self.d = k, w, d
        self.open = {"front": [], "back": [], "left": [], "right": []}
        self.holes = {"front": [], "back": [], "left": [], "right": []}   # 07/11: furos de verdade (vidro e porta)

    def half(self, face):
        return self.w / 2 if face in ("front", "back") else self.d / 2

    def fbox(self, face, u0, u1, z0, z1, o0, o1, mat, tint=(1, 1, 1), collide=False, D=None):
        """Caixa na fachada: u ao longo dela, o para fora a partir da parede (D = a distância do centro à parede)."""
        D = self.half_other(face) if D is None else D
        if face == "front":
            self.k.box(u0, D + o0, z0, u1, D + o1, z1, mat, tint, collide)
        elif face == "back":
            self.k.box(-u1, -D - o1, z0, -u0, -D - o0, z1, mat, tint, collide)
        elif face == "right":
            self.k.box(D + o0, -u1, z0, D + o1, -u0, z1, mat, tint, collide)
        else:
            self.k.box(-D - o1, u0, z0, -D - o0, u1, z1, mat, tint, collide)

    def half_other(self, face):
        return self.d / 2 if face in ("front", "back") else self.w / 2

    # ---- partes ------------------------------------------------------------------------------------
    def foundation(self, h=.35, out=.15, mat="PedraClara", tint=(1, 1, 1)):
        # 06/11 (o usuário: "degraus das casas muito altos para entrar" e "atravesso o chão"): o piso fica a no máximo
        # 20 cm do chão e o alicerce inteiro tem colisão (antes o piso de dentro não tinha: o herói afundava até a terra)
        h = min(h, .2)
        w, d = self.w / 2 + out, self.d / 2 + out
        self.k.box(-w, -d, -.4, w, d, h, mat, tint)
        self.k.collide(-w, -d, -.4, w, d, h)
        return h

    def wall_face(self, face, u0, u1, z0, z1, thick, mat, tint, inset=0.0):
        """07/11 (o usuário: "os vidros das casas serem vidros mesmo"): a parede de uma fachada em faixas, com os furos
        das janelas e da porta abertos (o vidro fica no meio da espessura)."""
        holes = [(max(a, u0), min(b, u1), max(c, z0), min(e, z1)) for a, b, c, e in self.holes[face] if a < u1 and b > u0 and c < z1 and e > z0]
        cuts = sorted({u0, u1} | {h[0] for h in holes} | {h[1] for h in holes})
        D = self.half_other(face) - inset
        for a, b in zip(cuts, cuts[1:]):
            if b - a < .005:
                continue
            spans = sorted((h[2], h[3]) for h in holes if h[0] < (a + b) / 2 < h[1])
            z = z0
            for c, e in spans + [(z1, z1)]:
                if c - z > .005:
                    self.fbox(face, a, b, z, c, -thick, 0, mat, tint, D=D)
                z = max(z, e)

    def walls(self, z0, z1, mat="Reboco", tint=(1, 1, 1), thick=.25, inset=0.0, door=None, collide=True):
        """As 4 paredes (com espessura), com os furos das janelas. door = (u, largura, altura) na frente: o vão aberto."""
        w, d = self.w / 2 - inset, self.d / 2 - inset
        k = self.k
        if door is not None:
            u, dw, dh = door
            self.holes["front"].append((u - dw / 2, u + dw / 2, z0 - .5, z0 + dh))
        self.wall_face("front", -w, w, z0, z1, thick, mat, tint, inset)
        self.wall_face("back", -w, w, z0, z1, thick, mat, tint, inset)
        self.wall_face("left", -d + thick, d - thick, z0, z1, thick, mat, tint, inset)
        self.wall_face("right", -d + thick, d - thick, z0, z1, thick, mat, tint, inset)
        if collide:
            if door is None:
                k.collide(-w, -d, 0, w, d, z1)
            else:
                u, dw, dh = door
                k.collide(-w, -d, 0, w, -d + thick, z1)
                k.collide(-w, -d, 0, -w + thick, d, z1)
                k.collide(w - thick, -d, 0, w, d, z1)
                k.collide(-w, d - thick, 0, u - dw / 2, d, z1)
                k.collide(u + dw / 2, d - thick, 0, w, d, z1)

    def timber(self, z0, z1, rails=(), braces=True, tint=rgb(86, 60, 44), out=.06, faces=("front", "back", "left", "right"), spacing=1.5):
        """O enxaimel: postes, travessas e mãos-francesas por fora da parede, desviando das janelas e portas."""
        b = .14
        for face in faces:
            L = self.half(face)
            n = max(2, int(round(2 * L / spacing)) + 1)
            us = [-L + b / 2 + i * (2 * L - b) / (n - 1) for i in range(n)]
            free_u = []
            for u in us:
                if any(o[0] - b < u < o[1] + b and o[2] < z1 and o[3] > z0 for o in self.open[face]):
                    continue
                self.fbox(face, u - b / 2, u + b / 2, z0, z1, 0, out, "MadeiraEscura", tint)
                free_u.append(u)
            for z in (z0 + .07, z1 - .07) + tuple(rails):
                self.fbox(face, -L, L, z - b / 2, z + b / 2, 0, out + .01, "MadeiraEscura", tint)
            if braces:
                for a, c in zip(free_u, free_u[1:]):
                    if c - a > spacing * 1.6 or self.k.rng.random() < .55:
                        continue
                    if any(o[0] < c and o[1] > a and o[2] < z1 and o[3] > z0 for o in self.open[face]):
                        continue
                    # a mão-francesa: uma tábua inclinada no painel
                    mid_u, mid_z = (a + c) / 2, (z0 + z1) / 2
                    ang = math.atan2(z1 - z0, c - a) * (1 if self.k.rng.random() < .5 else -1)
                    length = math.hypot(c - a, z1 - z0)
                    self._brace(face, mid_u, mid_z, length, ang, b * .8, out, tint)

    def _brace(self, face, u, z, length, ang, b, out, tint):
        D = self.half_other(face)
        # a tábua no plano da fachada, girada em volta da normal dela
        if face == "front":
            c, axis = Vector((u, D + out / 2, z)), "Y"
        elif face == "back":
            c, axis = Vector((-u, -D - out / 2, z)), "Y"
        elif face == "right":
            c, axis = Vector((D + out / 2, -u, z)), "X"
        else:
            c, axis = Vector((-D - out / 2, u, z)), "X"
        if axis == "Y":
            self.k.box(c.x - length / 2, c.y - out / 2, c.z - b / 2, c.x + length / 2, c.y + out / 2, c.z + b / 2, "MadeiraEscura", tint,
                       rot=Matrix.Rotation(-ang if face == "front" else ang, 3, "Y"), pivot=c)
        else:
            self.k.box(c.x - out / 2, c.y - length / 2, c.z - b / 2, c.x + out / 2, c.y + length / 2, c.z + b / 2, "MadeiraEscura", tint,
                       rot=Matrix.Rotation(ang, 3, "X"), pivot=c)

    def window(self, face, u, z, w=.9, h=1.2, shutters=True, flowers=False, arch=False, tint=rgb(92, 64, 46)):
        self.open[face].append((u - w / 2 - .1, u + w / 2 + .1, z - .1, z + h + .1))
        self.holes[face].append((u - w / 2, u + w / 2, z, z + h))
        f = .09
        self.fbox(face, u - w / 2, u + w / 2, z, z + h, -.135, -.115, "Vidro")      # no meio da parede (o furo)
        for a, c in ((u - w / 2 - f, u - w / 2), (u + w / 2, u + w / 2 + f)):
            self.fbox(face, a, c, z - f, z + h + f, 0, .1, "MadeiraEscura", tint)
        self.fbox(face, u - w / 2 - f, u + w / 2 + f, z + h, z + h + f, 0, .1, "MadeiraEscura", tint)
        self.fbox(face, u - .03, u + .03, z, z + h, -.11, .03, "MadeiraEscura", tint)       # o caixilho em cruz
        self.fbox(face, u - w / 2, u + w / 2, z + h * .55, z + h * .55 + .05, -.11, .03, "MadeiraEscura", tint)
        self.fbox(face, u - w / 2 - .14, u + w / 2 + .14, z - f - .06, z - f, 0, .2, "PedraClara", rgb(230, 225, 215))   # peitoril
        if arch:
            self.fbox(face, u - w / 2 - f, u + w / 2 + f, z + h + f, z + h + f + .18, 0, .1, "PedraClara")
        if shutters:
            sc = self.k.rng.choice((rgb(70, 110, 76), rgb(118, 70, 52), rgb(64, 84, 118), rgb(150, 116, 74)))
            for a, c in ((u - w / 2 - f - w / 2, u - w / 2 - f), (u + w / 2 + f, u + w / 2 + f + w / 2)):
                self.fbox(face, a, c, z, z + h, .02, .07, "MadeiraClara", sc)
                self.fbox(face, a + .04, c - .04, z + h * .3, z + h * .3 + .05, .07, .09, "MadeiraEscura", tint)
        if flowers:
            self.fbox(face, u - w / 2, u + w / 2, z - .42, z - .16, .05, .32, "MadeiraClara", rgb(130, 90, 60))
            for i in range(5):
                cu = u - w / 2 + .1 + i * (w - .2) / 4
                s = self.k.rng.uniform(.1, .16)
                self.fbox(face, cu - s, cu + s, z - .16, z - .16 + s * 1.6, .08, .3, "Folha")
                if self.k.rng.random() < .6:
                    fc = self.k.rng.choice((rgb(220, 70, 80), rgb(240, 200, 80), rgb(230, 230, 240), rgb(170, 90, 200)))
                    self.fbox(face, cu - .04, cu + .04, z - .16 + s * 1.5, z - .16 + s * 1.5 + .07, .14, .22, "Tecido", fc)

    def door(self, face, u, z, w=1.2, h=2.2, open_=False, arch=False, tint=rgb(96, 64, 44)):
        self.open[face].append((u - w / 2 - .15, u + w / 2 + .15, z, z + h + .2))
        f = .14
        for a, c in ((u - w / 2 - f, u - w / 2), (u + w / 2, u + w / 2 + f)):
            self.fbox(face, a, c, z, z + h + f, 0, .14, "MadeiraEscura", tint)
        self.fbox(face, u - w / 2 - f, u + w / 2 + f, z + h, z + h + f + .06, 0, .16, "MadeiraEscura", tint)
        if arch:
            self.fbox(face, u - w / 2 - f - .1, u + w / 2 + f + .1, z + h + f, z + h + f + .3, 0, .18, "PedraClara")
        if z < .5:
            # o degrau: metade da altura do piso (com colisão, para subir em dois passos baixos)
            self.fbox(face, u - w / 2 - .25, u + w / 2 + .25, -.1, max(z * .5, .02), 0, .55, "PedraClara", rgb(200, 196, 188), collide=True)
        else:
            self.fbox(face, u - w / 2 - .25, u + w / 2 + .25, z - .2, z, 0, .55, "PedraClara", rgb(200, 196, 188))   # a soleira (sacada)
        if open_:
            # 07/11 (o usuário: "as portas, apertar uma tecla para abrir"): a folha é do jogo (AVortexTown, o modelo
            # vila_porta), presa na dobradiça da esquerda no meio da parede, e abre e fecha com o E
            D = self.half_other(face)
            if face == "front":
                self.k.doors.append((u - w / 2, D - .13, z, w, h))
            return
        self.fbox(face, u - w / 2, u + w / 2, z, z + h, -.02, .06, "Madeira", tint)
        for zz in (z + .35, z + h - .45):
            self.fbox(face, u - w / 2 + .05, u + w / 2 - .05, zz, zz + .08, .06, .09, "Ferro", rgb(120, 120, 124))
        self.fbox(face, u + w / 2 - .22, u + w / 2 - .14, z + h * .45, z + h * .45 + .12, .06, .12, "Ferro", rgb(150, 140, 110))

    def gable_roof(self, z, rise, along="x", over=.45, mat="TelhaBarro", tint=(1, 1, 1), thick=.16, steps=True, gable_mat="Reboco",
                   gable_tint=(1, 1, 1), timber_gable=True):
        """Telhado de duas águas: along = a direção da cumeeira. Fileiras de telha em degrau, cumeeira e o oitão."""
        k = self.k
        if along == "x":
            L, D = self.w / 2 + over * .7, self.d / 2
        else:
            L, D = self.d / 2 + over * .7, self.w / 2
        e = D + over
        zr = z + rise
        drop = rise * over / D
        ze = z - drop

        def P(a, b, c):        # (ao longo da cumeeira, através, altura) -> mundo
            return (a, b, c) if along == "x" else (b, a, c)
        for s in (1, -1):
            pts = [P(-L, s * e, ze), P(L, s * e, ze), P(L, 0, zr), P(-L, 0, zr)]
            top = [P(-L, s * e, ze + thick), P(L, s * e, ze + thick), P(L, 0, zr + thick), P(-L, 0, zr + thick)]
            k.solid([Vector(p) for p in pts + top], mat, tint)
            if steps:
                # 07/11 (o usuário: "os telhados parecem uma coisa reta, sem textura nem nível, muito chapado"): telha
                # por telha. Cada uma é uma cunha deitada na água (a ponta de baixo levantada faz a sombra da fileira),
                # com o seu tom; barro: capa e canal alternados; madeira: tábuas desencontradas; palha: camadas grossas
                # com a beira irregular
                rng = k.rng
                slope = math.hypot(e, zr - ze)
                kind = "palha" if mat == "Palha" else "barro" if mat == "TelhaBarro" else "madeira"
                row = {"barro": .36, "madeira": .3, "palha": .5}[kind]
                n = max(2, int(slope / row))

                def S(a_, t_, h_):
                    return Vector(P(a_, s * e * t_, zr + (ze - zr) * t_ + thick + h_))
                for i in range(n):
                    t0, t1 = i / n, min(1.0, (i + 1) / n + .06)
                    a_ = -L
                    stagger = (i % 2) * .5
                    col = 0
                    while a_ < L - .01:
                        if kind == "barro":
                            wid = .28
                        elif kind == "madeira":
                            wid = rng.uniform(.17, .32)
                        else:
                            wid = rng.uniform(.9, 1.5)
                        if col == 0 and kind != "barro":
                            wid *= .5 + stagger
                        a1 = min(L, a_ + wid)
                        if kind == "barro":
                            capa = col % 2 == 0
                            top, low = (.03, .085) if capa else (.0, .035)
                            gap = .012
                        elif kind == "madeira":
                            top, low, gap = .006, .045, .01
                        else:
                            top, low, gap = .02, rng.uniform(.09, .14), 0.0
                        tb = t1 + (rng.uniform(0, .05) if kind == "palha" else 0)
                        g = rng.uniform(.86, 1.08) * (.94 if i % 2 else 1.0)
                        tt = tuple(min(1.0, c * g) for c in tint)
                        lo = [S(a_ + gap, t0, 0), S(a1 - gap, t0, 0), S(a1 - gap, tb, 0), S(a_ + gap, tb, 0)]
                        hi = [S(a_ + gap, t0, top), S(a1 - gap, t0, top), S(a1 - gap, tb, low), S(a_ + gap, tb, low)]
                        # (as telhas de barro com o barro liso: a textura de telhado repetia telhas dentro de cada uma)
                        k.solid(lo + hi, "TelhaLisa" if kind == "barro" else mat, tt)
                        a_ = a1
                        col += 1
            # a tábua da beirada
            k.solid([Vector(p) for p in (P(-L, s * e, ze - .12), P(L, s * e, ze - .12), P(L, s * (e - .1), ze - .12), P(-L, s * (e - .1), ze - .12),
                                         P(-L, s * e, ze + thick), P(L, s * e, ze + thick), P(L, s * (e - .1), ze + thick), P(-L, s * (e - .1), ze + thick))],
                    "MadeiraEscura", rgb(80, 56, 40))
        # 07/11 (o usuário: "as quinas dos telhados bugando"): a tábua de beiral nas duas pontas, cobrindo o fim das fileiras
        for a, a2 in ((L - .02, L + .1), (-L - .1, -L + .02)):
            for s in (1, -1):
                bot = [P(a, s * (e + .04), ze - .12), P(a2, s * (e + .04), ze - .12), P(a2, 0, zr - .12), P(a, 0, zr - .12)]
                top = [P(a, s * (e + .04), ze + thick + .1), P(a2, s * (e + .04), ze + thick + .1), P(a2, 0, zr + thick + .1), P(a, 0, zr + thick + .1)]
                k.solid([Vector(p) for p in bot + top], "MadeiraEscura", rgb(80, 56, 40))
        # a cumeeira
        k.solid([Vector(p) for p in (P(-L - .05, -.16, zr), P(L + .05, -.16, zr), P(L + .05, .16, zr), P(-L - .05, .16, zr),
                                     P(-L - .05, -.16, zr + thick + .14), P(L + .05, -.16, zr + thick + .14), P(L + .05, .16, zr + thick + .14), P(-L - .05, .16, zr + thick + .14))],
                mat, tuple(c * .8 for c in tint))
        # os oitões (o triângulo da parede embaixo do telhado), nas duas pontas
        W = (self.w / 2 if along == "x" else self.d / 2) - .02
        for s in (1, -1):
            x0, x1 = s * (W - .25), s * W
            tri = [P(x0, -D, z), P(x1, -D, z), P(x1, D, z), P(x0, D, z), P(x0, 0, zr), P(x1, 0, zr), P(x1, 0, zr), P(x0, 0, zr)]
            # um sólido de 8 pontos com o topo colapsado na cumeeira (vira prisma)
            k.solid([Vector(p) for p in (tri[0], tri[1], tri[2], tri[3], tri[4], tri[5], tri[5], tri[4])], gable_mat, gable_tint)
            if timber_gable:
                # a viga no meio do oitão e duas inclinadas
                for zz in (z + .05, z + rise * .5):
                    half = D * (1 - (zz - z) / rise)
                    a = [P(x1, -half, zz), P(x1 + s * .06, -half, zz), P(x1 + s * .06, half, zz), P(x1, half, zz)]
                    k.solid([Vector(p) for p in a + [Vector(p) + Vector((0, 0, .14)) for p in a]], "MadeiraEscura", rgb(84, 58, 42))
                k.solid([Vector(p) for p in (P(x1, -.07, z), P(x1 + s * .06, -.07, z), P(x1 + s * .06, .07, z), P(x1, .07, z),
                                             P(x1, -.07, zr), P(x1 + s * .06, -.07, zr), P(x1 + s * .06, .07, zr), P(x1, .07, zr))], "MadeiraEscura", rgb(84, 58, 42))
        return zr

    def chimney(self, x, y, z0, z1, w=.7):
        self.k.box(x - w / 2, y - w / 2, z0, x + w / 2, y + w / 2, z1, "Tijolo")
        self.k.box(x - w / 2 - .08, y - w / 2 - .08, z1, x + w / 2 + .08, y + w / 2 + .08, z1 + .14, "PedraClara", rgb(200, 196, 188))
        self.k.box(x - w / 2 + .12, y - w / 2 + .12, z1 + .14, x + w / 2 - .12, y + w / 2 - .12, z1 + .2, "Ferro", rgb(40, 38, 36))
        self.k.chimneys.append((x, y, z1 + .25))

    def lantern(self, face, u, z):
        self.fbox(face, u - .03, u + .03, z + .1, z + .14, 0, .35, "Ferro", rgb(60, 58, 56))
        self.fbox(face, u - .11, u + .11, z - .2, z + .1, .24, .46, "Lanterna", rgb(255, 220, 160))
        self.fbox(face, u - .13, u + .13, z + .1, z + .15, .22, .48, "Ferro", rgb(60, 58, 56))
        D = self.half_other(face)
        p = {"front": (u, D + .35, z - .05), "back": (-u, -D - .35, z - .05), "right": (D + .35, -u, z - .05), "left": (-D - .35, u, z - .05)}[face]
        self.k.lamps.append(p)

    def sign(self, face, u, z, w=1.4, h=.6, tint=rgb(120, 84, 52)):
        """06/11 (as placas cortavam o nome: "PRE...", "CAF..."): a tábua com o nome é do jogo (AVortexTown), deitada na
        parede 12 cm à frente deste ponto; aqui só os dois suportes de ferro em cima dela."""
        # 07/11: a placa agora é a pendurada (vila_placa_pendurada, com o braço dela): sem os suportes da tábua achatada
        D = self.half_other(face)
        self.k.sign = {"front": (u, D + .06, z), "back": (-u, -D - .06, z)}.get(face)

    def awning(self, face, u0, u1, z, depth=1.4, color=rgb(180, 60, 52)):
        D = self.half_other(face)
        if face != "front":
            return
        for i in range(int((u1 - u0) / .5)):
            a = u0 + i * .5
            c = color if i % 2 == 0 else rgb(236, 230, 218)
            pts = [(a, D, z), (a + .5, D, z), (a + .5, D + depth, z - .55), (a, D + depth, z - .55)]
            self.k.solid([Vector(p) for p in pts + [(p[0], p[1], p[2] + .05) for p in pts]], "Tecido", c)
        for a in (u0, u1):
            self.k.box(a - .03, D, z - .05, a + .03, D + depth, z - .02, "MadeiraEscura", rgb(80, 56, 40))

    def balcony(self, face, u0, u1, z, depth=1.0):
        self.fbox(face, u0, u1, z - .12, z, 0, depth, "MadeiraClara", rgb(150, 110, 70))
        for i in range(int((u1 - u0) / .3) + 1):
            uu = u0 + i * (u1 - u0) / int((u1 - u0) / .3)
            self.fbox(face, uu - .03, uu + .03, z, z + .85, depth - .08, depth - .02, "MadeiraEscura", rgb(84, 58, 42))
        self.fbox(face, u0, u1, z + .85, z + .95, depth - .12, depth, "MadeiraEscura", rgb(84, 58, 42))
        for uu in (u0 + .1, u1 - .1):
            self.fbox(face, uu - .06, uu + .06, z - .9, z - .12, depth - .2, depth - .08, "MadeiraEscura", rgb(84, 58, 42))

    def ivy(self, face, u, z0, z1):
        """07/11 (o usuário: "uns cipós flutuando na frente da casa"): a hera nasce no chão, colada na parede, cheia
        embaixo e afinando para cima, com dois ramos."""
        rng = self.k.rng
        z1 = z0 + (z1 - z0) * rng.uniform(.6, .9)
        self.fbox(face, u - .25, u + .25, 0, z0 + .25, 0, .22, "Folha")          # a touceira no pé da parede
        for branch in (0, -1, 1):
            top = z1 if branch == 0 else z0 + (z1 - z0) * rng.uniform(.4, .7)
            n = int((top - z0) / .22)
            for i in range(n):
                t = i / max(1, n)
                zz = z0 + i * .22
                s = (.22 - .12 * t) * rng.uniform(.7, 1.1)
                du = branch * t * .6 + rng.uniform(-.06, .06)
                self.fbox(face, u + du - s, u + du + s, zz, zz + .28, -.005, .07 - .03 * t, "Folha")


# ==================================================================================================
# os interiores (casas de entrar)
# ==================================================================================================
class Furn:
    """07/11 (o usuário: "os interiores muito simples e feios", com a prancha `Estudos de interiores residenciais`): os
    móveis na escala do herói (1,8 m), cada um com as texturas novas do Poly Haven (Piso, Movel, Colcha, Tapete, Lareira,
    Couro, Livros) e colisão. x, y no espaço da casa (a frente em +Y); fz = o piso; face = a parede em que encosta."""

    WOOD = rgb(150, 108, 72)
    DARK = rgb(96, 66, 46)
    IRON = rgb(64, 62, 60)

    def __init__(self, k, fz, rng):
        self.k, self.fz, self.rng = k, fz, rng

    def b(self, x0, y0, z0, x1, y1, z1, mat, tint=(1, 1, 1), collide=False):
        self.k.box(x0, y0, self.fz + z0, x1, y1, self.fz + z1, mat, tint, collide)

    def floor(self, x0, y0, x1, y1, mat="Piso", tint=rgb(200, 170, 140)):
        self.b(x0, y0, -.05, x1, y1, .02, mat, tint)

    def rug(self, x0, y0, x1, y1, c, border=rgb(214, 176, 90)):
        """O tapete com a borda (a prancha: verde ou vermelho com a faixa dourada)."""
        self.b(x0, y0, .02, x1, y1, .03, "Tapete", border)
        self.b(x0 + .12, y0 + .12, .03, x1 - .12, y1 - .12, .036, "Tapete", c)
        self.b(x0 + .2, y0 + .2, .036, x1 - .2, y1 - .2, .04, "Tapete", tuple(v * .8 for v in c))

    def bed(self, x, y, along="y", head=1, quilt=rgb(96, 128, 70), double=False):
        """A cama de madeira (cabeceira alta, pés, colchão, colcha, lençol dobrado e travesseiros). (x, y) = o meio;
        along = o eixo do comprimento; head = de que lado (+1/-1 nesse eixo) fica a cabeceira."""
        L, W = 2.0, (1.4 if double else 1.0)

        def R(a0, a1, c0, c1, z0, z1, mat, tint, collide=False):
            # (a: ao longo, a partir da cabeceira para o pé, 0..L; c: através, -W/2..W/2) -> a caixa da casa
            p0, p1 = sorted((head * (L / 2 - a0), head * (L / 2 - a1)))
            if along == "y":
                self.b(x + c0, y + p0, z0, x + c1, y + p1, z1, mat, tint, collide)
            else:
                self.b(x + p0, y + c0, z0, x + p1, y + c1, z1, mat, tint, collide)
        h = W / 2
        R(.05, L - .05, -h + .05, h - .05, .18, .32, "Movel", self.WOOD, True)        # o estrado
        for a in (0, L - .08):
            for c in (-h, h - .08):
                R(a, a + .08, c, c + .08, 0, .4, "Movel", self.DARK)
        R(.08, L - .06, -h + .06, h - .06, .32, .46, "Colcha", rgb(236, 230, 214))     # o colchão
        R(.6, L - .04, -h + .02, h - .02, .46, .53, "Colcha", quilt)                    # a colcha
        R(.6, .78, -h + .02, h - .02, .53, .56, "Colcha", rgb(240, 236, 226))          # o lençol dobrado
        for i in range(2 if double else 1):
            c = (i - .5) * .62 if double else 0
            R(.12, .5, c - .28, c + .28, .46, .6, "Colcha", rgb(248, 246, 240))       # o travesseiro
        R(0, .08, -h, h, 0, 1.1, "Movel", self.DARK)                                    # a cabeceira
        R(-.02, .1, -h - .02, h + .02, 1.1, 1.16, "Movel", self.WOOD)
        R(L - .08, L, -h, h, 0, .65, "Movel", self.DARK)                                # o pé

    def wardrobe(self, x, y, face):
        """O guarda-roupa de duas portas (1,1 x 0,55 x 1,95 m), encostado na parede `face` ("left": a parede do -x...)."""
        W, D, H = 1.1, .55, 1.95
        x0, y0, x1, y1 = self._wall_box(x, y, W, D, face)
        self.b(x0, y0, 0, x1, y1, H, "Movel", self.WOOD, collide=True)
        self.b(x0 - .04, y0 - .04, H, x1 + .04, y1 + .04, H + .08, "Movel", self.DARK)                 # a cornija
        self.b(x0 - .02, y0 - .02, 0, x1 + .02, y1 + .02, .1, "Movel", self.DARK)                        # o rodapé
        # as duas portas (almofadas) e os puxadores, na frente (o lado de dentro da casa)
        n = self._inward(face)
        for s in (-1, 1):
            if face in ("left", "right"):
                px = x1 if n[0] > 0 else x0
                self.b(px - .02 if n[0] > 0 else px - .0, y + s * .27 - .22, .2, px + .02 if n[0] > 0 else px + .02, y + s * .27 + .22, 1.8, "Movel", self.DARK)
                self.b(px, y + s * .06 - .02, 1.0, px + .05 * n[0], y + s * .06 + .02, 1.12, "Ferro", rgb(170, 140, 80))
            else:
                py = y1 if n[1] > 0 else y0
                self.b(x + s * .27 - .22, py - .02, .2, x + s * .27 + .22, py + .02, 1.8, "Movel", self.DARK)
                self.b(x + s * .06 - .02, py, 1.0, x + s * .06 + .02, py + .05 * n[1], 1.12, "Ferro", rgb(170, 140, 80))

    def chest(self, x, y, along="x", c=rgb(150, 70, 52)):
        """O baú (90 x 50 x 55 cm) com tampa abaulada e as cintas de ferro."""
        A, B = (.45, .25) if along == "x" else (.25, .45)
        self.b(x - A, y - B, 0, x + A, y + B, .42, "Movel", c, collide=True)
        self.b(x - A + .02, y - B + .02, .42, x + A - .02, y + B - .02, .52, "Movel", tuple(v * .9 for v in c))
        for t in (-.6, 0, .6):
            if along == "x":
                self.b(x + t * A - .03, y - B - .01, 0, x + t * A + .03, y + B + .01, .53, "Ferro", self.IRON)
            else:
                self.b(x - A - .01, y + t * B - .03, 0, x + A + .01, y + t * B + .03, .53, "Ferro", self.IRON)

    def nightstand(self, x, y, candle=True):
        self.b(x - .22, y - .22, 0, x + .22, y + .22, .55, "Movel", self.WOOD, collide=True)
        self.b(x - .24, y - .24, .55, x + .24, y + .24, .6, "Movel", self.DARK)
        self.b(x - .16, y - .23, .3, x + .16, y - .22, .45, "Movel", self.DARK)
        if candle:
            self.b(x - .07, y - .07, .6, x + .07, y + .07, .62, "Ferro", rgb(170, 140, 80))
            self.b(x - .03, y - .03, .62, x + .03, y + .03, .74, "Colcha", rgb(246, 240, 220))
            self.b(x - .015, y - .015, .74, x + .015, y + .015, .79, "Lanterna", rgb(255, 210, 130))

    def chair(self, x, y, facing, seat=rgb(160, 50, 44)):
        """A cadeira com encosto (o assento de couro); facing = para onde o assento olha (dx, dy)."""
        fx, fy = facing
        self.b(x - .22, y - .22, .42, x + .22, y + .22, .48, "Movel", self.WOOD)
        self.b(x - .19, y - .19, .48, x + .19, y + .19, .52, "Couro", seat)
        for sx in (-1, 1):
            for sy in (-1, 1):
                self.b(x + sx * .19 - .03, y + sy * .19 - .03, 0, x + sx * .19 + .03, y + sy * .19 + .03, .42, "Movel", self.DARK)
        # o encosto do lado oposto ao que ela olha
        bx, by = x - fx * .2, y - fy * .2
        if fx:
            self.b(bx - .03, y - .22, .48, bx + .03, y + .22, 1.05, "Movel", self.DARK)
        else:
            self.b(x - .22, by - .03, .48, x + .22, by + .03, 1.05, "Movel", self.DARK)
        self.k.collide(x - .24, y - .24, self.fz, x + .24, y + .24, self.fz + .52)

    def table(self, x, y, tw, td, chairs=4, things=True):
        """A mesa (75 cm) com as cadeiras em volta e uma toalha, pão, caneca e vela em cima."""
        self.b(x - tw / 2, y - td / 2, .7, x + tw / 2, y + td / 2, .76, "Movel", self.WOOD)
        for sx in (-1, 1):
            for sy in (-1, 1):
                self.b(x + sx * (tw / 2 - .1) - .05, y + sy * (td / 2 - .1) - .05, 0, x + sx * (tw / 2 - .1) + .05, y + sy * (td / 2 - .1) + .05, .7, "Movel", self.DARK)
        self.k.collide(x - tw / 2, y - td / 2, self.fz, x + tw / 2, y + td / 2, self.fz + .76)
        if things:
            self.b(x - tw * .3, y - td * .32, .76, x + tw * .3, y + td * .32, .765, "Colcha", rgb(236, 226, 200))
            self.b(x - .1, y - .06, .765, x + .1, y + .06, .84, "Madeira", rgb(214, 160, 90))        # o pão
            self.b(x + .25, y - .05, .765, x + .33, y + .05, .88, "Ferro", rgb(150, 150, 156))       # a caneca
            self.b(x - .3, y + .02, .765, x - .26, y + .06, .9, "Colcha", rgb(246, 240, 220))
            self.b(x - .295, y + .025, .9, x - .265, y + .055, .94, "Lanterna", rgb(255, 210, 130))
        spots = []
        if tw >= td:
            spots += [(x - tw / 2 - .35, y, (1, 0)), (x + tw / 2 + .35, y, (-1, 0))]
            spots += [(x + dx, y + s * (td / 2 + .3), (0, -s)) for s in (-1, 1) for dx in ((-.4, .4) if tw > 1.3 else (0,))]
        else:
            spots += [(x, y - td / 2 - .35, (0, 1)), (x, y + td / 2 + .35, (0, -1))]
            spots += [(x + s * (tw / 2 + .3), y + dy, (-s, 0)) for s in (-1, 1) for dy in ((-.4, .4) if td > 1.3 else (0,))]
        for cx, cy, f in spots[:chairs]:
            self.chair(cx, cy, f)

    def bookcase(self, x, y, face, W=1.2, H=1.9):
        """A estante com livros (a textura Livros: uma fileira a cada 45 cm, as prateleiras nos múltiplos de 45 cm)."""
        D = .38
        x0, y0, x1, y1 = self._wall_box(x, y, W, D, face)
        self.b(x0, y0, 0, x1, y1, H, "Movel", self.DARK, collide=True)
        n = self._inward(face)
        z = math.ceil((self.fz + .25) / .45) * .45 - self.fz
        while z + .42 < H:
            # a tábua e a fileira de livros um pouco para dentro da frente
            if face in ("left", "right"):
                fx = x1 if n[0] > 0 else x0
                self.b(fx - .3 * n[0], y0 + .05, z - .03, fx + .01 * n[0], y1 - .05, z, "Movel", self.WOOD)
                self.b(fx - .3 * n[0], y0 + .06, z, fx - .02 * n[0], y1 - .06, z + .42, "Livros")
            else:
                fy = y1 if n[1] > 0 else y0
                self.b(x0 + .05, fy - .3 * n[1], z - .03, x1 - .05, fy + .01 * n[1], z, "Movel", self.WOOD)
                self.b(x0 + .06, fy - .3 * n[1], z, x1 - .06, fy - .02 * n[1], z + .42, "Livros")
            z += .45

    def fireplace(self, x, y, face, top=3.0, W=1.7):
        """A lareira de pedra (a prancha: pedra cinza, a boca com a lenha em brasa, o console de madeira) com o peito da
        chaminé até o teto."""
        D = .6
        x0, y0, x1, y1 = self._wall_box(x, y, W, D, face)
        n = self._inward(face)
        stone = rgb(170, 168, 166)
        if face in ("left", "right"):
            fx = x1 if n[0] > 0 else x0
            back = x0 if n[0] > 0 else x1
            # as duas ombreiras e o alto da boca
            self.b(x0, y0, 0, x1, y0 + .35, 1.15, "Lareira", stone, collide=True)
            self.b(x0, y1 - .35, 0, x1, y1, 1.15, "Lareira", stone, collide=True)
            self.b(x0, y0, .85, x1, y1, 1.15, "Lareira", stone)
            self.b(min(back, back + .15 * n[0]), y0 + .35, 0, max(back, back + .15 * n[0]), y1 - .35, .85, "Lareira", rgb(70, 66, 64))
            self.b(min(fx, fx + .1 * n[0]) - .0, y0 - .08, 1.15, max(fx, fx + .1 * n[0]), y1 + .08, 1.24, "Movel", self.DARK)   # o console
            self.b(x0, y0 - .08, 1.15, x1, y1 + .08, 1.24, "Movel", self.DARK)
            bx0, bx1 = (x0 + .1, x0 + .55) if n[0] > 0 else (x1 - .55, x1 - .1)
            self.b(bx0, y0 + .4, 0, bx1, y1 - .4, .1, "Brasa")
            for i in range(3):
                yy = y0 + .45 + i * (W - .9) / 2
                self.b(bx0 + .05, yy - .06, .1, bx1 - .05, yy + .06, .22, "Madeira", rgb(120, 80, 50))
            self.b(x0 + (.1 if n[0] > 0 else .2), y + .0 - W * .3, 1.24, x1 - (.2 if n[0] > 0 else .1), y + W * .3, top, "Lareira", stone)
            self.k.collide(x0, y0, self.fz, x1, y1, self.fz + top)
            self.k.inner.append((fx + .6 * n[0], y, self.fz + .7))
        else:
            fy = y1 if n[1] > 0 else y0
            back = y0 if n[1] > 0 else y1
            self.b(x0, y0, 0, x0 + .35, y1, 1.15, "Lareira", stone, collide=True)
            self.b(x1 - .35, y0, 0, x1, y1, 1.15, "Lareira", stone, collide=True)
            self.b(x0, y0, .85, x1, y1, 1.15, "Lareira", stone)
            self.b(x0 + .35, min(back, back + .15 * n[1]), 0, x1 - .35, max(back, back + .15 * n[1]), .85, "Lareira", rgb(70, 66, 64))
            self.b(x0 - .08, y0, 1.15, x1 + .08, y1 + (.1 if n[1] > 0 else 0) - (0 if n[1] > 0 else .1) * 0, 1.24, "Movel", self.DARK)
            by0, by1 = (y0 + .1, y0 + .55) if n[1] > 0 else (y1 - .55, y1 - .1)
            self.b(x0 + .4, by0, 0, x1 - .4, by1, .1, "Brasa")
            for i in range(3):
                xx = x0 + .45 + i * (W - .9) / 2
                self.b(xx - .06, by0 + .05, .1, xx + .06, by1 - .05, .22, "Madeira", rgb(120, 80, 50))
            self.b(x - W * .3, y0 + (.1 if n[1] > 0 else .2), 1.24, x + W * .3, y1 - (.2 if n[1] > 0 else .1), top, "Lareira", stone)
            self.k.collide(x0, y0, self.fz, x1, y1, self.fz + top)
            self.k.inner.append((x, fy + .6 * n[1], self.fz + .7))
        # o caldeirão pendurado e as panelas no console
        return (x0, y0, x1, y1)

    def banner(self, x, y, face, c=rgb(160, 40, 40), z0=1.4, z1=2.5):
        """O estandarte na parede (a prancha: vermelho e azul com o brasão dourado)."""
        n = self._inward(face)
        if face in ("left", "right"):
            px = x + .02 * n[0]
            self.b(min(x, px), y - .32, z1, max(x, px) + .02, y + .32, z1 + .05, "Movel", self.DARK)
            self.b(min(x, px), y - .28, z0, max(x, px), y + .28, z1, "Colcha", c)
            self.b(min(x, px + .01 * n[0]), y - .1, (z0 + z1) / 2 - .05, max(x, px + .01 * n[0]) + .01, y + .1, (z0 + z1) / 2 + .2, "Ferro", rgb(214, 176, 90))
        else:
            py = y + .02 * n[1]
            self.b(x - .32, min(y, py), z1, x + .32, max(y, py) + .02, z1 + .05, "Movel", self.DARK)
            self.b(x - .28, min(y, py), z0, x + .28, max(y, py), z1, "Colcha", c)
            self.b(x - .1, min(y, py + .01 * n[1]), (z0 + z1) / 2 - .05, x + .1, max(y, py + .01 * n[1]) + .01, (z0 + z1) / 2 + .2, "Ferro", rgb(214, 176, 90))

    def frame(self, x, y, face, W=.9, H=.6, z=1.5):
        """Um quadro (um mapa ou paisagem) na parede: a moldura e a tela."""
        n = self._inward(face)
        if face in ("left", "right"):
            self.b(min(x, x + .05 * n[0]), y - W / 2, z, max(x, x + .05 * n[0]), y + W / 2, z + H, "Movel", self.DARK)
            self.b(min(x, x + .06 * n[0]), y - W / 2 + .06, z + .06, max(x, x + .06 * n[0]), y + W / 2 - .06, z + H - .06, "Colcha", rgb(214, 196, 150))
        else:
            self.b(x - W / 2, min(y, y + .05 * n[1]), z, x + W / 2, max(y, y + .05 * n[1]), z + H, "Movel", self.DARK)
            self.b(x - W / 2 + .06, min(y, y + .06 * n[1]), z + .06, x + W / 2 - .06, max(y, y + .06 * n[1]), z + H - .06, "Colcha", rgb(214, 196, 150))

    def barrel(self, x, y):
        self.b(x - .27, y - .27, 0, x + .27, y + .27, .8, "Madeira", rgb(120, 82, 54), collide=True)
        for zz in (.1, .62):
            self.b(x - .29, y - .29, zz, x + .29, y + .29, zz + .05, "Ferro", self.IRON)

    def sacks(self, x, y, n=3):
        for i in range(n):
            xx = x + i * .45
            self.b(xx - .2, y - .2, 0, xx + .2, y + .2, .5, "Colcha", rgb(214, 196, 160))
            self.b(xx - .08, y - .08, .5, xx + .08, y + .08, .58, "Colcha", rgb(190, 170, 130))
        self.k.collide(x - .22, y - .22, self.fz, x + (n - 1) * .45 + .22, y + .22, self.fz + .5)

    def counter(self, x0, x1, y, depth=.6, H=.92):
        """O balcão de cozinha ou de loja: o corpo de tábuas, o tampo claro e o que fica em cima."""
        self.b(x0, y - depth / 2, 0, x1, y + depth / 2, H - .05, "Movel", self.WOOD, collide=True)
        self.b(x0 - .03, y - depth / 2 - .03, H - .05, x1 + .03, y + depth / 2 + .03, H, "Movel", rgb(186, 150, 110))
        for i in range(int((x1 - x0) / .6)):
            xx = x0 + .3 + i * .6
            self.b(xx - .22, y + depth / 2 - .01, .1, xx + .22, y + depth / 2 + .01, H - .15, "Movel", self.DARK)

    def stairs(self, x0, x1, y_start, y_dir, top, step=.28):
        """A escada até o andar de cima (degraus de 30 cm), subindo de y_start para o lado y_dir, com o corrimão do lado
        de dentro e a colisão de rampa em degraus (o herói sobe andando)."""
        n = max(1, int(round(top / .3)))
        rise = top / n
        for i in range(n):
            ya, yb = y_start + y_dir * i * step, y_start + y_dir * (i + 1) * step
            self.b(x0, min(ya, yb), 0, x1, max(ya, yb), (i + 1) * rise, "Movel" if i % 2 else "Piso", self.WOOD)
            self.k.collide(x0, min(ya, yb), self.fz, x1, max(ya, yb), self.fz + (i + 1) * rise)
        # a longarina do lado de dentro e o corrimão
        inner_x = x1 if abs(x1) < abs(x0) else x0
        ye = y_start + y_dir * n * step
        for i in range(0, n, 2):
            yy = y_start + y_dir * (i + .5) * step
            self.b(inner_x - .03, yy - .03, (i + 1) * rise, inner_x + .03, yy + .03, (i + 1) * rise + .9, "Movel", self.DARK)
        self.k.solid([Vector(p) for p in ((inner_x - .04, y_start, self.fz + rise + .9), (inner_x + .04, y_start, self.fz + rise + .9),
                                          (inner_x + .04, ye, self.fz + top + .9), (inner_x - .04, ye, self.fz + top + .9),
                                          (inner_x - .04, y_start, self.fz + rise + .98), (inner_x + .04, y_start, self.fz + rise + .98),
                                          (inner_x + .04, ye, self.fz + top + .98), (inner_x - .04, ye, self.fz + top + .98))], "Movel", self.DARK)
        return min(y_start, ye), max(y_start, ye)

    def railing(self, x0, y0, x1, y1, z):
        """O guarda-corpo do mezanino (balaústres a cada 25 cm e o corrimão), de (x0, y0) a (x1, y1), no piso z (relativo)."""
        L = math.hypot(x1 - x0, y1 - y0)
        n = max(1, int(L / .25))
        for i in range(n + 1):
            t = i / n
            px, py = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
            self.b(px - .03, py - .03, z, px + .03, py + .03, z + .9, "Movel", self.DARK)
        self.b(min(x0, x1) - .05, min(y0, y1) - .05, z + .9, max(x0, x1) + .05, max(y0, y1) + .05, z + .98, "Movel", self.WOOD)
        self.k.collide(min(x0, x1) - .05, min(y0, y1) - .05, self.fz + z, max(x0, x1) + .05, max(y0, y1) + .05, self.fz + z + 1.0)

    # ---- apoio ----------------------------------------------------------------------------------------------
    @staticmethod
    def _inward(face):
        return {"left": (1, 0), "right": (-1, 0), "back": (0, 1), "front": (0, -1)}[face]

    @staticmethod
    def _wall_box(x, y, W, D, face):
        """A caixa de um móvel de largura W e fundo D encostado na parede `face`; (x, y) = o meio do lado da parede."""
        if face == "left":
            return x, y - W / 2, x + D, y + W / 2
        if face == "right":
            return x - D, y - W / 2, x, y + W / 2
        if face == "back":
            return x - W / 2, y, x + W / 2, y + D
        return x - W / 2, y - D, x + W / 2, y


QUILTS = (rgb(96, 128, 70), rgb(80, 110, 150), rgb(150, 60, 52), rgb(120, 90, 60))
RUGS = (rgb(96, 128, 70), rgb(160, 46, 40), rgb(70, 90, 140))
BANNERS = (rgb(160, 40, 40), rgb(50, 70, 140), rgb(60, 110, 60))


def interior(h, floor_z, kind, rng, upstairs=False, back_door=None, door_u=0.0):
    """Os interiores pela prancha `Estudos de interiores residenciais` (07/11). h.w, h.d = as medidas do térreo.
    casa = sobrado: lareira de pedra no fundo, mesa com cadeiras, estante, guarda-roupa ou escada para o quarto;
    chale = casa de pedra ou palha: o fogão de canto, a cama, ervas e sacos; loja = balcão, prateleiras e barris;
    taverna = balcão comprido, barris, mesas e a escada para os quartos. Devolve a escada (x0, x1, y0, y1) ou None."""
    k = h.k
    w, d = h.w / 2 - .25, h.d / 2 - .25          # a face de dentro das paredes
    f = Furn(k, floor_z, rng)
    f.floor(-w, -d, w, d, "PedraPiso" if kind == "taverna" else "Piso", rgb(176, 168, 158) if kind == "taverna" else rgb(205, 175, 140))
    stair = None
    # o vão das portas fica livre: a da frente (door_u) e a dos fundos (back_door: x)
    if kind == "casa":
        f.fireplace(w - 1.2, -d, "back", 3.0)
        if upstairs and d > 2.9:
            # a escada na parede da esquerda, da frente para o fundo, até o vão no piso de cima
            y0, y1 = f.stairs(-w, -w + .95, d - .5, -1, 3.0)
            stair = (-w, -w + 1.0, y0 - .05, y1 + .05)
            f.chest(-w + .5, y0 - .6, "y")
        else:
            f.bed(-w + 1.1, .1, "x", -1, rng.choice(QUILTS))
            f.nightstand(-w + .3, 1.0)
            f.chest(-w + 1.0, -.9, "x")
        f.rug(-1.3, -1.2, 1.3, .9, rng.choice(RUGS))
        f.table(0.1, -.15, 1.5, .85, chairs=4)
        # a estante na parede da direita, entre a lareira e a janela do lado; o balcão da cozinha na frente à direita
        if d > 2.6:
            f.bookcase(w, -d + 1.3, "right", 1.2)
        f.counter(w - 1.6, w - .1, d - .55, .55)
        if w - 2.4 > door_u + .8:
            f.barrel(w - 2.1, d - .45)
        # o estandarte entre a porta dos fundos e a janela do fundo; o quadro em cima da cama (sem escada)
        if w > 3.2:
            f.banner(-1.2, -d, "back", rng.choice(BANNERS))
        if stair is None:
            f.frame(-w, .1, "left", .9, .55, 1.45)
        k.inner.append((0, .3, floor_z + 2.3))
    elif kind == "chale":
        stone = rgb(176, 172, 168)
        # o fogão de canto (pedra) com o caldeirão na brasa e a coifa até o teto
        f.b(-w, -d, 0, -w + 1.3, -d + 1.3, .55, "Lareira", stone, collide=True)
        f.b(-w + .25, -d + .25, .55, -w + 1.05, -d + 1.05, .63, "Brasa")
        f.b(-w + .4, -d + .4, .63, -w + .9, -d + .9, 1.0, "Ferro", rgb(52, 50, 48))
        f.b(-w + .38, -d + .38, .98, -w + .92, -d + .92, 1.03, "Ferro", rgb(70, 68, 66))
        f.b(-w, -d, 1.9, -w + 1.3, -d + 1.3, 2.1, "Lareira", stone)
        f.b(-w + .25, -d + .25, 2.1, -w + 1.0, -d + 1.0, 3.2, "Lareira", stone)
        k.inner.append((-w + .9, -d + 1.5, floor_z + .7))
        f.rug(-1.1, -.7, 1.1, .9, rng.choice(RUGS))
        f.table(.2, -.1, 1.2, .75, chairs=2)
        f.bed(w - .55, -d + 1.05, "y", -1, rng.choice(QUILTS))
        f.nightstand(w - .3, -d + 2.35 if d > 2.6 else .3)
        f.chest(w - .55, d - .55, "x") if abs(door_u - (w - .55)) > 1.3 else None
        f.sacks(-w + .35, d - .5, 3)
        f.barrel(-w + .4, .2) if d > 2.4 else None
        # as ervas penduradas na viga
        f.b(-w, -.05, 2.3, w, .05, 2.4, "Movel", Furn.DARK)
        for i in range(7):
            x = -w + .5 + i * (2 * w - 1.0) / 6
            f.b(x - .06, -.05, 1.9, x + .06, .05, 2.3, "Folha", rng.choice((rgb(120, 150, 80), rgb(160, 140, 80), rgb(140, 90, 110))))
        k.inner.append((.5, .5, floor_z + 2.0))
    elif kind == "loja":
        # o balcão no meio (o lojista atrás), prateleiras de mercadoria no fundo, barris e caixotes
        f.counter(-w + .4, w - 1.2, -.1, .65, .95)
        f.b(-.4, -.25, .95, -.1, .05, 1.0, "Ferro", rgb(190, 160, 80))       # a balança
        f.b(-.38, -.23, 1.0, -.12, .03, 1.06, "Ferro", rgb(200, 170, 90))
        for i in range(3):
            zz = .45 + i * .55
            f.b(-w, -d, zz, w, -d + .45, zz + .05, "Movel", Furn.WOOD)
            for j in range(int(2 * w / .32)):
                if rng.random() < .25:
                    continue
                c = rng.choice((rgb(150, 50, 40), rgb(60, 90, 140), rgb(200, 170, 90), rgb(80, 120, 70), rgb(220, 210, 190)))
                kind_j = rng.random()
                xx = -w + .05 + j * .32
                if kind_j < .5:     # potes
                    f.b(xx, -d + .08, zz + .05, xx + .22, -d + .32, zz + .05 + rng.uniform(.18, .3), "Ferro" if kind_j < .15 else "Tijolo", c)
                else:               # peças de pano e caixinhas
                    f.b(xx, -d + .05, zz + .05, xx + .25, -d + .38, zz + .05 + rng.uniform(.12, .22), "Colcha", c)
        f.b(-w, -d, 0, w, -d + .45, .05, "Movel", Furn.DARK)
        for x in (-w + .03, w - .07):
            f.b(x, -d, 0, x + .04, -d + .45, 1.7, "Movel", Furn.DARK)
        k.collide(-w, -d, floor_z, w, -d + .45, floor_z + 1.8)
        f.barrel(w - .4, d - .5)
        f.barrel(w - 1.0, d - .5)
        f.sacks(-w + .4, d - .5, 2) if abs(door_u - (-w + .6)) > 1.4 else None
        f.chest(w - .5, -d + 1.0, "y")
        f.banner(-w, -d + 1.6, "left", rng.choice(BANNERS))
        f.rug(-1.0, .6, 1.0, 1.9, rng.choice(RUGS))
        k.inner.append((0, 1.2, floor_z + 2.3))
    elif kind == "taverna":
        # o balcão comprido com os barris atrás, as mesas com cadeiras, a lareira e a escada para os quartos de cima
        f.counter(w - 4.6, w - .6, -d + 1.9, .7, 1.05)
        for i in range(5):
            f.barrel(w - 4.2 + i * .8, -d + .35)
        f.b(w - 4.6, -d + .02, 1.4, w - .4, -d + .3, 1.45, "Movel", Furn.WOOD)
        for i in range(10):
            f.b(w - 4.5 + i * .4, -d + .05, 1.45, w - 4.4 + i * .4, -d + .2, 1.62, "Vidro" if i % 2 else "Tijolo", rgb(120, 150, 110) if i % 2 else rgb(200, 150, 110))
        for tx, ty in ((-w + 2.3, -.3), (-w + 2.3, 2.6), (.6, 2.0), (-1.2, -2.6), (2.6, .6)):
            f.table(tx, ty, 1.5, .85, chairs=4)
        f.fireplace(-1.0, -d, "back", 3.4, 2.0)
        f.rug(-2.2, -1.2, 1.2, .9, rgb(160, 46, 40))
        y0, y1 = f.stairs(-w, -w + 1.0, d - .4, -1, 3.4)
        stair = (-w, -w + 1.05, y0 - .05, y1 + .05)
        for x, c in ((-2.5, rgb(160, 40, 40)), (2.5, rgb(50, 70, 140))):
            f.banner(x, d, "front", c, 1.6, 2.8)
        for x in (-2.0, 2.0):
            k.inner.append((x, .5, floor_z + 2.5))
    return stair


def upstairs_room(h, z, stair, rng, kind="casa"):
    """07/11 (o usuário: "tem escada inútil"): o andar de cima de verdade: o piso com o vão da escada (com colisão),
    as paredes com colisão, o guarda-corpo no vão e o quarto (cama, guarda-roupa, baú, criado-mudo com vela, tapete).
    h.w, h.d = as medidas do andar de cima; stair = (x0, x1, y0, y1) do vão (no térreo)."""
    k = h.k
    W, D = h.w / 2 - .25, h.d / 2 - .25
    f = Furn(k, z, rng)
    sx0, sx1, sy0, sy1 = stair
    # o piso (assoalho) em volta do vão: atrás, na frente e do lado
    for x0, y0, x1, y1 in ((-W, -D, W, sy0), (-W, sy1, W, D), (sx1, sy0, W, sy1)):
        if x1 - x0 > .05 and y1 - y0 > .05:
            k.box(x0, y0, z - .14, x1, y1, z + .02, "Piso", rgb(205, 175, 140))
            k.collide(x0, y0, z - .14, x1, y1, z + .02)
    # as paredes de cima: colisão em 4 tiras
    t = .3
    for b in ((-W - t, -D - t, W + t, -D), (-W - t, D, W + t, D + t), (-W - t, -D, -W, D), (W, -D, W + t, D)):
        k.collide(b[0], b[1], z, b[2], b[3], z + 2.7)
    # o guarda-corpo do vão (do lado de dentro e no fim, deixando a saída do alto da escada)
    f.railing(sx1, sy0 + .9, sx1, sy1, 0)
    f.railing(sx0, sy1, sx1, sy1, 0)
    if kind == "taverna":
        # os quartos de aluguel: camas em fila com baú no pé e criado-mudo
        for i, x in enumerate((-W + 2.6, -W + 5.2, W - 4.0, W - 1.4)):
            if x + .6 > W:
                continue
            f.bed(x, -D + 1.05, "y", -1, QUILTS[i % len(QUILTS)])
            f.nightstand(x + .8, -D + .3)
            f.chest(x, -D + 2.4, "x")
        f.rug(-2.0, -.4, 2.0, 1.4, rgb(70, 90, 140))
        f.table(2.0, 2.0, 1.2, .75, chairs=2)
        for x in (-3.0, 3.0):
            k.inner.append((x, 0, z + 2.0))
        return
    # o quarto: a cama de casal no fundo (cabeceira na parede de trás), criados-mudos, guarda-roupa na parede da direita
    bx = min(W - 1.0, .8)
    f.bed(bx, -D + 1.05, "y", -1, rng.choice(QUILTS), double=True)
    f.nightstand(bx - 1.0, -D + .3)
    f.nightstand(bx + 1.0, -D + .3, candle=False) if bx + 1.25 < W else None
    f.wardrobe(W, .4, "right")
    f.chest(bx, -D + 2.45, "x")
    f.rug(bx - 1.2, -D + 1.3, bx + 1.2, -D + 3.2, rng.choice(RUGS))
    if W > 3.3:
        f.table(-W + 1.6, D - 1.0, 1.0, .7, chairs=1, things=False)
    f.banner(-W, -D + 1.2, "left", rng.choice(BANNERS))
    k.inner.append((0, 0, z + 1.9))


def roof_color(rng, kind):
    if kind == "TelhaBarro":
        return rng.choice((rgb(255, 255, 255), rgb(235, 220, 210), rgb(250, 235, 220)))
    if kind == "TelhaMadeira":
        return rng.choice((rgb(255, 255, 255), rgb(220, 210, 200), rgb(170, 160, 150)))
    return rgb(255, 255, 255)


def plaster_tint(rng):
    return rng.choice((rgb(255, 255, 255), rgb(250, 240, 220), rgb(240, 236, 228), rgb(255, 246, 226)))


def house(name, w, d, storeys, roof="TelhaBarro", along="x", enter=True, shop=False, stone_ground=False, balcony=False,
          jetty=True, rise=None, chimney=True, dormer=False, all_stone=False, flowers=True, kind="casa", sides=("left", "right")):
    # 07/11 (o usuário: "as casas não têm interior ainda"): todas de entrar. sides = as laterais com janela (a colada
    # na vizinha fica lisa: as variantes _e, _d e _g, que o BuildVila.py escolhe pelo vizinho)
    enter = True
    ikind = "loja" if shop else "chale" if (all_stone or roof == "Palha") else "casa"
    up = None                     # 07/11: (z, w, d) do andar de cima com quarto (a escada sobe até ele)
    k = Kit(name)
    rng = k.rng
    h = House(k, w, d)
    z = h.foundation(.35, .15)
    floor_z = z
    door_u = 0.0 if not shop else -w / 2 + 1.3
    hs = [3.0] + [2.7] * (storeys - 1)
    wall_tint = plaster_tint(rng)
    for i, hh in enumerate(hs):
        z0, z1 = z, z + hh
        stone = all_stone or (stone_ground and i == 0)
        mat = "PedraClara" if stone else "Reboco"
        tint = rgb(236, 232, 226) if stone else wall_tint
        if i == 0:
            h.door("front", door_u, z0, 1.25 if not enter else 1.4, 2.25, open_=enter, arch=stone)
            if shop:
                # a vitrine da loja (larga, baixa)
                h.window("front", w / 2 - 2.2, z0 + .7, 2.2, 1.4, shutters=False, flowers=False)
                # 06/11: o toldo só em cima da vitrine; a placa com o nome fica entre a porta e a vitrine
                h.awning("front", w / 2 - 3.6, w / 2 - .3, z0 + 2.8)
            else:
                for u in (-w / 2 + 1.4, w / 2 - 1.4):
                    if abs(u - door_u) > 1.6:
                        h.window("front", u, z0 + 1.0, .9, 1.2, flowers=flowers and rng.random() < .7)
            h.window("back", 0, z0 + 1.0, .9, 1.2)
            for face in sides:
                if h.half(face) > 2.6:
                    h.window(face, rng.uniform(-.8, .8), z0 + 1.0, .8, 1.1, shutters=rng.random() < .5)
            h.walls(z0, z1, mat, tint, door=(door_u, 1.4, 2.25) if enter else None, collide=True)
        else:
            if jetty and not all_stone:
                # o andar de cima avança 25 cm (as vigas aparecem embaixo)
                h.w += .5; h.d += .5
                for x in range(int(h.w / .6)):
                    xx = -h.w / 2 + .3 + x * .6
                    k.box(xx - .07, h.d / 2 - .4, z0 - .18, xx + .07, h.d / 2, z0, "MadeiraEscura", rgb(84, 58, 42))
            for u in ((-h.w / 2 + 1.3, h.w / 2 - 1.3) if h.w > 6.5 else (0,)):
                h.window("front", u, z0 + .8, .85, 1.15, flowers=flowers and rng.random() < .4)
            if h.w > 7 and not balcony:
                h.window("front", 0, z0 + .8, .7, 1.0, shutters=False)
            for u in ((-h.w / 2 + 1.6, h.w / 2 - 1.6) if h.w > 7 else (rng.uniform(-1, 1),)):
                h.window("back", u, z0 + .8, .85, 1.1, flowers=flowers and rng.random() < .3)
            h.walls(z0, z1, mat, tint, collide=False)
            # o teto de tábuas do andar de baixo (o piso do de cima): pela janela não se vê o telhado por dentro
            if i == 1 and ikind == "casa":
                up = (z0, h.w, h.d)       # o piso com o vão da escada vem no upstairs_room
            else:
                k.box(-h.w / 2 + .25, -h.d / 2 + .25, z0 - .12, h.w / 2 - .25, h.d / 2 - .25, z0, "MadeiraClara", rgb(170, 130, 92))
            if balcony and i == 1:
                h.balcony("front", -1.6, 1.6, z0 + .1)
                h.door("front", 0, z0 + .1, 1.0, 2.0)
        if not stone:
            h.timber(z0, z1, rails=(z0 + 1.0,) if hh > 2.8 else (), braces=True)
        else:
            # pedra: as quinas em blocos maiores (pedra de cantaria)
            for sx in (-1, 1):
                for sy in (-1, 1):
                    for j in range(int(hh / .5)):
                        if j % 2 == 0:
                            k.box(sx * h.w / 2 - .12, sy * h.d / 2 - .12, z0 + j * .5, sx * h.w / 2 + .12, sy * h.d / 2 + .12, z0 + j * .5 + .45, "PedraClara", rgb(215, 212, 205))
        z = z1
    # o telhado
    rise = rise if rise is not None else (h.d if along == "x" else h.w) * (.55 if roof == "Palha" else .42)
    ridge = h.gable_roof(z, rise, along, over=.5, mat=roof, tint=roof_color(rng, roof), gable_tint=wall_tint, timber_gable=not all_stone)
    if chimney:
        cx = h.w / 2 - 1.1            # 07/11: do lado da lareira (no fundo, à direita)
        h.chimney(cx, -h.d / 2 + 1.0, z - 1.0, ridge + .6)
    if dormer:
        # a água-furtada: uma casinha no telhado da frente
        dz = z + rise * .25
        dy = h.d / 2 * .45
        k.box(-.9, dy - .7, dz - .3, .9, dy + .5, dz + 1.3, "Reboco", wall_tint)
        k.box(-.45, dy + .5, dz + .2, .45, dy + .52, dz + 1.0, "Vidro", rgb(60, 70, 80))
        for s in (1, -1):
            pts = [(-1.15, dy + .7, dz + 1.25), (1.15, dy + .7, dz + 1.25), (1.15, dy - .9, dz + 1.25), (-1.15, dy - .9, dz + 1.25)]
            top = [(0, dy + .7, dz + 2.0), (0, dy + .7, dz + 2.0), (0, dy - .9, dz + 2.0), (0, dy - .9, dz + 2.0)]
        k.solid([Vector(p) for p in [(-1.15, dy + .75, dz + 1.25), (1.15, dy + .75, dz + 1.25), (1.15, dy - .9, dz + 1.25), (-1.15, dy - .9, dz + 1.25),
                                      (0, dy + .75, dz + 2.0), (0, dy + .75, dz + 2.0), (0, dy - .9, dz + 2.0), (0, dy - .9, dz + 2.0)]], roof, roof_color(rng, roof))
    # 07/11 (o usuário: "cipós flutuando na frente da casa", "escada saindo para fora"): daqui para baixo as medidas
    # do térreo (o andar de cima avança 25 cm; com as dele o lampião, a hera e os móveis ficavam fora da parede)
    h.w, h.d = w, d
    # lampião ao lado da porta e algum detalhe na base
    h.lantern("front", door_u + 1.0, floor_z + 2.2)
    if rng.random() < .5:
        h.ivy("front", -h.w / 2 + .4, floor_z, floor_z + 2.6)
    if rng.random() < .6:
        k.box(w / 2 - .8, d / 2 + .2, 0, w / 2 - .2, d / 2 + .8, .9, "Madeira", rgb(120, 82, 54))     # barril na frente
    backyard(h, w, d, floor_z, rng)
    if enter:
        stair = interior(h, floor_z, ikind, rng, up is not None, back_door=-(w / 2 - 1.5), door_u=door_u)
        if up is not None:
            h.w, h.d = up[1], up[2]
            if stair:
                upstairs_room(h, up[0], stair, rng)
            else:
                k.box(-h.w / 2 + .25, -h.d / 2 + .25, up[0] - .12, h.w / 2 - .25, h.d / 2 - .25, up[0], "MadeiraClara", rgb(170, 130, 92))
            h.w, h.d = w, d
    if shop:
        h.sign("front", (-w / 2 + w / 2 - 3.6) / 2, floor_z + 2.9)
    return k


def backyard(h, w, d, floor_z, rng):
    """06/11 (o usuário: "casas nas partes de trás muito simples"): os fundos com a porta da cozinha e o alpendre, a
    pilha de lenha coberta, o barril de chuva, um banquinho e vasos."""
    k = h.k
    keep = (h.w, h.d)
    h.w, h.d = w, d                          # o térreo (o andar de cima avança)
    bu = w / 2 - 1.5
    h.door("back", bu, floor_z, 1.0, 2.05, tint=rgb(110, 76, 50))
    # o alpendre por cima da porta (uma água de telha de madeira em mãos-francesas)
    D = d / 2
    pts = [(-(bu - .9), -D, floor_z + 2.65), (-(bu + .9), -D, floor_z + 2.65), (-(bu + .9), -D - 1.0, floor_z + 2.3), (-(bu - .9), -D - 1.0, floor_z + 2.3)]
    k.solid([Vector(p) for p in pts + [(p[0], p[1], p[2] + .06) for p in pts]], "TelhaMadeira", rgb(170, 160, 150))
    for uu in (bu - .8, bu + .8):
        h.fbox("back", uu - .05, uu + .05, floor_z + 1.8, floor_z + 2.6, 0, .9, "MadeiraEscura", rgb(84, 58, 42))
    # a lenha empilhada contra a parede, com um telhadinho
    u0, u1 = -w / 2 + .4, -w / 2 + .4 + min(2.4, w * .3)
    for r in range(4):
        for i in range(int((u1 - u0) / .2)):
            g = rng.uniform(.8, 1.1)
            uu = u0 + i * .2 + (r % 2) * .1
            if uu + .17 > u1:
                continue
            h.fbox("back", uu, uu + .17, r * .2, r * .2 + .17, .05, .65, "Madeira", (.62 * g, .44 * g, .3 * g))
    k.collide(*_back_box(h, u0, u1, 0, .85, 0, .7))
    pts = [(-u0 + .15, -D, 1.25), (-u1 - .15, -D, 1.25), (-u1 - .15, -D - .9, 1.0), (-u0 + .15, -D - .9, 1.0)]
    k.solid([Vector(p) for p in pts + [(p[0], p[1], p[2] + .05) for p in pts]], "TelhaMadeira", rgb(160, 150, 140))
    # o barril de chuva ao lado da porta e um banquinho
    bx = bu + 1.2 if bu + 1.5 < w / 2 else bu - 1.3
    h.fbox("back", bx - .3, bx + .3, 0, .85, .05, .65, "Madeira", rgb(120, 82, 54), collide=True)
    for zz in (.15, .65):
        h.fbox("back", bx - .32, bx + .32, zz, zz + .05, .04, .67, "Ferro", rgb(70, 68, 66))
    if w > 7:
        h.fbox("back", -.7, .7, .4, .46, .1, .45, "Madeira", rgb(130, 90, 58))
        for uu in (-.6, .6):
            h.fbox("back", uu - .05, uu + .05, 0, .4, .15, .4, "Madeira", rgb(130, 90, 58))
    # vasos de flor no chão
    for uu in ((bu + .95) if bx < bu else (bu - 1.0),):
        h.fbox("back", uu - .18, uu + .18, 0, .3, .1, .45, "Tijolo", rgb(200, 140, 100))
        h.fbox("back", uu - .2, uu + .2, .3, .55, .08, .47, "Folha")
    h.w, h.d = keep


def _back_box(h, u0, u1, z0, z1, o0, o1):
    D = h.d / 2
    return (-u1, -D - o1, z0, -u0, -D - o0, z1)


# ==================================================================================================
# os prédios especiais
# ==================================================================================================
def taverna():
    k = Kit("vila_taverna")
    w, d = 16, 12
    h = House(k, w, d)
    z = h.foundation(.35, .15)
    fz = z
    h.door("front", 0, z, 1.6, 2.4, open_=True)
    for u in (-5.5, -3.0, 3.0, 5.5):
        h.window("front", u, z + 1.0, 1.0, 1.3, flowers=True)
    for u in (-4, 0, 4):
        h.window("back", u, z + 1.0, 1.0, 1.3)
    for face in ("left", "right"):
        for u in (-2.5, 2.5):
            h.window(face, u, z + 1.0, .9, 1.2)
    h.walls(z, z + 3.4, "PedraClara", rgb(236, 230, 222), door=(0, 1.6, 2.4))
    z += 3.4
    h.w += .5; h.d += .5
    for x in range(int(h.w / .6)):
        xx = -h.w / 2 + .3 + x * .6
        k.box(xx - .07, h.d / 2 - .4, z - .18, xx + .07, h.d / 2, z, "MadeiraEscura", rgb(84, 58, 42))
    for u in (-6, -3.5, 3.5, 6):
        h.window("front", u, z + .8, .9, 1.2, flowers=True)
    h.balcony("front", -1.8, 1.8, z + .1, 1.2)
    h.door("front", 0, z + .1, 1.0, 2.0)
    for u in (-5, 0, 5):
        h.window("back", u, z + .8, .9, 1.1)
    h.walls(z, z + 2.9, "Reboco", rgb(250, 238, 214), collide=False)
    h.timber(z, z + 2.9, rails=(z + 1.0,))
    z += 2.9
    ridge = h.gable_roof(z, 4.0, "x", over=.6, mat="TelhaBarro", tint=rgb(240, 225, 210), gable_tint=rgb(250, 238, 214))
    h.chimney(4.5, -2.5, z - 1, ridge + .8, .9)
    h.chimney(-5.5, -3.0, z - 1, ridge + .3, .7)
    up = (h.w, h.d)
    h.w, h.d = w, d
    h.sign("front", -4.25, fz + 3.0, 1.8, .8)
    h.lantern("front", 1.6, fz + 2.3)
    h.lantern("front", -1.6, fz + 2.3)
    # barris e um banco do lado de fora
    for x in (-7.2, -6.5):
        k.box(x - .3, 6.3, 0, x + .3, 6.9, .9, "Madeira", rgb(120, 82, 54))
    k.box(4.0, 6.4, 0, 6.5, 6.8, .45, "Madeira", rgb(130, 90, 58))
    stair = interior(h, fz, "taverna", k.rng)
    h.w, h.d = up
    upstairs_room(h, fz + 3.4, stair, k.rng, "taverna")
    h.w, h.d = w, d
    return k


def prefeitura():
    k = Kit("vila_prefeitura")
    w, d = 17, 12
    h = House(k, w, d)
    z = h.foundation(.5, .2)
    fz = z
    h.door("front", 0, z, 2.2, 3.0, arch=True)
    for u in (-6.7, -4.6, -2.6, 2.6, 4.6, 6.7):
        h.window("front", u, z + 1.2, 1.0, 1.6, shutters=False, arch=True)
    for u in (-6, -2, 2, 6):
        h.window("back", u, z + 1.2, 1.0, 1.6, shutters=False)
    h.walls(z, z + 4.0, "PedraClara", rgb(232, 228, 220))
    z += 4.0
    for u in (-6.7, -4.6, -2.6, 2.6, 4.6, 6.7):
        h.window("front", u, z + .9, .9, 1.3, flowers=True)
    h.balcony("front", -2.2, 2.2, z + .1, 1.3)
    h.door("front", 0, z + .1, 1.4, 2.3)
    h.walls(z, z + 3.2, "Reboco", rgb(250, 240, 220), collide=False)
    h.timber(z, z + 3.2, rails=(z + 1.1,))
    z += 3.2
    ridge = h.gable_roof(z, 4.6, "x", over=.6, mat="TelhaMadeira", tint=rgb(150, 140, 135), gable_tint=rgb(250, 240, 220))
    # o frontão do meio, virado para a praça
    k.box(-3.2, d / 2 - .4, z, 3.2, d / 2 + .3, z + 2.2, "Reboco", rgb(250, 240, 220))
    k.solid([Vector(p) for p in [(-3.8, d / 2 + .6, z + 2.2), (3.8, d / 2 + .6, z + 2.2), (3.8, d / 2 - 3, z + 2.2), (-3.8, d / 2 - 3, z + 2.2),
                                  (0, d / 2 + .6, z + 4.4), (0, d / 2 + .6, z + 4.4), (0, d / 2 - 3, z + 4.4), (0, d / 2 - 3, z + 4.4)]], "TelhaBarro", rgb(240, 220, 205))
    h.window("front", 0, z + .4, 1.2, 1.4, shutters=False, arch=True)
    # a torre do sino no meio do telhado
    tz = ridge - 1.0
    k.box(-1.6, -1.6, tz, 1.6, 1.6, tz + 3.0, "PedraClara", rgb(232, 228, 220))
    for s in ((-1.6, 1.6), ):
        pass
    k.box(-1.7, -1.7, tz + 3.0, 1.7, 1.7, tz + 3.2, "MadeiraEscura", rgb(84, 58, 42))
    for x in (-1.5, 1.5):
        for y in (-1.5, 1.5):
            k.box(x - .15, y - .15, tz + 3.2, x + .15, y + .15, tz + 5.0, "MadeiraEscura", rgb(84, 58, 42))
    k.box(-.5, -.5, tz + 3.6, .5, .5, tz + 4.6, "Ferro", rgb(170, 140, 70))      # o sino
    k.solid([Vector(p) for p in [(-2.0, -2.0, tz + 5.0), (2.0, -2.0, tz + 5.0), (2.0, 2.0, tz + 5.0), (-2.0, 2.0, tz + 5.0),
                                  (0, 0, tz + 8.0), (0, 0, tz + 8.0), (0, 0, tz + 8.0), (0, 0, tz + 8.0)]], "TelhaMadeira", rgb(130, 120, 115))
    h.sign("front", -4.3, fz + 3.7, 2.0, .8)
    h.lantern("front", 1.8, fz + 2.8)
    h.lantern("front", -1.8, fz + 2.8)
    # bandeiras dos lados da porta
    for x in (-3.7, 3.7):
        k.box(x - .5, d / 2 + .05, fz + 4.4, x + .5, d / 2 + .12, fz + 6.6, "Tecido", rgb(140, 40, 40))
        k.box(x - .5, d / 2 + .05, fz + 4.4, x + .5, d / 2 + .13, fz + 4.6, "Tecido", rgb(220, 180, 80))
    return k


def capela():
    k = Kit("vila_capela")
    w, d = 10, 18
    h = House(k, w, d)
    z = h.foundation(.4, .2)
    # 07/11 (o usuário: "capela sem porta"): a torre do sino cobria a porta; agora a porta fica na frente da torre,
    # aberta, e a torre tem a passagem embaixo
    h.d += .4
    h.door("front", 0, z, 1.8, 3.0, open_=True, arch=True)
    h.d -= .4
    for face in ("left", "right"):
        for u in (-5, -1.5, 2, 5.5):
            h.window(face, u, z + 1.8, .8, 2.4, shutters=False, arch=True)
    h.walls(z, z + 6.0, "PedraClara", rgb(205, 205, 210), door=(0, 1.8, 3.0))
    chapel_inside(k, w, d, z)
    # contrafortes
    for face in ("left", "right"):
        for u in (-6.8, -3.3, .3, 3.8, 7.3):
            h.fbox(face, u - .3, u + .3, 0, 4.5, 0, .7, "PedraClara", rgb(190, 190, 196))
    ridge = h.gable_roof(z + 6.0, 4.5, "y", over=.5, mat="TelhaMadeira", tint=rgb(120, 118, 120), gable_mat="PedraClara", gable_tint=rgb(205, 205, 210), timber_gable=False)
    # a rosácea e a torre do sino na frente
    h.window("front", 0, z + 6.6, 1.4, 1.4, shutters=False, arch=True)
    for x0, x1 in ((-1.7, -.9), (.9, 1.7)):
        k.box(x0, d / 2 - 3.2, z, x1, d / 2 + .2, z + 3.25, "PedraClara", rgb(200, 200, 206), collide=True)
    k.box(-1.7, d / 2 - 3.2, z + 3.25, 1.7, d / 2 + .2, ridge + 2.5, "PedraClara", rgb(200, 200, 206))
    for x in (-1.2, 1.2):
        k.box(x - .3, d / 2 + .15, ridge + .3, x + .3, d / 2 + .25, ridge + 1.8, "Vidro")
    k.solid([Vector(p) for p in [(-2.0, d / 2 - 3.5, ridge + 2.5), (2.0, d / 2 - 3.5, ridge + 2.5), (2.0, d / 2 + .5, ridge + 2.5), (-2.0, d / 2 + .5, ridge + 2.5),
                                  (0, d / 2 - 1.5, ridge + 6.0), (0, d / 2 - 1.5, ridge + 6.0), (0, d / 2 - 1.5, ridge + 6.0), (0, d / 2 - 1.5, ridge + 6.0)]], "TelhaMadeira", rgb(110, 108, 110))
    k.box(-.06, d / 2 - 1.56, ridge + 6.0, .06, d / 2 - 1.44, ridge + 7.2, "Ferro", rgb(170, 140, 70))
    k.box(-.4, d / 2 - 1.56, ridge + 6.6, .4, d / 2 - 1.44, ridge + 6.72, "Ferro", rgb(170, 140, 70))
    h.sign("front", 2.4, z + 2.6, 1.6, .6)
    return k


def chapel_inside(k, w, d, z):
    """Bancos dos dois lados do corredor, o altar com toalha, velas e a cruz, o tapete vermelho."""
    wood = rgb(120, 82, 54)
    for r in range(6):
        y = d / 2 - 4.6 - r * 1.3
        for s in (-1, 1):
            x0, x1 = s * .9, s * (w / 2 - .6)
            k.box(min(x0, x1), y - .2, z + .4, max(x0, x1), y + .2, z + .46, "Madeira", wood)
            k.box(min(x0, x1), y - .24, z + .46, max(x0, x1), y - .18, z + .9, "Madeira", wood)
            for xx in (x0 + s * .1, x1 - s * .1):
                k.box(xx - .05, y - .18, z, xx + .05, y + .18, z + .4, "MadeiraEscura", rgb(84, 58, 42))
            k.collide(min(x0, x1), y - .24, z, max(x0, x1), y + .2, z + .9)
    k.box(-.75, -d / 2 + 3.5, z + .01, .75, d / 2 - 3.4, z + .03, "Tecido", rgb(150, 36, 40))
    ay = -d / 2 + 1.6
    k.box(-1.3, ay - .5, z, 1.3, ay + .5, z + 1.0, "PedraClara", rgb(220, 218, 214), collide=True)
    k.box(-1.35, ay - .55, z + .85, 1.35, ay + .55, z + 1.02, "Tecido", rgb(240, 236, 226))
    for x in (-1.0, -.5, .5, 1.0):
        k.box(x - .04, ay - .04, z + 1.02, x + .04, ay + .04, z + 1.3, "Tecido", rgb(245, 240, 225))
        k.box(x - .02, ay - .02, z + 1.3, x + .02, ay + .02, z + 1.36, "Lanterna", rgb(255, 200, 120))
    k.box(-.06, -d / 2 + .3, z + 1.6, .06, -d / 2 + .4, z + 3.4, "Ferro", rgb(190, 150, 70))
    k.box(-.5, -d / 2 + .3, z + 2.7, .5, -d / 2 + .4, z + 2.82, "Ferro", rgb(190, 150, 70))
    k.inner.append((0, ay + 1.2, z + 2.2))
    k.inner.append((0, d / 2 - 5.5, z + 3.0))


def forja():
    """07/11 (o usuário: "na forja a porta buga"): refeita. Na frente o telheiro aberto com a forja de tijolo (coifa e
    chaminé), o fole de couro, a bigorna no toco, o barril de têmpera, o rebolo e as ferramentas na parede; atrás a casa
    de pedra com a porta de verdade (aberta) e a loja do Bram (armas nos suportes, armadura, baú). O telhado de duas
    águas atrás e o de uma água na frente se encontram (antes ficava um vão entre os dois)."""
    k = Kit("vila_forja")
    w, d = 12, 10
    h = House(k, w, d)
    z = h.foundation(.3, .15)
    f = Furn(k, z, k.rng)
    stone, dark, iron = rgb(200, 196, 190), rgb(84, 58, 42), rgb(70, 70, 74)
    f.floor(-w / 2 + .3, -d / 2 + .3, w / 2 - .3, d / 2 - .1, "PedraPiso", rgb(170, 164, 156))
    # a casa de pedra de trás (y de -5 a 0): as 3 paredes de fora e a do meio com a porta (x de 1,2 a 2,6)
    H = 3.6
    k.box(-w / 2, -d / 2, z, w / 2, -d / 2 + .35, z + H, "PedraClara", stone, collide=True)
    k.box(-w / 2, -d / 2, z, -w / 2 + .35, 0, z + H, "PedraClara", stone, collide=True)
    k.box(w / 2 - .35, -d / 2, z, w / 2, 0, z + H, "PedraClara", stone, collide=True)
    dx0, dx1, dh = 1.2, 2.6, 2.3
    k.box(-w / 2, -.35, z, dx0, 0, z + H, "PedraClara", stone, collide=True)
    k.box(dx1, -.35, z, w / 2, 0, z + H, "PedraClara", stone, collide=True)
    k.box(dx0, -.35, z + dh, dx1, 0, z + H, "PedraClara", stone)
    k.box(dx0 - .12, -.05, z, dx0, .08, z + dh + .12, "MadeiraEscura", dark)          # o batente
    k.box(dx1, -.05, z, dx1 + .12, .08, z + dh + .12, "MadeiraEscura", dark)
    k.box(dx0 - .12, -.05, z + dh, dx1 + .12, .08, z + dh + .14, "MadeiraEscura", dark)
    k.box(dx0, -.35 - (dx1 - dx0) * .95, z, dx0 + .07, -.35, z + dh - .02, "Madeira", rgb(110, 76, 50))   # a folha aberta
    # quinas de cantaria
    for sx in (-1, 1):
        for j in range(7):
            if j % 2 == 0:
                k.box(sx * w / 2 - .14, -d / 2 - .1, z + j * .5, sx * w / 2 + .14, -d / 2 + .18, z + j * .5 + .45, "PedraClara", rgb(215, 212, 205))
    # o telhado de duas águas da casa de trás (cumeeira em y = -2,5) com os oitões de pedra
    zr, ze = z + 5.5, z + H
    for s in (1, -1):
        yb = -2.5 + s * 3.0
        pts = [(-w / 2 - .4, yb, ze - .15), (w / 2 + .4, yb, ze - .15), (w / 2 + .4, -2.5, zr), (-w / 2 - .4, -2.5, zr)]
        k.solid([Vector(p) for p in pts + [(p[0], p[1], p[2] + .18) for p in pts]], "TelhaMadeira", rgb(140, 130, 122))
    k.box(-w / 2 - .45, -2.7, zr, w / 2 + .45, -2.3, zr + .3, "TelhaMadeira", rgb(110, 104, 100))
    for sx in (-1, 1):
        x0, x1 = (w / 2 - .35, w / 2) if sx > 0 else (-w / 2, -w / 2 + .35)
        k.solid([Vector(p) for p in ((x0, -d / 2, ze), (x1, -d / 2, ze), (x1, 0, ze), (x0, 0, ze),
                                     (x0, -2.5, zr), (x1, -2.5, zr), (x1, -2.5, zr), (x0, -2.5, zr))], "PedraClara", stone)
    # o telheiro: os postes, a viga e o telhado de uma água que entra por baixo do beiral da casa
    for x in (-w / 2 + .2, -w / 4, w / 4, w / 2 - .2):
        k.box(x - .15, d / 2 - .35, z, x + .15, d / 2 - .05, z + 3.2, "MadeiraEscura", dark, collide=True)
    k.box(-w / 2, d / 2 - .35, z + 3.0, w / 2, d / 2 - .05, z + 3.3, "MadeiraEscura", dark)
    pts = [(-w / 2 - .4, d / 2 + .6, z + 3.1), (w / 2 + .4, d / 2 + .6, z + 3.1), (w / 2 + .4, .2, z + 3.75), (-w / 2 - .4, .2, z + 3.75)]
    k.solid([Vector(p) for p in pts + [(p[0], p[1], p[2] + .16) for p in pts]], "TelhaMadeira", rgb(150, 140, 130))
    for x in (-w / 2 + .2, w / 2 - .2):           # as laterais do telheiro: meia parede de tábuas
        k.box(x - .1, .1, z, x + .1, d / 2 - .35, z + 1.0, "MadeiraClara", rgb(160, 120, 80), collide=True)
    # a forja: base de tijolo com a brasa, a coifa e a chaminé atravessando o telhado
    k.box(-5.2, .2, z, -2.6, 2.4, z + .95, "Tijolo", collide=True)
    k.box(-4.9, .5, z + .95, -2.9, 2.1, z + 1.03, "Brasa")
    for i in range(6):
        k.box(-4.8 + i * .32, .6, z + 1.03, -4.62 + i * .32, 2.0, z + 1.1, "Brasa")
    k.solid([Vector(p) for p in ((-5.3, .2, z + 2.0), (-2.5, .2, z + 2.0), (-2.5, 2.5, z + 2.0), (-5.3, 2.5, z + 2.0),
                                 (-4.4, .2, z + 3.0), (-3.4, .2, z + 3.0), (-3.4, 1.2, z + 3.0), (-4.4, 1.2, z + 3.0))], "Tijolo")
    for x in (-5.2, -2.7):
        k.box(x, 2.3, z + .95, x + .12, 2.45, z + 2.0, "Ferro", iron)
    k.box(-4.4, .2, z + 3.0, -3.4, 1.2, z + 6.6, "Tijolo")
    k.box(-4.55, .05, z + 6.6, -3.25, 1.35, z + 6.75, "PedraClara", rgb(200, 196, 188))
    k.chimneys.append((-3.9, .7, z + 6.8))
    # o fole de couro ao lado
    k.solid([Vector(p) for p in ((-5.9, .7, z + .7), (-5.25, .7, z + .7), (-5.25, 1.9, z + .7), (-5.9, 1.9, z + .7),
                                 (-5.9, .7, z + 1.3), (-5.25, .7, z + 1.05), (-5.25, 1.9, z + 1.05), (-5.9, 1.9, z + 1.3))], "Couro", rgb(130, 80, 50))
    # a bigorna no toco
    ax, ay = -1.0, 2.6
    k.box(ax - .3, ay - .3, z, ax + .3, ay + .3, z + .55, "Madeira", rgb(110, 76, 50), collide=True)
    k.box(ax - .18, ay - .12, z + .55, ax + .18, ay + .12, z + .68, "Ferro", iron)
    k.box(ax - .1, ay - .08, z + .68, ax + .1, ay + .08, z + .78, "Ferro", iron)
    k.box(ax - .38, ay - .14, z + .78, ax + .3, ay + .14, z + .92, "Ferro", rgb(80, 80, 86))
    k.solid([Vector(p) for p in ((ax + .3, ay - .1, z + .8), (ax + .62, ay - .02, z + .86), (ax + .62, ay + .02, z + .86), (ax + .3, ay + .1, z + .8),
                                 (ax + .3, ay - .1, z + .92), (ax + .62, ay - .02, z + .9), (ax + .62, ay + .02, z + .9), (ax + .3, ay + .1, z + .92))], "Ferro", rgb(80, 80, 86))
    k.box(ax - .05, ay - .03, z + .92, ax + .25, ay + .03, z + .97, "Ferro", rgb(120, 120, 126))          # o martelo
    k.box(ax + .2, ay - .02, z + .92, ax + .5, ay + .02, z + .95, "Madeira", rgb(140, 100, 60))
    # o barril de têmpera, o rebolo e o carvão
    f.barrel(.4, 2.4)
    k.box(.18, 2.18, z + .8, .62, 2.62, z + .82, "Vidro", rgb(110, 140, 160))
    k.box(1.7, 1.6, z, 1.9, 2.4, z + .6, "Madeira", rgb(120, 84, 54))
    k.box(1.62, 1.75, z + .6, 1.98, 2.25, z + 1.1, "PedraClara", rgb(170, 166, 160))
    k.box(-2.4, .3, z, -1.6, .9, z + .45, "Ferro", rgb(40, 38, 36), collide=True)
    # as ferramentas na parede do meio (tenazes, martelos) e o suporte com as espadas prontas
    k.box(-1.4, -.02, z + 1.2, 0.8, .04, z + 1.28, "Madeira", rgb(110, 76, 50))
    for i in range(7):
        x = -1.2 + i * .3
        k.box(x - .02, .04, z + .6 + (i % 2) * .15, x + .02, .08, z + 1.25, "Ferro", rgb(110, 110, 116))
        k.box(x - .06, .04, z + .55 + (i % 2) * .15, x + .06, .1, z + .65 + (i % 2) * .15, "Ferro", rgb(90, 90, 96))
    k.box(3.4, .1, z, 5.4, .5, z + .1, "Madeira", rgb(110, 76, 50))
    k.box(3.4, .1, z + 1.3, 5.4, .3, z + 1.38, "Madeira", rgb(110, 76, 50))
    for i in range(6):
        x = 3.6 + i * .32
        k.box(x - .03, .25, z + .1, x + .03, .31, z + 1.45, "Ferro", rgb(190, 190, 198))
        k.box(x - .1, .24, z + 1.0, x + .1, .32, z + 1.05, "Ferro", rgb(170, 140, 70))
    k.collide(3.4, .1, z, 5.4, .5, z + 1.4)
    k.lamps.append((-3.9, 1.3, z + 1.6))
    # a loja do Bram (a casa de trás): suportes de armas, a armadura, o baú, a mesa
    for i in range(3):
        x = -w / 2 + 1.0 + i * 1.4
        f.b(x - .5, -d / 2 + .4, 0, x + .5, -d / 2 + .7, .12, "Madeira", rgb(110, 76, 50))
        f.b(x - .5, -d / 2 + .4, 1.4, x + .5, -d / 2 + .55, 1.48, "Madeira", rgb(110, 76, 50))
        for j in range(4):
            xx = x - .36 + j * .24
            f.b(xx - .03, -d / 2 + .55, .12, xx + .03, -d / 2 + .61, 1.6, "Ferro", rgb(190, 190, 198))
            f.b(xx - .09, -d / 2 + .52, 1.12, xx + .09, -d / 2 + .64, 1.17, "Ferro", rgb(170, 140, 70))
    k.collide(-w / 2 + .4, -d / 2 + .35, z, -w / 2 + 3.9, -d / 2 + .75, z + 1.6)
    # a armadura no cavalete
    axr = 1.0
    f.b(axr - .25, -d / 2 + .5, 0, axr + .25, -d / 2 + 1.0, .1, "Madeira", rgb(110, 76, 50))
    f.b(axr - .04, -d / 2 + .72, .1, axr + .04, -d / 2 + .78, 1.0, "Madeira", rgb(110, 76, 50))
    f.b(axr - .25, -d / 2 + .6, 1.0, axr + .25, -d / 2 + .9, 1.6, "Ferro", rgb(160, 160, 168))
    f.b(axr - .32, -d / 2 + .62, 1.45, axr + .32, -d / 2 + .88, 1.62, "Ferro", rgb(140, 140, 148))
    f.b(axr - .13, -d / 2 + .63, 1.62, axr + .13, -d / 2 + .87, 1.9, "Ferro", rgb(150, 150, 158))
    k.collide(axr - .32, -d / 2 + .5, z, axr + .32, -d / 2 + 1.0, z + 1.9)
    f.chest(4.4, -d / 2 + .7, "x")
    f.table(3.6, -2.4, 1.3, .8, chairs=1)
    f.b(3.3, -2.5, .76, 3.9, -2.3, .8, "Ferro", rgb(190, 190, 198))
    f.banner(-w / 2 + .35, -2.4, "left", rgb(160, 40, 40))
    f.rug(-2.0, -3.6, 0.6, -1.2, rgb(160, 46, 40))
    k.inner.append((0, -2.5, z + 2.4))
    House(k, w, d).sign("front", 3.2, z + 2.6, 1.6, .7)
    return k


def carpintaria():
    k = Kit("vila_carpintaria")
    w, d = 12, 9
    z = .15
    k.box(-w / 2, -d / 2, -.3, w / 2, d / 2, z, "Terra", rgb(200, 180, 150))
    for x in (-w / 2 + .3, 0, w / 2 - .3):
        for y in (-d / 2 + .3, d / 2 - .3):
            k.box(x - .15, y - .15, z, x + .15, y + .15, z + 3.2, "MadeiraEscura", rgb(84, 58, 42), collide=True)
    k.box(-w / 2, -d / 2, z, w / 2, -d / 2 + .25, z + 3.0, "MadeiraClara", rgb(170, 130, 90), collide=True)
    h = House(k, w, d)
    h.gable_roof(z + 3.2, 2.4, "x", over=.5, mat="TelhaMadeira", tint=rgb(170, 160, 150), gable_mat="MadeiraClara", gable_tint=rgb(170, 130, 90))
    # as pilhas de toras, as tábuas, a bancada e o cavalete
    for r in range(3):
        for i in range(5 - r):
            x = -w / 2 + 1.0 + i * .45 + r * .22
            k.box(x - .2, -d / 2 + .5, z + r * .38, x + .2, -d / 2 + 3.5, z + r * .38 + .38, "Madeira", rgb(150, 100, 60), rot=None)
    k.collide(-w / 2 + .8, -d / 2 + .5, z, -w / 2 + 3.2, -d / 2 + 3.5, z + 1.1)
    for i in range(6):
        k.box(1.0, -2.5 + i * .06, z + i * .06, 5.0, -1.3 + i * .06, z + i * .06 + .05, "MadeiraClara", rgb(200, 160, 110))
    k.box(-1.0, 1.0, z + .85, 1.6, 1.8, z + .95, "Madeira", rgb(130, 90, 58))
    for x in (-.9, 1.5):
        for y in (1.1, 1.7):
            k.box(x - .06, y - .06, z, x + .06, y + .06, z + .85, "Madeira", rgb(130, 90, 58))
    k.collide(-1.0, 1.0, z, 1.6, 1.8, z + .95)
    House(k, w, d).sign("front", 3.5, z + 2.6, 1.6, .7)
    return k


def quartel():
    k = Kit("vila_quartel")
    w, d = 16, 11
    h = House(k, w, d)
    z = h.foundation(.4, .2)
    h.door("front", 0, z, 2.0, 2.8, arch=True)
    for u in (-6.5, -3.5, 3.5, 6.5):
        h.window("front", u, z + 1.2, .8, 1.2, shutters=False)
    h.walls(z, z + 3.6, "PedraMuralha", rgb(220, 216, 210))
    z += 3.6
    for u in (-6.5, -3.5, 0, 3.5, 6.5):
        h.window("front", u, z + .8, .7, 1.0, shutters=False)
    h.walls(z, z + 3.0, "PedraMuralha", rgb(210, 206, 200), collide=False)
    z += 3.0
    # o topo com ameias
    k.box(-w / 2 - .1, -d / 2 - .1, z, w / 2 + .1, d / 2 + .1, z + .25, "PedraMuralha", rgb(200, 196, 190))
    for x in range(int(w / 1.2)):
        xx = -w / 2 + .4 + x * 1.2
        for y in (-d / 2, d / 2 - .5):
            k.box(xx, y, z + .25, xx + .6, y + .5, z + 1.0, "PedraMuralha", rgb(200, 196, 190))
    for y in range(int(d / 1.2)):
        yy = -d / 2 + .4 + y * 1.2
        for x in (-w / 2, w / 2 - .5):
            k.box(x, yy, z + .25, x + .5, yy + .6, z + 1.0, "PedraMuralha", rgb(200, 196, 190))
    # estandartes e o suporte de armas na frente
    for x in (-2.0, 2.0):
        k.box(x - .5, d / 2 + .05, z - 2.6, x + .5, d / 2 + .12, z - .2, "Tecido", rgb(60, 70, 130))
    for i in range(4):
        k.box(-7.2 + i * .5, d / 2 + .6, 0, -7.14 + i * .5, d / 2 + .7, 1.9, "Madeira", rgb(110, 80, 52))
        k.box(-7.23 + i * .5, d / 2 + .58, 1.9, -7.11 + i * .5, d / 2 + .72, 2.2, "Ferro", rgb(170, 170, 176))
    k.box(-7.4, d / 2 + .5, .9, -5.4, d / 2 + .8, 1.0, "Madeira", rgb(110, 80, 52))
    h.sign("front", 4.2, .2 + 3.2, 1.8, .7)
    h.lantern("front", 1.6, .2 + 2.6)
    h.lantern("front", -1.6, .2 + 2.6)
    return k


def estabulo():
    k = Kit("vila_estabulo")
    w, d = 16, 9
    z = .2
    k.box(-w / 2, -d / 2, -.3, w / 2, d / 2, z, "Terra", rgb(190, 160, 120))
    k.box(-w / 2, -d / 2, z, w / 2, -d / 2 + .25, z + 3.0, "MadeiraClara", rgb(160, 120, 80), collide=True)
    for x in (-w / 2 + .1, w / 2 - .25):
        k.box(x, -d / 2, z, x + .15, d / 2, z + 3.0, "MadeiraClara", rgb(160, 120, 80), collide=True)
    for i in range(5):
        x = -w / 2 + i * w / 4
        k.box(x - .15, d / 2 - .3, z, x + .15, d / 2, z + 3.0, "MadeiraEscura", rgb(84, 58, 42), collide=True)
        if 0 < i < 4:
            k.box(x - .06, -d / 2, z, x + .06, d / 2 - 1.6, z + 1.5, "Madeira", rgb(130, 92, 60))      # as baias
    for i in range(4):
        x0 = -w / 2 + i * w / 4 + .3
        k.box(x0, d / 2 - .25, z + .9, x0 + w / 4 - .6, d / 2 - .15, z + 1.05, "Madeira", rgb(130, 92, 60))
        k.box(x0, d / 2 - .25, z + .4, x0 + w / 4 - .6, d / 2 - .15, z + .55, "Madeira", rgb(130, 92, 60))
        # feno e o cocho
        k.box(x0 + .2, -d / 2 + .4, z, x0 + 1.6, -d / 2 + 1.4, z + .7, "Palha")
        k.box(x0 + 1.9, -d / 2 + .3, z + .3, x0 + 3.0, -d / 2 + .8, z + .7, "Madeira", rgb(120, 84, 54))
    h = House(k, w, d)
    h.gable_roof(z + 3.0, 2.8, "x", over=.6, mat="TelhaMadeira", tint=rgb(180, 168, 156), gable_mat="MadeiraClara", gable_tint=rgb(160, 120, 80))
    h.sign("front", 0, z + 2.6, 1.8, .7)
    return k


def moinho():
    k = Kit("vila_moinho")
    # a torre de pedra de 8 lados, afinando para cima, e o chapéu de madeira
    def ring(r, z):
        return [Vector((r * math.cos(a), r * math.sin(a), z)) for a in [i * math.pi / 4 + math.pi / 8 for i in range(8)]]
    zs = [0, 2.5, 5.0, 7.0]
    rs = [3.6, 3.3, 3.0, 2.8]
    for a in range(len(zs) - 1):
        b0, b1 = ring(rs[a], zs[a]), ring(rs[a + 1], zs[a + 1])
        for i in range(8):
            j = (i + 1) % 8
            c = Vector((0, 0, (zs[a] + zs[a + 1]) / 2))
            # cada lado como um sólido fino (fica com volume e textura)
            pts = [b0[i], b0[j], b0[j] * .9 + c * .1, b0[i] * .9 + c * .1, b1[i], b1[j], b1[j] * .9 + c * .1, b1[i] * .9 + c * .1]
            k.solid(pts, "PedraClara", rgb(225, 220, 212))
    k.collide(-3.1, -3.1, 0, 3.1, 3.1, 7.0)
    top = ring(3.2, 7.0)
    k.solid([top[0], top[2], top[4], top[6], Vector((0, 0, 10.0)), Vector((0, 0, 10.0)), Vector((0, 0, 10.0)), Vector((0, 0, 10.0))], "TelhaMadeira", rgb(150, 120, 100))
    k.solid([top[1], top[3], top[5], top[7], Vector((0, 0, 10.0)), Vector((0, 0, 10.0)), Vector((0, 0, 10.0)), Vector((0, 0, 10.0))], "TelhaMadeira", rgb(150, 120, 100))
    # porta e janelas na frente (+Y)
    k.box(-.6, 3.25, 0, .6, 3.55, 2.2, "Madeira", rgb(96, 64, 44))
    k.box(-.75, 3.3, 2.2, .75, 3.6, 2.4, "MadeiraEscura", rgb(84, 58, 42))
    for zz in (3.6, 5.6):
        k.box(-.35, 2.95, zz, .35, 3.25, zz + .8, "Vidro")
    # o eixo das pás
    k.box(-.25, 3.0, 7.55, .25, 4.3, 8.05, "MadeiraEscura", rgb(84, 58, 42))
    k.lamps.append((.9, 3.8, 2.0))
    return k


def moinho_pas():
    """As pás (giram em volta do eixo delas, o Y local da Unreal): no plano X-Z do Blender, o cubo no centro."""
    k = Kit("vila_moinho_pas")
    k.box(-.35, -.3, -.35, .35, .3, .35, "MadeiraEscura", rgb(84, 58, 42))
    for i in range(4):
        rot = Matrix.Rotation(i * math.pi / 2, 3, "Y")
        # a vara e a grade com o pano
        k.box(-.08, -.08, .3, .08, .08, 6.0, "MadeiraEscura", rgb(90, 64, 46), rot=rot, pivot=(0, 0, 0))
        for j in range(6):
            zz = 1.2 + j * .8
            k.box(-.06, -.04, zz, 1.4, .04, zz + .06, "Madeira", rgb(120, 86, 56), rot=rot, pivot=(0, 0, 0))
        k.box(.1, -.02, 1.2, 1.35, .02, 5.4, "Tecido", rgb(230, 222, 205), rot=rot, pivot=(0, 0, 0))
    return k


def horta():
    k = Kit("vila_horta")
    w, d = 12, 7
    k.box(-w / 2, -d / 2, -.2, w / 2, d / 2, .05, "Terra", rgb(150, 110, 80))
    rng = k.rng
    for r in range(5):
        y = -d / 2 + .8 + r * 1.35
        k.box(-w / 2 + .4, y - .35, .05, w / 2 - .4, y + .35, .22, "Terra", rgb(120, 86, 60))
        crop = rng.choice(("couve", "trigo", "abobora"))
        for i in range(int((w - 1) / .55)):
            x = -w / 2 + .7 + i * .55
            if crop == "trigo":
                k.box(x - .12, y - .12, .22, x + .12, y + .12, .22 + rng.uniform(.5, .7), "Palha", rgb(240, 220, 150))
            elif crop == "abobora" and i % 2 == 0:
                k.box(x - .2, y - .2, .22, x + .2, y + .2, .55, "Tecido", rgb(230, 130, 50))
                k.box(x - .3, y - .3, .22, x + .3, y + .3, .3, "Folha")
            else:
                s = rng.uniform(.14, .22)
                k.box(x - s, y - s, .22, x + s, y + s, .22 + s * 1.6, "Folha")
    # a cerca baixa
    for side in (-1, 1):
        k.box(-w / 2, side * d / 2 - .05, .05, w / 2, side * d / 2 + .05, .12 + .5, "Madeira", rgb(130, 92, 60))
    for i in range(int(w / 1.5) + 1):
        x = -w / 2 + i * w / int(w / 1.5)
        for side in (-1, 1):
            k.box(x - .06, side * d / 2 - .06, .05, x + .06, side * d / 2 + .06, .8, "MadeiraEscura", rgb(84, 58, 42))
    return k


# ==================================================================================================
# muralha, torre, portão
# ==================================================================================================
WALL_H = 6.0


def stone_courses(k, axis, sign, plane, u0, u1, z0, z1, tint=rgb(214, 210, 204), mat="PedraBruta"):
    """06/11 (o usuário: "as paredes das muralhas tá tudo plano"): a face vira fileiras de blocos de pedra, cada um de
    um tamanho, saindo 1,5 a 8 cm da parede, com juntas de 2,5 cm. axis = a normal da face ("x" ou "y"), sign = para que
    lado, plane = onde a parede está; u ao longo dela, z na altura."""
    rng = k.rng
    z = z0
    row = 0
    while z < z1 - .05:
        h = min(rng.uniform(.4, .62), z1 - z)
        u = u0
        while u < u1 - .05:
            w = min(rng.uniform(.5, 1.15) * (.5 if (u == u0 and row % 2) else 1), u1 - u)
            out = rng.uniform(.015, .08)
            g = rng.uniform(.8, 1.12)
            t = (tint[0] * g, tint[1] * g, tint[2] * g * rng.uniform(.97, 1.03))
            a0, a1 = u + .0125, u + w - .0125
            b0, b1 = z + .0125, z + h - .0125
            if axis == "y":
                k.box(a0, plane, b0, a1, plane + sign * out, b1, mat, t)
            else:
                k.box(plane, a0, b0, plane + sign * out, a1, b1, mat, t)
            u += w
        z += h
        row += 1


def stone_box(k, x0, y0, x1, y1, z0, z1, tint=rgb(214, 210, 204), faces="+x-x+y-y"):
    """As faces de lado de uma caixa de pedra cobertas de blocos (só as pedidas)."""
    if "+y" in faces:
        stone_courses(k, "y", 1, y1, x0, x1, z0, z1, tint)
    if "-y" in faces:
        stone_courses(k, "y", -1, y0, x0, x1, z0, z1, tint)
    if "+x" in faces:
        stone_courses(k, "x", 1, x1, y0, y1, z0, z1, tint)
    if "-x" in faces:
        stone_courses(k, "x", -1, x0, y0, y1, z0, z1, tint)


def crenels(k, x0, x1, y0, y1, z, along="x", tint=None):
    tint = tint or WALL_TINT
    n = int((x1 - x0) / 1.2) if along == "x" else int((y1 - y0) / 1.2)
    for i in range(n):
        if along == "x":
            a = x0 + .3 + i * 1.2
            k.box(a, y0, z, a + .6, y1, z + .9, "PedraBruta", tint)
        else:
            a = y0 + .3 + i * 1.2
            k.box(x0, a, z, x1, a + .6, z + .9, "PedraBruta", tint)


WALL_TINT = rgb(244, 240, 232)     # 06/11: mais clara (era 214)


def muralha():
    k = Kit("vila_muralha")
    L, T = 3.02, 1.2           # meio comprimento (encosta no vizinho) e meia espessura
    t = WALL_TINT
    k.box(-L, -T - .25, -.6, L, T + .25, .6, "PedraBruta", rgb(200, 196, 188))     # a base mais larga
    k.box(-L, -T, .6, L, T, WALL_H, "PedraMuralha", rgb(150, 142, 132), collide=True)
    k.collide(-L, -T - .25, 0, L, T + .25, .6)
    stone_box(k, -L, -T, L, T, .6, WALL_H - .3, t, "+y-y")
    k.box(-L, -T - .12, WALL_H - .3, L, T + .12, WALL_H, "PedraBruta", rgb(228, 224, 216))   # a cornija
    crenels(k, -L, L, T - .5, T + .12, WALL_H, tint=t)
    crenels(k, -L, L, -T - .12, -T + .5, WALL_H, tint=t)
    # musgo e manchas embaixo
    for i in range(3):
        x = k.rng.uniform(-2.5, 2.5)
        k.box(x - .4, T, .5, x + .4, T + .04, .5 + k.rng.uniform(.3, .9), "Folha", rgb(150, 170, 120))
    return k


def torre():
    k = Kit("vila_torre")
    S, H = 3.5, 9.5
    t = WALL_TINT
    k.box(-S - .3, -S - .3, -.6, S + .3, S + .3, .8, "PedraBruta", rgb(200, 196, 188))
    k.box(-S, -S, .8, S, S, H, "PedraMuralha", rgb(150, 142, 132), collide=True)
    k.collide(-S - .3, -S - .3, 0, S + .3, S + .3, .8)
    stone_box(k, -S, -S, S, S, .8, H - .4, t)
    k.box(-S - .25, -S - .25, H - .4, S + .25, S + .25, H, "PedraBruta", rgb(226, 222, 214))
    crenels(k, -S - .25, S + .25, S - .3, S + .25, H)
    crenels(k, -S - .25, S + .25, -S - .25, -S + .3, H)
    crenels(k, -S - .25, -S + .3, -S - .25, S + .25, H, "y")
    crenels(k, S - .3, S + .25, -S - .25, S + .25, H, "y")
    # seteiras nos 4 lados
    h = House(k, 2 * S, 2 * S)
    for face in ("front", "back", "left", "right"):
        for zz in (3.0, 6.0):
            h.fbox(face, -.12, .12, zz, zz + 1.1, -.05, .02, "Ferro", rgb(30, 30, 32))
            h.fbox(face, -.3, .3, zz - .15, zz, 0, .12, "PedraBruta", rgb(200, 196, 188))
    return k


def portao():
    k = Kit("vila_portao")
    t = WALL_TINT
    # duas torres, a passagem com arco de 5 m, as folhas abertas e a ponte de pedra em cima
    for s in (-1, 1):
        x0 = s * 3.0
        x1 = s * 8.0
        k.box(min(x0, x1), -2.5, -.6, max(x0, x1), 2.5, .8, "PedraBruta", rgb(200, 196, 188))
        k.box(min(x0, x1), -2.5, .8, max(x0, x1), 2.5, 11.0, "PedraMuralha", rgb(150, 142, 132), collide=True)
        stone_box(k, min(x0, x1), -2.5, max(x0, x1), 2.5, .8, 10.6, t, "+y-y" + ("-x" if s < 0 else "+x"))
        k.collide(min(x0, x1), -2.5, 0, max(x0, x1), 2.5, .8)
        k.box(min(x0, x1) - .2, -2.7, 10.6, max(x0, x1) + .2, 2.7, 11.0, "PedraBruta", rgb(226, 222, 214))
        crenels(k, min(x0, x1) - .2, max(x0, x1) + .2, 2.2, 2.7, 11.0)
        crenels(k, min(x0, x1) - .2, max(x0, x1) + .2, -2.7, -2.2, 11.0)
        # telhadinho pontudo em cima das torres
        cx = (x0 + x1) / 2
        k.solid([Vector(p) for p in [(cx - 2.2, -2.2, 11.9), (cx + 2.2, -2.2, 11.9), (cx + 2.2, 2.2, 11.9), (cx - 2.2, 2.2, 11.9),
                                      (cx, 0, 14.5), (cx, 0, 14.5), (cx, 0, 14.5), (cx, 0, 14.5)]], "TelhaBarro", rgb(230, 200, 180))
        for x in (cx - 1.9, cx + 1.9):
            for y in (-1.9, 1.9):
                k.box(x - .15, y - .15, 11.0, x + .15, y + .15, 11.9, "MadeiraEscura", rgb(84, 58, 42))
        h = House(k, 5, 5)
        for zz in (4.0, 7.5):
            k.box(cx - .12, 2.5, zz, cx + .12, 2.55, zz + 1.1, "Ferro", rgb(30, 30, 32))
            k.box(cx - .12, -2.55, zz, cx + .12, -2.5, zz + 1.1, "Ferro", rgb(30, 30, 32))
    # o arco por cima da passagem (em degraus de pedra)
    k.box(-3.0, -2.5, 5.2, 3.0, 2.5, 11.0, "PedraMuralha", rgb(150, 142, 132))
    stone_box(k, -3.0, -2.5, 3.0, 2.5, 5.4, 10.6, t, "+y-y")
    for i, (x, zz) in enumerate(((-3.0, 4.6), (-2.6, 4.9), (2.2, 4.9), (2.6, 4.6))):
        k.box(x, -2.5, zz, x + .4, 2.5, 5.2, "PedraBruta", rgb(200, 196, 188))
    k.box(-3.1, 2.45, 4.9, 3.1, 2.65, 5.4, "PedraClara", rgb(210, 206, 198))
    crenels(k, -3.0, 3.0, 2.2, 2.7, 11.0)
    crenels(k, -3.0, 3.0, -2.7, -2.2, 11.0)
    # as folhas do portão abertas para dentro
    for s in (-1, 1):
        k.box(s * 2.85, -2.4, 0, s * 2.7, -.2, 4.6, "MadeiraEscura", rgb(110, 76, 50))
        for zz in (.8, 2.2, 3.6):
            k.box(s * 2.86, -2.3, zz, s * 2.68, -.3, zz + .14, "Ferro", rgb(80, 80, 84))
    # o chão de pedra da passagem
    k.box(-3.0, -2.5, -.2, 3.0, 2.5, .015, "Terra", rgb(80, 72, 64))
    paving(k, -3.0, 3.0, -2.5, 2.5, .375, .38, .56)
    k.sign = (0, 2.8, 6.2)
    for x in (-3.4, 3.4):
        k.lamps.append((x, 2.9, 3.2))
        k.box(x - .1, 2.5, 3.0, x + .1, 2.9, 3.1, "Ferro", rgb(60, 58, 56))
        k.box(x - .15, 2.75, 2.7, x + .15, 3.05, 3.05, "Vidro", rgb(255, 210, 140))
    return k


# ==================================================================================================
# o chão: paralelepípedo (pedras com alturas diferentes), lajes das praças e terra batida
# ==================================================================================================
def rua(name, seed):
    """3 x 3 m de paralelepípedo: fileiras de pedras de 40 a 55 cm, deslocadas, cada uma numa altura e com um
    pouco de inclinação (umas maiores e mais altas), a junta de terra escura embaixo. O alto das pedras fica entre
    2 e 9 cm do chão."""
    k = Kit(name, seed)
    rng = k.rng
    k.box(-1.5, -1.5, -.15, 1.5, 1.5, .015, "Terra", rgb(80, 72, 64))
    # 06/11: as fileiras iguais em todas as placas (8 de 37,5 cm) e cada uma começando encostada na borda: as placas
    # vizinhas continuam as fileiras sem emenda (antes ficavam buracos e degraus entre uma placa e outra)
    y = -1.5
    row = 0
    while y < 1.5 - .05:
        hgt = .375
        x = -1.5
        first = True
        while x < 1.5 - .05:
            wid = rng.uniform(.38, .56)
            if rng.random() < .1:
                wid *= 1.4
            if first and row % 2:
                wid *= .5                              # a fileira de cima começa com meia pedra (o desencontro)
            first = False
            x1 = min(1.5, x + wid)
            y1 = min(1.5, y + hgt)
            top = rng.uniform(.025, .07) + (.03 if rng.random() < .12 else 0)
            g = rng.uniform(.62, .9)
            tint = (g * rng.uniform(.97, 1.03), g * rng.uniform(.97, 1.02), g * rng.uniform(.95, 1.03))
            if rng.random() < .05:
                tint = (g * .85, g * .95, g * .78)     # um pouco de musgo
            gap = .012                                 # 06/11: juntas finas (o usuário: as pedras muito separadas)
            # 07/11 (o usuário: "cada quadrado do chão você vê a separação"): nas fileiras ímpares a pedra da borda
            # não tem junta do lado da borda e fica reta, na mesma altura: com a da placa vizinha vira uma pedra só
            edge_l, edge_r = row % 2 and x <= -1.5 + 1e-6, row % 2 and x1 >= 1.5 - 1e-6
            if edge_l or edge_r:
                top, rot = .045, Matrix.Identity(3)
            else:
                rot = Matrix.Rotation(rng.uniform(-.035, .035), 3, "X") @ Matrix.Rotation(rng.uniform(-.035, .035), 3, "Y")
            cx, cy = (x + x1) / 2, (y + y1) / 2
            k.box(x + (0 if edge_l else gap), y + gap, -.04, x1 - (0 if edge_r else gap), y1 - gap, top, "PedraRua", tint, rot=rot, pivot=(cx, cy, 0))
            x = x1
        y += hgt
        row += 1
    return k


def paving(k, x0, x1, y0, y1, row, wmin, wmax, tone=(.62, .9), mat="PedraRua", tilt=.035, z0=0.0, warm=(1, 1, 1)):
    """Pedras em fileiras (cada fileira começa encostada na borda, as ímpares com meia pedra): de uma placa para a
    vizinha as fileiras continuam. Cada pedra com uma altura, um tom e uma leve inclinação."""
    rng = k.rng
    y = y0
    r = 0
    while y < y1 - .05:
        x = x0
        first = True
        while x < x1 - .05:
            wid = rng.uniform(wmin, wmax)
            if rng.random() < .1:
                wid *= 1.35
            if first and r % 2:
                wid *= .5
            first = False
            xe, ye = min(x1, x + wid), min(y1, y + row)
            if x1 - xe < wmin * .4:
                xe = x1                                 # sem lasquinha no fim da fileira
            top = z0 + rng.uniform(.025, .06) + (.025 if rng.random() < .1 else 0)
            edge_l, edge_r = r % 2 and x <= x0 + 1e-6, r % 2 and xe >= x1 - 1e-6       # 07/11: como na rua()
            if edge_l or edge_r:
                top = z0 + .045
            g = rng.uniform(*tone)
            tint = (g * warm[0] * rng.uniform(.97, 1.03), g * warm[1] * rng.uniform(.97, 1.02), g * warm[2] * rng.uniform(.95, 1.03))
            if rng.random() < .04:
                tint = (g * .85, g * .95, g * .78)
            rot = Matrix.Identity(3) if edge_l or edge_r else Matrix.Rotation(rng.uniform(-tilt, tilt), 3, "X") @ Matrix.Rotation(rng.uniform(-tilt, tilt), 3, "Y")
            k.box(x + (0 if edge_l else .012), y + .012, z0 - .04, xe - (0 if edge_r else .012), ye - .012, top, mat, tint, rot=rot, pivot=((x + xe) / 2, (y + ye) / 2, z0))
            x = xe
        y += row
        r += 1


def laje(name, seed):
    """06/11 (o usuário: "o chão da praça muito feio, parece um png colado"): 3 x 3 m de lajes de pedra de verdade, em
    fileiras de 75 cm (pedras de 55 a 110 cm), cada uma com altura e tom, mais claras e quentes que a rua."""
    k = Kit(name, seed)
    k.box(-1.5, -1.5, -.15, 1.5, 1.5, .015, "Terra", rgb(96, 88, 78))
    paving(k, -1.5, 1.5, -1.5, 1.5, .75, .55, 1.1, tone=(.82, 1.02), tilt=.02, warm=(1.04, 1.0, .93))
    return k


def placa():
    """07/11: a tábua das placas pintadas (1 x 1 m, o meio na origem, a frente em +Y): a face da frente com a imagem
    inteira, de pé (o jogo estica para a proporção de cada placa e põe a imagem dela); atrás, a madeira."""
    k = Kit("vila_placa")
    k.box(-.5, -.04, -.5, .5, -.005, .5, "MadeiraEscura", rgb(70, 48, 32))
    k.box(-.5, -.005, -.5, .5, 0, .5, "Placa")

    def uv(obj):
        me = obj.data
        lay = me.uv_layers[0]
        pidx = [i for i, m in enumerate(me.materials) if m.name == "MV_Placa"]
        for poly in me.polygons:
            if poly.material_index in pidx and poly.normal.y > .9:
                for li in poly.loop_indices:
                    co = me.vertices[me.loops[li].vertex_index].co
                    lay.data[li].uv = (.5 - co.x, co.z + .5)       # (a Unreal espelha o x ao importar)
    k.after = uv
    return k


SIGN_W, SIGN_H, SIGN_Y0, SIGN_ZT = .9, 1.15, .3, -.4
SIGN_OUTLINE = [(0, .10), (.2, .06), (.4, .02), (.5, 0), (.6, .02), (.8, .06), (1, .10), (1, .84), (.8, .88), (.62, .94),
                (.5, 1), (.38, .94), (.2, .88), (0, .84)]


def placa_pendurada():
    """07/11 (o usuário, com uma imagem de referência: "as placas não tão muito coladas? algo nesse estilo"): a placa
    pendurada. Um braço de ferro sai da parede (a origem, na parede; a rua em +Y) com a mão-francesa e as volutas; duas
    correntes seguram a tábua em forma de escudo, de pé e de lado para a rua (as faces olham para ±X, quem anda pela
    rua lê), com a moldura de ferro. A imagem (MV_Placa) vai nas duas faces; o jogo põe a de cada lugar."""
    k = Kit("vila_placa_pendurada")
    iron = rgb(46, 44, 42)
    W, H, y0, zt = SIGN_W, SIGN_H, SIGN_Y0, SIGN_ZT
    pts = [(y0 + u * W, zt - v * H) for u, v in SIGN_OUTLINE]
    # a tábua: o polígono de frente e de trás e as laterais
    bm = k.bm
    t = .02
    front = [bm.verts.new((t, y, z)) for y, z in pts]
    back = [bm.verts.new((-t, y, z)) for y, z in pts]
    k.parts += 1
    part = k.parts
    k.tints[part] = (1, 1, 1)
    for verts in (front, list(reversed(back))):
        f = bm.faces.new(verts)
        f.material_index = k.mat("Placa")
        f[k.part] = part
    side = k.mat("MadeiraEscura")
    for i in range(len(pts)):
        j = (i + 1) % len(pts)
        f = bm.faces.new((front[i], back[i], back[j], front[j]))
        f.material_index = side
        f[k.part] = part
    # a moldura de ferro: uma barra por lado do contorno
    for i in range(len(pts)):
        (ya, za), (yb, zb) = pts[i], pts[(i + 1) % len(pts)]
        L = math.hypot(yb - ya, zb - za) + .03
        a = math.atan2(zb - za, yb - ya)
        cy, cz = (ya + yb) / 2, (za + zb) / 2
        k.box(-.035, cy - L / 2, cz - .028, .035, cy + L / 2, cz + .028, "Ferro", iron, rot=Matrix.Rotation(a, 3, "X"), pivot=(0, cy, cz))
    # os rebites da moldura nos cantos
    for y, z in pts[::2]:
        for sx in (-1, 1):
            k.box(sx * .036 - .012, y - .018, z - .018, sx * .036 + .012, y + .018, z + .018, "Ferro", rgb(110, 104, 96))
    # o braço, a placa da parede, a mão-francesa e o remate
    k.box(-.09, 0, -.62, .09, .04, .22, "Ferro", iron)
    for zz in (-.5, .1):
        k.box(-.03, .04, zz - .03, .03, .07, zz + .03, "Ferro", rgb(110, 104, 96))
    L = y0 + W + .2
    k.box(-.035, 0, -.035, .035, L, .035, "Ferro", iron)
    k.box(-.05, L - .02, -.06, .05, L + .08, .06, "Ferro", iron)
    k.box(-.025, L + .08, -.025, .025, L + .2, .025, "Ferro", iron)
    a = math.atan2(.6, .85)
    k.box(-.025, .03 - .0, -.52 - .0, .025, .03 + math.hypot(.85, .6), -.47, "Ferro", iron, rot=Matrix.Rotation(a, 3, "X"), pivot=(0, .03, -.5))
    # as volutas: aros de ferro feitos de barrinhas
    for cy, cz, r in ((.32, -.16, .11), (.62, -.08, .07), (.16, -.32, .07)):
        for n in range(10):
            a0 = n / 10 * 2 * math.pi
            y, z = cy + r * math.cos(a0), cz + r * math.sin(a0)
            seg = 2 * math.pi * r / 10 + .01
            k.box(-.018, y - seg / 2, z - .016, .018, y + seg / 2, z + .016, "Ferro", iron, rot=Matrix.Rotation(a0 + math.pi / 2, 3, "X"), pivot=(0, y, z))
    # as correntes (elos alternados) do braço até os cantos de cima da tábua
    for u in (.14, .86):
        y = y0 + u * W
        v = min(SIGN_OUTLINE, key=lambda q: abs(q[0] - u))[1]
        top = zt - v * H
        z = -.04
        n = 0
        while z - .07 > top + .01:
            if n % 2 == 0:
                k.box(-.006, y - .022, z - .075, .006, y + .022, z, "Ferro", rgb(80, 78, 74))
            else:
                k.box(-.022, y - .006, z - .075, .022, y + .006, z, "Ferro", rgb(80, 78, 74))
            z -= .06
            n += 1

    def uv(obj):
        me = obj.data
        lay = me.uv_layers[0]
        pidx = [i for i, m in enumerate(me.materials) if m.name == "MV_Placa"]
        for poly in me.polygons:
            if poly.material_index in pidx and abs(poly.normal.x) > .9:
                for li in poly.loop_indices:
                    co = me.vertices[me.loops[li].vertex_index].co
                    u = (co.y - y0) / W
                    lay.data[li].uv = (u if poly.normal.x > 0 else 1 - u, (co.z - (zt - H)) / H)
    k.after = uv
    return k


def porta():
    """07/11: a folha da porta de abrir (1 m x 2,2 m; o jogo estica para cada vão): a dobradiça no x = 0, a folha para
    +x, a espessura em volta do y = 0. Tábuas na vertical, as travessas e as dobradiças de ferro, a argola dos dois lados."""
    k = Kit("vila_porta")
    rng = k.rng
    n = 5
    for i in range(n):
        g = rng.uniform(.88, 1.06)
        k.box(i / n + .004, -.035, 0, (i + 1) / n - .004, .035, 2.2, "Madeira", (.42 * g, .29 * g, .2 * g))
    for zz in (.35, 1.75):
        for sy in (-1, 1):
            k.box(.02, sy * .035, zz, .98, sy * .055, zz + .12, "Madeira", rgb(84, 58, 42))
            k.box(-.02, sy * .035, zz + .02, .55, sy * .06, zz + .1, "Ferro", rgb(64, 62, 60))      # a dobradiça
    for sy in (-1, 1):
        k.box(.82, sy * .035, 1.0, .9, sy * .08, 1.1, "Ferro", rgb(150, 140, 110))
        k.box(.8, sy * .07, .9, .92, sy * .09, 1.02, "Ferro", rgb(150, 140, 110))
    return k


def borda(name, seed):
    """07/11 (o usuário: "o chão não tem uma separação gradual; pedra e grama direto fica esquisito"): a faixa na beira
    das placas de pedra: o paralelepípedo com grama (RuaGrama, do Poly Haven) rente ao chão, com a beira recortada, e
    pedras soltas afundadas na grama, cada vez mais raras. x de -1,5 a 1,5; y de 0 (a beira da placa) para fora (+y)."""
    k = Kit(name, seed)
    rng = k.rng
    x = -1.5
    while x < 1.5 - .05:
        wid = min(1.5 - x, rng.uniform(.35, .7))
        depth = rng.uniform(.3, .75)
        k.box(x, 0, -.08, x + wid, depth, .012, "RuaGrama", (1, 1, 1))
        x += wid
    for i in range(rng.randint(7, 11)):
        y = rng.uniform(0, 1.1) ** 1.6
        px = rng.uniform(-1.4, 1.4)
        a, b = rng.uniform(.14, .3), rng.uniform(.12, .22)
        g = rng.uniform(.62, .88)
        rot = Matrix.Rotation(rng.uniform(-.6, .6), 3, "Z") @ Matrix.Rotation(rng.uniform(-.05, .05), 3, "X")
        k.box(px - a, y - b, -.06, px + a, y + b, rng.uniform(.0, .03), "PedraRua", (g, g * .98, g * .95), rot=rot, pivot=(px, y, 0))
    return k


def terra(name, seed):
    k = Kit(name, seed)
    rng = k.rng
    k.box(-1.5, -1.5, -.15, 1.5, 1.5, .03, "Terra", rgb(235, 220, 200))
    for i in range(rng.randint(4, 9)):
        x, y, s = rng.uniform(-1.3, 1.3), rng.uniform(-1.3, 1.3), rng.uniform(.04, .1)
        g = rng.uniform(.7, 1.0)
        k.box(x - s, y - s, .0, x + s, y + s, .03 + s * .6, "PedraClara", (g, g, g))
    return k


# ==================================================================================================
# 06/11: os enfeites no estilo do kit (o usuário: "muita coisa bugada dentro, talvez itens da vila antiga"): trocam os
# do Godot (vil_bench, vil_planter, vil_lamp, vil_stall_*, vil_crates, vil_well, vil_board, vil_fence, vil_wagon)
# ==================================================================================================
WOOD = rgb(130, 90, 58)
DARK = rgb(84, 58, 42)


def banco():
    k = Kit("vila_banco")
    k.box(-1.0, -.22, .42, 1.0, .22, .48, "Madeira", WOOD)
    for x in (-.85, .85):
        k.box(x - .06, -.18, 0, x + .06, .18, .42, "MadeiraEscura", DARK)
        k.box(x - .05, -.26, .48, x + .05, -.18, .95, "MadeiraEscura", DARK)
    k.box(-1.0, -.27, .7, 1.0, -.2, .9, "Madeira", WOOD)
    k.collide(-1.0, -.27, 0, 1.0, .22, .5)
    return k


def floreira():
    k = Kit("vila_floreira")
    rng = k.rng
    k.box(-.7, -.45, 0, .7, .45, .55, "PedraClara", rgb(220, 214, 204), collide=True)
    k.box(-.75, -.5, .55, .75, .5, .62, "PedraClara", rgb(200, 196, 188))
    k.box(-.62, -.37, .5, .62, .37, .64, "Terra", rgb(110, 80, 60))
    for i in range(14):
        x, y, r = rng.uniform(-.5, .5), rng.uniform(-.27, .27), rng.uniform(.09, .16)
        k.box(x - r, y - r, .62, x + r, y + r, .62 + r * 2.2, "Folha")
        if rng.random() < .55:
            c = rng.choice((rgb(220, 70, 80), rgb(240, 200, 80), rgb(235, 235, 245), rgb(170, 90, 200), rgb(240, 140, 60)))
            k.box(x - .05, y - .05, .62 + r * 2.2, x + .05, y + .05, .62 + r * 2.2 + .08, "Tecido", c)
    return k


def poste():
    """07/11 (o usuário: "os postes muito feios, remodele"): o poste de rua medieval: base de pedra em dois degraus,
    a haste de ferro com anéis, o braço com a voluta e o lampião pendurado na corrente (gaiola de ferro, vidro que
    acende à noite, o chapéu em pirâmide e o pináculo)."""
    k = Kit("vila_poste")
    iron = rgb(46, 44, 42)
    k.box(-.32, -.32, 0, .32, .32, .22, "PedraClara", rgb(205, 200, 192))
    k.box(-.24, -.24, .22, .24, .24, .42, "PedraClara", rgb(215, 210, 202))
    k.box(-.07, -.07, .42, .07, .07, 3.25, "Ferro", iron, collide=True)
    k.collide(-.32, -.32, 0, .32, .32, .42)
    for zz in (.5, 1.2, 2.9):
        k.box(-.1, -.1, zz, .1, .1, zz + .07, "Ferro", iron)
    # o braço e a voluta (degraus de ferro curvando para baixo)
    k.box(-.035, 0, 3.12, .035, .62, 3.18, "Ferro", iron)
    for i, (y, z) in enumerate(((.12, 3.05), (.2, 2.98), (.3, 2.94), (.4, 2.95))):
        k.box(-.03, y - .04, z, .03, y + .04, z + .06, "Ferro", iron)
    k.box(-.04, -.04, 3.25, .04, .04, 3.4, "Ferro", iron)
    k.box(-.07, -.07, 3.4, .07, .07, 3.47, "Ferro", iron)
    # a corrente e o lampião
    y = .58
    k.box(-.015, y - .015, 2.98, .015, y + .015, 3.12, "Ferro", iron)
    k.box(-.17, y - .17, 2.42, .17, y + .17, 2.47, "Ferro", iron)
    k.box(-.13, y - .13, 2.47, .13, y + .13, 2.82, "Lanterna", rgb(255, 214, 150))
    for sx in (-1, 1):
        for sy in (-1, 1):
            k.box(sx * .14 - .02, y + sy * .14 - .02, 2.47, sx * .14 + .02, y + sy * .14 + .02, 2.84, "Ferro", iron)
    k.box(-.17, y - .17, 2.82, .17, y + .17, 2.86, "Ferro", iron)
    k.solid([Vector(p) for p in ((-.19, y - .19, 2.86), (.19, y - .19, 2.86), (.19, y + .19, 2.86), (-.19, y + .19, 2.86),
                                 (0, y, 2.98), (0, y, 2.98), (0, y, 2.98), (0, y, 2.98))], "Ferro", iron)
    k.lamps.append((0, y, 2.65))
    return k


def barraca(name, color):
    k = Kit(name)
    rng = k.rng
    w, d = 3.2, 2.0
    for x in (-w / 2 + .1, w / 2 - .1):
        for y in (-d / 2 + .1, d / 2 - .1):
            top = 2.6 if y < 0 else 2.2
            k.box(x - .06, y - .06, 0, x + .06, y + .06, top, "MadeiraEscura", DARK, collide=True)
    # o balcão com o tampo e a saia de pano
    k.box(-w / 2 + .15, .3, 0, w / 2 - .15, d / 2 - .05, .9, "Madeira", WOOD)
    k.box(-w / 2 + .1, .25, .9, w / 2 - .1, d / 2, .96, "MadeiraClara", rgb(190, 150, 110))
    k.collide(-w / 2 + .15, .3, 0, w / 2 - .15, d / 2, .96)
    k.box(-w / 2 + .2, d / 2 - .02, .2, w / 2 - .2, d / 2 + .01, .85, "Tecido", color)
    # o toldo listrado inclinado
    for i in range(8):
        a = -w / 2 - .1 + i * (w + .2) / 8
        c = color if i % 2 == 0 else rgb(236, 230, 218)
        pts = [(a, -d / 2 - .1, 2.65), (a + (w + .2) / 8, -d / 2 - .1, 2.65), (a + (w + .2) / 8, d / 2 + .5, 2.15), (a, d / 2 + .5, 2.15)]
        k.solid([Vector(p) for p in pts + [(p[0], p[1], p[2] + .04) for p in pts]], "Tecido", c)
    # as mercadorias: cestos com frutas, potes e panos dobrados
    for i in range(7):
        x = -w / 2 + .45 + i * .4
        kind = rng.random()
        if kind < .4:
            k.box(x - .16, .45, .96, x + .16, .8, 1.12, "Palha", rgb(220, 190, 130))
            c = rng.choice((rgb(220, 60, 50), rgb(240, 170, 40), rgb(120, 170, 60), rgb(140, 60, 140)))
            for j in range(3):
                k.box(x - .12 + j * .08, .52, 1.12, x - .06 + j * .08, .72, 1.18, "Tecido", c)
        elif kind < .7:
            k.box(x - .1, .5, .96, x + .1, .7, 1.25, "Tijolo", rgb(210, 150, 110))
        else:
            k.box(x - .17, .42, .96, x + .17, .85, 1.02, "Tecido", rng.choice((rgb(60, 90, 160), rgb(180, 60, 60), rgb(220, 210, 180))))
    # caixotes atrás
    for x in (-1.0, .9):
        k.box(x - .3, -.8, 0, x + .3, -.2, .55, "MadeiraClara", rgb(170, 130, 90), collide=True)
    return k


def caixotes():
    k = Kit("vila_caixotes")
    for x, y, z, s in ((-.45, 0, 0, .7), (.35, .1, 0, .6), (-.4, .05, .7, .55)):
        k.box(x - s / 2, y - s / 2, z, x + s / 2, y + s / 2, z + s, "MadeiraClara", rgb(175, 135, 92))
        for e in (-1, 1):
            yy = y + e * (s / 2 - .03)
            k.box(x - s / 2 - .01, yy - .02, z, x + s / 2 + .01, yy + .02, z + s, "MadeiraEscura", DARK)
    k.box(.75, -.6, 0, 1.3, -.05, .85, "Madeira", rgb(120, 82, 54))
    for zz in (.12, .7):
        k.box(.73, -.62, zz, 1.32, -.03, zz + .05, "Ferro", rgb(70, 68, 66))
    k.collide(-.8, -.6, 0, 1.3, .45, 1.2)
    return k


def poco():
    k = Kit("vila_poco")
    n = 10
    for i in range(n):
        a0, a1 = i * 2 * math.pi / n, (i + 1) * 2 * math.pi / n
        ro, ri = 1.25, .95
        pts = [(ri * math.cos(a0), ri * math.sin(a0), 0), (ro * math.cos(a0), ro * math.sin(a0), 0), (ro * math.cos(a1), ro * math.sin(a1), 0), (ri * math.cos(a1), ri * math.sin(a1), 0)]
        g = k.rng.uniform(.85, 1.05)
        k.solid([Vector(p) for p in pts + [(p[0], p[1], .85) for p in pts]], "PedraBruta", (g * .95, g * .93, g * .9))
    k.collide(-1.2, -1.2, 0, 1.2, 1.2, .85)
    k.box(-.95, -.95, -.6, .95, .95, .3, "Terra", rgb(60, 52, 46))
    k.box(-.96, -.96, .3, .96, .96, .34, "Agua")             # 07/11: água de verdade (a da fonte e do lago)
    for x in (-1.1, 1.1):
        k.box(x - .1, -.1, .85, x + .1, .1, 2.6, "MadeiraEscura", DARK)
    k.box(-1.2, -.06, 2.0, 1.2, .06, 2.12, "Madeira", WOOD)
    k.box(-.15, -.15, 1.35, .15, .15, 1.65, "Madeira", rgb(120, 82, 54))
    for s in (1, -1):
        pts = [(-1.5, 0, 3.2), (1.5, 0, 3.2), (1.5, s * 1.1, 2.5), (-1.5, s * 1.1, 2.5)]
        k.solid([Vector(p) for p in pts + [(p[0], p[1], p[2] + .07) for p in pts]], "TelhaMadeira", rgb(170, 160, 150))
    k.box(-1.5, -.08, 2.55, 1.5, .08, 3.25, "MadeiraEscura", DARK)
    return k


def quadro():
    k = Kit("vila_quadro")
    rng = k.rng
    for x in (-1.1, 1.1):
        k.box(x - .08, -.08, 0, x + .08, .08, 2.4, "MadeiraEscura", DARK, collide=True)
    k.box(-1.1, -.04, .9, 1.1, .04, 2.1, "Madeira", WOOD)
    k.box(-1.2, -.06, 2.1, 1.2, .1, 2.18, "MadeiraEscura", DARK)
    pts = [(-1.35, -.4, 2.35), (1.35, -.4, 2.35), (1.35, .45, 2.15), (-1.35, .45, 2.15)]
    k.solid([Vector(p) for p in pts + [(p[0], p[1], p[2] + .05) for p in pts]], "TelhaMadeira", rgb(170, 160, 150))
    for i in range(7):
        x, z = rng.uniform(-.85, .85), rng.uniform(1.05, 1.85)
        c = rng.choice((rgb(240, 232, 210), rgb(230, 220, 190), rgb(250, 245, 230)))
        k.box(x - .18, .04, z - .12, x + .18, .055, z + .14, "Tecido", c)
        k.box(x - .02, .055, z + .09, x + .02, .07, z + .12, "Ferro", rgb(160, 40, 40))
    k.sign = (0, .1, 2.65)
    return k


def cerca():
    k = Kit("vila_cerca")
    L = 2.75
    for i in range(6):
        x = -L + i * 2 * L / 5
        k.box(x - .07, -.07, 0, x + .07, .07, 1.1, "MadeiraEscura", DARK)
    for zz in (.35, .8):
        k.box(-L, -.04, zz, L, .04, zz + .12, "Madeira", WOOD)
    k.collide(-L, -.1, 0, L, .1, 1.1)
    return k


def roda(k, x, y):
    n = 10
    for i in range(n):
        a0, a1 = i * 2 * math.pi / n, (i + 1) * 2 * math.pi / n
        out = [(x + .5 * math.cos(a), y, .5 + .5 * math.sin(a)) for a in (a0, a1)]
        inn = [(x + .42 * math.cos(a), y, .5 + .42 * math.sin(a)) for a in (a0, a1)]
        pts = [(inn[0][0], y - .06, inn[0][2]), (out[0][0], y - .06, out[0][2]), (out[1][0], y - .06, out[1][2]), (inn[1][0], y - .06, inn[1][2])]
        k.solid([Vector(p) for p in pts + [(p[0], y + .06, p[2]) for p in pts]], "MadeiraEscura", DARK)
    for a in (0, math.pi / 3, 2 * math.pi / 3):
        rot = Matrix.Rotation(a, 3, "Y")
        k.box(x - .03, y - .03, .1, x + .03, y + .03, .9, "MadeiraEscura", DARK, rot=rot, pivot=(x, y, .5))


def carroca():
    k = Kit("vila_carroca")
    k.box(-1.7, -.85, .55, 1.7, .85, .65, "MadeiraClara", rgb(160, 120, 80))
    for y in (-.85, .85):
        k.box(-1.7, y - .05, .65, 1.7, y + .05, 1.1, "Madeira", WOOD)
    for x in (-1.7, 1.7):
        k.box(x - .05, -.85, .65, x + .05, .85, 1.1, "Madeira", WOOD)
    for x in (-1.1, 1.1):
        for y in (-1.0, 1.0):
            roda(k, x, y)
    k.box(1.7, -.05, .55, 3.0, .05, .63, "MadeiraEscura", DARK)
    # a lona em arcos (listras) e a carga
    for i in range(5):
        x0 = -1.65 + i * .66
        base = [(x0, -.9, 1.1), (x0 + .66, -.9, 1.1), (x0 + .66, .9, 1.1), (x0, .9, 1.1)]
        top = [(x0, -.6, 2.1), (x0 + .66, -.6, 2.1), (x0 + .66, .6, 2.1), (x0, .6, 2.1)]
        k.solid([Vector(p) for p in base + top], "Tecido", rgb(226, 214, 186) if i % 2 else rgb(140, 50, 120))
    for x in (-1.2, -.5):
        k.box(x - .25, -.6, .65, x + .25, -.1, 1.1, "MadeiraClara", rgb(175, 135, 92))
    k.collide(-1.8, -1.1, 0, 3.0, 1.1, 2.1)
    k.sign = (0, 1.0, 1.55)
    return k


# ==================================================================================================
# a lista
# ==================================================================================================
HOUSES = {
    "vila_casa_1": ((8, 7, 1, "TelhaBarro", "x"), dict(stone_ground=True)),
    "vila_casa_2": ((9, 8, 2, "TelhaBarro", "y"), dict(stone_ground=True)),
    "vila_casa_3": ((7, 6, 1, "Palha", "x"), dict(all_stone=True, chimney=True, rise=3.4)),
    "vila_casa_4": ((10, 8, 2, "TelhaMadeira", "x"), dict(balcony=True, stone_ground=True, dormer=True)),
    "vila_casa_5": ((6, 9, 3, "TelhaBarro", "y"), dict(stone_ground=True)),
    "vila_casa_6": ((10, 9, 2, "TelhaBarro", "x"), dict(stone_ground=True, dormer=True)),
    "vila_casa_7": ((8, 8, 1, "TelhaMadeira", "y"), dict(all_stone=True, rise=3.6)),
    "vila_casa_8": ((9, 7, 2, "TelhaBarro", "x"), dict(shop=True)),
    "vila_loja": ((9, 8, 2, "TelhaMadeira", "y"), dict(shop=True, stone_ground=True)),
}
# sem sufixo: janela dos dois lados; _g: geminada (sem janela nas laterais)
# 07/11: só duas (com 4 por casa eram 36 malhas de ~14 materiais: o dobro de desenhos por quadro e o FPS caiu)
SIDES = {"": ("left", "right"), "_g": ()}
BUILDERS = {}
for _n, (_a, _kw) in HOUSES.items():
    for _suf, _sides in SIDES.items():
        BUILDERS[_n + _suf] = (lambda n=_n + _suf, a=_a, kw=_kw, sd=_sides: house(n, *a, sides=sd, **kw))
BUILDERS.update({
    "vila_taverna": taverna,
    "vila_prefeitura": prefeitura,
    "vila_capela": capela,
    # 11/11: a forja agora é montada com o kit da Quaternius (Tools/Blender/BuildForjaQuaternius.py); a antiga fica aqui
    # (a função forja) e volta se esta linha voltar
    # "vila_forja": forja,
    "vila_carpintaria": carpintaria,
    "vila_quartel": quartel,
    "vila_estabulo": estabulo,
    "vila_moinho": moinho,
    "vila_moinho_pas": moinho_pas,
    "vila_horta": horta,
    "vila_muralha": muralha,
    "vila_torre": torre,
    "vila_portao": portao,
})
BUILDERS.update({"vila_placa_pendurada": placa_pendurada, "vila_placa": placa, "vila_porta": porta, "vila_banco": banco, "vila_floreira": floreira, "vila_poste": poste, "vila_caixotes": caixotes, "vila_poco": poco,
                 "vila_quadro": quadro, "vila_cerca": cerca, "vila_carroca": carroca,
                 "vila_barraca_a": lambda: barraca("vila_barraca_a", rgb(180, 60, 52)),
                 "vila_barraca_b": lambda: barraca("vila_barraca_b", rgb(52, 96, 160))})
for i, n in enumerate("abcd"):
    BUILDERS["vila_rua_" + n] = (lambda n=n, i=i: rua("vila_rua_" + n, 100 + i))
for i, n in enumerate("abc"):
    BUILDERS["vila_borda_" + n] = (lambda n=n, i=i: borda("vila_borda_" + n, 400 + i))
for i, n in enumerate("ab"):
    BUILDERS["vila_laje_" + n] = (lambda n=n, i=i: laje("vila_laje_" + n, 200 + i))
    BUILDERS["vila_terra_" + n] = (lambda n=n, i=i: terra("vila_terra_" + n, 300 + i))


def to_godot_box(b):
    x0, y0, z0, x1, y1, z1 = b
    return [round((x0 + x1) / 2, 3), round((z0 + z1) / 2, 3), round(-(y0 + y1) / 2, 3), round(x1 - x0, 3), round(z1 - z0, 3), round(y1 - y0, 3)]


def main():
    names = [n for n in BUILDERS if not ONLY or n in ONLY]
    if HEADLESS:
        for o in list(bpy.data.objects):
            bpy.data.objects.remove(o, do_unlink=True)
    coll = bpy.data.collections.get("VilaKit") or bpy.data.collections.new("VilaKit")
    if coll.name not in bpy.context.scene.collection.children:
        bpy.context.scene.collection.children.link(coll)
    for o in list(coll.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    info = json.load(open(INFO, encoding="utf-8")) if os.path.exists(INFO) else {}
    os.makedirs(OUT, exist_ok=True)
    x = 0.0
    for name in names:
        k = BUILDERS[name]()
        obj = k.finish()
        if hasattr(k, "after"):
            k.after(obj)
        coll.objects.link(obj)
        dims = obj.dimensions
        if not HEADLESS:
            obj.location.x = x + dims.x / 2
            x += dims.x + 3
        # a área no chão (o alicerce) e as caixas no espaço do Godot
        bb = [obj.matrix_world.inverted() @ (obj.matrix_world @ Vector(c)) for c in obj.bound_box]
        foot = [round(min(v.x for v in bb), 3), round(-max(v.y for v in bb), 3), round(max(v.x for v in bb), 3), round(-min(v.y for v in bb), 3)]
        flat = name.startswith(("vila_rua", "vila_laje", "vila_terra", "vila_horta", "vila_borda")) or name in ("vila_porta", "vila_placa", "vila_placa_pendurada")
        info[name] = {
            "boxes": [to_godot_box(b) for b in k.boxes],
            "footing": [] if flat or name == "vila_moinho_pas" else foot,
            "sign": [round(k.sign[0], 3), round(k.sign[2], 3), round(-k.sign[1], 3)] if k.sign else None,
            "lamps": [[round(p[0], 3), round(p[2], 3), round(-p[1], 3)] for p in k.lamps],
            "chimneys": [[round(p[0], 3), round(p[2], 3), round(-p[1], 3)] for p in k.chimneys],
            "inner": [[round(p[0], 3), round(p[2], 3), round(-p[1], 3)] for p in k.inner],
            "doors": [[round(p[0], 3), round(p[2], 3), round(-p[1], 3), round(p[3], 3), round(p[4], 3)] for p in k.doors],
        }
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        log("%-18s %6d triângulos, %2d caixas, %d materiais, %.1f x %.1f x %.1f m" % (name, tris, len(k.boxes), len(obj.data.materials), dims.x, dims.y, dims.z))
        if HEADLESS:
            bpy.context.view_layer.update()
            for o in list(bpy.data.objects):
                if o and o.name in bpy.context.view_layer.objects:
                    o.select_set(o == obj)
            bpy.context.view_layer.objects.active = obj
            bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, name + ".fbx"), use_selection=True, object_types={"MESH"},
                                     mesh_smooth_type="FACE", add_leaf_bones=False, bake_anim=False, path_mode="STRIP",
                                     colors_type="SRGB", apply_scale_options="FBX_SCALE_UNITS")
    json.dump(info, open(INFO, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    log("feito:", len(names), "modelos")


main()
