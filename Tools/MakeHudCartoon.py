"""Arte cartoon da HUD (o mesmo estilo das janelas: Tools/MakeUiKitCartoon.py) -> Data/UI/HudCartoon/*.png.

Peças desenhadas 3x maiores e reduzidas (bordas lisas). Contorno escuro grosso, base mais escura e o brilho macio
dentro da borda arredondada (no esboço o brilho era um retângulo solto que vazava como faixa cinza).
Os painéis são de 9 partes (o AVortexAimHUD estica o meio e mantém os cantos); as barras e os botões claros são em
tons de cinza, para o jogo tingir com a cor de cada coisa (vida vermelha, mana azul, XP dourado...).

  H_PanelBlue, H_PanelNavy, H_PanelRed, H_PanelGold   painéis (64x64, canto 24)
  H_BarBack, H_BarFill                                 barra: fundo escuro e enchimento cinza para tingir (64x32)
  H_Slot, H_SlotLocked                                 botão de borracha das habilidades (96x96, canto 26)
  H_Round                                              botão redondo cinza (poções, + e -) para tingir (96x96)
  H_Disc                                               selo redondo com contorno (nível, contagem) para tingir (64x64)
  H_Ring                                               aro do retrato (128x128, miolo vazio)
  H_MapRing                                            aro do minimapa (512x512, miolo vazio)
  H_Key                                                tecla (pílula escura, 48x32)
  M_Quest, M_Npc, M_Enemy, M_Chest, M_Portal, M_Flag, M_Town, M_Poi, M_Boss, M_Party, M_Arrow   marcas do mapa (64x64)
    python Tools/MakeHudCartoon.py      (depois: Tools/ImportHudCartoon.py com o editor fechado)
"""
import math
import os
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "UI", "HudCartoon")
os.makedirs(OUT, exist_ok=True)
SS = 3
INK = (20, 20, 50)
BLUE = (62, 112, 214)
NAVY = (26, 36, 84)
RED = (178, 48, 58)
GOLD = (255, 198, 46)
GRAY = (226, 226, 226)
CREAM = (255, 248, 230)


def shade(c, k):
    return tuple(max(0, min(255, int(v * k))) for v in c[:3])


def save(big, name):
    w, h = big.width // SS, big.height // SS
    big.resize((w, h), Image.LANCZOS).save(os.path.join(OUT, name + ".png"))


def panel(w, h, r, fill, ow=5, gloss=True, bottom=True, outline=INK):
    W, H, R, O = w * SS, h * SS, r * SS, ow * SS
    big = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    d.rounded_rectangle((0, 0, W - 1, H - 1), R, fill=outline + (255,))
    inner = (O, O, W - 1 - O, H - 1 - O)
    mask = Image.new("L", (W, H), 0)
    ImageDraw.Draw(mask).rounded_rectangle(inner, max(1, R - O), fill=255)
    body = Image.new("RGBA", (W, H), fill + (255,))
    bd = ImageDraw.Draw(body)
    if bottom:
        lip = int(min(h * .16, 8) * SS)
        bd.rectangle((0, H - O - lip, W, H), fill=shade(fill, .74) + (255,))
    if gloss:
        top = int(h * .42 * SS)
        bd.rounded_rectangle((O + 3 * SS, O + 2 * SS, W - O - 3 * SS, O + top), int(top * .5), fill=shade(fill, 1.16) + (255,))
    big.paste(body, (0, 0), mask)
    return big


def circle(size, fill, ow=5, gloss=True, hole=0, outline=INK):
    S = size * SS
    big = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    d.ellipse((0, 0, S - 1, S - 1), fill=outline + (255,))
    o = ow * SS
    d.ellipse((o, o, S - 1 - o, S - 1 - o), fill=fill + (255,))
    if gloss:
        d.ellipse((o + 3 * SS, o + 2 * SS, S - o - 3 * SS, o + int(S * .42)), fill=shade(fill, 1.14) + (255,))
    if hole:
        h = hole * SS
        cut = Image.new("L", (S, S), 255)
        ImageDraw.Draw(cut).ellipse((h, h, S - 1 - h, S - 1 - h), fill=0)
        big.putalpha(Image.composite(big.split()[3], Image.new("L", (S, S), 0), cut))
        # o contorno de dentro (o aro tem borda dos dois lados)
        ImageDraw.Draw(big).ellipse((h - 3 * SS, h - 3 * SS, S - 1 - h + 3 * SS, S - 1 - h + 3 * SS), outline=outline + (255,), width=3 * SS)
    return big


# ---- painéis, barras e botões ----------------------------------------------------------------------------------------
save(panel(64, 64, 24, BLUE), "H_PanelBlue")
save(panel(64, 64, 24, NAVY, ow=4, gloss=False, bottom=False), "H_PanelNavy")
save(panel(64, 64, 24, RED), "H_PanelRed")
save(panel(64, 64, 24, GOLD, ow=5), "H_PanelGold")
save(panel(64, 32, 16, NAVY, ow=4, gloss=False, bottom=False), "H_BarBack")
save(panel(64, 32, 16, GRAY, ow=2, outline=(70, 70, 70)), "H_BarFill")
save(panel(96, 96, 26, (150, 194, 255), ow=5), "H_Slot")
save(panel(96, 96, 26, (90, 104, 150), ow=5), "H_SlotLocked")
save(circle(96, GRAY, ow=5), "H_Round")
save(circle(64, (240, 240, 240), ow=5, gloss=False), "H_Disc")
save(circle(128, BLUE, ow=6, hole=14, gloss=False), "H_Ring")
save(circle(512, BLUE, ow=10, hole=30, gloss=False), "H_MapRing")
save(panel(48, 32, 12, INK, ow=2, gloss=False, bottom=False, outline=INK), "H_Key")


