"""Add one physical step-height lane to the existing movement probe map."""

import unreal


levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
levels.load_level("/Game/Vortex/Maps/L_MovementProbe")

if any(actor.get_actor_label() == "StepProbe_Floor" for actor in actors.get_all_level_actors()):
    raise RuntimeError("Step probe lane already exists")


def add_block(label, location, scale):
    actor = actors.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator()
    )
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(cube)
    actor.set_editor_property("tags", [unreal.Name("Stairs")])


# Lane at y=1000 cm: +30 cm is climbable, then +60 cm relative to the ledge must block.
add_block("StepProbe_Floor", (-550, 1000, -50), (13, 4, 1))
add_block("StepProbe_30cm", (150, 1000, 15), (1, 4, 0.3))
add_block("StepProbe_Ledge30cm", (425, 1000, -35), (4.5, 4, 1.3))
add_block("StepProbe_60cm", (700, 1000, 60), (1, 4, 0.6))
add_block("StepProbe_Ledge90cm", (925, 1000, -5), (3.5, 4, 1.9))

if not levels.save_current_level():
    raise RuntimeError("Could not save step probe lane")
unreal.log("VORTEX_STEP_PROBE_LANE_CREATED")
