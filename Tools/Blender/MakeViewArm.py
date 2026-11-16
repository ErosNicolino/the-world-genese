"""O braço de bloco da primeira pessoa com arco (15/11): uma caixa de 1 m ao longo do X, com o UV num pedaço liso de pele
da textura do corpo do herói (humano_cor.png, 48 x 82: x 1..15, y 36..42, igual no masculino e no feminino). No jogo ela
recebe o material do corpo do herói, então sai na pele da raça. O código estica (UVortexWeaponParts::TickBowString).

blender -b --factory-startup -P Tools/Blender/MakeViewArm.py   ->   Data/VortexWeapons/Models/W_braco_1p.glb
"""
import os

import bmesh
import bpy

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o, do_unlink=True)
mesh = bpy.data.meshes.new("W_braco_1p")
bm = bmesh.new()
bmesh.ops.create_cube(bm, size=1.0)
uv = bm.loops.layers.uv.new("UVMap")
U0, U1, V0, V1 = 1 / 48, 15 / 48, 1 - 42 / 82, 1 - 36 / 82      # o Blender conta o V de baixo para cima
for face in bm.faces:
    for loop, (u, v) in zip(face.loops, ((U0, V0), (U1, V0), (U1, V1), (U0, V1))):
        loop[uv].uv = (u, v)
bm.to_mesh(mesh)
bm.free()
obj = bpy.data.objects.new("W_braco_1p", mesh)
bpy.context.scene.collection.objects.link(obj)
mat = bpy.data.materials.new("M_braco_1p")
mesh.materials.append(mat)
out = os.path.join(ROOT, "Data", "VortexWeapons", "Models", "W_braco_1p.glb")
bpy.ops.export_scene.gltf(filepath=out, export_format="GLB")
print("ok", out)
