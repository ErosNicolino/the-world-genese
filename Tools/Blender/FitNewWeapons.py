"""As 5 armas novas (Meshy, 29/10) no lugar das antigas, na mesma pegada da mão.

blender -b --factory-startup -P Tools/Blender/FitNewWeapons.py -- <raiz do projeto>

Entrada: Data/Modelos/Armas/<tipo>/*.fbx (+ as texturas). Referência: Data/VortexWeapons/Models/_antigas/W_<tipo>.glb
(a arma antiga, já na mão do herói: a pegada na origem). Cada arma nova (em pé, o cabo embaixo) vai para o eixo
comprido da antiga, com o mesmo comprimento e a pegada no mesmo ponto, então os golpes e as posturas continuam iguais.

Saída:
- Data/VortexWeapons/Models/W_<tipo>.glb (e W_adagas_gemeas_L.glb), material "MWN_<tipo>";
- Data/VortexWeapons/Textures/Novas/T_N_<tipo>{,_normal,_rough}.png (2048 / 1024 px) e metallic.json;
- o arco de ametista vira W_arco_ametista.glb (arco extra, sem a corda) e W_flecha.glb (a flecha, deitada no +X, a ponta na frente).
O arco não tem corda no modelo: a corda é feita na Unreal (VortexWeaponParts).
"""
import json
import os
import sys

import bpy
import bmesh
import mathutils
from mathutils import Matrix, Vector

ROOT = sys.argv[-1] if len(sys.argv) > 1 and os.path.isdir(sys.argv[-1]) else os.getcwd()
SRC = os.path.join(ROOT, "Data", "Modelos", "Armas")
OLD = os.path.join(ROOT, "Data", "VortexWeapons", "Models", "_antigas")
OUT = os.path.join(ROOT, "Data", "VortexWeapons", "Models")
TEX = os.path.join(ROOT, "Data", "VortexWeapons", "Textures", "Novas")
# tipo -> (pasta do modelo novo, referências antigas)
WEAPONS = {
    "espada_de_cinzas": ["W_espada_de_cinzas"],
    "martelo_de_bram": ["W_martelo_de_bram"],
    "adagas_gemeas": ["W_adagas_gemeas", "W_adagas_gemeas_L"],
    "cajado_de_ilsa": ["W_cajado_de_ilsa"],
    "arco_de_betula": ["W_arco_de_betula"],
}
report = []


def clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def world_points(objs):
    pts = []
    for o in objs:
        m = o.matrix_world
        pts += [m @ v.co for v in o.data.vertices]
    return pts


def frame(points, grip_at_origin=True):
    """Eixo comprido (a), eixo da largura (w) e o alcance para cada lado da origem ao longo de a."""
    n = len(points)
    c = sum(points, Vector()) / n
    cov = Matrix(((0, 0, 0), (0, 0, 0), (0, 0, 0)))
    for p in points:
        d = p - c
        for i in range(3):
            for j in range(3):
                cov[i][j] += d[i] * d[j]
    import numpy
    vals, vecs = numpy.linalg.eigh(numpy.array(cov))
    a = Vector(vecs[:, 2]).normalized()
    w = Vector(vecs[:, 1]).normalized()
    proj = [p.dot(a) for p in points]
    lo, hi = min(proj), max(proj)
    if -lo > hi:                       # a ponta é o lado mais longe da pegada (a origem)
        a = -a
        lo, hi = -hi, -lo
    # o lado da largura para onde as pontas vão (o arco: as pontas curvam para a corda)
    tips = [p for p in points if p.dot(a) > hi - (hi - lo) * .12] + [p for p in points if p.dot(a) < lo + (hi - lo) * .12]
    if tips:
        side = sum(((p - a * p.dot(a)).dot(w) for p in tips)) / len(tips) - sum(((p - a * p.dot(a)).dot(w) for p in points)) / n
        if side < 0:
            w = -w
    return a, w, lo, hi


def import_new(kind):
    folder = os.path.join(SRC, kind)
    fbx = next(f for f in os.listdir(folder) if f.lower().endswith(".fbx"))
    bpy.ops.import_scene.fbx(filepath=os.path.join(folder, fbx))
    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    for o in bpy.data.objects:
        if o.type != "MESH":
            o.select_set(False)
    bpy.ops.object.select_all(action="DESELECT")
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    o = bpy.context.view_layer.objects.active
    o.parent = None
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    return o, folder


