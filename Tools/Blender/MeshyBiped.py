"""Um modelo novo do Meshy (com o esqueleto automático do site, Bone_000...) animado com os clipes de um bicho antigo
de mesmo corpo (duas pernas, dois braços) — sem janela.

blender -b --factory-startup -P Tools/Blender/MeshyBiped.py -- <modelo do Meshy.glb|.fbx> <clipes antigos.glb> <saída.glb> <altura m> <bicho> [corrida.fbx]

- acha no esqueleto do site, pela posição, o quadril, a coluna, o pescoço, a cabeça, os braços e as pernas;
- põe o modelo na altura do jogo (o Azhûr tem 7 m) e de frente para o mesmo lado do antigo;
- cada osso copia, quadro a quadro, a direção do osso do antigo que faz o mesmo papel (como no MixamoToHero.py: os
  dois são do mesmo corpo, só a pose de descanso muda — o novo vem de braços abertos);
- o quadril sobe e desce como o do antigo (na proporção das alturas);
- [corrida.fbx] (02/11, o goblin): o modelo veio do site já com esqueleto de gente e uma corrida animada no lugar; ela
  entra no lugar do Run copiado do antigo. O goblin ganha também um porrete de blocos na mão (o antigo tinha um);
- sai um glb com a malha, o esqueleto e os clipes com os nomes do antigo (o jogo pede por eles); a textura de 8K
  vira 4K.
"""
import os
import sys

import bpy
from mathutils import Matrix, Quaternion, Vector

args = sys.argv[sys.argv.index("--") + 1:]
MESHY, SOURCE, OUT, HEIGHT, CREATURE = args[0], args[1], args[2], float(args[3]), args[4]
# 10/11: "<modelo>+<fonte>" separa o esqueleto do modelo (MANUAL) da fonte dos clipes (ROLES): goblin+mixamo é o goblin
# com as animações do Mixamo dos heróis (Tools/Blender/MixamoMonsterSource.py)
CREATURE, SRC_KIND = CREATURE.split("+") if "+" in CREATURE else (CREATURE, CREATURE)
RUN = args[5] if len(args) > 5 else None

