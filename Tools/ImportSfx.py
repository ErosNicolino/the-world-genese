"""Sons do jogo e músicas do Godot (somente leitura) — rode com o editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/ImportSfx.py -unattended

- Assets/Sfx/<nome>.wav -> /Game/Vortex/Audio/Sfx/<nome>: golpes, passos, pulo, queda, rolamento,
  acertos, dor, morte, cura, defesa, água e o lobo (uivo, rosnado, mordida, bote).
- Assets/Music -> /Game/Vortex/Audio/Music (em loop): vila (praça do santuário), noite (vale
  adormecido) e luta (fúria de Azhur). A exploração usa o tema do menu (MUS_Theme, como no Godot).
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
godot = os.path.normpath(os.path.join(root, "..", "..", "the-world-geneses", "Assets"))
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary

SFX = ["sword_swing_1", "sword_swing_2", "sword_swing_3", "sword_hit", "creature_hit", "hurt", "death", "jump", "land", "roll",
       "step_grass_1", "step_grass_2", "step_grass_3", "step_stone_1", "step_stone_2", "step_stone_3",
       "step_sand_1", "step_sand_2", "step_sand_3", "step_mud_1", "step_mud_2", "step_mud_3",
       "swim_stroke", "water_splash", "heal", "guard_armor", "skill_barrier_hit", "skill_slash",
       "wolf_bite", "wolf_growl", "wolf_howl", "wolf_pounce", "pickup", "level_up", "coin"]
MUSIC = {"praca_do_santuario": "MUS_Village", "vale_adormecido": "MUS_Night", "furia_de_azhur": "MUS_Combat"}


def bring(source, folder, name, loop):
    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = folder
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    wave = library.load_asset(folder + "/" + name)
    if not wave:
        raise RuntimeError("falhou: " + source)
    wave.set_editor_property("looping", loop)
    library.save_loaded_asset(wave)
    return wave


lines = []
for name in SFX:
    bring(os.path.join(godot, "Sfx", name + ".wav"), "/Game/Vortex/Audio/Sfx", name, False)
lines.append("%d sons" % len(SFX))
for source, name in MUSIC.items():
    wave = bring(os.path.join(godot, "Music", source + ".wav"), "/Game/Vortex/Audio/Music", name, True)
    lines.append("%s %.0fs" % (name, wave.get_editor_property("duration")))
with open(os.path.join(root, "Saved", "Logs", "ImportSfx.txt"), "w") as out:
    out.write("VORTEX_SFX " + " | ".join(lines) + "\n")
