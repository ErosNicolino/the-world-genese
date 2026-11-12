"""Import the Godot sword and three combat clips on the existing adventurer skeleton."""

import os
import unreal

project = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
source = os.path.join(project, "Data", "GodotWeapons")
destination = "/Game/Vortex/Weapons/Sword"
skeleton_path = "/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned_Skeleton"
skeleton = unreal.EditorAssetLibrary.load_asset(skeleton_path)
if not skeleton:
    raise RuntimeError("Import the original adventurer skeleton first")

tools = unreal.AssetToolsHelpers.get_asset_tools()
pipeline = unreal.InterchangeGenericAssetsPipeline()
pipeline.set_editor_property("scene_name_sub_folder", True)
pipeline.set_editor_property("asset_type_sub_folders", True)
common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
common.set_editor_property("skeleton", skeleton)
common.set_editor_property("try_auto_select_skeleton", False)
pipeline.get_editor_property("mesh_pipeline").set_editor_property("create_physics_asset", False)
pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
pipeline.get_editor_property("material_pipeline").get_editor_property("texture_pipeline").set_editor_property("import_textures", False)
override = unreal.InterchangePipelineStackOverride()
override.add_pipeline(pipeline)

attack = unreal.AssetImportTask()
attack.filename = os.path.join(source, "Adventurer_SwordAttacks.glb")
attack.destination_path = destination
attack.automated = True
attack.save = True
attack.replace_existing = True
attack.options = override
if not unreal.EditorAssetLibrary.does_asset_exist(destination + "/Adventurer_SwordAttacks/SkeletalMeshes/Adventurer_SwordAttacksSkill_Guard"):
    tools.import_asset_tasks([attack])

sword = unreal.AssetImportTask()
sword.filename = os.path.join(source, "Sword_Default.glb")
sword.destination_path = destination
sword.automated = True
sword.save = True
sword.replace_existing = True
if not unreal.EditorAssetLibrary.does_asset_exist(destination + "/Sword_Default/StaticMeshes/Sword_Default"):
    tools.import_asset_tasks([sword])

embedded_path = destination + "/Sword_Embedded/StaticMeshes/Sword_Embedded"
if not unreal.EditorAssetLibrary.does_asset_exist(embedded_path):
    embedded = unreal.AssetImportTask()
    embedded.filename = os.path.join(source, "Sword_Embedded.glb")
    embedded.destination_path = destination
    embedded.automated = True
    embedded.save = True
    embedded.replace_existing = True
    tools.import_asset_tasks([embedded])
embedded_mesh = unreal.EditorAssetLibrary.load_asset(embedded_path)
palette = unreal.EditorAssetLibrary.load_asset("/Game/Vortex/Characters/Adventurer/MI_AdventurerPalette")
if not embedded_mesh or not palette:
    raise RuntimeError("Embedded sword mesh or adventurer palette missing")
embedded_mesh.set_material(0, palette)
unreal.EditorAssetLibrary.save_loaded_asset(embedded_mesh)

assets = unreal.EditorAssetLibrary.list_assets(destination, recursive=True)
animations = []
meshes = []
for path in assets:
    data = unreal.EditorAssetLibrary.find_asset_data(path)
    kind = str(data.asset_class_path.asset_name)
    if kind == "AnimSequence":
        animations.append(path)
        sequence = unreal.EditorAssetLibrary.load_asset(path)
        if sequence.get_editor_property("skeleton").get_path_name() != skeleton.get_path_name():
            raise RuntimeError("Attack animation has a different skeleton: " + path)
    elif kind in ("StaticMesh", "SkeletalMesh"):
        meshes.append((kind, path))
unreal.log("VORTEX_SWORD_IMPORT animations={} meshes={}".format(animations, meshes))
if len(animations) < 4 or not meshes:
    raise RuntimeError("Sword import incomplete")
