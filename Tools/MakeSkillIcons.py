"""Ícones das classes e habilidades recortados de novo das 4 folhas do Godot (somente leitura).

python Tools/MakeSkillIcons.py      (depois: Tools/ImportItemIcons.py com o editor fechado)

As folhas (the-world-geneses/docs/icones_de_habilidades_armas_etc/<A|B|C|D>_Classes_Icones_usar.png, 1254 x 1254,
8 x 8) não estão alinhadas em linhas retas: o recorte antigo (ui/art/icons/atlas, uma grade fixa) saía torto, com a
moldura deslocada e um pedaço do ícone vizinho embaixo. Aqui cada ícone é achado pela própria moldura dentro da sua
casa e recortado por dentro dela (a barra de habilidades desenha a sua). Saída: Data/UI/SkillIcons/atlas_<A01..D64>.png
(192 x 192), uma folha de conferência (_folha_<X>.png) e as correções de nome e ícone (Data/Rpg/class_skill_fixes.json)
aplicadas no rpg.json, sem gerar o resto de novo.
"""
import json
import pathlib
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = pathlib.Path(__file__).resolve().parents[1]
GODOT = ROOT.parent.parent / "the-world-geneses"
SHEETS = GODOT / "docs" / "icones_de_habilidades_armas_etc"
OUT = ROOT / "Data" / "UI" / "SkillIcons"
SIZE = 192
INSET = .08   # tira a moldura dourada e o que vaza da casa vizinha


def crop_sheet(letter):
    src = Image.open(SHEETS / f"{letter}_Classes_Icones_usar.png").convert("RGB")
    gray = np.asarray(src.convert("L")).astype(float)
    pitch = src.width / 8
    preview = Image.new("RGB", (8 * 100, 8 * 100), (20, 20, 24))
    # a última linha da folha A tem 9 molduras mais estreitas: as casas dela saem das próprias molduras
    last_row = None
    if letter == "A":
        strip = gray[int(7 * pitch) + 15:src.height - 15].mean(0) > 30
        runs, start = [], None
        for x, lit_x in enumerate(list(strip) + [False]):
            if lit_x and start is None:
                start = x
            elif not lit_x and start is not None:
                if x - start > 80:
                    runs.append((start - 6, x + 6))
                start = None
        last_row = runs[:8]
    for r in range(8):
        for c in range(8):
            x0, y0, x1, y1 = int(c * pitch), int(r * pitch), int((c + 1) * pitch), int((r + 1) * pitch)
            if r == 7 and last_row and c < len(last_row):
                x0, x1 = last_row[c]
            lit = gray[y0:y1, x0:x1] > 18
            rows = np.where(lit.mean(1) > .45)[0]
            cols = np.where(lit.mean(0) > .45)[0]
            if len(rows) and len(cols) and rows[-1] - rows[0] > 105 and cols[-1] - cols[0] > 105:
                top, bottom, left, right = y0 + rows[0], y0 + rows[-1], x0 + cols[0], x0 + cols[-1]
            else:   # moldura não achada: o meio da casa
                top, bottom, left, right = y0 + 8, y1 - 8, x0 + 8, x1 - 8
            # a casa às vezes pega a faixa preta entre os ícones e a moldura do vizinho: corta na faixa preta
            # (3 linhas seguidas quase pretas) quando ela aparece nos últimos 55 px de cada lado
            def lines(a, b, step, value):
                dark = 0
                for i in range(a, b, step):
                    if value(i) < 24:
                        dark += 1
                        if dark >= 3:
                            edge = i   # passa a faixa preta até a moldura do próprio ícone
                            while value(edge) < 24:
                                edge += step
                            return edge
                    else:
                        dark = 0
                return None
            row = lambda y: gray[y, left:right].mean()
            col = lambda x: gray[top:bottom, x].mean()
            bottom = lines(bottom, bottom - 55, -1, row) or bottom
            top = lines(top, top + 55, 1, row) or top
            right = lines(right, right - 55, -1, col) or right
            left = lines(left, left + 55, 1, col) or left
            side = min(bottom - top, right - left) + 1
            cut = side * INSET
            cx, cy = (left + right + 1) / 2, (top + bottom + 1) / 2
            half = side / 2 - cut
            icon = src.crop((round(cx - half), round(cy - half), round(cx + half), round(cy + half))).resize((SIZE, SIZE), Image.LANCZOS)
            icon.save(OUT / f"atlas_{letter}{r * 8 + c + 1:02d}.png")
            preview.paste(icon.resize((96, 96), Image.LANCZOS), (c * 100 + 2, r * 100 + 2))
    preview.save(OUT / f"_folha_{letter}.png")


def apply_fixes():
    sys.path.insert(0, str(ROOT / "Tools"))
    from BuildRpgData import apply_skill_fixes
    path = ROOT / "Data" / "Rpg" / "rpg.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    changed = apply_skill_fixes(data["classSkills"])
    path.write_text(json.dumps(data, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    icons_path = ROOT / "Data" / "Rpg" / "icons.txt"
    icons = {line.strip() for line in icons_path.read_text(encoding="utf-8").splitlines() if line.strip()}
    icons |= {sk["icon"] for group in data["classSkills"]["classes"].values() for sk in group if sk.get("icon")}
    icons_path.write_text("\n".join(sorted(icons)) + "\n", encoding="utf-8")
    return changed, data


def skill_sheet(data):
    """Conferência: cada habilidade com o ícone novo, o nome, o tipo e o elemento."""
    items = [sk for group in data["classSkills"]["classes"].values() for sk in group]
    font = ImageFont.truetype("arial.ttf", 12)
    for page in range(0, len(items), 132):
        part = items[page:page + 132]
        sheet = Image.new("RGB", (8 * 230, (len(part) + 7) // 8 * 74), (30, 30, 36))
        draw = ImageDraw.Draw(sheet)
        for i, sk in enumerate(part):
            x, y = i % 8 * 230, i // 8 * 74
            icon = OUT / (sk["icon"] + ".png")
            if icon.exists():
                sheet.paste(Image.open(icon).resize((64, 64), Image.LANCZOS), (x + 4, y + 4))
            draw.text((x + 72, y + 6), sk["name"][:26], fill="white", font=font)
            draw.text((x + 72, y + 24), f"{sk['kind']} / {sk.get('element', '')}", fill=(180, 200, 255), font=font)
            draw.text((x + 72, y + 42), sk["icon"].replace("atlas_", ""), fill=(150, 150, 150), font=font)
        sheet.save(OUT / f"_habilidades_{page // 132 + 1}.png")


if __name__ == "__main__":
    OUT.mkdir(parents=True, exist_ok=True)
    for letter in "ABCD":
        crop_sheet(letter)
    changed, data = apply_fixes()
    skill_sheet(data)
    print("SKILL_ICONS 256 recortados,", changed, "mudanças no rpg.json")
