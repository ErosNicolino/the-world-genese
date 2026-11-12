"""Build a disposable visual-step / smooth-collision movement probe level."""

import math
import unreal


MAP_PATH = "/Game/Vortex/Maps/L_MovementProbe"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")

if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    raise RuntimeError(f"Map already exists: {MAP_PATH}")
if not levels.new_level(MAP_PATH, False):
    raise RuntimeError(f"Could not create {MAP_PATH}")


def block(label, location, scale, pitch=0.0, collision=True, hidden=False, tag=None):
    actor = actors.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(*location),
        unreal.Rotator(0, pitch, 0),
    )
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(cube)
    if not collision:
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        actor.set_actor_enable_collision(False)
    if hidden:
        actor.set_actor_hidden_in_game(True)
    if tag:
        actor.set_editor_property("tags", [unreal.Name(tag)])
    return actor


# 1 m cube units. The ramp and visual blocks overlap spatially, but only the ramp collides.
block("FlatGround", (-550, 0, -50), (13, 8, 1), tag="TerrainSlope")
slope_angle = math.degrees(math.atan2(300, 1000))
block(
    "SmoothCollision_TerrainSlope",
    (500, 0, 145),
    (math.hypot(1000, 300) / 100, 8, 0.12),
    pitch=slope_angle,
    hidden=True,
    tag="TerrainSlope",
)
for index in range(10):
    height = (index + 1) * 30
    block(
        f"BlockyVisual_{index:02d}",
        (index * 100 + 50, 0, height / 2),
        (1, 8, height / 100),
        collision=False,
        tag="TerrainSlope",
    )

# Genuine obstacles retain collision. They cannot inherit the slope proxy rule.
block("Cliff_BlocksMovement", (1060, 0, 350), (1, 8, 7), tag="Cliff")
block("Obstacle_BlocksMovement", (-150, 450, 75), (1.5, 1.5, 1.5), tag="Obstacle")

start = actors.spawn_actor_from_class(
    unreal.PlayerStart, unreal.Vector(-750, 0, 140), unreal.Rotator()
)
start.set_actor_label("MovementProbe_PlayerStart")
sun = actors.spawn_actor_from_class(
    unreal.DirectionalLight, unreal.Vector(0, 0, 900), unreal.Rotator(-45, -25, 0)
)
sun.set_actor_label("MovementProbe_Sun")

if not levels.save_current_level():
    raise RuntimeError("Could not save movement probe")
unreal.log(f"VORTEX_MOVEMENT_PROBE_CREATED {MAP_PATH}")
