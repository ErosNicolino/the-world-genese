"""Peças de cada raça (rw_*, undead_rw_*) do Godot como peças modulares do Unreal.

Uso: python Tools/ConvertGodotRacePieces.py

No Godot elas ficam em Art/Adventurer/RacePieces.glb (somente leitura), sem esqueleto: cada grupo
"<peça>__<osso>" fica embaixo de um nó com o nome do osso, com a malha no espaço desse osso. Aqui
os nós do RacePieces são enxertados, em memória, nos ossos de mesmo nome do Adventurer.glb, e
cada peça sai pelo mesmo conversor das outras (ConvertGodotAdventurer.convert): mesmo esqueleto,
mesma escala. A paleta do RacePieces contém a do herói inteira e mais os tons das raças; o jogo
usa ela como paleta única (Data/GodotCharacter/palette.png, ver BuildLookCatalog.py).
Acrescenta as peças ao manifest.json do guarda-roupa.
"""
import copy
import json
from pathlib import Path

from ConvertGodotAdventurer import convert, load_glb

ROOT = Path(__file__).resolve().parents[1]
GODOT = ROOT.parents[1] / "the-world-geneses" / "Art" / "Adventurer"
OUT = ROOT / "Data" / "GodotCharacter" / "Wardrobe"


def graft(base, base_blob, extra, extra_blob):
    gltf = copy.deepcopy(base)
    pad = (-len(base_blob)) % 4
    blob = bytes(base_blob) + b"\0" * pad + bytes(extra_blob)
    offset = len(base_blob) + pad
    view_base, acc_base, mesh_base, node_base = len(gltf["bufferViews"]), len(gltf["accessors"]), len(gltf["meshes"]), len(gltf["nodes"])
    for view in extra["bufferViews"]:
        v = dict(view)
        v["byteOffset"] = v.get("byteOffset", 0) + offset
        v["buffer"] = 0
        gltf["bufferViews"].append(v)
    for acc in extra["accessors"]:
        a = dict(acc)
        if "bufferView" in a:
            a["bufferView"] += view_base
        gltf["accessors"].append(a)
    for mesh in extra["meshes"]:
        m = copy.deepcopy(mesh)
        for prim in m["primitives"]:
            prim["attributes"] = {k: v + acc_base for k, v in prim["attributes"].items()}
            if "indices" in prim:
                prim["indices"] += acc_base
            prim.pop("material", None)
        gltf["meshes"].append(m)
    roots = set(extra["scenes"][0]["nodes"])
    for index, node in enumerate(extra["nodes"]):
        n = copy.deepcopy(node)
        if "mesh" in n:
            n["mesh"] += mesh_base
        if "children" in n:
            n["children"] = [c + node_base for c in n["children"]]
        if index in roots:
            # o nó do osso do RacePieces não entra: tinha o mesmo nome do osso do herói ("Head") e o
            # conversor achava ele (sem pai, sem pose) em vez do osso de verdade
            n = {"name": "__racepieces_" + node["name"]}
        gltf["nodes"].append(n)
    gltf["buffers"][0]["byteLength"] = len(blob)
    names = {n.get("name"): i for i, n in enumerate(gltf["nodes"][:node_base])}
    groups = []
    for root in extra["scenes"][0]["nodes"]:
        bone = extra["nodes"][root]["name"]
        if bone not in names:
            print("osso desconhecido", bone)
            continue
        kids = [c + node_base for c in extra["nodes"][root].get("children", [])]
        gltf["nodes"][names[bone]].setdefault("children", []).extend(kids)
        groups += [gltf["nodes"][k]["name"] for k in kids]
    return gltf, blob, set(groups)


def main():
    wardrobe = json.loads((GODOT / "wardrobe.json").read_text(encoding="utf-8"))
    base, base_blob = load_glb(GODOT / "Adventurer.glb")
    extra, extra_blob = load_glb(GODOT / "RacePieces.glb")
    gltf, blob, available = graft(base, base_blob, extra, extra_blob)
    manifest_path = OUT / "manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    made = 0
    for piece_id, piece in sorted(wardrobe["pieces"].items()):
        groups = piece["groups"]
        if not groups or not all(g in available for g in groups):
            continue
        convert(gltf, blob, groups, OUT / f"{piece_id}.glb", [])
        manifest[piece_id] = {"category": piece["category"], "groups": groups}
        made += 1
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"RACE_PIECES_CONVERTED {made}")


if __name__ == "__main__":
    main()
