"""As 35 espadas das pranchas do usuário em blocos (07/11), do Normal ao Divino.

A grade de pixels de cada uma (Tools/Weapons/SliceSwordSheets.py -> Data/VortexWeapons/Swords/<raridade>_<n>.png) vira
cubos, na mesma moldura da espada de blocos do jogo (Tools/Blender/MakeBlockSword.py: o eixo, o tamanho do bloco e a
pegada da espada do Meshy), então todas encaixam na mão igual e o alcance do golpe continua o mesmo:
- a guarda de cada espada (a linha mais larga da metade de baixo) fica na linha da guarda da espada atual (39) e o meio
  do cabo no eixo;
- a espessura: a beira da lâmina com 1 bloco, o resto com 3 (a guarda, o cabo e o pomo inteiros com 3);
- as partes claras e saturadas (gemas, runas, fogo, cristal) vão para o material que brilha (MW_<nome>_brasa).

blender -b --factory-startup -P Tools/Blender/MakePixelSwords.py -- <raiz> [raridade_n ...]
Sai Data/VortexWeapons/Models/W_espada_<raridade>_<n>_blocos.glb e Textures/Novas/T_espada_<raridade>_<n>_blocos(_rough).png
(Tools/ImportBlockSword.py leva para a Unreal). No Blender aberto: ARGS = [raiz, nomes...]; exec deste arquivo.
"""
import colorsys
import glob
import importlib.util
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector

ARGS = globals().get("ARGS") or (sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
ROOT = ARGS[0] if ARGS else r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
ONLY = ARGS[1:]
SWORDS = os.path.join(ROOT, "Data", "VortexWeapons", "Swords")
MODELS = os.path.join(ROOT, "Data", "VortexWeapons", "Models")
TEXTURES = os.path.join(ROOT, "Data", "VortexWeapons", "Textures", "Novas")
MESHY = os.path.join(MODELS, "W_espada_de_cinzas.glb")
GUARD_ROW = 39          # a linha da guarda na espada de blocos do jogo (MakeBlockSword.design)
BASE_ROWS = 56          # o tamanho do bloco = o comprimento da espada do Meshy / 56
PX = 4                  # cada pixel vira 4x4 na textura (a amostra cai no meio)

spec = importlib.util.spec_from_file_location("block_sword", os.path.join(ROOT, "Tools", "Blender", "MakeBlockSword.py"))
block_sword = importlib.util.module_from_spec(spec)
spec.loader.exec_module(block_sword)


def load_grid(path):
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    a = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1]     # linha 0 = em cima (a ponta)
    bpy.data.images.remove(img)
    return a


# 07/11 (o usuário: "uma espada enrolada que está segurando no meio"): onde a linha mais larga não é a guarda. A Sopro de
# Valdora tem a fita dourada mais larga que a guarda no meio da lâmina; a Ceifadora do Vazio, o anel acima do cabo; o
# Suspiro de Azhûr, a lâmina mais larga logo acima da gema. O número é a última linha antes do cabo.
GUARD_OVERRIDE = {"lendario_4": 49, "epico_5": 49, "lendario_1": 47}


def plan(a, name=""):
    """{(linha, coluna): (cor, espessura, acende)} já na moldura da espada do jogo (linha da guarda = 39)."""
    filled = a[..., 3] > .5
    R, C = filled.shape
    widths = filled.sum(axis=1)
    lower = range(int(R * .45), R)
    guard = GUARD_OVERRIDE.get(name) or max(lower, key=lambda r: (widths[r], -r))
    handle_rows = [r for r in range(guard + 2, min(R, guard + 9)) if widths[r]]
    cols = np.nonzero(filled[handle_rows].any(axis=0))[0] if handle_rows else np.nonzero(filled[guard])[0]
    center = int(round(cols.mean())) if len(cols) else C // 2
    blade_w = np.median([w for w in widths[:guard] if w]) if guard else 3
    out = {}
    for r in range(R):
        for c in range(C):
            if not filled[r, c]:
                continue
            rgb = a[r, c, :3]
            hh, ss, vv = colorsys.rgb_to_hsv(*rgb)
            wide_row = widths[r] > blade_w * 1.4 or r >= guard
            edge = not (0 < c < C - 1 and filled[r, c - 1] and filled[r, c + 1])
            thick = 3 if wide_row or not edge else 1
            glow = vv > .86 and ss > .5
            out[(r - guard + GUARD_ROW, c - center)] = (rgb, thick, glow)
    return out, R, C


