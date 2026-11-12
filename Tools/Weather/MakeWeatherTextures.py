"""As texturas da chuva e da neve (11/11): Data/Weather/T_Rain.png e T_Snow.png, em tons de cinza (o canal vermelho é a
opacidade), repetindo sem emenda. O material M_VortexPrecip (Tools/ImportTownLife.py) as rola no cilindro em volta da
câmera (UVortexWeatherSubsystem).

python Tools/Weather/MakeWeatherTextures.py
"""
import os
import random

from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "Data", "Weather")
rng = random.Random(1111)
os.makedirs(OUT, exist_ok=True)


def wrap_draw(size, draw_one, count):
    """Desenha cada pingo/floco também deslocado de uma volta, para a textura repetir sem emenda."""
    w, h = size
    img = Image.new("L", size, 0)
    d = ImageDraw.Draw(img)
    for _ in range(count):
        x, y = rng.uniform(0, w), rng.uniform(0, h)
        for ox in (-w, 0, w):
            for oy in (-h, 0, h):
                draw_one(d, x + ox, y + oy)
    return img


# chuva: riscos finos e compridos, de opacidades diferentes (256 x 512: a textura repete mais na horizontal)
def streak(d, x, y):
    length, alpha = rng.uniform(26, 70), rng.randint(60, 210)
    for k in range(int(length)):
        d.point((x, y + k), fill=int(alpha * (k / length) ** .7))


rain = wrap_draw((256, 512), streak, 150).filter(ImageFilter.GaussianBlur(.35))
rain.save(os.path.join(OUT, "T_Rain.png"))


# neve: flocos redondos e macios de vários tamanhos
def flake(d, x, y):
    r, alpha = rng.choice((1.2, 1.6, 2.2, 2.8, 3.6)), rng.randint(140, 255)
    d.ellipse((x - r, y - r, x + r, y + r), fill=alpha)


snow = wrap_draw((512, 512), flake, 260).filter(ImageFilter.GaussianBlur(.9))
snow.save(os.path.join(OUT, "T_Snow.png"))
print("CLIMA texturas em", OUT)
