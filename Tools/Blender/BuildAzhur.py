"""Azhûr, o Colosso da Caldeira, refeito para o Vortex (o primeiro chefe).

"/c/Program Files/Blender Foundation/Blender 5.2/blender.exe" --background --factory-startup --python Tools/Blender/BuildAzhur.py -- <saída.glb> [pasta das prévias]

Um gigante de 7 m em blocos (o mesmo estilo voxel do jogo): basalto escuro com placas de obsidiana,
lascas e rachaduras de lava espalhadas em cada pedra grande (segunda passada), runas acesas nos punhos
e canelas, correntes da ombreira ao cinto, garras, espinhos nos nós dos dedos e na coluna,
veios de lava brilhando entre as placas, o núcleo da Estrela Partida pulsando no peito (preso numa
grade de rocha), chifres e espinhos de obsidiana com pontas em brasa, punhos enormes com lava entre
os nós dos dedos e quatro pedras em brasa girando em volta dele.

Cada bloco fica preso a um osso só (sem deformar: o corpo é de pedra). Quatro materiais, pelos nomes
dos espaços: Rock, Obsidian, Lava e Core (Tools/ImportAzhur.py dá o material de cada um na Unreal).
Os clipes têm os nomes usados pelos golpes (Source/Vortex/Private/Creatures/VortexMonsterSpecies.cpp):
Idle, Walk, Awaken, Punch, Stomp, Throw, Seismic, Charge, Cataclysm, Hit, Death.
A frente do modelo é +Y no Blender (como os bichos do Godot: na Unreal giram -90°).
"""
import math
import os
import random
import sys

import bpy
from mathutils import Euler, Matrix, Quaternion, Vector

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = args[0] if args else os.path.join(os.getcwd(), "Azhur.glb")
PREVIEW = args[1] if len(args) > 1 else None
FPS = 30
rng = random.Random(1723)

scene = bpy.context.scene
for ob in list(scene.objects):
    bpy.data.objects.remove(ob, do_unlink=True)
scene.render.fps = FPS

# ---- materiais (cor base; a Unreal troca pelos dela, pelo nome) --------------------------------
def material(name, color, emit=0.0):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nodes = m.node_tree.nodes
    bsdf = nodes.get("Principled BSDF")
    attr = nodes.new("ShaderNodeVertexColor")
    attr.layer_name = "Col"
    mix = nodes.new("ShaderNodeMixRGB")
    mix.blend_type = 'MULTIPLY'
    mix.inputs[0].default_value = 1.0
    mix.inputs[1].default_value = (*color, 1)
    m.node_tree.links.new(attr.outputs["Color"], mix.inputs[2])
    m.node_tree.links.new(mix.outputs[0], bsdf.inputs["Base Color"])
    if emit > 0:
        bsdf.inputs["Emission Color"].default_value = (*color, 1)
        bsdf.inputs["Emission Strength"].default_value = emit
    bsdf.inputs["Roughness"].default_value = .9 if name == "Rock" else .3
    return m

MATS = {
    "Rock": material("Rock", (.20, .16, .14)),
    "Obsidian": material("Obsidian", (.07, .055, .09)),
    "Lava": material("Lava", (1.0, .42, .06), 4.0),
    "Core": material("Core", (1.0, .86, .45), 8.0),
}
MAT_INDEX = {name: i for i, name in enumerate(MATS)}