# os ossos do bicho antigo por papel (os dois lados: L é o do -X nos dois)
ROLES = {
    "azhur": {"hips": "Hips", "spine": "Spine", "chest": "Chest", "neck": "Neck", "head": "Head",
              "upperarm": "UpperArm_%s", "forearm": "Forearm_%s", "hand": "Hand_%s",
              "thigh": "Thigh_%s", "shin": "Shin_%s", "foot": "Foot_%s"},
    "golem": {"hips": "Hips", "chest": "Torso", "head": "Head", "upperarm": "Arm_%s", "forearm": "Forearm_%s",
              "thigh": "Leg_%s"},
    "titan": {"hips": "Hips", "spine": "Torso", "chest": "Chest", "head": "Head", "upperarm": "Arm_%s", "forearm": "Forearm_%s",
              "thigh": "Leg_%s"},
    "goblin": {"hips": "Body", "head": "Head", "upperarm": "Arm_%s", "thigh": "Leg_%s"},
    # 10/11: o Mixamo dos heróis (o lado vira Left/Right: SIDE_NAME)
    "mixamo": {"hips": "mixamorig:Hips", "spine": "mixamorig:Spine", "chest": "mixamorig:Spine2", "neck": "mixamorig:Neck",
               "head": "mixamorig:Head", "upperarm": "mixamorig:%sArm", "forearm": "mixamorig:%sForeArm", "hand": "mixamorig:%sHand",
               "thigh": "mixamorig:%sUpLeg", "shin": "mixamorig:%sLeg", "foot": "mixamorig:%sFoot"},
}[SRC_KIND]
SIDE_NAME = {"L": "Left", "R": "Right"} if SRC_KIND == "mixamo" else {"L": "L", "R": "R"}
# o goblin do Godot: braço e perna são um osso só, que aponta para cima no descanso (a malha pendura para baixo): a
# direção deles é a contrária da ponta
LEAF_DOWN = SRC_KIND == "goblin"
# o quanto o pulso do novo copia o giro do antigo (02/11): no Azhûr a mão do site gira em volta de outro eixo e, com o
# giro inteiro, as duas mãos entortavam e puxavam para o meio, para baixo. Com 35% a mão acompanha o antebraço
HAND_KEEP = {"azhur": .35}.get(CREATURE, 1.0)
# os antigos de perna inteiriça (golem, titã) não dobram o joelho: o novo dobra quando a coxa vem para a frente (o pé
# sobe no passo) e estica no apoio
KNEE = "shin" not in ROLES
# esqueletos que a busca automática não entende: os papéis à mão. Thorun é uma árvore: tronco com raízes no chão, sem
# pernas (anda arrastando as raízes); os braços saem de um osso que desce atrás do peito
MANUAL = {
    "titan": {"hips": "Bone_000", "spine": "Bone_010", "chest": "Bone_006", "head": "Bone_017",
              "upperarmL": "Bone_021", "forearmL": "Bone_020", "handL": "Bone_019",
              "upperarmR": "Bone_026", "forearmR": "Bone_025", "handR": "Bone_024", "front": -1, "own_arms": True},
    # o goblin novo (02/11) tem o esqueleto de gente do site (o do manequim): L aqui é o lado -X, o _r dele
    "goblin": {"hips": "pelvis", "spine": "spine_01", "chest": "spine_03", "neck": "neck_01", "head": "head",
               "upperarmL": "upperarm_r", "forearmL": "lowerarm_r", "handL": "hand_r",
               "upperarmR": "upperarm_l", "forearmR": "lowerarm_l", "handR": "hand_l",
               "thighL": "thigh_r", "shinL": "calf_r", "footL": "foot_r",
               "thighR": "thigh_l", "shinR": "calf_l", "footR": "foot_l", "front": -1},
    # 10/11: o Cogumelo Corrompido (Docs/Modelos_Meshy): a busca pela posição tomava a clavícula pela cabeça
    "mushroom": {"hips": "Bone_000", "spine": "Bone_004", "chest": "Bone_002", "neck": "Bone_017", "head": "Bone_016",
                 "upperarmL": "Bone_021", "forearmL": "Bone_020", "handL": "Bone_019",
                 "upperarmR": "Bone_026", "forearmR": "Bone_025", "handR": "Bone_024",
                 "thighL": "Bone_009", "shinL": "Bone_008", "footL": "Bone_007",
                 "thighR": "Bone_014", "shinR": "Bone_013", "footR": "Bone_012", "front": -1},
}.get(CREATURE)


def log(*parts):
    print("MESHY_BIPED", " ".join(str(p) for p in parts))


bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.render.fps = 30

# ---- o antigo, com os clipes
bpy.ops.import_scene.gltf(filepath=SOURCE)
src = next(o for o in bpy.data.objects if o.type == "ARMATURE")
src.name = "Source"
# o glTF traz cada clipe numa faixa ligada: o Death (deitado) somava por cima do clipe copiado e deitava o bicho
if src.animation_data:
    for t in src.animation_data.nla_tracks:
        t.mute = True
src_actions = {a.name: a for a in bpy.data.actions}
for o in list(bpy.data.objects):
    if o.type == "MESH":
        bpy.data.objects.remove(o, do_unlink=True)

# ---- o novo
before = set(bpy.data.objects)
if MESHY.lower().endswith(".fbx"):
    bpy.ops.import_scene.fbx(filepath=MESHY)
else:
    bpy.ops.import_scene.gltf(filepath=MESHY)
new = [o for o in bpy.data.objects if o not in before]
rig = next(o for o in new if o.type == "ARMATURE")
body = max((o for o in new if o.type == "MESH"), key=lambda o: len(o.data.vertices))
for o in new:
    if o.type == "MESH" and o != body:
        bpy.data.objects.remove(o, do_unlink=True)
rig.name = "Armature"
# o FBX do site vem com uma animação que guarda a escala do objeto: sem tirar, o primeiro quadro desfazia a altura
if rig.animation_data:
    rig.animation_data_clear()
bpy.context.view_layer.update()


