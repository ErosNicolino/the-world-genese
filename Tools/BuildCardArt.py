"""Arte das cartas do Despertar (Data/UI/Cards/*.png), desenhada aqui com PIL.

python Tools/BuildCardArt.py        (depois: Tools/ImportCardArt.py no editor fechado)

- T_Card_Back: o verso (azul-noite com o redemoinho do Vórtice em ouro e moldura trabalhada);
- T_Card_Bronze / Silver / Gold / Diamond: a frente de cada raridade (comum, rara, épica, lendária):
  moldura de metal chanfrada com brilho, pedras nos cantos, a janela do ícone e a faixa do nome; o
  ícone, o nome e os textos são desenhados por cima no jogo;
- T_Card_Glow: brilho redondo branco (o jogo tinge na cor da raridade);
- T_Card_Rays: raios de luz para a carta lendária.
"""
import math
import os
import random

from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "UI", "Cards")
W, H = 512, 768
rng = random.Random(7)

TIERS = {
    # nome: (escuro, médio, claro, brilho, pedra)
    "Bronze": ((70, 36, 18), (150, 86, 44), (220, 150, 92), (255, 214, 160), (190, 60, 30)),
    "Silver": ((70, 76, 86), (150, 160, 175), (215, 222, 232), (255, 255, 255), (70, 130, 220)),
    "Gold": ((96, 64, 10), (196, 146, 40), (250, 214, 96), (255, 248, 200), (200, 40, 60)),
    "Diamond": ((40, 30, 90), (120, 150, 230), (200, 235, 255), (255, 255, 255), (230, 120, 255)),
}


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


def metal_gradient(size, dark, mid, light, diagonal=True, prism=False):
    """Faixa de metal: escuro -> claro -> médio -> escuro (com um brilho de lado), ou prisma (diamante)."""
    w, h = size
    img = Image.new("RGBA", size)
    px = img.load()
    for y in range(h):
        for x in range(w):
            t = ((x / w) * .55 + (y / h) * .45) if diagonal else y / h
            if prism:
                hue = (t * 2.2 + .1 * math.sin(y * .05)) % 1.0
                r = .5 + .5 * math.sin(2 * math.pi * hue)
                g = .5 + .5 * math.sin(2 * math.pi * (hue + .33))
                b = .5 + .5 * math.sin(2 * math.pi * (hue + .66))
                base = lerp(mid, light, .5 + .5 * math.sin(t * 9.0))
                c = (int(base[0] * .55 + 255 * r * .45), int(base[1] * .55 + 255 * g * .45), int(base[2] * .55 + 255 * b * .45))
            else:
                k = .5 + .5 * math.sin(t * 7.5 + 1.2)
                c = lerp(dark, light, k) if k < .7 else lerp(light, mid, (k - .7) / .3)
            px[x, y] = (*c, 255)
    return img


def rounded_mask(size, radius, inset=0):
    m = Image.new("L", size, 0)
    d = ImageDraw.Draw(m)
    d.rounded_rectangle([inset, inset, size[0] - 1 - inset, size[1] - 1 - inset], radius=radius, fill=255)
    return m


def gem(draw, cx, cy, r, color, shine=(255, 255, 255)):
    pts = [(cx, cy - r), (cx + r, cy), (cx, cy + r), (cx - r, cy)]
    draw.polygon(pts, fill=lerp(color, (0, 0, 0), .35), outline=lerp(color, (255, 255, 255), .4))
    inner = [(cx, cy - r * .62), (cx + r * .62, cy), (cx, cy + r * .62), (cx - r * .62, cy)]
    draw.polygon(inner, fill=color)
    draw.polygon([(cx - r * .3, cy - r * .3), (cx, cy - r * .55), (cx + r * .05, cy - r * .2)], fill=(*shine, 220))


