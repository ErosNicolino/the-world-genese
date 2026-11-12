"""Cabelos de mechas do humano de blocos (01/11, 2ª versão; substitui o MakeBlockHair.py, só de cubos na grade).

O usuário pediu cabelos "não totalmente quadrados" e mais parecidos com as folhas de referência: cada cabelo agora é um
conjunto de mechas (prismas de 4 lados que afinam e se curvam), penteadas sobre a cabeça. Cada mecha nasce no couro,
corre pela superfície arredondada da cabeça na direção do penteado (flow), cai pela gravidade e para na altura do
corte; abaixo do queixo escorre pela frente ou pelas costas do tronco. Por baixo vai uma casca escura (o couro
cabeludo coberto), e por cima os enfeites de cada estilo: espetos, cachos, coques, rabos e tranças.

Medidas em casas: a cabeça é 16 x 16 x 16 (meio pixel da textura do rosto); i pela largura (0 = -X), j da frente (0 =
rosto, -Y) para trás, k de baixo (0 = queixo/ombro) para cima.

Saída igual à do MakeBlockHair.py (o Tools/ImportBlockHair.py e o jogo não mudam):
- a malha "Cabelo" com UV na paleta de 8 tons (T_Cabelo_Tons no jogo: a cor vem do herói);
- peso no mixamorig:Head; nos compridos, correntes de ossos Hair_<nome>_NN para a física (UVortexHairComponent).

No Blender aberto (MCP), com modelando ainda/personagem_base_rig.blend:
    STYLE = "moicano_punk"; exec(open(r".../Tools/Blender/MakeStrandHair.py", encoding="utf-8").read())
Sem janela: blender -b "modelando ainda/personagem_base_rig.blend" -P Tools/Blender/MakeStrandHair.py -- <estilo>
Para o jogo: blender -b --factory-startup -P Tools/Blender/MakeStrandHair.py -- jogo <estilo>
"""
import math
import os
import random
import sys

import bmesh
import bpy
from mathutils import Vector

ROOT = r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
OUT = os.path.join(ROOT, "modelando ainda")
ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
GAME = "jogo" in ARGS
SAVE = "salvar" in ARGS or GAME or bool(ARGS)
STYLE = next((a for a in ARGS if a not in ("jogo", "salvar")), None) or globals().get("STYLE", "moicano_punk")
GAME_OUT = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "Hair")
P = "mixamorig:"
random.seed(sum(map(ord, STYLE)))

# a paleta (a mesma do MakeBlockHair.py: a T_Cabelo_Tons do jogo não muda): sombra funda .. brilho
PALETTE = [(52, 30, 22), (70, 41, 28), (86, 52, 35), (100, 61, 41), (114, 71, 47), (130, 82, 54), (148, 96, 63),
           (168, 112, 74)]


def log(*parts):
    print("MECHAS", STYLE, " ".join(str(p) for p in parts))


# ---- no jogo: o esqueleto do herói como a Unreal tem (o glb do MixamoToHero.py), sem os clipes
if GAME:
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.ops.import_scene.gltf(filepath=os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "Hero_humano.glb"))
    for o in bpy.data.objects:
        if o.animation_data:
            o.animation_data.action = None
            for t in list(o.animation_data.nla_tracks):
                o.animation_data.nla_tracks.remove(t)
    for a in list(bpy.data.actions):
        bpy.data.actions.remove(a)
    for pb in next(o for o in bpy.data.objects if o.type == "ARMATURE").pose.bones:
        pb.matrix_basis.identity()
    bpy.context.view_layer.update()

# ---- a caixa da cabeça (os vértices só do osso da cabeça) e a do tronco
rig = next(o for o in bpy.data.objects if o.type == "ARMATURE")
body = next(o for o in bpy.data.objects if o.type == "MESH" and o.parent == rig and not o.name.startswith("Cabelo"))


def bone_box(name):
    g = body.vertex_groups[P + name].index
    pts = [body.matrix_world @ v.co for v in body.data.vertices if any(e.group == g and e.weight > .99 for e in v.groups)]
    return Vector([min(p[a] for p in pts) for a in range(3)]), Vector([max(p[a] for p in pts) for a in range(3)])


H0, H1 = bone_box("Head")
CELL = Vector(((H1.x - H0.x) / 16, (H1.y - H0.y) / 16, (H1.z - H0.z) / 16))
T0, T1 = bone_box("Spine2")


def cells(world):
    return Vector(((world.x - H0.x) / CELL.x, (world.y - H0.y) / CELL.y, (world.z - H0.z) / CELL.z))


def world(c):
    return Vector((H0.x + c.x * CELL.x, H0.y + c.y * CELL.y, H0.z + c.z * CELL.z))


TORSO_FRONT, TORSO_BACK = cells(T0).y, cells(T1).y                # ~3,6 e ~12,4
HC = Vector((8.0, 8.0, 8.0))
log("cabeça %.3f m, casa %.4f m, tronco j %.1f..%.1f" % (H1.x - H0.x, CELL.x, TORSO_FRONT, TORSO_BACK))

# ---- a superfície onde o cabelo deita: a cabeça arredondada (superelipse) com uma folga; abaixo do meio da cabeça
# as paredes são retas; abaixo do queixo, o tronco
ROUND = 4.5


def hug(q, off):
    d = q - HC
    a = 8.0 + off
    if q.z >= 8.0:
        f = (abs(d.x / a) ** ROUND + abs(d.y / a) ** ROUND + abs(d.z / a) ** ROUND) ** (1 / ROUND)
        return HC + d / max(f, 1e-6)
    if q.z >= -1.0:
        f = (abs(d.x / a) ** ROUND + abs(d.y / a) ** ROUND) ** (1 / ROUND)
        return Vector((8 + d.x / max(f, 1e-6), 8 + d.y / max(f, 1e-6), q.z))
    # no tronco: a frente ou as costas (com a folga), sem passar dos lados do tronco
    y = TORSO_FRONT - off if q.y < 8 else TORSO_BACK + off
    return Vector((min(max(q.x, -off * .5), 16 + off * .5), y, q.z))


def top(i, j, off):
    """O ponto da superfície (com a folga off) logo acima de (i, j); fora do contorno do alto, na parede ao lado."""
    a = 8.0 + off
    dx, dy = (i - 8) / a, (j - 8) / a
    rest = 1 - abs(dx) ** ROUND - abs(dy) ** ROUND
    if rest > 0:
        return Vector((i, j, 8 + a * rest ** (1 / ROUND)))
    return hug(Vector((i, j, 14.0)), off)


