"""As 25 armas do Vortex, feitas de novo no Blender com mais detalhe e textura própria.

No Blender (Tools/Blender/vortex_armory.py junto): exec(open(<este arquivo>).read())
ou: blender --background --python Tools/Blender/BuildVortexWeapons.py -- [tipo ...]

Cada arma é montada no referencial do item do Godot (Assets/Equipment/Weapons/<item>.glb: a pegada
na origem, o comprimento para +Z) com as mesmas proporções, e exportada para
Data/VortexWeapons/Models/W_<tipo>.glb (e W_<tipo>_L.glb das armas de par) já na mão do herói.
Depois: Tools/ImportVortexWeapons.py no editor (troca os modelos das definições de arma).
"""
import json
import math
import os
import sys

import bpy

ROOT = os.environ.get("VORTEX_ROOT", r"C:\Users\Valorant Niko\Documents\ChatGPT\Mangas\Vortex-UE5-migracao\Vortex")
sys.path.insert(0, os.path.join(ROOT, "Tools", "Blender"))
import importlib  # noqa: E402
import vortex_armory  # noqa: E402
importlib.reload(vortex_armory)
from vortex_armory import Weapon  # noqa: E402

R = math.radians
X90 = (R(90), 0, 0)
Y90 = (0, R(90), 0)
BUILDERS = {}


def builder(fn):
    BUILDERS[fn.__name__] = fn
    return fn


def grip(w, bottom, top, r=.07, mat="couro", wrap="couro_escuro", turns=6):
    """Cabo de madeira/couro com tiras enroladas."""
    h = top - bottom
    w.cyl(mat, r, h, (0, 0, bottom + h / 2), verts=10)
    for o in w.wrap(wrap, r * 1.04, h * .9, turns):
        o.location.z += bottom + h / 2


def sword_blade(w, base, tip, half, thick=.07, mat="aco", edge="prata", point=.28, waist=.9):
    """Lâmina reta de espada: larga na base, um pouco mais fina no meio, ponta em V."""
    pts = [(-half, base), (half, base), (half * waist, base + (tip - base) * .55), (half * .95, tip - point), (0, tip),
           (-half * .95, tip - point), (-half * waist, base + (tip - base) * .55)]
    return w.blade(mat, pts, thick, edge_mat=edge)


# ---- lâminas -----------------------------------------------------------------------------------------

@builder
def espada_de_cinzas(w):
    sword_blade(w, .3, 1.95, .12, .075, "aco_escuro", "prata")
    w.box("brasa", (.035, .16, 1.0), (0, 0, 1.0))                     # sulco de brasa no meio da lâmina
    for z in (.9, 1.15, 1.4):
        w.gem("brasa", .03, (0, -.04, z), stretch=1.6)
        w.gem("brasa", .03, (0, .04, z), stretch=1.6)
    w.box("ferro", (.72, .16, .1), (0, 0, .26), bevel=.02)             # guarda
    for s in (-1, 1):
        w.box("bronze", (.08, .18, .14), (s * .34, 0, .26), bevel=.02)
        w.spike("ferro", .04, .12, (s * .4, 0, .26), (0, R(90 * s), 0))
    w.gem("brasa", .045, (0, -.09, .26))
    w.gem("brasa", .045, (0, .09, .26))
    grip(w, -.3, .2, .065)
    w.cyl("bronze", .085, .04, (0, 0, .2))
    w.cyl("bronze", .085, .04, (0, 0, -.3))
    w.sphere("ferro", .085, (0, 0, -.38))                              # pomo
    w.gem("brasa", .04, (0, 0, -.47), stretch=1.2)


@builder
def espada_escudo(w):
    sword_blade(w, .33, 2.24, .085, .06, "aco", "prata", point=.22)
    w.box("aco_escuro", (.02, .13, 1.4), (0, 0, 1.2))
    w.box("bronze", (.42, .14, .08), (0, 0, .29), bevel=.02)
    for s in (-1, 1):
        w.sphere("bronze", .05, (s * .22, 0, .29))
    grip(w, -.35, .25, .055, "madeira", "couro", 5)
    w.box("bronze", (.12, .12, .1), (0, 0, -.4), bevel=.025)


