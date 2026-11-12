"""Herói novo (modelo do Docs, com o esqueleto automático do site) no esqueleto do aventureiro, para todas as
animações atuais servirem nele.

blender -b --factory-startup -P Tools/Blender/RigHero.py -- <raiz do projeto> <modelo.glb> <raça> [--preview] [--walls] [--beard] [--mixamo <fbx>]

1. Lê o esqueleto do aventureiro (Data/GodotCharacter/Adventurer_Skinned.glb): ossos, juntas e a altura.
2. Lê o modelo novo, tira a esfera que sobra dos sites, e põe na altura do aventureiro.
3. Descobre no esqueleto automático (Bone_000...) as correntes: quadril e coluna, cabeça, braços (clavícula, braço,
   antebraço, mão e dedos) e pernas (coxa, canela, pé), pela posição de cada osso.
4. Dobra os braços e as pernas do modelo (com os pesos que vieram dele) até a postura de descanso do aventureiro
   (braços caídos, pernas retas) e fixa essa forma na malha.
5. Monta o esqueleto do aventureiro com as juntas no corpo novo (a mesma orientação de cada osso; só a posição
   muda) e passa os pesos de pele de cada osso do site para o osso do aventureiro que faz o mesmo papel.
6. Exporta Data/Heroes/<raça>/Hero_<raça>.glb (malha + esqueleto, sem animação) e, com --preview, fotos do herói
   parado, correndo e golpeando com as animações do aventureiro (Saved/Previews/hero_<raça>_*.png).

Na Unreal (Tools/ImportHeroes.py) a malha usa o mesmo esqueleto do aventureiro; a translação dos ossos vem da malha
(modo "Skeleton"), então cada herói anima com as próprias proporções.
"""
import math
import os
import sys

import bpy
from mathutils import Matrix, Vector

args = sys.argv[sys.argv.index("--") + 1:]
ROOT, MODEL, RACE = args[0], args[1], args[2]
PREVIEW = "--preview" in args
WALLS = "--walls" in args          # tirar sobras grandes grudadas no modelo (a parede do elfo)
# o esqueleto do Mixamo no lugar do do site (--mixamo <FBX "With Skin">): as juntas e os pesos que o Mixamo achou no
# próprio modelo. O do site deixava o ombro do anão na clavícula, 15 cm para dentro do corpo, e o braço girava dali
MIXAMO = args[args.index("--mixamo") + 1] if "--mixamo" in args else None
BEARD = "--beard" in args          # a barba segue a cabeça, não o peito (o anão)
ADVENTURER =os.path.join(ROOT, "Data", "GodotCharacter", "Adventurer_Skinned.glb")
OUT_DIR = os.path.join(ROOT, "Data", "Heroes", RACE)
SHOTS = os.path.join(ROOT, "Saved", "Previews")
report = []


def log(*parts):
    line = " ".join(str(p) for p in parts)
    report.append(line)
    print("RIG_HERO", line)


def meshes():
    return [o for o in bpy.data.objects if o.type == "MESH"]


def height(objs):
    zs = [(o.matrix_world @ v.co).z for o in objs for v in o.data.vertices]
    return min(zs), max(zs)


bpy.ops.wm.read_factory_settings(use_empty=True)

# ---- 1. o aventureiro
bpy.ops.import_scene.gltf(filepath=ADVENTURER)
adv = next(o for o in bpy.data.objects if o.type == "ARMATURE")
adv.name = "AdvRef"
adv_body = max(meshes(), key=lambda o: len(o.data.vertices))
# a altura do corpo: do chão ao alto da cabeça (sem a espada na bainha, que passa da cabeça)
head_group = adv_body.vertex_groups["Head"].index
adv_low = min((adv_body.matrix_world @ v.co).z for v in adv_body.data.vertices)
adv_high = max((adv_body.matrix_world @ v.co).z for v in adv_body.data.vertices
               if any(g.group == head_group and g.weight > .5 for g in v.groups))
ADV_H = adv_high - adv_low
A = {b.name: {"head": adv.matrix_world @ b.head_local, "tail": adv.matrix_world @ b.tail_local,
              "parent": b.parent.name if b.parent else None} for b in adv.data.bones}
