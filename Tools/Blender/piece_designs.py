"""Peças de roupa separadas do herói (camisa, parte de cima, calça, calçado, chapéu, acessório), a partir das folhas em
"modelando ainda/Futuras modelagens/Roupas" (P_todas, P_fem1, P_acess_masc, P_acess_fem). Rodado pelo MakeBlockOutfit.py,
com id "P_<nome>"; cada peça fica só na sua parte do corpo, para qualquer uma servir com qualquer outra.
"""


def soft(color):
    """Tecido liso: dois tons com uma trama leve, em vez do granulado padrão. Vai por último nos enfeites (o primeiro que pinta vale)."""
    def f(face, u, v, p):
        if face in ("cima", "baixo"):
            return None
        x, y = p["texel"]
        return color, 1 if (x + y * 3) % 7 == 0 else 2
    return f


def cloth(color):
    """Tecido pintado à mão: luz em cima, sombra embaixo e dobras soltas."""
    def f(face, u, v, p):
        if face in ("cima", "baixo"):
            return None
        x, y = p["texel"]
        if (x * 2 + y) % 23 == 0:
            return color, 1
        if (x + y * 3) % 29 == 0:
            return color, 3
        return color, 3 if v < .18 else 1 if v > .72 else 2
    return f


def camisa_linho():
    # P_todas, linha 1, coluna 1: linho cru, gola com cordão em X, manga comprida com a barra dobrada
    shell("torso", color="creme", decos=[vneck(.26, .1), lace("couro_esc", v0=.02, v1=.3), hem("areia", 1, 1), soft("creme")])
    shell("braco", color="creme", decos=[soft("creme")])
    shell("antebraco", color="creme", off=.6, grow=(.3, .3, 0), decos=[band(.72, .8, "areia", 1), hem("areia", 1, 1), soft("creme")], name="manga")


def calca_la_marrom():
    # P_todas, linha 3, coluna 1: lã marrom até o tornozelo, cinto de corda com o nó e as duas pontas na frente
    cor = "couro"
    shell("quadril", color=cor, decos=[soft(cor)])
    shell("coxa", color=cor, decos=[soft(cor)])
    shell("canela", color=cor, decos=[hem("couro_esc", 1, 1), soft(cor)])
    shell("quadril", 0, .24, off=.95, color="areia", decos=[stripes("terra", period=2, tone=1, vertical=True)], name="cinto")
    block("quadril", (0, -6.4, 1), (3, 1, 2.4), "areia", [], "no")
    block("quadril", (-1, -6.2, -3.5), (1, .7, 7), "areia", [hem("terra", 1)], "ponta")
    block("quadril", (1.2, -6.2, -2.8), (1, .7, 5.6), "areia", [hem("terra", 1)], "ponta")


def chapeu_cacador():
    # P_acess_masc, linha 1, coluna 2: gorro verde de caçador com a aba enrolada e uma pena. Esconde o cabelo
    cor = "verde"
    head_wrap(5, cor, decos=[cloth(cor)], off=.7, name="gorro")
    block("cabeca", (0, .5, 10.4), (12, 12, 1.8), cor, [cloth(cor)], "copa")
    shell("cabeca", .24, .4, off=1.5, color="verde_folha", decos=[hem(cor, 1, 0), cloth("verde_folha")], name="aba")
    block("cabeca", (-5, -9.9, 3.8), (2.4, .7, 2.4), "bronze", [], "broche")
    for k, (y, z, w) in enumerate(((3, 6, 3), (4.8, 8.6, 3), (6.4, 11, 2.6), (7.8, 13.2, 1.8))):
        block("cabeca", (9.8, y, z), (.8, w, 2.8), "branco" if k % 2 else "areia", [], "pena")


ROW_EDGES = (0, .26, .52, .78, 1)
SHEET_ROWS = {"H01": (0, .5, 1), "E_pecas_masc": (0, .2, .4, .6, .8, 1), "E_pecas_fem": (0, .2, .4, .6, .8, 1), "E_sets": (0, .5, 1)}
SHEET_COLS = {"H01": (.055, .252, .451, .65, .85), "P_todas": (.122, .322, .513, .703, .892), "P_fem1": (.105, .303, .5, .7, .9), "E_sets": (.098, .4245, .765)}   # o meio de cada coluna
SHEET_BACK = {"H01": (.151, .348, .547, .746, .945), "E_sets": (.236, .569, .902)}   # 09/11: o meio da vista de costas (só o traje inteiro tem)
OWN_COLOR = {"pena", "broche", "capuz"}   # enfeites pequenos: ficam com a cor do desenho, sem pegar da folha
PALE = {"P_blusa_azul_bordada", "P_elfo_camisa_lua", "P_elfo_calca_seda", "P_elfo_manto_prata", "P_elfo_blusa_marfim", "P_elfo_blusa_lua", "P_elfo_manto_branco", "P_elfo_saia_marfim", "P_elfo_sapato_prata", "P_elfo_sapatilha"}   # azul quase cinza: a separação do boneco precisa ser mais fina
TORSO_HALF = .036   # meia largura do tronco na folha, em fração da largura da imagem


# ---- 09/11: chapéus em blocos finos, saídos do próprio desenho da folha
VOXEL_GRID = {"P_acess_masc": (5, 4, .95), "P_acess_fem": (5, 4, .95), "E_pecas_masc": (5, 5, .45), "E_pecas_fem": (5, 5, .45)}
VOXEL_REF = None
HEAD_WIDTH = {"P_acess_masc": 102, "P_acess_fem": 102, "E_pecas_masc": 62, "E_pecas_fem": 62}   # px da cabeça do boneco
HEAD_NECK = {"P_acess_masc": 16, "P_acess_fem": 16, "E_pecas_masc": 8, "E_pecas_fem": 8}   # px do pescoço (o herói não tem)


def piece_material(pid):
    """O material do relevo, pelo nome da peça."""
    if any(k in pid for k in ("tiara", "corrente", "colar", "bronze", "bracais", "chifres", "dente", "brincos")):
        return "metal"
    if any(k in pid for k in ("palha", "louros", "folhas", "flores")):
        return "madeira"
    return "couro"


def voxel_read(sheet, row, col):
    """Lê a célula da folha: o que é peça, o que é o boneco, e onde está a cabeça dele (é ela que dá a escala)."""
    im = bpy.data.images.load(os.path.join(ROOT, "modelando ainda", "Futuras modelagens", "Roupas", sheet + ".png"))
    w, h = im.size
    px = im.pixels[:]
    cols, rows, start_frac = VOXEL_GRID[sheet]
    x0, x1 = int(w * col / cols) + 6, int(w * (col + 1) / cols) - 6
    y0, y1 = int(h * row / rows) + 6, int(h * (row + 1) / rows) - 6
    elf = sheet.startswith("E_")

    def rgb(x, y):
        i = ((h - 1 - y) * w + x) * 4
        return px[i] * 255, px[i + 1] * 255, px[i + 2] * 255

    def kinds(x, y):
        r, g, b = rgb(x, y)
        sat, lum = max(r, g, b) - min(r, g, b), (r + g + b) / 3
        if elf:
            return sat > 26 or lum < 88 or r >= b + 14, abs(lum - 172) < 24 and sat < 16
        return sat > 22 or lum < 150, sat <= 22 and 150 <= lum < 197
    mask, body = {}, {}
    for y in range(y0, y1):
        for x in range(x0, x1):
            item, flesh = kinds(x, y)
            if item:
                mask[(x, y)] = True
            elif flesh:
                body[(x, y)] = True
    mid = (x0 + x1) // 2

    def span(y):
        xs = [x for x in range(mid - int((x1 - x0) * .36), mid + int((x1 - x0) * .36)) if (x, y) in body]
        return (min(xs), max(xs)) if len(xs) > 8 else None
    # de baixo para cima: os ombros (largos) e depois o pescoço; o chapéu tapa a cabeça, então a largura dela é uma medida fixa da folha
    y = int(y0 + (y1 - y0) * start_frac)
    all_spans = {yy: span(yy) for yy in range(y0, y)}
    wide = max((s_[1] - s_[0] for s_ in all_spans.values() if s_), default=170)
    while y > y0 and not (all_spans.get(y) and all_spans[y][1] - all_spans[y][0] >= .85 * wide):
        y -= 1
    shoulders = all_spans.get(y)
    while y > y0 and all_spans.get(y) and all_spans[y][1] - all_spans[y][0] >= .73 * wide:
        y -= 1
    bottom = y - HEAD_NECK[sheet]
    W = HEAD_WIDTH[sheet]
    cx = (shoulders[0] + shoulders[1]) / 2 if shoulders else mid
    H_ = (HB.hi.z - HB.lo.z) / TEX
    pxt = W / 16.0
    return dict(rgb=rgb, mask=mask, cx=cx, top=bottom - H_ * pxt, bottom=bottom, pxt=pxt, x0=x0, x1=x1, y0=y0, y1=y1, H=H_, voxel=True, strip=False, back=None, arms=set())


