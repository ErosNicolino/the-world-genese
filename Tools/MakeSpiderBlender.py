"""Aranha dos portais (Blender), versão 2 ("tá bem feia e simples"): em blocos, no estilo do mundo, mas com
anatomia de aranha de verdade (tarântula/aranha-lobo) e o toque de monstro do portal:
- cefalotórax em camadas (carapaça com sulco), quelíceras grossas com presas de osso, palpos na frente, 8 olhos
  vermelhos acesos (2 grandes na frente, 2 médios, 4 em cima);
- abdômen grande em ovo (camadas que afinam), listras em V roxas, rachaduras de energia do portal acesas em ciano,
  fiandeiras atrás, pelos espetados;
- 8 pernas de 3 segmentos (coxa, fêmur alto, tíbia até o chão) com juntas, faixas claras como de tarântula,
  tufos de pelo e garras escuras na ponta.
Esqueleto: root, body, head, abdomen, palpos (2 x 2) e pernas (8 x 3). Clipes: Idle, Walk, Attack, Leap, Hit, Death.
Exporta Data/GodotMonsters/Spider/Spider_Skinned.glb e Spider_Palette.png (Tools/ImportMonsters.py importa).
Rodar dentro do Blender: exec(open(<este arquivo>).read()) com VORTEX_ROOT definido. Volta para a cena que estava aberta.
A aranha olha para +Y do Blender (vira -Z no glTF, como os bichos do Godot).
"""
import math
import os
import bmesh
import bpy
from mathutils import Matrix, Vector

ROOT = globals().get("VORTEX_ROOT") or os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "GodotMonsters", "Spider")
os.makedirs(OUT, exist_ok=True)

previous = bpy.context.window.scene
scene = bpy.data.scenes.get("VortexSpider") or bpy.data.scenes.new("VortexSpider")
for obj in list(scene.collection.objects):
    bpy.data.objects.remove(obj, do_unlink=True)
bpy.context.window.scene = scene
scene.render.fps = 24

# ---- paleta: 4 x 3 cores em blocos de 2x2 px --------------------------------------------------------------
COLORS = ["1d1724", "2e2440", "3a2a52", "6a3fa0",   # corpo, carapaça, abdômen, listra
          "16121c", "4b3868", "ff2a2a", "e6dcc4",   # perna, faixa da perna, olho, presa
          "5c4a78", "3df0ff", "0c0a10", "8a1a2a"]   # pelo, runa (portal), garra, boca
BODY, CARAPACE, ABDOMEN, STRIPE, LEG, BAND, EYE, FANG, HAIR, RUNE, CLAW, MOUTH = range(12)
W, H = 8, 6
image = bpy.data.images.get("Spider_Palette")
if image and (image.size[0] != W or image.size[1] != H):
    bpy.data.images.remove(image)
    image = None
