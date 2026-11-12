"""O cachorro da Julia (02/11, refeito do zero): um bichon em blocos, como em modelando ainda/referencia.jpeg.

A 1ª versão era voxel solto (AVortexPet montava elipsoides de cubinhos): virava "uma bola de pelos". Esta tem formas
claras, cada uma um bloco com pixel art pintada face a face:
- cabeça grande e quadrada, focinho saliente, olhos pretos com o brilho, a mancha de lágrima, nariz preto e a língua;
- orelhas compridas caídas dos lados, até abaixo do queixo;
- corpo fofo, patas curtas de patinha clara e o rabo pompom.

Cada parte sai num arquivo, com a origem no ponto onde ela gira no jogo (AVortexPet anima pelos pivôs):
Data/Creatures/Pet/Dog_Body.glb (origem no meio do corpo), Dog_Head.glb (no pescoço), Dog_Leg.glb (no alto da pata) e
Dog_Tail.glb (na base), mais a textura T_Dog.png. 1 pixel da arte = 2 cm. Na Unreal: Tools/ImportDog.py.

blender -b --factory-startup -P Tools/Blender/MakeDog.py -- <raiz do projeto>
(no Blender aberto: exec com ARGS = [raiz, "ver"]: monta as partes na cena, sem exportar)
"""
import os
import random
import sys

import bmesh
import bpy
import numpy as np

PX = .02          # metros por pixel da arte
TEX = 128
rng = random.Random(7)

WHITE = (246, 243, 236)
CREAM = (232, 224, 210)
SHADE = (214, 204, 188)
STAIN = (196, 170, 140)
BLACK = (28, 24, 26)
GLINT = (250, 250, 250)
NOSE = (36, 30, 32)
PINK = (236, 128, 146)
PAW = (220, 210, 196)


def fur(x, y, face):
    """O pelo cacheado: branco com cachos (um "c" de 2 pixels) mais escuros, e a sombra embaixo."""
    h = rng.random()
    if face == "baixo":
        return SHADE
    if (x * 3 + y * 5) % 7 == 0 or h < .12:
        return CREAM if h > .04 else SHADE
    return WHITE


# ---- as partes: blocos (min e max em pixels, x para a frente, y para o lado, z para cima) e quem pinta cada face

def head_paint(face, x, y, w, h, box):
    """O rosto na frente do bloco da cabeça (x, y: pixels da face, y de cima para baixo)."""
    if box == "cabeca" and face == "frente":
        cx = w / 2
        for s in (-1, 1):
            ex = int(cx + s * 3.5 - (1 if s < 0 else 0))
            if y in (5, 6) and x in (ex, ex + 1):
                return GLINT if (y == 5 and x == ex + (1 if s > 0 else 0)) else BLACK
            if y == 7 and x in (ex, ex + 1):
                return STAIN
    if box == "focinho" and face == "frente":
        cx = w / 2
        if y in (0, 1) and abs(x + .5 - cx) < 1.6:
            return NOSE
        if y == 2 and abs(x + .5 - cx) < .6:
            return NOSE
        if y == 3 and abs(x + .5 - cx) < 2.2:
            return (120, 60, 64)
    if box == "lingua":
        return PINK
    if box.startswith("orelha"):
        return CREAM if (x + y) % 3 else SHADE
    return fur(x, y, face)


def leg_paint(face, x, y, w, h, box):
    if face == "baixo" or y >= h - 2:
        return PAW
    return fur(x, y, face)


PARTS = {
    # parte: (blocos, quem pinta)
    "Body": ([("corpo", (-8, -6, -5), (8, 6, 6)), ("peito", (5, -5, -4), (10, 5, 6)), ("lombo", (-7, -5, 6), (5, 5, 7))],
             lambda f, x, y, w, h, b: fur(x, y, f)),
    "Head": ([("cabeca", (-1, -7, 0), (13, 7, 13)), ("focinho", (13, -3, 1), (17, 3, 6)), ("lingua", (14, -1, 0), (17, 1, 1)),
              ("topete", (1, -6, 13), (11, 6, 15)), ("orelha_e", (2, -9, -5), (7, -7, 11)), ("orelha_d", (2, 7, -5), (7, 9, 11))],
             head_paint),
    "Leg": ([("pata", (-2.5, -2.5, -8), (2.5, 2.5, 0))], leg_paint),
    "Tail": ([("pompom", (-3, -3, 0), (3, 3, 6))], lambda f, x, y, w, h, b: fur(x, y, f)),
}

