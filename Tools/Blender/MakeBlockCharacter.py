"""O personagem base de blocos (a referência de 01/11: cabeça cúbica, tronco, braços e pernas retos, cueca clara), feito
no Blender aberto pelo MCP.

No Blender: exec(open(".../MakeBlockCharacter.py", encoding="utf-8").read())

As medidas saíram da imagem de referência (em pixels dela; 187 px = 1,40 m): cabeça 46 x 44 x 44, tronco 42 x 24 x 55,
cueca 44 x 26 x 23, braços 20 x 22 x 69, pernas 21 x 22 x 64 com o pé (19 de altura) 4 px à frente. Cada bloco é uma
caixa com a própria textura em pixel (texel de ~5,7 px da referência, filtro "Closest"): a cor da pele com ruído, mais
escura embaixo e nas bordas, o topo da cabeça mais escuro, o peito e a cueca com dobras. A frente olha para -Y.
Sai Data/Modelos/PersonagemBase/personagem_base.blend (e .glb).
"""
import os
import random
import math

import bpy
from mathutils import Vector

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
PX = 1.40 / 187.0          # metros por pixel da referência
TEXEL = 5.7                # pixels da referência por pixel da textura
OUT = os.path.join(ROOT, "Data", "Modelos", "PersonagemBase")
random.seed(7)

SKIN = [(234, 169, 118), (232, 166, 116), (231, 165, 115), (229, 162, 113), (226, 158, 111)]
SKIN_DARK = (214, 144, 103)
SKIN_DEEP = (196, 126, 91)
CLOTH = [(222, 197, 172), (219, 193, 168), (216, 189, 165), (212, 184, 159)]
CLOTH_DARK = (196, 167, 143)
CLOTH_DEEP = (176, 148, 126)


def mix(a, b, t):
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(3))


def paint(cols, rows, face, part):
    """Os pixels de uma face (lista de linhas, de cima para baixo)."""
    cloth = part == "cueca"
    pal = CLOTH if cloth else SKIN
    dark = CLOTH_DARK if cloth else SKIN_DARK
    deep = CLOTH_DEEP if cloth else SKIN_DEEP
    img = []
    for r in range(rows):
        line = []
        for c in range(cols):
            col = random.choice(pal)
            t = r / max(1, rows - 1)                       # 0 em cima, 1 embaixo
            # mais escuro descendo (a luz vem de cima) e na última linha (a sombra da borda)
            col = mix(col, dark, .25 * t * t)
            if r == rows - 1 and face != "top":
                col = mix(col, deep, .55)
            if face == "bottom":
                col = mix(col, deep, .5)
            if part == "cabeca" and face == "top":
                col = mix(col, dark, .45)
            if part == "cabeca" and face != "top" and face != "bottom" and r == 0:
                col = mix(col, dark, .35)
            # cantos verticais um pouco mais escuros (o volume)
            if c in (0, cols - 1) and face in ("front", "back"):
                col = mix(col, dark, .2)
            line.append(col)
        img.append(line)
    if part == "tronco" and face == "front":
        # peito (duas manchas mais escuras embaixo dos peitorais) e o meio do abdômen
        cy = rows // 3
        for c in range(cols):
            if c not in (cols // 2 - 1, cols // 2):
                img[cy][c] = mix(img[cy][c], SKIN_DARK, .45)
        for r in range(cy + 1, rows - 1):
            img[r][cols // 2] = mix(img[r][cols // 2], SKIN_DARK, .3)
    if part == "cueca" and face == "front":
        # o volume no meio e as dobras laterais
        mid = cols // 2
        for r in range(1, rows):
            img[r][mid - 1] = mix(img[r][mid - 1], CLOTH_DEEP, .35)
            img[r][mid + 1] = mix(img[r][mid + 1], CLOTH_DEEP, .35)
            img[r][mid] = mix(img[r][mid], (232, 212, 190), .5)
    if part == "cueca":
        # o cós em cima
        for c in range(cols):
            img[0][c] = mix(img[0][c], (230, 210, 188), .5)
    return img


def make_box(name, size, center, part):
    """Uma caixa (w, d, h em pixels da referência) com a textura de pixel nas 6 faces."""
    w, d, h = size
    W, D, Hh = w * PX, d * PX, h * PX
    cx, cy, cz = center[0] * PX, center[1] * PX, center[2] * PX
    x0, x1, y0, y1, z0, z1 = cx - W / 2, cx + W / 2, cy - D / 2, cy + D / 2, cz, cz + Hh
    cw, cd, ch = max(1, round(w / TEXEL)), max(1, round(d / TEXEL)), max(1, round(h / TEXEL))
    # as faces: (nome, 4 cantos no sentido anti-horário visto de fora, a partir do canto de baixo à esquerda, colunas, linhas)
    faces = [
        ("front", [(x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)], cw, ch),
        ("back", [(x1, y1, z0), (x0, y1, z0), (x0, y1, z1), (x1, y1, z1)], cw, ch),
        ("right", [(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)], cd, ch),
        ("left", [(x0, y1, z0), (x0, y0, z0), (x0, y0, z1), (x0, y1, z1)], cd, ch),
        ("top", [(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)], cw, cd),
        ("bottom", [(x0, y1, z0), (x1, y1, z0), (x1, y0, z0), (x0, y0, z0)], cw, cd),
    ]
    # atlas: as faces lado a lado numa linha
    aw = sum(f[2] for f in faces)
    ah = max(f[3] for f in faces)
    image = bpy.data.images.new("T_" + name, aw, ah, alpha=True)
    pixels = [0.0] * (aw * ah * 4)
    uvs = []
    ox = 0
    for fname, corners, cols, rows in faces:
        img = paint(cols, rows, fname, part)
        for r in range(rows):
            for c in range(cols):
                col = img[r][c]
                py = ah - 1 - r                              # a imagem do Blender começa embaixo
                i = (py * aw + ox + c) * 4
                pixels[i:i + 4] = [col[0] / 255.0, col[1] / 255.0, col[2] / 255.0, 1.0]
        u0, u1 = ox / aw, (ox + cols) / aw
        v0, v1 = (ah - rows) / ah, 1.0
        uvs.append([(u0, v0), (u1, v0), (u1, v1), (u0, v1)])
        ox += cols
    image.pixels[:] = pixels
    image.pack()
    verts, polys = [], []
    for _, corners, _, _ in faces:
        polys.append(tuple(range(len(verts), len(verts) + 4)))
        verts.extend(corners)
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], polys)
    layer = mesh.uv_layers.new(name="UV")
    for p, quad in zip(mesh.polygons, uvs):
        for li, uv in zip(p.loop_indices, quad):
            layer.data[li].uv = uv
    mat = bpy.data.materials.new("M_" + name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = image
    tex.interpolation = "Closest"
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = .9
    mesh.materials.append(mat)
    obj = bpy.data.objects.new(name, mesh)
    bpy.data.collections["PersonagemBase"].objects.link(obj)
    return obj


# ---- cena limpa (o personagem numa coleção própria)
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o, do_unlink=True)
bpy.data.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)
coll = bpy.data.collections.get("PersonagemBase") or bpy.data.collections.new("PersonagemBase")
if coll.name not in bpy.context.scene.collection.children:
    bpy.context.scene.collection.children.link(coll)