def escudo(w):
    """O escudo redondo da Espada e Escudo (vai para a mão esquerda: W_espada_escudo_L)."""
    w.cyl("madeira", .55, .08, (0, 0, 0), rot=X90, verts=20, bevel=.01)
    w.ring("ferro", .55, .035, (0, 0, 0), rot=X90, segs=28)
    w.sphere("aco", .14, (0, -.06, 0))                                   # umbo
    for i in range(8):
        a = i * math.pi / 4
        w.sphere("bronze", .025, (math.cos(a) * .44, -.05, math.sin(a) * .44), subdiv=1)
    w.box("tecido_azul", (.12, .02, 1.0), (0, -.045, 0), bevel=0)        # faixa pintada
    w.box("couro", (.28, .06, .06), (0, .07, 0), bevel=.01)              # alça


@builder
def espadao(w):
    sword_blade(w, .4, 3.94, .16, .08, "aco", "prata", point=.4, waist=.85)
    w.box("aco_escuro", (.03, .17, 2.6), (0, 0, 1.9))
    w.box("ferro", (.8, .18, .12), (0, 0, .36), bevel=.025)
    for s in (-1, 1):
        w.box("ferro", (.08, .2, .22), (s * .38, 0, .4), rot=(0, R(-20 * s), 0), bevel=.02)
    w.box("couro_vermelho", (.2, .19, .14), (0, 0, .5), bevel=.02)       # ricasso
    grip(w, -.75, .3, .06, "madeira_escura", "couro_vermelho", 9)
    w.sphere("ferro", .1, (0, 0, -.82))
    w.gem("rubi", .05, (0, -.09, .36))
    w.gem("rubi", .05, (0, .09, .36))


@builder
def katana_do_vento(w):
    # lâmina curva no plano YZ (o fio para +Y, como a do Godot)
    pts = []
    n = 14
    for i in range(n + 1):
        t = i / n
        z = .44 + t * 2.3
        bend = .12 * t * t
        pts.append((-.045 + bend - .045 * t, z))
    back = [(x + .1 - .05 * (i / n) ** 2, z) for i, (x, z) in enumerate(pts)]
    outline = [(x, z) for (x, z) in back] + [(pts[-1][0] + .02, 2.78)] + [(x, z) for (x, z) in reversed(pts)]
    b = w.blade("aco", outline, .05, rot=(0, 0, R(90)), edge_mat="prata")
    w.cyl("ouro", .17, .04, (0, 0, .4), verts=8)                         # tsuba
    w.ring("ouro", .13, .015, (0, 0, .42))
    w.cyl("bronze", .055, .07, (0, 0, .46))
    w.cyl("couro_escuro", .05, .72, (0, 0, -.05), verts=8)
    for i in range(6):                                                   # trançado em losango
        w.gem("tecido_azul", .03, (0, -.045, -.36 + i * .12), stretch=1.6)
        w.gem("tecido_azul", .03, (0, .045, -.36 + i * .12), stretch=1.6)
    w.cyl("ouro", .055, .06, (0, 0, -.44))


@builder
def adagas_gemeas(w):
    pts = [(-.09, .28), (.09, .28), (.075, .8), (.04, 1.15), (0, 1.3), (-.04, 1.15), (-.075, .8)]
    w.blade("aco", pts, .04, edge_mat="prata")
    w.box("aco_escuro", (.015, .05, .7), (0, 0, .7))
    w.box("ouro", (.42, .1, .06), (0, 0, .25), bevel=.015)
    for s in (-1, 1):
        w.cyl("ouro", .025, .1, (s * .23, 0, .29), r2=.0, verts=6)
    w.gem("ametista", .035, (0, -.06, .25))
    grip(w, -.21, .21, .045, "couro_escuro", "couro_vermelho", 4)
    w.sphere("ouro", .055, (0, 0, -.26))


