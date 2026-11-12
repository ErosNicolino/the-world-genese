"""Roupas do humano de blocos (01/11): as 6 da ficha "modelando ainda/Ficha de Trajes RPG em Pixel Art.png" (3 do corpo
masculino, 3 do feminino), feitas de cascas de bloco por cima das partes do corpo e pintadas em pixel.

Cada peça é uma caixa em volta de um pedaço do corpo (o tronco, a cueca, o braço, o antebraço, a mão, a coxa, a canela
ou o pé), no eixo do osso dele, com uma folga de espessura; um trecho do comprimento (de/até, 0 = a ponta de cima do
osso) diz até onde vai a manga, a barra ou o cano da bota. Os enfeites com volume (a ombreira, o capuz, as bolsas, as
abas do manto) são caixas a mais. A pintura é por texel (a mesma grade dos cabelos: meio pixel da cabeça): a cor de
base com 4 tons e ruído, e os desenhos de cada roupa (cintos, fivelas, barras, cordões, alças, o acolchoado).
Cada vértice pega os pesos do vértice do corpo mais perto (segue os membros rígidos e a coluna).

Sai Data/Heroes/humano_Mixamo/Roupas/Roupa_<id>.glb (a malha no esqueleto do Hero_humano) e T_Roupa_<id>.png (a
textura); Tools/ImportBlockOutfits.py leva para a Unreal.

07/11: a roupa sai em 4 materiais, um por região do corpo (M_Roupa_cabeca, _tronco, _pernas, _pes): a armadura vestida
esconde a região que ela cobre (AVortexCharacter, ShowMaterialSection) e não fica por cima, atravessando.
As peças de armadura (Data/Rpg/armors.json) saem deste mesmo gerador: o id "A_<conjunto>_<peça>" monta a peça pelo
Tools/Blender/armor_designs.py e sai em Data/Heroes/humano_Mixamo/Armaduras/A_<...>.glb, T_A_<...>.png e
T_A_<...>_mask.png (vermelho = brilha; verde = a franja, que pega a cor do cabelo do herói); Tools/ImportBlockArmor.py
leva para a Unreal. Cores com "*" na frente brilham ("*lava"); a cor "cabelo" é a franja.

No Blender aberto (MCP):  OUTFIT = "aventureiro"; exec(open(r".../MakeBlockOutfit.py", encoding="utf-8").read())
Sem janela, para o jogo:  blender -b --factory-startup -P Tools/Blender/MakeBlockOutfit.py -- jogo <id>
"""
import os
import random
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.kdtree import KDTree

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
GAME = "jogo" in ARGS
OUTFIT = next((a for a in ARGS if a != "jogo"), None) or globals().get("OUTFIT", "aventureiro")
# 10/11 (o usuário: "as texturas das imagens pode ignorar, crie você as roupas como as 3 primeiras"): os trajes Elfo_* e
# Traje_* agora são desenhados aqui (OUTFITS), como o aventureiro; a pintura amostrada da folha (piece_designs.py) ficou sem uso
SET = False
PIECE = OUTFIT.startswith("P_") or SET   # peça separada (piece_designs.py): sai como as armaduras, num material só
ARMOR = OUTFIT.startswith("A_") or PIECE
HATLIKE = False   # 09/11: chapéus e acessórios: relevo, sombra de encaixe e material como as armaduras
ELF_DETAIL = SET or OUTFIT.startswith("P_elfo_")   # o dobro de pixels, para a pintura das folhas élficas aparecer
OUT = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "Roupas" if SET else "Pecas" if PIECE else "Armaduras" if ARMOR else "Roupas")
P = "mixamorig:"
random.seed(sum(map(ord, OUTFIT)))


def log(*parts):
    print("ROUPA", OUTFIT, " ".join(str(p) for p in parts))


# ---- as cores (do escuro ao claro, 4 tons cada), tiradas da ficha
C = {
    "creme": [(196, 176, 140), (218, 199, 163), (232, 216, 184), (244, 232, 206)],
    "couro": [(70, 40, 22), (92, 55, 30), (112, 70, 40), (134, 88, 52)],
    "couro_esc": [(45, 30, 22), (60, 40, 28), (76, 52, 36), (92, 64, 44)],
    "marrom_calca": [(52, 37, 30), (66, 47, 37), (80, 58, 45), (94, 70, 54)],
    "metal": [(98, 100, 106), (132, 135, 140), (168, 170, 175), (204, 206, 210)],
    "ouro": [(150, 104, 36), (190, 138, 48), (218, 168, 64), (240, 200, 102)],
    "vermelho": [(98, 26, 28), (124, 36, 36), (148, 48, 46), (170, 66, 60)],
    "cinza": [(78, 80, 86), (98, 100, 106), (118, 120, 126), (138, 140, 146)],
    "azul": [(22, 30, 62), (30, 42, 82), (40, 56, 104), (54, 72, 126)],
    "azul_esc": [(24, 26, 36), (32, 35, 48), (42, 46, 60), (54, 58, 74)],
    "verde": [(42, 60, 32), (56, 78, 42), (70, 96, 52), (86, 114, 64)],
    "branco": [(196, 196, 192), (218, 218, 214), (234, 234, 230), (246, 246, 242)],
    "vinho": [(84, 22, 32), (106, 30, 42), (128, 40, 52), (148, 54, 64)],
    "pele": [(196, 132, 96), (214, 150, 112), (226, 166, 126), (236, 182, 142)],
    # 02/11: as dos NPCs
    "preto": [(18, 17, 23), (24, 23, 30), (31, 30, 39), (44, 43, 55)],
    "carmim": [(104, 10, 22), (138, 18, 32), (168, 30, 44), (196, 52, 62)],
    "jeans": [(54, 78, 110), (74, 100, 136), (98, 126, 160), (128, 154, 186)],
    "roxo": [(48, 26, 76), (66, 36, 102), (86, 50, 128), (110, 70, 154)],
    "verde_folha": [(52, 86, 40), (68, 108, 50), (88, 130, 62), (112, 154, 80)],
    "terra": [(120, 84, 50), (146, 106, 66), (170, 128, 84), (194, 152, 104)],
    "areia": [(168, 140, 96), (190, 162, 116), (210, 184, 138), (228, 206, 162)],
    "aco_azul": [(70, 84, 104), (98, 114, 136), (128, 144, 166), (168, 182, 200)],
    "azul_real": [(24, 46, 104), (34, 62, 136), (48, 82, 166), (70, 108, 194)],
    "prata": [(150, 156, 166), (184, 190, 198), (214, 218, 224), (240, 242, 246)],
    "rosa": [(150, 70, 90), (180, 92, 112), (206, 118, 136), (228, 150, 164)],
    "mostarda": [(150, 112, 30), (180, 138, 44), (206, 164, 62), (228, 190, 90)],
    # 02/11: a Lexy (o suéter de oncinha rosa)
    "rosa_bebe": [(204, 128, 142), (222, 148, 160), (236, 166, 176), (246, 186, 194)],
    "onca": [(44, 24, 26), (58, 32, 34), (74, 42, 44), (92, 54, 56)],
    "magenta": [(170, 20, 84), (204, 30, 104), (230, 52, 126), (246, 92, 156)],
    # 10/11: a Bunny (o conjunto verde-sálvia de flores)
    "salvia": [(112, 138, 92), (136, 162, 112), (158, 184, 132), (182, 204, 156)],
    "rosa_po": [(178, 120, 112), (204, 146, 136), (224, 170, 158), (240, 196, 184)],
    # 10/11: os trajes desenhados (o mago da lua e o caçador das sombras)
    "azul_po": [(70, 86, 122), (92, 110, 148), (118, 136, 174), (150, 166, 200)],
    "violeta": [(66, 44, 84), (88, 62, 108), (112, 84, 134), (140, 112, 160)],
    # 07/11: as armaduras (das pranchas)
    "bronze": [(120, 62, 26), (160, 88, 38), (196, 118, 54), (226, 152, 82)],
    "gelo": [(110, 160, 206), (152, 198, 234), (192, 226, 248), (228, 246, 255)],
    "coral": [(150, 60, 48), (196, 90, 70), (226, 122, 98), (244, 160, 134)],
    "teal": [(18, 84, 84), (28, 120, 114), (44, 156, 146), (86, 196, 182)],
    "violeta_esc": [(26, 18, 38), (40, 28, 58), (56, 40, 80), (76, 56, 104)],
    "runa": [(150, 60, 230), (176, 92, 250), (206, 142, 255), (232, 198, 255)],
    "rubro": [(70, 16, 16), (104, 24, 22), (138, 36, 30), (170, 56, 44)],
    "brasa": [(220, 70, 20), (250, 120, 30), (255, 170, 60), (255, 220, 130)],
    "quitina": [(54, 68, 38), (76, 94, 50), (100, 120, 64), (128, 146, 84)],
    "veneno": [(60, 150, 30), (90, 200, 40), (130, 235, 60), (190, 255, 120)],
    "obsidiana": [(16, 14, 18), (26, 24, 30), (38, 34, 42), (54, 48, 58)],
    "lava": [(210, 70, 12), (245, 118, 22), (255, 166, 44), (255, 216, 120)],
    "casca": [(46, 32, 22), (64, 46, 30), (84, 60, 40), (104, 78, 52)],
    "esmeralda": [(18, 130, 56), (30, 176, 78), (60, 220, 110), (150, 255, 180)],
    "ciano": [(40, 196, 206), (80, 226, 232), (140, 248, 248), (200, 255, 255)],
    "azul_noite": [(20, 24, 50), (28, 34, 70), (38, 46, 92), (52, 62, 118)],
    "madeira_clara": [(148, 128, 78), (178, 158, 98), (204, 186, 124), (228, 212, 158)],
    "pelo": [(104, 104, 108), (146, 146, 150), (184, 184, 188), (220, 220, 224)],
    "ferro": [(70, 74, 82), (104, 108, 116), (140, 144, 152), (178, 182, 190)],
    "dourado": [(160, 108, 30), (206, 150, 40), (236, 190, 64), (252, 226, 120)],
    "branco_puro": [(206, 210, 222), (226, 230, 240), (242, 244, 250), (252, 253, 255)],
    "lilas": [(120, 96, 160), (150, 126, 190), (184, 164, 218), (214, 200, 240)],
    "remendo": [(120, 40, 40), (60, 110, 60), (60, 70, 130), (170, 150, 60)],
    # 09/11: as orelhas dos elfos (a pele de Tools/MakeElfSkins.py)
    "pele_clara": [(214, 190, 170), (232, 212, 194), (246, 230, 214), (255, 240, 226)],
    "pele_escura": [(54, 47, 78), (72, 63, 100), (92, 82, 124), (112, 100, 146)],
    # 10/11: o top do corpo feminino (T_humano_Pixel_F), para o busto
    "top_f": [(190, 162, 139), (212, 188, 165), (232, 212, 190), (244, 228, 208)],
    # a franja dos capuzes: só os tons (o material multiplica pela cor do cabelo do herói)
    "cabelo": [(100, 100, 100), (116, 116, 116), (128, 128, 128), (142, 142, 142)],
}

# os NPCs (02/11): a roupa de cada um, no corpo dele
NPC_FEMALE = {"lary", "julia", "juh", "ilsa", "nicolly", "rhea", "amal", "lyra", "rosa", "marta", "lexy", "bunny"}
NPC_MALE = {"bram", "ducci", "sahil", "tavian", "garrick"}

# ---- o corpo: o herói do jogo (a malha e o esqueleto do Hero_humano), em repouso
FEMALE = OUTFIT in ("arqueira", "sacerdotisa", "aventureira") or OUTFIT in NPC_FEMALE or OUTFIT.endswith("_f")
WAIST = FEMALE and OUTFIT not in NPC_FEMALE   # 10/11: as roupas das heroínas seguem a cintura do corpo feminino (female_waist.py)   # as armaduras: o corpo comum
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o, do_unlink=True)
src = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "Feminino" if FEMALE else "", "Hero_humano.glb")
bpy.ops.import_scene.gltf(filepath=os.path.normpath(src))
for o in bpy.data.objects:
    if o.animation_data:
        o.animation_data.action = None
        for t in list(o.animation_data.nla_tracks):
            o.animation_data.nla_tracks.remove(t)
for a in list(bpy.data.actions):
    bpy.data.actions.remove(a)
rig = next(o for o in bpy.data.objects if o.type == "ARMATURE")
for pb in rig.pose.bones:
    pb.matrix_basis.identity()
bpy.context.view_layer.update()
body = next(o for o in bpy.data.objects if o.type == "MESH" and o.find_armature() == rig)
BW = body.matrix_world
GROUPS = {g.index: g.name for g in body.vertex_groups}
VERTS = []                                                       # (posição no mundo, {grupo: peso})
for v in body.data.vertices:
    VERTS.append((BW @ v.co, {GROUPS[g.group]: g.weight for g in v.groups if g.weight > .01}))


def group_points(names, strong=.5):
    names = {P + n for n in names}
    return [p for p, w in VERTS if any(w.get(n, 0) > strong for n in names)]


head = group_points(["Head"], .99)
TEX = (max(p.x for p in head) - min(p.x for p in head)) / 16      # um texel = meio pixel da cabeça
log("texel %.4f m" % TEX)


def bone_frame(name):
    """A matriz do osso em repouso no mundo, sem escala: o eixo Y vai da cabeça para a ponta do osso."""
    b = rig.data.bones[P + name]
    m = rig.matrix_world @ b.matrix_local
    loc, rot, _ = m.decompose()
    return Matrix.LocRotScale(loc, rot, None)


class Box:
    """Uma caixa num referencial (M: local -> mundo), de lo a hi no local; 'along' é o eixo do comprimento (com o
    sentido: 0 = começo/em cima)."""

    def __init__(self, M, lo, hi, axis, flip):
        self.M, self.lo, self.hi, self.axis, self.flip = M, Vector(lo), Vector(hi), axis, flip

    def cut(self, a, b):
        lo, hi = self.lo.copy(), self.hi.copy()
        L0, L1 = self.lo[self.axis], self.hi[self.axis]
        if self.flip:
            lo[self.axis], hi[self.axis] = L1 - (L1 - L0) * b, L1 - (L1 - L0) * a
        else:
            lo[self.axis], hi[self.axis] = L0 + (L1 - L0) * a, L0 + (L1 - L0) * b
        return Box(self.M, lo, hi, self.axis, self.flip)


def part_box(names, frame=None, strong=.5, exclude_front=None):
    """A caixa das partes do corpo dos ossos dados: no referencial do osso frame (o comprimento no eixo Y dele) ou,
    sem frame, nos eixos do mundo (o comprimento é o Z, de cima para baixo)."""
    pts = group_points(names, strong)
    if frame:
        M = bone_frame(frame)
        inv = M.inverted()
        loc = [inv @ p for p in pts]
        return Box(M, [min(p[a] for p in loc) for a in range(3)], [max(p[a] for p in loc) for a in range(3)], 1, False)
    if exclude_front is not None:
        ys = sorted(p.y for p in group_points(exclude_front, strong))
    lo = Vector([min(p[a] for p in pts) for a in range(3)])
    hi = Vector([max(p[a] for p in pts) for a in range(3)])
    if exclude_front is not None:
        lo.y = ys[0]
    return Box(Matrix.Identity(4), lo, hi, 2, True)


TORSO = part_box(["Spine", "Spine1", "Spine2"], exclude_front=["Spine"])
HIPS = part_box(["Hips"])
PARTS = {"torso": TORSO, "quadril": HIPS}
PART_BONE = {}        # as partes dos membros e o osso delas (as peças nelas são rígidas, como o corpo)
for side in ("Left", "Right"):
    s = "E" if side == "Left" else "D"
    for name, bone in (("braco", "Arm"), ("antebraco", "ForeArm"), ("mao", "Hand"), ("coxa", "UpLeg"), ("canela", "Leg"),
                       ("pe", "Foot")):
        PARTS["%s_%s" % (name, s)] = part_box([side + bone], side + bone, .99)
        PART_BONE["%s_%s" % (name, s)] = side + bone
# o pé: no referencial do Foot o comprimento é do tornozelo para a ponta; para cascas de bota vale o do mundo (alto)
for s, side in (("E", "Left"), ("D", "Right")):
    PARTS["pe_" + s] = part_box([side + "Foot"])
# a cabeça e o pescoço (02/11: os óculos, o chapéu), rígidos no osso da cabeça
PARTS["cabeca"] = part_box(["Head"], strong=.99)
PART_BONE["cabeca"] = "Head"
# o corpo de blocos não tem pescoço (a cabeça, 16 x 15 texels, fica em cima do tronco): é a última linha da cabeça
# (pintar ali parecia o queixo com a textura da roupa: a choker saiu)
PARTS["pescoco"] = PARTS["cabeca"].cut(.93, 1.0)
PART_BONE["pescoco"] = "Head"
BUST = None
if FEMALE:
    pts = [p for p in group_points(["Spine2"]) if p.y < TORSO.lo.y - TEX * .3]
    if pts:
        # um pouco mais alto e mais baixo que os vértices medidos (02/11: aparecia uma linha de pele no topo do busto)
        BUST = Box(Matrix.Identity(4), [min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts) - .3 * TEX],
                   [max(p.x for p in pts), TORSO.lo.y, max(p.z for p in pts) + .8 * TEX], 2, True)
        PARTS["busto"] = BUST

# ---- as peças
JOINT_OVERLAP = 2.5   # texels que cada peça do membro passa da junta
PIECES = []           # (caixa, folga em texels, cores, enfeites, nome)
PIECE_PART = []       # a parte do corpo de cada peça


def shell(parts, a=0.0, b=1.0, off=.45, color="creme", decos=(), grow=(0, 0, 0), sides=("E", "D"), name=""):
    """Uma casca em volta das partes (com _E/_D: as duas); a e b: o trecho do comprimento; off: a folga (texels);
    grow: a mais em cada eixo local (texels: largura, profundidade, comprimento)."""
    for p in ([parts] if isinstance(parts, str) else parts):
        keys = [p + "_" + s for s in sides] if p + "_E" in PARTS else [p]
        for k in keys:
            if k in PARTS:
                # 08/11 (o usuário: "as roupas somem no joelho e no cotovelo"): as peças de cima e de baixo do membro passam um pouco da
                # junta: terminando reto nela, dobrar abria uma cunha. A de baixo fica um fio mais larga, para as duas não piscarem
                box = PARTS[k]
                reach = JOINT_OVERLAP * TEX / max(1e-6, box.hi[box.axis] - box.lo[box.axis])
                a2, b2, off2 = a, b, off
                if p in ("braco", "coxa") and b >= 1.0: b2 = 1.0 + reach
                if p in ("antebraco", "canela") and a <= 0.0: a2, off2 = -reach, off + .08
                PIECES.append((box.cut(a2, b2), off2, color, list(decos), grow, name or k))
                PIECE_PART.append(k)


def block(center_part, offset, size, color="couro", decos=(), name="bloco", frame_from=None):
    """Uma caixa solta (bolsa, ombreira, capuz), no referencial da parte dada: offset do centro da parte e tamanho, em
    texels (x, y, z locais)."""
    box = PARTS[center_part]
    mid = (box.lo + box.hi) / 2
    c = mid + Vector(offset) * TEX
    h = Vector(size) * TEX / 2
    PIECES.append((Box(box.M, c - h, c + h, box.axis, box.flip), 0, color, list(decos), (0, 0, 0), name))
    PIECE_PART.append(center_part)


