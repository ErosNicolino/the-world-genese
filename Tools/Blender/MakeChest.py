"""O baú em blocos (02/11): o comum (madeira e ferro) e o raro (vinho e ouro, com gemas), cada um em duas peças.

O usuário pediu "um baú bonito que abre com animação e fica aberto mesmo depois de pegar a recompensa". O antigo era
feito de caixas lisas em código (VortexWorldFeatures: TreasureChest do Godot). Este tem pixel art face a face:
tábuas com veio, cintas de ferro (ou de ouro no raro), cantoneiras, a fechadura e, dentro, as moedas (no 2º material,
que brilha quando a tampa abre). Mesmo tamanho do antigo: 1 m de largura, 65 cm de fundo, 50 cm de altura e a tampa
abaulada por cima.

Saem Data/Props/Chest/Chest_Body.glb e Chest_Lid.glb (e _Rare), com a textura T_Chest.png. 1 pixel = 2,5 cm. O corpo
tem a origem no meio do fundo; a tampa, na dobradiça (a borda de trás em cima do corpo). A frente é -Y (no Blender).
Na Unreal: Tools/ImportChest.py; AVortexWorldFeatures monta e abre.

blender -b --factory-startup -P Tools/Blender/MakeChest.py -- <raiz do projeto>
(no Blender aberto: exec com ARGS = [raiz, "ver"])
"""
import os
import random
import sys

import bmesh
import bpy
import numpy as np

PX = .025
TEX = 256

PALETTES = {
    "": {"wood": (128, 84, 46), "wood_d": (96, 60, 32), "wood_l": (152, 104, 60), "band": (78, 82, 92), "band_l": (132, 138, 150),
         "lock": (226, 178, 64), "lock_d": (160, 112, 36), "gem": None},
    "_Rare": {"wood": (128, 38, 44), "wood_d": (92, 24, 30), "wood_l": (156, 56, 60), "band": (214, 166, 62), "band_l": (250, 214, 120),
              "lock": (240, 198, 84), "lock_d": (176, 124, 44), "gem": (74, 208, 255)},
}
COIN = [(255, 214, 92), (240, 182, 54), (255, 236, 150), (210, 150, 40)]


def painter(pal):
    rng = random.Random(3)

    def paint(face, x, y, w, h, box):
        # as moedas: ouro de vários tons, um brilho de vez em quando
        if box == "moedas":
            return COIN[rng.randrange(4)] if rng.random() > .08 else (255, 250, 220)
        if box in ("fechadura", "lingueta"):
            if pal["gem"] and box == "fechadura" and face == "frente" and abs(x - w / 2 + .5) < 1 and abs(y - h / 2 + .5) < 1:
                return pal["gem"]
            if box == "fechadura" and face == "frente" and x == w // 2 and y >= h // 2:
                return (40, 30, 24)   # o buraco da chave
            return pal["lock_d"] if (x == 0 or y == 0 or x == w - 1 or y == h - 1) else pal["lock"]
        if box.startswith("canto"):
            return pal["band_l"] if (x + y) % 3 == 0 else pal["band"]
        # madeira: tábuas na horizontal (uma linha escura a cada 5 pixels), veio solto; cintas a 5 pixels das pontas
        band_cols = (4, 5, w - 6, w - 5) if face in ("frente", "tras", "cima") and w >= 30 else ()
        if x in band_cols:
            return pal["band_l"] if y % 4 == 0 else pal["band"]
        if face in ("frente", "tras", "esq", "dir") and (y % 5 == 4):
            return pal["wood_d"]
        if face == "cima" and (y % 5 == 4):
            return pal["wood_d"]
        r = rng.random()
        if r < .1:
            return pal["wood_d"]
        if r < .2:
            return pal["wood_l"]
        if pal["gem"] and face == "frente" and box == "tampa_cima" and y == h // 2 and x in (w // 4, 3 * w // 4):
            return pal["gem"]
        return pal["wood"]
    return paint


# (nome, mín, máx) em pixels: x para o lado, y para trás (-y é a frente), z para cima
BODY = [
    ("caixa", (-20, -13, 0), (20, 13, 19)),
    ("moedas", (-18, -11, 19), (18, 11, 20)),
    ("fechadura", (-3, -14, 12), (3, -13, 18)),
    ("canto_ee", (-21, -14, 0), (-18, -11, 3)), ("canto_ed", (18, -14, 0), (21, -11, 3)),
    ("canto_te", (-21, 11, 0), (-18, 14, 3)), ("canto_td", (18, 11, 0), (21, 14, 3)),
]
# a tampa em volta da dobradiça (a borda de trás em cima do corpo): fechada, vai de y = -26 (frente) a 0
LID = [
    ("tampa", (-20, -26, 0), (20, 0, 5)),
    ("tampa_cima", (-19, -25, 5), (19, -1, 8)),
    ("lingueta", (-2, -27, -4), (2, -26, 2)),
]
GLOW = {"moedas"}