# ---- esqueleto ---------------------------------------------------------------------------------
# (nome, cabeça, cauda, pai)
BONES = [
    ("Root", (0, 0, 0), (0, 0, .5), None),
    ("Hips", (0, 0, 2.3), (0, 0, 2.9), "Root"),
    ("Spine", (0, 0, 2.9), (0, 0, 3.9), "Hips"),
    ("Chest", (0, 0, 3.9), (0, 0, 5.4), "Spine"),
    ("Core", (0, .95, 4.55), (0, 1.45, 4.55), "Chest"),
    ("Neck", (0, 0, 5.4), (0, 0, 5.8), "Chest"),
    ("Head", (0, 0, 5.8), (0, 0, 6.8), "Neck"),
    ("Shoulder_R", (.9, 0, 5.0), (1.9, 0, 5.0), "Chest"),
    ("UpperArm_R", (1.95, 0, 5.0), (2.15, 0, 3.6), "Shoulder_R"),
    ("Forearm_R", (2.15, 0, 3.6), (2.25, .15, 2.35), "UpperArm_R"),
    ("Hand_R", (2.25, .15, 2.35), (2.25, .2, 1.55), "Forearm_R"),
    ("Shoulder_L", (-.9, 0, 5.0), (-1.9, 0, 5.0), "Chest"),
    ("UpperArm_L", (-1.95, 0, 5.0), (-2.15, 0, 3.6), "Shoulder_L"),
    ("Forearm_L", (-2.15, 0, 3.6), (-2.25, .15, 2.35), "UpperArm_L"),
    ("Hand_L", (-2.25, .15, 2.35), (-2.25, .2, 1.55), "Forearm_L"),
    ("Thigh_R", (.72, 0, 2.35), (.8, 0, 1.25), "Hips"),
    ("Shin_R", (.8, 0, 1.25), (.8, 0, .4), "Thigh_R"),
    ("Foot_R", (.8, 0, .4), (.8, .7, .12), "Shin_R"),
    ("Thigh_L", (-.72, 0, 2.35), (-.8, 0, 1.25), "Hips"),
    ("Shin_L", (-.8, 0, 1.25), (-.8, 0, .4), "Thigh_L"),
    ("Foot_L", (-.8, 0, .4), (-.8, .7, .12), "Shin_L"),
    ("ShardRing", (0, 0, 4.6), (0, 0, 5.1), "Root"),
]
SHARD_RADIUS = 3.4
for i in range(4):
    a = math.radians(45 + 90 * i)
    x, y = math.cos(a) * SHARD_RADIUS, math.sin(a) * SHARD_RADIUS
    z = 4.4 + (.6 if i % 2 else 0)
    BONES.append(("Shard_%d" % (i + 1), (x, y, z), (x, y, z + .5), "ShardRing"))

arm_data = bpy.data.armatures.new("AzhurRig")
rig = bpy.data.objects.new("Azhur", arm_data)
scene.collection.objects.link(rig)
bpy.context.view_layer.objects.active = rig
rig.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
for name, head, tail, parent in BONES:
    b = arm_data.edit_bones.new(name)
    b.head, b.tail = head, tail
    b.roll = 0
    if parent:
        b.parent = arm_data.edit_bones[parent]
bpy.ops.object.mode_set(mode='OBJECT')

# ---- blocos ------------------------------------------------------------------------------------
verts, faces, face_mats, vert_bone, vert_col = [], [], [], [], []

def shade(mat):
    base = {"Rock": .85, "Obsidian": .9, "Lava": .92, "Core": 1.0}[mat]
    v = base + rng.uniform(-.12, .12) if mat in ("Rock", "Obsidian") else base + rng.uniform(-.06, .06)
    tint = rng.uniform(-.04, .04)
    return (min(1, v + tint), min(1, v), min(1, v - tint), 1.0)

MAIN = []   # blocos grandes de pedra (a segunda passada põe lascas e rachaduras neles)

def box(center, size, mat, bone, rot=(0, 0, 0), bevel=.06, tone=1.0):
    """Bloco chanfrado (um cubo com as quinas cortadas), preso a um osso."""
    if mat in ("Rock", "Obsidian") and sorted(size)[1] >= .6:
        MAIN.append((tuple(center), tuple(size), mat, bone, tuple(rot)))
    cx, cy, cz = center
    sx, sy, sz = (s * .5 for s in size)
    b = min(bevel, sx * .4, sy * .4, sz * .4)
    R = Euler(tuple(math.radians(r) for r in rot)).to_matrix()
    base = len(verts)
    # 24 vértices: cada face com seus 4 cantos recuados pela chanfradura
    pts = []
    for axis in range(3):
        for sign in (-1, 1):
            corners = []
            for u in (-1, 1):
                for v in (-1, 1):
                    p = [0, 0, 0]
                    ext = [sx, sy, sz]
                    p[axis] = sign * ext[axis]
                    a1, a2 = [i for i in range(3) if i != axis]
                    p[a1] = u * (ext[a1] - b)
                    p[a2] = v * (ext[a2] - b)
                    corners.append(p)
            pts.append(corners)
    col = tuple(min(1.0, c * tone) for c in shade(mat)[:3]) + (1.0,)
    for corners in pts:
        for p in corners:
            q = R @ Vector(p)
            verts.append((cx + q.x, cy + q.y, cz + q.z))
            vert_bone.append(bone)
            vert_col.append(col)
    def idx(face, k): return base + face * 4 + k
    # faces principais (ordem de giro para fora)
    order = {0: (0, 1, 3, 2), 1: (0, 2, 3, 1), 2: (0, 2, 3, 1), 3: (0, 1, 3, 2), 4: (0, 1, 3, 2), 5: (0, 2, 3, 1)}
    for f in range(6):
        o = order[f]
        faces.append(tuple(idx(f, k) for k in o)); face_mats.append(MAT_INDEX[mat])
    # tiras das chanfraduras entre faces vizinhas (fecham o bloco)
    bm_faces = []
    # simples: cada par de faces adjacentes -> quad entre as arestas mais próximas
    for fa in range(6):
        for fb in range(fa + 1, 6):
            if fa // 2 == fb // 2:
                continue
            A = [idx(fa, k) for k in range(4)]
            B = [idx(fb, k) for k in range(4)]
            pairs = sorted(((Vector(verts[a]) - Vector(verts[c])).length, a, c) for a in A for c in B)
            used_a, used_b, chosen = set(), set(), []
            for d, a, c in pairs:
                if a in used_a or c in used_b:
                    continue
                chosen.append((a, c)); used_a.add(a); used_b.add(c)
                if len(chosen) == 2:
                    break
            (a1, c1), (a2, c2) = chosen
            faces.append((a1, a2, c2, c1)); face_mats.append(MAT_INDEX[mat])
    # cantos (triângulos entre três faces)
    for sx_ in (-1, 1):
        for sy_ in (-1, 1):
            for sz_ in (-1, 1):
                tri = []
                for axis, sgn in ((0, sx_), (1, sy_), (2, sz_)):
                    f = axis * 2 + (0 if sgn < 0 else 1)
                    target = Vector((cx, cy, cz)) + R @ Vector((sx_ * sx, sy_ * sy, sz_ * sz))
                    best = min((idx(f, k) for k in range(4)), key=lambda i: (Vector(verts[i]) - target).length)
                    tri.append(best)
                faces.append(tuple(tri)); face_mats.append(MAT_INDEX[mat])

