"""Textura de ruído que se repete sem emenda para a lava (Data/UI/Lava/T_LavaNoise.png):
R = manchas grandes e macias, G = veios finos (cristas), B = células (placas de crosta).
    python Tools/MakeLavaNoise.py      (depois: Tools/BuildLava.py no editor fechado importa e monta)
"""
import os
import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "UI", "Lava")
os.makedirs(OUT, exist_ok=True)
N = 512
rng = np.random.default_rng(73)


def periodic(power):
    """Ruído periódico filtrado no espectro (repete nas bordas)."""
    f = np.fft.fftfreq(N)
    fx, fy = np.meshgrid(f, f)
    k = np.sqrt(fx ** 2 + fy ** 2) + 1e-6
    spectrum = (rng.normal(size=(N, N)) + 1j * rng.normal(size=(N, N))) / k ** power
    spectrum[0, 0] = 0
    out = np.real(np.fft.ifft2(spectrum))
    return (out - out.min()) / (out.max() - out.min())


def cells(count):
    """Células de Voronoi periódicas: distância à borda da célula (0 na fresta, 1 no meio)."""
    pts = rng.random((count, 2)) * N
    yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
    d1 = np.full((N, N), 1e9, np.float32)
    d2 = np.full((N, N), 1e9, np.float32)
    for px, py in pts:
        for ox in (-N, 0, N):
            for oy in (-N, 0, N):
                d = np.hypot(xx - (px + ox), yy - (py + oy))
                closer = d < d1
                d2 = np.where(closer, d1, np.minimum(d2, d))
                d1 = np.where(closer, d, d1)
    edge = d2 - d1
    return np.clip(edge / np.percentile(edge, 95), 0, 1)


big = periodic(2.4)
fine = periodic(1.6)
ridges = 1 - np.abs(fine * 2 - 1)
plates = cells(42)
img = np.stack([big, ridges ** 1.5, plates], axis=-1)
Image.fromarray((img * 255).astype(np.uint8)).save(os.path.join(OUT, "T_LavaNoise.png"))
print("ok", OUT)
