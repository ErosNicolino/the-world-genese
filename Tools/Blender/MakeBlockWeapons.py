"""As outras armas iniciais em blocos (02/11), no estilo da Espada de Cinzas (Tools/Blender/MakeBlockSword.py): pixel art
em cubos, com volume, UV limpo e um segundo material que brilha (gemas, runas, cristal).

- adagas_gemeas (e adagas_gemeas_L): lâmina curta com runas violeta, guarda de prata com gema e cabo trançado;
- martelo_de_bram: cabo de carvalho com couro na pegada, cabeça de ferro larga com faixas de ouro e uma runa de brasa;
- cajado_de_ilsa: haste de bétula, galhos que seguram um cristal de jade aceso, folhas;
- arco_de_betula: arco curvo de bétula (casca branca com riscos pretos), pegada de couro e pontas de ouro.

Cada uma no tamanho, no ângulo e na pegada do modelo do Meshy (lidos do arquivo dele: o eixo comprido, a ponta estreita
na linha 0 e a origem onde a mão segura), então o alcance e a mão não mudam.

blender -b --factory-startup -P Tools/Blender/MakeBlockWeapons.py -- <raiz do projeto> [arma ...]
Sai Data/VortexWeapons/Models/W_<arma>_blocos.glb e Textures/Novas/T_<arma>_blocos(_rough).png. Na Unreal:
Tools/ImportBlockSword.py (VORTEX_WEAPONS=todas).
"""
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector

PX = 4
TEX_W, TEX_H = 128, 512

C = {
    "E": (226, 233, 242), "S": (178, 188, 204), "M": (142, 152, 170), "D": (102, 108, 126), "K": (74, 78, 94),
    "1": (255, 222, 122), "2": (255, 156, 48), "3": (228, 86, 30), "4": (150, 42, 24),
    "I": (70, 72, 88), "H": (108, 112, 130), "G": (242, 198, 84), "g": (184, 128, 46), "h": (255, 234, 150),
    "L": (134, 80, 46), "W": (88, 52, 30), "P": (52, 40, 58), "p": (34, 26, 40),
    "B": (226, 220, 204), "b": (58, 52, 50), "O": (128, 86, 50), "o": (92, 60, 36),
    "V": (168, 96, 255), "v": (214, 176, 255), "J": (54, 214, 168), "j": (176, 255, 224),
    "F": (96, 172, 72), "f": (60, 122, 50),
}
ROUGH = {"E": .62, "S": .7, "M": .74, "D": .78, "K": .8, "I": .78, "H": .72, "G": .55, "g": .6, "h": .5,
         "L": .9, "W": .92, "P": .9, "p": .92, "B": .85, "b": .9, "O": .88, "o": .9, "F": .85, "f": .88}
GLOW = set("1234VvJj")


def tone_rows(r, a, b):
    return a if r % 2 else b


# ---- os desenhos: {(linha, coluna): (letra, espessura)}; a linha 0 é a ponta estreita do modelo do Meshy

