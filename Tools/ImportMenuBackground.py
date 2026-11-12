"""Import an existing Vortex valley screenshot as front-end background art."""

from pathlib import Path
import unreal

root = Path(__file__).resolve().parents[1]
source = root / "Docs" / "Images" / "fase14_praca.png"
target = "/Game/Vortex/UI/Menu/T_MenuBackground"
if unreal.EditorAssetLibrary.does_asset_exist(target):
    raise RuntimeError(f"Asset already exists: {target}")
if not source.is_file():
    raise RuntimeError(f"Missing source screenshot: {source}")

task = unreal.AssetImportTask()
task.filename = str(source)
task.destination_path = "/Game/Vortex/UI/Menu"
task.destination_name = "T_MenuBackground"
task.automated = True
task.save = True
task.replace_existing = False
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
if not unreal.EditorAssetLibrary.does_asset_exist(target):
    raise RuntimeError(f"Import failed: {target}")
unreal.log(f"VORTEX_MENU_BACKGROUND_IMPORTED {target}")
