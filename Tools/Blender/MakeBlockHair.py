"""Cabelos separados do humano de blocos (01/11): blocos de meio pixel da cabeça (a mesma grade 16 x 16 do rosto,
Tools/MakeBlockFace.py), feitos por estilo, que o jogo veste por cima do corpo e pinta na cor escolhida no menu.

Estilos (STYLES):
- liso_curto: o "Cabelo Liso Curto" da referência (volumoso, franja em mechas acima das sobrancelhas, laterais até a
  orelha, nuca coberta). Sem física.
- rabo_cavalo_alto: o "Rabo de Cavalo Alto" (versão feminina): o cabelo puxado para trás, franja aberta no meio, mechas
  na frente das orelhas até o ombro e o rabo preso no alto da nuca, caindo até o meio das costas. O rabo tem uma corrente
  de ossos (Hair_Tail_01..05, filhos de mixamorig:Head) para a física do jogo.

No Blender aberto (MCP), com modelando ainda/personagem_base_rig.blend aberto:
    STYLE = "rabo_cavalo_alto"; exec(open(r".../Tools/Blender/MakeBlockHair.py", encoding="utf-8").read())
Sem janela: blender -b "modelando ainda/personagem_base_rig.blend" -P Tools/Blender/MakeBlockHair.py -- <estilo>
Para o jogo: blender -b --factory-startup -P Tools/Blender/MakeBlockHair.py -- jogo <estilo>
    (no esqueleto do Hero_humano.glb que o jogo usa; sai Data/Heroes/humano_Mixamo/Hair/Hair_<estilo>.glb e a
    T_Cabelo_Tons.png; Tools/ImportBlockHair.py leva para a Unreal)

1. a caixa da cabeça sai dos vértices presos só ao osso mixamorig:Head, então o cabelo encaixa na cabeça do rig;
2. o cabelo é um conjunto de blocos (i, j, k) por fora da cabeça (16 x 16 x 16 casas): só as faces de fora viram malha;
3. cada face pega uma cor de uma paleta de 8 marrons: mais claro em cima, mais escuro embaixo e nas pontas, com mechas
   mais claras; no jogo vale só o tom (T_Cabelo_Tons), a cor vem do herói;
4. peso 1 no mixamorig:Head (o rabo: nos ossos da corrente, pela altura); sai modelando ainda/cabelo_<estilo>.fbx e
   .blend, ou o glb do jogo.
"""
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
STYLE = next((a for a in ARGS if a != "jogo"), None) or globals().get("STYLE", "liso_curto")
GAME_OUT = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "Hair")
P = "mixamorig:"
random.seed(11)

# a paleta (da referência): sombra funda, escuros, médios, claros e o brilho
PALETTE = [(52, 30, 22), (70, 41, 28), (86, 52, 35), (100, 61, 41), (114, 71, 47), (130, 82, 54), (148, 96, 63),
           (168, 112, 74)]


def log(*parts):
    print("CABELO", STYLE, " ".join(str(p) for p in parts))


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

# ---- a caixa da cabeça (os vértices só do osso da cabeça)
rig = next(o for o in bpy.data.objects if o.type == "ARMATURE")
body = next(o for o in bpy.data.objects if o.type == "MESH" and o.parent == rig and not o.name.startswith("Cabelo"))
head_group = body.vertex_groups[P + "Head"].index
pts = [body.matrix_world @ v.co for v in body.data.vertices
       if any(g.group == head_group and g.weight > .99 for g in v.groups)]
x0, x1 = min(p.x for p in pts), max(p.x for p in pts)
y0, y1 = min(p.y for p in pts), max(p.y for p in pts)
z0, z1 = min(p.z for p in pts), max(p.z for p in pts)
CX, CY, CZ = (x1 - x0) / 16, (y1 - y0) / 16, (z1 - z0) / 16      # uma casa (meio pixel da textura da cabeça)
log("cabeça %.3f x %.3f x %.3f m, casa %.4f m" % (x1 - x0, y1 - y0, z1 - z0, CX))

# ---- os blocos: i pela largura (0 = -X), j da frente (0 = rosto) para trás, k de baixo (0) para cima; a cabeça é 0..15
hair = set()
tail = set()            # os blocos do rabo (pesos na corrente de ossos, cor própria)
tie = set()             # o prendedor
shade = set()           # os vãos das tranças (2 tons mais escuros)
CHAIN = []              # o caminho do rabo: (j, k) de cada junta, de cima para baixo (a última é a ponta)
CHAINS = []             # as correntes de física: (nome, juntas (i, j, k) de cima para baixo, blocos que ela move)
THIN = 1.0              # a espessura das casas de fora da cabeça (o raspado: bem rente, senão vira capacete)
LIGHTER = 0             # tons a mais (o raspado é mais claro: a pele aparece por baixo)


def fill(i, j, k, into=None):
    hair.add((i, j, k))
    if into is not None:
        into.add((i, j, k))


def jag(n, lo, hi):
    """Pontas irregulares em grupos de 2 casas (as mechas)."""
    out = []
    for g in range(0, n, 2):
        v = random.randint(lo, hi)
        out += [v, v]
    return out[:n]


# ---- peças para montar os estilos


def cap(L=2, Lf=1, thick=2):
    """O topo: thick casas em cima da cabeça, L casas para os lados e para trás, Lf para a frente; cantos arredondados."""
    for i in range(-L, 16 + L):
        for j in range(-Lf, 16 + L):
            corner = i in (-L, 15 + L) and j in (-Lf, 15 + L)
            for t in range(thick):
                if corner and (thick == 1 or t == thick - 1):
                    continue
                fill(i, j, 16 + t)


