"""A versão feminina do humano de blocos (01/11): o mesmo corpo, esqueleto e animações; muda só a cor (um top claro no
peito, do mesmo pano da cueca). O rosto feminino (cílios) sai do Tools/MakeBlockFace.py humano --feminino.

python Tools/MakeBlockFemale.py   ->   Data/Heroes/Mixamo/humano_cor_f.png (a partir do humano_cor.png)

No atlas (Tools/Blender/RigBlockCharacter.py) o tronco é a faixa de 36 x 10 que começa na linha 63: frente (colunas
0-6), costas (7-13), lado direito (14-17), lado esquerdo (18-21), topo e base. O top cobre as linhas 1 a 4 das quatro
faces de lado, com a borda de cima mais clara e a de baixo mais escura, e as dobras no meio da frente.
"""
import os
import random

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "Data", "Heroes", "Mixamo", "humano_cor.png")
OUT = os.path.join(ROOT, "Data", "Heroes", "Mixamo", "humano_cor_f.png")
TORSO_Y, SIDES = 63, 22                        # a linha do tronco no atlas e as colunas das 4 faces de lado
TOP_ROWS = (1, 2, 3, 4)
CLOTH = [(222, 197, 172), (219, 193, 168), (216, 189, 165), (212, 184, 159)]
EDGE = (232, 212, 190)
DEEP = (190, 162, 139)
random.seed(5)

im = Image.open(SRC).convert("RGBA")
for r in TOP_ROWS:
    for c in range(SIDES):
        col = random.choice(CLOTH)
        if r == TOP_ROWS[0]:
            col = EDGE
        elif r == TOP_ROWS[-1]:
            col = DEEP
        im.putpixel((c, TORSO_Y + r), col + (255,))
# o meio da frente (colunas 0-6): a dobra entre os dois lados do peito
im.putpixel((3, TORSO_Y + 2), DEEP + (255,))
im.save(OUT)
print("feminina:", OUT)
