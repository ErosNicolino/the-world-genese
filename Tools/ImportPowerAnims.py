"""As animações dos poderes das espadas e dos arcos na Unreal (07/11) — editor fechado, depois do
Tools/Blender/MakePowerAnims.py.
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportPowerAnims.py -unattended -nullrhi

Data/Heroes/humano_Mixamo/Poderes_humano.glb (+ poderes.json) -> /Game/Vortex/Characters/Heroes/Anims/humano/
MX_humano_Poder_<espada|arco>_<chave>_Full (e _Carga_Full), no esqueleto que o herói já usa (Hero_humano_Skeleton): o
UVortexAnimInstance acha pelo nome (Clip) e o VortexSwords::SpecialClip escolhe o da arma. Relatório em
Saved/Logs/ImportPowerAnims.txt.
"""
import json
import os

import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
# 07/11: também as dos poderes de classe (Classes_humano.glb + classes.json, MakePowerAnims.py --classes);
# VORTEX_ANIMS=classes importa só elas
_which = os.environ.get("VORTEX_ANIMS", "poderes")
# 08/11: VORTEX_ANIMS=npc importa os ofícios dos moradores (Npc_humano.glb + npc.json, MakePowerAnims.py --npc)
_files = {"classes": ("Classes_humano.glb", "classes.json"), "npc": ("Npc_humano.glb", "npc.json")}.get(_which, ("Poderes_humano.glb", "poderes.json"))
SRC = os.path.join(root, "Data", "Heroes", "humano_Mixamo", _files[0])
report = json.load(open(os.path.join(root, "Data", "Heroes", "humano_Mixamo", _files[1]), encoding="utf-8"))
ANIMS = "/Game/Vortex/Characters/Heroes/Anims/humano"
STAGING = "/Game/Vortex/Characters/Heroes/humano/_poderes"
skeleton = library.load_asset("/Game/Vortex/Characters/Heroes/humano/Hero_humano_Skeleton")
lines = ["esqueleto: %s" % (skeleton.get_path_name() if skeleton else "NENHUM")]
kind = lambda p: str(library.find_asset_data(p).asset_class_path.asset_name)

if library.does_directory_exist(STAGING):
    library.delete_directory(STAGING)
pipeline = unreal.InterchangeGenericAssetsPipeline()
pipeline.set_editor_property("scene_name_sub_folder", False)
pipeline.set_editor_property("asset_type_sub_folders", False)
common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
common.set_editor_property("skeleton", skeleton)
common.set_editor_property("try_auto_select_skeleton", False)
common.set_editor_property("import_only_animations", True)
pipeline.get_editor_property("mesh_pipeline").set_editor_property("create_physics_asset", False)
pipeline.get_editor_property("animation_pipeline").set_editor_property("import_animations", True)
pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
override = unreal.InterchangePipelineStackOverride()
override.add_pipeline(pipeline)
task = unreal.AssetImportTask()
task.filename = SRC
task.destination_path = STAGING
task.automated = True
task.save = False
task.replace_existing = True
task.options = override
tools.import_asset_tasks([task])

count = 0
for p in library.list_assets(STAGING, recursive=True):
    if kind(p) != "AnimSequence":
        continue
    asset = library.load_asset(p)
    name = next((n for n in sorted(report, key=len, reverse=True) if asset.get_name().endswith(n)), None)
    if not name:
        library.delete_asset(p.split(".")[0])
        continue
    asset.set_editor_property("enable_root_motion", False)
    final = "%s/MX_humano_%s" % (ANIMS, name)
    if library.does_asset_exist(final):
        library.delete_asset(final)
    library.rename_asset(p.split(".")[0], final)
    moved = library.load_asset(final)
    if moved:
        library.save_loaded_asset(moved)
        count += 1
        lines.append("  %s (%s, %.2f s, esqueleto %s)" % (name, report[name].get("kind", "classe"), report[name]["seconds"],
                                                         moved.get_editor_property("skeleton").get_name()))
library.delete_directory(STAGING)
lines.insert(0, "%d de %d animações" % (count, len(report)))
open(os.path.join(root, "Saved", "Logs", "ImportPowerAnims.txt"), "w", encoding="utf-8").write("\n".join(lines) + "\n")
unreal.log_warning("VORTEX_POWER_ANIMS " + lines[0] + " | " + lines[1])