def bar(a, b, thick, mat, bone):
    """Bloco comprido de a até b (veios de lava, grades)."""
    a, b = Vector(a), Vector(b)
    d = b - a
    length = d.length
    rot = d.to_track_quat('Z', 'Y').to_euler()
    box(tuple((a + b) * .5), (thick, thick, length), mat, bone, tuple(math.degrees(r) for r in rot), bevel=thick * .3)

def spike(base, height, width, mat, bone, tilt=(0, 0, 0), tip="Lava"):
    """Espinho em três blocos que afinam; a ponta em brasa."""
    R = Euler(tuple(math.radians(t) for t in tilt)).to_matrix()
    for k, (f, w) in enumerate(((0.0, 1.0), (.42, .7), (.74, .42))):
        c = Vector(base) + R @ Vector((0, 0, height * (f + .16)))
        box(tuple(c), (width * w, width * w, height * .36), mat if k < 2 else tip, bone, tilt, bevel=.04)

# quadril e cinto
box((0, 0, 2.62), (2.3, 1.45, .85), "Rock", "Hips")
for x in (-.85, -.28, .28, .85):
    box((x, .78, 2.5), (.5, .12, .62), "Obsidian", "Hips", (8, 0, 0))
bar((-1.0, .74, 2.92), (1.0, .74, 2.92), .08, "Lava", "Hips")
box((0, -.7, 2.6), (1.6, .2, .7), "Obsidian", "Hips")
# abdômen em gomos com lava entre eles
for k, z in enumerate((3.1, 3.42, 3.74)):
    box((0, 0, z), (1.85 - k * .05, 1.3, .26), "Rock", "Spine")
    bar((-.85, .66, z + .15), (.85, .66, z + .15), .06, "Lava", "Spine")
# peito: bloco grande, peitorais de obsidiana, o núcleo no meio
box((0, 0, 4.6), (3.05, 1.8, 1.6), "Rock", "Chest")
box((0, -.2, 5.25), (2.6, 1.5, .5), "Rock", "Chest")
for x in (-.82, .82):
    box((x, .93, 4.75), (1.25, .32, 1.05), "Obsidian", "Chest", (0, 0, -6 if x > 0 else 6))
    bar((x * 1.75, .9, 4.15), (x * .3, .95, 4.15), .07, "Lava", "Chest")
# veios saindo do núcleo pelo peito
for a in range(8):
    ang = math.radians(a * 45 + 22)
    bar((math.cos(ang) * .42, .96, 4.55 + math.sin(ang) * .42), (math.cos(ang) * 1.05, .92, 4.55 + math.sin(ang) * .78), .06, "Lava", "Chest")