image = image or bpy.data.images.new("Spider_Palette", W, H)
pixels = [0.0] * (W * H * 4)
for c, hexcode in enumerate(COLORS):
    r, g, b = (int(hexcode[i:i + 2], 16) / 255.0 for i in (0, 2, 4))
    for dx in range(2):
        for dy in range(2):
            x, y = (c % 4) * 2 + dx, (c // 4) * 2 + dy
            i = (y * W + x) * 4
            pixels[i:i + 4] = [r, g, b, 1.0]
image.pixels = pixels
image.filepath_raw = os.path.join(OUT, "Spider_Palette.png")
image.file_format = "PNG"
image.save()


def cell_uv(c):
    return ((c % 4) * 2 + 1) / W, ((c // 4) * 2 + 1) / H


def material(name, glow):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    tex = next((n for n in nodes if n.type == "TEX_IMAGE"), None) or nodes.new("ShaderNodeTexImage")
    tex.image = image
    tex.interpolation = "Closest"
    mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    if glow:
        mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Emission Color"])
        bsdf.inputs["Emission Strength"].default_value = 4.0
    return mat


# ---- esqueleto ---------------------------------------------------------------------------------------------
arm_data = bpy.data.armatures.new("Spider_Rig")
rig = bpy.data.objects.new("Spider_Skinned", arm_data)
scene.collection.objects.link(rig)
bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode="EDIT")
bones = {}


def bone(name, head, tail, parent=None, connect=False):
    b = arm_data.edit_bones.new(name)
    b.head, b.tail = Vector(head), Vector(tail)
    b.align_roll(Vector((0, 0, 1)))
    if parent:
        b.parent = bones[parent]
        b.use_connect = connect
    bones[name] = b
    return b


BODY_Z = .42
bone("root", (0, 0, 0), (0, 0, .2))
bone("body", (0, -.08, BODY_Z), (0, .3, BODY_Z), "root")
bone("head", (0, .3, BODY_Z), (0, .56, BODY_Z - .02), "body")
bone("abdomen", (0, -.1, BODY_Z + .05), (0, -.85, BODY_Z + .18), "body")
for side, sname in ((1, "R"), (-1, "L")):
    bone("palp%s_a" % sname, (side * .1, .55, BODY_Z - .05), (side * .17, .72, BODY_Z + .02), "head")
    bone("palp%s_b" % sname, (side * .17, .72, BODY_Z + .02), (side * .16, .86, BODY_Z - .18), "palp%s_a" % sname, True)
LEGS = []
for side, sname in ((1, "R"), (-1, "L")):
    for i, (y, spread) in enumerate(((.24, .34), (.1, .12), (-.04, -.1), (-.18, -.32))):
        hip = (side * .26, y, BODY_Z)
        knee = (side * .6, y + spread * .8, BODY_Z + .36)
        ankle = (side * .98, y + spread * 1.5, BODY_Z + .1)
        foot = (side * 1.16, y + spread * 1.85, 0.0)
        a, b, c = ("leg%s%d_%s" % (sname, i, s) for s in "abc")
        bone(a, hip, knee, "body")
        bone(b, knee, ankle, a, True)
        bone(c, ankle, foot, b, True)
        LEGS.append((a, b, c, side, i, sname))
bpy.ops.object.mode_set(mode="OBJECT")

# ---- malha em blocos (cada bloco preso a um osso) ------------------------------------------------------------
mesh = bpy.data.meshes.new("Spider_Mesh")
body_obj = bpy.data.objects.new("Spider_Body", mesh)
scene.collection.objects.link(body_obj)
bm = bmesh.new()
uv_layer = bm.loops.layers.uv.new("UVMap")
deform = bm.verts.layers.deform.verify()
groups = []
GLOWS = {EYE, RUNE}


def group(name):
    if name not in groups:
        groups.append(name)
    return groups.index(name)


def block(center, size, color, bone_name, frame=None):
    hx, hy, hz = (s / 2 for s in size)
    corners = [Vector((sx * hx, sy * hy, sz * hz)) for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)]
    if frame:
        corners = [frame @ c for c in corners]
    verts = [bm.verts.new(Vector(center) + c) for c in corners]
    gi = group(bone_name)
    for v in verts:
        v[deform][gi] = 1.0
    u, w = cell_uv(color)
    for f in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)):
        face = bm.faces.new([verts[i] for i in f])
        face.material_index = 1 if color in GLOWS else 0
        for loop in face.loops:
            loop[uv_layer].uv = (u, w)


def frame_along(p0, p1):
    y = (p1 - p0).normalized()
    x = y.cross(Vector((0, 0, 1)))
    if x.length < 1e-4:
        x = Vector((1, 0, 0))
    x.normalize()
    return Matrix((x, y, x.cross(y))).transposed()


