"""As placas da vila (07/11, o usuário: "melhorar as placas, uma fonte medieval e o símbolo de cada lugar, como na forja a
bigorna"; depois, com uma imagem de referência: a tábua de escudo pendurada no braço de ferro, o símbolo em bronze
batido, o nome curto embaixo, o espiral em cima).

python Tools/Town/MakeSigns.py   ->   Data/GodotTown/Placas/T_Placa_<SLUG>.png  (512 x 656, a forma do escudo da tábua
vila_placa_pendurada do kit: o jogo põe esta imagem nas duas faces)

A tábua: madeira escura em tábuas na vertical, gasta e arranhada; o espiral do Vortex em cima; o símbolo do lugar grande,
em bronze com relevo (luz em cima à esquerda) e contorno escuro; o nome curto em Cinzel 900 bronze e o resto do nome
menor; o filete com o losango embaixo. Tools/ImportVilaKit.py leva para /Game/Vortex/Town/Vila/Placas.
"""
import math
import os
import random
import re
import unicodedata

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "Data", "GodotTown", "Placas")
FONT = os.path.join(ROOT, "Content", "Vortex", "UI", "Fonts", "Cinzel-900.ttf")
WOOD = os.path.join(ROOT, "Data", "GodotTown", "VilaTex", "T_Vila_MadeiraEscura.png")
W, H = 512, 656
SS = 2
INK = (20, 12, 8)

# o nome da placa (como está na planta) -> (símbolo, nome curto, o resto)
SIGNS = {
    "PREFEITURA": ("coroa", "Prefeitura", ""),
    "TAVERNA DO JAVALI DOURADO": ("caneca", "Taverna", "do Javali Dourado"),
    "QUARTEL DA GUARDA": ("escudo", "Quartel", "da Guarda"),
    "CARPINTARIA": ("serrote", "Carpintaria", ""),
    "FORJA DE BRAM": ("bigorna", "Forja", "de Bram"),
    "CAPELA": ("sino", "Capela", ""),
    "ESTÁBULO": ("ferradura", "Estábulo", ""),
    "PADARIA": ("pao", "Padaria", ""),
    "ERVAS DE ILSA": ("folha", "Ervas", "de Ilsa"),
    "MAPAS DE NICOLLY": ("rosa", "Mapas", "de Nicolly"),
    "ALFAIATE": ("tesoura", "Alfaiate", ""),
    "ARMARIA": ("espadas", "Armaria", ""),
    "BAZAR DO DUCCI": ("bolsa", "Bazar", "do Ducci"),
    "MISSÕES": ("pergaminho", "Missões", ""),
}


def slug(label):
    s = unicodedata.normalize("NFKD", label).encode("ascii", "ignore").decode()
    return re.sub(r"[^A-Z0-9]+", "_", s.upper()).strip("_")