# o núcleo (osso próprio: pulsa) e a grade de rocha em volta
box((0, 1.12, 4.55), (.62, .42, .62), "Core", "Core", bevel=.1)
box((0, 1.28, 4.55), (.36, .2, .36), "Core", "Core", (0, 45, 0), bevel=.06)
for x in (-.38, -.12, .12, .38):
    bar((x, 1.25, 4.15), (x, 1.25, 4.95), .09, "Obsidian", "Chest")
bar((-.5, 1.22, 4.98), (.5, 1.22, 4.98), .12, "Obsidian", "Chest")
bar((-.5, 1.22, 4.12), (.5, 1.22, 4.12), .12, "Obsidian", "Chest")
# costas: o vulcão que ele carrega (a Caldeira nas costas), rachaduras de lava
box((0, -1.15, 5.15), (1.7, .7, .9), "Rock", "Chest")
box((0, -1.35, 5.8), (1.3, .65, .6), "Rock", "Chest")
for x, y in ((-.5, -1.35), (.5, -1.35), (0, -1.1), (0, -1.65)):
    box((x, y, 6.3), (.5 if x else .9, .5 if not x else .45, .5), "Obsidian", "Chest", (0, 0, 0))
box((0, -1.37, 6.22), (.62, .38, .2), "Lava", "Chest", bevel=.03)
for x in (-.6, 0, .6):
    bar((x, -.92, 4.0), (x * 1.3, -.92, 5.3), .07, "Lava", "Chest")
bar((-1.2, -.92, 4.4), (1.2, -.92, 4.6), .06, "Lava", "Chest")
# costas: espinhos
for k, (x, z) in enumerate(((-.7, 5.2), (.7, 5.2), (-.45, 4.5), (.45, 4.5), (0, 3.7))):
    spike((x, -.85, z), 1.0 - k * .08, .38, "Obsidian", "Chest" if z > 4 else "Spine", (-55, 0, x * 25))
# pescoço e cabeça
box((0, .05, 5.62), (.95, .95, .5), "Rock", "Neck")
box((0, .1, 6.3), (1.55, 1.4, 1.15), "Rock", "Head")
box((0, .62, 6.62), (1.45, .32, .26), "Obsidian", "Head", (12, 0, 0))   # testa
box((0, .66, 6.0), (1.0, .26, .3), "Obsidian", "Head")                   # queixo
for x in (-.32, .32):
    box((x, .74, 6.38), (.26, .1, .13), "Lava", "Head")                  # olhos
bar((-.3, .8, 6.07), (.3, .8, 6.07), .07, "Lava", "Head")                 # boca
for side in (-1, 1):
    # chifres grossos: saem de lado, curvam para cima e para trás; a ponta em brasa
    box((side * .95, 0, 6.55), (.6, .55, .5), "Obsidian", "Head", (0, side * -20, 0))
    spike((side * 1.2, -.05, 6.65), 1.25, .66, "Obsidian", "Head", (-15, side * 52, 0))
    spike((side * 1.62, -.12, 7.25), .7, .4, "Obsidian", "Head", (-30, side * 20, 0))
# coroa de espinhos em volta do topo da cabeça (um vulcão pequeno) com lava no meio
for k in range(6):
    ang = math.radians(k * 60 + 30)
    spike((math.cos(ang) * .55, .1 + math.sin(ang) * .5, 6.85), .55, .22, "Obsidian", "Head", (math.sin(ang) * -25, math.cos(ang) * 25, 0))
