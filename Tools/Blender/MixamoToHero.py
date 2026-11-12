"""O herói do Mixamo como veio: a malha, o esqueleto completo e os pesos do Mixamo, com os clipes sem conversão — sem
janela.

blender -b --factory-startup -P Tools/Blender/MixamoToHero.py -- <raiz do projeto> <raça> [--clips <outra raça>] [clipe ...]

31/10: antes os clipes eram copiados para o esqueleto do aventureiro (15 ossos): a coluna virava um bloco, a clavícula
e a ponta do pé sumiam e cada osso perdia o giro em volta de si (cotovelo e mão tortos). Agora:
1. a malha e o esqueleto saem do FBX "With Skin" da raça (Data/Heroes/Mixamo/<raça>/<raça>.fbx, ou o maior FBX da
   pasta), no tamanho do herói antigo (Data/Heroes/<raça>/Hero_<raça>.glb);
2. cada clipe só copia, osso por osso, o giro que o Mixamo deu (o mesmo esqueleto: nada muda). Um clipe de outra
   raça (--clips anao) entra igual: os esqueletos do Mixamo têm os mesmos ossos;
3. os cortes (golpe 1, 2, 3, as partes do pulo) e os tempos dos golpes continuam (SPLITS); o deslocamento para a
   frente sai (quem anda no jogo é a cápsula) e a velocidade do clipe fica anotada;
4. ossos extras sem peso, presos aos do Mixamo, com os nomes que o jogo usa para a arma e para medir (Hand_R, Sword,
   Hand_L, Sheath, Head, Torso, Hips, Foot_L/R): no mesmo lugar e no mesmo giro de antes em relação à mão, então a
   arma pega igual;
5. sai Data/Heroes/<raça>_Mixamo/Hero_<raça>.glb (malha, esqueleto e um clipe por faixa) e clips.json. Na Unreal:
   Tools/ImportMixamoHeroes.py.
"""
import json
import math
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Quaternion, Vector

args = sys.argv[sys.argv.index("--") + 1:]
# --clips <raça>: completa com os clipes do Mixamo de outra raça: os da própria raça vêm primeiro e o que faltar
# (emotes, danças, nado...) sai dos dela
CLIP_RACE = None
if "--clips" in args:
    i = args.index("--clips")
    CLIP_RACE = args[i + 1]
    del args[i:i + 2]
# --beard (o anão): a barba segue a cabeça, não o peito (como o RigHero.py --beard fazia no herói antigo)
BEARD = "--beard" in args
if BEARD:
    args.remove("--beard")
# --flatface: os buracos dos olhos, das sobrancelhas e da boca ficam lisos (as regiões "flat" de
# Data/Heroes/<raça>_Mixamo/Face/face.json, de Tools/MakeHeroFaces.py): o rosto animado é desenhado por cima
FLAT_FACE = "--flatface" in args
if FLAT_FACE:
    args.remove("--flatface")
# --skin <fbx> --out <pasta> (01/11): outra malha no mesmo esqueleto da raça (o corpo feminino do humano de blocos,
# RigBlockCharacter.py --feminino): sai na pasta dada, com o mesmo nome Hero_<raça>.glb (o jogo procura pelo nome)
SKIN = OUT_DIR = None
for flag in ("--skin", "--out"):
    if flag in args:
        i = args.index(flag)
        if flag == "--skin":
            SKIN = args[i + 1]
        else:
            OUT_DIR = args[i + 1]
        del args[i:i + 2]
ROOT, RACE = args[0], args[1]
ONLY = set(args[2:])
CLIP_RACE = CLIP_RACE or RACE
# várias raças separadas por vírgula (o elfo usa os do humano e os do anão): na ordem
CLIP_RACES = CLIP_RACE.split(",")
CLIP_RACE = CLIP_RACES[0]
SRC = os.path.join(ROOT, "Data", "Heroes", "Mixamo", CLIP_RACE)
OWN = os.path.join(ROOT, "Data", "Heroes", "Mixamo", RACE)
OUT = OUT_DIR or os.path.join(ROOT, "Data", "Heroes", RACE + "_Mixamo")
P = "mixamorig:"

