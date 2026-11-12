"""A Forja de Bram com o kit da Quaternius (11/11; Medieval Village MegaKit e Fantasy Props MegaKit, CC0, versões grátis).

O mesmo desenho e o mesmo lugar da forja antiga (Tools/Blender/BuildVilaKit.py, função forja; cópia em
Backups/forja_antiga_2026-11-11): uma sala fechada atrás (12 x 6 m, de pedra, com porta e janelas e o telhado de telha)
e o alpendre aberto na frente (4 m, telhado de tábua em pilares), com a fornalha de tijolo e brasa e a chaminé, a
bigorna, a pedra de amolar, o suporte de armas, a bancada, barris, balde e corrente. O Bram fica no alpendre, na frente
da bigorna (Tools/Town/BuildVila.py: at_building("FORJA DE BRAM", -0.7, -3.4)): ali fica livre.

As texturas do kit viram pixel art (Tools/Town/MakeQuaterniusTextures.py): cada material do kit vira MV_Q_<nome>, que o
Tools/ImportVilaKit.py troca pelo MI_Vila_Q_<nome>. A brasa usa o MV_Brasa da vila (brilha).

Coordenadas: como o BuildVilaKit (Blender: a frente é +Y, o chão é z = 0; Godot: x = x, y = z, z = -y).
Sai Data/GodotTown/VilaKit/vila_forja.fbx e a entrada "vila_forja" do Data/GodotTown/vila_models.json (caixas de colisão,
alicerce, placa, lampião e chaminé). Depois: Tools/Town/BuildVila.py e Tools/ImportVilaKit.py.

No Blender aberto (MCP, para ver):  exec(open(r"<raiz>/Tools/Blender/BuildForjaQuaternius.py").read())
Sem janela (exporta):               blender -b --factory-startup -P Tools/Blender/BuildForjaQuaternius.py -- exportar
"""
import json
import math
import os
import sys

import bpy
from mathutils import Matrix, Vector

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
VILA = os.path.join(ROOT, "modelando ainda", "quaternius_vila", "pacote", "Medieval Village MegaKit[Standard]", "glTF")
PROPS = os.path.join(ROOT, "modelando ainda", "quaternius_objetos", "pacote", "Exports", "glTF")
TEX = os.path.join(ROOT, "Data", "GodotTown", "VilaTex")
OUT = os.path.join(ROOT, "Data", "GodotTown", "VilaKit", "vila_forja.fbx")
INFO = os.path.join(ROOT, "Data", "GodotTown", "vila_models.json")
EXPORT = "exportar" in sys.argv
# material do kit -> textura do jogo
MATS = {"MI_WoodTrim": "Q_WoodTrim", "MI_WoodTrim_Wear": "Q_WoodTrim", "MI_Plaster": "Q_Plaster", "MI_UnevenBrick": "Q_UnevenBrick",
        "MI_Brick": "Q_Brick", "MI_RedBrick": "Q_RedBrick", "MI_RockTrim": "Q_RockTrim", "MI_RoundTiles": "Q_RoundTiles",
        "MI_MetalOrnaments": "Q_Metal", "MI_Trim_Metal": "Q_Metal", "MI_Trim_Furniture": "Q_Furniture", "MI_Trim_Props": "Q_Props",
        "MI_Trim_Props_Vertex": "Q_Props", "MI_Trim_Cloth": "Q_Cloth", "MI_WindowGlass": "Vidro"}

coll = bpy.data.collections.get("Forja") or bpy.data.collections.new("Forja")
if coll.name not in bpy.context.scene.collection.children:
    bpy.context.scene.collection.children.link(coll)
for o in list(coll.objects):
    bpy.data.objects.remove(o, do_unlink=True)

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
        img = bpy.data.images.load(path, check_existing=True)
        node = m.node_tree.nodes.new("ShaderNodeTexImage")
        node.image = img
        node.interpolation = "Closest"
        m.node_tree.links.new(node.outputs[0], bsdf.inputs[0])
    return m


def source(name):
    """A peça do kit (importada uma vez, com os materiais trocados), escondida, para copiar."""
    if name in _cache:
        return _cache[name]
    path = os.path.join(VILA, name + ".gltf")
    if not os.path.exists(path):
        path = os.path.join(PROPS, name + ".gltf")
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    meshes = [o for o in new if o.type == "MESH"]
    for o in meshes:
        # guarda a posição real (com a rotação do pai do arquivo) antes de soltar; soltar primeiro perdia a rotação
        world = o.matrix_world.copy()
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
    for slot in obj.material_slots:
        old = slot.material.name.split(".")[0] if slot.material else ""
        slot.material = material(MATS.get(old, "Q_Props"))
    obj.name = "SRC_" + name
    for c in obj.users_collection:
        c.objects.unlink(obj)
    _cache[name] = obj
    return obj


parts = []
boxes = []   # caixas de colisão no espaço do Blender (x0, y0, z0, x1, y1, z1)


