"""Animações dos animais da Quaternius (Ultimate Animated Animals, CC0: Data/Creatures/Quaternius_Animais/) nos nossos
bichos de quatro patas — sem janela.

blender -b --factory-startup -P Tools/Blender/QuadrupedTransfer.py -- <raiz do projeto> <bicho> [--preview]

Cada osso nosso gira (no mundo) o mesmo tanto que o osso da Quaternius que faz o mesmo papel gira a partir da pose
de pé dele (o primeiro quadro do Idle de cada um). Copiar a direção, como no MixamoToHero.py, só serve para o mesmo
corpo: aqui as patas têm outras proporções (a canela da Quaternius aponta para trás; a nossa, para baixo) e copiar a
direção encolhia as patas. O quadril copia também a altura (na proporção das duas alturas) e perde o deslocamento
para a frente (quem anda é o jogo). Os clipes que a Quaternius não tem (o uivo e o andar de lado
do lobo) continuam os nossos. Sai o mesmo arquivo do bicho, com os clipes trocados; depois, o import de sempre.
"""
import os
import sys

import bpy
from mathutils import Matrix, Quaternion, Vector

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
ROOT = args[0] if args else ""
CREATURE = args[1] if len(args) > 1 else "wolf"
QUATERNIUS = os.path.join(ROOT, "Data", "Creatures", "Quaternius_Animais")

# bicho: o nosso arquivo, o animal da Quaternius, os ossos (nosso <- dela, e o filho dela que dá a direção do osso)
# e os clipes (nosso <- dela)
CREATURES = {
    "wolf": {
        "file": os.path.join("Data", "Modelos", "Lobo", "Lobo_Ametista.fbx"),
        "animal": "Wolf",
        "hips": ("Hips", "Back"),
        "bones": [
            ("Hips", "Back", "Torso"), ("chest", "Torso3", "Neck1"), ("head", "Head", None),
            ("tail", "Tail1", "Tail2"), ("tailstart", "Tail2", "Tail4"), ("tail1", "Tail4", "Tail6"), ("tail2", "Tail6", "Tail7"),
            ("tail3", "Tail7", "Tail8"),
            ("backleg", "BackLeg.L", "BackUpperLeg.L"), ("backleg0", "BackUpperLeg.L", "BackLowerLeg.L"), ("backleg1", "BackLowerLeg.L", None),
            ("R_backleg", "BackLeg.R", "BackUpperLeg.R"), ("R_backleg0", "BackUpperLeg.R", "BackLowerLeg.R"), ("R_backleg1", "BackLowerLeg.R", None),
            ("frontleg", "FrontUpperLeg.L", "FrontLowerLeg.L"), ("frontleg0", "FrontLowerLeg.L", None),
            ("R_frontleg", "FrontUpperLeg.R", "FrontLowerLeg.R"), ("R_frontleg0", "FrontLowerLeg.R", None),
        ],
        # (clipe dela, opções): start/end cortam; strike cai em "at" segundos (o instante do dano em AVortexWolf:
        # mordida 0,3 s, bote 0,55 s) e o clipe dura "time"
        "clips": {"Idle": ("Idle", {}), "Walk": ("Walk", {}), "Run": ("Gallop", {}),
                  "Attack": ("Attack", {"start": 10, "strike": 29, "at": .3, "time": .8}),
                  "Pounce": ("Gallop_Jump", {"strike": 22, "at": .55, "time": 1.0}),
                  "Hit": ("Idle_HitReact1", {}), "Death": ("Death", {})},
        "moving": {"Walk", "Run", "Pounce"},
    },
}


