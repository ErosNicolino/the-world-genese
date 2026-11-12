"""O mapa do Arsenal, o modo de teste (07/11) — editor fechado.
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/MakeArsenalMap.py -unattended -nullrhi

Copia o L_TerrainProto (a mesma luz, céu, neblina e pós-processo) para L_Arsenal, tira o terreno e os bichos, e põe o
AVortexArsenal no centro (ele monta o chão, os pedestais, os manequins e os bonecos) com o PlayerStart na frente da
placa de ajuda.
"""
import unreal

SRC, DST = "/Game/Vortex/Maps/L_TerrainProto", "/Game/Vortex/Maps/L_Arsenal"
library = unreal.EditorAssetLibrary
if library.does_asset_exist(DST):
    library.delete_asset(DST)
assert library.duplicate_asset(SRC, DST), "não copiou o mapa"
unreal.EditorLoadingAndSavingUtils.load_map(DST)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in actors.get_all_level_actors():
    name = a.get_class().get_name()
    if name in ("VortexTerrainStreamer", "VortexWolf") or name.startswith("Vortex") and name != "VortexArsenal":
        actors.destroy_actor(a)
    elif name == "PlayerStart":
        a.set_actor_location(unreal.Vector(0, 0, 120), False, False)
        a.set_actor_rotation(unreal.Rotator(0, 0, 0), False)
arsenal_class = unreal.load_class(None, "/Script/Vortex.VortexArsenal")
actors.spawn_actor_from_class(arsenal_class, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
unreal.EditorLoadingAndSavingUtils.save_current_level()
left = sorted(a.get_class().get_name() for a in actors.get_all_level_actors())
unreal.log_warning("VORTEX_ARSENAL_MAP ok %s" % left)
