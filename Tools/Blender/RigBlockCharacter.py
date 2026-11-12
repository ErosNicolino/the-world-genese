"""O personagem base de blocos (Tools/Blender/MakeBlockCharacter.py) com o esqueleto do Mixamo, pronto para subir no
Mixamo como personagem já rigado (ele reconhece os ossos mixamorig:* e só aplica as animações) — no Blender aberto.

No Blender: exec(open(".../RigBlockCharacter.py", encoding="utf-8").read())

1. abre modelando ainda/personagem_base_partes.blend (as partes separadas, em A-pose);
2. uma textura só (as 9 imagens empilhadas: T_PersonagemBase.png) e um material só;
3. cada face dividida em pedaços de ~3 cm (o desenho não muda): o Mixamo e a deformação precisam de vértices;
4. o esqueleto do humano do Mixamo (Data/Heroes/Mixamo/humano/humano.fbx, sem as imagens) com cada junta movida para o
   boneco: quadril, coluna e pescoço no meio do tronco; braços e pernas pelo eixo de cada bloco (a A-pose), cotovelo e
   joelho no meio; pé e ponta do pé no bloco do pé;
5. pesos de boneco de bloco: cada bloco num osso; o braço inteiro com dobra macia no cotovelo e no pulso (02/11,
   ARM_SMOOTH) e a perna (coxa, canela) em pedaços
   rígidos que se sobrepõem 2 cm na junta, a canela 2 cm dentro do pé, o pé inteiro no Foot (01/11); dobra suave só
   entre as vértebras;
6. sai modelando ainda/personagem_base_rig.fbx (e .blend), com a textura junto.

--feminino (01/11): o corpo feminino, com um busto pequeno (um bloco no peito, na cor do top da humano_cor_f.png) e a
cintura levemente afinada; sai personagem_base_rig_f.fbx (e .blend). Sem janela:
    blender -b --factory-startup -P Tools/Blender/RigBlockCharacter.py -- --feminino
"""
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
OUT = os.path.join(ROOT, "modelando ainda")
P = "mixamorig:"
FEMALE = "--feminino" in (sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []) or globals().get("FEMININO", False)
SUFFIX = "_f" if FEMALE else ""


def log(*parts):
    print("RIG_BLOCO", " ".join(str(p) for p in parts))


bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT, "personagem_base_partes.blend"))
parts = {o.name: o for o in bpy.data.collections["PersonagemBase"].objects}

# ---- uma textura e um material
imgs = [(o, next(n for n in o.data.materials[0].node_tree.nodes if n.type == "TEX_IMAGE").image) for o in parts.values()]
W = max(i.size[0] for _, i in imgs)
H = sum(i.size[1] + 1 for _, i in imgs)
atlas = bpy.data.images.new("T_PersonagemBase", W, H, alpha=True)
px = [0.0] * (W * H * 4)
y, place = 0, {}
for o, img in imgs:
    w, h = img.size
    src = list(img.pixels)
    for r in range(h):
        for c in range(w):
            si, di = (r * w + c) * 4, ((y + r) * W + c) * 4
            px[di:di + 4] = src[si:si + 4]
    place[o.name] = (w / W, h / H, y / H)
    y += h + 1
atlas.pixels[:] = px
os.makedirs(os.path.join(OUT, "texturas"), exist_ok=True)
atlas.filepath_raw = os.path.join(OUT, "texturas", "T_PersonagemBase.png")
atlas.file_format = "PNG"
atlas.save()
mat = bpy.data.materials.new("M_PersonagemBase")
mat.use_nodes = True
nt = mat.node_tree
bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
tex = nt.nodes.new("ShaderNodeTexImage")
tex.image = atlas
tex.interpolation = "Closest"
nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
bsdf.inputs["Roughness"].default_value = .9

# eixo de cada bloco (antes de dividir): o centro da face de cima e o da de baixo, pela pose (A-pose)
axes = {}
for name, o in parts.items():
    me = o.data
    faces = sorted(me.polygons, key=lambda p: (o.matrix_world @ p.center).z)
    bottom, top = o.matrix_world @ faces[0].center, o.matrix_world @ faces[-1].center
    axes[name] = (top, bottom)

for name, o in parts.items():
    sx, sy, oy = place[name]
    for d in o.data.uv_layers.active.data:
        d.uv = (d.uv.x * sx, oy + d.uv.y * sy)
    o.data.materials.clear()
    o.data.materials.append(mat)

