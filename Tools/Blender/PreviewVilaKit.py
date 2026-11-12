"""Fotos do kit da vila (conferência sem abrir o Blender): importa os FBX de Data/GodotTown/VilaKit, põe as texturas
nos materiais MV_<nome> e renderiza cada modelo em três quartos.

blender -b --factory-startup -P Tools/Blender/PreviewVilaKit.py -- <raiz> <pasta de saída> [modelo ...]
"""
import math
import os
import sys

import bpy
from mathutils import Vector

ARGS = sys.argv[sys.argv.index("--") + 1:]
ROOT, OUTDIR = ARGS[0], ARGS[1]
ONLY = ARGS[2:]
KIT = os.path.join(ROOT, "Data", "GodotTown", "VilaKit")
TEX = os.path.join(ROOT, "Data", "GodotTown", "VilaTex")
os.makedirs(OUTDIR, exist_ok=True)

for o in list(bpy.data.objects):
    bpy.data.objects.remove(o, do_unlink=True)
scene = bpy.context.scene
for engine in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE", "BLENDER_WORKBENCH"):
    try:
        scene.render.engine = engine
        break
    except TypeError:
        continue
scene.render.resolution_x, scene.render.resolution_y = 900, 640
scene.view_settings.view_transform = "Standard"
world = bpy.data.worlds.new("w")
world.use_nodes = True
world.node_tree.nodes["Background"].inputs[0].default_value = (.55, .68, .85, 1)
world.node_tree.nodes["Background"].inputs[1].default_value = .8
scene.world = world
sun = bpy.data.objects.new("sol", bpy.data.lights.new("sol", "SUN"))
sun.data.energy = 3.5
sun.rotation_euler = (math.radians(50), 0, math.radians(30))
scene.collection.objects.link(sun)
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
scene.collection.objects.link(cam)
scene.camera = cam
ground = bpy.data.meshes.new("chao")
ground.from_pydata([(-60, -60, -.01), (60, -60, -.01), (60, 60, -.01), (-60, 60, -.01)], [], [(0, 1, 2, 3)])
g = bpy.data.objects.new("chao", ground)
gm = bpy.data.materials.new("chao")
gm.use_nodes = True
next(n for n in gm.node_tree.nodes if n.type == "BSDF_PRINCIPLED").inputs["Base Color"].default_value = (.25, .38, .18, 1)
ground.materials.append(gm)
scene.collection.objects.link(g)


def fix_materials(obj):
    for slot in obj.material_slots:
        m = slot.material
        if not m or not m.name.startswith("MV_"):
            continue
        name = m.name.split(".")[0][3:]
        m.use_nodes = True
        nt = m.node_tree
        bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
        tex = nt.nodes.new("ShaderNodeTexImage")
        p = os.path.join(TEX, "T_Vila_%s.png" % name)
        if os.path.exists(p):
            tex.image = bpy.data.images.load(p, check_existing=True)
        vc = nt.nodes.new("ShaderNodeVertexColor")
        mix = nt.nodes.new("ShaderNodeMix")
        mix.data_type, mix.blend_type = "RGBA", "MULTIPLY"
        mix.inputs[0].default_value = 1
        nt.links.new(tex.outputs["Color"], mix.inputs[6])
        nt.links.new(vc.outputs["Color"], mix.inputs[7])
        nt.links.new(mix.outputs[2], bsdf.inputs["Base Color"])
        bsdf.inputs["Roughness"].default_value = .85
        if name == "Brasa":
            nt.links.new(tex.outputs["Color"], bsdf.inputs["Emission Color"])
            bsdf.inputs["Emission Strength"].default_value = 3


files = sorted(f for f in os.listdir(KIT) if f.endswith(".fbx") and (not ONLY or f[:-4] in ONLY))
for f in files:
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=os.path.join(KIT, f))
    new = [o for o in bpy.data.objects if o not in before]
    for o in new:
        fix_materials(o)
    obj = new[0]
    bb = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    lo = Vector((min(v.x for v in bb), min(v.y for v in bb), min(v.z for v in bb)))
    hi = Vector((max(v.x for v in bb), max(v.y for v in bb), max(v.z for v in bb)))
    c = (lo + hi) / 2
    size = max((hi - lo).length, 3.0)
    # três quartos pela frente (+Y) e um pouco de cima
    eye = c + Vector((.75, 1.15, .7)).normalized() * size * 1.15
    cam.location = eye
    cam.rotation_euler = (c - eye).to_track_quat("-Z", "Y").to_euler()
    scene.render.filepath = os.path.join(OUTDIR, f[:-4] + ".png")
    bpy.ops.render.render(write_still=True)
    for o in new:
        bpy.data.objects.remove(o, do_unlink=True)
print("PREVIEW feito", len(files))
