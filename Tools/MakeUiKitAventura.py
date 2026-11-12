"""Kit "Aventura" das janelas do mundo (inventário, herói, técnicas, classes, alquimia, missões, loja):
pixel-art com cara de jogo de blocos, no lugar das molduras de linha dourada fina do kit antigo.

- Madeira escura com veio, aro de ouro e rebites nos cantos (a janela).
- Pedra em blocos ladrilhada no fundo (W_StoneTile, repetida, não esticada).
- Espaços de item "afundados" (chanfro para dentro) e brilho da raridade por cima (W_SlotGlow, tingido no jogo).
- Botões e abas "de bloco" (chanfro para fora), faixa de título vermelha com as pontas dobradas.

Cada peça é desenhada pequena e ampliada 3x sem suavizar (pixels nítidos). As de nove fatias têm a margem
indicada em VortexUiKit.h (Aventura::*).
    python Tools/MakeUiKitAventura.py      (depois: Tools/ImportUiKitAventura.py com o editor fechado)
"""
import os
import random
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "UI", "KitAventura")
os.makedirs(OUT, exist_ok=True)
SCALE = 3
rng = random.Random(7)

OUTLINE = (14, 10, 8, 255)
WOOD = [(58, 36, 20), (74, 47, 27), (88, 57, 33), (104, 68, 39)]
WOOD_HI = (142, 98, 58)
WOOD_LO = (36, 22, 12)
GOLD = (233, 182, 72)
GOLD_HI = (255, 230, 150)
GOLD_LO = (150, 98, 34)
STONE = [(30, 34, 48), (34, 38, 54), (38, 43, 60), (27, 31, 44)]
MORTAR = (18, 20, 30)
INSET = (13, 16, 25)
INSET_HI = (58, 66, 88)
INSET_LO = (6, 7, 12)


def img(w, h, fill=(0, 0, 0, 0)):
    return Image.new("RGBA", (w, h), fill)


def put(im, x, y, c):
    if 0 <= x < im.width and 0 <= y < im.height:
        im.putpixel((x, y), c if len(c) == 4 else c + (255,))


def rect(im, x0, y0, x1, y1, c):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            put(im, x, y, c)


def save(im, name):
    im.resize((im.width * SCALE, im.height * SCALE), Image.NEAREST).save(os.path.join(OUT, name + ".png"))


def bevel(im, x0, y0, x1, y1, face, hi, lo, depth=1, raised=True):
    """Caixa com chanfro: claro em cima/esquerda e escuro embaixo/direita (raised) ou o contrário (afundado)."""
    rect(im, x0, y0, x1, y1, face)
    a, b = (hi, lo) if raised else (lo, hi)
    for d in range(depth):
        for x in range(x0 + d, x1 - d + 1):
            put(im, x, y0 + d, a)
            put(im, x, y1 - d, b)
        for y in range(y0 + d, y1 - d + 1):
            put(im, x0 + d, y, a)
            put(im, x1 - d, y, b)


def outline(im, x0, y0, x1, y1, c=OUTLINE):
    for x in range(x0, x1 + 1):
        put(im, x, y0, c)
        put(im, x, y1, c)
    for y in range(y0, y1 + 1):
        put(im, x0, y, c)
        put(im, x1, y, c)


