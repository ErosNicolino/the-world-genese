"""As animações do lobo da Quaternius (CC0, Data/Creatures/Quaternius_Animais/Wolf.gltf) no Lobo Ametista (10/11).

blender -b --factory-startup -P Tools/Blender/RetargetWolf.py
Lê Data/Modelos/Lobo/Lobo_Ametista.blend (o lobo do jogo, de Tools/BuildAmethystWolf.py) e troca os clipes feitos à mão
pelos da Quaternius (o usuário: "a movimentação do lobo meio quebrada e pouca"):
  Idle <- Idle, Walk <- Walk, Run <- Gallop, Attack <- Attack, Death <- Death, Hit <- Idle_HitReact1,
  Pounce <- Gallop_Jump; e os novos para os especiais: Bite2 <- Attack (mais rápido, outro lado), Rear <- Jump_ToIdle,
  IdleSniff <- Idle_2_HeadLow. Howl, StrafeL e StrafeR continuam os de antes.
Como no MixamoToHero/MeshyBiped: o tronco, a cabeça e o rabo copiam o giro (em relação à pose de pé de cada um); as
pernas copiam a direção de cada osso. As patas ficam no chão (sem afundar nem flutuar), menos no ar do bote.
Grava Lobo_Ametista.fbx e .blend (o .blend antigo vai para Lobo_Ametista_antes_retarget.blend). Depois:
Tools/ImportAmethystWolf.py (editor fechado).
"""
import math
import os
import shutil

import bpy
from mathutils import Matrix, Quaternion, Vector

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
SRC = os.path.join(ROOT, "Data", "Modelos", "Lobo")
BLEND = os.path.join(SRC, "Lobo_Ametista.blend")
BACKUP = os.path.join(SRC, "Lobo_Ametista_antes_retarget.blend")
if not os.path.exists(BACKUP):
    shutil.copy(BLEND, BACKUP)
bpy.ops.wm.open_mainfile(filepath=BACKUP)
scene = bpy.context.scene
scene.render.fps = 30
rig = next(o for o in scene.objects if o.type == "ARMATURE")
mesh = next(o for o in scene.objects if o.type == "MESH")
keep = {a.name: a for a in bpy.data.actions}

objs = set(bpy.data.objects)
acts = set(bpy.data.actions)
bpy.ops.import_scene.gltf(filepath=os.path.join(ROOT, "Data", "Creatures", "Quaternius_Animais", "Wolf.gltf"))
q = next(o for o in set(bpy.data.objects) - objs if o.type == "ARMATURE")
qacts = {a.name.split(".")[0]: a for a in set(bpy.data.actions) - acts}
for o in set(bpy.data.objects) - objs:
    if o.type == "MESH":
        o.hide_render = True


def set_action(obj, action, frame):
    obj.animation_data_create()
    obj.animation_data.action = action
    try:
        obj.animation_data.action_slot = action.slots[0]
    except Exception:
        pass
    scene.frame_set(int(frame))
    bpy.context.view_layer.update()


def W(obj, name):
    return obj.matrix_world @ obj.pose.bones[name].matrix


# (osso do Lobo Ametista, osso da Quaternius, tipo, ponta da Quaternius, ponta nossa). tipo "g": copia o giro; "d": a
# direção do osso (perna). A ponta é o osso (ou alvo de IK) para onde o osso aponta.
MAP = [("Hips", "Back", "g", None, None), ("chest", "Torso3", "g", None, None), ("head", "Head", "g", None, None)]
# 10/11 (o usuário: "cuidado com a cauda do lobo"): a cauda NÃO é copiada (a da Quaternius é erguida e de 8 ossos; a
# nossa dobrava seco no meio e despencava no chão). Ela segue o corpo na pose natural, com um balanço suave (SWAY)
TAIL = ["tail", "tailstart", "tail1", "tail2", "tail3"]
for ours, theirs in (("", ".L"), ("R_", ".R")):
    MAP += [(ours + "frontleg", "FrontShoulder" + theirs, "g", None, None),
            (ours + "frontleg0", "FrontUpperLeg" + theirs, "d", "FrontLowerLeg" + theirs, ours + "frontleg1"),
            (ours + "frontleg1", "FrontLowerLeg" + theirs, "d", "FF" + theirs, ours + "frontleg2"),
            (ours + "frontleg2", "FrontLowerLeg" + theirs, "g", None, None),
            (ours + "backleg", "BackLeg" + theirs, "g", None, None),
            (ours + "backleg0", "BackUpperLeg" + theirs, "d", "BackLowerLeg" + theirs, ours + "backleg1"),
            (ours + "backleg1", "BackLowerLeg" + theirs, "d", "FFB" + theirs, ours + "backleg2"),
            (ours + "backleg2", "BackLowerLeg" + theirs, "g", None, None)]