def normal_at(q):
    d = q - HC
    if q.z >= 8.0:
        g = Vector([math.copysign(abs(c) ** (ROUND - 1), c) for c in d])
    elif q.z >= -1.0:
        g = Vector((math.copysign(abs(d.x) ** (ROUND - 1), d.x), math.copysign(abs(d.y) ** (ROUND - 1), d.y), 0))
    else:
        g = Vector((0, -1 if q.y < 8 else 1, 0))
    return g.normalized() if g.length > 1e-9 else Vector((0, 0, 1))


FRONT_LINE = 14       # a linha do cabelo na testa (casas)
FACE_MIN = 9.5        # na frente do rosto nenhuma mecha desce abaixo daqui (as sobrancelhas; os olhos ficam livres)

# ---- as peças
STRANDS = []          # cada mecha: {"pts": [Vector], "w", "d", "tip", "tone", "group"}
BLOBS = []            # blocos soltos (cachos, coques, nós das tranças, prendedores): centro, tamanho, giro, tom, grupo
GROUPS = {}           # grupo de física -> número de ossos


def strand(pts, w=3.0, d=2.0, tip=.3, tone=0, group=None, wave=0.0, waves=0.0, curl=0.0):
    """Uma mecha pelos pontos dados (em casas). wave: ondas de lado (amplitude em casas, waves = quantas)."""
    if len(pts) < 2:
        return
    if wave or curl:
        out = []
        for n, p in enumerate(pts):
            f = n / (len(pts) - 1)
            nrm = normal_at(p)
            t = (pts[min(n + 1, len(pts) - 1)] - pts[max(n - 1, 0)]).normalized()
            side = t.cross(nrm).normalized()
            out.append(p + side * wave * math.sin(f * waves * math.pi * 2) * f
                       + nrm * curl * (.5 + .5 * math.cos(f * waves * math.pi * 2)) * f)
        pts = out
    STRANDS.append({"pts": pts, "w": w, "d": d, "tip": tip, "tone": tone, "group": group})


def lay(root, flow, bottom, off=1.2, step=1.0, lift=0.0, spread=0.0, max_len=80, curve_out=0.0):
    """O caminho de uma mecha penteada: nasce em root (i, j no alto da cabeça ou um ponto já na superfície), corre na
    direção flow (i, j) e cai até a altura bottom. lift: volume (a mecha sobe um pouco antes de deitar). spread: abre
    para fora na ponta (cabelo armado). curve_out: a ponta vira para fora (as pontas viradas)."""
    p = hug(Vector(root), off) if len(root) == 3 else top(root[0], root[1], off)
    v = Vector((flow[0], flow[1], 0))
    v = v.normalized() if v.length > 1e-6 else Vector((0, 0, -1))
    pts = [p]
    for n in range(max_len):
        g = max(.12, min(1.0, (15.5 - p.z) / 2.5))                  # no alto penteia, nos lados cai
        dirv = (v * (1 - g) + Vector((0, 0, -1)) * g).normalized()
        q = p + dirv * step
        f = len(pts) / 12.0
        extra = off + lift * math.exp(-f * 2) + spread * max(0.0, (14 - q.z) / 14) + curve_out * max(0.0, (bottom + 3 - q.z) / 3)
        q = hug(q, extra)
        stop = bottom
        if q.y < .5 and 1.0 < q.x < 15.0:
            stop = max(bottom, FACE_MIN)                       # na frente do rosto: acima dos olhos
        if q.z <= stop:
            bottom = stop
            # a ponta exatamente no corte
            t = (p.z - bottom) / max(1e-6, p.z - q.z)
            pts.append(p.lerp(q, t))
            break
        nv = q - p
        if nv.length > 1e-6:
            v = Vector((nv.x, nv.y, 0)).normalized() if abs(nv.z) < .9 * nv.length else v
        pts.append(q)
        p = q
    return pts


def comb(root, flow, bottom, w=3.2, d=2.0, tip=.35, tone=0, group=None, **kw):
    wave = kw.pop("wave", 0.0)
    waves = kw.pop("waves", 0.0)
    curl = kw.pop("curl", 0.0)
    strand(lay(root, flow, bottom, **kw), w, d, tip, tone, group, wave, waves, curl)


def blob(c, size, rot=(0, 0, 0), tone=0, group=None):
    BLOBS.append((Vector(c), Vector(size) if hasattr(size, "__len__") else Vector((size, size, size)),
                  Vector(rot), tone, group))


def spike(root, direction, length, w=3.5, d=3.0, tip=.08, bend=(0, 0, 0), tone=1, steps=6):
    """Um espeto: sai do couro na direção dada e curva (bend) até a ponta fina."""
    base = hug(Vector(root), .6)
    dirv = Vector(direction).normalized()
    pts = [base]
    for n in range(1, steps + 1):
        f = n / steps
        pts.append(base + dirv * length * f + Vector(bend) * f * f)
    strand(pts, w, d, tip, tone)


CELLS = {}            # (i, j, k) -> [tom, grupo, mecha, couro]
SOLID_HEAD = lambda i, j, k: 0 <= i < 16 and 0 <= j < 16 and 0 <= k < 16


def put(c, tone, group, sid, scalp_cell=False):
    if SOLID_HEAD(*c):
        return
    cur = CELLS.get(c)
    if cur is None or (cur[3] and not scalp_cell):
        CELLS[c] = [tone, group, sid, scalp_cell]


STEPS = []            # espetos em degraus: (raiz, direção, comprimento, base, tom)


def step_spike(root, direction, length, base=4.0, tone=0, end=1.0):
    """Um espeto em degraus (pirâmide inclinada): camadas quadradas que afinam de base até end casas, cada uma andando
    na direção dada (o eixo maior da direção é o da pilha)."""
    STEPS.append((Vector(root), Vector(direction).normalized(), length, base, tone, end))


def scalp(bottom_at, off=.55, tone=1):
    """A casca do couro (escura) por baixo das mechas: cobre de bottom_at(azimute) para cima."""
    STRANDS.append({"scalp": (bottom_at, off, tone)})


def flow_from(center_i, center_j=None):
    """Penteado que sai de um ponto (a coroa) para todos os lados."""
    def f(i, j):
        return (i - center_i, j - (center_j if center_j is not None else center_i))
    return f


