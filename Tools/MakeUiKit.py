"""Kit das janelas no estilo da HUD (azul-noite com moldura dourada e gemas azuis), Data/UI/Kit/*.png.
Cada peça é desenhada para nove fatias (os cantos ficam, o meio estica) e usada pelo Slate
(VortexUiKit.h): janela, painel, espaço de item (normal, passando o mouse, escolhido), botão (comum,
dourado), aba e a faixa do título (buff_frame da HUD do Godot, copiada).
    python Tools/MakeUiKit.py      (depois: Tools/ImportUiKit.py no editor fechado)
"""
import os
import shutil
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "UI", "Kit")
GODOT = os.path.normpath(os.path.join(ROOT, "..", "..", "the-world-geneses", "ui", "art", "hud"))
os.makedirs(OUT, exist_ok=True)

GOLD_HI = (246, 215, 122)
GOLD = (201, 146, 58)
GOLD_LO = (107, 69, 19)
NAVY_TOP = (13, 21, 40)
NAVY_BOT = (6, 10, 20)
GEM = (60, 140, 255)
S = 4   # desenha 4x maior e reduz (bordas lisas)


def canvas(w, h):
    return Image.new("RGBA", (w * S, h * S), (0, 0, 0, 0))


def finish(img, name):
    w, h = img.size
    img = img.resize((w // S, h // S), Image.LANCZOS)
    img.save(os.path.join(OUT, name + ".png"))


def gradient(w, h, top, bottom, alpha=255):
    t = np.linspace(0, 1, h)[:, None, None]
    col = np.array(top)[None, None, :] * (1 - t) + np.array(bottom)[None, None, :] * t
    arr = np.concatenate([np.repeat(col, w, axis=1), np.full((h, w, 1), alpha)], axis=2)
    # um grão leve (não fica chapado)
    rng = np.random.default_rng(7)
    arr[..., :3] += rng.normal(0, 1.6, (h, w, 1))
    return Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), "RGBA")


def bevel_rect(draw, box, width, hi=GOLD_HI, mid=GOLD, lo=GOLD_LO):
    """Moldura dourada com relevo: claro em cima/esquerda, escuro embaixo/direita."""
    x0, y0, x1, y1 = box
    for i in range(width):
        t = i / max(1, width - 1)
        c_top = tuple(int(hi[k] * (1 - t) + mid[k] * t) for k in range(3))
        c_bot = tuple(int(mid[k] * (1 - t) + lo[k] * t) for k in range(3))
        draw.line([(x0 + i, y0 + i), (x1 - i, y0 + i)], fill=c_top + (255,))
        draw.line([(x0 + i, y0 + i), (x0 + i, y1 - i)], fill=c_top + (255,))
        draw.line([(x0 + i, y1 - i), (x1 - i, y1 - i)], fill=c_bot + (255,))
        draw.line([(x1 - i, y0 + i), (x1 - i, y1 - i)], fill=c_bot + (255,))


def gem(draw, cx, cy, r):
    draw.polygon([(cx, cy - r), (cx + r, cy), (cx, cy + r), (cx - r, cy)], fill=GOLD_LO + (255,))
    r2 = r * .72
    draw.polygon([(cx, cy - r2), (cx + r2, cy), (cx, cy + r2), (cx - r2, cy)], fill=GEM + (255,))
    r3 = r * .3
    draw.polygon([(cx - r3 * .2, cy - r2 * .8), (cx + r3, cy - r3 * .2), (cx - r3 * .2, cy - r3 * .2)], fill=(200, 230, 255, 255))