# as poses de referência: os dois de pé (o primeiro quadro do Idle de cada um)
set_action(q, qacts["Idle"], qacts["Idle"].frame_range[0])
set_action(rig, keep["Idle"], keep["Idle"].frame_range[0])
REF = {}
for ours, theirs, kind, tchild, ochild in MAP:
    Rs = W(q, theirs).to_quaternion()
    Ro = W(rig, ours).to_quaternion()
    Q = Quaternion()
    if kind == "d":
        ds = (W(q, tchild).translation - W(q, theirs).translation).normalized()
        do = (W(rig, ochild).translation - W(rig, ours).translation).normalized()
        Q = ds.rotation_difference(do)
    REF[ours] = (Rs, Ro, Q)
hs0 = W(q, "Back").translation.copy()
ho0 = W(rig, "Hips").translation.copy()
K = ho0.z / max(.01, hs0.z)
POSE0 = {pb.name: (pb.rotation_quaternion.copy(), pb.location.copy()) for pb in rig.pose.bones}
for pb in rig.pose.bones:
    pb.rotation_mode = "QUATERNION"
order = [b.name for b in rig.data.bones]   # pais antes dos filhos
by = {m[0]: m for m in MAP}


def lowest():
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    ev = mesh.evaluated_get(dg)
    data = ev.to_mesh()
    z = min((ev.matrix_world @ v.co).z for v in data.vertices)
    ev.to_mesh_clear()
    return z


def sway(t, degrees, cycles):
    """O balanço da cauda de um lado para o outro (em volta do vertical), maior na ponta, com um atraso por osso."""
    if degrees <= 0:
        return
    for k, name in enumerate(TAIL):
        pb = rig.pose.bones[name]
        a = math.radians(degrees) * (.25 + .2 * k) * math.sin(2 * math.pi * cycles * t - .55 * k)
        M = rig.matrix_world @ pb.matrix
        P = Matrix.Translation(M.translation)
        pb.matrix = rig.matrix_world.inverted() @ (P @ Matrix.Rotation(a, 4, "Z") @ P.inverted() @ M)
        bpy.context.view_layer.update()


def bake(name, source, speed=1.0, ground=lambda t: 1.0, mirror=False, tail=(6.0, 1.0)):
    act = qacts[source]
    first, last = act.frame_range
    frames = max(2, int(round((last - first) / speed)))
    out = bpy.data.actions.new(name)
    out.use_fake_user = True
    prev = {}
    for i in range(frames + 1):
        set_action(q, act, first + (last - first) * i / frames)
        rig.animation_data.action = None
        for pb in rig.pose.bones:
            r, l = POSE0[pb.name]
            pb.rotation_quaternion, pb.location = r, l
        bpy.context.view_layer.update()
        for o in order:
            if o not in by:
                continue
            ours, theirs, kind, _tc, _oc = by[o]
            if mirror and theirs[-2:] in (".L", ".R"):
                theirs = theirs[:-2] + (".R" if theirs.endswith(".L") else ".L")
            Rs, Ro, Q = REF[ours]
            Ws = W(q, theirs).to_quaternion()
            Wo = Ws @ Rs.inverted() @ Q.inverted() @ Ro
            pb = rig.pose.bones[ours]
            pos = W(rig, ours).translation
            if ours == "Hips":
                pos = ho0 + (W(q, "Back").translation - hs0) * K
            pb.matrix = rig.matrix_world.inverted() @ (Matrix.Translation(pos) @ Wo.to_matrix().to_4x4())
            bpy.context.view_layer.update()
        sway(i / frames, *tail)
        # as patas no chão (no ar do bote fica solto)
        z = lowest()
        lift = -z if z < 0 else -z * ground(i / frames)
        hips = rig.pose.bones["Hips"]
        hips.matrix = Matrix.Translation((0, 0, lift)) @ hips.matrix
        bpy.context.view_layer.update()
        rig.animation_data.action = out
        for pb in rig.pose.bones:
            r = pb.rotation_quaternion.copy()
            if pb.name in prev and prev[pb.name].dot(r) < 0:
                r = -r
            prev[pb.name] = r
            pb.rotation_quaternion = r
            pb.keyframe_insert("rotation_quaternion", frame=i + 1)
            pb.keyframe_insert("location", frame=i + 1)
            pb.keyframe_insert("scale", frame=i + 1)
        rig.animation_data.action = None
    out.use_frame_range = True
    out.frame_start, out.frame_end = 1, frames + 1
    print("RETARGET", name, "<-", source, frames + 1, "quadros")
    return out