def put(name, x, y, z=0.0, rot=0.0, scale=(1, 1, 1)):
    src = source(name)
    o = src.copy()
    o.data = src.data
    o.name = name
    o.location = (x, y, z)
    o.rotation_mode = "XYZ"   # o importador de glTF deixa em quatérnio: sem isto o giro era ignorado
    o.rotation_euler = (0, 0, math.radians(rot))
    o.scale = scale
    coll.objects.link(o)
    parts.append(o)
    return o


def box(x0, y0, z0, x1, y1, z1):
    boxes.append((x0, y0, z0, x1, y1, z1))


def cube(name, mat, x0, y0, z0, x1, y1, z1):
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
    me.materials.append(material(mat) if mat.startswith("Q_") else (bpy.data.materials.get("MV_" + mat) or bpy.data.materials.new("MV_" + mat)))
    o = bpy.data.objects.new(name, me)
    coll.objects.link(o)
    parts.append(o)
    return o


# ---- a sala de pedra (y de -5 a 1) --------------------------------------------------------------------------------
H = 3.12   # altura da parede do kit
# 11/11 (o usuário: "paredes lisas, parece um papel"): a face de fora da parede do kit é o -y da peça (a pedra); as paredes
# estavam viradas ao contrário, com o reboco liso para fora e a pedra para dentro
for i, x in enumerate((-5, -3, -1, 1, 3, 5)):
    put("Wall_UnevenBrick_Window_Wide_Flat" if x in (-3, 3) else "Wall_UnevenBrick_Straight", x, -5)              # fundos
    front = "Wall_UnevenBrick_Door_Round" if x == 1 else "Wall_UnevenBrick_Straight"
    put(front, x, 1, rot=180)                                                                                      # frente
for y in (-4, -2, 0):
    put("Wall_UnevenBrick_Straight", -6, y, rot=-90)
    put("Wall_UnevenBrick_Window_Wide_Flat" if y == -2 else "Wall_UnevenBrick_Straight", 6, y, rot=90)
for (x, y, r) in ((-6, -5, 0), (6, -5, 90), (-6, 1, -90), (6, 1, 180)):
    put("Corner_Exterior_Brick", x, y, rot=r)
box(-6.2, -5.2, 0, 6.2, -4.7, H)          # fundos
box(-6.2, -5.2, 0, -5.7, 1.2, H)          # lado esquerdo
box(5.7, -5.2, 0, 6.2, 1.2, H)            # lado direito
# 11/11: o vão do arco da porta (medido no kit): x de 0,40 a 1,65 m, até 2,45 m de altura
box(-6.2, 0.7, 0, 0.4, 1.2, H)            # frente, à esquerda da porta
box(1.65, 0.7, 0, 6.2, 1.2, H)            # frente, à direita da porta
box(-6.2, 0.7, 2.45, 6.2, 1.2, H)         # em cima da porta
DOOR = (0.4, 0.89, 0.0, 1.25, 2.2)        # a folha de abrir do jogo (vila_porta): dobradiça, largura e altura (Blender)


def glass(x, y, rot):
    """11/11 (o usuário: "sem janela, parece um papel"): a janela do kit (moldura, vidro e venezianas) no vão da parede."""
    put("Window_Wide_Flat1", x, y, rot=rot)
    put("WindowShutters_Wide_Flat_Open", x, y, rot=rot)


glass(-3, -5, 0)
glass(3, -5, 0)
glass(6, -2, 90)
# chão de tijolo na sala e no alpendre
for x in (-5, -3, -1, 1, 3, 5):
    for y in (-4, -2, 0, 2, 4):
        put("Floor_Brick", x, y, .02)
box(-6.3, -5.3, -0.3, 6.3, 5.3, .05)
# o telhado de telha da sala: 6 x 12 deitado ao longo de x
put("Roof_RoundTiles_6x12", 0, -2, H, rot=90)
# as empenas (o triângulo de reboco e madeira debaixo do telhado, nos dois lados)
put("Roof_Front_Brick6", -6, -2, H, rot=-90)
put("Roof_Front_Brick6", 6, -2, H, rot=90)

# ---- o alpendre (y de 1 a 5): pilares, telhado de tábua e cerca nos lados -----------------------------------------
POST = 2.90   # 11/11: o telhado do alpendre começa a 2,88 m em cima dos pilares; com 2,55 ficava um vão de 33 cm
for x in (-6, -2, 2, 6):
    put("Corner_Exterior_Wood", x, 5, rot=0, scale=(1, 1, POST / 3.0))
    box(x - .2, 4.8, 0, x + .2, 5.2, POST)
for x in (-5, -3, -1, 1, 3, 5):
    put("Roof_Wooden_2x1", x, 1.0, H - .15, rot=180, scale=(1.0, 2.7, .55))   # 2,26 m de largura: encostam sem fresta
for y in (2, 4):
    put("Prop_WoodenFence_Single", -6, y, rot=90)
    put("Prop_WoodenFence_Single", 6, y, rot=90)
