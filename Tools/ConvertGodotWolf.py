"""Convert the original rigid-node Wolf.glb to a skinned UE-compatible GLB.

Reads Godot art only. Run: python -B Tools/ConvertGodotWolf.py <Wolf.glb> <output.glb>

As cores de cada voxel também vão para uma paleta (Wolf_Palette.png, ao lado do GLB) e cada
vértice aponta para o seu texel em TEXCOORD_0. Na Unreal as cores de vértice deste modelo
chegavam como um cinza só; a paleta, lida sem filtro, reproduz as cores do Godot.
O Godot usa as cores de vértice como albedo linear (vertex_color_is_srgb = false): a paleta
guarda a mesma cor em sRGB, para uma textura sRGB.
"""
import json
import os
import struct
import sys

import numpy as np
from PIL import Image

from ConvertGodotAdventurer import Writer, load_glb, local_matrix, read_accessor

SCALE = 0.56  # monsters.json: Wolf.gltf_scale; source height ~2.75 m -> 1.54 m
PALETTE_SIZE = 64  # até 4096 cores


def linear_to_srgb8(color):
    c = np.clip(color, 0.0, 1.0)
    srgb = np.where(c <= 0.0031308, c * 12.92, 1.055 * np.power(c, 1 / 2.4) - 0.055)
    return np.round(srgb * 255).astype(np.uint8)


def convert(source, destination):
    gltf, blob = load_glb(source)
    nodes = gltf["nodes"]
    parent = {child: i for i, node in enumerate(nodes) for child in node.get("children", [])}
    writer = Writer()

    def global_matrix(index):
        transform = local_matrix(nodes[index])
        while index in parent:
            index = parent[index]
            transform = local_matrix(nodes[index]) @ transform
        return transform

    poses = [global_matrix(i) for i in range(len(nodes))]
    for pose in poses:
        pose[:3, 3] *= SCALE
    inverse = np.asarray([np.linalg.inv(pose).T.reshape(16) for pose in poses], np.float32)
    primitives = []
    total_vertices = 0
    palette = {}  # sRGB 8 bits -> índice
    for material in range(2):
        positions, normals, colors, joints, indices = [], [], [], [], []
        for i, node in enumerate(nodes):
            if "mesh" not in node:
                continue
            transform = global_matrix(i)
            normal_transform = np.linalg.inv(transform[:3, :3]).T
            for primitive in gltf["meshes"][node["mesh"]]["primitives"]:
                if primitive.get("material", 0) != material:
                    continue
                attrs = primitive["attributes"]
                local = read_accessor(gltf, blob, attrs["POSITION"]).astype(np.float64)
                positions.append(((transform[:3, :3] @ local.T).T + transform[:3, 3]) * SCALE)
                normal = (normal_transform @ read_accessor(gltf, blob, attrs["NORMAL"]).T).T
                normals.append(normal / np.maximum(np.linalg.norm(normal, axis=1, keepdims=True), 1e-9))
                color = read_accessor(gltf, blob, attrs["COLOR_0"]).astype(np.float32)
                if color.shape[1] == 3:
                    color = np.column_stack((color, np.ones(len(color), np.float32)))
                colors.append(color)
                joints.append(np.full(len(local), i, np.uint16))
                source_indices = read_accessor(gltf, blob, primitive["indices"]).astype(np.uint32)
                indices.append(source_indices + sum(len(group) for group in positions[:-1]))
        if not positions:
            continue
        pos = np.concatenate(positions).astype(np.float32)
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
                raise RuntimeError("paleta do lobo cheia")
            uv[v] = ((index % PALETTE_SIZE + 0.5) / PALETTE_SIZE, (index // PALETTE_SIZE + 0.5) / PALETTE_SIZE)
        attrs = {
            "POSITION": writer.add(pos, 5126, "VEC3", 34962, minmax=True),
            "NORMAL": writer.add(np.concatenate(normals).astype(np.float32), 5126, "VEC3", 34962),
            "COLOR_0": writer.add(np.concatenate(colors).astype(np.float32), 5126, "VEC4", 34962),
            "JOINTS_0": writer.add(joint_ids, 5123, "VEC4", 34962),
            "WEIGHTS_0": writer.add(weights, 5126, "VEC4", 34962),
            "TEXCOORD_0": writer.add(uv, 5126, "VEC2", 34962),
        }
        primitives.append({"attributes": attrs, "indices": writer.add(np.concatenate(indices), 5125, "SCALAR", 34963), "material": material})

    new_nodes = []
    for node in nodes:
        entry = {"name": node["name"], "translation": (np.asarray(node.get("translation", [0, 0, 0])) * SCALE).tolist()}
        for key in ("rotation", "scale", "children"):
            if key in node:
                entry[key] = node[key]
        new_nodes.append(entry)
    mesh_node = len(new_nodes)
    new_nodes.append({"name": "Wolf_Skinned", "mesh": 0, "skin": 0})
    clips = []
    for animation in gltf["animations"]:
        samplers, channels = [], []
        for channel in animation["channels"]:
            sampler = animation["samplers"][channel["sampler"]]
            path = channel["target"]["path"]
            times = read_accessor(gltf, blob, sampler["input"]).astype(np.float32)
            values = read_accessor(gltf, blob, sampler["output"]).astype(np.float32)
            if path == "translation":
                values *= SCALE
            samplers.append({"input": writer.add(times, 5126, "SCALAR", minmax=True),
                             "output": writer.add(values, 5126, {"translation": "VEC3", "rotation": "VEC4", "scale": "VEC3"}[path]),
                             "interpolation": sampler.get("interpolation", "LINEAR")})
            channels.append({"sampler": len(samplers) - 1, "target": {"node": channel["target"]["node"], "path": path}})
        clips.append({"name": animation["name"], "samplers": samplers, "channels": channels})

    result = {"asset": {"version": "2.0", "generator": "Vortex ConvertGodotWolf"},
              "scene": 0, "scenes": [{"nodes": [0, mesh_node]}], "nodes": new_nodes,
              "meshes": [{"name": "Wolf_Skinned", "primitives": primitives}],
              "skins": [{"joints": list(range(len(nodes))), "inverseBindMatrices": writer.add(inverse, 5126, "MAT4"), "skeleton": 0}],
              "materials": gltf["materials"], "animations": clips,
              "accessors": writer.accessors, "bufferViews": writer.views,
              "buffers": [{"byteLength": len(writer.blob)}]}
    encoded = json.dumps(result, separators=(",", ":")).encode()
    encoded += b" " * ((-len(encoded)) % 4)
    while len(writer.blob) % 4:
        writer.blob.append(0)
    with open(destination, "wb") as file:
        file.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(encoded) + 8 + len(writer.blob)))
        file.write(struct.pack("<II", len(encoded), 0x4E4F534A) + encoded)
        file.write(struct.pack("<II", len(writer.blob), 0x004E4942) + writer.blob)
    image = np.zeros((PALETTE_SIZE, PALETTE_SIZE, 3), np.uint8)
    for key, index in palette.items():
        image[index // PALETTE_SIZE, index % PALETTE_SIZE] = key
    palette_path = os.path.join(os.path.dirname(os.path.abspath(destination)), "Wolf_Palette.png")
    Image.fromarray(image, "RGB").save(palette_path)
    print(f"WOLF_CONVERTED vertices={total_vertices} colors={len(palette)} palette={palette_path} clips={[clip['name'] for clip in clips]}")


if __name__ == "__main__":
    convert(sys.argv[1], sys.argv[2])
