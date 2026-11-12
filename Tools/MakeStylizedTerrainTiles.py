"""Chão em pixel art que combina com a grama (10/11).

Refaz os quadradinhos de grama (0, 1, 2, 3 e 18) e a beira de grama (7) de Data/GodotTerrain/terrain_tiles.png, em
32 px como os outros. Cada um usa 5 tons fixos tirados da média do original: as sombras puxam para o azul e as luzes
para o amarelo, como na pixel art. O fundo são manchas em grupos de pixels (sem ruído pixel a pixel) e por cima vão
tufinhos com a luz de cima e da esquerda (pixel claro em cima, sombra embaixo e à direita).

A média de cada quadradinho fica igual à do original: o streamer tinge o chão por cor = média x tom da região
(FVortexTerrainData::CellTint), e a grama usa essa cor na raiz. Mudar a média mudaria a cor do mundo.
As manchas grandes (dezenas de metros) ficam no material (M_TerrainTiles), não aqui. Tudo repete sem emenda.

Original: Backups/terrain_tiles_32px_2026-11-10.png. Depois: Tools/ImportTerrainTiles.py com o editor fechado.

  python Tools/MakeStylizedTerrainTiles.py
"""
import os
import shutil

import numpy as np
from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
ATLAS = os.path.join(ROOT, "Data", "GodotTerrain", "terrain_tiles.png")
BACKUP = os.path.join(ROOT, "..", "Backups", "terrain_tiles_32px_2026-11-10.png")
T = 32
GRASS_TILES = (0, 1, 2, 3, 18)
LIP_TILE = 7

if not os.path.exists(BACKUP):
    os.makedirs(os.path.dirname(BACKUP), exist_ok=True)
    shutil.copy2(ATLAS, BACKUP)
src = np.asarray(Image.open(BACKUP).convert("RGB")).astype(np.float32) / 255.0
assert src.shape[:2] == (32, 21 * 32), src.shape


def palette(mean):
    """5 tons em volta da média: sombra fria, meio e luz quente."""
    steps = (0.62, 0.89, 1.0, 1.09, 1.30)
    shift = (np.array([-0.04, 0.0, 0.06]), np.array([-0.02, 0.0, 0.03]), np.zeros(3), np.array([0.03, 0.02, -0.03]), np.array([0.07, 0.05, -0.06]))
    return [np.clip(mean * s + d, 0, 1) for s, d in zip(steps, shift)]


def tile_noise(rng, cells):
    grid = rng.random((cells, cells))
    y, x = np.mgrid[0:T, 0:T] / T * cells
    x0, y0 = np.floor(x).astype(int), np.floor(y).astype(int)
    fx, fy = x - x0, y - y0
    fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
    g = lambda a, b: grid[b % cells, a % cells]
    return (g(x0, y0) * (1 - fx) + g(x0 + 1, y0) * fx) * (1 - fy) + (g(x0, y0 + 1) * (1 - fx) + g(x0 + 1, y0 + 1) * fx) * fy


# tufinho visto de cima: 1 = luz, 0 = sombra (linha, coluna a partir do canto de cima à esquerda)
TUFTS = [
    {(0, 1): 1, (1, 0): 1, (1, 1): 2, (1, 2): 2, (2, 1): 0, (2, 2): 0},
    {(0, 0): 1, (0, 2): 1, (1, 1): 2, (1, 0): 2, (2, 1): 0, (1, 2): 0},
    {(0, 1): 1, (1, 1): 2, (2, 1): 0},
    {(0, 0): 1, (1, 1): 2, (2, 2): 0, (1, 2): 0},
]


def paint_grass(seed, mean):
    rng = np.random.default_rng(seed)
    tones = palette(mean)
    # fundo em 3 tons (1, 2, 3) em manchas de 4 a 8 px
    field = tile_noise(rng, 4) * 0.7 + tile_noise(rng, 8) * 0.3
    index = np.where(field < 0.38, 1, np.where(field < 0.66, 2, 3))
    # tufinhos: luz (4), meio (3) e sombra (0)
    for _ in range(16):
        tuft = TUFTS[rng.integers(len(TUFTS))]
        x, y = rng.integers(0, T, 2)
        for (r, c), kind in tuft.items():
            index[(y + r) % T, (x + c) % T] = {1: 4, 2: 3, 0: 0}[kind]
    img = np.zeros((T, T, 3), np.float32)
    for k, tone in enumerate(tones):
        img[index == k] = tone
    # a média volta à do original (a cor do mundo não muda)
    return np.clip(img * (mean / np.maximum(img.reshape(-1, 3).mean(0), 1e-3)), 0, 1), index


out = src.copy()
for i in GRASS_TILES:
    mean = src[:, i * T:(i + 1) * T].reshape(-1, 3).mean(0)
    out[:, i * T:(i + 1) * T], _ = paint_grass(1000 + i, mean)

# a beira: franja de grama (6 a 10 px, com pontas caindo) e uma linha de sombra embaixo dela
lip = src[:, LIP_TILE * T:(LIP_TILE + 1) * T].copy()
green_mean = src[:, 0:T].reshape(-1, 3).mean(0) * 0.9
fringe, _ = paint_grass(1007, green_mean)
rng = np.random.default_rng(77)
depth = 7 + rng.integers(0, 3, T)
for x in range(T):
    if rng.random() < 0.18:
        depth[x] += rng.integers(2, 4)   # uma ponta de grama descendo
for x in range(T):
    d = int(depth[x])
    lip[:d, x] = fringe[:d, x]
    lip[0, x] = palette(green_mean)[4]   # a quina de cima, clara
    lip[d:d + 1, x] *= 0.62               # sombra embaixo da franja
out[:, LIP_TILE * T:(LIP_TILE + 1) * T] = lip

Image.fromarray((out * 255 + 0.5).astype(np.uint8)).save(ATLAS)
print("TERRAIN_TILES", ATLAS, out.shape)