def ring_roots(k_top=True, step=2.0, inset=1.0):
    """Raízes em grade no alto da cabeça (i, j)."""
    out = []
    v = inset
    while v <= 16 - inset + 1e-6:
        u = inset
        while u <= 16 - inset + 1e-6:
            out.append((u + random.uniform(-.4, .4), v + random.uniform(-.4, .4)))
            u += step
        v += step
    return out


def azimuth(i, j):
    return math.atan2(j - 8, i - 8)


def zone(i, j):
    """frente / lados / trás pelo ângulo em volta da cabeça (a frente é j pequeno)."""
    a = math.degrees(azimuth(i, j))       # -90 = frente, 90 = trás, 0 = direita (+i), 180 = esquerda
    if -135 <= a <= -45:
        return "frente"
    if 45 <= a <= 135:
        return "tras"
    return "direita" if -45 < a < 45 else "esquerda"


def fringe_bottom(i, base, jag=1.5, open_mid=0.0):
    """A altura da ponta da franja em i: base com mechas desiguais; open_mid abre no meio (cortina)."""
    return base + random.uniform(-jag, jag) + open_mid * max(0.0, 1 - abs(i - 8) / 5)


# ---- penteados ---------------------------------------------------------------------------------------------------
# cada um: a casca (scalp), as mechas (comb/strand) e os enfeites (blob/spike). bottom: a altura da ponta em casas
# (16 = o alto da cabeça, 9 = as sobrancelhas, 6 = os olhos, 0 = o queixo, -8 = o peito)


def flow_part(part_i, front_to=None, crown_j=7.0):
    """Risca em part_i: cada lado penteia para o seu lado; as raízes da frente puxam para a testa (a cortina, ou
    front_to para um lado só) e as de trás para a nuca."""
    def f(i, j):
        side = -1 if i < part_i else 1
        dy = (j - crown_j) / 4.0
        dx = side * (.55 if j <= 3 else max(.12, 1.0 - max(0.0, j - 8) / 5))
        if front_to is not None and j < 5:
            dx = front_to
        return (dx, dy)
    return f


def head_cover(flow, bottoms, w=4.2, d=2.4, tip=.45, step=2.0, off=1.0, lift=0.0, groups=None, jag=1.0, sides=True, **kw):
    """Cobre a cabeça de mechas penteadas: raízes em grade no alto, cada uma na direção flow(i, j) até bottoms(zona, i, j)."""
    roots = ring_roots(step=step)
    if sides:
        edge = []
        for v in range(1, 16, 2):
            edge += [(.4, v + .5), (15.6, v + .5), (v + .5, 15.6)]
        roots += [(i + random.uniform(-.3, .3), j + random.uniform(-.3, .3)) for (i, j) in edge]
    for (i, j) in roots:
        z = zone(i, j)
        flowv = flow(i, j)
        if i < 1 or i > 15 or j > 15:
            out = (-1 if i < 1 else 1 if i > 15 else 0, 1 if j > 15 else 0)
            flowv = (flowv[0] * .4 + out[0], flowv[1] * .4 + out[1])
        # a ponta: a zona para onde a mecha vai (a frente de quem penteia para a frente)
        tgt_i, tgt_j = i + flowv[0] * 8, j + flowv[1] * 8
        zt = zone(tgt_i, tgt_j)
        b = bottoms(zt, i, j) + random.uniform(-jag, jag)
        g = groups(zt, i, j) if groups else None
        comb((i, j), flowv, b, w=w * random.uniform(.85, 1.15), d=d, tip=tip, tone=random.choice((-1, 0, 0, 1)),
             group=g, off=off + random.uniform(0, .5), lift=lift, **kw)


def long_groups(top=4):
    def g(z, i, j):
        return {"tras": "Back", "esquerda": "Left", "direita": "Right", "frente": None}[z]
    return g


def style_liso_curto():
    scalp(lambda a: 6)
    head_cover(flow_from(8, 9), lambda z, i, j: {"frente": 9.5, "tras": 3, "esquerda": 6, "direita": 6}[z], lift=1.2)


def style_longo_desfiado():
    scalp(lambda a: 4)
    head_cover(flow_part(6.5), lambda z, i, j: {"frente": 7, "tras": -6, "esquerda": -3, "direita": -3}[z],
               lift=1.5, spread=1.6, jag=2.5, groups=long_groups(), wave=.8, waves=1.5, w=3.0)
    for n in range(10):                                     # mechas soltas e desfiadas por cima
        i = random.uniform(1, 15)
        comb((i, random.uniform(2, 10)), (random.uniform(-1, 1), 1), random.uniform(-5, 2), w=2.2, d=1.5, tip=.2,
             tone=1, off=2.4, spread=2.0, wave=1.0, waves=1.0, group="Back")


def style_moicano_punk():
    # moicano punk: os lados raspados (a pele aparece) e uma crista estreita no meio (6 casas), da testa até a nuca, com
    # uma fileira só de espetos em degraus, em leque: os da frente inclinados para a frente, os do meio mais altos e
    # em pé, os de trás deitando para trás
    for j in range(-1, 18):
        for i in range(5, 11):
            for k in range(16, 18):
                put((i, j, k), 0, None, -2)
    for j in (16, 17):
        for i in range(5, 11):
            for k in range(9, 16):
                put((i, j, k), 0, None, -2)
    count = 8
    for n in range(count):
        f = n / (count - 1)
        ang = math.radians(-18 + f * 108)                    # o leque: de -18° (para a frente) até 90° (deitado atrás)
        d = (0.0, math.sin(ang), math.cos(ang))
        length = 9 + 6 * math.sin(f * math.pi) + random.uniform(-.5, .5)
        root = (8.0, 2.5 + f * 13.5, 17.5 - max(0.0, f - .8) * 9)
        step_spike(root, d, length, base=5, tone=random.choice((0, 1)), end=1)


def style_risca_lateral():
    scalp(lambda a: 6)
    head_cover(flow_part(5, front_to=-1.4), lambda z, i, j: {"frente": 10, "tras": 3, "esquerda": 6, "direita": 6}[z],
               lift=1.8, jag=1.2)
    # a franja varrida para o lado (da risca para a direita, por cima da testa)
    for n in range(5):
        comb((5 + n * 1.5, 3), (1.4, -.5), random.uniform(9, 11.5), w=3.2, d=2.0, off=2.2, lift=1.5, tone=1)