# FBX do Mixamo -> nome no jogo (UVortexAnimInstance pede por esses nomes). O que não está aqui entra com o nome do
# arquivo (sem a raça), para usar depois
NAMES = {
    # 01/11: andar e corrida lenta novos (os antigos ficam com outro nome, para usar depois)
    "anao_Idle": "Idle", "Unarmed Walk Forward": "WalkOld", "anao_Jog": "Jog", "anao_Run": "RunOld", "Running": "RunStartOld", "anao_Sprint": "Sprint",
    "walk anao": "Walk", "Slow Run anao": "Run", "Slow Run human": "Run",
    # correndo com a arma na mão (01/11: o mesmo FBX nos dois; o anão usa o do humano)
    "Sword And Shield Run": "RunArmed",
    # o elfo rigado no Mixamo (01/11)
    "Walking elfo": "Walk", "Run Forward elfo": "Run",
    "Run Forward demonio": "Run", "Run Forward esqueleto": "Run", "gigante correndo": "Run", "barbaro andando": "Walk",
    "anao_Walk_Back": "Walk_Back", "morte": "Death", "nadando": "Swim", "flutuando": "Swim_Idle", "pulo parado": "Jump",
    # os do humano (no modelo dele)
    "Walking human": "Walk", "Standing React Death Backward": "Death",
}
SKIP = {"anao_Walk", "_duplicado_Idle"}     # repetidos (o andar que você escolheu é o de clips/; "Running" é a arrancada, não um laço)
# clipes que a raça não usa (Data/Heroes/Mixamo/<raça>/fora.json, {"clipes": [...]}): nem os dela nem os que viriam das
# outras raças (o humano de blocos não tem a corrida rápida: o Shift é a corrida lenta)
_fora = os.path.join(OWN, "fora.json")
OMIT = set(json.load(open(_fora, encoding="utf-8"))["clipes"]) if os.path.exists(_fora) else set()
# os clipes que andam: o deslocamento sai e a velocidade fica anotada (os outros só perdem o que escorregam)
LOCOMOTION = {"Walk", "Jog", "Run", "Sprint", "Walk_Back", "Swim", "RunArmed"}
# um FBX em pedaços: (nome, primeiro quadro, último quadro, opções), quadros do Mixamo contando do 1. Opções:
#   ground: o quadril não passa da altura de pé (no pulo quem sobe é a cápsula do jogo; as pernas encolhem no ar)
#   time: duração no jogo (s); o pedaço estica ou encolhe para caber
#   strike, at: o quadro do golpe (a mão mais rápida) cai no instante "at" (s), o meio da janela de dano da arma
#     (UVortexWeaponDefinition: Duration, ActiveStart-ActiveEnd), e o resto estica dos dois lados
# Golpes "_Full" mexem o corpo inteiro (UVortexAnimInstance); os outros só o tronco e os braços, as pernas seguem andando.
# Sem clipe próprio (o cajado, pegar do chão), o golpe sai de pedaços dos outros
SPLITS = {
    "pulo parado": [("Jump_Start", 22, 31, {"ground": True}), ("Jump_Loop", 31, 37, {"ground": True}),
                    ("Jump_Land", 40, 58, {"ground": True})],
    # espada longa (DA_DefaultSword): 0,62 / 0,62 / 0,94 s, golpe em 0,26 / 0,27 / 0,42
    "Stable Sword Inward Slash": [("Atk_Espada_1", 11, 56, {"strike": 37, "at": .26, "time": .62}),
                                  ("Atk_Cajado_2", 11, 56, {"strike": 37, "at": .29, "time": .67})],
    "golpe espada vertical": [("Atk_Espada_2", 5, 35, {"strike": 20, "at": .27, "time": .62}),
                              ("Atk_Cajado_1", 5, 35, {"strike": 20, "at": .29, "time": .67})],
    "terceiro golpe giratorio": [("Atk_Espada_3_Full", 1, 40, {"strike": 19, "at": .42, "time": .94})],
    # martelo de Bram: 0,8 / 0,8 / 1,1 s, golpe em 0,45 / 0,45 / 0,62 (o terceiro é o pulo com pancada no chão)
    "espada 2 maos combo": [("Atk_Martelo_1", 1, 33, {"strike": 20, "at": .45, "time": .8}),
                            ("Atk_Martelo_2", 31, 56, {"strike": 41, "at": .45, "time": .8}),
                            ("Atk_Cajado_3", 56, 99, {"strike": 69, "at": .48, "time": 1.12})],
    "golpe pesado de cima": [("Atk_Martelo_3_Full", 41, 100, {"strike": 73, "at": .62, "time": 1.1})],
    # pegar do chão (02/11): o "Picking Up" do Mixamo, só o abaixar-pegar-levantar (quadros 22 a 168, 4,9 s), em 1,7 s.
    # (Antes era um pedaço do golpe pesado de cima: o herói parecia deitar no chão)
    "anao_Pegar": [("Pegar", 22, 168, {"time": 1.7})],
    # adagas gêmeas: 0,42 / 0,42 / 0,75 s, golpe em 0,17 / 0,17 / 0,32
    "One Hand Sword Combo": [("Atk_Adagas_1", 23, 41, {"strike": 31, "at": .17, "time": .42}),
                             ("Atk_Adagas_2", 51, 71, {"strike": 60, "at": .17, "time": .42}),
                             ("Atk_Adagas_3", 79, 111, {"strike": 91, "at": .32, "time": .75})],
    # arco: puxar a corda (segurando o botão) e soltar (a flecha sai em 0,05 s). O soltar acaba antes de o Mixamo abaixar
    # o arco (01/11: com o resto do clipe o arco deitava na mão e parecia que ele jogava o arco)
    "Shooting Arrow": [("Arco_Draw", 38, 103, {"time": .8}), ("Arco_Release", 125, 134, {"strike": 127, "at": .05, "time": .35})],
    # esquiva: a cambalhota do "correndo e rolando" no tempo da esquiva (0,58 s)
    "correndo e rolando": [("Dodge", 9, 57, {"time": .62})],
    # tranco ao levar dano (o tronco; as pernas seguem)
    "Receive Uppercut To The Face": [("Hit", 1, 20, {})],
    # sacar a arma ao trocar de arma
    "desenbaiando": [("Sacar", 12, 46, {"time": .7})],
    # ---- os do humano (no modelo dele); o que não está aqui sai dos do anão
    "Fast Run human": [("Sprint", 1, 17, {})],
    # a cambalhota (01/11): o mergulho, o rolamento e o levantar, no tempo da esquiva (0,58 s). Para trás e para os lados
    # saem dela no fim (make_derived)
    # (16: ainda de pé, inclinando; 60: de pé de novo) — começar no mergulho (24) fazia o corpo aparecer deitado de
    # repente, e com o empurrão da esquiva parecia teletransporte
    "Stand To Roll humano": [("Dodge", 16, 60, {"time": .7})],
    # ---- o humano de blocos (01/11, modelando ainda/): os clipes que precisam de corte ou tempo; os outros entram pelo
    # nome do arquivo (humano_<clipe>.fbx)
    "humano_golpe1": [("Atk_Espada_1", 6, 40, {"strike": 19, "at": .26, "time": .62})],
    "humano_golpe2": [("Atk_Espada_2", 10, 45, {"strike": 26, "at": .27, "time": .62})],
    "humano_giratorio": [("Atk_Espada_3_Full", 1, 40, {"strike": 17, "at": .42, "time": .94})],
    "humano_rolamento": [("Dodge", 16, 43, {"time": .7})],
    "humano_pulo": [("Jump_Start", 1, 6, {"ground": True}), ("Jump_Loop", 7, 16, {"ground": True}),
                    ("Jump_Land", 17, 26, {"ground": True})],
    "humano_arco_puxar": [("Arco_Draw", 1, 32, {"time": .8})],
    "Stand To Roll anao": [("Dodge", 16, 60, {"time": .7})],
    "Jumping humna": [("Jump_Start", 7, 16, {"ground": True}), ("Jump_Loop", 22, 28, {"ground": True}),
                      ("Jump_Land", 34, 52, {"ground": True})],
    "Standing Melee Attack Horizontal": [("Atk_Espada_1", 15, 55, {"strike": 30, "at": .26, "time": .62})],
    "Standing Melee Attack Downward": [("Atk_Espada_2", 10, 50, {"strike": 26, "at": .27, "time": .62})],
    "Standing Melee Attack 360 Low": [("Atk_Espada_3_Full", 5, 60, {"strike": 29, "at": .42, "time": .94})],
    "One Hand Club Combo": [("Atk_Martelo_1", 5, 32, {"strike": 19, "at": .45, "time": .8}),
                            ("Atk_Martelo_2", 30, 56, {"strike": 42, "at": .45, "time": .8}),
                            ("Atk_Martelo_3_Full", 56, 100, {"strike": 67, "at": .62, "time": 1.1})],
    "Dual Weapon Combo": [("Atk_Adagas_1", 12, 42, {"strike": 34, "at": .17, "time": .42}),
                          ("Atk_Adagas_2", 44, 66, {"strike": 55, "at": .17, "time": .42}),
                          ("Atk_Adagas_3", 68, 100, {"strike": 80, "at": .32, "time": .75})],
    "Standing Draw Arrow": [("Arco_Draw", 1, 32, {"time": .8})],
}