def bone_curves_only(action):
    """Tira do clipe as curvas do objeto (posição, giro e escala do esqueleto inteiro): só os ossos passam."""
    bags = []
    try:
        for layer in action.layers:
            for strip in layer.strips:
                for slot in action.slots:
                    bag = strip.channelbag(slot)
                    if bag:
                        bags.append(bag.fcurves)
    except AttributeError:
        bags.append(action.fcurves)
    for curves in bags:
        for fc in [c for c in curves if not c.data_path.startswith("pose.bones")]:
            curves.remove(fc)


def bounds(objs):
    pts = [o.matrix_world @ Vector(c) for o in objs for c in o.bound_box]
    return Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts))), \
        Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))


def src_height():
    lo, hi = None, None
    for b in src.data.bones:
        for p in (src.matrix_world @ b.head_local, src.matrix_world @ b.tail_local):
            lo = p.z if lo is None else min(lo, p.z)
            hi = p.z if hi is None else max(hi, p.z)
    return lo, hi


# altura do jogo e os pés no chão
lo, hi = bounds([body])
k = HEIGHT / (hi.z - lo.z)
rig.scale = rig.scale * k
rig.location = rig.location * k
bpy.context.view_layer.update()
lo, hi = bounds([body])
rig.location.z -= lo.z
bpy.context.view_layer.update()
# o esqueleto do FBX vem girado 90° (e agora com escala): no glb a Unreal montava a pose de descanso amassada e
# deitada. Giro, escala e posição vão para dentro dos ossos e da malha (os dois ficam sem transformação)
if rig.rotation_mode != "QUATERNION" and any(abs(a) > 1e-4 for a in rig.rotation_euler) or abs(rig.scale.x - 1) > 1e-4:
    bpy.ops.object.select_all(action="DESELECT")
    body.select_set(True)
    bpy.context.view_layer.objects.active = body
    bpy.ops.object.parent_clear(type="CLEAR_KEEP_TRANSFORM")
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.select_all(action="DESELECT")
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    body.parent = rig
    body.matrix_parent_inverse = rig.matrix_world.inverted()
    bpy.context.view_layer.update()
    log("transformação aplicada", tuple(round(x, 3) for x in rig.dimensions))


def head(b):
    return rig.matrix_world @ b.head_local


def tail(b):
    return rig.matrix_world @ b.tail_local


def descendants(b):
    return 1 + sum(descendants(c) for c in b.children)


def chain(b):
    out = [b]
    while out[-1].children:
        out.append(max(out[-1].children, key=descendants))
    return out


# ---- os papéis no esqueleto do site
root = next(b for b in rig.data.bones if not b.parent)
spine = [root]
while True:
    up = [c for c in spine[-1].children if abs(head(c).x) < .06 * HEIGHT and max(tail(d).z for d in [c] + list(c.children_recursive)) > head(spine[-1]).z]
    if not up:
        break
    spine.append(max(up, key=lambda c: descendants(c) if head(c).z > head(spine[-1]).z - .02 * HEIGHT else 0))
    if len(spine) > 12:
        break
legs, arms = {}, {}
for s in spine:
    for c in s.children:
        if c in spine:
            continue
        ch = chain(c)
        side = "L" if sum(head(b).x for b in ch) < 0 else "R"
        # o que sai acima do quadril é braço (o golem tem os braços compridos, abaixo do quadril); abaixo, perna
        if head(c).z > head(root).z + .1 * HEIGHT:
            if max(abs(tail(b).x) for b in ch) > .2 * HEIGHT and (side not in arms or len(ch) > len(arms[side])):
                arms[side] = ch
        elif min(tail(b).z for b in ch) < head(root).z - .15 * HEIGHT:
            legs[side] = ch
# o primeiro osso deitado (sai do peito para o lado) é o ombro, não o braço: alinhado ao braço do antigo (que desce),
# ele girava o bloco do ombro inteiro para baixo e o braço saía da cintura (o golem). Ele fica com o peito
for s, a in arms.items():
    d = tail(a[0]) - head(a[0])
    if len(a) > 3 and abs(d.z) < .5 * d.length:
        arms[s] = a[1:]
