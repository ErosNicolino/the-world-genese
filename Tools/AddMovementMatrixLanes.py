"""Adiciona ao L_MovementProbe as pistas da matriz de movimento da Fase 5.

Cada pista fica numa faixa própria de Y, com chão no topo z=0 antes de x=0:
  y=2000  rampa de 20 graus  (sobe)
  y=2600  rampa de 40 graus  (sobe; o limite é 45)
  y=3200  rampa de 50 graus  (bloqueia)
  y=3800  escada de 8 degraus de 25 cm (sobe)
  y=4400  piscina: cai na água em x=0, nada e sai pela margem em rampa (x 1000..1600)
Rodar uma vez:  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo>
"""

import math
import unreal


MAP_PATH = "/Game/Vortex/Maps/L_MovementProbe"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
levels.load_level(MAP_PATH)

if any(actor.get_actor_label().startswith("Matrix_") for actor in actors.get_all_level_actors()):
    raise RuntimeError("Matrix lanes already exist")


def block(label, location, scale, pitch=0.0, tag="Matrix"):
    actor = actors.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(0, pitch, 0)
    )
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(cube)
    actor.set_editor_property("tags", [unreal.Name(tag)])
    return actor


def ramp(name, y, angle, run=600.0, thickness=12.0):
    rise = run * math.tan(math.radians(angle))
    length = math.hypot(run, rise)
    block(f"Matrix_{name}_Floor", (-550, y, -50), (13, 4, 1))
    block(f"Matrix_{name}_Ramp", (run / 2, y, rise / 2 - thickness / 2), (length / 100, 4, thickness / 100), pitch=angle)
    block(f"Matrix_{name}_Top", (run + 300, y, rise - 50), (6, 4, 1))


ramp("Ramp20", 2000, 20)
ramp("Ramp40", 2600, 40)
ramp("Ramp50", 3200, 50)

# escada maciça: 8 degraus de 25 cm de altura e 50 cm de piso
block("Matrix_Stairs_Floor", (-550, 3800, -50), (13, 4, 1))
for i in range(8):
    height = (i + 1) * 25
    block(f"Matrix_Stairs_Step{i}", (i * 50 + 25, 3800, height / 2), (0.5, 4, height / 100))
block("Matrix_Stairs_Top", (700, 3800, 150), (6, 4, 1))

# piscina: chão até x=0, fundo a -250 até x=1600, margem em rampa de x=1000 (-250) a 1600 (0)
block("Matrix_Pool_Floor", (-600, 4400, -50), (12, 4, 1))
block("Matrix_Pool_Bottom", (800, 4400, -300), (16, 4, 1))
shore = math.degrees(math.atan2(250, 600))
block("Matrix_Pool_Shore", (1300, 4400, -125 - 6), (math.hypot(600, 250) / 100, 4, 0.12), pitch=shore)
block("Matrix_Pool_Exit", (1900, 4400, -50), (6, 4, 1))
water_class = unreal.load_class(None, "/Script/Vortex.VortexWaterZone")
water = actors.spawn_actor_from_class(water_class, unreal.Vector(700, 4400, -150), unreal.Rotator())
water.set_actor_label("Matrix_Pool_Water")
water.get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector(700, 200, 110))

if not levels.save_current_level():
    raise RuntimeError("Could not save matrix lanes")
unreal.log("VORTEX_MATRIX_LANES_CREATED")
