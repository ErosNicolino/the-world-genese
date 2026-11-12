"""A HUD no estilo das imagens das habilidades (07/11, o usuário: "a HUD não combina com as imagens das skills, que são
quadradas: parece um quadrado arredondado e dentro dele outro PNG quadrado"; "quero algo bonito, veja como os jogos fazem").

Como os jogos de fantasia fazem (Diablo, WoW, Lost Ark): o ícone é só a arte, e a moldura é UMA, desenhada pela HUD, a
mesma em todo encaixe. Aqui: fundo azul-noite (o mesmo dos ícones) com textura leve, moldura de ouro em relevo (clara em
cima, escura embaixo), um fio escuro dentro e fora, rebites nos cantos; cantos quase retos (não mais a borracha
arredondada do cartoon). Mesmos nomes e tamanhos do Tools/MakeHudCartoon.py (o código da HUD não muda):

  H_PanelBlue, H_PanelNavy, H_PanelRed, H_PanelGold   painéis (64x64, 9 partes)
  H_BarBack, H_BarFill                                 barra: fundo fundo e o enchimento cinza para tingir (64x32)
  H_Slot, H_SlotLocked                                 o encaixe das habilidades (96x96): moldura grossa, miolo fundo
  H_Round                                              o encaixe redondo das poções (cinza para tingir, 96x96)
  H_Disc                                               a moeda/selo (cinza para tingir, 64x64)
  H_Ring, H_MapRing                                    aros de ouro do retrato e do minimapa (miolo vazio)
  H_Key                                                a plaquinha da tecla (48x32)

    python Tools/MakeHudFantasy.py      (depois: Tools/ImportHudCartoon.py com o editor fechado)
As peças antigas (cartoon) ficaram em Data/UI/HudCartoon_antigo.
"""
import math
import os
import random

from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "UI", "HudCartoon")
SS = 4
INK = (8, 9, 16)
NIGHT = (22, 26, 44)
GOLD_HI, GOLD, GOLD_LO = (255, 228, 150), (214, 162, 74), (112, 74, 32)


