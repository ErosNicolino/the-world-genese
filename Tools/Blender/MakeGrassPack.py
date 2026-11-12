"""Grama da Quaternius (Stylized Nature MegaKit, CC0) no tamanho do jogo (10/11).

Os 4 tufos do pacote (1 a 1,8 m de altura) viram as touceiras do jogo: rasteira, duas de prado e alta. A forma, a UV
(coluna = cor, v = 1 na raiz e 0 na ponta) e o material ficam como vieram; só a medida muda (largura e altura à parte).
Sai em modelando ainda/quaternius_natureza/grama_jogo/SM_GrassPack_<nome>.fbx, em cm; depois Tools/ImportGrassPack.py.

  blender -b --factory-startup -P Tools/Blender/MakeGrassPack.py
"""
import os
import bpy

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "modelando ainda", "quaternius_natureza", "pacote", "glTF")
OUT = os.path.join(ROOT, "modelando ainda", "quaternius_natureza", "grama_jogo")
# nome no jogo: (arquivo do pacote, altura em cm, largura em cm)
PIECES = {
    "Rasteira": ("Grass_Common_Short", 18.0, 46.0),
    "PradoFina": ("Grass_Wispy_Short", 30.0, 42.0),
    "PradoAlta": ("Grass_Common_Tall", 38.0, 36.0),
    "Alta": ("Grass_Wispy_Tall", 56.0, 46.0),
}
os.makedirs(OUT, exist_ok=True)
for name, (source, height, width) in PIECES.items():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=os.path.join(SRC, source + ".gltf"))
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    bpy.ops.object.select_all(action="DESELECT")
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.parent = None
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    xs = [v.co.x for v in obj.data.vertices]
    ys = [v.co.y for v in obj.data.vertices]
    zs = [v.co.z for v in obj.data.vertices]
    # metros -> cm, com largura e altura à parte; a raiz no zero
    sx = width / 100.0 / max(max(xs) - min(xs), max(ys) - min(ys))
    sz = height / 100.0 / (max(zs) - min(zs))
    for v in obj.data.vertices:
        v.co.x *= sx
        v.co.y *= sx
        v.co.z = (v.co.z - min(zs)) * sz
    obj.name = "SM_GrassPack_" + name
    obj.data.name = obj.name
    for o in list(bpy.context.scene.objects):
        if o != obj:
            bpy.data.objects.remove(o)
    path = os.path.join(OUT, obj.name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=False, object_types={"MESH"}, apply_unit_scale=True,
                             global_scale=1.0, mesh_smooth_type="FACE", add_leaf_bones=False, bake_anim=False)
    print("GRASS_PACK", name, "verts", len(obj.data.vertices), "->", path)
