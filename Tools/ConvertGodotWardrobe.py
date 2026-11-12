"""Gera peças modulares a partir do catálogo do Godot, sem escrever no projeto Godot.

Uso: python Tools/ConvertGodotWardrobe.py [--all-local]
Por padrão produz o visual inicial e uma alternativa para cada categoria básica.
"""

import argparse
import json
from pathlib import Path

from ConvertGodotAdventurer import convert, load_glb

ROOT = Path(__file__).resolve().parents[1]
GODOT = ROOT.parents[1] / "the-world-geneses" / "Art" / "Adventurer"
OUT = ROOT / "Data" / "GodotCharacter" / "Wardrobe"
PILOT = [
    "face_male", "face_female", "hair_male_1", "hair_male_2", "hair_female_1", "hair_female_2", "eyes_m_calm", "eyes_m_fierce",
    "mouth_small", "mouth_smile", "shirt_tunic", "shirt_punk", "pants_dark",
    "pants_jeans_punk", "boots_laced", "boots_punk", "gloves_leather",
    "gloves_punk", "gear_strap", "gear_belt",
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--all-local", action="store_true", help="todas as peças do Adventurer.glb")
    args = parser.parse_args()
    wardrobe = json.loads((GODOT / "wardrobe.json").read_text(encoding="utf-8"))
    gltf, blob = load_glb(GODOT / "Adventurer.glb")
    available = {node.get("name") for node in gltf["nodes"]}
    pieces = wardrobe["pieces"]
    ids = sorted(pieces) if args.all_local else PILOT
    manifest = {}
    OUT.mkdir(parents=True, exist_ok=True)
    for piece_id in ["core"] + ids:
        groups = sorted(n for n in available if n.startswith("core__")) if piece_id == "core" else pieces[piece_id]["groups"]
        if not groups or any(group not in available for group in groups):
            continue  # grupos rw_* ficam em RacePieces.glb, etapa posterior
        target = OUT / f"{piece_id}.glb"
        convert(gltf, blob, groups, target, [])
        manifest[piece_id] = {"category": "core" if piece_id == "core" else pieces[piece_id]["category"], "groups": groups}
    (OUT / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"WARDROBE_CONVERTED {len(manifest)} peças em {OUT}")


if __name__ == "__main__":
    main()
