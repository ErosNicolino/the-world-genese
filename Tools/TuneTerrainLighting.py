"""Set a restrained daylight grade on the terrain prototype, keeping it editable."""
import unreal

path = "/Game/Vortex/Maps/L_TerrainProto"
world = unreal.EditorLoadingAndSavingUtils.load_map(path)
if not world:
    raise RuntimeError("Could not load terrain map")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = actors.get_all_level_actors()

def labelled(name):
    return next((actor for actor in all_actors if actor.get_actor_label() == name), None)

sun = labelled("Terrain_Sun")
fog = labelled("Terrain_Fog")
sky_light = labelled("Terrain_SkyLight")
if not sun or not fog or not sky_light:
    raise RuntimeError("Terrain daylight actors missing")

sun_light = sun.get_editor_property("light_component")
sun_light.set_editor_property("intensity", 6.0)
sun_light.set_editor_property("use_temperature", True)
sun_light.set_editor_property("temperature", 5800.0)
sky_light.get_editor_property("light_component").set_editor_property("intensity", 1.15)

fog_component = fog.get_editor_property("component")
fog_component.set_editor_property("fog_density", 0.016)
fog_component.set_editor_property("fog_height_falloff", 0.14)

grade = labelled("Terrain_DaylightGrade")
if not grade:
    grade = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    grade.set_actor_label("Terrain_DaylightGrade")
grade.set_editor_property("unbound", True)
settings = grade.get_editor_property("settings")
settings.set_editor_property("override_auto_exposure_bias", True)
settings.set_editor_property("auto_exposure_bias", -0.38)
settings.set_editor_property("override_auto_exposure_speed_up", True)
settings.set_editor_property("auto_exposure_speed_up", 2.0)
settings.set_editor_property("override_auto_exposure_speed_down", True)
settings.set_editor_property("auto_exposure_speed_down", 1.0)
grade.set_editor_property("settings", settings)

if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level():
    raise RuntimeError("Could not save terrain lighting")
unreal.log("VORTEX_TERRAIN_LIGHTING sun=6.0 sky=1.15 temperature=5800 bias=-0.38 fog=0.016")