# ossos extras (sem peso) com os nomes do herói antigo: (nome, pai do Mixamo ou extra, como acha o lugar)
#   "hand": a mão antiga, girada como o MixamoToHero antigo girava (a arma pega no mesmo jeito em relação à mão)
#   "child": filho de outro extra, com o mesmo afastamento do herói antigo
#   "rest": o lugar e o giro do osso antigo de pé (tronco, cabeça e pés: os dois corpos de pé iguais)
HELPERS = [
    ("Hand_R", P + "RightHand", "hand"), ("Socket_Weapon_R", "Hand_R", "child"), ("Sword", "Socket_Weapon_R", "child"),
    ("Hand_L", P + "LeftHand", "hand"),
    ("Torso", P + "Spine2", "rest"), ("Sheath", "Torso", "child"), ("Sword_Hilt", "Sheath", "child"),
    ("Head", P + "Head", "rest"), ("Hips", P + "Hips", "rest"), ("Foot_L", P + "LeftFoot", "rest"), ("Foot_R", P + "RightFoot", "rest"),
]
OLD_PARENT = {"Socket_Weapon_R": "Hand_R", "Sword": "Socket_Weapon_R", "Sheath": "Torso", "Sword_Hilt": "Sheath"}
HAND = {"Hand_R": ("Forearm_R", P + "RightForeArm"), "Hand_L": ("Forearm_L", P + "LeftForeArm")}


def log(*parts):
    print("MIXAMO_HERO", " ".join(str(p) for p in parts))


def go(frame):
    """Põe a cena num quadro fracionário (o Mixamo é amostrado entre os quadros quando o pedaço estica)."""
    scene.frame_set(int(frame), subframe=frame - int(frame))


def rest(obj, name):
    return obj.matrix_world @ obj.data.bones[name].matrix_local


def import_fbx(path, images=False):
    """Importa um FBX do Mixamo. Sem as imagens (01/11): os FBX "With Skin" do anão trazem as texturas de 8K
    embutidas, e só abrir um levava o Blender de 0,6 a 7 GB (e travava o PC). A textura do herói vem do <raça>_cor.png."""
    before = set(bpy.data.objects)
    import bpy_extras.image_utils as image_utils
    original = image_utils.load_image
    if not images:
        def tiny(*a, **k):
            img = bpy.data.images.get("_vazio") or bpy.data.images.new("_vazio", 1, 1)
            return img
        image_utils.load_image = tiny
    try:
        bpy.ops.import_scene.fbx(filepath=path, automatic_bone_orientation=False, ignore_leaf_bones=False, use_image_search=False)
    finally:
        image_utils.load_image = original
    new = [o for o in bpy.data.objects if o not in before]
    return next((o for o in new if o.type == "ARMATURE"), None), new


KEEP_IMAGES = set()      # as imagens do herói (o resto vem com cada FBX "With Skin" e sai logo)


def drop(objects):
    for o in objects:
        bpy.data.objects.remove(o, do_unlink=True)
    # 01/11: cada FBX do anão traz a textura de 8K embutida; sem tirar à mão, 70 clipes passavam de 8 GB e travavam o PC
    for img in list(bpy.data.images):
        if img.name not in KEEP_IMAGES:
            bpy.data.images.remove(img)
    for mat in list(bpy.data.materials):
        if not mat.users:
            bpy.data.materials.remove(mat)
    # e as malhas e esqueletos de cada FBX (60 mil vértices cada; sem isso a memória crescia a cada clipe)
    for me in list(bpy.data.meshes):
        if not me.users:
            bpy.data.meshes.remove(me)
    for ar in list(bpy.data.armatures):
        if not ar.users:
            bpy.data.armatures.remove(ar)
    bpy.data.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)
    # cada FBX "With Skin" traz malha, material e textura de novo: sem limpar, 70 clipes estouravam a memória
    bpy.data.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)


bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.render.fps = 30

files = []
for base in ([OWN] if OWN != SRC else []) + [os.path.join(ROOT, "Data", "Heroes", "Mixamo", r) for r in CLIP_RACES]:
    for folder in (base, os.path.join(base, "clips"), os.path.join(base, "emotes")):
        if os.path.isdir(folder):
            files += [os.path.join(folder, f) for f in sorted(os.listdir(folder)) if f.lower().endswith(".fbx")]

# ---- o herói antigo: o tamanho e o lugar dos ossos que o jogo usa
before = set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=os.path.join(ROOT, "Data", "Heroes", RACE, "Hero_%s.glb" % RACE))
old_objects = [o for o in bpy.data.objects if o not in before]
old = next(o for o in old_objects if o.type == "ARMATURE")
old_body = max((o for o in old_objects if o.type == "MESH" and o.parent == old), key=lambda o: len(o.data.vertices))
old_height = max((old_body.matrix_world @ v.co).z for v in old_body.data.vertices)
old_rest = {b.name: rest(old, b.name) for b in old.data.bones}
old_side = (old_rest["Hand_L"].translation.x > 0)
drop(old_objects)