def hoofed(file, animal, mirrored, extra=()):
    """Veado e cavalo do Godot (Data/GodotMonsters): um osso de corpo, pescoço, cabeça, rabo e duas partes por pata.
    O cavalo está espelhado (a pata esquerda em -X): os lados são ligados pela posição, não pelo nome."""
    L, R = (".R", ".L") if mirrored else (".L", ".R")
    bones = [("Body", "Torso2", "Torso3"), ("Neck", "Neck1", "Neck3"), ("Head", "Head", None), ("Tail", "Tail1", "Tail2"),
             ("Leg_FL", "FrontUpperLeg" + L, "FrontLowerLeg" + L), ("Knee_FL", "FrontLowerLeg" + L, None),
             ("Leg_FR", "FrontUpperLeg" + R, "FrontLowerLeg" + R), ("Knee_FR", "FrontLowerLeg" + R, None),
             ("Leg_BL", "BackLeg" + L, "BackUpperLeg" + L), ("Knee_BL", "BackLowerLeg" + L, None),
             ("Leg_BR", "BackLeg" + R, "BackUpperLeg" + R), ("Knee_BR", "BackLowerLeg" + R, None)] + list(extra)
    return {"file": file, "animal": animal, "hips": ("Body", "Torso2"), "bones": bones,
            "clips": {"Idle": ("Idle", {}), "Walk": ("Walk", {}), "Run": ("Gallop", {}), "Eat": ("Eating", {}),
                      "Hit": ("Idle_HitReact1", {}), "Death": ("Death", {})},
            "moving": {"Walk", "Run"}}


# o cavalo novo do Meshy (Data/Modelos/Cavalo): o esqueleto automático do site (Bone_000...). O lado +X é o .L da
# Quaternius. Não tem clipe nenhum: a pose de pé é a de descanso dele. Sai um glb novo (o original fica) com a sela
# (osso "Saddle" no lombo, onde o herói senta: AVortexMonster::RideLift) e a textura de 8K em 2K
CREATURES["horse_meshy"] = {
    "file": os.path.join("Data", "Modelos", "Cavalo", "Cavalo_Meshy.glb"),
    "out": os.path.join("Data", "Modelos", "Cavalo", "Horse_Skinned.glb"),
    "animal": "Horse",
    "hips": ("Bone_000", "Back"),
    "bones": [
        ("Bone_000", "Back", "Torso"), ("Bone_001", "Torso", "Torso2"), ("Bone_003", "Torso2", "Torso3"), ("Bone_002", "Torso3", "Neck1"),
        ("Bone_031", "Neck1", "Neck2"), ("Bone_030", "Neck2", "Neck3"), ("Bone_029", "Neck3", "Head"), ("Bone_028", "Head", None),
        ("Bone_007", "Tail1", "Tail2"), ("Bone_006", "Tail3", "Tail4"), ("Bone_005", "Tail5", "Tail6"), ("Bone_004", "Tail7", None),
        ("Bone_022", "FrontUpperLeg.L", "FrontLowerLeg.L"), ("Bone_021", "FrontLowerLeg.L", None),
        ("Bone_027", "FrontUpperLeg.R", "FrontLowerLeg.R"), ("Bone_026", "FrontLowerLeg.R", None),
        ("Bone_017", "BackLeg.L", "BackUpperLeg.L"), ("Bone_016", "BackUpperLeg.L", "BackLowerLeg.L"), ("Bone_015", "BackLowerLeg.L", None),
        ("Bone_012", "BackLeg.R", "BackUpperLeg.R"), ("Bone_011", "BackUpperLeg.R", "BackLowerLeg.R"), ("Bone_010", "BackLowerLeg.R", None),
    ],
    "clips": {"Idle": ("Idle", {}), "Walk": ("Walk", {}), "Run": ("Gallop", {}), "Eat": ("Eating", {}),
              "Hit": ("Idle_HitReact1", {}), "Death": ("Death", {}), "Rear": ("Attack_Kick", {})},
    "moving": {"Walk", "Run"},
    "saddle": ((0.0, .25, 1.115), "Bone_003"),
    "texture": 2048,
}

CREATURES["deer"] = hoofed(os.path.join("Data", "GodotMonsters", "Deer", "Deer_Skinned.glb"), "Deer", False,
                           [("Ear_L", "Ear1.L", "Ear2.L"), ("Ear_R", "Ear1.R", "Ear2.R")])
for coat in ("Baio", "Malhado", "Negro", "Palomino", "Tordilho"):
    CREATURES["horse_" + coat.lower()] = hoofed(os.path.join("Data", "GodotMonsters", "Horse_" + coat, "Horse_%s_Skinned.glb" % coat),
                                                "Horse", True)


QUAT_ACTIONS = {}     # os clipes da Quaternius pelo nome