box((0, .1, 6.9), (.7, .7, .12), "Lava", "Head", bevel=.03)
# ombros: ombreiras com espinhos e borda de lava
for side, s in (("R", 1), ("L", -1)):
    box((s * 1.95, 0, 5.38), (1.35, 1.45, .75), "Obsidian", "Shoulder_" + side)
    bar((s * 1.35, .74, 5.05), (s * 2.6, .74, 5.05), .07, "Lava", "Shoulder_" + side)
    spike((s * 2.25, -.25, 5.7), 1.1, .44, "Obsidian", "Shoulder_" + side, (-15, s * 32, 0))
    spike((s * 1.8, .35, 5.72), .75, .34, "Obsidian", "Shoulder_" + side, (5, s * 18, 0))
    # respiro de lava no alto da ombreira
    box((s * 1.95, -.1, 5.83), (.55, .55, .2), "Rock", "Shoulder_" + side)
    box((s * 1.95, -.1, 5.95), (.36, .36, .08), "Lava", "Shoulder_" + side, bevel=.02)
    # braço
    box((s * 2.05, 0, 4.3), (.95, .95, 1.35), "Rock", "UpperArm_" + side)
    box((s * 2.15, 0, 3.6), (1.05, 1.05, .14), "Lava", "UpperArm_" + side, bevel=.03)
    box((s * 2.2, .08, 2.95), (1.05, 1.05, 1.15), "Rock", "Forearm_" + side)
    box((s * 2.2, .62, 2.95), (.8, .14, .95), "Obsidian", "Forearm_" + side)
    box((s * 2.62, .08, 2.95), (.14, .8, .95), "Obsidian", "Forearm_" + side)
    bar((s * 1.75, .62, 3.3), (s * 2.6, .62, 2.6), .06, "Lava", "Forearm_" + side)
    # punho enorme: nós de obsidiana com lava entre eles, gotas de lava embaixo
    box((s * 2.25, .2, 1.95), (1.35, 1.25, 1.15), "Rock", "Hand_" + side)
    for k in range(4):
        box((s * (1.82 + k * .29), .86, 2.12), (.25, .22, .32), "Obsidian", "Hand_" + side)
        if k < 3:
            box((s * (1.96 + k * .29), .84, 2.12), (.05, .16, .26), "Lava", "Hand_" + side, bevel=.01)
    box((s * 2.25, .2, 1.36), (.9, .9, .1), "Lava", "Hand_" + side, bevel=.03)
    for k in range(3):
        box((s * (2.0 + k * .25), .25 + k * .1, 1.25 - k * .07), (.12, .12, .14), "Lava", "Hand_" + side, bevel=.02)
    # pernas
    box((s * .77, 0, 1.8), (1.05, 1.05, 1.15), "Rock", "Thigh_" + side)
    box((s * .8, .58, 1.28), (.65, .22, .5), "Obsidian", "Shin_" + side)   # joelheira
    box((s * .8, 0, .8), (1.15, 1.1, .85), "Rock", "Shin_" + side)
    bar((s * .4, .56, 1.0), (s * 1.2, .56, .65), .06, "Lava", "Shin_" + side)
    box((s * .8, .3, .22), (1.3, 1.65, .45), "Rock", "Foot_" + side)
    for k in range(3):
        box((s * (.45 + k * .35), 1.12, .2), (.3, .25, .35), "Obsidian", "Foot_" + side)
# as pedras em brasa que giram em volta (cada uma: dois blocos tortos e o miolo de lava)
for i in range(4):
    name = "Shard_%d" % (i + 1)
    head = Vector(BONES[[b[0] for b in BONES].index(name)][1])
    s = .55 + .12 * (i % 2)
    box(tuple(head + Vector((0, 0, .25))), (s, s * .9, s * 1.1), "Rock", name, (rng.uniform(0, 40), rng.uniform(0, 40), rng.uniform(0, 90)))
    box(tuple(head + Vector((.12, -.05, .35))), (s * .75, s * .8, s * .7), "Obsidian", name, (rng.uniform(0, 60), 30, 15))
    box(tuple(head + Vector((0, 0, .3))), (s * .55, s * .55, s * .55), "Lava", name, (45, 45, 0))

# ---- detalhe: lascas, rachaduras de lava, runas, correntes, garras ---------------------------------
def on_face(center, size, rot, face, u, v, lift):
    """Ponto na face do bloco (face 0..5 = -X +X -Y +Y -Z +Z; u, v em -1..1) e a normal dela."""
    R = Euler(tuple(math.radians(r) for r in rot)).to_matrix()
    axis, sign = face // 2, (-1 if face % 2 == 0 else 1)
    a1, a2 = [i for i in range(3) if i != axis]
    p = [0.0, 0.0, 0.0]
    p[axis] = sign * (size[axis] * .5 + lift)
    p[a1] = u * size[a1] * .5
    p[a2] = v * size[a2] * .5
    n = [0.0, 0.0, 0.0]
    n[axis] = sign
    return Vector(center) + R @ Vector(p), R @ Vector(n), (a1, a2)

