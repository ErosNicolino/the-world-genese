"""Place one test wolf near the starting trail of L_TerrainProto (idempotent)."""
import unreal

path = "/Game/Vortex/Maps/L_TerrainProto"
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level.load_level(path):
    raise RuntimeError("Could not load " + path)
wolf = next((a for a in actors.get_all_level_actors() if a.get_actor_label() == "Wolf_Prototype_01"), None)
if not wolf:
    cls = unreal.load_class(None, "/Script/Vortex.VortexWolf")
    wolf = actors.spawn_actor_from_class(cls, unreal.Vector(-41400, -6000, 2500), unreal.Rotator(0, 180, 0))
    wolf.set_actor_label("Wolf_Prototype_01")
else:
    # Six metres from the starting point, on the trail, so it is visible at entry.
    wolf.set_actor_location(unreal.Vector(-41400, -6000, 2500), False, True)
if not level.save_current_level():
    raise RuntimeError("Could not save map with wolf")
unreal.log("VORTEX_WOLF_TERRAIN_PLACED")