def clip_name(action):
    """O nome do clipe sem o prefixo do FBX (Armature|...) nem o sufixo de nome repetido (.001)."""
    name = action.name.split("|")[-1]
    return name[:-4] if len(name) > 4 and name[-4] == "." and name[-3:].isdigit() else name


def log(*parts):
    print("QUADRUPED", " ".join(str(p) for p in parts))


def rest(obj, name):
    return obj.matrix_world @ obj.data.bones[name].matrix_local


def world(obj, name):
    return obj.matrix_world @ obj.pose.bones[name].matrix


def direction(obj, bone, child, posed=False):
    """A direção do osso: até o filho que dá a direção, ou o próprio eixo (a ponta)."""
    m = world(obj, bone) if posed else rest(obj, bone)
    if child:
        c = world(obj, child) if posed else rest(obj, child)
        return (c.translation - m.translation).normalized()
    return (m.to_3x3() @ Vector((0, 1, 0))).normalized()


def our_child(ours, bone):
    """O filho principal do nosso osso (o de maior descendência), para a direção dele."""
    b = ours.data.bones[bone]
    if not b.children:
        return None
    return max(b.children, key=lambda c: len(c.children_recursive)).name


def lowest(armature):
    """A altura do ponto mais baixo da malha do bicho, na pose de agora."""
    body = next(o for o in bpy.data.objects if o.type == "MESH" and o.parent == armature)
    graph = bpy.context.evaluated_depsgraph_get()
    evaluated = body.evaluated_get(graph)
    mesh = evaluated.to_mesh()
    z = min((body.matrix_world @ v.co).z for v in mesh.vertices)
    evaluated.to_mesh_clear()
    return z


def play(obj, act, frame):
    obj.animation_data_create()
    obj.animation_data.action = act
    # o clipe pode ter mais de um encaixe (o do esqueleto e o da malha): o do objeto com o nome dele, ou o primeiro
    # de objeto (com o primeiro só, o cavalo do Meshy ficava parado)
    try:
        slots = [sl for sl in act.slots if sl.target_id_type == "OBJECT"]
        named = [sl for sl in slots if sl.name_display == obj.name or sl.identifier.endswith(obj.name)]
        obj.animation_data.action_slot = (named or slots or list(act.slots))[0]
    except Exception:
        pass
    bpy.context.scene.frame_set(frame)
    bpy.context.view_layer.update()


def standing(ours, quat, cfg, our_idle):
    """As poses de pé de referência (o primeiro quadro do Idle de cada um; sem Idle, a pose de descanso): giros no
    mundo e a altura do quadril."""
    hips_o, hips_q = cfg["hips"]
    if our_idle:
        play(ours, our_idle, int(our_idle.frame_range[0]))
    else:
        ours.animation_data_create()
        ours.animation_data.action = None
        for pb in ours.pose.bones:
            pb.matrix_basis = Matrix.Identity(4)
        bpy.context.view_layer.update()
    Ro = {o: world(ours, o).to_quaternion() for o, _, _ in cfg["bones"]}
    ho0 = world(ours, hips_o).translation.copy()
    q_idle = QUAT_ACTIONS["Idle"]
    play(quat, q_idle, int(q_idle.frame_range[0]))
    # o chão: o ponto mais baixo da malha de pé (no clipe novo nada passa disso para baixo)
    paws = lowest(ours)
    Rq = {q: world(quat, q).to_quaternion() for _, q, _ in cfg["bones"]}
    hq0 = world(quat, hips_q).translation.copy()
    ours.animation_data.action = None
    return Ro, Rq, ho0, hq0, paws


def go(frame):
    """Um quadro fracionário (o clipe dela amostrado entre os quadros quando estica ou encolhe)."""
    bpy.context.scene.frame_set(int(frame), subframe=frame - int(frame))


