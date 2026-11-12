"""Kit "Cartoon" das janelas do mundo (herói, mochila, técnicas, classe, alquimia, missões, loja), no lugar do
kit "Aventura" em pixel-art (que brigava com a letra de desenho animado).

Estilo de jogo de celular em desenho animado: cantos bem redondos, contorno escuro grosso, cores vivas, a parte
de baixo dos botões mais escura (parece que dá para apertar) e um brilho em cima. As peças têm os mesmos nomes
do kit Aventura (W_Frame, W_Card, W_Button...), então o código só troca a pasta e as margens (VortexUiKit.h).
Desenhadas 4x maiores e reduzidas (bordas lisas). Peças novas: W_ButtonRed, W_ButtonGreen e os efeitos
W_Star, W_Spark e W_Ring (estrelinhas e faíscas dos cliques).
    python Tools/MakeUiKitCartoon.py      (depois: Tools/ImportUiKitCartoon.py com o editor fechado)
"""
import math
import os
from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "UI", "KitCartoon")
os.makedirs(OUT, exist_ok=True)
SS = 4  # desenha 4x maior e reduz

INK = (20, 20, 50, 255)


def canvas(w, h):
    im = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))
    return im, ImageDraw.Draw(im)


def save(im, name):
    w, h = im.width // SS, im.height // SS
    im.resize((w, h), Image.LANCZOS).save(os.path.join(OUT, name + ".png"))


def rr(d, box, r, fill):
    x0, y0, x1, y1 = box
    d.rounded_rectangle((x0 * SS, y0 * SS, x1 * SS - 1, y1 * SS - 1), max(1, r * SS), fill=fill)


def shade(c, k):
    return tuple(max(0, min(255, int(v * k))) for v in c[:3]) + (c[3] if len(c) > 3 else 255,)


def clip(layer, box, r, w, h):
    """Recorta a camada no formato da caixa redonda."""
    mask, md = canvas(w, h)
    rr(md, box, r, (255, 255, 255, 255))
    out = Image.new("RGBA", layer.size, (0, 0, 0, 0))
    out.paste(layer, (0, 0), Image.composite(layer, Image.new("RGBA", layer.size, (0, 0, 0, 0)), mask.split()[3]))
    return out


def mix(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3)) + (255,)


def piece(name, w, h, r, face, ow=5, band=0, band_color=None, shine=True, inset=False, outline=INK, shine_color=None):
    """Caixa redonda com contorno; band = faixa mais escura embaixo (botões, cartões); inset = sombra por dentro em cima."""
    im, d = canvas(w, h)
    face = face + (255,) if len(face) == 3 else face
    rr(d, (0, 0, w, h), r, outline)
    inner = (ow, ow, w - ow, h - ow)
    ri = max(2, r - ow)
    if band:
        rr(d, inner, ri, band_color or shade(face, .72))
        rr(d, (inner[0], inner[1], inner[2], inner[3] - band), ri, face)
    else:
        rr(d, inner, ri, face)
    if inset:
        # afundado: uma sombra por dentro, embaixo do contorno de cima (a caixa menos ela mesma deslocada para baixo)
        sh, sd = canvas(w, h)
        rr(sd, inner, ri, (0, 0, 0, 100))
        rr(sd, (inner[0], inner[1] + 6, inner[2], inner[3] + 6), ri, (0, 0, 0, 0))
        im.alpha_composite(clip(sh, inner, ri, w, h))
    if shine:
        hi = shine_color or mix(face, (255, 255, 255), .35)
        top = inner[1] + 3
        sh, sd = canvas(w, h)
        sd.rounded_rectangle(((inner[0] + ri * .6) * SS, top * SS, (inner[2] - ri * .6) * SS, (top + max(3, min(8, (h - 2 * ow) * .12))) * SS),
                             4 * SS, fill=hi[:3] + (150,))
        im.alpha_composite(sh)
    save(im, name)


BLUE = (61, 111, 209)
NAVY = (31, 50, 108)
CARD = (44, 76, 150)
YEL = (255, 200, 40)
ORANGE = (255, 138, 42)

# a janela: azul vivo, com faixa clara em cima e escura embaixo (dentro das margens, não esticam)
im, d = canvas(160, 160)
rr(d, (0, 0, 160, 160), 40, INK)
rr(d, (7, 7, 153, 153), 33, shade(BLUE + (255,), .78))
rr(d, (7, 7, 153, 143), 33, BLUE + (255,))
band, bd = canvas(160, 160)
bd.rectangle((0, 0, 160 * SS, 40 * SS), fill=mix(BLUE, (255, 255, 255), .16))
im.alpha_composite(clip(band, (7, 7, 153, 143), 33, 160, 160))
save(im, "W_Frame")
# o fundo por baixo da moldura: nada (a moldura já é cheia)
Image.new("RGBA", (8, 8), (0, 0, 0, 0)).save(os.path.join(OUT, "W_StoneTile.png"))

piece("W_Panel", 96, 96, 22, NAVY, ow=5, shine=False, inset=True)
piece("W_Card", 96, 96, 20, CARD, ow=5, band=7)
piece("W_CardLocked", 96, 96, 20, (70, 76, 96), ow=5, band=7)

