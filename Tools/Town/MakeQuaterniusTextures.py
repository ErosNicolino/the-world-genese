"""As texturas dos kits da Quaternius (Medieval Village e Fantasy Props, CC0) em pixel art, para a vila (11/11).

Cada textura de cor do kit (1024 px, pintada à mão) vai para o jogo em 512 px, como é (a escolha do usuário em 11/11).
Com -pixel, vira pixel art de 128 px e 14 tons, como o chão novo. As UVs do kit continuam valendo. Sai Data/GodotTown/VilaTex/T_Vila_Q_<nome>.png,
o caminho que o Tools/ImportVilaKit.py já importa (os modelos usam o material MV_Q_<nome>).

  python Tools/Town/MakeQuaterniusTextures.py
"""
import os
import sys

from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
VILA = os.path.join(ROOT, "modelando ainda", "quaternius_vila", "pacote", "Medieval Village MegaKit[Standard]", "Textures")
PROPS = os.path.join(ROOT, "modelando ainda", "quaternius_objetos", "pacote", "Textures")
OUT = os.path.join(ROOT, "Data", "GodotTown", "VilaTex")
SIZE, TONES = 128, 14
PIXEL = "-pixel" in sys.argv
# nome no jogo: o arquivo do kit
SOURCES = {
    "Q_Plaster": (VILA, "T_Plaster_BaseColor.png"),
    "Q_UnevenBrick": (VILA, "T_UnevenBrick_BaseColor.png"),
    "Q_Brick": (VILA, "T_Brick_BaseColor.png"),
    "Q_RedBrick": (VILA, "T_RedBrick_BaseColor.png"),
    "Q_RockTrim": (VILA, "T_RockTrim_BaseColor.png"),
    "Q_WoodTrim": (VILA, "T_WoodTrim_BaseColor.png"),
    "Q_RoundTiles": (VILA, "T_RoundTiles_BaseColor.png"),
    "Q_Metal": (PROPS, "T_Trim_Metal_BaseColor.png"),
    "Q_Furniture": (PROPS, "T_Trim_Furniture_BaseColor.png"),
    "Q_Props": (PROPS, "T_Trim_Props_BaseColor.png"),
    "Q_Cloth": (PROPS, "T_Trim_Cloth_BaseColor.png"),
}
for name, (folder, file) in SOURCES.items():
    path = os.path.join(folder, file)
    if not os.path.exists(path):
        print("FALTA", path)
        continue
    image = Image.open(path).convert("RGB")
    # 11/11: o usuário escolheu a textura original do kit (pintada à mão), não a pixel art: só reduz para 512 px (menos
    # memória; de longe não muda). -pixel volta à pixel art de 128 px e 14 tons
    if PIXEL:
        small = image.resize((SIZE, SIZE), Image.BOX).quantize(TONES, method=Image.Quantize.MEDIANCUT).convert("RGB")
    else:
        small = image.resize((512, 512), Image.LANCZOS)
    small.save(os.path.join(OUT, "T_Vila_%s.png" % name))
    print("QTEX", name, "<-", file)
