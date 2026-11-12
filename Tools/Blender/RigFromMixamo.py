"""O esqueleto completo do Mixamo (o do humano) encaixado numa raça que não foi rigada no Mixamo (elfo, demônio,
morto-vivo), com os pesos que o modelo dela já tinha — sem janela ou dentro do Blender (MCP).

blender -b --factory-startup -P Tools/Blender/RigFromMixamo.py -- <raiz do projeto> <raça>
(no Blender aberto: RACE = "elfo"; exec(open(".../RigFromMixamo.py").read()))

1. o modelo antigo da raça (Data/Heroes/<raça>/Hero_<raça>.glb, de Tools/Blender/RigHero.py): a malha com os pesos no
   esqueleto simples (Hips, Torso, Head, UpperArm, Forearm, Hand, Thigh, Shin, Foot) que já funcionava no jogo;
2. o esqueleto do humano do Mixamo (Data/Heroes/Mixamo/humano/humano.fbx), com cada junta movida para a junta da raça:
   quadril, coxas, joelhos, tornozelos, ombros, cotovelos e pulsos nos do modelo antigo; a coluna dividida entre o
   quadril e o pescoço como no humano; a cabeça e a ponta do pé com as proporções do humano;
3. os pesos: Torso dividido entre Spine, Spine1 e Spine2 pela altura (e os ombros); Head entre Neck e Head; o resto
   osso a osso;
4. sai Data/Heroes/Mixamo/<raça>/<raça>.fbx, no formato de um FBX "With Skin" do Mixamo: o Tools/Blender/MixamoToHero.py
   faz o resto (clipes do humano e do anão, rosto, arco, corrida armada).
"""
import os
import sys

import bpy
from mathutils import Vector

if "--" in sys.argv:
    ROOT, RACE = sys.argv[sys.argv.index("--") + 1:][:2]
else:
    ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
    RACE = globals().get("RACE", "elfo")
P = "mixamorig:"


def log(*parts):
    print("RIG_MIXAMO", RACE, " ".join(str(p) for p in parts))


for o in list(bpy.data.objects):
    bpy.data.objects.remove(o, do_unlink=True)
bpy.data.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)

# ---- o modelo antigo da raça
bpy.ops.import_scene.gltf(filepath=os.path.join(ROOT, "Data", "Heroes", RACE, "Hero_%s.glb" % RACE))
old = next(o for o in bpy.data.objects if o.type == "ARMATURE")
body = max((o for o in bpy.data.objects if o.type == "MESH"), key=lambda o: len(o.data.vertices))
for o in list(bpy.data.objects):
    if o.type == "MESH" and o != body:
        bpy.data.objects.remove(o, do_unlink=True)
if old.animation_data:
    old.animation_data.action = None
for pb in old.pose.bones:
    pb.matrix_basis.identity()
bpy.context.view_layer.update()
J = {b.name: old.matrix_world @ b.head_local for b in old.data.bones}
JT = {b.name: old.matrix_world @ b.tail_local for b in old.data.bones}
points = [body.matrix_world @ v.co for v in body.data.vertices]
top = max(p.z for p in points)

# ---- o esqueleto do humano do Mixamo (sem as imagens do FBX: só os ossos interessam)
import bpy_extras.image_utils as image_utils
original = image_utils.load_image
image_utils.load_image = lambda *a, **k: bpy.data.images.get("_vazio") or bpy.data.images.new("_vazio", 1, 1)
before = set(bpy.data.objects)
try:
    bpy.ops.import_scene.fbx(filepath=os.path.join(ROOT, "Data", "Heroes", "Mixamo", "humano", "humano.fbx"),
                             automatic_bone_orientation=False, ignore_leaf_bones=False, use_image_search=False)
finally:
    image_utils.load_image = original
new = [o for o in bpy.data.objects if o not in before]
rig = next(o for o in new if o.type == "ARMATURE")
for o in new:
    if o.type == "MESH":
        bpy.data.objects.remove(o, do_unlink=True)
