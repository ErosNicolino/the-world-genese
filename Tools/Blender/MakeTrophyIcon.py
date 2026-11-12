"""Ícone de troféu do menu (Conquistas), branco e chapado como os outros ícones do menu.

blender --background --factory-startup --python Tools/Blender/MakeTrophyIcon.py
Grava Data/UI/Menu/T_MenuIcon_Trophy.png (256 px, branco com transparência). Depois: Tools/ImportMenuIcons.py.
"""
import os

import bpy
import numpy as np

ROOT = os.environ.get("VORTEX_ROOT", os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))
OUT = os.path.join(ROOT, "Data", "UI", "Menu", "T_MenuIcon_Trophy.png")
N = 256
SS = 4                                   # supersampling para bordas lisas
y, x = np.mgrid[0:N * SS, 0:N * SS] / SS
x = x / N
y = 1.0 - y / N                          # y para cima, 0..1
shape = np.zeros_like(x, dtype=bool)

# taça: meio círculo achatado em cima, boca reta
cx, top = .5, .86
cup = ((x - cx) / .26) ** 2 + ((y - top) / .36) ** 2 <= 1.0
shape |= cup & (y <= top)
# alças: anéis dos dois lados
for side in (-1, 1):
    hx = cx + side * .27
    r = np.hypot(x - hx, (y - .72) / 1.15)
    shape |= (r <= .13) & (r >= .075) & ((x - cx) * side > .2)
# haste e base
shape |= (abs(x - cx) <= .045) & (y >= .3) & (y <= .52)
shape |= (abs(x - cx) <= .17) & (y >= .2) & (y <= .29)
shape |= (abs(x - cx) <= .23) & (y >= .1) & (y <= .19)
# estrela vazada na taça
ang = np.arctan2(y - .7, x - cx)
rad = np.hypot(x - cx, y - .7)
star = rad <= (.05 + .035 * np.cos(5 * (ang - np.pi / 2)) ** 2)
shape &= ~star

alpha = shape.reshape(N, SS, N, SS).mean(axis=(1, 3))
pixels = np.ones((N, N, 4), dtype=np.float32)
pixels[..., 3] = alpha[::-1]             # a imagem do Blender começa embaixo
os.makedirs(os.path.dirname(OUT), exist_ok=True)
img = bpy.data.images.new("T_MenuIcon_Trophy", N, N, alpha=True)
img.pixels = pixels.ravel()
img.filepath_raw = OUT
img.file_format = "PNG"
img.save()
print("VORTEX_TROPHY_ICON", OUT)