# ---- membros rígidos (01/11): a perna e o braço viram blocos separados, cada um num osso só, que se sobrepõem OVER na
# junta (no joelho, no cotovelo e no pulso a malha contínua dobrava como borracha e amassava nas danças). A canela
# entra OVER no pé (o tornozelo não abre vão)
OVER = .02
# 02/11 (pedido do usuário): o braço volta a ser um bloco só, com a dobra macia no cotovelo e no pulso. Rígido, o
# cotovelo abria um vão na quina de fora quando dobrava (dava para ver através); macio ele amassa um pouco nas danças,
# e o usuário prefere assim. A perna continua em pedaços rígidos
ARM_SMOOTH = True
# 03/11 (pedido do usuário: "cotovelos e joelhos, tá quebrado"): a perna também fica um bloco só, com a dobra macia no
# joelho (rígida, abria um vão na quina quando dobrava)
LEG_SMOOTH = True
KNEE_T = {}


def cut_piece(o, name, t0, t1):
    """Cópia do bloco o só entre t0 e t1 do eixo dele (0 = em cima, 1 = embaixo), com OVER a mais de cada lado cortado e
    a tampa fechada (a cor da tampa é a da borda)."""
    c = o.copy()
    c.data = o.data.copy()
    c.name = name
    for col in o.users_collection:
        col.objects.link(c)
    bm = bmesh.new()
    bm.from_mesh(c.data)
    bm.transform(o.matrix_world)
    c.matrix_world = Matrix.Identity(4)
    top, bottom = axes[o.name]
    d = (bottom - top)
    L = d.length
    d.normalize()
    uvl = bm.loops.layers.uv.active
    for t, keep_below in ((t0, True), (t1, False)):
        if (keep_below and t <= 0) or (not keep_below and t >= 1):
            continue
        co = top + d * (t * L + (-OVER if keep_below else OVER))
        geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
        res = bmesh.ops.bisect_plane(bm, geom=geom, plane_co=co, plane_no=d, clear_inner=keep_below, clear_outer=not keep_below)
        edges = [e for e in res["geom_cut"] if isinstance(e, bmesh.types.BMEdge)]
        made = bmesh.ops.edgeloop_fill(bm, edges=edges) if edges else {"faces": []}
        for f in made["faces"]:
            for loop in f.loops:
                other = next((l for l in loop.vert.link_loops if l.face is not f), None)
                if other is not None:
                    loop[uvl].uv = other[uvl].uv.copy()
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(c.data)
    bm.free()
    axes[name] = axes[o.name]
    return c


for s in ("L", "R"):
    side = "Left" if s == "L" else "Right"
    leg = parts.pop("Perna_" + s)
    # o joelho: a junta do Leg (no meio entre o alto da perna e o chão), no eixo da perna
    l_top, l_bot = axes[leg.name]
    f_top, f_bot = axes["Pe_" + s]
    knee = l_top.lerp(f_bot, .5)
    tk = max(0.0, min(1.0, (knee - l_top).dot(l_bot - l_top) / (l_bot - l_top).length_squared))
    KNEE_T[s] = tk
    if LEG_SMOOTH:
        # inteira: só a sola entra OVER no pé (o tornozelo não abre vão)
        zmin = min(v.co.z for v in leg.data.vertices)
        for v in leg.data.vertices:
            if v.co.z < zmin + 1e-4:
                v.co.z -= OVER
        parts.pop("Perna_" + s, None)
        parts["Perna_" + s] = leg
        if ARM_SMOOTH:
            continue
    parts["Perna_%s_cima" % s] = cut_piece(leg, "Perna_%s_cima" % s, 0.0, tk)
    low = cut_piece(leg, "Perna_%s_baixo" % s, tk, 1.0)
    # a canela entra no pé
    zmin = min(v.co.z for v in low.data.vertices)
    for v in low.data.vertices:
        if v.co.z < zmin + 1e-4:
            v.co.z -= OVER
    parts["Perna_%s_baixo" % s] = low
    bpy.data.objects.remove(leg, do_unlink=True)
    if ARM_SMOOTH:
        continue
    arm = parts.pop("Braco_" + s)
    parts["Braco_%s_cima" % s] = cut_piece(arm, "Braco_%s_cima" % s, 0.0, .5)
    parts["Braco_%s_meio" % s] = cut_piece(arm, "Braco_%s_meio" % s, .5, .85)
    parts["Braco_%s_mao" % s] = cut_piece(arm, "Braco_%s_mao" % s, .85, 1.0)
    bpy.data.objects.remove(arm, do_unlink=True)

