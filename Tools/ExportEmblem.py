"""Exporta o brasão do menu (/Game/Vortex/UI/Menu/T_Emblem) para Saved/Emblem.png; o Tools/PackageAlpha.ps1
faz dele o ícone do jogo e do instalador (Build/Windows/Vortex.ico). Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi
"""
import os
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
tex = unreal.EditorAssetLibrary.load_asset("/Game/Vortex/UI/Menu/T_Emblem")
task = unreal.AssetExportTask()
task.object = tex
task.filename = os.path.join(ROOT, "Saved", "Emblem.png")
task.automated = True
task.replace_identical = True
task.prompt = False
task.exporter = unreal.TextureExporterPNG()
ok = unreal.Exporter.run_asset_export_task(task)
unreal.log_warning(f"VORTEX_EMBLEM {'ok' if ok else 'FALHOU'} {task.filename}")
