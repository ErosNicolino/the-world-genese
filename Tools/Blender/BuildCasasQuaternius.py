"""As casas e a loja da vila com o kit da Quaternius (11/11; Medieval Village MegaKit e Fantasy Props MegaKit, CC0).

Troca as vila_casa_1..8 e a vila_loja do kit antigo (Tools/Blender/BuildVilaKit.py; cópias em Backups/vila_2026-11-11).
Cada casa é uma lista de paredes por lado numa grade de 2 m (a pedra do kit fica do lado de fora: a face de fora da
parede é o -y da peça), o telhado do kit do tamanho da planta, a chaminé por dentro (a lareira) até passar do telhado,
móveis com colisão e, nos sobrados, a escada com o vão no piso de cima.

Letras das paredes: . lisa, w janela larga, r janela larga em arco, t janela estreita em arco, d porta.
A frente e os fundos vão do -x para o +x; os lados vão dos fundos (-y) para a frente (+y).

Sai Data/GodotTown/VilaKit/<casa>.fbx e a entrada de cada uma no Data/GodotTown/vila_models.json (caixas de colisão,
alicerce, lampião, chaminé, luz de dentro e porta). Depois: Tools/Town/BuildVila.py e Tools/ImportVilaKit.py.

No Blender aberto (MCP, para ver):  exec(open(r"<raiz>/Tools/Blender/BuildCasasQuaternius.py").read())
Sem janela (exporta):               blender -b --factory-startup -P Tools/Blender/BuildCasasQuaternius.py -- exportar
"""
import importlib
import math
import os
import sys