adv_actions = list(bpy.data.actions)
for o in list(meshes()):
    bpy.data.objects.remove(o, do_unlink=True)
log("aventureiro", len(A), "ossos, altura %.3f" % ADV_H, len(adv_actions), "animações")

# ---- 2. o modelo novo
before = set(bpy.data.objects)
if MIXAMO:
    bpy.ops.import_scene.fbx(filepath=MIXAMO, automatic_bone_orientation=False, ignore_leaf_bones=False)
    for o in bpy.data.objects:
        if o not in before and o.type == "ARMATURE":
            o.animation_data_clear()          # a malha na pose em que foi montada (o clipe do FBX sai)
else:
    bpy.ops.import_scene.gltf(filepath=MODEL)
new_objs = [o for o in bpy.data.objects if o not in before]
rig = next(o for o in new_objs if o.type == "ARMATURE")
# o corpo é a malha maior; o resto (a esfera que os sites deixam, a parede do elfo) sai
parts = [o for o in new_objs if o.type == "MESH"]
body = max(parts, key=lambda o: len(o.data.vertices))
for o in parts:
    if o != body:
        bpy.data.objects.remove(o, do_unlink=True)
# sobras grudadas no modelo (a parede atrás do elfo): peças largas ou fundas demais para ser parte do corpo. O modelo
# do site vem em triângulos soltos (cada pedaço da textura separado): as peças são achadas numa cópia com os vértices
# iguais soldados, e as faces delas saem do original
import bmesh as _bm
weld = _bm.new() if WALLS else None
if weld:
    weld.from_mesh(body.data)
    _bm.ops.remove_doubles(weld, verts=weld.verts, dist=1e-4)
    weld.faces.ensure_lookup_table()
zs = [v.co.z for v in weld.verts] if weld else [0, 1]
tall = max(zs) - min(zs)
seen = set()
wall_centers = set()
for f in (weld.faces if weld else []):
    if f.index in seen:
        continue
    stack, piece = [f], []
    seen.add(f.index)
    while stack:
        x = stack.pop()
        piece.append(x)
        for e in x.edges:
            for o in e.link_faces:
                if o.index not in seen:
                    seen.add(o.index)
                    stack.append(o)
    vs = {v for face in piece for v in face.verts}
    xs = [v.co.x for v in vs]
    ys = [v.co.y for v in vs]
    if max(xs) - min(xs) > .75 * tall or max(ys) - min(ys) > .45 * tall:
        for face in piece:
            c = face.calc_center_median()
            wall_centers.add((round(c.x, 4), round(c.y, 4), round(c.z, 4)))
if weld:
    weld.free()
removed = 0
if wall_centers:
    bm = _bm.new()
    bm.from_mesh(body.data)
    gone = [f for f in bm.faces if tuple(round(v, 4) for v in f.calc_center_median()) in wall_centers]
    removed = len(gone)
    _bm.ops.delete(bm, geom=gone, context="FACES")
    loose = [v for v in bm.verts if not v.link_faces]
    _bm.ops.delete(bm, geom=loose, context="VERTS")
    bm.to_mesh(body.data)
    bm.free()
if removed:
    log("sobras tiradas:", removed, "faces")
low, high = height([body])
k = ADV_H / (high - low)
rig.scale = rig.scale * k
rig.location = rig.location * k
bpy.context.view_layer.update()
bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True)
body.select_set(True)
bpy.context.view_layer.objects.active = rig
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
low, high = height([body])
log("modelo", os.path.basename(MODEL), len(body.data.vertices), "vértices, escala %.3f, altura %.3f" % (k, high - low))

# ---- 3. as correntes do esqueleto automático
B = {b.name: b for b in rig.data.bones}
W = rig.matrix_world


def head(b):
    return W @ b.head_local


def tail(b):
    return W @ b.tail_local


def subtree_size(b):
    return 1 + sum(subtree_size(c) for c in b.children)


def main_chain(b):
    """b e, a cada passo, o filho com a maior descendência (o caminho principal: o braço, não o dedo)."""
    chain = [b]
    while chain[-1].children:
        chain.append(max(chain[-1].children, key=subtree_size))
    return chain


