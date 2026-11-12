"""Os heróis do Docs prontos para o Mixamo: Data/Heroes/Mixamo/<raça>.fbx (só a malha, em A-pose, sem o esqueleto do
site, a textura de cor em 2048 embutida no FBX).

blender -b --factory-startup -P Tools/Blender/HeroesForMixamo.py -- <raiz do projeto>

O elfo sai sem a parede (a mesma limpeza do RigHero.py --walls). No Mixamo: Upload Character, o FBX, marcar queixo,
pulsos, cotovelos, joelhos e virilha, Skeleton LOD "Standard Skeleton (65)"; depois baixar as animações em FBX
"Without Skin" (e o personagem uma vez "With Skin") para Data/Heroes/Mixamo/<raça>/.
"""
import os
import sys

import bmesh
import bpy

ROOT = sys.argv[-1]
OUT = os.path.join(ROOT, "Data", "Heroes", "Mixamo")
HEROES = {"humano": "personagem principal", "anao": "anão", "elfo": "elfo", "barbaro": "Barbaro gigante", "diabo": "Demonio",
          "morto_vivo": "esqueleto"}
os.makedirs(OUT, exist_ok=True)
for race, name in HEROES.items():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=os.path.join(ROOT, "Docs", "Modelos_Meshy", name + ".glb"))
    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    body = max(meshes, key=lambda o: len(o.data.vertices))
    # tira o esqueleto do site (o Mixamo faz o dele), mantendo a forma em A
    for o in list(bpy.data.objects):
        if o != body:
            bpy.data.objects.remove(o, do_unlink=True)
    body.modifiers.clear()
    body.vertex_groups.clear()
    body.parent = None
    bpy.context.view_layer.objects.active = body
    body.select_set(True)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    if race == "elfo":
        # a parede: peças largas ou fundas demais para o corpo (na cópia soldada, como no RigHero.py)
        weld = bmesh.new()
        weld.from_mesh(body.data)
        bmesh.ops.remove_doubles(weld, verts=weld.verts, dist=1e-4)
        weld.faces.ensure_lookup_table()
        zs = [v.co.z for v in weld.verts]
        tall = max(zs) - min(zs)
        seen, wall = set(), set()
        for f in weld.faces:
            if f.index in seen:
                continue
            stack, piece = [f], []
            seen.add(f.index)
            while stack:
                x = stack.pop()
                piece.append(x)
                for e in x.edges:
                    for o in e.link_faces:
                        if o.index not in seen:
                            seen.add(o.index)
                            stack.append(o)
            vs = {v for face in piece for v in face.verts}
            xs = [v.co.x for v in vs]
            ys = [v.co.y for v in vs]
            if max(xs) - min(xs) > .75 * tall or max(ys) - min(ys) > .45 * tall:
                for face in piece:
                    c = face.calc_center_median()
                    wall.add((round(c.x, 4), round(c.y, 4), round(c.z, 4)))
        weld.free()
        bm = bmesh.new()
        bm.from_mesh(body.data)
        gone = [f for f in bm.faces if tuple(round(v, 4) for v in f.calc_center_median()) in wall]
        bmesh.ops.delete(bm, geom=gone, context="FACES")
        bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
        bm.to_mesh(body.data)
        bm.free()
    for mat in body.data.materials:
        if not mat or not mat.use_nodes:
            continue
        for node in list(mat.node_tree.nodes):
            if node.type != "TEX_IMAGE" or not node.image:
                continue
            if any(l.to_socket.name == "Base Color" for l in node.outputs[0].links):
                if max(node.image.size) > 2048:
                    node.image.scale(2048, 2048)
                node.image.filepath_raw = os.path.join(OUT, race + "_cor.png")
                node.image.file_format = "PNG"
                node.image.save()
            else:
                mat.node_tree.nodes.remove(node)
    body.name = race
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, race + ".fbx"), use_selection=True, path_mode="COPY", embed_textures=True,
                             object_types={"MESH"}, apply_scale_options="FBX_SCALE_ALL")
    print("MIXAMO_FBX", race, len(body.data.vertices), "vertices")