def shape_mask(kind, size):
    """A silhueta do símbolo (255) com os detalhes em cinza (viram sulcos no bronze), num quadrado de `size` px."""
    m = Image.new("L", (size, size), 0)
    d = ImageDraw.Draw(m)
    s = size / 100.0

    def P(*pts):
        return [(x * s, y * s) for x, y in pts]

    def poly(pts, v=255):
        d.polygon(P(*pts), fill=v)

    def rect(x0, y0, x1, y1, v=255):
        d.rectangle([x0 * s, y0 * s, x1 * s, y1 * s], fill=v)

    def ell(x0, y0, x1, y1, v=255, width=0):
        if width:
            d.ellipse([x0 * s, y0 * s, x1 * s, y1 * s], outline=v, width=int(width * s))
        else:
            d.ellipse([x0 * s, y0 * s, x1 * s, y1 * s], fill=v)

    def line(a, b, w, v=255):
        d.line(P(a, b), fill=v, width=int(w * s))

    if kind == "bigorna":
        poly([(4, 46), (66, 46), (96, 52), (68, 60), (62, 60), (58, 72), (68, 84), (32, 84), (40, 72), (36, 60), (12, 60), (4, 54)])
        rect(26, 84, 74, 94)
        line((62, 10), (86, 32), 7)
        poly([(46, 4), (68, 0), (76, 10), (54, 16)])
    elif kind == "caneca":
        rect(22, 24, 66, 92)
        ell(56, 38, 92, 76, 255, 9)
        ell(14, 6, 36, 30); ell(32, 2, 54, 28); ell(50, 6, 72, 30)
        for x in (32, 44, 56):
            rect(x, 36, x + 3, 84, 120)
    elif kind == "coroa":
        poly([(8, 80), (12, 30), (32, 56), (50, 16), (68, 56), (88, 30), (92, 80)])
        rect(8, 80, 92, 92)
        rect(8, 84, 92, 86, 120)
    elif kind == "escudo":
        poly([(16, 8), (84, 8), (84, 50), (50, 96), (16, 50)])
        poly([(26, 18), (74, 18), (74, 48), (50, 82), (26, 48)], 150)
        rect(46, 18, 54, 80); rect(26, 40, 74, 48)
    elif kind == "serrote":
        poly([(4, 40), (78, 30), (82, 56), (10, 62)])
        for i in range(9):
            x = 10 + i * 7.6
            poly([(x, 61 - i * .9), (x + 7.6, 60 - i * .9), (x + 3.8, 70 - i * .9)])
        rect(76, 20, 98, 68); ell(80, 34, 92, 54, 0)
    elif kind == "sino":
        poly([(30, 18), (70, 18), (78, 66), (92, 78), (8, 78), (22, 66)])
        rect(44, 4, 56, 18); ell(40, 76, 60, 96)
        rect(14, 70, 86, 74, 140)
    elif kind == "ferradura":
        ell(12, 6, 88, 90, 255, 19)
        d.rectangle([30 * s, 62 * s, 70 * s, 96 * s], fill=0)
        for a in (-60, -25, 25, 60, -95, 95):
            x = 50 + 37 * math.sin(math.radians(a)); y = 48 - 37 * math.cos(math.radians(a))
            ell(x - 3, y - 3, x + 3, y + 3, 90)
    elif kind == "pao":
        ell(6, 28, 94, 82)
        for x in (26, 46, 66):
            poly([(x, 36), (x + 8, 34), (x + 2, 74), (x - 6, 76)], 130)
    elif kind == "folha":
        poly([(50, 4), (80, 30), (84, 58), (64, 84), (50, 94), (36, 84), (16, 58), (20, 30)])
        rect(48, 18, 52, 100, 120)
        for y, dx in ((34, 22), (52, 26), (70, 20)):
            line((50, y + 8), (50 - dx, y), 3, 120); line((50, y + 8), (50 + dx, y), 3, 120)
    elif kind == "rosa":
        ell(8, 8, 92, 92, 255, 6)
        poly([(50, 2), (59, 50), (50, 98), (41, 50)])
        poly([(2, 50), (50, 41), (98, 50), (50, 59)])
        ell(44, 44, 56, 56, 110)
    elif kind == "tesoura":
        ell(6, 60, 40, 94, 255, 8); ell(60, 60, 94, 94, 255, 8)
        poly([(30, 64), (38, 60), (82, 4), (74, 4)]); poly([(70, 64), (62, 60), (18, 4), (26, 4)])
        ell(45, 33, 55, 43, 110)
    elif kind == "espadas":
        for sgn in (1, -1):
            c, sn = math.cos(sgn * .7), math.sin(sgn * .7)

            def R(pts):
                return [(50 + (x - 50) * c - (y - 50) * sn, 50 + (x - 50) * sn + (y - 50) * c) for x, y in pts]
            poly(R([(46, 10), (54, 10), (54, 66), (46, 66)])); poly(R([(46, 10), (54, 10), (50, 0)]))
            poly(R([(34, 66), (66, 66), (66, 73), (34, 73)])); poly(R([(46, 73), (54, 73), (54, 92), (46, 92)]))
    elif kind == "bolsa":
        ell(12, 30, 88, 96)
        poly([(32, 34), (68, 34), (76, 12), (60, 20), (50, 8), (40, 20), (24, 12)])
        rect(28, 30, 72, 38, 130)
        d.text((50 * s, 68 * s), "$", fill=120, font=ImageFont.truetype(FONT, int(30 * s)), anchor="mm")
    elif kind == "pergaminho":
        rect(20, 14, 80, 86)
        ell(10, 6, 30, 26); ell(70, 74, 90, 94)
        rect(20, 6, 80, 16); rect(20, 84, 80, 94)
        for y in (30, 42, 54, 66):
            rect(30, y, 70, y + 4, 130)
    return m


