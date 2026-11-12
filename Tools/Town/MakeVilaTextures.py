"""As texturas da vila nova (05/11): as referências do usuário (modelando ainda/Futuras modelagens/Texturas/Materiais da
cidade, todas emendam quando repetidas) em 512 px, mais as que faltavam, desenhadas aqui (vidro, folhagem, brasa).

python Tools/Town/MakeVilaTextures.py   ->   Data/GodotTown/VilaTex/T_Vila_<nome>.png

Tools/Blender/BuildVilaKit.py usa os mesmos nomes (os materiais MV_<nome>) e o tamanho que cada uma cobre no mundo;
Tools/ImportVilaKit.py leva para a Unreal.
"""
import os
import random

from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "modelando ainda", "Futuras modelagens", "Texturas", "Materiais da cidade")
OUT = os.path.join(ROOT, "Data", "GodotTown", "VilaTex")
SIZE = 512

FROM_FILES = {
    "PedraClara": "Pedra clara para fundações e fachadas.jpg",
    "PedraMuralha": "Pedra escura para muralhas e torres.jpg",
    "PedraRua": "Textura seamless de pedra da rua.jpg",
    "Calcamento": "Pedra de calçamento para ruas.jpg",
    "Reboco": "Reboco de barro para paredes.jpg",
    "MadeiraEscura": "Madeira escura para vigas e portas.jpg",
    "MadeiraClara": "Madeira clara para paredes e pisos.jpg",
    "Madeira": "Textura seamless de madeira.jpg",
    "TelhaBarro": "Telhas de barro.jpg",
    "TelhaMadeira": "Telhas de madeira.jpg",
    "Palha": "Palha para telhados e depósitos.jpg",
    "Tijolo": "Tijolos para lareiras e fornos.jpg",
    "Terra": "Terra batida para caminhos.jpg",
    "Ferro": "Ferro envelhecido.jpg",
    "Tecido": "Textura seamless de tecido.jpg",
}