FACES = [("frente", 1, -1, (0, 2)), ("tras", 1, 1, (0, 2)), ("dir", 0, 1, (1, 2)), ("esq", 0, -1, (1, 2)),
         ("cima", 2, 1, (0, 1)), ("baixo", 2, -1, (0, 1))]


def build(root, export):
    image = np.zeros((TEX, TEX, 4), dtype=np.float32)
    image[..., 3] = 1
    shelf = [0, 0, 0]
    objects = {}
    for suffix, pal in PALETTES.items():
        paint = painter(pal)
        for part, boxes in (("Body", BODY), ("Lid", LID)):
            bm = bmesh.new()
            uv = bm.loops.layers.uv.new("UVMap")
            for name, lo, hi in boxes:
                lo, hi = np.array(lo, float), np.array(hi, float)
                for fname, axis, sign, (ua, va) in FACES:
                    w, h = int(round(hi[ua] - lo[ua])), int(round(hi[va] - lo[va]))
                    if shelf[0] + w > TEX:
                        shelf = [0, shelf[1] + shelf[2], 0]
                    tx, ty = shelf[0], shelf[1]
                    shelf[0] += w + 1
                    shelf[2] = max(shelf[2], h + 1)
                    assert ty + h <= TEX, "textura cheia"
                    for py in range(h):
                        for px in range(w):
                            image[ty + py, tx + px, :3] = np.array(paint(fname, px, py, w, h, name)) / 255
                    plane = hi[axis] if sign > 0 else lo[axis]

                    def corner(cu, cv):
                        p = np.zeros(3)
                        p[axis] = plane
                        p[ua] = cu
                        p[va] = cv
                        return p
                    u0, u1 = lo[ua], hi[ua]
                    if (axis == 1 and sign > 0) or (axis == 0 and sign < 0):
                        u0, u1 = u1, u0
                    v_top, v_bot = (hi[va], lo[va]) if va == 2 else (hi[va], lo[va])
                    pts = [corner(u0, v_top), corner(u1, v_top), corner(u1, v_bot), corner(u0, v_bot)]
                    uvs = [(tx, ty), (tx + w, ty), (tx + w, ty + h), (tx, ty + h)]
                    normal = np.zeros(3)
                    normal[axis] = sign
                    if np.dot(np.cross(pts[1] - pts[0], pts[3] - pts[0]), normal) < 0:
                        pts = [pts[0], pts[3], pts[2], pts[1]]
                        uvs = [uvs[0], uvs[3], uvs[2], uvs[1]]
                    face = bm.faces.new([bm.verts.new(tuple(p * PX)) for p in pts])
                    face.material_index = 1 if name in GLOW else 0
                    for loop, (a, b) in zip(face.loops, uvs):
                        loop[uv].uv = (a / TEX, 1 - b / TEX)
            me = bpy.data.meshes.new("Chest_%s%s" % (part, suffix))
            bm.to_mesh(me)
            bm.free()
            obj = bpy.data.objects.new(me.name, me)
            bpy.context.scene.collection.objects.link(obj)
            objects[part + suffix] = obj
    img = bpy.data.images.new("T_Chest", TEX, TEX, alpha=True)
    img.pixels = image[::-1].ravel()
    for slot, glow in (("M_Chest", 0.0), ("M_Chest_Brilho", 2.0)):
        mat = bpy.data.materials.new(slot)
        mat.use_nodes = True
        nt = mat.node_tree
        bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
        tex = nt.nodes.new("ShaderNodeTexImage")
        tex.image = img
        tex.interpolation = "Closest"
        nt.links.new(tex.outputs[0], bsdf.inputs["Base Color"])
        if glow:
            nt.links.new(tex.outputs[0], bsdf.inputs["Emission Color"])
            bsdf.inputs["Emission Strength"].default_value = glow
        for obj in objects.values():
            obj.data.materials.append(mat)
    if export:
        out = os.path.join(root, "Data", "Props", "Chest")
        os.makedirs(out, exist_ok=True)
        img.filepath_raw = os.path.join(out, "T_Chest.png")
        img.file_format = "PNG"
        img.save()
        for key, obj in objects.items():
            bpy.ops.object.select_all(action="DESELECT")
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            bpy.ops.export_scene.gltf(filepath=os.path.join(out, "%s.glb" % obj.name), use_selection=True, export_format="GLB",
                                      export_yup=True, export_materials="PLACEHOLDER")
    return objects


if __name__ == "__main__":
    args = globals().get("ARGS") or sys.argv[sys.argv.index("--") + 1:]
    headless = "--" in sys.argv
    if headless:
        bpy.ops.wm.read_factory_settings(use_empty=True)
    objs = build(args[0], headless)
    print("CHEST", {k: tuple(round(x, 3) for x in o.dimensions) for k, o in objs.items()})
