"""Monta o lobo ametista (Meshy) para a Unreal: tamanho do lobo antigo, dez animações e o FBX.

Roda dentro do Blender (pelo MCP, com exec deste arquivo e VORTEX_ROOT definido, ou
blender -b --python Tools/BuildAmethystWolf.py). Lê Data/Modelos/Lobo/Lobo_Andar.fbx (malha,
esqueleto de 27 ossos e o andar do Meshy) e grava Lobo_Ametista.fbx e Lobo_Ametista.blend.

As animações novas partem da pose média do andar (a pose de repouso do Meshy não é a de pé) e
somam rotações dadas no espaço do mundo (X lateral, -Y frente, Z cima), convertidas para o espaço
de cada osso. Correr e andar de lado reaproveitam o ciclo de patas do andar original: correr é o
mesmo trote mais rápido e mais amplo; de lado, o balanço das coxas é girado 90 graus.
"""
import math
import os

import bpy
from mathutils import Quaternion, Vector

ROOT = globals().get("VORTEX_ROOT") or os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "Data", "Modelos", "Lobo")
LENGTH_M = 2.6  # do focinho à ponta do rabo, como o lobo antigo (corpo de 272 cm na Unreal)

TAIL = ["tail", "tailstart", "tail1", "tail2", "tail3"]
SIDES = ["", "R_"]

# ---------------------------------------------------------------- cena limpa e importação
for collection in (bpy.data.objects, bpy.data.meshes, bpy.data.armatures, bpy.data.actions, bpy.data.materials):
    for block in list(collection):
        collection.remove(block)
bpy.context.scene.render.fps = 30
bpy.context.scene.frame_set(1)  # a caixa da malha segue a pose do quadro: medida sempre no mesmo
bpy.ops.import_scene.fbx(filepath=os.path.join(SRC, "Lobo_Andar.fbx"))
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
mesh = next(o for o in bpy.context.scene.objects if o.type == 'MESH')
arm.name, mesh.name = "Armature", "Lobo_Ametista"
source_walk = arm.animation_data.action


def fcurves(action):
    return [fc for layer in action.layers for strip in layer.strips for bag in strip.channelbags for fc in bag.fcurves]


# o andar também anima o próprio objeto (rotação 90 e escala 0,01): isso desfaria o "aplicar"
for layer in source_walk.layers:
    for strip in layer.strips:
        for bag in strip.channelbags:
            for fc in [f for f in bag.fcurves if not f.data_path.startswith("pose.")]:
                bag.fcurves.remove(fc)


def mesh_bounds(evaluated=False):
    bpy.context.view_layer.update()
    if evaluated:
        depsgraph = bpy.context.evaluated_depsgraph_get()
        obj = mesh.evaluated_get(depsgraph)
        data = obj.to_mesh()
        pts = [obj.matrix_world @ v.co for v in data.vertices]
        obj.to_mesh_clear()
    else:
        pts = [mesh.matrix_world @ Vector(c) for c in mesh.bound_box]
    return (Vector([min(p[i] for p in pts) for i in range(3)]), Vector([max(p[i] for p in pts) for i in range(3)]))


# ---------------------------------------------------------------- tamanho e eixos aplicados
lo, hi = mesh_bounds()
scale = LENGTH_M / (hi.y - lo.y)
arm.scale = arm.scale * scale
total_scale = arm.scale.x
bpy.ops.object.select_all(action='DESELECT')
arm.select_set(True)
mesh.select_set(True)
bpy.context.view_layer.objects.active = arm
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
for fc in fcurves(source_walk):
    if fc.data_path.endswith(".location"):
        for key in fc.keyframe_points:
            key.co[1] *= total_scale
            key.handle_left[1] *= total_scale
            key.handle_right[1] *= total_scale

W0, W1 = (int(round(v)) for v in source_walk.frame_range)
PERIOD = W1 - W0
pose_bones = list(arm.pose.bones)


def sample(action, frame):
    """Pose local de cada osso num quadro da ação: {nome: (Quaternion, Vector)}."""
    by = {(fc.data_path, fc.array_index): fc for fc in fcurves(action)}
    out = {}
    for pb in pose_bones:
        q = [by.get((f'pose.bones["{pb.name}"].rotation_quaternion', i)) for i in range(4)]
        loc = [by.get((f'pose.bones["{pb.name}"].location', i)) for i in range(3)]
        out[pb.name] = (Quaternion([c.evaluate(frame) if c else float(i == 0) for i, c in enumerate(q)]).normalized(),
                        Vector([c.evaluate(frame) if c else 0.0 for c in loc]))
    return out