def drawn(name):
    rnd = random.Random(name)
    img = Image.new("RGB", (SIZE, SIZE))
    d = ImageDraw.Draw(img)
    if name == "Vidro":
        # vidro de janela antigo: azul escuro esverdeado, um reflexo claro em diagonal, as divisões de chumbo
        for y in range(SIZE):
            t = y / SIZE
            d.line([(0, y), (SIZE, y)], fill=(int(40 + 25 * t), int(58 + 30 * t), int(72 + 30 * t)))
        for k in range(3):
            o = rnd.randint(0, SIZE)
            d.polygon([(o, 0), (o + 60, 0), (o - 140, SIZE), (o - 200, SIZE)], fill=(92, 118, 130))
        img = img.filter(ImageFilter.GaussianBlur(10))
        d = ImageDraw.Draw(img)
        for p in range(0, SIZE, SIZE // 4):
            d.line([(p, 0), (p, SIZE)], fill=(30, 30, 32), width=6)
            d.line([(0, p), (SIZE, p)], fill=(30, 30, 32), width=6)
    elif name == "Folha":
        # folhagem (heras, jardineiras, copas pequenas): manchas de folhas em verdes
        img.paste((52, 86, 40), (0, 0, SIZE, SIZE))
        for n in range(1800):
            x, y, r = rnd.randint(0, SIZE), rnd.randint(0, SIZE), rnd.randint(6, 16)
            g = rnd.choice(((70, 112, 48), (92, 138, 58), (56, 96, 42), (110, 150, 66), (44, 74, 36)))
            for ox in (-SIZE, 0, SIZE):
                for oy in (-SIZE, 0, SIZE):
                    d.ellipse([x + ox - r, y + oy - r * .7, x + ox + r, y + oy + r * .7], fill=g)
    elif name == "Brasa":
        # carvão em brasa (a forja; o material brilha)
        img.paste((40, 14, 6), (0, 0, SIZE, SIZE))
        for n in range(900):
            x, y, r = rnd.randint(0, SIZE), rnd.randint(0, SIZE), rnd.randint(8, 22)
            c = rnd.choice(((255, 140, 30), (255, 90, 20), (200, 50, 10), (60, 20, 10), (255, 200, 80)))
            d.ellipse([x - r, y - r, x + r, y + r], fill=c)
        img = img.filter(ImageFilter.GaussianBlur(3))
    elif name == "Lanterna":
        # 07/11: o vidro do lampião (quente, com o miolo mais claro: a chama), brilha à noite pelo Glow
        for y in range(SIZE):
            for x in range(0, SIZE, 4):
                r = ((x - SIZE / 2) ** 2 + (y - SIZE * .55) ** 2) ** .5 / (SIZE * .6)
                t = max(0.0, 1 - r)
                d.rectangle([x, y, x + 3, y], fill=(int(200 + 55 * t), int(130 + 110 * t), int(60 + 120 * t * t)))
        img = img.filter(ImageFilter.GaussianBlur(6))
    elif name == "Pano":
        # o pano listrado dos toldos (as cores vêm da cor de cada peça; aqui só o tecido claro com listras)
        img.paste((232, 226, 214), (0, 0, SIZE, SIZE))
        for x in range(0, SIZE, SIZE // 4):
            d.rectangle([x, 0, x + SIZE // 8, SIZE], fill=(180, 60, 52))
        img = img.filter(ImageFilter.GaussianBlur(1))
    return img


# 06/11: o usuário achou as de cima feias ("eram só referência; quero melhores"): as principais vêm do Poly Haven (CC0,
# livres para jogo), em 1K, com o mapa de relevo (normal, no padrão DirectX da Unreal: T_Vila_<nome>_N). As de cima
# ficam só para o que não tem par lá (tecido) e as desenhadas aqui (vidro, folha, brasa, pano).
POLYHAVEN = {
    "PedraClara": "medieval_blocks_05", "PedraMuralha": "stone_wall", "PedraBruta": "plastered_stone_wall",
    "PedraRua": "plastered_stone_wall", "Calcamento": "medieval_blocks_02", "Reboco": "clay_plaster",
    "MadeiraEscura": "old_wood_floor", "Madeira": "medieval_wood", "MadeiraClara": "oak_wood_planks",
    "TelhaBarro": "clay_roof_tiles_02", "TelhaMadeira": "roof_09", "Palha": "reed_roof_04", "Tijolo": "red_brick_03",
    "Terra": "stony_dirt_path", "Ferro": "metal_plate",
    # 07/11 (o usuário: "os interiores muito simples e feios; baixe texturas novas para cada item"): as dos móveis
    "Piso": "old_wooden_floor_02", "Movel": "wood_cabinet_worn_long", "Colcha": "rough_linen", "Tapete": "floral_jacquard",
    "Lareira": "castle_brick_07", "Couro": "brown_leather", "Livros": "book_pattern", "PedraPiso": "stone_tiles_02",
    # e a emenda da rua com a grama (o usuário: "pedra e grama direto fica esquisito")
    "RuaGrama": "grassy_cobblestone",
}


def polyhaven(name, asset):
    import io
    import json as _json
    import urllib.request
    def get(url):
        return urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "VortexGame-texture-fetch/1.0"}), timeout=120).read()
    files = _json.loads(get("https://api.polyhaven.com/files/" + asset))
    for kind, suffix in (("Diffuse", ""), ("nor_dx", "_N")):
        url = files[kind if kind in files else "col1"]["1k"]["jpg"]["url"]   # (book_pattern: a cor é "col1")
        data = get(url)
        Image.open(io.BytesIO(data)).convert("RGB").resize((1024, 1024), Image.LANCZOS).save(os.path.join(OUT, "T_Vila_%s%s.png" % (name, suffix)))


os.makedirs(OUT, exist_ok=True)
import sys
ONLY = sys.argv[1:]          # só estas (as outras ficam como estão): python MakeVilaTextures.py Piso Movel
if ONLY:
    for name in ONLY:
        polyhaven(name, POLYHAVEN[name])
        print("poly haven", name, POLYHAVEN[name])
    sys.exit(0)
for name, file in FROM_FILES.items():
    im = Image.open(os.path.join(SRC, file)).convert("RGB").resize((SIZE, SIZE), Image.LANCZOS)
    im.save(os.path.join(OUT, "T_Vila_%s.png" % name))
for name, asset in POLYHAVEN.items():
    polyhaven(name, asset)
    print("poly haven", name, asset)
for name in ("Vidro", "Folha", "Brasa", "Pano", "Lanterna"):
    drawn(name).save(os.path.join(OUT, "T_Vila_%s.png" % name))
Image.new("RGB", (16, 16), (128, 128, 255)).save(os.path.join(OUT, "T_Vila_Plano_N.png"))   # o relevo plano (padrão do M_VilaKit)
print("VILA_TEX", len(os.listdir(OUT)), "texturas em", OUT)
