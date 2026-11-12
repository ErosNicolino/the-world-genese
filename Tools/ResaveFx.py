"""Re-salva os efeitos Niagara dos pacotes com a compilação desta versão da Unreal.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/ResaveFx.py -unattended

Por quê: os efeitos dos pacotes (ArrowTrail, Basic_VFX, FreeParticle_SoftTofu...) foram salvos numa
versão anterior. Sem o resultado da compilação guardado no arquivo, a engine recompilava o sistema
toda vez que ele era carregado ("Compiling System NS_ArrowTrail_Nature took 0.2-0.65 sec" no log),
e cada flecha, magia ou poder travava o jogo um instante antes de sair. Ao salvar no editor, o
Niagara espera a compilação terminar e grava o resultado junto (UNiagaraSystem::PreSave).
Relatório em Saved/Logs/ResaveFx.txt.
"""
import os
import time
import unreal

FOLDERS = ["/Game/ArrowTrail", "/Game/Basic_VFX", "/Game/FreeParticle_SoftTofu", "/Game/Vefects", "/Game/Vortex"]
library = unreal.EditorAssetLibrary
root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
lines = []
total = 0
for folder in FOLDERS:
    if not library.does_directory_exist(folder):
        continue
    for path in library.list_assets(folder, recursive=True):
        data = library.find_asset_data(path)
        kind = str(data.asset_class_path.asset_name)
        if kind not in ("NiagaraSystem", "NiagaraEmitter"):
            continue
        started = time.time()
        asset = library.load_asset(path)
        if not asset:
            lines.append("FALHOU ao carregar " + path)
            continue
        asset.modify()
        ok = library.save_loaded_asset(asset, False)
        total += 1
        lines.append("%s %s %.1fs %s" % ("ok" if ok else "FALHOU", kind, time.time() - started, path))
        unreal.log("VORTEX_RESAVE_FX " + lines[-1])
lines.append("%d efeitos re-salvos" % total)
with open(os.path.join(root, "Saved", "Logs", "ResaveFx.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
