"""Fase 14: dados mínimos dos postos de NPC, a partir do npcs.json do Godot (somente leitura).

Guarda o ID estável, nome, título, posto, direção, patrulha e a aparência (peças e cores, como no
Godot). Junta os NPCs só do Vortex de Data/Population/npc_extra.json (por exemplo a Julia e o
cachorrinho dela). Falas e diálogos ficam no Godot até as fases de diálogo.

Conversão (mesma do passeio de medição): Godot x -> Unreal X, Godot z -> Unreal Y, metros -> cm.
No Godot a frente de um NPC com giro t é (-sin t, -cos t) em (x, z); no Unreal isso vira o yaw
atan2(-cos t, -sin t).

Uso: python Tools/ExtractNpcPosts.py
"""
import json
import math
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT.parents[1] / "the-world-geneses" / "data" / "npcs.json"
TARGET = ROOT / "Data" / "Population" / "npc_posts.json"


def to_cm(point):
    return [round(float(point[0]) * 100.0, 1), round(float(point[1]) * 100.0, 1)]


def to_yaw(godot_degrees):
    t = math.radians(float(godot_degrees))
    return round(math.degrees(math.atan2(-math.cos(t), -math.sin(t))), 2)


def main():
    source = json.loads(SOURCE.read_text(encoding="utf-8"))
    posts = []
    for npc in source["npcs"]:
        # 02/11: a Juh era para ser a Julia (as missões dela passaram para a Julia, Tools/BuildStoryData.py)
        if npc["id"] == "juh":
            continue
        posts.append({
            "id": npc["id"],
            "name": npc["name"],
            "title": npc.get("title", ""),
            "post_cm": to_cm(npc["position"]),
            "yaw": to_yaw(npc.get("facing", 0.0)),
            "patrol_cm": [to_cm(p) for p in npc.get("patrol", [])],
            "look": npc.get("look", {}),
        })
    extra_path = ROOT / "Data" / "Population" / "npc_extra.json"
    if extra_path.exists():
        for npc in json.loads(extra_path.read_text(encoding="utf-8"))["npcs"]:
            posts.append({
                "id": npc["id"], "name": npc["name"], "title": npc.get("title", ""),
                "post_cm": to_cm(npc["position"]), "yaw": to_yaw(npc.get("facing", 0.0)),
                "patrol_cm": [to_cm(p) for p in npc.get("patrol", [])],
                "look": npc.get("look", {}), "pet": npc.get("pet", ""),
            })
    ids = [p["id"] for p in posts]
    if len(set(ids)) != len(ids):
        raise RuntimeError("IDs repetidos em npcs.json")
    # 10/11: quem já está no arquivo fica como está (o visual de blocos foi escrito à mão e os postos vêm do
    # Tools/Town/BuildVila.py; reescrever apagava os dois). Só os NPCs novos entram.
    doc = {"_doc": "Gerado por Tools/ExtractNpcPosts.py a partir de the-world-geneses/data/npcs.json "
                   "(somente leitura). Unreal: cm, X = x do Godot, Y = z do Godot; yaw em graus.", "version": 1, "npcs": []}
    if TARGET.exists():
        doc = json.loads(TARGET.read_text(encoding="utf-8"))
    have = {n["id"] for n in doc["npcs"]}
    doc["npcs"] += [p for p in posts if p["id"] not in have]
    posts = doc["npcs"]
    TARGET.parent.mkdir(parents=True, exist_ok=True)
    TARGET.write_text(json.dumps(doc, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"VORTEX_NPC_POSTS {len(posts)} -> {TARGET}")


if __name__ == "__main__":
    main()