def voxel_runs(V, r_stop=None):
    """As fileiras de meio texel do desenho, juntando as iguais: (c0, c1, r0, r1) na grade da cabeça."""
    def hit(c, r):
        x, y = int(V["cx"] + (c - 8) * V["pxt"]), int(V["top"] + r * V["pxt"])
        n = sum((x + i, y + j) in V["mask"] for i in (-1, 0, 1) for j in (-1, 0, 1))
        return n >= 5
    rmin = (V["y0"] - V["top"]) / V["pxt"]
    rmax = min((V["y1"] - V["top"]) / V["pxt"], r_stop if r_stop is not None else 1e9)
    cmin, cmax = 8 + (V["x0"] - V["cx"]) / V["pxt"], 8 + (V["x1"] - V["cx"]) / V["pxt"]
    out, open_runs = [], {}
    r = rmin
    while r < rmax:
        c, row = cmin, []
        while c < cmax:
            if hit(c, r + .25):
                a = c
                while c < cmax and hit(c, r + .25):
                    c += .5
                if c - a >= 1:
                    row.append((a, c))
            c += .5
        nxt = {}
        for span_ in row:
            key = next((k for k in open_runs if abs(k[0] - span_[0]) <= 1 and abs(k[1] - span_[1]) <= 1), None)
            nxt[span_] = open_runs.pop(key) if key else r
        for k, r0 in open_runs.items():
            out.append((k[0], k[1], r0, r))
        open_runs = nxt
        r += .5
    out += [(k[0], k[1], r0, r) for k, r0 in open_runs.items()]
    # fios soltos: sombra do boneco e borda do desenho, não a peça
    return [k for k in out if (k[3] - k[2]) >= 1 or (k[1] - k[0]) >= 4]


def voxel_hat(sheet, row, col):
    """O chapéu é o desenho em fatias: dentro da altura da cabeça, a fatia atravessa de frente a trás; acima dela, afina junto com a largura (a ponta fica pontuda, não um telhado)."""
    global VOXEL_REF
    V = voxel_read(sheet, row, col)
    VOXEL_REF = V
    H_ = V["H"]
    depth_all = (HB.hi.y - HB.lo.y) / TEX
    mid_y = depth_all / 2
    # a janela do rosto fica sempre livre: abaixo da testa só passam as laterais e a parte de trás
    TOP_FACE = 6.2
    runs = []
    for c0, c1, r0, r1 in voxel_runs(V, H_ + 9):
        if r1 <= TOP_FACE or c1 <= .4 or c0 >= 15.6 or r0 >= H_:
            runs.append((c0, c1, r0, r1, None))
            continue
        if r0 < TOP_FACE:
            runs.append((c0, c1, r0, TOP_FACE, None))
        low = max(r0, TOP_FACE)
        if c0 < .4:
            runs.append((c0, .4, low, r1, None))
        if c1 > 15.6:
            runs.append((15.6, c1, low, r1, None))
        runs.append((max(c0, .4), min(c1, 15.6), low, r1, "back"))
    for c0, c1, r0, r1, part in runs:
        if part == "back":
            hbox(c0, c1, r0, r1, depth_all + .1, depth_all + 1.2, "creme", [], "chapeu")
            continue
        w = c1 - c0
        if r0 >= 0 or w >= 15:
            d0, d1 = -1.2 if w < 24 else mid_y - w / 2, depth_all + 1.2 if w < 24 else mid_y + w / 2
        else:
            d = max(2.5, min(w, depth_all + 2.4))
            d0, d1 = mid_y - d / 2, mid_y + d / 2
        hbox(c0, c1, r0, r1, d0, d1, "creme", [], "chapeu")


def voxel_face(sheet, row, col, r_stop=None, r_from=None):
    """Rosto e pescoço: o desenho vira uma placa colada na frente da cabeça ou do peito."""
    global VOXEL_REF
    V = voxel_read(sheet, row, col)
    VOXEL_REF = V
    H_ = V["H"]
    x_mid = (HB.lo.x + HB.hi.x) / 2
    for c0, c1, r0, r1 in voxel_runs(V, r_stop):
        if r_from is not None and r1 <= r_from:
            continue
        if r0 < H_:
            hbox(c0, c1, r0, min(r1, H_), -1.5, -.8, "creme", [], "placa")
        if r1 > H_:
            lo = Vector((x_mid + (c0 - 8) * TEX, TORSO.lo.y - 1.9 * TEX, HB.hi.z - r1 * TEX))
            hi = Vector((x_mid + (c1 - 8) * TEX, TORSO.lo.y - 1.2 * TEX, HB.hi.z - max(r0, H_) * TEX))
            PIECES.append((Box(Matrix.Identity(4), lo, hi, 2, True), 0, "creme", [], (0, 0, 0), "placa"))
            PIECE_PART.append("torso")


def voxel_waist(sheet, row, col):
    """Cinto e faixa: a folha mostra o boneco só até o peito, com o cinto na barriga. O desenho desce para o quadril, centrado."""
    global VOXEL_REF
    V = voxel_read(sheet, row, col)
    VOXEL_REF = V
    runs = voxel_runs(V)
    cmin, cmax, rmin = min(k[0] for k in runs), max(k[1] for k in runs), min(k[2] for k in runs)
    V["dc"] = 8 - (cmin + cmax) / 2
    V["dr"] = (HB.hi.z - HIPS.hi.z) / TEX + .8 - rmin
    x_mid = (HB.lo.x + HB.hi.x) / 2
    for c0, c1, r0, r1 in runs:
        a, b = c0 + V["dc"], c1 + V["dc"]
        lo = Vector((x_mid + (a - 8) * TEX, HIPS.lo.y - 2.2 * TEX, HB.hi.z - (r1 + V["dr"]) * TEX))
        hi = Vector((x_mid + (b - 8) * TEX, HIPS.lo.y - 1.5 * TEX, HB.hi.z - (r0 + V["dr"]) * TEX))
        PIECES.append((Box(Matrix.Identity(4), lo, hi, 2, True), 0, "couro", [], (0, 0, 0), "placa"))
        PIECE_PART.append("quadril")
    waist_belt("couro")


def voxel_bag(sheet, row, col, x):
    """A bolsa: só a parte de baixo do desenho (sem a alça que na folha sobe até a cabeça), pendurada no quadril."""
    global VOXEL_REF
    V = voxel_read(sheet, row, col)
    VOXEL_REF = V
    runs = voxel_runs(V)
    rmin, rmax = min(k[2] for k in runs), max(k[3] for k in runs)
    low = [k for k in runs if k[2] >= rmin + (rmax - rmin) * .55]
    c_mid = (min(k[0] for k in low) + max(k[1] for k in low)) / 2
    r_mid = (min(k[2] for k in low) + max(k[3] for k in low)) / 2
    V["dc"] = 8 + x - c_mid
    V["dr"] = (HB.hi.z - (HIPS.lo.z - .5 * TEX)) / TEX - r_mid


