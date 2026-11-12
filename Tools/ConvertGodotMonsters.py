"""Converte as criaturas do Godot (Art/Monsters/<Espécie>.glb, peças rígidas animadas por nó) em
malhas com esqueleto para a Unreal, como o ConvertGodotWolf.py faz com o lobo.

Lê o Godot só para ler. Rode: python -B Tools/ConvertGodotMonsters.py [Espécie ...]
Saída: Data/GodotMonsters/<Espécie>/<Espécie>_Skinned.glb, <Espécie>_Palette.png e monsters.json
(escala, altura, título e a cor do brilho de cada espécie). Depois: Tools/ImportMonsters.py no editor.

- cada peça vira um osso (o nó do Godot) com peso 1, na escala do monsters.json (gltf_scale);
- as cores dos voxels vão para uma paleta 64x64 (sRGB) e cada vértice aponta para o seu texel;
- material 0 = corpo (fosco), material 1 = brilho (olhos, cristais, runas; emissiveFactor do Godot);
- os clipes do Blockbench (Idle, Walk, Attack, Hit, Death e os golpes próprios) vão junto.
"""
import json
import os
import pathlib
import struct
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ConvertGodotAdventurer import Writer, load_glb, local_matrix, read_accessor  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parents[1]
GODOT = ROOT.parent.parent / "the-world-geneses" / "Art" / "Monsters"
OUT = ROOT / "Data" / "GodotMonsters"
PALETTE_SIZE = 64
# o lobo tem conversão própria (ConvertGodotWolf.py) e já está no jogo
SPECIES = ["Slime", "Mushroom", "Golem", "Bat", "Rabbit", "Deer", "GlassScorpion", "RootTitan", "Frog", "Goblin",
           "RuneColossus", "Horse_Baio", "Horse_Tordilho", "Horse_Malhado", "Horse_Negro", "Horse_Palomino"]


def linear_to_srgb8(color):
    c = np.clip(color, 0.0, 1.0)
    srgb = np.where(c <= 0.0031308, c * 12.92, 1.055 * np.power(c, 1 / 2.4) - 0.055)
    return np.round(srgb * 255).astype(np.uint8)