def style_meio_dividido():
    scalp(lambda a: 6)
    head_cover(flow_part(8, front_to=-.8), lambda z, i, j: {"frente": 8.5, "tras": 3, "esquerda": 5, "direita": 5}[z],
               lift=1.4, jag=1.5, curve_out=.8)
    # a cortina: das duas bandas da risca para os lados da testa
    for side in (-1, 1):
        for n in range(4):
            comb((8 + side * (1 + n * 1.6), 2.5), (side * 1.0, -.6), random.uniform(7.5, 10), w=3.0, d=2.0, off=2.0,
                 lift=1.2, curve_out=1.0, tone=random.choice((0, 1)))


def style_penteado_tras():
    scalp(lambda a: 5)
    # tudo para trás, com volume na frente (o topete) e os lados penteados para trás rentes
    head_cover(lambda i, j: (0.25 * (i - 8) / 8, 1.0), lambda z, i, j: {"frente": 6, "tras": 3, "esquerda": 6, "direita": 6}[z],
               lift=1.0, jag=.8)
    for n in range(7):
        i = 2.5 + n * 1.8
        comb((i, 1.0, 16.0), (0, 1), 12, w=4.2, d=3.0, off=3.0, lift=3.0, tone=1)


def style_undercut():
    scalp(lambda a: 5, tone=3)                              # lados e nuca raspados (rente)
    # o topo comprido, volumoso, varrido para a frente e para um lado, caindo um pouco na testa
    for (i, j) in ring_roots(step=1.8, inset=2.5):
        comb((i, j), (.55, -1.0), random.uniform(11, 13.5), w=4.4, d=2.8, off=1.8 + random.uniform(0, .8), lift=2.6,
             curve_out=.8, tone=random.choice((0, 1, 1)))


def style_mullet():
    scalp(lambda a: 5)
    head_cover(flow_from(8, 8), lambda z, i, j: {"frente": 9, "tras": -6, "esquerda": 5, "direita": 5}[z],
               lift=1.5, jag=2.0, groups=lambda z, i, j: "Back" if z == "tras" else None, wave=.6, waves=1.2)


def style_raspado():
    # rente em tudo, com a linha do cabelo marcada na testa e um pouco mais cheio no alto
    scalp(lambda a: 5, tone=3)
    for (i, j) in ring_roots(step=1.0, inset=1.0):
        if random.random() < .35:
            put((int(i), int(j), 16), 4, None, -3)


def style_espetado_curto():
    # curto e armado: cachos baixos cobrindo o alto e espetos curtos em degraus apontando para cima e para fora
    scalp(lambda a: 5, tone=2)
    curls_on([(i, j, 30) for (i, j) in ring_roots(step=2.2, inset=.8)], size=(2.6, 3.2), off=1.8)
    for (i, j) in ring_roots(step=2.8, inset=1.5):
        n = normal_at(top(i, j, 1))
        root = top(i, j, 2.0)
        step_spike(root, (n.x * 1.2 + random.uniform(-.3, .3), n.y * 1.2 - .3, 1.0), random.uniform(4, 6.5), base=3,
                   tone=random.choice((0, 1, 1)), end=1)
    for n in range(10):                                     # franja curta e arrepiada
        comb((2 + n * 1.3, 2.0), (random.uniform(-.3, .3), -1), random.uniform(11, 13), w=3.2, d=2.4, off=2.2, lift=1.4,
             curve_out=1.2, tone=1)


def curls_on(roots, size=(2.6, 3.4), off=1.5, tone_choice=(0, 1, 1, 2), group=None):
    for (i, j, k) in roots:
        q = top(i, j, off) if k >= 30 else hug(Vector((i, j, k)), off)
        s = random.uniform(*size)
        blob(q, (s, s, s * .85), (random.uniform(-.5, .5), random.uniform(-.5, .5), random.uniform(0, 1.6)),
             random.choice(tone_choice), group)


def style_degrade_cacheado():
    # os lados em degradê (rente) e o topo cheio de cachos redondos, em duas camadas
    scalp(lambda a: 5, tone=3)
    curls_on([(i, j, 30) for (i, j) in ring_roots(step=2.0, inset=.5)], size=(3.0, 3.8), off=2.0)
    curls_on([(i, j, 30) for (i, j) in ring_roots(step=2.6, inset=2.0)], size=(3.0, 3.8), off=3.6)
    for n in range(26):
        a = random.uniform(0, math.tau)
        curls_on([(8 + math.cos(a) * 9, 8 + math.sin(a) * 9, random.uniform(13, 16))], size=(2.6, 3.2), off=1.6)


def side_locks(bottom, n=3, w=3.0, inset=0.0, group=None, wave=0.0):
    """As mechas da frente, ao lado do rosto (emolduram o rosto e caem na frente do ombro)."""
    for side in (-1, 1):
        for m in range(n):
            i = 8 + side * (7.6 + m * .9 - inset)
            comb((i, 1.0 + m * 1.2), (side * 1.0, -.5), bottom + random.uniform(-1.5, 1.5), w=w, d=2.0, off=1.6 + m * .3,
                 lift=1.0, group=(("Left" if side < 0 else "Right") if group else None), wave=wave, waves=1.5)


def curtain(bottom=8.5, n=4, side_drop=1.5, w=3.0):
    """Franja de cortina aberta no meio."""
    for side in (-1, 1):
        for m in range(n):
            comb((8 + side * (.6 + m * 1.5), 3.0), (side * .55, -1.0), bottom + 2 - m * side_drop + random.uniform(-.8, .8),
                 w=w, d=2.0, off=1.8, lift=1.4, tone=random.choice((0, 1)))


def straight_bangs(bottom=9.0):
    for m in range(7):
        comb((2 + m * 2, 3.5), (0, -1), bottom + random.uniform(-.6, .6), w=3.0, d=2.2, off=1.8, lift=1.6,
             tone=random.choice((0, 1)))


def long_back(bottom, group="Back", wave=0.0, waves=1.5, spread=1.2, step=2.2, lift=1.4):
    """O cabelo comprido penteado do alto para trás e para os lados, caindo nas costas."""
    def f(i, j):
        return ((i - 8) * .12, 1.0)
    for (i, j) in ring_roots(step=step):
        if j < 4:
            continue
        side = "Left" if i < 5 else "Right" if i > 11 else group
        comb((i, j), f(i, j), bottom + random.uniform(-1.8, 1.8), w=3.4, d=2.0, tip=.35, group=side, off=1.0 + random.uniform(0, .5),
             lift=lift, spread=spread, wave=wave, waves=waves)