# pose base: média do ciclo do andar (de pé, patas embaixo do corpo)
cycle = [sample(source_walk, W0 + f) for f in range(PERIOD)]
base = {}
for pb in pose_bones:
    ref = cycle[0][pb.name][0]
    acc = [0.0] * 4
    loc = Vector()
    for frame in cycle:
        q, l = frame[pb.name]
        sign = 1.0 if ref.dot(q) >= 0 else -1.0
        for i in range(4):
            acc[i] += q[i] * sign
        loc += l
    base[pb.name] = (Quaternion(acc).normalized(), loc / len(cycle))


def apply_pose(pose):
    for pb in pose_bones:
        q, l = pose[pb.name]
        pb.rotation_mode = 'QUATERNION'
        pb.rotation_quaternion = q
        pb.location = l
        pb.scale = (1, 1, 1)
    bpy.context.view_layer.update()


# chão e centro: patas no z=0 e o corpo centrado na origem (a Unreal põe a origem no pé da cápsula)
arm.animation_data.action = None
apply_pose(base)
lo, hi = mesh_bounds(evaluated=True)
hips_y = (arm.data.bones["Hips"].head_local.y + arm.data.bones["chest"].head_local.y) / 2
arm.location = (0.0, -hips_y, -lo.z)
bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)
apply_pose(base)

# rotação de cada osso na pose base, no espaço da armadura (= mundo, armadura sem transformação)
POSED = {pb.name: pb.matrix.to_quaternion() for pb in pose_bones}
HIPS_REST_INV = arm.data.bones["Hips"].matrix_local.to_3x3().inverted()


def rot(pitch=0.0, yaw=0.0, roll=0.0):
    """Rotação no mundo, em graus. pitch + abaixa o focinho / joga a pata para trás / ergue o rabo;
    yaw + vira para a esquerda do lobo; roll + tomba o topo para a esquerda do lobo."""
    return (Quaternion((0, 0, 1), math.radians(yaw)) @ Quaternion((0, -1, 0), math.radians(roll))
            @ Quaternion((1, 0, 0), math.radians(pitch)))


def local(name, world):
    r = POSED[name]
    return r.inverted() @ world @ r


def scaled(q, k):
    """A mesma rotação com o ângulo multiplicado por k."""
    if q.w < 0:
        q = -q
    angle = q.angle
    if angle < 1e-6:
        return Quaternion()
    return Quaternion(q.axis, angle * k)


def compose(src, deltas=None, offset=None):
    out = {}
    for name, (q, l) in src.items():
        if deltas and name in deltas:
            q = q @ local(name, deltas[name])
        out[name] = (q, l.copy())
    if offset is not None and offset.length > 0:
        q, l = out["Hips"]
        out["Hips"] = (q, l + HIPS_REST_INV @ offset)
    return out


def walk_at(t):
    """Pose do andar original no ciclo t (0..1)."""
    return sample(source_walk, W0 + PERIOD * (t % 1.0))


def sm(x):
    x = min(1.0, max(0.0, x))
    return x * x * (3 - 2 * x)


def ramp(f, a, b, c, d):
    """Sobe de a até b, segura, desce de c até d."""
    return sm((f - a) / max(1e-6, b - a)) * (1.0 - sm((f - c) / max(1e-6, d - c)))


def add(deltas, name, q):
    deltas[name] = q @ deltas[name] if name in deltas else q


ACTIONS = []


def bake(name, frames, fn, ground=lambda f: 1.0):
    """ground(f): 0..1, quanto o ponto mais baixo do corpo é preso no chão naquele quadro
    (nem pata afundando, nem flutuando); 0 deixa solto (o ar do bote)."""
    action = bpy.data.actions.new(name)
    action.use_fake_user = True
    arm.animation_data_create()
    arm.animation_data.action = None
    last = {}
    for f in range(frames + 1):
        pose = fn(f)
        arm.animation_data.action = None  # sem a ação, a medida usa a pose deste quadro
        apply_pose(pose)
        lowest = mesh_bounds(evaluated=True)[0].z
        lift = -lowest if lowest < 0 else -lowest * ground(f)  # abaixo do chão, nunca
        q, l = pose["Hips"]
        pose["Hips"] = (q, l + HIPS_REST_INV @ Vector((0, 0, lift)))
        arm.animation_data.action = action
        for pb in pose_bones:
            q, l = pose[pb.name]
            # mesmo hemisfério do quadro anterior: sem giro de 360 na interpolação
            if pb.name in last and last[pb.name].dot(q) < 0:
                q = -q
            last[pb.name] = q
            pb.rotation_quaternion = q
            pb.location = l
            pb.scale = (1, 1, 1)
            pb.keyframe_insert("rotation_quaternion", frame=f + 1)
            pb.keyframe_insert("location", frame=f + 1)
            pb.keyframe_insert("scale", frame=f + 1)
    action.use_frame_range = True
    action.frame_start, action.frame_end = 1, frames + 1
    ACTIONS.append(action)
    return action


