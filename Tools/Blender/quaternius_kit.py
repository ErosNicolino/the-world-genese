"""As peças do kit da Quaternius para montar construções da vila (11/11): o que o BuildForjaQuaternius.py faz, num módulo
para as casas (BuildCasasQuaternius.py). Medidas de cada peça em `modelando ainda/quaternius_medidas.json`.

Parede do kit: 2 m de largura no x, a face de fora no +y (de -0,09 a 0,31), 3,12 m de altura.
Coordenadas do Blender: a frente da construção é +y, o chão é z = 0 (Godot: x = x, y = z, z = -y).
"""
import contextlib
import io
import json
import logging
import math
import os

import bpy
from mathutils import Vector

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
VILA = os.path.join(ROOT, "modelando ainda", "quaternius_vila", "pacote", "Medieval Village MegaKit[Standard]", "glTF")
PROPS = os.path.join(ROOT, "modelando ainda", "quaternius_objetos", "pacote", "Exports", "glTF")
TEX = os.path.join(ROOT, "Data", "GodotTown", "VilaTex")
KIT_OUT = os.path.join(ROOT, "Data", "GodotTown", "VilaKit")
INFO = os.path.join(ROOT, "Data", "GodotTown", "vila_models.json")
H = 3.12   # altura da parede do kit
# material do kit -> textura do jogo (T_Vila_<nome>.png; o ImportVilaKit.py troca o MV_<nome> pelo MI_Vila_<nome>)
MATS = {"MI_WoodTrim": "Q_WoodTrim", "MI_WoodTrim_Wear": "Q_WoodTrim", "MI_Plaster": "Q_Plaster", "MI_UnevenBrick": "Q_UnevenBrick",
        "MI_Brick": "Q_Brick", "MI_RedBrick": "Q_RedBrick", "MI_RockTrim": "Q_RockTrim", "MI_RoundTiles": "Q_RoundTiles",
        "MI_MetalOrnaments": "Q_Metal", "MI_Trim_Metal": "Q_Metal", "MI_Trim_Furniture": "Q_Furniture", "MI_Trim_Props": "Q_Props",
        "MI_Trim_Props_Vertex": "Q_Props", "MI_Trim_Cloth": "Q_Cloth", "MI_WindowGlass": "Vidro"}
# peças com triângulos demais para repetir em 50 casas (o canto de pedra tem 3.100): a parte que fica de cada uma
DECIMATE = {"Corner_Exterior_Brick": .22, "Lantern_Wall": .4, "Cauldron": .4, "Cabinet": .5, "Bookcase_2": .5, "Crate_Wooden": .5,
            "Balcony_Simple_Straight": .5, "Bed_Twin1": .6, "Bed_Twin2": .6, "Workbench": .6}
_cache = {}


def material(key):
    name = "MV_" + key
    m = bpy.data.materials.get(name)
    if m:
        return m
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    path = os.path.join(TEX, "T_Vila_%s.png" % key)
    if os.path.exists(path):
        node = m.node_tree.nodes.new("ShaderNodeTexImage")
        node.image = bpy.data.images.load(path, check_existing=True)
        node.interpolation = "Closest"
        m.node_tree.links.new(node.outputs[0], bsdf.inputs[0])
    return m


def source(name):
    """A peça do kit (importada uma vez, com os materiais trocados), fora da cena, para copiar."""
    if name in _cache and _cache[name].name in bpy.data.objects:
        return _cache[name]
    path = os.path.join(VILA, name + ".gltf")
    if not os.path.exists(path):
        path = os.path.join(PROPS, name + ".gltf")
    before = set(bpy.data.objects)
    logging.disable(logging.CRITICAL)          # o importador de glTF escreve duas linhas por peça
    with contextlib.redirect_stdout(io.StringIO()):
        bpy.ops.import_scene.gltf(filepath=path)
    logging.disable(logging.NOTSET)
    new = [o for o in bpy.data.objects if o not in before]
    meshes = [o for o in new if o.type == "MESH"]
    for o in meshes:
        world = o.matrix_world.copy()          # guarda a rotação do pai do arquivo antes de soltar
        o.parent = None
        o.matrix_world = world
    bpy.ops.object.select_all(action="DESELECT")
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for o in new:
        if o != obj and o.name in bpy.data.objects:
            bpy.data.objects.remove(o, do_unlink=True)
    if name in DECIMATE:
        mod = obj.modifiers.new("menos", "DECIMATE")
        mod.ratio = DECIMATE[name]
        bpy.ops.object.modifier_apply(modifier=mod.name)
    for slot in obj.material_slots:
        old = slot.material.name.split(".")[0] if slot.material else ""
        slot.material = material(MATS.get(old, "Q_Props"))
    obj.name = "SRC_" + name
    for c in obj.users_collection:
        c.objects.unlink(obj)
    _cache[name] = obj
    return obj


