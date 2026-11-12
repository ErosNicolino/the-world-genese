"""O cabelo da Lexy (02/11): pintado pela cor de vértice (R = 0 vira a cor hair_streak no M_BlockHairMechas, R = 1 a
cor do cabelo; no meio, a mistura).
- sem "degrade": o topo rosa (a coroa e o alto da franja) no camadas_medias (Hair_camadas_mechas);
- com "degrade": o chanel (bob_curto) do rosa no alto ao preto embaixo (Hair_bob_degrade_mechas), como o usuário descreveu.

blender -b --factory-startup -P Tools/Blender/MakeLexyHair.py -- <cabelo de origem.glb> <saída.glb> [degrade]
Depois: Tools/ImportBlockHair.py.
"""
import bpy, sys
args = sys.argv[sys.argv.index("--")+1:]
src, out = args[:2]
GRADIENT = "degrade" in args[2:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
hair = next(o for o in bpy.context.scene.objects if o.type == 'MESH' and o.name.startswith("Cabelo"))
me = hair.data
W = hair.matrix_world
zs = [(W @ v.co).z for v in me.vertices]; ys = [(W @ v.co).y for v in me.vertices]
z0, z1 = min(zs), max(zs); y0, y1 = min(ys), max(ys)
# em ponto flutuante (linear): a cor em byte é sRGB, e o glTF converte para linear: o meio do degradê virava quase
# rosa (0,5 -> 0,21) e o chanel saía todo rosa
attr = me.color_attributes.new("Color", 'FLOAT_COLOR', 'CORNER')
pink = 0
for poly in me.polygons:
    c = W @ poly.center
    h = (c.z - z0) / (z1 - z0)
    front = (c.y - y0) / (y1 - y0)      # 0 = frente (o rosto fica em -Y)
    # o topo rosa da foto: a coroa e a franja de cima (na frente desce um pouco mais), o resto escuro
    streak = h > .82 or (front < .25 and h > .74)
    for li in poly.loop_indices:
        if GRADIENT:
            # o degradê: rosa no alto, preto do meio para baixo (pela altura de cada canto, sem degrau entre as faces)
            vz = (W @ me.vertices[me.loops[li].vertex_index].co).z
            k = min(1.0, max(0.0, ((z1 - vz) / (z1 - z0) - .08) / .4))
            r = k * k * (3 - 2 * k)
            attr.data[li].color = (r, 1.0, 1.0, 1.0)
        else:
            attr.data[li].color = (0.0, 1.0, 1.0, 1.0) if streak else (1.0, 1.0, 1.0, 1.0)
    pink += streak
print("LEXY_HAIR faces", len(me.polygons), "rosa", pink)
bpy.ops.object.select_all(action="SELECT")
try:
    bpy.ops.export_scene.gltf(filepath=out, export_format="GLB", export_vertex_color="ACTIVE", export_animations=False)
except TypeError:
    bpy.ops.export_scene.gltf(filepath=out, export_format="GLB", export_colors=True, export_animations=False)
