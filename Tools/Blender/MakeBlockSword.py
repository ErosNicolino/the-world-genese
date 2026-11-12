"""A Espada de Cinzas em blocos (02/11): pixel art em cubos, no estilo do herói de blocos.

A do Meshy tinha milhares de triângulos soltos e a pixel art espalhada num mosaico de ilhas pequenas: na Unreal ficava
escura, manchada e facetada. Esta é feita de cubos com UV limpo: cada face pega o centro do seu pixel na textura.

2ª versão (o usuário achou a 1ª "feia e simples"): 56 linhas em vez de 40 e volume de verdade.
- A lâmina tem o miolo mais grosso que o fio, e o sulco de brasa afundado no meio.
- A guarda é larga, com as pontas curvadas para a lâmina e um rubi no centro.
- O cabo é de couro trançado, entre dois anéis de ouro.
- O pomo é de ouro, com um rubi.
- A brasa e os rubis ficam num segundo material (MW_espada_blocos_brasa), que brilha no jogo.

Tamanho, ângulo e pegada copiados da espada do Meshy (o alcance do golpe sai do tamanho do modelo e a mão segura no
mesmo ponto): o eixo comprido, a ponta, a guarda e a origem na pegada.

blender -b --factory-startup -P Tools/Blender/MakeBlockSword.py -- <espada_meshy.glb> <saida.glb> <saida_cor.png> <saida_rough.png>
(no Blender aberto: exec deste arquivo com ARGS = [...] definido antes; deixa os objetos na cena para ver)
"""
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector

ROWS = 56          # da ponta (0) ao pomo (55)
COLS = 11          # colunas -5..5
PX = 4             # cada pixel da pixel art vira 4x4 na textura (a amostra cai no meio)
TEX_W, TEX_H = 64, 256   # potência de 2 (a de 36x160 saía pontilhada e borrada na Unreal)

# cores (sRGB)
C = {
    "E": (226, 233, 242),   # fio (o brilho)
    "S": (178, 188, 204),   # aço claro (o lado da luz)
    "M": (142, 152, 170),   # aço
    "D": (102, 108, 126),   # aço escuro (o lado da sombra)
    "K": (74, 78, 94),      # aço do talão (perto da guarda)
    "1": (255, 222, 122),   # brasa clara
    "2": (255, 156, 48),    # brasa
    "3": (228, 86, 30),     # brasa funda
    "4": (150, 42, 24),     # brasa apagando
    "I": (70, 72, 88),      # ferro da guarda
    "H": (108, 112, 130),   # ferro com luz
    "G": (242, 198, 84),    # ouro
    "g": (184, 128, 46),    # ouro escuro
    "h": (255, 234, 150),   # ouro com luz
    "L": (134, 80, 46),     # couro
    "W": (88, 52, 30),      # tira do couro
    "R": (226, 42, 64),     # rubi
    "r": (255, 132, 150),   # brilho do rubi
}
ROUGH = {"E": .62, "S": .7, "M": .74, "D": .78, "K": .8, "1": .85, "2": .85, "3": .85, "4": .85, "I": .78, "H": .72,
         "G": .55, "g": .6, "h": .5, "L": .9, "W": .92, "R": .4, "r": .35}
GLOW = set("1234Rr")   # no material que brilha