shoulder = min(head(arms[s][0]).z for s in arms) if arms else head(spine[-1]).z
upper = [b for b in spine if head(b).z > shoulder - .02 * HEIGHT]
role = {"hips": root.name}
mid = [b for b in spine[1:] if head(b).z <= shoulder - .02 * HEIGHT]
if mid:
    role["spine"] = mid[0].name
    role["chest"] = mid[-1].name if len(mid) > 1 else mid[0].name
if upper:
    role["neck"] = upper[0].name
    role["head"] = upper[1].name if len(upper) > 1 else upper[0].name
for s in ("L", "R"):
    if s in arms:
        a = arms[s]
        role["upperarm" + s], role["forearm" + s], role["hand" + s] = a[0].name, a[1].name, a[2].name if len(a) > 2 else a[1].name
    if s in legs:
        g = legs[s]
        role["thigh" + s], role["shin" + s], role["foot" + s] = g[0].name, g[1].name, g[2].name if len(g) > 2 else g[1].name
if MANUAL:
    role = {k: v for k, v in MANUAL.items() if k not in ("front", "own_arms")}
# braços na pose do próprio modelo: só o movimento do antigo (a partir do Idle dele) passa. O Thorun tem o tronco largo:
# com a direção do antigo (braço reto para baixo) os braços afundavam dentro do tronco e sumiam
OWN_ARMS = bool(MANUAL and MANUAL.get("own_arms"))
LEGS = "thighL" in role
KNEE = KNEE and LEGS
log("papéis", role)

# ---- de frente para o mesmo lado: o antigo olha para onde aponta o pé (a ponta do pé)
def facing(obj, foot):
    b = obj.data.bones[foot]
    d = (obj.matrix_world @ b.tail_local) - (obj.matrix_world @ b.head_local)
    return Vector((0, 1 if d.y > 0 else -1, 0))


# a frente do antigo pelo núcleo do peito (fica na frente do corpo); a ponta do osso do pé não serve: o importador
# do glTF gira os ossos e ela pode apontar para trás
core = next((b for b in src.data.bones if b.name == "Core"), None)
src_front = Vector((0, 1 if core and (src.matrix_world @ core.head_local).y > (src.matrix_world @ src.data.bones[ROLES["hips"]].head_local).y else -1, 0))
if SRC_KIND == "mixamo":
    # o Mixamo: a frente é para onde a ponta do pé aponta (do tornozelo até os dedos)
    _f = src.matrix_world @ src.data.bones["mixamorig:LeftFoot"].head_local
    _t = src.matrix_world @ src.data.bones["mixamorig:LeftToeBase"].head_local
    src_front = Vector((0, 1 if _t.y > _f.y else -1, 0))
if MANUAL and "front" in MANUAL:
    our_front = Vector((0, MANUAL["front"], 0))
else:
    our_foot = rig.data.bones[role["footL"]]
    d = chain(our_foot)[-1]
    our_front = Vector((0, 1 if (tail(d) - head(our_foot)).y > 0 else -1, 0))
if src_front.y != our_front.y:
    # o importador do glTF deixa o objeto em quatérnio: girar pela matriz (rotation_euler seria ignorado)
    src.matrix_world = Matrix.Rotation(3.14159265, 4, "Z") @ src.matrix_world
    bpy.context.view_layer.update()
log("frente antigo", src_front.y, "novo", our_front.y)
slo, shi = src_height()
ks = HEIGHT / max(.01, shi - slo)
src.scale = src.scale * ks
bpy.context.view_layer.update()

# ---- pares (nosso osso, osso antigo, filho antigo para a direção, nosso filho)
PAIRS = []
probe = ROLES.get("upperarm") or ROLES.get("thigh")
SWAP = {"L": "R", "R": "L"} if probe and (src.matrix_world @ src.data.bones[probe % SIDE_NAME["L"]].head_local).x > 0 else {}
log("lados do antigo trocados" if SWAP else "lados iguais")
ORDER = [("hips", None), ("spine", "chest"), ("chest", "neck"), ("neck", "head"), ("head", None)]
for r, child in ORDER:
    if r in ROLES and r in role:
        PAIRS.append((role[r], ROLES[r], ROLES.get(child) if child else None, role.get(child) if child else None))