@builder
def machado(w):
    """Machado de lenhador/guerra: cabo de freixo, cabeça de ferro escuro com fio largo e polido (barba)."""
    w.cyl("madeira", .055, 2.1, (0, 0, .5), verts=10)
    for o in w.wrap("couro", .058, .34, 3):
        o.location.z -= .33
    # lâmina barbuda para +X: o fio (prata) curvo e largo, as faces de ferro escuro
    pts = [(.07, 1.24), (.3, 1.18), (.52, 1.02), (.7, 1.0), (.78, 1.2), (.8, 1.42), (.74, 1.66), (.56, 1.74), (.3, 1.64), (.07, 1.58)]
    w.blade("aco_escuro", pts, .09, edge_mat="prata", ridge=False, edge=.3)
    w.box("ferro", (.18, .16, .44), (0, 0, 1.41), bevel=.025)              # olho
    w.box("aco_escuro", (.24, .11, .14), (-.18, 0, 1.41), bevel=.03)       # contrapeso (martelo)
    for z in (1.3, 1.52):
        w.sphere("bronze", .025, (.04, -.085, z), subdiv=1)
        w.sphere("bronze", .025, (.04, .085, z), subdiv=1)
    w.box("couro_vermelho", (.13, .13, .1), (0, 0, 1.12), bevel=.02)      # amarra abaixo da cabeça
    w.cyl("ferro", .065, .06, (0, 0, -.55))


@builder
def machados_duplos(w):
    """Machadinha de mão: cabo curto envolto em couro, cabeça com fio largo e espigão atrás."""
    w.cyl("madeira", .045, 1.15, (0, 0, .18), verts=10)
    for o in w.wrap("couro_escuro", .048, .5, 4):
        o.location.z -= .1
    pts = [(.05, .64), (.22, .6), (.38, .52), (.5, .6), (.54, .78), (.5, .96), (.36, 1.02), (.2, .98), (.05, .92)]
    w.blade("aco_escuro", pts, .07, edge_mat="prata", ridge=False, edge=.3)
    w.box("ferro", (.13, .11, .32), (0, 0, .79), bevel=.018)
    w.spike("aco_escuro", .04, .16, (-.1, 0, .79), (0, R(-90), 0))
    w.sphere("bronze", .02, (.04, -.06, .79), subdiv=1)
    w.sphere("bronze", .02, (.04, .06, .79), subdiv=1)
    w.cyl("ferro", .05, .04, (0, 0, -.4))


@builder
def martelo_de_bram(w):
    w.cyl("madeira_escura", .08, 2.3, (0, 0, .55), verts=10)
    for o in w.wrap("couro", .084, 1.6, 9):
        o.location.z += .6
    w.box("aco_escuro", (1.15, .46, .5), (0, 0, 1.98), bevel=.04)          # cabeça
    for s in (-1, 1):
        w.box("ouro", (.06, .5, .54), (s * .45, 0, 1.98), bevel=.015)       # faixas douradas
        w.box("aco", (.1, .4, .44), (s * .62, 0, 1.98), bevel=.03)          # faces de bater
        for dy in (-.12, .12):
            for dz in (-.12, .12):
                w.sphere("ouro", .025, (s * .2, dy * 1.9, 1.98 + dz), subdiv=1)
    w.box("brasa", (.3, .48, .14), (0, 0, 1.98))                          # runa de brasa no meio
    w.cyl("ouro", .11, .1, (0, 0, 1.68))
    w.cyl("ouro", .1, .08, (0, 0, -.52))
    w.box("aco", (.12, .12, .2), (0, 0, -.64), taper=(.3, .3), bevel=.01)


@builder
def maca_sagrada(w):
    w.cyl("madeira", .05, 1.5, (0, 0, .35), verts=10)
    for o in w.wrap("couro", .053, 1.3, 8):
        o.location.z += .35
    w.cyl("aco_escuro", .12, .55, (0, 0, 1.52), verts=8)
    for i in range(6):                                                    # flanges pontudas
        a = i * math.pi / 3
        w.box("aco", (.13, .035, .5), (math.cos(a) * .17, math.sin(a) * .17, 1.52), rot=(0, 0, a), bevel=.008, taper=(.4, 1.0))
    w.ring("ouro", .13, .015, (0, 0, 1.27))
    w.gem("topazio", .07, (0, 0, 1.9))
    w.cyl("ouro", .08, .12, (0, 0, 1.12))
    w.sphere("ouro", .065, (0, 0, -.44))