# ---- marcas do mapa ---------------------------------------------------------------------------------------------------
def mark(name, draw):
    S = 64 * SS
    big = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    draw(ImageDraw.Draw(big), S)
    save(big, name)


def k(v):
    return int(v * SS)


def quest(d, S):
    d.ellipse((k(8), k(8), k(56), k(56)), fill=INK)
    d.ellipse((k(13), k(13), k(51), k(51)), fill=GOLD)
    d.rounded_rectangle((k(28), k(18), k(36), k(38)), k(3), fill=INK)
    d.ellipse((k(28), k(41), k(36), k(49)), fill=INK)


def npc(d, S):
    d.ellipse((k(16), k(16), k(48), k(48)), fill=INK)
    d.ellipse((k(21), k(21), k(43), k(43)), fill=CREAM)


def enemy(d, S):
    d.ellipse((k(14), k(14), k(50), k(50)), fill=INK)
    d.ellipse((k(19), k(19), k(45), k(45)), fill=(232, 64, 64))
    d.ellipse((k(24), k(22), k(34), k(30)), fill=(255, 150, 150))


def boss(d, S):
    d.ellipse((k(4), k(4), k(60), k(60)), fill=INK)
    d.ellipse((k(10), k(10), k(54), k(54)), fill=(170, 70, 230))
    d.polygon([(k(20), k(40)), (k(20), k(24)), (k(26), k(32)), (k(32), k(20)), (k(38), k(32)), (k(44), k(24)), (k(44), k(40))], fill=GOLD, outline=INK)


def party(d, S):
    d.ellipse((k(14), k(14), k(50), k(50)), fill=INK)
    d.ellipse((k(19), k(19), k(45), k(45)), fill=(80, 160, 255))


def chest(d, S):
    d.rounded_rectangle((k(10), k(18), k(54), k(50)), k(7), fill=INK)
    d.rounded_rectangle((k(15), k(23), k(49), k(45)), k(4), fill=(186, 112, 50))
    d.rectangle((k(15), k(31), k(49), k(36)), fill=GOLD)
    d.rectangle((k(29), k(28), k(35), k(40)), fill=INK)


def portal(d, S):
    d.ellipse((k(14), k(6), k(50), k(58)), fill=INK)
    d.ellipse((k(19), k(11), k(45), k(53)), fill=(120, 200, 255))
    d.ellipse((k(26), k(20), k(38), k(44)), fill=(225, 246, 255))


def flag(d, S):
    d.rectangle((k(18), k(8), k(24), k(54)), fill=INK)
    d.polygon([(k(24), k(8)), (k(56), k(20)), (k(24), k(32))], fill=INK)
    d.polygon([(k(26), k(12)), (k(50), k(20)), (k(26), k(28))], fill=(255, 110, 200))
    d.ellipse((k(10), k(50), k(32), k(60)), fill=INK)


def town(d, S):
    d.ellipse((k(4), k(4), k(60), k(60)), fill=INK)
    d.ellipse((k(9), k(9), k(55), k(55)), fill=GOLD)
    pts = []
    for i in range(10):
        a = -math.pi / 2 + i * math.pi / 5
        r = 18 if i % 2 == 0 else 8
        pts.append((k(32 + math.cos(a) * r), k(33 + math.sin(a) * r)))
    d.polygon(pts, fill=CREAM, outline=INK)


def poi(d, S):
    d.polygon([(k(32), k(8)), (k(56), k(32)), (k(32), k(56)), (k(8), k(32))], fill=INK)
    d.polygon([(k(32), k(15)), (k(49), k(32)), (k(32), k(49)), (k(15), k(32))], fill=(236, 226, 196))


def arrow(d, S):
    # a seta do herói, apontando para cima (o jogo gira)
    pts = [(k(32), k(4)), (k(56), k(56)), (k(32), k(44)), (k(8), k(56))]
    d.polygon(pts, fill=INK)
    inner = [(k(32), k(14)), (k(48), k(50)), (k(32), k(40)), (k(16), k(50))]
    d.polygon(inner, fill=GOLD)
    d.polygon([(k(32), k(14)), (k(32), k(40)), (k(16), k(50))], fill=(255, 226, 120))


for name, fn in (("M_Quest", quest), ("M_Npc", npc), ("M_Enemy", enemy), ("M_Boss", boss), ("M_Party", party), ("M_Chest", chest),
                 ("M_Portal", portal), ("M_Flag", flag), ("M_Town", town), ("M_Poi", poi), ("M_Arrow", arrow)):
    mark(name, fn)

# folha de prévia
files = sorted(f for f in os.listdir(OUT) if f.endswith(".png") and f != "preview.png")
sheet = Image.new("RGBA", (1100, 420), (60, 80, 70, 255))
x = y = 10
for f in files:
    im = Image.open(os.path.join(OUT, f))
    im.thumbnail((120, 120))
    if x + im.width > 1090:
        x, y = 10, y + 135
    sheet.alpha_composite(im, (x, y))
    x += im.width + 12
sheet.save(os.path.join(OUT, "preview.png"))
print("VORTEX_HUD_CARTOON", len(files), "peças")
