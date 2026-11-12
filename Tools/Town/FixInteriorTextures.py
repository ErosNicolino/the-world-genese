"""07/11: acerto das texturas dos móveis (depois do MakeVilaTextures.py ... Colcha Tapete Lareira Livros).
Colcha, Tapete e Lareira viram cinza claro (a cor de cada peça vem da cor do vértice: colcha verde, tapete vermelho,
lareira de pedra cinza); Livros é desenhada aqui (lombadas coloridas em prateleira, emenda na horizontal)."""
import os
import random

from PIL import Image, ImageDraw, ImageFilter, ImageOps

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "Data", "GodotTown", "VilaTex")


def neutral(name, mean=200, contrast=1.0):
    p = os.path.join(OUT, "T_Vila_%s.png" % name)
    g = ImageOps.grayscale(Image.open(p))
    g = ImageOps.autocontrast(g, cutoff=1)
    avg = sum(i * c for i, c in enumerate(g.histogram())) / (g.width * g.height)
    g = g.point(lambda v: max(0, min(255, int(mean + (v - avg) * contrast))))
    Image.merge("RGB", (g, g, g)).save(p)


neutral("Colcha", 215, .45)
neutral("Tapete", 190, .9)
neutral("Lareira", 175, .8)

# os livros: UMA fileira de lombadas por textura (o kit põe cada prateleira em múltiplos de 45 cm, o tamanho que a
# textura cobre: a base dos livros cai sempre na tábua)
S = 512
rnd = random.Random(7)
img = Image.new("RGB", (S, S), (34, 24, 18))
d = ImageDraw.Draw(img)
colors = ((150, 40, 34), (40, 70, 120), (60, 100, 52), (170, 130, 60), (100, 50, 90), (120, 80, 50), (190, 170, 130), (30, 30, 34))
x = 0
while x < S:
    w = rnd.randint(18, 40)
    top = rnd.randint(70, 190)
    c = rnd.choice(colors)
    d.rectangle([x, top, min(S, x + w - 3), S - 1], fill=c)
    for band in (top + 18, S - 40):
        d.rectangle([x + 2, band, min(S, x + w - 5), band + 6], fill=(212, 180, 90))
    d.line([(x + w - 3, top), (x + w - 3, S)], fill=(20, 14, 10), width=3)
    x += w
img.filter(ImageFilter.GaussianBlur(.6)).save(os.path.join(OUT, "T_Vila_Livros.png"))
n = os.path.join(OUT, "T_Vila_Livros_N.png")
if os.path.exists(n):
    os.remove(n)
print("ok")

# 07/11: o barro liso das telhas 3D (o kit faz telha por telha; a textura de telhado repetia telhas dentro de cada uma)
rnd = random.Random(11)
img = Image.new("RGB", (S, S), (196, 104, 62))
d = ImageDraw.Draw(img)
for n in range(2600):
    x, y, r = rnd.randint(0, S), rnd.randint(0, S), rnd.randint(4, 26)
    c = rnd.choice(((210, 118, 72), (178, 92, 56), (160, 80, 50), (220, 140, 92), (140, 84, 60), (120, 96, 80)))
    for ox in (-S, 0, S):
        for oy in (-S, 0, S):
            d.ellipse([x + ox - r, y + oy - r * .7, x + ox + r, y + oy + r * .7], fill=c)
img = img.filter(ImageFilter.GaussianBlur(5))
d = ImageDraw.Draw(img)
for n in range(160):
    x, y = rnd.randint(0, S), rnd.randint(0, S)
    d.line([(x, y), (x + rnd.randint(-6, 6), y + rnd.randint(20, 80))], fill=(110, 70, 50), width=2)
img.filter(ImageFilter.GaussianBlur(1.2)).save(os.path.join(OUT, "T_Vila_TelhaLisa.png"))
