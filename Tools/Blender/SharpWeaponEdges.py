"""As 5 armas do Meshy (Tools/Blender/FitNewWeapons.py) com as quinas vivas: as normais se separam nas quinas acima de 35°
— sem janela.

blender -b --factory-startup -P Tools/Blender/SharpWeaponEdges.py -- <raiz do projeto>

O Meshy faz cada bloco com os vértices compartilhados entre as faces; a Unreal suavizava as normais nos cantos e cada
face do bloco se partia em triângulos claros e escuros (a arma parecia pedra). Aqui as faces de cada bloco ficam lisas
e as quinas, vivas, e o glb é regravado no mesmo lugar (Data/VortexWeapons/Models/W_<tipo>.glb).
"""
import os
import sys

import bpy

ROOT = sys.argv[sys.argv.index("--") + 1]
MODELS = os.path.join(ROOT, "Data", "VortexWeapons", "Models")
NAMES = ["W_espada_de_cinzas", "W_martelo_de_bram", "W_adagas_gemeas", "W_adagas_gemeas_L", "W_cajado_de_ilsa", "W_arco_de_betula"]
for name in NAMES:
    path = os.path.join(MODELS, name + ".glb")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=path)
    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    for o in meshes:
        bpy.context.view_layer.objects.active = o
        o.select_set(True)
        o.data.normals_split_custom_data_clear() if hasattr(o.data, "normals_split_custom_data_clear") else None
        bpy.ops.object.shade_smooth_by_angle(angle=0.61)
        for m in list(o.modifiers):
            bpy.ops.object.modifier_apply(modifier=m.name)
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.gltf(filepath=path, use_selection=True, export_format="GLB", export_normals=True)
    print("SHARP", name, sum(len(o.data.vertices) for o in meshes), "vértices antes da separação")
