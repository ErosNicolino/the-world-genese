"""As espadas das pranchas do usuário (07/11: `modelando ainda/Futuras modelagens/Armas/Espadas/Espadas - <Raridade>.jpg`,
5 por prancha, do Normal ao Divino) recortadas e reduzidas à grade de pixels que o Blender transforma em blocos
(Tools/Blender/MakePixelSwords.py).

python Tools/Weapons/SliceSwordSheets.py   ->   Data/VortexWeapons/Swords/<raridade>_<1..5>.png (RGBA, 1 pixel = 1 bloco)
                                                e Data/VortexWeapons/Swords/_conferencia.png

- O fundo é a cor da borda da prancha; o que difere dele é espada. As faíscas, o brilho e a fumaça em volta (pedaços
  soltos) saem: fica a peça maior e o que encosta nela.
- As espadas são separadas pelas colunas vazias entre elas.
- A mais alta de cada prancha vira 64 blocos de comprimento; as outras na mesma escala (as curtas continuam curtas).
"""
import os

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "modelando ainda", "Futuras modelagens", "Armas", "Espadas")
OUT = os.path.join(ROOT, "Data", "VortexWeapons", "Swords")
TIERS = [("comum", "Normal"), ("incomum", "Incomum"), ("raro", "Raro"), ("epico", "Épico"), ("lendario", "Lendário"),
         ("unico", "Único"), ("divino", "Divino")]
TALLEST = 64
# as de lâmina branca quase da cor do fundo: só o contorno aparecia; nelas o limite contra o fundo é bem mais fino
SOLID = {"raro_2": 9, "unico_4": 7}


def mask_of(img, thr=34):
    a = np.asarray(img).astype(np.float32)
    border = np.concatenate([a[:8].reshape(-1, 3), a[-8:].reshape(-1, 3), a[:, :8].reshape(-1, 3), a[:, -8:].reshape(-1, 3)])
    bg = np.median(border, axis=0)
    diff = np.abs(a - bg).max(axis=2)
    m = diff > thr
    m = ndimage.binary_opening(m, iterations=2)
    m = ndimage.binary_fill_holes(m)
    return m


def split_columns(m, want=5):
    occ = m.sum(axis=0) > 2
    runs, start = [], None
    for x, on in enumerate(occ):
        if on and start is None:
            start = x
        if not on and start is not None:
            runs.append([start, x])
            start = None
    if start is not None:
        runs.append([start, len(occ)])
    # junta pedaços colados (faíscas ao lado da lâmina)
    merged = []
    for r in runs:
        if merged and r[0] - merged[-1][1] < 18:
            merged[-1][1] = r[1]
        else:
            merged.append(r)
    merged.sort(key=lambda r: -m[:, r[0]:r[1]].sum())
    return sorted(merged[:want])


def main_piece(m):
    """A peça maior e o que fica a até 3 px dela (sem as faíscas soltas)."""
    lab, n = ndimage.label(m)
    if n <= 1:
        return m
    sizes = ndimage.sum(m, lab, range(1, n + 1))
    big = lab == (1 + int(np.argmax(sizes)))
    near = ndimage.binary_dilation(big, iterations=3)
    keep = np.zeros_like(m)
    for i in range(1, n + 1):
        part = lab == i
        if (part & near).any() and sizes[i - 1] > 40:
            keep |= part
    return keep


def to_grid(img, m, ppv):
    ys, xs = np.nonzero(m)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    a = np.asarray(img).astype(np.float32)[y0:y1, x0:x1]
    mm = m[y0:y1, x0:x1]
    H, W = max(1, round((y1 - y0) / ppv)), max(1, round((x1 - x0) / ppv))
    out = np.zeros((H, W, 4), np.uint8)
    for gy in range(H):
        for gx in range(W):
            ya, yb = int(gy * ppv), max(int(gy * ppv) + 1, int((gy + 1) * ppv))
            xa, xb = int(gx * ppv), max(int(gx * ppv) + 1, int((gx + 1) * ppv))
            cell = mm[ya:yb, xa:xb]
            if cell.size == 0 or cell.mean() < .45:
                continue
            # o miolo da célula (a borda mistura com o vizinho): a cor mais comum do miolo
            cy0, cy1 = ya + (yb - ya) // 4, yb - (yb - ya) // 4
            cx0, cx1 = xa + (xb - xa) // 4, xb - (xb - xa) // 4
            px = a[cy0:max(cy1, cy0 + 1), cx0:max(cx1, cx0 + 1)][mm[cy0:max(cy1, cy0 + 1), cx0:max(cx1, cx0 + 1)]]
            if px.size == 0:
                px = a[ya:yb, xa:xb][cell]
            out[gy, gx, :3] = np.median(px, axis=0).astype(np.uint8)
            out[gy, gx, 3] = 255
    return out


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    sheet_rows = []
    for tier, label in TIERS:
        img = Image.open(os.path.join(SRC, "Espadas - %s.jpg" % label)).convert("RGB")
        m = mask_of(img)
        cols = split_columns(m)
        crops = []
        for c0, c1 in cols:
            sub = np.zeros_like(m)
            sub[:, c0:c1] = m[:, c0:c1]
            crops.append(main_piece(sub))
        tallest = max(np.ptp(np.nonzero(c)[0]) + 1 for c in crops)
        ppv = tallest / TALLEST
        grids = []
        for i, c in enumerate(crops, 1):
            if "%s_%d" % (tier, i) in SOLID:
                c0, c1 = cols[i - 1]
                fine = mask_of(img, SOLID["%s_%d" % (tier, i)])
                sub = np.zeros_like(m)
                sub[:, max(0, c0 - 10):c1 + 10] = fine[:, max(0, c0 - 10):c1 + 10]
                c = main_piece(sub)
            g = to_grid(img, c, ppv)
            Image.fromarray(g, "RGBA").save(os.path.join(OUT, "%s_%d.png" % (tier, i)))
            grids.append(g)
            print("espada %s_%d: %d x %d blocos" % (tier, i, g.shape[1], g.shape[0]))
        sheet_rows.append(grids)
    # a conferência: as 35 lado a lado, 6x maiores
    S = 6
    W = sum(max(g.shape[1] for g in row) * S + 12 for row in sheet_rows[:1]) * 5
    Hs = [max(g.shape[0] for g in row) * S + 12 for row in sheet_rows]
    widest = max(g.shape[1] for row in sheet_rows for g in row) * S + 12
    sheet = Image.new("RGBA", (widest * 5, sum(Hs)), (60, 60, 64, 255))
    y = 0
    for row, h in zip(sheet_rows, Hs):
        for i, g in enumerate(row):
            im = Image.fromarray(g, "RGBA").resize((g.shape[1] * S, g.shape[0] * S), Image.NEAREST)
            sheet.alpha_composite(im, (i * widest + 6, y + 6))
        y += h
    sheet.save(os.path.join(OUT, "_conferencia.png"))
    print("conferência:", os.path.join(OUT, "_conferencia.png"))
