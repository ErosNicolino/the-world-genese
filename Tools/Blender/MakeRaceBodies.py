"""Os corpos das outras raças em blocos (02/11), no estilo do herói (Tools/Blender/MakeBlockCharacter.py), para subir no
Mixamo: elfo, anão, bárbaro, esqueleto e demônio, com as proporções de modelando ainda/outros corpos.png.

Cada corpo é feito de caixas com a pintura em pixel (a mesma receita do herói: a cor com ruído, mais escura embaixo e
nas bordas), na mesma A-pose (braços 40° para fora, pernas 7°), olhando para -Y. As medidas estão em "pixels da
referência" do herói (187 = 1,40 m):
- elfo: alto e magro (1,47 m), a cabeça um pouco menor;
- anão: baixo e largo (1,20 m), com a barriga;
- bárbaro: o maior (1,60 m), ombros e braços grossos;
- esqueleto: ossos finos (coluna, costelas, bacia), a cabeça lisa de osso;
- demônio: pele vermelha, garras e cascos escuros (1,54 m).

As peças que se escolhem no menu, como o cabelo, saem à parte, no lugar certo da cabeça de cada um:
- item_orelhas_elfo (as orelhas pontudas), item_chifres_demonio (os chifres curvos), item_cranio_esqueleto (o crânio
  com os olhos fundos, o nariz e os dentes, por cima da cabeça lisa).

Sai em modelando ainda/: corpo_<raça>.fbx e .blend, e item_<peça>.fbx (as texturas dentro do FBX, para o Mixamo).
blender -b --factory-startup -P Tools/Blender/MakeRaceBodies.py -- [raça ...]
"""
import math
import os
import random
import sys

import bpy
from mathutils import Matrix, Vector

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
OUT = os.path.join(ROOT, "modelando ainda")
PX = 1.40 / 187.0
TEXEL = 5.7
ARM_DEG, LEG_DEG, ARM_GAP = 40.0, 7.0, 2.0


def mix(a, b, t):
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(3))


SKIN = {"base": [(234, 169, 118), (232, 166, 116), (231, 165, 115), (229, 162, 113), (226, 158, 111)],
        "dark": (214, 144, 103), "deep": (196, 126, 91)}
SKIN_ELF = {"base": [(242, 196, 158), (240, 192, 154), (238, 190, 152), (236, 187, 149)], "dark": (222, 172, 136), "deep": (204, 152, 118)}
SKIN_RED = {"base": [(184, 54, 50), (180, 52, 48), (176, 50, 46), (172, 48, 44)], "dark": (150, 40, 38), "deep": (120, 30, 30)}
BONE = {"base": [(236, 228, 208), (232, 224, 204), (228, 220, 198), (224, 214, 192)], "dark": (204, 192, 166), "deep": (176, 164, 140)}
CLOTH = {"base": [(222, 197, 172), (219, 193, 168), (216, 189, 165), (212, 184, 159)], "dark": (196, 167, 143), "deep": (176, 148, 126)}
HORN = {"base": [(92, 40, 42), (84, 36, 38), (100, 46, 46)], "dark": (66, 26, 30), "deep": (48, 18, 22)}


