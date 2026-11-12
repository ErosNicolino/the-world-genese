"""Ruído suave e contínuo (sem emenda) para a variação de cor em escala grande do terreno e da vegetação.

blender --background --factory-startup --python Tools/Blender/MakeMacroNoise.py
Grava Data/Textures/T_MacroNoise.png (256 px, cinza; R = manchas grandes, G = manchas médias, B = miúdo).
Sempre igual (semente fixa). Depois: Tools/ImproveVegetationMaterials.py no editor fechado.
"""
import os

import bpy
import numpy as np

ROOT = os.environ.get("VORTEX_ROOT", os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))
OUT = os.path.join(ROOT, "Data", "Textures", "T_MacroNoise.png")
N = 256
rng = np.random.default_rng(7)


def tileable(cells):
    """Ruído de valor com interpolação suave, repetindo nas bordas (a grade dá a volta)."""
    grid = rng.random((cells, cells))
    t = np.arange(N) * cells / N
    i0 = np.floor(t).astype(int) % cells
    i1 = (i0 + 1) % cells
    f = t - np.floor(t)
    f = f * f * (3 - 2 * f)
    rows0, rows1 = grid[i0], grid[i1]
    a = rows0[:, i0] * (1 - f) + rows0[:, i1] * f
    b = rows1[:, i0] * (1 - f) + rows1[:, i1] * f
    return a * (1 - f)[:, None] + b * f[:, None]


def octaves(base, count):
    total, amp, norm = np.zeros((N, N)), 1.0, 0.0
    for o in range(count):
        total += tileable(base * 2 ** o) * amp
        norm += amp
        amp *= .5
    v = total / norm
    return (v - v.min()) / (v.max() - v.min())


channels = [octaves(4, 3), octaves(12, 3), octaves(48, 2)]
pixels = np.ones((N, N, 4), dtype=np.float32)
for c, v in enumerate(channels):
    pixels[..., c] = v
os.makedirs(os.path.dirname(OUT), exist_ok=True)
img = bpy.data.images.new("T_MacroNoise", N, N, alpha=False)
img.colorspace_settings.name = "Non-Color"
img.pixels = pixels.ravel()
img.filepath_raw = OUT
img.file_format = "PNG"
img.save()
print("VORTEX_MACRO_NOISE", OUT)
