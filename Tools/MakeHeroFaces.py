"""Os quadros do rosto animado (estilo Hytale) de cada herói do Mixamo, desenhados em cima da frente da cabeça
(Tools/Blender/BakeHeroFace.py).

python Tools/MakeHeroFaces.py [raça ...]

Acha os olhos pelo branco, as sobrancelhas logo acima e a boca abaixo, e desenha no mesmo estilo de pixels grandes da
textura:
- olhos (Face/eyes.png, 4x2): aberto, meio fechado, fechado, olhando à esquerda, à direita, bravo, feliz (^ ^), dor;
- boca (Face/mouth.png, 4x2): neutra, A, O, E, sorriso, dentes cerrados, M (fechada), grito.
Só os pixels de pele mudam na boca (o bigode do anão fica por cima). Os retângulos dos olhos e da boca (no retângulo da
cabeça, de 0 a 1, v para cima) vão para Face/face.json; a Unreal (Tools/ImportHeroFace.py) projeta os quadros na frente
da cabeça. Uma folha de conferência sai em Face/preview.png.
"""
import json
import os
import sys

from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EYE_FRAMES = ["aberto", "meio", "fechado", "esquerda", "direita", "bravo", "feliz", "dor"]
MOUTH_FRAMES = ["neutra", "A", "O", "E", "sorriso", "dentes", "M", "grito"]


def lum(c):
    return .3 * c[0] + .59 * c[1] + .11 * c[2]


def components(mask, w, h):
    """As manchas ligadas de um conjunto de pixels: lista de listas de (x, y)."""
    seen, out = set(), []
    for start in mask:
        if start in seen:
            continue
        stack, blob = [start], []
        seen.add(start)
        while stack:
            x, y = stack.pop()
            blob.append((x, y))
            for n in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if n in mask and n not in seen:
                    seen.add(n)
                    stack.append(n)
        out.append(blob)
    return out


def bbox(points):
    xs, ys = [p[0] for p in points], [p[1] for p in points]
    return min(xs), min(ys), max(xs) + 1, max(ys) + 1