def adagas(info):
    px = {}
    rows = info["rows"]

    def put(r, k, ch, t):
        px[(r, k)] = (ch, t)
    for r, line in enumerate(["E", "E", "SED", "SMD"]):
        for i, ch in enumerate(line):
            put(r, i - len(line) // 2, ch, 1)
    for r in range(4, 24):
        rune = 6 <= r <= 21 and r % 4 == 1
        put(r, -2, "E", 1); put(r, 2, "E", 1)
        put(r, -1, "S" if r % 5 else "M", 1); put(r, 1, "D" if r % 3 else "M", 1)
        put(r, 0, "V" if rune else "M", 3)
    for k, ch in zip(range(-2, 3), "KDMDK"):
        put(24, k, ch, 3)
    for i, line in enumerate(["gIIIVIIIg", "GIIIgIIIG"]):
        for j, ch in enumerate(line):
            put(25 + i, j - 4, ch, 5 if ch == "V" else 3)
    for r in range(27, rows - 3):
        for k, ch in zip(range(-1, 2), tone_rows(r, "PpP", "pPp")):
            put(r, k, ch, 3)
    put(rows - 3, -1, "g", 3); put(rows - 3, 0, "G", 3); put(rows - 3, 1, "g", 3)
    for k, ch in zip(range(-2, 3), "gGVGg"):
        put(rows - 2, k, ch, 5)
    put(rows - 1, -1, "g", 3); put(rows - 1, 0, "h", 3); put(rows - 1, 1, "g", 3)
    return px


def martelo(info):
    px = {}
    rows, grip = info["rows"], info["grip_row"]

    def put(r, k, ch, t):
        px[(r, k)] = (ch, t)
    for r, line in ((0, "IHI"), (1, "HHHHH"), (2, "IHHHI")):
        for i, ch in enumerate(line):
            put(r, i - len(line) // 2, ch, 5)
    head0 = rows - 17
    for r in range(3, head0 - 2):
        wrap = abs(r - grip) <= 7
        line = tone_rows(r, "LWL", "WLW") if wrap else ("OoO" if r % 5 else "oOo")
        for k, ch in zip(range(-1, 2), line):
            put(r, k, ch, 3)
    for r in (grip - 8, grip + 8):
        for k, ch in zip(range(-2, 3), "gGhGg"):
            put(r, k, ch, 5)
    for r in range(head0 - 2, head0):
        for k, ch in zip(range(-2, 3), "IHHHI"):
            put(r, k, ch, 5)
    # a cabeça: um bloco largo de ferro (as quinas chanfradas), duas faixas de ouro e a runa de brasa no meio
    for r in range(head0, rows):
        i = r - head0
        half = 12 if i in (0, 16) else 13
        for k in range(-half, half + 1):
            ch = "H" if i < 3 else "K" if abs(k) >= half - 1 else "I"
            if i in (3, 13):
                ch = "G" if abs(k) % 3 else "g"
            if abs(k) <= 2 and 6 <= i <= 10 and abs(k) + abs(i - 8) <= 2:
                ch = "2" if abs(k) + abs(i - 8) < 2 else "3"
            put(r, k, ch, 11 if abs(k) < half else 9)
    return px


def cajado(info):
    px = {}
    rows, grip = info["rows"], info["grip_row"]

    def put(r, k, ch, t):
        if (r, k) not in px or t >= px[(r, k)][1]:
            px[(r, k)] = (ch, t)
    for k, ch in zip(range(-1, 2), "IHI"):
        put(0, k, ch, 3)
        put(1, k, ch, 3)
    top = rows - 25
    for r in range(2, top + 1):
        wrap = abs(r - grip) <= 4
        bark = "B" if (r * 7) % 9 else "b"
        line = tone_rows(r, "LWL", "WLW") if wrap else ("B" + bark + "B")
        for k, ch in zip(range(-1, 2), line):
            put(r, k, ch, 3)
    for r in (grip - 5, grip + 5):
        for k, ch in zip(range(-2, 3), "gGhGg"):
            put(r, k, ch, 5)
    # dois galhos que abrem e fecham em volta do cristal
    for r in range(top, rows):
        a = (r - top) / (rows - 1 - top)
        off = round(7 * math.sin(math.pi * a))
        for s in (-1, 1):
            for w in (0, 1):
                put(r, s * (off + w), "b" if (r + w) % 4 == 0 else "B", 3)
        if r in (top + 6, top + 14, rows - 3):
            for s in (-1, 1):
                put(r, s * (off + 2), "F", 3)
                put(r + 1, s * (off + 2), "f", 3)
    # o cristal de jade: um losango aceso, mais claro na diagonal de cima
    mid, hr, hw = top + 13, 8, 4
    for r in range(mid - hr, mid + hr + 1):
        for k in range(-hw, hw + 1):
            if abs(r - mid) / hr + abs(k) / hw <= 1.0:
                put(r, k, "j" if (k - (r - mid)) in (-1, 0) and r > mid - 5 else "J", 5)
    return px


def arco(info):
    px = {}
    rows = info["rows"]
    depth, sign = info["bow_depth"], info["bow_sign"]

    def put(r, k, ch, t):
        px[(r, k)] = (ch, t)
    mid = (rows - 1) / 2
    for r in range(rows):
        x = (r - mid) / mid
        col = sign * round(depth * (1 - x * x))
        tip = r < 2 or r > rows - 3
        grip = abs(r - mid) <= 4
        width = 3 if grip else 2
        for w in range(width):
            k = col - sign * w
            if tip:
                ch = "G" if w == 0 else "g"
            elif grip:
                ch = tone_rows(r, "L", "W")
            else:
                ch = "b" if (r * 5 + w * 3) % 7 == 0 else "B"
            put(r, k, ch, 3 if not tip else 3)
    return px


WEAPONS = {
    "adagas_gemeas": ("W_adagas_gemeas.glb", .016, adagas),
    "adagas_gemeas_L": ("W_adagas_gemeas_L.glb", .016, adagas),
    "martelo_de_bram": ("W_martelo_de_bram.glb", .0172, martelo),
    "cajado_de_ilsa": ("W_cajado_de_ilsa.glb", .0183, cajado),
    "arco_de_betula": ("W_arco_de_betula.glb", .0175, arco),
}


def frame(path):
    """O eixo comprido (da ponta estreita para a larga), a largura, a espessura, onde a mão segura e a curva do arco."""
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    mesh = [o for o in new if o.type == 'MESH'][0]
    P = np.array([mesh.matrix_world @ v.co for v in mesh.data.vertices])
    for o in new:
        bpy.data.objects.remove(o)
    c = P.mean(0)
    _, _, Vt = np.linalg.svd(P - c, full_matrices=False)
    ax = Vt[0]
    t = (P - c) @ ax
    if np.ptp((P[t < t.min() + .05] - c) @ Vt[1]) > np.ptp((P[t > t.max() - .05] - c) @ Vt[1]):
        ax, t = -ax, -t
    thin = Vt[2]
    wide = np.cross(thin, ax)
    L = t.max() - t.min()
    ends = (t < t.min() + L * .08) | (t > t.max() - L * .08)
    middle = np.abs(t - (t.min() + t.max()) / 2) < L * .08

    def bend_along(axis):
        w = (P - c) @ axis
        return w[middle].mean() - w[ends].mean()
    bend = max(abs(bend_along(wide)), abs(bend_along(thin)))
    # para o arco: o lado para onde o meio (a pegada) se projeta no modelo do Meshy, que está certo no jogo
    bend_side = 1.0 if bend_along(wide) >= 0 else -1.0
    return c, ax, wide, thin, t.min(), t.max(), float((-c) @ ax), bend, float((-c) @ wide), bend_side


def build(root, name):
    glb, voxel, design = WEAPONS[name]
    c, ax, wide, thin, t0, t1, origin, bend, origin_w, bend_side = frame(os.path.join(root, "Data", "VortexWeapons", "Models", glb))
    rows = max(8, round((t1 - t0) / voxel))
    v = (t1 - t0) / rows
    info = {"rows": rows, "grip_row": int((origin - t0) / v), "bow_depth": int(abs(bend) / v), "bow_sign": 1}
    px = design(info)
    cols = [k for (_, k) in px]
    k0 = min(cols)
    assert (max(cols) - k0 + 1) * PX <= TEX_W and rows * PX <= TEX_H, (name, rows, max(cols) - k0 + 1)
    color = np.ones((TEX_H, TEX_W, 4), dtype=np.float32)
    color[..., :3] = .3
    rough = np.ones((TEX_H, TEX_W, 4), dtype=np.float32)
    rough[..., :3] = .8
    filled = {}
    for (r, k), (ch, n) in px.items():
        col = k - k0
        # a linha r da arte na linha r da imagem (de cima para baixo)
        color[r * PX:(r + 1) * PX, col * PX:(col + 1) * PX, :3] = np.array(C[ch]) / 255.0
        rough[r * PX:(r + 1) * PX, col * PX:(col + 1) * PX, :3] = ROUGH.get(ch, .8)
        for d in range(n):
            filled[(r, k, d - n // 2)] = (col, r, 1 if ch in GLOW else 0)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    A, Wd, T = Vector(ax), Vector(wide), Vector(thin)
    if design is arco:
        # o arco (02/11): a curva no mesmo plano e para o mesmo lado do modelo do Meshy (o meio para a frente, a corda
        # para o herói; no outro plano ele ficava "de lado") e o meio do arco (a pegada) exatamente na mão (a origem);
        # as pontas ficam na coluna 0
        Wd = Wd * bend_side
        origin_w = float(Vector(-np.array(c)).dot(Wd))
        base = Vector(c) + Wd * (origin_w - info["bow_sign"] * info["bow_depth"] * v)
    else:
        base = Vector(c)
    for (r, k, d), (col, row, slot) in filled.items():
        center = base + A * (t0 + (r + .5) * v) + Wd * (k * v) + T * (d * v)
        u = (col + .5) * PX / TEX_W
        # o V do Blender conta de baixo para cima
        vv = 1 - (row + .5) * PX / TEX_H
        for dr, dk, dd in ((0, 1, 0), (0, -1, 0), (1, 0, 0), (-1, 0, 0), (0, 0, 1), (0, 0, -1)):
            if (r + dr, k + dk, d + dd) in filled:
                continue
            nrm = A * dr + Wd * dk + T * dd
            e1, e2 = (Wd, T) if dr else (T, A) if dk else (A, Wd)
            fc = center + nrm * (v / 2)
            corners = [fc + (e1 * a + e2 * b) * (v / 2) for a, b in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
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
    # a imagem do numpy é de cima para baixo; o Blender guarda de baixo para cima
    img = bpy.data.images.new("T_%s_blocos" % name, TEX_W, TEX_H, alpha=True)
    img.pixels = color[::-1].ravel()
    img_r = bpy.data.images.new("T_%s_blocos_rough" % name, TEX_W, TEX_H, alpha=True)
    img_r.colorspace_settings.name = "Non-Color"   # antes dos pixels: trocar depois apaga a imagem
    img_r.pixels = rough[::-1].ravel()
    for mname, glow in (("MW_%s_blocos" % name, 0.0), ("MW_%s_blocos_brasa" % name, 3.0)):
        mat = bpy.data.materials.new(mname)
        mat.use_nodes = True
        nt = mat.node_tree
        bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
        tex = nt.nodes.new("ShaderNodeTexImage")
        tex.image = img
        tex.interpolation = "Closest"
        nt.links.new(tex.outputs[0], bsdf.inputs["Base Color"])
        bsdf.inputs["Roughness"].default_value = .7
        if glow:
            nt.links.new(tex.outputs[0], bsdf.inputs["Emission Color"])
            bsdf.inputs["Emission Strength"].default_value = glow
        me.materials.append(mat)
    print("BLOCK_WEAPON", name, "linhas", rows, "pegada", info["grip_row"], "faces", len(me.polygons),
          "dims", tuple(round(x, 3) for x in obj.dimensions))
    return obj, img, img_r


def export(root, name, obj, img, img_r):
    tex = os.path.join(root, "Data", "VortexWeapons", "Textures", "Novas")
    img.filepath_raw = os.path.join(tex, "T_%s_blocos.png" % name)
    img.file_format = "PNG"
    img.save()
    img_r.filepath_raw = os.path.join(tex, "T_%s_blocos_rough.png" % name)
    img_r.file_format = "PNG"
    img_r.save()
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.gltf(filepath=os.path.join(root, "Data", "VortexWeapons", "Models", "W_%s_blocos.glb" % name),
                              use_selection=True, export_format="GLB", export_normals=True, export_texcoords=True,
                              export_materials="EXPORT", export_yup=True)


if __name__ == "__main__":
    args = globals().get("ARGS") or sys.argv[sys.argv.index("--") + 1:]
    root, names = args[0], args[1:] or list(WEAPONS)
    headless = "--" in sys.argv
    if headless:
        bpy.ops.wm.read_factory_settings(use_empty=True)
    for n in names:
        obj, img, img_r = build(root, n)
        if headless:
            export(root, n, obj, img, img_r)
