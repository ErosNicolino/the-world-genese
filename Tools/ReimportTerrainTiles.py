"""Reimporta só a textura do chão (Data/GodotTerrain/terrain_tiles.png -> /Game/Vortex/Materials/terrain_tiles), sem
mexer no M_TerrainTiles. O Tools/ImportTerrainTiles.py refaz o material do zero e apagaria as manchas do
Tools/ImproveVegetationMaterials.py. Editor fechado:

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<raiz>/Tools/ReimportTerrainTiles.py -unattended -nullrhi
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
task = unreal.AssetImportTask()
task.filename = os.path.join(root, "Data", "GodotTerrain", "terrain_tiles.png")
task.destination_path = "/Game/Vortex/Materials"
task.destination_name = "terrain_tiles"
task.automated = True
task.replace_existing = True
task.replace_existing_settings = False
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.EditorAssetLibrary.load_asset("/Game/Vortex/Materials/terrain_tiles")
texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log("VORTEX_TERRAIN_TILES_REIMPORTED %dx%d" % (texture.blueprint_get_size_x(), texture.blueprint_get_size_y()))
