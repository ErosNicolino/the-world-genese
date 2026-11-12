"""Arte do menu novo (SourceArt/Menu, feita no Blender: SourceArt/Menu/MenuArt.blend) e a fonte Cinzel.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportMenuArt.py -unattended

- imagens T_Menu*.png -> /Game/Vortex/UI/Menu/Art (sem mipmaps, compressão de interface, grupo UI);
- Fonts/Cinzel-600/700/900.ttf -> /Game/Vortex/UI/Fonts/FF_Cinzel_<peso> (FontFace; o menu monta a fonte composta).
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
src = os.path.join(root, "SourceArt", "Menu")
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
lines = []

tasks = []
for f in sorted(os.listdir(src)):
    if f.startswith("T_Menu") and f.endswith(".png"):
        t = unreal.AssetImportTask()
        t.filename = os.path.join(src, f)
        t.destination_path = "/Game/Vortex/UI/Menu/Art"
        t.destination_name = f[:-4]
        t.automated = True
        t.save = True
        t.replace_existing = True
        tasks.append(t)
# a fonte não é importada (o importador de fontes precisa da janela do editor): os .ttf ficam em
# Content/Vortex/UI/Fonts e vão no pacote como arquivo (DefaultGame.ini, DirectoriesToAlwaysStageAsUFS)
fonts = os.path.join(src, "Fonts")
dest = os.path.join(root, "Content", "Vortex", "UI", "Fonts")
os.makedirs(dest, exist_ok=True)
import shutil
for f in sorted(os.listdir(fonts)):
    if f.endswith(".ttf") or f == "OFL.txt":
        shutil.copy2(os.path.join(fonts, f), os.path.join(dest, f))
        lines.append("fonte %s" % f)
tools.import_asset_tasks(tasks)

for t in tasks:
    path = t.destination_path + "/" + t.destination_name
    asset = lib.load_asset(path) if lib.does_asset_exist(path) else None
    if isinstance(asset, unreal.Texture2D):
        asset.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        asset.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        asset.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        asset.set_editor_property("srgb", True)
        lib.save_loaded_asset(asset)
    elif isinstance(asset, unreal.FontFace):
        # carregada no jogo pelo menu (sem cache em disco à parte)
        asset.set_editor_property("loading_policy", unreal.FontLoadingPolicy.INLINE)
        lib.save_loaded_asset(asset)
    lines.append("%s %s" % (path, type(asset).__name__ if asset else "FALHOU"))

with open(os.path.join(root, "Saved", "Logs", "ImportMenuArt.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
