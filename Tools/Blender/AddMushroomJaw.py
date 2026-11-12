"""NÃO USAR (10/11): o usuário rejeitou a boca aberta ("ficou muito ruim"); o jogo usa o rosto original fechado
(Data/Modelos/Cogumelo_Corrompido/Mushroom_Skinned.glb = a cópia de Mushroom_Skinned_sem_boca.glb). Daqui vale só a
máscara T_Cogumelo_Mascara.png (já gerada). A versão rejeitada: Mushroom_Skinned_com_boca_rejeitada.glb.

O rosto do Cogumelo Corrompido ganha vida (10/11, o usuário: "a opção B"): a cara é esculpida na malha, então em vez
de uma placa de rosto (como a do herói) ele ganha:
- uma mandíbula de verdade: o osso "jaw" (filho da cabeça) segura o queixo e o lábio de baixo, e cada clipe abre a
  boca no tempo do golpe (pouco parado e andando, mais no cuspe e nos esporos, bem aberta na nuvem, nas raízes e na morte);
- a máscara de brilho (T_Cogumelo_Mascara.png): vermelho = os olhos alaranjados, verde = as manchas roxas do chapéu,
  achados pela cor da própria textura. O material (Tools/ImportMeshyMonsters.py) usa para os olhos pulsarem, piscarem e
  acenderem antes do golpe (AVortexMonster).

blender -b --factory-startup -P Tools/Blender/AddMushroomJaw.py
Lê e regrava Data/Modelos/Cogumelo_Corrompido/Mushroom_Skinned.glb (saído do MeshyBiped.py, mushroom+mixamo).
"""
import math
import os

import bpy
from mathutils import Quaternion, Vector

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
DIR = os.path.join(ROOT, "Data", "Modelos", "Cogumelo_Corrompido")
GLB = os.path.join(DIR, "Mushroom_Skinned.glb")

# o rosto (metros, o modelo de 1,30 m de frente para -Y): o vinco da boca e o queixo, medidos no meio da cara
# (10/11: medidos com marcadores na frente da cara: os olhos em .82, o vinco dos lábios em .745, o queixo até ~.66)
MOUTH_Z, CHIN_Z, FRONT_Y, HALF_W = .745, .66, -.12, .11

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.render.fps = 30
bpy.ops.import_scene.gltf(filepath=os.path.join(DIR, "Mushroom_Skinned_sem_boca.glb"))   # sempre do MeshyBiped (sem boca)
rig = next(o for o in bpy.data.objects if o.type == "ARMATURE")
body = max((o for o in bpy.data.objects if o.type == "MESH"), key=lambda o: len(o.data.vertices))
tracks = {t.name: t for t in rig.animation_data.nla_tracks} if rig.animation_data else {}
for t in tracks.values():
    t.mute = True
actions = {name: t.strips[0].action for name, t in tracks.items() if t.strips}
rig.animation_data.action = None
for pb in rig.pose.bones:
    pb.matrix_basis.identity()
bpy.context.view_layer.update()


def sm(x):
    x = min(1.0, max(0.0, x))
    return x * x * (3 - 2 * x)


# ---- o osso da mandíbula: a dobradiça atrás da boca, apontando para a frente
bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode="EDIT")
eb = rig.data.edit_bones
inv = rig.matrix_world.inverted()
jaw = eb.new("jaw")
jaw.head = inv @ Vector((0, -.11, MOUTH_Z + .01))
jaw.tail = inv @ Vector((0, -.25, MOUTH_Z - .01))
jaw.parent = eb["Bone_016"]
bpy.ops.object.mode_set(mode="OBJECT")

# ---- os pesos: o que fica abaixo do vinco da boca, na frente e no meio da cara, vai para a mandíbula (com bordas suaves)
group = body.vertex_groups.get("jaw") or body.vertex_groups.new(name="jaw")
W = body.matrix_world
moved = 0
for v in body.data.vertices:
    p = W @ v.co
    if p.y > FRONT_Y + .05 or abs(p.x) > HALF_W + .03 or p.z > MOUTH_Z + .01 or p.z < CHIN_Z - .04:
        continue
    w = (1 - sm((p.z - (MOUTH_Z - .012)) / .022))      # o vinco: corta quase seco (os lábios se separam)
    w *= sm((p.z - (CHIN_Z - .04)) / .07)              # embaixo do queixo: passa devagar para o pescoço
    w *= 1 - sm((abs(p.x) - (HALF_W - .04)) / .07)     # dos lados: some antes da bochecha
    w *= 1 - sm((p.y - (FRONT_Y - .03)) / .08)         # para trás: some antes da garganta
    if w < .01:
        continue
    for g in v.groups:
        g.weight *= (1 - w)
    group.add([v.index], w, "REPLACE")
    moved += 1