box(-6.2, 1, 0, -5.8, 5, .9)
box(5.8, 1, 0, 6.2, 5, .9)

# ---- a fornalha: base de tijolo com a brasa em cima e a chaminé atravessando o telhado ------------------------------
FX, FY = -3.9, 1.95
put("Prop_Chimney", FX, FY, 0, scale=(2.4, 1.5, .34))
# 11/11 (o usuário: "chaminé cortada, a lava cortada"): a brasa só na metade da frente e a chaminé numa peça só, na metade
# de trás, do topo da fornalha até 7,2 m (antes eram duas peças, com a tampa de uma no meio, e a de baixo entrava na brasa)
cube("Brasa", "Brasa", FX - 1.0, FY, 1.06, FX + 1.0, FY + .65, 1.14)
CHIMNEY_Y = FY - .4
put("Prop_Chimney", FX, CHIMNEY_Y, 1.08, scale=(1.0, .8, (7.2 - 1.08) / 3.18))
box(FX - 1.2, FY - .75, 0, FX + 1.2, FY + .75, 1.1)
# ---- as coisas do ferreiro ----------------------------------------------------------------------------------------
# 11/11 (o usuário: "ferreiro martelando o ar"): a bigorna na frente do Bram, que fica em (-0,7, 3,4) olhando para a rua
put("Anvil_Log", -.7, 4.1)
box(-1.15, 3.75, 0, -.2, 4.5, 1.05)
put("Bucket_Metal", -1.6, 4.5)
put("Chain_Coil", -1.9, 1.7, rot=30)
put("Whetstone", 3.8, 3.3, rot=-90)
box(3.3, 2.8, 0, 4.4, 3.9, 1.1)
put("WeaponStand", 4.5, 1.7)
box(3.8, 1.2, 0, 5.2, 2.2, 1.1)
# a bancada encostada na cerca da esquerda (antes ficava na frente da porta e não dava para passar)
put("Workbench", -5.2, 3.6, rot=90)
box(-5.75, 2.6, 0, -4.65, 4.6, .9)
put("Barrel", 5.2, 4.3)
put("Barrel", 4.5, 4.5, rot=40)
box(4.1, 3.9, 0, 5.6, 4.9, .9)
put("Crate_Wooden", -3.2, 4.3, rot=15)
box(-3.8, 3.7, 0, -2.6, 4.9, 1.1)
# o lampião na parede de fora, à direita da porta (antes estava virado para dentro, rente ao chão)
put("Lantern_Wall", 2.6, 1.09, 1.2, rot=180)
# a passagem da rua até a porta (x de 0,4 a 1,65) fica livre

if EXPORT:
    bpy.ops.object.select_all(action="DESELECT")
    for o in parts:
        o.data = o.data.copy()
        o.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.make_single_user(object=True, obdata=True)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bpy.ops.object.join()
    forja = bpy.context.view_layer.objects.active
    forja.name = "vila_forja"
    forja.data.name = "vila_forja"
    # o kit guarda nas cores de vértice a máscara de desgaste do shader dele (quase preta no telhado); o M_VilaKit multiplica
    # a textura pela cor do vértice, e o telhado saía preto no jogo: sem cor de vértice, a Unreal usa branco
    while forja.data.color_attributes:
        forja.data.color_attributes.remove(forja.data.color_attributes[0])
    bpy.ops.export_scene.fbx(filepath=OUT, use_selection=True, object_types={"MESH"}, mesh_smooth_type="FACE", add_leaf_bones=False,
                             bake_anim=False, path_mode="STRIP", colors_type="SRGB", apply_scale_options="FBX_SCALE_UNITS")

    def to_godot_box(b):
        x0, y0, z0, x1, y1, z1 = b
        return [round((x0 + x1) / 2, 3), round((z0 + z1) / 2, 3), round(-(y0 + y1) / 2, 3), round(x1 - x0, 3), round(z1 - z0, 3), round(y1 - y0, 3)]

    info = json.load(open(INFO, encoding="utf-8"))
    info["vila_forja"] = {
        "boxes": [to_godot_box(b) for b in boxes],
        "footing": [-6.45, -5.6, 6.45, 5.5],
        "sign": [3.2, 2.8, -5.06],
        "lamps": [[FX, 1.4, -(FY + .3)], [2.6, 1.9, -2.3]],     # a brasa e o lampião
        "chimneys": [[FX, 7.2, -CHIMNEY_Y]],
        "inner": [[0, 2.6, 2.0]],
        "doors": [[DOOR[0], DOOR[2], -DOOR[1], DOOR[3], DOOR[4]]],
    }
    json.dump(info, open(INFO, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    tris = sum(len(p.vertices) - 2 for p in forja.data.polygons)
    print("FORJA_QUATERNIUS exportada: %d triângulos, %d materiais, %d caixas -> %s" % (tris, len(forja.data.materials), len(boxes), OUT))
else:
    print("FORJA_QUATERNIUS montada na coleção Forja: %d peças" % len(parts))
