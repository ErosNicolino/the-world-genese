"""Os ícones dos lugares da vila no minimapa e no mapa (11/11, o usuário: "fazer ícones para elas no minimapa e mapa").

O símbolo de cada placa (Tools/Town/MakeSigns.py: bigorna, caneca, pão, tesoura...) num selo redondo no estilo das outras
marcas do mapa (Tools/MakeHudCartoon.py: contorno escuro grosso, miolo colorido). Uma cor por tipo de lugar.

python Tools/Town/MakeShopIcons.py   ->   Data/UI/HudCartoon/M_Loja_<SLUG>.png (64 x 64)
Depois: Tools/ImportHudCartoon.py (editor fechado) leva para /Game/Vortex/UI/HudCartoon/T_M_Loja_<SLUG>.
O AVortexAimHUD desenha um por construção com nome da vila.json (as que têm morador).
"""
import os
import sys

from PIL import Image, ImageChops, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import MakeSigns  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "Data", "UI", "HudCartoon")
SS = 4
INK = (20, 20, 50)
CREAM = (255, 248, 230)
# a cor do selo: comércio dourado, ofício laranja, cidade azul, a capela clara
COLOR = {"coroa": (80, 130, 230), "escudo": (80, 130, 230), "sino": (200, 210, 235), "pergaminho": (255, 198, 46),
         "caneca": (232, 140, 60), "bigorna": (232, 110, 60), "serrote": (232, 140, 60), "ferradura": (190, 140, 90)}
SHOP = (255, 198, 46)


def icon(label):
    kind = MakeSigns.SIGNS[label][0]
    S = 64 * SS
    big = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    d.ellipse((2 * SS, 2 * SS, 62 * SS, 62 * SS), fill=INK)
    d.ellipse((7 * SS, 7 * SS, 57 * SS, 57 * SS), fill=COLOR.get(kind, SHOP))
    # o símbolo da placa em creme com o contorno escuro (só a silhueta: os sulcos cinza somem nesse tamanho)
    m = MakeSigns.shape_mask(kind, 38 * SS).point(lambda v: 255 if v > 40 else 0)
    edge = m.copy()
    for _ in range(2 * SS):
        edge = ImageChops.lighter(edge, edge.transform(edge.size, Image.AFFINE, (1, 0, 1, 0, 1, 0)))
        edge = ImageChops.lighter(edge, edge.transform(edge.size, Image.AFFINE, (1, 0, -1, 0, 1, 0)))
        edge = ImageChops.lighter(edge, edge.transform(edge.size, Image.AFFINE, (1, 0, 0, 0, 1, 1)))
        edge = ImageChops.lighter(edge, edge.transform(edge.size, Image.AFFINE, (1, 0, 0, 0, 1, -1)))
    at = (13 * SS, 13 * SS)
    big.paste(Image.new("RGBA", m.size, INK + (255,)), at, edge)
    big.paste(Image.new("RGBA", m.size, CREAM + (255,)), at, m)
    return big.resize((64, 64), Image.LANCZOS)


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    sheet = Image.new("RGBA", (len(MakeSigns.SIGNS) * 72 + 8, 80), (60, 80, 70, 255))
    for n, label in enumerate(MakeSigns.SIGNS):
        im = icon(label)
        assert im.getbbox(), label
        im.save(os.path.join(OUT, "M_Loja_%s.png" % MakeSigns.slug(label)))
        sheet.alpha_composite(im, (8 + n * 72, 8))
    sheet.save(os.path.join(ROOT, "Docs", "Images", "icones_lojas.png"))
    print("ICONES %d lugares -> %s" % (len(MakeSigns.SIGNS), OUT))
