"""O braço de bloco da primeira pessoa com arco (Tools/Blender/MakeViewArm.py) na Unreal — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportViewArm.py -unattended -nullrhi
Data/VortexWeapons/Models/W_braco_1p.glb -> /Game/Vortex/Weapons/Models/W_braco_1p (malha fixa; o material vem do herói)
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
t = unreal.AssetImportTask()
t.filename = os.path.join(root, "Data", "VortexWeapons", "Models", "W_braco_1p.glb")
t.destination_path = "/Game/Vortex/Weapons/Models"
t.automated = True
t.save = True
t.replace_existing = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
found = [str(a.package_name) for a in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path("/Game/Vortex/Weapons/Models", True) if "braco_1p" in str(a.asset_name)]
print("VORTEX_BRACO", found)