def convert(species, scale):
    gltf, blob = load_glb(str(GODOT / f"{species}.glb"))
    nodes = gltf["nodes"]
    roots = gltf["scenes"][gltf.get("scene", 0)]["nodes"]
    parent = {child: i for i, node in enumerate(nodes) for child in node.get("children", [])}
    # várias raízes: um osso "Root" novo por cima de todas
    synthetic = len(roots) > 1
    offset = 1 if synthetic else 0
    writer = Writer()

    def global_matrix(index):
        transform = local_matrix(nodes[index])
        while index in parent:
            index = parent[index]
            transform = local_matrix(nodes[index]) @ transform
        return transform

    poses = [global_matrix(i) for i in range(len(nodes))]
    for pose in poses:
        pose[:3, 3] *= scale
    if synthetic:
        poses = [np.eye(4)] + poses
    inverse = np.asarray([np.linalg.inv(pose).T.reshape(16) for pose in poses], np.float32)
    materials = gltf.get("materials") or [{}]
    primitives = []
    total_vertices = 0
    palette = {}
    lowest = 1e9
    highest = -1e9
    for material in range(len(materials)):
        positions, normals, colors, joints, indices = [], [], [], [], []
        base = 0
        for i, node in enumerate(nodes):
            if "mesh" not in node:
                continue
            transform = global_matrix(i)
            # peças guardadas encolhidas no Godot (a pedra do golem só cresce no arremesso): a Unreal
            # ignora essa escala na pose de repouso e a peça aparecia inteira; o jogo usa o projétil
            if np.linalg.norm(transform[:3, 0]) < .1:
                continue
            normal_transform = np.linalg.inv(transform[:3, :3]).T
            for primitive in gltf["meshes"][node["mesh"]]["primitives"]:
                if primitive.get("material", 0) != material:
                    continue
                attrs = primitive["attributes"]
                local = read_accessor(gltf, blob, attrs["POSITION"]).astype(np.float64)
                world = ((transform[:3, :3] @ local.T).T + transform[:3, 3]) * scale
                positions.append(world)
                normal = (normal_transform @ read_accessor(gltf, blob, attrs["NORMAL"]).T).T
                normals.append(normal / np.maximum(np.linalg.norm(normal, axis=1, keepdims=True), 1e-9))
                if "COLOR_0" in attrs:
                    color = read_accessor(gltf, blob, attrs["COLOR_0"]).astype(np.float32)
                    if color.dtype != np.float32 or color.max() > 1.0:
                        color = color / 255.0
                else:
                    color = np.full((len(local), 4), .7, np.float32)
                if color.shape[1] == 3:
                    color = np.column_stack((color, np.ones(len(color), np.float32)))
                colors.append(color)
                joints.append(np.full(len(local), i + offset, np.uint16))
                source_indices = read_accessor(gltf, blob, primitive["indices"]).astype(np.uint32)
                indices.append(source_indices + base)
                base += len(local)
        if not positions:
            continue
        pos = np.concatenate(positions).astype(np.float32)
        lowest = min(lowest, float(pos[:, 1].min()))
        highest = max(highest, float(pos[:, 1].max()))
        count = len(pos)
        total_vertices += count
        joint_ids = np.zeros((count, 4), np.uint16)
        joint_ids[:, 0] = np.concatenate(joints)
        weights = np.zeros((count, 4), np.float32)
        weights[:, 0] = 1
        srgb = linear_to_srgb8(np.concatenate(colors)[:, :3])
        uv = np.zeros((count, 2), np.float32)
        for v, key in enumerate(map(tuple, srgb)):
            index = palette.setdefault(key, len(palette))
            if index >= PALETTE_SIZE * PALETTE_SIZE:
                raise RuntimeError(f"paleta de {species} cheia")
            uv[v] = ((index % PALETTE_SIZE + 0.5) / PALETTE_SIZE, (index // PALETTE_SIZE + 0.5) / PALETTE_SIZE)
        attributes = {
            "POSITION": writer.add(pos, 5126, "VEC3", 34962, minmax=True),
            "NORMAL": writer.add(np.concatenate(normals).astype(np.float32), 5126, "VEC3", 34962),
            "COLOR_0": writer.add(np.concatenate(colors).astype(np.float32), 5126, "VEC4", 34962),
            "JOINTS_0": writer.add(joint_ids, 5123, "VEC4", 34962),
            "WEIGHTS_0": writer.add(weights, 5126, "VEC4", 34962),
            "TEXCOORD_0": writer.add(uv, 5126, "VEC2", 34962),
        }
        primitives.append({"attributes": attributes, "indices": writer.add(np.concatenate(indices), 5125, "SCALAR", 34963),
                           "material": material})

    new_nodes = []
    if synthetic:
        new_nodes.append({"name": "Root", "children": [r + 1 for r in roots]})
    for node in nodes:
        entry = {"name": node.get("name", f"Bone_{len(new_nodes)}"),
                 "translation": (np.asarray(node.get("translation", [0, 0, 0])) * scale).tolist()}
        for key in ("rotation", "scale"):
            if key in node:
                entry[key] = node[key]
        if "children" in node:
            entry["children"] = [c + offset for c in node["children"]]
        new_nodes.append(entry)
    mesh_node = len(new_nodes)
    name = f"{species}_Skinned"
    new_nodes.append({"name": name, "mesh": 0, "skin": 0})
    clips = []
    for animation in gltf.get("animations", []):
        samplers, channels = [], []
        for channel in animation["channels"]:
            sampler = animation["samplers"][channel["sampler"]]
            path = channel["target"]["path"]
            if path not in ("translation", "rotation", "scale"):
                continue
            times = read_accessor(gltf, blob, sampler["input"]).astype(np.float32)
            values = read_accessor(gltf, blob, sampler["output"]).astype(np.float32)
            if path == "translation":
                values = values * scale
            samplers.append({"input": writer.add(times, 5126, "SCALAR", minmax=True),
                             "output": writer.add(values, 5126, {"translation": "VEC3", "rotation": "VEC4", "scale": "VEC3"}[path]),
                             "interpolation": sampler.get("interpolation", "LINEAR")})
            channels.append({"sampler": len(samplers) - 1, "target": {"node": channel["target"]["node"] + offset, "path": path}})
        clips.append({"name": animation["name"], "samplers": samplers, "channels": channels})

    skeleton_root = 0 if synthetic else roots[0]
    result = {"asset": {"version": "2.0", "generator": "Vortex ConvertGodotMonsters"},
              "scene": 0, "scenes": [{"nodes": [0 if synthetic else roots[0], mesh_node]}], "nodes": new_nodes,
              "meshes": [{"name": name, "primitives": primitives}],
              "skins": [{"joints": list(range(len(poses))), "inverseBindMatrices": writer.add(inverse, 5126, "MAT4"),
                         "skeleton": skeleton_root}],
              "materials": [{"name": f"{species}_{'Glow' if i else 'Body'}", "pbrMetallicRoughness": m.get("pbrMetallicRoughness", {})}
                            for i, m in enumerate(materials)],
              "animations": clips,
              "accessors": writer.accessors, "bufferViews": writer.views,
              "buffers": [{"byteLength": len(writer.blob)}]}
    encoded = json.dumps(result, separators=(",", ":")).encode()
    encoded += b" " * ((-len(encoded)) % 4)
    while len(writer.blob) % 4:
        writer.blob.append(0)
    folder = OUT / species
    folder.mkdir(parents=True, exist_ok=True)
    with open(folder / f"{name}.glb", "wb") as file:
        file.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(encoded) + 8 + len(writer.blob)))
        file.write(struct.pack("<II", len(encoded), 0x4E4F534A) + encoded)
        file.write(struct.pack("<II", len(writer.blob), 0x004E4942) + writer.blob)
    image = np.zeros((PALETTE_SIZE, PALETTE_SIZE, 3), np.uint8)
    for key, index in palette.items():
        image[index // PALETTE_SIZE, index % PALETTE_SIZE] = key
    Image.fromarray(image, "RGB").save(folder / f"{species}_Palette.png")
    glow = materials[1].get("emissiveFactor", [0, 0, 0]) if len(materials) > 1 else [0, 0, 0]
    print(f"MONSTER_CONVERTED {species} vertices={total_vertices} colors={len(palette)} bones={len(poses)} "
          f"altura={highest - lowest:.2f}m clips={[c['name'] for c in clips]}")
    return {"clips": [c["name"] for c in clips], "glow": glow, "height_m": round(highest - lowest, 3),
            "bottom_m": round(lowest, 3), "materials": len(materials)}


def main():
    info = json.loads((GODOT / "monsters.json").read_text(encoding="utf-8"))
    wanted = sys.argv[1:] or SPECIES
    catalog_path = OUT / "monsters.json"
    catalog = json.loads(catalog_path.read_text(encoding="utf-8")) if catalog_path.exists() else {}
    for species in wanted:
        entry = info.get(species, {})
        # cavalos: o GLB já vem em metros (Mount.ModelScale = 1 no Godot)
        scale = 1.0 if species.startswith("Horse_") else float(entry.get("gltf_scale", .56))
        entry = {**entry, "gltf_scale": scale}
        result = convert(species, scale)
        catalog[species] = {"title": entry.get("title", species), "scale": entry.get("gltf_scale", .56),
                            "godot_height_m": entry.get("height_m", 1.0), **result}
    OUT.mkdir(parents=True, exist_ok=True)
    catalog_path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