FACES = [  # nome, eixo da normal, sinal, (eixo u, eixo v) na face, vista de fora (v desce)
    ("frente", 0, 1, (1, 2)), ("tras", 0, -1, (1, 2)), ("dir", 1, 1, (0, 2)), ("esq", 1, -1, (0, 2)),
    ("cima", 2, 1, (0, 1)), ("baixo", 2, -1, (0, 1)),
]


def build(root, export):
    image = np.zeros((TEX, TEX, 4), dtype=np.float32)
    image[..., 3] = 1
    image[..., :3] = np.array(WHITE) / 255
    shelf = [0, 0, 0]       # x, y, altura da prateleira
    objects = {}
    for part, (boxes, paint) in PARTS.items():
        bm = bmesh.new()
        uv = bm.loops.layers.uv.new("UVMap")
        for name, lo, hi in boxes:
            lo, hi = np.array(lo, float), np.array(hi, float)
            size = hi - lo
            for fname, axis, sign, (ua, va) in FACES:
                w, h = int(round(size[ua])), int(round(size[va]))
                if shelf[0] + w > TEX:
                    shelf = [0, shelf[1] + shelf[2], 0]
                tx, ty = shelf[0], shelf[1]
                shelf[0] += w + 1
                shelf[2] = max(shelf[2], h + 1)
                assert ty + h <= TEX, "textura cheia"
                for py in range(h):
                    for px in range(w):
                        image[ty + py, tx + px, :3] = np.array(paint(fname, px, py, w, h, name)) / 255
                # os 4 cantos da face (em pixels), na ordem u-, u+ ; v de cima (alto) para baixo
                plane = hi[axis] if sign > 0 else lo[axis]

                def corner(cu, cv):
                    p = np.zeros(3)
                    p[axis] = plane
                    p[ua] = cu
                    p[va] = cv
                    return p
                # u cresce para a direita de quem olha de fora; v de cima para baixo
                u0, u1 = (lo[ua], hi[ua])
                if (axis == 0 and sign < 0) or (axis == 1 and sign > 0):
                    u0, u1 = u1, u0
                if va == 2:
                    v_top, v_bot = hi[va], lo[va]
                else:
                    v_top, v_bot = (hi[va], lo[va]) if axis == 2 and sign > 0 else (lo[va], hi[va])
                pts = [corner(u0, v_top), corner(u1, v_top), corner(u1, v_bot), corner(u0, v_bot)]
                uvs = [(tx, ty), (tx + w, ty), (tx + w, ty + h), (tx, ty + h)]
                verts = [bm.verts.new(tuple(p * PX)) for p in pts]
                normal = np.zeros(3)
                normal[axis] = sign
                e1 = np.array(pts[1]) - np.array(pts[0])
                e2 = np.array(pts[3]) - np.array(pts[0])
                if np.dot(np.cross(e1, e2), normal) < 0:
                    # a frente para fora (o Blender conta a volta no sentido anti-horário)
                    verts = [verts[0], verts[3], verts[2], verts[1]]
                    uvs = [uvs[0], uvs[3], uvs[2], uvs[1]]
                face = bm.faces.new(verts)
                for loop, (a, b) in zip(face.loops, uvs):
                    loop[uv].uv = (a / TEX, 1 - b / TEX)
        me = bpy.data.meshes.new("Dog_" + part)
        bm.to_mesh(me)
        bm.free()
        obj = bpy.data.objects.new("Dog_" + part, me)
        bpy.context.scene.collection.objects.link(obj)
        objects[part] = obj

    img = bpy.data.images.new("T_Dog", TEX, TEX, alpha=True)
    img.pixels = image[::-1].ravel()
    mat = bpy.data.materials.new("M_Dog")
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = img
    tex.interpolation = "Closest"
    nt.links.new(tex.outputs[0], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = .9
    for obj in objects.values():
        obj.data.materials.append(mat)
    if export:
        out = os.path.join(root, "Data", "Creatures", "Pet")
        os.makedirs(out, exist_ok=True)
        img.filepath_raw = os.path.join(out, "T_Dog.png")
        img.file_format = "PNG"
        img.save()
        for part, obj in objects.items():
            bpy.ops.object.select_all(action="DESELECT")
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            bpy.ops.export_scene.gltf(filepath=os.path.join(out, "Dog_%s.glb" % part), use_selection=True,
                                      export_format="GLB", export_yup=True, export_materials="NONE")
    return objects


if __name__ == "__main__":
    args = globals().get("ARGS") or sys.argv[sys.argv.index("--") + 1:]
    headless = "--" in sys.argv
    if headless:
        bpy.ops.wm.read_factory_settings(use_empty=True)
    objs = build(args[0], headless)
    print("DOG", {k: tuple(round(x, 3) for x in o.dimensions) for k, o in objs.items()})
