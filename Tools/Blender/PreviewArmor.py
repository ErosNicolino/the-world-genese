"""A foto de conferência das armaduras (07/11): o herói de blocos com as peças vestidas, de frente, de 3/4 e de costas.

blender -b --factory-startup -P Tools/Blender/PreviewArmor.py -- <raiz> <saida.png> <conjunto> [<conjunto> ...]
Cada conjunto vira uma linha da imagem (as peças de Data/Heroes/humano_Mixamo/Armaduras/A_<conjunto>_<peça>.glb, com a
textura delas; a franja pintada num castanho, como seria com o cabelo do herói).
"""
import math
import os
import sys

import bpy
from mathutils import Vector

ARGS = sys.argv[sys.argv.index("--") + 1:]
ROOT, OUT, SETS = ARGS[0], ARGS[1], ARGS[2:]
ARM = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "Armaduras")
HAIR = (0.36, 0.22, 0.12)


def clear():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)


def textured(obj, tex_path, mask_path):
    img = bpy.data.images.load(tex_path, check_existing=True)
    mask = bpy.data.images.load(mask_path, check_existing=True) if os.path.exists(mask_path) else None
    mat = bpy.data.materials.new("prev")
    mat.use_nodes = True
    mat.use_backface_culling = True   # como no jogo: a face virada para dentro some (07/11: os buracos das armaduras)
    nt = mat.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    t = nt.nodes.new("ShaderNodeTexImage")
    t.image = img
    t.interpolation = "Closest"
    color = t.outputs["Color"]
    if mask:
        mask.colorspace_settings.name = "Non-Color"
        m = nt.nodes.new("ShaderNodeTexImage")
        m.image = mask
        m.interpolation = "Closest"
        sep = nt.nodes.new("ShaderNodeSeparateColor")
        nt.links.new(m.outputs["Color"], sep.inputs["Color"])
        tint = nt.nodes.new("ShaderNodeMix")
        tint.data_type = "RGBA"
        tint.blend_type = "MULTIPLY"
        tint.inputs["B"].default_value = (HAIR[0] * 2, HAIR[1] * 2, HAIR[2] * 2, 1)
        nt.links.new(color, tint.inputs["A"])
        nt.links.new(sep.outputs["Green"], tint.inputs["Factor"])
        color = tint.outputs["Result"]
        emit = nt.nodes.new("ShaderNodeMath")
        emit.operation = "MULTIPLY"
        nt.links.new(sep.outputs["Red"], emit.inputs[0])
        emit.inputs[1].default_value = 3.0
        nt.links.new(color, bsdf.inputs["Emission Color"])
        nt.links.new(emit.outputs[0], bsdf.inputs["Emission Strength"])
    nt.links.new(color, bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = .9
    for slot in obj.material_slots:
        slot.material = mat
    if not obj.material_slots:
        obj.data.materials.append(mat)


def body_material(obj):
    tex = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "humano_cor.png")
    if not os.path.exists(tex):
        return
    textured(obj, tex, "")


def shoot(path, angle):
    scene = bpy.context.scene
    cam = scene.camera
    r = 4.2
    cam.location = (math.sin(math.radians(angle)) * r, -math.cos(math.radians(angle)) * r, 1.0)
    cam.rotation_euler = (math.radians(90), 0, math.radians(angle))
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    except TypeError:
        try:
            scene.render.engine = "BLENDER_EEVEE"
        except TypeError:
            pass
    scene.render.resolution_x, scene.render.resolution_y = 360, 480
    scene.render.film_transparent = False
    world = bpy.data.worlds.new("w")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs[0].default_value = (.12, .13, .16, 1)
    world.node_tree.nodes["Background"].inputs[1].default_value = 1.0
    scene.world = world
    shots = []
    tmp = os.path.join(os.path.dirname(OUT), "_prev")
    os.makedirs(tmp, exist_ok=True)
    for key in SETS:
        clear()
        bpy.ops.import_scene.gltf(filepath=os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "Hero_humano.glb"))
        for o in bpy.data.objects:
            if o.animation_data:
                o.animation_data.action = None
        rig = next(o for o in bpy.data.objects if o.type == "ARMATURE")
        for pb in rig.pose.bones:
            pb.matrix_basis.identity()
        for slot in ("head", "chest", "legs", "boots"):
            name = "A_%s_%s" % (key, slot)
            path = os.path.join(ARM, name + ".glb")
            if not os.path.exists(path):
                continue
            before = set(bpy.data.objects)
            bpy.ops.import_scene.gltf(filepath=path)
            for o in set(bpy.data.objects) - before:
                if o.type == "MESH":
                    textured(o, os.path.join(ARM, "T_%s.png" % name), os.path.join(ARM, "T_%s_mask.png" % name))
                if o.type == "ARMATURE":
                    o.hide_render = True
        cam_data = bpy.data.cameras.new("cam")
        cam_data.type = "ORTHO"
        cam_data.ortho_scale = 2.1
        cam = bpy.data.objects.new("cam", cam_data)
        scene.collection.objects.link(cam)
        scene.camera = cam
        for angle, (energy, rot) in ((0, (3, (50, 0, 20))),):
            sun = bpy.data.lights.new("sun", "SUN")
            sun.energy = energy
            so = bpy.data.objects.new("sun", sun)
            so.rotation_euler = tuple(math.radians(a) for a in rot)
            scene.collection.objects.link(so)
        row = []
        for angle in (0, 35, 180):
            p = os.path.join(tmp, "%s_%d.png" % (key, angle))
            shoot(p, angle)
            row.append(p)
        shots.append(row)
    # junta: cada conjunto uma linha
    W, H = 360, 480
    sheet = bpy.data.images.new("prev", W * 3, H * len(shots))
    px = [0.0] * (W * 3 * H * len(shots) * 4)
    for r, row in enumerate(reversed(shots)):
        for c, p in enumerate(row):
            im = bpy.data.images.load(p)
            src = im.pixels[:]
            for y in range(H):
                a = ((r * H + y) * W * 3 + c * W) * 4
                b = (y * W) * 4
                px[a:a + W * 4] = src[b:b + W * 4]
    sheet.pixels = px
    sheet.filepath_raw = OUT
    sheet.file_format = "PNG"
    sheet.save()
    print("PREVIEW", OUT)


main()
