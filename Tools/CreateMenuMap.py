"""Create the lightweight front-end world. Never overwrites an existing map."""

import unreal

MAP_PATH = "/Game/Vortex/Maps/L_Menu"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    raise RuntimeError(f"Map already exists: {MAP_PATH}")
if not levels.new_level(MAP_PATH, False):
    raise RuntimeError(f"Could not create: {MAP_PATH}")

settings = unreal.EditorLevelLibrary.get_editor_world().get_world_settings()
menu_mode = unreal.load_class(None, "/Script/Vortex.VortexMenuGameMode")
if not menu_mode:
    raise RuntimeError("VortexMenuGameMode C++ class is unavailable")
settings.set_editor_property("default_game_mode", menu_mode)
if not levels.save_current_level():
    raise RuntimeError("Could not save menu map")
unreal.log(f"VORTEX_MENU_MAP_CREATED {MAP_PATH}")
