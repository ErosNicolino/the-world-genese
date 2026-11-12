"""Marca os materiais usados em lotes (ISM) com o uso "Instanced Static Meshes". Sem a marcação, o jogo
fora do editor troca pelo material padrão (as pedras da praça da cidade e as gotas da fonte saíam
quadriculadas/escuras). Rode com o editor fechado:

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/FixInstancedMaterials.py -unattended
"""
import os
import unreal

library = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
done = []
for path in ["/Game/Vortex/Stadium/M_StadiumStone", "/Game/Vortex/Stadium/M_StadiumGlow", "/Game/Vortex/Materials/M_VortexWaterSimple",
             "/Game/Vortex/Materials/M_VortexWater", "/Game/Vortex/Materials/M_VortexWaterEpic", "/Game/Vortex/Materials/M_VoxelProp"]:
    if not library.does_asset_exist(path):
        continue
    material = library.load_asset(path)
    if not material.get_editor_property("used_with_instanced_static_meshes"):
        material.set_editor_property("used_with_instanced_static_meshes", True)
        MEL.recompile_material(material)
        library.save_loaded_asset(material)
        done.append(path)
# materiais-base das peças de voxel importadas e das armas (malhas Nanite: sem a marcação, as armas
# saíam com o material padrão, cinza quadriculado)
for folder in ["/Game/Vortex/Props", "/Game/Vortex/Characters/Adventurer", "/Game/Vortex/Weapons"]:
    for path in library.list_assets(folder, recursive=True):
        data = library.find_asset_data(path)
        if data.asset_class_path.asset_name != "Material":
            continue
        material = library.load_asset(path)
        changed = False
        for flag in ("used_with_instanced_static_meshes", "used_with_nanite"):
            if not material.get_editor_property(flag):
                material.set_editor_property(flag, True)
                changed = True
        if changed:
            MEL.recompile_material(material)
            library.save_loaded_asset(material)
            done.append(path)
root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
with open(os.path.join(root, "Saved", "Logs", "FixInstancedMaterials.txt"), "w") as out:
    out.write("VORTEX_ISM_MATERIALS %s\n" % ",".join(done))
