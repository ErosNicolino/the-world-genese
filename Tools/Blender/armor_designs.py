"""Os desenhos das 21 armaduras (07/11, das pranchas `modelando ainda/Futuras modelagens/Armaduras`), peça por peça.

Rodado de dentro do Tools/Blender/MakeBlockOutfit.py (exec, com as funções dele: shell, block, os enfeites, PARTS, TEX),
pelo id "A_<conjunto>_<peça>": ARMOR_BUILD monta só aquela peça. As cores são as do dicionário C de lá; "*" na frente
= brilha; "cabelo" = a franja (a cor do cabelo do herói).

Os capacetes: o nosso rosto tem só os olhos grandes (sem boca), nas linhas 7 a 12 da grade 16 x 15 da frente da cabeça.
Os fechados têm uma janela larga nessa altura (os olhos inteiros à vista) e o queixo fechado; os capuzes e chapéus
deixam o rosto aberto com a franja por baixo. Todo capacete esconde o cabelo do herói (AVortexCharacter).
"""
import random as _random

from mathutils import Matrix as _M, Vector as _V

HB = PARTS["cabeca"]
HW, HH, HD = 16.0, (HB.hi.z - HB.lo.z) / TEX, (HB.hi.y - HB.lo.y) / TEX


PART_INFO = {}
HELM = {}   # 09/11: o último elmo montado (o relevo por material vai por cima dele)
# o material de cada conjunto: o que a textura imita e o relevo do elmo
MATERIAL = {"viajante": "couro", "recruta": "metal", "cacador": "couro", "malha_ferro": "metal", "batedor": "couro", "bronze": "metal", "guarda_real": "metal",
            "geada": "cristal", "mare": "escama", "noite_violeta": "metal", "escorpiao": "quitina", "dragao_rubro": "escama", "grifo_dourado": "metal", "magma": "rocha",
            "bosque_ancestral": "madeira", "espelho": "metal", "sem_sombra": "metal", "andarilho": "couro", "aurora": "metal", "arvore_mundo": "madeira", "estrelas": "metal"}


def hbox(c0, c1, r0, r1, d0, d1, color, decos=(), name="elmo"):
    """Uma caixa na grade da cabeça: colunas da esquerda da frente (0 a 16; fora da cabeça < 0 ou > 16), linhas de cima
    (0 a 15; acima da cabeça < 0) e a profundidade a partir da frente (< 0 = na frente do rosto; HD = a nuca)."""
    x0 = (HB.lo.x + HB.hi.x) / 2 - 8 * TEX
    lo = _V((x0 + min(c0, c1) * TEX, HB.lo.y + min(d0, d1) * TEX, HB.hi.z - max(r0, r1) * TEX))
    hi = _V((x0 + max(c0, c1) * TEX, HB.lo.y + max(d0, d1) * TEX, HB.hi.z - min(r0, r1) * TEX))
    PIECES.append((Box(_M.Identity(4), lo, hi, 2, True), 0, color, list(decos), (0, 0, 0), name))
    PIECE_PART.append("cabeca")


def mirror(boxes):
    """As caixas de um lado e as do outro (colunas espelhadas)."""
    out = []
    for c0, c1, r0, r1, d0, d1 in boxes:
        out.append((c0, c1, r0, r1, d0, d1))
        out.append((HW - c1, HW - c0, r0, r1, d0, d1))
    return out


def wblock(part, center, size, color, decos=(), name="enfeite"):
    """Uma caixa solta nos eixos do mundo (tamanho em texels), presa à parte do corpo dada."""
    h = _V(size) * TEX / 2
    PIECES.append((Box(_M.Identity(4), center - h, center + h, 2, True), 0, color, list(decos), (0, 0, 0), name))
    PIECE_PART.append(part)


def corners(part, a=0.0, b=1.0):
    box = PARTS[part].cut(a, b)
    return [box.M @ _V((x, y, z)) for x in (box.lo.x, box.hi.x) for y in (box.lo.y, box.hi.y) for z in (box.lo.z, box.hi.z)]


def mid(points):
    return sum(points, _V()) / len(points)


# ---- os padrões pintados (enfeites a mais: f(face, u, v, p) -> (cor, tom))