def segment(p0, p1, thick, color, bone_name, taper=1.0, bands=0, band_color=BAND):
    """Segmento de perna de p0 a p1, em 1 bloco (afinando com taper) e faixas claras."""
    p0, p1 = Vector(p0), Vector(p1)
    frame = frame_along(p0, p1)
    length = (p1 - p0).length
    pieces = 3
    for k in range(pieces):
        t0, t1 = k / pieces, (k + 1) / pieces
        a, b = p0.lerp(p1, t0), p0.lerp(p1, t1)
        s = thick * (1 - (1 - taper) * (t0 + t1) / 2)
        block((a + b) / 2, (s, length / pieces + s * .2, s), color, bone_name, frame)
    for k in range(bands):
        t = (k + 1) / (bands + 1)
        s = thick * (1 - (1 - taper) * t) * 1.18
        block(p0.lerp(p1, t), (s, .045, s), band_color, bone_name, frame)


# cefalotórax: base, carapaça elevada com sulco, a "testa" com os olhos
block((0, .1, BODY_Z), (.56, .58, .26), BODY, "body")
block((0, .1, BODY_Z + .15), (.44, .46, .1), CARAPACE, "body")
block((0, .1, BODY_Z + .21), (.06, .4, .02), BODY, "body")
for side in (-1, 1):
    block((side * .16, .02, BODY_Z + .14), (.12, .3, .08), CARAPACE, "body")
block((0, .44, BODY_Z + .02), (.4, .26, .3), BODY, "head")
block((0, .44, BODY_Z + .18), (.3, .2, .06), CARAPACE, "head")
# olhos: 2 grandes na frente, 2 médios ao lado, 4 pequenos em cima
for x in (-.065, .065):
    block((x, .575, BODY_Z + .07), (.085, .03, .085), EYE, "head")
for x in (-.15, .15):
    block((x, .56, BODY_Z + .1), (.05, .03, .05), EYE, "head")
for x, y in ((-.08, .5), (.08, .5), (-.05, .44), (.05, .44)):
    block((x, y, BODY_Z + .22), (.04, .04, .025), EYE, "head")
# quelíceras (grossas, com pelo) e as presas curvas de osso
for side in (-1, 1):
    block((side * .085, .6, BODY_Z - .08), (.13, .13, .2), BODY, "head")
    block((side * .085, .62, BODY_Z - .02), (.135, .02, .08), HAIR, "head")
    block((side * .08, .65, BODY_Z - .21), (.06, .07, .1), FANG, "head")
    block((side * .06, .67, BODY_Z - .27), (.04, .05, .06), FANG, "head")
block((0, .6, BODY_Z - .14), (.06, .04, .05), MOUTH, "head")
# palpos
for sname in ("L", "R"):
    pa, pb = arm_data.bones["palp%s_a" % sname], arm_data.bones["palp%s_b" % sname]
    segment(pa.head_local, pa.tail_local, .07, BODY, "palp%s_a" % sname)
    segment(pb.head_local, pb.tail_local, .06, LEG, "palp%s_b" % sname, .8, 1)
# abdômen: camadas que afinam (forma de ovo), listras em V, rachaduras do portal, pelos e fiandeiras
AZ = BODY_Z + .2
for size, z in (((.62, .7, .16), AZ - .22), ((.86, .94, .2), AZ - .06), ((.9, 1.0, .2), AZ + .1), ((.72, .84, .16), AZ + .26), ((.44, .56, .08), AZ + .38)):
    block((0, -.5, z), size, ABDOMEN, "abdomen")
block((0, -.98, AZ + .02), (.5, .14, .4), ABDOMEN, "abdomen")
for k, y in enumerate((-.26, -.44, -.62, -.8)):
    w = .5 - k * .08
    for side in (-1, 1):
        block((side * w * .32, y + .03, AZ + .42 - k * .04), (w * .46, .07, .03), STRIPE, "abdomen")
block((0, -.32, AZ + .43), (.08, .08, .03), STRIPE, "abdomen")
# rachaduras de energia do portal (ciano aceso) nos lados e em cima
for side in (-1, 1):
    for y, z, s in ((-.3, AZ + .08, .2), (-.5, AZ + .2, .26), (-.72, AZ + .02, .18)):
        block((side * .455, y, z), (.02, s, .04), RUNE, "abdomen")
        block((side * .455, y + s * .4, z + .07), (.02, .04, .1), RUNE, "abdomen")
