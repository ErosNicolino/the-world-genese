"""Fase 15: o sol do L_TerrainProto precisa girar durante o jogo (dia e noite).

Troca Terrain_Sun de Stationary para Movable. As cores, a intensidade e a direção ficam a cargo
de UVortexDayNightSubsystem em tempo de jogo; o mapa continua com a luz de dia para o editor.
"""
import unreal

world = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Vortex/Maps/L_TerrainProto")
if not world:
    raise RuntimeError("Could not load L_TerrainProto")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sun = next((a for a in actors.get_all_level_actors() if a.get_actor_label() == "Terrain_Sun"), None)
if not sun:
    raise RuntimeError("Terrain_Sun is missing")
light = sun.get_component_by_class(unreal.DirectionalLightComponent)
light.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sun.get_editor_property("root_component").set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level():
    raise RuntimeError("Could not save the movable sun")
unreal.log("VORTEX_DAYNIGHT_MAP sun=movable")