def frame(name, dark, mid, light, glow, stone):
    prism = name == "Diamond"
    card = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    # base escura com textura (fundo do papel da carta)
    base = Image.new("RGBA", (W, H), (14, 12, 18, 255))
    bd = ImageDraw.Draw(base)
    for _ in range(2600):
        x, y = rng.randrange(W), rng.randrange(H)
        v = rng.randrange(14, 30)
        bd.point((x, y), fill=(v, v - 2, v + 4, 255))
    card.paste(base, (0, 0), rounded_mask((W, H), 34))
    # moldura de metal (anel externo largo, chanfro claro em cima, sombra embaixo)
    metal = metal_gradient((W, H), dark, mid, light, prism=prism)
    ring = Image.new("L", (W, H), 0)
    rd = ImageDraw.Draw(ring)
    rd.rounded_rectangle([0, 0, W - 1, H - 1], radius=34, fill=255)
    rd.rounded_rectangle([26, 26, W - 27, H - 27], radius=20, fill=0)
    card.paste(metal, (0, 0), ring)
    d = ImageDraw.Draw(card)
    d.rounded_rectangle([3, 3, W - 4, H - 4], radius=32, outline=(*light, 255), width=3)
    d.rounded_rectangle([24, 24, W - 25, H - 25], radius=21, outline=(*dark, 255), width=4)
    d.rounded_rectangle([30, 30, W - 31, H - 31], radius=17, outline=(*glow, 120), width=2)
    # filete interno de metal fino
    d.rounded_rectangle([44, 44, W - 45, H - 45], radius=12, outline=(*mid, 200), width=2)
    # janela do ícone (círculo com aro) e faixa do nome
    cx, cy, r = W // 2, 250, 140
    d.ellipse([cx - r - 14, cy - r - 14, cx + r + 14, cy + r + 14], fill=(*dark, 255))
    halo = metal_gradient((2 * r + 20, 2 * r + 20), dark, mid, light, prism=prism)
    hm = Image.new("L", halo.size, 0)
    hd = ImageDraw.Draw(hm)
    hd.ellipse([0, 0, halo.size[0] - 1, halo.size[1] - 1], fill=255)
    hd.ellipse([12, 12, halo.size[0] - 13, halo.size[1] - 13], fill=0)
    card.paste(halo, (cx - r - 10, cy - r - 10), hm)
    window = Image.new("RGBA", (2 * r, 2 * r))
    wd = ImageDraw.Draw(window)
    for k in range(r, 0, -2):
        t = k / r
        c = lerp((8, 8, 14), lerp(dark, mid, .35), (1 - t) * .6)
        wd.ellipse([r - k, r - k, r + k, r + k], fill=(*c, 255))
    card.paste(window, (cx - r, cy - r), window)
    # raios finos atrás da janela (sol da raridade)
    for k in range(24):
        a = 2 * math.pi * k / 24
        x1, y1 = cx + math.cos(a) * (r + 22), cy + math.sin(a) * (r + 22)
        x2, y2 = cx + math.cos(a) * (r + 46 + (k % 2) * 16), cy + math.sin(a) * (r + 46 + (k % 2) * 16)
        if 48 < y2 < 420:
            d.line([x1, y1, x2, y2], fill=(*light, 160), width=3)
    # faixa do nome
    band = metal_gradient((W - 100, 74), dark, mid, light, diagonal=False, prism=prism)
    bm = Image.new("L", band.size, 0)
    ImageDraw.Draw(bm).polygon([(18, 0), (band.size[0] - 18, 0), (band.size[0], 37), (band.size[0] - 18, 73), (18, 73), (0, 37)], fill=255)
    card.paste(band, (50, 430), bm)
    d.polygon([(50 + 24, 438), (W - 50 - 24, 438), (W - 50 - 8, 467), (W - 50 - 24, 496), (50 + 24, 496), (50 + 8, 467)], fill=(16, 13, 20, 255))
    # pedras nos cantos e no meio de cima/baixo
    for (gx, gy) in ((40, 40), (W - 41, 40), (40, H - 41), (W - 41, H - 41)):
        gem(d, gx, gy, 20, stone)
    gem(d, W // 2, 30, 16, stone)
    gem(d, W // 2, H - 31, 16, stone)
    # ornamentos laterais
    for side in (1, -1):
        x = W // 2 + side * (W // 2 - 13)
        for k in range(5):
            y = 140 + k * 120
            d.polygon([(x, y - 18), (x + side * 9, y), (x, y + 18), (x - side * 9, y)], fill=(*light, 255), outline=(*dark, 255))
    if prism:
        # brilhos de diamante espalhados pela moldura
        for _ in range(70):
            x, y = rng.randrange(W), rng.randrange(H)
            if 50 < x < W - 50 and 50 < y < H - 50:
                continue
            s = rng.randrange(3, 9)
            d.line([x - s, y, x + s, y], fill=(255, 255, 255, 230), width=2)
            d.line([x, y - s, x, y + s], fill=(255, 255, 255, 230), width=2)
    return card


def back():
    card = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    base = Image.new("RGBA", (W, H))
    px = base.load()
    for y in range(H):
        for x in range(W):
            dx, dy = (x - W / 2) / W, (y - H / 2) / H
            r = math.sqrt(dx * dx + dy * dy)
            c = lerp((34, 30, 78), (8, 8, 22), min(1, r * 1.8))
            px[x, y] = (*c, 255)
    card.paste(base, (0, 0), rounded_mask((W, H), 34))
    d = ImageDraw.Draw(card)
    gold, dark_gold = (236, 196, 104), (120, 84, 30)
    d.rounded_rectangle([3, 3, W - 4, H - 4], radius=32, outline=gold, width=8)
    d.rounded_rectangle([22, 22, W - 23, H - 23], radius=20, outline=dark_gold, width=3)
    d.rounded_rectangle([34, 34, W - 35, H - 35], radius=14, outline=(*gold, 150), width=2)
    # padrão de losangos
    for y in range(60, H - 60, 44):
        for x in range(60 + (22 if (y // 44) % 2 else 0), W - 60, 44):
            d.polygon([(x, y - 10), (x + 10, y), (x, y + 10), (x - 10, y)], outline=(70, 64, 140, 255))
    # o redemoinho do Vórtice: espirais em ouro e um olho no meio
    cx, cy = W // 2, H // 2
    d.ellipse([cx - 150, cy - 150, cx + 150, cy + 150], fill=(16, 14, 40, 255), outline=gold, width=5)
    for arm in range(5):
        pts = []
        for k in range(120):
            t = k / 119
            a = arm * 2 * math.pi / 5 + t * 4.2
            rr = 18 + t * 122
            pts.append((cx + math.cos(a) * rr, cy + math.sin(a) * rr))
        d.line(pts, fill=gold, width=7 - int(arm % 2) * 2)
    d.ellipse([cx - 26, cy - 26, cx + 26, cy + 26], fill=(255, 236, 170, 255), outline=dark_gold, width=4)
    d.ellipse([cx - 10, cy - 10, cx + 10, cy + 10], fill=(40, 20, 60, 255))
    for (gx, gy) in ((40, 40), (W - 41, 40), (40, H - 41), (W - 41, H - 41)):
        gem(d, gx, gy, 18, (110, 80, 220))
    return card


def glow():
    img = Image.new("RGBA", (256, 256), (255, 255, 255, 0))
    px = img.load()
    for y in range(256):
        for x in range(256):
            r = math.hypot(x - 128, y - 128) / 128
            a = max(0.0, 1 - r) ** 2.2
            px[x, y] = (255, 255, 255, int(255 * a))
    return img


def rays():
    img = Image.new("RGBA", (512, 512), (255, 255, 255, 0))
    d = ImageDraw.Draw(img)
    for k in range(18):
        a = 2 * math.pi * k / 18
        w = .07 if k % 2 else .04
        pts = [(256, 256), (256 + math.cos(a - w) * 256, 256 + math.sin(a - w) * 256), (256 + math.cos(a + w) * 256, 256 + math.sin(a + w) * 256)]
        d.polygon(pts, fill=(255, 255, 255, 120 if k % 2 else 70))
    img = img.filter(ImageFilter.GaussianBlur(6))
    # some na borda
    px = img.load()
    for y in range(512):
        for x in range(512):
            r = math.hypot(x - 256, y - 256) / 256
            c = px[x, y]
            px[x, y] = (c[0], c[1], c[2], int(c[3] * max(0.0, 1 - r)))
    return img


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, colors in TIERS.items():
        frame(name, *colors).save(os.path.join(OUT, "T_Card_%s.png" % name))
    back().save(os.path.join(OUT, "T_Card_Back.png"))
    glow().save(os.path.join(OUT, "T_Card_Glow.png"))
    rays().save(os.path.join(OUT, "T_Card_Rays.png"))
    print("cartas em", OUT)


if __name__ == "__main__":
    main()