# ---- a malha e o esqueleto do Mixamo
skin = SKIN or os.path.join(OWN, RACE + ".fbx")
if not os.path.exists(skin):
    # o FBX "With Skin" é o grande (os "Without Skin" têm uns 200 KB)
    skin = max((f for f in files if f.startswith(OWN)), key=os.path.getsize)
hero, objects = import_fbx(skin)
# a cor do herói (sem as imagens do FBX): o <raça>_cor.png que foi para o Mixamo
color_png = os.path.join(ROOT, "Data", "Heroes", "Mixamo", RACE + "_cor.png")
hero.name = "Armature"
body = max((o for o in objects if o.type == "MESH"), key=lambda o: len(o.data.vertices))
for mat in [m for m in body.data.materials if m]:
    if not os.path.exists(color_png):
        break
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = next((n for n in nt.nodes if n.type == "BSDF_PRINCIPLED"), None)
    if not bsdf:
        continue
    tex = next((n for n in nt.nodes if n.type == "TEX_IMAGE"), None) or nt.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(color_png, check_existing=True)
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
KEEP_IMAGES.update(n.image.name for m in body.data.materials if m and m.use_nodes for n in m.node_tree.nodes if n.type == "TEX_IMAGE" and n.image)
drop([o for o in objects if o not in (hero, body)])
if hero.animation_data:
    hero.animation_data.action = None
for pb in hero.pose.bones:
    pb.matrix_basis = Matrix.Identity(4)
    pb.rotation_mode = "QUATERNION"
bpy.context.view_layer.update()
height = max((body.matrix_world @ v.co).z for v in body.data.vertices)
k_size = old_height / height
hero.matrix_world = Matrix.Scale(k_size, 4) @ hero.matrix_world
bpy.context.view_layer.update()
if (rest(hero, P + "LeftHand").translation.x > 0) != old_side:
    hero.matrix_world = Matrix.Rotation(3.14159265, 4, "Z") @ hero.matrix_world
    bpy.context.view_layer.update()
log("pele", os.path.basename(skin), "altura %.2f -> %.2f m" % (height, old_height), len(hero.data.bones), "ossos")

# ---- faces do avesso (01/11): ~1% dos triângulos do Mixamo tem a ordem dos vértices ao contrário da normal. O Blender
# mostra os dois lados; a Unreal só o da frente, e no jogo ficavam buracos (o peito e a barba do anão). Desvira essas
# faces e mantém as normais de antes
def fix_flipped(obj):
    me = obj.data
    corner = [Vector(c.vector) for c in me.corner_normals]
    keep = {}
    flip = set()
    for poly in me.polygons:
        avg = Vector()
        for li in poly.loop_indices:
            avg += corner[li]
            keep[(poly.index, me.loops[li].vertex_index)] = corner[li]
        if avg.length > 1e-6 and avg.dot(poly.normal) < 0:
            flip.add(poly.index)
    if not flip:
        return 0
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.faces.ensure_lookup_table()
    for i in flip:
        bm.faces[i].normal_flip()
    bm.to_mesh(me)
    bm.free()
    me.update()
    me.normals_split_custom_set([keep[(poly.index, me.loops[li].vertex_index)] for poly in me.polygons for li in poly.loop_indices])
    me.update()
    return len(flip)


log("faces do avesso desviradas: %d" % fix_flipped(body))

