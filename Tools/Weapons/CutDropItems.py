"""Tira o fundo dos ícones novos dos itens (Data/UI/SkillIcons/Novos/item_<id>.png) para virarem modelos 3D (08/11).

python Tools/Weapons/CutDropItems.py
Sai Data/VortexWeapons/DropItems/<id>.png (48 x 48, com transparência), que o Tools/Blender/MakeDropItems.py transforma em
blocos, e a conferência Data/VortexWeapons/DropItems/_conferencia.png.
O fundo é um degradê suave: é o que se alcança a partir da borda andando só entre vizinhos quase iguais; brilhos soltos caem fora.
"""
import pathlib
from collections import deque

import numpy as np
from PIL import Image, ImageFilter

ROOT = pathlib.Path(__file__).resolve().parents[2]
ICONS = ROOT / "Data" / "UI" / "SkillIcons" / "Novos"
OUT = ROOT / "Data" / "VortexWeapons" / "DropItems"
GRID = 48
STEP = 11          # diferença máxima entre vizinhos do fundo (soma de R, G e B)
KEEP_EDGE = {"deer_antler", "mapa_de_viajante", "amulet_moon"}   # da cor do fundo: descascar comeria o desenho
HOLES = {"ring_jade", "amulet_moon", "bark_fiber", "bandit_bandana", "healing_herb"}   # o fundo aparece por dentro deles


def background(rgb):
    h, w = rgb.shape[:2]
    bg = np.zeros((h, w), bool)
    queue = deque((y, x) for y in range(h) for x in (0, w - 1))
    queue.extend((y, x) for x in range(w) for y in (0, h - 1))
    for y, x in queue:
        bg[y, x] = True
    while queue:
        y, x = queue.popleft()
        for ny, nx in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)):
            if 0 <= ny < h and 0 <= nx < w and not bg[ny, nx] and np.abs(rgb[ny, nx] - rgb[y, x]).sum() < STEP:
                bg[ny, nx] = True
                queue.append((ny, nx))
    return bg


def largest_parts(mask):
    """Só os pedaços grandes: sem os brilhinhos."""
    h, w = mask.shape
    seen = np.zeros_like(mask)
    parts = []
    for sy, sx in zip(*np.nonzero(mask)):
        if seen[sy, sx]:
            continue
        seen[sy, sx] = True
        queue = deque([(sy, sx)])
        cells = []
        while queue:
            y, x = queue.popleft()
            cells.append((y, x))
            for ny, nx in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)):
                if 0 <= ny < h and 0 <= nx < w and mask[ny, nx] and not seen[ny, nx]:
                    seen[ny, nx] = True
                    queue.append((ny, nx))
        parts.append(cells)
    biggest = max(map(len, parts))
    out = np.zeros_like(mask)
    for cells in parts:
        if len(cells) >= biggest * .08:
            for y, x in cells:
                out[y, x] = True
    return out


def open_holes(rgb, mask):
    """Tira de dentro da silhueta as manchas lisas da cor do fundo."""
    h, w = mask.shape
    back = rgb[~mask].mean(axis=0)
    seen = np.zeros_like(mask)
    for sy, sx in zip(*np.nonzero(mask)):
        if seen[sy, sx]:
            continue
        seen[sy, sx] = True
        queue = deque([(sy, sx)])
        cells = []
        while queue:
            y, x = queue.popleft()
            cells.append((y, x))
            for ny, nx in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)):
                if 0 <= ny < h and 0 <= nx < w and mask[ny, nx] and not seen[ny, nx] and np.abs(rgb[ny, nx] - rgb[y, x]).sum() < STEP:
                    seen[ny, nx] = True
                    queue.append((ny, nx))
        tone = rgb[tuple(zip(*cells))].mean(axis=0)
        # a mesma cor do fundo, com qualquer brilho (o fundo é mais claro no meio); o ouro do anel, não
        if len(cells) > h * w * .002 and tone @ back / (np.linalg.norm(tone) * np.linalg.norm(back)) > .975:
            for y, x in cells:
                mask[y, x] = False
    return mask


def peel_fringe(rgb, mask, turns=10):
    """A aura do fundo em volta do item: descasca a borda enquanto ela for parecida com o fundo logo ao lado (no máximo 2 blocos)."""
    for _ in range(turns):
        total, count = np.zeros(rgb.shape, float), np.zeros(mask.shape, float)
        out = (~mask).astype(float)
        for dy in range(-5, 6):
            for dx in range(-5, 6):
                total += np.roll(rgb * out[..., None], (dy, dx), (0, 1))
                count += np.roll(out, (dy, dx), (0, 1))
        near = total / np.maximum(count, 1)[..., None]
        drop = mask & ~erode(mask) & (count > 0) & (np.abs(rgb - near).sum(axis=2) < 120)
        if not drop.any():
            break
        mask = mask & ~drop
    return mask


def erode(mask):
    m = mask.copy()
    m[1:] &= mask[:-1]
    m[:-1] &= mask[1:]
    m[:, 1:] &= mask[:, :-1]
    m[:, :-1] &= mask[:, 1:]
    return m


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    files = sorted(ICONS.glob("item_*.png"))
    W = 8
    sheet = Image.new("RGB", (W * 104, (len(files) // W + 1) * 104), (60, 70, 90))
    for i, path in enumerate(files):
        src = Image.open(path).convert("RGB").filter(ImageFilter.GaussianBlur(.8))
        mask = largest_parts(erode(~background(np.asarray(src).astype(int))))
        if path.stem[5:] in HOLES:
            mask = erode(open_holes(np.asarray(src).astype(int), mask))
        if path.stem[5:] not in KEEP_EDGE:
            mask = largest_parts(peel_fringe(np.asarray(src).astype(int), mask))
        full = np.asarray(Image.open(path).convert("RGB"))
        ys, xs = np.nonzero(mask)
        side = max(ys.max() - ys.min(), xs.max() - xs.min()) + 1      # o item ocupa a grade inteira
        cy, cx = (ys.max() + ys.min()) // 2, (xs.max() + xs.min()) // 2
        cell = side / GRID
        out = np.zeros((GRID, GRID, 4), np.uint8)
        for r in range(GRID):
            for c in range(GRID):
                y0, x0 = int(cy - side / 2 + r * cell), int(cx - side / 2 + c * cell)
                y1, x1 = max(y0 + 1, int(y0 + cell)), max(x0 + 1, int(x0 + cell))
                y0, x0 = max(y0, 0), max(x0, 0)
                m = mask[y0:y1, x0:x1]
                if m.size and m.mean() >= .5:
                    out[r, c, :3] = np.median(full[y0:y1, x0:x1][m], axis=0)
                    out[r, c, 3] = 255
        image = Image.fromarray(out, "RGBA")
        image.save(OUT / (path.stem[5:] + ".png"))
        big = image.resize((96, 96), Image.NEAREST)
        sheet.paste(big, (i % W * 104 + 4, i // W * 104 + 4), big)
    sheet.save(OUT / "_conferencia.png")
    print(len(files), "itens")


main()
