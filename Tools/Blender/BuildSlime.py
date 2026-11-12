"""O corpo do slime novo: Data/Creatures/Slime/SM_Slime.glb (Tools/ImportSlime.py leva para a Unreal).

blender -b --factory-startup -P Tools/Blender/BuildSlime.py -- <raiz do projeto>

Uma gota de gelatina bem redonda e macia (08/11), mais larga embaixo, com o fundo achatado na origem (o chão). Olhando
para +X (a frente na Unreal).
Espaços de material (o nome vira o espaço na Unreal):
- MS_Body: a gelatina (translúcida, deformada pelo material: amassar, balançar, esticar até o inimigo);
- MS_Core: o miolo que aparece dentro.
Cada vértice guarda na cor (R) o quanto ele acompanha a esticada do ataque; B = 1 na frente da gelatina, onde o material
desenha o rosto. UV: a frente vista de reto (U de lado, V de 0 no chão a 1 no alto), para o rosto amassar junto com a gota.
"""
import math
import os
import sys

import bmesh
import bpy
from mathutils import Vector

ROOT = sys.argv[-1] if len(sys.argv) > 1 and os.path.isdir(sys.argv[-1]) else os.getcwd()
OUT = os.path.join(ROOT, "Data", "Creatures", "Slime")
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
WIDTH, HEIGHT = .92, .68


def material(name):
    return bpy.data.materials.get(name) or bpy.data.materials.new(name)


def finish(obj, mat, follow):
    obj.data.materials.append(material(mat))
    layer = obj.data.color_attributes.new("Col", "BYTE_COLOR", "CORNER")
    for d in layer.data:
        d.color = (follow, 0, 0, 1)
    for p in obj.data.polygons:
        p.use_smooth = True
    return obj


# gelatina: cubo bem subdividido, puxado para a esfera (cantos arredondados), mais largo embaixo e achatado no chão
bpy.ops.mesh.primitive_cube_add(size=1)
body = bpy.context.active_object
body.name = "Slime"
mod = body.modifiers.new("sub", "SUBSURF")
mod.levels = 4
mod.render_levels = 4
bpy.ops.object.modifier_apply(modifier=mod.name)
mesh = body.data
for v in mesh.vertices:
    p = v.co.copy()
    sphere = p.normalized() * .5
    p = p.lerp(sphere, .94)                       # quase esfera
    t = (p.z + .5)                                # 0 embaixo, 1 em cima
    p.x *= WIDTH * (1.14 - .3 * t)                # mais larga embaixo
    p.y *= WIDTH * (1.14 - .3 * t)
    p.z = max(p.z, -.43) * HEIGHT / .93
    v.co = p
lowest = min(v.co.z for v in mesh.vertices)
for v in mesh.vertices:
    v.co.z -= lowest
finish(body, "MS_Body", 1.0)
top = max(v.co.z for v in mesh.vertices)
uv = mesh.uv_layers.active   # o cubo já vem com uma camada: é a primeira que a Unreal lê
front = mesh.color_attributes["Col"]
for loop in mesh.loops:
    co = mesh.vertices[loop.vertex_index].co
    uv.data[loop.index].uv = (co.y / WIDTH + .5, co.z / top)
    if co.x > 0:
        front.data[loop.index].color = (1, 0, 1, 1)

# miolo: uma gota menor lá dentro, mais para baixo
bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3, radius=.13, location=(0, 0, HEIGHT * .3))
core = bpy.context.active_object
core.scale = (1.0, 1.0, .85)
bpy.ops.object.transform_apply(scale=True)
finish(core, "MS_Core", .5)


parts = [body, core]
bpy.ops.object.select_all(action="DESELECT")
for o in parts:
    o.select_set(True)
bpy.context.view_layer.objects.active = body
bpy.ops.object.join()
slime = bpy.context.active_object
slime.name = "SM_Slime"
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "SM_Slime.glb"), use_selection=True, export_format="GLB",
                          export_vertex_color="ACTIVE", export_all_vertex_colors=True)
tris = sum(len(p.vertices) - 2 for p in slime.data.polygons)
print("BUILD_SLIME", tris, "tris", [round(x, 3) for x in slime.dimensions])
