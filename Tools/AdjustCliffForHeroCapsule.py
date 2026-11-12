"""Bring the probe wall against the ramp so the 25 cm radius hero stays supported.

Run once with UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<this file>.
"""

import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level("/Game/Vortex/Maps/L_MovementProbe")
cliffs = [a for a in actors.get_all_level_actors() if a.get_actor_label() == "Cliff_BlocksMovement"]
if len(cliffs) != 1:
    raise RuntimeError(f"Expected one cliff, found {len(cliffs)}")
cliffs[0].set_actor_location(unreal.Vector(1060, 0, 350), False, False)
if not levels.save_current_level():
    raise RuntimeError("Could not save movement probe")
unreal.log("VORTEX_HERO_CAPSULE_CLIFF_ADJUSTED x=1060")
