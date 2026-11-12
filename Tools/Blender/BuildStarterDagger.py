"""A adaga inicial do herói (Data/fantasy dagger 3d model.glb) no lugar da Espada de Cinzas.

blender --background --factory-startup --python Tools/Blender/BuildStarterDagger.py -- [comprimento] [triângulos]

O modelo vem do Tripo deitado na diagonal, em centímetros e com ~98 mil triângulos. Aqui ele vai para o
mesmo referencial das armas de BuildVortexWeapons.py (pegada na origem, lâmina para +Z, guarda no eixo X,
espessura no Y), com o comprimento da Espada de Cinzas, e recebe o referencial da mão da espada
(frames.json, "espada_de_cinzas") antes de ir para Data/VortexWeapons/StarterDagger/W_adaga_inicial.glb (fora de Models,
que o ImportVortexWeapons.py reimporta inteira).
Depois: Tools/ImportStarterDagger.py no editor fechado.
"""
import json
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix

ROOT = os.environ.get("VORTEX_ROOT", os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))
SOURCE = os.path.join(ROOT, "Data", "fantasy dagger 3d model.glb")
OUT = os.path.join(ROOT, "Data", "VortexWeapons", "StarterDagger", "W_adaga_inicial.glb")
args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
LENGTH = float(args[0]) if args else 2.3          # do pomo à ponta, em unidades do item (a espada tem 2,42)
TRIANGLES = int(args[1]) if len(args) > 1 else 12000
GRIP_RADIUS = .075                                # raio da pegada no item (a espada tem .065 com as tiras)

scene = bpy.context.scene
for o in list(scene.objects):
    bpy.data.objects.remove(o, do_unlink=True)
bpy.ops.import_scene.gltf(filepath=SOURCE)
obj = next(o for o in scene.objects if o.type == "MESH")
obj.data.transform(obj.matrix_world)
obj.parent = None
obj.matrix_world = Matrix.Identity(4)

# eixos pela forma: o maior é o comprimento, o segundo a largura da guarda
p = np.array([v.co[:] for v in obj.data.vertices])
center = p.mean(0)
_, _, axes = np.linalg.svd(p - center, full_matrices=False)
z = axes[0]
along = (p - center) @ z
# a ponta é o lado mais fino: compara a largura nos 10% de cada ponta
lo, hi = along.min(), along.max()
x_axis = axes[1]
width = lambda m: np.ptp((p[m] - center) @ x_axis) if m.any() else 0
if width(along > hi - .1 * (hi - lo)) > width(along < lo + .1 * (hi - lo)):
    z = -z
    along = -along
    lo, hi = along.min(), along.max()
y_axis = np.cross(z, x_axis)
x_axis = np.cross(y_axis, z)

# guarda: a primeira fatia (do pomo para a ponta) bem mais larga que a lâmina; a pegada logo abaixo
bins = np.linspace(lo, hi, 81)
widths = []
for a, b in zip(bins[:-1], bins[1:]):
    m = (along >= a) & (along < b)
    widths.append(width(m))
widths = np.array(widths)
blade = np.median(widths[50:70])
guard_i = next(i for i, w in enumerate(widths) if w > blade * 1.6)
# a pegada: a parte mais fina entre o pomo e a guarda (até 15% mais larga que o ponto mais fino)
peak = int(np.argmax(widths[:guard_i // 2 + 1]))                  # o pomo
thin = peak + int(np.argmin(widths[peak:guard_i]))
a = b = thin
while a > peak + 1 and widths[a - 1] < widths[thin] * 1.15:
    a -= 1
while b < guard_i - 1 and widths[b + 1] < widths[thin] * 1.15:
    b += 1
print("DAGGER_WIDTHS", " ".join("%.0f" % w for w in widths[:guard_i + 3]))
grip_lo, grip_hi = bins[a], bins[b + 1]
grip_center = (grip_lo + grip_hi) / 2
print("DAGGER_SHAPE length=%.1f blade=%.1f guard=%.1f grip=%.1f..%.1f" % (hi - lo, blade, widths[guard_i], grip_lo, grip_hi))

# referencial do item: origem no meio da pegada, comprimento para +Z, escala pelo comprimento
scale = LENGTH / (hi - lo)
origin = center + z * grip_center
basis = Matrix((x_axis.tolist(), y_axis.tolist(), z.tolist())).to_4x4()
item = Matrix.Diagonal((scale, scale, scale, 1)) @ basis @ Matrix.Translation((-origin).tolist())
obj.data.transform(item)

# a pegada do Tripo é grossa para a mão do herói: afina o cabo e o pomo em volta do eixo
top = (grip_hi - grip_center) * scale
bottom = (grip_lo - grip_center) * scale
radius = lambda v: (v.co.x ** 2 + v.co.y ** 2) ** .5
grip_r = max(radius(v) for v in obj.data.vertices if bottom <= v.co.z < top)
pommel_r = max(radius(v) for v in obj.data.vertices if v.co.z < bottom)
for v in obj.data.vertices:
    if v.co.z < top:
        k = min(1.0, GRIP_RADIUS / grip_r) if v.co.z >= bottom else min(1.0, GRIP_RADIUS * 1.35 / pommel_r)
        v.co.x *= k
        v.co.y *= k
print("DAGGER_GRIP radius %.3f -> %.3f, pommel %.3f" % (grip_r, min(grip_r, GRIP_RADIUS), pommel_r))

# menos triângulos (uma arma na mão não precisa de 98 mil)
tris = sum(len(f.vertices) - 2 for f in obj.data.polygons)
if tris > TRIANGLES:
    mod = obj.modifiers.new("Decimate", "DECIMATE")
    mod.ratio = TRIANGLES / tris
    with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj]):
        bpy.ops.object.modifier_apply(modifier=mod.name)
for poly in obj.data.polygons:
    poly.use_smooth = True

# referencial da mão da Espada de Cinzas (glTF -> Blender, como em vortex_armory.Weapon.export)
frames = json.load(open(os.path.join(ROOT, "Data", "VortexWeapons", "frames.json"), encoding="utf-8"))["weapons"]
gl_to_bl = Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))
mb = gl_to_bl @ Matrix(frames["espada_de_cinzas"]["right"]) @ gl_to_bl.inverted()
obj.data.transform(mb)
if mb.determinant() < 0:
    obj.data.flip_normals()
obj.name = obj.data.name = "W_adaga_inicial"
if obj.data.materials:
    obj.data.materials[0].name = "MW_adaga_inicial"

layer = scene.view_layers[0]
for o in scene.objects:
    o.select_set(o == obj, view_layer=layer)
with bpy.context.temp_override(active_object=obj, selected_objects=[obj]):
    bpy.ops.export_scene.gltf(filepath=OUT, use_selection=True, export_format="GLB", export_apply=True,
                              export_image_format="AUTO", export_materials="EXPORT", export_yup=True)
tris = sum(len(f.vertices) - 2 for f in obj.data.polygons)
print("VORTEX_STARTER_DAGGER %s triangles=%d scale=%.5f" % (OUT, tris, scale))