# ---- o rosto liso: cada região vai para a altura da pele logo abaixo dela (a bochecha); o que está bem à frente (o
# cabelo da franja, o nariz, o bigode) fica onde está
if FLAT_FACE:
    with open(os.path.join(ROOT, "Data", "Heroes", RACE + "_Mixamo", "Face", "face.json"), encoding="utf-8") as f:
        face = json.load(f)
    cx, _, cz = face["rect"]["center"]
    half = face["rect"]["half"]
    mw = body.matrix_world
    inv = mw.inverted()
    head_z = rest(hero, P + "Head").translation.z
    points = [mw @ v.co for v in body.data.vertices]
    normals = [(mw.to_3x3() @ v.normal).normalized() for v in body.data.vertices]
    uvs = [((p.x - cx) / (2 * half) + .5, (p.z - cz) / (2 * half) + .5) for p in points]
    front = [i for i, p in enumerate(points) if p.z > head_z - .02 and normals[i].y < -.6]
    y_front = min(points[i].y for i in front)
    moved = 0
    new_normal = {}
    inv3 = inv.to_3x3()
    for u0, v0, w, h in face["flat"]:
        # o plano da pele em volta da região (a testa e a bochecha não estão na mesma profundidade): y = a + b*x + c*z,
        # pelos vértices da frente numa moldura em volta dela (sem o que está muito à frente: cabelo, nariz)
        m = .05
        ring = [i for i in front if u0 - m <= uvs[i][0] <= u0 + w + m and v0 - m <= uvs[i][1] <= v0 + h + m
                and not (u0 <= uvs[i][0] <= u0 + w and v0 <= uvs[i][1] <= v0 + h)]
        if len(ring) < 12:
            continue
        ys = sorted(points[i].y for i in ring)
        mid = ys[len(ys) // 2]
        ring = [i for i in ring if abs(points[i].y - mid) < .03]
        S = [[0.0] * 3 for _ in range(3)]
        R = [0.0] * 3
        for i in ring:
            row = (1.0, points[i].x, points[i].z)
            for r in range(3):
                R[r] += row[r] * points[i].y
                for c in range(3):
                    S[r][c] += row[r] * row[c]
        coef = list(Matrix(S).inverted_safe() @ Vector(R))
        plane = lambda q: coef[0] + coef[1] * q.x + coef[2] * q.z
        for i, q in enumerate(points):
            u, v = uvs[i]
            if not (u0 <= u <= u0 + w and v0 <= v <= v0 + h) or q.z < head_z - .02 or q.y > y_front + .16:
                continue
            target = plane(q)
            if q.y < target - .03:
                continue                 # bem à frente: cabelo, nariz
            q = q.copy()
            q.y = target
            body.data.vertices[i].co = inv @ q
            # a normal do plano (para a frente): a antiga apontava para dentro do buraco e marcava vincos na luz
            new_normal[i] = (inv3 @ Vector((coef[1], -1.0, coef[2]))).normalized()
            moved += 1
    body.data.update()
    me = body.data
    normals = [Vector(new_normal[l.vertex_index]) if l.vertex_index in new_normal else Vector(me.corner_normals[l.index].vector)
               for l in me.loops]
    me.normals_split_custom_set(normals)
    me.update()
    log("rosto liso: %d vértices" % moved)

# ---- a barba: os pedaços da frente, abaixo do queixo, com a cor do cabelo (ou das contas douradas das tranças), passam
# para a cabeça; só a ponta guarda um pouco do peito. O Mixamo a prendia no peito, e virar a cabeça a deixava parada
if BEARD:
    image = next((n.image for m in body.data.materials if m and m.use_nodes for n in m.node_tree.nodes
                  if n.type == "TEX_IMAGE" and n.image), None)
    if image:
        W, H = image.size
        px = image.pixels[:]
        uv = body.data.uv_layers.active.data
        color = [None] * len(body.data.vertices)
        for loop in body.data.loops:
            if color[loop.vertex_index] is None:
                u, v = uv[loop.index].uv
                i = ((int((v % 1) * H) % H) * W + int((u % 1) * W) % W) * 4
                color[loop.vertex_index] = px[i:i + 3]
        hairy = lambda c: c[0] > .38 and c[1] < .3 and c[2] < .15 and c[0] > 2 * c[1]
        golden = lambda c: c[0] > .5 and c[1] > .35 and c[2] < .3 and c[1] < .8 * c[0]
        head_group = body.vertex_groups.get(P + "Head") or body.vertex_groups.new(name=P + "Head")
        chest_group = body.vertex_groups.get(P + "Spine2") or body.vertex_groups.new(name=P + "Spine2")
        bm = bmesh.new()
        bm.from_mesh(body.data)
        bm.verts.ensure_lookup_table()
        points = [body.matrix_world @ v.co for v in body.data.vertices]
        neck, tall = rest(hero, P + "Neck").translation, old_height
        seen, moved = set(), 0
        for start in bm.verts:
            if start.index in seen:
                continue
            piece, stack = [], [start]
            seen.add(start.index)
            while stack:
                a = stack.pop()
                piece.append(a.index)
                for e in a.link_edges:
                    b = e.other_vert(a)
                    if b.index not in seen:
                        seen.add(b.index)
                        stack.append(b)
            zs = [points[i].z for i in piece]
            if max(zs) > neck.z + .13 * tall or min(zs) < neck.z - .2 * tall:
                continue
            if sum(1 for i in piece if hairy(color[i]) or golden(color[i])) < .4 * len(piece):
                continue
            if sum(1 for i in piece if points[i].y < neck.y - .05 * tall) < .5 * len(piece):
                continue
            for i in piece:
                t = min(1.0, max(0.0, (points[i].z - (neck.z - .2 * tall)) / (.12 * tall)))
                for g in body.vertex_groups:
                    g.remove([i])
                head_group.add([i], .6 + .4 * t, "REPLACE")
                if t < 1:
                    chest_group.add([i], .4 * (1 - t), "REPLACE")
                moved += 1
        bm.free()
        log("barba: %d vértices passaram para a cabeça" % moved)

# ---- os ossos extras
world = {}
for name, parent, how in HELPERS:
    if how == "hand":
        fore_old, fore_mix = HAND[name]
        Ro = old_rest[name].to_quaternion()
        d_old = old_rest[name].translation - old_rest[fore_old].translation
        d_mix = rest(hero, parent).translation - rest(hero, fore_mix).translation
        Q = d_mix.normalized().rotation_difference(d_old.normalized())
        world[name] = Matrix.LocRotScale(rest(hero, parent).translation, Q.inverted() @ Ro, None)
    elif how == "child":
        o_parent = OLD_PARENT[name]
        relative = old_rest[o_parent].inverted() @ old_rest[name]
        world[name] = world[o_parent] @ relative
    else:
        world[name] = Matrix.LocRotScale(old_rest[name].translation, old_rest[name].to_quaternion(), None)
bpy.context.view_layer.objects.active = hero
bpy.ops.object.mode_set(mode="EDIT")
inv = hero.matrix_world.inverted()
turn = hero.matrix_world.to_quaternion().inverted()
for name, parent, how in HELPERS:
    bone = hero.data.edit_bones.new(name)
    W = world[name]
    bone.head = inv @ W.translation
    bone.tail = bone.head + Vector((0, .05 / k_size, 0))
    bone.matrix = Matrix.LocRotScale(inv @ W.translation, turn @ W.to_quaternion(), None)
    bone.length = .05 / k_size
    bone.parent = hero.data.edit_bones[parent]
    bone.use_deform = False
bpy.ops.object.mode_set(mode="OBJECT")
for pb in hero.pose.bones:
    pb.rotation_mode = "QUATERNION"
MIX_BONES = [b.name for b in hero.data.bones if b.name.startswith(P)]
hips_rest = rest(hero, P + "Hips").translation.copy()


def directions(obj):
    """O referencial de cada osso do Mixamo no descanso: a direção para o filho principal (no peito, o pescoço; na
    ponta, a do pai até ele) e a frente do corpo (-Y) perpendicular a ela. Só a direção deixava o osso livre para
    girar em volta dela: o peito (que tem o pescoço e os dois ombros) torcia 60°, entortando o pescoço e enfiando um
    braço no corpo."""
    out = {}
    for b in obj.data.bones:
        if not b.name.startswith(P):
            continue
        kids = [c for c in b.children if c.name.startswith(P)]
        head = (obj.matrix_world @ b.head_local)
        if kids:
            spine = [c for c in kids if any(k in c.name for k in ("Spine", "Neck", "Head"))]
            kid = spine[0] if spine else max(kids, key=lambda c: len(c.children_recursive))
            d = (obj.matrix_world @ kid.head_local) - head
        elif b.parent:
            d = head - (obj.matrix_world @ b.parent.head_local)
        else:
            d = Vector((0, 0, 1))
        d = d.normalized() if d.length > 1e-6 else Vector((0, 0, 1))
        front = Vector((0, -1, 0))
        if abs(d.dot(front)) > .95:
            front = Vector((0, 0, 1))
        side = d.cross(front).normalized()
        front = side.cross(d).normalized()
        out[b.name] = Matrix((d, front, side)).transposed().to_quaternion()
    return out


hero_dirs = directions(hero)
hero_rest_q = {n: rest(hero, n).to_quaternion() for n in MIX_BONES}
hero_turn = hero.matrix_world.to_quaternion()
# o giro de cada osso em relação ao pai, no descanso (para passar do mundo para o giro do osso)
hero_rel = {n: ((hero.data.bones[n].parent.matrix_local.inverted() @ hero.data.bones[n].matrix_local) if hero.data.bones[n].parent
                else hero.data.bones[n].matrix_local).to_quaternion() for n in MIX_BONES}
ORDER = [n for n in MIX_BONES]                     # a ordem do esqueleto: o pai antes dos filhos


def convert(path, name, span=None, options=None):
    actions_before = set(bpy.data.actions)
    mix, objects = import_fbx(path)
    if not mix or not mix.animation_data or not mix.animation_data.action:
        log("SEM ANIMAÇÃO", path)
        drop(objects)
        return None
    act = mix.animation_data.action
    first, last = (int(round(f)) for f in act.frame_range)
    if span:
        first, last = span
    scene.frame_set(first)
    src_hips = rest(mix, P + "Hips").translation
    k = hips_rest.z / max(.01, src_hips.z)          # os dois com os pés no chão (z = 0)

    # os quadros do Mixamo que viram cada quadro do jogo (com "time", fracionários: o pedaço estica ou encolhe)
    options = options or {}
    frames = list(range(first, last + 1))
    if "time" in options:
        count = max(2, int(round(options["time"] * scene.render.fps)) + 1)
        strike, at = options.get("strike"), options.get("at")
        frames = []
        for i in range(count):
            t = i / scene.render.fps
            if strike is None:
                frames.append(first + (last - first) * i / (count - 1))
            elif t <= at:
                frames.append(first + (strike - first) * t / max(1e-3, at))
            else:
                frames.append(strike + (last - strike) * min(1.0, (t - at) / max(1e-3, options["time"] - at)))
    hips = []
    for f in frames:
        go(f)
        hips.append((mix.matrix_world @ mix.pose.bones[P + "Hips"].matrix).translation.copy())
    drift = (hips[-1] - hips[0]) * k
    drift.z = 0
    # 01/11: o clipe cortado no meio (a rolada começa no quadro 16 do "Quick Roll") já começa com o quadril 1,4 m à
    # frente: o corpo pulava para a frente na esquiva e voltava no fim. O primeiro quadro fica em cima da cápsula
    start = (hips[0] - src_hips) * k
    start.z = 0
    seconds = max(1e-3, (len(frames) - 1) / scene.render.fps)
    speed = drift.length / seconds * 100.0          # cm/s no tamanho do herói
    # quem anda no lugar (já veio "in place"): a velocidade é a do pé no chão, que vai para trás
    if name in LOCOMOTION and speed < 20:
        feet = []
        for f in frames:
            go(f)
            feet.append([(mix.matrix_world @ mix.pose.bones[P + side + "Foot"].matrix).translation.copy() for side in ("Left", "Right")])
        slide = []
        for i in range(1, len(frames)):
            low = min((0, 1), key=lambda s: feet[i][s].z)
            d = (feet[i][low] - feet[i - 1][low]) * k
            d.z = 0
            slide.append(d.length * scene.render.fps * 100.0)
        slide.sort()
        speed = slide[len(slide) // 2] if slide else 0.0

    out = bpy.data.actions.new(name)
    hero.animation_data_create()
    hero.animation_data.action = out
    shared = [n for n in MIX_BONES if n in mix.pose.bones]
    # 31/10: os descansos dos esqueletos do Mixamo não são iguais de um arquivo para outro (o humano tem os braços ~45°
    # girados em relação ao anão; os FBX "sem pele" do anão, 13-22° no antebraço e na mão). Copiar o giro relativo ao
    # descanso de cada osso cruzava as mãos. Agora cada osso segue no mundo o osso do clipe, descontando a diferença
    # entre as direções de descanso (no mesmo esqueleto, a diferença é zero e o giro sai igual)
    src_dirs = directions(mix)
    src_rest_q = {n: rest(mix, n).to_quaternion() for n in shared}
    fix = {n: hero_dirs[n] @ src_dirs[n].inverted() for n in shared}
    for i, f in enumerate(frames):
        go(f)
        # o giro de cada osso no mundo como o do clipe, passado para o giro do osso (pai antes do filho)
        posed = {}
        for n in ORDER:
            bone = hero.data.bones[n]
            parent = bone.parent.name if bone.parent else None
            if n in fix:
                Ws = (mix.matrix_world @ mix.pose.bones[n].matrix).to_quaternion()
                world_q = Ws @ src_rest_q[n].inverted() @ fix[n].inverted() @ hero_rest_q[n]
                A = hero_turn.inverted() @ world_q
            else:
                A = (posed[parent] if parent in posed else Quaternion()) @ hero_rel[n]
            posed[n] = A
            above = posed[parent] if parent in posed else Quaternion()
            hero.pose.bones[n].rotation_quaternion = (above @ hero_rel[n]).inverted() @ A
        bpy.context.view_layer.update()
        # o quadril: a altura e o balanço do Mixamo (na proporção dos quadris), sem andar para a frente
        pb = hero.pose.bones[P + "Hips"]
        t = i / max(1, len(frames) - 1)
        pos = hips_rest + (hips[i] - src_hips) * k - drift * t - start
        if options.get("ground"):
            pos.z = min(pos.z, hips_rest.z)
        M = hero.matrix_world @ pb.matrix
        pb.matrix = hero.matrix_world.inverted() @ Matrix.LocRotScale(pos, M.to_quaternion(), M.to_scale())
        for n in shared:
            hero.pose.bones[n].keyframe_insert("rotation_quaternion", frame=i)
        pb.keyframe_insert("location", frame=i)
    # cada clipe numa faixa (o exportador faz uma animação por faixa, com o nome dela)
    track = hero.animation_data.nla_tracks.new()
    track.name = name
    strip = track.strips.new(name, 0, out)
    try:
        if out.slots and not strip.action_slot:
            strip.action_slot = out.slots[0]
    except Exception:
        pass
    track.mute = True
    hero.animation_data.action = None
    for pb in hero.pose.bones:
        pb.matrix_basis = Matrix.Identity(4)
    drop(objects)
    for a in set(bpy.data.actions) - actions_before - {out}:
        bpy.data.actions.remove(a)
    log("%-18s %4d quadros  %6.0f cm/s  <- %s" % (name, len(frames), speed, os.path.basename(path)))
    return {"frames": len(frames), "seconds": seconds, "speed": round(speed, 1), "source": os.path.basename(path)}


report = {}
for path in files:
    stem = os.path.splitext(os.path.basename(path))[0]
    if stem in SKIP or stem == RACE:
        continue
    # anao_Danca_HipHop -> Danca_HipHop; "Guitar Playing" -> GuitarPlaying
    for part, a, b, opts in SPLITS.get(stem, []):
        if part in OMIT:
            continue
        if part in report:
            log("REPETIDO", part, path)
        elif not ONLY or part in ONLY:
            result = convert(path, part, (a, b), opts)
            if result:
                report[part] = result
    if stem in SPLITS:
        continue
    prefix = next((r + "_" for r in [RACE] + CLIP_RACES if stem.startswith(r + "_")), None)
    name = NAMES.get(stem) or (stem[len(prefix):] if prefix else "".join(w[:1].upper() + w[1:] for w in stem.split()))
    if (ONLY and name not in ONLY) or name in OMIT:
        continue
    if name in report:
        log("REPETIDO", name, path)
        continue
    result = convert(path, name)
    if result:
        report[name] = result

# ---- clipes feitos a partir dos outros (01/11): a esquiva para trás e para os lados, a arrancada e a freada


def sample(action, frame):
    """A pose (giro de cada osso e a posição do quadril) do clipe num quadro, no esqueleto do herói."""
    hero.animation_data.action = action
    try:
        hero.animation_data.action_slot = action.slots[0]
    except Exception:
        pass
    go(frame)
    bpy.context.view_layer.update()
    return {pb.name: (pb.rotation_quaternion.copy(), pb.location.copy()) for pb in hero.pose.bones}


def length(action):
    return int(round(action.frame_range[1]))


def write(name, poses, source):
    """Grava uma lista de poses como um clipe novo, numa faixa (como os convertidos)."""
    out = bpy.data.actions.new(name)
    hero.animation_data.action = out
    for i, pose in enumerate(poses):
        for n, (q, loc) in pose.items():
            pb = hero.pose.bones[n]
            pb.rotation_quaternion = q
            pb.keyframe_insert("rotation_quaternion", frame=i)
            if n == P + "Hips":
                pb.location = loc
                pb.keyframe_insert("location", frame=i)
    track = hero.animation_data.nla_tracks.new()
    track.name = name
    strip = track.strips.new(name, 0, out)
    try:
        if out.slots and not strip.action_slot:
            strip.action_slot = out.slots[0]
    except Exception:
        pass
    track.mute = True
    hero.animation_data.action = None
    report[name] = {"frames": len(poses), "seconds": (len(poses) - 1) / scene.render.fps, "speed": 0.0, "source": source}
    log("%-18s %4d quadros  (feito de %s)" % (name, len(poses), source))


def blend(a, b, w):
    return {n: (a[n][0].slerp(b[n][0], w), a[n][1].lerp(b[n][1], w)) for n in a}


def smooth(t):
    t = max(0.0, min(1.0, t))
    return t * t * t * (t * (6 * t - 15) + 10)


# a pose da montaria (graus): coxas para a frente e abertas, o joelho dobrado, os braços nas rédeas, o tronco inclinado
RIDE = {"coxa": 78, "abre": 22, "joelho": 80, "braco": 28, "antebraco": 55, "tronco": 6}


def make_derived():
    actions = {a.name: a for a in bpy.data.actions}
    fps = scene.render.fps
    hips = P + "Hips"
    if "Dodge" in actions and "Dodge_Back" not in report:
        dodge = actions["Dodge"]
        frames = [sample(dodge, f) for f in range(length(dodge) + 1)]
        # para trás: o rolamento de trás para a frente
        write("Dodge_Back", frames[::-1], "Dodge ao contrário")
        # para os lados: o corpo vira 90° para o lado e rola; no fim volta de frente
        up = hero.matrix_world.to_quaternion().inverted() @ Vector((0, 0, 1))
        for name, sign in (("Dodge_Left", 1), ("Dodge_Right", -1)):
            poses = []
            n = len(frames)
            for i, pose in enumerate(frames):
                t = i / max(1, n - 1)
                turn = smooth(t / .15) * (1 - smooth((t - .7) / .3))
                q, loc = pose[hips]
                pose = dict(pose)
                # o giro é no espaço do esqueleto (em volta do "para cima"); passado para o giro do osso
                rest_q = hero.data.bones[hips].matrix_local.to_quaternion()
                pose[hips] = (rest_q.inverted() @ Quaternion(up, sign * 1.5707963 * turn) @ rest_q @ q, loc)
                poses.append(pose)
            write(name, poses, "Dodge virado %+d°" % (sign * 90))
    if "Idle" in actions and "Ride" not in report:
        # montado (01/11): o herói do Mixamo não tinha clipe de montaria e ficava em pé (flutuando) em cima do cavalo.
        # O parado (com a respiração) sentado: as coxas para a frente e abertas em volta do cavalo, os joelhos dobrados
        # (as canelas para baixo), os braços para a frente segurando as rédeas e o tronco um pouco inclinado. A altura da
        # sela é a do quadril em repouso (AVortexMonster::RideLift)
        idle_act = actions["Idle"]
        to_arm = hero.matrix_world.to_quaternion().inverted()
        ax_side, ax_front = to_arm @ Vector((1, 0, 0)), to_arm @ Vector((0, 1, 0))

        def turn(pose, bone, axis, deg):
            if bone not in pose:
                return
            q, loc = pose[bone]
            rq = hero.data.bones[bone].matrix_local.to_quaternion()
            pose[bone] = (rq.inverted() @ Quaternion(axis, math.radians(deg)) @ rq @ q, loc)

        poses = []
        for f in range(length(idle_act) + 1):
            pose = dict(sample(idle_act, f))
            for side, sgn in (("Left", 1), ("Right", -1)):
                turn(pose, P + side + "UpLeg", ax_front, -sgn * RIDE["abre"])
                turn(pose, P + side + "UpLeg", ax_side, -RIDE["coxa"])
                turn(pose, P + side + "Leg", ax_side, RIDE["joelho"])
                turn(pose, P + side + "Arm", ax_side, -RIDE["braco"])
                turn(pose, P + side + "ForeArm", ax_side, -RIDE["antebraco"])
            turn(pose, P + "Spine", ax_side, -RIDE["tronco"])
            poses.append(pose)
        write("Ride", poses, "Idle sentado na sela")
    if "Idle" in actions and "Run" in actions:
        idle = sample(actions["Idle"], 0)
        run = actions["Run"]
        n_run = length(run)
        # arrancar: do parado para a corrida em 0,35 s, a corrida acelerando
        poses = []
        count = int(.35 * fps) + 1
        for i in range(count):
            t = i / (count - 1)
            f = n_run * .5 * t * t
            poses.append(blend(idle, sample(run, f), smooth(t)))
        write("RunStart", poses, "Idle e Run")
        # frear: a corrida desacelerando (meio passo) e o corpo assentando no parado em 0,5 s
        poses = []
        count = int(.5 * fps) + 1
        for i in range(count):
            t = i / (count - 1)
            f = n_run * .5 * (1 - (1 - t) ** 2)
            poses.append(blend(sample(run, f), idle, smooth((t - .15) / .85)))
        write("RunStop", poses, "Run e Idle")


make_derived()


def add_bow_grip():
    """O arco na mão esquerda (01/11): no Mixamo a esquerda segura o arco e a direita puxa a corda; o osso
    Bow_L (sem peso, preso a mixamorig:LeftHand) é a pegada medida no quadro do arco puxado (Arco_Draw): o arco em pé,
    o cabo na palma e a corda (o lado +Y do modelo W_arco_de_betula) virada para a mão direita."""
    action = next((a for a in bpy.data.actions if a.name == "Arco_Draw"), None)
    if not action or "Bow_L" in hero.data.bones:
        return
    sample(action, length(action))
    world = lambda n: hero.matrix_world @ hero.pose.bones[n].matrix
    left, right = world(P + "LeftHand"), world(P + "RightHand")
    # o meio da palma: o modelo do Mixamo não tem dedos; a mão continua o antebraço uns 7 cm (no tamanho do herói)
    fore = world(P + "LeftForeArm").translation
    grip = left.translation + (left.translation - fore).normalized() * .07 * (old_height / 1.4)
    aim = (left.translation - right.translation)
    aim.z = 0
    aim.normalize()
    up = Vector((0, 0, 1))
    y = -aim                                  # a corda para o lado da mão direita
    x = y.cross(up).normalized()
    up = x.cross(y).normalized()
    bow = Matrix((x, y, up)).transposed().to_4x4()
    bow.translation = grip
    hand = Matrix.LocRotScale(left.translation, left.to_quaternion(), None)
    local = hand.inverted() @ bow
    for pb in hero.pose.bones:
        pb.matrix_basis = Matrix.Identity(4)
    hero.animation_data.action = None
    bpy.context.view_layer.update()
    hand_rest = rest(hero, P + "LeftHand")
    W = Matrix.LocRotScale(hand_rest.translation, hand_rest.to_quaternion(), None) @ local
    bpy.context.view_layer.objects.active = hero
    bpy.ops.object.mode_set(mode="EDIT")
    inv = hero.matrix_world.inverted()
    turn = hero.matrix_world.to_quaternion().inverted()
    bone = hero.data.edit_bones.new("Bow_L")
    bone.head = inv @ W.translation
    bone.tail = bone.head + Vector((0, .05 / k_size, 0))
    bone.matrix = Matrix.LocRotScale(inv @ W.translation, turn @ W.to_quaternion(), None)
    bone.length = .05 / k_size
    bone.parent = hero.data.edit_bones[P + "LeftHand"]
    bone.use_deform = False
    bpy.ops.object.mode_set(mode="OBJECT")
    log("pegada do arco na mão esquerda (Bow_L)")


add_bow_grip()

os.makedirs(OUT, exist_ok=True)
for t in hero.animation_data.nla_tracks:
    t.mute = False
bpy.ops.object.select_all(action="DESELECT")
hero.select_set(True)
body.select_set(True)
bpy.context.view_layer.objects.active = hero
glb = os.path.join(OUT, "Hero_%s.glb" % RACE)
bpy.ops.export_scene.gltf(filepath=glb, use_selection=True, export_format="GLB", export_animations=True,
                          export_animation_mode="NLA_TRACKS", export_skins=True, export_apply=False, export_yup=True,
                          export_force_sampling=True, export_frame_step=1, export_def_bones=False)
with open(os.path.join(OUT, "clips.json"), "w", encoding="utf-8") as f:
    json.dump(report, f, ensure_ascii=False, indent=1)
log("exportado", glb, len(report), "clipes")