root = next(b for b in rig.data.bones if not b.parent)
if MIXAMO:
    root = next(b for b in rig.data.bones if b.name.endswith("Hips"))
    X = root.name[:-len("Hips")]                  # "mixamorig:"
    MIXAMO_ROLES = {"Hips": "Hips", "Neck": "Head", "LeftArm": "UpperArm_L", "LeftForeArm": "Forearm_L", "LeftHand": "Hand_L",
                    "RightArm": "UpperArm_R", "RightForeArm": "Forearm_R", "RightHand": "Hand_R", "LeftUpLeg": "Thigh_L",
                    "LeftLeg": "Shin_L", "LeftFoot": "Foot_L", "RightUpLeg": "Thigh_R", "RightLeg": "Shin_R", "RightFoot": "Foot_R"}
    roles = {role: B[X + name] for name, role in MIXAMO_ROLES.items() if role != "Hips"}
    # cada osso do Mixamo vira o papel do ancestral mais perto com papel; a coluna e a clavícula são o tronco
    mapping = {}
    for b in rig.data.bones:
        p = b
        while p and p.name[len(X):] not in MIXAMO_ROLES and not p.name[len(X):].startswith(("Spine", "LeftShoulder", "RightShoulder")):
            p = p.parent
        short = p.name[len(X):] if p else "Hips"
        mapping[b.name] = MIXAMO_ROLES.get(short, "Torso")
    log("Mixamo:", len(rig.data.bones), "ossos", {k: v.name for k, v in roles.items()})
# coluna: do quadril para cima, sempre o filho mais alto e no meio
spine = [root]
while not MIXAMO:
    # no meio do corpo e sem descer muito (alguns esqueletos têm o osso da bacia um pouco abaixo da raiz)
    up = [c for c in spine[-1].children if abs(head(c).x) < .05 * ADV_H and head(c).z >= head(spine[-1]).z - .1 * ADV_H
          and max(tail(d).z for d in [c] + list(c.children_recursive)) > head(spine[-1]).z]
    if not up:
        break
    spine.append(max(up, key=lambda c: head(c).z + tail(c).z))
spine_set = set(spine)
# os ramos que saem da coluna: braços (acima do meio do corpo) e pernas (abaixo do quadril)
branches = [c for s in spine for c in s.children if c not in spine_set]
hips_z = head(root).z
legs = {}
arms = {}
for c in ([] if MIXAMO else branches):
    chain = main_chain(c)
    side = "R" if sum(head(b).x for b in chain) < 0 else "L"     # direita = -X (como o aventureiro)
    end = chain[-1]
    if tail(end).z < hips_z - .1 * ADV_H:
        legs[side] = chain
    elif abs(tail(end).x) > .12 * ADV_H:
        arms[side] = chain
log("coluna", [b.name for b in spine])
log("braços", {s: [b.name for b in c] for s, c in arms.items()}, "pernas", {s: [b.name for b in c] for s, c in legs.items()})
if not MIXAMO and (set(arms) != {"L", "R"} or set(legs) != {"L", "R"}):
    raise SystemExit("RIG_HERO_FAIL não achei os dois braços e as duas pernas")

if not MIXAMO:
    mapping = {}       # osso do site -> osso do aventureiro
    roles = {}         # papel -> osso do site (as juntas)
for side, chain in arms.items():
    lengths = [(tail(b) - head(b)).length for b in chain]
    longest = max(lengths[:4])
    upper = next(i for i, l in enumerate(lengths) if l >= .6 * longest)
    for i, b in enumerate(chain):
        mapping[b.name] = "Torso" if i < upper else "UpperArm_" + side if i == upper else "Forearm_" + side if i == upper + 1 else "Hand_" + side
    roles["UpperArm_" + side], roles["Forearm_" + side] = chain[upper], chain[upper + 1]
    roles["Hand_" + side] = chain[upper + 2] if upper + 2 < len(chain) else None
    # dedos e o resto da mão (os outros filhos da mão)
    for b in chain[upper + 2:]:
        for d in b.children_recursive:
            mapping.setdefault(d.name, "Hand_" + side)
