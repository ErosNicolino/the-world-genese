"""Postos das criaturas do vale: Data/Population/creature_posts.json (o Godot só é lido).

python Tools/ExtractCreaturePosts.py

- os postos fixos da área do vale (data/areas.json: espécie, nível, x/z em metros);
- o chefe da Caldeira (CalderaBiome.Center = (170, 110), nível 12, como ValleyPopulation.cs);
- os limites da área (dentro dela só nascem os postos fixos e os animais; fora, as criaturas
  selvagens sorteadas por quadra de 72 m, como ValleyPopulation.GenerateRegion).
Conversão: Godot x -> Unreal X, Godot z -> Unreal Y, metros -> cm.
"""
import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
GODOT = ROOT.parents[1] / "the-world-geneses" / "data"
TARGET = ROOT / "Data" / "Population" / "creature_posts.json"


def main():
    areas = json.loads((GODOT / "areas.json").read_text(encoding="utf-8"))["areas"]
    vale = areas["vale_santuario"]
    posts = []
    for c in vale.get("creatures", []):
        posts.append({"species": c["species"], "level": int(c.get("level", 1)), "at_cm": [round(c["x"] * 100, 1), round(c["z"] * 100, 1)]})
    posts.append({"species": "RuneColossus", "level": 12, "at_cm": [17000.0, 11000.0], "boss": True})
    b = vale["bounds"]
    data = {
        "_doc": "Gerado por Tools/ExtractCreaturePosts.py a partir do Godot. Postos fixos do vale e o chefe da Caldeira; area_cm = limites da área feita à mão.",
        "area_cm": [b["x_min"] * 100, b["z_min"] * 100, b["x_max"] * 100, b["z_max"] * 100],
        "town_cm": [0, 1500], "town_radius_cm": 9000,
        "posts": posts,
    }
    TARGET.write_text(json.dumps(data, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print("CREATURE_POSTS", len(posts))


if __name__ == "__main__":
    main()