def flourish(draw, cx, cy, sx, sy, size):
    """Ornamento de canto: duas volutas douradas saindo do canto (sx, sy = para onde abre)."""
    for k, l in ((0, size), (1, size * .62)):
        w = int(size * .09)
        if k == 0:
            draw.line([(cx, cy), (cx + sx * l, cy)], fill=GOLD + (255,), width=w)
            draw.line([(cx, cy), (cx, cy + sy * l)], fill=GOLD + (255,), width=w)
        else:
            draw.arc([cx + sx * l * .2 - l * .5, cy + sy * l * .2 - l * .5, cx + sx * l * .2 + l * .5, cy + sy * l * .2 + l * .5],
                     0, 360, fill=GOLD_HI + (200,), width=max(1, w // 2))


# ---- janela (192, fatias de 48) ---------------------------------------------------------------------
def window():
    W = 192
    img = canvas(W, W)
    inset = 7 * S
    fill = gradient(W * S - 2 * inset, W * S - 2 * inset, NAVY_TOP, NAVY_BOT, 248)
    img.alpha_composite(fill, (inset, inset))
    d = ImageDraw.Draw(img)
    # sombra externa curta
    bevel_rect(d, (3 * S, 3 * S, W * S - 3 * S - 1, W * S - 3 * S - 1), 5 * S)
    d.rectangle([9 * S, 9 * S, W * S - 9 * S - 1, W * S - 9 * S - 1], outline=(0, 0, 0, 200), width=S)
    d.rectangle([15 * S, 15 * S, W * S - 15 * S - 1, W * S - 15 * S - 1], outline=GOLD + (120,), width=S)
    # cantos: voluta e a gema azul
    for cx, cy, sx, sy in ((14, 14, 1, 1), (W - 14, 14, -1, 1), (14, W - 14, 1, -1), (W - 14, W - 14, -1, -1)):
        flourish(d, cx * S, cy * S, sx, sy, 26 * S)
        gem(d, cx * S, cy * S, 7 * S)
    finish(img, "T_UiWindow")


# ---- painel interno (64, fatias de 16) --------------------------------------------------------------
def panel():
    W = 64
    img = canvas(W, W)
    img.alpha_composite(gradient(W * S - 4 * S, W * S - 4 * S, (8, 13, 26), (4, 7, 15), 215), (2 * S, 2 * S))
    d = ImageDraw.Draw(img)
    d.rectangle([2 * S, 2 * S, W * S - 2 * S - 1, W * S - 2 * S - 1], outline=GOLD + (170,), width=S)
    d.rectangle([4 * S, 4 * S, W * S - 4 * S - 1, W * S - 4 * S - 1], outline=(0, 0, 0, 160), width=S)
    for cx, cy in ((3, 3), (W - 3, 3), (3, W - 3), (W - 3, W - 3)):
        r = 3 * S
        d.polygon([(cx * S, cy * S - r), (cx * S + r, cy * S), (cx * S, cy * S + r), (cx * S - r, cy * S)], fill=GOLD_HI + (255,))
    finish(img, "T_UiPanel")


# ---- espaço de item (64) ---------------------------------------------------------------------------
def slot(name, rim_hi, rim, glow, gems):
    W = 64
    img = canvas(W, W)
    d = ImageDraw.Draw(img)
    if glow:
        halo = canvas(W, W)
        ImageDraw.Draw(halo).rectangle([3 * S, 3 * S, W * S - 3 * S, W * S - 3 * S], outline=glow + (210,), width=4 * S)
        img.alpha_composite(halo.filter(ImageFilter.GaussianBlur(3 * S)))
    img.alpha_composite(gradient(W * S - 10 * S, W * S - 10 * S, (16, 26, 48), (5, 9, 18), 240), (5 * S, 5 * S))
    bevel_rect(d, (4 * S, 4 * S, W * S - 4 * S - 1, W * S - 4 * S - 1), 3 * S, hi=rim_hi, mid=rim, lo=GOLD_LO)
    # sombra de dentro (o espaço parece fundo)
    shade = canvas(W, W)
    ImageDraw.Draw(shade).rectangle([7 * S, 7 * S, W * S - 7 * S, W * S - 7 * S], outline=(0, 0, 0, 170), width=3 * S)
    img.alpha_composite(shade.filter(ImageFilter.GaussianBlur(2 * S)))
    if gems:
        for cx, cy in ((6, 6), (W - 6, 6), (6, W - 6), (W - 6, W - 6)):
            gem(d, cx * S, cy * S, 4 * S)
    finish(img, name)


# ---- botões (96x40, fatias de 14) ------------------------------------------------------------------
def button(name, top, bottom, rim_hi, rim, text_glow=None):
    W, H = 96, 40
    img = canvas(W, H)
    img.alpha_composite(gradient(W * S - 6 * S, H * S - 6 * S, top, bottom, 250), (3 * S, 3 * S))
    d = ImageDraw.Draw(img)
    bevel_rect(d, (2 * S, 2 * S, W * S - 2 * S - 1, H * S - 2 * S - 1), 2 * S, hi=rim_hi, mid=rim, lo=GOLD_LO)
    d.line([(6 * S, 6 * S), (W * S - 6 * S, 6 * S)], fill=(255, 255, 255, 40), width=S)   # brilho de cima
    for cx in (7, W - 7):
        r = 3 * S
        d.polygon([(cx * S, H * S / 2 - r), (cx * S + r, H * S / 2), (cx * S, H * S / 2 + r), (cx * S - r, H * S / 2)], fill=rim_hi + (255,))
    finish(img, name)


# ---- abas (96x36, fatias de 14) --------------------------------------------------------------------
def tab(name, active):
    W, H = 96, 36
    img = canvas(W, H)
    d = ImageDraw.Draw(img)
    if active:
        img.alpha_composite(gradient(W * S - 4 * S, H * S - 2 * S, (22, 36, 66), (9, 15, 30), 245), (2 * S, 2 * S))
        bevel_rect(d, (1 * S, 1 * S, W * S - 1 * S - 1, H * S + 4 * S), 2 * S)
        gem(d, W * S / 2, 2 * S, 4 * S)
    else:
        d.line([(4 * S, H * S - 2 * S), (W * S - 4 * S, H * S - 2 * S)], fill=GOLD + (110,), width=S)
    finish(img, name)


# ---- linha com relevo (128x40, fatias de 14): atributos, estatísticas, missões --------------------
def row(name, rim, glow):
    W, H = 128, 40
    img = canvas(W, H)
    if glow:
        halo = canvas(W, H)
        ImageDraw.Draw(halo).rounded_rectangle([3 * S, 3 * S, W * S - 3 * S, H * S - 3 * S], 6 * S, outline=glow + (150,), width=3 * S)
        img.alpha_composite(halo.filter(ImageFilter.GaussianBlur(2 * S)))
    body = gradient(W * S - 6 * S, H * S - 6 * S, (22, 34, 60), (8, 13, 26), 236)
    mask = Image.new("L", body.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, body.size[0] - 1, body.size[1] - 1], 5 * S, fill=255)
    img.paste(body, (3 * S, 3 * S), mask)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([3 * S, 3 * S, W * S - 3 * S - 1, H * S - 3 * S - 1], 5 * S, outline=rim + (220,), width=S)
    d.line([(9 * S, 5 * S), (W * S - 9 * S, 5 * S)], fill=(255, 255, 255, 34), width=S)        # brilho de cima
    d.line([(9 * S, H * S - 5 * S), (W * S - 9 * S, H * S - 5 * S)], fill=(0, 0, 0, 120), width=S)  # sombra de baixo
    finish(img, name)


# ---- gema redonda branca (32): o jogo pinta na cor de cada atributo ------------------------------
def round_gem():
    W = 32
    img = canvas(W, W)
    d = ImageDraw.Draw(img)
    c = W * S / 2
    d.ellipse([2 * S, 2 * S, W * S - 2 * S, W * S - 2 * S], fill=(110, 80, 30, 255))                 # aro
    d.ellipse([4 * S, 4 * S, W * S - 4 * S, W * S - 4 * S], fill=(150, 150, 150, 255))
    d.ellipse([6 * S, 6 * S, W * S - 6 * S, W * S - 6 * S], fill=(225, 225, 225, 255))
    for k in range(6):   # facetas
        import math
        a0 = k * math.pi / 3
        d.line([(c, c), (c + math.cos(a0) * 10 * S, c + math.sin(a0) * 10 * S)], fill=(180, 180, 180, 255), width=S)
    d.ellipse([9 * S, 7 * S, 15 * S, 12 * S], fill=(255, 255, 255, 255))                          # brilho
    finish(img, "T_UiGemRound")


# ---- faixa de título (256x44, fatias de 30 nas pontas) ---------------------------------------------
def banner():
    W, H = 256, 44
    img = canvas(W, H)
    pts = [(14 * S, 6 * S), (W * S - 14 * S, 6 * S), (W * S - 2 * S, H * S / 2), (W * S - 14 * S, H * S - 6 * S), (14 * S, H * S - 6 * S), (2 * S, H * S / 2)]
    body = gradient(W * S, H * S, (30, 46, 80), (10, 16, 32), 245)
    mask = Image.new("L", (W * S, H * S), 0)
    ImageDraw.Draw(mask).polygon(pts, fill=255)
    img.paste(body, (0, 0), mask)
    d = ImageDraw.Draw(img)
    d.line(pts + [pts[0]], fill=GOLD + (255,), width=2 * S)
    inner = [(16 * S, 9 * S), (W * S - 16 * S, 9 * S), (W * S - 6 * S, H * S / 2), (W * S - 16 * S, H * S - 9 * S), (16 * S, H * S - 9 * S), (6 * S, H * S / 2)]
    d.line(inner + [inner[0]], fill=GOLD_HI + (90,), width=S)
    for cx in (7, W - 7):
        gem(d, cx * S, H * S / 2, 4 * S)
    finish(img, "T_UiBanner")


# ---- divisória (256x14): fio dourado com losango no meio ------------------------------------------
def divider():
    W, H = 256, 14
    img = canvas(W, H)
    d = ImageDraw.Draw(img)
    for x in range(0, W * S, S):
        t = 1 - abs(x - W * S / 2) / (W * S / 2)
        d.line([(x, H * S / 2), (x + S, H * S / 2)], fill=GOLD + (int(255 * min(1, t * 1.6)),), width=S)
    gem(d, W * S / 2, H * S / 2, 5 * S)
    finish(img, "T_UiDivider")


window()
panel()
row("T_UiRow", (120, 90, 40), None)
row("T_UiRowHover", GOLD, (246, 215, 122))
round_gem()
banner()
divider()
slot("T_UiSlot", (190, 146, 70), (140, 100, 40), None, False)
slot("T_UiSlotHover", GOLD_HI, GOLD, (246, 215, 122), False)
slot("T_UiSlotActive", GOLD_HI, GOLD, (80, 150, 255), True)
button("T_UiButton", (20, 32, 58), (9, 15, 30), (210, 160, 80), (150, 104, 40))
button("T_UiButtonHover", (32, 50, 88), (14, 22, 42), GOLD_HI, GOLD)
button("T_UiButtonPressed", (8, 13, 26), (14, 22, 42), GOLD, GOLD_LO)
button("T_UiButtonGold", (232, 190, 92), (150, 100, 30), (255, 236, 160), (210, 160, 70))
button("T_UiButtonGoldHover", (250, 214, 120), (176, 120, 40), (255, 246, 200), (232, 190, 92))
button("T_UiButtonDisabled", (16, 18, 24), (10, 11, 15), (90, 90, 96), (60, 60, 66))
tab("T_UiTab", False)
tab("T_UiTabActive", True)
shutil.copy(os.path.join(GODOT, "buff_frame.png"), os.path.join(OUT, "T_UiHeader.png"))
print("ok", OUT)
