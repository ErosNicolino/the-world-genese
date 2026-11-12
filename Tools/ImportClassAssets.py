"""Arte das cartas do Despertar e os sons das habilidades de classe (editor fechado).

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportClassAssets.py -unattended

- Data/UI/Cards/T_Card_*.png (Tools/BuildCardArt.py) -> /Game/Vortex/UI/Cards (interface, sem mipmaps);
- os sons de classe gravados do Godot (Assets/Sfx/recorded/class_<elemento>_cast_v1.wav e
  class_<tipo>_impact_v1.wav, somente leitura) -> /Game/Vortex/Audio/Sfx/class_<...> na categoria Efeitos.
Depois rode Tools/ImportItemIcons.py para os ícones das classes e das habilidades (Data/Rpg/icons.txt).
Relatório em Saved/Logs/ImportClassAssets.txt.
"""
import glob
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
lines = []


def import_file(path, folder, name):
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = folder
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    return library.load_asset(folder + "/" + name)


cards = os.path.join(root, "Data", "UI", "Cards")
for path in sorted(glob.glob(os.path.join(cards, "T_Card_*.png"))):
    name = os.path.splitext(os.path.basename(path))[0]
    texture = import_file(path, "/Game/Vortex/UI/Cards", name)
    if not texture:
        lines.append("FALHOU " + name)
        continue
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    library.save_loaded_asset(texture)
    lines.append("carta " + name)

recorded = os.path.normpath(os.path.join(root, "..", "..", "the-world-geneses", "Assets", "Sfx", "recorded"))
effects = library.load_asset("/Game/Vortex/Audio/SC_Effects")
for path in sorted(glob.glob(os.path.join(recorded, "class_*_v1.wav"))):
    name = os.path.basename(path)[:-len("_v1.wav")]
    sound = import_file(path, "/Game/Vortex/Audio/Sfx", name)
    if not sound:
        lines.append("FALHOU " + name)
        continue
    if effects:
        sound.set_editor_property("sound_class_object", effects)
    library.save_loaded_asset(sound)
    lines.append("som " + name)

with open(os.path.join(root, "Saved", "Logs", "ImportClassAssets.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
