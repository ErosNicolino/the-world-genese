"""Os 35 arcos das pranchas do usuário em blocos (07/11), sem a corda (a corda é da Unreal: VortexWeaponParts), e a
flecha de cada um.

Arco: a grade de pixels (Tools/Weapons/SliceBowSheets.py -> Data/VortexWeapons/Bows/<raridade>_<n>.png, a pegada para a
direita) vira cubos na mesma moldura do arco de blocos do jogo (Tools/Blender/MakeBlockWeapons.py: o eixo, o lado da
curva e a pegada do arco do Meshy), então todos ficam na mão igual:
- o meio do arco (a pegada) cai na mão; o maior de cada prancha tem o comprimento do arco atual (64 blocos);
- a espessura: 3 blocos; as pontinhas soltas (folhas, espinhos, penas) 1;
- as partes claras e saturadas (gemas, runas, fogo, cristal) vão para o material que brilha (MW_<nome>_brasa).

Flecha: do tamanho e na posição da flecha do jogo (W_flecha: a ponta na origem), em blocos, com as cores do arco dela
(Data/Rpg/bows.json, Tools/Weapons/BuildBows.py): a haste na cor da madeira do arco, a ponta no formato do elemento
(fogo: chama; gelo: cristal; raio: zigue-zague; natureza: folha; veneno: farpas; água: gota; luz: estrela; sombra:
lasca escura; sem elemento: ponta de ferro) e as penas na cor de destaque do arco. Das raras para cima a ponta brilha.

blender -b --factory-startup -P Tools/Blender/MakePixelBows.py -- <raiz> [raridade_n ...]
Sai Data/VortexWeapons/Models/W_arco_<chave>_blocos.glb, W_flecha_<chave>_blocos.glb, as texturas em Textures/Novas e
a conferência Data/VortexWeapons/Bows/_flechas.png (Tools/ImportBlockSword.py leva para a Unreal).
"""
import colorsys
import importlib.util
import json
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector

ARGS = globals().get("ARGS") or (sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
ROOT = ARGS[0] if ARGS else r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
ONLY = ARGS[1:]
BOWS = os.path.join(ROOT, "Data", "VortexWeapons", "Bows")
MODELS = os.path.join(ROOT, "Data", "VortexWeapons", "Models")
TEXTURES = os.path.join(ROOT, "Data", "VortexWeapons", "Textures", "Novas")
BASE_ROWS = 64          # o maior arco de cada prancha = o comprimento do arco do Meshy
ARROW_ROWS = 46         # a flecha: 46 blocos de comprido
PX = 4

spec = importlib.util.spec_from_file_location("block_weapons", os.path.join(ROOT, "Tools", "Blender", "MakeBlockWeapons.py"))
block_weapons = importlib.util.module_from_spec(spec)
spec.loader.exec_module(block_weapons)


def load_grid(path):
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    a = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1]     # linha 0 = em cima
    bpy.data.images.remove(img)
    return a