# ---------------------------------------------------------------- animações
def idle(f):
    t = 2 * math.pi * f / 90.0
    d = {"chest": rot(pitch=1.2 * math.sin(2 * t)),
         "head": rot(pitch=3.0 * math.sin(t + 1.0), yaw=9.0 * math.sin(t))}
    for i, bone in enumerate(TAIL):
        d[bone] = rot(yaw=5.0 * math.sin(t * 2 - i * .6))
    return compose(base, d, Vector((0, 0, .006 * math.sin(2 * t))))


def walk(f):
    return walk_at(f / 30.0)


RUN_FRAMES = 16


def run(f):
    t = f / RUN_FRAMES
    src = walk_at(t)
    out = {}
    for name, (q, l) in src.items():
        bq = base[name][0]
        k = 1.3 if "leg" in name else 1.0
        out[name] = (bq @ scaled(bq.inverted() @ q, k), l)
    phase = 2 * math.pi * t
    d = {"chest": rot(pitch=4.0 + 2.0 * math.sin(2 * phase)), "head": rot(pitch=-6.0 - 2.0 * math.sin(2 * phase + .8))}
    for i, bone in enumerate(TAIL):
        d[bone] = rot(pitch=(8.0 if i == 0 else 3.0), yaw=4.0 * math.sin(phase - i * .7))
    return compose(out, d, Vector((0, 0, .035 * math.cos(2 * phase))))


UPPER = {"backleg", "backleg0", "frontleg", "frontleg0"}


def strafe(direction):
    """direction +1: anda para a esquerda do lobo; -1: para a direita. O corpo segue olhando para frente."""
    spin = Quaternion((0, 0, 1), math.radians(90.0 * direction))

    def fn(f):
        t = f / 30.0
        src = walk_at(t)
        out = {}
        for name, (q, l) in src.items():
            bq = base[name][0]
            bone = name[2:] if name.startswith("R_") else name
            delta = bq.inverted() @ q
            if bone in UPPER:
                r = POSED[name]
                world = r @ delta @ r.inverted()
                world = spin @ scaled(world, .75) @ spin.inverted()
                delta = r.inverted() @ world @ r
            elif "leg" in bone:
                delta = scaled(delta, .85)
            out[name] = (bq @ delta, l)
        phase = 2 * math.pi * t
        d = {"chest": rot(roll=-2.5 * direction), "head": rot(yaw=4.0 * direction, roll=2.0 * direction)}
        for i, bone in enumerate(TAIL):
            d[bone] = rot(yaw=(-6.0 * direction if i == 0 else 0.0) + 3.0 * math.sin(phase - i * .7))
        return compose(out, d, Vector((.02 * direction * math.sin(2 * phase), 0, 0)))
    return fn


def howl(f):
    w = ramp(f, 0, 14, 58, 74)
    shake = math.sin(f * 1.4) * 1.2 * ramp(f, 14, 20, 50, 58)
    # em pé, como o lobo uiva de verdade: peito alto, focinho para o céu, rabo baixo
    # o peito sobe pouco (girar mais derrubava a juba); as patas da frente compensam e ficam no lugar
    # (o modelo não tem osso de pescoço: a cabeça girando mais que ~36 graus estufa a garganta)
    d = {"chest": rot(pitch=-9.0 * w), "head": rot(pitch=-36.0 * w + shake)}
    for side in SIDES:
        add(d, side + "frontleg", rot(pitch=9.0 * w))
    for i, bone in enumerate(TAIL):
        d[bone] = rot(pitch=-4.0 * w)
    return compose(base, d)


def attack(f):
    # mordida: arma (0-5), bote curto com o focinho baixando (5-9, o dano sai em 0,3 s) e volta
    wind = ramp(f, 0, 5, 5, 9)
    lunge = ramp(f, 5, 9, 12, 24)
    d = {"chest": rot(pitch=-7.0 * wind + 10.0 * lunge),
         "head": rot(pitch=-14.0 * wind + 22.0 * lunge, yaw=4.0 * lunge)}
    for side in SIDES:
        add(d, side + "frontleg0", rot(pitch=-20.0 * lunge + 6.0 * wind))
        add(d, side + "frontleg1", rot(pitch=10.0 * lunge))
        add(d, side + "backleg0", rot(pitch=10.0 * lunge - 6.0 * wind))
    for i, bone in enumerate(TAIL):
        d[bone] = rot(pitch=10.0 * wind - 4.0 * lunge)
    return compose(base, d, Vector((0, .12 * wind - .32 * lunge, -.03 * wind - .02 * lunge)))