rig.animation_data_clear()
rig.name = "Armature"
bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True)
bpy.context.view_layer.objects.active = rig
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
H = {b.name: b.head_local.copy() for b in rig.data.bones}       # o humano (1,70 m)
HT = {b.name: b.tail_local.copy() for b in rig.data.bones}
k = (top - 0) / 1.70                                              # do humano do Mixamo para a raça

# ---- as juntas novas
pos = {}
pos["Hips"] = J["Hips"].copy()
neck = J["Head"].copy()                                           # no modelo antigo o osso Head começa no pescoço
human_neck, human_hips = H[P + "Neck"], H[P + "Hips"]
for name in ("Spine", "Spine1", "Spine2"):
    t = (H[P + name].z - human_hips.z) / max(1e-4, human_neck.z - human_hips.z)
    pos[name] = pos["Hips"].lerp(neck, t)
pos["Neck"] = neck
# a cabeça: do pescoço até o topo do modelo, nas proporções do humano
human_top = HT[P + "HeadTop_End"] if P + "HeadTop_End" in HT else H[P + "Head"]
span = max(1e-4, human_top.z - human_neck.z)
race_span = top - neck.z
for name in ("Head", "HeadTop_End"):
    if P + name in H:
        d = H[P + name] - human_neck
        pos[name] = neck + Vector((d.x * k, d.y * k, d.z * race_span / span))
for side, s in (("Left", "L"), ("Right", "R")):
    pos[side + "Arm"] = J["UpperArm_" + s].copy()
    pos[side + "ForeArm"] = J["Forearm_" + s].copy()
    pos[side + "Hand"] = J["Hand_" + s].copy()
    # o ombro entre o peito e o braço, como no humano
    t = (H[P + side + "Shoulder"] - H[P + "Spine2"]).length / max(1e-4, (H[P + side + "Arm"] - H[P + "Spine2"]).length)
    pos[side + "Shoulder"] = pos["Spine2"].lerp(pos[side + "Arm"], t) + Vector((0, 0, (pos[side + "Arm"].z - pos["Spine2"].z) * .0))
    pos[side + "UpLeg"] = J["Thigh_" + s].copy()
    pos[side + "Leg"] = J["Shin_" + s].copy()
    pos[side + "Foot"] = J["Foot_" + s].copy()
    for name in ("ToeBase", "Toe_End"):
        if P + side + name in H:
            d = H[P + side + name] - H[P + side + "Foot"]
            pos[side + name] = pos[side + "Foot"] + d * k
    # a ponta da mão (o modelo não tem dedos): continua o antebraço com o tamanho da mão do humano
    hand_len = (HT[P + side + "Hand"] - H[P + side + "Hand"]).length * k
    pos[side + "HandTip"] = pos[side + "Hand"] + (pos[side + "Hand"] - pos[side + "ForeArm"]).normalized() * hand_len

bpy.ops.object.mode_set(mode="EDIT")
eb = rig.data.edit_bones
for b in eb:
    name = b.name[len(P):]
    if name in pos:
        b.head = pos[name]
for b in eb:
    name = b.name[len(P):]
    kids = [c for c in b.children]
    main = next((c for c in kids if c.name[len(P):] in ("Spine", "Spine1", "Spine2", "Neck", "Head", "HeadTop_End")), None)
    if main is None and len(kids) == 1:
        main = kids[0]
    if name.endswith("Hand") and name + "Tip" in pos:
        b.tail = pos[name + "Tip"]
    elif main is not None and (main.head - b.head).length > 1e-4:
        b.tail = main.head
    else:
        d = HT[b.name] - H[b.name]
        b.tail = b.head + d * k
bpy.ops.object.mode_set(mode="OBJECT")
log("juntas encaixadas: quadril %.2f, pescoço %.2f, topo %.2f m" % (pos["Hips"].z, neck.z, top))

