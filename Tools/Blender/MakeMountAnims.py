# 16/11: subir e descer do cavalo (herói de blocos, esqueleto do Mixamo). O clipe vai do parado (Idle) até o montado (Ride)
# com giros por cima, no espaço do esqueleto: o pé esquerdo sobe ao estribo, as mãos vão à sela, a perna direita passa por
# cima do lombo. O herói sobe pelo lado esquerdo do cavalo; quem desloca o corpo até a sela é o jogo (VortexCharacter.cpp,
# o deslize de montar). Desmontar é o mesmo ao contrário.
# Roda no Blender (pelo MCP: exec(open(este arquivo).read()); ou blender -b -P este arquivo) e grava
# Data/Heroes/humano_Mixamo/Monta_humano.glb + monta.json; na Unreal: VORTEX_ANIMS=monta com Tools/ImportPowerAnims.py.
import json
import math
import os

import bpy
from mathutils import Matrix

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
HERO = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo")
FPS = 24
SECONDS = .8
P = "mixamorig:"

for group in (bpy.data.objects, bpy.data.actions, bpy.data.meshes, bpy.data.armatures):
    for item in list(group):
        group.remove(item)
bpy.ops.import_scene.gltf(filepath=os.path.join(HERO, "Hero_humano.glb"))
arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
for extra in [o for o in bpy.data.objects if o.type == "MESH" and not o.find_armature()]:
    bpy.data.objects.remove(extra)
ORDER = [b.name for b in arm.data.bones]


def smooth(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3 - 2 * t)


def curve(u, keys):
    if u <= keys[0][0]:
        return keys[0][1]
    for (a, va), (b, vb) in zip(keys, keys[1:]):
        if u <= b:
            return va + (vb - va) * smooth((u - a) / (b - a))
    return keys[-1][1]


def sample(name, frame):
    """A pose local de todos os ossos num quadro do clipe."""
    action = bpy.data.actions[name]
    arm.animation_data_create()
    arm.animation_data.action = action
    if getattr(action, "slots", None):
        arm.animation_data.action_slot = action.slots[0]
    bpy.context.scene.frame_set(frame)
    return {pb.name: (pb.rotation_quaternion.copy(), pb.location.copy()) for pb in arm.pose.bones}


IDLE, RIDE = sample("Idle", 0), sample("Ride", 0)
arm.animation_data.action = None
for action in list(bpy.data.actions):
    bpy.data.actions.remove(action)
# as faixas NLA que vieram do arquivo ficam sem clipe e, por cima das novas, travam tudo na pose de descanso (16/11: os
# clipes novos saíam parados): fora
if arm.animation_data:
    for track in list(arm.animation_data.nla_tracks):
        arm.animation_data.nla_tracks.remove(track)

# giros por cima (graus; eixo do esqueleto: X = de lado, Y = de frente; o herói olha para -Y, a esquerda dele é +X)
#   X: - levanta para a frente (perna, braço), + inclina o tronco para a frente / dobra o joelho
#   Y: + abre a perna direita para a direita (por cima do cavalo)
STIRRUP = [(0, 0), (.28, 1), (.5, .45), (.72, 0)]          # o pé esquerdo no estribo e as mãos na sela
OVER = [(.22, 0), (.56, 1), (.86, 0)]                      # a perna direita passa por cima
EXTRA = [
    (P + "Spine", "X", 14, STIRRUP), (P + "Spine1", "X", 16, OVER),
    (P + "LeftUpLeg", "X", -58, STIRRUP), (P + "LeftLeg", "X", 72, STIRRUP),
    (P + "RightUpLeg", "Y", 78, OVER), (P + "RightUpLeg", "X", -12, OVER), (P + "RightLeg", "X", 22, OVER),
    (P + "LeftArm", "X", -62, STIRRUP), (P + "RightArm", "X", -62, STIRRUP), (P + "LeftForeArm", "X", -18, STIRRUP), (P + "RightForeArm", "X", -18, STIRRUP),
    (P + "Head", "X", 10, STIRRUP),
]