@builder
def foice(w):
    w.cyl("madeira", .05, 3.8, (0, 0, 1.2), verts=10)
    for z in (-.3, .3):
        for o in w.wrap("couro", .053, .25, 2):
            o.location.z += z
    # lâmina em meia-lua longa para +X saindo do topo
    pts = [(.02, 2.98), (.5, 3.06), (1.0, 3.02), (1.42, 2.86), (1.72, 2.62), (1.4, 3.02), (.95, 3.22), (.45, 3.27), (.02, 3.2)]
    w.blade("aco_escuro", pts, .06, edge_mat="prata", ridge=False)
    w.box("ferro", (.16, .14, .3), (0, 0, 3.08), bevel=.02)
    w.ring("bronze", .055, .012, (0, 0, 2.9))
    w.cyl("ferro", .06, .05, (0, 0, -.7))

@builder
def lanca_do_patrulheiro(w):
    w.cyl("madeira_clara", .045, 3.75, (0, 0, .43), verts=10)
    for o in w.wrap("couro", .048, 1.2, 8):
        o.location.z += .45
    for z in (-.3, 1.14, 2.3):
        w.cyl("bronze", .055, .06, (0, 0, z))
    pts = [(-.12, 2.42), (.12, 2.42), (.09, 2.75), (0, 3.12), (-.09, 2.75)]
    w.blade("aco", pts, .07, edge_mat="prata")
    w.cyl("bronze", .06, .12, (0, 0, 2.38), r2=.04)
    for s in (-1, 1):
        w.box("bronze", (.06, .03, .14), (s * .1, 0, 2.38), rot=(0, R(25 * s), 0))
    w.cyl("tecido_azul", .05, .02, (0, 0, 2.26))
    w.cyl("ferro", .05, .15, (0, 0, -1.55), r2=.0)


@builder
def alabarda(w):
    w.cyl("madeira", .05, 4.4, (0, 0, 1.4), verts=10)
    for z in (-.2, 1.4, 2.9):
        w.ring("couro", .053, .012, (0, 0, z))
    pts = [(.04, 3.3), (.45, 3.2), (.62, 3.45), (.64, 3.85), (.45, 4.05), (.04, 3.9)]
    w.blade("aco_azul", pts, .07, edge_mat="prata", ridge=False)
    w.box("aco_escuro", (.14, .12, .62), (0, 0, 3.6), bevel=.02)
    w.spike("aco_escuro", .05, .34, (-.2, 0, 3.6), (0, R(-90), 0))      # gancho
    pts2 = [(-.07, 3.9), (.07, 3.9), (.05, 4.3), (0, 4.6), (-.05, 4.3)]
    w.blade("aco_azul", pts2, .06, edge_mat="prata")
    w.cyl("ferro", .05, .12, (0, 0, -.8), r2=.0)


@builder
def cajado_de_ilsa(w):
    # tronco retorcido: cilindros curtos levemente deslocados
    for i in range(24):
        z = -1.4 + i * .18
        off = .02 * math.sin(i * .9)
        w.cyl("madeira_escura", .065 - .0006 * i, .2, (off, -off, z), rot=(R(3 * math.sin(i)), R(3 * math.cos(i)), 0), verts=8)
    for i in range(10):                                                  # trepadeira e folhas
        z = -.9 + i * .32
        a = i * 1.4
        w.ring("folha", .075, .012, (0, 0, z), rot=(R(15), 0, a))
        w.box("folha", (.12, .02, .06), (math.cos(a) * .12, math.sin(a) * .12, z + .05), rot=(0, R(30), a), bevel=.0)
    for s in range(4):                                                   # garras que seguram o cristal
        a = s * math.pi / 2
        w.box("ouro", (.03, .03, .4), (math.cos(a) * .1, math.sin(a) * .1, 2.75), rot=(R(-12 * math.sin(a)), R(12 * math.cos(a)), 0), bevel=.008)
    w.ring("ouro", .1, .02, (0, 0, 2.6))
    w.gem("jade", .12, (0, 0, 2.99), stretch=2.4)
    w.cyl("ouro", .07, .1, (0, 0, -1.42))


