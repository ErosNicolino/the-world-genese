"""Os estilos de rosto do humano de blocos na Unreal (01/11) — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportBlockFaces.py -unattended -nullrhi

Data/Heroes/humano_Mixamo/Face/eyes_<estilo>.png e eyes_f_<estilo>.png (Tools/MakeBlockFace.py --estilo <estilo>
[--feminino]) -> /Game/Vortex/Characters/Heroes/humano/T_humano_Eyes_<estilo> e T_humano_Eyes_F_<estilo> (pixel nítido,
sem mipmap, sempre na memória, como os do Tools/ImportHeroFace.py). O AVortexCharacter troca o "Eyes" do rosto pelo do
estilo escolhido (Look.Pieces["block_face"], lista em Data/Looks/rostos.json).
"""
import glob
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = os.path.join(root, "Data", "Heroes", "humano_Mixamo", "Face")
DEST = "/Game/Vortex/Characters/Heroes/humano"
lines = []

for path in sorted(glob.glob(os.path.join(FOLDER, "eyes_*.png"))):
    base = os.path.splitext(os.path.basename(path))[0]          # eyes_azul, eyes_f_azul (eyes_f é o feminino padrão)
    if base == "eyes_f":
        continue
    female = base.startswith("eyes_f_")
    style = base[len("eyes_f_"):] if female else base[len("eyes_"):]
    name = "T_humano_Eyes_%s%s" % ("F_" if female else "", style)
    t = unreal.AssetImportTask()
    t.filename = path
    t.destination_path = DEST
    t.destination_name = name
    t.automated = True
    t.save = True
    t.replace_existing = True
    tools.import_asset_tasks([t])
    tex = library.load_asset(DEST + "/" + name)
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("never_stream", True)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    library.save_loaded_asset(tex)
    lines.append(name)

# 09/11: as 2 peles do elfo (Tools/MakeElfSkins.py)
for path in sorted(glob.glob(os.path.join(root, "Data", "Heroes", "humano_Mixamo", "Elfo", "T_elfo_*.png"))):
    name = os.path.splitext(os.path.basename(path))[0]
    t = unreal.AssetImportTask()
    t.filename = path
    t.destination_path = DEST + "/Elfo"
    t.destination_name = name
    t.automated = True
    t.save = True
    t.replace_existing = True
    tools.import_asset_tasks([t])
    tex = library.load_asset(DEST + "/Elfo/" + name)
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("never_stream", True)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    library.save_loaded_asset(tex)
    lines.append(name)

with open(os.path.join(root, "Saved", "Logs", "ImportBlockFaces.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
unreal.log_warning("VORTEX_BLOCK_FACES " + " | ".join(lines))