if FEMALE:
    # o busto: um bloco baixo na frente do peito (linhas 1 a 4 das 10 do tronco, onde o top é desenhado), 2,2 cm para
    # fora; a frente pega o desenho do top e o resto a cor do pano
    tr = parts["Tronco"]
    bm = bmesh.new()
    bm.from_mesh(tr.data)
    bm.transform(tr.matrix_world)
    uvl = bm.loops.layers.uv.active
    front = min(bm.faces, key=lambda f: f.calc_center_median().y)
    us = [l[uvl].uv.x for l in front.loops]
    vs = [l[uvl].uv.y for l in front.loops]
    u0, u1, v0, v1 = min(us), max(us), min(vs), max(vs)
    xs = [v.co.x for v in bm.verts]
    zs = [v.co.z for v in bm.verts]
    y_front = min(v.co.y for v in bm.verts)
    bm.free()
    x_mid, half = (min(xs) + max(xs)) / 2, (max(xs) - min(xs)) / 2
    z_top, rows = max(zs), (max(zs) - min(zs)) / 10
    cloth = ((u0 + (u1 - u0) * 3.5 / 7), v1 - (v1 - v0) * 2.5 / 10)
    bm = bmesh.new()
    res = bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=Vector((half * 1.62, .022, rows * 3.2)), verts=bm.verts)
    bmesh.ops.translate(bm, vec=Vector((x_mid, y_front - .011, z_top - rows * 2.75)), verts=bm.verts)
    uvl = bm.loops.layers.uv.new("UV")
    for f in bm.faces:
        n = f.normal
        for l in f.loops:
            if n.y < -.5:
                # a frente: o desenho do top (colunas 1 a 6 da frente, linhas 1 a 4)
                fx = (l.vert.co.x - (x_mid - half * .81)) / (half * 1.62)
                fz = (l.vert.co.z - (z_top - rows * 4.35)) / (rows * 3.2)
                l[uvl].uv = (u0 + (u1 - u0) * (.5 + 6 * fx) / 7, v1 - (v1 - v0) * (4.9 - 3.8 * fz) / 10)
            else:
                l[uvl].uv = cloth
    me = bpy.data.meshes.new("Busto")
    bm.to_mesh(me)
    bm.free()
    me.materials.append(mat)
    bust = bpy.data.objects.new("Busto", me)
    tr.users_collection[0].objects.link(bust)
    parts["Busto"] = bust
    axes["Busto"] = axes["Tronco"]

for name, o in parts.items():
    bm = bmesh.new()
    bm.from_mesh(o.data)
    longest = max(e.calc_length() for e in bm.edges)
    bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=max(2, int(longest / .03)), use_grid_fill=True)
    bm.to_mesh(o.data)
    bm.free()

if FEMALE:
    # a cintura: o tronco afina até 9% na largura, mais no terço de baixo (o peito e a cueca ficam como estão)
    tr = parts["Tronco"]
    top, bottom = axes["Tronco"]
    h = top.z - bottom.z
    mw = tr.matrix_world
    inv = mw.inverted()
    for v in tr.data.vertices:
        w = mw @ v.co
        t = (w.z - bottom.z) / h                                     # 0 embaixo, 1 em cima
        bump = max(0.0, 1 - abs(t - .32) / .34)
        bump = bump * bump * (3 - 2 * bump)
        w.x = top.x + (w.x - top.x) * (1 - .09 * bump)
        v.co = inv @ w

# ---- o esqueleto do Mixamo (o do humano), sem as imagens do FBX
import bpy_extras.image_utils as image_utils
original = image_utils.load_image
image_utils.load_image = lambda *a, **k: bpy.data.images.get("_vazio") or bpy.data.images.new("_vazio", 1, 1)
before = set(bpy.data.objects)
try:
    bpy.ops.import_scene.fbx(filepath=os.path.join(ROOT, "Data", "Heroes", "Mixamo", "humano", "humano.fbx"),
                             automatic_bone_orientation=False, ignore_leaf_bones=False, use_image_search=False)
