"""Ícones novos do menu (Data/UI/Menu/T_MenuIcon_*.png) em /Game/Vortex/UI/Menu/Art — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportMenuIcons.py -unattended -nullrhi

Hoje: T_MenuIcon_Trophy (Conquistas), feito por Tools/Blender/MakeTrophyIcon.py. Mesmas configurações de
interface dos outros ícones (sem mipmaps, grupo de UI, sem compressão com perda).
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
folder = os.path.join(root, "Data", "UI", "Menu")
destination = "/Game/Vortex/UI/Menu/Art"
tools = unreal.AssetToolsHelpers.get_asset_tools()
for name in sorted(os.listdir(folder)):
    if not (name.startswith("T_MenuIcon_") and name.endswith(".png")):
        continue
    task = unreal.AssetImportTask()
    task.filename = os.path.join(folder, name)
    task.destination_path = destination
    task.destination_name = name[:-4]
    task.automated = True
    task.save = True
    task.replace_existing = True
    tools.import_asset_tasks([task])
    texture = unreal.EditorAssetLibrary.load_asset(destination + "/" + name[:-4])
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    unreal.log("VORTEX_MENU_ICON " + name[:-4])
