"""A fonte de animações de gente para os monstros de duas pernas (10/11): o esqueleto do Mixamo dos heróis e os clipes
dele, com os nomes que o jogo pede ao monstro. Usada pelo Tools/Blender/MeshyBiped.py (fonte "mixamo"), no lugar dos
clipes duros do Godot (o goblin antigo tinha o braço de um osso só: andava "reto, com os braços parados").

blender -b --factory-startup -P Tools/Blender/MixamoMonsterSource.py -- <saída.glb> <Nome=clipe> [<Nome=clipe> ...]
  ex.: ... -- Data/Modelos/Goblin_Meshy/fonte_mixamo.glb Idle=Idle Walk=Walk Run=RunArmed Attack=golpe1 Throw=golpe2 Hit=Hit Death=Death
Os clipes são os Data/Heroes/Mixamo/humano/clips/humano_<clipe>.fbx.
"""
import os
import sys

import bpy

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
args = sys.argv[sys.argv.index("--") + 1:]
OUT = args[0] if os.path.isabs(args[0]) else os.path.join(ROOT, args[0])
WANT = [a.split("=", 1) for a in args[1:]]
CLIPS = os.path.join(ROOT, "Data", "Heroes", "Mixamo", "humano", "clips")

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.render.fps = 30
bpy.ops.import_scene.fbx(filepath=os.path.join(ROOT, "Data", "Heroes", "Mixamo", "humano", "humano.fbx"), automatic_bone_orientation=False)
rig = next(o for o in bpy.data.objects if o.type == "ARMATURE")
for a in list(bpy.data.actions):
    bpy.data.actions.remove(a)
rig.animation_data_create()
for name, clip in WANT:
    objs, acts = set(bpy.data.objects), set(bpy.data.actions)
    bpy.ops.import_scene.fbx(filepath=os.path.join(CLIPS, "humano_%s.fbx" % clip), automatic_bone_orientation=False)
    act = next(iter(set(bpy.data.actions) - acts))
    for o in set(bpy.data.objects) - objs:
        bpy.data.objects.remove(o, do_unlink=True)
    act.name = name
    track = rig.animation_data.nla_tracks.new()
    track.name = name
    strip = track.strips.new(name, int(act.frame_range[0]), act)
    try:
        if act.slots and not strip.action_slot:
            strip.action_slot = act.slots[0]
    except Exception:
        pass
    print("FONTE", name, "<-", clip, tuple(int(f) for f in act.frame_range))
rig.animation_data.action = None
bpy.ops.object.select_all(action="SELECT")
os.makedirs(os.path.dirname(OUT), exist_ok=True)
bpy.ops.export_scene.gltf(filepath=OUT, use_selection=True, export_format="GLB", export_animations=True,
                          export_animation_mode="NLA_TRACKS", export_skins=True, export_yup=True)
print("FONTE exportada", OUT)
