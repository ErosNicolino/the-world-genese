"""Importa o aventureiro convertido (Data/GodotCharacter/Adventurer_Skinned.glb, gerado por
ConvertGodotAdventurer.py) para /Game/Vortex/Characters/Adventurer: malha com esqueleto, os clipes
e um material com a paleta x cor do vértice (como no Godot)."""

import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(PROJECT, "Data", "GodotCharacter", "Adventurer_Skinned.glb")
DEST = "/Game/Vortex/Characters/Adventurer"
tools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary

task = unreal.AssetImportTask()
task.filename = SRC
task.destination_path = DEST
task.automated = True
task.save = True
task.replace_existing = True
tools.import_asset_tasks([task])

assets = unreal.EditorAssetLibrary.list_assets(DEST, recursive=True)
kinds = {}
for path in assets:
    kinds.setdefault(unreal.EditorAssetLibrary.find_asset_data(path).asset_class_path.asset_name, []).append(path)
for k, v in sorted(kinds.items()):
    unreal.log(f"VORTEX_ADVENTURER_ASSET {k}: {len(v)} {v[:3]}")

# material: paleta x cor do vértice, usado em malha com esqueleto
mat_path = "/Game/Vortex/Materials/M_AdventurerPalette"
if not unreal.EditorAssetLibrary.does_asset_exist(mat_path):
    material = tools.create_asset("M_AdventurerPalette", "/Game/Vortex/Materials", unreal.Material, unreal.MaterialFactoryNew())
    color = MEL.create_material_expression(material, unreal.MaterialExpressionVertexColor, -600, 0)
    palette = MEL.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -600, 200)
    palette.set_editor_property("parameter_name", "Palette")
    mul = MEL.create_material_expression(material, unreal.MaterialExpressionMultiply, -300, 100)
    MEL.connect_material_expressions(color, "", mul, "A")
    MEL.connect_material_expressions(palette, "RGB", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = MEL.create_material_expression(material, unreal.MaterialExpressionConstant, -300, 300)
    rough.set_editor_property("r", 0.88)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    material.set_editor_property("used_with_skeletal_mesh", True)
    MEL.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
material = unreal.EditorAssetLibrary.load_asset(mat_path)

texture = next((unreal.EditorAssetLibrary.load_asset(p) for p in kinds.get("Texture2D", [])), None)
if texture:
    texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
mi_path = f"{DEST}/MI_AdventurerPalette"
if not unreal.EditorAssetLibrary.does_asset_exist(mi_path):
    mi = tools.create_asset("MI_AdventurerPalette", DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, material)
    if texture:
        MEL.set_material_instance_texture_parameter_value(mi, "Palette", texture)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
mi = unreal.EditorAssetLibrary.load_asset(mi_path)
for path in kinds.get("SkeletalMesh", []):
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    materials = mesh.get_editor_property("materials")
    for slot in materials:
        slot.set_editor_property("material_interface", mi)
    mesh.set_editor_property("materials", materials)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
unreal.log("VORTEX_ADVENTURER_IMPORTED")