@builder
def varinha(w):
    w.cyl("madeira_escura", .028, 1.0, (0, 0, .7), r2=.018, verts=8)
    w.cyl("madeira", .04, .5, (0, 0, .07), r2=.034, verts=8)
    w.ring("couro", .042, .01, (0, 0, .33))
    w.ring("ouro", .03, .008, (0, 0, .9))
    w.gem("ametista", .035, (0, 0, 1.24), stretch=1.8)
    w.cyl("ouro", .03, .05, (0, 0, 1.18))
    w.sphere("ouro", .045, (0, 0, -.18))


@builder
def chicote(w):
    """Só o cabo: a tira de couro comprida é uma corda animada no jogo (VortexWeaponParts), presa na ponta."""
    w.cyl("madeira_escura", .05, .62, (0, 0, .06), verts=8)
    for o in w.wrap("couro_vermelho", .053, .52, 6):
        o.location.z += .06
    w.sphere("ouro", .06, (0, 0, -.28))
    w.cyl("ouro", .058, .05, (0, 0, .38))
    w.cyl("couro", .045, .14, (0, 0, .47), r2=.03)                        # começo da tira (afina)


@builder
def correntes(w):
    """Só o cabo e o primeiro elo: a corrente e a lâmina da ponta (correntes_lamina) são animadas no jogo."""
    w.cyl("couro_escuro", .05, .3, (0, 0, 0), verts=8)
    for o in w.wrap("couro", .053, .26, 4):
        pass
    w.cyl("ferro", .06, .04, (0, 0, -.17))
    w.ring("ferro", .055, .014, (0, 0, .2))
    w.ring("aco_escuro", .045, .012, (0, 0, .3), rot=(R(90), 0, R(90)), segs=10)


def correntes_lamina(w):
    """A lâmina curva da ponta da corrente (no jogo, presa ao último elo)."""
    w.ring("aco_escuro", .045, .012, (0, 0, 0), rot=(R(90), 0, 0), segs=10)
    pts = [(-.05, .04), (.1, .02), (.26, .14), (.34, .34), (.24, .5), (.06, .58), (.12, .38), (.02, .2)]
    w.blade("aco", pts, .05, edge_mat="prata", ridge=False)


@builder
def chakrams(w):
    w.ring("aco", .36, .035, (0, 0, .45), rot=(R(90), 0, 0), segs=28)
    for i in range(8):                                                   # dentes da borda
        a = i * math.pi / 4
        w.spike("prata", .05, .14, (math.cos(a) * .42, 0, .45 + math.sin(a) * .42), (0, -a + R(90), 0), verts=4)
    w.box("couro", (.6, .05, .08), (0, 0, .45), bevel=.01)             # empunhadura no meio
    w.ring("ouro", .3, .012, (0, 0, .45), rot=(R(90), 0, 0), segs=24)


@builder
def manoplas_de_ferro(w):
    w.box("aco_escuro", (.4, .44, .5), (0, 0, -.12), bevel=.05)          # braçadeira
    w.box("aco", (.42, .3, .26), (0, .06, .17), bevel=.04)               # dorso da mão
    for i in range(4):
        w.box("aco", (.09, .12, .1), (-.15 + i * .1, .12, .3), bevel=.02)   # nós dos dedos
        w.spike("prata", .025, .07, (-.15 + i * .1, .2, .3), X90, verts=4)
    for z in (-.3, -.05):
        w.ring("bronze", .25, .02, (0, 0, z), segs=18)
    w.box("couro_escuro", (.3, .05, .24), (0, -.16, .05), bevel=.01)