for side, chain in legs.items():
    for i, b in enumerate(chain):
        mapping[b.name] = "Thigh_" + side if i == 0 else "Shin_" + side if i == 1 else "Foot_" + side
        for d in b.children_recursive:
            mapping.setdefault(d.name, "Foot_" + side if i >= 1 else mapping[b.name])
    roles["Thigh_" + side], roles["Shin_" + side] = chain[0], chain[1]
    roles["Foot_" + side] = chain[2] if len(chain) > 2 else None
shoulder_z = (head(roles["UpperArm_L"]).z + head(roles["UpperArm_R"]).z) / 2
neck = roles["Head"] if MIXAMO else next((b for b in spine if head(b).z > shoulder_z), spine[-1])
roles["Head"] = neck
for b in ([] if MIXAMO else spine):
    z = head(b).z
    mapping[b.name] = "Head" if z >= head(neck).z - 1e-4 else "Hips" if z < hips_z + .3 * (shoulder_z - hips_z) else "Torso"
# o que sobrou (cabelo, chifres, rabo, capa): o papel do ancestral mais perto que tem papel
for b in rig.data.bones:
    if b.name in mapping:
        continue
    p = b.parent
    while p and p.name not in mapping:
        p = p.parent
    mapping[b.name] = mapping[p.name] if p else "Hips"
log("papéis", {k: (v.name if v else None) for k, v in roles.items()})

# ---- 4. braços e pernas na postura de descanso do aventureiro
bpy.ops.object.select_all(action="DESELECT")
bpy.context.view_layer.objects.active = rig
rig.select_set(True)
bpy.ops.object.mode_set(mode="POSE")


def joint(name):
    return rig.matrix_world @ rig.pose.bones[name].head


def align(bone, child_head_of, target):
    """Gira o osso (na pose) para a direção junta->próxima junta ficar igual à do aventureiro."""
    pb = rig.pose.bones[bone.name]
    bpy.context.view_layer.update()
    here = joint(bone.name)
    there = child_head_of()
    now = (there - here).normalized()
    want = target.normalized()
    q = now.rotation_difference(want)
    world = rig.matrix_world @ pb.matrix
    pivot = Matrix.Translation(here)
    world = pivot @ q.to_matrix().to_4x4() @ pivot.inverted() @ world
    pb.matrix = rig.matrix_world.inverted() @ world
    bpy.context.view_layer.update()


def adv_dir(a, b):
    return A[b]["head"] - A[a]["head"]


for side in ("L", "R"):
    up, fore, hand = roles["UpperArm_" + side], roles["Forearm_" + side], roles["Hand_" + side]
    align(up, lambda: joint(fore.name), adv_dir("UpperArm_" + side, "Forearm_" + side))
    if hand:
        align(fore, lambda: joint(hand.name), adv_dir("Forearm_" + side, "Hand_" + side))
        tip = main_chain(hand)
        if len(tip) > 1:
            # a mão do aventureiro aponta para o soquete da arma (a esquerda: o mesmo, espelhado)
            hand_dir = adv_dir("Hand_R", "Socket_Weapon_R")
            if side == "L":
                hand_dir = Vector((-hand_dir.x, hand_dir.y, hand_dir.z))
            align(hand, lambda: joint(tip[1].name), hand_dir)
    thigh, shin, foot = roles["Thigh_" + side], roles["Shin_" + side], roles["Foot_" + side]
    align(thigh, lambda: joint(shin.name), adv_dir("Thigh_" + side, "Shin_" + side))
    if foot:
        align(shin, lambda: joint(foot.name), adv_dir("Shin_" + side, "Foot_" + side))
bpy.context.view_layer.update()
J = {name: joint(b.name) for name, b in roles.items() if b}
J["Hips"] = joint(root.name)
J["Wrist_end_R"] = J.get("Hand_R")
bpy.ops.object.mode_set(mode="OBJECT")
# fixa a forma na malha (o modificador do esqueleto do site aplicado; os grupos de pele continuam)
bpy.ops.object.select_all(action="DESELECT")
bpy.context.view_layer.objects.active = body
body.select_set(True)
mod = next(m for m in body.modifiers if m.type == "ARMATURE")
bpy.ops.object.modifier_apply(modifier=mod.name)
body.parent = None