def chips_and_cracks(center, size, mat, bone, rot):
    area = size[0] * size[1] + size[1] * size[2] + size[0] * size[2]
    for _ in range(int(area * 5)):
        face = rng.choice((0, 1, 2, 3, 5))   # embaixo não aparece
        u, v = rng.uniform(-.82, .82), rng.uniform(-.82, .82)
        s = rng.uniform(.07, .2)
        at, n, _ = on_face(center, size, rot, face, u, v, s * .2)
        chip = "Obsidian" if (mat == "Rock" and rng.random() < .22) else ("Rock" if mat == "Obsidian" and rng.random() < .3 else mat)
        box(tuple(at), (s, s * rng.uniform(.7, 1.3), s * rng.uniform(.6, 1.2)), chip, bone,
            (rot[0] + rng.uniform(-14, 14), rot[1] + rng.uniform(-14, 14), rot[2] + rng.uniform(-30, 30)), bevel=.02,
            tone=rng.choice((.62, .75, 1.3, 1.45)))
    # rachaduras: zigue-zague de lava nas faces de frente, de lado e de trás das pedras grandes
    if mat != "Rock":
        return
    for _ in range(max(1, int(area * .55))):
        face = rng.choice((0, 1, 2, 3))
        u, v = rng.uniform(-.6, .6), rng.uniform(-.7, .3)
        prev, _, _ = on_face(center, size, rot, face, u, v, .012)
        for k in range(rng.randint(3, 5)):
            u = max(-.9, min(.9, u + rng.uniform(-.35, .35)))
            v = max(-.9, min(.9, v + rng.uniform(.18, .38)))
            nxt, _, _ = on_face(center, size, rot, face, u, v, .012)
            bar(tuple(prev), tuple(nxt), .035, "Lava", bone)
            # galho curto de vez em quando
            if rng.random() < .3:
                side, _, _ = on_face(center, size, rot, face, u + rng.uniform(-.3, .3), v + .15, .012)
                bar(tuple(nxt), tuple(side), .028, "Lava", bone)
            prev = nxt

for center, size, mat, bone, rot in list(MAIN):
    chips_and_cracks(center, size, mat, bone, rot)

for side, sgn in (("R", 1), ("L", -1)):
    # runas em brasa no punho e na canela (anel de obsidiana com quadradinhos acesos)
    box((sgn * 2.22, .1, 2.42), (1.18, 1.18, .2), "Obsidian", "Forearm_" + side)
    for k in range(5):
        u = -.6 + k * .3
        box((sgn * 2.22 + u * .5, .7, 2.42), (.09, .05, .12 if k % 2 else .07), "Lava", "Forearm_" + side, bevel=.01)
        box((sgn * 2.81, .1 + u * .5, 2.42), (.05, .09, .12 if k % 2 == 0 else .07), "Lava", "Forearm_" + side, bevel=.01)
    for k in range(3):
        box((sgn * .8 + (k - 1) * .28, .57, .6), (.1, .05, .16), "Lava", "Shin_" + side, bevel=.01)
    # espinhos nos nós dos dedos e ao longo do antebraço
    for k in range(4):
        spike((sgn * (1.82 + k * .29), 1.0, 2.2), .38, .14, "Obsidian", "Hand_" + side, (-80, 0, 0))
    for k in range(3):
        spike((sgn * 2.7, -.1 + k * .1, 3.35 - k * .35), .5 - k * .08, .2, "Obsidian", "Forearm_" + side, (0, sgn * 75, 0))
    # garras nos dedos dos pés
    for k in range(3):
        spike((sgn * (.45 + k * .35), 1.3, .18), .35, .16, "Obsidian", "Foot_" + side, (-85, 0, 0))
    # corrente da ombreira até o cinto (elos alternados)
    a, b = Vector((sgn * 1.55, .9, 5.15)), Vector((sgn * .55, .85, 3.0))
    for k in range(9):
        c = a.lerp(b, k / 8) + Vector((0, .06 * math.sin(k / 8 * math.pi), 0))
        box(tuple(c), (.13, .05, .22) if k % 2 else (.05, .13, .22), "Obsidian", "Chest" if k < 6 else "Spine", (0, 0, sgn * 25), bevel=.015)
    # placas da cintura caindo (saia de pedra) de lado
    box((sgn * 1.1, .1, 2.35), (.3, 1.1, .7), "Obsidian", "Hips", (0, sgn * 12, 0))
# crista de espinhos baixos ao longo da coluna
for k in range(6):
    z = 3.2 + k * .42
    spike((0, -.95 if z > 3.9 else -.72, z), .45, .2, "Obsidian", "Chest" if z > 3.9 else "Spine", (-70, 0, 0))
# lava pingando do queixo e da boca
for x, z in ((-.18, 5.88), (.05, 5.8), (.22, 5.9)):
    box((x, .8, z), (.07, .07, .16), "Lava", "Head", bevel=.015)
# respiro maior no vulcão das costas, com borda irregular
for k in range(8):
    ang = math.radians(k * 45)
    box((math.cos(ang) * .42, -1.37 + math.sin(ang) * .3, 6.5 + rng.uniform(0, .15)), (.22, .22, .3 + rng.uniform(0, .2)), "Rock" if k % 2 else "Obsidian", "Chest",
        (rng.uniform(-10, 10), rng.uniform(-10, 10), k * 45), bevel=.03)
