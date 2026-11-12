"""Cria L_TerrainProto: céu, sol, névoa parecida com a do Godot, o streamer do terreno e o
ponto de partida no começo do passeio na mata (-420, -60) m."""

import unreal

MAP_PATH = "/Game/Vortex/Maps/L_TerrainProto"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    raise RuntimeError(f"Map already exists: {MAP_PATH}")
if not levels.new_level(MAP_PATH, False):
    raise RuntimeError(f"Could not create {MAP_PATH}")


def spawn(cls, label, location=(0, 0, 0), rotation=(0, 0, 0)):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*location), unreal.Rotator(*rotation))
    actor.set_actor_label(label)
    return actor


sun = spawn(unreal.DirectionalLight, "Terrain_Sun", (0, 0, 5000), (0, -48, -35))
light = sun.get_component_by_class(unreal.DirectionalLightComponent)
light.set_editor_property("atmosphere_sun_light", True)
light.set_editor_property("intensity", 7.0)
spawn(unreal.SkyAtmosphere, "Terrain_Sky")
sky = spawn(unreal.SkyLight, "Terrain_SkyLight", (0, 0, 3000))
sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
sky_component.set_editor_property("real_time_capture", True)
fog = spawn(unreal.ExponentialHeightFog, "Terrain_Fog", (0, 0, 0))
fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
fog_component.set_editor_property("fog_density", 0.012)
fog_component.set_editor_property("fog_height_falloff", 0.15)
fog_component.set_editor_property("start_distance", 2800.0)
fog_component.set_editor_property("fog_max_opacity", 0.85)
fog_component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.55, 0.65, 0.8, 1.0))
streamer_class = unreal.load_class(None, "/Script/Vortex.VortexTerrainStreamer")
spawn(streamer_class, "Terrain_Streamer")
spawn(unreal.PlayerStart, "Terrain_PlayerStart", (-42000, -6000, 2500))

if not levels.save_current_level():
    raise RuntimeError("Could not save terrain map")
unreal.log(f"VORTEX_TERRAIN_MAP_CREATED {MAP_PATH}")
