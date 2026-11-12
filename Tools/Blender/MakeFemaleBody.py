"""O corpo feminino (10/11): copia o Hero_humano.glb e afina a cintura (female_waist.py), no mesmo esqueleto.
blender -b --factory-startup -P Tools/Blender/MakeFemaleBody.py
Sai Data/Heroes/humano_Mixamo/Feminino/Hero_humano.glb (o anterior vai para Feminino/Hero_humano_busto_antigo.glb).
Depois, com o editor fechado: Tools/ImportBlockFemale.py.
"""
import os
import shutil

import bpy

ROOT = r"C:\\Users\\Eros Nicolino\\Documents\\4 - Projetos\\Jogos e 3D\\Vortex-UE5-migracao\\Vortex".replace("\\\\", "\\")
exec(open(os.path.join(ROOT, "Tools", "Blender", "female_waist.py"), encoding="utf-8").read())
SRC = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "Hero_humano.glb")
OUT = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "Feminino", "Hero_humano.glb")
OLD = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "Feminino", "Hero_humano_busto_antigo.glb")
if os.path.exists(OUT) and not os.path.exists(OLD):
    shutil.copy(OUT, OLD)

for o in list(bpy.data.objects):
    bpy.data.objects.remove(o, do_unlink=True)
bpy.ops.import_scene.gltf(filepath=SRC)
rig = next(o for o in bpy.data.objects if o.type == "ARMATURE")
body = next(o for o in bpy.data.objects if o.type == "MESH" and o.find_armature() == rig)
for o in rig.pose.bones:
    o.matrix_basis.identity()
for o in list(bpy.data.objects):
    if o.type == "MESH" and o != body:
        bpy.data.objects.remove(o, do_unlink=True)
W = body.matrix_world
Wi = W.inverted()
moved = 0
# 10/11: só os pontos presos à coluna e ao quadril (os braços ficam fora, mesmo perto do tronco)
CORE = {g.index for g in body.vertex_groups if g.name.split(":")[-1] in ("Hips", "Spine", "Spine1", "Spine2")}
for v in body.data.vertices:
    if not v.groups or max(v.groups, key=lambda g: g.weight).group not in CORE:
        continue
    p = W @ v.co
    before = p.copy()
    shape_point(p, .3)
    if (p - before).length > 1e-6:
        v.co = Wi @ p
        moved += 1
for o in bpy.context.view_layer.objects:
    o.select_set(o in (rig, body))
bpy.context.view_layer.objects.active = rig
bpy.ops.export_scene.gltf(filepath=OUT, use_selection=True, export_format="GLB", export_animations=False, export_skins=True,
                          export_apply=False, export_yup=True, export_def_bones=False)
print("CORPO_FEMININO pontos afinados", moved, "->", OUT)
