"""Create the small authored network framework test map in the Unreal Editor."""

import unreal


MAP_PATH = "/Game/Vortex/Maps/L_Framework"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    raise RuntimeError(f"Map already exists: {MAP_PATH}")
if not levels.new_level(MAP_PATH, False):
    raise RuntimeError(f"Could not create level: {MAP_PATH}")

floor = actors.spawn_actor_from_class(
    unreal.StaticMeshActor, unreal.Vector(0, 0, -50), unreal.Rotator()
)
floor.set_actor_label("Framework_Floor")
floor.set_actor_scale3d(unreal.Vector(20, 20, 1))
cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
floor.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(cube)

for index, x in enumerate((-250, 250), start=1):
    start = actors.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(x, 0, 160), unreal.Rotator()
    )
    start.set_actor_label(f"Framework_PlayerStart_{index}")

light = actors.spawn_actor_from_class(
    unreal.DirectionalLight, unreal.Vector(0, 0, 700), unreal.Rotator(-45, -25, 0)
)
light.set_actor_label("Framework_Sun")

if not levels.save_current_level():
    raise RuntimeError("Could not save framework level")
unreal.log(f"VORTEX_FRAMEWORK_MAP_CREATED {MAP_PATH}")