print("MANDIBULA pontos", moved)

# ---- 10/11: a boca abre de verdade. Sem corte, a pele do lábio só esticava (a cara é uma peça só). Corta só a linha dos
# lábios (já esculpida: as arestas que ligam a parte da mandíbula, peso > .5, à de cima, no meio da cara) e põe uma
# cavidade escura atrás (presa à cabeça), na cor mais escura da própria textura
import bmesh
import numpy as np


def base_color_image():
    """A textura ligada à cor (o material do Meshy tem três: cor, metal/rugosidade e relevo)."""
    for m in body.data.materials:
        if m and m.use_nodes:
            b = next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
            for l in b.inputs["Base Color"].links:
                if l.from_node.type == "TEX_IMAGE":
                    return l.from_node.image
    raise RuntimeError("sem textura de cor")


bm = bmesh.new()
bm.from_mesh(body.data)
dl = bm.verts.layers.deform.active
gi = group.index
Wm = body.matrix_world


def jw(v):
    return v[dl].get(gi, 0.0)


cut = [e for e in bm.edges
       if (jw(e.verts[0]) > .5) != (jw(e.verts[1]) > .5)
       and all(abs((Wm @ v.co).x) < HALF_W - .025 and (Wm @ v.co).y < FRONT_Y - .06 and abs((Wm @ v.co).z - MOUTH_Z) < .03 for v in e.verts)]
bmesh.ops.split_edges(bm, edges=cut)
# depois do corte: cada ponto da borda fica só com o lado dele (embaixo: mandíbula inteira; em cima: nada de mandíbula)
fixed = 0
for v in bm.verts:
    if not v.link_faces or not any(e.is_boundary for e in v.link_edges):
        continue
    zc = sum((Wm @ f.calc_center_median()).z for f in v.link_faces) / len(v.link_faces)
    if abs((Wm @ v.co).z - MOUTH_Z) > .03 or abs((Wm @ v.co).x) > HALF_W:
        continue
    below = zc < (Wm @ v.co).z
    d = v[dl]
    if below:
        for k in list(d.keys()):
            d[k] = 0.0
        d[gi] = 1.0
    else:
        d[gi] = 0.0
    fixed += 1
bm.to_mesh(body.data)
bm.free()
body.data.update()
print("BOCA corte", len(cut), "arestas", fixed, "pontos")

img0 = base_color_image()
sm_img = img0.copy()
sm_img.scale(256, 256)
full = np.array(sm_img.pixels[:], dtype=np.float32).reshape(256, 256, 4)
arr, alpha = full[..., :3], full[..., 3]
bpy.data.images.remove(sm_img)
lum = arr.sum(axis=2)
best, uv_dark = 9.0, (.5, .5)
for y in range(1, 255):
    for x in range(1, 255):
        v = lum[y - 1:y + 2, x - 1:x + 2].max()
        # opaco (os cantos vazios da textura são pretos e transparentes: a cavidade sumia)
        if v < best and arr[y, x, 0] >= arr[y, x, 2] and alpha[y - 1:y + 2, x - 1:x + 2].min() > .99:
            best, uv_dark = v, ((x + .5) / 256, (y + .5) / 256)
print("BOCA cor escura", round(best / 3, 3), uv_dark)
bm = bmesh.new()
bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=8, radius=1.0)
for v in bm.verts:
    v.co = Vector((v.co.x * .09, -.18 + v.co.y * .05, MOUTH_Z - .015 + v.co.z * .045))
# (as faces para fora: o material desenha um lado só; a frente da bola fica logo atrás dos lábios)
uvl = bm.loops.layers.uv.new("UVMap")
# um quadradinho em volta do ponto escuro (não um ponto: com área zero o relevo e o reflexo saíam errados, cinza)
for f in bm.faces:
    for k, loop in enumerate(f.loops):
        du, dv = ((-1, -1), (1, -1), (1, 1), (-1, 1))[k % 4]
        loop[uvl].uv = (uv_dark[0] + du * .0025, uv_dark[1] + dv * .0025)