def wood_pixel(x, y):
    # veio na horizontal: faixas com ruído
    band = (y * 3 + (x // 7) + rng.randint(0, 1)) % 4
    return WOOD[band]


def rivet(im, cx, cy):
    rect(im, cx - 1, cy - 1, cx + 1, cy + 1, GOLD)
    put(im, cx - 1, cy - 1, GOLD_HI)
    put(im, cx, cy - 1, GOLD_HI)
    put(im, cx - 1, cy, GOLD_HI)
    put(im, cx + 1, cy + 1, GOLD_LO)
    put(im, cx + 1, cy, GOLD_LO)
    put(im, cx, cy + 1, GOLD_LO)


# ---- a janela: moldura de madeira com aro de ouro e rebites (o meio é transparente: a pedra vem por baixo)
def window():
    W = 48
    im = img(W, W)
    band = 6
    for y in range(W):
        for x in range(W):
            if x < band or y < band or x >= W - band or y >= W - band:
                put(im, x, y, wood_pixel(x, y))
    # chanfro da madeira (luz em cima/esquerda)
    for i in range(W):
        put(im, i, 1, WOOD_HI); put(im, 1, i, WOOD_HI)
        put(im, i, W - 2, WOOD_LO); put(im, W - 2, i, WOOD_LO)
    outline(im, 0, 0, W - 1, W - 1)
    # aro de ouro por dentro da madeira
    for i in range(band - 1, W - band + 1):
        put(im, i, band - 1, GOLD_HI); put(im, band - 1, i, GOLD_HI)
        put(im, i, W - band, GOLD_LO); put(im, W - band, i, GOLD_LO)
    outline(im, band, band, W - band - 1, W - band - 1, (8, 9, 14, 255))
    # rebites nos cantos e no meio dos lados (os do meio esticam junto, ficam só nos cantos)
    for cx, cy in ((3, 3), (W - 4, 3), (3, W - 4), (W - 4, W - 4)):
        rivet(im, cx, cy)
    save(im, "W_Frame")


def stone_tile():
    T = 32
    im = img(T, T)
    # blocos de 8x8 desencontrados (fileira sim, fileira não), tons sorteados, argamassa escura
    for row in range(4):
        off = 4 if row % 2 else 0
        for col in range(-1, 5):
            tone = STONE[rng.randint(0, 3)]
            x0 = col * 8 + off
            y0 = row * 8
            for y in range(y0, y0 + 8):
                for x in range(x0, x0 + 8):
                    xx = x % T
                    c = tone
                    if (x - x0) == 7 or (y - y0) == 7:
                        c = MORTAR
                    elif (x - x0) == 0 or (y - y0) == 0:
                        c = tuple(min(255, v + 6) for v in tone)
                    elif rng.random() < .08:
                        c = tuple(max(0, v - 4) for v in tone)
                    put(im, xx, y, c)
    save(im, "W_StoneTile")


def panel():
    S = 24
    im = img(S, S)
    bevel(im, 0, 0, S - 1, S - 1, INSET + (235,), INSET_HI, INSET_LO, depth=1, raised=False)
    # sombra mais grossa em cima (parece afundado na pedra)
    for x in range(1, S - 1):
        put(im, x, 1, (9, 11, 18))
    save(im, "W_Panel")


def slot(name, edge_hi, edge_lo, gold=False):
    S = 20
    im = img(S, S)
    outline(im, 0, 0, S - 1, S - 1)
    bevel(im, 1, 1, S - 2, S - 2, (12, 14, 22), edge_hi, edge_lo, depth=2, raised=False)
    # fundo com um pontilhado leve (não fica chapado)
    for y in range(4, S - 4):
        for x in range(4, S - 4):
            if (x + y) % 5 == 0:
                put(im, x, y, (16, 19, 29))
    if gold:
        outline(im, 1, 1, S - 2, S - 2, GOLD + (255,))
        outline(im, 2, 2, S - 3, S - 3, GOLD_LO + (255,))
    save(im, name)


def slot_glow():
    S = 20
    im = img(S, S)
    # moldura branca que some para dentro (tingida com a cor da raridade no jogo)
    for i, a in enumerate((255, 170, 90, 40)):
        outline(im, 1 + i, 1 + i, S - 2 - i, S - 2 - i, (255, 255, 255, a))
    save(im, "W_SlotGlow")


def button(name, face, hi, lo, raised=True):
    W, H = 24, 12
    im = img(W, H)
    outline(im, 0, 0, W - 1, H - 1)
    bevel(im, 1, 1, W - 2, H - 2, face, hi, lo, depth=1, raised=raised)
    # sombra de baixo mais grossa (botão de bloco)
    for x in range(1, W - 1):
        put(im, x, H - 3, lo if raised else hi)
    save(im, name)


def tab(name, active):
    W, H = 22, 20
    im = img(W, H)
    face = (122, 84, 44) if active else (70, 45, 26)
    hi = GOLD_HI if active else (110, 76, 44)
    lo = GOLD_LO if active else (38, 24, 13)
    outline(im, 0, 0, W - 1, H - 1)
    bevel(im, 1, 1, W - 2, H - 2, face, hi, lo, depth=1)
    for y in range(2, H - 2):
        for x in range(2, W - 2):
            if (y * 3 + x // 6) % 5 == 0:
                put(im, x, y, tuple(max(0, v - 10) for v in face))
    if active:
        outline(im, 1, 1, W - 2, H - 2, GOLD + (255,))
    save(im, name)


def ribbon():
    W, H = 48, 14
    im = img(W, H)
    red, red_hi, red_lo = (150, 32, 30), (196, 58, 48), (92, 18, 18)
    # pontas dobradas (mais escuras) e o corpo
    for y in range(H):
        for x in range(W):
            tail = x < 6 or x >= W - 6
            if tail:
                notch = abs(y - H // 2) < (3 - abs((x if x < 6 else W - 1 - x) - 0) // 2)
                if (x < 2 or x >= W - 2) and abs(y - H // 2) < 3:
                    continue
                put(im, x, y, red_lo)
            else:
                put(im, x, y, red)
    for x in range(6, W - 6):
        put(im, x, 1, GOLD_HI); put(im, x, 2, red_hi)
        put(im, x, H - 2, GOLD_LO); put(im, x, H - 3, red_lo)
    outline(im, 0, 0, W - 1, H - 1)
    for x in range(W):
        for y in (0, H - 1):
            if x < 2 or x >= W - 2:
                put(im, x, y, (0, 0, 0, 0))
    save(im, "W_Ribbon")


def card(name, face, hi, lo):
    S = 24
    im = img(S, S)
    outline(im, 0, 0, S - 1, S - 1)
    bevel(im, 1, 1, S - 2, S - 2, face, hi, lo, depth=2)
    save(im, name)


def bar_back():
    W, H = 16, 8
    im = img(W, H)
    outline(im, 0, 0, W - 1, H - 1)
    bevel(im, 1, 1, W - 2, H - 2, (10, 12, 18), INSET_HI, INSET_LO, raised=False)
    save(im, "W_BarBack")


def bar_fill():
    W, H = 16, 8
    im = img(W, H)
    rect(im, 0, 0, W - 1, H - 1, (210, 210, 210))
    for x in range(W):
        put(im, x, 0, (255, 255, 255)); put(im, x, 1, (240, 240, 240))
        put(im, x, H - 1, (140, 140, 140))
    save(im, "W_BarFill")


def stage():
    # o chão da vitrine do herói: uma placa de pedra em losango com borda de ouro, vista de cima em perspectiva
    W, H = 64, 24
    im = img(W, H)
    cx, cy = W // 2, H // 2
    for y in range(H):
        for x in range(W):
            d = abs(x - cx) / (W / 2) + abs(y - cy) / (H / 2)
            if d <= 1.0:
                tone = STONE[(x // 6 + y // 3) % 4]
                if d > .86:
                    tone = GOLD if d < .93 else GOLD_LO
                elif d > .8:
                    tone = (12, 14, 22)
                put(im, x, y, tone)
    save(im, "W_Stage")


def key_cap():
    W, H = 16, 14
    im = img(W, H)
    outline(im, 0, 0, W - 1, H - 1)
    bevel(im, 1, 1, W - 2, H - 2, (52, 58, 78), (96, 104, 130), (22, 25, 36), depth=1)
    for x in range(1, W - 1):
        put(im, x, H - 3, (22, 25, 36))
    save(im, "W_Key")


window()
stone_tile()
panel()
slot("W_Slot", INSET_HI, INSET_LO)
slot("W_SlotHover", (110, 118, 150), INSET_LO)
slot("W_SlotActive", GOLD_HI, GOLD_LO, gold=True)
slot_glow()
button("W_Button", (46, 52, 72), (92, 100, 128), (18, 20, 30))
button("W_ButtonHover", (60, 68, 94), (120, 130, 164), (22, 25, 36))
button("W_ButtonPressed", (34, 38, 54), (92, 100, 128), (18, 20, 30), raised=False)
button("W_ButtonGold", (214, 160, 58), GOLD_HI, GOLD_LO)
button("W_ButtonGoldHover", (236, 188, 84), (255, 242, 180), GOLD_LO)
button("W_ButtonDisabled", (40, 40, 44), (64, 64, 70), (20, 20, 22))
tab("W_Tab", False)
tab("W_TabActive", True)
ribbon()
card("W_Card", (36, 41, 58), (70, 78, 104), (16, 18, 28))
card("W_CardLocked", (24, 26, 32), (44, 46, 54), (12, 12, 16))
bar_back()
bar_fill()
stage()
key_cap()
print("VORTEX_KIT_AVENTURA", len(os.listdir(OUT)), "peças em", OUT)