def timeline(first, last, opts, fps):
    """Os quadros dela que viram cada quadro nosso (com strike/at/time: o golpe cai no instante do dano)."""
    first, last = opts.get("start", first), opts.get("end", last)
    if "time" not in opts:
        return [float(f) for f in range(first, last + 1)]
    count = max(2, int(round(opts["time"] * fps)) + 1)
    strike, at = opts.get("strike"), opts.get("at")
    out = []
    for i in range(count):
        t = i / fps
        if strike is None:
            out.append(first + (last - first) * i / (count - 1))
        elif t <= at:
            out.append(first + (strike - first) * t / max(1e-3, at))
        else:
            out.append(strike + (last - strike) * min(1.0, (t - at) / max(1e-3, opts["time"] - at)))
    return out


def transfer(ours, quat, cfg, clip, source, moving, ref, opts=None):
    act = QUAT_ACTIONS.get(source)
    if not act:
        log("SEM CLIPE", source)
        return None
    play(quat, act, int(act.frame_range[0]))
    scene = bpy.context.scene
    first, last = (int(round(f)) for f in act.frame_range)
    Ro, Rq, ho0, hq0, paws = ref
    hips_o, hips_q = cfg["hips"]
    k = ho0.z / max(.01, hq0.z)
    frames = timeline(first, last, opts or {}, scene.render.fps)
    hips = []
    for f in frames:
        go(f)
        hips.append(world(quat, hips_q).translation.copy())
    drift = (hips[-1] - hips[0]) * k if moving else Vector()
    drift.z = 0
    speed = drift.length / max(1e-3, (len(frames) - 1) / scene.render.fps) * 100.0
    out = bpy.data.actions.new(clip)
    ours.animation_data_create()
    ours.animation_data.action = out
    ours.animation_data.action = out
    for pb in ours.pose.bones:
        pb.rotation_mode = "QUATERNION"
    names = [o for o, _, _ in cfg["bones"]]
    # pais antes dos filhos
    order = [b.name for b in ours.data.bones if b.name in names]
    by = {o: (q, qc) for o, q, qc in cfg["bones"]}
    for i, f in enumerate(frames):
        go(f)
        for pb in ours.pose.bones:
            pb.matrix_basis = Matrix.Identity(4)
        bpy.context.view_layer.update()
        for o in order:
            q, qc = by[o]
            Wq = world(quat, q).to_quaternion()
            Wo = Wq @ Rq[q].inverted() @ Ro[o]
            pb = ours.pose.bones[o]
            pos = (ours.matrix_world @ pb.matrix).translation
            if o == hips_o:
                t = i / max(1, len(frames) - 1)
                pos = ho0 + (hips[i] - hq0) * k - drift * t
            pb.matrix = ours.matrix_world.inverted() @ (Matrix.Translation(pos) @ Wo.to_matrix().to_4x4())
            bpy.context.view_layer.update()
        # pata enterrada (o ponto mais baixo da malha abaixo do chão de pé): o corpo sobe só o que falta (no ar
        # continua solto)
        sink = paws - lowest(ours)
        if sink > 0:
            hb = ours.pose.bones[hips_o]
            m = ours.matrix_world @ hb.matrix
            hb.matrix = ours.matrix_world.inverted() @ (Matrix.Translation(Vector((0, 0, sink))) @ m)
            bpy.context.view_layer.update()
        for pb in ours.pose.bones:
            pb.keyframe_insert("rotation_quaternion", frame=i)
            if pb.name == hips_o:
                pb.keyframe_insert("location", frame=i)
    log("%-8s <- %-16s %3d quadros %5.0f cm/s" % (clip, source, len(frames), speed))
    return out