def scales(color, tone=0):
    """Escamas: a borda de baixo de cada escama mais escura, em fileiras desencontradas."""
    def f(face, u, v, p):
        if face in ("cima", "baixo"):
            return None
        x, y = p["texel"]
        if y % 3 == 2 and (x + 2 * ((y // 3) % 2)) % 4 != 1:
            return color, tone
        if y % 3 == 1 and (x + 2 * ((y // 3) % 2)) % 4 in (0, 2):
            return color, tone + 1
    return f


def chainmail(color):
    def f(face, u, v, p):
        if face in ("cima", "baixo"):
            return None
        x, y = p["texel"]
        return color, 3 if (x + y) % 2 else 1
    return f


def runes(color, density=.45, faces=("frente", "costas", "esq", "dir")):
    """Runas: um glifo de 3 x 4 texels sorteado em parte das casas de 5 x 6."""
    def f(face, u, v, p):
        if face not in faces:
            return None
        x, y = p["texel"]
        cx, cy = x // 5, y // 6
        rnd = _random.Random(cx * 7919 + cy * 104729 + len(face) * 31 + p["rows"])
        if rnd.random() > density:
            return None
        lx, ly = x % 5 - 1, y % 6 - 1
        if not (0 <= lx < 3 and 0 <= ly < 4):
            return None
        bits = rnd.getrandbits(12)
        if (bits >> (ly * 3 + lx)) & 1 or lx == 1:
            return color, 2
    return f


def cracks(color, faces=None):
    """Rachaduras acesas (a lava): duas famílias de linhas tortas que se cruzam."""
    def f(face, u, v, p):
        if faces is not None and face not in faces:
            return None
        x, y = p["texel"]
        wob = (y * 7 + x // 3) % 3 - 1
        if (x * 7 + y * 3 + wob) % 23 == 0 or (x * 5 - y * 7 + 100 * 29) % 29 == 0:
            return color, 2
    return f


def plates(color, period=6, tone=0):
    """As placas: uma linha escura a cada period texels."""
    return stripes(color, period, 1, tone)


def rivets(color, period=6, tone=3):
    return dots(color, period, tone)


def bark(color):
    def f(face, u, v, p):
        if face in ("cima", "baixo"):
            return None
        x, y = p["texel"]
        if (x + (y // 4) % 2) % 3 == 0:
            return color, 0 if (y // 2) % 2 else 1
    return f


def leaves(color, every=7):
    return sparkle(color, every)


def stars(color="*dourado", every=9):
    """Constelações: estrelinhas acesas espalhadas."""
    def f(face, u, v, p):
        x, y = p["texel"]
        h = _random.Random(x * 3301 + y * 9157 + len(face) * 17 + p["rows"]).random()
        if h < 1 / every:
            return color, 3
    return f


def patches(color="remendo"):
    """Remendos coloridos (o andarilho): quadrados de 4 x 4 em algumas casas."""
    def f(face, u, v, p):
        if face in ("cima", "baixo"):
            return None
        x, y = p["texel"]
        cell = _random.Random((x // 6) * 7919 + (y // 6) * 104729 + len(face) * 31).random()
        if cell < .3 and 1 <= x % 6 <= 4 and 1 <= y % 6 <= 4:
            return color, int(cell * 13) % 4
    return f


def outline(color, tone=None):
    """O contorno de 1 texel em volta da face (o brilho ciano do Sem Sombra)."""
    def f(face, u, v, p):
        x, y = p["texel"]
        if face in ("cima", "baixo"):
            return None
        if y == 0 or y == p["rows"] - 1 or u < .06 or u > .94:
            return color, tone
    return f


# ---- os capacetes

def helm_closed(main, trim=None, decos=(), o=1.0, window=(6.4, 12.7), nasal=False, brow=(), chin=(), visor_trim=True):
    """Fechado: topo, nuca e lados inteiros; na frente, a testa e o queixo, com a janela dos olhos no meio."""
    trim = trim or main
    HELM.update(kind="closed", main=main, trim=trim, o=o, top=window[0], low=window[1])
    hbox(-o, HW + o, -o, 0, -o, HD + o, main, decos, "elmo_topo")
    hbox(-o, HW + o, 0, HH, HD, HD + o, main, decos, "elmo_nuca")
    for c0, c1 in ((-o, 0), (HW, HW + o)):
        hbox(c0, c1, 0, HH, -o, HD, main, decos, "elmo_lado")
    hbox(0, HW, 0, window[0], -o, 0, main, list(brow) + list(decos), "elmo_testa")
    hbox(0, HW, window[1], HH, -o, 0, main, list(chin) + list(decos), "elmo_queixo")
    if visor_trim:
        # a borda da janela (um friso por fora)
        hbox(-o * .3, HW + o * .3, window[0] - .8, window[0], -o - .4, -o, trim, [], "elmo_friso")
        hbox(-o * .3, HW + o * .3, window[1], window[1] + .8, -o - .4, -o, trim, [], "elmo_friso")
    if nasal:
        hbox(6.6, 9.4, window[0], window[1] - 1.5, -o - .5, -o + .2, trim, [], "elmo_nasal")
    # 07/11 (o usuário: "cadê as camadas"): placas por cima do elmo, em degraus
    hbox(-o - .5, HW + o + .5, window[0] - 3.2, window[0] - 1.0, -o - .7, HD + o + .5, trim, [], "elmo_faixa")
    for c0, c1 in ((-o - .7, 1.0), (HW - 1.0, HW + o + .7)):
        hbox(c0, c1, window[0] - .4, HH - .5, -o - .7, 4.5, main, [], "elmo_bochecha")
    hbox(5.5, 10.5, -o - 1.0, -o, -o - .3, HD + o + .3, trim, [], "elmo_crista")
    hbox(-o - .3, HW + o + .3, HH - 3.5, HH + 1.2, HD + o, HD + o + 1.0, main, [], "elmo_nuca_guarda")


def helm_hood(main, fringe=True, decos=(), o=1.0, brow_rows=3.4, edge=None):
    """Capuz: cobre o topo, a nuca e os lados; o rosto fica aberto, com a franja (cor do cabelo) por baixo da borda."""
    HELM.update(kind="hood", main=main, trim=edge or main, o=o, top=brow_rows, low=HH)
    hbox(-o, HW + o, -o, 0, -o, HD + o, main, decos, "capuz_topo")
    hbox(-o, HW + o, 0, HH + .5, HD, HD + o, main, decos, "capuz_nuca")
    for c0, c1 in ((-o, 0), (HW, HW + o)):
        hbox(c0, c1, 0, HH + .5, -o, HD, main, decos, "capuz_lado")
    hbox(0, HW, 0, brow_rows, -o, 0, main, list(decos), "capuz_testa")
    if edge:
        hbox(-o - .2, HW + o + .2, brow_rows - .9, brow_rows, -o - .5, -o, edge, [], "capuz_borda")
    if fringe:
        hbox(.6, HW - .6, brow_rows, brow_rows + 2.2, -.35, 0, "cabelo", [jag(1)], "franja")
    # as camadas: a dobra do capuz em volta do rosto e o pano caindo atrás
    hbox(-o - .6, 1.2, 0, HH + .5, -o - .6, 2.5, main, list(decos), "capuz_dobra")
    hbox(HW - 1.2, HW + o + .6, 0, HH + .5, -o - .6, 2.5, main, list(decos), "capuz_dobra")
    hbox(-o - .4, HW + o + .4, -o - .6, .6, -o - .6, HD + o + .6, main, list(decos), "capuz_dobra_topo")
    hbox(-o + .5, HW + o - .5, HH - 2, HH + 3, HD + o, HD + o + 1.2, main, list(decos), "capuz_pano")


def helm_brim(dome, brim, fringe=True, o=1.0, brim_out=2.6, dome_decos=()):
    """Chapéu de ferro de aba larga (o recruta): a cúpula, a aba em volta e o cabelo aparecendo por baixo."""
    HELM.update(kind="brim", main=dome, trim=brim, o=o, top=2.2, low=HH)
    hbox(-o, HW + o, -o - 1.2, 2.2, -o, HD + o, dome, dome_decos, "chapeu_cupula")
    hbox(-o - brim_out, HW + o + brim_out, 2.2, 3.0, -o - brim_out, HD + o + brim_out, brim, [], "chapeu_aba")
    if fringe:
        hbox(.4, HW - .4, 3.0, 5.2, -.35, 0, "cabelo", [jag(1)], "franja")
        hbox(-.35, 0, 3.0, 8, 0, HD, "cabelo", [jag(2)], "franja_lado")
        hbox(HW, HW + .35, 3.0, 8, 0, HD, "cabelo", [jag(2)], "franja_lado")
        hbox(0, HW, 3.0, 9, HD, HD + .35, "cabelo", [jag(2)], "franja_nuca")


def horns(color, boxes=None, d=(5, 9)):
    boxes = boxes or [(-2.5, -.5, 2, 4.5), (-4.5, -2.5, .5, 3.5), (-5.8, -3.8, -2.2, .8), (-6, -4.6, -5, -2)]
    for c0, c1, r0, r1, d0, d1 in mirror([(c0, c1, r0, r1, d[0], d[1]) for c0, c1, r0, r1 in boxes]):
        hbox(c0, c1, r0, r1, d0, d1, color, [], "chifre")


def wings(color, decos=(), d=(8, 10.5)):
    feathers = [(-2.6, -.8, 1, 8.5), (-4.6, -2.6, -.5, 6.5), (-6.4, -4.6, -2, 4.5), (-7.8, -6.4, -3.5, 2)]
    for c0, c1, r0, r1, d0, d1 in mirror([(c0, c1, r0, r1, d[0], d[1]) for c0, c1, r0, r1 in feathers]):
        hbox(c0, c1, r0, r1, d0, d1, color, list(decos) + [hem(color, 1, 0)], "asa")


def plume(color, o=1.0):
    hbox(6.6, 9.4, -o - 5, -o, 0, HD + 1, color, [stripes(color, 2, 1, 1)], "penacho")
    hbox(6.6, 9.4, -o - 3, 8, HD + o, HD + o + 2.6, color, [stripes(color, 2, 1, 1)], "penacho_cauda")


def gem(color, row=3.2, o=1.0, size=2.4):
    hbox(8 - size / 2, 8 + size / 2, row, row + size, -o - .7, -o, color, [], "gema")


def spikes_ridge(color, o=1.0, n=4):
    for k in range(n):
        d0 = 1.5 + k * (HD - 3) / n
        hbox(7, 9, -o - 2.2 + (k % 2) * .6, -o, d0, d0 + 2, color, [], "crista")


def antlers(color):
    branches = [(1.5, 3.5, -3.5, -1, 6.5, 8.5), (-.5, 1.5, -6.5, -3.5, 6.5, 8.5), (-2.5, -.5, -8.5, -6, 6.5, 8.5),
                (3, 4.5, -7.5, -3.5, 6.5, 8.5), (-3.5, -2, -4.5, -2.5, 6.5, 8.5)]
    for b in mirror(branches):
        hbox(*b, color, [], "galho")


def scorpion_tail(color, sting="*veneno", o=1.0):
    for c0, c1, r0, r1, d0, d1 in ((6.4, 9.6, -o - 2.6, -o, 10, HD + 1.5), (6.4, 9.6, -o - 5.2, -o - 2.4, 7, 12), (6.6, 9.4, -o - 7, -o - 5, 3, 9),
                                   (6.8, 9.2, -o - 6.4, -o - 4, .5, 4)):
        hbox(c0, c1, r0, r1, d0, d1, color, [plates(color, 2, 0)], "rabo")
    hbox(7.2, 8.8, -o - 4, -o - 1.8, -1.5, 1.2, sting, [], "ferrao")


def dragon_crown(color, o=1.0):
    back = [(1, 3, -2.5, -.5, 9, 12), (0, 2, -4.5, -2.5, 12, 15), (-.5, 1.2, -6, -4.2, 15, 18)]
    for b in mirror(back):
        hbox(*b, color, [], "chifre_dragao")
    hbox(1, HW - 1, 5, 6.4, -o - 2.2, -o, color, [plates(color, 2, 0)], "focinho")
    spikes_ridge(color, o, 5)


def star_crown(color="*dourado", o=1.0):
    hbox(7, 9, -o - 6, -o, 5.5, 8.5, color, [], "estrela")
    hbox(4.5, 11.5, -o - 4, -o - 2.6, 5.5, 8.5, color, [], "estrela")
    for b in mirror([(2, 3.6, -o - 3.2, -o, 6, 8), (-1.4, .4, 1, 3, 6, 8)]):
        hbox(*b, color, [], "estrela_ponta")


# ---- o peitoral (o tronco e os braços), as calças e as botas

def chest(main, decos=(), o=1.0, sleeves=1.0, sleeve_color=None, sleeve_decos=None, pauldron=None, pauldron_decos=(),
          collar=None, collar_decos=(), bracers=None, bracer_decos=(), gem_color=None, belt=None, step=.7):
    """O peitoral em camadas (07/11): o gibão por baixo, a placa do peito por cima (step texels mais alta), a faixa da
    cintura, a ombreira em 3 lâminas (cada uma um degrau), a gola e a braçadeira."""
    PART_INFO.update(main=pauldron or main, trim=collar or belt or main)
    shell("torso", off=o, color=main, decos=list(decos))
    shell("torso", .07, .74, off=o + step, color=main, decos=list(decos), name="peito_placa")
    shell("torso", .76, .9, off=o + step * .6, color=main, decos=list(decos), name="peito_faixa")
    shell("braco", 0, sleeves, off=o, color=sleeve_color or main, decos=list(sleeve_decos if sleeve_decos is not None else decos) + [hem(sleeve_color or main, 1, 0)], name="manga")
    if pauldron:
        for k, (a, b, extra, g) in enumerate(((0, .24, 2.4, .9), (.2, .4, 1.8, .6), (.36, .54, 1.2, .35))):
            shell("braco", a, b, off=o + extra, grow=(g, g, 0), color=pauldron, decos=list(pauldron_decos) + [hem(pauldron, 1, 0)], name="ombreira")
    shell("torso", 0, .1, off=o + step + .5, color=collar or main, decos=list(collar_decos) + ([jag(1)] if collar else []), name="gola")
    if bracers or pauldron:
        shell("antebraco", .3, .95, off=.9, color=bracers or pauldron, decos=list(bracer_decos) + [band(0, .14, bracers or pauldron, 3)], name="bracadeira")
        shell("antebraco", .3, .42, off=1.5, color=bracers or pauldron, decos=[], name="bracadeira_punho")
    if belt:
        shell("torso", .88, 1, off=o + .5, color=belt, decos=[rect(.42, .58, .05, .95, "ouro", hollow=(belt, 0))], name="cinto")
    if gem_color:
        t = PARTS["torso"]
        c = _V(((t.lo.x + t.hi.x) / 2, t.lo.y - (o + .55) * TEX, t.hi.z - 7 * TEX))
        wblock("torso", c, (3.2, 1.1, 3.2), gem_color, [], "gema_peito")


def shoulder_blocks(color, kind="spike", o=1.0, decos=()):
    """Enfeites em cima das ombreiras: espinhos, asas, cristais, galhos, pontas de estrela."""
    for side in ("E", "D"):
        pts = corners("braco_" + side, 0, .45)
        c = mid(pts)
        top = max(p.z for p in pts)
        out = 1 if c.x > 0 else -1
        if kind == "spike":
            wblock("braco_" + side, _V((c.x, c.y, top + (o + 2.6) * TEX)), (1.8, 1.8, 3.4), color, decos, "espinho")
            wblock("braco_" + side, _V((c.x + out * 3 * TEX, c.y, top + (o + 1.6) * TEX)), (1.6, 1.6, 2.4), color, decos, "espinho")
        elif kind == "wing":
            for k, (dx, dz, h) in enumerate(((1, 3, 6), (3, 5, 6), (5, 7, 5), (7, 8.5, 4))):
                wblock("braco_" + side, _V((c.x + out * dx * TEX, c.y + 2.5 * TEX, top + dz * TEX)), (2, 1.2, h), color,
                       list(decos) + [hem(color, 1, 0)], "asa_ombro")
        elif kind == "crystal":
            for dx, dz, h in ((0, 3, 4), (2.5, 2.2, 3), (-1.8, 2, 2.6)):
                wblock("braco_" + side, _V((c.x + out * dx * TEX, c.y, top + (o + dz) * TEX)), (1.8, 1.8, h), color, decos, "cristal")
        elif kind == "branch":
            wblock("braco_" + side, _V((c.x, c.y + 1 * TEX, top + (o + 3.5) * TEX)), (1.4, 1.4, 5), color, decos, "galho")
            wblock("braco_" + side, _V((c.x + out * 2 * TEX, c.y + 1 * TEX, top + (o + 5.5) * TEX)), (3, 1.4, 1.4), color, decos, "galho")
        elif kind == "star":
            wblock("braco_" + side, _V((c.x, c.y, top + (o + 3) * TEX)), (1.6, 1.6, 4.2), color, decos, "estrela_ombro")
            wblock("braco_" + side, _V((c.x, c.y, top + (o + 3) * TEX)), (4.2, 1.6, 1.6), color, decos, "estrela_ombro")
        elif kind == "shell":
            wblock("braco_" + side, _V((c.x, c.y, top + (o + 1.6) * TEX)), (5.5, 4, 2.2), color, decos, "concha")
        elif kind == "fur":
            wblock("braco_" + side, _V((c.x, c.y, top + (o + 1.2) * TEX)), (6, 6, 2.4), color, list(decos) + [jag(1)], "pelo_ombro")


def legs(main, decos=(), o=.8, belt=None, belt_decos=(), tassets=None, tasset_decos=(), knee=None, knee_decos=(), step=.6):
    """As calças em camadas (07/11): a calça, o coxote por cima (step mais alto), a caneleira, o saiote em 2 lâminas."""
    PART_INFO.update(main=knee or main, trim=belt or tassets or main)
    shell("quadril", off=o, color=main, decos=list(decos))
    shell("coxa", off=o, color=main, decos=list(decos))
    shell("canela", 0, .62, off=o - .1, color=main, decos=list(decos))
    shell("coxa", .08, .78, off=o + step, color=main, decos=list(decos), name="coxote")
    shell("canela", .08, .5, off=o + step * .7, color=main, decos=list(decos), name="caneleira")
    if belt:
        shell("quadril", 0, .3, off=o + .6, color=belt, decos=list(belt_decos) or [rect(.42, .58, .05, .95, "ouro", hollow=(belt, 0))], name="cinto")
    if tassets:
        shell("quadril", .25, 1, off=o + 1.0, color=tassets, decos=list(tasset_decos) + [hem(tassets, 1, 0)], name="saiote")
        shell("coxa", 0, .3, off=o + step + 1.0, color=tassets, decos=list(tasset_decos) + [hem(tassets, 1, 0)], name="saiote")
        shell("coxa", .24, .5, off=o + step + .6, color=tassets, decos=list(tasset_decos) + [hem(tassets, 1, 0)], name="saiote2")
    if knee:
        for side in ("E", "D"):
            pts = corners("canela_" + side, 0, .2)
            c = mid(pts)
            front = min(p.y for p in pts)
            wblock("canela_" + side, _V((c.x, front - (o + .7) * TEX, c.z)), (5.6, 1.6, 3.6), knee, knee_decos, "joelheira")


def boots(main, decos=(), o=1.0, cuff=None, cuff_decos=(), toe=None, claws=None, wings_color=None, top=.45):
    shell("canela", top, 1, off=o, color=main, decos=list(decos) + [band(0, .14, main, 3)], name="bota")
    shell("pe", off=.6, color=main, decos=list(decos) + [hem(toe or main, 1, 0)], name="bota_pe")
    shell("pe", .8, 1, off=1.0, color="couro_esc", decos=[], name="bota_sola")
    shell("canela", top + .2, top + .3, off=o + .5, color=main, decos=list(decos), name="bota_anel")
    if toe:
        for side in ("E", "D"):
            p = PARTS["pe_" + side]
            wblock("pe_" + side, _V(((p.lo.x + p.hi.x) / 2, p.lo.y - .9 * TEX, p.lo.z + 2 * TEX)), (8.2, 1.2, 4), toe, [], "bota_bico")
    if cuff:
        shell("canela", top, top + .17, off=o + .8, color=cuff, decos=list(cuff_decos) + [jag(1)], name="bota_cano")
    if claws:
        for side in ("E", "D"):
            p = PARTS["pe_" + side]
            for k in (.2, .5, .8):
                x = p.lo.x + (p.hi.x - p.lo.x) * k
                wblock("pe_" + side, _V((x, p.lo.y - 1.4 * TEX, p.lo.z + .9 * TEX)), (1.4, 2.6, 1.6), claws, [], "garra")
    if wings_color:
        for side in ("E", "D"):
            pts = corners("canela_" + side, .45, .7)
            c = mid(pts)
            out = 1 if c.x > 0 else -1
            for dx, dz, h in ((1.5, 1, 3.5), (3, 2, 3)):
                wblock("canela_" + side, _V((c.x + out * (4 + dx) * TEX, c.y + 1.5 * TEX, c.z + dz * TEX)), (1.6, 1, h), wings_color,
                       [], "asa_bota")


# ---- os 21 conjuntos

def viajante(slot):
    if slot == "head":
        helm_hood("couro", decos=[stripes("couro", 5, 1, 1)], edge="creme")
    elif slot == "chest":
        chest("couro", decos=[vneck(.22, .1, "creme"), lace("couro_esc", .5, .3, .8)], sleeve_color="creme", sleeves=.5,
              pauldron="couro", pauldron_decos=[plates("couro_esc", 4, 1)], belt="creme")
    elif slot == "legs":
        legs("creme", decos=[stripes("creme", 6, 1, 1)], belt="couro", tassets="couro")
    else:
        boots("couro", cuff="couro_esc")


def recruta(slot):
    if slot == "head":
        helm_brim("ferro", "ferro", dome_decos=[plates("ferro", 3, 0)])
    elif slot == "chest":
        chest("creme", decos=[quilt("creme")], sleeves=.75, sleeve_decos=[quilt("creme")], collar="creme")
    elif slot == "legs":
        legs("creme", decos=[quilt("creme")])
    else:
        boots("ferro", decos=[stripes("creme", 3, 1, 1)], toe="ferro", cuff="creme")


def cacador(slot):
    if slot == "head":
        helm_hood("couro_esc", edge="pelo", decos=[stripes("couro_esc", 4, 1, 1)])
        hbox(-1.6, HW + 1.6, 1.6, 3.6, -1.6, HD + 1.6, "pelo", [jag(1)], "pelo_borda")
    elif slot == "chest":
        chest("couro_esc", decos=[plates("couro", 5, 1), lace("couro", .5, .2, .7)], collar="pelo", pauldron="couro_esc")
        shoulder_blocks("pelo", "fur")
    elif slot == "legs":
        legs("couro_esc", decos=[stripes("couro", 5, 1, 1)], belt="couro", knee="pelo", knee_decos=[jag(1)])
    else:
        boots("couro_esc", cuff="pelo")


def malha_ferro(slot):
    if slot == "head":
        helm_closed("ferro", "couro_esc", decos=[plates("ferro", 4, 0)], nasal=True, chin=[chainmail("metal")])
    elif slot == "chest":
        chest("metal", decos=[chainmail("metal"), lace("couro", .5, .15, .85)], sleeves=1.0, sleeve_decos=[chainmail("metal")],
              pauldron="ferro", pauldron_decos=[rivets("couro", 4, 3)], belt="couro_esc")
    elif slot == "legs":
        legs("metal", decos=[chainmail("metal")], belt="couro_esc", knee="ferro")
    else:
        boots("ferro", decos=[plates("ferro", 4, 0)], toe="ferro", cuff="couro")


def batedor(slot):
    if slot == "head":
        helm_hood("verde", decos=[rivets("dourado", 4, 2)], edge="couro_esc")
    elif slot == "chest":
        chest("verde", decos=[rivets("dourado", 4, 2), vline(.47, .53, "verde", tone=0)], sleeve_color="verde_folha", sleeves=.5,
              pauldron="couro", pauldron_decos=[rivets("dourado", 3, 3)], belt="couro_esc")
    elif slot == "legs":
        legs("verde", decos=[rivets("dourado", 5, 2)], belt="couro", knee="couro")
    else:
        boots("couro_esc", decos=[band(.3, .4, "verde", 2)], cuff="verde")


def bronze(slot):
    if slot == "head":
        helm_closed("bronze", "bronze", decos=[scales("bronze")], window=(6.0, 12.9))
    elif slot == "chest":
        chest("bronze", decos=[scales("bronze")], pauldron="bronze", pauldron_decos=[plates("bronze", 3, 0)], sleeves=1.0, belt="couro")
    elif slot == "legs":
        legs("couro_esc", decos=[], belt="couro", tassets="bronze", tasset_decos=[scales("bronze")])
    else:
        boots("bronze", decos=[scales("bronze")], toe="bronze")


def guarda_real(slot):
    if slot == "head":
        helm_closed("prata", "azul_real", decos=[plates("prata", 5, 1)])
        plume("azul_real")
    elif slot == "chest":
        chest("prata", decos=[plates("prata", 6, 1)], pauldron="prata", pauldron_decos=[band(.5, .62, "azul_real", 2)],
              gem_color="*ciano")
        panel(2, 26, 9, "azul_real", decos=[hem("dourado", 1, 2), vline(0, .08, "dourado", tone=2), vline(.92, 1, "dourado", tone=2)], gap=2.2, name="tabardo")
    elif slot == "legs":
        legs("prata", decos=[plates("prata", 5, 1)], belt="couro_esc", tassets="azul_real", tasset_decos=[hem("dourado", 1, 2)], knee="prata")
    else:
        boots("prata", decos=[band(.5, .6, "azul_real", 2)], toe="prata")


def geada(slot):
    if slot == "head":
        helm_closed("gelo", "branco", decos=[sparkle("branco", 6)], window=(6.6, 12.6))
        for b in [(1, 3, -4, -1, 4, 7), (5, 7, -5.5, -1, 3, 6), (9, 11, -5, -1, 5, 8), (13, 15, -3.5, -1, 4, 7), (3, 5, -3, -1, 9, 12),
                  (11, 13, -3, -1, 9, 12)]:
            hbox(*b, "gelo", [sparkle("branco", 4)], "cristal")
        hbox(-1.6, HW + 1.6, HH - 2, HH, -1.6, HD + 1.6, "pelo", [jag(1)], "pelo_gola")
    elif slot == "chest":
        chest("gelo", decos=[sparkle("branco", 5), plates("gelo", 5, 1)], sleeves=1.0, collar="pelo", pauldron="gelo",
              pauldron_decos=[sparkle("branco", 4)])
        shoulder_blocks("gelo", "crystal", decos=[sparkle("branco", 3)])
    elif slot == "legs":
        legs("gelo", decos=[sparkle("branco", 5), plates("gelo", 6, 1)], belt="pelo", knee="gelo")
    else:
        boots("gelo", decos=[sparkle("branco", 5)], cuff="pelo")


def mare(slot):
    if slot == "head":
        helm_closed("teal", "coral", decos=[scales("teal")])
        for b in [(2, 5, -3, -1, 3, 8), (6, 10, -4, -1, 3, 9), (11, 14, -3, -1, 3, 8)]:
            hbox(*b, "teal", [stripes("teal", 2, 1, 3, vertical=True)], "concha")
    elif slot == "chest":
        chest("teal", decos=[scales("teal")], sleeves=1.0, pauldron="coral", pauldron_decos=[sparkle("coral", 3)], gem_color="*ciano")
        shoulder_blocks("coral", "shell", decos=[stripes("coral", 2, 1, 3, vertical=True)])
    elif slot == "legs":
        legs("teal", decos=[scales("teal")], belt="coral", tassets="teal", tasset_decos=[stripes("teal", 2, 1, 3, vertical=True)])
    else:
        boots("teal", decos=[scales("teal")], toe="coral", cuff="*ciano")


def noite_violeta(slot):
    if slot == "head":
        helm_closed("violeta_esc", "runa", decos=[runes("*runa", .12)], window=(6.8, 12.4))
        horns("violeta_esc")
    elif slot == "chest":
        chest("violeta_esc", decos=[runes("*runa", .14), plates("violeta_esc", 6, 0)], pauldron="violeta_esc",
              pauldron_decos=[runes("*runa", .2)])
        shoulder_blocks("violeta_esc", "spike")
    elif slot == "legs":
        legs("violeta_esc", decos=[runes("*runa", .12)], belt="violeta_esc", knee="violeta_esc", knee_decos=[runes("*runa", .5)])
    else:
        boots("violeta_esc", decos=[runes("*runa", .15)], toe="violeta_esc")


def escorpiao(slot):
    if slot == "head":
        helm_closed("areia", "quitina", decos=[plates("quitina", 4, 0)], window=(6.8, 12.4), brow=[band(.6, 1, "quitina", 1)])
        scorpion_tail("quitina")
    elif slot == "chest":
        chest("quitina", decos=[plates("areia", 6, 1), vline(.45, .55, "*veneno", .2, .7)], pauldron="areia",
              pauldron_decos=[plates("quitina", 3, 0)])
        shoulder_blocks("areia", "spike")
    elif slot == "legs":
        legs("quitina", decos=[plates("areia", 6, 1)], tassets="areia", tasset_decos=[plates("quitina", 3, 0)], knee="areia")
    else:
        boots("quitina", decos=[plates("areia", 6, 1)], claws="areia")


def dragao_rubro(slot):
    if slot == "head":
        helm_closed("rubro", "obsidiana", decos=[scales("rubro")], window=(7.0, 12.4))
        dragon_crown("obsidiana")
        gem("*brasa", 4.6, size=1.6)
    elif slot == "chest":
        chest("rubro", decos=[scales("rubro"), dots("*brasa", 7, 2)], pauldron="rubro", pauldron_decos=[scales("rubro")],
              gem_color="*brasa")
        shoulder_blocks("obsidiana", "spike")
    elif slot == "legs":
        legs("rubro", decos=[scales("rubro"), dots("*brasa", 8, 2)], knee="obsidiana")
    else:
        boots("rubro", decos=[scales("rubro")], claws="obsidiana")


def grifo_dourado(slot):
    if slot == "head":
        helm_closed("dourado", "dourado", decos=[plates("dourado", 5, 1)], window=(6.6, 12.5))
        wings("dourado")
    elif slot == "chest":
        chest("prata", decos=[pendant(.5, .4, .22, "dourado", 2, "star"), plates("prata", 6, 1)], pauldron="dourado",
              pauldron_decos=[plates("dourado", 3, 1)], belt="dourado")
        shoulder_blocks("dourado", "wing")
    elif slot == "legs":
        legs("prata", decos=[plates("prata", 5, 1)], belt="dourado", tassets="dourado", knee="dourado")
    else:
        boots("dourado", decos=[plates("dourado", 4, 1)], claws="dourado")


def magma(slot):
    if slot == "head":
        helm_closed("obsidiana", "*lava", decos=[cracks("*lava")], window=(7.0, 11.8))
    elif slot == "chest":
        chest("obsidiana", decos=[cracks("*lava")], pauldron="obsidiana", pauldron_decos=[cracks("*lava")], sleeves=1.0, gem_color="*lava")
    elif slot == "legs":
        legs("obsidiana", decos=[cracks("*lava")], knee="obsidiana", knee_decos=[cracks("*lava")])
    else:
        boots("obsidiana", decos=[cracks("*lava")], cuff="*lava")


def bosque_ancestral(slot):
    if slot == "head":
        helm_closed("casca", "dourado", decos=[bark("casca")], window=(6.8, 12.6), brow=[band(.55, .8, "dourado", 2)])
        antlers("casca")
        gem("*esmeralda", 3.6, size=1.8)
    elif slot == "chest":
        chest("casca", decos=[bark("casca"), leaves("verde_folha", 9)], pauldron="casca", pauldron_decos=[bark("casca")],
              belt="dourado", gem_color="*esmeralda")
        shoulder_blocks("casca", "branch")
    elif slot == "legs":
        legs("casca", decos=[bark("casca")], belt="dourado", tassets="verde_folha", tasset_decos=[jag(2)], knee="dourado")
    else:
        boots("casca", decos=[bark("casca"), band(.2, .3, "dourado", 2)], toe="casca")


def espelho(slot):
    if slot == "head":
        helm_closed("prata", "lilas", decos=[sparkle("branco_puro", 6)], window=(6.6, 12.5))
        gem("*runa", 3.0, size=2.0)
    elif slot == "chest":
        chest("prata", decos=[sparkle("branco_puro", 5), plates("prata", 6, 1)], pauldron="prata", pauldron_decos=[hem("lilas", 1, 2)],
              gem_color="*runa")
    elif slot == "legs":
        legs("prata", decos=[sparkle("branco_puro", 6)], belt="lilas", tassets="prata", tasset_decos=[hem("lilas", 1, 2)], knee="prata")
    else:
        boots("prata", decos=[sparkle("branco_puro", 5)], toe="prata")


def sem_sombra(slot):
    if slot == "head":
        helm_closed("branco_puro", "*ciano", decos=[outline("*ciano")], window=(6.6, 12.6))
    elif slot == "chest":
        chest("branco_puro", decos=[outline("*ciano")], pauldron="branco_puro", pauldron_decos=[outline("*ciano")],
              belt="azul_noite")
    elif slot == "legs":
        legs("branco_puro", decos=[outline("*ciano")], belt="azul_noite", belt_decos=[hem("azul_noite", 1, 0)])
    else:
        boots("branco_puro", decos=[outline("*ciano")], cuff="*ciano")


def andarilho(slot):
    if slot == "head":
        helm_hood("couro", decos=[patches()], edge="couro_esc")
        hbox(10, 15.6, -1.8, 3, -1.4, 2, "ferro", [rivets("cinza", 3, 3)], "placa_remendo")
    elif slot == "chest":
        chest("couro", decos=[patches(), lace("couro_esc", .5, .3, .9)], pauldron="ferro", pauldron_decos=[rivets("cinza", 3, 3)],
              sleeves=.5, sleeve_color="remendo", gem_color="*ciano", belt="couro_esc")
    elif slot == "legs":
        legs("couro", decos=[patches()], belt="couro_esc", knee="ferro")
    else:
        boots("couro_esc", decos=[patches()], cuff="couro")


def aurora(slot):
    if slot == "head":
        helm_closed("branco", "dourado", decos=[plates("dourado", 6, 2)], window=(6.6, 12.5))
        wings("branco", decos=[stripes("dourado", 4, 1, 2)])
        gem("*esmeralda", 3.0, size=1.8)
    elif slot == "chest":
        chest("branco", decos=[plates("dourado", 6, 2)], pauldron="dourado", pauldron_decos=[plates("branco", 3, 2)],
              gem_color="*esmeralda")
        shoulder_blocks("branco", "wing", decos=[stripes("dourado", 4, 1, 2)])
    elif slot == "legs":
        legs("madeira_clara", decos=[], belt="dourado", tassets="branco", tasset_decos=[stripes("dourado", 4, 1, 2), jag(2)], knee="dourado")
    else:
        boots("branco", decos=[band(.5, .6, "dourado", 2)], toe="dourado", wings_color="branco")


def arvore_mundo(slot):
    if slot == "head":
        helm_closed("madeira_clara", "dourado", decos=[bark("madeira_clara")], window=(6.8, 12.6))
        antlers("madeira_clara")
        gem("*esmeralda", 3.2, size=2.2)
    elif slot == "chest":
        chest("madeira_clara", decos=[bark("madeira_clara"), leaves("dourado", 10)], pauldron="madeira_clara",
              pauldron_decos=[bark("madeira_clara")], gem_color="*esmeralda")
        shoulder_blocks("madeira_clara", "branch")
    elif slot == "legs":
        legs("madeira_clara", decos=[bark("madeira_clara")], belt="verde", tassets="verde", knee="madeira_clara",
             knee_decos=[dots("*esmeralda", 3, 3)])
    else:
        boots("madeira_clara", decos=[bark("madeira_clara"), band(.3, .4, "casca", 1)], toe="madeira_clara")


def estrelas(slot):
    if slot == "head":
        helm_closed("branco_puro", "dourado", decos=[stars("*dourado", 40)], window=(6.6, 12.5))
        star_crown("*dourado")
    elif slot == "chest":
        chest("branco_puro", decos=[stars("*dourado", 30), pendant(.5, .42, .2, "*dourado", 3, "star")], pauldron="dourado")
        shoulder_blocks("*dourado", "star")
    elif slot == "legs":
        legs("branco_puro", decos=[stars("*dourado", 32)], belt="dourado", knee="dourado", knee_decos=[pendant(.5, .5, .4, "*dourado", 3, "star")])
    else:
        boots("branco_puro", decos=[stars("*dourado", 32)], toe="dourado", wings_color="*dourado")


SETS = {f.__name__: f for f in (viajante, recruta, cacador, malha_ferro, batedor, bronze, guarda_real, geada, mare, noite_violeta,
                                escorpiao, dragao_rubro, grifo_dourado, magma, bosque_ancestral, espelho, sem_sombra, andarilho,
                                aurora, arvore_mundo, estrelas)}


def helm_relief(mat):
    """Volume de verdade por cima do elmo: cristas, rebites, lascas de rocha, ripas, cristais. Nunca na frente dos olhos."""
    main, trim, o, top, low = HELM["main"], HELM["trim"], HELM["o"], HELM["top"], HELM["low"]
    rnd = _random.Random(sum(map(ord, OUTFIT)))
    if HELM["kind"] == "hood":
        # a costura e a tira do capuz: duas camadas de couro
        hbox(6.8, 9.2, -o - .9, -o, -o - .3, HD + o + .3, trim, [], "capuz_costura")
        for c0, c1 in ((-o - 1.1, -o - .5), (HW + o + .5, HW + o + 1.1)):
            hbox(c0, c1, 3, 9, 3, 7, trim, [], "capuz_remendo")
        return
    if HELM["kind"] == "brim":
        hbox(-o - .3, HW + o + .3, .6, 2.2, -o - .3, HD + o + .3, trim, [], "chapeu_faixa")
        hbox(7, 9, -o - 2.0, -o - 1.2, -o, HD + o, main, [], "chapeu_crista")
        for c in (1.5, 5, 11, 14.5):
            hbox(c - .5, c + .5, .9, 1.9, -o - .8, -o - .3, "prata", [], "chapeu_rebite")
        return
    if mat == "metal":
        hbox(7.1, 8.9, -o - 2.2, -o - .9, -o - .6, HD + o + .6, trim, [], "elmo_quilha")     # a quilha sobre a crista
        hbox(5.8, 10.2, top - 5.2, top - 3.0, -o - 1.2, -o - .6, trim, [], "elmo_testeira")
        hbox(7.0, 9.0, top - 6.4, top - 5.2, -o - 1.2, -o - .6, trim, [], "elmo_testeira")
        for c in (.8, 3.4, 12.6, 15.2):                                                    # os rebites da faixa
            hbox(c - .45, c + .45, top - 2.6, top - 1.7, -o - 1.15, -o - .7, "prata", [], "elmo_rebite")
        for c0, c1 in ((-o - 1.2, -o - .7), (HW + o + .7, HW + o + 1.2)):
            for r in (top + 1, top + 4, top + 7):
                hbox(c0, c1, r, r + .9, 1, 2, "prata", [], "elmo_rebite")
        hbox(-o - .7, HW + o + .7, HH - 1.4, HH - .4, HD + o + 1, HD + o + 1.5, trim, [], "elmo_nuca_friso")
    elif mat == "rocha":
        for k in range(16):   # lascas soltas, sem ordem: a rocha não tem linha reta
            c, w, h, d = rnd.uniform(-o, HW + o - 3), rnd.uniform(2, 4.5), rnd.uniform(1, 2.2), rnd.uniform(2, 5)
            hbox(c, c + w, -o - h, -o, rnd.uniform(-o, HD - d), rnd.uniform(-o, HD - d) + d, main, [], "elmo_lasca")
        for s_ in (0, 1):
            for k in range(5):
                r, h, d = rnd.uniform(0, HH - 3), rnd.uniform(1.5, 3.5), rnd.uniform(1.5, 4.5)
                c0, c1 = (-o - rnd.uniform(.5, 1.6), -o) if s_ == 0 else (HW + o, HW + o + rnd.uniform(.5, 1.6))
                y0 = rnd.uniform(0, HD - d)
                hbox(c0, c1, r, r + h, y0, y0 + d, main, [], "elmo_lasca")
        for c in (0, 3, 6, 9, 12):
            hbox(c, c + 3.4, top - rnd.uniform(2.6, 4.4), top - .9, -o - rnd.uniform(.8, 1.5), -o, main, [], "elmo_lasca")
    elif mat == "madeira":
        for c in range(0, 16, 3):   # as ripas do tronco, de alturas diferentes
            hbox(c, c + 2, -o - rnd.uniform(.5, 1.8), top - 1, -o - rnd.uniform(.4, .9), -o, main, [], "elmo_ripa")
        for s_ in (-1, 1):
            x = 8 + s_ * 8.8
            hbox(x - 1, x + 1, -2.5, 1, 4, 7, main, [], "elmo_galho")
            hbox(x + s_ * .6 - .8, x + s_ * .6 + .8, -5, -2.5, 4.6, 6.4, main, [], "elmo_galho")
    elif mat == "cristal":
        for c, h in ((2, 3.5), (6, 5.5), (10.5, 4.5), (13.5, 2.8)):
            for k in range(3):
                w = 3 - k * .9
                hbox(c + k * .45, c + k * .45 + w, -o - h * (k + 1) / 3, -o - h * k / 3, 3 + k * .5, 3 + k * .5 + w + 1, trim if k == 2 else main, [], "elmo_cristal")
    elif mat == "escama":
        for row in range(3):   # fileiras de placas, uma sobre a outra
            off = (row % 2) * 1.5
            for c in range(-1, 16, 3):
                hbox(c + off, c + off + 2.6, -o - .9, -o, row * 4 + .5, row * 4 + 4.2, main, [], "elmo_escama")
        hbox(6.6, 9.4, top - 5.4, top - 3.2, -o - 1.1, -o - .6, trim, [], "elmo_testeira")
    elif mat == "quitina":
        for d in (1, 5.5, 10):   # os gomos da carapaça
            hbox(-o - .7, HW + o + .7, -o - 1.3, HH * .55, d, d + 2.5, trim, [], "elmo_gomo")


def limb_relief(mat, slot):
    """09/11: a ombreira e a joelheira de cada material, com volume (combinando com o que cada ícone mostra)."""
    main, trim = PART_INFO["main"], PART_INFO["trim"]
    rnd = _random.Random(sum(map(ord, OUTFIT)))
    for side in ("E", "D"):
        if slot == "chest":
            part = "braco_" + side
            pts = corners(part, 0, .4)
            c = mid(pts)
            top = c.z + 2.2 * TEX                      # o braço é inclinado: o ponto mais alto dele ficava na altura da cabeça
            out = 1 if c.x > 0 else -1
            w = 8 * TEX
            at = lambda dx, dy, dz: _V((c.x + out * dx * TEX, c.y + dy * TEX, top + dz * TEX))
            if mat == "metal":
                wblock(part, at(0, 0, 3.3), (w / TEX + 2.4, 8, .9), trim, [], "ombro_friso")
                wblock(part, at(0, 0, 4.1), (3, 8.6, .9), main, [], "ombro_quilha")
                for dy in (-3, 0, 3):
                    wblock(part, at(out * 0 + 4.2, dy, 3.8), (.9, .9, .9), "prata", [], "ombro_rebite")
            elif mat == "rocha":
                for k in range(7):
                    wblock(part, at(rnd.uniform(-3, 3.5), rnd.uniform(-3.5, 3.5), rnd.uniform(3, 4.4)), (rnd.uniform(2, 4), rnd.uniform(2, 4), rnd.uniform(1.4, 3)), main, [], "ombro_lasca")
            elif mat == "madeira":
                for dy in (-3, 0, 3):
                    wblock(part, at(rnd.uniform(-1, 1), dy, 3.6), (w / TEX + 1.6, 2.4, rnd.uniform(1.2, 2.2)), main, [], "ombro_casca")
            elif mat == "cristal":
                for dx, dy, h in ((-2, -2, 3.4), (1.6, 1.4, 4.6), (3.4, -2.4, 2.8)):
                    wblock(part, at(dx, dy, 3 + h / 2), (1.9, 1.9, h), trim, [], "ombro_cristal")
            elif mat == "escama":
                for row in range(3):
                    for k in range(3):
                        wblock(part, at(-3 + k * 3 + (row % 2) * 1.2, -3.2 + row * 3.2, 3.2), (2.6, 2.8, .9), main, [], "ombro_escama")
            elif mat == "quitina":
                for dy in (-3, 0, 3):
                    wblock(part, at(0, dy, 3.7), (w / TEX + 1.2, 2.2, 1.6), trim, [], "ombro_gomo")
            else:
                wblock(part, at(0, 0, 3), (w / TEX + 1.6, 7, .7), main, [], "ombro_couro")
                wblock(part, at(4, 0, 2), (1, 7.4, 2.2), trim, [], "ombro_tira")
        elif slot == "legs":
            part = "canela_" + side
            pts = corners(part, 0, .2)
            c = mid(pts)
            front = min(p.y for p in pts)
            at = lambda dx, dz: _V((c.x + dx * TEX, front - 2.2 * TEX, c.z + dz * TEX))
            if mat == "metal":
                wblock(part, at(0, 0), (4.2, 1, 3), trim, [], "joelho_centro")
                wblock(part, at(0, 2.2), (2.4, .9, 1.4), main, [], "joelho_ponta")
            elif mat == "rocha":
                for k in range(4):
                    wblock(part, at(rnd.uniform(-2.2, 2.2), rnd.uniform(-1.6, 1.8)), (rnd.uniform(1.8, 3.4), rnd.uniform(1, 1.8), rnd.uniform(1.8, 3)), main, [], "joelho_lasca")
            elif mat == "madeira":
                wblock(part, at(0, 0), (5, 1.3, 2.4), main, [], "joelho_no")
            elif mat == "cristal":
                wblock(part, at(0, 1), (2.2, 1.2, 4), trim, [], "joelho_cristal")
            elif mat in ("escama", "quitina"):
                for dz in (-1.4, 1):
                    wblock(part, at(0, dz), (5, 1.1, 1.5), trim, [], "joelho_placa")
            else:
                wblock(part, at(0, 0), (4.4, .8, 2.4), main, [], "joelho_couro")


def relief(grid, fname, pi, tx, ty):
    """09/11 (o usuário: "chapados, sem relevo"): a luz na borda de cima, a sombra na de baixo e o material de cada conjunto pintados por cima do desenho."""
    rows, cols = len(grid), len(grid[0])
    mat = globals().get("RELIEF_MAT") or MATERIAL.get(OUTFIT[2:].rsplit("_", 1)[0], "metal")
    seed = pi * 131 + len(fname) * 7

    def h(a, b, extra=0):
        return _random.Random((a * 7919 + b * 104729 + extra * 31 + seed) & 0xffffff).random()
    out = [row[:] for row in grid]
    for r in range(rows):
        for c in range(cols):
            k = 1.0
            if rows >= 3 and cols >= 3:
                if r == 0:
                    k += .24
                elif r == rows - 1:
                    k -= .16
                if c == 0:
                    k += .1
                elif c == cols - 1:
                    k -= .08
            if fname not in ("baixo",):
                if mat == "metal":
                    k += .2 if (c + r) % 19 < 2 else 0                     # o reflexo em diagonal
                    k += -.14 if h(c // 2, r) > .97 else 0                 # arranhões
                    k += .1 if (r == 1 or c == 1) and h(c, r, 2) > .5 else 0   # o desgaste perto da borda
                elif mat == "rocha":
                    k += (h(c // 3, r // 3) - .5) * .3                     # as manchas
                    k += (h(c, r, 1) - .5) * .16                           # o grão
                    if (c * 3 + r * 7 + int(h(c // 4, r // 4, 3) * 5)) % 23 == 0:
                        k -= .42                                           # as fendas
                elif mat == "couro":
                    k += (h(c // 2, r // 2) - .5) * .16
                    if rows > 5 and r in (1, rows - 2) and c % 3 == 0:
                        k += .3                                            # o pesponto
                elif mat == "madeira":
                    k += -.24 if (c + (r // 6) % 2) % 4 == 0 else (h(c, r // 3) - .5) * .12   # os veios
                elif mat == "cristal":
                    k += .16 if ((c - r) // 4) % 2 else -.08               # as facetas
                    k += .35 if h(c, r) > .985 else 0
                elif mat == "escama":
                    k += .12 if r % 3 == 0 else -.1 if r % 3 == 2 else 0
                    k += (h(c, r) - .5) * .08
                elif mat == "quitina":
                    k += -.26 if r % 5 == 4 else .14 if r % 5 == 0 else 0
                    k += .16 if (c + r) % 13 < 2 else 0
            out[r][c] = tuple(min(255, max(0, v * k)) for v in grid[r][c])
            if mat == "metal":   # o canal azul da máscara: metal de verdade na Unreal (M_ArmorBlock)
                mi = ((HEIGHT - 1 - (ty + r)) * WIDTH + tx + c) * 4
                mask[mi + 2], mask[mi + 3] = 1.0, 1.0
    return out


def ARMOR_BUILD(armor_id):
    """A_<conjunto>_<peça>: monta só a peça pedida."""
    body = armor_id[2:]
    key, slot = body.rsplit("_", 1)
    SETS[key](slot)
    if slot == "head" and HELM:
        helm_relief(MATERIAL.get(key, "metal"))
    if slot in ("chest", "legs") and PART_INFO:
        limb_relief(MATERIAL.get(key, "metal"), slot)
