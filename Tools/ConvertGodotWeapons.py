"""Modelos das 25 armas do Godot (somente leitura) prontos para a mão do herói na Unreal.

python Tools/ConvertGodotWeapons.py

Para cada tipo em Data/Rpg/weapons.json (Tools/BuildWeaponData.py), lê
the-world-geneses/Assets/Equipment/Weapons/<item>.glb (peças do Blender, cada uma com um material de cor) e
escreve Data/GodotWeapons/Models/W_<tipo>.glb:
- todas as peças juntas numa malha só, a cor de cada material no vértice (COLOR_0; o alfa guarda o brilho:
  1 = sem brilho, menos que 1 = emissivo, para runas, cristais e gemas);
- já na pegada do Godot (WeaponDefinition.cs: o eixo comprido para a frente do soquete, inclinado
  "grip" graus em X) e na escala do herói (a mesma do ConvertGodotAdventurer): presa no osso
  Socket_Weapon_R sem nenhum ajuste, a arma fica na mão como no Godot;
- armas de par (adagas, manoplas, machadinhas, chakrams) também ganham W_<tipo>_L.glb: a cópia
  espelhada para o osso Hand_L (AdventurerEquipment.AddPairedCopy do Godot).
"""
import json
import math
import pathlib
import struct

import numpy as np

from ConvertGodotAdventurer import SCALE, load_glb, read_accessor, quat_matrix

ROOT = pathlib.Path(__file__).resolve().parents[1]
GODOT = ROOT.parent.parent / "the-world-geneses"
OUT = ROOT / "Data" / "GodotWeapons" / "Models"


def node_matrix(node):
    m = np.eye(4)
    if "matrix" in node:
        return np.array(node["matrix"], dtype=np.float64).reshape(4, 4).T
    s = np.diag(node.get("scale", [1, 1, 1]) + [1])
    r = np.eye(4)
    if "rotation" in node:
        r[:3, :3] = quat_matrix(node["rotation"])
    t = np.eye(4)
    t[:3, 3] = node.get("translation", [0, 0, 0])
    return t @ r @ s


def grip_matrix(pitch):
    """Godot: Basis(Right, pitch) * Basis(Up, Back, Right) (colunas)."""
    if pitch is None:
        return np.eye(4)
    m = np.eye(4)
    m[:3, :3] = np.array([[0, 0, 1], [1, 0, 0], [0, 1, 0]], dtype=np.float64)
    a = math.radians(pitch)
    rx = np.eye(4)
    rx[1, 1], rx[1, 2], rx[2, 1], rx[2, 2] = math.cos(a), -math.sin(a), math.sin(a), math.cos(a)
    return rx @ m


def flatten(path):
    """Todas as malhas do .glb no espaço da cena, com a cor (e o brilho) do material de cada uma."""
    gltf, blob = load_glb(path)
    positions, normals, colors, indices = [], [], [], []
    count = 0

    def visit(index, parent):
        nonlocal count
        node = gltf["nodes"][index]
        world = parent @ node_matrix(node)
        if "mesh" in node:
            for prim in gltf["meshes"][node["mesh"]]["primitives"]:
                pos = read_accessor(gltf, blob, prim["attributes"]["POSITION"]).astype(np.float64)
                nrm = read_accessor(gltf, blob, prim["attributes"]["NORMAL"]).astype(np.float64) if "NORMAL" in prim["attributes"] \
                    else np.tile([0, 1, 0], (len(pos), 1))
                idx = read_accessor(gltf, blob, prim["indices"]).astype(np.uint32) if "indices" in prim else np.arange(len(pos), dtype=np.uint32)
                mat = gltf["materials"][prim["material"]] if "material" in prim else {}
                base = mat.get("pbrMetallicRoughness", {}).get("baseColorFactor", [.8, .8, .8, 1])
                emissive = mat.get("emissiveFactor", [0, 0, 0])
                glow = max(emissive) if emissive else 0.0
                strength = mat.get("extensions", {}).get("KHR_materials_emissive_strength", {}).get("emissiveStrength", 1.0)
                glow = min(1.0, glow * strength / 3.0)
                color = [max(base[0], emissive[0]) if glow else base[0], max(base[1], emissive[1]) if glow else base[1],
                         max(base[2], emissive[2]) if glow else base[2], 1.0 - .9 * glow]
                hom = np.c_[pos, np.ones(len(pos))] @ world.T
                positions.append(hom[:, :3])
                normals.append(nrm @ np.linalg.inv(world[:3, :3]))
                colors.append(np.tile(color, (len(pos), 1)))
                indices.append(idx + count)
                count += len(pos)
        for child in node.get("children", []):
            visit(child, world)

    scene = gltf["scenes"][gltf.get("scene", 0)]
    for root in scene["nodes"]:
        visit(root, np.eye(4))
    return np.concatenate(positions), np.concatenate(normals), np.concatenate(colors), np.concatenate(indices)


