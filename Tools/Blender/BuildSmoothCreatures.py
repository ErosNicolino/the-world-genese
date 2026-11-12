"""Aranha, morcego e cogumelo com os modelos lisos novos (Docs/05_Referencias, feitos no Tripo), nos esqueletos
e animações que esses bichos já têm no jogo (Data/GodotMonsters/<Espécie>/<Espécie>_Skinned.glb).

blender -b -P Tools/Blender/BuildSmoothCreatures.py            (depois: Tools/ImportSmoothCreatures.py, editor fechado)

Para cada espécie:
- abre o bicho atual (esqueleto + clipes) e tira a malha de blocos;
- abre o modelo novo; na aranha, joga fora os ossos do Tripo (entortavam a malha toda) e usa a malha parada;
- gira para olhar como o bicho atual (aranha: +Y do Blender; morcego e cogumelo: -Y) e põe no mesmo tamanho e
  no mesmo chão da malha antiga;
- reduz os triângulos (60 a 70 mil viram ~12 a 18 mil; são muitos bichos na tela);
- liga a malha aos ossos (pesos automáticos do Blender; o que ficar sem osso vai para o osso mais perto);
- exporta Data/SmoothCreatures/<Espécie>/<Espécie>_Skinned.glb com a textura do modelo e os mesmos clipes
  (Idle, Walk, Attack...), com os mesmos nomes.
Fotos de conferência em Data/SmoothCreatures/<Espécie>/_pose_<clipe>.png.
"""
import math
import os
import sys

import bpy
from mathutils import Matrix, Vector

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REFS = os.path.join(ROOT, "Docs", "05_Referencias")
OLD = os.path.join(ROOT, "Data", "GodotMonsters")
OUT = os.path.join(ROOT, "Data", "SmoothCreatures")

# fonte, giro para olhar como o bicho atual, como medir o tamanho e quantos triângulos no fim
SPECIES = {
    # a aranha do Tripo vem deitada: cara para +Z e costas para +X. Gira para cara em +Y e costas em +Z.
    "Spider": {"file": "cartoon spider 3d model.glb", "rotate": Matrix(((0, 1, 0), (0, 0, 1), (1, 0, 0))).to_4x4(), "fit": "width", "tris": 18000, "skip": []},
    "Bat": {"file": "flying+bat+3d+model.glb", "rotate": Matrix.Identity(4), "fit": "width", "tris": 14000, "skip": ["Gem", "Crystal"]},
    "Mushroom": {"file": "mushroom+character+3d+model.glb", "rotate": Matrix.Identity(4), "fit": "height", "tris": 14000, "skip": ["Eyes"]},
}


def bounds(objects):
    lo = Vector((1e9,) * 3)
    hi = Vector((-1e9,) * 3)
    for o in objects:
        for v in o.data.vertices:
            w = o.matrix_world @ v.co
            lo = Vector(map(min, lo, w))
            hi = Vector(map(max, hi, w))
    return lo, hi


def skin_weights(mesh, arm, skip, span):
    """Pesos suaves pela distância aos ossos (os 3 mais perto, 1/d^4): as juntas dobram sem rasgar. Os pesos
    automáticos do Blender (calor) falham nessas malhas do Tripo, feitas de várias peças soltas. Ossos de detalhe
    do bicho antigo (olhos, gema, cristal) ficam de fora: na malha nova não há peça separada para eles."""
    segments = [(b.name, arm.matrix_world @ b.head_local, arm.matrix_world @ b.tail_local) for b in arm.data.bones
                if b.use_deform and b.name not in skip and b.parent is not None]
    groups = {name: mesh.vertex_groups.new(name=name) for name, _, _ in segments}
    soft = span * .03
    for v in mesh.data.vertices:
        p = mesh.matrix_world @ v.co
        near = []
        for name, h, t in segments:
            d = t - h
            k = max(0.0, min(1.0, (p - h).dot(d) / max(d.length_squared, 1e-9)))
            near.append(((h + d * k - p).length, name))
        near.sort()
        best = near[:3]
        weights = [1.0 / (d + soft) ** 4 for d, _ in best]
        total = sum(weights)
        for (d, name), w in zip(best, weights):
            if w / total > .02:
                groups[name].add([v.index], w / total, "REPLACE")
    return len(segments)