box((0, -1.37, 6.42), (.5, .32, .1), "Lava", "Chest", bevel=.02)

mesh = bpy.data.meshes.new("AzhurBody")
mesh.from_pydata(verts, [], faces)
mesh.update()
for m in MATS.values():
    mesh.materials.append(m)
for poly, mi in zip(mesh.polygons, face_mats):
    poly.material_index = mi
    poly.use_smooth = False
color = mesh.color_attributes.new("Col", 'BYTE_COLOR', 'POINT')
for i, c in enumerate(vert_col):
    color.data[i].color = c
mesh.validate()
body = bpy.data.objects.new("AzhurBody", mesh)
scene.collection.objects.link(body)
for name, *_ in BONES:
    body.vertex_groups.new(name=name)
for i, bone in enumerate(vert_bone):
    body.vertex_groups[bone].add([i], 1.0, 'REPLACE')
body.parent = rig
mod = body.modifiers.new("Rig", 'ARMATURE')
mod.object = rig
# normais para fora (os blocos foram montados à mão)
bpy.context.view_layer.objects.active = body
body.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.remove_doubles(threshold=.0005)
bpy.ops.mesh.normals_make_consistent(inside=False)
bpy.ops.object.mode_set(mode='OBJECT')
body.select_set(False)

# ---- animações ---------------------------------------------------------------------------------
X, Y, Z = Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))
REST = {b.name: b.matrix_local.to_quaternion() for b in arm_data.bones}

def world_rot(bone, steps):
    """Giros em eixos do mundo (na pose de descanso) -> rotação local do osso."""
    q = Quaternion()
    for axis, deg in steps:
        q = q @ Quaternion(axis, math.radians(deg))
    r = REST[bone]
    return r.inverted() @ q @ r

def world_move(bone, offset):
    return REST[bone].inverted() @ Vector(offset)

def smooth(t):
    # quíntica: começa e para sem tranco (a cúbica dava um solavanco no começo de cada chave)
    t = max(0.0, min(1.0, t))
    return t * t * t * (t * (6 * t - 15) + 10)

def key(points, t):
    """Interpola entre chaves [(tempo, valor)] com suavização."""
    if t <= points[0][0]:
        return points[0][1]
    for (t0, v0), (t1, v1) in zip(points, points[1:]):
        if t <= t1:
            a = smooth((t - t0) / max(1e-6, t1 - t0))
            return v0 + (v1 - v0) * a
    return points[-1][1]

# 02/11: as animações ficam em Tools/Blender/azhur_moves.py (refeitas, mais naturais, e as novas das habilidades)
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import azhur_moves as moves
moves.setup(X, Y, Z, key)


# ---- fluidez -------------------------------------------------------------------------------------
# Cada clipe acima diz só os grandes movimentos (ombro, tronco, pernas). Aqui, em todos:
# - o giro do peito se espalha pela coluna (curva em vez de dobradiça) e o da cabeça passa um pouco pelo pescoço;
# - o que vem na ponta chega depois: antebraço, mão e cabeça seguem o osso de cima com atraso (o peso da pedra);
# - respiração pesada contínua (peito, ombros), menos na morte.
LAG = [("Forearm_R", "UpperArm_R", .1, .35), ("Forearm_L", "UpperArm_L", .1, .35),
       ("Hand_R", "Forearm_R", .1, .5), ("Hand_L", "Forearm_L", .1, .5),
       ("Head", "Chest", .14, .3), ("Spine", "Hips", .08, .25)]


def scaled(steps, k):
    return [(axis, deg * k) for axis, deg in steps]


def fluid(fn, t, T, loop):
    p = fn(t, T)
    rot = p.setdefault("rot", {})

    def at(when):
        if loop:
            when %= T
        return fn(max(0.0, min(T, when)), T).get("rot", {})

    # curva da coluna
    if "Chest" in rot and "Spine" not in rot:
        rot["Spine"] = scaled(rot["Chest"], .4)
        rot["Chest"] = scaled(rot["Chest"], .6)
    if "Head" in rot and "Neck" not in rot:
        rot["Neck"] = scaled(rot["Head"], .35)
        rot["Head"] = scaled(rot["Head"], .65)
    # atraso: a ponta segue o quanto o pai girou um pouco antes (o contrário do giro dele agora)
    for child, parent, delay, amount in LAG:
        now = rot.get(parent)
        if not now:
            continue
        before = at(t - delay).get(parent, [])
        extra = scaled(before, amount) + scaled(now, -amount)
        rot[child] = rot.get(child, []) + extra
    # respiração
    if fn is not moves.death and fn is not moves.stun:
        b = math.sin(2 * math.pi * t / (T if loop else 2.0))
        rot["Chest"] = rot.get("Chest", []) + [(X, 1.2 * b)]
        rot["Shoulder_R"] = rot.get("Shoulder_R", []) + [(Y, -1.5 * b)]
        rot["Shoulder_L"] = rot.get("Shoulder_L", []) + [(Y, 1.5 * b)]
    return p