def build(name, a, frame):
    c, ax, wide, thin, t0, t1 = frame
    v = (t1 - t0) / BASE_ROWS
    px, R, C = plan(a, name.split("espada_")[-1].replace("_blocos", "") if "espada_" in name else name)
    W = 1 << max(5, int(np.ceil(np.log2(C * PX))))
    H = 1 << max(6, int(np.ceil(np.log2(R * PX))))
    color = np.ones((H, W, 4), np.float32)
    color[..., :3] = .3
    rough = np.ones((H, W, 4), np.float32)
    rough[..., :3] = .8
    rows = sorted({r for r, _ in px})
    r_min = rows[0]
    k_min = min(k for _, k in px)
    filled = {}
    for (r, k), (rgb, n, glow) in px.items():
        row, col = r - r_min, k - k_min
        y0 = H - (row + 1) * PX            # a imagem do Blender cresce para cima
        color[y0:y0 + PX, col * PX:(col + 1) * PX, :3] = rgb
        sat = max(rgb) - min(rgb)
        rough[y0:y0 + PX, col * PX:(col + 1) * PX, :3] = .55 if sat < .12 else .75
        for d in range(n):
            filled[(r, k, d - n // 2)] = (col, row, 1 if glow else 0)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    A, Wd, T = Vector(ax), Vector(wide), Vector(thin)
    base = Vector(c)
    for (r, k, d), (col, row, slot) in filled.items():
        center = base + A * (t0 + (r + .5) * v) + Wd * (k * v) + T * (d * v)
        u = (col + .5) * PX / W
        vv = 1 - (row + .5) * PX / H
        for dr, dk, dd in ((0, 1, 0), (0, -1, 0), (1, 0, 0), (-1, 0, 0), (0, 0, 1), (0, 0, -1)):
            if (r + dr, k + dk, d + dd) in filled:
                continue
            nrm = A * dr + Wd * dk + T * dd
            e1, e2 = (Wd, T) if dr else (T, A) if dk else (A, Wd)
            fc = center + nrm * (v / 2)
            corners = [fc + (e1 * p + e2 * q) * (v / 2) for p, q in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
            if (e1.cross(e2)).dot(nrm) < 0:
                corners.reverse()
            face = bm.faces.new([bm.verts.new(p) for p in corners])
            face.material_index = slot
            for loop in face.loops:
                loop[uv].uv = (u, vv)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=v * 1e-3)
    me = bpy.data.meshes.new("W_%s_blocos" % name)
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new("W_%s_blocos" % name, me)
    bpy.context.scene.collection.objects.link(obj)
    img = bpy.data.images.new("T_%s_blocos" % name, W, H, alpha=True)
    img.pixels = color.ravel()
    img_r = bpy.data.images.new("T_%s_blocos_rough" % name, W, H, alpha=True)
    img_r.colorspace_settings.name = "Non-Color"
    img_r.pixels = rough.ravel()
    for mname, strength in (("MW_%s_blocos" % name, 0.0), ("MW_%s_blocos_brasa" % name, 3.0)):
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


def main():
    if bpy.app.background:
        bpy.ops.wm.read_factory_settings(use_empty=True)
    src, imported, c, ax, wide, thin, t0, t1 = block_sword.meshy_frame(MESHY)
    for o in imported:
        bpy.data.objects.remove(o, do_unlink=True)
    frame = (c, ax, wide, thin, t0, t1)
    os.makedirs(TEXTURES, exist_ok=True)
    names = sorted(os.path.splitext(os.path.basename(p))[0] for p in glob.glob(os.path.join(SWORDS, "*_*.png")) if not os.path.basename(p).startswith("_"))
    x = 0.0
    for key in names:
        if ONLY and key not in ONLY:
            continue
        name = "espada_" + key
        obj, img, img_r = build(name, load_grid(os.path.join(SWORDS, key + ".png")), frame)
        print("PIXEL_SWORD %-22s %5d faces  %.2f x %.2f x %.2f m" % (name, len(obj.data.polygons), *obj.dimensions))
        if bpy.app.background:
            img.filepath_raw = os.path.join(TEXTURES, "T_%s_blocos.png" % name)
            img.file_format = "PNG"
            img.save()
            img_r.filepath_raw = os.path.join(TEXTURES, "T_%s_blocos_rough.png" % name)
            img_r.file_format = "PNG"
            img_r.save()
            bpy.ops.object.select_all(action="DESELECT")
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            bpy.ops.export_scene.gltf(filepath=os.path.join(MODELS, "W_%s_blocos.glb" % name), use_selection=True, export_format="GLB",
                                      export_normals=True, export_texcoords=True, export_materials="EXPORT", export_yup=True)
            bpy.data.objects.remove(obj, do_unlink=True)
        else:
            obj.location.x = x
            x += .35


main()