# ---- 5. o esqueleto do aventureiro no corpo novo
bpy.ops.object.select_all(action="DESELECT")
adv.select_set(True)
bpy.context.view_layer.objects.active = adv
bpy.ops.object.duplicate()
hero = bpy.context.view_layer.objects.active
hero.name = "Armature"
hero.data.name = "Armature"
hero.animation_data_clear()
bpy.ops.object.mode_set(mode="EDIT")
eb = hero.data.edit_bones
for b in eb:
    b.use_connect = False
inv = hero.matrix_world.inverted()
pos = {}
pos["Hips"] = J["Hips"]
pos["Root"] = Vector((J["Hips"].x, J["Hips"].y, low))
pos["Head"] = J["Head"]
mid = (J["UpperArm_L"] + J["UpperArm_R"]) / 2
torso_t = (A["Torso"]["head"].z - A["Hips"]["head"].z) / (A["UpperArm_L"]["head"].z - A["Hips"]["head"].z)
pos["Torso"] = J["Hips"].lerp(Vector((J["Hips"].x, J["Hips"].y, mid.z)), torso_t)
for side in ("L", "R"):
    for part in ("UpperArm", "Forearm", "Hand", "Thigh", "Shin", "Foot"):
        n = part + "_" + side
        if n in J:
            pos[n] = J[n]
# o que não tem junta no modelo novo (soquete da arma, bainha): o mesmo deslocamento do aventureiro a partir do pai
order = [b.name for b in adv.data.bones]          # pais antes dos filhos
for name in order:
    if name not in pos:
        parent = A[name]["parent"]
        pos[name] = pos[parent] + (A[name]["head"] - A[parent]["head"]) if parent in pos else A[name]["head"]
for name in order:
    b = eb[name]
    d = A[name]["tail"] - A[name]["head"]
    b.head = inv @ pos[name]
    b.tail = inv @ (pos[name] + d)
bpy.ops.object.mode_set(mode="OBJECT")
for name in ("Hips", "Torso", "Head", "UpperArm_L", "Forearm_L", "Hand_L", "Thigh_L", "Shin_L", "Foot_L"):
    log("junta %-11s aventureiro %s  novo %s" % (name, tuple(round(v, 3) for v in A[name]["head"]), tuple(round(v, 3) for v in pos[name])))

# pesos: cada grupo do site vira o osso do aventureiro do mesmo papel
weights = {}
groups = {g.index: g.name for g in body.vertex_groups}
for v in body.data.vertices:
    acc = {}
    for g in v.groups:
        target = mapping.get(groups.get(g.group))
        if target and g.weight > 0:
            acc[target] = acc.get(target, 0.0) + g.weight
    total = sum(acc.values())
    if total <= 0:
        continue
    weights[v.index] = {t: w / total for t, w in acc.items()}
body.vertex_groups.clear()
targets = {}
for name in A:
    targets[name] = body.vertex_groups.new(name=name)
for i, acc in weights.items():
    for t, w in acc.items():
        targets[t].add([i], w, "REPLACE")
# o braço é só o braço: o que fica para dentro da junta do ombro (colete, ombreira, tranças da barba, peito) passa para o
# tronco aos poucos (todo a partir de 4 cm para dentro). Com o peso do site, abrir o ombro no jogo esticava o colete e a barba
for side, sign in (("L", 1), ("R", -1)):
    ua = targets["UpperArm_" + side]
    joint_x = sign * pos["UpperArm_" + side].x
    for v in body.data.vertices:
        w = next((g.weight for g in v.groups if g.group == ua.index), 0.0)
        inside = joint_x - sign * (body.matrix_world @ v.co).x
        if w <= 0 or inside <= .005:
            continue
        move = w * min(1.0, inside / .04)
        ua.add([v.index], w - move, "REPLACE")
        targets["Torso"].add([v.index], move, "ADD")
