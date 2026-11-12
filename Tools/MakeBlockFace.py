"""O rosto animado do humano de blocos (o rosto do modelo é liso): olhos e sobrancelhas desenhados do zero, no estilo da
referência de 01/11 (olhos grandes e escuros com um brilho comprido, sem boca), na grade dos pixels da cabeça (meio pixel
da textura), sobre fundo transparente.

python Tools/MakeBlockFace.py [raça] [--feminino]   (padrão: humano; antes, Tools/Blender/BakeHeroFace.py <raiz> <raça> --force)
(--feminino: os mesmos olhos com cílios para fora, em Face/eyes_f.png e preview_f.png; a boca e os retângulos são os
mesmos)

A frente da cabeça (Face/head.png) é dividida numa grade de 16 x 16 (cada casa = meio pixel da textura da cabeça). Saem
Face/eyes.png e Face/mouth.png (4 x 2 quadros, na ordem de Tools/MakeHeroFaces.py) e os retângulos em Face/face.json;
Tools/ImportHeroFace.py e o AVortexCharacter::TickFace usam igual aos outros heróis. Conferência em Face/preview.png.
"""
import json
import os
import sys

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FEMALE = "--feminino" in sys.argv
STYLE_ID = sys.argv[sys.argv.index("--estilo") + 1] if "--estilo" in sys.argv else "padrao"
ARGS = [a for i, a in enumerate(sys.argv[1:], 1) if not a.startswith("--") and sys.argv[i - 1] != "--estilo"]
RACE = ARGS[0] if ARGS else "humano"
FOLDER = os.path.join(ROOT, "Data", "Heroes", RACE + "_Mixamo", "Face")
EYE_FRAMES = ["aberto", "meio", "fechado", "esquerda", "direita", "bravo", "feliz", "dor"]
MOUTH_FRAMES = ["neutra", "A", "O", "E", "sorriso", "dentes", "M", "grito"]

LASH = (34, 20, 20, 255)        # o contorno de cima do olho e os olhos fechados
BROW = (98, 54, 42, 255)        # sobrancelhas
IRIS = (44, 26, 24, 255)        # o olho escuro
IRIS_LOW = (92, 54, 44, 255)    # a parte de baixo do olho, mais clara
LID = (150, 98, 78, 255)        # a sombra no canto de baixo, do lado de fora
SHINE_HI = (204, 176, 162, 255) # o brilho comprido, mais claro embaixo
SHINE_LO = (244, 226, 212, 255)
BLUSH = (236, 128, 116, 150)    # as bochechas coradas (feliz)

