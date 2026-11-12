"""Read the saved movement probe actors and their effective collision settings."""

import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level("/Game/Vortex/Maps/L_MovementProbe")
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if label.startswith(("SmoothCollision", "BlockyVisual", "Cliff", "FlatGround")):
        mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
        unreal.log(
            f"VORTEX_PROBE {label} pos={actor.get_actor_location()} "
            f"rot={actor.get_actor_rotation()} collision={mesh.get_collision_enabled()} "
            f"actorCollision={actor.get_actor_enable_collision()}"
        )
