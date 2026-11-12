"""Fotos das armas novas (Data/VortexWeapons/Models/W_*.glb) para conferir: Saved/WeaponPreviews/<tipo>.png.

blender --background --factory-startup --python Tools/Blender/RenderWeaponPreviews.py -- [tipo ...]
Cada arma aparece de lado, com luz de estúdio, num fundo cinza-azulado (EEVEE, 512 px).
"""
import math
import os
import sys

import bpy
from mathutils import Vector

ROOT = os.environ.get("VORTEX_ROOT", r"C:\Users\Valorant Niko\Documents\ChatGPT\Mangas\Vortex-UE5-migracao\Vortex")
MODELS = os.path.join(ROOT, "Data", "VortexWeapons", "Models")
OUT = os.path.join(ROOT, "Saved", "WeaponPreviews")


def render(name):
    scene = bpy.context.scene
    for o in list(scene.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.ops.import_scene.gltf(filepath=os.path.join(MODELS, f"W_{name}.glb"))
    meshes = [o for o in scene.objects if o.type == "MESH"]
    lo = Vector((1e9, 1e9, 1e9))
    hi = Vector((-1e9, -1e9, -1e9))
    for o in meshes:
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c)
            lo = Vector(map(min, lo, w))
            hi = Vector(map(max, hi, w))
    center = (lo + hi) / 2
    size = max((hi - lo).length, .1)
    cam_data = bpy.data.cameras.new("cam")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = size * 1.15
    cam = bpy.data.objects.new("cam", cam_data)
    scene.collection.objects.link(cam)
    cam.location = center + Vector((size * 2.0, -size * .4, size * .25))
    cam.rotation_euler = (center - cam.location).to_track_quat("-Z", "Y").to_euler()
    scene.camera = cam
    for loc, energy in (((3, -3, 4), 900), ((-3, 2, 2), 350), ((0, 4, -2), 250)):
        light = bpy.data.objects.new("luz", bpy.data.lights.new("luz", "AREA"))
        light.data.energy = energy * size
        light.data.size = 2.0
        scene.collection.objects.link(light)
        light.location = center + Vector(loc) * size
        light.rotation_euler = (center - light.location).to_track_quat("-Z", "Y").to_euler()
    world = scene.world or bpy.data.worlds.new("mundo")
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs[0].default_value = (.08, .1, .13, 1)
    world.node_tree.nodes["Background"].inputs[1].default_value = .6
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = scene.render.resolution_y = 512
    scene.render.film_transparent = False
    os.makedirs(OUT, exist_ok=True)
    scene.render.filepath = os.path.join(OUT, f"{name}.png")
    bpy.ops.render.render(write_still=True)


names = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
if not names:
    names = sorted(f[2:-4] for f in os.listdir(MODELS) if f.startswith("W_") and f.endswith(".glb") and not f.endswith("_L.glb"))
for n in names:
    render(n)
print("VORTEX_PREVIEWS", len(names))
