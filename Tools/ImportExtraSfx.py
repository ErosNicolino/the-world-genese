"""Sons novos feitos no projeto (02/11) — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportExtraSfx.py -unattended -nullrhi

Data/Audio/<nome>.wav -> /Game/Vortex/Audio/Sfx/<nome>; os <nome>_v1.. (as variações que o VortexAudio sorteia) vão para
Sfx/Recorded. Ex.: o assobio do cavalo (Tools/MakeWhistle.py).
"""
import glob
import os
import re
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
lines = []
for path in sorted(glob.glob(os.path.join(root, "Data", "Audio", "*.wav"))):
    name = os.path.splitext(os.path.basename(path))[0]
    # MUS_<nome> (03/11): músicas (a do Azhûr), na pasta das músicas e em laço
    folder = "/Game/Vortex/Audio/Music" if name.startswith("MUS_") else "/Game/Vortex/Audio/Sfx/Recorded" if re.search(r"_v\d+$", name) else "/Game/Vortex/Audio/Sfx"
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = folder
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    wave = library.load_asset(folder + "/" + name)
    if not wave:
        lines.append("FALHOU " + name)
        continue
    wave.set_editor_property("looping", name.startswith("MUS_"))
    library.save_loaded_asset(wave)
    lines.append("%s/%s" % (folder, name))
with open(os.path.join(root, "Saved", "Logs", "ImportExtraSfx.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
unreal.log_warning("VORTEX_EXTRA_SFX " + " | ".join(lines))