sys.path.insert(0, r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex\Tools\Blender")
import quaternius_kit as qk  # noqa: E402

importlib.reload(qk)
H = qk.H
EXPORT = "exportar" in sys.argv
ROOF = {4: (3.73, 2.76), 6: (4.89, 4.12), 8: (6.0, 4.98)}       # vão do telhado -> (altura da cumeeira, meia largura com o beiral)
WINDOWS = {"w": "Wide_Flat", "r": "Wide_Round", "t": "Thin_Round"}
# lado -> (giro da parede, eixo que anda)
SIDES = {"front": 180, "back": 0, "left": -90, "right": 90}


def rotated(x, y, rot):
    a = math.radians(rot)
    return x * math.cos(a) - y * math.sin(a), x * math.sin(a) + y * math.cos(a)


def house(name, W, D, ground, upper=None, ridge="x", style=("UnevenBrick", "Plaster"), furniture=(), up_furniture=(), shop=False, cauldron=True, enter=True):
    """ground/upper = {"front": "wd.w", "back": ..., "left": ..., "right": ...}; upper = None faz uma casa térrea.
    enter=False (11/11, o usuário: "pode deixar sem poder entrar em nenhuma, somente as que têm NPC"): a porta do kit
    fechada e sem colisão aberta, sem móveis, escada, piso de cima nem luz de dentro (menos triângulos)."""
    b = qk.Build(name)
    floors = 2 if upper else 1
    top = floors * H
    door = None

    def spot(side, i):
        """O centro da parede i do lado (x, y)."""
        if side in ("front", "back"):
            return -W / 2 + 1 + 2 * i, D / 2 if side == "front" else -D / 2
        return (-W / 2 if side == "left" else W / 2), -D / 2 + 1 + 2 * i

    def furn(piece, x, y, z=0.0, rot=0.0, solid=True, scale=(1, 1, 1)):
        o = b.put(piece, x, y, z, rot, scale)
        if solid:
            corners = [rotated(c[0] * scale[0], c[1] * scale[1], rot) for c in o.bound_box]
            tall = max(c[2] for c in o.bound_box) * scale[2]
            b.box(x + min(c[0] for c in corners), y + min(c[1] for c in corners), z, x + max(c[0] for c in corners), y + max(c[1] for c in corners), z + tall)
        return o

    # ---- paredes, janelas, porta e cantos ------------------------------------------------------------------------
    for level, (plan, look) in enumerate(((ground, style[0]), (upper, style[1]))[:floors]):
        z = level * H
        for side, row in plan.items():
            assert len(row) == (W if side in ("front", "back") else D) // 2, (name, side, row)
            for i, kind in enumerate(row):
                x, y = spot(side, i)
                rot = SIDES[side]
                if kind == "d":
                    b.put("Wall_%s_Door_Flat" % look, x, y, z, rot)
                    b.put("DoorFrame_Flat_WoodDark", x, y, z, rot)
                    door = x
                elif kind in WINDOWS:
                    # a janela larga do reboco sem a base de tijolo (Flat2) no andar de cima
                    wall = "Wall_%s_Window_%s" % (look, WINDOWS[kind]) + ("2" if kind == "w" and look == "Plaster" and level else "")
                    b.put(wall, x, y, z, rot)
                    b.put("Window_%s1" % WINDOWS[kind], x, y, z, rot)
                    if kind != "t":
                        b.put("WindowShutters_%s_Open" % WINDOWS[kind], x, y, z, rot)
                else:
                    b.put("Wall_%s_Straight" % look + ("_Base" if look == "Plaster" and not level else ""), x, y, z, rot)
        for x, y, r in ((-W / 2, -D / 2, 0), (W / 2, -D / 2, 90), (-W / 2, D / 2, -90), (W / 2, D / 2, 180)):
            b.put("Corner_Exterior_Brick" if look == "UnevenBrick" else "Corner_Exterior_Wood", x, y, z, r, (1, 1, H / 3.0))
    # colisão das paredes (de 31 cm para dentro a 12 cm para fora da linha), com o vão da porta (1,3 x 2,25 m) na frente
    b.box(-W / 2 - .12, -D / 2 - .12, 0, W / 2 + .12, -D / 2 + .31, top)
    b.box(-W / 2 - .12, -D / 2, 0, -W / 2 + .31, D / 2, top)
    b.box(W / 2 - .31, -D / 2, 0, W / 2 + .12, D / 2, top)
    if enter:
        b.box(-W / 2 - .12, D / 2 - .31, 0, door - .65, D / 2 + .12, top)
        b.box(door + .65, D / 2 - .31, 0, W / 2 + .12, D / 2 + .12, top)
        b.box(door - .65, D / 2 - .31, 2.25, door + .65, D / 2 + .12, top)
    else:
        b.box(-W / 2 - .12, D / 2 - .31, 0, W / 2 + .12, D / 2 + .12, top)
        b.put("Door_2_Flat", door + .54, D / 2 - .11, 0, rot=180)     # a folha do kit, fechada (a dobradiça no +x)
    # ---- o chão ---------------------------------------------------------------------------------------------------
    cells = [(-W / 2 + 1 + 2 * i, -D / 2 + 1 + 2 * k) for i in range(W // 2) for k in range(D // 2)]
    for x, y in cells:
        b.put("Floor_Brick" if shop else "Floor_WoodDark", x, y, .02)
    b.box(-W / 2 - .2, -D / 2 - .2, -.3, W / 2 + .2, D / 2 + .2, .05)
    # ---- o telhado e as empenas -----------------------------------------------------------------------------------
    span, length = (D, W) if ridge == "x" else (W, D)
    peak, half = ROOF[span]
    b.put("Roof_RoundTiles_%dx%d" % (span, length), 0, 0, top, rot=90 if ridge == "x" else 0)
    for (x, y, r) in (((-W / 2, 0, -90), (W / 2, 0, 90)) if ridge == "x" else ((0, D / 2, 180), (0, -D / 2, 0))):
        b.put("Roof_Front_Brick%d" % span, x, y, top, r)

    def roof_z(u):
        """A altura do telhado a u metros da cumeeira."""
        return top + peak - (peak + .78) * abs(u) / half

    # ---- a chaminé: a coluna da lareira encostada na parede da direita, do chão até passar do telhado ---------------
    cx, cy = W / 2 - .31 - .5, (D / 4 if ridge == "x" else .5)
    for level in range(floors if enter else 0):
        b.put("Prop_Chimney2", cx, cy, level * H, scale=(1, 1, H / 3.0))
    chimney_top = roof_z(cy if ridge == "x" else cx) + 1.3
    b.put("Prop_Chimney", cx, cy, top, scale=(1, 1, (chimney_top - top) / 3.18))
    b.box(cx - .5, cy - .5, 0, cx + .5, cy + .5, top)
    if cauldron and enter:
        furn("Cauldron", cx - .95, cy)
    # ---- o lampião ao lado da porta ---------------------------------------------------------------------------------
    lamp_x = door + (1.0 if door < W / 2 - 1.5 else -1.0)
    b.put("Lantern_Wall", lamp_x, D / 2 + .09, 1.2, rot=180)
    # ---- os móveis --------------------------------------------------------------------------------------------------
    for item in furniture if enter else ():
        furn(*item[:3], 0.0, *item[3:])
    inner = [[0, 2.5, 0]] if enter else []
    if floors == 2 and enter:
        # a escada (1,76 m de largura, sobe 3,03 m em 4,35 m para o +y da peça) e o vão de 2 x 4 m no piso de cima
        if ridge == "x":      # ao longo da parede dos fundos, subindo para o +x; o patamar no canto dos fundos à direita
            end = 2.0 if W == 8 else 1.0
            sx, sy, srot = end - 4.35, -D / 2 + .31 + .88, -90
            hole = (end - 4, -D / 2, end, -D / 2 + 2)
            rails = [(end - 3, -D / 2 + 3, 0), (end - 1, -D / 2 + 3, 0), (end - 5, -D / 2 + 1, 90)]
        else:                 # ao longo da parede da esquerda, subindo para os fundos; o patamar no canto dos fundos
            end = -D / 2 + 2.0
            sx, sy, srot = -W / 2 + .31 + .88, end + 4.35, 180
            hole = (-W / 2, end, -W / 2 + 2, end + 4)
            rails = [(-W / 2 + 1, end + 1, 90), (-W / 2 + 1, end + 3, 90), (-W / 2 + 1, end + 3, 180)]
        b.put("Stair_Interior_Solid", sx, sy, .02, srot, (1, 1, H / 3.03))
        steps = 12
        for i in range(steps):
            # cada degrau é uma caixa do começo dele até o fim da escada (maciça por baixo)
            (x0, y0), (x1, y1) = rotated(-.88, i * 4.35 / steps, srot), rotated(.88, 4.35, srot)
            b.box(sx + x0, sy + y0, 0, sx + x1, sy + y1, (i + 1) * H / steps)
        for x, y in cells:
            if not (hole[0] < x < hole[2] and hole[1] < y < hole[3]):
                b.put("Floor_WoodDark", x, y, H + .01)
                b.box(x - 1, y - 1, H - .2, x + 1, y + 1, H + .02)
        for x, y, r in rails:
            b.put("Balcony_Simple_Straight", x, y, H + .11, r)
            rx, ry = rotated(0, -1, r)
            (ax, ay), (bx, by) = rotated(-1, -.08, r), rotated(1, .08, r)
            b.box(x + rx + ax, y + ry + ay, H, x + rx + bx, y + ry + by, H + 1.1)
        for item in up_furniture:
            furn(*item[:3], H + .02, *item[3:])
        inner.append([0, round(H + 2.4, 2), 0])
    info = {
        "footing": [-W / 2 - .45, -D / 2 - .45, W / 2 + .45, D / 2 + .45],
        # 11/11 (o usuário: "as placas das lojas bugando nas casas novas"): o braço da placa pendurada entrava nas vigas;
        # agora a tábua vai presa na parede, em cima da porta (de 2,43 a 3,07 m, entre o batente e a viga do andar)
        "sign": [round(door, 2), 2.75, -(D / 2 + .12)] if shop else None,
        "sign_flat": bool(shop),
        "lamps": [[lamp_x, 1.9, -(D / 2 + 1.0)]],
        "chimneys": [[cx, round(chimney_top, 2), -cy]],
        "inner": inner,
    }
    # a porta de abrir do jogo; a casa fechada guarda o lugar dela só para os moradores (Tools/Town/BuildNpcNav.py: "a casa
    # onde dormem") e o caminho até a rua (Tools/Town/BuildVila.py)
    info["doors" if enter else "closed_door"] = [[round(door - .62, 2), 0.0, -(D / 2 - .11), 1.25, 2.2]]
    return b, info


# os móveis: (peça, x, y, giro[, colisão])
def home(W, D, bed=True):
    """Mesa com cadeiras no meio, estante e armário nos fundos; a cama no canto dos fundos à esquerda (casa térrea)."""
    items = [("Table_Large", 0, .2, 0), ("Chair_1", -1.75, .2, -90), ("Chair_1", 1.75, .2, 90), ("Stool", 0, 1.05, 0),
             ("Barrel", -W / 2 + .8, D / 2 - .8, 0), ("Crate_Wooden", -W / 2 + .85, D / 2 - 1.75, 10)]
    if bed:
        items += [("Bed_Twin1", -W / 2 + 1.35, -D / 2 + 1.6, 0), ("Nightstand_Shelf", -W / 2 + 2.75, -D / 2 + .55, 0),
                  ("Bookcase_2", .6, -D / 2 + .55, 0), ("Cabinet", 2.3, -D / 2 + .52, 0)]
    return items


def bedroom(W, D):
    """O quarto do sobrado: a cama no canto da frente à esquerda, o criado-mudo e a estante no recuo ao lado da escada."""
    x, y = -W / 2 + 1.4, D / 2 - 1.6
    return [("Bed_Twin1", x, y, 0), ("Nightstand_Shelf", x + 1.4, y + .9, 180), ("Bookcase_2", -W / 2 + .55, -D / 2 + 1.2, 90)]


CASAS = [
    # casa térrea de pedra
    ("vila_casa_1", 8, 6, dict(front="wd.w", back=".t..", left=".w.", right="..."), None, "x", ("UnevenBrick", "Plaster"), home(8, 6), ()),
    # sobrado: pedra embaixo, enxaimel em cima
    ("vila_casa_2", 8, 6, dict(front="wd.w", back="....", left="..w", right="..."), dict(front=".rr.", back="...t", left=".w.", right="..."),
     "x", ("UnevenBrick", "Plaster"), home(8, 6, False), bedroom(8, 6)),
    # chalé de reboco, a empena para a rua
    ("vila_casa_3", 6, 6, dict(front="wdw", back=".t.", left=".w.", right="..."), None, "y", ("Plaster", "Plaster"),
     [("Table_Large", -.5, .3, 0), ("Stool", -.5, 1.15, 0), ("Stool", .9, -1.6, 0), ("Bed_Twin1", -1.45, -1.7, 90), ("Barrel", -2.1, 2.1, 0)], ()),
    # sobrado comprido de reboco
    ("vila_casa_4", 10, 6, dict(front="wwdww", back=".....", left="..w", right="..."), dict(front="w.w.w", back="....t", left=".w.", right="..."),
     "x", ("Plaster", "Plaster"), home(10, 6, False) + [("Bookcase_2", -4.45, .3, 90), ("Bench", 2.2, 2.3, 0)], bedroom(10, 6) + [("Cabinet", 1.5, 2.45, 180)]),
    # casa estreita e funda, a empena para a rua
    ("vila_casa_5", 6, 8, dict(front=".dw", back="...", left="....", right="...."), dict(front=".r.", back=".t.", left="....", right="w..."),
     "y", ("UnevenBrick", "Plaster"),
     [("Table_Large", .6, -2.2, 90), ("Stool", -.3, -2.2, 0), ("Stool", 1.5, -2.9, 0), ("Barrel", 2.2, 3.2, 0), ("Cabinet", 2.0, -3.5, 0)],
     [("Bed_Twin1", 1.7, 2.4, 180), ("Nightstand_Shelf", .35, 3.45, 180), ("Bookcase_2", .9, -3.45, 0)]),
    # casa grande de dois andares
    ("vila_casa_6", 10, 8, dict(front="w.dww", back=".....", left="..ww", right="...."), dict(front="r.w.r", back="....t", left=".ww.", right="...."),
     "x", ("UnevenBrick", "Plaster"), home(10, 8, False) + [("Bookcase_2", -4.45, .5, 90), ("Bench", 2.2, 3.3, 0), ("Workbench", 2.6, -.9, 0)],
     bedroom(10, 8) + [("Bed_Twin2", .2, 2.4, 0), ("Cabinet", 2.6, 3.45, 180)]),
    # casa térrea grande de pedra
    ("vila_casa_7", 8, 8, dict(front="wd.w", back=".t..", left=".ww.", right="...."), None, "x", ("UnevenBrick", "Plaster"),
     home(8, 8) + [("Bench", 1.6, -1.5, 0)], ()),
    # casa com frente de loja e toldo
    ("vila_casa_8", 8, 6, dict(front="dwww", back="....", left="...", right="..."), dict(front=".ww.", back="...t", left=".w.", right="..."),
     "x", ("Plaster", "Plaster"), [("Table_Large", 0, .9, 0), ("Stool", 0, .1, 0), ("Bookcase_2", -3.45, .6, 90), ("Barrel_Apples", .8, 2.3, 0)],
     bedroom(8, 6)),
]
# a loja (padaria, ervas, mapas, alfaiate, armaria): o balcão no meio (o lojista fica atrás, em y = -1,1; 1,2 m livres de
# cada lado para ele passar), prateleiras nos fundos
LOJA = ("vila_loja", 8, 8, dict(front="wdww", back="....", left="....", right="...."), dict(front=".rr.", back="..t.", left="....", right="w..."),
        "y", ("UnevenBrick", "Plaster"),
        [("Workbench", .3, .2, 0), ("Bookcase_2", -.4, -3.45, 0), ("Bookcase_2", 1.2, -3.45, 0), ("Barrel", 3.2, 3.2, 0), ("Crate_Wooden", 3.2, 2.2, 0),
         ("Shelf_Arch", 2.4, -3.6, 180, False)],
        [("Bed_Twin1", 2.6, 2.4, 180), ("Nightstand_Shelf", 1.2, 3.45, 180), ("Bookcase_2", .9, -3.45, 0), ("Cabinet", 2.6, -3.45, 0)])

total = 0
for n, spec in enumerate(CASAS + [LOJA]):
    name, W, D, ground, upper, ridge, style, furniture, up_furniture = spec
    shop = name in ("vila_loja", "vila_casa_8")
    # só a loja (os lojistas atrás do balcão) é de entrar; a taverna, a capela e a forja são de outros scripts
    b, info = house(name, W, D, ground, upper, ridge, style, furniture, up_furniture, shop, cauldron=name != "vila_loja", enter=name == "vila_loja")
    if shop:
        # o toldo de tábua em cima das vitrines
        for x in [-W / 2 + 1 + 2 * i for i, k in enumerate(ground["front"]) if k == "w"]:
            b.put("Roof_Wooden_2x1", x, D / 2 + .09, 2.55, rot=180, scale=(1, .8, .7))
    if EXPORT:
        total += b.export(info)
    else:
        b.shift((n % 5) * 16 - 32)
        for o in b.parts:
            o.location.y += (n // 5) * -18
        print(name, len(b.parts), "peças", len(b.boxes), "caixas")
if EXPORT:
    print("CASAS_QUATERNIUS exportadas: %d casas, %d triângulos" % (len(CASAS) + 1, total))