for s in ("L", "R"):
    for r, child in (("upperarm", "forearm"), ("forearm", "hand"), ("hand", None), ("thigh", "shin"), ("shin", "foot"), ("foot", None)):
        if r in ROLES and r + s in role:
            # o lado do antigo pela posição (no golem do Godot a esquerda fica em +X; no Meshy, em -X)
            o_side = SWAP.get(s, s)
            PAIRS.append((role[r + s], ROLES[r] % SIDE_NAME[o_side], (ROLES[child] % SIDE_NAME[o_side]) if child and child in ROLES else None, role.get(child + s) if child else None))
log("pares", [(a, b) for a, b, _, _ in PAIRS])


def rest(obj, name):
    return obj.matrix_world @ obj.data.bones[name].matrix_local


def direction(obj, name, child, posed):
    m = (obj.matrix_world @ obj.pose.bones[name].matrix) if posed else rest(obj, name)
    if LEAF_DOWN and obj == src and not child:
        pb = obj.pose.bones[name]
        return -((obj.matrix_world @ pb.tail) - (obj.matrix_world @ pb.head)).normalized()
    if child:
        c = (obj.matrix_world @ obj.pose.bones[child].matrix) if posed else rest(obj, child)
        return (c.translation - m.translation).normalized()
    # osso da ponta (sem filho): a ponta que o importador dá aponta para qualquer lado (no golem, para cima); segue a
    # direção do osso de cima até ele (o braço reto)
    parent = obj.data.bones[name].parent
    if parent is not None:
        pm = (obj.matrix_world @ obj.pose.bones[parent.name].matrix) if posed else rest(obj, parent.name)
        d = m.translation - pm.translation
        if d.length > 1e-4:
            return d.normalized()
    t = (obj.matrix_world @ (obj.pose.bones[name].tail if posed else obj.data.bones[name].tail_local))
    return (t - m.translation).normalized()


# a referência do antigo é o primeiro quadro do Idle (de pé de verdade): nos do Godot o descanso guardado no arquivo
# tem o quadril girado 90° em relação aos clipes (deitava o golem e o Thorun); no Azhûr de blocos dá no mesmo
idle = src_actions.get("Idle")
if idle:
    src.animation_data.action = idle
    try:
        src.animation_data.action_slot = idle.slots[0]
    except Exception:
        pass
    scene.frame_set(int(idle.frame_range[0]))
    bpy.context.view_layer.update()
info = {}
for o, sname, schild, ochild in PAIRS:
    Rs = (src.matrix_world @ src.pose.bones[sname].matrix).to_quaternion()
    Ro = rest(rig, o).to_quaternion()
    # tronco e cabeça: os dois estão de pé no descanso, só o giro passa (a ponta desses ossos o importador aponta para
    # qualquer lado: alinhar por ela deitava o golem e o Thorun). Braços e pernas: alinhados pela direção do osso
    trunk = o in {role.get(k) for k in ("hips", "spine", "chest", "neck", "head")}
    trunk = trunk or (OWN_ARMS and o in {role.get(k + s) for k in ("upperarm", "forearm", "hand") for s in "LR"})
    Q = Quaternion() if trunk else direction(src, sname, schild, True).rotation_difference(direction(rig, o, ochild, False))
    info[o] = (Rs, Ro, Q)
hs0 = (src.matrix_world @ src.pose.bones[ROLES["hips"]].matrix).translation.copy()
ho0 = rest(rig, role["hips"]).translation
kh = ho0.z / max(.01, hs0.z)
for pb in rig.pose.bones:
    pb.rotation_mode = "QUATERNION"
