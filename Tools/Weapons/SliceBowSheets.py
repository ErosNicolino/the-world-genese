"""Os arcos das pranchas do usuário (07/11: `modelando ainda/Futuras modelagens/Armas/Arcos/Arcos - <Raridade>.jpg`,
5 por prancha, do Normal ao Divino) recortados e reduzidos à grade de pixels que o Blender transforma em blocos
(Tools/Blender/MakePixelBows.py). O mesmo recorte das espadas (SliceSwordSheets.py), com três diferenças:

- a corda sai: ela é fina (bem menos que um pixel da arte) e some na abertura da máscara; a do jogo é feita na
  Unreal (VortexWeaponParts::TickBowString), de ponta a ponta do modelo;
- todos ficam virados para o mesmo lado: a pegada (o meio) para a direita, as pontas para a esquerda (algumas pranchas
  desenham o arco ao contrário);
- a prancha dos Únicos tem os nomes escritos embaixo: a faixa do texto sai.

python Tools/Weapons/SliceBowSheets.py   ->   Data/VortexWeapons/Bows/<raridade>_<1..5>.png (1 pixel = 1 bloco)
                                              e Data/VortexWeapons/Bows/_conferencia.png
"""
import importlib.util
import os

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "modelando ainda", "Futuras modelagens", "Armas", "Arcos")
OUT = os.path.join(ROOT, "Data", "VortexWeapons", "Bows")
TIERS = [("comum", "Normal"), ("incomum", "Incomum"), ("raro", "Raro"), ("epico", "Épico"), ("lendario", "Lendário"),
         ("unico", "Único"), ("divino", "Divino")]
TALLEST = 64
TEXT_BAND = {"unico": .82}     # a parte de baixo da prancha com os nomes (fração da altura)
# os arcos brancos quase da cor do fundo: o limite contra o fundo bem mais fino
LOW = {"divino_3": 9, "unico_4": 9}
# a sombra desenhada atrás do arco (só na prancha dos Incomuns): sai pela cor
SHADOW = {"incomum": (137, 140, 156)}

spec = importlib.util.spec_from_file_location("slice_swords", os.path.join(os.path.dirname(os.path.abspath(__file__)), "SliceSwordSheets.py"))
swords = importlib.util.module_from_spec(spec)
spec.loader.exec_module(swords)


def mask_of(img, thr=34, text_band=None):
    """Como o das espadas, mas a corda sai ANTES de tapar os buracos (senão o vão entre o arco e a corda enchia)."""
    a = np.asarray(img).astype(np.float32)
    border = np.concatenate([a[:8].reshape(-1, 3), a[-8:].reshape(-1, 3), a[:, :8].reshape(-1, 3), a[:, -8:].reshape(-1, 3)])
    m = np.abs(a - np.median(border, axis=0)).max(axis=2) > thr
    if text_band:
        m[int(m.shape[0] * text_band):] = False
    m = ndimage.binary_opening(m, iterations=2) & ~string_of(m)
    return ndimage.binary_fill_holes(m)


def runs(line):
    """O comprimento do trecho cheio onde cada ponto da linha está (0 nos vazios)."""
    out = np.zeros(len(line), np.int32)
    d = np.diff(np.concatenate([[0], line.astype(np.int8), [0]]))
    for s, e in zip(np.nonzero(d == 1)[0], np.nonzero(d == -1)[0]):
        out[s:e] = e - s
    return out


def string_of(m):
    """A corda: o traço reto, comprido na vertical (mais de 40% da altura dos arcos) e fino na horizontal (até 24 px;
    os braços do arco são mais grossos ou não ficam em pé por tanto tempo). Um pouco mais larga, para levar a borda."""
    rows = np.nonzero(m.any(axis=1))[0]
    tall = (rows.max() - rows.min()) * .4 if len(rows) else 1
    vertical = np.stack([runs(m[:, x]) for x in range(m.shape[1])], axis=1)
    horizontal = np.stack([runs(m[y]) for y in range(m.shape[0])])
    s = (vertical > tall) & (horizontal <= 24)
    return ndimage.binary_dilation(s, structure=np.ones((1, 5)))


def facing_right(g):
    """Vira a grade para a pegada ficar à direita das pontas."""
    filled = g[..., 3] > 0
    rows = np.nonzero(filled.any(axis=1))[0]
    r0, r1 = rows.min(), rows.max()
    n = r1 - r0 + 1

    def center(rs):
        cols = [np.nonzero(filled[r])[0].mean() for r in rs if filled[r].any()]
        return np.mean(cols)
    mid = center(range(r0 + int(n * .4), r0 + int(n * .6) + 1))
    tips = center(list(range(r0, r0 + max(2, n // 10))) + list(range(r1 - max(2, n // 10) + 1, r1 + 1)))
    return g if mid >= tips else g[:, ::-1].copy()


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    sheet_rows = []
    for tier, label in TIERS:
        img = Image.open(os.path.join(SRC, "Arcos - %s.jpg" % label)).convert("RGB")
        m = mask_of(img, text_band=TEXT_BAND.get(tier))
        cols = swords.split_columns(m)
        crops = []
        for c0, c1 in cols:
            sub = np.zeros_like(m)
            sub[:, c0:c1] = m[:, c0:c1]
            crops.append(swords.main_piece(sub))
        tallest = max(np.ptp(np.nonzero(c)[0]) + 1 for c in crops)
        ppv = tallest / TALLEST
        grids = []
        for i, c in enumerate(crops, 1):
            if "%s_%d" % (tier, i) in LOW:
                c0, c1 = cols[i - 1]
                fine = mask_of(img, LOW["%s_%d" % (tier, i)], TEXT_BAND.get(tier))
                sub = np.zeros_like(m)
                sub[:, max(0, c0 - 10):c1 + 10] = fine[:, max(0, c0 - 10):c1 + 10]
                c = swords.main_piece(sub)
            g = swords.to_grid(img, c, ppv)
            if tier in SHADOW:
                g[np.abs(g[..., :3].astype(int) - SHADOW[tier]).max(axis=2) < 16] = 0
            g = facing_right(g)
            Image.fromarray(g, "RGBA").save(os.path.join(OUT, "%s_%d.png" % (tier, i)))
            grids.append(g)
            print("arco %s_%d: %d x %d blocos" % (tier, i, g.shape[1], g.shape[0]))
        assert len(grids) == 5, (tier, len(grids))
        sheet_rows.append(grids)
    S = 6
    widest = max(g.shape[1] for row in sheet_rows for g in row) * S + 12
    Hs = [max(g.shape[0] for g in row) * S + 12 for row in sheet_rows]
    sheet = Image.new("RGBA", (widest * 5, sum(Hs)), (60, 60, 64, 255))
    y = 0
    for row, h in zip(sheet_rows, Hs):
        for i, g in enumerate(row):
            im = Image.fromarray(g, "RGBA").resize((g.shape[1] * S, g.shape[0] * S), Image.NEAREST)
            sheet.alpha_composite(im, (i * widest + 6, y + 6))
        y += h
    sheet.save(os.path.join(OUT, "_conferencia.png"))
    print("conferência:", os.path.join(OUT, "_conferencia.png"))