def textures(kind, folder, tag):
    """Reduz as texturas do Meshy (8K) e devolve o metálico médio."""
    os.makedirs(TEX, exist_ok=True)
    pngs = [f for f in os.listdir(folder) if f.lower().endswith(".png")]
    metallic = 0.0
    for f in pngs:
        img = bpy.data.images.load(os.path.join(folder, f))
        low = f.lower()
        if low.endswith("_metallic.png"):
            px = img.pixels[:]
            step = max(4, (len(px) // 4 // 20000) * 4)
            vals = px[0::step]
            metallic = sum(vals) / max(1, len(vals))
            continue
        name, size = ("T_N_%s_normal" % tag, 1024) if low.endswith("_normal.png") else \
            ("T_N_%s_rough" % tag, 1024) if low.endswith("_roughness.png") else ("T_N_%s" % tag, 2048)
        img.scale(size, size)
        img.filepath_raw = os.path.join(TEX, name + ".png")
        img.file_format = "PNG"
        img.save()
    return metallic


def place(o, ref_path, grip_mid=False):
    """Põe o modelo novo (em pé, +Z) no eixo da arma antiga, com o mesmo comprimento e a pegada no mesmo ponto."""
    bpy.ops.import_scene.gltf(filepath=ref_path)
    refs = [r for r in bpy.context.selected_objects if r.type == "MESH"]
    a, w, lo, hi = frame(world_points(refs))
    for r in list(bpy.context.selected_objects):
        bpy.data.objects.remove(r, do_unlink=True)
    pts = [v.co.copy() for v in o.data.vertices]
    zmin = min(p.z for p in pts)
    zmax = max(p.z for p in pts)
    length = zmax - zmin
    scale = (hi - lo) / length
    grip_frac = -lo / (hi - lo)
    grip = Vector((0, 0, zmin + grip_frac * length))
    # largura do modelo novo: o eixo X ou Y, o mais largo; o lado das pontas igual ao da antiga
    xs = max(p.x for p in pts) - min(p.x for p in pts)
    ys = max(p.y for p in pts) - min(p.y for p in pts)
    nw = Vector((1, 0, 0)) if xs >= ys else Vector((0, 1, 0))
    tips = [p for p in pts if p.z > zmax - length * .12 or p.z < zmin + length * .12]
    if tips and sum(p.dot(nw) for p in tips) / len(tips) - sum(p.dot(nw) for p in pts) / len(pts) < 0:
        nw = -nw
    nz = Vector((0, 0, 1))
    nt = nz.cross(nw)
    t = a.cross(w)
    src = Matrix((nw, nt, nz)).transposed()
    dst = Matrix((w, t, a)).transposed()
    rot = dst @ src.inverted()
    M = Matrix.Translation(Vector()) @ rot.to_4x4() @ Matrix.Scale(scale, 4) @ Matrix.Translation(-grip)
    o.data.transform(M)
    o.matrix_world = Matrix.Identity(4)
    return scale, grip_frac, (hi - lo)


def export(o, name, material):
    o.name = name
    o.data.materials.clear()
    mat = bpy.data.materials.get(material) or bpy.data.materials.new(material)
    o.data.materials.append(mat)
    bpy.ops.object.select_all(action="DESELECT")
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, name + ".glb"), use_selection=True, export_format="GLB",
                              export_materials="PLACEHOLDER", export_apply=True)


metals = {}
for kind, refs in WEAPONS.items():
    for ref in refs:
        clear()
        o, folder = import_new(kind)
        if ref == refs[0]:
            metals[kind] = textures(kind, folder, kind)
        tris = sum(len(p.vertices) - 2 for p in o.data.polygons)
        scale, frac, length = place(o, os.path.join(OLD, ref + ".glb"))
        export(o, ref, "MWN_" + kind)
        report.append("%s <- %s: %d tris, escala %.3f, pegada a %.0f%% do cabo, comprimento %.2f m" % (ref, kind, tris, scale, frac * 100, length))

# arco de ametista + flecha: separa as duas peças
clear()
o, folder = import_new("arco_flecha")
metals["arco_flecha"] = textures("arco_flecha", folder, "arco_flecha")
bpy.ops.object.mode_set(mode="EDIT")
bpy.ops.mesh.select_all(action="SELECT")
bpy.ops.mesh.separate(type="LOOSE")
bpy.ops.object.mode_set(mode="OBJECT")
parts = [p for p in bpy.data.objects if p.type == "MESH"]
# a flecha: as peças estreitas (largura pequena em X); o arco: o resto
arrow, bow = [], []
for p in parts:
    xs = [(p.matrix_world @ v.co).x for v in p.data.vertices]
    (arrow if max(xs) - min(xs) < .12 and min(xs) < -.2 else bow).append(p)


def join(objs):
    bpy.ops.object.select_all(action="DESELECT")
    for p in objs:
        p.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    if len(objs) > 1:
        bpy.ops.object.join()
    return bpy.context.view_layer.objects.active


report.append("arco_flecha: %d pecas, flecha %d, arco %d" % (len(parts), len(arrow), len(bow)))
if arrow and bow:
    a = join(arrow)
    pts = [v.co for v in a.data.vertices]
    zmin, zmax = min(p.z for p in pts), max(p.z for p in pts)
    cx = sum(p.x for p in pts) / len(pts)
    cy = sum(p.y for p in pts) / len(pts)
    # deitada: a ponta (+Z no modelo) para +X, 80 cm, a ponta na origem
    s = .8 / (zmax - zmin)
    a.data.transform(Matrix.Rotation(1.5707963, 4, "Y") @ Matrix.Scale(s, 4) @ Matrix.Translation(Vector((-cx, -cy, -zmax))))
    export(a, "W_flecha", "MWN_arco_flecha")
    b = join(bow)
    bpy.ops.object.select_all(action="DESELECT")
    for p in list(bpy.data.objects):
        if p != b:
            bpy.data.objects.remove(p, do_unlink=True)
    place(b, os.path.join(OLD, "W_arco_de_betula.glb"))
    export(b, "W_arco_ametista", "MWN_arco_flecha")

with open(os.path.join(TEX, "metallic.json"), "w", encoding="utf-8") as f:
    json.dump(metals, f, indent=1)
with open(os.path.join(ROOT, "Saved", "Logs", "FitNewWeapons.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(report) + "\n")
print("FIT_NEW_WEAPONS\n" + "\n".join(report))