def tie(c, size=(2.4, 2.0, 2.0)):
    blob(c, size, (0, 0, 0), -9)                            # o prendedor (tom 0)


def tail(start, path, width=6.0, group="Tail", wave=.6, strands=7, tip=.25):
    """Um rabo: mechas juntas no prendedor, abrindo um pouco e afinando até a ponta (path: pontos em casas)."""
    GROUPS[group] = 5
    pts = [Vector(start)] + [Vector(p) for p in path]
    for s in range(strands):
        a = s / strands * math.tau
        off = Vector((math.cos(a), math.sin(a) * .7, 0)) * width * .22
        sp = [p + off * (.6 + .6 * n / len(pts)) for n, p in enumerate(pts)]
        strand(sp, w=width * .42, d=width * .38, tip=tip, tone=random.choice((-1, 0, 0, 1)), group=group, wave=wave, waves=1.2)


def braid(start, path, width=4.0, group="Braid", knots=None):
    """Trança: nós alternados (blocos girados) ao longo do caminho, mais finos na ponta, e um tufo no fim."""
    GROUPS[group] = 5
    pts = [Vector(start)] + [Vector(p) for p in path]
    total = sum((b - a).length for a, b in zip(pts, pts[1:]))
    n = knots or max(4, int(total / 2.6))
    for m in range(n):
        f = m / max(1, n - 1)
        d = f * total
        for a, b in zip(pts, pts[1:]):
            seg = (b - a).length
            if d <= seg:
                c = a.lerp(b, d / seg)
                break
            d -= seg
        else:
            c = pts[-1]
        s = width * (1 - .35 * f)
        blob(c, (s, s * .85, 3.4), (0, 0, (.45 if m % 2 else -.45)), 0 if m % 2 else 1, group)
    tie(pts[-1] + Vector((0, 0, -.5)), (2.0, 1.8, 1.4))
    end = pts[-1]
    strand([end, end + Vector((0, 0, -2.5)), end + Vector((0, -.3, -4.5))], w=3.0, d=2.4, tip=.3, group=group)


def bun(c, r=4.0, group=None):
    """Coque: um bolo de mechas enroladas (blocos em anel) com o miolo cheio."""
    c = Vector(c)
    blob(c, (r * 1.3, r * 1.3, r * 1.1), (0, 0, .4), 0, group)
    for m in range(9):
        a = m / 9 * math.tau
        p = c + Vector((math.cos(a) * r * .65, math.sin(a) * r * .65, random.uniform(-.6, .9)))
        blob(p, (r * .75, r * .55, r * .6), (random.uniform(-.3, .3), random.uniform(-.3, .3), a), random.choice((0, 1, 2)), group)


def style_liso_longo():
    scalp(lambda a: 5)
    curtain(9, n=3)
    side_locks(-7, group=True)
    long_back(-9)
    head_cover(flow_part(8), lambda z, i, j: {"frente": 10, "tras": -8, "esquerda": -6, "direita": -6}[z],
               groups=long_groups(), lift=1.2, step=2.6)


def style_ondulado_longo():
    scalp(lambda a: 5)
    curtain(8.5, n=3)
    side_locks(-7, group=True, wave=1.2)
    long_back(-9, wave=1.4, waves=2.5, spread=2.0)
    head_cover(flow_part(7), lambda z, i, j: {"frente": 10, "tras": -8, "esquerda": -6, "direita": -6}[z],
               groups=long_groups(), lift=1.6, step=2.6, wave=1.2, waves=2.5)


def style_cacheado_longo():
    scalp(lambda a: 5, tone=0)
    long_back(-7, wave=1.8, waves=4, spread=2.0, lift=2.0)
    side_locks(-5, group=True, wave=1.6)
    curls_on([(i, j, 30) for (i, j) in ring_roots(step=2.0, inset=.5)], size=(3.2, 4.0), off=2.4)
    for n in range(60):                                     # cachos por toda a volta, até os ombros
        a = random.uniform(-math.pi, math.pi)
        if -2.4 < a < -.75:
            continue                                        # o rosto fica livre (a frente é -90°)
        curls_on([(8 + math.cos(a) * 9, 8 + math.sin(a) * 9, random.uniform(-6, 15))], size=(3.0, 3.8), off=2.2)


def style_meio_preso():
    scalp(lambda a: 5)
    curtain(8.5, n=3)
    side_locks(-6, group=True)
    long_back(-8)
    # a parte de cima presa atrás num coque pequeno
    head_cover(lambda i, j: (0, 1), lambda z, i, j: 11, step=2.6, lift=1.0)
    bun((8, 18.5, 13.5), 3.0)


def style_rabo_cavalo_alto():
    scalp(lambda a: 6)
    head_cover(lambda i, j: ((i - 8) * .05, 1.0), lambda z, i, j: {"frente": 13, "tras": 9, "esquerda": 8, "direita": 8}[z],
               step=2.4, lift=.6)
    curtain(9.5, n=2)
    side_locks(-2, n=1, w=2.6)
    tie((8, 18.0, 14.5), (3.4, 2.4, 3.0))
    tail((8, 18.8, 14.5), [(8, 22, 13), (8, 23, 6), (8, 22, -2), (8, 21, -9)], width=6.5)


def style_rabo_cavalo_baixo():
    scalp(lambda a: 5)
    head_cover(lambda i, j: ((i - 8) * .05, 1.0), lambda z, i, j: {"frente": 12, "tras": 4, "esquerda": 6, "direita": 6}[z],
               step=2.4, lift=.8)
    curtain(9, n=3)
    side_locks(1, n=1, w=2.6)
    # preso na nuca, à esquerda, e jogado por cima do ombro esquerdo para a frente
    tie((4.0, 17.0, 4.0), (3.0, 2.6, 2.6))
    tail((4.0, 17.5, 4.0), [(1.5, 17, 1), (-1.0, 13, -1), (-1.5, 6, -3), (0.5, 1.5, -7), (2.0, 1.0, -11)], width=6.0)


