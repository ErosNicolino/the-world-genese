"""Extract the sword already authored under the adventurer's Sword bone.

The Godot source remains read-only. Coordinates use the same scale as the hero conversion.
"""
import json
import struct
import sys

import numpy as np

from ConvertGodotAdventurer import SCALE, Writer, load_glb, read_accessor


def extract(source, destination):
    gltf, blob = load_glb(source)
    nodes = gltf["nodes"]
    sword = next(i for i, node in enumerate(nodes) if node.get("name") == "Sword")
    children = nodes[sword].get("children", [])
    if len(children) != 1 or nodes[children[0]].get("name") != "Sword_mesh":
        raise RuntimeError("Source rig no longer matches the reviewed Sword bone")
    mesh = gltf["meshes"][nodes[children[0]]["mesh"]]
    if len(mesh["primitives"]) != 1:
        raise RuntimeError("Expected one embedded sword primitive")
    primitive = mesh["primitives"][0]
    writer = Writer()
    attributes = {}
    for name, kind in (("POSITION", "VEC3"), ("NORMAL", "VEC3"), ("TEXCOORD_0", "VEC2"), ("COLOR_0", "VEC4")):
        values = read_accessor(gltf, blob, primitive["attributes"][name]).astype(np.float32)
        if name == "POSITION":
            values *= SCALE
        attributes[name] = writer.add(values, 5126, kind, 34962, minmax=name == "POSITION")
    indices = read_accessor(gltf, blob, primitive["indices"]).astype(np.uint32)
    index = writer.add(indices, 5125, "SCALAR", 34963)
    source_image = gltf["images"][0]
    view = gltf["bufferViews"][source_image["bufferView"]]
    start = view.get("byteOffset", 0)
    image_view = writer.add_bytes(blob[start:start + view["byteLength"]])
    result = {"asset": {"version": "2.0", "generator": "Vortex ExtractGodotEmbeddedSword"},
              "scene": 0, "scenes": [{"nodes": [0]}],
              "nodes": [{"name": "Sword_Embedded", "mesh": 0}],
              "meshes": [{"name": "Sword_Embedded", "primitives": [{"attributes": attributes, "indices": index, "material": 0}]}],
              "materials": [gltf["materials"][primitive["material"]]],
              "images": [{"name": "palette", "mimeType": source_image.get("mimeType", "image/png"), "bufferView": image_view}],
              "textures": [{"source": 0, "sampler": 0}], "samplers": [{"magFilter": 9728, "minFilter": 9728}],
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
    print(f"EMBEDDED_SWORD_EXTRACTED vertices={len(read_accessor(gltf, blob, primitive['attributes']['POSITION']))}")


if __name__ == "__main__":
    extract(sys.argv[1], sys.argv[2])
