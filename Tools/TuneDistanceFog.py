"""Give the prototype valley a soft distance veil inspired by FogArea.

The reference project's FogArea assets are inspected as visual reference only;
this map uses Unreal's native Exponential Height Fog without copying the pack.

The terrain is only built up to ~110-126 m around the players, so the veil must
be closed there: clear up to 45 m, dense past ~100 m and fully opaque at the
edge. The height falloff thins the fog upward so the sky keeps some blue.
Values were chosen with Tools/RunFogShot.ps1 (three long views, with and
without fog).
"""
import unreal

DENSITY = 0.32
FALLOFF = 0.35
START = 4500.0
OPACITY = 1.0

world = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Vortex/Maps/L_TerrainProto")
if not world:
    raise RuntimeError("Could not load L_TerrainProto")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
fog = next((a for a in actors.get_all_level_actors() if a.get_actor_label() == "Terrain_Fog"), None)
if not fog:
    raise RuntimeError("Terrain_Fog is missing")
component = fog.get_editor_property("component")
component.set_editor_property("fog_density", DENSITY)
component.set_editor_property("fog_height_falloff", FALLOFF)
component.set_editor_property("start_distance", START)
component.set_editor_property("fog_max_opacity", OPACITY)
component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.40, 0.48, 0.58, 1.0))
if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level():
    raise RuntimeError("Could not save fog")
unreal.log(f"VORTEX_DISTANCE_FOG density={DENSITY} start={START} falloff={FALLOFF} opacity={OPACITY}")