def style_maria_chiquinha():
    scalp(lambda a: 5)
    curtain(9, n=3)
    long_back(-7, step=2.6)
    head_cover(flow_part(8), lambda z, i, j: {"frente": 11, "tras": -5, "esquerda": 3, "direita": 3}[z],
               groups=long_groups(), step=2.8)
    # dois rabos grossos no alto dos lados, caindo compridos na frente dos ombros
    for side, g in ((-1, "TailL"), (1, "TailR")):
        x = 8 + side * 10.5
        tie((8 + side * 9.2, 8.5, 15.5), (2.6, 3.0, 3.0))
        tail((x, 8.5, 15.5), [(x + side * 2.5, 8, 12), (x + side * 2.8, 6, 4), (x + side * 2.0, 3.5, -3),
                              (x + side * 1.0, 2.0, -10)], width=8.0, group=g, strands=8)


def style_bob_curto():
    scalp(lambda a: 4)
    head_cover(flow_part(8), lambda z, i, j: {"frente": 9, "tras": 1, "esquerda": 0, "direita": 0}[z],
               lift=1.8, step=2.2, curve_out=1.6, jag=.6, w=3.6)
    curtain(9, n=3)


def style_camadas_medias():
    scalp(lambda a: 5)
    curtain(8.5, n=3)
    side_locks(-7, group=True, wave=1.6)
    long_back(-9, wave=1.4, waves=2.0, spread=2.6)
    head_cover(flow_part(8), lambda z, i, j: {"frente": 9.5, "tras": -6, "esquerda": random.choice((-6, -2, 2)),
                                              "direita": random.choice((-6, -2, 2))}[z],
               groups=long_groups(), lift=2.0, spread=2.2, curve_out=1.4, step=2.4, wave=1.2, waves=2.0, jag=2.0)


def style_franja_longa():
    scalp(lambda a: 5)
    straight_bangs(8.5)
    side_locks(-8, group=True)
    long_back(-10, spread=.8)
    head_cover(lambda i, j: ((i - 8) * .1, .6), lambda z, i, j: {"frente": 9, "tras": -9, "esquerda": -7, "direita": -7}[z],
               groups=long_groups(), lift=1.2, step=2.4, jag=.6)


def style_curto_baguncado():
    scalp(lambda a: 4)
    head_cover(flow_from(8, 9), lambda z, i, j: {"frente": 8.5, "tras": 0, "esquerda": 1, "direita": 1}[z],
               lift=2.0, spread=1.5, curve_out=1.8, jag=2.2, wave=.9, waves=1.3, step=2.2)
    for n in range(10):
        a = random.uniform(-math.pi, 0)
        comb((8 + math.cos(a) * 5, 8 + math.sin(a) * 5), (math.cos(a), math.sin(a)), random.uniform(6, 10), w=2.6,
             off=2.4, lift=2.0, curve_out=2.0, tone=1)


def style_coque_despojado():
    scalp(lambda a: 6)
    head_cover(lambda i, j: ((8 - i) * .05, (8 - j) * .05 + .2), lambda z, i, j: {"frente": 12, "tras": 7, "esquerda": 8, "direita": 8}[z],
               step=2.4, lift=.8)
    curtain(9, n=2)
    side_locks(0, n=1, w=2.4, wave=.8)
    bun((8, 9, 20.5), 4.2)
    for n in range(5):                                      # fios soltos do coque
        a = random.uniform(0, math.tau)
        strand([Vector((8, 9, 21)), Vector((8 + math.cos(a) * 4, 9 + math.sin(a) * 4, 23)),
                Vector((8 + math.cos(a) * 6, 9 + math.sin(a) * 6, 22))], w=1.8, d=1.4, tip=.2, tone=1)


def style_tranca_lateral():
    scalp(lambda a: 5)
    curtain(9, n=3)
    long_back(-6, step=2.6)
    head_cover(flow_part(6), lambda z, i, j: {"frente": 10, "tras": -5, "esquerda": 0, "direita": 2}[z],
               groups=long_groups(), step=2.6)
    # a trança sai de trás da orelha direita e cai na frente do ombro
    braid((17.5, 9, 7), [(18.0, 6, 2), (16.5, 2.5, -3), (15.0, 1.5, -8), (14.5, 1.5, -13)], width=6.2)


def style_dois_coques():
    scalp(lambda a: 5)
    curtain(9, n=3)
    side_locks(-3, n=1, w=2.6)
    long_back(-8, step=2.6)
    head_cover(flow_part(8), lambda z, i, j: {"frente": 11, "tras": -6, "esquerda": 2, "direita": 2}[z],
               groups=long_groups(), step=2.8)
    bun((1.5, 8.5, 20.0), 4.4)
    bun((14.5, 8.5, 20.0), 4.4)


def style_trancas_duplas():
    scalp(lambda a: 5)
    head_cover(flow_part(8), lambda z, i, j: {"frente": 11, "tras": 4, "esquerda": 5, "direita": 5}[z], step=2.4, lift=.8)
    curtain(9.5, n=2)
    for side, g in ((-1, "BraidL"), (1, "BraidR")):
        x = 8 + side * 9.2
        braid((x, 8, 6), [(x - side * .5, 5, 1), (x - side * 1.5, 2.5, -4), (x - side * 2.2, 2.0, -9), (x - side * 2.5, 2.0, -12)],
              width=5.8, group=g)


STYLES = {n[6:]: f for n, f in list(globals().items()) if n.startswith("style_") and callable(f)}
STYLES[STYLE]()

# ---- os blocos: cada peça (mecha = cápsula que afina, coque/cacho = elipsoide, casca do couro) vira as casas que ela
# ocupa por fora da cabeça. O cabelo continua de blocos (como as referências), mas com a silhueta das mechas
def raster_capsule(a, b, ra, rb, tone, group, sid):
    lo = [math.floor(min(a[x] - ra, b[x] - rb)) - 1 for x in range(3)]
    hi = [math.ceil(max(a[x] + ra, b[x] + rb)) + 1 for x in range(3)]
    ab = b - a
    L2 = max(ab.length_squared, 1e-9)
    for i in range(lo[0], hi[0] + 1):
        for j in range(lo[1], hi[1] + 1):
            for k in range(lo[2], hi[2] + 1):
                p = Vector((i + .5, j + .5, k + .5))
                t = max(0.0, min(1.0, (p - a).dot(ab) / L2))
                r = ra + (rb - ra) * t
                if (p - (a + ab * t)).length <= r:
                    put((i, j, k), tone, group, sid)