# ---- distância -------------------------------------------------------------------------------------

@builder
def arco_de_betula(w):
    # um arco inteiro numa tira contínua (do topo à ponta de baixo), curvando para +Y; corda reta em Y=.22
    pts = []
    for s_ in (-1, 1):
        rng = range(16, -1, -1) if s_ < 0 else range(0, 17)
        for i in rng:
            t = i / 16
            if s_ > 0 and i == 0:
                continue
            pts.append((0, .03 + .27 * t ** 1.7, s_ * (.04 + t * 1.42)))
    w.sweep("betula", pts, .085, .06, twist_up=(1, 0, 0))
    for s_ in (-1, 1):
        w.ring("ouro", .045, .01, (0, .27, s_ * 1.36), rot=X90)
        w.cyl("betula", .035, .1, (0, .3, s_ * 1.47), verts=6)
        for k in range(3):
            w.gem("jade", .02, (0, .02 + .05 * k, s_ * (.45 + k * .28)))
    w.cyl("couro", .055, .42, (0, .02, 0), verts=8)                       # pegada
    for o in w.wrap("couro_escuro", .058, .38, 4):
        o.location.y += .02
    w.box("ouro", (.04, .03, .08), (0, .07, .24), bevel=.005)             # apoio da flecha
    w.cyl("corda", .006, 2.84, (0, .3, 0), verts=5)

@builder
def besta(w):
    w.box("madeira", (.12, .14, 1.3), (0, 0, .3), bevel=.02)              # coronha
    w.box("madeira_escura", (.1, .18, .3), (0, -.08, -.42), rot=(R(-15), 0, 0), bevel=.03)
    w.box("aco_escuro", (.06, .08, 1.1), (0, .1, .45), bevel=.01)          # trilho
    for s in (-1, 1):                                                     # arco (prod)
        w.box("madeira_escura", (.62, .05, .06), (s * .32, .05, .9), rot=(0, R(-12 * s), 0), bevel=.012)
        w.cyl("aco", .03, .06, (s * .64, .05, .97), rot=Y90)
    w.cyl("corda", .006, 1.25, (0, .12, .82), rot=Y90, verts=5)
    w.box("aco", (.03, .06, .1), (0, -.1, .05), bevel=.005)               # gatilho
    w.cyl("aco", .012, .7, (0, .15, .62), verts=6)                        # virote
    w.cyl("aco", .025, .08, (0, .15, .99), r2=0, verts=4)
    w.ring("bronze", .07, .012, (0, 0, .6), rot=X90)


@builder
def pistola_runica(w):
    """Pistola de verdade: cano octogonal para a frente (+Z) acima da mão, tambor rúnico, cão, guarda-mato
    e o cabo para baixo (+Y), curvo, de madeira com placas de bronze."""
    w.cyl("aco_escuro", .075, .82, (0, -.2, .5), verts=8)                 # cano
    w.cyl("bronze", .095, .07, (0, -.2, .9), verts=8)                      # boca
    w.ring("runa", .07, .012, (0, -.2, .93))
    for z in (.36, .52, .68):
        w.ring("runa", .079, .009, (0, -.2, z))
    w.cyl("bronze", .13, .2, (0, -.17, .05), verts=10)                     # tambor
    for i in range(6):
        a = i * math.pi / 3
        w.gem("runa", .022, (math.cos(a) * .1, -.17 + math.sin(a) * .1, .16), stretch=.6)
    w.box("aco_escuro", (.12, .16, .34), (0, -.13, -.1), bevel=.02)        # armação
    w.box("aco", (.05, .1, .1), (0, -.3, -.24), rot=(R(-30), 0, 0), bevel=.01)   # cão
    w.box("madeira_escura", (.12, .5, .16), (0, .16, -.16), rot=(R(-12), 0, 0), bevel=.04)   # cabo para baixo
    w.box("bronze", (.13, .4, .1), (0, .16, -.16), rot=(R(-12), 0, 0), bevel=.02)
    w.sphere("bronze", .07, (0, .42, -.22))                                # pomo
    w.ring("aco", .07, .012, (0, .02, .1), rot=(0, R(90), 0))              # guarda-mato
    w.box("aco", (.025, .07, .025), (0, .0, .06), bevel=.004)              # gatilho