def paint(cols, rows, face, part, pal, rng):
    img = []
    for r in range(rows):
        line = []
        for c in range(cols):
            col = rng.choice(pal["base"])
            t = r / max(1, rows - 1)
            col = mix(col, pal["dark"], .25 * t * t)
            if r == rows - 1 and face != "top":
                col = mix(col, pal["deep"], .55)
            if face == "bottom":
                col = mix(col, pal["deep"], .5)
            if part == "cabeca" and face == "top":
                col = mix(col, pal["dark"], .45)
            if c in (0, cols - 1) and face in ("front", "back"):
                col = mix(col, pal["dark"], .2)
            line.append(col)
        img.append(line)
    if part == "tronco" and face == "front":
        cy = rows // 3
        for c in range(cols):
            if c not in (cols // 2 - 1, cols // 2):
                img[cy][c] = mix(img[cy][c], pal["dark"], .45)
        for r in range(cy + 1, rows - 1):
            img[r][cols // 2] = mix(img[r][cols // 2], pal["dark"], .3)
    if part == "costelas" and face in ("front", "back", "left", "right"):
        # as costelas: listras escuras entre os ossos
        for r in range(rows):
            if r % 2 == 1:
                for c in range(cols):
                    if face != "front" or c not in (cols // 2,):
                        img[r][c] = mix(img[r][c], (90, 82, 70), .7)
    if part == "barriga" and face == "front":
        img[rows // 2][cols // 2] = mix(img[rows // 2][cols // 2], pal["deep"], .6)   # o umbigo
    if part in ("garra", "casco"):
        for r in range(rows):
            for c in range(cols):
                img[r][c] = mix(img[r][c], (60, 22, 26), .75)
    if part == "cueca":
        for c in range(cols):
            img[0][c] = mix(img[0][c], (230, 210, 188), .5)
    if part == "cranio" and face == "front":
        # os olhos fundos, o nariz e os dentes
        mid = cols // 2
        for r in range(rows):
            for c in range(cols):
                eye = r in (rows // 3, rows // 3 + 1) and c in (mid - 3, mid - 2, mid + 1, mid + 2)
                nose = r == rows // 3 + 3 and c in (mid - 1, mid)
                teeth_row = r == rows - 3
                if eye or nose:
                    img[r][c] = (34, 26, 24)
                elif teeth_row and c % 2 == 0 and 1 < c < cols - 2:
                    img[r][c] = (60, 50, 44)
    return img


def make_box(coll, name, size, center, part, pal, rng):
    """Uma caixa (w, d, h em pixels da referência; center = (x, y, z da base)) com a pintura em pixel."""
    w, d, h = size
    W, D, Hh = w * PX, d * PX, h * PX
    cx, cy, cz = center[0] * PX, center[1] * PX, center[2] * PX
    x0, x1, y0, y1, z0, z1 = cx - W / 2, cx + W / 2, cy - D / 2, cy + D / 2, cz, cz + Hh
    cw, cd, ch = max(1, round(w / TEXEL)), max(1, round(d / TEXEL)), max(1, round(h / TEXEL))
    faces = [
        ("front", [(x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)], cw, ch),
        ("back", [(x1, y1, z0), (x0, y1, z0), (x0, y1, z1), (x1, y1, z1)], cw, ch),
        ("right", [(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)], cd, ch),
        ("left", [(x0, y1, z0), (x0, y0, z0), (x0, y0, z1), (x0, y1, z1)], cd, ch),
        ("top", [(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)], cw, cd),
        ("bottom", [(x0, y1, z0), (x1, y1, z0), (x1, y0, z0), (x0, y0, z0)], cw, cd),
    ]
    aw = sum(f[2] for f in faces)
    ah = max(f[3] for f in faces)
    image = bpy.data.images.new("T_" + name, aw, ah, alpha=True)
    pixels = [0.0] * (aw * ah * 4)
    uvs = []
    ox = 0
    for fname, corners, cols, rows in faces:
        img = paint(cols, rows, fname, part, pal, rng)
        for r in range(rows):
            for c in range(cols):
                col = img[r][c]
                i = ((ah - 1 - r) * aw + ox + c) * 4
                pixels[i:i + 4] = [col[0] / 255.0, col[1] / 255.0, col[2] / 255.0, 1.0]
        uvs.append([(ox / aw, (ah - rows) / ah), ((ox + cols) / aw, (ah - rows) / ah), ((ox + cols) / aw, 1.0), (ox / aw, 1.0)])
        ox += cols
    image.pixels[:] = pixels
    image.pack()
    verts, polys = [], []
    for _, corners, _, _ in faces:
        polys.append(tuple(range(len(verts), len(verts) + 4)))
        verts.extend(corners)
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], polys)
    layer = mesh.uv_layers.new(name="UV")
    for p, quad in zip(mesh.polygons, uvs):
        for li, uv in zip(p.loop_indices, quad):
            layer.data[li].uv = uv
    mat = bpy.data.materials.new("M_" + name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = image
    tex.interpolation = "Closest"
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = .9
    mesh.materials.append(mat)
    obj = bpy.data.objects.new(name, mesh)
    coll.objects.link(obj)
    return obj


def humanoid(head, torso, hips, arm, leg, foot, shoulder_gap=2.0):
    """As peças de um corpo de bloco, empilhadas do chão para cima (medidas (w, d, h) em pixels da referência).
    Devolve (nome, tamanho, centro da base, parte) e as alturas que a pose precisa."""
    fz = 0
    lz = foot[2]
    hz = lz + leg[2]
    tz = hz + hips[2]
    cz = tz + torso[2]
    leg_x = hips[0] / 4 + .5
    arm_x = torso[0] / 2 + shoulder_gap + arm[0] / 2
    parts = [
        ("Cabeca", head, (0, 0, cz), "cabeca"),
        ("Tronco", torso, (0, 0, tz), "tronco"),
        ("Quadril", hips, (0, 0, hz), "cueca"),
        ("Braco_L", arm, (arm_x, 0, cz - arm[2]), "braco"),
        ("Braco_R", arm, (-arm_x, 0, cz - arm[2]), "braco"),
        ("Perna_L", leg, (leg_x, 0, lz), "perna"),
        ("Perna_R", leg, (-leg_x, 0, lz), "perna"),
        ("Pe_L", foot, (leg_x, -2, fz), "pe"),
        ("Pe_R", foot, (-leg_x, -2, fz), "pe"),
    ]
    return parts, {"shoulder": cz - 3, "torso_half": torso[0] / 2, "hip": hz, "knee": lz, "leg_x": leg_x, "head_top": cz + head[2],
                   "head": head, "head_z": cz}


def race_parts(race):
    """As peças de cada raça (e a paleta de cada peça)."""
    if race == "elfo":
        parts, info = humanoid((44, 42, 42), (38, 22, 60), (40, 24, 24), (17, 19, 74), (19, 20, 52), (20, 24, 18))
        return parts, info, {p[0]: SKIN_ELF if p[3] != "cueca" else CLOTH for p in parts}
    if race == "anao":
        parts, info = humanoid((48, 46, 44), (58, 34, 48), (56, 34, 20), (24, 26, 56), (25, 26, 30), (26, 30, 18))
        # a barriga, na frente do tronco
        parts.append(("Barriga", (44, 8, 30), (0, -20, info["hip"] + 20 + 4), "barriga"))
        return parts, info, {p[0]: SKIN if p[3] != "cueca" else CLOTH for p in parts}
    if race == "barbaro":
        parts, info = humanoid((50, 46, 46), (60, 32, 66), (52, 30, 24), (28, 30, 78), (27, 28, 56), (27, 32, 20), shoulder_gap=1.0)
        return parts, info, {p[0]: SKIN if p[3] != "cueca" else CLOTH for p in parts}
    if race == "demonio":
        parts, info = humanoid((46, 44, 44), (50, 28, 62), (48, 28, 24), (22, 24, 74), (22, 24, 52), (22, 26, 20))
        out = []
        for name, size, at, part in parts:
            if name.startswith("Pe"):
                part = "casco"
            if part == "cueca":
                part = "quadril"   # sem a faixa clara da cueca
            out.append((name, size, at, part))
        # as garras: a ponta dos braços, escura
        for side, s in (("L", 1), ("R", -1)):
            arm = next(p for p in out if p[0] == "Braco_" + side)
            out.append(("Garra_" + side, (arm[1][0] + 1, arm[1][1] + 1, 10), (arm[2][0], arm[2][1], arm[2][2] - 9), "garra"))
        return out, info, {p[0]: SKIN_RED for p in out}
    if race == "esqueleto":
        # ossos finos: a coluna, as costelas e a bacia; os membros de 10 a 11 pixels
        foot, leg, hips_h = (18, 24, 10), (11, 11, 54), 16
        lz = foot[2]
        hz = lz + leg[2]
        tz = hz + hips_h
        cz = tz + 60
        parts = [
            ("Cabeca", (42, 40, 40), (0, 0, cz), "cabeca"),
            ("Coluna", (10, 10, 60), (0, 4, tz), "osso"),
            ("Costelas", (36, 20, 32), (0, 0, tz + 24), "costelas"),
            ("Bacia", (34, 18, hips_h), (0, 0, hz), "osso"),
            ("Braco_L", (10, 10, 72), (24, 0, cz - 72), "osso"),
            ("Braco_R", (10, 10, 72), (-24, 0, cz - 72), "osso"),
            ("Perna_L", leg, (10, 0, lz), "osso"),
            ("Perna_R", leg, (-10, 0, lz), "osso"),
            ("Pe_L", foot, (10, -2, 0), "pe"),
            ("Pe_R", foot, (-10, -2, 0), "pe"),
        ]
        info = {"shoulder": cz - 3, "torso_half": 18, "hip": hz, "knee": lz, "leg_x": 10, "head_top": cz + 40, "head": (42, 40, 40), "head_z": cz}
        return parts, info, {p[0]: BONE for p in parts}
    raise ValueError(race)


def items_for(race, info):
    """As peças de escolher (orelhas, chifres, crânio), no lugar da cabeça desta raça."""
    hw, hd, hh = info["head"]
    z = info["head_z"]
    if race == "elfo":
        out = []
        for side, s in (("L", 1), ("R", -1)):
            # a orelha pontuda em degraus, subindo para trás e para fora
            out.append(("Orelha_%s_1" % side, (8, 10, 16), (s * (hw / 2 + 4), 3, z + hh * .3), "orelha"))
            out.append(("Orelha_%s_2" % side, (8, 8, 12), (s * (hw / 2 + 11), 5, z + hh * .5), "orelha"))
            out.append(("Orelha_%s_3" % side, (7, 6, 10), (s * (hw / 2 + 17), 7, z + hh * .68), "orelha"))
            out.append(("Orelha_%s_4" % side, (5, 4, 8), (s * (hw / 2 + 22), 8, z + hh * .85), "orelha"))
        return "orelhas_elfo", out, SKIN_ELF
    if race == "demonio":
        out = []
        for side, s in (("L", 1), ("R", -1)):
            # o chifre: sai do alto da cabeça, vai para fora e curva para cima
            out.append(("Chifre_%s_1" % side, (14, 14, 10), (s * (hw / 2 - 5), 0, z + hh), "chifre"))
            out.append(("Chifre_%s_2" % side, (13, 13, 10), (s * (hw / 2 + 6), 0, z + hh + 5), "chifre"))
            out.append(("Chifre_%s_3" % side, (11, 11, 12), (s * (hw / 2 + 14), 0, z + hh + 12), "chifre"))
            out.append(("Chifre_%s_4" % side, (9, 9, 12), (s * (hw / 2 + 17), 0, z + hh + 24), "chifre"))
            out.append(("Chifre_%s_5" % side, (6, 6, 10), (s * (hw / 2 + 15), 0, z + hh + 36), "chifre"))
        return "chifres_demonio", out, HORN
    if race == "esqueleto":
        # o crânio por cima da cabeça lisa (um pouco maior), com o queixo mais estreito embaixo
        return "cranio_esqueleto", [("Cranio", (hw + 2, hd + 2, hh - 8), (0, 0, z + 9), "cranio"),
                                    ("Queixo", (hw - 10, hd - 6, 10), (0, -2, z), "osso")], BONE
    return None, [], None


def pose(objs, info):
    """A A-pose do herói: os braços abertos no ombro, as pernas abertas no quadril (o pé vai junto)."""
    by = {o["parte"]: o for o in objs}
    for side, s in (("L", 1), ("R", -1)):
        pivot = Vector((s * (info["torso_half"] + ARM_GAP) * PX, 0, info["shoulder"] * PX))
        turn = Matrix.Translation(pivot) @ Matrix.Rotation(math.radians(-s * ARM_DEG), 4, "Y") @ Matrix.Translation(-pivot)
        for name in ("Braco_" + side, "Garra_" + side):
            if name in by:
                by[name].data.transform(turn)
        hip = Vector((s * info["leg_x"] * PX, 0, info["hip"] * PX))
        spread = Matrix.Translation(hip) @ Matrix.Rotation(math.radians(-s * LEG_DEG), 4, "Y") @ Matrix.Translation(-hip)
        by["Perna_" + side].data.transform(spread)
        knee = Vector((s * info["leg_x"] * PX, 0, info["knee"] * PX))
        by["Pe_" + side].data.transform(Matrix.Translation(spread @ knee - knee))
    lowest = min((o.matrix_world @ v.co).z for o in objs for v in o.data.vertices)
    for o in objs:
        o.data.transform(Matrix.Translation(Vector((0, 0, -lowest))))
    return lowest


def export(objs, path):
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, path_mode="COPY", embed_textures=True,
                             apply_unit_scale=True, bake_space_transform=False, object_types={"MESH"})


def build(race, save):
    rng = random.Random(sum(map(ord, race)))
    coll = bpy.data.collections.new("Corpo_" + race)
    bpy.context.scene.collection.children.link(coll)
    parts, info, pals = race_parts(race)
    objs = [make_box(coll, "%s_%s" % (race, n), size, at, part, pals[n], rng) for n, size, at, part in parts]
    for o, (n, *_rest) in zip(objs, parts):
        o["parte"] = n   # a pose acha a peça por aqui (o nome leva a raça: as peças de todas na mesma cena)
    item, item_parts, item_pal = items_for(race, info)
    items = [make_box(coll, "%s_%s" % (race, n), size, at, part, item_pal, rng) for n, size, at, part in item_parts]
    drop = pose(objs, info)
    for o in items:
        o.data.transform(Matrix.Translation(Vector((0, 0, -drop))))
    height = max((o.matrix_world @ v.co).z for o in objs for v in o.data.vertices)
    print("CORPO", race, "%d blocos, altura %.2f m" % (len(objs), height), ("item " + item) if item else "")
    if save:
        # o corpo em blocos separados, como o personagem_base.fbx que foi para o Mixamo
        export(objs, os.path.join(OUT, "corpo_%s.fbx" % race))
        if items:
            export(items, os.path.join(OUT, "item_%s.fbx" % item))
    return objs, items


if __name__ == "__main__":
    args = globals().get("ARGS") or (sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    headless = "--" in sys.argv
    races = [a for a in args if a not in ("ver",)] or ["elfo", "anao", "barbaro", "esqueleto", "demonio"]
    if headless:
        bpy.ops.wm.read_factory_settings(use_empty=True)
    for i, race in enumerate(races):
        objs, items = build(race, headless)
        if not headless:
            for o in objs + items:
                o.location.x += 12 + i * 1.2
    if headless:
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "corpos_racas.blend"), copy=True)