# espaços de item: afundados; com o mouse, mais claros; escolhido, aro amarelo
piece("W_Slot", 64, 64, 15, (24, 38, 86), ow=4, shine=False, inset=True)
piece("W_SlotHover", 64, 64, 15, (44, 66, 136), ow=4, shine=False, inset=True, outline=(240, 246, 255, 255))
im, d = canvas(64, 64)
rr(d, (0, 0, 64, 64), 15, YEL + (255,))
rr(d, (4, 4, 60, 60), 12, INK)
rr(d, (7, 7, 57, 57), 9, (0, 0, 0, 0))
im2, d2 = canvas(64, 64)
im2.alpha_composite(im)
save(im2, "W_SlotActive")
# o brilho da raridade por dentro (branco, tingido no jogo)
im, d = canvas(64, 64)
for i, a in enumerate((230, 150, 80, 30)):
    d.rounded_rectangle(((4 + i * 2) * SS, (4 + i * 2) * SS, (60 - i * 2) * SS, (60 - i * 2) * SS), (12 - i) * SS, outline=(255, 255, 255, a), width=2 * SS)
save(im, "W_SlotGlow")

# botões: 0 azul, 1 amarelo (principal), vermelho (fechar), verde (+), apagado
def button(name, face, pressed=False):
    piece(name, 96, 52, 18, face, ow=4, band=0 if pressed else 8)
button("W_Button", (74, 158, 240))
button("W_ButtonHover", (110, 182, 255))
button("W_ButtonPressed", (60, 136, 214), pressed=True)
button("W_ButtonGold", YEL)
button("W_ButtonGoldHover", (255, 222, 90))
button("W_ButtonRed", (240, 72, 72))
button("W_ButtonRedHover", (255, 110, 100))
button("W_ButtonGreen", (104, 206, 64))
button("W_ButtonGreenHover", (140, 228, 96))
button("W_ButtonDisabled", (104, 112, 136))

# linha das listas (opções, salas, heróis): cantos menores, cabe em linhas baixas
piece("W_Row", 64, 40, 12, CARD, ow=3, band=4)
piece("W_RowHover", 64, 40, 12, (66, 106, 196), ow=3, band=4, outline=(240, 246, 255, 255))

piece("W_Tab", 96, 96, 22, (45, 86, 176), ow=5, band=8)
piece("W_TabActive", 96, 96, 22, YEL, ow=5, band=8)

# fita dos títulos: branca (tingida no jogo; laranja por padrão), em pílula
piece("W_Ribbon", 96, 44, 20, (255, 255, 255), ow=4, band=7, band_color=(196, 196, 196, 255), shine_color=(255, 255, 255))
piece("W_Key", 64, 56, 13, (70, 110, 200), ow=4, band=7)
piece("W_BarBack", 64, 24, 12, (16, 26, 60), ow=3, shine=False, inset=True)
im, d = canvas(64, 24)
rr(d, (0, 0, 64, 24), 12, (215, 215, 215, 255))
rr(d, (0, 0, 64, 15), 12, (255, 255, 255, 255))
d.rounded_rectangle((8 * SS, 4 * SS, 56 * SS, 8 * SS), 2 * SS, fill=(255, 255, 255, 255))
save(im, "W_BarFill")

# a placa da vitrine do herói: uma elipse azul clara com contorno
im, d = canvas(192, 72)
d.ellipse((2 * SS, 10 * SS, 190 * SS, 70 * SS), fill=(0, 0, 0, 70))
d.ellipse((2 * SS, 4 * SS, 190 * SS, 62 * SS), fill=INK)
d.ellipse((7 * SS, 8 * SS, 185 * SS, 58 * SS), fill=(70, 120, 210, 255))
d.ellipse((7 * SS, 8 * SS, 185 * SS, 50 * SS), fill=(110, 160, 240, 255))
save(im, "W_Stage")

# efeitos dos cliques: estrela de 5 pontas branca com contorno, faísca redonda e anel
def star_points(cx, cy, ro, ri, n=5, rot=-90):
    pts = []
    for k in range(n * 2):
        a = math.radians(rot + k * 180 / n)
        r = ro if k % 2 == 0 else ri
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    return pts
im, d = canvas(64, 64)
d.polygon([(x * SS, y * SS) for x, y in star_points(32, 34, 30, 13)], fill=INK)
d.polygon([(x * SS, y * SS) for x, y in star_points(32, 34, 23, 10)], fill=(255, 255, 255, 255))
save(im, "W_Star")
im, d = canvas(32, 32)
for i in range(16, 0, -1):
    a = int(255 * (1 - i / 16) ** 1.6)
    d.ellipse(((16 - i) * SS, (16 - i) * SS, (16 + i) * SS, (16 + i) * SS), fill=(255, 255, 255, a))
save(im, "W_Spark")
im, d = canvas(64, 64)
d.ellipse((3 * SS, 3 * SS, 61 * SS, 61 * SS), outline=(255, 255, 255, 255), width=5 * SS)
save(im, "W_Ring")
print("VORTEX_KIT_CARTOON", len(os.listdir(OUT)), "peças em", OUT)
