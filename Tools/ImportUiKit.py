"""Importa o kit das janelas (Data/UI/Kit, de Tools/MakeUiKit.py) para /Game/Vortex/UI/Kit como texturas
de interface (sem mipmaps, sem compressão com perda). Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi
"""
import os
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(ROOT, "Data", "UI", "Kit")
DEST = "/Game/Vortex/UI/Kit"
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
for name in sorted(os.listdir(SRC)):
    if not name.endswith(".png"):
        continue
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SRC, name)
    task.destination_path = DEST
    task.automated = True
    task.save = True
    task.replace_existing = True
    tools.import_asset_tasks([task])
    tex = library.load_asset(f"{DEST}/{name[:-4]}")
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    library.save_loaded_asset(tex)
unreal.log_warning("VORTEX_UIKIT ok")