def wrap(bottom, L=2, Lf=1, top=16):
    """Em volta da cabeça (frente, lados e nuca): cada coluna (i, j) de fora da cabeça desce até bottom(i, j) (None = sem
    cabelo ali). Os cantos de fora ficam vazios (arredondado)."""
    for i in range(-L, 16 + L):
        for j in range(-Lf, 16 + L):
            if 0 <= i < 16 and 0 <= j < 16:
                continue
            if i in (-L, 15 + L) and j in (-Lf, 15 + L):
                continue
            b = bottom(i, j)
            if b is None:
                continue
            for k in range(b, top):
                fill(i, j, k)


def zone(i, j):
    """Onde fica a coluna: frente, esquerda (-X), direita ou nuca."""
    return "frente" if j < 0 else "nuca" if j > 15 else "esquerda" if i < 0 else "direita" if i > 15 else "cabeça"


def puff(k, i0, i1, j0, j1, prob, size=2):
    """Tufos de size x size casas numa camada (o volume por cima)."""
    for i in range(i0, i1, size):
        for j in range(j0, j1, size):
            if random.random() < prob:
                for di in range(size):
                    for dj in range(size):
                        fill(i + di, j + dj, k)


def curls(count, kmin, size=2):
    """Cachos: bolinhas de size casas grudadas por fora do cabelo que já existe (para cima ou para o lado de fora)."""
    base = sorted(c for c in hair if c[2] >= kmin)
    for _ in range(count):
        i, j, k = random.choice(base)
        di = -1 if i < 4 else 1 if i > 11 else 0
        dj = -1 if j < 2 else 1 if j > 13 else 0
        up = random.random() < .6 or (di == 0 and dj == 0)

        def start(v, d):
            # para fora (d = -1 ou 1) ou centrado na casa (d = 0)
            return v - size if d < 0 else v + 1 if d > 0 else v - random.randint(0, size - 1)
        if up:
            oi, oj, ok = i - random.randint(0, size - 1), j - random.randint(0, size - 1), k + 1
        else:
            oi, oj, ok = start(i, di), start(j, dj), k - random.randint(0, size - 1)
        for a in range(size):
            for b in range(size):
                for c in range(size):
                    fill(oi + a, oj + b, ok + c)


def spike(i0, j0, k0, width, depth, height, lean=0, lean_i=0):
    """Um espeto (moicano, espetado): afina subindo e pode inclinar para trás (lean) ou para o lado (lean_i)."""
    for t in range(height):
        f = t / max(1, height - 1)
        w = max(1, round(width * (1 - .75 * f)))
        d = max(1, round(depth * (1 - .6 * f)))
        si = i0 + (width - w) // 2 + round(lean_i * f)
        sj = j0 + (depth - d) // 2 + round(lean * f)
        for i in range(si, si + w):
            for j in range(sj, sj + d):
                fill(i, j, k0 + t)


def chain(name, cells, top=5, bones=3):
    """Uma corrente de física para os blocos dados (os de baixo de top): juntas no meio deles, de top até a ponta."""
    cells = {c for c in cells if c[2] < top}
    if not cells:
        return
    ci = sum(c[0] for c in cells) / len(cells)
    cj = sum(c[1] for c in cells) / len(cells)
    kmin = min(c[2] for c in cells)
    points = [(ci, cj, top - (top - kmin) * n / bones) for n in range(bones + 1)]
    CHAINS.append((name, points, cells))


def long_chains(top=5):
    """Cabelo comprido: três correntes (nuca, esquerda, direita) com o que passa de top para baixo."""
    low = [c for c in hair if c[2] < top]
    back = [c for c in low if c[1] >= 12]
    left = [c for c in low if c[1] < 12 and c[0] < 8]
    right = [c for c in low if c[1] < 12 and c[0] >= 8]
    chain("Back", back, top)
    chain("Left", left, top)
    chain("Right", right, top)