# enfeites: cada um é f(face, u, v, piece) -> (cor, tom) ou None. face: frente/costas/esq/dir/cima/baixo; u de 0 a 1
# na largura da face vista de fora; v de 0 a 1 no comprimento da peça (0 = em cima)


def band(v0, v1, color, tone=None, faces=None):
    def f(face, u, v, p):
        if (faces is None or face in faces) and face not in ("cima", "baixo") and v0 <= v <= v1:
            return color, tone
    return f


def vline(u0, u1, color, v0=0.0, v1=1.0, tone=None, faces=("frente",)):
    def f(face, u, v, p):
        if face in faces and u0 <= u <= u1 and v0 <= v <= v1:
            return color, tone
    return f


def rect(u0, u1, v0, v1, color, tone=None, faces=("frente",), hollow=None):
    """Retângulo (hollow: a cor do miolo, a fivela)."""
    def f(face, u, v, p):
        if face in faces and u0 <= u <= u1 and v0 <= v <= v1:
            if hollow:
                du, dv = (u1 - u0) * .3, (v1 - v0) * .3
                if u0 + du < u < u1 - du and v0 + dv < v < v1 - dv:
                    return hollow
            return color, tone
    return f


def vneck(depth, width, color="pele"):
    def f(face, u, v, p):
        if face == "frente" and abs(u - .5) < width * (1 - v / depth) and v < depth:
            return color, 2
    return f


def open_front(half, v_neck, inner, edge):
    """A frente aberta (colete, manto): o meio mostra a peça de baixo (inner), mais largo em cima (o decote), com a
    borda (edge) de 1 texel."""
    def f(face, u, v, p):
        if face != "frente":
            return None
        w = half + (.12 * (1 - v / v_neck) if v < v_neck else 0)
        d = abs(u - .5)
        if d < w - .04:
            return inner, 2
        if d < w:
            return edge, 1
    return f


def cross(color, width=.09, faces=("frente", "costas")):
    """As alças em X no peito (e nas costas)."""
    def f(face, u, v, p):
        if face in faces and v < .85 and (abs(u - v * 1.1) < width or abs((1 - u) - v * 1.1) < width):
            return color, None
    return f


def quilt(color):
    """O acolchoado: losangos mais escuros."""
    def f(face, u, v, p):
        if face in ("cima", "baixo"):
            return None
        x, y = p["texel"]
        if (x + y) % 5 == 0 or (x - y) % 5 == 0:
            return color, 1
    return f


def lace(color, u=.5, v0=.15, v1=.85):
    def f(face, uu, v, p):
        x, y = p["texel"]
        if face == "frente" and v0 <= v <= v1 and abs(uu - u) < .12 and (y % 2 == 0) and abs(uu - u) > .02:
            return color, 3
    return f


def hem(color, rows=1, tone=None):
    """A barra de baixo da peça (as últimas linhas de texel)."""
    def f(face, u, v, p):
        if face in ("cima", "baixo"):
            return None
        if p["texel"][1] >= p["rows"] - rows:
            return color, tone
    return f