def save(big, name):
    big.resize((big.width // SS, big.height // SS), Image.LANCZOS).save(os.path.join(OUT, name + ".png"))


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def vgrad(W, H, top, bottom):
    g = Image.new("RGB", (W, H))
    d = ImageDraw.Draw(g)
    for y in range(H):
        d.line([(0, y), (W, y)], fill=lerp(top, bottom, y / max(1, H - 1)))
    return g


def grain(img, amount=10, seed=1):
    """Uma textura leve de pedra/couro (o fundo dos ícones não é liso)."""
    rnd = random.Random(seed)
    n = Image.new("L", (img.width // 6 + 1, img.height // 6 + 1))
    n.putdata([128 + rnd.randint(-amount, amount) for _ in range(n.width * n.height)])
    n = n.resize(img.size, Image.BICUBIC).filter(ImageFilter.GaussianBlur(2))
    r, g, b = img.split()[:3]
    f = lambda ch: Image.eval(Image.merge("LA", (ch, n)).convert("L") if False else ch, lambda v: v)
    out = Image.merge("RGB", [Image.composite(ch, ch, n) for ch in (r, g, b)])
    shade = n.point(lambda v: v)
    return Image.blend(img.convert("RGB"), Image.merge("RGB", (shade, shade, shade)), .06)


def mask_round(W, H, r, inset=0):
    m = Image.new("L", (W, H), 0)
    ImageDraw.Draw(m).rounded_rectangle((inset, inset, W - 1 - inset, H - 1 - inset), max(0, r - inset), fill=255)
    return m


def gold_band(W, H, r, outer, inner, lo=GOLD_LO, mid=GOLD, hi=GOLD_HI):
    """O aro de ouro entre `outer` e `inner` (px na escala SS), em relevo: claro em cima e à esquerda."""
    band = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    ring = Image.new("L", (W, H), 0)
    ring.paste(mask_round(W, H, r, outer), (0, 0))
    ring = Image.composite(Image.new("L", (W, H), 0), ring, mask_round(W, H, r, inner))
    g = vgrad(W, H, hi, lo)
    # o fio de luz no meio do aro
    mid_img = Image.new("RGB", (W, H), mid)
    g = Image.blend(g, mid_img, .35)
    band.paste(g, (0, 0), ring)
    edge = mask_round(W, H, r, (outer + inner) // 2)
    hl = Image.new("L", (W, H), 0)
    ImageDraw.Draw(hl).rounded_rectangle(((outer + inner) // 2,) * 2 + (W - 1 - (outer + inner) // 2, H - 1 - (outer + inner) // 2),
                                         max(0, r - (outer + inner) // 2), outline=110, width=max(1, SS))
    band = Image.alpha_composite(band, Image.merge("RGBA", (Image.new("L", (W, H), 255),) * 3 + (Image.composite(hl, Image.new("L", (W, H), 0), ring),)))
    return band


def rivet(d, cx, cy, rad):
    d.ellipse((cx - rad, cy - rad, cx + rad, cy + rad), fill=INK)
    d.ellipse((cx - rad * .75, cy - rad * .75, cx + rad * .75, cy + rad * .75), fill=GOLD_LO)
    d.ellipse((cx - rad * .6, cy - rad * .7, cx + rad * .45, cy + rad * .35), fill=GOLD)
    d.ellipse((cx - rad * .45, cy - rad * .55, cx + rad * .05, cy - rad * .05), fill=GOLD_HI)


def frame(w, h, r=5, band=(1.6, 5.2), body=NIGHT, body_lo=None, studs=True, gloss=True, seed=1):
    """Painel/encaixe: fio escuro, aro de ouro de band[0] a band[1] px, fio escuro e o miolo com textura e sombra."""
    W, H, R = w * SS, h * SS, r * SS
    o, i = int(band[0] * SS), int(band[1] * SS)
    big = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    big.paste(Image.new("RGB", (W, H), INK), (0, 0), mask_round(W, H, R))
    big = Image.alpha_composite(big, gold_band(W, H, R, o, i))
    ins = i + int(1.1 * SS)
    core = vgrad(W, H, lerp(body, (255, 255, 255), .05) if gloss else body, body_lo or lerp(body, INK, .45))
    core = grain(core, 14, seed)
    m = mask_round(W, H, R, ins)
    big.paste(core, (0, 0), m)
    # a sombra de dentro (o miolo afundado embaixo da moldura)
    sh = Image.new("L", (W, H), 0)
    ImageDraw.Draw(sh).rounded_rectangle((ins, ins, W - 1 - ins, H - 1 - ins), max(0, R - ins), outline=170, width=int(2.4 * SS))
    sh = sh.filter(ImageFilter.GaussianBlur(2.2 * SS))
    sh = Image.composite(sh, Image.new("L", (W, H), 0), m)
    big = Image.alpha_composite(big, Image.merge("RGBA", (Image.new("L", (W, H), 0),) * 3 + (sh,)))
    if studs:
        d = ImageDraw.Draw(big)
        c = (o + i) / 2
        for x in (c, W - 1 - c):
            for y in (c, H - 1 - c):
                rivet(d, x, y, (i - o) * .62)
    return big


def disc(size, fill, ring=True, hole=0, gold=False):
    S = size * SS
    big = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    d.ellipse((0, 0, S - 1, S - 1), fill=INK)
    if gold:
        g = vgrad(S, S, GOLD_HI, GOLD_LO)
        g = Image.blend(g, Image.new("RGB", (S, S), GOLD), .3)
    else:
        g = vgrad(S, S, lerp(fill, (255, 255, 255), .35), lerp(fill, INK, .35))
    m = Image.new("L", (S, S), 0)
    ImageDraw.Draw(m).ellipse((int(1.5 * SS), int(1.5 * SS), S - 1 - int(1.5 * SS), S - 1 - int(1.5 * SS)), fill=255)
    big.paste(g, (0, 0), m)
    if ring:
        # o aro interno (o fio escuro e a luz em cima)
        r0 = int(S * .14)
        d.ellipse((r0, r0, S - 1 - r0, S - 1 - r0), outline=INK, width=int(1.3 * SS))
        d.arc((r0 + SS, r0 + SS, S - 1 - r0 - SS, S - 1 - r0 - SS), 200, 330, fill=(255, 255, 255, 120), width=int(1.2 * SS))
    if hole:
        h = int(hole * SS)
        hm = Image.new("L", (S, S), 255)
        ImageDraw.Draw(hm).ellipse((h, h, S - 1 - h, S - 1 - h), fill=0)
        a = Image.composite(big.split()[3], Image.new("L", (S, S), 0), hm)
        big.putalpha(a)
        d = ImageDraw.Draw(big)
        d.ellipse((h - SS, h - SS, S - h + SS - 1, S - h + SS - 1), outline=INK, width=int(1.4 * SS))
    return big


def ornate_ring(size, hole):
    """O aro de ouro do retrato e do minimapa: duas linhas, os entalhes a cada 30 graus e os 4 cravos de safira."""
    big = disc(size, GOLD, ring=False, hole=hole, gold=True)
    S = size * SS
    d = ImageDraw.Draw(big)
    c = S / 2
    rin, rout = c - hole * SS, c - 1.5 * SS
    mid = (rin + rout) / 2
    d.ellipse((c - mid, c - mid, c + mid, c + mid), outline=GOLD_LO, width=int(max(1, size / 128) * SS))
    for k in range(12):
        a = math.radians(k * 30)
        x0, y0 = c + math.cos(a) * (rin + 2 * SS), c + math.sin(a) * (rin + 2 * SS)
        x1, y1 = c + math.cos(a) * (rout - 2 * SS), c + math.sin(a) * (rout - 2 * SS)
        d.line([(x0, y0), (x1, y1)], fill=GOLD_LO, width=int(max(1, size / 160) * SS))
    for k in range(4):
        a = math.radians(45 + k * 90)
        x, y = c + math.cos(a) * mid, c + math.sin(a) * mid
        rr = (rout - rin) * .38
        d.ellipse((x - rr, y - rr, x + rr, y + rr), fill=INK)
        d.ellipse((x - rr * .75, y - rr * .75, x + rr * .75, y + rr * .75), fill=(40, 90, 190))
        d.ellipse((x - rr * .5, y - rr * .6, x + rr * .1, y - rr * .05), fill=(150, 200, 255))
    return big


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    save(frame(64, 64, 5, body=(30, 40, 74), seed=2), "H_PanelBlue")
    save(frame(64, 64, 5, body=NIGHT, seed=3), "H_PanelNavy")
    save(frame(64, 64, 5, body=(84, 24, 30), seed=4), "H_PanelRed")
    save(frame(64, 64, 5, body=(190, 140, 60), body_lo=(120, 80, 30), seed=5), "H_PanelGold")
    save(frame(64, 32, 3, band=(1.2, 3.6), body=(10, 12, 22), studs=False, gloss=False, seed=6), "H_BarBack")
    # o enchimento: cinza com brilho em cima (o jogo tinge: vida, mana, XP)
    W, H = 64 * SS, 32 * SS
    fill = vgrad(W, H, (250, 250, 250), (120, 120, 120))
    fd = ImageDraw.Draw(fill)
    fd.rectangle((0, int(H * .12), W, int(H * .36)), fill=(255, 255, 255))
    fill = fill.filter(ImageFilter.GaussianBlur(1.5 * SS))
    bar = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    bar.paste(fill, (0, 0), mask_round(W, H, 2 * SS, int(.6 * SS)))
    save(bar, "H_BarFill")
    save(frame(96, 96, 4, band=(1.6, 8.0), body=(16, 20, 36), seed=7), "H_Slot")
    save(frame(96, 96, 4, band=(1.6, 8.0), body=(12, 13, 20), seed=8), "H_SlotLocked")
    save(disc(96, (200, 200, 200), hole=0), "H_Round")
    save(disc(64, (235, 235, 235), ring=True), "H_Disc")
    save(ornate_ring(128, 16), "H_Ring")
    save(ornate_ring(512, 34), "H_MapRing")
    save(frame(48, 32, 3, band=(1.0, 3.0), body=(14, 16, 28), studs=False, seed=9), "H_Key")
    # a folha de conferência
    names = ["H_PanelNavy", "H_PanelBlue", "H_PanelRed", "H_PanelGold", "H_Slot", "H_SlotLocked", "H_Key", "H_BarBack", "H_BarFill", "H_Round", "H_Disc", "H_Ring"]
    sheet = Image.new("RGBA", (12 * 140, 150), (60, 70, 60, 255))
    for n, name in enumerate(names):
        im = Image.open(os.path.join(OUT, name + ".png")).convert("RGBA")
        im = im.resize((min(128, im.width * 2), min(128, im.height * 2)), Image.NEAREST)
        sheet.alpha_composite(im, (n * 140 + 6, 10))
    sheet.save(os.path.join(OUT, "preview_fantasia.png"))
    print("HUD fantasia: %d peças em %s" % (len(names) + 1, OUT))