def metal(mask, top=(244, 212, 150), bottom=(120, 84, 48), outline=6):
    """Bronze batido na forma da máscara: o degradê, o relevo (luz em cima à esquerda), os sulcos dos detalhes e o
    contorno escuro. Devolve RGBA."""
    w, h = mask.size
    solid = mask.point(lambda v: 255 if v > 40 else 0)
    grad = Image.new("RGB", (w, h))
    gd = ImageDraw.Draw(grad)
    for y in range(h):
        t = y / max(1, h - 1)
        gd.line([(0, y), (w, y)], fill=tuple(int(top[i] + (bottom[i] - top[i]) * t) for i in range(3)))
    blur = solid.filter(ImageFilter.GaussianBlur(3 * SS))
    lit = ImageChops.subtract(blur, ImageChops.offset(blur, 3 * SS, 3 * SS))
    shade = ImageChops.subtract(blur, ImageChops.offset(blur, -3 * SS, -3 * SS))
    rgb = Image.composite(Image.new("RGB", (w, h), (255, 240, 200)), grad, lit.point(lambda v: min(255, v * 3)))
    rgb = Image.composite(Image.new("RGB", (w, h), (60, 36, 18)), rgb, shade.point(lambda v: min(200, v * 3)))
    detail = mask.point(lambda v: 255 if 40 < v < 200 else 0).filter(ImageFilter.GaussianBlur(1))
    rgb = Image.composite(Image.new("RGB", (w, h), (80, 52, 28)), rgb, detail.point(lambda v: v * 2 // 3))
    out = Image.new("RGBA", (w, h), INK + (0,))
    out.putalpha(solid.filter(ImageFilter.MaxFilter(outline * SS | 1)))
    body = rgb.convert("RGBA")
    body.putalpha(solid)
    out.alpha_composite(body)
    return out


def text_mask(text, size):
    font = ImageFont.truetype(FONT, size)
    l, t, r, b = font.getbbox(text)
    m = Image.new("L", (r - l + 16 * SS, b - t + 16 * SS), 0)
    ImageDraw.Draw(m).text((8 * SS - l, 8 * SS - t), text, font=font, fill=255)
    return m


def wood():
    """A tábua: as tábuas na vertical (a madeira escura do kit, escurecida), juntas, arranhões e a sombra da moldura."""
    big = (W * SS, H * SS)
    tex = Image.open(WOOD).convert("RGB").resize((H * SS, H * SS)).rotate(90)
    board = Image.new("RGB", big)
    for x in range(0, big[0], tex.width):
        board.paste(tex, (x, 0))
    board = board.point(lambda v: int(v * .55))
    d = ImageDraw.Draw(board)
    rnd = random.Random(3)
    planks = 5
    for i in range(1, planks):
        x = i * big[0] // planks + rnd.randint(-6, 6) * SS
        d.line([(x, 0), (x, big[1])], fill=(16, 10, 6), width=3 * SS)
        d.line([(x + 3 * SS, 0), (x + 3 * SS, big[1])], fill=(70, 50, 34), width=SS)
    for _ in range(120):
        x, y = rnd.randint(0, big[0]), rnd.randint(0, big[1])
        L = rnd.randint(10, 60) * SS
        a = rnd.uniform(-.4, .4) + math.pi / 2
        d.line([(x, y), (x + math.cos(a) * L * .3, y + math.sin(a) * L)], fill=(90, 70, 52) if rnd.random() < .5 else (12, 8, 6), width=SS)
    vign = Image.new("L", big, 0)
    ImageDraw.Draw(vign).rectangle([0, 0, big[0], big[1]], outline=255, width=40 * SS)
    vign = vign.filter(ImageFilter.GaussianBlur(26 * SS))
    return Image.composite(Image.new("RGB", big, (8, 5, 3)), board, vign.point(lambda v: v * 3 // 4))


def swirl(size):
    """O espiral do Vortex (as pás curvas em volta do miolo)."""
    m = Image.new("L", (size, size), 0)
    d = ImageDraw.Draw(m)
    c = size / 2
    for k in range(6):
        a0 = k * 60
        pts = []
        for j in range(14):
            t = j / 13
            r = c * (.18 + .8 * t)
            a = math.radians(a0 + 110 * t)
            pts.append((c + r * math.cos(a), c + r * math.sin(a)))
        for j in range(13, -1, -1):
            t = j / 13
            r = c * (.18 + .8 * t)
            a = math.radians(a0 + 110 * t - 26 * (1 - t) - 8)
            pts.append((c + r * math.cos(a) * .92, c + r * math.sin(a) * .92))
        d.polygon(pts, fill=255)
    d.ellipse([c * .82, c * .82, c * 1.18, c * 1.18], fill=255)
    return m


def sign(label):
    kind, name, rest = SIGNS[label]
    board = wood().convert("RGBA")
    BW, BH = board.size
    sw = metal(swirl(int(86 * SS)), outline=4)
    board.alpha_composite(sw, (BW // 2 - sw.width // 2, int(70 * SS)))
    ic = metal(shape_mask(kind, int(250 * SS)))
    board.alpha_composite(ic, (BW // 2 - ic.width // 2, int(170 * SS)))
    d = ImageDraw.Draw(board)
    if kind == "bigorna":
        # as faíscas do martelo
        cx, cy = BW // 2 + 30 * SS, int(282 * SS)
        rnd = random.Random(5)
        for k in range(14):
            a = math.radians(rnd.uniform(200, 340))
            r0, r1 = rnd.uniform(14, 24) * SS, rnd.uniform(40, 70) * SS
            d.line([(cx + math.cos(a) * r0, cy + math.sin(a) * r0), (cx + math.cos(a) * r1, cy + math.sin(a) * r1)], fill=(255, 190, 80, 230), width=2 * SS)
    size = int(92 * SS)
    tm = text_mask(name, size)
    while tm.width > BW * .8:
        size = int(size * .92)
        tm = text_mask(name, size)
    tt = metal(tm, outline=5)
    y = int(445 * SS)
    board.alpha_composite(tt, (BW // 2 - tt.width // 2, y))
    y += tt.height - 6 * SS
    if rest:
        rm = text_mask(rest, int(40 * SS))
        if rm.width > BW * .78:
            rm = rm.resize((int(BW * .78), int(rm.height * BW * .78 / rm.width)))
        rt = metal(rm, top=(230, 200, 150), bottom=(150, 110, 70), outline=4)
        board.alpha_composite(rt, (BW // 2 - rt.width // 2, y))
        y += rt.height
    d = ImageDraw.Draw(board)
    ly = min(BH - 70 * SS, y + 10 * SS)
    d.line([(BW * .22, ly), (BW * .78, ly)], fill=(200, 160, 100), width=3 * SS)
    r = 12 * SS
    d.polygon([(BW / 2, ly - r), (BW / 2 + r, ly), (BW / 2, ly + r), (BW / 2 - r, ly)], fill=(220, 180, 110), outline=INK)
    return board.convert("RGB").resize((W, H), Image.LANCZOS)


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    for old in os.listdir(OUT):
        if old.startswith("T_Placa_"):
            os.remove(os.path.join(OUT, old))
    for label in SIGNS:
        path = os.path.join(OUT, "T_Placa_%s.png" % slug(label))
        sign(label).save(path)
        print("placa", label, "->", os.path.basename(path))