def design():
    """A pixel art (linha 0 = ponta) e a espessura (em cubos) de cada pixel: {(linha, coluna): (letra, espessura)}."""
    px = {}

    def put(r, k, ch, t):
        px[(r, k)] = (ch, t)

    # a ponta: estreita, abrindo de 2 em 2
    tip = ["E", "E", "SED", "SMD", "ESMDE", "ESMDE"]
    for r, line in enumerate(tip):
        half = len(line) // 2
        for i, ch in enumerate(line):
            k = i - half
            put(r, k, ch, 3 if (len(line) >= 5 and abs(k) <= 1) else 1)
    # a lâmina: fio claro, aço com luz à esquerda e sombra à direita, o miolo grosso e o sulco de brasa afundado
    for r in range(6, 36):
        groove = 9 <= r <= 33
        ember = "1232"[r % 4] if r % 7 else "4"
        put(r, -3, "E", 1)
        put(r, 3, "E", 1)
        put(r, -2, "S" if r % 9 else "E", 1)
        put(r, 2, "D" if r % 5 else "M", 1)
        put(r, -1, "S" if r % 6 else "M", 3)
        put(r, 1, "M" if r % 4 else "D", 3)
        put(r, 0, ember if groove else "M", 1 if groove else 3)
    # o talão: mais escuro e sem fio, logo acima da guarda
    for r in (36, 37):
        for k, ch in zip(range(-2, 3), "KDMDK"):
            put(r, k, ch, 3)
    # as pontas da guarda curvadas para a lâmina
    for r, ch in ((36, "g"), (37, "G")):
        put(r, -5, ch, 3)
        put(r, 5, ch, 3)
    # a guarda: ferro com as bordas de ouro e o rubi no meio
    guard = ["gHIIIhIIIHg", "GIIIgRgIIIG", "gIIIIgIIIIg"]
    for i, line in enumerate(guard):
        for j, ch in enumerate(line):
            put(38 + i, j - 5, ch, 5 if ch in "Rg" and abs(j - 5) <= 1 else 3)
    # o cabo: anel de ouro, couro trançado, anel de ouro
    for r in (41, 50):
        for k, ch in zip(range(-2, 3), "gGhGg"):
            put(r, k, ch, 5)
    for r in range(42, 50):
        for k, ch in zip(range(-1, 2), "LWL" if r % 2 else "WLW"):
            put(r, k, ch, 3)
    # o pomo: pescoço, o bloco de ouro com o rubi e o topo
    for k, ch in zip(range(-1, 2), "gGg"):
        put(51, k, ch, 3)
    for r, line in ((52, "GhRhG"), (53, "GRrRG"), (54, "gGRGg")):
        for k, ch in zip(range(-2, 3), line):
            put(r, k, ch, 5)
    for k, ch in zip(range(-1, 2), "ghg"):
        put(55, k, ch, 3)
    return px


def meshy_frame(path):
    """O eixo comprido (da ponta ao pomo), a largura e a espessura da espada do Meshy, e onde fica cada linha."""
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    mesh = [o for o in new if o.type == 'MESH'][0]
    P = np.array([mesh.matrix_world @ v.co for v in mesh.data.vertices])
    c = P.mean(0)
    _, _, Vt = np.linalg.svd(P - c, full_matrices=False)
    ax = Vt[0]
    t = (P - c) @ ax
    # a ponta é o lado estreito: o eixo aponta da ponta para o pomo
    near_lo = P[t < t.min() + .05]
    near_hi = P[t > t.max() - .05]
    if np.ptp((near_lo - c) @ Vt[1]) > np.ptp((near_hi - c) @ Vt[1]):
        ax, t = -ax, -t
    thin = Vt[2]
    wide = np.cross(thin, ax)
    return mesh, new, c, ax, wide, thin, t.min(), t.max()