def pose(u):
    blend = smooth((u - .3) / .65)   # do parado ao montado, a partir de 30% do clipe
    for pb in arm.pose.bones:
        (qa, la), (qb, lb) = IDLE[pb.name], RIDE[pb.name]
        pb.rotation_quaternion = qa.slerp(qb, blend)
        pb.location = la.lerp(lb, blend)
    bpy.context.view_layer.update()
    for name in ORDER:
        for bone, axis, degrees, keys in EXTRA:
            angle = degrees * curve(u, keys) if bone == name else 0
            if not angle:
                continue
            pb = arm.pose.bones[name]
            head = pb.matrix.to_translation()
            pb.matrix = Matrix.Translation(head) @ Matrix.Rotation(math.radians(angle), 4, axis) @ Matrix.Translation(-head) @ pb.matrix
            bpy.context.view_layer.update()


def push(action):
    """Cada clipe na sua faixa NLA, com o encaixe (slot) dele: no Blender 5 o exportador só anima o clipe ativo; os outros
    saíam parados (16/11: o Montar e o Desmontar chegaram ao jogo sem movimento). É o que o MakePowerAnims.py faz."""
    track = arm.animation_data.nla_tracks.new()
    track.name = action.name
    strip = track.strips.new(action.name, 0, action)
    try:
        if action.slots and not strip.action_slot:
            strip.action_slot = action.slots[0]
    except Exception:
        pass
    arm.animation_data.action = None


def bake(name, backwards):
    action = bpy.data.actions.new(name)
    action.use_fake_user = True
    arm.animation_data.action = action
    frames = int(round(SECONDS * FPS))
    for f in range(frames + 1):
        pose((frames - f if backwards else f) / frames)
        for pb in arm.pose.bones:
            pb.keyframe_insert("rotation_quaternion", frame=f)
            pb.keyframe_insert("location", frame=f)
    bpy.context.scene.frame_start, bpy.context.scene.frame_end = 0, frames
    push(action)
    return frames / FPS


report = {}
for clip, backwards in (("Montar", False), ("Desmontar", True)):
    report[clip] = {"item": clip, "kind": "monta", "seconds": round(bake(clip, backwards), 3)}

# ---- 16/11: Pouso, a chegada ao Estádio do Despertar (VortexArrival, no VortexCharacter.cpp): o pouso de super-herói.
# Começa já no chão (o herói aparece no impacto): joelho esquerdo e punho direito no chão, a perna direita dobrada à
# frente, o braço esquerdo aberto para trás, a cabeça baixa; ergue a cabeça, e se levanta até o parado. 2,1 s
POUSO_SECONDS = 2.1
DOWN = [(0, 1), (.42, 1), (.8, 0)]                # agachado até 42% do clipe, de pé aos 80%
HEAD = [(0, 1), (.16, 1), (.34, -.35), (.5, -.35), (.8, 0)]   # a cabeça baixa, depois encara
POUSO = [
    (P + "Spine", "X", 30, DOWN), (P + "Spine1", "X", 22, DOWN), (P + "Spine2", "X", 12, DOWN),
    (P + "RightUpLeg", "X", -112, DOWN), (P + "RightLeg", "X", 100, DOWN),
    (P + "LeftUpLeg", "X", 12, DOWN), (P + "LeftLeg", "X", 96, DOWN),
    (P + "RightArm", "X", -52, DOWN), (P + "RightArm", "Y", -8, DOWN),
    (P + "LeftArm", "X", 58, DOWN), (P + "LeftArm", "Y", -34, DOWN), (P + "LeftForeArm", "X", -22, DOWN),
    (P + "Head", "X", 26, HEAD), (P + "Neck", "X", -30, DOWN),
]
POUSO_DROP = .2   # o quadril desce (m)