for sid, s in enumerate(STRANDS):
    if "scalp" in s:
        bottom_at, off, tone = s["scalp"]
        # a casca: uma camada colada na cabeça (os lados, a nuca e o alto), de bottom_at para cima
        for i in range(-1, 17):
            for j in range(-1, 17):
                for k in range(0, 17):
                    if SOLID_HEAD(i, j, k):
                        continue
                    if k < bottom_at(math.atan2(j + .5 - 8, i + .5 - 8)):
                        continue
                    # na testa a casca para na linha do cabelo (o rosto fica livre); nas têmporas desce um pouco
                    if j < 0 and k < (FRONT_LINE if 1 <= i <= 14 else FRONT_LINE - 3):
                        continue
                    put((i, j, k), tone, None, -1, True)
        continue
    pts = s["pts"]
    n = len(pts)
    r0 = (s["w"] + s["d"]) / 4
    for m in range(n - 1):
        f0, f1 = m / (n - 1), (m + 1) / (n - 1)
        ra = max(.8, r0 * (1 - (1 - s["tip"]) * f0 ** 1.2))
        rb = max(.8, r0 * (1 - (1 - s["tip"]) * f1 ** 1.2))
        raster_capsule(pts[m], pts[m + 1], ra, rb, s["tone"], s.get("group"), sid)
for sid2, (root, dirv, length, base, tone, end) in enumerate(STEPS):
    axis = max(range(3), key=lambda a: abs(dirv[a]))
    u, w = [a for a in range(3) if a != axis]
    step = dirv / abs(dirv[axis])                           # uma casa por camada no eixo da pilha
    layers = max(1, round(length * abs(dirv[axis])))
    for t in range(layers):
        f = t / max(1, layers - 1)
        size = max(end, round(base + (end - base) * f ** 1.7))
        c = root + step * t
        lo_u = math.floor(c[u] - size / 2 + .5)
        lo_w = math.floor(c[w] - size / 2 + .5)
        for du in range(int(size)):
            for dw in range(int(size)):
                q = [0, 0, 0]
                q[axis] = math.floor(c[axis])
                q[u] = lo_u + du
                q[w] = lo_w + dw
                put(tuple(q), tone + (1 if f > .6 else 0), None, 20000 + sid2)
for bid, (c, size, rot, tone, group) in enumerate(BLOBS):
    from mathutils import Euler
    inv = Euler(rot, "XYZ").to_matrix().inverted()
    h = size / 2
    rr = max(h)
    for i in range(math.floor(c.x - rr) - 1, math.ceil(c.x + rr) + 2):
        for j in range(math.floor(c.y - rr) - 1, math.ceil(c.y + rr) + 2):
            for k in range(math.floor(c.z - rr) - 1, math.ceil(c.z + rr) + 2):
                q = inv @ (Vector((i + .5, j + .5, k + .5)) - c)
                if (q.x / h.x) ** 2 + (q.y / h.y) ** 2 + (q.z / h.z) ** 2 <= 1.0:
                    put((i, j, k), -9 if tone == -9 else tone, group, 10000 + bid)

# ---- as correntes de física: os grupos das mechas compridas (Back, Left, Right) e os rabos/tranças
for v in CELLS.values():
    if v[1] in ("Back", "Left", "Right") and v[1] not in GROUPS:
        GROUPS[v[1]] = 3
CHAINS = []           # (nome, juntas em casas, de cima para baixo)
TOP = 5.0
for name, bones in GROUPS.items():
    pts = [Vector((c[0] + .5, c[1] + .5, c[2] + .5)) for c, v in CELLS.items() if v[1] == name]
    low = [p for p in pts if p.z < TOP] if name in ("Back", "Left", "Right") else pts
    if len(low) < 6:
        continue
    top = TOP if name in ("Back", "Left", "Right") else max(p.z for p in low)
    bottom = min(p.z for p in low)
    joints = []
    for n in range(bones + 1):
        z = top - (top - bottom) * n / bones
        near = [p for p in low if abs(p.z - z) < 2.5] or low
        joints.append(Vector((sum(p.x for p in near) / len(near), sum(p.y for p in near) / len(near), z)))
    CHAINS.append((name, joints))

# ---- a malha: só as faces de fora, com o tom de cada casa (a mecha, a luz de cima, as pontas mais escuras)


def solid(c):
    return c in CELLS or SOLID_HEAD(*c)


streak = {}


def tone_of(c, nrm):
    v = CELLS[c]
    base, sid, is_scalp = v[0], v[2], v[3]
    if base == -9:
        return 0                                                    # o prendedor
    if sid not in streak:
        streak[sid] = random.choice((0, 0, 0, 1, -1))
    i, j, k = c
    if is_scalp:
        # cabelo rente: grão de tons por casa (sem isso a casca parece um capacete liso)
        g = ((i * 73856093) ^ (j * 19349663) ^ (k * 83492791)) % 9
        t = base + (1 if nrm[2] > 0 else 0) + (1 if g < 2 else -1 if g > 7 else 0)
    elif nrm[2] > 0:
        t = 4 + base + streak[sid] + (1 if k >= 18 else 0)
    elif nrm[2] < 0:
        t = 1
    else:
        t = 3 + base + streak[sid] + (1 if k >= 12 else 0) - (1 if k < 0 else 0)
        below = (i, j, k - 1)
        if below not in CELLS:
            t -= 1                                                  # a ponta da mecha
        elif CELLS[below][2] != sid:
            t -= 1                                                  # o vão entre duas mechas
    return max(0, min(len(PALETTE) - 1, t))


DIRS = [((1, 0, 0), (1, 0, 0)), ((-1, 0, 0), (0, 0, 0)), ((0, 1, 0), (0, 1, 0)), ((0, -1, 0), (0, 0, 0)),
        ((0, 0, 1), (0, 0, 1)), ((0, 0, -1), (0, 0, 0))]
bm = bmesh.new()
uv = bm.loops.layers.uv.new("UV")
verts = {}
owner = {}                                                          # vértice -> grupos das casas que o tocam
FACES = 0


def vert(p):
    if p not in verts:
        verts[p] = bm.verts.new(world(Vector(p)))
    return verts[p]


for c in sorted(CELLS):
    i, j, k = c
    for nrm, off in DIRS:
        if solid((i + nrm[0], j + nrm[1], k + nrm[2])):
            continue
        axis = [a for a in range(3) if nrm[a] != 0][0]
        u, w = [a for a in range(3) if a != axis]
        base = [i, j, k]
        base[axis] += off[axis]
        quad = []
        for du, dv in ((0, 0), (1, 0), (1, 1), (0, 1)):
            q = list(base)
            q[u] += du
            q[w] += dv
            quad.append(tuple(q))
        for q in quad:
            owner.setdefault(q, set()).add(CELLS[c][1])
        f = bm.faces.new([vert(q) for q in quad])
        t = tone_of(c, nrm)
        for loop in f.loops:
            loop[uv].uv = ((t + .5) / len(PALETTE), .5)
        FACES += 1
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
order = list(verts.keys())

