"""Importa o kit "Aventura" (Data/UI/KitAventura, de Tools/MakeUiKitAventura.py) para /Game/Vortex/UI/KitAventura
como texturas de interface: sem mipmaps, sem compressão com perda e com filtro de pixel (os blocos ficam nítidos
quando a peça estica). Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi
"""
import os
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(ROOT, "Data", "UI", "KitAventura")
DEST = "/Game/Vortex/UI/KitAventura"
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
count = 0
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
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    library.save_loaded_asset(tex)
    count += 1
unreal.log_warning(f"VORTEX_UIKIT_AVENTURA {count} peças")