@builder
def livro_da_aurora(w):
    """O grimório sem a capa da frente (livro_da_aurora_capa) e com as folhas que viram
    (livro_da_aurora_pagina): no jogo a capa abre e as páginas passam ao usar."""
    w.box("tecido_vermelho", (.62, .05, .82), (0, .1, .41), bevel=.015)    # capa de trás
    w.box("papel", (.58, .16, .78), (.0, 0, .41), bevel=.005)              # páginas
    w.box("tecido_vermelho", (.06, .25, .82), (-.31, 0, .41), bevel=.02)  # lombada
    for sz in (0, 1):
        w.box("ouro", (.08, .27, .08), (-.28, 0, .02 + sz * .78), bevel=.01)
        w.box("ouro", (.08, .08, .08), (.28, .1, .02 + sz * .78), bevel=.01)


def livro_da_aurora_capa(w):
    """A capa da frente (gira na lombada). A dobradiça sobra um pouco para fora: o jogo acha o eixo por ela."""
    w.box("tecido_vermelho", (.62, .05, .82), (0, -.1, .41), bevel=.015)
    w.cyl("ouro", .03, .82, (-.34, -.1, .41), verts=8)                     # dobradiça
    for sz in (0, 1):
        w.box("ouro", (.08, .08, .08), (.28, -.1, .02 + sz * .78), bevel=.01)
    w.gem("topazio", .08, (0, -.14, .41), rot=X90, stretch=.5)            # sol nascente
    for i in range(8):
        a = i * math.pi / 8
        w.box("ouro", (.02, .01, .1), (math.cos(a) * .14, -.13, .41 + math.sin(a) * .14), rot=(0, -a + R(90), 0), bevel=0)
    w.box("ouro", (.06, .03, .1), (.31, -.1, .41), bevel=.01)               # fecho


def livro_da_aurora_pagina(w):
    """Uma folha solta por cima das páginas (vira da direita para a esquerda)."""
    w.box("papel", (.56, .012, .76), (.0, -.072, .41), bevel=0)
    w.cyl("papel", .012, .76, (-.33, -.072, .41), verts=6)                 # a dobra (o eixo)
    for i in range(5):
        w.box("runa", (.3, .004, .015), (.02, -.08, .2 + i * .1), bevel=0)


@builder
def orbe(w):
    w.cyl("couro", .04, .34, (0, 0, .05), verts=8)
    w.cyl("prata", .07, .1, (0, 0, .24))
    for i in range(3):                                                    # gaiola
        w.ring("prata", .3, .012, (0, 0, .62), rot=(R(90), 0, i * math.pi / 3), segs=24)
    w.gem("cristal", .22, (0, 0, .62), stretch=1.0)
    w.sphere("safira", .08, (0, 0, .62), subdiv=1)
    w.gem("prata", .04, (0, 0, .97))


@builder
def catalisador(w):
    w.cyl("couro", .04, .34, (0, 0, .05), verts=8)
    w.cyl("bronze", .1, .08, (0, 0, .27))
    w.sphere("vidro", .16, (0, 0, .5))                                    # frasco
    w.sphere("brasa", .1, (0, 0, .47), subdiv=1)                          # mistura
    w.cyl("vidro", .05, .14, (0, 0, .7))
    w.cyl("madeira", .055, .07, (0, 0, .8))
    for i in range(3):                                                    # garras
        a = i * 2 * math.pi / 3
        w.box("bronze", (.03, .03, .32), (math.cos(a) * .15, math.sin(a) * .15, .47), rot=(R(15 * math.sin(a)), R(-15 * math.cos(a)), 0), bevel=.008)