finally:
    image_utils.load_image = original
new = [o for o in bpy.data.objects if o not in before]
rig = next(o for o in new if o.type == "ARMATURE")
for o in new:
    if o.type == "MESH":
        bpy.data.objects.remove(o, do_unlink=True)
rig.animation_data_clear()
rig.name = "Armature"


def ctx(active, selected):
    """Contexto para os operadores (com janela, se houver: no Blender aberto pelo MCP; sem, rodando com -b)."""
    for o in bpy.data.objects:
        o.select_set(o in selected)
    bpy.context.view_layer.objects.active = active
    kw = dict(active_object=active, selected_objects=selected, selected_editable_objects=selected, object=active)
    wm = bpy.context.window_manager
    if wm.windows:
        win = wm.windows[0]
        area = next((a for a in win.screen.areas if a.type == "VIEW_3D"), None)
        kw.update(window=win, area=area) if area else kw.update(window=win)
    return bpy.context.temp_override(**kw)


with ctx(rig, [rig]):
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)


def along(name, t):
    top, bottom = axes[name]
    return top.lerp(bottom, t)


cab_top, cab_bot = axes["Cabeca"]
tr_top, tr_bot = axes["Tronco"]
cu_top, cu_bot = axes["Cueca"]
pos = {
    "Hips": cu_bot.lerp(cu_top, .45),
    "Spine": cu_top.copy(),
    "Spine1": tr_bot.lerp(tr_top, .35),
    "Spine2": tr_bot.lerp(tr_top, .68),
    "Neck": tr_top.copy(),
    "Head": tr_top.lerp(cab_top, .08),
    "HeadTop_End": cab_top.copy(),
}
for side, s in (("Left", "L"), ("Right", "R")):
    arm = "Braco_" + s
    pos[side + "Arm"] = along(arm, .08)
    pos[side + "Shoulder"] = pos["Spine2"].lerp(Vector((pos[side + "Arm"].x * .45, 0, pos[side + "Arm"].z)), 1.0)
    pos[side + "ForeArm"] = along(arm, .5)
    pos[side + "Hand"] = along(arm, .85)
    pos[side + "HandTip"] = along(arm, 1.0)
    leg, foot = "Perna_" + s, "Pe_" + s
    l_top, l_bot = axes[leg]
    f_top, f_bot = axes[foot]
    pos[side + "UpLeg"] = l_top.copy()
    total = (l_top - f_bot).length
    pos[side + "Leg"] = l_top.lerp(f_bot, .5)
    pos[side + "Foot"] = f_top.lerp(f_bot, .4)
    fo = parts[foot]
    front_y = min((fo.matrix_world @ v.co).y for v in fo.data.vertices)
    pos[side + "ToeBase"] = Vector((pos[side + "Foot"].x, front_y * .35, f_bot.z + (f_top.z - f_bot.z) * .25))
    pos[side + "Toe_End"] = Vector((pos[side + "Foot"].x, front_y, f_bot.z + (f_top.z - f_bot.z) * .25))

with ctx(rig, [rig]):
    bpy.ops.object.mode_set(mode="EDIT")
eb = rig.data.edit_bones
old_head = {b.name: b.head.copy() for b in eb}
for b in eb:
    n = b.name[len(P):]
    if n in pos:
        b.head = pos[n]
    else:
        # dedos e o que não tem no boneco: na ponta da mão (ou junto do pai)
        side = "Left" if "Left" in n else "Right" if "Right" in n else None
        b.head = pos.get((side or "") + "HandTip", b.parent.head if b.parent else b.head) if side and "Hand" in n else (b.parent.head if b.parent else b.head)
for b in eb:
    n = b.name[len(P):]
    kids = list(b.children)
    main = next((c for c in kids if c.name[len(P):] in ("Spine", "Spine1", "Spine2", "Neck", "Head", "HeadTop_End")), None) if n.startswith(("Hips", "Spine", "Neck", "Head")) else None
    if main is None and kids:
        main = kids[0] if len(kids) == 1 else next((c for c in kids if not ("Thumb" in c.name or "Index" in c.name or "Pinky" in c.name or "Ring" in c.name)), kids[0])
    if n.endswith("Hand"):
        b.tail = pos[n + "Tip"]
    elif main is not None and (main.head - b.head).length > 1e-3:
        b.tail = main.head
    else:
        d = (b.head - b.parent.head) if b.parent else Vector((0, 0, .05))
        b.tail = b.head + (d.normalized() if d.length > 1e-6 else Vector((0, 0, 1))) * .03