rig.animation_data_create()
order = [b.name for b in rig.data.bones]
by = {o: (s, sc) for o, s, sc, _ in PAIRS}
for name, act in sorted(src_actions.items()):
    src.animation_data.action = act
    try:
        src.animation_data.action_slot = act.slots[0]
    except Exception:
        pass
    first, last = (int(round(f)) for f in act.frame_range)
    # os clipes do Godot não têm chave em todo osso (o Hit não mexe no quadril): sem voltar ao descanso, o osso ficava
    # na pose do clipe anterior (o Death, deitado) e deitava todos os seguintes
    for pb in src.pose.bones:
        pb.matrix_basis = Matrix.Identity(4)
    out = bpy.data.actions.new(name)
    rig.animation_data.action = out
    swing_before = {}
    for i, f in enumerate(range(first, last + 1)):
        scene.frame_set(f)
        for pb in rig.pose.bones:
            pb.matrix_basis = Matrix.Identity(4)
        bpy.context.view_layer.update()
        for o in order:
            if o not in by:
                continue
            s, sc = by[o]
            Rs, Ro, Q = info[o]
            Ws = (src.matrix_world @ src.pose.bones[s].matrix).to_quaternion()
            Wo = Ws @ Rs.inverted() @ Q.inverted() @ Ro
            pb = rig.pose.bones[o]
            pos = (rig.matrix_world @ pb.matrix).translation
            if o == role["hips"]:
                hp = (src.matrix_world @ src.pose.bones[s].matrix).translation
                pos = ho0 + (hp - hs0) * kh
            pb.matrix = rig.matrix_world.inverted() @ (Matrix.Translation(pos) @ Wo.to_matrix().to_4x4())
            if HAND_KEEP < 1.0 and o in (role.get("handL"), role.get("handR")):
                pb.rotation_quaternion = Quaternion().slerp(pb.rotation_quaternion, HAND_KEEP)
            bpy.context.view_layer.update()
        if KNEE:
            for side in ("L", "R"):
                thigh, shin = rig.pose.bones[role["thigh" + side]], rig.pose.bones[role["shin" + side]]
                d = ((rig.matrix_world @ thigh.tail) - (rig.matrix_world @ thigh.head)).normalized()
                swing = -d.y * our_front.y * -1          # quanto a coxa vai para a frente
                speed = (swing - swing_before.get(side, swing)) * scene.render.fps
                swing_before[side] = swing
                bend = max(0.0, min(.8, speed * .35 + max(0.0, swing) * .6))
                w = rig.matrix_world @ shin.matrix
                pivot = Matrix.Translation(w.translation)
                axis = Vector((1, 0, 0)) * (1 if our_front.y < 0 else -1)
                shin.matrix = rig.matrix_world.inverted() @ (pivot @ Quaternion(axis, bend).to_matrix().to_4x4() @ pivot.inverted() @ w)
                bpy.context.view_layer.update()
        for pb in rig.pose.bones:
            if pb.name in by or (KNEE and pb.name in (role["shinL"], role["shinR"])):
                pb.keyframe_insert("rotation_quaternion", frame=i)
            if pb.name == role["hips"]:
                pb.keyframe_insert("location", frame=i)
    track = rig.animation_data.nla_tracks.new()
    track.name = name
    strip = track.strips.new(name, 0, out)
    try:
        if out.slots and not strip.action_slot:
            strip.action_slot = out.slots[0]
    except Exception:
        pass
    rig.animation_data.action = None
    log("clipe", name, last - first + 1, "quadros")

# a corrida do próprio modelo (no lugar, mesmo esqueleto) no lugar do Run copiado
if RUN:
    before = set(bpy.data.actions)
    objs = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=RUN)
    run = next(a for a in bpy.data.actions if a not in before)
    bone_curves_only(run)
    for o in set(bpy.data.objects) - objs:
        bpy.data.objects.remove(o, do_unlink=True)
    for t in list(rig.animation_data.nla_tracks):
        if t.name == "Run":
            old = t.strips[0].action if t.strips else None
            rig.animation_data.nla_tracks.remove(t)
            if old:
                bpy.data.actions.remove(old)
    run.name = "Run"
    track = rig.animation_data.nla_tracks.new()
    track.name = "Run"
    strip = track.strips.new("Run", int(run.frame_range[0]), run)
    try:
        if run.slots and not strip.action_slot:
            strip.action_slot = run.slots[0]
    except Exception:
        pass
    log("corrida do modelo", run.frame_range[:])