@builder
def instrumento(w):
    w.sphere("madeira_clara", .32, (0, 0, -.3))                           # corpo do alaúde
    for o in w.parts[-1:]:
        o.scale = (1.0, .35, 1.15)
    w.cyl("madeira_escura", .1, .03, (0, -.11, -.25), rot=X90)            # boca
    w.box("madeira", (.12, .06, 1.0), (0, 0, .55), bevel=.01)             # braço
    w.box("madeira_escura", (.16, .08, .22), (0, .03, 1.1), rot=(R(-20), 0, 0), bevel=.015)   # cabeça
    for i in range(4):
        w.cyl("ouro", .012, .06, (-.07 + .045 * i, .05, 1.1), rot=Y90, verts=6)
        w.cyl("corda", .004, 1.3, (-.03 + .02 * i, -.04, .4), verts=4)
    w.box("couro", (.18, .03, .04), (0, -.1, -.45), bevel=.005)


EXTRAS = {"livro_da_aurora_capa": "livro_da_aurora", "livro_da_aurora_pagina": "livro_da_aurora", "correntes_lamina": "correntes",
          "pocao_vermelha": "catalisador", "pocao_azul": "catalisador", "pocao_verde": "catalisador"}


def _pocao(w, liquid):
    """Garrafinha de poção na mão (o corpo na palma, o gargalo para cima): vidro, o líquido brilhando,
    rolha e uma fita de couro no gargalo."""
    w.cyl(liquid, .13, .28, (0, 0, .1), verts=12)                          # o líquido aparece pelo vidro
    w.sphere(liquid, .13, (0, 0, .24))                                     # ombro arredondado
    w.ring("vidro", .132, .012, (0, 0, .2))                                # reflexo do vidro
    w.cyl("vidro", .05, .16, (0, 0, .42), verts=8)                         # gargalo
    w.cyl("couro", .056, .04, (0, 0, .4), verts=8)
    w.cyl("madeira_clara", .045, .08, (0, 0, .53), verts=8)               # rolha
    w.cyl("vidro", .135, .02, (0, 0, -.04), verts=12)                      # fundo


def pocao_vermelha(w):
    _pocao(w, "rubi")


def pocao_azul(w):
    _pocao(w, "safira")


def pocao_verde(w):
    _pocao(w, "jade")


def build(names=None):
    frames = json.load(open(os.path.join(ROOT, "Data", "VortexWeapons", "frames.json"), encoding="utf-8"))["weapons"]
    out = os.path.join(ROOT, "Data", "VortexWeapons", "Models")
    if bpy.app.background:
        # em segundo plano: a cena padrão, limpa (uma cena nova não tem o grafo avaliado e o exportador quebrava)
        scene = bpy.context.scene
        for o in list(scene.objects):
            bpy.data.objects.remove(o, do_unlink=True)
    else:
        scene = bpy.data.scenes.get("VortexArmasNovas") or bpy.data.scenes.new("VortexArmasNovas")
        if bpy.context.window:
            bpy.context.window.scene = scene
    report = {}
    for extra, base in EXTRAS.items():
        frames[extra] = {"right": frames[base]["right"]}
    for name in names or list(BUILDERS):
        w = Weapon(name, scene)
        BUILDERS[name](w)
        obj = w.finish()
        if name == "espada_escudo":
            shield = Weapon("espada_escudo_escudo", scene)
            escudo(shield)
            shield.finish()
            shield.name = "espada_escudo"
            report["escudo"] = shield.export_left(out, frames)
        report[name] = {"verts": len(obj.data.vertices), "materials": [m.name for m in obj.data.materials],
                        "files": [os.path.basename(p) for p in w.export(out, frames)]}
        # peças animadas no jogo (capa e folha do grimório, lâmina da corrente)
        for extra, base in EXTRAS.items():
            if base != name:
                continue
            e = Weapon(extra, scene)
            globals()[extra](e)
            e.finish()
            report[extra] = [os.path.basename(p) for p in e.export(out, frames)]
    report["texturas"] = vortex_armory.export_textures(os.path.join(ROOT, "Data", "VortexWeapons", "Textures"))
    return report


if __name__ == "__main__" and bpy.app.background:
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    print("VORTEX_WEAPONS", json.dumps(build(args or None)))