def build(species, info):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    # 1) o bicho atual: esqueleto e clipes
    bpy.ops.import_scene.gltf(filepath=os.path.join(OLD, species, species + "_Skinned.glb"))
    arm = next(o for o in scene.objects if o.type == "ARMATURE")
    old_meshes = [o for o in scene.objects if o.type == "MESH" and len(o.data.polygons) > 200]
    old_lo, old_hi = bounds(old_meshes)
    for o in [o for o in scene.objects if o.type == "MESH"]:
        bpy.data.objects.remove(o, do_unlink=True)
    actions = list(bpy.data.actions)
    if arm.animation_data:
        arm.animation_data.action = None
    for b in arm.pose.bones:
        b.matrix_basis = Matrix.Identity(4)

    # 2) o modelo novo, sem os ossos do Tripo
    before = set(scene.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(REFS, info["file"]))
    added = [o for o in scene.objects if o not in before]
    new_actions = [a for a in bpy.data.actions if a not in actions]
    for a in new_actions:
        bpy.data.actions.remove(a)
    meshes = [o for o in added if o.type == "MESH"]
    for o in meshes:
        for m in list(o.modifiers):
            if m.type == "ARMATURE":
                o.modifiers.remove(m)
        world = o.matrix_world.copy()
        o.parent = None
        o.matrix_world = world
        o.vertex_groups.clear()
    for o in added:
        if o.type != "MESH":
            bpy.data.objects.remove(o, do_unlink=True)
    bpy.ops.object.select_all(action="DESELECT")
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    mesh = bpy.context.view_layer.objects.active
    mesh.name = species + "_Body"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # 3) gira, mede e põe no lugar da malha antiga
    mesh.data.transform(info["rotate"])
    lo, hi = bounds([mesh])
    size, old_size = hi - lo, old_hi - old_lo
    if info["fit"] == "height":
        scale = old_size.z / size.z
    else:
        scale = old_size.x / size.x
    mesh.data.transform(Matrix.Scale(scale, 4))
    lo, hi = bounds([mesh])
    shift = Vector(((old_lo.x + old_hi.x) / 2 - (lo.x + hi.x) / 2, (old_lo.y + old_hi.y) / 2 - (lo.y + hi.y) / 2, old_lo.z - lo.z))
    mesh.data.transform(Matrix.Translation(shift))
    mesh.data.update()

    # 4) menos triângulos (mantém as UVs da textura)
    tris = sum(len(p.vertices) - 2 for p in mesh.data.polygons)
    if tris > info["tris"]:
        dec = mesh.modifiers.new("menos", "DECIMATE")
        dec.ratio = info["tris"] / tris
        bpy.ops.object.modifier_apply(modifier=dec.name)
    tris_after = sum(len(p.vertices) - 2 for p in mesh.data.polygons)

    # 5) pele nos ossos do bicho atual
    bpy.ops.object.select_all(action="DESELECT")
    mesh.select_set(True)
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.parent_set(type="ARMATURE_NAME")
    for g in list(mesh.vertex_groups):
        mesh.vertex_groups.remove(g)
    loose = skin_weights(mesh, arm, info["skip"], max(old_hi - old_lo))

    # 6) exporta com os clipes do bicho atual
    folder = os.path.join(OUT, species)
    os.makedirs(folder, exist_ok=True)
    if not arm.animation_data:
        arm.animation_data_create()
    for a in actions:
        track = arm.animation_data.nla_tracks.new()
        track.name = a.name
        strip = track.strips.new(a.name, int(a.frame_range[0]), a)
        strip.name = a.name
        track.mute = True
    target = os.path.join(folder, species + "_Skinned.glb")
    bpy.ops.export_scene.gltf(filepath=target, export_format="GLB", use_selection=False, export_animations=True,
                              export_animation_mode="NLA_TRACKS", export_skins=True, export_apply=False,
                              export_yup=True, export_image_format="AUTO")
    print("SMOOTH_CREATURE %s escala=%.3f tris=%d->%d ossos_usados=%d clipes=%s -> %s" % (
        species, scale, tris, tris_after, loose, ",".join(a.name for a in actions), target))

    # 7) fotos de conferência: o primeiro quadro e o do meio de cada clipe
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    scene.collection.objects.link(cam)
    scene.camera = cam
    center = (old_lo + old_hi) / 2
    span = max(old_hi - old_lo)
    cam.location = center + Vector((span * 1.1, -span * 1.5, span * .8))
    cam.rotation_euler = (center - cam.location).to_track_quat("-Z", "Y").to_euler()
    sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
    sun.data.energy = 3.5
    sun.rotation_euler = (math.radians(45), 0, math.radians(30))
    scene.collection.objects.link(sun)
    scene.world = bpy.data.worlds.new("w")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs[0].default_value = (.55, .62, .7, 1)
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = scene.render.resolution_y = 320
    for track in arm.animation_data.nla_tracks:
        track.mute = True
    for a in actions:
        arm.animation_data.action = a
        scene.frame_set(int(a.frame_range[0] + (a.frame_range[1] - a.frame_range[0]) * .5))
        scene.render.filepath = os.path.join(folder, "_pose_%s.png" % a.name)
        bpy.ops.render.render(write_still=True)
    arm.animation_data.action = None
    scene.frame_set(0)
    for b in arm.pose.bones:
        b.matrix_basis = Matrix.Identity(4)
    scene.render.filepath = os.path.join(folder, "_pose_parado.png")
    bpy.ops.render.render(write_still=True)


only = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
for name, data in SPECIES.items():
    if not only or name in only:
        build(name, data)