# ---- os blocos (z = altura da base do bloco, em pixels da referência; y negativo = frente)
parts = [
    ("Cabeca", (46, 44, 44), (0, 0, 143), "cabeca"),
    ("Tronco", (42, 24, 56), (0, 0, 87), "tronco"),
    ("Cueca", (44, 26, 23), (0, 0, 64), "cueca"),
    ("Braco_L", (20, 22, 69), (31, 0, 71), "braco"),
    ("Braco_R", (20, 22, 69), (-31, 0, 71), "braco"),
    ("Perna_L", (21, 22, 45), (11.5, 0, 19), "perna"),
    ("Perna_R", (21, 22, 45), (-11.5, 0, 19), "perna"),
    ("Pe_L", (22, 26, 19), (11.5, -2, 0), "pe"),
    ("Pe_R", (22, 26, 19), (-11.5, -2, 0), "pe"),
]
objs = [make_box(*p) for p in parts]

# ---- A-pose (para rigar e animar depois): os braços abertos ARM_DEG para fora, girando no ombro (o canto de cima, do
# lado do tronco), com uma folga do corpo; as pernas abertas LEG_DEG, girando no quadril (o pé vai junto). A pose fica
# gravada na malha (é o descanso)
from mathutils import Matrix
ARM_DEG, LEG_DEG, ARM_GAP = 40.0, 7.0, 2.0
by = {o.name: o for o in objs}
for side, s in (("L", 1), ("R", -1)):
    pivot = Vector((s * (21 + ARM_GAP) * PX, 0, 140 * PX))          # o ombro: borda de cima, encostada no tronco
    turn = Matrix.Translation(pivot) @ Matrix.Rotation(math.radians(-s * ARM_DEG), 4, "Y") @ \
        Matrix.Translation(Vector((s * ARM_GAP * PX, 0, 0)) - pivot)
    by["Braco_" + side].data.transform(turn)
    hip = Vector((s * 11.5 * PX, 0, 64 * PX))
    spread = Matrix.Translation(hip) @ Matrix.Rotation(math.radians(-s * LEG_DEG), 4, "Y") @ Matrix.Translation(-hip)
    by["Perna_" + side].data.transform(spread)
    # o pé fica reto no chão: só acompanha a ponta de baixo da perna, para o lado e um pouco para cima
    knee = Vector((s * 11.5 * PX, 0, 19 * PX))
    moved = spread @ knee
    by["Pe_" + side].data.transform(Matrix.Translation(moved - knee))
# os pés de volta ao chão (girados, o canto de baixo subia ou descia)
lowest = min((o.matrix_world @ v.co).z for o in objs for v in o.data.vertices)
for o in objs:
    o.data.transform(Matrix.Translation(Vector((0, 0, -lowest))))
print("PERSONAGEM", len(objs), "blocos, altura %.2f m, A-pose (braços %d°, pernas %d°)" % (187 * PX, ARM_DEG, LEG_DEG))

os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "personagem_base.blend"), copy=True)
