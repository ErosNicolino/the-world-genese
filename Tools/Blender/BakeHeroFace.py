"""A frente da cabeça do herói do Mixamo numa imagem (as cores exatas da textura, sem luz), para desenhar os olhos e as
bocas do rosto animado (Tools/MakeHeroFaces.py) — sem janela.

blender -b --factory-startup -P Tools/Blender/BakeHeroFace.py -- <raiz do projeto> <raça>

Sai Data/Heroes/<raça>_Mixamo/Face/head.png (512 px, transparente fora da cabeça) e face.json: o retângulo da
projeção (no espaço do herói de pé, em m) e a posição de alguns ossos, para a Unreal achar o mesmo retângulo.
"""
import json
import os
import sys

import bpy
from mathutils import Vector

ROOT, RACE = sys.argv[sys.argv.index("--") + 1:][:2]
# --model <glb>: a foto sai de outro arquivo (o modelo com os buracos, antes do --flatface)
MODEL = sys.argv[sys.argv.index("--model") + 1] if "--model" in sys.argv else None
OUT = os.path.join(ROOT, "Data", "Heroes", RACE + "_Mixamo", "Face")
# a foto da cabeça é a do modelo com os buracos (os desenhos saem dela); depois do MixamoToHero.py --flatface o rosto
# fica liso e a foto sairia esticada: só refaz com --force
if os.path.exists(os.path.join(OUT, "head.png")) and "--force" not in sys.argv:
    print("FACE já existe head.png (use --force para refazer)")
    sys.exit(0)
SIZE = 512
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=MODEL or os.path.join(ROOT, "Data", "Heroes", RACE + "_Mixamo", "Hero_%s.glb" % RACE))
arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
body = max((o for o in bpy.data.objects if o.type == "MESH"), key=lambda o: len(o.data.vertices))
if arm.animation_data:
    arm.animation_data.action = None
    for t in arm.animation_data.nla_tracks:
        t.mute = True
for pb in arm.pose.bones:
    pb.matrix_basis.identity()
bpy.context.view_layer.update()
bone = lambda n: arm.matrix_world @ arm.data.bones[n].head_local
head = bone("mixamorig:Head")
# a frente do rosto: as faces viradas para a frente (-Y) mais à frente, acima do pescoço
mw, m3 = body.matrix_world, body.matrix_world.to_3x3()
front = []
for p in body.data.polygons:
    c, n = mw @ p.center, (m3 @ p.normal).normalized()
    if c.z > head.z and n.y < -.8:
        front.append(c)
# a cabeça inteira de frente (Tools/MakeHeroFaces.py acha os olhos pela cor e recorta o rosto)
y0 = min(c.y for c in front)
top = max((mw @ v.co).z for v in body.data.vertices)
cx = sum(c.x for c in front) / len(front)
half = (top - head.z) * .5 + .05
cz = (top + head.z) * .5 - .04
rect = {"center": [cx, y0, cz], "half": half, "depth": .13}
# --keeprect: o mesmo enquadramento de antes (o rig mudou, mas as medidas do Face/ajuste.json continuam valendo)
if "--keeprect" in sys.argv and os.path.exists(os.path.join(OUT, "face.json")):
    with open(os.path.join(OUT, "face.json"), encoding="utf-8") as f:
        rect = json.load(f)["rect"]
    cx, y0, cz = rect["center"]
    half = rect["half"]
# câmera de frente, ortográfica, só a cor da textura (sem luz)
scene = bpy.context.scene
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "FLAT"
scene.display.shading.color_type = "TEXTURE"
scene.render.film_transparent = True
# as cores da textura como são: o filtro padrão (AgX) desbotava e acinzentava o rosto desenhado
scene.view_settings.view_transform = "Standard"
scene.view_settings.look = "None"
scene.view_settings.exposure = 0
scene.view_settings.gamma = 1
scene.render.resolution_x = scene.render.resolution_y = SIZE
scene.render.image_settings.color_mode = "RGBA"
for img in bpy.data.images:
    img.colorspace_settings.name = img.colorspace_settings.name
for m in body.data.materials:
    for n in (m.node_tree.nodes if m and m.use_nodes else []):
        if n.type == "TEX_IMAGE":
            n.interpolation = "Closest"
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
scene.collection.objects.link(cam)
scene.camera = cam
cam.data.type = "ORTHO"
cam.data.ortho_scale = half * 2
cam.data.clip_start = .001
cam.data.clip_end = 2
cam.location = Vector((cx, y0 - .5, cz))
cam.rotation_euler = (1.5707963, 0, 0)
scene.render.filepath = os.path.join(OUT, "head.png")
bpy.ops.render.render(write_still=True)
bones = {n: list(bone("mixamorig:" + n)) for n in ("Hips", "Head", "LeftHand", "RightHand", "LeftFoot", "LeftToeBase")}
json.dump({"rect": rect, "bones": bones, "size": SIZE}, open(os.path.join(OUT, "face.json"), "w"), indent=1)
print("FACE", RACE, rect)
