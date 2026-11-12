"""Repair the saved probe after verifying collision/rotation on reload."""

import math
import unreal


levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level("/Game/Vortex/Maps/L_MovementProbe")
visual_count = 0
proxy_count = 0
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if label.startswith("BlockyVisual_"):
        actor.set_actor_enable_collision(False)
        visual_count += 1
    elif label == "SmoothCollision_TerrainSlope":
        angle = math.degrees(math.atan2(300, 1000))
        actor.set_actor_rotation(unreal.Rotator(0, angle, 0), False)
        proxy_count += 1

if visual_count != 10 or proxy_count != 1:
    raise RuntimeError(f"Unexpected actors: visual={visual_count}, proxy={proxy_count}")
if not levels.save_current_level():
    raise RuntimeError("Could not save repaired probe")
unreal.log("VORTEX_MOVEMENT_PROBE_REPAIRED")