def style_liso_curto():
    # o topo: 2 casas de espessura e um volume por cima, com os cantos arredondados
    for i in range(-2, 18):
        for j in range(-2, 18):
            corner = i in (-2, 17) and j in (-2, 17)
            fill(i, j, 16)
            if not corner:
                fill(i, j, 17)
    for i in range(-1, 17):
        for j in range(-1, 17):
            edge = i in (-1, 16) or j in (-1, 16)
            if not edge or random.random() < .55:
                fill(i, j, 18)
    for i in range(0, 16, 2):
        for j in range(1, 16, 2):
            if random.random() < .5:
                for di in (0, 1):
                    for dj in (0, 1):
                        fill(i + di, j + dj, 19)
    # as laterais: até a costeleta na frente, até a orelha no meio e mais baixo atrás
    side_jag = jag(20, 0, 2)
    for i in (-2, -1, 16, 17):
        for j in range(-2, 18):
            bottom = 9 if j <= 1 else 7 if j <= 5 else 4
            bottom += side_jag[j + 2] + (1 if i in (-2, 17) else 0)       # a camada de fora um pouco mais curta
            for k in range(bottom, 16):
                fill(i, j, k)
    # a nuca: até perto do pescoço
    back_jag = jag(20, 0, 2)
    for i in range(-2, 18):
        for j in (16, 17):
            bottom = 2 + back_jag[i + 2] + (1 if j == 17 else 0)
            for k in range(bottom, 16):
                fill(i, j, k)
    # a franja (j = -1, 1 casa na frente do rosto): mechas que param acima das sobrancelhas (linha 6 do rosto = k 9;
    # os olhos começam na linha 8 = k 7), mais compridas nas têmporas
    fringe = {-2: 6, -1: 6, 0: 8, 1: 8, 2: 11, 3: 10, 4: 12, 5: 11, 6: 10, 7: 12, 8: 12, 9: 10, 10: 11, 11: 12, 12: 10,
              13: 11, 14: 8, 15: 8, 16: 6, 17: 6}
    for i, bottom in fringe.items():
        for k in range(bottom, 16):
            fill(i, -1, k)
        # o volume da franja: uma segunda camada em cima, em mechas alternadas
        for k in range(15 if (i // 2) % 2 else 13, 16):
            fill(i, -2, k)


def style_rabo_cavalo_alto():
    # o topo liso (puxado para trás): 2 casas, os cantos arredondados, e um pouco de volume atrás, onde junta
    for i in range(-2, 18):
        for j in range(-2, 18):
            corner = i in (-2, 17) and j in (-2, 17)
            fill(i, j, 16)
            if not corner:
                fill(i, j, 17)
    for i in range(1, 15):
        for j in range(9, 17):
            fill(i, j, 18)
    # as laterais puxadas para trás: cobrem acima da orelha
    for i in (-2, -1, 16, 17):
        for j in range(2, 18):
            for k in range(6 + (1 if i in (-2, 17) else 0), 16):
                fill(i, j, k)
    # a nuca presa: só até o meio da cabeça de trás
    for i in range(-2, 18):
        for j in (16, 17):
            for k in range(5 + (1 if j == 17 else 0), 16):
                fill(i, j, k)
    # as mechas da frente (na frente das orelhas, até o ombro): 2 casas de largura, um pouco mais curtas por fora
    lock_jag = jag(4, 0, 1)
    for n, i in enumerate((-2, -1, 16, 17)):
        for j in range(-2, 2):
            for k in range(-5 + lock_jag[n] + (1 if j == -2 else 0), 16):
                fill(i, j, k)
    # a franja aberta no meio: curta no meio, descendo para os lados como uma cortina (deixa os olhos e as sobrancelhas)
    fringe = {0: 2, 1: 6, 2: 8, 3: 10, 4: 11, 5: 12, 6: 13, 7: 14, 8: 14, 9: 13, 10: 12, 11: 11, 12: 10, 13: 8, 14: 6,
              15: 2}
    for i, bottom in fringe.items():
        for k in range(bottom, 16):
            fill(i, -1, k)
        for k in range(max(bottom + 2, 13), 16):
            fill(i, -2, k)
    # o prendedor: no alto da nuca, 4 x 2 x 4 casas
    for i in range(6, 10):
        for j in (18, 19):
            for k in range(12, 16):
                fill(i, j, k, tie)
    # o rabo: sai do prendedor para trás e cai até o meio das costas, afinando
    CHAIN[:] = [(21, 14), (22, 8), (21, 2), (20, -4), (19, -10), (19, -15)]
    for k in range(-15, 17):
        # o centro do rabo nesta altura (entre as juntas) e a largura (6 casas em cima, 2 na ponta)
        for (ja, ka), (jb, kb) in zip(CHAIN, CHAIN[1:]):
            if kb <= k <= ka:
                t = (ka - k) / max(1, ka - kb)
                jc = round(ja + (jb - ja) * t)
                break
        else:
            jc = CHAIN[0][0]
        f = (16 - k) / 31.0                                     # 0 em cima, 1 na ponta
        half_i = 3 if f < .5 else 2 if f < .85 else 1
        half_j = 3 if f < .25 else 2 if f < .7 else 1         # a base do rabo mais cheia (o volume do rabo alto)
        if k > 15:
            half_i, half_j = 2, 2
        for i in range(8 - half_i, 8 + half_i):
            for j in range(jc - half_j, jc + half_j):
                if j >= 20 or k < 12:
                    fill(i, j, k, tail)
    # pontas irregulares embaixo
    for i in range(5, 11):
        if random.random() < .5:
            for j in range(16, 24):
                for k in (-15, -14):
                    if (i, j, k) in hair:
                        hair.discard((i, j, k))
                        tail.discard((i, j, k))
    CHAINS.append(("Tail", [(8, j, k) for j, k in CHAIN], set(tail)))


# ---- peças dos femininos: tubos (rabos, marias-chiquinhas), tranças e coques


def tube(points, r0, r1, into=None, braid=False):
    """Um tubo ao longo das juntas (i, j, k), raio r0 em cima e r1 na ponta. braid: a trança (o tubo vai e volta de
    lado a cada 2 casas e os vãos ficam mais escuros)."""
    import math
    segs = list(zip(points, points[1:]))
    lengths = [math.dist(a, b) for a, b in segs]
    total = sum(lengths)
    done = 0.0
    n = 0
    for (a, b), length in zip(segs, lengths):
        steps = max(1, int(length * 2))
        for s_ in range(steps + 1):
            t = s_ / steps
            f = (done + length * t) / total
            r = r0 + (r1 - r0) * f
            c = [a[x] + (b[x] - a[x]) * t for x in range(3)]
            if braid:
                c[0] += .8 if (n // 4) % 2 else -.8
            n += 1
            for i in range(math.floor(c[0] - r), math.ceil(c[0] + r) + 1):
                for j in range(math.floor(c[1] - r), math.ceil(c[1] + r) + 1):
                    for k in range(math.floor(c[2] - .5), math.ceil(c[2] + .5)):
                        if (i + .5 - c[0]) ** 2 + (j + .5 - c[1]) ** 2 <= r * r:
                            fill(i, j, k, into)
                            if braid and n % 4 == 0:
                                shade.add((i, j, k))
        done += length


def bun(ci, cj, ck, r):
    """Um coque: uma bola de casas de raio r."""
    import math
    for i in range(math.floor(ci - r), math.ceil(ci + r) + 1):
        for j in range(math.floor(cj - r), math.ceil(cj + r) + 1):
            for k in range(math.floor(ck - r), math.ceil(ck + r) + 1):
                if (i + .5 - ci) ** 2 + (j + .5 - cj) ** 2 + (k + .5 - ck) ** 2 <= r * r:
                    fill(i, j, k)


def band(ci, cj, ck, r=1.6):
    """O prendedor (o tom mais escuro)."""
    import math
    for i in range(math.floor(ci - r), math.ceil(ci + r) + 1):
        for j in range(math.floor(cj - r), math.ceil(cj + r) + 1):
            for k in (math.floor(ck), math.floor(ck) + 1):
                if (i + .5 - ci) ** 2 + (j + .5 - cj) ** 2 <= r * r:
                    fill(i, j, k, tie)


CURTAIN = {0: 2, 1: 6, 2: 8, 3: 10, 4: 11, 5: 12, 6: 13, 7: 14, 8: 14, 9: 13, 10: 12, 11: 11, 12: 10, 13: 8, 14: 6, 15: 2}
BLUNT = {i: 8 for i in range(1, 15)} | {0: 4, 15: 4}               # a franja reta até os olhos (cobre a sobrancelha)


def long_base(back, side, lock, bangs, jag_hi=2, Lf=1, puffs=.5):
    """O comprido: topo, franja (bangs[i] = onde termina), mechas da frente (lock), lados e nuca descendo até side/back."""
    cap(Lf=Lf)
    puff(18, 0, 16, 1, 16, puffs)
    fj, sj, bj = jag(22, 0, 1), jag(22, 0, jag_hi), jag(22, 0, jag_hi)

    def bottom(i, j):
        z = zone(i, j)
        if z == "frente":
            if j < -1:
                return 14
            return lock + sj[i % 20] if i < 0 or i > 15 else bangs[i] + fj[i + 2]
        if z == "nuca":
            return back + bj[i + 2]
        return (lock if j < 2 else side) + sj[j + 2]
    wrap(bottom, Lf=Lf)
    for i, b in bangs.items():
        for k in range(max(b + 2, 13), 16):
            fill(i, -2, k)


def waves(kmin, kmax):
    """As ondas: a camada de fora vai e volta a cada 3 casas."""
    for (i, j, k) in sorted(hair):
        if kmin <= k <= kmax and (k // 3) % 2 == 0:
            z = zone(i, j)
            if z == "esquerda" and i == -2:
                fill(-3, j, k)
            elif z == "direita" and i == 17:
                fill(18, j, k)
            elif z == "nuca" and j == 17:
                fill(i, 18, k)


# ---- os femininos da referência (1 a 15; o 5 é o rabo de cavalo alto)


def style_liso_longo():
    long_base(-11, -10, -8, CURTAIN)
    long_chains(top=2)


def style_ondulado_longo():
    long_base(-11, -10, -8, CURTAIN, jag_hi=3)
    waves(-11, 4)
    curls(25, 15)
    long_chains(top=2)


def style_cacheado_longo():
    long_base(-10, -9, -7, CURTAIN, jag_hi=3, puffs=.8)
    curls(140, -9)
    long_chains(top=2)


def style_meio_preso():
    # meio preso: o comprido solto e um coque pequeno no alto da nuca
    long_base(-10, -9, -7, CURTAIN)
    bun(8, 20, 13, 3.8)
    band(8, 17.5, 12.5, 2.2)
    long_chains(top=2)


def style_rabo_cavalo_baixo():
    # puxado para trás, preso embaixo na nuca, o rabo caindo por cima do ombro esquerdo para a frente
    cap()
    puff(18, 0, 16, 2, 16, .4)

    def bottom(i, j):
        z = zone(i, j)
        if z == "frente":
            return 9 if i < 0 or i > 15 else CURTAIN[i]
        return 3 if z == "nuca" else 6
    wrap(bottom)
    band(5, 17.5, 2.5)
    path = [(5, 18.5, 2), (1, 16, -2), (-2.5, 10, -4), (-3, 4, -7), (-2.5, 1, -11), (-2, 0.5, -15)]
    cells = set()
    tube(path, 2.6, 1.2, cells)
    CHAINS.append(("Tail", path, cells))


def style_maria_chiquinha():
    # duas chiquinhas altas dos lados, com prendedor, caindo até o peito
    long_base(4, 4, 2, CURTAIN)
    for side, i in (("L", -3.5), ("R", 19.5)):
        band(i, 8, 12.5)
        path = [(i, 8.5, 12), (i - 1.5 if side == "L" else i + 1.5, 10, 5), (i - 1 if side == "L" else i + 1, 10, -3),
                (i - .5 if side == "L" else i + .5, 9, -10), (i, 9, -14)]
        cells = set()
        tube(path, 2.8, 1.4, cells)
        CHAINS.append(("Pig" + side, path, cells))


def style_bob_curto():
    # chanel: tudo na altura do queixo, a franja aberta, um pouco de volume embaixo
    cap()
    puff(18, 0, 16, 1, 16, .45)
    bj = jag(22, 0, 1)

    def bottom(i, j):
        z = zone(i, j)
        if z == "frente":
            if j < -1:
                return 14
            return 0 if i < 0 or i > 15 else CURTAIN[i]
        return (0 if z == "nuca" else -1) + bj[(i + j) % 20]
    wrap(bottom)
    for (i, j, k) in sorted(hair):
        if k <= 2:
            z = zone(i, j)
            if z == "esquerda" and i == -2:
                fill(-3, j, k)
            elif z == "direita" and i == 17:
                fill(18, j, k)


def style_camadas_medias():
    long_base(-7, -6, -5, CURTAIN, jag_hi=3, puffs=.7)
    waves(-7, 5)
    curls(45, -6)
    long_chains(top=2)


def style_franja_longa():
    long_base(-12, -11, -9, BLUNT)
    long_chains(top=2)


def style_curto_baguncado():
    cap()
    puff(18, -1, 17, -1, 17, .7)
    puff(19, 0, 16, 1, 16, .3)
    fj, sj = jag(22, 0, 2), jag(22, 0, 4)
    bangs = {i: 11 + fj[i + 2] for i in range(16)}                 # acima da sobrancelha (encostada parecia sobrancelha grossa)

    def bottom(i, j):
        z = zone(i, j)
        if z == "frente":
            return 3 + sj[i % 20] if i < 0 or i > 15 else bangs[i]
        return (0 if z == "nuca" else 2) + sj[(i + j) % 20]
    wrap(bottom)
    curls(45, 4)


def style_coque_despojado():
    # coque bagunçado no alto, o resto puxado, mechas soltas na frente do rosto
    cap()

    def bottom(i, j):
        z = zone(i, j)
        if z == "frente":
            return 0 if i in (-1, 0, 15, 16) else 6 if i < 0 or i > 15 else CURTAIN[i] + 1
        return 5 if z == "nuca" else 7
    wrap(bottom)
    bun(8, 9, 20.5, 4.2)
    curls(30, 19)


def style_tranca_lateral():
    # comprido atrás e uma trança grossa do lado direito, por cima do ombro, na frente
    long_base(-10, -2, 2, CURTAIN)
    long_chains(top=2)
    path = [(17, 1.5, 6), (17.5, 1, 0), (16.5, 0.5, -6), (15.5, 0.5, -12), (15, 0.5, -15)]
    cells = set()
    tube(path, 2.2, 1.6, cells, braid=True)
    band(15, 0.5, -15.5, 1.4)
    CHAINS.append(("Braid", path, cells))


def style_dois_coques():
    # dois coques no alto e o resto solto até o peito
    long_base(-8, -7, -6, CURTAIN)
    for ci in (2.5, 13.5):
        bun(ci, 6, 19.5, 3.3)
    long_chains(top=2)


def style_trancas_duplas():
    # duas tranças saindo de trás das orelhas, caindo na frente dos ombros
    long_base(4, 3, 2, CURTAIN)
    for side, i in (("L", -2), ("R", 18)):
        di = -.5 if side == "L" else .5
        path = [(i, 3, 5), (i + di, 1.5, -1), (i + di * 2, 1, -7), (i + di * 2, 1, -13), (i + di * 2, 1, -15)]
        cells = set()
        tube(path, 2.0, 1.5, cells, braid=True)
        band(i + di * 2, 1, -15.5, 1.3)
        CHAINS.append(("Braid" + side, path, cells))


# ---- os masculinos da referência (6 a 15)


def style_longo_desfiado():
    # comprido e repicado até o ombro, franja bagunçada e mechas na frente do rosto; física leve
    cap(L=2, Lf=2)
    puff(18, -1, 17, -1, 17, .7)
    puff(19, 0, 16, 1, 16, .35)
    fj, sj, bj = jag(22, 0, 3), jag(22, 0, 3), jag(22, 0, 3)

    def bottom(i, j):
        z = zone(i, j)
        if z == "frente":
            if j == -2:
                return 14 + (i % 3 == 0)
            return -3 + fj[i + 3] if i < 1 or i > 14 else 10 + fj[i + 3]
        if z == "nuca":
            return -7 + bj[i + 3]
        return (-4 if j < 3 else -6) + sj[j + 3]
    wrap(bottom, L=2, Lf=2)
    curls(40, 10)
    long_chains()


def fan_spike(angle, length, width=4, thick=6):
    """Um espeto saindo do meio da cabeça na direção do ângulo (0 = para cima, 90 = para trás, -45 = para a frente e
    para cima), afinando até a ponta; no meio da largura (colunas 6-9)."""
    import math
    a = math.radians(angle)
    dj, dk = math.sin(a), math.cos(a)
    r = min(8.5 / abs(dj) if abs(dj) > 1e-3 else 99, 8.5 / abs(dk) if abs(dk) > 1e-3 else 99)
    for step in range(int(length * 2) + 1):
        t = step / 2
        f = t / length
        w = max(2, round(width * (1 - .5 * f)))
        th = max(1, round(thick * (1 - .9 * f)))              # largo na base (os espetos se juntam) e pontudo
        pj, pk = 7.5 + dj * (r + t), 7.5 + dk * (r + t)
        for i in range(8 - (w + 1) // 2, 8 - (w + 1) // 2 + w):
            for o in range(th):
                off = o - (th - 1) / 2
                fill(i, round(pj - dk * off), round(pk + dj * off))


def style_moicano_punk():
    # a crista no meio (4 casas): um leque de espetos grossos da testa até a nuca; os lados raspados
    for j in range(-1, 18):
        for i in range(6, 10):
            fill(i, j, 16)
    for i in range(6, 10):
        for k in range(13, 16):
            fill(i, -1, k)
        for j in (16, 17):
            for k in range(4, 16):
                fill(i, j, k)
    for angle, length in ((-42, 6), (-14, 8), (14, 8), (42, 8), (70, 7), (98, 6), (126, 5)):
        fan_spike(angle, length)


def style_risca_lateral():
    # risca do lado esquerdo, volume penteado para a direita caindo na testa
    cap()
    puff(18, -1, 17, -1, 17, .6)
    puff(19, 4, 17, 0, 14, .5)
    sj, bj = jag(22, 0, 2), jag(22, 0, 2)

    def bottom(i, j):
        z = zone(i, j)
        if z == "frente":
            return 8 if i > 15 else 9 if i < 0 else 13 if i < 4 else 13 - (i - 4) // 3
        if z == "nuca":
            return 3 + bj[i + 2]
        return (8 if j < 3 else 6) + sj[j + 2]
    wrap(bottom)
    for i in range(6, 16):
        for k in range(13, 16):
            fill(i, -2, k)


def style_meio_dividido():
    # repartido no meio, as duas cortinas descendo para as têmporas, bagunçado em cima
    cap()
    puff(18, -1, 17, -1, 17, .65)
    puff(19, 0, 16, 1, 16, .3)
    curtain = {0: 6, 1: 7, 2: 8, 3: 10, 4: 11, 5: 12, 6: 13, 7: 14, 8: 14, 9: 13, 10: 12, 11: 11, 12: 10, 13: 8, 14: 7,
               15: 6}
    fj, sj, bj = jag(22, 0, 1), jag(22, 0, 2), jag(22, 0, 2)

    def bottom(i, j):
        z = zone(i, j)
        if z == "frente":
            return 5 if i < 0 or i > 15 else curtain[i] + fj[i + 2]
        if z == "nuca":
            return 3 + bj[i + 2]
        return 6 + sj[j + 2]
    wrap(bottom)
    curls(14, 17)


def style_penteado_tras():
    # penteado para trás: sem franja, a frente alta (topete) e os lados rentes
    cap(L=1, Lf=1)
    puff(18, -1, 17, -1, 10, .9)
    for i in range(1, 15):
        for j in range(-1, 5):
            fill(i, j, 19 if j < 3 else 18)
    for i in range(0, 16):
        for k in (16, 17, 18):
            fill(i, -2, k)

    def bottom(i, j):
        z = zone(i, j)
        return 14 if z == "frente" else 4 if z == "nuca" else 8
    wrap(bottom, L=1, Lf=1)


def style_undercut():
    # o de cima comprido e jogado para o lado; os lados e a nuca raspados
    cap()
    puff(18, -1, 17, -1, 17, .9)
    puff(19, 2, 17, 0, 15, .6)
    puff(20, 6, 17, 1, 13, .3)

    def bottom(i, j):
        z = zone(i, j)
        if z == "frente":
            return 14 if i < 6 else max(9, 12 - (i - 6) // 3)
        return 11 if z == "nuca" else 13
    wrap(bottom)
    for i in range(4, 16):
        for k in range(14, 17):
            fill(i, -2, k)
    # o que cai por cima do lado direito
    for j in range(0, 11):
        for k in range(13, 18):
            fill(18, j, k)


def style_mullet():
    # curto na frente e nos lados, comprido atrás; física leve no comprido
    cap()
    puff(18, -1, 17, -1, 17, .7)
    fj, sj, bj = jag(22, 0, 2), jag(22, 0, 3), jag(22, 0, 3)

    def bottom(i, j):
        z = zone(i, j)
        if z == "frente":
            return 7 if i < 0 or i > 15 else 10 + fj[i + 2]
        if z == "nuca":
            return -6 + bj[i + 2]
        return (6 if j < 4 else 4 if j < 10 else -4) + sj[j + 2]
    wrap(bottom)
    curls(20, 17)
    chain("Back", [c for c in hair if c[2] < 4 and c[1] >= 8], top=4)


def style_raspado():
    # máquina: uma camada fina colada na cabeça (THIN), mais clara, com a linha do cabelo na testa, a costeleta e a nuca
    global THIN, LIGHTER
    THIN, LIGHTER = .28, 1
    cap(L=1, Lf=1, thick=1)

    def bottom(i, j):
        z = zone(i, j)
        if z == "frente":
            return 13 if 1 < i < 14 else 14                     # as entradas nos cantos da testa
        if z == "nuca":
            return 3
        return 6 if j in (1, 2) else 10 if j < 1 else 7         # a costeleta na frente da orelha
    wrap(bottom, L=1, Lf=1)
    # os cantos de cima fechados (com a camada fina não ficam quadrados)
    for i in (-1, 16):
        for j in (-1, 16):
            fill(i, j, 16)


def style_espetado_curto():
    # curto e espetado: colunas de 2 x 2 de alturas diferentes, a franja em pontas
    cap()
    for i in range(-1, 17, 2):
        for j in range(-1, 17, 2):
            h = random.randint(0, 3) + (1 if 2 < i < 13 and 2 < j < 13 else 0)
            for di in (0, 1):
                for dj in (0, 1):
                    for k in range(18, 18 + h):
                        fill(i + di, j + dj, k)
    fj, sj, bj = jag(22, 0, 2), jag(22, 0, 2), jag(22, 0, 2)

    def bottom(i, j):
        z = zone(i, j)
        if z == "frente":
            return 9 if i < 0 or i > 15 else 12 + fj[i + 2]
        if z == "nuca":
            return 5 + bj[i + 2]
        return 8 + sj[j + 2]
    wrap(bottom)
    for i in range(0, 16, 3):
        for k in (15, 16):
            fill(i, -2, k)


def style_degrade_cacheado():
    # cacheado em cima, degradê nos lados (curto embaixo, uma casa só)
    cap()

    def bottom(i, j):
        z = zone(i, j)
        if z == "frente":
            return 12
        return 9 if z == "nuca" else 11
    wrap(bottom)
    # o degradê: uma casa rente por baixo do volume

    def fade(i, j):
        z = zone(i, j)
        return None if z == "frente" else 3 if z == "nuca" else 6
    wrap(fade, L=1, Lf=1, top=12)
    puff(18, -1, 17, -1, 17, .8)
    curls(70, 12)
    for i in range(0, 16, 2):
        if random.random() < .7:
            for di in (0, 1):
                for k in (12, 13):
                    fill(i + di, -2, k)


STYLES = {"liso_curto": style_liso_curto, "rabo_cavalo_alto": style_rabo_cavalo_alto,
          "longo_desfiado": style_longo_desfiado, "moicano_punk": style_moicano_punk, "risca_lateral": style_risca_lateral,
          "meio_dividido": style_meio_dividido, "penteado_tras": style_penteado_tras, "undercut": style_undercut,
          "mullet": style_mullet, "raspado": style_raspado, "espetado_curto": style_espetado_curto,
          "degrade_cacheado": style_degrade_cacheado,
          "liso_longo": style_liso_longo, "ondulado_longo": style_ondulado_longo, "cacheado_longo": style_cacheado_longo,
          "meio_preso": style_meio_preso, "rabo_cavalo_baixo": style_rabo_cavalo_baixo,
          "maria_chiquinha": style_maria_chiquinha, "bob_curto": style_bob_curto, "camadas_medias": style_camadas_medias,
          "franja_longa": style_franja_longa, "curto_baguncado": style_curto_baguncado,
          "coque_despojado": style_coque_despojado, "tranca_lateral": style_tranca_lateral,
          "dois_coques": style_dois_coques, "trancas_duplas": style_trancas_duplas}
STYLES[STYLE]()


def solid(c):
    i, j, k = c
    return c in hair or (0 <= i < 16 and 0 <= j < 16 and 0 <= k < 16)


# ---- a malha: só as faces de fora
streak = {i: random.random() < .3 for i in range(-2, 18)}      # mechas mais claras (por coluna)
clumps = {}


def clump(i, j, k):
    """A variação de cor por tufo de 2 x 2 x 2 casas (cor por casa fica granulada demais)."""
    key = (i // 2, j // 2, k // 2)
    if key not in clumps:
        clumps[key] = random.choice((-1, 0, 0, 0, 1))
    return clumps[key]


def tone(c, n):
    i, j, k = c
    if c in tie:
        return 0                                                    # o prendedor: o tom mais escuro
    if c in shade:
        return 1                                                    # o vão da trança
    if n[2] > 0:
        t = 4 + clump(i, j, k) + (1 if k >= 19 else 0) + (1 if streak.get(i) else 0)
    elif n[2] < 0:
        t = 3 if j == -2 else 2 if c in tail else 0 if k > 4 else 1   # embaixo da franja e do rabo: sem risco escuro
    elif c in tail or k < 2:
        # o rabo e as mechas compridas: tom médio com mechas, mais escuro na ponta
        t = 3 + clump(i, j, k) + (1 if streak.get(i if n[0] == 0 else j % 16) else 0)
        if not solid((i, j, k - 1)):
            t -= 1
    else:
        # as faces de lado: mais escuras descendo (a luz vem de cima), as mechas mais claras
        t = 2 + (k - 4) // 6 + clump(i, j, k) + (1 if streak.get(i if n[0] == 0 else j) else 0)
        if not solid((i, j, k - 1)):
            t -= 1                                                  # a ponta da mecha
    if THIN < 1:
        # rente (raspado): sem as mechas claras (viram listras) e mais claro embaixo dos lados (o degradê)
        t = 3 + clump(i, j, k) + (1 if k < 9 else 0) + (1 if n[2] > 0 else 0)
    return max(0, min(len(PALETTE) - 1, t + LIGHTER))


DIRS = [((1, 0, 0), (1, 0, 0)), ((-1, 0, 0), (0, 0, 0)), ((0, 1, 0), (0, 1, 0)), ((0, -1, 0), (0, 0, 0)),
        ((0, 0, 1), (0, 0, 1)), ((0, 0, -1), (0, 0, 0))]


def squash(v):
    """Fora da cabeça (antes de 0 ou depois de 16) as casas têm só THIN de espessura."""
    return v * THIN if v < 0 else 16 + (v - 16) * THIN if v > 16 else v


def corner(i, j, k):
    """O canto (i, j, k) da grade em metros (j cresce para trás)."""
    return Vector((x0 + squash(i) * CX, y0 + squash(j) * CY, z0 + (k if k < 16 else squash(k)) * CZ))   # a frente: -Y


bm = bmesh.new()
uv = bm.loops.layers.uv.new("UV")
verts = {}
owner = {}                                                          # vértice -> {corrente ou -1 (cabeça)}
cell_chain = {}
for n, (_, _, cells) in enumerate(CHAINS):
    for c in cells:
        cell_chain[c] = n


def vert(p):
    if p not in verts:
        verts[p] = bm.verts.new(corner(*p))
    return verts[p]


faces = 0
for c in sorted(hair):
    i, j, k = c
    for n, off in DIRS:
        if solid((i + n[0], j + n[1], k + n[2])):
            continue
        axis = [a for a in range(3) if n[a] != 0][0]
        u, v = [a for a in range(3) if a != axis]
        base = [i, j, k]
        base[axis] += off[axis]
        quad = []
        for du, dv in ((0, 0), (1, 0), (1, 1), (0, 1)):
            q = list(base)
            q[u] += du
            q[v] += dv
            quad.append(tuple(q))
        for q in quad:
            owner.setdefault(q, set()).add(cell_chain.get(c, -1))
        f = bm.faces.new([vert(q) for q in quad])
        t = tone(c, n)
        for loop in f.loops:
            loop[uv].uv = ((t + .5) / len(PALETTE), .5)
        faces += 1
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
order = list(verts.keys())                                          # a ordem dos vértices na malha

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

# ---- as correntes de física (ossos Hair_<nome>_NN filhos da cabeça) e os pesos
obj.parent = rig
head_vg = obj.vertex_groups.new(name=P + "Head")
chain_bones = []                                                    # por corrente: os nomes dos ossos
if CHAINS:
    with bpy.context.temp_override(active_object=rig, object=rig, selected_objects=[rig], selected_editable_objects=[rig]):
        bpy.ops.object.mode_set(mode="EDIT")
        eb = rig.data.edit_bones
        for b in [b for b in eb if b.name.startswith("Hair_")]:
            eb.remove(b)
        inv = rig.matrix_world.inverted()
        for name, points, _ in CHAINS:
            parent = eb[P + "Head"]
            names = []
            for n, (pa, pb) in enumerate(zip(points, points[1:])):
                b = eb.new("Hair_%s_%02d" % (name, n + 1))
                b.head = inv @ corner(pa[0] + .5, pa[1] + .5, pa[2])
                b.tail = inv @ corner(pb[0] + .5, pb[1] + .5, pb[2])
                b.parent = parent
                b.use_connect = n > 0
                b.use_deform = True
                parent = b
                names.append(b.name)
            chain_bones.append(names)
        bpy.ops.object.mode_set(mode="OBJECT")
groups = {n: obj.vertex_groups.new(name=n) for names in chain_bones for n in names}
for index, p in enumerate(order):
    own = owner.get(p, {-1})
    if len(own) != 1 or -1 in own:
        head_vg.add([index], 1.0, "REPLACE")
        continue
    names = chain_bones[next(iter(own))]
    tops = [pt[2] for pt in CHAINS[next(iter(own))][1]]
    # pela altura, entre as duas juntas da corrente; acima da primeira, a cabeça e o primeiro osso
    k = p[2]
    if k >= tops[0]:
        t = min(1.0, (k - tops[0]) / 3.0)
        head_vg.add([index], t, "REPLACE")
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
log("%d blocos, %d faces, %d vértices, correntes: %s" % (len(hair), faces, len(mesh.vertices),
                                                        ", ".join("%s (%d ossos)" % (c[0], len(b)) for c, b in zip(CHAINS, chain_bones)) or "nenhuma"))

# ---- os tons para o jogo: a cor do cabelo é escolhida no menu, e a textura guarda só o quanto cada tom é mais claro ou
# mais escuro que o tom-base (a cor "hair_color"): valor linear = (luz do tom / luz do tom-base) / 2. O M_BlockHair faz
# cor x valor x 2


def linear(c):
    c /= 255.0
    return c / 12.92 if c <= .04045 else ((c + .055) / 1.055) ** 2.4


def light(col):
    return .2126 * linear(col[0]) + .7152 * linear(col[1]) + .0722 * linear(col[2])


BASE_TONE = (120, 64, 38)                                          # o "#784026" do hair_color
tones = bpy.data.images.get("T_Cabelo_Tons") or bpy.data.images.new("T_Cabelo_Tons", len(PALETTE), 1)
tones.colorspace_settings.name = "Non-Color"
px = []
for col in PALETTE:
    v = min(1.0, light(col) / light(BASE_TONE) / 2)
    px += [v, v, v, 1.0]
tones.pixels[:] = px

# ---- os arquivos: o cabelo com o esqueleto (sem o corpo)
bpy.context.view_layer.update()
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
    raise SystemExit(0)
img.filepath_raw = os.path.join(OUT, "texturas", "T_Cabelo_Paleta.png")
img.file_format = "PNG"
os.makedirs(os.path.dirname(img.filepath_raw), exist_ok=True)
img.save()
with bpy.context.temp_override(active_object=rig, object=rig, selected_objects=[rig, obj],
                               selected_editable_objects=[rig, obj]):
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, "cabelo_%s.fbx" % STYLE), use_selection=True,
                             object_types={"ARMATURE", "MESH"}, path_mode="COPY", embed_textures=True,
                             add_leaf_bones=False, bake_anim=False, apply_scale_options="FBX_SCALE_ALL")
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "cabelo_%s.blend" % STYLE), copy=True)
log("exportado", os.path.join(OUT, "cabelo_%s.fbx" % STYLE))
