"""As armaduras das pranchas do usuário (07/11: `modelando ainda/Futuras modelagens/Armaduras/Armaduras - <Raridade>.jpg`,
3 conjuntos por prancha, do Normal ao Divino), recortadas peça por peça.

Cada prancha tem 3 colunas (os conjuntos) e em cada coluna, de cima para baixo: o capacete, o peitoral, as calças e as
botas (as duas botas lado a lado). O recorte separa as colunas pelos vãos verticais e, dentro delas, as peças pelos
vãos horizontais.

python Tools/Armor/SliceArmorSheets.py   ->   Data/VortexArmor/Pieces/<raridade>_<conjunto 1..3>_<peça>.png (RGBA, o fundo
                                              transparente; a referência de cor da modelagem e o desenho do ícone)
                                              e Data/VortexArmor/Pieces/_conferencia.png
"""
import os

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "modelando ainda", "Futuras modelagens", "Armaduras")
OUT = os.path.join(ROOT, "Data", "VortexArmor", "Pieces")
TIERS = [("comum", "Normal"), ("incomum", "Incomum"), ("raro", "Raro"), ("epico", "Épico"), ("lendario", "Lendário"),
         ("unico", "Único"), ("divino", "Divino")]
SLOTS = ["head", "chest", "legs", "boots"]
CUTS = [(.18, .30), (.47, .53), (.74, .79)]   # onde ficam os vãos entre capacete/peitoral, peitoral/calças e calças/botas


def mask_of(a, thr=28):
    border = np.concatenate([a[:8].reshape(-1, 3), a[-8:].reshape(-1, 3), a[:, :8].reshape(-1, 3), a[:, -8:].reshape(-1, 3)])
    m = np.abs(a - np.median(border, axis=0)).max(axis=2) > thr
    m = ndimage.binary_opening(m, iterations=2)
    return ndimage.binary_fill_holes(m)


def runs(occ, min_gap, min_len=20):
    """Os trechos cheios de um perfil, juntando os separados por menos de min_gap."""
    out, start = [], None
    for i, on in enumerate(occ):
        if on and start is None:
            start = i
        if not on and start is not None:
            out.append([start, i])
            start = None
    if start is not None:
        out.append([start, len(occ)])
    merged = []
    for r in out:
        if merged and r[0] - merged[-1][1] < min_gap:
            merged[-1][1] = r[1]
        else:
            merged.append(r)
    return [r for r in merged if r[1] - r[0] >= min_len]


def biggest(runs_, n):
    """Os n trechos maiores, na ordem."""
    return sorted(sorted(runs_, key=lambda r: -(r[1] - r[0]))[:n])


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    rows_out = []
    for tier, label in TIERS:
        img = Image.open(os.path.join(SRC, "Armaduras - %s.jpg" % label)).convert("RGB")
        a = np.asarray(img).astype(np.float32)
        m = mask_of(a)
        cols = biggest(runs(m.sum(axis=0) > 2, 40), 3)
        assert len(cols) == 3, (tier, cols)
        row = []
        for s, (c0, c1) in enumerate(cols, 1):
            sub = m[:, c0:c1]
            prof = ndimage.uniform_filter1d(sub.sum(axis=1).astype(float), 5)
            # as peças às vezes se encostam (o contorno, o brilho): as 4 linhas ficam sempre na mesma altura nas pranchas,
            # então o corte é no ponto mais vazio perto de cada divisão (em fração da altura)
            Hs = sub.shape[0]
            cuts = [int(lo * Hs + np.argmin(prof[int(lo * Hs):int(hi * Hs)])) for lo, hi in CUTS]
            edges = [0] + cuts + [Hs]
            parts = []
            for r0, r1 in zip(edges[:-1], edges[1:]):
                rows = np.nonzero(sub[r0:r1].any(axis=1))[0]
                parts.append([r0 + rows.min(), r0 + rows.max() + 1])
            for slot, (r0, r1) in zip(SLOTS, parts):
                piece = sub[r0:r1]
                xs = np.nonzero(piece.any(axis=0))[0]
                x0, x1 = c0 + xs.min(), c0 + xs.max() + 1
                rgba = np.zeros((r1 - r0, x1 - x0, 4), np.uint8)
                rgba[..., :3] = np.asarray(img)[r0:r1, x0:x1]
                rgba[..., 3] = m[r0:r1, x0:x1] * 255
                Image.fromarray(rgba, "RGBA").save(os.path.join(OUT, "%s_%d_%s.png" % (tier, s, slot)))
                row.append(Image.fromarray(rgba, "RGBA"))
            print("armadura %s_%d: %s" % (tier, s, [(r1 - r0) for r0, r1 in parts]))
        rows_out.append(row)
    # a conferência: cada prancha numa linha, as 12 peças lado a lado, a 160 px de altura
    H = 160
    thumbs = [[im.resize((max(1, im.width * H // im.height), H)) for im in row] for row in rows_out]
    W = max(sum(t.width + 8 for t in row) for row in thumbs)
    sheet = Image.new("RGBA", (W, (H + 10) * len(thumbs)), (60, 60, 64, 255))
    for y, row in enumerate(thumbs):
        x = 0
        for t in row:
            sheet.alpha_composite(t, (x, y * (H + 10)))
            x += t.width + 8
    sheet.save(os.path.join(OUT, "_conferencia.png"))
    print("conferência:", os.path.join(OUT, "_conferencia.png"))
