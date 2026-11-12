"""Salva de novo, já compilados na 5.8, os efeitos (Niagara) que o jogo recompilava toda vez que o mundo abria (01/11):
cada um levava 2 a 6 s e o mundo travava na primeira abertura. Vieram de pacotes feitos numa versão anterior da Unreal
(ArrowTrail, Free_Spells, Vefects...), e o resultado da compilação não ficava guardado neles. Editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ResaveNiagara.py -unattended

Sem lista: todos os NiagaraSystem de /Game. Com Data/Tools/niagara_resave.txt (um caminho por linha): só esses.
Relatório em Saved/Logs/ResaveNiagara.txt.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
lst = os.path.join(root, "Data", "Tools", "niagara_resave.txt")
if os.path.exists(lst):
    paths = [l.strip() for l in open(lst, encoding="utf-8") if l.strip() and not l.startswith("#")]
else:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    paths = [str(d.package_name) for d in registry.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Niagara", "NiagaraSystem"))
             if str(d.package_name).startswith("/Game")]
lines = []
for p in paths:
    path = p.split(".")[0]
    asset = library.load_asset(path)
    if not asset:
        lines.append("FALTA %s" % path)
        continue
    # salvar espera a compilação terminar (UNiagaraSystem::PreSave) e guarda o resultado no próprio arquivo
    ok = library.save_loaded_asset(asset, only_if_is_dirty=False)
    lines.append("%s %s" % ("ok" if ok else "FALHOU", path))
with open(os.path.join(root, "Saved", "Logs", "ResaveNiagara.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
unreal.log_warning("VORTEX_RESAVE_NIAGARA %d sistemas" % len(lines))
