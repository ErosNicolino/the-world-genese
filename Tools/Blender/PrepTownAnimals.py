"""Os bichos do Cube World Kit (Quaternius, CC0) prontos para a Unreal (11/11).

Os .glb originais (`modelando ainda/quaternius_cubos/pacote`) trazem uma esfera solta ("Icosphere", 1,9 m, que não é do
bicho), pontos de ajuda (*_end_end) e o esqueleto com escala 100 e ossos de 1 cm: na Unreal saíam formas gigantes e cada
bicho de um tamanho. Aqui: tira a esfera e os pontos, aplica a escala no esqueleto (e multiplica os movimentos das
animações pela mesma escala), dá nomes curtos às animações (Idle, Walk, Run, Idle_Eating...) e grava
Data/Town/Animals/<Bicho>.glb. Depois: Tools/ImportTownLife.py.

blender -b --factory-startup --disable-autoexec -P Tools/Blender/PrepTownAnimals.py
"""
import glob
import os

import bpy

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
SOURCE = os.path.join(ROOT, "modelando ainda", "quaternius_cubos", "pacote")
OUT = os.path.join(ROOT, "Data", "Town", "Animals")
os.makedirs(OUT, exist_ok=True)

for path in sorted(glob.glob(os.path.join(SOURCE, "*.glb"))):
    name = os.path.splitext(os.path.basename(path))[0]
    if name == "Horse":
        continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=path)
    arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
    body = [o for o in bpy.data.objects if o.type == "MESH" and o.parent == arm]
    for o in list(bpy.data.objects):
        if o != arm and o not in body:
            bpy.data.objects.remove(o, do_unlink=True)
    factor = arm.scale.x
    # a escala vai para dentro do esqueleto e da malha
    bpy.ops.object.select_all(action="DESELECT")
    for o in [arm] + body:
        o.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    # os movimentos das animações estavam na escala antiga dos ossos
    actions = []
    for action in bpy.data.actions:
        # Blender 5: as curvas ficam nas camadas da ação (layers > strips > channelbags)
        curves = [fc for layer in action.layers for strip in layer.strips for bag in strip.channelbags for fc in bag.fcurves]
        for fc in curves:
            if fc.data_path.endswith(".location"):
                for k in fc.keyframe_points:
                    k.co[1] *= factor
                    k.handle_left[1] *= factor
                    k.handle_right[1] *= factor
        action["curves"] = len(curves)
    # cada animação vem duas vezes (uma para a armação e outra quase vazia): fica a que tem mais curvas
    best = {}
    for action in list(bpy.data.actions):
        short = action.name.split("|")[-1].split(".")[0]
        if short not in best or action["curves"] > best[short]["curves"]:
            if short in best:
                bpy.data.actions.remove(best[short])
            best[short] = action
        else:
            bpy.data.actions.remove(action)
    for short, action in best.items():
        action.name = short
        actions.append(short)
    tris = sum(len(p.vertices) - 2 for o in body for p in o.data.polygons)
    height = max((o.matrix_world @ v.co).z for o in body for v in o.data.vertices)
    bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, name + ".glb"), export_format="GLB", export_animations=True,
                              export_animation_mode="ACTIONS", export_skins=True, export_apply=False, use_selection=False)
    print("BICHO %s: escala %.0f aplicada, %.2f m de altura, %d triângulos, %s" % (name, factor, height, tris, ", ".join(sorted(actions))))
