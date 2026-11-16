# 16/11: clipes novos do cavalo (o do Meshy, Data/Modelos/Cavalo/Horse_Skinned.glb), posados por giros no espaço do
# esqueleto: o Brake (a freada: senta nas patas de trás, as da frente travam à frente, o pescoço recua) e o Jump (o salto).
# Roda no Blender (pelo MCP: exec(open(este arquivo).read()); ou blender -b -P este arquivo) e grava
# Data/Modelos/Cavalo/Horse_Anims.glb; na Unreal, Tools/ImportHorseAnims.py põe o clipe nas cinco pelagens.
import math
import os

import bpy
from mathutils import Matrix

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
SRC = os.path.join(ROOT, "Data", "Modelos", "Cavalo", "Horse_Skinned.glb")
OUT = os.path.join(ROOT, "Data", "Modelos", "Cavalo", "Horse_Anims.glb")
FPS = 24

for group in (bpy.data.objects, bpy.data.actions, bpy.data.meshes, bpy.data.armatures):
    for item in list(group):
        group.remove(item)
bpy.ops.import_scene.gltf(filepath=SRC)
arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
for extra in [o for o in bpy.data.objects if o.type == "MESH" and not o.find_armature()]:
    bpy.data.objects.remove(extra)   # a esfera que o importador cria para os ossos
for action in list(bpy.data.actions):
    bpy.data.actions.remove(action)
# as faixas NLA que vieram do arquivo ficam sem clipe e, por cima das novas, travam tudo na pose de descanso (16/11: os
# clipes novos saíam parados): fora
if arm.animation_data:
    for track in list(arm.animation_data.nla_tracks):
        arm.animation_data.nla_tracks.remove(track)
ORDER = [b.name for b in arm.data.bones]   # pais antes dos filhos


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


def pose(rot, drop=0.0):
    """rot: {osso: graus em volta do eixo de lado do esqueleto (+ = a ponta desce/vai para trás)}; drop: a raiz desce (m)."""
    for pb in arm.pose.bones:
        pb.rotation_quaternion = (1, 0, 0, 0)
        pb.location = (0, 0, 0)
    bpy.context.view_layer.update()
    for name in ORDER:
        pb = arm.pose.bones[name]
        if name == ORDER[0] and drop:
            pb.matrix = Matrix.Translation((0, 0, -drop)) @ pb.matrix
            bpy.context.view_layer.update()
        angle = rot.get(name, 0)
        if angle:
            head = pb.matrix.to_translation()
            pb.matrix = Matrix.Translation(head) @ Matrix.Rotation(math.radians(angle), 4, "X") @ Matrix.Translation(-head) @ pb.matrix
            bpy.context.view_layer.update()


# a freada no auge: a raiz (quadril) empina o peito; frente = 022/027 (ombro) e 021/026; trás = 012/017 (coxa),
# 011/016, 010/015 e 009/014 (casco); pescoço 030/029, cabeça 028, rabo 007/006
SIT = {"Bone_000": -11,
       "Bone_022": -16, "Bone_027": -16, "Bone_021": 4, "Bone_026": 4,
       "Bone_012": -34, "Bone_017": -34, "Bone_011": 46, "Bone_016": 46, "Bone_010": -28, "Bone_015": -28, "Bone_009": 27, "Bone_014": 27,
       "Bone_030": -16, "Bone_029": 6, "Bone_028": 20, "Bone_007": -35, "Bone_006": -10}
SIT_DROP = .25
LATE = {"Bone_030", "Bone_029", "Bone_028", "Bone_007", "Bone_006"}   # o pescoço e o rabo chegam um pouco depois


def bake(name, seconds, frame_pose):
    """frame_pose(u) -> (giros, quanto a raiz desce). Cada clipe vai para a sua faixa NLA com o encaixe dele: no Blender 5 o
    exportador só anima o clipe ativo, os outros saem parados."""
    action = bpy.data.actions.new(name)
    action.use_fake_user = True
    arm.animation_data_create()
    arm.animation_data.action = action
    frames = int(round(seconds * FPS))
    for f in range(frames + 1):
        rot, drop = frame_pose(f / frames)
        pose(rot, drop)
        for pb in arm.pose.bones:
            pb.keyframe_insert("rotation_quaternion", frame=f)
            pb.keyframe_insert("location", frame=f)
    bpy.context.scene.frame_start, bpy.context.scene.frame_end = 0, frames
    track = arm.animation_data.nla_tracks.new()
    track.name = name
    strip = track.strips.new(name, 0, action)
    try:
        if action.slots and not strip.action_slot:
            strip.action_slot = action.slots[0]
    except Exception:
        pass
    arm.animation_data.action = None
    return action


def brake(u):
    # 0,95 s: desce em 0,17 s, escorrega segurando e levanta devagar (acaba de pé: emenda no parado)
    weight = [(0, 0), (.18, 1), (.55, .8), (1, 0)]
    w, late = curve(u, weight), curve(u - .06, weight)
    return {n: a * (late if n in LATE else w) for n, a in SIT.items()}, SIT_DROP * w


# 16/11 (o usuário: "crie animação ao pular com o cavalo, que ele continua andando mesmo no ar"): o salto, em laço enquanto
# o cavaleiro está no ar. As patas da frente recolhidas debaixo do peito, as de trás esticadas para trás, o pescoço à
# frente e o rabo solto; um balanço pequeno para não ficar duro (o laço fecha: tudo em seno de uma volta)
LEAP = {"Bone_000": -5,
        "Bone_022": -34, "Bone_027": -30, "Bone_021": 74, "Bone_026": 70, "Bone_020": -58, "Bone_025": -54,
        "Bone_012": 38, "Bone_017": 34, "Bone_011": -22, "Bone_016": -20, "Bone_010": 26, "Bone_015": 24, "Bone_009": 20, "Bone_014": 20,
        "Bone_030": 14, "Bone_029": -6, "Bone_028": -6, "Bone_007": -48, "Bone_006": -14}
SWAY = {"Bone_000": 2.5, "Bone_022": 5, "Bone_027": -5, "Bone_021": 6, "Bone_026": -6, "Bone_012": -5, "Bone_017": 5, "Bone_030": 3, "Bone_007": 6, "Bone_006": 6}


def leap(u):
    wave = math.sin(u * 2 * math.pi)
    return {n: a + SWAY.get(n, 0) * wave for n, a in LEAP.items()}, 0.0


bake("Brake", .95, brake)
bake("Jump", .7, leap)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.export_scene.gltf(filepath=OUT, export_format="GLB", use_selection=True, export_animations=True, export_animation_mode="NLA_TRACKS",
                          export_skins=True, export_yup=True, export_force_sampling=True, export_frame_step=1)
print("CAVALO exportado", OUT, [a.name for a in bpy.data.actions])