def pounce(f):
    # bote: agacha (0-8), salta (8-17, o dano sai em 0,55 s), pousa (17-22) e volta ao lugar
    crouch = ramp(f, 0, 8, 8, 11)
    u = (f - 8) / 9.0
    air = math.sin(math.pi * u) if 0 < u < 1 else 0.0
    ahead = sm(u) * (1.0 - sm((f - 19) / 14.0))
    land = ramp(f, 16, 18, 20, 27)
    d = {"chest": rot(pitch=6.0 * crouch - 12.0 * air + 8.0 * land),
         "head": rot(pitch=-8.0 * crouch - 6.0 * air + 12.0 * land)}
    for side in SIDES:
        add(d, side + "backleg0", rot(pitch=-16.0 * crouch + 30.0 * air - 8.0 * land))
        add(d, side + "backleg1", rot(pitch=22.0 * crouch - 10.0 * air + 12.0 * land))
        add(d, side + "frontleg0", rot(pitch=10.0 * crouch - 38.0 * air + 6.0 * land))
        add(d, side + "frontleg1", rot(pitch=-8.0 * crouch + 22.0 * air + 14.0 * land))
    for i, bone in enumerate(TAIL):
        d[bone] = rot(pitch=-4.0 * crouch + 10.0 * air)
    offset = Vector((0, -.75 * ahead, -.13 * crouch + .42 * air - .06 * land))
    return compose(base, d, offset)


def hit(f):
    h = sm(f / 2.0) * (1.0 - sm((f - 3) / 9.0))
    d = {"chest": rot(pitch=-9.0 * h, roll=5.0 * h), "head": rot(pitch=-14.0 * h, yaw=10.0 * h)}
    for side in SIDES:
        add(d, side + "frontleg0", rot(pitch=-6.0 * h))
    for i, bone in enumerate(TAIL):
        d[bone] = rot(yaw=-6.0 * h)
    return compose(base, d, Vector((0, .1 * h, 0)))


def death(f):
    stagger = ramp(f, 0, 6, 6, 12)
    fall = sm((f - 8) / 16.0)
    settle = sm((f - 24) / 10.0)
    d = {"Hips": rot(roll=84.0 * fall),
         "chest": rot(pitch=-6.0 * stagger),
         "head": rot(pitch=-14.0 * stagger + 18.0 * fall, yaw=-10.0 * fall - 4.0 * settle)}
    for side in SIDES:
        for bone, k in (("frontleg0", 1.0), ("frontleg1", -.6), ("backleg0", -1.0), ("backleg1", .6)):
            add(d, side + bone, rot(pitch=12.0 * k * fall))
    for i, bone in enumerate(TAIL):
        d[bone] = rot(pitch=-6.0 * fall, yaw=3.0 * settle)
    return compose(base, d, Vector((.18 * fall, 0, -.06 * stagger - .5 * fall)))


bake("Idle", 90, idle)
bake("Walk", 30, walk)
bake("Run", RUN_FRAMES, run)
bake("StrafeL", 30, strafe(1))
bake("StrafeR", 30, strafe(-1))
bake("Howl", 75, howl)
bake("Attack", 24, attack)
# no ar (quadros 8 a 18) o bote fica solto; agachado e pousado, preso no chão
bake("Pounce", 33, pounce, ground=lambda f: 1.0 - ramp(f, 7, 9, 17, 19))
bake("Hit", 12, hit)
bake("Death", 45, death)
bpy.data.actions.remove(source_walk)

arm.animation_data.action = bpy.data.actions["Idle"]
scene = bpy.context.scene
scene.frame_start, scene.frame_end = 1, 91
print("VORTEX_AMETHYST_WOLF", [(a.name, int(a.frame_end)) for a in ACTIONS], "escala", round(total_scale, 4))


def export():
    bpy.ops.object.select_all(action='DESELECT')
    arm.select_set(True)
    mesh.select_set(True)
    bpy.ops.export_scene.fbx(
        filepath=os.path.join(SRC, "Lobo_Ametista.fbx"), use_selection=True,
        object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False, armature_nodetype='NULL',
        bake_anim=True, bake_anim_use_all_actions=True, bake_anim_use_nla_strips=False,
        bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0,
        mesh_smooth_type='FACE', path_mode='STRIP', apply_unit_scale=True)
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(SRC, "Lobo_Ametista.blend"), copy=True)


if globals().get("VORTEX_EXPORT", True):
    export()
