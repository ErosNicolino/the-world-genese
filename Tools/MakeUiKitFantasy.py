"""As janelas (herói, mochila, técnicas, classe, alquimia, missões, loja, opções) no estilo das imagens das habilidades
(07/11, o usuário: "novos estilos de HUD e menus, como o inventário, que combinem com as imagens das skills e com a cara
do jogo"). Mesmo desenho do Tools/MakeHudFantasy.py: azul-noite com textura, moldura de ouro em relevo, rebites; os
botões são placas de metal escovado com aro de ouro. Mesmos nomes do kit cartoon (W_Frame, W_Card, W_Button...), na
mesma pasta (Data/UI/KitCartoon): o código não muda. As peças antigas ficaram em Data/UI/KitCartoon_antigo.
    python Tools/MakeUiKitFantasy.py      (depois: Tools/ImportUiKitCartoon.py com o editor fechado)
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from MakeHudFantasy import GOLD, GOLD_HI, GOLD_LO, INK, NIGHT, SS, disc, frame, lerp, mask_round, vgrad   # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "UI", "KitCartoon")


def save(big, name):
    big.resize((big.width // SS, big.height // SS), Image.LANCZOS).save(os.path.join(OUT, name + ".png"))


def window():
    """A janela (160x160, margem 30%): a moldura larga com o filete duplo, os cantos com o florão e o título claro."""
    big = frame(160, 160, 6, band=(2.0, 11.0), body=(20, 24, 42), seed=21, studs=False)
    W = 160 * SS
    d = ImageDraw.Draw(big)
    # o filete fino por dentro da moldura
    k = int(15 * SS)
    d.rounded_rectangle((k, k, W - 1 - k, W - 1 - k), 3 * SS, outline=GOLD_LO + (255,), width=int(1.2 * SS))
    # os florões dos cantos: um losango de ouro com a safira
    for cx in (6.5 * SS, W - 1 - 6.5 * SS):
        for cy in (6.5 * SS, W - 1 - 6.5 * SS):
            r = 9 * SS
            d.polygon([(cx, cy - r), (cx + r, cy), (cx, cy + r), (cx - r, cy)], fill=INK)
            r2 = 7 * SS
            d.polygon([(cx, cy - r2), (cx + r2, cy), (cx, cy + r2), (cx - r2, cy)], fill=GOLD)
            r3 = 3.4 * SS
            d.ellipse((cx - r3, cy - r3, cx + r3, cy + r3), fill=(40, 90, 190))
            d.ellipse((cx - r3 * .5, cy - r3 * .7, cx + r3 * .1, cy - r3 * .1), fill=(160, 210, 255))
    # um brilho de luz no alto do miolo (dentro das margens: não estica)
    glow = Image.new("L", (W, W), 0)
    ImageDraw.Draw(glow).rectangle((k, k, W - k, int(40 * SS)), fill=26)
    glow = glow.filter(ImageFilter.GaussianBlur(6 * SS))
    big.alpha_composite(Image.merge("RGBA", (Image.new("L", (W, W), 120), Image.new("L", (W, W), 150), Image.new("L", (W, W), 230), glow)))
    return big


def button(face, w=96, h=52, rim=(1.4, 4.6), seed=30):
    """A placa do botão: metal com aro de ouro, a luz em cima e a sombra embaixo (dá para apertar)."""
    return frame(w, h, 5, band=rim, body=face, body_lo=lerp(face, INK, .5), studs=False, seed=seed)


def ribbon(w=96, h=44):
    """A fita do título (branca, tingida no jogo): as pontas em rabo de andorinha dentro das margens (25%)."""
    W, H = w * SS, h * SS
    big = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    n = int(10 * SS)
    outline = [(0, 0), (W, 0), (W - n, H / 2), (W, H), (0, H), (n, H / 2)]
    d.polygon(outline, fill=INK)
    g = vgrad(W, H, (255, 255, 255), (170, 170, 170))
    m = Image.new("L", (W, H), 0)
    o = 2.4 * SS
    ImageDraw.Draw(m).polygon([(o * 1.6, o), (W - o * 1.6, o), (W - n - o * .4, H / 2), (W - o * 1.6, H - o), (o * 1.6, H - o), (n + o * .4, H / 2)], fill=255)
    big.paste(g, (0, 0), m)
    d.line([(o * 3, o * 2.2), (W - o * 3, o * 2.2)], fill=(255, 255, 255, 160), width=int(1.2 * SS))
    d.line([(o * 3, H - o * 2.2), (W - o * 3, H - o * 2.2)], fill=(90, 90, 90, 200), width=int(1.2 * SS))
    return big


def parchment(w=128, h=128, seed=50):
    """07/11: a folha de pergaminho (as páginas do diário de missões, a fala do vendedor): papel envelhecido com manchas,
    a beira queimada mais escura, um filete marrom por dentro e a borda de ouro fina por fora."""
    import random
    W, H = w * SS, h * SS
    rnd = random.Random(seed)
    paper = vgrad(W, H, (240, 222, 180), (214, 190, 140))
    d = ImageDraw.Draw(paper)
    for _ in range(260):
        x, y, r = rnd.randint(0, W), rnd.randint(0, H), rnd.randint(4, 22) * SS
        c = rnd.choice(((226, 204, 156), (232, 214, 170), (206, 178, 128), (244, 230, 196)))
        d.ellipse((x - r, y - r, x + r, y + r), fill=c)
    paper = paper.filter(ImageFilter.GaussianBlur(5 * SS))
    burn = Image.new("L", (W, H), 0)
    ImageDraw.Draw(burn).rounded_rectangle((0, 0, W - 1, H - 1), 6 * SS, outline=255, width=int(12 * SS))
    burn = burn.filter(ImageFilter.GaussianBlur(7 * SS))
    paper = Image.composite(Image.new("RGB", (W, H), (120, 82, 42)), paper, burn.point(lambda v: v * 3 // 4))
    big = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    big.paste(Image.new("RGB", (W, H), INK), (0, 0), mask_round(W, H, 6 * SS))
    big.paste(Image.new("RGB", (W, H), GOLD_LO), (0, 0), mask_round(W, H, 6 * SS, int(1.0 * SS)))
    big.paste(Image.new("RGB", (W, H), GOLD), (0, 0), mask_round(W, H, 6 * SS, int(1.8 * SS)))
    big.paste(paper, (0, 0), mask_round(W, H, 5 * SS, int(3.2 * SS)))
    dd = ImageDraw.Draw(big)
    k = int(9 * SS)
    dd.rounded_rectangle((k, k, W - 1 - k, H - 1 - k), 3 * SS, outline=(150, 108, 60, 200), width=int(1.2 * SS))
    return big


def ring_only(w, h, color, inner_alpha=0, width=4.0):
    W, H = w * SS, h * SS
    big = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    d.rounded_rectangle((0, 0, W - 1, H - 1), 5 * SS, fill=INK)
    d.rounded_rectangle((SS, SS, W - 1 - SS, H - 1 - SS), 4 * SS, fill=color + (255,))
    k = int(width * SS)
    d.rounded_rectangle((k, k, W - 1 - k, H - 1 - k), 3 * SS, fill=INK)
    d.rounded_rectangle((k + SS, k + SS, W - 1 - k - SS, H - 1 - k - SS), 2 * SS, fill=(0, 0, 0, inner_alpha))
    return big


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    save(window(), "W_Frame")
    Image.new("RGBA", (8, 8), (0, 0, 0, 0)).save(os.path.join(OUT, "W_StoneTile.png"))
    save(frame(96, 96, 5, band=(1.4, 4.6), body=(16, 20, 36), seed=22, studs=False), "W_Panel")
    save(frame(96, 96, 5, band=(1.4, 5.2), body=(34, 44, 78), seed=23), "W_Card")
    save(frame(96, 96, 5, band=(1.4, 5.2), body=(30, 32, 40), seed=24), "W_CardLocked")
    # os espaços de item: afundados como o encaixe das habilidades
    save(frame(64, 64, 4, band=(1.2, 4.2), body=(14, 18, 32), seed=25, studs=False), "W_Slot")
    save(frame(64, 64, 4, band=(1.2, 4.6), body=(32, 42, 76), seed=26, studs=False), "W_SlotHover")
    save(ring_only(64, 64, GOLD_HI, width=4.2), "W_SlotActive")
    # o brilho da raridade por dentro (branco, tingido no jogo)
    W = 64 * SS
    glow = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    gd = ImageDraw.Draw(glow)
    for i, a in enumerate((230, 150, 80, 30)):
        gd.rounded_rectangle(((5 + i * 2) * SS, (5 + i * 2) * SS, W - (5 + i * 2) * SS, W - (5 + i * 2) * SS), 3 * SS, outline=(255, 255, 255, a), width=2 * SS)
    save(glow, "W_SlotGlow")
    # botões: azul-aço (comum), ouro (principal), vermelho (fechar), verde (+), apagado
    save(button((48, 70, 120), seed=31), "W_Button")
    save(button((70, 98, 160), seed=32), "W_ButtonHover")
    save(button((36, 52, 92), seed=33), "W_ButtonPressed")
    save(button((200, 150, 64), seed=34), "W_ButtonGold")
    save(button((228, 180, 90), seed=35), "W_ButtonGoldHover")
    save(button((140, 34, 40), seed=36), "W_ButtonRed")
    save(button((176, 52, 56), seed=37), "W_ButtonRedHover")
    save(button((48, 110, 52), seed=38), "W_ButtonGreen")
    save(button((70, 140, 70), seed=39), "W_ButtonGreenHover")
    save(button((60, 62, 72), seed=40), "W_ButtonDisabled")
    save(frame(64, 40, 4, band=(1.0, 3.0), body=(28, 36, 64), seed=41, studs=False), "W_Row")
    save(frame(64, 40, 4, band=(1.0, 3.4), body=(46, 62, 108), seed=42, studs=False), "W_RowHover")
    save(frame(96, 96, 5, band=(1.4, 5.0), body=(32, 40, 70), seed=43, studs=False), "W_Tab")
    save(frame(96, 96, 5, band=(1.4, 5.0), body=(196, 146, 62), body_lo=(130, 88, 34), seed=44), "W_TabActive")
    save(ribbon(), "W_Ribbon")
    save(parchment(), "W_Parchment")
    save(frame(64, 56, 4, band=(1.2, 4.0), body=(20, 24, 42), seed=45, studs=False), "W_Key")
    save(frame(64, 24, 3, band=(1.0, 3.0), body=(10, 12, 22), seed=46, studs=False, gloss=False), "W_BarBack")
    Wb, Hb = 64 * SS, 24 * SS
    fill = vgrad(Wb, Hb, (255, 255, 255), (150, 150, 150))
    ImageDraw.Draw(fill).rectangle((0, int(Hb * .14), Wb, int(Hb * .38)), fill=(255, 255, 255))
    fill = fill.filter(ImageFilter.GaussianBlur(1.2 * SS))
    bar = Image.new("RGBA", (Wb, Hb), (0, 0, 0, 0))
    bar.paste(fill, (0, 0), mask_round(Wb, Hb, 2 * SS, int(.5 * SS)))
    save(bar, "W_BarFill")
    # o pedestal da vitrine do herói: o disco de pedra escura com o aro de ouro
    W2, H2 = 192 * SS, 72 * SS
    st = Image.new("RGBA", (W2, H2), (0, 0, 0, 0))
    sd = ImageDraw.Draw(st)
    sd.ellipse((2 * SS, 12 * SS, 190 * SS, 70 * SS), fill=(0, 0, 0, 90))
    sd.ellipse((2 * SS, 4 * SS, 190 * SS, 62 * SS), fill=INK)
    sd.ellipse((6 * SS, 7 * SS, 186 * SS, 59 * SS), fill=GOLD_LO)
    sd.ellipse((8 * SS, 8 * SS, 184 * SS, 56 * SS), fill=GOLD)
    sd.ellipse((14 * SS, 11 * SS, 178 * SS, 52 * SS), fill=(26, 32, 56))
    sd.ellipse((24 * SS, 14 * SS, 168 * SS, 44 * SS), fill=(40, 50, 84))
    save(st, "W_Stage")
    print("UI fantasia em", OUT)
