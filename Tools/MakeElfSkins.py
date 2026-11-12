"""As 2 peles do elfo (09/11): a textura do humano de blocos com só a pele repintada, mantendo as sombras e o granulado.

python Tools/MakeElfSkins.py   ->   Data/Heroes/humano_Mixamo/Elfo/T_elfo_<clara|escura>[_F].png e preview.png
"""
import os

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "Data", "Heroes", "Mixamo")
OUT = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "Elfo")
# a cor na sombra mais funda e na parte mais clara da pele do humano
TONES = {"clara": ((205, 178, 158), (255, 240, 226)), "escura": ((50, 43, 74), (112, 100, 146))}


def is_skin(r, g, b):
    return r >= 150 and .55 <= g / r <= .78 and .36 <= b / r <= .60


def luma(c):
    return .3 * c[0] + .59 * c[1] + .11 * c[2]


os.makedirs(OUT, exist_ok=True)
sheet = []
for suffix, name in (("", "humano_cor.png"), ("_F", "humano_cor_f.png")):
    im = Image.open(os.path.join(SRC, name)).convert("RGBA")
    skin = [p[:3] for p in im.getdata() if p[3] and is_skin(*p[:3])]
    assert len(skin) > 500, "pele não encontrada em " + name
    lo, hi = min(map(luma, skin)), max(map(luma, skin))
    for tone, (dark, light) in TONES.items():
        out = im.copy()
        px = out.load()
        for y in range(out.height):
            for x in range(out.width):
                r, g, b, a = px[x, y]
                if a and is_skin(r, g, b):
                    t = (luma((r, g, b)) - lo) / max(1.0, hi - lo)
                    px[x, y] = tuple(round(dark[i] + (light[i] - dark[i]) * t) for i in range(3)) + (a,)
        out.save(os.path.join(OUT, "T_elfo_%s%s.png" % (tone, suffix)))
        sheet.append(out.resize((out.width * 5, out.height * 5), Image.NEAREST))
w, h = sheet[0].size
preview = Image.new("RGB", (w * len(sheet), h), (60, 60, 60))
for i, s in enumerate(sheet):
    preview.paste(s, (i * w, 0), s)
preview.save(os.path.join(OUT, "preview.png"))
print("ok", len(sheet))
