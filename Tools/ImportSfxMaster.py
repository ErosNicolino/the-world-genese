"""Importa a mixagem final dos sons (Data/SfxMaster, de Tools/BuildSfxMaster.py) — rode com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportSfxMaster.py -unattended -nullrhi

- Data/SfxMaster/Sfx/<nome>.wav       -> /Game/Vortex/Audio/Sfx/<nome> (substitui o som do mesmo nome; o jogo procura por ele)
- Data/SfxMaster/Recorded/<nome>_vN   -> /Game/Vortex/Audio/Sfx/Recorded/<nome>_vN (VortexAudio sorteia uma das tomadas)
- Data/SfxMaster/Ambience/amb_*.wav   -> /Game/Vortex/Audio/amb_* (em laço, categoria Ambiente)
Categoria de volume de cada um (as opções de áudio): efeitos -> SC_Effects, ambientes -> SC_Ambience.
Relatório em Saved/Logs/ImportSfxMaster.txt.
"""
import glob
import os
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(ROOT, "Data", "SfxMaster")
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
effects = unreal.load_asset("/Game/Vortex/Audio/SC_Effects")
ambience = unreal.load_asset("/Game/Vortex/Audio/SC_Ambience")
lines = []


def bring(path, folder, sound_class, loop):
    name = os.path.basename(path)[:-4]
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = folder
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    wave = library.load_asset(f"{folder}/{name}")
    if not wave:
        lines.append("FALHOU " + name)
        return False
    wave.set_editor_property("looping", loop)
    if sound_class:
        wave.set_editor_property("sound_class_object", sound_class)
    library.save_loaded_asset(wave, False)
    return True


counts = {}
for sub, folder, sound_class in (("Sfx", "/Game/Vortex/Audio/Sfx", effects), ("Recorded", "/Game/Vortex/Audio/Sfx/Recorded", effects),
                                 ("Ambience", "/Game/Vortex/Audio", ambience)):
    files = sorted(glob.glob(os.path.join(SRC, sub, "*.wav")))
    ok = 0
    for path in files:
        name = os.path.basename(path)[:-4]
        ok += bring(path, folder, sound_class, sub == "Ambience" or name == "portal_hum")
    counts[sub] = f"{ok}/{len(files)}"
lines.insert(0, "importados: " + ", ".join(f"{k} {v}" for k, v in counts.items()))
with open(os.path.join(ROOT, "Saved", "Logs", "ImportSfxMaster.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
unreal.log_warning("VORTEX_SFX_IMPORT " + " | ".join(lines[:5]))
