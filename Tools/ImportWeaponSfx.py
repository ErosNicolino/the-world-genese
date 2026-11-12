"""Os sons das armas (Tools/BuildWeaponSfx.py) no jogo — rode com o editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportWeaponSfx.py -unattended

Data/WeaponSfx/out/wfx_*.wav -> /Game/Vortex/Audio/Sfx/wfx_* na categoria Efeitos (SC_Effects, o volume
de efeitos das opções). O jogo procura wfx_<família>_swing/hit/shot e os sons dos secundários por nome.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
source = os.path.join(root, "Data", "WeaponSfx", "out")
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
effects = library.load_asset("/Game/Vortex/Audio/SC_Effects")
FOLDER = "/Game/Vortex/Audio/Sfx"
done = []
tasks = []
for f in sorted(os.listdir(source)):
    if not f.endswith(".wav"):
        continue
    t = unreal.AssetImportTask()
    t.filename = os.path.join(source, f)
    t.destination_path = FOLDER
    t.destination_name = f[:-4]
    t.automated = True
    t.replace_existing = True
    t.save = False
    tasks.append(t)
tools.import_asset_tasks(tasks)
for t in tasks:
    wave = library.load_asset(FOLDER + "/" + t.destination_name)
    if not wave:
        done.append("FALHOU " + t.destination_name)
        continue
    wave.set_editor_property("looping", False)
    if effects:
        wave.set_editor_property("sound_class_object", effects)
    library.save_loaded_asset(wave)
    done.append(t.destination_name)
with open(os.path.join(root, "Saved", "Logs", "ImportWeaponSfx.txt"), "w", encoding="utf-8") as out:
    out.write("VORTEX_WEAPON_SFX %d\n%s\n" % (len(done), "\n".join(done)))