me = bpy.data.meshes.new("Boca")
bm.to_mesh(me)
bm.free()
mouth = bpy.data.objects.new("Boca", me)
scene.collection.objects.link(mouth)
me.materials.append(body.data.materials[0])
me.transform(body.matrix_world.inverted())
mouth.matrix_world = body.matrix_world.copy()
mouth.vertex_groups.new(name="Bone_016").add(list(range(len(me.vertices))), 1.0, "REPLACE")
# a língua (o usuário: "preenche dentro da boca"): presa à mandíbula, no vermelho-escuro do próprio chapéu
best_t, uv_red = 9.0, (.5, .5)
for y in range(1, 255):
    for x in range(1, 255):
        c = arr[y, x]
        if alpha[y - 1:y + 2, x - 1:x + 2].min() > .99 and c[0] > .28 and c[1] < .16 and c[2] < .16:
            d = abs(c[0] - .38) + c[1] + c[2]
            if d < best_t:
                best_t, uv_red = d, ((x + .5) / 256, (y + .5) / 256)
print("BOCA língua", uv_red)
bm = bmesh.new()
bmesh.ops.create_uvsphere(bm, u_segments=10, v_segments=6, radius=1.0)
for v in bm.verts:
    z = v.co.z
    v.co = Vector((v.co.x * .055, -.19 + v.co.y * .045, MOUTH_Z - .022 + z * .011 + (max(0.0, -v.co.y)) * .005))
uvt = bm.loops.layers.uv.new("UVMap")
for f in bm.faces:
    for k, loop in enumerate(f.loops):
        du, dv = ((-1, -1), (1, -1), (1, 1), (-1, 1))[k % 4]
        loop[uvt].uv = (uv_red[0] + du * .0025, uv_red[1] + dv * .0025)
me_t = bpy.data.meshes.new("Lingua")
bm.to_mesh(me_t)
bm.free()
tongue = bpy.data.objects.new("Lingua", me_t)
scene.collection.objects.link(tongue)
me_t.materials.append(body.data.materials[0])
me_t.transform(body.matrix_world.inverted())
tongue.matrix_world = body.matrix_world.copy()
tongue.vertex_groups.new(name="jaw").add(list(range(len(me_t.vertices))), 1.0, "REPLACE")

# os dentes (o usuário: "os dentes estão muito finos, parece malha quebrada"): as pontas finas eram o lábio esculpido
# aberto pelo corte. Presas grossas de verdade, na cor creme das bolinhas do chapéu: em cima presas à cabeça, embaixo à
# mandíbula; logo atrás da borda dos lábios (fechada, a boca quase as esconde)
best_c, uv_cream = 9.0, (.5, .5)
for y in range(1, 255):
    for x in range(1, 255):
        c = arr[y, x]
        if alpha[y - 1:y + 2, x - 1:x + 2].min() > .99 and arr[y - 1:y + 2, x - 1:x + 2].min() > .62:
            d = abs(c[0] - .9) + abs(c[1] - .84) + abs(c[2] - .68)
            if d < best_c:
                best_c, uv_cream = d, ((x + .5) / 256, (y + .5) / 256)
print("BOCA dentes cor", uv_cream)
Wb = body.matrix_world
front_pts = [Wb @ v.co for v in body.data.vertices if abs((Wb @ v.co).z - MOUTH_Z) < .015 and (Wb @ v.co).y < -.15]


def lip_y(x):
    near = [p.y for p in front_pts if abs(p.x - x) < .012]
    return min(near) if near else -.22


bm = bmesh.new()
uvd = bm.loops.layers.uv.new("UVMap")
teeth_jaw = []   # quais pontos vão para a mandíbula
for row, (zbase, ztip, xs) in enumerate(((MOUTH_Z + .006, MOUTH_Z - .02, (-.07, -.045, -.02, .02, .045, .07)),
                                        (MOUTH_Z - .014, MOUTH_Z + .008, (-.058, -.03, 0.0, .03, .058)))):
    for x in xs:
        big = abs(x) > .06 or (row == 0 and abs(abs(x) - .045) < .001)   # as presas dos cantos maiores
        r = .0125 if big else .0105
        tip = ztip - (.008 if (big and row == 0) else 0) + (.006 if (big and row == 1) else 0)
        y0 = lip_y(x) + .011
        ring = []
        for k in range(6):
            a = math.tau * k / 6
            ring.append(bm.verts.new((x + math.cos(a) * r, y0 + math.sin(a) * r * .8, zbase)))
        apex = bm.verts.new((x + (.002 if x > 0 else -.002), y0 - .003, tip))
        cap = bm.faces.new(ring if row == 0 else list(reversed(ring)))
        for k in range(6):
            q = [ring[k], ring[(k + 1) % 6], apex] if row == 0 else [ring[(k + 1) % 6], ring[k], apex]
            bm.faces.new(q)
        if row == 1:
            teeth_jaw += ring + [apex]
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
for f in bm.faces:
    for k, loop in enumerate(f.loops):
        du, dv = ((-1, -1), (1, -1), (1, 1), (-1, 1))[k % 4]
        loop[uvd].uv = (uv_cream[0] + du * .0025, uv_cream[1] + dv * .0025)