def jag(depth=2):
    """A barra recortada (pontas): some texels de baixo ficam vazios (a cor da peça mais escura)."""
    def f(face, u, v, p):
        x, y = p["texel"]
        if face not in ("cima", "baixo") and y >= p["rows"] - depth and (x // 2) % 2 == 0:
            return None, 0
    return f


def sparkle(color="branco", every=5, faces=None):
    """Paetê: texels soltos claros (o brilho), espalhados num padrão sem repetir à vista."""
    def f(face, u, v, p):
        if faces is not None and face not in faces:
            return None
        x, y = p["texel"]
        h = random.Random(x * 7919 + y * 104729 + len(face) * 31 + p["rows"] * 13).random()
        if h < 1 / every:
            return (color, 2) if h < .3 / every else (None, 3)
    return f


def stripes(color, period=4, width=1, tone=None, faces=None, vertical=False):
    def f(face, u, v, p):
        if face in ("cima", "baixo") or (faces is not None and face not in faces):
            return None
        x, y = p["texel"]
        if ((x if vertical else y) % period) < width:
            return color, tone
    return f


def dots(color, period=5, tone=None, faces=None):
    """Bolinhas/botões: um texel a cada period."""
    def f(face, u, v, p):
        if face in ("cima", "baixo") or (faces is not None and face not in faces):
            return None
        x, y = p["texel"]
        if x % period == period // 2 and y % period == period // 2:
            return color, tone
    return f


def leopard(ring="onca", core="magenta", period=9, faces=None):
    """Oncinha: rosetas espalhadas (uma por quadra de period texels, num lugar sorteado): o miolo de 3 texels na cor
    core e a borda escura em volta, com falhas (a roseta da onça não fecha)."""
    def f(face, u, v, p):
        if faces is not None and face not in faces:
            return None
        x, y = p["texel"]
        cx, cy = x // period, y // period
        h = random.Random(cx * 7919 + cy * 104729 + len(face) * 31 + p["rows"] * 13).random()
        ox = cx * period + 3 + int(h * (period - 6))
        oy = cy * period + 3 + int((h * 13.7) % 1 * (period - 6))
        d = abs(x - ox) + abs(y - oy)
        if d <= 1:
            return core, 1 if d == 0 else 2
        if 2 <= d <= 3 and (x * 3 + y * 5 + cx) % 6:
            return ring, 1 if d == 2 else 0
    return f


def pendant(u, v, size, color, tone=3, shape="diamond"):
    """Um pingente na frente (a estrela do colar): um losango ou uma estrela de texels em volta de (u, v)."""
    def f(face, uu, vv, p):
        if face != "frente":
            return None
        du, dv = abs(uu - u) / size, abs(vv - v) / size
        if shape == "star":
            # a estrela de 5 pontas: o miolo, a ponta de cima e as 4 de lado/baixo (em diagonal)
            sx, sy = (uu - u) / size, (vv - v) / size
            if (abs(sx) < .22 and -1 < sy < .3) or (abs(sy + .2) < .18 and abs(sx) < 1) or                     (sy > 0 and abs(abs(sx) - sy * .75) < .2 and sy < 1):
                return color, tone
        elif du + dv < 1:
            return color, tone
    return f


def chain(u0, u1, v_mid, v_top, color, tone=3):
    """A corrente do colar: um V do pescoço até o pingente."""
    def f(face, u, v, p):
        if face != "frente" or not (u0 <= u <= u1):
            return None
        mid = (u0 + u1) / 2
        want = v_top + (v_mid - v_top) * (1 - abs(u - mid) / ((u1 - u0) / 2))
        if abs(v - want) < .035:
            return color, tone
    return f


def skirt(top, length, flare, color, decos=(), tiers=2, off=.6, name="saia"):
    """Uma saia rodada em volta das duas pernas (a partir do quadril): top = de onde sai (0 = a cintura, 1 = a base do
    quadril), length = o comprimento abaixo do quadril (texels), flare = quanto abre a mais embaixo (texels, por lado).
    Feita de faixas (tiers) cada vez mais largas, como uma saia de pixel; pesa no quadril (a perna passa por dentro)."""
    # 02/11: abaixo do quadril a saia é feita de duas metades, cada uma presa à sua coxa (como nos jogos de blocos): no
    # passo ela abre com as pernas. Inteira, presa ao quadril, a perna atravessava; presa às duas coxas, rasgava no meio
    h = HIPS
    z_top = h.hi.z - (h.hi.z - h.lo.z) * top
    z_bot = h.lo.z - length * TEX
    mid = (h.lo.x + h.hi.x) / 2
    left_x = (rig.matrix_world @ rig.data.bones[P + "LeftUpLeg"].head_local).x
    for k in range(tiers):
        za = z_top - (z_top - z_bot) * k / tiers
        zb = z_top - (z_top - z_bot) * (k + 1) / tiers
        g = flare * (k + 1) / tiers
        lo = Vector((h.lo.x - g * TEX, h.lo.y - g * TEX * .8, zb))
        hi = Vector((h.hi.x + g * TEX, h.hi.y + g * TEX * .8, za))
        decs = list(decos) + ([hem(color, 1, 0)] if k == tiers - 1 else [])
        if za <= h.lo.z + TEX * .5:
            for half_lo, half_hi in ((lo.x, mid), (mid, hi.x)):
                part = "coxa_E" if ((half_lo + half_hi) / 2 > mid) == (left_x > mid) else "coxa_D"
                PIECES.append((Box(Matrix.Identity(4), Vector((half_lo, lo.y, lo.z)), Vector((half_hi, hi.y, hi.z)), 2, True),
                               off, color, decs, (0, 0, 0), "%s_%d" % (name, k)))
                PIECE_PART.append(part)
        else:
            PIECES.append((Box(Matrix.Identity(4), lo, hi, 2, True), off, color, decs, (0, 0, 0), "%s_%d" % (name, k)))
            PIECE_PART.append("quadril")


BUSTY = False   # 10/11: peça do corpo feminino: as camisas e coletes ganham o volume do busto por cima


def bust(color, extra=0.0, decos=()):
    """O busto arredondado (10/11, o usuário: "o peito é um retângulo"): dois montes de 4 camadas, cada uma menor e mais à frente, presos
    ao tronco. extra (texels) faz a casca das roupas por cima do corpo. Do tamanho do busto do corpo feminino antigo (que as roupas dele cobrem)."""
    t = TORSO
    cx = (t.lo.x + t.hi.x) / 2
    for s_ in (-1, 1):
        # 6 camadas, cada uma menor e mais à frente (um domo), e a pintura com luz no centro e sombra na borda de cada camada
        layers = ((5.8, 7.0, .35), (5.2, 6.4, .7), (4.6, 5.6, 1.0), (3.8, 4.8, 1.3), (3.0, 3.8, 1.55), (2.0, 2.6, 1.72))
        for k_, (w, h, d) in enumerate(layers):
            x0 = cx + s_ * 3.0 * TEX
            lo = Vector((x0 - (w / 2 + extra) * TEX, t.lo.y - (d + extra) * TEX, t.hi.z - (5.3 + h / 2 + extra) * TEX))
            hi = Vector((x0 + (w / 2 + extra) * TEX, t.lo.y + .3 * TEX, t.hi.z - (5.3 - h / 2 - extra) * TEX))

            def round_light(face, u, v, p_, k_=k_):
                if face != "frente":
                    return None
                r = ((u - .5) ** 2 + (v - .42) ** 2) ** .5 * 2
                return color, max(0, min(3, int(3.4 - r * 2.2 + k_ * .25)))
            under = [hem(color, 1, 0)] if k_ == 0 else []
            PIECES.append((Box(Matrix.Identity(4), lo, hi, 2, True), 0, color, list(decos) + under + [round_light], (0, 0, 0), "busto"))
            PIECE_PART.append("torso")


CLOTH_TUBE = {}       # 09/11: a saia de tecido real (Chaos Cloth): um tubo só, sem emenda, numa malha à parte (<id>_tecido)


def cloth_skirt(length, flare, color):
    """O vestido maleável: uma superfície contínua da cintura à barra. A Unreal simula o pano (o comando
    VortexMakeCloth). length: texels abaixo do quadril; flare: quanto abre embaixo (texels)."""
    h = HIPS
    CLOTH_TUBE.update(z_top=h.hi.z - (h.hi.z - h.lo.z) * .55, z_bot=h.lo.z - length * TEX, cx=(h.lo.x + h.hi.x) / 2, cy=(h.lo.y + h.hi.y) / 2,
                      a=(h.hi.x - h.lo.x) / 2 + 2.2 * TEX, b=(h.hi.y - h.lo.y) / 2 + 2.2 * TEX, flare=flare * TEX, color=color)


def cloth_strip(side, bone, z_top, z_bot, width, color, y, dx=0.0):
    """Um pano solto retangular, em tecido real, preso no osso dado: z em metros no mundo, width e dx em texels."""
    t = TORSO
    CLOTH_TUBE.update(sheet=side, z_top=z_top, z_bot=z_bot, cx=(t.lo.x + t.hi.x) / 2 + dx * TEX, cy=(t.lo.y + t.hi.y) / 2, y=y,
                      a=width * TEX / 2, b=TEX, flare=(.8 if width >= 8 else 0) * TEX, color=color, bone=bone, small=width < 8)


def cloth_sheet(side, z_top, z_bot, width, color, gap=2.2):
    """Uma capa nas costas ou um tabardo na frente: de z_top a z_bot texels abaixo do alto do tronco."""
    t, h = TORSO, HIPS
    y = min(t.lo.y, h.lo.y) - gap * TEX if side == "frente" else max(t.hi.y, h.hi.y) + gap * TEX
    cloth_strip(side, "Spine2", t.hi.z - z_top * TEX, t.hi.z - z_bot * TEX, width, color, y)


def tube_point(s_, t):
    """s_: a volta (0 = o meio das costas), t: 0 na cintura e 1 na barra. No pano solto, s_ vai de um lado ao outro."""
    import math
    T = CLOTH_TUBE
    if T.get("sheet"):
        return Vector((T["cx"] + (2 * s_ - 1) * (T["a"] + T["flare"] * t), T["y"], T["z_top"] + (T["z_bot"] - T["z_top"]) * t))
    th = math.pi / 2 + 2 * math.pi * s_
    c, n = math.cos(th), math.sin(th)
    return Vector((T["cx"] + (T["a"] + T["flare"] * t) * math.copysign(abs(c) ** .55, c),
                   T["cy"] + (T["b"] + T["flare"] * t) * math.copysign(abs(n) ** .55, n), T["z_top"] + (T["z_bot"] - T["z_top"]) * t))


def glasses(color="preto", lens=None):
    """Óculos de armação (02/11, a Julia): as armações em volta dos olhos do rosto de blocos (as colunas 1 a 6 e 9 a 14,
    as linhas 7 a 12 da grade 16 x 16 da frente da cabeça), a ponte e as hastes até as orelhas. Feitos de barrinhas: o
    olho fica à vista."""
    hb = PARTS["cabeca"]
    front = hb.lo.y
    top = hb.hi.z
    x0 = (hb.lo.x + hb.hi.x) / 2 - 8 * TEX                     # a coluna 0 da grade (vista de frente, da esquerda)
    t = .55 * TEX
    def bar(c0, c1, r0, r1, depth=.6):
        lo = Vector((x0 + c0 * TEX, front - depth * TEX - .15 * TEX, top - r1 * TEX))
        hi = Vector((x0 + c1 * TEX, front - .15 * TEX, top - r0 * TEX))
        PIECES.append((Box(Matrix.Identity(4), lo, hi, 2, True), 0, color, [], (0, 0, 0), "oculos"))
        PIECE_PART.append("cabeca")
    for c0, c1 in ((1, 6.6), (9.4, 15)):
        bar(c0, c1, 7.2, 7.2 + t / TEX)                          # em cima
        bar(c0, c1, 12.4 - t / TEX, 12.4)                        # embaixo
        bar(c0, c0 + t / TEX, 7.2, 12.4)                         # dos lados
        bar(c1 - t / TEX, c1, 7.2, 12.4)
    bar(6.6, 9.4, 8.2, 8.2 + t / TEX)                            # a ponte
    # as hastes: dos lados da cabeça, da frente até a orelha
    for side in (0, 1):
        x = hb.lo.x - .35 * TEX if side == 0 else hb.hi.x
        lo = Vector((x, front, top - 8.4 * TEX))
        hi = Vector((x + .35 * TEX, front + 7 * TEX, top - 7.8 * TEX))
        PIECES.append((Box(Matrix.Identity(4), lo, hi, 2, True), 0, color, [], (0, 0, 0), "haste"))
        PIECE_PART.append("cabeca")


def panel(z_top, z_bot, width, color, decos=(), side="frente", gap=1.3, thick=.7, name="painel"):
    """Um painel solto na frente (ou nas costas) do corpo: o avental, o tabardo. z_top/z_bot em texels a partir do alto
    do tronco (positivo para baixo; passa do quadril e desce pelas pernas); width em texels; gap = a distância da pele
    (por cima das outras peças). Pesa no quadril/coluna, como as peças do tronco."""
    if ARMOR and not PIECE and name.startswith("tabardo") and not CLOTH_TUBE:   # 09/11: o tabardo das armaduras é pano de verdade
        cloth_sheet(side, z_top, z_bot, width, color, gap)
        return
    t, h = TORSO, HIPS
    top = t.hi.z
    cx = (t.lo.x + t.hi.x) / 2
    if side == "frente":
        y1 = min(t.lo.y, h.lo.y, BUST.lo.y if BUST else t.lo.y) - gap * TEX
        y0 = y1 - thick * TEX
    else:
        y0 = max(t.hi.y, h.hi.y) + gap * TEX
        y1 = y0 + thick * TEX
    lo = Vector((cx - width * TEX / 2, y0, top - z_bot * TEX))
    hi = Vector((cx + width * TEX / 2, y1, top - z_top * TEX))
    PIECES.append((Box(Matrix.Identity(4), lo, hi, 2, True), 0, color, list(decos), (0, 0, 0), name))
    PIECE_PART.append("torso" if z_bot * TEX < t.hi.z - t.lo.z else "quadril")


def back_block(z_top, z_bot, width, depth, color, decos=(), dx=0.0, name="costas"):
    """Uma caixa nas costas (a aljava, a mochila): de z_top a z_bot texels abaixo do alto do tronco."""
    t = TORSO
    cx = (t.lo.x + t.hi.x) / 2 + dx * TEX
    y0 = t.hi.y + 1.1 * TEX
    lo = Vector((cx - width * TEX / 2, y0, t.hi.z - z_bot * TEX))
    hi = Vector((cx + width * TEX / 2, y0 + depth * TEX, t.hi.z - z_top * TEX))
    PIECES.append((Box(Matrix.Identity(4), lo, hi, 2, True), 0, color, list(decos), (0, 0, 0), name))
    PIECE_PART.append("torso")


def head_wrap(rows, color, decos=(), off=1.6, name="turbante"):
    """O turbante / a faixa na cabeça: as linhas de cima da cabeça (rows texels), por fora do cabelo curto."""
    hb = PARTS["cabeca"]
    frac = rows / ((hb.hi.z - hb.lo.z) / TEX)
    PIECES.append((hb.cut(0, frac), off, color, list(decos), (0, 0, 0), name))
    PIECE_PART.append("cabeca")
    # o topo: uma tampa por cima
    lo = Vector((hb.lo.x - off * TEX, hb.lo.y - off * TEX, hb.hi.z))
    hi = Vector((hb.hi.x + off * TEX, hb.hi.y + off * TEX, hb.hi.z + (off + .8) * TEX))
    PIECES.append((Box(Matrix.Identity(4), lo, hi, 2, True), 0, color, list(decos), (0, 0, 0), name + "_topo"))
    PIECE_PART.append("cabeca")


# ---- as roupas


def aventureiro():
    # camisa creme de manga curta, colete marrom aberto, cinto com fivela e bolsa, calça marrom, botas com o cano dobrado
    shell("torso", color="creme", decos=[vneck(.32, .14)])
    shell("braco", 0, .55, color="creme", decos=[hem("creme", 1, 1)])
    shell("torso", off=1.1, color="couro", decos=[vneck(.22, .07), open_front(.21, .3, "creme", "couro_esc"),
                                                 rect(.18, .28, .45, .52, "couro_esc", faces=("frente",))], name="colete")
    shell("quadril", 0, .45, off=.9, color="couro_esc", decos=[rect(.42, .58, .05, .95, "metal", hollow=("couro_esc", 0))],
          name="cinto")
    shell("quadril", .4, 1, color="marrom_calca")
    shell(["coxa", "canela"], color="marrom_calca")
    block("quadril", (9, -1, 1.5), (4, 3.5, 4), "couro", [rect(.1, .9, 0, .3, "couro_esc", faces=("frente", "esq", "dir"))], "bolsa")
    shell("canela", .55, 1, off=.9, color="couro", decos=[band(0, .2, "couro", 3)], name="bota")
    shell("pe", off=.5, color="couro", decos=[hem("couro_esc", 1)])


def guerreiro():
    # gibão vermelho acolchoado até o quadril, alças de couro em X, ombreira de metal no ombro esquerdo, braçadeiras,
    # cinto, calça cinza, botas com a biqueira de metal
    shell("torso", color="vermelho", decos=[cross("couro"), quilt("vermelho")])
    shell("braco", 0, .6, color="vermelho", decos=[quilt("vermelho"), hem("vermelho", 1, 0)])
    shell("quadril", 0, .5, off=.9, color="vermelho", decos=[quilt("vermelho"), jag(1)], name="saia_gibao")
    shell("quadril", .1, .4, off=1.5, color="couro_esc", decos=[rect(.42, .58, .05, .95, "metal", hollow=("couro_esc", 0))],
          name="cinto")
    shell("quadril", .5, 1, color="cinza")
    shell(["coxa", "canela"], color="cinza")
    shell("antebraco", .15, 1, off=.8, color="couro", decos=[band(0, .12, "couro", 3), band(.9, 1, "couro_esc", 0)],
          name="bracadeira")
    shell("braco", 0, .42, off=2.4, grow=(.5, .5, 0), color="metal", sides=("E",),
          decos=[band(.3, .38, "metal", 0), band(.62, .7, "metal", 0), hem("metal", 1, 0)], name="ombreira")
    shell("canela", .45, 1, off=.9, color="couro_esc", decos=[band(0, .18, "couro", 2)], name="bota")
    shell("pe", off=.5, color="couro_esc", decos=[rect(0, 1, 0, 1, "metal", faces=("frente",))])


def mago():
    # manto azul até os joelhos com barra dourada, capuz caído nas costas, cinto com fivela dourada, mangas compridas com
    # o punho dourado, calça escura e botas
    gold = "ouro"
    shell("torso", color="azul", decos=[vline(.46, .54, gold), vline(.40, .46, "azul", tone=0)])
    shell("braco", color="azul")
    shell("antebraco", color="azul", grow=(1, 1, 0), decos=[band(.75, 1, gold, None)], name="manga")
    shell("quadril", color="azul", off=1.0, decos=[vline(.46, .54, gold)], name="manto")
    shell("coxa", color="azul", off=1.6, grow=(1.2, 1.2, 0), decos=[hem(gold, 1)], name="aba")
    shell("canela", 0, .25, color="azul", off=1.6, grow=(1.2, 1.2, 0), decos=[hem(gold, 1)], name="aba_baixo")
    shell("canela", .25, 1, color="azul_esc")
    shell("quadril", .05, .35, off=1.9, color="couro", decos=[rect(.42, .58, .05, .95, gold, hollow=("couro", 0))], name="cinto")
    block("torso", (0, 6.5, 5), (15, 4.5, 10), "azul", [rect(.12, .88, 0, .85, "azul_esc", 0, faces=("costas", "cima")),
                                                       band(0, 1, gold, None, faces=("esq", "dir")), hem(gold, 1)], "capuz")
    shell("torso", 0, .12, off=1.2, color="azul", grow=(1, 1, 0), decos=[band(.5, 1, gold, None)], name="gola")
    shell("pe", off=.5, color="azul_esc")


def arqueira():
    # camisa creme de manga curta bufante, corpete verde com cordão e alças, túnica verde até o quadril com a barra em
    # pontas, braçadeiras, cinto com fivela e bolsa, calça marrom, botas altas até o joelho
    shell("torso", color="creme", decos=[vneck(.25, .12)])
    shell("busto", color="verde", off=1.0, name="busto")
    shell("braco", 0, .45, color="creme", off=.9, grow=(.8, .8, 0), decos=[hem("creme", 1, 1)], name="manga")
    shell("torso", .18, 1, off=1.0, color="verde", decos=[lace("couro_esc"), vline(.15, .22, "couro"), vline(.78, .85, "couro")],
          name="corpete")
    shell("quadril", 0, .55, off=1.0, color="verde", decos=[jag(1)], name="tunica")
    shell("quadril", .05, .3, off=1.6, color="couro", decos=[rect(.42, .58, .05, .95, "ouro", hollow=("couro", 0))], name="cinto")
    shell("quadril", .55, 1, color="couro_esc")
    shell("coxa", color="couro_esc")
    block("quadril", (9, -1, .5), (4, 3.5, 4), "couro", [], "bolsa")
    shell("antebraco", .2, 1, off=.8, color="couro", decos=[band(0, .1, "couro", 3)], name="bracadeira")
    shell("canela", color="couro", off=.9, decos=[band(0, .14, "couro", 3), rect(.4, .6, .5, .62, "ouro", hollow=("couro", 0))],
          name="bota")
    shell("pe", off=.5, color="couro", decos=[hem("couro_esc", 1)])


def sacerdotisa():
    # vestido branco até o meio da coxa com faixas douradas na frente e a barra dourada, capinha nos ombros com a borda
    # dourada, cinto com o emblema dourado, meias brancas com a faixa dourada e sandálias
    gold = "ouro"
    shell("torso", color="branco", decos=[vneck(.18, .1), vline(.3, .36, gold, .3), vline(.64, .7, gold, .3)])
    shell("busto", color="branco", off=1.0, name="busto")
    shell("quadril", color="branco", off=1.0, decos=[vline(.3, .36, gold), vline(.64, .7, gold)], name="vestido")
    shell("coxa", 0, .45, color="branco", off=1.4, grow=(1, 1, 0), decos=[hem(gold, 1)], name="saia")
    shell("torso", 0, .3, off=1.5, color="branco", grow=(1, 1, 0), decos=[hem(gold, 1)], name="capa")
    shell("braco", 0, .3, off=1.6, color="branco", decos=[hem(gold, 1)], name="capa_ombro")
    shell("quadril", 0, .25, off=1.7, color="couro", decos=[rect(.4, .6, 0, 1, gold, hollow=("couro_esc", 0))], name="cinto")
    shell("canela", .1, .85, color="branco", decos=[band(0, .12, gold)], name="meia")
    shell("pe", off=.4, color="branco", decos=[band(.3, .5, "couro", None, faces=("frente", "esq", "dir"))], name="sandalia")


def aventureira():
    # blusa creme de manga curta bufante, colete vinho curto com a barra em pontas, cinto com bolsas, short/saia cinza,
    # braçadeiras e botas curtas
    shell("torso", color="creme", decos=[vneck(.25, .12)])
    shell("busto", color="vinho", off=1.0, name="busto")
    shell("braco", 0, .45, color="creme", off=.9, grow=(.8, .8, 0), decos=[hem("creme", 1, 1)], name="manga")
    shell("torso", off=1.0, color="vinho", decos=[vline(.44, .56, "creme", tone=2), lace("couro_esc", .5, .3, .7), jag(1)],
          name="colete")
    shell("quadril", 0, .3, off=1.5, color="couro", decos=[rect(.42, .58, .05, .95, "metal", hollow=("couro", 0))], name="cinto")
    shell("quadril", .2, 1, color="cinza", off=.8)
    shell("coxa", 0, .4, color="cinza", off=1.2, grow=(.6, .6, 0), decos=[hem("cinza", 1, 0)], name="short")
    block("quadril", (9, -1, -.5), (4, 3.5, 4), "couro", [], "bolsa")
    block("quadril", (-9, -1, -.5), (4, 3.5, 4), "couro", [], "bolsa2")
    shell("antebraco", .25, 1, off=.8, color="couro", decos=[band(0, .1, "couro", 3)], name="bracadeira")
    shell("canela", .6, 1, off=.9, color="couro_esc", decos=[band(0, .2, "couro_esc", 3), lace("couro", .5, .2, .9)], name="bota")
    shell("pe", off=.5, color="couro_esc")


# ---- os NPCs (02/11)


def lary():
    # a foto de referência: blusa vermelha ombro a ombro (os ombros de fora) com a manga larga em sino, short preto de
    # paetê, botas pretas de cano alto franzidas e uma pulseira de pérolas (a choker no queixo parecia roupa no rosto)
    shell("torso", .2, 1, color="carmim", decos=[band(0, .1, "carmim", 3), stripes("carmim", 5, 1, 1, faces=("frente",))],
          name="blusa")
    shell("busto", color="carmim", off=1.0, decos=[band(0, .25, "carmim", 3)], name="busto")
    # a faixa do ombro a ombro passa por cima do braço; a manga cai larga até o meio do antebraço
    shell("braco", .3, 1, color="carmim", off=.7, grow=(.3, .3, 0), decos=[band(0, .14, "carmim", 3)], name="manga")
    shell("antebraco", 0, .35, color="carmim", off=1.4, grow=(1.1, 1.1, 0), decos=[hem("carmim", 1, 0)], name="sino")
    shell("antebraco", .35, .7, color="carmim", off=1.8, grow=(1.8, 1.8, 0), decos=[hem("carmim", 1, 0)], name="sino_baixo")
    shell("antebraco", .9, 1, color="branco", off=.4, sides=("D",), decos=[dots("prata", 2, 3)], name="pulseira")
    # o short de paetê: a cintura alta e a perna curtinha
    shell("quadril", 0, 1, color="preto", off=.8, decos=[band(0, .12, "preto", 1), sparkle("prata", 7)], name="short")
    shell("coxa", 0, .26, color="preto", off=.9, grow=(.3, .3, 0), decos=[sparkle("prata", 7), hem("preto", 1, 0)], name="short_perna")
    # as botas: até abaixo do joelho, franzidas (as dobras mais claras), a sola grossa
    shell("canela", .02, 1, color="preto", off=.7,
          decos=[band(0, .1, "preto", 3), stripes("preto", 3, 1, 2), band(.85, 1, "preto", 1)], name="bota")
    shell("pe", off=.5, color="preto", decos=[hem("cinza", 1, 0)], name="bota_pe")


def julia():
    # a foto de referência: vestido preto de manga comprida, a saia rodada até o meio da coxa com a barra de renda, o
    # colar com a estrela prateada, meias pretas até a coxa, botas pretas de plataforma e óculos de armação
    shell("torso", color="preto", decos=[vneck(.16, .1), chain(.32, .68, .3, .02, "prata"),
                                         stripes("preto", 4, 1, 1, faces=("costas",), vertical=True)], name="vestido")
    shell("busto", color="preto", off=1.0, decos=[pendant(.5, .32, .2, "prata", 3, "star")], name="busto")
    shell("braco", color="preto", off=.8, grow=(.5, .5, 0), decos=[band(0, .15, "preto", 3)], name="manga_bufante")
    shell("antebraco", color="preto", off=.6, decos=[band(.86, 1, "preto", 3)], name="manga")
    shell("quadril", 0, .35, off=1.4, color="preto", decos=[rect(.44, .56, .1, .9, "prata", hollow=("preto", 0))], name="cinto")
    # a saia rodada em 3 faixas, com as pregas (listras mais claras) e a barra recortada da renda
    skirt(.25, 7, 3.0, "preto", decos=[jag(1), stripes("preto", 3, 1, 3, vertical=True)], tiers=3, name="saia")
    shell("coxa", .3, 1, color="azul_esc", off=.5, decos=[band(0, .12, "preto", 3)], name="meia")
    shell("canela", 0, .5, color="azul_esc", off=.5, name="meia_baixo")
    shell("canela", .45, 1, color="preto", off=1.1, grow=(.4, .4, 0),
          decos=[band(0, .12, "preto", 3), band(.3, .36, "cinza", 2, faces=("frente", "esq", "dir")), band(.6, .66, "cinza", 2, faces=("frente", "esq", "dir"))],
          name="bota")
    shell("pe", off=.8, color="preto", decos=[band(.75, 1, "cinza", 1)], name="plataforma")
    glasses("preto")


# alturas no corpo (texels a partir do alto do tronco): cintura 19,5; base do quadril 27,5; joelho ~39; tornozelo ~44;
# chão ~50. skirt(): o comprimento conta da base do quadril (11 = até o joelho, 16 = até o tornozelo)


def lexy():
    # a cabeleireira do Santuário (a foto do salão): suéter rosa largo de oncinha (rosetas marrom com o miolo magenta),
    # gola e punhos canelados rosa, short rosa e coturno preto de cano longo com cadarço
    knit = [leopard(), stripes("rosa_bebe", 2, 1, 3, faces=("costas",), vertical=True)]
    shell("torso", color="rosa_bebe", off=1.2, grow=(.3, .3, 0),
          decos=[band(0, .09, "rosa", 3)] + knit, name="sueter")
    shell("busto", color="rosa_bebe", off=1.6, decos=[band(0, .2, "rosa", 3)] + knit, name="busto")
    # as mangas largas e compridas, o punho canelado apertado no pulso
    shell("braco", color="rosa_bebe", off=1.1, grow=(.5, .5, 0), decos=knit, name="manga")
    shell("antebraco", 0, .82, color="rosa_bebe", off=1.3, grow=(.6, .6, 0), decos=knit, name="manga_baixo")
    shell("antebraco", .82, 1, color="rosa", off=.8, decos=[stripes("rosa", 2, 1, 3, vertical=True)], name="punho")
    # o suéter cobre o quadril (oversized) e termina na barra canelada; o short rosa aparece embaixo
    shell("quadril", 0, .55, color="rosa_bebe", off=1.6, grow=(.2, .2, 0),
          decos=[band(.8, 1, "rosa", 3), stripes("rosa", 2, 1, 2, vertical=True, faces=("frente", "costas", "esq", "dir"))] + knit[:1],
          name="sueter_barra")
    shell("quadril", .5, 1, color="rosa", off=.8, decos=[hem("rosa", 1, 0)], name="short")
    shell("coxa", 0, .22, color="rosa", off=.9, grow=(.3, .3, 0), decos=[hem("rosa", 1, 3)], name="short_perna")
    # o coturno: cano até logo abaixo do joelho, o cadarço cinza na frente, a borda de cima dobrada e a sola grossa
    shell("canela", 0, 1, color="preto", off=.8, grow=(.2, .2, 0),
          decos=[band(0, .1, "preto", 3), lace("cinza", .5, .12, .95), band(.5, .54, "preto", 2)], name="coturno")
    shell("pe", off=.6, color="preto", decos=[hem("cinza", 2, 0), rect(.35, .65, 0, .5, "preto", 3, faces=("frente",))], name="coturno_pe")


def juh():
    # moradora do vale (a amiga da Lary na foto): regata branca, saia jeans curta, tênis brancos
    shell("torso", .06, 1, color="branco", decos=[vneck(.2, .14), band(0, .06, "branco", 1)], name="regata")
    shell("busto", color="branco", off=1.0, name="busto")
    shell("quadril", 0, 1, color="jeans", off=.9, decos=[band(0, .14, "jeans", 1), vline(.3, .34, "jeans", .15, 1, 1),
                                                       dots("prata", 7, 3, faces=("frente",))], name="saia_jeans")
    skirt(1.0, 3, 1.0, "jeans", decos=[hem("areia", 1, 2)], tiers=1, name="barra")
    shell("pe", off=.5, color="branco", decos=[hem("cinza", 1, 1), band(.3, .5, "jeans", 2, faces=("esq", "dir"))], name="tenis")
    shell("canela", .85, 1, color="branco", off=.4, name="meia")


def ilsa():
    # herbalista: vestido verde até a canela, avental creme com bolsos de ervas, mangas dobradas, faixa de couro e uma
    # bolsa de ervas; botas marrons
    shell("torso", color="verde_folha", decos=[vneck(.2, .1), lace("couro_esc", .5, .1, .3)], name="vestido")
    shell("busto", color="verde_folha", off=1.0, name="busto")
    shell("braco", color="verde_folha", off=.8, name="manga")
    shell("antebraco", 0, .4, color="creme", off=1.1, grow=(.4, .4, 0), decos=[stripes("creme", 3, 1, 1)], name="manga_dobrada")
    shell("quadril", 0, 1, color="verde_folha", off=1.0, name="saia_alto")
    skirt(.3, 14, 2.6, "verde_folha", decos=[hem("terra", 1, 1)], tiers=3, name="saia")
    panel(10, 37, 10, "creme", decos=[rect(.12, .44, .55, .7, "creme", 0), rect(.56, .88, .55, .7, "creme", 0),
                                      rect(.2, .32, .5, .56, "verde", 3), hem("creme", 1, 1)], name="avental")
    shell("quadril", 0, .25, off=1.9, color="couro", decos=[rect(.44, .56, .1, .9, "ouro", hollow=("couro", 0))], name="faixa")
    block("quadril", (-9, -1, 0), (4, 3.5, 4.5), "couro", [rect(0, 1, 0, .3, "verde", 2, faces=("frente", "esq", "dir"))], "bolsa_ervas")
    shell("pe", off=.5, color="couro", decos=[hem("couro_esc", 1)])


def bram():
    # ferreiro: camisa escura de manga dobrada, avental grosso de couro com bolso e alças, luvas de couro grandes, calça
    # marrom, botas de bico reforçado
    shell("torso", color="azul_esc", decos=[vneck(.18, .09)], name="camisa")
    shell("braco", 0, .75, color="azul_esc", off=.6, decos=[hem("azul_esc", 1, 3)], name="manga")
    panel(2, 38, 13, "couro_esc", decos=[rect(.3, .7, .38, .5, "couro_esc", 0), band(.0, .03, "couro", 3),
                                        dots("metal", 9, 3), hem("couro_esc", 1, 0)], name="avental")
    shell("torso", 0, .3, off=1.5, color="couro_esc", sides=("E", "D"), decos=[cross("couro_esc", .07, faces=("costas",))], name="alcas")
    shell("antebraco", .45, 1, off=1.0, color="couro", grow=(.5, .5, 0), decos=[band(0, .14, "couro", 3)], name="luva")
    shell("mao", off=.8, color="couro", grow=(.3, .3, 0), name="luva_mao")
    shell("quadril", .3, 1, color="marrom_calca")
    shell(["coxa", "canela"], color="marrom_calca")
    shell("canela", .6, 1, off=.9, color="couro_esc", decos=[band(0, .18, "couro_esc", 3)], name="bota")
    shell("pe", off=.6, color="couro_esc", decos=[rect(0, 1, 0, 1, "metal", faces=("frente",))])


def nicolly():
    # cartógrafa: blusa creme, colete marrom com botões, bolsa de mapas atravessada, o tubo dos mapas nas costas,
    # calça de linho e botas
    shell("torso", color="creme", decos=[vneck(.25, .12)])
    shell("busto", color="couro", off=1.0, decos=[dots("ouro", 4, 3, faces=("frente",))], name="busto")
    shell("braco", color="creme", off=.6)
    shell("antebraco", 0, .85, color="creme", off=.6, decos=[hem("creme", 1, 0)])
    shell("torso", .12, 1, off=1.0, color="couro", decos=[open_front(.12, .25, "creme", "couro_esc"),
                                                        cross("couro_esc", .06, faces=("frente", "costas"))], name="colete")
    block("quadril", (8, -1.5, 0), (5, 4, 5), "couro_esc", [rect(.1, .9, 0, .3, "couro", 2, faces=("frente",)),
                                                           rect(.4, .6, .3, .45, "ouro", 3, faces=("frente",))], "bolsa")
    back_block(1, 17, 3, 3, "terra", [band(0, .08, "couro_esc", 1), band(.92, 1, "couro_esc", 1), band(.48, .52, "couro", 2)],
               dx=-3, name="tubo_mapas")
    shell("quadril", .3, 1, color="areia")
    shell(["coxa", "canela"], color="areia")
    shell("canela", .55, 1, off=.9, color="couro", decos=[lace("couro_esc", .5, .1, .9)], name="bota")
    shell("pe", off=.5, color="couro", decos=[hem("couro_esc", 1)])


def ducci():
    # mercador mago ambulante: manto roxo comprido com a barra e a gola douradas, faixa dourada, estrelas bordadas,
    # mochila grande de mercador nas costas, sapatos de bico
    gold = "ouro"
    shell("torso", color="roxo", decos=[vline(.46, .54, gold), dots(gold, 10, 3)], name="manto")
    shell("braco", color="roxo", off=.8, decos=[dots(gold, 10, 3)])
    shell("antebraco", color="roxo", off=1.2, grow=(1.2, 1.2, 0), decos=[band(.78, 1, gold, None)], name="manga")
    shell("quadril", color="roxo", off=1.1, decos=[vline(.46, .54, gold)], name="manto_baixo")
    skirt(.4, 15, 2.2, "roxo", decos=[hem(gold, 1, 2), vline(.46, .54, gold), dots(gold, 10, 3)], tiers=3, name="manto_saia")
    shell("quadril", .05, .35, off=2.0, color=gold, decos=[rect(.42, .58, .1, .9, "vinho", 2)], name="faixa")
    shell("torso", 0, .12, off=1.3, color="roxo", grow=(1, 1, 0), decos=[band(.4, 1, gold, None)], name="gola")
    back_block(1, 18, 12, 6, "couro", [band(.0, .08, "couro_esc", 1), band(.45, .52, "couro_esc", 1),
                                         rect(.3, .7, .2, .4, "couro_esc", 1, faces=("costas",)), dots(gold, 8, 3)], name="mochila")
    back_block(-2, 1, 10, 4, "vinho", [stripes("mostarda", 3, 1, 2)], name="manta")
    shell("pe", off=.6, color="roxo", decos=[hem(gold, 1, 1)])


def rhea():
    # capitã da guarda: peitoral de aço, tabardo azul com o brasão dourado, ombreiras, manoplas, cinto, calça de couro e
    # grevas de aço
    shell("torso", color="aco_azul", decos=[band(0, .1, "aco_azul", 3), stripes("aco_azul", 4, 1, 1)], name="peitoral")
    shell("busto", color="aco_azul", off=1.0, decos=[band(0, .3, "aco_azul", 3)], name="busto")
    panel(6, 36, 9, "azul_real", decos=[pendant(.5, .22, .12, "ouro", 3), hem("ouro", 1, 2),
                                       vline(.0, .08, "ouro", tone=2), vline(.92, 1, "ouro", tone=2)], name="tabardo")
    panel(4, 36, 9, "azul_real", side="costas", decos=[hem("ouro", 1, 2)], name="tabardo_costas")
    shell("braco", 0, .45, off=2.0, grow=(.6, .6, 0), color="aco_azul", decos=[band(.3, .36, "aco_azul", 0), hem("aco_azul", 1, 0)], name="ombreira")
    shell("braco", .45, 1, color="couro_esc", off=.6)
    shell("antebraco", .2, 1, off=1.0, color="aco_azul", decos=[band(0, .12, "aco_azul", 3)], name="manopla")
    shell("mao", off=.7, color="aco_azul", name="manopla_mao")
    shell("quadril", 0, .3, off=1.6, color="couro", decos=[rect(.42, .58, .05, .95, "ouro", hollow=("couro", 0))], name="cinto")
    shell("quadril", .3, 1, color="couro_esc")
    shell("coxa", color="couro_esc")
    shell("canela", color="aco_azul", off=1.0, decos=[band(0, .12, "aco_azul", 3), vline(.45, .55, "aco_azul", tone=3)], name="greva")
    shell("pe", off=.6, color="aco_azul", decos=[hem("couro_esc", 1)])


def sahil():
    # mercador das dunas: túnica cor de areia até os joelhos, faixa vermelha na cintura, colete curto, turbante com a
    # ponta solta, calça larga e sandálias
    shell("torso", color="areia", decos=[vneck(.25, .1), vline(.48, .52, "terra", .25, .6)], name="tunica")
    shell("braco", color="areia", off=.8, grow=(.4, .4, 0))
    shell("antebraco", 0, .8, color="areia", off=1.0, grow=(.6, .6, 0), decos=[hem("terra", 1, 1)], name="manga")
    shell("torso", .1, 1, off=1.1, color="terra", decos=[open_front(.16, .3, "areia", "couro"), dots("mostarda", 5, 3)], name="colete")
    shell("quadril", 0, 1, color="areia", off=1.0, name="tunica_baixo")
    skirt(.3, 9, 1.6, "areia", decos=[hem("vermelho", 1, 2)], tiers=2, name="tunica_saia")
    shell("quadril", .05, .4, off=2.0, color="vermelho", decos=[stripes("mostarda", 3, 1, 2)], name="faixa")
    head_wrap(3.5, "creme", decos=[stripes("areia", 2, 1, 1), rect(.4, .6, .3, .9, "vermelho", 2, faces=("frente",))])
    shell("canela", .5, 1, color="areia", off=.9, grow=(.5, .5, 0), name="calca_larga")
    shell("pe", off=.4, color="pele", decos=[band(.2, .45, "couro", 1, faces=("frente", "esq", "dir")), hem("couro", 1, 0)], name="sandalia")


def amal():
    # guia do acampamento: blusa de couro cru, lenço no pescoço, colete de pele, braçadeiras, cinto com bolsas, calça
    # de couro e botas de amarrar
    shell("torso", color="terra", decos=[vneck(.18, .1), lace("couro_esc", .5, .1, .4)], name="blusa")
    shell("busto", color="terra", off=1.0, name="busto")
    shell("braco", color="terra", off=.6)
    shell("torso", 0, .14, off=1.5, color="vermelho", grow=(.6, .6, 0), decos=[stripes("mostarda", 3, 1, 2)], name="lenco")
    shell("torso", .14, .85, off=1.1, color="couro", decos=[open_front(.14, .2, "terra", "creme"), stripes("couro", 2, 1, 3, faces=("costas",))],
          name="colete_pele")
    shell("antebraco", .2, 1, off=.9, color="couro_esc", decos=[lace("terra", .5, .2, .9)], name="bracadeira")
    shell("quadril", 0, .3, off=1.6, color="couro_esc", decos=[rect(.42, .58, .05, .95, "metal", hollow=("couro_esc", 0))], name="cinto")
    block("quadril", (8.5, -1, -.5), (4, 3.5, 4), "couro", [], "bolsa")
    block("quadril", (-8.5, -1, -.5), (4, 3.5, 4), "couro", [], "bolsa2")
    shell("quadril", .3, 1, color="couro_esc")
    shell(["coxa", "canela"], color="couro_esc")
    shell("canela", .45, 1, off=.9, color="couro", decos=[lace("couro_esc", .5, .1, .9), band(0, .12, "couro", 3)], name="bota")
    shell("pe", off=.5, color="couro", decos=[hem("couro_esc", 1)])


def tavian():
    # provedor dos caçadores: capa verde com o capuz caído, gibão de couro, aljava com flechas nas costas, braçadeira
    # de arqueiro, calça verde-escura e botas altas
    shell("torso", color="couro", decos=[vneck(.18, .08), quilt("couro")], name="gibao")
    shell("braco", color="verde", off=.6)
    shell("antebraco", .3, 1, off=.9, color="couro_esc", sides=("E",), decos=[band(0, .1, "couro_esc", 3)], name="bracadeira")
    shell("torso", 0, .45, off=1.8, color="verde", grow=(1.2, .4, 0), decos=[hem("verde", 1, 0), jag(1)], name="capa")
    panel(4, 30, 15, "verde", side="costas", gap=2.4, decos=[jag(1), hem("verde", 1, 0)], name="capa_costas")
    block("torso", (0, 6, 9.5), (13, 4, 5), "verde", [rect(.12, .88, 0, .8, "verde_folha", 0, faces=("costas", "cima"))], "capuz")
    back_block(-3, 15, 3.5, 3.5, "couro_esc", [band(0, .1, "couro", 2)], dx=3.5, name="aljava")
    back_block(-6, -3, 2.5, 2.5, "creme", [stripes("vermelho", 2, 1, 2)], dx=3.5, name="flechas")
    shell("quadril", 0, .3, off=1.6, color="couro_esc", decos=[rect(.42, .58, .05, .95, "metal", hollow=("couro_esc", 0))], name="cinto")
    shell("quadril", .3, 1, color="verde")
    shell(["coxa", "canela"], color="verde")
    shell("canela", .3, 1, off=.9, color="couro", decos=[band(0, .12, "couro", 3)], name="bota")
    shell("pe", off=.5, color="couro", decos=[hem("couro_esc", 1)])


def lyra():
    # rastreadora de Verdalume: túnica verde-escura com o capuz caído, corpete de couro, braçadeiras, cinto com faca,
    # calça escura e botas até o joelho
    shell("torso", color="verde", decos=[vneck(.22, .1)], name="tunica")
    shell("busto", color="couro_esc", off=1.0, name="busto")
    shell("braco", color="verde", off=.6)
    shell("antebraco", .15, 1, off=.9, color="couro_esc", decos=[lace("couro", .5, .2, .9)], name="bracadeira")
    shell("torso", .35, 1, off=1.0, color="couro_esc", decos=[lace("couro", .5, .1, .7)], name="corpete")
    shell("quadril", 0, .6, off=1.0, color="verde", decos=[jag(1)], name="tunica_baixo")
    block("torso", (0, 6, 9.5), (12, 4, 5), "verde", [rect(.12, .88, 0, .8, "verde_folha", 0, faces=("costas", "cima"))], "capuz")
    shell("quadril", .05, .3, off=1.7, color="couro", decos=[rect(.42, .58, .05, .95, "metal", hollow=("couro", 0))], name="cinto")
    block("quadril", (-8, -.5, 1), (2.5, 2.5, 7), "metal", [band(0, .4, "couro_esc", 1)], "faca")
    shell("quadril", .6, 1, color="azul_esc")
    shell(["coxa", "canela"], color="azul_esc")
    shell("canela", .1, 1, off=.9, color="couro_esc", decos=[band(0, .12, "couro_esc", 3)], name="bota")
    shell("pe", off=.5, color="couro_esc")


def garrick():
    # armeiro: cota de malha cinza, avental de couro curto por cima, ombreira de couro, manoplas de aço, cinto de
    # ferramentas e botas de ferro
    shell("torso", color="metal", decos=[stripes("metal", 2, 1, 1), stripes("metal", 2, 1, 3, vertical=True)], name="cota")
    shell("braco", 0, .7, color="metal", off=.7, decos=[stripes("metal", 2, 1, 1), hem("metal", 1, 0)], name="cota_manga")
    shell("quadril", 0, .5, color="metal", off=.8, decos=[stripes("metal", 2, 1, 1)], name="cota_baixo")
    panel(8, 34, 11, "couro", decos=[band(0, .04, "couro_esc", 1), dots("metal", 8, 3), hem("couro_esc", 1, 0)], name="avental")
    shell("braco", 0, .35, off=1.8, grow=(.4, .4, 0), color="couro_esc", sides=("D",), decos=[hem("couro", 1, 2)], name="ombreira")
    shell("antebraco", .3, 1, off=1.0, color="aco_azul", decos=[band(0, .12, "aco_azul", 3)], name="manopla")
    shell("mao", off=.7, color="aco_azul")
    shell("quadril", .15, .4, off=2.1, color="couro_esc", decos=[dots("metal", 4, 3)], name="cinto_ferramentas")
    block("quadril", (8.5, -1, -1), (3, 3, 6), "metal", [band(0, .3, "couro_esc", 1)], "martelo_cinto")
    shell("quadril", .5, 1, color="marrom_calca")
    shell(["coxa", "canela"], color="marrom_calca")
    shell("canela", .45, 1, off=1.0, color="aco_azul", decos=[band(0, .14, "aco_azul", 3)], name="bota_ferro")
    shell("pe", off=.6, color="aco_azul", decos=[hem("couro_esc", 1)])


def rosa():
    # alfaiate: blusa creme de manga bufante, corpete vinho com cordão, saia vermelha comprida com a barra bordada, fita
    # métrica amarela no pescoço, almofada de alfinetes no pulso e sapatinhos
    shell("torso", color="creme", decos=[vneck(.22, .12)])
    shell("busto", color="vinho", off=1.0, decos=[lace("creme", .5, .1, .9)], name="busto")
    shell("braco", 0, .5, color="creme", off=1.0, grow=(.8, .8, 0), decos=[hem("creme", 1, 1)], name="manga")
    shell("torso", .3, 1, off=1.0, color="vinho", decos=[lace("creme", .5, .05, .7)], name="corpete")
    shell("torso", 0, .1, off=1.5, color="mostarda", decos=[stripes("preto", 3, 1, 1, vertical=True)], name="fita_metrica")
    panel(0, 12, 2, "mostarda", decos=[stripes("preto", 3, 1, 1)], gap=1.6, name="fita_ponta")
    shell("antebraco", .75, .95, off=1.0, color="vermelho", sides=("E",), decos=[dots("prata", 2, 3)], name="alfineteira")
    shell("quadril", 0, 1, color="carmim", off=1.0, name="saia_alto")
    skirt(.25, 15, 2.4, "carmim", decos=[hem("mostarda", 1, 2), dots("rosa", 6, 2)], tiers=3, name="saia")
    shell("pe", off=.4, color="couro_esc", decos=[hem("preto", 1)])


def marta():
    # taverneira: blusa creme de manga dobrada, corpete vinho de amarrar, saia marrom comprida, avental branco com
    # bolso e o pano de prato no ombro
    shell("torso", color="creme", decos=[vneck(.26, .14)])
    shell("busto", color="vinho", off=1.0, decos=[lace("creme", .5, .2, .9)], name="busto")
    shell("braco", color="creme", off=.9, grow=(.4, .4, 0))
    shell("antebraco", 0, .3, color="creme", off=1.1, grow=(.4, .4, 0), decos=[stripes("creme", 2, 1, 1)], name="manga_dobrada")
    shell("torso", .32, 1, off=1.0, color="vinho", decos=[lace("creme", .5, .05, .6)], name="corpete")
    shell("quadril", 0, 1, color="couro", off=1.0, name="saia_alto")
    skirt(.25, 15, 2.2, "couro", decos=[hem("couro_esc", 1, 1)], tiers=3, name="saia")
    panel(20, 41, 11, "branco", decos=[rect(.3, .7, .3, .45, "branco", 1), hem("creme", 1, 1)], gap=3.6, name="avental")
    shell("quadril", 0, .18, off=2.0, color="branco", name="laco_avental")
    shell("braco", 0, .3, off=1.6, sides=("D",), color="azul", decos=[stripes("branco", 2, 1, 2)], name="pano")
    shell("pe", off=.4, color="couro_esc", decos=[hem("preto", 1)])


def bunny():
    # a jardineira do Santuário (a prancha do usuário, 10/11): top curto verde-sálvia de flores rosas com o babado rosa na
    # gola e na barra, os ombros de fora (a alça caída no braço, verde com a borda rosa), a barriga de fora, a calça
    # pantalona verde de flores com o cós rosa, o babado rosa na canela, a faixa verde no tornozelo e a sandália
    flor = dots("rosa", 4, 2)
    shell("torso", 0, .5, color="salvia", decos=[band(0, .12, "rosa_po", 3), band(.84, 1, "rosa_po", 2), flor], name="top")
    shell("busto", color="salvia", off=1.0, decos=[band(0, .25, "rosa_po", 3), flor], name="busto")
    shell("braco", .12, .34, color="salvia", off=.6, decos=[band(0, .3, "rosa_po", 2), band(.7, 1, "verde_folha", 1)], name="alca")
    shell("torso", .84, 1, color="rosa_po", off=.8, decos=[stripes("rosa_po", 2, 1, 3, vertical=True)], name="cos")
    shell("quadril", color="salvia", off=.9, decos=[flor], name="calca")
    shell("coxa", color="salvia", off=1.1, grow=(.6, .6, 0), decos=[flor, stripes("salvia", 3, 1, 3, vertical=True)], name="pantalona")
    shell("canela", 0, .55, color="salvia", off=1.3, grow=(.8, .8, 0), decos=[flor], name="pantalona_baixo")
    shell("canela", .55, .85, color="rosa_po", off=1.5, grow=(1, 1, 0),
          decos=[stripes("rosa_po", 2, 1, 3, vertical=True), hem("rosa_po", 1, 1), dots("salvia", 5, 1)], name="babado")
    shell("canela", .85, 1, color="verde_folha", off=.6, decos=[band(.5, .6, "rosa_po", 2)], name="tornozelo")
    shell("pe", off=.4, color="pele", decos=[band(.2, .45, "rosa_po", 1, faces=("frente", "esq", "dir")), hem("verde_folha", 1, 0)], name="sandalia")


# ---- 10/11: os trajes do menu desenhados à mão (as folhas H01 e E_sets só como guia de formas e cores). O mesmo
# desenho veste os dois corpos; o feminino (_f) muda o comprimento e um detalhe ou outro
def _fem():
    return OUTFIT.endswith("_f")


def _belt(c="couro", buckle="metal", off=1.7, a=.05, b=.3):
    shell("quadril", a, b, off=off, color=c, decos=[rect(.42, .58, .05, .95, buckle, hollow=(c, 0)), band(0, .12, c, 3)], name="cinto")


def _pouch(c="couro", side=1, name="bolsa"):
    block("quadril", (8.5 * side, -1, -.5), (4, 3.5, 4), c, [rect(.1, .9, 0, .3, "couro_esc", 2, faces=("frente", "esq", "dir")),
                                                          rect(.42, .58, .3, .45, "ouro", 3, faces=("frente",))], name)


def _pants(c="marrom_calca", a=.4):
    shell("quadril", a, 1, color=c)
    shell(["coxa", "canela"], color=c)


def _boots(c="couro", top=.5, fur=None, laced=True):
    decos = [band(0, .16, c, 3), band(.16, .2, "couro_esc", 0)] + ([lace("couro_esc", .5, .3, .92)] if laced else [])
    shell("canela", top, 1, off=.9, color=c, decos=decos, name="bota")
    shell("pe", off=.5, color=c, decos=[hem("couro_esc", 1), band(.45, .55, "couro_esc", 1, faces=("frente", "esq", "dir"))])
    if fur:
        shell("canela", top - .08, top + .22, off=1.5, grow=(.8, .8, 0), color=fur, decos=[stripes("creme", 2, 1, 3), dots("creme", 3, 3), jag(1)], name="bota_pelo")


def _hood(c, inner=None, trim=None):
    block("torso", (0, 6.5, 5), (15, 4.5, 10), c, [rect(.12, .88, 0, .85, inner or c, 0, faces=("costas", "cima"))]
          + ([band(0, 1, trim, None, faces=("esq", "dir")), hem(trim, 1)] if trim else [hem(c, 1, 0)]), "capuz")


def traje_peregrino():
    # túnica de linho creme com a gola amarrada, cordão trançado na cintura, bolsa, capinha verde-azulada com capuz,
    # calça marrom e botas de amarrar; a peregrina com a túnica mais comprida e as fendas do lado
    f = _fem()
    shell("torso", color="creme", decos=[vneck(.16, .08), stripes("creme", 5, 1, 1), lace("terra", .5, .02, .2)], name="tunica")
    shell("braco", color="creme", off=.6, decos=[stripes("creme", 5, 1, 1)])
    shell("antebraco", 0, .92, color="creme", off=.7, grow=(.3, .3, 0), decos=[hem("creme", 1, 0)], name="manga")
    shell("quadril", color="creme", off=1.0, decos=[stripes("creme", 5, 1, 1)], name="tunica_baixo")
    shell("coxa", 0, .75 if f else .4, color="creme", off=1.4, grow=(.6, .6, 0),
          decos=[jag(1), hem("areia", 1, 1)] + ([vline(.0, .1, "marrom_calca", .35, 1, 1, faces=("esq", "dir"))] if f else []), name="aba")
    shell("quadril", .05, .25, off=1.6, color="areia", decos=[stripes("terra", 2, 1, 2), band(.4, .6, "terra", 1)], name="cordao")
    panel(19.5, 25, 1.5, "areia", decos=[stripes("terra", 2, 1, 2)], gap=1.9, name="cordao_ponta")
    _pouch()
    shell("torso", 0, .32, off=1.9, grow=(1, 1, 0), color="teal", decos=[hem("teal", 1, 0), jag(1), stripes("teal", 4, 1, 1)], name="capa")
    shell("braco", 0, .32, off=2.0, color="teal", decos=[hem("teal", 1, 0), jag(1)], name="capa_ombro")
    _hood("teal", "teal")
    _pants("marrom_calca", .5)
    _boots("couro", .5)


def traje_escudeiro():
    # gibão vermelho acolchoado fechado com 3 presilhas de couro, gola alta, cinto com fivela de ferro e bolsa, calça
    # escura e botas; a escudeira com o gibão até o joelho
    f = _fem()
    q = quilt("vermelho")
    shell("torso", color="vermelho", decos=[q, vline(.48, .52, "vinho", tone=0)] +
          [rect(.4, .6, v, v + .05, "couro_esc", 1) for v in (.22, .45, .68)], name="gibao")
    shell("torso", 0, .1, off=1.0, grow=(.4, .4, 0), color="vermelho", decos=[q, hem("vinho", 1, 0)], name="gola")
    shell("braco", color="vermelho", off=.8, decos=[q])
    shell("antebraco", 0, .92, color="vermelho", off=.8, decos=[q, hem("vinho", 1, 1)], name="manga")
    shell("quadril", color="vermelho", off=1.0, decos=[q, vline(.48, .52, "vinho", tone=0)], name="gibao_baixo")
    shell("coxa", 0, .85 if f else .35, color="vermelho", off=1.5, grow=(.8, .8, 0), decos=[q, hem("vinho", 1, 1)], name="aba")
    _belt("couro", "ferro")
    _pouch()
    _pants("marrom_calca", .5)
    _boots("couro", .5, laced=False)


def traje_mago():
    # manto roxo comprido com a barra e a frente em linho, capuz, gola, cinto com fivela e um livro preso no quadril; a
    # maga com o corpete de amarrar e as mangas largas em sino
    f = _fem()
    trim = "areia"
    shell("torso", color="roxo", decos=[vline(.4, .6, trim, tone=1), stripes("roxo", 5, 1, 1)] + ([lace("ouro", .5, .2, .7)] if f else []), name="manto")
    shell("torso", 0, .12, off=1.3, grow=(1, 1, 0), color="roxo", decos=[hem(trim, 1, 1)], name="gola")
    shell("braco", color="roxo", off=.7)
    shell("antebraco", color="roxo", off=1.2, grow=(1.8, 1.8, 0) if f else (.9, .9, 0), decos=[band(.8, 1, trim, 1)], name="manga")
    shell("quadril", color="roxo", off=1.0, decos=[vline(.4, .6, trim, tone=1)], name="manto_baixo")
    shell("coxa", color="roxo", off=1.6, grow=(1.2, 1.2, 0), decos=[stripes("roxo", 4, 1, 1, vertical=True)], name="aba")
    shell("canela", 0, .72, color="roxo", off=1.6, grow=(1.2, 1.2, 0), decos=[hem(trim, 2, 1), dots("violeta", 6, 3)], name="aba_baixo")
    shell("canela", .72, 1, color="couro_esc")
    _hood("roxo", "violeta")
    _belt("couro", "metal", 1.9)
    block("quadril", (8.5, -1.2, -1), (3.5, 4, 5), "couro", [rect(.15, .85, .08, .92, "creme", 3, faces=("frente",)),
                                                            band(.45, .55, "ouro", 2, faces=("frente", "esq", "dir"))], "livro")
    shell("pe", off=.5, color="couro_esc", decos=[hem("preto", 1)])


def traje_cacador():
    # túnica verde com o colete de couro amarrado, gola de pele de carneiro, cinto e bolsa, calça marrom e botas com
    # a borda de pele
    f = _fem()
    shell("torso", color="verde", decos=[vneck(.14, .08)], name="tunica")
    shell("braco", color="verde", off=.6, decos=[stripes("verde", 5, 1, 1)])
    shell("antebraco", 0, .92, color="verde", off=.7, decos=[hem("verde", 1, 0), band(.7, .76, "couro", 1)], name="manga")
    shell("torso", .12, 1, off=1.1, color="couro", decos=[lace("areia", .5, .12, .8), vline(.0, .08, "couro_esc", tone=1),
                                                        vline(.92, 1, "couro_esc", tone=1), stripes("couro", 3, 1, 3, faces=("costas",))], name="colete")
    shell("quadril", color="verde", off=1.0, name="tunica_baixo")
    shell("coxa", 0, .5 if f else .35, color="verde", off=1.4, grow=(.6, .6, 0), decos=[jag(1), hem("verde", 1, 0)], name="aba")
    shell("torso", 0, .3, off=1.7, grow=(1.6, 1.6, 0), color="areia", decos=[stripes("creme", 2, 1, 3), dots("creme", 3, 3), jag(2)], name="gola_pelo")
    shell("braco", 0, .38, off=1.8, grow=(.8, .8, 0), color="areia", decos=[stripes("creme", 2, 1, 3), dots("creme", 3, 3), jag(1)], name="pelo_ombro")
    _belt("couro_esc", "metal")
    _pouch()
    _pants("marrom_calca", .5)
    _boots("couro", .5, fur="areia", laced=False)


def traje_arauto():
    # camisa creme de mangas compridas, tabardo azul sem manga com a cruz dourada na frente e nas costas, cinto e bolsa,
    # calça cinza e botas; a arauta com o tabardo até o tornozelo e a borda dourada
    f = _fem()
    both = ("frente", "costas")
    shell("torso", color="creme", decos=[vneck(.12, .08)], name="camisa")
    shell("braco", color="creme", off=.6, decos=[stripes("creme", 5, 1, 1)])
    shell("antebraco", 0, .92, color="creme", off=.7, grow=(.3, .3, 0), decos=[hem("creme", 1, 0)], name="manga")
    cross_ = [vline(.43, .57, "mostarda", tone=2, faces=both), band(.3, .4, "mostarda", 2, faces=both)]
    edge = [vline(0, .07, "mostarda", tone=1, faces=both), vline(.93, 1, "mostarda", tone=1, faces=both)] if f else []
    shell("torso", .02, 1, off=1.1, color="azul_real", decos=cross_ + edge + [stripes("azul_real", 5, 1, 1)], name="tabardo")
    shell("quadril", off=1.1, color="azul_real", decos=[vline(.43, .57, "mostarda", tone=2, faces=both)] + edge, name="tabardo_baixo")
    shell("coxa", 0, 1 if f else .5, off=1.5, grow=(.7, .7, 0), color="azul_real", decos=[hem("mostarda" if f else "azul_real", 1, 1)], name="aba")
    if f:
        shell("canela", 0, .82, off=1.5, grow=(.7, .7, 0), color="azul_real", decos=[hem("mostarda", 1, 2), dots("azul_po", 7, 1)], name="aba_baixo")
    _belt("couro", "metal", 1.9)
    _pouch()
    _pants("cinza", .5)
    _boots("couro", .5, laced=False)


def elfo_floresta():
    # o guardião da floresta: camisa creme, túnica verde com os bordados de folha dourados e a barra em folhas, corpete de
    # couro amarrado, ombreiras de folha, capuz caído, cinto com fivela de prata, calça marrom-escura e botas com tiras em X
    f = _fem()
    shell("torso", color="creme", decos=[vneck(.2, .1)], name="camisa")
    shell("braco", color="creme", off=.6)
    shell("antebraco", 0, .92, color="creme", off=.7, grow=(.3, .3, 0), decos=[hem("creme", 1, 0), band(.75, .82, "couro", 1)], name="manga")
    shell("torso", .08, 1, off=1.0, color="verde", decos=[open_front(.16, .3, "couro", "ouro"), dots("ouro", 6, 2, faces=("costas",)),
                                                        vline(.48, .52, "ouro", .2, .9, 2, faces=("costas",))], name="tunica")
    shell("torso", .2, .95, off=1.3, color="couro", decos=[lace("areia", .5, .05, .9), vline(0, .1, "couro_esc", tone=1), vline(.9, 1, "couro_esc", tone=1)],
          name="corpete")
    shell("quadril", color="verde", off=1.0, decos=[dots("ouro", 6, 2)], name="tunica_baixo")
    shell("coxa", 0, .55 if f else .45, color="verde", off=1.4, grow=(.6, .6, 0), decos=[jag(2), hem("ouro", 1, 2), dots("ouro", 5, 2)], name="aba")
    shell("braco", 0, .38, off=1.8, grow=(.5, .5, 0), color="verde", decos=[jag(1), hem("ouro", 1, 2), vline(.45, .55, "ouro", tone=2, faces=("frente", "costas", "esq", "dir"))], name="ombreira_folha")
    _hood("verde", "verde_folha", "ouro")
    _belt("couro", "prata")
    _pants("couro_esc", .5)
    shell("canela", .3, 1, off=.9, color="couro", decos=[cross("couro_esc", .08, faces=("frente", "costas")), band(0, .14, "couro", 3)], name="bota")
    shell("pe", off=.5, color="couro", decos=[hem("couro_esc", 1)])


def elfo_mago():
    # o mago da corte da lua: manto azul-acinzentado com a frente e as mangas em linho creme, o manto de ombro com a
    # joia prateada, os bordados, cinto de prata, sapatos prateados; a maga com o corpete amarrado
    f = _fem()
    shell("torso", color="creme", decos=[vneck(.14, .08)] + ([lace("azul_po", .5, .25, .65)] if f else []), name="veste")
    shell("torso", .05, 1, off=1.0, color="azul_po", decos=[open_front(.18, .5, "creme", "prata"), pendant(.5, .2, .1, "prata", 3, "star")], name="manto")
    shell("torso", 0, .3, off=1.8, grow=(1.1, 1.1, 0), color="azul_po", decos=[hem("creme", 1, 1), jag(1), pendant(.5, .45, .14, "prata", 3)], name="manto_ombro")
    shell("braco", color="creme", off=.7)
    shell("antebraco", color="creme", off=1.2, grow=(1.4, 1.4, 0) if f else (1, 1, 0), decos=[band(.72, 1, "azul_po", 1), vline(.45, .55, "azul_po", .3, .65, 2)], name="manga")
    shell("quadril", color="azul_po", off=1.0, decos=[vline(.32, .68, "creme", tone=1)], name="manto_baixo")
    shell("coxa", color="azul_po", off=1.6, grow=(1.2, 1.2, 0), decos=[dots("creme", 6, 2)], name="aba")
    shell("canela", 0, .85, color="azul_po", off=1.6, grow=(1.2, 1.2, 0), decos=[hem("creme", 2, 1), dots("creme", 6, 2)], name="aba_baixo")
    shell("canela", .85, 1, color="creme")
    _belt("prata", "azul_po", 1.9)
    shell("pe", off=.5, color="prata", decos=[hem("azul_po", 1, 1)])


def elfo_sombra():
    # o caçador das sombras: túnica violeta com as bordas lilás e os ramos claros, mangas cinza-escuras, capuz, as alças
    # em X com fivela, cinto, barra em folhas, calça e botas escuras
    f = _fem()
    shell("torso", color="azul_esc", name="camisa")
    shell("braco", color="azul_esc", off=.6, decos=[stripes("azul_esc", 4, 1, 1)])
    shell("antebraco", 0, .92, color="azul_esc", off=.7, decos=[hem("preto", 1, 0), band(.6, .7, "couro_esc", 1)], name="manga")
    shell("torso", .06, 1, off=1.0, color="violeta", decos=[open_front(.1, .25, "azul_esc", "lilas"), vline(.48, .52, "lilas", .3, .9, 2, faces=("costas",)),
                                                          cross("couro_esc", .07, faces=("frente",)), rect(.62, .74, .5, .6, "metal", hollow=("couro_esc", 0))], name="tunica")
    shell("quadril", color="violeta", off=1.0, decos=[dots("lilas", 7, 2)], name="tunica_baixo")
    shell("coxa", 0, .55 if f else .45, color="violeta", off=1.4, grow=(.6, .6, 0), decos=[jag(2), hem("lilas", 1, 2)], name="aba")
    shell("braco", 0, .36, off=1.8, grow=(.5, .5, 0), color="violeta", decos=[jag(1), hem("lilas", 1, 1)], name="ombreira")
    _hood("violeta", "azul_esc", "lilas")
    _belt("couro_esc", "metal")
    _pants("preto", .5)
    _boots("couro_esc", .45, laced=False)

# ---- 11/11 (o usuário: "crie os visuais dos bandidos: o pequeno, o gordo, os normais e os cavaleiros" e "os viajantes,
# roupas únicas para eles"): os bandidos (AVortexMonster: Bandit, capitão, BanditBrute, BanditArcher; o BanditShield usa a
# do bandido por baixo da cota de malha) e os viajantes das estradas


def _mask(c):
    """O lenço do bandido: a metade de baixo do rosto coberta, amarrado em volta da cabeça."""
    PIECES.append((PARTS["cabeca"].cut(.5, .84), 1.2, c, [hem(c, 1, 0), stripes(c, 3, 1, 1)], (0, 0, 0), "lenco"))
    PIECE_PART.append("cabeca")


def bandido():
    # camisa cinza, colete de couro escuro aberto com as alças em X, cinto de ferro e bolsa, calça preta, botas escuras,
    # capuz preto e o lenço vinho no rosto
    shell("torso", color="cinza", decos=[vneck(.2, .08)], name="camisa")
    shell("braco", color="cinza")
    shell("antebraco", 0, .9, color="cinza", off=.6, decos=[hem("couro_esc", 1, 0)], name="manga")
    shell("torso", off=1.1, color="couro_esc", decos=[open_front(.21, .3, "cinza", "preto"), cross("couro")], name="colete")
    shell("quadril", 0, .55, off=1.0, color="couro_esc", decos=[jag(1)], name="colete_baixo")
    _belt("couro", "ferro")
    _pouch("couro_esc")
    _pants("preto", .5)
    _boots("couro_esc", .5)
    _hood("preto", "couro_esc")
    _mask("vinho")


def bandido_capitao():
    # o capitão: casaco vinho comprido até os joelhos com a barra dourada e botões, faixa carmim na cintura, alça de
    # couro, braçadeiras, calça preta e botas de cano alto; a cabeça descoberta com a faixa preta
    shell("torso", color="vinho", decos=[vline(.47, .53, "ouro", tone=1)] + [rect(.44, .56, v, v + .04, "ouro", 2) for v in (.2, .4, .6)], name="casaco")
    shell("braco", color="vinho", off=.7)
    shell("antebraco", 0, .95, color="vinho", off=.8, grow=(.4, .4, 0), decos=[hem("ouro", 1, 1)], name="manga")
    shell("quadril", color="vinho", off=1.1, decos=[vline(.47, .53, "ouro", tone=1)], name="casaco_baixo")
    shell("coxa", 0, .7, color="vinho", off=1.5, grow=(.7, .7, 0), decos=[hem("ouro", 1, 1), jag(1)], name="aba")
    shell("quadril", 0, .3, off=1.8, color="carmim", decos=[stripes("vinho", 3, 1, 1)], name="faixa")
    shell("torso", off=1.6, color="couro", decos=[cross("couro_esc")], name="alca")
    shell("antebraco", .3, 1, off=1.1, color="couro_esc", decos=[band(0, .12, "couro", 3)], name="bracadeira")
    _pants("preto", .5)
    _boots("couro_esc", .35, laced=False)
    head_wrap(3, "preto", [hem("vinho", 1, 0)], name="faixa_cabeca")


def bandido_gordo():
    # o brutamontes (o jogo engorda o corpo): avental de couro grosso por cima da barriga, sem mangas, braçadeiras de
    # ferro, cinto largo, calça marrom, botas e a bandana vermelha
    shell("torso", color="couro_esc", decos=[vneck(.28, .12)], name="camisa")
    shell("torso", off=1.8, grow=(1.2, 1.2, 0), color="couro", decos=[stripes("couro_esc", 4, 1, 1), rect(.2, .8, .55, .9, "couro_esc", 1)], name="avental")
    shell("quadril", 0, .8, off=1.8, grow=(1.0, 1.0, 0), color="couro", decos=[stripes("couro_esc", 4, 1, 1), hem("couro_esc", 1)], name="avental_baixo")
    shell("antebraco", .2, 1, off=1.0, color="ferro", decos=[band(.4, .5, "couro_esc", 0), hem("ferro", 1, 0)], name="bracadeira")
    _belt("couro_esc", "ferro", off=2.6)
    _pants("marrom_calca", .7)
    _boots("couro_esc", .5, laced=False)
    head_wrap(4, "vermelho", [stripes("vinho", 3, 1, 1)], name="bandana")


def bandido_pequeno():
    # o batedor, baixinho: túnica verde-musgo rasgada, capa curta de couro com capuz, lenço no rosto, calça marrom e botas
    shell("torso", color="verde", decos=[jag(2), stripes("verde", 4, 1, 1)], name="tunica")
    shell("braco", color="verde", off=.6)
    shell("antebraco", 0, .85, color="verde", off=.7, decos=[jag(1)], name="manga")
    shell("quadril", 0, .9, color="verde", off=1.0, decos=[jag(2)], name="tunica_baixo")
    shell("torso", 0, .35, off=1.9, grow=(1, 1, 0), color="couro_esc", decos=[jag(1), hem("couro", 1, 0)], name="capa")
    shell("braco", 0, .35, off=2.0, color="couro_esc", decos=[jag(1)], name="capa_ombro")
    _hood("couro_esc", "verde")
    _mask("couro_esc")
    _belt("couro", "metal", off=1.5)
    _pouch("couro", -1)
    _pants("marrom_calca", .6)
    _boots("couro", .5)


def viajante_mercador():
    # o mercador: túnica de linho com colete azul, cinto com a bolsa de moedas, a mochila grande de couro com o
    # cobertor enrolado em cima, calça e botas de viagem
    shell("torso", color="creme", decos=[vneck(.16, .08)], name="tunica")
    shell("braco", color="creme", off=.6)
    shell("antebraco", 0, .9, color="creme", off=.7, decos=[hem("areia", 1, 0)], name="manga")
    shell("torso", off=1.1, color="azul_po", decos=[open_front(.2, .3, "creme", "azul"), rect(.15, .28, .5, .58, "ouro", 2)], name="colete")
    shell("quadril", color="creme", off=1.0, decos=[stripes("areia", 5, 1, 1)], name="tunica_baixo")
    _belt("couro", "ouro")
    _pouch("couro", 1)
    block("torso", (0, 8, -1), (13, 6, 15), "couro", [rect(.1, .9, .1, .3, "couro_esc", 1, faces=("costas",)),
                                                     rect(.4, .6, .3, .5, "ouro", 2, faces=("costas",))], "mochila")
    block("torso", (0, 8, 8.5), (15, 5, 4), "vermelho", [stripes("creme", 3, 1, 1)], "cobertor")
    _pants("marrom_calca", .5)
    _boots("couro", .5)


def viajante_bardo():
    # o bardo: gibão listrado de mostarda e vinho com mangas bufantes, capa curta vinho num ombro, faixa na cabeça,
    # calça escura e botas de cano dobrado
    shell("torso", color="mostarda", decos=[stripes("vinho", 4, 2, 1, vertical=True), vneck(.18, .08)], name="gibao")
    shell("braco", color="vinho", off=1.0, grow=(.8, .8, 0), decos=[stripes("mostarda", 3, 1, 2)], name="manga_bufante")
    shell("antebraco", 0, .9, color="mostarda", off=.7, decos=[hem("vinho", 1, 1)], name="manga")
    shell("quadril", 0, .6, color="vinho", off=1.0, decos=[jag(1), hem("mostarda", 1)], name="gibao_baixo")
    shell("braco", 0, .5, off=2.2, grow=(.6, .6, 0), color="vinho", sides=("E",), decos=[hem("ouro", 1, 1)], name="capa_ombro")
    _belt("couro_esc", "ouro")
    _pants("azul_esc", .5)
    _boots("couro", .4, laced=False)
    head_wrap(3, "vinho", [hem("mostarda", 1, 0)], name="faixa_cabeca")


def viajante_campones():
    # o camponês que leva a colheita: camisa de algodão cru com as mangas enroladas, colete de lã, avental curto,
    # calça marrom remendada, botas de lama e o lenço de pano na cabeça
    shell("torso", color="areia", decos=[vneck(.2, .1), stripes("areia", 5, 1, 1)], name="camisa")
    shell("braco", 0, .7, color="areia", off=.7, decos=[hem("areia", 2, 1)], name="manga")
    shell("torso", off=1.1, color="salvia", decos=[open_front(.2, .32, "areia", "verde")], name="colete")
    shell("quadril", 0, .7, off=1.4, color="creme", decos=[hem("terra", 1, 1)], name="avental")
    _belt("terra", "ferro", off=1.6)
    _pants("marrom_calca", .5)
    shell("coxa", .3, .5, off=.6, color="terra", decos=[stripes("marrom_calca", 2, 1, 1)], name="remendo")
    _boots("terra", .55, laced=False)
    head_wrap(3, "creme", [stripes("areia", 3, 1, 1)], name="lenco_cabeca")


OUTFITS = {"bunny": bunny, "aventureiro": aventureiro, "guerreiro": guerreiro, "mago": mago,
           "arqueira": arqueira, "sacerdotisa": sacerdotisa, "aventureira": aventureira, "lary": lary, "julia": julia,
           "juh": juh, "ilsa": ilsa, "bram": bram, "nicolly": nicolly, "ducci": ducci, "rhea": rhea, "sahil": sahil,
           "amal": amal, "tavian": tavian, "lyra": lyra, "garrick": garrick, "rosa": rosa, "marta": marta, "lexy": lexy,
           "bandido": bandido, "bandido_capitao": bandido_capitao, "bandido_gordo": bandido_gordo, "bandido_pequeno": bandido_pequeno,
           "viajante_mercador": viajante_mercador, "viajante_bardo": viajante_bardo, "viajante_campones": viajante_campones}
for _sx in "mf":   # 10/11: os trajes do menu desenhados (mesmo desenho nos dois corpos)
    OUTFITS.update({"Traje_peregrino_" + _sx: traje_peregrino, "Traje_escudeiro_" + _sx: traje_escudeiro, "Traje_mago_" + _sx: traje_mago,
                    "Traje_cacador_" + _sx: traje_cacador, "Traje_arauto_" + _sx: traje_arauto, "Elfo_floresta_" + _sx: elfo_floresta,
                    "Elfo_mago_" + _sx: elfo_mago, "Elfo_sombra_" + _sx: elfo_sombra})
if PIECE:
    # as funções de relevo das armaduras primeiro: as peças redefinem os nomes iguais (legs, horns...)
    exec(open(os.path.join(ROOT, "Tools", "Blender", "armor_designs.py"), encoding="utf-8").read(), globals())
    exec(open(os.path.join(ROOT, "Tools", "Blender", "piece_designs.py"), encoding="utf-8").read(), globals())
    HATLIKE = OUTFIT in CATALOG and CATALOG[OUTFIT][0] in ("chapeu", "acessorio", "cintura", "bracos")
    BUSTY = OUTFIT in CATALOG and CATALOG[OUTFIT][2] == "F" and CATALOG[OUTFIT][0] in ("camisa", "cima", "traje", "busto")
    ELF_DETAIL = ELF_DETAIL or HATLIKE
    if HATLIKE:
        RELIEF_MAT = piece_material(OUTFIT)
    PIECE_BUILD(OUTFIT)
elif ARMOR:
    exec(open(os.path.join(ROOT, "Tools", "Blender", "armor_designs.py"), encoding="utf-8").read(), globals())
    ARMOR_BUILD(OUTFIT)
else:
    OUTFITS[OUTFIT]()
    BUSTY = OUTFIT in ("arqueira", "sacerdotisa", "aventureira")   # os trajes femininos do início


def region_of(pi):
    """A região do corpo da peça (o material dela): 0 cabeça, 1 tronco e braços, 2 pernas, 3 pés (as botas)."""
    part, name = PIECE_PART[pi], PIECES[pi][5]
    if part in ("cabeca", "pescoco"):
        return 0
    if part.startswith("pe_") or "bota" in name:
        return 3
    if part == "quadril" or part.startswith(("coxa", "canela")):
        return 2
    return 1


REGIONS = ["cabeca", "tronco", "pernas", "pes"]

# ---- a malha e a textura (um atlas de faixas: cada face da caixa vira um retângulo de texels)
FACES = [("dir", 0, 1), ("esq", 0, -1), ("costas", 1, 1), ("frente", 1, -1), ("cima", 2, 1), ("baixo", 2, -1)]


def world_name(M, axis, sign):
    """O nome da face pela direção no mundo (a frente do herói é -Y)."""
    n = (M.to_3x3() @ Vector([sign if a == axis else 0 for a in range(3)])).normalized()
    best = max((("dir", n.x), ("esq", -n.x), ("costas", n.y), ("frente", -n.y), ("cima", n.z), ("baixo", -n.z)),
               key=lambda t: t[1])
    return best[0]


PAINT = TEX / (4 if PIECE else 2)   # 10/11: as peças com o dobro de pixels, para o detalhe da folha caber       # o texel da pintura: metade do da grade (os detalhes da ficha)
tiles = []            # (peça, face, eixo da normal, sinal, colunas, linhas, eixo u, eixo v)
for pi, (box, off, color, decos, grow, name) in enumerate(PIECES):
    lo = box.lo - Vector((off + grow[0], off + grow[1], off + grow[2])) * TEX
    hi = box.hi + Vector((off + grow[0], off + grow[1], off + grow[2])) * TEX
    # o comprimento não ganha folga nas pontas (a manga para onde manda o trecho)
    lo[box.axis] = box.lo[box.axis]
    hi[box.axis] = box.hi[box.axis]
    PIECES[pi] = (Box(box.M, lo, hi, box.axis, box.flip), off, color, decos, grow, name)
    for fname, axis, sign in FACES:
        others = [a for a in range(3) if a != axis]
        # v sempre no eixo do comprimento quando ele está na face; u o outro
        if box.axis in others:
            va = box.axis
            ua = others[0] if others[1] == box.axis else others[1]
        else:
            ua, va = others
        cols = max(1, round((hi[ua] - lo[ua]) / PAINT))
        rows = max(1, round((hi[va] - lo[va]) / PAINT))
        tiles.append((pi, axis, sign, cols, rows, ua, va))

# empacota em prateleiras, 512 de largura
WIDTH = 1024 if HATLIKE else 512
x = y = shelf = 0
place = []
for t in tiles:
    cols, rows = t[3], t[4]
    if x + cols + 1 > WIDTH:
        x, y, shelf = 0, y + shelf + 1, 0
    place.append((x, y))
    x += cols + 1
    shelf = max(shelf, rows)
TUBE_RECT = None
if CLOTH_TUBE:
    _h = CLOTH_TUBE["z_top"] - CLOTH_TUBE["z_bot"]
    _w = 2 * CLOTH_TUBE["a"] if CLOTH_TUBE.get("sheet") else 2 * (CLOTH_TUBE["a"] + CLOTH_TUBE["b"]) + 2 * CLOTH_TUBE["flare"]
    _tc, _tr = max(4, round(_w / PAINT)), max(4, round(_h / PAINT))
    if x + _tc + 1 > WIDTH:
        x, y, shelf = 0, y + shelf + 1, 0
    TUBE_RECT = (x, y, _tc, _tr)
    shelf = max(shelf, _tr)
HEIGHT = 1
while HEIGHT < y + shelf + 1:
    HEIGHT *= 2
px = [0.0] * (WIDTH * HEIGHT * 4)
mask = [0.0] * (WIDTH * HEIGHT * 4)   # as armaduras: R = brilha, G = a franja (a cor do cabelo)


def tone_pick(rnd, base_tone, c, r, seed):
    """O tom: liso (o tom 2) com manchas de 2 x 2 texels um tom acima ou abaixo (ruído por texel parecia sujeira)."""
    if base_tone is not None:
        return base_tone
    h = random.Random(seed * 1009 + (c // 2) * 97 + (r // 2) * 13).random()
    return 1 if h < .1 else 3 if h > .94 else 2


# ---- a ficha: a cor de cada texel sai da vista da ficha (frente, lado esquerdo ou costas) onde ele aparece, com o herói
# na pose da ficha (os braços abaixados). Onde a ficha mostra o fundo (a peça passou do desenho), fica a pintura acima
REF = os.path.join(ROOT, "modelando ainda", "Ficha de Trajes RPG em Pixel Art.png")
SHEET = {"aventureiro": (0, 0), "guerreiro": (0, 1), "mago": (0, 2), "arqueira": (1, 0), "sacerdotisa": (1, 1),
         "aventureira": (1, 2)}
VIEWS = {}
if os.path.exists(REF) and OUTFIT in SHEET:
    rim = bpy.data.images.load(REF)
    RW, RH = rim.size
    RPX = rim.pixels[:]

    def rp(x, y):
        i = ((RH - 1 - y) * RW + x) * 4
        return RPX[i] * 255, RPX[i + 1] * 255, RPX[i + 2] * 255

    BG = rp(5, 5)

    def is_bg(c):
        return sum(abs(a - b) for a, b in zip(c, BG)) < 70

    y0, y1 = ((30, 450), (550, 975))[SHEET[OUTFIT][0]]
    cols_on = [any(not is_bg(rp(x, y)) for y in range(y0, y1, 3)) for x in range(RW)]
    spans, x = [], 0
    while x < RW:
        if cols_on[x]:
            a = x
            while x < RW and cols_on[x]:
                x += 1
            if x - a > 20:
                spans.append((a, x))
        else:
            x += 1
    for k, view in enumerate(("frente", "lado", "costas")):
        a, b = spans[SHEET[OUTFIT][1] * 3 + k]
        ys = [y for y in range(y0, y1) if any(not is_bg(rp(xx, y)) for xx in range(a, b, 2))]
        top, bottom = min(ys), max(ys)
        head = [xx for xx in range(a, b) if not is_bg(rp(xx, top + 12))]
        VIEWS[view] = (a, b, top, bottom, (min(head) + max(head)) / 2)

    # a pose da ficha: os braços abaixados até a largura da frente bater com a da ficha
    zs = [p.z for p, _ in VERTS]
    Z_TOP, Z_BOT = max(zs), min(zs)
    a, b, top, bottom, cx = VIEWS["frente"]
    SCALE = (bottom - top) / (Z_TOP - Z_BOT)
    want = (b - a) / SCALE
    DOM = [max(w, key=w.get)[len(P):] if w else "Hips" for _, w in VERTS]

    def deforms():
        bpy.context.view_layer.update()
        out = {}
        for pb in rig.pose.bones:
            out[pb.name[len(P):] if pb.name.startswith(P) else pb.name] = (
                rig.matrix_world @ pb.matrix @ pb.bone.matrix_local.inverted() @ rig.matrix_world.inverted())
        return out

    def pose_arms(theta):
        for side in ("Left", "Right"):
            rig.pose.bones[P + side + "Arm"].matrix_basis.identity()
        bpy.context.view_layer.update()
        inv = rig.matrix_world.inverted()
        for side, sgn in (("Left", 1), ("Right", -1)):
            pb = rig.pose.bones[P + side + "Arm"]
            h = pb.head.copy()
            R = inv.to_3x3() @ Matrix.Rotation(sgn * theta, 3, "Y") @ rig.matrix_world.to_3x3()
            pb.matrix = Matrix.Translation(h) @ R.to_4x4() @ Matrix.Translation(-h) @ pb.matrix
            bpy.context.view_layer.update()

    import math
    best = None
    for deg in range(0, 76, 3):
        pose_arms(math.radians(deg))
        D = deforms()
        width = max(abs((D.get(d, Matrix.Identity(4)) @ p).x) for (p, _), d in zip(VERTS, DOM)) * 2
        if best is None or abs(width - want) < best[0]:
            best = (abs(width - want), deg)
    pose_arms(math.radians(best[1]))
    DEF = deforms()
    log("ficha: vistas %s, braços abaixados %d°" % ({k: v[:2] for k, v in VIEWS.items()}, best[1]))

    def piece_bone(box):
        _, n, _ = tree0.find(box.M @ ((box.lo + box.hi) / 2))
        return DOM[n]

    tree0 = KDTree(len(VERTS))
    for n, (pp, _) in enumerate(VERTS):
        tree0.insert(pp, n)
    tree0.balance()

    def sample(p, n):
        """A cor da ficha para o ponto p (posado) com a normal n; None no fundo."""
        ax, ay, az = abs(n.x), abs(n.y), abs(n.z)
        if ax > ay and ax > az:
            view, X = "lado", p.y
        elif n.y > 0 and ay >= az:
            view, X = "costas", -p.x
        else:
            view, X = "frente", p.x
        a, b, top, bottom, cx = VIEWS[view]
        ix = int(cx + X * SCALE)
        iy = int(bottom - (p.z - Z_BOT) * SCALE)
        for dx in (0, -3 if ix > cx else 3, -6 if ix > cx else 6):
            xx = min(max(ix + dx, 0), RW - 1)
            c = rp(xx, min(max(iy, 0), RH - 1))
            if not is_bg(c):
                return c
        return None

LIMBS = ("Arm", "ForeArm", "Hand", "UpLeg", "Leg", "Foot")
CENTER_TREE = KDTree(len(VERTS))
for _n, (_p, _) in enumerate(VERTS):
    CENTER_TREE.insert(_p, _n)
CENTER_TREE.balance()


def piece_dom(pi):
    """O osso da peça: o da parte do membro em que ela está (None no tronco e na cueca)."""
    bone = PART_BONE.get(PIECE_PART[pi])
    return P + bone if bone else None


def rigid_piece(pi):
    return piece_dom(pi) is not None


def is_skin(c):
    r, g, b = c
    return r > 180 and 120 < g < 200 and 85 < b < 165 and r - g > 30 and g - b > 12 and r - b > 60


def despeckle(grid, limit=55.0):
    """Tira os pontos soltos (a borda da ficha misturando a roupa com o fundo ou a pele): um texel muito diferente de
    quase todos os vizinhos vira a média dos vizinhos parecidos entre si. As linhas (costura, barra) têm vizinhos iguais
    na direção delas e ficam."""
    rows, cols = len(grid), len(grid[0])
    out = [row[:] for row in grid]
    dist = lambda a, b: sum(abs(x - y) for x, y in zip(a, b))
    for r in range(rows):
        for c in range(cols):
            me = grid[r][c]
            near = [grid[r + dr][c + dc] for dr in (-1, 0, 1) for dc in (-1, 0, 1)
                    if (dr or dc) and 0 <= r + dr < rows and 0 <= c + dc < cols]
            if len(near) < 3:
                continue
            same = sum(1 for n in near if dist(n, me) < limit)
            if same <= 1:
                # a cor mais comum entre os vizinhos (o grupo maior de parecidos)
                best = max(near, key=lambda n: sum(1 for m in near if dist(m, n) < limit))
                group = [n for n in near if dist(n, best) < limit]
                out[r][c] = tuple(sum(ch) / len(group) for ch in zip(*group))
    return out


# ---- 07/11 (o usuário: "muito simples, cadê as camadas, as texturas"): as armaduras pegam a pintura da prancha. A peça
# recortada (Data/VortexArmor/Pieces/<fonte>.png, Tools/Armor/SliceArmorSheets.py) é projetada nos blocos: a frente na
# imagem inteira (a caixa da peça no mundo -> a caixa da imagem), as costas espelhadas e mais escuras, os lados na faixa
# da borda; o topo e a sola com a média. As partes claras e saturadas (gemas, lava, runas) brilham.
ARMOR_REF = None
if ARMOR and not PIECE:
    import json as _json
    _pieces = _json.load(open(os.path.join(ROOT, "Data", "Rpg", "armors.json"), encoding="utf-8"))["pieces"]
    _item = "arm_" + OUTFIT[2:]
    _src = os.path.join(ROOT, "Data", "VortexArmor", "Pieces", _pieces[_item]["source"] + ".png") if _item in _pieces else ""
    if os.path.exists(_src):
        _im = bpy.data.images.load(_src)
        _w, _h = _im.size
        _px = _im.pixels[:]
        _alpha = [[_px[((_h - 1 - y) * _w + x) * 4 + 3] for x in range(_w)] for y in range(_h)]
        _ys = [y for y in range(_h) if any(a > .5 for a in _alpha[y])]
        _xs = [x for x in range(_w) if any(_alpha[y][x] > .5 for y in range(0, _h, 2))]
        _all = [b.M @ Vector((x, y, z)) for b, *_ in PIECES for x in (b.lo.x, b.hi.x) for y in (b.lo.y, b.hi.y) for z in (b.lo.z, b.hi.z)]
        ARMOR_REF = {"w": _w, "h": _h, "px": _px, "alpha": _alpha, "bx": (min(_xs), max(_xs)), "by": (min(_ys), max(_ys)),
                     "X": (min(v.x for v in _all), max(v.x for v in _all)), "Y": (min(v.y for v in _all), max(v.y for v in _all)),
                     "Z": (min(v.z for v in _all), max(v.z for v in _all))}
        log("prancha", os.path.basename(_src), _w, _h)


def armor_sample(pw, face, pi=None):
    """A cor da prancha no ponto pw (mundo, em repouso) da face dada; None fora do desenho."""
    if PIECE:
        return piece_sample(pw, face, pi)
    R = ARMOR_REF
    (X0, X1), (Y0, Y1), (Z0, Z1) = R["X"], R["Y"], R["Z"]
    v = (Z1 - pw.z) / max(1e-6, Z1 - Z0)
    if face == "frente":
        u = (pw.x - X0) / max(1e-6, X1 - X0)
    elif face == "costas":
        u = (X1 - pw.x) / max(1e-6, X1 - X0)
    elif face in ("esq", "dir"):
        k = (pw.y - Y0) / max(1e-6, Y1 - Y0)
        u = .03 + .22 * k if face == "esq" else .97 - .22 * k
    else:
        return None
    (bx0, bx1), (by0, by1) = R["bx"], R["by"]
    alpha = R["alpha"]

    def color_at(x_, y_):
        i_ = ((R["h"] - 1 - y_) * R["w"] + x_) * 4
        return tuple(R["px"][i_ + k] * 255 for k in range(3))

    def plate(band):
        """A cor de uma faixa da prancha (a mais comum dela): as costas e os lados são placas, em vez de uma linha da imagem repetida."""
        key = ("plate", band)
        if key not in R:
            y0_, y1_ = by0 + (by1 - by0) * band // 6, by0 + (by1 - by0) * (band + 1) // 6
            got_ = sorted((color_at(x_, y_) for y_ in range(int(y0_), max(int(y0_) + 1, int(y1_)), 2) for x_ in range(bx0, bx1, 2) if alpha[y_][x_] > .5), key=sum)
            R[key] = got_[len(got_) // 2] if got_ else (90, 80, 70)
        return R[key]

    # 09/11 (o usuário: "puxado linhas retas"): só a frente usa o desenho; costas e lados são placas pintadas, com rebites e frisos
    if face != "frente":
        vv = min(.999, max(0, v)) * 6
        band = int(vv)
        got = plate(band)
        tx_, ty_ = int((pw.x + pw.y) / (TEX / 2)), int(pw.z / (TEX / 2))
        k_ = 1.0 + ((tx_ * 2 + ty_) % 23 == 0) * -.1 + ((tx_ + ty_ * 3) % 29 == 0) * .08
        if vv - band > .86:
            k_ = .55                                  # o friso entre as placas
        elif vv - band < .16 and tx_ % 6 == 0:
            k_ = 1.5                                  # os rebites
        elif face == "costas" and abs(pw.x - (X0 + X1) / 2) < TEX / 2:
            k_ = .68                                  # a costura do meio
        if "accent" not in R:
            # a cor mais viva da prancha (a lava, a gema, o dourado): vira o desenho das costas
            cs = [color_at(x_, y_) for y_ in range(by0, by1, 3) for x_ in range(bx0, bx1, 3) if alpha[y_][x_] > .5]
            R["accent"] = max(cs, key=lambda c_: (max(c_) - min(c_)) + sum(c_) * .1) if cs else got
        if face == "costas":
            across = abs(pw.x - (X0 + X1) / 2) / (TEX / 2)
            yy = (Z1 - pw.z) / (TEX / 2)
            if OUTFIT.endswith("_chest"):
                if yy < 26 and abs(across - (24 - yy * .85)) < .8:
                    k_ = .5                           # as placas das omoplatas, em V
                if across < 1.6 and yy % 7 < 3.5 and yy > 4:
                    return R["accent"]                # o espinhaço
                if abs(across - 7) < 1.1 and abs(yy % 14 - 7) < 1.1 and yy > 10:
                    return R["accent"]
            elif band == 2 and .3 < vv - band < .55 and tx_ % 5 < 3:
                return R["accent"]
        elif band == 2 and .3 < vv - band < .55 and tx_ % 4 == 0:
            return R["accent"]
        return tuple(min(255, c * k_) for c in got)
    x = int(bx0 + min(.999, max(0, u)) * (bx1 - bx0))
    y = int(by0 + min(.999, max(0, v)) * (by1 - by0))
    # 10/11: o pixel exato da prancha (ela já é pixel art: a média borrava); fora dela, o vizinho mais perto
    for d in range(0, 5):
        for a_, b_ in ((0, 0),) if d == 0 else ((d, 0), (-d, 0), (0, d), (0, -d)):
            if 0 <= x + a_ < R["w"] and 0 <= y + b_ < R["h"] and alpha[y + b_][x + a_] > .5:
                return color_at(x + a_, y + b_)
    return plate(min(5, int(min(.999, max(0, v)) * 6)))


if PIECE:
    ARMOR_REF = piece_ref_setup()

METALS = ("metal", "ferro", "prata", "ouro", "dourado", "aco_azul", "bronze")


def joint_shadow(grid, pi, axis, sign, ua, va, cols, rows):
    """09/11: escurece onde outra peça passa por cima desta (embaixo da ombreira, do cinto, da crista): o que dá profundidade."""
    import numpy as np
    box = PIECES[pi][0]
    uu, vv_ = np.meshgrid((np.arange(cols) + .5) / cols, (np.arange(rows) + .5) / rows)
    local = np.zeros((rows, cols, 3))
    local[..., axis] = box.hi[axis] if sign > 0 else box.lo[axis]
    local[..., ua] = box.lo[ua] + (box.hi[ua] - box.lo[ua]) * uu
    local[..., va] = box.lo[va] + (box.hi[va] - box.lo[va]) * vv_
    R_ = np.array(box.M.to_3x3())
    T_ = np.array(box.M.translation)
    world = local @ R_.T + T_ + (R_[:, axis] * sign * .6 * TEX)     # um pouco para fora da face
    reach = 1.5 * TEX
    k = np.ones((rows, cols))
    for j, other in enumerate(PIECES):
        if j == pi:
            continue
        b = other[0]
        inv = b.M.inverted()
        Ri = np.array(inv.to_3x3())
        L = world @ Ri.T + np.array(inv.translation)
        lo_, hi_ = np.array(b.lo), np.array(b.hi)
        d = np.maximum(lo_ - L, L - hi_).max(axis=-1)
        k = np.minimum(k, .74 + .26 * np.clip(d / reach, 0, 1))   # 10/11: mais leve (as armaduras ficaram escuras)
    return [[tuple(v * k[r, c] for v in grid[r][c]) for c in range(cols)] for r in range(rows)]


bm = bmesh.new()
uvl = bm.loops.layers.uv.new("UV")
VPIECE = []           # a peça de cada vértice (na ordem em que nascem)
PIECE_AVG = {}        # peça -> soma das cores da ficha nos lados (r, g, b, quantos): a cor do topo e da sola
for (pi, axis, sign, cols, rows, ua, va), (tx, ty) in zip(tiles, place):
    box, off, color, decos, grow, name = PIECES[pi]
    fname = world_name(box.M, axis, sign)
    rnd = random.Random(pi * 31 + axis * 7 + (sign > 0))
    # o sentido de u visto de fora (para a esquerda/direita da face) e o de v (0 = começo do comprimento)
    vflip = box.flip if va == box.axis else False
    if VIEWS:
        bone_of = piece_dom(pi)[len(P):] if piece_dom(pi) else "Hips"
        normal = box.M.to_3x3() @ Vector([sign if a == axis else 0 for a in range(3)])
    grid = [[None] * cols for _ in range(rows)]
    for r in range(rows):
        for c in range(cols):
            u = (c + .5) / cols
            v = (r + .5) / rows
            vv = 1 - v if vflip else v
            info = {"texel": (c, r if not vflip else rows - 1 - r), "rows": rows}
            col, tone = color, None
            from_deco = False
            for d in decos:
                res = d(fname, u, vv if va == box.axis else v, info)
                if res:
                    col, tone = res
                    from_deco = True
                    break
            if col is None:
                col = color
            shade = tone_pick(rnd, tone, c, r, pi * 7 + axis)
            if fname == "cima":
                shade = min(3, shade + 1)
            elif fname == "baixo":
                shade = max(0, shade - 1)
            if va == box.axis and (vv if va == box.axis else v) > .9 and tone is None:
                shade = max(0, shade - 1)
            # 02/11 (as roupas pintadas sem ficha, os NPCs): a luz de cima na primeira linha e o contorno escuro nas
            # beiradas da face (o bloco aparece, como na pixel art); as da ficha já trazem o sombreado dela
            if not VIEWS and tone is None and fname not in ("cima", "baixo"):
                if (vv if va == box.axis else v) < .08:
                    shade = min(3, shade + 1)
                if (c == 0 or c == cols - 1) and cols > 3:
                    shade = max(0, shade - 1)
            glow = col.startswith("*")
            col = col.lstrip("*")
            rgb = C[col][shade]
            # 10/11 (o usuário: "texturas, sombras, brilhos"): a luz de cima (a peça clareia no alto e escurece embaixo) e o
            # grão de cada material: a trama do pano (xadrez leve), as manchas do couro e o reflexo na borda do metal
            if not VIEWS and not ARMOR_REF and col not in ("pele", "cabelo"):
                vpos = (vv if va == box.axis else v) if fname not in ("cima", "baixo") else .5
                k = 1.07 - .16 * vpos
                if col in METALS:
                    k *= 1.2 if (r == 0 or c == 0) else .95 if (r == rows - 1 or c == cols - 1) else 1.0
                elif col.startswith("couro") or col == "marrom_calca":
                    k *= .9 if random.Random(pi * 977 + c * 31 + r * 17).random() < .14 else 1.0
                else:
                    k *= 1.035 if (c + r) % 2 else .975
                rgb = tuple(min(255, x * k) for x in rgb)
            # com a prancha, os padrões pintados (runas, rachaduras, estrelas) saem: a prancha já tem os dela. A cor acesa
            # da peça inteira (a gema) fica
            if ARMOR_REF and col != "cabelo" and (from_deco or not glow):
                glow = False
                q = Vector(box.lo)
                q[axis] = box.hi[axis] if sign > 0 else box.lo[axis]
                q[ua] = box.lo[ua] + (box.hi[ua] - box.lo[ua]) * u
                q[va] = box.lo[va] + (box.hi[va] - box.lo[va]) * v
                if fname in ("cima", "baixo"):
                    acc = PIECE_AVG.get(pi)
                    if acc and acc[3]:
                        k = (1.1 if fname == "cima" else .6) * random.Random(pi * 131 + c * 17 + r * 7).choice((.94, 1.0, 1.06))
                        rgb = tuple(min(255, acc[n] / acc[3] * k) for n in range(3))
                else:
                    got = armor_sample(box.M @ q, fname, pi)
                    if got:
                        rgb = got
                        acc = PIECE_AVG.setdefault(pi, [0.0, 0.0, 0.0, 0])
                        for n in range(3):
                            acc[n] += got[n]
                        acc[3] += 1
                        mx, mn = max(got), min(got)
                        # gema, lava, runa: clara e saturada (o branco e o dourado fosco não)
                        if mx > 200 and (mx - mn) > 110:
                            glow = True
            if glow or col == "cabelo" or (col in METALS and not ARMOR):
                mi = ((HEIGHT - 1 - (ty + r)) * WIDTH + tx + c) * 4
                # 10/11: azul = metal (a fivela brilha no M_ArmorBlock)
                mask[mi:mi + 4] = [1.0 if glow else 0.0, 1.0 if col == "cabelo" else 0.0, 1.0 if col in METALS and not ARMOR else 0.0, 1.0]
            if VIEWS:
                q = Vector(box.lo)
                q[axis] = box.hi[axis] if sign > 0 else box.lo[axis]
                q[ua] = box.lo[ua] + (box.hi[ua] - box.lo[ua]) * u
                q[va] = box.lo[va] + (box.hi[va] - box.lo[va]) * v
                Dm = DEF.get(bone_of, Matrix.Identity(4))
                got = sample(Dm @ (box.M @ q), (Dm.to_3x3() @ normal).normalized())
                # pele só no decote (a frente do tronco); nas outras peças a pele é o braço/perna da ficha fora do
                # lugar: fica a pintura da peça
                if got and is_skin(got) and not (name in ("torso", "colete") and fname == "frente"):
                    got = None
                if fname in ("cima", "baixo"):
                    # o topo (ombro, cintura) e a sola não aparecem em nenhuma vista da ficha: a cor média dos lados da
                    # peça, mais clara em cima e escura embaixo, com um grão leve (antes pegava cores soltas da frente)
                    acc = PIECE_AVG.get(pi)
                    if acc and acc[3]:
                        g = random.Random(pi * 131 + c * 17 + r * 7).choice((.94, 1.0, 1.0, 1.06))
                        k = (1.08 if fname == "cima" else .62) * g
                        rgb = tuple(min(255, acc[n] / acc[3] * k) for n in range(3))
                elif got:
                    rgb = got
                    acc = PIECE_AVG.setdefault(pi, [0.0, 0.0, 0.0, 0])
                    for n in range(3):
                        acc[n] += got[n]
                    acc[3] += 1
            grid[r][c] = rgb
    if VIEWS:
        grid = despeckle(despeckle(grid, 60.0), 60.0)
    if (ARMOR and not PIECE) or HATLIKE:
        grid = relief(grid, fname, pi, tx, ty)
    if (ARMOR and not PIECE) or ELF_DETAIL or not VIEWS:   # 10/11: as roupas desenhadas também (a sombra embaixo do cinto, da gola)
        grid = joint_shadow(grid, pi, axis, sign, ua, va, cols, rows)
    for r in range(rows):
        for c in range(cols):
            rgb = grid[r][c]
            i = ((HEIGHT - 1 - (ty + r)) * WIDTH + tx + c) * 4
            px[i:i + 4] = [rgb[0] / 255, rgb[1] / 255, rgb[2] / 255, 1.0]
    # a face: 4 cantos no local, de lo a hi nos eixos u e v, no lado axis/sign
    lo, hi = box.lo, box.hi
    # as peças do tronco em fatias de 2 texels no comprimento (dobram com a coluna, como o corpo); as dos membros são
    # rígidas, uma face só
    cuts = 1 if rigid_piece(pi) or va != box.axis else max(1, round((hi[va] - lo[va]) / (2 * TEX)))
    ring = []
    for k in range(cuts + 1):
        dv = k / cuts
        row = []
        for du in (0, 1):
            q = Vector(lo)
            q[axis] = hi[axis] if sign > 0 else lo[axis]
            q[ua] = hi[ua] if du else lo[ua]
            q[va] = lo[va] + (hi[va] - lo[va]) * dv
            row.append((bm.verts.new(box.M @ q), du, dv))
            VPIECE.append(pi)
        ring.append(row)
    for k in range(cuts):
        quad = [ring[k][0], ring[k][1], ring[k + 1][1], ring[k + 1][0]]
        f = bm.faces.new([q[0] for q in quad])
        f.material_index = 0 if ARMOR and not SET else region_of(pi)
        # 07/11 (o usuário: "faltando um lado, quebrado"): cada face é solta, e o recalc_face_normals não sabe o lado de fora
        # de uma face solta (virava umas para dentro, e o jogo não as desenha): vira pela direção da face na caixa
        f.normal_update()
        if f.normal.dot(box.M.to_3x3() @ Vector([sign if a == axis else 0 for a in range(3)])) < 0:
            f.normal_flip()
        # 09/11: pelo vértice, e não pela ordem: a face virada acima inverte a ordem dos cantos e embaralhava a textura (as "linhas puxadas")
        by_vert = {q[0]: q for q in quad}
        for loop in f.loops:
            _, du, dv = by_vert[loop.vert]
            # a linha 0 do tile é a do começo (v = 0); na imagem do Blender a linha de baixo é a 0
            loop[uvl].uv = ((tx + du * cols) / WIDTH, 1 - (ty + dv * rows) / HEIGHT)
# (sem recalc_face_normals: as faces já nascem viradas para fora)
if TUBE_RECT:
    # o tubo: a volta inteira sem vértice repetido. A textura é espelhada (0 = costas, 1 = frente, nos dois lados): por isso não há
    # emenda, que abriria um rasgo no pano simulado
    tx, ty, tc, tr = TUBE_RECT
    col_name = CLOTH_TUBE["color"]
    for r in range(tr):
        for c in range(tc):
            s_ = (c + .5) / tc / (1 if CLOTH_TUBE.get("sheet") else 2)
            p3 = tube_point(s_, (r + .5) / tr)
            d = p3 - Vector((CLOTH_TUBE["cx"], CLOTH_TUBE["cy"], p3.z))
            fname = CLOTH_TUBE["sheet"] if CLOTH_TUBE.get("sheet") else ("costas" if d.y > 0 else "frente") if abs(d.y) / CLOTH_TUBE["b"] > abs(d.x) / CLOTH_TUBE["a"] * .8 else "dir"
            got = armor_sample(p3, fname, -1) if ARMOR_REF else None
            rgb = got or C[col_name][2 if (c + r * 3) % 7 else 1]
            i = ((HEIGHT - 1 - (ty + r)) * WIDTH + tx + c) * 4
            px[i:i + 4] = [rgb[0] / 255, rgb[1] / 255, rgb[2] / 255, 1.0]
    sheet = CLOTH_TUBE.get("sheet")
    N, M = ((4, 5) if CLOTH_TUBE.get("small") else (12, 10)) if sheet else (32, 12)
    W_ = N + 1 if sheet else N
    ring = [[bm.verts.new(tube_point(i / N, j / M)) for i in range(W_)] for j in range(M + 1)]
    VPIECE.extend([-1] * ((M + 1) * W_))
    for j in range(M):
        for i in range(N):
            cell = [(i, j), (i, j + 1), ((i + 1) % W_, j + 1), ((i + 1) % W_, j)]
            f = bm.faces.new([ring[b][a] for a, b in cell])
            f.material_index = 2 if SET else 0
            f.normal_update()
            mid_ = f.calc_center_median()
            out_dir = Vector((0, -1 if sheet == "frente" else 1, 0)) if sheet else mid_ - Vector((CLOTH_TUBE["cx"], CLOTH_TUBE["cy"], mid_.z))
            if f.normal.dot(out_dir) < 0:
                f.normal_flip()
            for loop in f.loops:
                a, b = next((a, b) for a, b in cell if ring[b][a] == loop.vert)
                u_ = a / N if sheet else (1 - abs(1 - 2 * a / N))
                loop[uvl].uv = ((tx + u_ * tc) / WIDTH, 1 - (ty + b / M * tr) / HEIGHT)

TEX_NAME = "T_" + OUTFIT if ARMOR and not SET else "T_Roupa_" + OUTFIT
img = bpy.data.images.new(TEX_NAME, WIDTH, HEIGHT, alpha=True)
img.pixels[:] = px
img_normal = None
if (ARMOR and not PIECE) or (ELF_DETAIL and not SET) or not ARMOR:   # (inclui chapéus e acessórios; 10/11: e as roupas)
    # 09/11: o mapa de relevo por pixel, tirado da própria pintura (o escuro é fundo, o claro é alto), no padrão da Unreal
    import numpy as np
    a_ = np.array(px, dtype=np.float32).reshape(HEIGHT, WIDTH, 4)
    h_ = a_[..., :3].mean(axis=2)
    ok = a_[..., 3] > .5

    def grad(ax):
        up, down = np.roll(h_, -1, ax), np.roll(h_, 1, ax)
        valid = ok & np.roll(ok, -1, ax) & np.roll(ok, 1, ax)
        return np.where(valid, (up - down) / 2, 0.0)
    strength = 3.2
    nx, ny = -grad(1) * strength, grad(0) * strength
    nz = np.ones_like(nx)
    ln = np.sqrt(nx * nx + ny * ny + nz * nz)
    out_ = np.stack((nx / ln * .5 + .5, ny / ln * .5 + .5, nz / ln * .5 + .5, np.ones_like(nx)), axis=2)
    img_normal = bpy.data.images.new(TEX_NAME + "_normal", WIDTH, HEIGHT, alpha=True)
    img_normal.colorspace_settings.name = "Non-Color"
    img_normal.pixels[:] = out_.ravel().tolist()
img_mask = bpy.data.images.new(TEX_NAME + "_mask", WIDTH, HEIGHT, alpha=True)
img_mask.colorspace_settings.name = "Non-Color"
img_mask.pixels[:] = mask
# 08/11: cada peça do braço e da perna em anéis ao longo do comprimento, como o corpo. Uma caixa de 8 cantos não dobra: com os pesos do
# joelho nos cantos dela, a caixa inteira afinava em triângulo e a pele aparecia
_piece = bm.verts.layers.int.new("piece")
bm.verts.index_update()
for _v in bm.verts:
    _v[_piece] = VPIECE[_v.index]
_limbs = {pi: PIECES[pi][0] for pi in range(len(PIECES)) if PIECE_PART[pi].split("_")[0] in ("braco", "antebraco", "coxa", "canela")}
_cuts = {}
for _e in bm.edges:
    pi = _e.verts[0][_piece]
    box = _limbs.get(pi)
    if box is None:
        continue
    axis = box.M.to_3x3().col[box.axis].normalized()
    along = _e.verts[1].co - _e.verts[0].co
    if along.length > TEX and abs(along.normalized().dot(axis)) > .95:
        _cuts.setdefault(max(1, int(along.length / (1.6 * TEX))), []).append(_e)
for n, edges in _cuts.items():
    bmesh.ops.subdivide_edges(bm, edges=edges, cuts=n, use_grid_fill=True)
bm.verts.index_update()
bm.verts.ensure_lookup_table()
VPIECE = [_v[_piece] for _v in bm.verts]
bm.verts.layers.int.remove(_piece)

mesh = bpy.data.meshes.new("Roupa")
bm.to_mesh(mesh)
bm.free()
obj = bpy.data.objects.new("Roupa", mesh)
rig.users_collection[0].objects.link(obj)
for mname in (["M_Armadura"] if ARMOR and not SET else ["M_Roupa_" + r for r in REGIONS]):
    mat = bpy.data.materials.new(mname)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = img
    tex.interpolation = "Closest"
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = .9
    mesh.materials.append(mat)

for pb in rig.pose.bones:
    pb.matrix_basis.identity()
bpy.context.view_layer.update()

# ---- os pesos: os do vértice do corpo mais perto
tree = KDTree(len(VERTS))
for n, (p, _) in enumerate(VERTS):
    tree.insert(p, n)
tree.balance()
obj.parent = rig
vgs = {}
# o tronco só com os vértices do corpo da coluna e do quadril (02/11: sem a cabeça: a gola pegava o peso da cabeça e,
# quando ela virava, puxava a roupa do tronco para cima)
CORE = [n for n, (_, w) in enumerate(VERTS)
        if w and max(w, key=w.get)[len(P):] in ("Hips", "Spine", "Spine1", "Spine2")]
core_tree = KDTree(len(CORE))
for k, n in enumerate(CORE):
    core_tree.insert(VERTS[n][0], k)
core_tree.balance()
if WAIST:
    # 10/11: a curva só no que fica sobre o tronco (o vértice do corpo mais perto é da coluna ou do quadril): antes pegava
    # o lado de dentro dos braços e das mangas e entortava o ombro (o usuário: "a cintura/clavícula bugou")
    exec(open(os.path.join(ROOT, "Tools", "Blender", "female_waist.py"), encoding="utf-8").read())
    for v in mesh.vertices:
        _co, n, _d = tree.find(v.co)
        w = VERTS[n][1]
        if w and max(w, key=w.get)[len(P):] in ("Hips", "Spine", "Spine1", "Spine2"):
            shape_point(v.co, .3)
# 03/11 (o usuário: "cotovelos e joelhos quebrados; antes era um só e dobrava e torcia"): o corpo agora dobra macio no
# cotovelo, no pulso e no joelho (RigBlockCharacter.py: ARM_SMOOTH, LEG_SMOOTH). As peças do braço e da perna copiam o
# peso do vértice do corpo mais perto, só do mesmo membro (antes iam rígidas num osso e a manga abria no cotovelo)
CHAINS = {}
for side in ("Left", "Right"):
    for kind, bones in (("braco", ("Arm", "ForeArm", "Hand")), ("perna", ("UpLeg", "Leg"))):
        CHAINS[(side, kind)] = tuple(P + side + b for b in bones)
LIMB_OF = {bone: key for key, bones in CHAINS.items() for bone in bones}
LIMB_VERTS, LIMB_TREES = {}, {}
for key, bones in CHAINS.items():
    ids = [n for n, (_, w) in enumerate(VERTS) if w and max(w, key=w.get) in bones]
    if not ids:
        continue
    t = KDTree(len(ids))
    for k, n in enumerate(ids):
        t.insert(VERTS[n][0], k)
    t.balance()
    LIMB_VERTS[key], LIMB_TREES[key] = ids, t
dom_cache = {}
for v in mesh.vertices:
    pi = VPIECE[v.index]
    if pi not in dom_cache:
        dom_cache[pi] = (piece_dom(pi), rigid_piece(pi))
    dom, rigid = dom_cache[pi]
    # as peças dos membros vão inteiras no osso da parte (rígido como o corpo); as do tronco pegam os pesos do vértice
    # mais perto da coluna/quadril (a dobra da coluna; nunca do braço)
    if rigid and LIMB_OF.get(dom) in LIMB_TREES:
        key = LIMB_OF[dom]
        _, k, _ = LIMB_TREES[key].find(v.co)
        weights = {g: w for g, w in VERTS[LIMB_VERTS[key][k]][1].items() if g in CHAINS[key] or w > .05}
    elif rigid:
        weights = {dom: 1.0}
    else:
        _, k, _ = core_tree.find(v.co)
        weights = VERTS[CORE[k]][1]
        # 02/11: abaixo do quadril (aventais, tabardo, capas) a peça segue a média das duas coxas: balança com o passo
        # sem rasgar (cada lado numa coxa torcia o painel quando uma perna ia para a frente e a outra para trás). As
        # saias são outra coisa: duas metades, cada uma na sua coxa (skirt)
        drop = HIPS.lo.z - v.co.z
        if drop > 0:
            t = min(1.0, drop / (10 * TEX))
            blend = .6 * t * t * (3 - 2 * t)
            left = .5
            mixed = {g: w * (1 - blend) for g, w in weights.items()}
            for g, w in ((P + "LeftUpLeg", blend * left), (P + "RightUpLeg", blend * (1 - left))):
                if w > 1e-3:
                    mixed[g] = mixed.get(g, 0) + w
            weights = mixed
    if pi == -1:   # o pano: preso no osso dele; a saia, embaixo, também vai com a perna do lado (10/11: "duras, sem movimento com as pernas")
        weights = {P + CLOTH_TUBE.get("bone", "Hips"): 1.0}
        if not CLOTH_TUBE.get("sheet"):
            T_ = CLOTH_TUBE
            t = max(0.0, min(1.0, (T_["z_top"] - v.co.z) / (T_["z_top"] - T_["z_bot"])))
            left_x = (rig.matrix_world @ rig.data.bones[P + "LeftUpLeg"].head_local).x
            side = max(-1.0, min(1.0, (v.co.x - T_["cx"]) / T_["a"])) * (1 if left_x > T_["cx"] else -1)
            leg = .85 * t
            knee = (rig.matrix_world @ rig.data.bones[P + "LeftLeg"].head_local).z
            shin = max(0.0, min(1.0, (knee - v.co.z) / max(1e-4, knee - T_["z_bot"]))) * .7   # abaixo do joelho a canela também puxa
            left, right = leg * (.5 + .5 * side), leg * (.5 - .5 * side)
            weights = {P + "Hips": 1 - leg, P + "LeftUpLeg": left * (1 - shin), P + "LeftLeg": left * shin,
                       P + "RightUpLeg": right * (1 - shin), P + "RightLeg": right * shin}
    for g, w in weights.items():
        vg = vgs.get(g) or obj.vertex_groups.new(name=g)
        vgs[g] = vg
        vg.add([v.index], w, "REPLACE")
mod = obj.modifiers.new("Armature", "ARMATURE")
mod.object = rig
log("%d peças, %d faces, textura %dx%d" % (len(PIECES), len(mesh.polygons), WIDTH, HEIGHT))

# ---- 10/11: o busto redondo (o usuário: "peitos reais, que se movimentam com o andar, tamanho nas opções"). Uma malha lisa por lado,
# com 2 ossos de física Hair_busto<E|D>_NN (UVortexHairComponent). O corpo (P_busto_<tam>) e cada roupa feminina (<id>_busto_<tam>) têm os
# MESMOS ossos: a física dá o mesmo movimento aos dois, e a roupa vai junto. A cor de cada face vem da textura da roupa, no ponto do peito.
BUST_SIZES = {"p": (2.9, 2.8, 1.2), "m": (3.3, 3.1, 1.7), "g": (3.7, 3.4, 2.3)}   # meia largura, meia altura e quanto sai para a frente (texels)


def bust_tile_color(x, z):
    """A cor da roupa no peito: o texel da face da frente mais à frente que cobre (x, z)."""
    best = None
    for pi, (box, *_r) in enumerate(PIECES):
        if box.M != Matrix.Identity(4) or PIECE_PART[pi] not in ("torso", "busto") or not (box.lo.x <= x <= box.hi.x and box.lo.z <= z <= box.hi.z):
            continue
        if best is None or box.lo.y < PIECES[best][0].lo.y:
            best = pi
    if best is None:
        return None
    box = PIECES[best][0]
    for (pi, axis, sign, cols, rows, ua, va), (tx, ty) in zip(tiles, place):
        if pi == best and axis == 1 and sign < 0:
            c = min(cols - 1, int((x - box.lo[ua]) / max(1e-6, box.hi[ua] - box.lo[ua]) * cols))
            r = min(rows - 1, int((z - box.lo[va]) / max(1e-6, box.hi[va] - box.lo[va]) * rows))
            i = ((HEIGHT - 1 - (ty + r)) * WIDTH + tx + c) * 4
            return tuple(px[i + n] * 255 for n in range(3))
    return None


def make_busts(prefix, body):
    import math
    t = TORSO
    cx = (t.lo.x + t.hi.x) / 2
    zc = t.hi.z - 5.6 * TEX
    extra = 0 if body else .45
    for size, (rx, rz, rd) in BUST_SIZES.items():
        bm2 = bmesh.new()
        bmesh.ops.create_uvsphere(bm2, u_segments=12, v_segments=8, radius=1.0)
        uvb = bm2.loops.layers.uv.new("UV")
        sides = []
        for s_ in (-1, 1):
            pass
        faces_color = []
        # dois montes: copia a esfera para o outro lado
        geom = bm2.verts[:] + bm2.edges[:] + bm2.faces[:]
        dup = bmesh.ops.duplicate(bm2, geom=geom)
        halves = [(set(v for v in bm2.verts if v not in set(e for e in dup["geom"] if isinstance(e, bmesh.types.BMVert))), -1),
                  (set(e for e in dup["geom"] if isinstance(e, bmesh.types.BMVert)), 1)]
        side_of = {}
        for verts, s_ in halves:
            x0 = cx + s_ * 2.9 * TEX
            for v in verts:
                d = v.co.copy()
                # em cima desce suave do peito, embaixo é mais cheio (gota), não uma bola
                front = (rd + extra) * (1 - .5 * max(0.0, d.z)) * TEX
                v.co = Vector((x0 + d.x * (rx + extra) * TEX, t.lo.y + .25 * TEX + d.y * front if d.y < 0 else t.lo.y + .25 * TEX + d.y * .6 * TEX,
                               zc + d.z * (rz + extra) * TEX - (d.y < 0) * (-d.y) * .45 * TEX * (rd / 2.3)))
                side_of[v] = s_
        bm2.faces.ensure_lookup_table()
        W2 = 16
        img2 = bpy.data.images.new("T_busto", W2, W2, alpha=True)
        pix = [0.0] * (W2 * W2 * 4)
        light = Vector((0, -.45, .9)).normalized()
        for fi, f in enumerate(bm2.faces):
            f.normal_update()
            mid_ = f.calc_center_median()
            if body:
                base = C["top_f"][2]
            else:
                base = bust_tile_color(mid_.x, mid_.z) or bust_tile_color(cx, zc) or C["creme"][2]
            k = .86 if f.normal.z < -.55 else 1.0   # só a sombra de baixo; o resto a luz do jogo faz (sombreado liso)
            col = tuple(min(255, int(c * k) // 10 * 10 + 5) for c in base)
            cxp, cyp = fi % W2, fi // W2
            i = ((W2 - 1 - cyp) * W2 + cxp) * 4
            pix[i:i + 4] = [col[0] / 255, col[1] / 255, col[2] / 255, 1.0]
            for loop in f.loops:
                loop[uvb].uv = ((cxp + .5) / W2, 1 - (cyp + .5) / W2)
        img2.pixels[:] = pix
        mesh2 = bpy.data.meshes.new("Busto")
        bm2.to_mesh(mesh2)
        for poly in mesh2.polygons:
            poly.use_smooth = True
        vside = [side_of.get(v, 1) for v in bm2.verts]
        bm2.free()
        o2 = bpy.data.objects.new("Busto", mesh2)
        rig.users_collection[0].objects.link(o2)
        mat2 = bpy.data.materials.new("M_Busto")
        mat2.use_nodes = True
        nt2 = mat2.node_tree
        tex2 = nt2.nodes.new("ShaderNodeTexImage")
        tex2.image = img2
        tex2.interpolation = "Closest"
        nt2.links.new(tex2.outputs["Color"], next(n for n in nt2.nodes if n.type == "BSDF_PRINCIPLED").inputs["Base Color"])
        mesh2.materials.append(mat2)
        # os ossos: os mesmos no corpo e em todas as roupas (independem da folga da roupa)
        with bpy.context.temp_override(active_object=rig, object=rig, selected_objects=[rig], selected_editable_objects=[rig]):
            bpy.ops.object.mode_set(mode="EDIT")
            eb = rig.data.edit_bones
            inv = rig.matrix_world.inverted()
            for s_, tag in ((-1, "E"), (1, "D")):
                x0 = cx + s_ * 2.9 * TEX
                for n_ in (1, 2):
                    name = "Hair_busto%s_%02d" % (tag, n_)
                    b = eb.get(name) or eb.new(name)
                    b.head = inv @ Vector((x0, t.lo.y + (.2 - (n_ - 1) * rd * .55) * TEX, zc))
                    b.tail = inv @ Vector((x0, t.lo.y - n_ * rd * .55 * TEX, zc - n_ * .25 * TEX))
                    b.parent = eb[P + "Spine2"] if n_ == 1 else eb["Hair_busto%s_01" % tag]
                    b.use_connect = n_ == 2
                    b.use_deform = True
            bpy.ops.object.mode_set(mode="OBJECT")
        o2.parent = rig
        groups2 = {}
        for v in mesh2.vertices:
            tag = "E" if vside[v.index] < 0 else "D"
            depth = max(0.0, min(1.0, (t.lo.y - v.co.y) / ((rd + extra) * TEX)))
            ws = {P + "Spine2": max(0.0, 1 - depth * 2.2), "Hair_busto%s_01" % tag: min(1.0, depth * 2.2) * (1 - depth), "Hair_busto%s_02" % tag: min(1.0, depth * 2.2) * depth}
            tot = sum(ws.values())
            for g, w in ws.items():
                if w > 1e-3:
                    vg = groups2.get(g) or o2.vertex_groups.new(name=g)
                    groups2[g] = vg
                    vg.add([v.index], w / tot, "REPLACE")
        mod2 = o2.modifiers.new("Armature", "ARMATURE")
        mod2.object = rig
        if GAME:
            name = prefix + "_busto_" + size
            img2.filepath_raw = os.path.join(OUT, ("T_%s.png" if ARMOR and not SET else "T_%s.png") % name)
            img2.file_format = "PNG"
            img2.save()
            for x in list(bpy.context.view_layer.objects):
                x.select_set(x in (rig, o2))
            bpy.context.view_layer.objects.active = rig
            bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, name + ".glb"), use_selection=True, export_format="GLB", export_animations=False,
                                      export_skins=True, export_apply=False, export_yup=True, export_def_bones=False)
            bpy.data.objects.remove(o2, do_unlink=True)
        log("busto", prefix, size)


if BUSTY and False:   # 10/11: sem busto (o usuário: "deixe sem peito")
    make_busts(("P_busto" if OUTFIT == "P_busto" else OUTFIT) if (ARMOR and not SET) else "Roupa_" + OUTFIT, OUTFIT == "P_busto")

if GAME:
    # sobras do glb do herói (a Icosphere ia junto na exportação)
    for o in list(bpy.data.objects):
        if o.type == "MESH" and o not in (body, obj):
            bpy.data.objects.remove(o, do_unlink=True)
    os.makedirs(OUT, exist_ok=True)
    img.filepath_raw = os.path.join(OUT, TEX_NAME + ".png")
    img.file_format = "PNG"
    img.save()
    if img_normal:
        img_normal.filepath_raw = os.path.join(OUT, TEX_NAME + "_normal.png")
        img_normal.file_format = "PNG"
        img_normal.save()
    if True:   # 10/11: as roupas também (o metal e o brilho no M_ArmorBlock)
        img_mask.filepath_raw = os.path.join(OUT, TEX_NAME + "_mask.png")
        img_mask.file_format = "PNG"
        img_mask.save()
    body.hide_set(True)
    PREFIX = "%s" if ARMOR and not SET else "Roupa_%s"
    if CLOTH_TUBE:
        # 09/11: o pano numa malha à parte (<id>_tecido), a única que usa a física; a textura é a mesma
        import shutil
        obj2 = obj.copy()
        obj2.data = mesh.copy()
        rig.users_collection[0].objects.link(obj2)

        def keep(o, want):
            b2 = bmesh.new()
            b2.from_mesh(o.data)
            b2.verts.ensure_lookup_table()
            bmesh.ops.delete(b2, geom=[b2.verts[i] for i in range(len(VPIECE)) if (VPIECE[i] == -1) != want], context="VERTS")
            b2.to_mesh(o.data)
            b2.free()
        keep(obj, False)
        keep(obj2, True)
        shutil.copy(img.filepath_raw, os.path.join(OUT, TEX_NAME + "_tecido.png"))
        if not SET:
            shutil.copy(img_mask.filepath_raw, os.path.join(OUT, TEX_NAME + "_tecido_mask.png"))

        def export(o, file):
            for x in list(bpy.context.view_layer.objects):
                x.select_set(x in (rig, o))
            bpy.context.view_layer.objects.active = rig
            bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, file), use_selection=True, export_format="GLB", export_animations=False,
                                      export_skins=True, export_apply=False, export_yup=True, export_def_bones=False)
        export(obj, PREFIX % OUTFIT + ".glb")
        export(obj2, PREFIX % OUTFIT + "_tecido.glb")
        log("exportado com tecido", OUTFIT)
        raise SystemExit(0)
    for o in list(bpy.context.view_layer.objects):
        o.select_set(o in (rig, obj))
    bpy.context.view_layer.objects.active = rig
    path = os.path.join(OUT, PREFIX % OUTFIT + ".glb")
    bpy.ops.export_scene.gltf(filepath=path, use_selection=True, export_format="GLB", export_animations=False,
                              export_skins=True, export_apply=False, export_yup=True, export_def_bones=False)
    log("exportado para o jogo", path)