block((0, -.55, AZ + .44), (.03, .3, .02), RUNE, "abdomen")
for side in (-1, 1):
    for y in (-.22, -.42, -.62, -.82):
        block((side * .47, y, AZ + .18), (.05, .03, .14), HAIR, "abdomen")
for x, y in ((-.18, -.3), (.2, -.36), (-.24, -.66), (.22, -.7), (0, -.86), (-.1, -.5), (.12, -.52)):
    block((x, y, AZ + .44), (.035, .035, .09), HAIR, "abdomen")
for side in (-1, 1):
    block((side * .06, -1.07, AZ - .04), (.06, .08, .06), CARAPACE, "abdomen")
# pernas: coxa grossa, fêmur alto com faixas e pelos, tíbia fina até a garra
for a, b, c, side, i, sname in LEGS:
    ba, bb, bc = (arm_data.bones[n] for n in (a, b, c))
    segment(ba.head_local, ba.tail_local, .12, LEG, a, .85, 1)
    block(ba.tail_local, (.14, .14, .14), BODY, a)
    segment(bb.head_local, bb.tail_local, .1, LEG, b, .85, 1)
    block(bb.tail_local, (.11, .11, .11), BODY, b)
    segment(bc.head_local, bc.tail_local, .08, LEG, c, .55, 1)
    block(Vector(bc.tail_local) + Vector((0, 0, .03)), (.05, .05, .07), CLAW, c)
    frame = frame_along(Vector(ba.head_local), Vector(ba.tail_local))
    for t in (.3, .65):
        p = Vector(ba.head_local).lerp(Vector(ba.tail_local), t)
        block(p + Vector((0, 0, .07)), (.03, .03, .08), HAIR, a)
bm.to_mesh(mesh)
bm.free()
mesh.materials.append(material("Spider_Body_Mat", False))
mesh.materials.append(material("Spider_Glow_Mat", True))
for name in groups:
    body_obj.vertex_groups.new(name=name)
body_obj.parent = rig
modifier = body_obj.modifiers.new("Armature", "ARMATURE")
modifier.object = rig

# ---- animações -------------------------------------------------------------------------------------------------
rig.animation_data_create()
pose = rig.pose.bones
for pb in pose:
    pb.rotation_mode = "XYZ"


def reset():
    for pb in pose:
        pb.rotation_euler = (0, 0, 0)
        pb.location = (0, 0, 0)


def key(frame):
    for pb in pose:
        pb.keyframe_insert("rotation_euler", frame=frame)
        pb.keyframe_insert("location", frame=frame)


def leg(a, lift, swing, side, knee=0.0, ankle=0.0):
    """lift: ergue a perna na coxa (graus); swing: para a frente; knee/ankle: dobram as juntas."""
    pose[a].rotation_euler = (math.radians(lift), 0, math.radians(swing * side))
    pose[a.replace("_a", "_b")].rotation_euler = (math.radians(knee), 0, 0)
    pose[a.replace("_a", "_c")].rotation_euler = (math.radians(ankle), 0, 0)


def palps(lift, spread=0.0):
    for sname, side in (("L", -1), ("R", 1)):
        pose["palp%s_a" % sname].rotation_euler = (math.radians(lift), 0, math.radians(spread * side))
        pose["palp%s_b" % sname].rotation_euler = (math.radians(-lift * .6), 0, 0)


def action(name, frames, pose_at):
    act = bpy.data.actions.get(name)
    if act:
        bpy.data.actions.remove(act)
    act = bpy.data.actions.new(name)
    act.use_fake_user = True
    rig.animation_data.action = act
    for f in frames:
        reset()
        pose_at(f)
        key(f)
    return act


def idle(f):
    t = f / 48.0 * math.tau
    pose["abdomen"].rotation_euler = (math.radians(4 * math.sin(t)), 0, math.radians(2 * math.sin(t * .5)))
    pose["root"].location = (0, .012 * math.sin(t), 0)
    palps(12 * max(0.0, math.sin(t * 2)), 6 * math.sin(t))
    for a, b, c, side, i, sname in LEGS:
        leg(a, 2.5 * math.sin(t + i * .9), 0, side, -1.5 * math.sin(t + i))