# a barba (--beard, o anão): o site a prendia no peito, e virar a cabeça a deixava parada. Os pedaços da frente, abaixo
# do queixo, com a cor do cabelo (ou das contas douradas das tranças) passam para a cabeça; só a ponta guarda um pouco
# do tronco
if BEARD:
    image = next((n.image for m in body.data.materials if m and m.use_nodes for n in m.node_tree.nodes
                  if n.type == "TEX_IMAGE" and n.image and any(l.to_socket.name == "Base Color" for l in n.outputs[0].links)), None)
    if image:
        W, H = image.size
        px = image.pixels[:]
        uv = body.data.uv_layers.active.data
        color = [None] * len(body.data.vertices)
        for loop in body.data.loops:
            if color[loop.vertex_index] is None:
                u, v = uv[loop.index].uv
                i = ((int((v % 1) * H) % H) * W + int((u % 1) * W) % W) * 4
                color[loop.vertex_index] = px[i:i + 3]
        hairy = lambda c: c[0] > .38 and c[1] < .3 and c[2] < .15 and c[0] > 2 * c[1]
        golden = lambda c: c[0] > .5 and c[1] > .35 and c[2] < .3 and c[1] < .8 * c[0]
        bm = _bm.new()
        bm.from_mesh(body.data)
        bm.verts.ensure_lookup_table()
        world = [body.matrix_world @ v.co for v in body.data.vertices]
        neck, tall = pos["Head"], high - low
        seen, moved = set(), 0
        for start in bm.verts:
            if start.index in seen:
                continue
            piece, stack = [], [start]
            seen.add(start.index)
            while stack:
                a = stack.pop()
                piece.append(a.index)
                for e in a.link_edges:
                    b = e.other_vert(a)
                    if b.index not in seen:
                        seen.add(b.index)
                        stack.append(b)
            zs = [world[i].z for i in piece]
            if max(zs) > neck.z + .13 * tall or min(zs) < neck.z - .2 * tall:
                continue
            if sum(1 for i in piece if hairy(color[i]) or golden(color[i])) < .4 * len(piece):
                continue
            if sum(1 for i in piece if world[i].y < neck.y - .05 * tall) < .5 * len(piece):
                continue
            for i in piece:
                t = min(1.0, max(0.0, (world[i].z - (neck.z - .2 * tall)) / (.12 * tall)))
                for g in body.vertex_groups:
                    g.remove([i])
                targets["Head"].add([i], .6 + .4 * t, "REPLACE")
                if t < 1:
                    targets["Torso"].add([i], .4 * (1 - t), "REPLACE")
                moved += 1
        bm.free()
        log("barba: %d vértices passaram para a cabeça" % moved)
unweighted = len(body.data.vertices) - len(weights)
log("pesos: %d vértices, %d sem peso" % (len(weights), unweighted))
body.parent = hero
mod = body.modifiers.new("Armature", "ARMATURE")
mod.object = hero
bpy.data.objects.remove(rig, do_unlink=True)
body.name = "Hero_" + RACE

# ---- 6. exporta e fotografa
# textura: só a cor, em 2048 (vinha em 8192, pesada demais e no jogo aparecia a cor média enquanto carregava);
# o mapa de normais e o de metal/rugosidade saem (o material do herói é só a cor, fosco)
for mat in body.data.materials:
    if not mat or not mat.use_nodes:
        continue
    for node in list(mat.node_tree.nodes):
        if node.type != "TEX_IMAGE" or not node.image:
            continue
        if any(l.to_socket.name == "Base Color" for l in node.outputs[0].links):
            if max(node.image.size) > 2048:
                node.image.scale(2048, 2048)
        else:
            mat.node_tree.nodes.remove(node)
os.makedirs(OUT_DIR, exist_ok=True)
bpy.ops.object.select_all(action="DESELECT")
hero.select_set(True)
body.select_set(True)
bpy.context.view_layer.objects.active = hero
out = os.path.join(OUT_DIR, "Hero_%s.glb" % RACE)
bpy.ops.export_scene.gltf(filepath=out, use_selection=True, export_format="GLB", export_animations=False, export_skins=True,
                          export_apply=False, export_yup=True)
log("exportado", out)