def ramp(t, a, b, c, d):
    def sm(x):
        x = min(1.0, max(0.0, x))
        return x * x * (3 - 2 * x)
    return sm((t - a) / max(1e-6, b - a)) * (1.0 - sm((t - c) / max(1e-6, d - c)))


new = {
    "Idle": bake("Idle_novo", "Idle", tail=(5.0, 2.0)),
    "Walk": bake("Walk_novo", "Walk", tail=(9.0, 1.0)),
    "Run": bake("Run_novo", "Gallop", tail=(6.0, 1.0)),
    "Attack": bake("Attack_novo", "Attack", tail=(4.0, 1.0)),
    "Death": bake("Death_novo", "Death", tail=(0.0, 1.0)),
    "Hit": bake("Hit_novo", "Idle_HitReact1", tail=(4.0, 1.0)),
    "Pounce": bake("Pounce_novo", "Gallop_Jump", ground=lambda t: 1.0 - ramp(t, .25, .35, .7, .8), tail=(4.0, 1.0)),
    # novos, para os especiais (VortexWolf.cpp)
    "Bite2": bake("Bite2_novo", "Attack", speed=1.25, mirror=True, tail=(4.0, 1.0)),
    "Rear": bake("Rear_novo", "Jump_ToIdle", ground=lambda t: 1.0 - ramp(t, .1, .2, .5, .65), tail=(5.0, 1.0)),
    "IdleSniff": bake("IdleSniff_novo", "Idle_2_HeadLow", tail=(6.0, 3.0)),
}
for name, act in new.items():
    if name in keep:
        bpy.data.actions.remove(keep[name])
    act.name = name
# limpa a Quaternius
for o in set(bpy.data.objects) - objs:
    bpy.data.objects.remove(o, do_unlink=True)
for a in list(bpy.data.actions):
    if a.name not in ("Idle", "Walk", "Run", "Attack", "Death", "Hit", "Pounce", "Bite2", "Rear", "IdleSniff", "Howl", "StrafeL", "StrafeR"):
        bpy.data.actions.remove(a)
rig.animation_data.action = bpy.data.actions["Idle"]
print("RETARGET ações", sorted(a.name for a in bpy.data.actions))

bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True)
mesh.select_set(True)
bpy.context.view_layer.objects.active = rig
bpy.ops.export_scene.fbx(
    filepath=os.path.join(SRC, "Lobo_Ametista.fbx"), use_selection=True,
    object_types={"ARMATURE", "MESH"}, add_leaf_bones=False, armature_nodetype="NULL",
    bake_anim=True, bake_anim_use_all_actions=True, bake_anim_use_nla_strips=False,
    bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0,
    mesh_smooth_type="FACE", path_mode="STRIP", apply_unit_scale=True)
bpy.ops.wm.save_as_mainfile(filepath=BLEND, copy=True)
print("RETARGET pronto")
