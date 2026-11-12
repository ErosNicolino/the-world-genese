"""Referencial de cada arma na mão do herói: Data/VortexWeapons/frames.json.

python Tools/WeaponFrames.py

Para as armas novas feitas no Blender (Tools/Blender/BuildVortexWeapons.py): a mesma conta do
ConvertGodotWeapons.py, em matrizes 4x4 no espaço do glTF (Y para cima):
- right: pegada (o ângulo "grip" do Godot) · escala do herói -> W_<tipo>.glb no osso Socket_Weapon_R;
- left (armas de par): espelho · soquete · pegada · espelho · escala -> W_<tipo>_L.glb no osso Hand_L.
O modelo é feito no referencial do item do Godot (Assets/Equipment/Weapons/<item>.glb: a pegada na
origem e o comprimento no eixo Y), então a arma nova fica na mão como a do Godot.
"""
import json
import pathlib

import numpy as np

from ConvertGodotAdventurer import SCALE, load_glb
from ConvertGodotWeapons import grip_matrix, node_matrix

ROOT = pathlib.Path(__file__).resolve().parents[1]
GODOT = ROOT.parent.parent / "the-world-geneses"


def main():
    weapons = json.loads((ROOT / "Data" / "Rpg" / "weapons.json").read_text(encoding="utf-8"))["types"]
    hero, _ = load_glb(GODOT / "Art" / "Adventurer" / "Adventurer.glb")
    socket = node_matrix(next(n for n in hero["nodes"] if n.get("name") == "Socket_Weapon_R"))
    mirror = np.diag([-1.0, 1.0, 1.0, 1.0])
    scale = np.diag([SCALE, SCALE, SCALE, 1.0])
    frames = {}
    for w in weapons:
        if not w["item"]:
            continue
        grip = grip_matrix(w["grip"])
        entry = {"item": w["item"], "name": w["name"], "grip": w["grip"], "paired": bool(w["paired"]),
                 "right": (scale @ grip).tolist()}
        if w["paired"]:
            entry["left"] = (scale @ mirror @ socket @ grip @ mirror).tolist()
        if w["id"] == "espada_escudo":
            # o escudo no antebraço esquerdo: de frente para fora, sem a inclinação da pegada
            entry["shield"] = (scale @ mirror @ socket @ mirror).tolist()
        frames[w["id"]] = entry
    out = ROOT / "Data" / "VortexWeapons"
    out.mkdir(parents=True, exist_ok=True)
    (out / "frames.json").write_text(json.dumps({"scale": SCALE, "weapons": frames}, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print("WEAPON_FRAMES", len(frames))


if __name__ == "__main__":
    main()
