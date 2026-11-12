"""Importa as peças modulares geradas por ConvertGodotWardrobe.py no esqueleto do herói."""

import json
import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(PROJECT, "Data", "GodotCharacter", "Wardrobe")
DEST = "/Game/Vortex/Characters/Adventurer/Wardrobe"
SKELETON = "/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned_Skeleton"
manifest = json.load(open(os.path.join(SRC, "manifest.json"), encoding="utf-8"))
only = {name.strip() for name in os.environ.get("VORTEX_WARDROBE_ONLY", "").split(",") if name.strip()}
if only:
    manifest = {name: entry for name, entry in manifest.items() if name in only}
base_skeleton = unreal.EditorAssetLibrary.load_asset(SKELETON)
if not base_skeleton:
    raise RuntimeError("Importe primeiro Adventurer_Skinned")

tools = unreal.AssetToolsHelpers.get_asset_tools()
tasks = []
for piece_id in manifest:
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("scene_name_sub_folder", True)
    pipeline.set_editor_property("asset_type_sub_folders", True)
    common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    common.set_editor_property("skeleton", base_skeleton)
    common.set_editor_property("try_auto_select_skeleton", False)
    pipeline.get_editor_property("mesh_pipeline").set_editor_property("create_physics_asset", False)
    pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
    pipeline.get_editor_property("material_pipeline").get_editor_property("texture_pipeline").set_editor_property("import_textures", False)
    override = unreal.InterchangePipelineStackOverride()
    override.add_pipeline(pipeline)
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SRC, piece_id + ".glb")
    task.destination_path = DEST
    task.automated = True
    task.save = True
    task.replace_existing = True
    task.options = override
    tasks.append(task)
tools.import_asset_tasks(tasks)

for piece_id in manifest:
    path = f"{DEST}/{piece_id}/SkeletalMeshes/{piece_id}"
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    if not mesh:
        raise RuntimeError(f"Malha ausente: {path}")
    if mesh.get_editor_property("skeleton").get_path_name() != base_skeleton.get_path_name():
        raise RuntimeError(f"Esqueleto diferente: {path}")
    unreal.log(f"WARDROBE_SHARED_SKELETON {piece_id}")
unreal.log(f"WARDROBE_IMPORTED {len(manifest)}")
