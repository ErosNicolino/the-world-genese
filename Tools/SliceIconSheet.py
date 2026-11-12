"""Recorta as folhas de ícones geradas no ChatGPT (8 por imagem, grade 4 x 2) em ícones soltos.

python Tools/SliceIconSheet.py            (todas as folhas que estiverem em Data/UI/SkillIcons/Folhas)
python Tools/SliceIconSheet.py H01 C03    (só essas)

As folhas e os prompts estão em Docs/04_Interface/PROMPTS_ICONES_HABILIDADES.md. Salve cada imagem gerada com o
nome da folha (H01.png ... H33.png para as habilidades, C01.png ... C09.png para as classes) em
Data/UI/SkillIcons/Folhas. Data/UI/SkillIcons/Folhas/folhas.json diz qual habilidade ou classe vai em cada casa.
Cada quadrado é achado pelo próprio desenho (o que difere do fundo escuro entre eles), então a folha não precisa
sair perfeitamente alinhada. Saída: Data/UI/SkillIcons/Novos/<skill_id|class_id>.png (256 x 256) e uma folha de
conferência _conferencia_<folha>.png com o nome de cada ícone embaixo.
Os ícones novos ainda não entram no jogo sozinhos: falta apontar o rpg.json para eles e importar.
"""
import json
import pathlib
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = pathlib.Path(__file__).resolve().parents[1]
SHEETS = ROOT / "Data" / "UI" / "SkillIcons" / "Folhas"
OUT = ROOT / "Data" / "UI" / "SkillIcons" / "Novos"
SIZE = 256
COLS, ROWS = 4, 2


def runs(profile, threshold, min_len):
    """Trechos seguidos acima do limite (as faixas ocupadas por quadrados)."""
    out, start = [], None
    for i, on in enumerate(list(profile > threshold) + [False]):
        if on and start is None:
            start = i
        elif not on and start is not None:
            if i - start >= min_len:
                out.append((start, i))
            start = None
    return out


def find_tiles(src):
    """Os quadrados da folha, em ordem de leitura. O ChatGPT não respeita a grade exata (a 2ª linha às vezes
    começa antes da metade), então as linhas e colunas saem do próprio desenho: o que difere do fundo escuro."""
    a = np.asarray(src).astype(int)
    k = 12
    corners = np.concatenate([a[:k, :k].reshape(-1, 3), a[:k, -k:].reshape(-1, 3), a[-k:, :k].reshape(-1, 3), a[-k:, -k:].reshape(-1, 3)])
    mask = np.abs(a - np.median(corners, axis=0)).sum(axis=2) > 40
    tiles = []
    for y0, y1 in runs(mask.mean(axis=1), .12, src.height // 8):
        for x0, x1 in runs(mask[y0:y1].mean(axis=0), .35, src.width // 12):
            side = min(x1 - x0, y1 - y0)
            cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
            inset = side * .015   # tira a borda escura que algumas folhas desenham em volta do quadrado
            tiles.append((int(cx - side / 2 + inset), int(cy - side / 2 + inset), int(cx + side / 2 - inset), int(cy + side / 2 - inset)))
    return tiles


def slice_sheet(sheet_id, names):
    src = Image.open(SHEETS / f"{sheet_id}.png").convert("RGB")
    preview = Image.new("RGB", (COLS * 140, ROWS * 160), (16, 24, 38))
    draw = ImageDraw.Draw(preview)
    try:
        font = ImageFont.truetype("segoeui.ttf", 11)
    except OSError:
        font = ImageFont.load_default()
    tiles = find_tiles(src)
    if len(tiles) != len(names):
        print(f"  {sheet_id}: achei {len(tiles)} quadrados para {len(names)} ícones; confira a folha")
    for i, (name, box) in enumerate(zip(names, tiles)):
        col, row = i % COLS, i // COLS
        icon = src.crop(box).resize((SIZE, SIZE), Image.LANCZOS)
        icon.save(OUT / f"{name}.png")
        preview.paste(icon.resize((120, 120)), (col * 140 + 10, row * 160 + 10))
        draw.text((col * 140 + 10, row * 160 + 134), name[:22], fill=(230, 220, 190), font=font)
    preview.save(OUT / f"_conferencia_{sheet_id}.png")
    print(f"{sheet_id}: {len(names)} ícones")


def main():
    plan = json.loads((SHEETS / "folhas.json").read_text(encoding="utf-8"))
    OUT.mkdir(parents=True, exist_ok=True)
    wanted = sys.argv[1:] or [s for s in plan if (SHEETS / f"{s}.png").exists()]
    if not wanted:
        print(f"Nenhuma folha encontrada em {SHEETS}")
    for sheet_id in wanted:
        if sheet_id not in plan:
            print(f"{sheet_id}: não está em folhas.json")
        elif not (SHEETS / f"{sheet_id}.png").exists():
            print(f"{sheet_id}: falta {sheet_id}.png")
        else:
            slice_sheet(sheet_id, [entry["id"] for entry in plan[sheet_id]])


if __name__ == "__main__":
    main()