def make(race):
    folder = os.path.join(ROOT, "Data", "Heroes", race + "_Mixamo", "Face")
    head = Image.open(os.path.join(folder, "head.png")).convert("RGBA")
    meta = json.load(open(os.path.join(folder, "face.json"), encoding="utf-8"))
    W, H = head.size
    px = head.load()
    # o branco dos olhos: claro e sem cor, na metade de cima do rosto
    white = {(x, y) for y in range(int(H * .25), int(H * .8)) for x in range(W)
             if px[x, y][3] > 200 and min(px[x, y][:3]) > 175 and max(px[x, y][:3]) - min(px[x, y][:3]) < 40}
    blobs = sorted(components(white, W, H), key=len, reverse=True)
    pairs = sorted(blobs[:4], key=len, reverse=True)[:2]
    if len(pairs) < 2:
        raise SystemExit("%s: não achei os dois olhos" % race)
    # a pele: a cor mais comum logo abaixo de cada olho (a bochecha)
    samples = []
    for blob in pairs:
        x0, y0, x1, y1 = bbox(blob)
        for y in range(y1 + 4, min(H, y1 + 12)):
            for x in range(x0, x1):
                if px[x, y][3] > 200:
                    samples.append(tuple(v // 6 * 6 for v in px[x, y][:3]))
    skin = max(set(samples), key=samples.count)
    is_skin = lambda c: c[3] > 200 and sum(abs(c[i] - skin[i]) for i in range(3)) < 70
    # o olho inteiro: o branco mais tudo o que não é pele ligado a ele (a íris, o brilho e o contorno)
    full, eyes = [], []
    for blob in pairs:
        x0, y0, x1, y1 = bbox(blob)
        ew, eh = x1 - x0, y1 - y0
        lim = (x0 - 2 * ew, y0 - int(eh * .35), x1 + 2 * ew, y1 + int(eh * .35))
        seen, stack = set(blob), list(blob)
        while stack:
            x, y = stack.pop()
            for n in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if n in seen or not (lim[0] <= n[0] < lim[2] and lim[1] <= n[1] < lim[3]) or not (0 <= n[0] < W and 0 <= n[1] < H):
                    continue
                c = px[n]
                if c[3] > 200 and not is_skin(c):
                    seen.add(n)
                    stack.append(n)
        full.append(bbox(list(seen)))
        eyes.append(bbox(blob) if len(seen) == len(blob) else bbox([p for p in seen if min(px[p][:3]) > 175]))
    order = sorted(range(2), key=lambda i: full[i][0])
    full, eyes = [full[i] for i in order], [eyes[i] for i in order]
    # o branco do olho inteiro (de um lado e do outro da íris)
    eyes = [(f[0], f[1], f[2], f[3]) for f in full]
    # ajuste à mão por raça (Face/ajuste.json, em pixels do head.png): os olhos de cada um (x0, y0, x1, y1) e a boca
    # (centro x, centro y, largura, altura) — no demônio o cabelo escuro encosta nos olhos e a busca juntava os dois; no
    # elfo a boca é uma linha fina e a busca caía no queixo
    tune = {}
    tune_path = os.path.join(folder, "ajuste.json")
    if os.path.exists(tune_path):
        with open(tune_path, encoding="utf-8") as f:
            tune = json.load(f)
    if "eyes" in tune:
        full = eyes = [tuple(b) for b in sorted(tune["eyes"], key=lambda b: b[0])]
    eye_h = max(b[3] - b[1] for b in full)
    eye_w = max(b[2] - b[0] for b in full)
    grid = max(3, round(eye_h / 9))           # o tamanho do "pixel" do estilo
    cx = (full[0][2] + full[1][0]) // 2
    below = max(b[3] for b in full)
    dark = min((px[x, y][:3] for b in full for y in range(b[1], b[3]) for x in range(b[0], b[2]) if px[x, y][3] > 200), key=lum)
    lash = tuple(int(v * .8) for v in dark)
    # região dos olhos: das sobrancelhas (1 olho acima) até um pouco abaixo dos olhos, de um olho ao outro
    region_eye = (max(0, full[0][0] - grid * 2), max(0, min(b[1] for b in full) - int(eye_h * 1.2)),
                  min(W, full[1][2] + grid * 2), min(H, max(b[3] for b in full) + grid * 2))
    # a boca, descendo do olho: a primeira linha escura no meio da pele (o humano); se vier antes o bigode, o vão de pele
    # logo abaixo dele (o anão)
    def skin_share(y):
        row = [px[x, y] for x in range(cx - int(eye_w * .6), cx + int(eye_w * .6))]
        return sum(1 for c in row if is_skin(c)) / max(1, len(row)), row

    def hair_band(y):
        """Uma faixa alta sem pele (o bigode); a linha da boca desenhada é fina."""
        return all(skin_share(k)[0] < .2 for k in range(y, min(H, y + max(grid * 3, int(eye_h * .25)))))

    # a boca desenhada na pele (o humano): uma linha escura antes de qualquer pelo
    mouth_y, hair_at = None, None
    for y in range(below + grid * 2, min(H, below + eye_h * 5)):
        share, row = skin_share(y)
        if share < .2 and hair_band(y):
            hair_at = y
            break
        darkish = sum(1 for c in row if c[3] > 200 and lum(c) < lum(skin) * .85 and not is_skin(c) and c[0] >= c[2])
        # uma linha escura na pele que não seja a sombra do nariz logo em cima do bigode
        if share > .5 and darkish >= len(row) * .04 and not any(hair_band(k) for k in range(y, min(H, y + grid * 3), max(1, grid // 2))):
            mouth_y = y
            break
    # senão o bigode (o anão): a boca é o vão de pele logo abaixo dele
    if mouth_y is None and hair_at is not None:
        for y in range(hair_at, min(H, hair_at + eye_h * 3)):
            if skin_share(y)[0] > .45:
                gap = y
                while gap + 1 < H and skin_share(gap + 1)[0] > .45:
                    gap += 1
                mouth_y = (y + gap) // 2
                break
    if mouth_y is None:
        mouth_y = below + eye_h
    mw, mh = int(eye_w * 1.6), int(eye_h * .8)
    region_mouth = (cx - mw // 2 - grid, mouth_y - mh // 2, cx + mw // 2 + grid, mouth_y + mh // 2 + grid)
    if "mouth" in tune:
        mx, my, mw_, mh_ = tune["mouth"]
        region_mouth = (int(mx - mw_ / 2), int(my - mh_ / 2), int(mx + mw_ / 2), int(my + mh_ / 2))

    def snap(v):
        return int(round(v / grid) * grid)

    def blocks(draw, x0, y0, x1, y1, color):
        draw.rectangle([snap(x0), snap(y0), max(snap(x0) + grid, snap(x1)) - 1, max(snap(y0) + grid, snap(y1)) - 1], fill=color)

    def line(draw, p0, p1, color):
        """Uma linha em blocos do tamanho do pixel do estilo."""
        steps = max(1, int(max(abs(p1[0] - p0[0]), abs(p1[1] - p0[1])) / (grid * .5)))
        for k in range(steps + 1):
            t = k / steps
            x, y = p0[0] + (p1[0] - p0[0]) * t, p0[1] + (p1[1] - p0[1]) * t
            blocks(draw, x - grid * .5, y - grid * .5, x + grid * .5, y + grid * .5, color)

    # ---- olhos
    base_eye = head.crop(region_eye)
    ox, oy = region_eye[0], region_eye[1]
    local = [(b[0] - ox, b[1] - oy, b[2] - ox, b[3] - oy) for b in full]
    mid_x = region_eye[2] - region_eye[0]
    mid_x = (local[0][2] + local[1][0]) / 2
    # sobrancelhas: as manchas que não são pele acima dos olhos e não encostam no topo (o cabelo encosta)
    top_eye = min(b[1] for b in local)
    above = {(x, y) for y in range(0, top_eye) for x in range(base_eye.width)
             if base_eye.getpixel((x, y))[3] > 200 and not is_skin(base_eye.getpixel((x, y)))}
    brows = [b for b in components(above, base_eye.width, top_eye) if min(p[1] for p in b) > 0 and len(b) > grid * grid]
    sclera = max((base_eye.getpixel((x, y)) for (x0, y0, x1, y1) in local for y in range(y0, y1) for x in range(x0, x1)), key=lum)

    def fill(im, box, cover_to=None):
        """Pele por cima do olho (a pálpebra): até a altura cover_to(x) de cada coluna, ou o olho todo."""
        x0, y0, x1, y1 = box
        for x in range(x0, x1):
            yy = y1 if cover_to is None else int(cover_to(x))
            for y in range(y0, min(y1, yy)):
                if im.getpixel((x, y))[3] > 200:
                    im.putpixel((x, y), skin + (255,))

    # as cores do olho original: o branco, a íris (o tom do meio), a pupila (o mais escuro) e o contorno
    # a íris pelo miolo de cada olho (sem a borda, que é contorno ou sobrancelha): a cor mais comum que não é branco,
    # pele nem quase preto
    inner_px = []
    for (x0, y0, x1, y1) in local:
        for y in range(y0 + (y1 - y0) // 4, y1 - (y1 - y0) // 5):
            for x in range(x0 + (x1 - x0) // 5, x1 - (x1 - x0) // 5):
                c = base_eye.getpixel((x, y))
                if c[3] > 200 and not is_skin(c) and not (min(c[:3]) > 175 and max(c[:3]) - min(c[:3]) < 40):
                    inner_px.append(tuple(v // 8 * 8 for v in c[:3]))
    mids = [c for c in inner_px if lum(c) > 35] or inner_px or [(90, 60, 40)]
    iris_c = max(set(mids), key=mids.count)
    pupil = tuple(int(v * .35) for v in iris_c)
    shine = (255, 255, 255)

    def draw_eye(im, box, look, i):
        """O olho aberto redesenhado em blocos: contorno, branco, íris na direção do olhar, pupila e brilho."""
        x0, y0, x1, y1 = box
        d = ImageDraw.Draw(im)
        fill(im, box)
        sx0, sy0, sx1, sy1 = x0, y0 + grid, x1, y1
        blocks(d, sx0, sy0, sx1, sy1, sclera)
        iw = (sx1 - sx0) * .56
        center = (sx0 + sx1) / 2 + look * (sx1 - sx0 - iw) / 2
        ix0, ix1 = center - iw / 2, center + iw / 2
        blocks(d, ix0, sy0, ix1, sy1, iris_c + (255,))
        pw = iw * .5
        blocks(d, center - pw / 2, sy0 + (sy1 - sy0) * .25, center + pw / 2, sy1 - (sy1 - sy0) * .1, pupil + (255,))
        blocks(d, ix0 + grid * .5, sy0 + grid * .5, ix0 + grid * 1.5, sy0 + grid * 1.5, shine + (255,))
        # a pálpebra de cima mais grossa (a linha dos cílios)
        blocks(d, x0, y0, x1, y0 + grid, lash + (255,))
        # a ponta de fora dos cílios desce um bloco
        outer = x0 if i == 0 else x1 - grid
        blocks(d, outer - (grid if i == 0 else -grid), y0, outer + (0 if i == 0 else grid), y0 + grid * 2, lash + (255,))

    # ajuste: o olho original pintado de pele antes (o olho redesenhado fica menor que o contorno original)
    if "clear_eyes" in tune:
        cleared = base_eye.copy()
        for x0, y0, x1, y1 in tune["clear_eyes"]:
            for y in range(max(0, y0 - oy), min(cleared.height, y1 - oy)):
                for x in range(max(0, x0 - ox), min(cleared.width, x1 - ox)):
                    if cleared.getpixel((x, y))[3] > 200:
                        cleared.putpixel((x, y), skin + (255,))
        base_eye = cleared
    frames = []
    for name in EYE_FRAMES:
        im = base_eye.copy()
        for i, box in enumerate(local):
            look = {"esquerda": -1, "direita": 1}.get(name, 0)
            if name in ("aberto", "meio", "bravo", "esquerda", "direita"):
                draw_eye(im, box, look, i)
        d = ImageDraw.Draw(im)
        for i, (x0, y0, x1, y1) in enumerate(local):
            h, w = y1 - y0, x1 - x0
            inner_side = x1 if i == 0 else x0          # o lado do nariz
            outer_side = x0 if i == 0 else x1
            if name == "meio":
                fill(im, local[i], lambda x: y0 + h * .5)
                line(d, (x0, y0 + h * .5), (x1, y0 + h * .5), lash)
            elif name == "fechado":
                fill(im, local[i])
                line(d, (x0, y0 + h * .55), (x1, y0 + h * .6), lash)
            elif name == "feliz":
                fill(im, local[i])
                line(d, (x0 + grid * .5, y0 + h * .7), ((x0 + x1) / 2, y0 + h * .35), lash)
                line(d, ((x0 + x1) / 2, y0 + h * .35), (x1 - grid * .5, y0 + h * .7), lash)
            elif name == "dor":
                fill(im, local[i])
                line(d, (outer_side, y0 + h * .25), (inner_side - (grid if i == 0 else -grid), y0 + h * .5), lash)
                line(d, (inner_side - (grid if i == 0 else -grid), y0 + h * .5), (outer_side, y0 + h * .75), lash)
            elif name == "bravo":
                # a pálpebra desce em diagonal: mais baixa do lado do nariz
                top = lambda x, o=outer_side, n=inner_side: y0 + h * (.15 + .35 * (abs(x - o) / max(1, abs(n - o))))
                fill(im, local[i], top)
                line(d, (outer_side, top(outer_side)), (inner_side, top(inner_side)), lash)
            elif name in ("esquerda", "direita") and False:
                # tudo dentro do contorno (a íris e o brilho juntos) desliza para o lado, em passos do pixel do estilo;
                # o lado que fica vazio vira branco (antes só a íris andava e o brilho ficava: faixa branca dupla)
                m = max(grid, int(min(w, h) * .12))
                ix0, iy0, ix1, iy1 = x0 + m, y0 + m, x1 - m, y1 - m
                shift = max(grid, int(round((ix1 - ix0) * .28 / grid)) * grid) * (-1 if name == "esquerda" else 1)
                for y in range(iy0, iy1):
                    row = [base_eye.getpixel((x, y)) for x in range(ix0, ix1)]
                    if sum(1 for c in row if is_skin(c)) > len(row) * .5:
                        continue                      # a linha é pálpebra/pele: fica
                    for k, x in enumerate(range(ix0, ix1)):
                        j = k - shift
                        im.putpixel((x, y), row[j] if 0 <= j < len(row) else sclera)
        if name in ("bravo", "dor"):
            # sobrancelhas: a ponta do lado do nariz desce (bravo) ou sobe (dor)
            moved = Image.new("RGBA", im.size, (0, 0, 0, 0))
            for brow in brows:
                bx0, bx1 = min(p[0] for p in brow), max(p[0] for p in brow)
                for (x, y) in brow:
                    im.putpixel((x, y), skin + (255,))
                for (x, y) in brow:
                    toward = (x - bx0) / max(1, bx1 - bx0)
                    if (bx0 + bx1) / 2 > mid_x:
                        toward = 1 - toward
                    step = int(round(toward * 2)) * grid * (1 if name == "bravo" else -1)
                    if 0 <= y + step < im.height:
                        moved.putpixel((x, y + step), base_eye.getpixel((x, y)))
            im.alpha_composite(moved)
        frames.append(im)
    eye_sheet = sheet(frames)

    # ---- boca
    base_mouth = head.crop(region_mouth)
    mwid, mhei = base_mouth.size
    skin_mask = [[is_skin(base_mouth.getpixel((x, y))) or (lum(base_mouth.getpixel((x, y))) < lum(skin) * .9 and base_mouth.getpixel((x, y))[3] > 200
                  and abs(y - mhei / 2) < mhei * .3 and abs(x - mwid / 2) < mwid * .4) for x in range(mwid)] for y in range(mhei)]
    inside, teeth, lip = (70, 22, 26), (238, 232, 220), tuple(int(v * .62) for v in skin)
    cxm, cym = mwid / 2, mhei / 2
    shapes = {
        "neutra": [],
        "A": [("in", -.18, -.10, .18, .22), ("teeth", -.14, -.10, .14, -.02)],
        "O": [("in", -.09, -.12, .09, .16)],
        "E": [("in", -.26, -.04, .26, .08), ("teeth", -.22, -.04, .22, .02)],
        "sorriso": [("lip", -.30, -.02, -.20, .04), ("lip", -.20, .04, .20, .10), ("lip", .20, -.02, .30, .04)],
        "dentes": [("in", -.24, -.06, .24, .10), ("teeth", -.22, -.04, .22, .08), ("lip", -.24, .01, .24, .03)],
        "M": [("lip", -.20, .00, .20, .05)],
        "grito": [("in", -.24, -.16, .24, .30), ("teeth", -.20, -.16, .20, -.06)],
    }
    frames = []
    for name in MOUTH_FRAMES:
        im = base_mouth.copy()
        if name != "neutra":
            # tira a boca antiga (a linha escura na pele) e desenha a nova só onde é pele
            layer = Image.new("RGBA", im.size, (0, 0, 0, 0))
            d = ImageDraw.Draw(layer)
            for y in range(mhei):
                for x in range(mwid):
                    c = im.getpixel((x, y))
                    if skin_mask[y][x] and not is_skin(c):
                        im.putpixel((x, y), skin + (255,))
            for kind, x0, y0, x1, y1 in shapes[name]:
                color = {"in": inside, "teeth": teeth, "lip": lip}[kind]
                blocks(d, cxm + x0 * mwid, cym + y0 * mwid, cxm + x1 * mwid, cym + y1 * mwid, color + (255,))
            for y in range(mhei):
                for x in range(mwid):
                    c = layer.getpixel((x, y))
                    if c[3] and skin_mask[y][x]:
                        im.putpixel((x, y), c)
        frames.append(im)
    mouth_sheet = sheet(frames)
    eye_sheet.save(os.path.join(folder, "eyes.png"))
    mouth_sheet.save(os.path.join(folder, "mouth.png"))

    def uvbox(r):
        # no retângulo da cabeça, de 0 a 1, com v para cima
        return [r[0] / W, 1 - r[3] / H, (r[2] - r[0]) / W, (r[3] - r[1]) / H]

    # as regiões que a malha deixa lisas (Tools/Blender/MixamoToHero.py --flatface): cada olho com a sobrancelha, e a
    # boca quando ela fica na pele (no anão a boca fica entre o bigode e a barba: o volume deles fica)
    flat = []
    for i, b in enumerate(full):
        mine = [p for brow in brows for p in brow if (p[0] + ox < cx) == (i == 0)]
        bx0 = min([b[0] - grid] + [p[0] + ox - grid for p in mine])
        bx1 = max([b[2] + grid] + [p[0] + ox + grid for p in mine])
        by0 = min([b[1] - grid] + [p[1] + oy - grid for p in mine])
        # o buraco do olho na malha é maior que o branco: a região vai um pouco além (dentro do retângulo desenhado)
        ew_, eh_ = b[2] - b[0], b[3] - b[1]
        bx0, bx1 = min(bx0, b[0] - int(ew_ * .25)), max(bx1, b[2] + int(ew_ * .25))
        flat.append(uvbox((max(region_eye[0], bx0), max(region_eye[1], by0), min(region_eye[2], bx1),
                           min(region_eye[3], b[3] + max(grid, int(eh_ * .3))))))
    mouth_skin = sum(1 for y in range(mhei) for x in range(mwid) if is_skin(base_mouth.getpixel((x, y)))) / max(1, mwid * mhei)
    if mouth_skin > .6:
        flat.append(uvbox((region_mouth[0] + grid, region_mouth[1] + grid, region_mouth[2] - grid, region_mouth[3] - grid)))
    meta.update({"eyes": uvbox(region_eye), "mouth": uvbox(region_mouth), "grid": [4, 2], "flat": flat,
                 "eye_frames": EYE_FRAMES, "mouth_frames": MOUTH_FRAMES})
    json.dump(meta, open(os.path.join(folder, "face.json"), "w", encoding="utf-8"), indent=1)
    # conferência: cada par olho/boca na cabeça inteira
    tiles = []
    for i in range(8):
        im = head.copy()
        im.alpha_composite(eye_sheet.crop(cell(eye_sheet, i)), (region_eye[0], region_eye[1]))
        im.alpha_composite(mouth_sheet.crop(cell(mouth_sheet, i)), (region_mouth[0], region_mouth[1]))
        d = ImageDraw.Draw(im)
        d.text((8, 8), "%s / %s" % (EYE_FRAMES[i], MOUTH_FRAMES[i]), fill=(255, 255, 255, 255))
        tiles.append(im.resize((256, 256), Image.NEAREST))
    preview = Image.new("RGBA", (256 * 4, 256 * 2), (40, 40, 40, 255))
    for i, t in enumerate(tiles):
        preview.alpha_composite(t, ((i % 4) * 256, (i // 4) * 256))
    preview.save(os.path.join(folder, "preview.png"))
    print("%s: olhos %s boca %s grade %d px, pele %s" % (race, region_eye, region_mouth, grid, skin))


def sheet(frames):
    w, h = frames[0].size
    out = Image.new("RGBA", (w * 4, h * 2), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        out.paste(f, ((i % 4) * w, (i // 4) * h))
    return out


def cell(sheet_image, i):
    w, h = sheet_image.width // 4, sheet_image.height // 2
    return ((i % 4) * w, (i // 4) * h, (i % 4 + 1) * w, (i // 4 + 1) * h)


if __name__ == "__main__":
    for race in sys.argv[1:] or ["humano", "anao"]:
        make(race)