def texture_uv(want):
    """O UV de um pedaço da textura do próprio modelo com a cor pedida (o porrete do goblin): procura numa cópia
    pequena um ponto cujos vizinhos também têm essa cor."""
    import numpy as np
    img = next(n.image for n in body.data.materials[0].node_tree.nodes if n.type == "TEX_IMAGE" and n.image)
    small = img.copy()
    small.scale(128, 128)
    px = np.array(small.pixels[:], dtype=np.float32).reshape(128, 128, 4)[..., :3]
    bpy.data.images.remove(small)
    ok = np.zeros((128, 128), bool)
    for y in range(1, 127):
        for x in range(1, 127):
            ok[y, x] = all(want(px[y + dy, x + dx]) for dy in (-1, 0, 1) for dx in (-1, 0, 1))
    ys, xs = np.nonzero(ok)
    if not len(ys):
        return (.5, .5)
    i = len(ys) // 2
    return ((xs[i] + .5) / 128, (ys[i] + .5) / 128)


if CREATURE == "goblin":
    # o porrete (o goblin antigo tinha um): blocos presos na mão direita (a do -X), o cabo no punho e a cabeça para a
    # frente quando o braço pende; madeira e tachas com cores da própria textura
    import bmesh
    hb = rig.data.bones[role["handL"]]
    H = rig.matrix_world @ hb.head_local
    knuckles = [rig.matrix_world @ c.head_local for c in hb.children]
    K = sum(knuckles, Vector()) / len(knuckles) if knuckles else rig.matrix_world @ hb.tail_local
    fist = H.lerp(K, .7)
    d = (K - H).normalized()
    front = Vector((0, our_front.y, 0))
    a = (front - d * front.dot(d)).normalized()
    b = d.cross(a).normalized()
    c = a.cross(b).normalized()
    # madeira: marrom claro (vermelho > verde > azul); as cintas do porrete: o couro escuro
    wood = texture_uv(lambda c: c[0] > c[1] * 1.12 > c[2] * 1.25 and .3 < c[0] < .65)
    stud = texture_uv(lambda c: c[0] > c[1] * 1.05 >= c[2] and max(c) < .2)
    log("cores do porrete", wood, stud)
    u = HEIGHT / 1.03
    BOXES = [(-.05, .2, .022, wood), (.2, .42, .055, wood), (.26, .30, .065, stud), (.34, .38, .065, stud)]
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    for t0, t1, r, col in BOXES:
        pts = [fist + a * ((t1 if i & 1 else t0) * u) + b * ((r if i & 2 else -r) * u) + c * ((r if i & 4 else -r) * u) for i in range(8)]
        vs = [bm.verts.new(q) for q in pts]
        for q in ((0, 2, 6, 4), (1, 5, 7, 3), (0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6)):
            f = bm.faces.new([vs[i] for i in q])
            for loop in f.loops:
                loop[uvl].uv = col
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new("Club")
    bm.to_mesh(me)
    bm.free()
    club = bpy.data.objects.new("Club", me)
    scene.collection.objects.link(club)
    me.materials.append(body.data.materials[0])
    # nas coordenadas da malha do corpo, presa ao osso da mão
    me.transform(body.matrix_world.inverted())
    club.matrix_world = body.matrix_world.copy()
    g = club.vertex_groups.new(name=role["handL"])
    g.add(list(range(len(me.vertices))), 1.0, "REPLACE")
    bpy.ops.object.select_all(action="DESELECT")
    club.select_set(True)
    body.select_set(True)
    bpy.context.view_layer.objects.active = body
    bpy.ops.object.join()
    log("porrete na mão", role["handL"])

# textura: a de 8K vira 4K
for mat in body.data.materials:
    if mat and mat.use_nodes:
        for n in mat.node_tree.nodes:
            if n.type == "TEX_IMAGE" and n.image and max(n.image.size) > 4096:
                n.image.scale(4096, 4096)
bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True)
body.select_set(True)
bpy.context.view_layer.objects.active = rig
os.makedirs(os.path.dirname(OUT), exist_ok=True)
bpy.ops.export_scene.gltf(filepath=OUT, use_selection=True, export_format="GLB", export_animations=True,
                          export_animation_mode="NLA_TRACKS", export_skins=True, export_apply=False, export_yup=True)
log("exportado", OUT)
