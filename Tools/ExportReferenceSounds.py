"""Exporta para WAV os sons dos pacotes do VortexReferencia (rode NO PROJETO DE REFERÊNCIA, editor fechado).

UnrealEditor-Cmd.exe <VortexReferencia.uproject> -run=pythonscript -script=<este arquivo> -unattended

Saída: <Vortex>/Data/WeaponSfx/ref/<pacote>__<nome>.wav (magia modular em mono, Free_Sounds_Pack e
SmallSoundKit/Magic). Os sons de cada arma saem daqui (Tools/BuildWeaponSfx.py).
"""
import os
import unreal

OUT = r"C:\Users\Valorant Niko\Documents\ChatGPT\Mangas\Vortex-UE5-migracao\Vortex\Data\WeaponSfx\ref"
FOLDERS = ["/Game/FreeModularMagicSFX", "/Game/Free_Sounds_Pack/wav", "/Game/SmallSoundKit/SSKWav/MagicWav"]
os.makedirs(OUT, exist_ok=True)
lib = unreal.EditorAssetLibrary
done, failed = 0, 0
for folder in FOLDERS:
    for path in lib.list_assets(folder, recursive=True):
        data = lib.find_asset_data(path)
        if str(data.asset_class_path.asset_name) != "SoundWave" or "/Stereo/" in path:
            continue
        name = path.split("/")[-1].split(".")[0]
        task = unreal.AssetExportTask()
        task.object = lib.load_asset(path)
        task.filename = os.path.join(OUT, "%s__%s.wav" % (folder.split("/")[2], name))
        task.automated = True
        task.prompt = False
        task.replace_identical = True
        task.exporter = unreal.SoundExporterWAV()
        if unreal.Exporter.run_asset_export_task(task):
            done += 1
        else:
            failed += 1
with open(os.path.join(OUT, "_export.txt"), "w") as f:
    f.write("exportados %d falhas %d\n" % (done, failed))