LOOPS = {name for name, _, _, loop in moves.CLIPS if loop}
CLIPS = [(name, fn, seconds) for name, fn, seconds, _ in moves.CLIPS]

rig.animation_data_create()
pose_bones = rig.pose.bones
for pb in pose_bones:
    pb.rotation_mode = 'QUATERNION'
for name, fn, seconds in CLIPS:
    action = bpy.data.actions.new(name)
    action.use_fake_user = True
    rig.animation_data.action = action
    frames = int(round(seconds * FPS))
    for f in range(0, frames + 1):
        t = f / FPS
        p = fluid(fn, t, seconds, name in LOOPS)
        for pb in pose_bones:
            pb.rotation_quaternion = world_rot(pb.name, p.get("rot", {}).get(pb.name, []))
            pb.location = world_move(pb.name, p.get("move", {}).get(pb.name, (0, 0, 0)))
            s = p.get("scale", {}).get(pb.name, 1.0)
            pb.scale = (s, s, s)
            pb.keyframe_insert("rotation_quaternion", frame=f)
            pb.keyframe_insert("location", frame=f)
            pb.keyframe_insert("scale", frame=f)
    action.frame_range = (0, frames)
    track = rig.animation_data.nla_tracks.new()
    track.name = name
    track.strips.new(name, 0, action)
rig.animation_data.action = None

# ---- prévias -----------------------------------------------------------------------------------
if PREVIEW:
    os.makedirs(PREVIEW, exist_ok=True)
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
    scene.collection.objects.link(cam)
    scene.camera = cam
    target = bpy.data.objects.new("Target", None)
    target.location = (0, 0, 3.4)
    scene.collection.objects.link(target)
    track_c = cam.constraints.new('TRACK_TO')
    track_c.target = target
    track_c.track_axis = 'TRACK_NEGATIVE_Z'
    track_c.up_axis = 'UP_Y'
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN'))
    sun.rotation_euler = (math.radians(55), 0, math.radians(25))
    sun.data.energy = 3
    scene.collection.objects.link(sun)
    scene.render.engine = 'BLENDER_EEVEE' if 'BLENDER_EEVEE' in [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items] else 'BLENDER_EEVEE_NEXT'
    scene.render.resolution_x, scene.render.resolution_y = 640, 640
    world = bpy.data.worlds.new("W")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs[0].default_value = (.05, .03, .03, 1)
    scene.world = world
    shots = [("front", "Idle", 0, (2, 13, 5)), ("side", "Idle", 0, (13, 3, 4.5)), ("back", "Idle", 0, (-6, -11, 6)),
             ("punch", "Punch", 26, (10, 10, 4)), ("seismic", "Seismic", 36, (11, 9, 5)), ("cataclysm", "Cataclysm", 42, (12, 10, 5)),
             ("awaken", "Awaken", 55, (9, 11, 4)), ("death", "Death", 85, (12, 8, 4))]
    for label, clip, frame, at in shots:
        for track in rig.animation_data.nla_tracks:
            track.mute = track.name != clip
        scene.frame_set(frame)
        cam.location = at
        scene.render.filepath = os.path.join(PREVIEW, "azhur_%s.png" % label)
        bpy.ops.render.render(write_still=True)
    for track in rig.animation_data.nla_tracks:
        track.mute = False

# ---- exportar ----------------------------------------------------------------------------------
os.makedirs(os.path.dirname(OUT) or ".", exist_ok=True)
bpy.ops.object.select_all(action='DESELECT')
rig.select_set(True)
body.select_set(True)
bpy.context.view_layer.objects.active = rig
bpy.ops.export_scene.gltf(filepath=OUT, export_format='GLB', use_selection=True, export_animations=True,
                          export_animation_mode='ACTIONS', export_apply=False, export_yup=True,
                          export_vertex_color='ACTIVE', export_materials='EXPORT')
print("AZHUR_DONE", OUT, len(verts), "vértices", len(faces), "faces")
