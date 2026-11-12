"""Import the Vortex terrain's own 21-tile strip and build its UE material."""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
source = os.path.join(root, "Data", "GodotTerrain", "terrain_tiles.png")
destination = "/Game/Vortex/Materials"
tools = unreal.AssetToolsHelpers.get_asset_tools()
texture_path = destination + "/terrain_tiles"
if not unreal.EditorAssetLibrary.does_asset_exist(texture_path):
    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = destination
    task.automated = True
    task.save = True
    tools.import_asset_tasks([task])
texture = unreal.EditorAssetLibrary.load_asset(texture_path)
if not texture:
    raise RuntimeError("Terrain tile atlas did not import")
texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
unreal.EditorAssetLibrary.save_loaded_asset(texture)

material_path = destination + "/M_TerrainTiles"
if not unreal.EditorAssetLibrary.does_asset_exist(material_path):
    material = tools.create_asset("M_TerrainTiles", destination, unreal.Material, unreal.MaterialFactoryNew())
else:
    material = unreal.EditorAssetLibrary.load_asset(material_path)
mel = unreal.MaterialEditingLibrary
mel.delete_all_material_expressions(material)
sample = mel.create_material_expression(material, unreal.MaterialExpressionTextureSample, -600, -120)
sample.set_editor_property("texture", texture)
tint = mel.create_material_expression(material, unreal.MaterialExpressionVertexColor, -600, 100)
multiply = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -300, 0)
mel.connect_material_expressions(sample, "RGB", multiply, "A")
mel.connect_material_expressions(tint, "RGB", multiply, "B")
mel.connect_material_property(multiply, "", unreal.MaterialProperty.MP_BASE_COLOR)
rough = mel.create_material_expression(material, unreal.MaterialExpressionConstant, -300, 220)
rough.set_editor_property("r", .95)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log("VORTEX_TERRAIN_TILES_IMPORTED atlas=" + texture_path + " material=" + material_path)
