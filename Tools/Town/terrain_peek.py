"""Lê Data/GodotTerrain/terrain.bin (o mesmo formato do FVortexTerrainData::Load) e desenha o relevo em volta da vila.

python Tools/Town/terrain_peek.py [x0 z0 x1 z1 em m] [saida.png]
Cada célula: altura (int16 x 25 cm), camada, giro, cor. Unreal X = x do Godot, Y = z do Godot.
"""
import struct
import sys

import numpy as np
from PIL import Image, ImageDraw

ROOT = __file__.replace("\\", "/").rsplit("/Tools/", 1)[0]


def load():
    b = open(ROOT + "/Data/GodotTerrain/terrain.bin", "rb").read()
    assert b[:4] == b"VTRN"
    ver, cmx, cmy, ccx, ccy, cpc = struct.unpack_from("<6i", b, 4)
    cell, water, surface = struct.unpack_from("<3f", b, 28)
    off = 40
    n = ccx * ccy * cpc * cpc
    rec = np.frombuffer(b, dtype=np.dtype([("h", "<i2"), ("layer", "u1"), ("rot", "u1"), ("r", "u1"), ("g", "u1"), ("b", "u1")]), count=n, offset=off)
    # índice: ((chunkY*ccx + chunkX)*cpc + ix)*cpc + iz
    h = rec["h"].reshape(ccy, ccx, cpc, cpc).astype(np.float32) * .25          # metros
    rgb = np.stack([rec["r"], rec["g"], rec["b"]], -1).reshape(ccy, ccx, cpc, cpc, 3)
    # para uma grade [x][z]: x = chunkX*cpc + ix, z = chunkY*cpc + iz
    H = h.transpose(1, 2, 0, 3).reshape(ccx * cpc, ccy * cpc)
    C = rgb.transpose(1, 2, 0, 3, 4).reshape(ccx * cpc, ccy * cpc, 3)
    return dict(H=H, C=C, x0=cmx * cpc, z0=cmy * cpc, cell=cell, water=water, surface=surface)


def at(T, x, z):
    i = int(np.floor(x / T["cell"])) - T["x0"]
    k = int(np.floor(z / T["cell"])) - T["z0"]
    return T["H"][i, k]


if __name__ == "__main__":
    T = load()
    print("célula %.2f m, água %.2f, superfície %.2f, grade %s, origem %d,%d" % (T["cell"], T["water"], T["surface"], T["H"].shape, T["x0"], T["z0"]))
    a = [float(v) for v in sys.argv[1:5]] if len(sys.argv) >= 5 else [-180, -150, 180, 210]
    out = sys.argv[5] if len(sys.argv) > 5 else None
    c = T["cell"]
    i0, i1 = int(a[0] / c) - T["x0"], int(a[2] / c) - T["x0"]
    k0, k1 = int(a[1] / c) - T["z0"], int(a[3] / c) - T["z0"]
    sub = T["H"][i0:i1, k0:k1]
    print("alturas na área: min %.1f max %.1f média %.1f" % (sub.min(), sub.max(), sub.mean()))
    if out:
        lo, hi = np.percentile(sub, 2), np.percentile(sub, 98)
        g = np.clip((sub - lo) / max(.1, hi - lo), 0, 1)
        img = np.zeros(sub.shape + (3,), np.uint8)
        img[..., 0] = (60 + 160 * g); img[..., 1] = (90 + 130 * g); img[..., 2] = (60 + 80 * g)
        img[sub <= T["water"]] = (40, 90, 170)
        # x para a direita, z para baixo (como o mapa de cima)
        im = Image.fromarray(img.transpose(1, 0, 2)).resize((sub.shape[0] * 4, sub.shape[1] * 4), Image.NEAREST)
        d = ImageDraw.Draw(im)
        s = 4 / c
        def P(x, z): return ((x - a[0]) * s, (z - a[1]) * s)
        # curvas de nível a cada 2 m (pontos)
        for x in range(int(a[0]), int(a[2]), 10):
            d.line([P(x, a[1]), P(x, a[3])], fill=(255, 255, 255, 40) if x % 50 else (255, 255, 255), width=1 if x % 50 else 2)
        for z in range(int(a[1]), int(a[3]), 10):
            d.line([P(a[0], z), P(a[2], z)], fill=(255, 255, 255) if z % 50 == 0 else (200, 200, 200), width=2 if z % 50 == 0 else 1)
        d.rectangle([P(-62, -34), P(62, 80)], outline=(255, 60, 60), width=3)
        im.save(out)
        print("salvo", out, "(linhas a cada 10 m, grossas a cada 50 m; vermelho = vila de hoje)")