def build(meshy_glb):
    src, imported, c, ax, wide, thin, t0, t1 = meshy_frame(meshy_glb)
    v = (t1 - t0) / ROWS
    W, H = TEX_W, TEX_H
    # sem transparência e com fundo preenchido: o mapa de rugosidade com fundo transparente saía 0 (espelho) na Unreal
    color = np.ones((H, W, 4), dtype=np.float32)
    color[..., :3] = .3
    rough = np.ones((H, W, 4), dtype=np.float32)
    rough[..., :3] = .8
    filled = {}
    for (r, k), (ch, n) in design().items():
        col = k + COLS // 2
        y0 = (ROWS - 1 - r) * PX     # a imagem cresce para baixo; o Blender lê de baixo para cima
        color[y0:y0 + PX, col * PX:(col + 1) * PX, :3] = np.array(C[ch]) / 255.0
        rough[y0:y0 + PX, col * PX:(col + 1) * PX, :3] = ROUGH[ch]
        for d in range(n):
            filled[(r, k, d - n // 2)] = (col, r, 1 if ch in GLOW else 0)

    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    A, Wd, T = Vector(ax), Vector(wide), Vector(thin)
    base = Vector(c)
    dirs = [(0, 1, 0), (0, -1, 0), (1, 0, 0), (-1, 0, 0), (0, 0, 1), (0, 0, -1)]
    for (r, k, d), (col, row, slot) in filled.items():
        center = base + A * (t0 + (r + .5) * v) + Wd * (k * v) + T * (d * v)
        u = (col + .5) * PX / W
        vv = (ROWS - 1 - row + .5) * PX / H
        for dr, dk, dd in dirs:
            if (r + dr, k + dk, d + dd) in filled:
                continue
            n = A * dr + Wd * dk + T * dd
            if dr:
                e1, e2 = Wd, T
            elif dk:
                e1, e2 = T, A
            else:
                e1, e2 = A, Wd
            fc = center + n * (v / 2)
            corners = [fc + (e1 * a + e2 * b) * (v / 2) for a, b in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
            if (e1.cross(e2)).dot(n) < 0:
                corners.reverse()
            face = bm.faces.new([bm.verts.new(p) for p in corners])
            face.material_index = slot
            for loop in face.loops:
                loop[uv].uv = (u, vv)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=v * 1e-3)
    me = bpy.data.meshes.new("W_espada_de_cinzas_blocos")
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new("W_espada_de_cinzas_blocos", me)
    bpy.context.scene.collection.objects.link(obj)
    for p in me.polygons:
        p.use_smooth = False

    img = bpy.data.images.new("T_espada_blocos", W, H, alpha=True)
    img.pixels = color.ravel()
    img_r = bpy.data.images.new("T_espada_blocos_rough", W, H, alpha=True)
    # o espaço de cor antes dos pixels: trocar depois apagava a imagem (o PNG saía todo 0, um espelho)
    img_r.colorspace_settings.name = "Non-Color"
    img_r.pixels = rough.ravel()
    for name, glow in (("MW_espada_blocos", 0.0), ("MW_espada_blocos_brasa", 3.0)):
        mat = bpy.data.materials.new(name)
        mat.use_nodes = True
        nt = mat.node_tree
        bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
        tex = nt.nodes.new("ShaderNodeTexImage")
        tex.image = img
        tex.interpolation = "Closest"
        nt.links.new(tex.outputs[0], bsdf.inputs["Base Color"])
        bsdf.inputs["Metallic"].default_value = 0.0
        bsdf.inputs["Roughness"].default_value = .7
        if glow:
            nt.links.new(tex.outputs[0], bsdf.inputs["Emission Color"])
            bsdf.inputs["Emission Strength"].default_value = glow
        me.materials.append(mat)
    return obj, src, imported, img, img_r


def export(obj, out_glb, img, img_r, out_png, out_rough):
    img.filepath_raw = out_png
    img.file_format = "PNG"
    img.save()
    img_r.filepath_raw = out_rough
    img_r.file_format = "PNG"
    img_r.save()
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.gltf(filepath=out_glb, use_selection=True, export_format="GLB", export_normals=True,
                              export_texcoords=True, export_materials="EXPORT", export_yup=True)


if __name__ == "__main__":
    args = globals().get("ARGS") or sys.argv[sys.argv.index("--") + 1:]
    if "--" in sys.argv:
        bpy.ops.wm.read_factory_settings(use_empty=True)
    obj, src, imported, img, img_r = build(args[0])
    if len(args) >= 4:
        export(obj, args[1], img, img_r, args[2], args[3])
    print("BLOCK_SWORD faces", len(obj.data.polygons), "dims", tuple(round(x, 3) for x in obj.dimensions))