def voxels_to_mesh(name, filled, colors, v):
    """filled: {(posição Vector): (cor rgb, acende)} -> malha de cubos com UV numa textura de cores (cada cor um quadrado)."""
    palette = []
    index = {}
    for rgb, glow in colors.values():
        key = (tuple(np.round(np.asarray(rgb) * 255).astype(int)), glow)
        if key not in index:
            index[key] = len(palette)
            palette.append(key)
    side = 1
    while side * side < len(palette):
        side *= 2
    W = max(32, side * PX)
    color = np.ones((W, W, 4), np.float32)
    color[..., :3] = .3
    rough = np.ones((W, W, 4), np.float32)
    rough[..., :3] = .8
    for i, (rgb, glow) in enumerate(palette):
        x, y = i % side, i // side
        y0 = W - (y + 1) * PX
        color[y0:y0 + PX, x * PX:(x + 1) * PX, :3] = np.asarray(rgb) / 255.0
        sat = max(rgb) - min(rgb)
        rough[y0:y0 + PX, x * PX:(x + 1) * PX, :3] = .55 if sat < 30 else .75
    keys = set(filled)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    for cell, (center, axes) in filled.items():
        rgb, glow = colors[cell]
        i = index[(tuple(np.round(np.asarray(rgb) * 255).astype(int)), glow)]
        u = ((i % side) + .5) * PX / W
        vv = 1 - ((i // side) + .5) * PX / W
        A, Wd, T = axes
        r, k, d = cell
        for dr, dk, dd in ((0, 1, 0), (0, -1, 0), (1, 0, 0), (-1, 0, 0), (0, 0, 1), (0, 0, -1)):
            if (r + dr, k + dk, d + dd) in keys:
                continue
            nrm = A * dr + Wd * dk + T * dd
            e1, e2 = (Wd, T) if dr else (T, A) if dk else (A, Wd)
            fc = center + nrm * (v / 2)
            corners = [fc + (e1 * p + e2 * q) * (v / 2) for p, q in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
            if (e1.cross(e2)).dot(nrm) < 0:
                corners.reverse()
            face = bm.faces.new([bm.verts.new(p) for p in corners])
            face.material_index = 1 if glow else 0
            for loop in face.loops:
                loop[uv].uv = (u, vv)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=v * 1e-3)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    tex_name = "T" + name[1:]
    img = bpy.data.images.new(tex_name, W, W, alpha=True)
    img.pixels = color.ravel()
    img_r = bpy.data.images.new(tex_name + "_rough", W, W, alpha=True)
    img_r.colorspace_settings.name = "Non-Color"
    img_r.pixels = rough.ravel()
    for mname, strength in (("MW" + name[1:], 0.0), ("MW" + name[1:] + "_brasa", 3.0)):
        mat = bpy.data.materials.new(mname)
        mat.use_nodes = True
        nt = mat.node_tree
        bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
        tex = nt.nodes.new("ShaderNodeTexImage")
        tex.image = img
        tex.interpolation = "Closest"
        nt.links.new(tex.outputs[0], bsdf.inputs["Base Color"])
        bsdf.inputs["Roughness"].default_value = .7
        if strength:
            nt.links.new(tex.outputs[0], bsdf.inputs["Emission Color"])
            bsdf.inputs["Emission Strength"].default_value = strength
        me.materials.append(mat)
    return obj, img, img_r


# ---- o arco

def bow_cells(a):
    """{(linha, coluna, profundidade): (rgb, acende)} com a pegada em (0, 0)."""
    filled = a[..., 3] > .5
    rows = np.nonzero(filled.any(axis=1))[0]
    r0, r1 = rows.min(), rows.max()
    grip_r = (r0 + r1) // 2
    mid = [r for r in range(grip_r - 3, grip_r + 4) if filled[r].any()]
    grip_k = int(round(np.mean([np.nonzero(filled[r])[0].mean() for r in mid])))
    R, C = filled.shape
    out = {}
    for r in range(R):
        for c in range(C):
            if not filled[r, c]:
                continue
            rgb = a[r, c, :3]
            hh, ss, vv = colorsys.rgb_to_hsv(*rgb)
            near = sum(1 for dr, dc in ((1, 0), (-1, 0), (0, 1), (0, -1))
                       if 0 <= r + dr < R and 0 <= c + dc < C and filled[r + dr, c + dc])
            thick = 3 if near >= 2 else 1
            glow = vv > .86 and ss > .5
            for d in range(thick):
                out[(r - grip_r, c - grip_k, d - thick // 2)] = (rgb, glow)
    return out


def build_bow(key, a, frame):
    c, ax, wide, thin, t0, t1, origin, bend, origin_w, bend_side = frame
    v = (t1 - t0) / BASE_ROWS
    A, Wd, T = Vector(ax), Vector(wide) * bend_side, Vector(thin)
    origin_w = float(Vector(-np.array(c)).dot(Wd))
    C0 = Vector(c)
    cells = bow_cells(a)
    filled, colors = {}, {}
    for (r, k, d), (rgb, glow) in cells.items():
        # a pegada no meio do comprimento do arco do Meshy, como o arco de blocos atual (MakeBlockWeapons.arco)
        center = C0 + A * ((t0 + t1) / 2 + r * v) + Wd * (origin_w + k * v) + T * (d * v)
        filled[(r, k, d)] = (center, (A, Wd, T))
        colors[(r, k, d)] = (rgb, glow)
    return voxels_to_mesh("W_arco_%s_blocos" % key, filled, colors, v)


# ---- a flecha

HEADS = {
    # meia-largura da ponta, da ponta para trás
    "": [0, 1, 1, 2, 2, 1, 0],
    "fire": [0, 1, 1, 2, 1, 2, 2, 1],
    "ice": [0, 0, 1, 1, 1, 2, 2, 1, 1],
    "lightning": [0, 1, 2, 1, 2, 2, 1],
    "nature": [0, 1, 1, 2, 2, 2, 1, 1],
    "poison": [0, 1, 2, 1, 2, 3, 1],
    "water": [0, 1, 1, 2, 2, 2, 1],
    "holy": [0, 1, 1, 3, 1, 1, 0],
    "shadow": [0, 0, 1, 1, 2, 1, 1, 1],
}
METAL = {"": ((178, 186, 198), (112, 118, 132)), "fire": ((255, 170, 60), (220, 70, 30)), "ice": ((200, 240, 255), (120, 200, 240)),
         "lightning": ((150, 210, 255), (60, 110, 250)), "nature": ((120, 220, 90), (50, 140, 60)), "poison": ((170, 240, 80), (120, 60, 160)),
         "water": ((120, 220, 240), (40, 130, 200)), "holy": ((255, 236, 150), (230, 170, 60)), "shadow": ((190, 130, 255), (80, 40, 140))}
RANKS = ["common", "uncommon", "rare", "epic", "legendary", "unique", "divine"]


def palette_of(a):
    """A cor da madeira (a mais comum) e a de destaque (a mais saturada que aparece bastante) do arco."""
    px = (a[a[..., 3] > .5][:, :3] * 255).astype(int)
    q = (px // 24) * 24 + 12
    keys, counts = np.unique(q, axis=0, return_counts=True)
    order = np.argsort(-counts)
    body = keys[order[0]]
    sat = lambda col: max(col) - min(col)
    accent = max((keys[i] for i in order[:12]), key=lambda col: (sat(col), counts[list(map(tuple, keys)).index(tuple(col))]))
    if sat(accent) < 40:
        accent = keys[order[1]] if len(order) > 1 else body
    return tuple(int(x) for x in body), tuple(int(x) for x in accent)


def arrow_frame():
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(MODELS, "W_flecha.glb"))
    new = [o for o in bpy.data.objects if o not in before]
    mesh = [o for o in new if o.type == "MESH"][0]
    P = np.array([mesh.matrix_world @ vtx.co for vtx in mesh.data.vertices])
    for o in new:
        bpy.data.objects.remove(o, do_unlink=True)
    c = P.mean(0)
    _, _, Vt = np.linalg.svd(P - c, full_matrices=False)
    ax = Vt[0] if (P @ Vt[0]).mean() > 0 else -Vt[0]       # da ponta (a origem) para as penas
    t = P @ ax
    return ax, Vt[1], Vt[2], float(t.max() - max(0.0, t.min()))


def build_arrow(key, entry, a, frame, band=.8):
    ax, side1, side2, length = frame
    v = length / ARROW_ROWS
    A, S1, S2 = Vector(ax), Vector(side1), Vector(side2)
    element, rank = entry["element"], RANKS.index(entry["rarity"])
    body, accent = palette_of(a)
    tip, edge = METAL.get(element, METAL[""])
    head = HEADS.get(element, HEADS[""])
    if rank >= 4:
        head = head + [1]                     # as lendárias para cima: a ponta mais comprida
    head_glow = rank >= 2 and element != ""
    feather = accent if element else tuple(min(255, int(x * 1.15)) for x in accent)
    cells = {}

    def put(r, p, q, rgb, glow=False):
        cells[(r, p, q)] = (np.asarray(rgb) / 255.0, glow)
    n_head = len(head)
    for r, half in enumerate(head):
        for s in range(-half, half + 1):
            col = edge if abs(s) == half and half > 0 else tip
            put(r, s, 0, col, head_glow)
            put(r, 0, s, col, head_glow)
    # o colar entre a ponta e a haste (das incomuns para cima, na cor de destaque)
    shaft_from = n_head
    if rank >= 1:
        put(n_head, 0, 0, accent, rank >= 3)
        shaft_from += 1
    for r in range(shaft_from, ARROW_ROWS):
        put(r, 0, 0, body if r % 7 else tuple(int(x * band) for x in body))
    # as penas: em cruz, nas últimas linhas, mais altas atrás
    vane = [1, 1, 2, 2, 2, 3, 3, 2] if rank < 4 else [1, 2, 2, 3, 3, 3, 3, 2]
    start = ARROW_ROWS - len(vane) - 1
    for i, h in enumerate(vane):
        r = start + i
        for s in range(1, h + 1):
            col = feather if s < h else tuple(min(255, int(x * 1.25)) for x in feather)
            for p, q in ((s, 0), (-s, 0), (0, s), (0, -s)):
                put(r, p, q, col, rank >= 5 and s == h)
    put(ARROW_ROWS - 1, 0, 0, tuple(int(x * .6) for x in body))   # o encaixe da corda
    filled, colors = {}, {}
    for (r, p, q), val in cells.items():
        center = A * ((r + .5) * v) + S1 * (p * v) + S2 * (q * v)
        filled[(r, p, q)] = (center, (A, S1, S2))
        colors[(r, p, q)] = val
    obj, img, img_r = voxels_to_mesh("W_flecha_%s_blocos" % key, filled, colors, v)
    return obj, img, img_r, cells


def side_view(cells, scale=4):
    """A flecha de lado (para a conferência): a ponta à direita."""
    R = max(r for r, _, _ in cells) + 1
    out = np.zeros((9 * scale, R * scale, 4), np.float32)
    for (r, p, q), (rgb, glow) in cells.items():
        if q != 0:
            continue
        x, y = (R - 1 - r) * scale, (4 - p) * scale
        if 0 <= y < out.shape[0]:
            out[y:y + scale, x:x + scale, :3] = rgb
            out[y:y + scale, x:x + scale, 3] = 1
    return out


def save(obj, img, img_r):
    img.filepath_raw = os.path.join(TEXTURES, img.name + ".png")
    img.file_format = "PNG"
    img.save()
    img_r.filepath_raw = os.path.join(TEXTURES, img_r.name + ".png")
    img_r.file_format = "PNG"
    img_r.save()
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.gltf(filepath=os.path.join(MODELS, obj.name + ".glb"), use_selection=True, export_format="GLB",
                              export_normals=True, export_texcoords=True, export_materials="EXPORT", export_yup=True)
    bpy.data.objects.remove(obj, do_unlink=True)


def main():
    if bpy.app.background:
        bpy.ops.wm.read_factory_settings(use_empty=True)
    frame = block_weapons.frame(os.path.join(MODELS, "W_arco_de_betula.glb"))
    arrow = arrow_frame()
    os.makedirs(TEXTURES, exist_ok=True)
    bows = json.load(open(os.path.join(ROOT, "Data", "Rpg", "bows.json"), encoding="utf-8"))["bows"]
    views = []
    x = 0.0
    for item, entry in bows.items():
        key = entry["key"]
        if ONLY and key not in ONLY:
            continue
        a = load_grid(os.path.join(BOWS, key + ".png"))
        obj, img, img_r = build_bow(key, a, frame)
        print("PIXEL_BOW %-14s %5d faces  %.2f x %.2f x %.2f m" % (key, len(obj.data.polygons), *obj.dimensions))
        aobj, aimg, aimg_r, cells = build_arrow(key, entry, a, arrow)
        print("PIXEL_ARROW %-12s %5d faces  %.2f m" % (key, len(aobj.data.polygons), max(aobj.dimensions)))
        views.append(side_view(cells))
        if bpy.app.background:
            save(obj, img, img_r)
            save(aobj, aimg, aimg_r)
        else:
            obj.location.x = x
            aobj.location.x = x
            x += .5
    if views and bpy.app.background:
        W = max(v.shape[1] for v in views)
        sheet = np.zeros((sum(v.shape[0] + 4 for v in views), W, 4), np.float32)
        sheet[..., :3] = .24
        sheet[..., 3] = 1
        y = 0
        for v in views:
            part = sheet[y:y + v.shape[0], :v.shape[1]]
            m = v[..., 3:] > 0
            part[...] = np.where(m, v, part)
            y += v.shape[0] + 4
        im = bpy.data.images.new("_flechas", W, sheet.shape[0], alpha=True)
        im.pixels = sheet[::-1].ravel()
        im.filepath_raw = os.path.join(BOWS, "_flechas.png")
        im.file_format = "PNG"
        im.save()


if __name__ != "pixel_bows":   # Tools/Blender/MakeBetulaArrow.py usa as funções sem gerar os 35 arcos
    main()
