"""O corpo feminino do humano de blocos na Unreal (01/11) — editor fechado, depois do ImportMixamoHeroes.py.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportBlockFemale.py -unattended -nullrhi

Data/Heroes/humano_Mixamo/Feminino/Hero_humano.glb (RigBlockCharacter.py --feminino + MixamoToHero.py --skin --out:
o busto e a cintura fina, no mesmo esqueleto) -> /Game/Vortex/Characters/Heroes/humano/Feminino/Hero_humano, presa ao
Hero_humano_Skeleton (toca os mesmos clipes) e com o MI_Hero_humano. O AVortexCharacter usa essa malha no corpo
feminino (HeroModelFor); o mesmo nome do masculino faz o resto do jogo (clipes, rosto, material) achar tudo igual.
Relatório em Saved/Logs/ImportBlockFemale.txt.
10/11: ao reimportar, apagar antes Content/Vortex/Characters/Heroes/humano/Feminino/Hero_humano.uasset (editor fechado):
o delete_asset daqui não solta a malha velha e o RenameAsset falha ("An asset already exists"), deixando a antiga.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
SRC = os.path.join(root, "Data", "Heroes", "humano_Mixamo", "Feminino", "Hero_humano.glb")
FOLDER = "/Game/Vortex/Characters/Heroes/humano"
DEST = FOLDER + "/Feminino"
STAGING = DEST + "/_import"
lines = []
kind = lambda p: str(library.find_asset_data(p).asset_class_path.asset_name)

skeleton = library.load_asset(FOLDER + "/Hero_humano_Skeleton")
if library.does_directory_exist(STAGING):
    library.delete_directory(STAGING)
pipeline = unreal.InterchangeGenericAssetsPipeline()
pipeline.set_editor_property("scene_name_sub_folder", False)
pipeline.set_editor_property("asset_type_sub_folders", False)
common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
common.set_editor_property("try_auto_select_skeleton", False)
common.set_editor_property("skeleton", skeleton)
pipeline.get_editor_property("mesh_pipeline").set_editor_property("create_physics_asset", False)
pipeline.get_editor_property("animation_pipeline").set_editor_property("import_animations", False)
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
assets = library.list_assets(STAGING, recursive=True)
mesh_path = next((p for p in assets if kind(p) == "SkeletalMesh"), None)
if not mesh_path:
    lines.append("FALHOU: %s" % assets)
else:
    target = DEST + "/Hero_humano"
    if library.does_asset_exist(target):
        library.delete_asset(target)
    library.rename_asset(mesh_path.split(".")[0], target)
    mesh = library.load_asset(target)
    lines.append("esqueleto: %s" % mesh.get_editor_property("skeleton").get_path_name())
    mi = library.load_asset(FOLDER + "/MI_Hero_humano")
    if mi:
        slots = []
        for slot in mesh.get_editor_property("materials"):
            s = unreal.SkeletalMaterial()
            s.set_editor_property("material_interface", mi)
            s.set_editor_property("material_slot_name", slot.get_editor_property("material_slot_name"))
            slots.append(s)
        mesh.set_editor_property("materials", slots)
    library.save_loaded_asset(mesh)
    lines.append("malha: %s" % target)
if library.does_directory_exist(STAGING):
    library.delete_directory(STAGING)
with open(os.path.join(root, "Saved", "Logs", "ImportBlockFemale.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
unreal.log_warning("VORTEX_BLOCK_FEMALE " + " | ".join(lines))