def write(path, pos, nrm, col, idx):
    nrm = nrm / np.maximum(np.linalg.norm(nrm, axis=1, keepdims=True), 1e-8)
    blob = bytearray()
    views, accessors = [], []

    def add(array, component, kind, target, minmax=False):
        data = array.tobytes()
        while len(blob) % 4:
            blob.append(0)
        views.append({"buffer": 0, "byteOffset": len(blob), "byteLength": len(data), "target": target})
        blob.extend(data)
        acc = {"bufferView": len(views) - 1, "componentType": component, "count": len(array), "type": kind}
        if minmax:
            acc["min"] = array.min(axis=0).tolist()
            acc["max"] = array.max(axis=0).tolist()
        accessors.append(acc)
        return len(accessors) - 1

    p = add(pos.astype(np.float32), 5126, "VEC3", 34962, True)
    n = add(nrm.astype(np.float32), 5126, "VEC3", 34962)
    c = add(col.astype(np.float32), 5126, "VEC4", 34962)
    i = add(idx.astype(np.uint32), 5125, "SCALAR", 34963)
    gltf = {"asset": {"version": "2.0", "generator": "Vortex ConvertGodotWeapons"}, "scene": 0, "scenes": [{"nodes": [0]}],
            "nodes": [{"name": path.stem, "mesh": 0}],
            "meshes": [{"name": path.stem, "primitives": [{"attributes": {"POSITION": p, "NORMAL": n, "COLOR_0": c}, "indices": i, "material": 0}]}],
            "materials": [{"name": "Weapon", "pbrMetallicRoughness": {"baseColorFactor": [1, 1, 1, 1], "metallicFactor": 0, "roughnessFactor": .6}, "doubleSided": True}],
            "accessors": accessors, "bufferViews": views, "buffers": [{"byteLength": len(blob)}]}
    js = json.dumps(gltf, separators=(",", ":")).encode()
    js += b" " * ((-len(js)) % 4)
    while len(blob) % 4:
        blob.append(0)
    with open(path, "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(blob)))
        f.write(struct.pack("<II", len(js), 0x4E4F534A) + js)
        f.write(struct.pack("<II", len(blob), 0x004E4942) + bytes(blob))


def main():
    weapons = json.loads((ROOT / "Data" / "Rpg" / "weapons.json").read_text(encoding="utf-8"))["types"]
    hero, _ = load_glb(GODOT / "Art" / "Adventurer" / "Adventurer.glb")
    socket = node_matrix(next(n for n in hero["nodes"] if n.get("name") == "Socket_Weapon_R"))
    mirror = np.diag([-1.0, 1.0, 1.0, 1.0])
    OUT.mkdir(parents=True, exist_ok=True)
    done = []
    for w in weapons:
        if not w["item"]:
            continue
        pos, nrm, col, idx = flatten(GODOT / "Assets" / "Equipment" / "Weapons" / (w["item"] + ".glb"))
        grip = grip_matrix(w["grip"])
        hom = np.c_[pos, np.ones(len(pos))]
        right = (hom @ grip.T)[:, :3] * SCALE
        write(OUT / f"W_{w['id']}.glb", right, nrm @ grip[:3, :3].T, col, idx)
        if w["paired"]:
            # cópia na mão esquerda: espelho · (soquete · pegada) · espelho, no espaço do osso Hand_L
            left_m = mirror @ socket @ grip @ mirror
            left = (hom @ left_m.T)[:, :3]
            left[:, :] *= SCALE
            write(OUT / f"W_{w['id']}_L.glb", left, nrm @ left_m[:3, :3].T, col, idx)
        done.append(f"{w['id']}({len(pos)})")
    print("WEAPON_MODELS", len(done), " ".join(done))


if __name__ == "__main__":
    main()
