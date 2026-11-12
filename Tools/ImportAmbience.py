"""Floresta viva: sons de ambiente em /Game/Vortex/Audio — rode com o editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/ImportAmbience.py -unattended

Do Godot (somente leitura, Assets/Sfx): amb_day (pássaros), amb_night (grilos), amb_wind (vento) e
amb_town (para a cidade, Fase 16). Do pacote de sons de referência do jogador (Free_Sounds_Pack,
VortexReferencia): o riacho calmo, para perto dos lagos; o .uasset é copiado tal como está para
Content/Free_Sounds_Pack/wav/ (mesmo caminho do pacote). Todos em loop.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
godot_sfx = os.path.normpath(os.path.join(root, "..", "..", "the-world-geneses", "Assets", "Sfx"))
destination = "/Game/Vortex/Audio"
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
lines = []
for name in ("amb_day", "amb_night", "amb_wind", "amb_town"):
    task = unreal.AssetImportTask()
    task.filename = os.path.join(godot_sfx, name + ".wav")
    task.destination_path = destination
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    wave = library.load_asset(destination + "/" + name)
    if not wave:
        raise RuntimeError("falhou: " + name)
    wave.set_editor_property("looping", True)
    library.save_loaded_asset(wave)
    lines.append("%s %.1fs" % (name, wave.get_editor_property("duration")))
stream = library.load_asset("/Game/Free_Sounds_Pack/wav/Ambient_Water_Stream_Calm_3")
if stream:
    stream.set_editor_property("looping", True)
    library.save_loaded_asset(stream)
    lines.append("riacho %.1fs" % stream.get_editor_property("duration"))
with open(os.path.join(root, "Saved", "Logs", "ImportAmbience.txt"), "w") as out:
    out.write("VORTEX_AMBIENCE " + " | ".join(lines) + "\n")