def voxel_color(pw, face):
    V = VOXEL_REF
    x_mid = (HB.lo.x + HB.hi.x) / 2
    c, r = 8 + (pw.x - x_mid) / TEX - V.get("dc", 0), (HB.hi.z - pw.z) / TEX - V.get("dr", 0)
    x, y = int(V["cx"] + (c - 8) * V["pxt"]), int(V["top"] + r * V["pxt"])
    got = piece_pick(V, x, y, 1) if face in ("frente", "costas") else None
    if not got:   # os lados e o que passou do desenho: a cor mais comum de uma faixa de 5 texels (por fileira ficava listrado)
        band_top = int(V["top"] + (r // 5) * 5 * V["pxt"])
        row = sorted((V["rgb"](k, yy) for yy in range(band_top, band_top + int(5 * V["pxt"]), 3) for k in range(V["x0"], V["x1"], 3) if (k, yy) in V["mask"]), key=sum)
        row = row[len(row) // 2:len(row) // 2 + 1] if row else []
        if row:
            got = tuple(int(sum(p_[n] for p_ in row) / len(row)) // 22 * 22 + 11 for n in range(3))
    return tuple(c_ * .86 for c_ in got) if got and face == "costas" else got


def piece_ref_setup():
    if VOXEL_REF:
        return VOXEL_REF
    """Separa a peça do fundo cinza e do boneco e mede onde ela está no quadro. As mangas são medidas à parte: o braço da folha
    tem outro ângulo que o do herói em repouso."""
    if OUTFIT not in PIECE_SHEET:   # chapéus: a forma é diferente da foto, então são pintados à mão (cloth)
        return None
    sheet, row, col, strip = PIECE_SHEET[OUTFIT]
    path = os.path.join(ROOT, "modelando ainda", "Futuras modelagens", "Roupas", sheet + ".png")
    im = bpy.data.images.load(path)
    w, h = im.size
    px = im.pixels[:]
    mid = SHEET_COLS.get(sheet, SHEET_COLS["P_fem1"])[col]
    reach = .06 if sheet in SHEET_BACK else .093
    x0, x1 = max(0, int(w * (mid - reach))), min(w, int(w * (mid + reach)))
    edges = SHEET_ROWS.get(sheet, ROW_EDGES)
    y0, y1 = int(h * edges[row]), int(h * edges[row + 1])
    if sheet in SHEET_ROWS:   # sem a linha escura que separa as células
        y0, y1 = y0 + 8, y1 - 8
    def rgb(x, y):
        i = ((h - 1 - y) * w + x) * 4
        return px[i] * 255, px[i + 1] * 255, px[i + 2] * 255
    mask = {}
    for y in range(y0, y1):
        for x in range(x0, x1):
            r, g, b = rgb(x, y)
            if max(r, g, b) - min(r, g, b) > (10 if OUTFIT in PALE else 26) or (r + g + b) / 3 < 88 or (sheet.startswith("E_") and r >= b + 6):
                mask[(x, y)] = True
    def strict(k):   # a medida usa só o que é com certeza roupa (o tom quente solto pegava o pescoço do boneco e deslocava tudo)
        r, g, b = rgb(*k)
        return max(r, g, b) - min(r, g, b) > (10 if OUTFIT in PALE else 26) or (r + g + b) / 3 < 88
    front_mask = {k: True for k in mask if strict(k)}
    back = None
    if sheet in SHEET_BACK:
        mb = SHEET_BACK[sheet][col]
        for y in range(y0, y1):
            for x in range(max(0, int(w * (mb - .06))), min(w, int(w * (mb + .06)))):
                r, g, b = rgb(x, y)
                if max(r, g, b) - min(r, g, b) > (10 if OUTFIT in PALE else 26) or (r + g + b) / 3 < 88 or (sheet.startswith("E_") and r >= b + 6):
                    mask[(x, y)] = True
        got = [k for k in mask if abs(k[0] - w * mb) <= w * .06 and strict(k)]
        if got:
            back = ((min(k[0] for k in got), max(k[0] for k in got)), (min(k[1] for k in got), max(k[1] for k in got)))
    xs = [k[0] for k in front_mask]
    cx = (min(xs) + max(xs)) // 2
    arm_ids = [n for n in range(len(PIECES)) if PIECE_PART[n].split("_")[0] in ("braco", "antebraco", "mao")]
    has_arms = bool(arm_ids)
    half = int(TORSO_HALF * w) if has_arms or strip else w
    center = [k for k in front_mask if abs(k[0] - cx) <= half]
    if len(center) < 500:   # a peça clara demais para separar do boneco: fica a pintura à mão
        log("folha sem peça", sheet, row, col)
        return None
    if any(PIECE_PART[n] == "cabeca" for n in range(len(PIECES))):
        # chapéu: a pena e o que sai para fora não contam na medida, só o corpo do chapéu
        rows = {}
        for k in center:
            rows.setdefault(k[1], []).append(k[0])
        wide = max(max(v) - min(v) for v in rows.values())
        keep = {y for y, v in rows.items() if max(v) - min(v) >= .45 * wide}
        center = [k for k in center if k[1] in keep]
    pts = [PIECES[n][0].M @ Vector((a, b, c)) for n in range(len(PIECES)) if n not in arm_ids and PIECES[n][5] not in OWN_COLOR
           for B in [PIECES[n][0]] for a in (B.lo.x, B.hi.x) for b in (B.lo.y, B.hi.y) for c in (B.lo.z, B.hi.z)]
    if CLOTH_TUBE and not CLOTH_TUBE.get("sheet"):   # o pano não está em PIECES: entra na medida
        pts += [Vector((CLOTH_TUBE["cx"] + sx * CLOTH_TUBE["a"], CLOTH_TUBE["cy"] + sy * CLOTH_TUBE["b"], z)) for sx in (-1, 1) for sy in (-1, 1) for z in (CLOTH_TUBE["z_top"], CLOTH_TUBE["z_bot"])]
    log("costas da folha", back)
    R = {"back": back, "strip": strip, "px": px, "w": w, "h": h, "rgb": rgb, "mask": mask, "cx": cx, "half": half, "arms": set(arm_ids),
         "bx": (min(k[0] for k in center), max(k[0] for k in center)), "by": (min(k[1] for k in center), max(k[1] for k in center)),
         "X": (min(v.x for v in pts), max(v.x for v in pts)) if pts else (0, 1),
         "Y": (min(v.y for v in pts), max(v.y for v in pts)) if pts else (0, 1),
         "Z": (min(v.z for v in pts), max(v.z for v in pts)) if pts else (0, 1)}
    if has_arms:
        for s, side in (("E", "Left"), ("D", "Right")):
            a = rig.matrix_world @ rig.data.bones[P + side + "Arm"].head_local
            b = rig.matrix_world @ rig.data.bones[P + side + "Hand"].head_local
            right = a.x > (R["X"][0] + R["X"][1]) / 2
            arm = [k for k in front_mask if (k[0] > cx + half if right else k[0] < cx - half)]
            if not arm:
                return None
            ids = [n for n in arm_ids if PIECE_PART[n].endswith("_" + s)]
            tmax = max((PIECES[n][0].M @ Vector((c0, c1, c2)) - a).dot(b - a) / (b - a).length_squared for n in ids
                       for B in [PIECES[n][0]] for c0 in (B.lo.x, B.hi.x) for c1 in (B.lo.y, B.hi.y) for c2 in (B.lo.z, B.hi.z))
            R["arm_" + s] = {"a": a, "b": b, "right": right, "y": (min(k[1] for k in arm), max(k[1] for k in arm)), "pts": arm, "tmax": max(.05, min(1.0, tmax))}
            # 09/11: o eixo do braço na imagem (a manga é inclinada na folha: seguir a linha da imagem riscava a pintura)
            ys_ = sorted(k[1] for k in arm)
            lo_, hi_ = ys_[len(ys_) // 12], ys_[-len(ys_) // 12]
            tp = [k for k in arm if k[1] <= lo_]
            bt = [k for k in arm if k[1] >= hi_]
            ia = Vector((sum(k[0] for k in tp) / len(tp), sum(k[1] for k in tp) / len(tp)))
            ib = Vector((sum(k[0] for k in bt) / len(bt), sum(k[1] for k in bt) / len(bt)))
            ax_ = (ib - ia).normalized()
            pe_ = Vector((-ax_.y, ax_.x))
            R["arm_" + s].update(ia=ia, ib=ib, ip=pe_, iw=max(4.0, sorted(abs((Vector(k) - ia).dot(pe_)) for k in arm)[int(len(arm) * .85)]))
    if sheet == "H01":   # 10/11: a folha H01 tem o herói com cabeça e cabelo: a roupa começa no pescoço
        for key in ("by",):
            y0_, y1_ = R[key]
            R[key] = (int(y0_ + (y1_ - y0_) * .3), y1_)
        if R["back"]:
            (bx_, (y0_, y1_)) = R["back"]
            R["back"] = (bx_, (int(y0_ + (y1_ - y0_) * .3), y1_))
    R["wf"] = .39 if sheet == "H01" else .29   # onde fica a cintura entre o pescoço e o pé, na folha
    log("folha", sheet, row, col, "peca", R["bx"], R["by"])
    return R


def piece_trim(base, colors):
    """A cor de enfeite: a mais frequente bem diferente da base; sem nenhuma, um tom mais claro da própria base."""
    count = {}
    for c in colors:
        key = tuple(int(v) // 28 for v in c)
        count.setdefault(key, [0, c])[0] += 1
    far = [v for v in count.values() if sum(abs(a - b) for a, b in zip(v[1], base)) > 75]
    return max(far, key=lambda v: v[0])[1] if far else tuple(min(255, c * 1.35 + 12) for c in base)


def piece_pick(R, x, y, step_x):
    """Um pixel da folha, sem média (10/11, o usuário: "sem qualidade": a média apagava as bordas). Fora da peça, o vizinho mais perto."""
    def out(c):
        return tuple(int(v) // 12 * 12 + 6 for v in c)   # poucos tons: pixel art, como os trajes da ficha
    if (x, y) in R["mask"]:
        return out(R["rgb"](x, y))
    for d in range(1, 10):
        for i, j in ((d, 0), (-d, 0), (0, d), (0, -d), (d, d), (-d, d), (d, -d), (-d, -d)):
            if (x + i, y + j) in R["mask"]:
                return out(R["rgb"](x + i, y + j))
    if not ELF_DETAIL:
        for k in range(10, 45):
            if (x + step_x * k, y) in R["mask"]:
                return out(R["rgb"](x + step_x * k, y))
    return None


def piece_sample(pw, face, pi):
    R = ARMOR_REF
    if R.get("voxel"):
        # alça e cinto feitos à mão ficam na cor deles: o desenho só vale para a peça que veio da folha
        return None if pi >= 0 and PIECES[pi][5] in ("alca", "cinto", "bolsa", "mapa") else voxel_color(pw, face)
    if PIECES[pi][5] in OWN_COLOR:
        return None
    if pi in R["arms"]:
        s = PIECE_PART[pi].split("_")[-1]
        A = R["arm_" + s]
        axis = A["b"] - A["a"]
        t = max(0, min(.999, (pw - A["a"]).dot(axis) / axis.length_squared))
        t = min(.999, t / A["tmax"])   # manga curta: o fim dela é o fim da manga da folha
        if ELF_DETAIL and "ia" in A:
            ax = axis.normalized()
            perp = Vector((ax.z, 0, -ax.x)).normalized()
            # a manga em 10 trechos, cada um na cor mais comum daquele pedaço da folha (o punho muda de cor e nada risca)
            if "seg" not in A:
                ys_ = (min(k[1] for k in A["pts"]), max(k[1] for k in A["pts"]))
                # a manga numa cor só; só o punho (o último pedaço do desenho) tem a cor dele: 10 trechos viravam listras
                all_ = sorted((R["rgb"](*k) for k in A["pts"]), key=sum)
                base_ = all_[len(all_) // 2]
                y_cut = ys_[0] + (ys_[1] - ys_[0]) * .85
                cuff_ = sorted((R["rgb"](*k) for k in A["pts"] if k[1] >= y_cut), key=sum)
                A["seg"] = [base_] * 9 + [cuff_[len(cuff_) // 2] if cuff_ else base_]
            got = A["seg"][min(9, int(t * 10))]
            if got:
                tx_, ty_ = int((pw.x + pw.y) / (TEX / 4)), int(pw.z / (TEX / 4))
                k_ = 1.0 + ((tx_ * 2 + ty_) % 23 == 0) * -.09 + ((tx_ + ty_ * 3) % 29 == 0) * .07
                got = tuple(min(255, c * k_) for c in got)
                if "trim" not in A:
                    A["trim"] = piece_trim(A["seg"][4] or got, [c for c in A["seg"] if c] + [R["rgb"](*k) for k in A["pts"][::5]])
                # o punho, a costura do ombro e losangos bordados ao longo da manga
                dd = pw - (A["a"] + axis * t * A["tmax"])
                across = abs(dd.dot(perp) + dd.y) / (TEX / 4)
                along = int(t * A["tmax"] * axis.length / (TEX / 4))
                dark = tuple(c * .6 for c in got)
                if t > .88:
                    got = A["trim"]
                elif t > .85 or t < .03:
                    got = dark
                elif t < .8 and across + abs(along % 10 - 5) < 3:
                    got = A["trim"]
            return tuple(c * .84 for c in got) if got and face == "costas" else got
        y = int(A["y"][0] + t * (A["y"][1] - A["y"][0]))
        row = [k[0] for k in A["pts"] if k[1] == y]
        if not row:
            return None
        rx = (min(row), max(row))
        ax = axis.normalized()
        perp = Vector((ax.z, 0, -ax.x)).normalized()
        if perp.x < 0:
            perp = -perp
        off = (pw - (A["a"] + axis * t)).dot(perp) / (2.6 * TEX)
        u = max(0, min(1, (off + 1) / 2))
        if not A["right"]:
            u = 1 - u
        got = piece_pick(R, int(rx[0] + u * (rx[1] - rx[0])), y, 1 if u < .5 else -1)
        return tuple(c * .84 for c in got) if got and face == "costas" else got
    (X0, X1), (Y0, Y1), (Z0, Z1) = R["X"], R["Y"], R["Z"]
    v = min(.999, max(0, (Z1 - pw.z) / max(1e-6, Z1 - Z0)))
    if ELF_DETAIL and face in ("frente", "costas", "esq", "dir"):
        # 09/11 (o usuário: "riscadas e esticadas"): nada de esticar uma faixa estreita da folha sobre a face inteira. Lados e costas sem
        # desenho próprio repetem, na mesma escala, a faixa de fora da peça (espelhada para ir e voltar, sem cordões nem emendas)
        def flat(px_x, px_y, keys):
            """A cor mais comum da região, com dobras leves: as costas lisas e os lados, sem nenhuma faixa da folha."""
            if keys not in R:
                x0_, x1_, y0_, y1_ = keys
                got_ = sorted((R["rgb"](a_, b_) for a_ in range(x0_, x1_, 3) for b_ in range(y0_, y1_, 3) if (a_, b_) in R["mask"]), key=sum)
                R[keys] = got_[len(got_) // 2] if got_ else (120, 110, 100)
            k_ = 1.0 + ((px_x * 2 + px_y) % 23 == 0) * -.09 + ((px_x + px_y * 3) % 29 == 0) * .07
            return tuple(min(255, c_ * k_) for c_ in R[keys])
        (fx0, fx1), (fy0, fy1) = R["bx"], R["by"]
        whole = bool(R["back"])
        sx = (fy1 - fy0) / max(1e-6, Z1 - Z0) if whole else (fx1 - fx0) / max(1e-6, X1 - X0)
        dx = (pw.x - (X0 + X1) / 2) * sx
        dy = (pw.y - (Y0 + Y1) / 2) * sx
        # o traje inteiro: a cintura da folha e a do corpo no mesmo lugar (a escala de cima e a de baixo são diferentes)
        waist = TORSO.lo.z
        above = pw.z >= waist
        if whole:
            v = R.get("wf", .29) * (Z1 - pw.z) / max(1e-6, Z1 - waist) if above else R.get("wf", .29) + (1 - R.get("wf", .29)) * (waist - pw.z) / max(1e-6, waist - Z0)
            v = min(.999, max(0, v))
        if face == "costas" and whole and pi == -1:   # só o vestido tem a vista de costas aproveitável; pernas e mangas são pintadas
            (kx0, kx1), (ky0, ky1) = R["back"]
            mid = (kx0 + kx1) // 2
            x = int(mid - dx)
            return piece_pick(R, x, int(ky0 + v * (ky1 - ky0)), 1 if x < mid else -1)
        mid = (fx0 + fx1) // 2
        half = (fx1 - fx0) / 2
        y = int(fy0 + v * (fy1 - fy0))
        if face == "frente":
            x = int(mid + dx)
            return piece_pick(R, x, y, 1 if x < mid else -1)
        # as costas e os lados: lisos, na cor da peça (a folha só tem o desenho da frente, e repeti-lo fazia listras)
        tx_ = int(((pw.x if face == "costas" else pw.y) - X0) / (TEX / 4))
        ty_ = int((Z1 - pw.z) / (TEX / 4))
        q0 = int(fy0 + (0 if above or not whole else R.get("wf", .29)) * (fy1 - fy0))
        q1 = int(fy0 + (R.get("wf", .29) if above and whole else 1) * (fy1 - fy0))
        keys = (int(fx0), int(fx1), q0, max(q0 + 4, q1))
        got = flat(tx_, ty_, keys)
        if ("t",) + keys not in R:
            R[("t",) + keys] = piece_trim(R[keys], [R["rgb"](a_, b_) for a_ in range(keys[0], keys[1], 2) for b_ in range(keys[2], keys[3], 2) if (a_, b_) in R["mask"]])
        trim = R[("t",) + keys]
        # 09/11: as costas e os lados com costura, barra e bordado (a folha de peças só desenha a frente)
        step = TEX / 4
        ztop = Z1
        zbot = waist if whole and above else Z0
        tt, bb = (ztop - pw.z) / step, (pw.z - zbot) / step
        across = int(abs(pw.x - (X0 + X1) / 2) / (step * 2))
        dark = tuple(c * .6 for c in R[keys])
        paint = None
        if tt < 4 or bb < 5:
            paint = trim
            if bb < 5 and face != "esq" and (tx_ % 10) in (4, 5) and 1 <= bb < 4:
                paint = dark
        elif tt < 5 or bb < 6:
            paint = dark
        elif face == "costas":
            if across == 0:
                paint = dark
            elif above and (ztop - zbot) / step > 50 and 16 <= tt <= 76:
                d = ((tt - 16) / 2) % 11
                if across <= 0 or (d < 8 and abs(across - d) <= 1.1):
                    paint = trim
        elif abs(pw.y - (Y0 + Y1) / 2) < step:
            paint = dark
        if paint:
            got = paint
        return tuple(c * (.86 if face == "costas" else .94) for c in got)
    if face == "frente":
        u = (pw.x - X0) / max(1e-6, X1 - X0)
    elif face == "costas":   # as costas não copiam cordões e botões do meio da frente: usam a faixa lisa perto da beirada
        k = (X1 - pw.x) / max(1e-6, X1 - X0)
        u = k if not R["strip"] and v > .35 else .1 + .14 * k   # abaixo do cinto, o espelho da frente: a beirada caía no vão entre as pernas
    elif face in ("esq", "dir"):
        k = (pw.y - Y0) / max(1e-6, Y1 - Y0)
        u = .06 + .14 * k if face == "esq" else .94 - .14 * k
    else:
        return None
    if face == "costas" and R["back"]:   # o traje inteiro tem a vista de costas de verdade
        (bx0, bx1), (by0, by1) = R["back"]
        u = (X1 - pw.x) / max(1e-6, X1 - X0)
        x = int(bx0 + min(.999, max(0, u)) * (bx1 - bx0))
        y = int(by0 + v * (by1 - by0))
        return piece_pick(R, x, y, 1 if x < (bx0 + bx1) // 2 else -1)
    (bx0, bx1), (by0, by1) = R["bx"], R["by"]
    x = int(bx0 + min(.999, max(0, u)) * (bx1 - bx0))
    y = int(by0 + v * (by1 - by0))
    got = piece_pick(R, x, y, 1 if x < R["cx"] else -1)
    return tuple(c * .84 for c in got) if got and face == "costas" else got


# ---- ajudantes de forma (as cores das roupas com folha só valem onde a folha não tem pintura)


def shirt(c, sleeve="long", puff=0.0):
    shell("torso", color=c)
    shell("braco", 0, 1 if sleeve != "short" else .62, color=c, grow=(puff, puff, 0))
    if sleeve == "long":
        shell("antebraco", color=c, off=.6, grow=(puff, puff, 0), name="manga")
    elif sleeve == "rolled":
        shell("antebraco", 0, .4, color=c, off=.7, name="manga")


def vest(c, off=1.0):
    shell("torso", off=off, color=c, name="colete")


def cape(c, hood=False):
    cloth_sheet("costas", 2, 32, 22, c, 1.6)   # 09/11: a capa cai em tecido real
    shell("torso", 0, .2, off=1.3, color=c, decos=[hem(c, 1, 0), cloth(c)], name="capa")
    shell("braco", 0, .34, off=1.4, color=c, decos=[hem(c, 1, 0), cloth(c)], name="capa_ombro")
    if hood:
        block("torso", (0, 7, 5), (14, 4.5, 8), c, [cloth(c)], "capuz")


def fur_wrap(c):
    shell("torso", 0, .3, off=1.9, grow=(.6, .6, 0), color=c, decos=[jag(1), cloth(c)], name="pelo")
    for x in (-10.5, 10.5):
        block("torso", (x, 0, 6), (7, 11, 4.5), c, [jag(1), cloth(c)], "pelo_ombro")


def legs(c):
    shell("quadril", color=c)
    shell(["coxa", "canela"], color=c)


def long_skirt(c, length, hose=None):
    # 09/11: em tiras com ossos, para balançar com o movimento (a cintura fica fixa, com o corpo)
    shell("quadril", 0, .6, off=1.0, color=c, name="cintura")
    cloth_skirt(length, 1.6, c)
    if hose:
        shell(["coxa", "canela"], color=hose)


def boot(c, a=.5, off=.9):
    shell("canela", a, 1, off=off, color=c, name="bota")
    shell("pe", off=.5, color=c)


def shoe(c):
    shell("pe", off=.5, color=c)


def tiny(part, off, size, color, name="enfeite"):
    block(part, off, size, color, [cloth(color)], name)


def cap(rows, c, off=.7, dome=True):
    head_wrap(rows, c, decos=[cloth(c)], off=off, name="gorro")
    if dome:
        tiny("cabeca", (0, 0, 10.4), (12, 12, 1.8), c, "copa")


def brim(c, wide=5.0, z=4.2):
    w = 16 + 2 * wide
    tiny("cabeca", (0, 0, z), (w, w, 1.3), c, "aba")


def hood(c, drop=2.0):
    tiny("cabeca", (0, 0, 9.5), (19, 19, 2.4), c, "topo")
    for x in (-9.4, 9.4):
        tiny("cabeca", (x, 1, 1), (1.6, 17, 15 + drop), c, "lado")
    tiny("cabeca", (0, 9.4, 1), (19, 1.6, 15 + drop), c, "nuca")
    tiny("cabeca", (0, -9.2, 7.6), (19, 1.2, 2.2), c, "testa")


def cone(c, base=5.0):
    brim(c, base, 5)
    for k, (w, z) in enumerate(((15, 7.4), (12, 10), (9, 12.6), (6, 15.1), (3.5, 17.4))):
        tiny("cabeca", (k * .9, k * .6, z), (w, w, 2.8), c, "cone")


def band_head(c, a=.18, b=.32, off=.45):
    shell("cabeca", a, b, off=off, color=c, decos=[cloth(c)], name="tiara")


def wreath(leaf, flower=None):
    band_head(leaf, .1, .3, .6)
    for k in range(12):
        x, y = [(-7.4 + 2.96 * (k % 6), -8.8), (-7.4 + 2.96 * (k % 6), 8.8)][k // 6]
        tiny("cabeca", (x, y, 6.2 + (k % 2)), (2.4, 1.3, 2.4), flower if flower and k % 2 else leaf, "folha")
    for s in (-8.8, 8.8):
        for k in range(4):
            tiny("cabeca", (s, -5.5 + k * 3.6, 6.5), (1.3, 2.4, 2.4), flower if flower and k % 2 == 0 else leaf, "folha")


def feather(c1="branco", c2="areia", side=9.8):
    for k, (y, z, w) in enumerate(((3, 6, 3), (4.8, 8.6, 3), (6.4, 11, 2.6), (7.8, 13.2, 1.8))):
        block("cabeca", (side, y, z), (.8, w, 2.8), c1 if k % 2 else c2, [], "pena")


def chest_y():
    return -((TORSO.hi.y - TORSO.lo.y) / TEX / 4 + 1.0)


def neck_band(c):
    shell("torso", 0, .1, off=1.0, color=c, decos=[cloth(c)], name="gola")


def necklace(c, gem=None, drop=5):
    y = chest_y()
    for k in range(drop + 1):
        w = (drop - k) * .75 + .4
        for s in (-1, 1):
            tiny("torso", (s * w, y, 9 - k), (1, .6, 1.1), c, "corrente")
    if gem:
        tiny("torso", (0, y - .2, 9 - drop - 1.3), (2.6, .9, 2.6), gem, "pingente")


def scarf_front(c, w=11, h=8):
    neck_band(c)
    y = chest_y()
    for k in range(int(h)):
        tiny("torso", (0, y, 9 - k), (max(1.2, w * (1 - k / h)), .7, 1.1), c, "lenco")


def half_mask(c):
    for k in range(5):
        tiny("cabeca", (0, -8.7, -2.5 - k * 1.1), (15 - k * 2.6, .9, 1.2), c, "mascara")


def eyepatch(c):
    shell("cabeca", .33, .4, off=.3, color=c, decos=[cloth(c)], name="tira")
    tiny("cabeca", (4, -8.8, 1), (5, .9, 4.5), c, "tapa")


def waist_belt(c, buckle="bronze"):
    shell("quadril", 0, .28, off=1.15, color=c, decos=[rect(.42, .58, .05, .95, buckle, hollow=(c, 0)), cloth(c)], name="cinto")


def pouch(c, x=9, z=0):
    tiny("quadril", (x, -2, z), (4, 3.5, 4.6), c, "bolsa")


def strap(c, flip=1):
    y = chest_y()
    for k in range(14):
        tiny("torso", (flip * (6.4 - k), y, 7.4 - k * 1.05), (2.6, .7, 2), c, "alca")


def wrist(c, a=.6, b=.86, off=.85):
    shell("antebraco", a, b, off=off, color=c, decos=[hem("couro_esc", 1, 0), cloth(c)], name="pulseira")


def upper_band(c):
    shell("braco", .5, .72, off=.8, color=c, decos=[hem("couro_esc", 1, 0), cloth(c)], name="bracal")


def earrings(c):
    for x in (-8.7, 8.7):
        tiny("cabeca", (x, 0, -5.8), (1, 1, 1), "couro_esc", "gancho")
        tiny("cabeca", (x, 0, -7.6), (1.6, 1.6, 2.4), c, "brinco")


def horns(c):
    for s in (-1, 1):
        for k, (x, z) in enumerate(((9.8, 7), (11.4, 8.8), (12.2, 11), (12, 13))):
            tiny("cabeca", (s * x, 0, z), (2.8 - k * .4, 2.8 - k * .4, 2.6), c, "chifre")


# ---- o catálogo: id -> (espaço, nome, público, esconde o cabelo, folha ou None, desenho)
M, F = "M", "F"
T = "P_todas"
W = "P_fem1"
CATALOG = {
    # camisas masculinas
    "P_camisa_linho": ("camisa", "Camisa de Linho", M, False, (T, 0, 0, True), camisa_linho),
    "P_camisa_la_carmim": ("camisa", "Camisa de Lã Carmim", M, False, (T, 0, 1, True), lambda: shirt("vermelho", "short")),
    "P_camisa_musgo": ("camisa", "Camisa de Musgo", M, False, (T, 0, 2, True), lambda: shirt("verde", "rolled")),
    "P_camisa_azul": ("camisa", "Camisa Azul de Punhos", M, False, (T, 0, 3, True), lambda: shirt("azul", "long")),
    "P_camisa_acolchoada": ("camisa", "Camisa Acolchoada", M, False, (T, 0, 4, True), lambda: shirt("couro_esc", "long")),
    # parte de cima masculina
    "P_cima_colete_couro": ("cima", "Colete de Couro", M, False, (T, 1, 0, True), lambda: vest("couro")),
    "P_cima_gibao_carmim": ("cima", "Gibão Carmim", M, False, (T, 1, 1, True), lambda: vest("vermelho")),
    "P_cima_capa_capuz": ("cima", "Capa com Capuz", M, False, (T, 1, 2, False), lambda: cape("teal", True)),
    "P_cima_pele_ombros": ("cima", "Pele nos Ombros", M, False, None, lambda: fur_wrap("pelo")),
    "P_cima_tabardo_azul": ("cima", "Tabardo Azul", M, False, (T, 1, 4, True), lambda: vest("azul")),
    # calças masculinas
    "P_calca_la_marrom": ("calca", "Calça de Lã Marrom", M, False, (T, 2, 0, False), calca_la_marrom),
    "P_calca_cinza": ("calca", "Calça Cinza com Bolsa", M, False, (T, 2, 1, False), lambda: legs("cinza")),
    "P_calca_linho": ("calca", "Calça de Linho", M, False, (T, 2, 2, False), lambda: legs("areia")),
    "P_calca_couro": ("calca", "Calça de Couro", M, False, (T, 2, 3, False), lambda: legs("couro_esc")),
    "P_calca_musgo_saiote": ("calca", "Calça de Musgo com Saiote", M, False, (T, 2, 4, False), lambda: (legs("verde"), shell("quadril", 0, .6, off=1.0, color="verde", name="cintura"), cloth_skirt(8, 1.2, "verde"))),
    # calçados masculinos
    "P_calcado_amarrado": ("calcado", "Botas Amarradas", M, False, (T, 3, 0, False), lambda: boot("couro", .45)),
    "P_calcado_cano_alto": ("calcado", "Botas de Cano Alto", M, False, (T, 3, 1, False), lambda: boot("couro", .2)),
    "P_calcado_inverno": ("calcado", "Botas de Inverno", M, False, (T, 3, 2, False), lambda: boot("couro_esc", .4)),
    "P_calcado_pano": ("calcado", "Sapatos de Pano", M, False, (T, 3, 3, False), lambda: shoe("couro")),
    "P_calcado_biqueira": ("calcado", "Botas com Biqueira", M, False, (T, 3, 4, False), lambda: boot("couro_esc", .45)),
    # blusas femininas
    "P_blusa_linho": ("camisa", "Blusa de Linho", F, False, (W, 0, 0, True), lambda: shirt("creme", "long", .3)),
    "P_blusa_rosa": ("camisa", "Blusa Rosa de Lã", F, False, (W, 0, 1, True), lambda: shirt("rosa", "long", .3)),
    "P_blusa_azul_bordada": ("camisa", "Blusa Azul Bordada", F, False, (W, 0, 2, True), lambda: shirt("azul", "long", .3)),
    "P_blusa_vinho": ("camisa", "Blusa Vinho Bufante", F, False, (W, 0, 3, True), lambda: shirt("vinho", "long", .5)),
    "P_blusa_salvia": ("camisa", "Blusa de Sálvia", F, False, (W, 0, 4, True), lambda: shirt("verde", "long", .3)),
    # corpetes e parte de cima femininos
    "P_corpete_couro": ("cima", "Corpete de Couro", F, False, (W, 1, 0, True), lambda: vest("couro")),
    "P_corpete_verde": ("cima", "Corpete Verde", F, False, (W, 1, 1, True), lambda: vest("verde")),
    "P_capa_ameixa": ("cima", "Capa Ameixa", F, False, (W, 1, 2, False), lambda: cape("vinho")),
    "P_manto_pele_branco": ("cima", "Manto de Pele Branca", F, False, None, lambda: fur_wrap("creme")),
    "P_colete_azul_bordado": ("cima", "Colete Bordado", F, False, (W, 1, 4, True), lambda: vest("teal")),
    # saias femininas
    "P_saia_terra": ("calca", "Saia Longa de Lã", F, False, (W, 2, 0, False), lambda: long_skirt("couro", 26)),
    "P_saia_musgo": ("calca", "Saia Aberta de Musgo", F, False, (W, 2, 1, False), lambda: long_skirt("verde", 14, "couro_esc")),
    "P_saia_vinho": ("calca", "Saia Vinho Bordada", F, False, (W, 2, 2, False), lambda: long_skirt("vinho", 22)),
    "P_saia_couro_meia": ("calca", "Saiote de Couro", F, False, (W, 2, 3, False), lambda: long_skirt("cinza", 8, "azul_esc")),
    "P_saia_creme": ("calca", "Saia de Linho", F, False, (W, 2, 4, False), lambda: long_skirt("creme", 16)),
    # calçados femininos
    "P_sapato_cano_curto": ("calcado", "Botinhas Amarradas", F, False, (W, 3, 0, False), lambda: boot("couro", .6)),
    "P_sapato_cano_alto": ("calcado", "Botas Altas", F, False, (W, 3, 1, False), lambda: boot("couro_esc", .15)),
    "P_sapato_pano": ("calcado", "Sapatilhas Bordadas", F, False, (W, 3, 2, False), lambda: shoe("couro")),
    "P_sapato_pele": ("calcado", "Botinhas de Pele", F, False, (W, 3, 3, False), lambda: boot("couro", .55)),
    "P_sapato_verde": ("calcado", "Botas Verdes", F, False, (W, 3, 4, False), lambda: boot("verde", .4)),
    # chapéus masculinos
    "P_chapeu_capuz_marrom": ("chapeu", "Capuz de Feltro", M, True, None, lambda: hood("couro")),
    "P_chapeu_cacador": ("chapeu", "Chapéu de Caçador", M, True, None, chapeu_cacador),
    "P_chapeu_palha": ("chapeu", "Chapéu de Palha", M, True, None, lambda: (cap(5, "madeira_clara"), brim("madeira_clara"))),
    "P_chapeu_coifa": ("chapeu", "Coifa de Lã", M, True, None, lambda: hood("cinza", 0)),
    "P_chapeu_pele": ("chapeu", "Gorro com Pele", M, True, None, lambda: (cap(6, "couro"), shell("cabeca", .28, .44, off=1.6, color="branco", decos=[cloth("branco")], name="pele"))),
    "P_chapeu_mago": ("chapeu", "Chapéu de Mago", M, True, None, lambda: cone("azul_real")),
    "P_chapeu_boina": ("chapeu", "Boina Carmim", M, True, None, lambda: (cap(4, "vermelho"), tiny("cabeca", (2, 0, 10.8), (18, 16, 2.6), "vermelho", "boina"), tiny("cabeca", (6, -8.6, 7.5), (2.4, .8, 2.4), "bronze", "broche"))),
    "P_chapeu_tiara_bronze": ("chapeu", "Tiara de Bronze", M, False, None, lambda: (band_head("bronze"), tiny("cabeca", (0, -8.9, 4.6), (2.6, .9, 2.6), "azul_noite", "gema"))),
    "P_chapeu_chifres": ("chapeu", "Elmo de Chifres", M, True, None, lambda: (cap(6, "couro_esc"), horns("areia"))),
    "P_chapeu_viajante": ("chapeu", "Chapéu de Viajante", M, True, None, lambda: (cap(5, "preto"), brim("preto", 4), shell("cabeca", .26, .36, off=1, color="areia", decos=[cloth("areia")], name="corda"))),
    # acessórios masculinos
    "P_acess_lenco": ("acessorio", "Lenço Vermelho", M, False, None, lambda: scarf_front("vermelho")),
    "P_acess_dente": ("acessorio", "Colar de Dente", M, False, None, lambda: necklace("couro", "creme")),
    "P_acess_mascara": ("acessorio", "Meia Máscara", M, False, None, lambda: half_mask("preto")),
    "P_acess_corrente": ("acessorio", "Corrente de Bronze", M, False, None, lambda: necklace("bronze", "ouro")),
    "P_acess_tapa_olho": ("acessorio", "Tapa-olho", M, False, None, lambda: eyepatch("couro_esc")),
    "P_acess_cinto_bolsas": ("acessorio", "Cinto com Bolsas", M, False, None, lambda: (waist_belt("couro"), pouch("couro", -9), pouch("couro", 9))),
    "P_acess_alca_mapa": ("acessorio", "Alça com Mapa", M, False, None, lambda: (strap("couro"), tiny("torso", (-7.5, chest_y() - .4, -6), (5, 2, 3), "areia", "mapa"))),
    "P_acess_pulseiras": ("acessorio", "Pulseiras de Couro", M, False, None, lambda: wrist("couro")),
    "P_acess_bolsa_lateral": ("acessorio", "Bolsa Lateral", M, False, None, lambda: (strap("terra"), pouch("couro", -9, 1))),
    "P_acess_bracadeiras": ("acessorio", "Braçadeiras com Pele", M, False, None, lambda: (wrist("couro", .2, .95), shell("antebraco", .18, .3, off=1.3, color="branco", decos=[cloth("branco")], name="pele"))),
    # chapéus femininos
    "P_chapeu_lenco_linho": ("chapeu", "Lenço de Cabeça", F, True, None, lambda: (cap(6, "creme", .5), tiny("cabeca", (0, 9.2, 2), (5, 1.6, 3), "creme", "no"))),
    "P_chapeu_cacadora": ("chapeu", "Gorro de Caçadora", F, True, None, lambda: (cap(5, "verde"), brim("verde_folha", 1.5), feather())),
    "P_chapeu_palha_fita": ("chapeu", "Chapéu de Palha com Fita", F, True, None, lambda: (cap(5, "madeira_clara"), brim("madeira_clara"), shell("cabeca", .24, .34, off=.9, color="couro", decos=[cloth("couro")], name="fita"))),
    "P_chapeu_coroa_flores": ("chapeu", "Coroa de Flores", F, False, None, lambda: wreath("verde_folha", "lilas")),
    "P_chapeu_boina_vinho": ("chapeu", "Boina de Veludo", F, True, None, lambda: (cap(4, "vinho"), tiny("cabeca", (2, 0, 10.8), (18, 16, 2.6), "vinho", "boina"))),
    "P_chapeu_bruxa": ("chapeu", "Chapéu de Bruxa", F, True, None, lambda: cone("roxo")),
    "P_chapeu_tiara_prata": ("chapeu", "Tiara de Prata", F, False, None, lambda: (band_head("prata"), tiny("cabeca", (0, -8.9, 4.6), (2.6, .9, 2.6), "azul", "gema"))),
    "P_chapeu_veu": ("chapeu", "Véu com Capuz", F, True, None, lambda: hood("creme", 5)),
    "P_chapeu_folhas": ("chapeu", "Coroa de Folhas", F, False, None, lambda: wreath("verde")),
    "P_chapeu_capuz_inverno": ("chapeu", "Capuz de Inverno", F, True, None, lambda: hood("branco", 3)),
    # acessórios femininos
    "P_acess_fita": ("acessorio", "Fita no Pescoço", F, False, None, lambda: scarf_front("azul", 7, 3)),
    "P_acess_colar_verde": ("acessorio", "Colar de Pedra Verde", F, False, None, lambda: necklace("bronze", "esmeralda")),
    "P_acess_colar_conchas": ("acessorio", "Colar de Conchas", F, False, None, lambda: necklace("areia", "branco")),
    "P_acess_meio_veu": ("acessorio", "Meio Véu", F, False, None, lambda: half_mask("creme")),
    "P_acess_brincos": ("acessorio", "Brincos de Bronze", F, False, None, lambda: earrings("bronze")),
    "P_acess_cinto_ervas": ("acessorio", "Cinto de Ervas", F, False, None, lambda: (waist_belt("couro"), pouch("verde_folha", 9))),
    "P_acess_faixa_livro": ("acessorio", "Faixa com Livro", F, False, None, lambda: (waist_belt("vermelho", "vermelho"), tiny("quadril", (-9, -2, -1), (4.5, 2, 5.5), "couro_esc", "livro"))),
    "P_acess_punhos_bordados": ("acessorio", "Punhos Bordados", F, False, None, lambda: wrist("azul_noite")),
    "P_acess_bolsa_redonda": ("acessorio", "Bolsa Redonda", F, False, None, lambda: (strap("couro"), pouch("couro", -9, 1))),
    "P_acess_bracais_bronze": ("acessorio", "Braçais de Bronze", F, False, None, lambda: upper_band("bronze")),
}


# ---- 09/11: os elfos (folhas E_pecas_masc, E_pecas_fem e E_sets; o id P_elfo_* só aparece no menu do elfo)
def ear(segments, tone):
    for s in (-1, 1):
        for x, y, z, sx, sy, sz in segments:
            block("cabeca", (s * x, y, z), (sx, sy, sz), tone, [], "orelha")


EARS = {
    "curta": [(8.6, 1, -2, 1.2, 3, 4), (9.6, .5, 0, 1.2, 2, 2.5), (10.4, 0, 1.4, 1, 1.2, 1.6)],
    "longa": [(8.6, 1, -2, 1.2, 3.4, 4.4), (9.8, .8, -.6, 1.4, 2.6, 3), (11.2, .6, .6, 1.4, 2, 2.2), (12.6, .4, 1.6, 1.4, 1.4, 1.6), (13.9, .2, 2.4, 1.2, .9, 1)],
    "tras": [(8.6, 1, -2, 1.2, 3.4, 4.4), (8.9, 3.4, -.6, 1, 3, 3), (8.9, 6, .5, 1, 3, 2.2), (8.9, 8.4, 1.3, .9, 2.6, 1.4), (8.9, 10.4, 1.9, .8, 2, .9)],
    "caida": [(8.6, 1, -1, 1.2, 3.4, 4), (9.8, 1, -2.4, 1.4, 2.8, 3), (11, 1, -3.8, 1.2, 2.2, 2.6), (11.9, 1, -5.2, 1, 1.4, 1.8)],
}
for _kind, _seg in EARS.items():
    for _tone in ("clara", "escura"):
        CATALOG["P_orelha_%s_%s" % (_kind, _tone)] = ("orelha", "Orelha", M, False, None, (lambda s=_seg, t=_tone: ear(s, "pele_" + t)))

EM, EF = "E_pecas_masc", "E_pecas_fem"


def _row(sheet, row, slot, body, items):
    for col, (pid, name, build) in enumerate(items):
        CATALOG["P_elfo_" + pid] = (slot, name, body, False, (sheet, row, col, slot in ("camisa", "cima")), build)


_row(EM, 0, "camisa", M, [("camisa_marfim", "Camisa de Marfim", lambda: shirt("creme")), ("camisa_musgo", "Camisa de Musgo Élfica", lambda: shirt("verde")), ("camisa_lua", "Camisa da Lua", lambda: shirt("azul")), ("camisa_violeta", "Camisa Violeta", lambda: shirt("roxo")), ("camisa_carvao", "Camisa de Carvão", lambda: shirt("preto"))])
_row(EM, 1, "cima", M, [("colete_folhas", "Colete de Folhas", lambda: vest("couro")), ("tabardo_ouro", "Tabardo Dourado", lambda: vest("ouro")), ("capa_floresta", "Capa da Floresta", lambda: cape("verde")), ("manto_prata", "Manto de Prata", lambda: cape("creme")), ("arreio_negro", "Arreio Negro", lambda: vest("preto"))])
_row(EM, 2, "calca", M, [("calca_vinha", "Calça de Vinhas", lambda: legs("couro")), ("calca_musgo", "Calça de Musgo", lambda: legs("verde")), ("calca_seda", "Calça de Seda", lambda: legs("creme")), ("calca_violeta", "Calça Violeta", lambda: legs("roxo")), ("calca_negra", "Calça de Couro Negro", lambda: legs("preto"))])
_row(EM, 3, "calcado", M, [("bota_camurca", "Botas de Camurça", lambda: boot("couro", .45)), ("bota_folha", "Botas de Folha", lambda: boot("verde", .2)), ("sapato_prata", "Sapatos de Prata", lambda: shoe("prata")), ("bota_negra", "Botas Negras", lambda: boot("preto", .35)), ("bota_violeta", "Botas Violeta", lambda: boot("roxo", .35))])
_row(EF, 0, "camisa", F, [("blusa_marfim", "Blusa de Marfim", lambda: shirt("creme", "long", .3)), ("blusa_salvia", "Blusa de Sálvia Élfica", lambda: shirt("verde", "long", .3)), ("blusa_lua", "Blusa da Lua", lambda: shirt("azul", "long", .3)), ("blusa_violeta", "Blusa Violeta", lambda: shirt("roxo", "long", .3)), ("blusa_carvao", "Blusa de Carvão", lambda: shirt("preto", "long", .3))])
_row(EF, 1, "cima", F, [("corpete_folhas", "Corpete de Folhas", lambda: vest("couro")), ("corpete_ouro", "Corpete Dourado", lambda: vest("ouro")), ("capa_musgo", "Capa de Musgo", lambda: cape("verde")), ("manto_branco", "Manto Branco", lambda: cape("creme")), ("corpete_negro", "Corpete Negro", lambda: vest("preto"))])
_row(EF, 2, "calca", F, [("saia_musgo", "Saia Longa de Musgo", lambda: long_skirt("verde", 22)), ("saiote_camurca", "Saiote de Camurça", lambda: long_skirt("verde", 7, "couro")), ("saia_marfim", "Saia de Marfim", lambda: long_skirt("creme", 22)), ("saiote_violeta", "Saiote Violeta", lambda: long_skirt("roxo", 8, "roxo")), ("saia_negra", "Saia Negra", lambda: long_skirt("preto", 22))])
_row(EF, 3, "calcado", F, [("bota_verde", "Botas Verdes Élficas", lambda: boot("verde", .5)), ("bota_folha_f", "Botas de Folha Altas", lambda: boot("couro", .2)), ("sapatilha", "Sapatilhas de Prata", lambda: shoe("prata")), ("bota_negra_f", "Botas Negras", lambda: boot("preto", .35)), ("bota_violeta_f", "Botas Violeta", lambda: boot("roxo", .4))])
for _id, _name, _body, _hides, _build, _slot in (
    ("tiara_folha", "Tiara da Folha", M, False, lambda: (band_head("prata"), tiny("cabeca", (0, -8.9, 4.6), (2.6, .9, 2.6), "esmeralda", "gema")), "chapeu"),
    ("louros", "Coroa de Louros", M, False, lambda: wreath("verde"), "chapeu"),
    ("capuz_floresta", "Capuz da Floresta", M, True, lambda: hood("verde"), "chapeu"),
    ("colar_lua", "Colar da Lua", M, False, lambda: necklace("prata", "prata"), "acessorio"),
    ("mascara_negra", "Máscara Negra", M, False, lambda: half_mask("preto"), "acessorio"),
    ("tiara_azul", "Tiara Azul", F, False, lambda: (band_head("prata"), tiny("cabeca", (0, -8.9, 4.6), (2.6, .9, 2.6), "gelo", "gema")), "chapeu"),
    ("coroa_flores", "Coroa de Flores Élfica", F, False, lambda: wreath("verde_folha", "branco"), "chapeu"),
    ("veu_palido", "Véu Pálido", F, True, lambda: hood("creme", 4), "chapeu"),
    ("colar_verde", "Colar de Esmeralda", F, False, lambda: necklace("prata", "esmeralda"), "acessorio"),
    ("veu_escuro", "Véu Escuro", F, False, lambda: half_mask("roxo"), "acessorio")):
    CATALOG["P_elfo_" + _id] = (_slot, _name, _body, _hides, None, _build)


def elf_tunic(top, under, hose, boots):
    shirt(under, "long", .2)
    shell("torso", off=1.1, color=top, name="colete")
    shell("quadril", color=top, off=1.1, name="tunica")
    skirt(.5, 7, 1.0, top, tiers=2, name="barra")
    shell(["coxa", "canela"], color=hose)
    boot(boots, .35)


def elf_robe(c, under, shoes="prata"):
    """10/11: o manto longo em blocos, como o mago do início (sem o pano simulado): abas presas às pernas."""
    shirt(under, "long", .3)
    shell("torso", off=1.1, color=c, name="colete")
    shell("quadril", color=c, off=1.0, name="manto")
    shell("coxa", color=c, off=1.6, grow=(1.2, 1.2, 0), name="aba")
    shell("canela", 0, .7, color=c, off=1.6, grow=(1.2, 1.2, 0), name="aba_baixo")
    shell("canela", .7, 1, color=c, off=.6)
    shoe(shoes)


def h01_tunic(top, under, hose, boots, cape=None, fur=False, tabard=False):
    """Os trajes da folha H01: camisa, túnica até a coxa, calça e botas; capa curta, pele nos ombros ou tabardo."""
    shirt(under, "long", .2)
    shell("torso", off=1.1, color=top, name="colete")
    shell("quadril", color=top, off=1.1, name="tunica")
    shell("coxa", 0, .5 if not tabard else .9, color=top, off=1.6, grow=(.8, .8, 0), name="aba")
    shell(["coxa", "canela"], color=hose)
    boot(boots, .45)
    if cape:
        shell("torso", 0, .3, off=1.6, grow=(.5, .5, 0), color=cape, name="capa")
        shell("braco", 0, .34, off=1.5, color=cape, name="capa_ombro")
    if fur:
        fur_wrap("pelo")


# os trajes inteiros (Roupa_Elfo_*, Data/Looks/roupas.json)
CATALOG.update({
    "Elfo_floresta_m": ("traje", "Guardião da Floresta", M, False, ("E_sets", 0, 0, True), lambda: elf_tunic("verde", "creme", "couro_esc", "couro")),
    "Elfo_floresta_f": ("traje", "Guardiã da Floresta", F, False, ("E_sets", 1, 0, True), lambda: elf_tunic("verde", "creme", "couro_esc", "couro")),
    "Elfo_mago_m": ("traje", "Mago da Corte da Lua", M, False, ("E_sets", 0, 1, True), lambda: elf_robe("azul", "creme")),
    "Elfo_mago_f": ("traje", "Maga da Corte da Lua", F, False, ("E_sets", 1, 1, True), lambda: elf_robe("azul", "creme")),
    "Elfo_sombra_m": ("traje", "Caçador das Sombras", M, False, ("E_sets", 0, 2, True), lambda: elf_tunic("roxo", "preto", "preto", "preto")),
    "Elfo_sombra_f": ("traje", "Caçadora das Sombras", F, False, ("E_sets", 1, 2, True), lambda: elf_tunic("roxo", "preto", "preto", "preto")),
})
PIECE_SHEET = {k: v[4] for k, v in CATALOG.items() if v[4]}


# ---- 09/11: os chapéus saem das folhas; os acessórios do rosto e do pescoço também, e o resto continua desenhado à mão
AM, AF = "P_acess_masc", "P_acess_fem"
HAIR_MODE = {}   # id -> "tudo" (esconde), "franja" (corta o cabelo acima da aba) ou "mostra"
for _id, _sheet, _r, _c, _mode in (
    ("P_chapeu_capuz_marrom", AM, 0, 0, "tudo"), ("P_chapeu_cacador", AM, 0, 1, "franja"), ("P_chapeu_palha", AM, 0, 2, "franja"), ("P_chapeu_coifa", AM, 0, 3, "tudo"), ("P_chapeu_pele", AM, 0, 4, "franja"),
    ("P_chapeu_mago", AM, 1, 0, "franja"), ("P_chapeu_boina", AM, 1, 1, "franja"), ("P_chapeu_tiara_bronze", AM, 1, 2, "mostra"), ("P_chapeu_chifres", AM, 1, 3, "tudo"), ("P_chapeu_viajante", AM, 1, 4, "franja"),
    ("P_chapeu_lenco_linho", AF, 0, 0, "tudo"), ("P_chapeu_cacadora", AF, 0, 1, "franja"), ("P_chapeu_palha_fita", AF, 0, 2, "franja"), ("P_chapeu_coroa_flores", AF, 0, 3, "mostra"), ("P_chapeu_boina_vinho", AF, 0, 4, "franja"),
    ("P_chapeu_bruxa", AF, 1, 0, "franja"), ("P_chapeu_tiara_prata", AF, 1, 1, "mostra"), ("P_chapeu_veu", AF, 1, 2, "tudo"), ("P_chapeu_folhas", AF, 1, 3, "mostra"), ("P_chapeu_capuz_inverno", AF, 1, 4, "tudo"),
    ("P_elfo_tiara_folha", EM, 4, 0, "mostra"), ("P_elfo_louros", EM, 4, 1, "mostra"), ("P_elfo_capuz_floresta", EM, 4, 2, "tudo"),
    ("P_elfo_tiara_azul", EF, 4, 0, "mostra"), ("P_elfo_coroa_flores", EF, 4, 1, "mostra"), ("P_elfo_veu_palido", EF, 4, 2, "tudo")):
    v = CATALOG[_id]
    HAIR_MODE[_id] = _mode
    CATALOG[_id] = (v[0], v[1], v[2], _mode == "tudo", None, (lambda a=_sheet, b=_r, c=_c: voxel_hat(a, b, c)))


def veil(c):
    """Um véu que cai atrás da cabeça, em tecido real."""
    cloth_strip("costas", "Head", HB.lo.z + 7 * TEX, HB.lo.z - 9 * TEX, 13, c, HB.hi.y + 2.2 * TEX)


def pendant_cloth(c):
    """O pingente balança: um pedacinho de pano no peito; a corrente fica fixa."""
    t = TORSO
    cloth_strip("frente", "Spine2", t.hi.z - 5.5 * TEX, t.hi.z - 11 * TEX, 3.4, c, t.lo.y - 2.2 * TEX)


def pouch_cloth(c, x=-9):
    h = HIPS
    cloth_strip("frente", "Hips", h.lo.z + 3 * TEX, h.lo.z - 4 * TEX, 4.6, c, h.lo.y - 1.5 * TEX, x)


for _pid, _extra in (("P_chapeu_veu", lambda: veil("creme")), ("P_elfo_veu_palido", lambda: veil("creme"))):
    _v = CATALOG[_pid]
    CATALOG[_pid] = (_v[0], _v[1], _v[2], _v[3], None, (lambda f=_v[5], g=_extra: (f(), g())))

# cada coisa no lugar certo do corpo: o desenho vem da folha, a posição não
for _id, _sheet, _r, _c, _build in (
    ("P_acess_cinto_bolsas", AM, 3, 0, lambda f: f()),):
    v = CATALOG[_id]
    CATALOG[_id] = (v[0], v[1], v[2], False, None, (lambda a=_sheet, b=_r, c=_c, g=_build: g(lambda: voxel_waist(a, b, c))))
# o cinto de corda e a faixa têm pontas soltas na folha, que desciam até o joelho: ficam desenhados à mão
_v = CATALOG["P_acess_cinto_ervas"]
CATALOG["P_acess_cinto_ervas"] = (_v[0], _v[1], _v[2], False, None, (lambda: (waist_belt("couro"), pouch_cloth("verde_folha", 9))))
for _id, _sheet, _r, _c in (("P_acess_bolsa_lateral", AM, 3, 3), ("P_acess_bolsa_redonda", AF, 3, 3)):
    v = CATALOG[_id]
    CATALOG[_id] = (v[0], v[1], v[2], False, None, (lambda a=_sheet, b=_r, c=_c: (strap("terra"), voxel_bag(a, b, c, -9), pouch_cloth("couro", -9))))

# os 3 espaços de acessório
for _id in ("P_acess_cinto_bolsas", "P_acess_alca_mapa", "P_acess_bolsa_lateral", "P_acess_cinto_ervas", "P_acess_faixa_livro", "P_acess_bolsa_redonda"):
    CATALOG[_id] = ("cintura",) + CATALOG[_id][1:]
for _id in ("P_acess_pulseiras", "P_acess_bracadeiras", "P_acess_punhos_bordados", "P_acess_bracais_bronze"):
    CATALOG[_id] = ("bracos",) + CATALOG[_id][1:]
# 10/11: o busto do corpo feminino (fora do menu: o personagem veste sozinho, como as orelhas)
CATALOG["P_busto"] = ("busto", "Busto", F, False, None, lambda: block("torso", (0, 0, 0), (1, 1, 1), "top_f", [], "oculto"))

# 10/11 (o usuário: "pode tirar os chapéus"): ficam só os acessórios
for _k in [k for k, v in CATALOG.items() if v[0] == "chapeu"]:
    del CATALOG[_k]
for _k in ("P_cima_capa_capuz", "P_capa_ameixa", "P_elfo_capa_floresta", "P_elfo_manto_prata", "P_elfo_capa_musgo", "P_elfo_manto_branco"):
    CATALOG[_k] = CATALOG[_k][:4] + (None,) + CATALOG[_k][5:]
# 10/11 (o usuário: "presets como os 3 primeiros"): os 5 trajes da folha H01, masculino e feminino
for _col, (_id, _nm, _nf, _build) in enumerate((
        ("peregrino", "Peregrino", "Peregrina", lambda: h01_tunic("creme", "creme", "couro", "couro", cape="teal")),
        ("escudeiro", "Escudeiro", "Escudeira", lambda: h01_tunic("vermelho", "vermelho", "couro_esc", "couro")),
        ("mago", "Mago andarilho", "Maga andarilha", lambda: elf_robe("roxo", "roxo", "couro")),
        ("cacador", "Caçador da floresta", "Caçadora da floresta", lambda: h01_tunic("verde", "verde", "couro", "couro", fur=True)),
        ("arauto", "Arauto", "Arauta", lambda: h01_tunic("azul", "creme", "cinza", "couro", tabard=True)))):
    CATALOG["Traje_%s_m" % _id] = ("traje", _nm, M, False, ("H01", 0, _col, True), _build)
    CATALOG["Traje_%s_f" % _id] = ("traje", _nf, F, False, ("H01", 1, _col, True), _build)
PIECE_SHEET = {k: v[4] for k, v in CATALOG.items() if v[4]}


def PIECE_BUILD(piece_id):
    CATALOG[piece_id][5]()


if __name__ == "__main__":
    # python Tools/Blender/piece_designs.py  (sem Blender): reescreve Data/Looks/pecas.json
    import json
    import pathlib
    # os trajes inteiros ficam em roupas.json; "tecido": tem a malha <id>_tecido, com a saia que balança
    FRANJA_Z = 133.0   # cm no modelo em repouso: acima daqui o cabelo é cortado nos chapéus "franja" (M_BlockHair: ClipZ)
    out = {k: {"slot": v[0], "name": v[1], "audience": v[2], "hidesHair": v[3]} for k, v in CATALOG.items() if v[0] != "traje"}
    for k, v in out.items():
        if k.startswith("P_elfo_"):
            v["race"] = "elfo"
    for k in ("P_acess_bolsa_lateral", "P_acess_bolsa_redonda", "P_acess_cinto_ervas"):
        out[k]["tecido"] = True
    for k in ("P_saia_terra", "P_saia_musgo", "P_saia_vinho", "P_saia_couro_meia", "P_saia_creme", "P_elfo_saia_musgo", "P_elfo_saiote_camurca", "P_elfo_saia_marfim", "P_elfo_saiote_violeta", "P_elfo_saia_negra",
              "P_calca_musgo_saiote", "P_cima_capa_capuz", "P_capa_ameixa", "P_elfo_capa_floresta", "P_elfo_manto_prata", "P_elfo_capa_musgo", "P_elfo_manto_branco"):
        out[k]["tecido"] = True
    path = pathlib.Path(__file__).resolve().parents[2] / "Data" / "Looks" / "pecas.json"
    path.write_text(json.dumps({"franjaZ": FRANJA_Z, "pieces": out}, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(len(out), "peças em", path)