old = bpy.data.objects.get("Cabelo")
if old:
    bpy.data.objects.remove(old, do_unlink=True)
mesh = bpy.data.meshes.new("Cabelo")
bm.to_mesh(mesh)
bm.free()
obj = bpy.data.objects.new("Cabelo", mesh)
(rig.users_collection[0] if rig.users_collection else bpy.context.scene.collection).objects.link(obj)

# a textura (a paleta) e o material
img = bpy.data.images.get("T_Cabelo_Paleta") or bpy.data.images.new("T_Cabelo_Paleta", len(PALETTE), 1)
px = []
for col in PALETTE:
    px += [col[0] / 255, col[1] / 255, col[2] / 255, 1.0]
img.pixels[:] = px
mat = bpy.data.materials.get("M_Cabelo") or bpy.data.materials.new("M_Cabelo")
mat.use_nodes = True
nt = mat.node_tree
bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
tex = next((n for n in nt.nodes if n.type == "TEX_IMAGE"), None) or nt.nodes.new("ShaderNodeTexImage")
tex.image = img
tex.interpolation = "Closest"
nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
bsdf.inputs["Roughness"].default_value = .9
mesh.materials.append(mat)

# ---- os ossos de física (Hair_<nome>_NN filhos da cabeça) e os pesos
obj.parent = rig
head_vg = obj.vertex_groups.new(name=P + "Head")
chain_bones = {}
with bpy.context.temp_override(active_object=rig, object=rig, selected_objects=[rig], selected_editable_objects=[rig]):
    bpy.ops.object.mode_set(mode="EDIT")
    eb = rig.data.edit_bones
    for b in [b for b in eb if b.name.startswith("Hair_")]:
        eb.remove(b)
    inv = rig.matrix_world.inverted()
    for name, joints in CHAINS:
        parent = eb[P + "Head"]
        names = []
        for n, (pa, pb) in enumerate(zip(joints, joints[1:])):
            b = eb.new("Hair_%s_%02d" % (name, n + 1))
            b.head = inv @ world(pa)
            b.tail = inv @ world(pb)
            b.parent = parent
            b.use_connect = n > 0
            b.use_deform = True
            parent = b
            names.append(b.name)
        chain_bones[name] = (names, [j.z for j in joints])
    bpy.ops.object.mode_set(mode="OBJECT")
groups = {n: obj.vertex_groups.new(name=n) for names, _ in chain_bones.values() for n in names}
for index, p in enumerate(order):
    own = owner.get(p, {None})
    chained = [g for g in own if g in chain_bones]
    if len(chained) != 1 or len(own) > 1:
        head_vg.add([index], 1.0, "REPLACE")
        continue
    names, tops = chain_bones[chained[0]]
    k = p[2]
    if k >= tops[0]:
        t = min(1.0, (k - tops[0]) / 3.0)
        head_vg.add([index], t, "REPLACE")
        if t < 1:
            groups[names[0]].add([index], 1 - t, "REPLACE")
        continue
    for n in range(len(names)):
        ka, kb = tops[n], tops[n + 1]
        if kb <= k <= ka or n == len(names) - 1:
            t = max(0.0, min(1.0, (ka - k) / max(1e-3, ka - kb)))
            if n + 1 < len(names):
                groups[names[n]].add([index], 1 - t, "REPLACE")
                groups[names[n + 1]].add([index], t, "REPLACE")
            else:
                groups[names[n]].add([index], 1.0, "REPLACE")
            break
mod = obj.modifiers.new("Armature", "ARMATURE")
mod.object = rig
log("%d mechas, %d blocos soltos, %d casas, %d faces, %d vértices, correntes: %s" % (
    sum(1 for s in STRANDS if "pts" in s), len(BLOBS), len(CELLS), FACES, len(mesh.vertices),
    ", ".join("%s (%d ossos)" % (n, len(b[0])) for n, b in chain_bones.items()) or "nenhuma"))

# ---- os tons para o jogo (iguais aos do MakeBlockHair.py)


def linear(c):
    c /= 255.0
    return c / 12.92 if c <= .04045 else ((c + .055) / 1.055) ** 2.4


def light(col):
    return .2126 * linear(col[0]) + .7152 * linear(col[1]) + .0722 * linear(col[2])


BASE_TONE = (120, 64, 38)
tones = bpy.data.images.get("T_Cabelo_Tons") or bpy.data.images.new("T_Cabelo_Tons", len(PALETTE), 1)
tones.colorspace_settings.name = "Non-Color"
px = []
for col in PALETTE:
    v = min(1.0, light(col) / light(BASE_TONE) / 2)
    px += [v, v, v, 1.0]
tones.pixels[:] = px

bpy.context.view_layer.update()
if SAVE:
    for o in list(bpy.context.view_layer.objects):
        if o:
            o.select_set(o in (rig, obj))
if GAME:
    os.makedirs(GAME_OUT, exist_ok=True)
    tones.filepath_raw = os.path.join(GAME_OUT, "T_Cabelo_Tons.png")
    tones.file_format = "PNG"
    tones.save()
    bpy.context.view_layer.objects.active = rig
    path = os.path.join(GAME_OUT, "Hair_%s.glb" % STYLE)
    bpy.ops.export_scene.gltf(filepath=path, use_selection=True, export_format="GLB", export_animations=False,
                              export_skins=True, export_apply=False, export_yup=True, export_def_bones=False)
    log("exportado para o jogo", path)
elif SAVE:
    with bpy.context.temp_override(active_object=rig, object=rig, selected_objects=[rig, obj],
                                   selected_editable_objects=[rig, obj]):
        bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, "cabelo_%s.fbx" % STYLE), use_selection=True,
                                 object_types={"ARMATURE", "MESH"}, path_mode="COPY", embed_textures=True,
                                 add_leaf_bones=False, bake_anim=False, apply_scale_options="FBX_SCALE_ALL")
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "cabelo_%s.blend" % STYLE), copy=True)
    log("exportado", os.path.join(OUT, "cabelo_%s.fbx" % STYLE))