# primeira pessoa (como no Fortnite): o corpo sem a cabeça (Data/Heroes/<raça>_FP/Hero_<raça>_FP.glb), que só quem joga
# vê. Encolher a cabeça na animação esticava a barba, o cabelo e a gola numa estrela na frente da câmera
bpy.ops.object.select_all(action="DESELECT")
body.select_set(True)
bpy.context.view_layer.objects.active = body
bpy.ops.object.duplicate()
fp = bpy.context.view_layer.objects.active
fp.name = "Hero_%s_FP" % RACE
head_group = fp.vertex_groups["Head"].index if "Head" in fp.vertex_groups else -1
bm = _bm.new()
bm.from_mesh(fp.data)
deform = bm.verts.layers.deform.active
gone = [v for v in bm.verts if v[deform].get(head_group, 0.0) >= .2]
_bm.ops.delete(bm, geom=gone, context="VERTS")
bm.to_mesh(fp.data)
bm.free()
fp_dir = os.path.join(ROOT, "Data", "Heroes", RACE + "_FP")
os.makedirs(fp_dir, exist_ok=True)
bpy.ops.object.select_all(action="DESELECT")
hero.select_set(True)
fp.select_set(True)
bpy.context.view_layer.objects.active = hero
fp_out = os.path.join(fp_dir, "Hero_%s_FP.glb" % RACE)
bpy.ops.export_scene.gltf(filepath=fp_out, use_selection=True, export_format="GLB", export_animations=False, export_skins=True,
                          export_apply=False, export_yup=True)
log("corpo da primeira pessoa (sem a cabeça):", len(fp.data.vertices), "vértices", fp_out)
bpy.data.objects.remove(fp, do_unlink=True)

if PREVIEW:
    os.makedirs(SHOTS, exist_ok=True)
    bpy.data.objects.remove(adv, do_unlink=True)
    sc = bpy.context.scene
    sc.render.resolution_x, sc.render.resolution_y = 360, 480
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    sc.collection.objects.link(cam)
    sc.camera = cam
    sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
    sun.data.energy = 4
    sun.rotation_euler = (.7, .2, .6)
    sc.collection.objects.link(sun)
    world = bpy.data.worlds.new("w")
    world.color = (.4, .42, .45)
    sc.world = world
    c = Vector((0, 0, ADV_H * .5))
    for name, frame in (("Idle", 0), ("Run", 6), ("Swing", 10), ("Death", 30)):
        act = next((a for a in adv_actions if name.lower() in a.name.lower()), None)
        if name == "Swing":
            act = next((a for a in bpy.data.actions if "attack" in a.name.lower() or "swing" in a.name.lower() or "dodge" in a.name.lower()), act)
        if not act:
            continue
        # só as rotações (como na Unreal com a translação vinda da malha); o quadril mantém a altura do corpo novo
        clip = act.copy()
        try:
            curves = clip.fcurves
        except AttributeError:
            curves = [fc for layer in clip.layers for strip in layer.strips for bag in strip.channelbags for fc in bag.fcurves]
        for fc in list(curves):
            if fc.data_path.endswith(".location") and "Root" not in fc.data_path:
                try:
                    curves.remove(fc)
                except Exception:
                    fc.mute = True
        hero.animation_data_create()
        hero.animation_data.action = clip
        try:
            hero.animation_data.action_slot = clip.slots[0]
        except Exception:
            pass
        for view, (dx, dy) in enumerate(((0.0, -1.0), (-.8, -.6))):
            sc.frame_set(frame)
            cam.location = c + Vector((dx, dy, .15)) * ADV_H * 2.2
            cam.rotation_euler = (c - cam.location).to_track_quat("-Z", "Y").to_euler()
            sc.render.filepath = os.path.join(SHOTS, "hero_%s_%s_%d.png" % (RACE, name.lower(), view))
            bpy.ops.render.render(write_still=True)
    log("fotos em", SHOTS)

os.makedirs(os.path.join(ROOT, "Saved", "Logs"), exist_ok=True)
with open(os.path.join(ROOT, "Saved", "Logs", "RigHero_%s.txt" % RACE), "w", encoding="utf-8") as f:
    f.write("\n".join(report) + "\n")