with ctx(rig, [rig]):
    bpy.ops.object.mode_set(mode="OBJECT")

# ---- os pesos (cada bloco num osso, dobra suave)


def ramp(t, at, width):
    """0 antes de at-width, 1 depois de at+width (suave no meio)."""
    x = max(0.0, min(1.0, (t - (at - width)) / (2 * width)))
    return x * x * (3 - 2 * x)


def param(name, p):
    top, bottom = axes[name]
    d = bottom - top
    return max(0.0, min(1.0, (p - top).dot(d) / d.length_squared))


for name, o in parts.items():
    groups = {}

    def add(i, bone, w):
        if w <= 1e-4:
            return
        g = groups.get(bone) or o.vertex_groups.new(name=P + bone)
        groups[bone] = g
        g.add([i], w, "REPLACE")

    for v in o.data.vertices:
        p = o.matrix_world @ v.co
        i = v.index
        if name == "Cabeca":
            add(i, "Head", 1)
        elif name == "Cueca":
            add(i, "Hips", 1)
        elif name == "Busto":
            add(i, "Spine2", 1)
        elif name == "Tronco":
            z = p.z
            a = ramp(z, pos["Spine1"].z, (pos["Spine1"].z - pos["Spine"].z) * .5)
            b = ramp(z, pos["Spine2"].z, (pos["Spine2"].z - pos["Spine1"].z) * .5)
            add(i, "Spine", 1 - a)
            add(i, "Spine1", a * (1 - b))
            add(i, "Spine2", a * b)
        elif name in ("Braco_L", "Braco_R"):
            # o braço inteiro, dobra macia (como até 31/10): o cotovelo no meio, o pulso a 85%
            side = "Left" if name.endswith("L") else "Right"
            t = param(name, p)
            e = ramp(t, .5, .06)
            h = ramp(t, .85, .04)
            add(i, side + "Arm", 1 - e)
            add(i, side + "ForeArm", e * (1 - h))
            add(i, side + "Hand", e * h)
        elif name.startswith("Braco"):
            # cada pedaço num osso só (rígido): braço, antebraço e mão
            side = "Left" if "_L" in name else "Right"
            add(i, side + {"cima": "Arm", "meio": "ForeArm", "mao": "Hand"}[name.split("_")[-1]], 1)
        elif name in ("Perna_L", "Perna_R"):
            # a perna inteira, dobra macia no joelho (03/11)
            side = "Left" if name.endswith("L") else "Right"
            k = ramp(param(name, p), KNEE_T[name[-1]], .06)
            add(i, side + "UpLeg", 1 - k)
            add(i, side + "Leg", k)
        elif name.startswith("Perna"):
            side = "Left" if "_L" in name else "Right"
            add(i, side + ("UpLeg" if name.endswith("cima") else "Leg"), 1)
        elif name.startswith("Pe"):
            # o pé inteiro no Foot: com a ponta no ToeBase o bloco quebrava ao meio quando a ponta girava (danças)
            side = "Left" if name.endswith("L") else "Right"
            add(i, side + "Foot", 1)

# ---- um objeto só, preso ao esqueleto
objs = list(parts.values())
active = parts["Tronco"]
with ctx(active, objs):
    bpy.ops.object.join()
body = active
body.name = "PersonagemBase"
body.parent = rig
mod = body.modifiers.new("Armature", "ARMATURE")
mod.object = rig
for img in list(bpy.data.images):
    if img != atlas and img.users == 0:
        bpy.data.images.remove(img)
log("%d vértices, %d ossos, 1 material" % (len(body.data.vertices), len(rig.data.bones)))

# ---- os arquivos
with ctx(rig, [rig, body]):
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, "personagem_base_rig%s.fbx" % SUFFIX), use_selection=True,
                             object_types={"ARMATURE", "MESH"}, path_mode="COPY", embed_textures=True, add_leaf_bones=False,
                             bake_anim=False, apply_scale_options="FBX_SCALE_ALL")
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "personagem_base_rig%s.blend" % SUFFIX), copy=True)
log("exportado", os.path.join(OUT, "personagem_base_rig%s.fbx" % SUFFIX))