def main():
    cfg = CREATURES[CREATURE]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.fps = 30
    path = os.path.join(ROOT, cfg["file"])
    out_path = os.path.join(ROOT, cfg.get("out", cfg["file"]))
    glb = path.lower().endswith(".glb")
    if glb:
        bpy.ops.import_scene.gltf(filepath=path)
    else:
        bpy.ops.import_scene.fbx(filepath=path)
    ours = next(o for o in bpy.data.objects if o.type == "ARMATURE")
    # sobras do site (a esfera solta): só a malha presa no esqueleto fica
    for o in list(bpy.data.objects):
        if o.type == "MESH" and o.parent != ours:
            bpy.data.objects.remove(o, do_unlink=True)
    if "texture" in cfg:
        for o in bpy.data.objects:
            if o.type == "MESH":
                for mat in o.data.materials:
                    if mat and mat.use_nodes:
                        for n in mat.node_tree.nodes:
                            if n.type == "TEX_IMAGE" and n.image and max(n.image.size) > cfg["texture"]:
                                n.image.scale(cfg["texture"], cfg["texture"])
    keep = {clip_name(a): a for a in bpy.data.actions}
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(QUATERNIUS, cfg["animal"] + ".gltf"))
    new = [o for o in bpy.data.objects if o not in before]
    quat = next(o for o in new if o.type == "ARMATURE")
    QUAT_ACTIONS.update({clip_name(a): a for a in bpy.data.actions if a not in keep.values()})
    ref = standing(ours, quat, cfg, keep.get("Idle"))
    # do mesmo tamanho que o nosso (a altura do quadril)
    log("nosso", ours.name, len(ours.data.bones), "ossos; dela", quat.name, len(quat.data.bones))
    made = {}
    for clip, (source, opts) in cfg["clips"].items():
        a = transfer(ours, quat, cfg, clip, source, clip in cfg["moving"], ref, opts)
        if a:
            made[clip] = a
    for o in new:
        bpy.data.objects.remove(o, do_unlink=True)
    # sai o que não vai: os clipes da Quaternius e os nossos antigos que foram trocados (pela referência, ANTES de dar
    # os nomes do jogo: os novos nasciam "Idle.001" porque o da Quaternius já se chamava "Idle", e apagar pelo nome
    # levava o clipe novo — o veado e os cavalos saíam parados)
    finals = list(made.items()) + [(c, a) for c, a in keep.items() if c not in made]
    wanted = {a for _, a in finals}
    for a in list(bpy.data.actions):
        if a not in wanted:
            bpy.data.actions.remove(a)
    # cada clipe numa faixa com o nome do jogo: os novos e os nossos que ficaram
    ours.animation_data.action = None
    for t in list(ours.animation_data.nla_tracks):
        ours.animation_data.nla_tracks.remove(t)
    for clip, act in finals:
        act.name = clip
        track = ours.animation_data.nla_tracks.new()
        track.name = clip
        strip = track.strips.new(clip, 0, act)
        # no Blender 5 a faixa precisa do encaixe do clipe
        try:
            if act.slots and not strip.action_slot:
                strip.action_slot = act.slots[0]
        except Exception:
            pass
    bpy.ops.object.select_all(action="DESELECT")
    for o in bpy.data.objects:
        o.select_set(True)
    if "saddle" in cfg:
        # o osso da sela no lombo (sem peso: só marca onde o herói senta). Depois dos clipes: criado antes (no modo de
        # edição), os clipes saíam parados
        (x, y, z), parent = cfg["saddle"]
        bpy.context.view_layer.objects.active = ours
        bpy.ops.object.mode_set(mode="EDIT")
        bone = ours.data.edit_bones.new("Saddle")
        inv = ours.matrix_world.inverted()
        bone.head = inv @ Vector((x, y, z))
        bone.tail = inv @ Vector((x, y, z + .2))
        bone.parent = ours.data.edit_bones[parent]
        bpy.ops.object.mode_set(mode="OBJECT")
    if glb:
        # o mesmo glb do ConvertGodotMonsters.py (malha, esqueleto e um clipe por faixa)
        bpy.ops.export_scene.gltf(filepath=out_path, use_selection=True, export_format="GLB", export_animations=True,
                                  export_animation_mode="NLA_TRACKS", export_skins=True, export_apply=False, export_yup=True,
                                  export_force_sampling=True)
        log("exportado", out_path, sorted(t.name for t in ours.animation_data.nla_tracks))
        return
    # as mesmas opções do BuildAmethystWolf.py (um clipe por faixa)
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={"ARMATURE", "MESH"}, add_leaf_bones=False,
                             armature_nodetype="NULL", bake_anim=True, bake_anim_use_all_actions=False,
                             bake_anim_use_nla_strips=True, bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0,
                             mesh_smooth_type="FACE", path_mode="STRIP", apply_unit_scale=True)
    log("exportado", path, sorted(t.name for t in ours.animation_data.nla_tracks))


if __name__ == "__main__" and ROOT:
    main()