def walk(f):
    t = f / 16.0 * math.tau
    for a, b, c, side, i, sname in LEGS:
        group_a = (i + (0 if sname == "L" else 1)) % 2 == 0
        phase = t + (0 if group_a else math.pi)
        swing = 24 * math.sin(phase)
        lift = max(0.0, 26 * math.cos(phase))
        leg(a, lift, swing, side, -lift * .5, lift * .3)
    pose["root"].location = (0, .03 * abs(math.sin(t)), 0)
    pose["abdomen"].rotation_euler = (math.radians(3 * math.sin(t * 2)), 0, math.radians(4 * math.sin(t)))
    palps(10 * math.sin(t * 2), 4)


def attack(f):
    # empina, abre os palpos, ergue as patas da frente e crava as presas
    up = math.sin(min(f, 8) / 8 * math.pi / 2) if f <= 8 else max(0.0, 1 - (f - 8) / 10)
    strike = 1.0 if 8 <= f <= 12 else 0.0
    pose["body"].rotation_euler = (math.radians(-30 * up + 18 * strike), 0, 0)
    pose["head"].rotation_euler = (math.radians(28 * strike), 0, 0)
    palps(40 * up, 25 * up)
    for a, b, c, side, i, sname in LEGS:
        if i == 0:
            leg(a, 60 * up - 35 * strike, 28 * up, side, -35 * up, -20 * up)
        elif i == 1:
            leg(a, 20 * up, 10 * up, side, -10 * up)
        else:
            leg(a, 3 * up, 0, side, 8 * up)


def leap(f):
    crouch = min(1.0, f / 5) if f <= 5 else 0.0
    air = math.sin(max(0.0, min(1.0, (f - 5) / 12)) * math.pi)
    pose["root"].location = (0, -.12 * crouch + .55 * air, 0)
    pose["body"].rotation_euler = (math.radians(-18 * air), 0, 0)
    palps(30 * air, 20 * air)
    for a, b, c, side, i, sname in LEGS:
        leg(a, -14 * crouch + 32 * air, (24 if i < 2 else -22) * air, side, 22 * crouch - 28 * air, -10 * air)


def hit(f):
    k = math.sin(min(1.0, f / 10) * math.pi)
    pose["body"].rotation_euler = (math.radians(16 * k), 0, math.radians(9 * k))
    pose["abdomen"].rotation_euler = (math.radians(-10 * k), 0, 0)
    palps(-15 * k)
    for a, b, c, side, i, sname in LEGS:
        leg(a, 20 * k, -10 * k, side, -12 * k)


def death(f):
    k = min(1.0, f / 18)
    pose["root"].rotation_euler = (0, math.radians(180 * k), 0)
    pose["root"].location = (0, .5 * math.sin(k * math.pi) - .05 * k, 0)
    palps(50 * k, 20 * k)
    for a, b, c, side, i, sname in LEGS:
        leg(a, 60 * k, 0, side, 75 * k, 50 * k)


action("Idle", range(0, 49, 6), idle)
action("Walk", range(0, 17, 2), walk)
action("Attack", [0, 4, 8, 10, 12, 15, 18], attack)
action("Leap", [0, 3, 5, 8, 11, 14, 17, 20], leap)
action("Hit", [0, 3, 5, 7, 10], hit)
action("Death", [0, 4, 8, 12, 16, 18, 24, 30], death)
rig.animation_data.action = bpy.data.actions["Idle"]
reset()

# ---- exportar ------------------------------------------------------------------------------------------------
bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True)
body_obj.select_set(True)
bpy.context.view_layer.objects.active = rig
path = os.path.join(OUT, "Spider_Skinned.glb")
bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_animations=True,
                          export_animation_mode="ACTIONS", export_skins=True, export_yup=True)
print("VORTEX_SPIDER exportada", path, os.path.getsize(path), "bytes; blocos", len(mesh.polygons) // 6)