def pose_pouso(u):
    for pb in arm.pose.bones:
        pb.rotation_quaternion, pb.location = IDLE[pb.name][0].copy(), IDLE[pb.name][1].copy()
    bpy.context.view_layer.update()
    hips = arm.pose.bones[P + "Hips"]
    hips.matrix = Matrix.Translation((0, 0, -POUSO_DROP * curve(u, DOWN))) @ hips.matrix
    bpy.context.view_layer.update()
    for name in ORDER:
        for bone, axis, degrees, keys in POUSO:
            angle = degrees * curve(u, keys) if bone == name else 0
            if not angle:
                continue
            pb = arm.pose.bones[name]
            head = pb.matrix.to_translation()
            pb.matrix = Matrix.Translation(head) @ Matrix.Rotation(math.radians(angle), 4, axis) @ Matrix.Translation(-head) @ pb.matrix
            bpy.context.view_layer.update()


action = bpy.data.actions.new("Pouso")
action.use_fake_user = True
arm.animation_data.action = action
frames = int(round(POUSO_SECONDS * FPS))
for f in range(frames + 1):
    pose_pouso(f / frames)
    for pb in arm.pose.bones:
        pb.keyframe_insert("rotation_quaternion", frame=f)
        pb.keyframe_insert("location", frame=f)
push(action)
report["Pouso"] = {"item": "Pouso", "kind": "monta", "seconds": round(frames / FPS, 3)}

# ---- 16/11: Vida_Beber, beber a poção (só o braço esquerdo e a cabeça valem no jogo: VortexAnimInstance.cpp, DrinkPose).
# O de antes (life_anims_hand.py, por camadas) abria o braço para o lado: a mão ficava a 40 cm do rosto. Aqui a mão vai
# até a frente da boca (conferido no Blender: mão em x 0,18, 4 cm à frente do rosto, na altura da boca), a cabeça vai
# para trás e dá três goles. 1,25 s = UVortexHeroSheet::DrinkSeconds
RAISE = [(0, 0), (.24, 1), (1, 1)]
SIPS = [(0, 0), (.24, .75), (.4, 1), (.54, .8), (.68, 1), (.82, .8), (.96, 1), (1, 1)]
BEBER = [(P + "LeftArm", "X", -72, RAISE), (P + "LeftArm", "Z", -24, RAISE), (P + "LeftForeArm", "X", -42, SIPS), (P + "Head", "X", -16, SIPS)]
action = bpy.data.actions.new("Vida_Beber")
action.use_fake_user = True
arm.animation_data.action = action
frames = int(round(1.25 * FPS))
for f in range(frames + 1):
    for pb in arm.pose.bones:
        pb.rotation_quaternion, pb.location = IDLE[pb.name][0].copy(), IDLE[pb.name][1].copy()
    bpy.context.view_layer.update()
    for name in ORDER:
        for bone, axis, degrees, keys in BEBER:
            angle = degrees * curve(f / frames, keys) if bone == name else 0
            if not angle:
                continue
            pb = arm.pose.bones[name]
            head = pb.matrix.to_translation()
            pb.matrix = Matrix.Translation(head) @ Matrix.Rotation(math.radians(angle), 4, axis) @ Matrix.Translation(-head) @ pb.matrix
            bpy.context.view_layer.update()
    for pb in arm.pose.bones:
        pb.keyframe_insert("rotation_quaternion", frame=f)
        pb.keyframe_insert("location", frame=f)
push(action)
report["Vida_Beber"] = {"item": "Vida_Beber", "kind": "monta", "seconds": round(frames / FPS, 3)}
if "--sem-exportar" not in os.environ.get("VORTEX_MONTA", ""):
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.gltf(filepath=os.path.join(HERO, "Monta_humano.glb"), export_format="GLB", use_selection=True, export_animations=True,
                              export_animation_mode="NLA_TRACKS", export_skins=True, export_yup=True, export_force_sampling=True, export_frame_step=1)
    json.dump(report, open(os.path.join(HERO, "monta.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print("MONTA", report)