head = Image.open(os.path.join(FOLDER, "head.png")).convert("RGBA")
W, H = head.size
xs = [x for x in range(W) if any(head.getpixel((x, y))[3] > 200 for y in range(H // 3, H // 2))]
ys = [y for y in range(H) if head.getpixel((W // 2, y))[3] > 200]
# a frente da cabeça: a parte de cima (o pescoço embaixo é mais estreito)
fx0, fx1 = xs[0], xs[-1] + 1
fy0 = ys[0]
side = fx1 - fx0
fy1 = fy0 + side
U = side / 16.0                                  # uma casa da grade, em pixels do head.png

# as regiões (em casas): os olhos com as sobrancelhas e as bochechas; a boca fica vazia (o rosto não tem boca)
EYE_BOX = (0, 5, 16, 13)                         # x0, y0, x1, y1
MOUTH_BOX = (5, 13, 11, 15)


def blank(box):
    x0, y0, x1, y1 = box
    return Image.new("RGBA", (round((x1 - x0) * U), round((y1 - y0) * U)), (0, 0, 0, 0))


def cell(img, box, cx, cy, color, w=1, h=1):
    """Pinta casas (na grade da cabeça) dentro da região."""
    x0, y0 = box[0], box[1]
    for dx in range(w):
        for dy in range(h):
            px0, py0 = round((cx + dx - x0) * U), round((cy + dy - y0) * U)
            px1, py1 = round((cx + dx + 1 - x0) * U), round((cy + dy + 1 - y0) * U)
            for x in range(px0, min(px1, img.width)):
                for y in range(py0, min(py1, img.height)):
                    img.putpixel((x, y), color)


# os estilos de rosto (01/11, as predefinições do menu, Data/Looks/rostos.json): saem em eyes_<estilo>.png (o padrão
# continua em eyes.png). top = a linha de cima do olho aberto; brow = a linha da sobrancelha em cada coluna (0 = fora,
# 3 = perto do nariz); shine = as casas do brilho (coluna, linha a partir do topo); dots = olhos de pontinho (2 x 2);
# freckles = sardas nas bochechas
STYLES = {
    "padrao": {},
    "azul": {"iris": (34, 58, 118, 255), "iris_low": (74, 112, 178, 255)},
    "verde": {"iris": (30, 74, 40, 255), "iris_low": (70, 130, 74, 255)},
    "sereno": {"top": 10, "brow": (7, 7, 7, 7)},
    "determinado": {"top": 9, "brow": (5, 5, 6, 6), "iris_low": (44, 26, 24, 255)},
    "fofo": {"shine": [(1, 1), (2, 3)]},   # 02/11: os olhos do tamanho do padrão (maiores ficava estranho), com dois brilhos
    "pontos": {"dots": True, "brow": (8, 8, 8, 8)},
    "sardas": {"freckles": [(-1, 12), (4, 11), (-1, 10), (5, 12)]},
    # 09/11: os olhos do elfo (claro e dark elfo; a sobrancelha do dark elfo é prateada, para aparecer na pele escura)
    "elfo_verde": {"iris": (34, 104, 62, 255), "iris_low": (112, 196, 104, 255), "brow_color": (150, 116, 70, 255)},
    "elfo_dourado": {"iris": (146, 96, 20, 255), "iris_low": (232, 180, 60, 255), "brow_color": (150, 116, 70, 255)},
    "elfo_gelo": {"iris": (62, 124, 168, 255), "iris_low": (160, 222, 242, 255), "brow_color": (150, 116, 70, 255)},
    "dark_vermelho": {"iris": (138, 18, 30, 255), "iris_low": (232, 60, 60, 255), "brow_color": (196, 196, 214, 255)},
    "dark_violeta": {"iris": (88, 38, 160, 255), "iris_low": (182, 112, 242, 255), "brow_color": (196, 196, 214, 255)},
    "dark_prata": {"iris": (118, 124, 146, 255), "iris_low": (216, 222, 236, 255), "brow_color": (196, 196, 214, 255)},
}
STYLE = STYLES[STYLE_ID]
BROW = STYLE.get("brow_color", BROW)
if "iris" in STYLE:
    IRIS = STYLE["iris"]
if "iris_low" in STYLE:
    IRIS_LOW = STYLE["iris_low"]
FRECKLE = (176, 104, 80, 255)

# como na referência (01/11): olhos grandes de 4 x 5 casas, escuros, com um brilho comprido do lado de fora, e
# sobrancelhas finas por cima da parte de dentro; entre os olhos, 4 casas (o vão de um nariz, sem nariz)
EYES = [(2, 1), (13, -1)]                        # a coluna do lado de fora e o passo para dentro


def eye_frame(name):
    img = blank(EYE_BOX)
    look = {"esquerda": -1, "direita": 1}.get(name, 0)
    for start, step in EYES:
        def put(o, row, color, start=start, step=step):
            """o: 0 = a coluna de fora do olho, 3 = a de dentro (perto do nariz); o olhar desloca o olho inteiro."""
            cell(img, EYE_BOX, start + step * o + look, row, color)
        for (o, row) in STYLE.get("freckles", []):
            put(o, row, FRECKLE)
        if STYLE.get("dots"):
            # olhos de pontinho: 2 x 2 no meio do lugar do olho, a sobrancelha curta
            for o in (1, 2):
                put(o, 6 if name == "bravo" and o == 2 else 7 if name == "bravo" else STYLE["brow"][o], BROW)
            if name in ("aberto", "esquerda", "direita", "bravo"):
                for o in (1, 2):
                    put(o, 10, IRIS)
                    put(o, 11, IRIS)
                put(1, 10, SHINE_LO)
            elif name == "meio":
                for o in (1, 2):
                    put(o, 11, IRIS)
            elif name in ("fechado", "dor"):
                for o in (1, 2):
                    put(o, 11, LASH)
            elif name == "feliz":
                put(1, 10, LASH)
                put(2, 10, LASH)
                put(0, 11, LASH)
                put(3, 11, LASH)
                put(0, 12, BLUSH)
                put(1, 12, BLUSH)
            if FEMALE and name not in ("fechado", "dor", "feliz"):
                put(0, 9, LASH)
            continue
        # sobrancelha
        if name == "bravo":
            # descendo para o nariz
            put(0, 5, BROW)
            put(1, 5, BROW)
            put(2, 6, BROW)
            put(3, 6, BROW)
        elif name == "dor":
            put(1, 6, BROW)
            put(2, 5, BROW)
            put(3, 5, BROW)
        else:
            brow = STYLE.get("brow")
            for o in ((0, 1, 2, 3) if brow else (1, 2, 3)):
                put(o, brow[o] if brow else 6, BROW)
        if name in ("aberto", "esquerda", "direita", "meio", "bravo"):
            base = STYLE.get("top", 8)
            top = {"meio": min(11, base + 2), "bravo": min(11, base + 1)}.get(name, base)
            for o in range(4):
                put(o, top, LASH)                          # o contorno de cima
            for row in range(top + 1, 13):
                low = row >= 11
                if row < 12:
                    put(0, row, LID if low else IRIS)
                # o brilho: a coluna 1 inteira (o padrão) ou as casas do estilo
                shines = STYLE.get("shine")
                put(1, row, (SHINE_LO if low else SHINE_HI) if not shines or (1, row - top) in shines else (IRIS_LOW if low else IRIS))
                put(2, row, SHINE_LO if shines and (2, row - top) in shines else IRIS_LOW if low else IRIS)
                put(3, row, IRIS_LOW if low else IRIS)
            if name == "bravo":
                put(3, top + 1, LASH)                      # a pálpebra desce do lado do nariz
            if FEMALE:
                put(-1, top, LASH)                         # os cílios: uma ponta para fora e para cima
                put(-1, top - 1, LASH)
        elif name == "fechado":
            for o in range(4):
                put(o, 11, LASH)
            if FEMALE:
                put(-1, 12, LASH)
        elif name == "feliz":
            # ∩ fechado, com as bochechas coradas embaixo
            for o in range(4):
                put(o, 10, LASH)
            put(0, 11, LASH)
            put(3, 11, LASH)
            if FEMALE:
                put(-1, 10, LASH)
            put(0, 12, BLUSH)
            put(1, 12, BLUSH)
        elif name == "dor":
            # > <: a ponta para o nariz
            put(0, 9, LASH)
            put(1, 9, LASH)
            put(2, 10, LASH)
            put(3, 10, LASH)
            put(0, 11, LASH)
            put(1, 11, LASH)
    return img


def mouth_frame(name):
    # sem boca (o estilo da referência): os 8 quadros vazios, para o TickFace continuar igual
    return blank(MOUTH_BOX)


def sheet(frames):
    w, h = frames[0].size
    out = Image.new("RGBA", (w * 4, h * 2), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        out.paste(f, ((i % 4) * w, (i // 4) * h))
    return out


eyes = [eye_frame(n) for n in EYE_FRAMES]
mouths = [mouth_frame(n) for n in MOUTH_FRAMES]
SUFFIX = "" if STYLE_ID == "padrao" else "_" + STYLE_ID
sheet(eyes).save(os.path.join(FOLDER, ("eyes_f%s.png" if FEMALE else "eyes%s.png") % SUFFIX))
if not FEMALE and not SUFFIX:
    sheet(mouths).save(os.path.join(FOLDER, "mouth.png"))


def region(box):
    x0, y0, x1, y1 = box
    return (fx0 + x0 * U, fy0 + y0 * U, fx0 + x0 * U + round((x1 - x0) * U), fy0 + y0 * U + round((y1 - y0) * U))


def uvbox(r):
    return [r[0] / W, 1 - r[3] / H, (r[2] - r[0]) / W, (r[3] - r[1]) / H]


# as regiões do rosto (só no padrão: os estilos usam as mesmas)
if not SUFFIX:
    with open(os.path.join(FOLDER, "face.json"), encoding="utf-8") as f:
        meta = json.load(f)
    meta.update({"eyes": uvbox(region(EYE_BOX)), "mouth": uvbox(region(MOUTH_BOX)), "grid": [4, 2], "flat": [],
                 "eye_frames": EYE_FRAMES, "mouth_frames": MOUTH_FRAMES})
    with open(os.path.join(FOLDER, "face.json"), "w", encoding="utf-8") as f:
        json.dump(meta, f, indent=1)

tiles = []
for i in range(8):
    im = head.copy()
    er, mr = region(EYE_BOX), region(MOUTH_BOX)
    im.alpha_composite(eyes[i], (round(er[0]), round(er[1])))
    im.alpha_composite(mouths[i], (round(mr[0]), round(mr[1])))
    bg = Image.new("RGBA", im.size, (40, 40, 40, 255))
    bg.alpha_composite(im)
    tiles.append(bg.resize((256, 256), Image.NEAREST))
preview = Image.new("RGBA", (1024, 512))
for i, t in enumerate(tiles):
    preview.paste(t, ((i % 4) * 256, (i // 4) * 256))
preview.save(os.path.join(FOLDER, ("preview_f%s.png" if FEMALE else "preview%s.png") % SUFFIX))
print("%s: frente da cabeça %d,%d lado %d px, casa %.1f px" % (RACE, fx0, fy0, side, U))
