"""A HQ de abertura na Unreal (antes: python Tools/MakeIntroComic.py). Rode com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportIntroComic.py -unattended -nullrhi

- Data/UI/Abertura/folha_N.png -> /Game/Vortex/UI/Abertura/T_Folha_N (interface, com mipmaps: a folha inteira
  aparece pequena e o quadro aparece grande, sem serrilhar);
- Data/UI/Abertura/Cinema/Cxx.png -> /Game/Vortex/UI/Abertura/Cinema/T_Cxx (as cenas pintadas do modo cinema: quando
  existem, ocupam o lugar do quadro da folha);
- Data/UI/Abertura/MUS_Abertura.wav -> /Game/Vortex/Audio/Music/MUS_Abertura (a canção "Acorde": a HQ segue o tempo dela);
- Data/Narracao/final/pN_qM.wav (a voz escolhida, quando existir) -> /Game/Vortex/Audio/Narracao/pN_qM.
"""
import glob
import os
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
lines = []


def bring(path, folder, name):
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = folder
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    return library.load_asset(folder + "/" + name)


pages = 0
for path in sorted(glob.glob(os.path.join(ROOT, "Data", "UI", "Abertura", "folha_*.png"))):
    n = os.path.basename(path)[len("folha_"):-4]
    texture = bring(path, "/Game/Vortex/UI/Abertura", "T_Folha_" + n)
    if not texture:
        raise RuntimeError("falhou: " + path)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("never_stream", True)
    library.save_loaded_asset(texture)
    pages += 1
lines.append("%d folhas" % pages)
# as cenas de cinema (uma imagem larga por verso; C06a/C06b nos versos de 4 linhas; C00 atrás do título)
scenes = 0
for path in sorted(glob.glob(os.path.join(ROOT, "Data", "UI", "Abertura", "Cinema", "C*.png"))):
    texture = bring(path, "/Game/Vortex/UI/Abertura/Cinema", "T_" + os.path.basename(path)[:-4])
    if not texture:
        raise RuntimeError("falhou: " + path)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("never_stream", True)
    library.save_loaded_asset(texture)
    scenes += 1
lines.append("%d cenas" % scenes)
song = os.path.join(ROOT, "Data", "UI", "Abertura", "MUS_Abertura.wav")
if os.path.exists(song):
    music = bring(song, "/Game/Vortex/Audio/Music", "MUS_Abertura")
    if music:
        # categoria Música: segue o volume de música das opções (o ambiente e os efeitos calam durante a HQ)
        music.set_editor_property("sound_class_object", library.load_asset("/Game/Vortex/Audio/SC_Music"))
        library.save_loaded_asset(music)
        lines.append("cancao %.1f s" % music.get_editor_property("duration"))

voices = 0
for path in sorted(glob.glob(os.path.join(ROOT, "Data", "Narracao", "final", "p*_q*.wav"))):
    name = os.path.basename(path)[:-4]
    sound = bring(path, "/Game/Vortex/Audio/Narracao", name)
    if sound:
        library.save_loaded_asset(sound)
        voices += 1
lines.append("%d falas" % voices)

with open(os.path.join(ROOT, "Saved", "Logs", "ImportIntroComic.txt"), "w", encoding="utf-8") as out:
    out.write(" | ".join(lines) + "\n")
unreal.log_warning("VORTEX_INTRO_IMPORT " + " | ".join(lines))