jaw_ids = {v.index for v in teeth_jaw}
bm.verts.index_update()
jaw_ids = {v.index for v in teeth_jaw}
me_d = bpy.data.meshes.new("Dentes")
bm.to_mesh(me_d)
bm.free()
teeth = bpy.data.objects.new("Dentes", me_d)
scene.collection.objects.link(teeth)
me_d.materials.append(body.data.materials[0])
me_d.transform(body.matrix_world.inverted())
teeth.matrix_world = body.matrix_world.copy()
up_ids = [i for i in range(len(me_d.vertices)) if i not in jaw_ids]
teeth.vertex_groups.new(name="Bone_016").add(up_ids, 1.0, "REPLACE")
teeth.vertex_groups.new(name="jaw").add(sorted(jaw_ids), 1.0, "REPLACE")

bpy.ops.object.select_all(action="DESELECT")
teeth.select_set(True)
tongue.select_set(True)
mouth.select_set(True)
body.select_set(True)
bpy.context.view_layer.objects.active = body
bpy.ops.object.join()

# ---- a boca em cada clipe: (graus no máximo, curva de 0 a 1 pela fração do clipe)
OPEN = {
    "Idle": (4.0, lambda t: .5 + .5 * math.sin(2 * math.pi * 2 * t)),
    "Walk": (3.0, lambda t: .5 + .5 * math.sin(2 * math.pi * t)),
    "Attack": (16.0, lambda t: sm((t - .12) / .12) * (1 - sm((t - .32) / .15))),       # o cuspe
    "Spores": (12.0, lambda t: sm((t - .15) / .15) * (1 - sm((t - .7) / .2))),
    "Burst": (20.0, lambda t: sm((t - .2) / .15) * (1 - sm((t - .8) / .15))),         # a nuvem: o rugido
    "Roots": (18.0, lambda t: sm((t - .25) / .12) * (1 - sm((t - .6) / .2))),
    "Hop": (6.0, lambda t: math.sin(math.pi * t)),
    "Hit": (10.0, lambda t: sm(t / .12) * (1 - sm((t - .35) / .3))),
    "Tremble": (10.0, lambda t: sm(t / .12) * (1 - sm((t - .35) / .3))),
    "Death": (18.0, lambda t: sm((t - .3) / .3)),
}
pb = rig.pose.bones["jaw"]
pb.rotation_mode = "QUATERNION"
for name, act in actions.items():
    deg, curve = OPEN.get(name, (0.0, lambda t: 0.0))
    first, last = (int(round(f)) for f in act.frame_range)
    rig.animation_data.action = act
    try:
        rig.animation_data.action_slot = act.slots[0]
    except Exception:
        pass
    for f in range(first, last + 1, 2):
        t = (f - first) / max(1, last - first)
        # abre girando em volta do X do osso (o queixo desce)
        pb.rotation_quaternion = Quaternion((1, 0, 0), math.radians(deg * curve(t)))
        pb.keyframe_insert("rotation_quaternion", frame=f)
    rig.animation_data.action = None
    print("BOCA", name, deg)

# ---- a máscara dos olhos (alaranjados e claros) e das manchas roxas, da própria textura
img = base_color_image()
small = img.copy()
small.scale(1024, 1024)
import numpy as np
px = np.array(small.pixels[:], dtype=np.float32).reshape(1024, 1024, 4)
r, g, b = px[..., 0], px[..., 1], px[..., 2]
eyes = (r > .75) & (g > .35) & (g < .85) & (b < .35) & (r - b > .45)
spots = (b > .35) & (r > .3) & (g < .25) & (b - g > .2)
mask = np.zeros_like(px)
mask[..., 0] = eyes
mask[..., 1] = spots
mask[..., 3] = 1
out = bpy.data.images.new("T_Cogumelo_Mascara", 1024, 1024, alpha=True)
out.colorspace_settings.name = "Non-Color"
out.pixels[:] = mask.ravel().tolist()
out.filepath_raw = os.path.join(DIR, "T_Cogumelo_Mascara.png")
out.file_format = "PNG"
out.save()
print("MASCARA olhos", int(eyes.sum()), "manchas", int(spots.sum()))

bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True)
body.select_set(True)
bpy.context.view_layer.objects.active = rig
bpy.ops.export_scene.gltf(filepath=GLB, use_selection=True, export_format="GLB", export_animations=True,
                          export_animation_mode="NLA_TRACKS", export_skins=True, export_apply=False, export_yup=True)
print("MANDIBULA exportado", GLB)