class Build:
    """Uma construção: as peças (put, cube) e as caixas de colisão (box), tudo no espaço do Blender."""

    def __init__(self, name):
        self.name = name
        self.parts, self.boxes = [], []
        self.coll = bpy.data.collections.get(name) or bpy.data.collections.new(name)
        if self.coll.name not in bpy.context.scene.collection.children:
            bpy.context.scene.collection.children.link(self.coll)
        for o in list(self.coll.objects):
            bpy.data.objects.remove(o, do_unlink=True)

    def put(self, name, x, y, z=0.0, rot=0.0, scale=(1, 1, 1)):
        src = source(name)
        o = src.copy()
        o.name = name
        o.location = (x, y, z)
        o.rotation_mode = "XYZ"   # o importador de glTF deixa em quatérnio: sem isto o giro era ignorado
        o.rotation_euler = (0, 0, math.radians(rot))
        o.scale = scale
        self.coll.objects.link(o)
        self.parts.append(o)
        return o

    def box(self, x0, y0, z0, x1, y1, z1):
        self.boxes.append((min(x0, x1), min(y0, y1), z0, max(x0, x1), max(y0, y1), z1))

    def cube(self, name, mat, x0, y0, z0, x1, y1, z1):
        import bmesh
        me = bpy.data.meshes.new(name)
        bm = bmesh.new()
        bmesh.ops.create_cube(bm, size=1.0)
        for v in bm.verts:
            v.co = Vector(((x0 + x1) / 2 + v.co.x * (x1 - x0), (y0 + y1) / 2 + v.co.y * (y1 - y0), (z0 + z1) / 2 + v.co.z * (z1 - z0)))
        bm.to_mesh(me)
        bm.free()
        uv = me.uv_layers.new()
        for loop in me.loops:
            uv.data[loop.index].uv = (0.5, 0.5)
        me.materials.append(material(mat))
        o = bpy.data.objects.new(name, me)
        self.coll.objects.link(o)
        self.parts.append(o)
        return o

    def shift(self, dx):
        """Só para ver no Blender: afasta a construção das outras."""
        for o in self.parts:
            o.location.x += dx

    def export(self, info):
        """Junta tudo, grava Data/GodotTown/VilaKit/<nome>.fbx e a entrada do vila_models.json (info, mais as caixas)."""
        bpy.ops.object.select_all(action="DESELECT")
        for o in self.parts:
            o.data = o.data.copy()
            o.select_set(True)
        bpy.context.view_layer.objects.active = self.parts[0]
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        bpy.ops.object.join()
        obj = bpy.context.view_layer.objects.active
        obj.name = obj.data.name = self.name
        # o kit guarda nas cores de vértice a máscara de desgaste do shader dele; o M_VilaKit multiplica a textura pela cor
        # do vértice, e o telhado saía preto: sem cor de vértice, a Unreal usa branco
        while obj.data.color_attributes:
            obj.data.color_attributes.remove(obj.data.color_attributes[0])
        bpy.ops.export_scene.fbx(filepath=os.path.join(KIT_OUT, self.name + ".fbx"), use_selection=True, object_types={"MESH"},
                                 mesh_smooth_type="FACE", add_leaf_bones=False, bake_anim=False, path_mode="STRIP", colors_type="SRGB",
                                 apply_scale_options="FBX_SCALE_UNITS")

        def godot(b):
            x0, y0, z0, x1, y1, z1 = b
            return [round((x0 + x1) / 2, 3), round((z0 + z1) / 2, 3), round(-(y0 + y1) / 2, 3), round(x1 - x0, 3), round(z1 - z0, 3), round(y1 - y0, 3)]

        doc = json.load(open(INFO, encoding="utf-8"))
        doc[self.name] = dict(info, boxes=[godot(b) for b in self.boxes])
        json.dump(doc, open(INFO, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        print("QUATERNIUS %s: %d triângulos, %d materiais, %d caixas" % (self.name, tris, len(obj.data.materials), len(self.boxes)))
        bpy.data.objects.remove(obj, do_unlink=True)
        return tris