# ---- os pesos do modelo antigo nos ossos do Mixamo
groups = {g.index: g.name for g in body.vertex_groups}
weights = []
for v in body.data.vertices:
    weights.append({groups[g.group]: g.weight for g in v.groups if g.weight > 0})
for g in list(body.vertex_groups):
    body.vertex_groups.remove(g)
out = {}


def add(i, bone, w):
    if w <= 0:
        return
    if bone not in out:
        out[bone] = body.vertex_groups.new(name=P + bone)
    out[bone].add([i], w, "ADD")


spine_z = [(n, pos[n].z) for n in ("Hips", "Spine", "Spine1", "Spine2", "Neck")]


def split(z, chain):
    """O peso entre dois ossos vizinhos da cadeia pela altura do vértice."""
    for (a, za), (b, zb) in zip(chain, chain[1:]):
        if z <= zb:
            t = max(0.0, min(1.0, (z - za) / max(1e-4, zb - za)))
            return [(a, 1 - t), (b, t)]
    return [(chain[-1][0], 1.0)]


SIMPLE = {"Hips": "Hips", "Root": "Hips", "UpperArm_L": "LeftArm", "Forearm_L": "LeftForeArm", "Hand_L": "LeftHand",
          "UpperArm_R": "RightArm", "Forearm_R": "RightForeArm", "Hand_R": "RightHand", "Thigh_L": "LeftUpLeg",
          "Shin_L": "LeftLeg", "Foot_L": "LeftFoot", "Thigh_R": "RightUpLeg", "Shin_R": "RightLeg", "Foot_R": "RightFoot",
          "Sheath": "Spine2", "Sword_Hilt": "Spine2", "Socket_Weapon_R": "RightHand", "Sword": "RightHand"}
for i, ws in enumerate(weights):
    p = points[i]
    for name, w in ws.items():
        if name == "Torso":
            # o ombro: a parte do tronco depois do ombro, para o lado do braço
            side = "Left" if p.x > 0 else "Right"
            sh = pos[side + "Shoulder"]
            reach = abs(p.x) - abs(sh.x)
            if reach > 0 and p.z > pos["Spine2"].z:
                t = min(1.0, reach / max(1e-4, abs(pos[side + "Arm"].x) - abs(sh.x)))
                add(i, side + "Shoulder", w * t)
                w *= 1 - t
            # o resto pela altura, entre as vértebras (o que passa do pescoço fica no peito)
            for bone, f in split(min(p.z, pos["Neck"].z), spine_z[1:4]):
                add(i, bone, w * f)
            if p.z < pos["Spine"].z:
                pass
        elif name == "Head":
            # abaixo da junta da cabeça: o pescoço
            for bone, f in split(p.z, [("Neck", pos["Neck"].z), ("Head", pos["Head"].z)]):
                add(i, bone, w * f)
        elif name in SIMPLE:
            add(i, SIMPLE[name], w)
body.parent = rig
for m in list(body.modifiers):
    body.modifiers.remove(m)
mod = body.modifiers.new("Armature", "ARMATURE")
mod.object = rig
bpy.data.objects.remove(old, do_unlink=True)
body.name = RACE
log("pesos: %d ossos com peso, %d vértices" % (len(out), len(points)))

# ---- o FBX "With Skin" da raça (o MixamoToHero.py lê este)
folder = os.path.join(ROOT, "Data", "Heroes", "Mixamo", RACE)
os.makedirs(folder, exist_ok=True)
bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True)
body.select_set(True)
bpy.context.view_layer.objects.active = rig
path = os.path.join(folder, RACE + ".fbx")
bpy.ops.export_scene.fbx(filepath=path, use_selection=True, add_leaf_bones=False, bake_anim=False,
                         object_types={"ARMATURE", "MESH"}, path_mode="COPY", embed_textures=False)
log("exportado", path)
