"""Conferência das animações dos poderes (07/11): uma fileira de quadros de cada animação do Poderes_humano.glb.

blender -b --factory-startup -P Tools/Blender/PreviewPowerAnims.py -- <raiz> <saida.png> <animação ...>
"""
import math
import os
import sys

import bpy

ARGS = sys.argv[sys.argv.index("--") + 1:]
ROOT, OUT, NAMES = ARGS[0], ARGS[1], ARGS[2:]
N = 8
W, H = 220, 300

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
bpy.ops.import_scene.gltf(filepath=os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", os.environ.get("VORTEX_GLB", "Poderes_humano.glb")))
hero = next(o for o in bpy.data.objects if o.type == "ARMATURE")
try:
    scene.render.engine = "BLENDER_WORKBENCH"
except TypeError:
    pass
scene.render.resolution_x, scene.render.resolution_y = W, H
scene.display.shading.light = "STUDIO"
scene.display.shading.color_type = "MATERIAL"
cam_data = bpy.data.cameras.new("cam")
cam_data.type = "ORTHO"
cam_data.ortho_scale = 2.6
cam = bpy.data.objects.new("cam", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam
ang = math.radians(30)
cam.location = (math.sin(ang) * 5, -math.cos(ang) * 5, 1.0)
cam.rotation_euler = (math.radians(90), 0, ang)
tmp = os.path.join(os.path.dirname(OUT), "_anim")
os.makedirs(tmp, exist_ok=True)
rows = []
for name in NAMES:
    act = next((a for a in bpy.data.actions if name in a.name), None)
    if not act:
        print("SEM", name)
        continue
    hero.animation_data.action = act
    try:
        hero.animation_data.action_slot = act.slots[0]
    except Exception:
        pass
    f0, f1 = act.frame_range
    row = []
    for k in range(N):
        f = f0 + (f1 - f0) * k / (N - 1)
        scene.frame_set(int(f))
        p = os.path.join(tmp, "%s_%d.png" % (name, k))
        scene.render.filepath = p
        bpy.ops.render.render(write_still=True)
        row.append(p)
    rows.append(row)
sheet = bpy.data.images.new("sheet", W * N, H * len(rows))
px = [0.0] * (W * N * H * len(rows) * 4)
for r, row in enumerate(reversed(rows)):
    for c, p in enumerate(row):
        src = bpy.data.images.load(p).pixels[:]
        for y in range(H):
            a = ((r * H + y) * W * N + c * W) * 4
            px[a:a + W * 4] = src[y * W * 4:(y + 1) * W * 4]
sheet.pixels = px
sheet.filepath_raw = OUT
sheet.file_format = "PNG"
sheet.save()
print("PREVIEW", OUT)
