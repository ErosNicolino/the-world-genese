"""Converte o aventureiro do Godot (peças rígidas presas aos ossos) num personagem com pele.

O Adventurer.glb do Godot não tem pele: cada peça (corpo, cabelo, roupa) é uma malha presa a um
nó-osso, e as animações movem esses nós. A Unreal anima personagens por esqueleto + pele, então
este script reescreve o .glb:
  - os 20 nós-osso (Root, Hips, Torso, Head, braços, pernas, soquetes) viram as juntas de uma pele;
  - as peças do visual escolhido (Data/GodotCharacter/default_look.json, exportado pelo Godot) são
    levadas para o espaço do modelo e juntadas numa malha só, cada vértice 100% preso ao seu osso
    (o mesmo movimento rígido do Godot);
  - ficam só os clipes pedidos, com as trilhas dos ossos;
  - tudo é escalado para a altura do herói no jogo (1,40 m; o Godot usa WorldScale = 1,40 / 3,8125).
O Godot e seus arquivos não são alterados (só leitura).

Uso: python ConvertGodotAdventurer.py <Adventurer.glb> <default_look.json> <saida.glb> [clipes separados por vírgula]
"""

import json
import struct
import sys

import numpy as np

SCALE = 1.40 / (61.0 / 16.0)
RIG = ["Root", "Hips", "Torso", "Head", "UpperArm_L", "Forearm_L", "Hand_L", "UpperArm_R", "Forearm_R", "Hand_R",
       "Socket_Weapon_R", "Sword", "Sword_Hilt", "Sheath", "Thigh_L", "Shin_L", "Foot_L", "Thigh_R", "Shin_R", "Foot_R"]
DEFAULT_CLIPS = ["Idle", "Walk", "Run", "Jump_Start", "Jump_Loop", "Jump_Land", "Dodge", "Dodge_Back", "Dodge_Left",
                 "Dodge_Right", "Hit", "Death"]
COMPONENTS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}
DTYPES = {5126: np.float32, 5123: np.uint16, 5125: np.uint32, 5121: np.uint8, 5122: np.int16, 5120: np.int8}


def load_glb(path):
    data = open(path, "rb").read()
    length = struct.unpack_from("<I", data, 12)[0]
    gltf = json.loads(data[20:20 + length])
    offset = 20 + length
    bin_length = struct.unpack_from("<I", data, offset)[0]
    return gltf, data[offset + 8:offset + 8 + bin_length]


def read_accessor(gltf, blob, index):
    acc = gltf["accessors"][index]
    view = gltf["bufferViews"][acc["bufferView"]]
    dtype = np.dtype(DTYPES[acc["componentType"]])
    n = COMPONENTS[acc["type"]]
    start = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
    stride = view.get("byteStride", 0)
    if stride and stride != dtype.itemsize * n:
        rows = [np.frombuffer(blob, dtype, n, start + i * stride) for i in range(acc["count"])]
        array = np.array(rows)
    else:
        array = np.frombuffer(blob, dtype, acc["count"] * n, start).reshape(acc["count"], n) if n > 1 else \
            np.frombuffer(blob, dtype, acc["count"], start)
    if acc.get("normalized") and dtype != np.float32:
        array = array.astype(np.float32) / np.iinfo(dtype).max
    return np.array(array)


def quat_matrix(q):
    x, y, z, w = q
    return np.array([
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])


def local_matrix(node):
    if "matrix" in node:
        return np.array(node["matrix"], dtype=np.float64).reshape(4, 4).T
    m = np.eye(4)
    m[:3, :3] = quat_matrix(node.get("rotation", [0, 0, 0, 1])) @ np.diag(node.get("scale", [1, 1, 1]))
    m[:3, 3] = node.get("translation", [0, 0, 0])
    return m


class Writer:
    def __init__(self):
        self.blob = bytearray()
        self.views, self.accessors = [], []

    def add(self, array, component, kind, target=None, minmax=False, normalized=False):
        array = np.ascontiguousarray(array)
        while len(self.blob) % 4:
            self.blob.append(0)
        view = {"buffer": 0, "byteOffset": len(self.blob), "byteLength": array.nbytes}
        if target:
            view["target"] = target
        self.blob += array.tobytes()
        self.views.append(view)
        acc = {"bufferView": len(self.views) - 1, "componentType": component, "count": len(array), "type": kind}
        if normalized:
            acc["normalized"] = True
        if minmax:
            flat = array.reshape(len(array), -1)
            acc["min"] = flat.min(axis=0).tolist()
            acc["max"] = flat.max(axis=0).tolist()
        self.accessors.append(acc)
        return len(self.accessors) - 1

    def add_bytes(self, raw):
        while len(self.blob) % 4:
            self.blob.append(0)
        self.views.append({"buffer": 0, "byteOffset": len(self.blob), "byteLength": len(raw)})
        self.blob += raw
        return len(self.views) - 1


def convert(gltf, blob, look, out, clips=DEFAULT_CLIPS):
    nodes = gltf["nodes"]
    look = set(look)
    parent = {}
    for i, node in enumerate(nodes):
        for c in node.get("children", []):
            parent[c] = i
    name_index = {node.get("name"): i for i, node in enumerate(nodes)}

    def global_matrix(i):
        m = local_matrix(nodes[i])
        while i in parent:
            i = parent[i]
            m = local_matrix(nodes[i]) @ m
        return m

    # Escala só nas posições (translações e vértices), nunca na matriz dos ossos: com escala na
    # matriz a Unreal entendia "osso com escala 0,367" e guardava as distâncias entre ossos no
    # tamanho original, enquanto os clipes vinham reduzidos, e os membros encolhiam para dentro.
    def scaled(m):
        m = m.copy()
        m[:3, 3] *= SCALE
        return m

    joints = [name_index[n] for n in RIG]
    joint_of = {node_index: j for j, node_index in enumerate(joints)}
    rest = [scaled(global_matrix(i)) for i in joints]
    inverse_bind = np.array([np.linalg.inv(m).T.reshape(16) for m in rest], dtype=np.float32)

    # peças do visual: malhas abaixo dos grupos "<peça>__<osso>"
    w = Writer()
    positions, normals, uvs, colors, weights_j, indices = [], [], [], [], [], []
    base = 0
    for group_name in sorted(look):
        group = name_index.get(group_name)
        if group is None:
            print("faltando", group_name)
            continue
        joint = group
        while joint not in joint_of:
            joint = parent[joint]
        stack = [group]
        while stack:
            i = stack.pop()
            stack.extend(nodes[i].get("children", []))
            if "mesh" not in nodes[i]:
                continue
            g = global_matrix(i)
            normal_matrix = np.linalg.inv(g[:3, :3]).T
            for prim in gltf["meshes"][nodes[i]["mesh"]]["primitives"]:
                a = prim["attributes"]
                p = read_accessor(gltf, blob, a["POSITION"]).astype(np.float64)
                positions.append(((g[:3, :3] @ p.T).T + g[:3, 3]) * SCALE)
                n = read_accessor(gltf, blob, a["NORMAL"]).astype(np.float64)
                n = (normal_matrix @ n.T).T
                normals.append(n / np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-9))
                uvs.append(read_accessor(gltf, blob, a["TEXCOORD_0"]) if "TEXCOORD_0" in a else np.zeros((len(p), 2)))
                if "COLOR_0" in a:
                    c = read_accessor(gltf, blob, a["COLOR_0"]).astype(np.float32)
                    if c.shape[1] == 3:
                        c = np.hstack([c, np.ones((len(c), 1), np.float32)])
                else:
                    c = np.ones((len(p), 4), np.float32)
                colors.append(c)
                weights_j.append(np.full(len(p), joint_of[joint]))
                idx = read_accessor(gltf, blob, prim["indices"]) if "indices" in prim else np.arange(len(p))
                indices.append(idx.astype(np.uint32) + base)
                base += len(p)
    if not positions:
        raise ValueError(f"Nenhuma geometria encontrada para {sorted(look)}")
    pos = np.concatenate(positions).astype(np.float32)
    count = len(pos)
    joint_ids = np.zeros((count, 4), np.uint16)
    joint_ids[:, 0] = np.concatenate(weights_j)
    weights = np.zeros((count, 4), np.float32)
    weights[:, 0] = 1
    attributes = {
        "POSITION": w.add(pos, 5126, "VEC3", 34962, minmax=True),
        "NORMAL": w.add(np.concatenate(normals).astype(np.float32), 5126, "VEC3", 34962),
        "TEXCOORD_0": w.add(np.concatenate(uvs).astype(np.float32), 5126, "VEC2", 34962),
        "COLOR_0": w.add(np.concatenate(colors).astype(np.float32), 5126, "VEC4", 34962),
        "JOINTS_0": w.add(joint_ids, 5123, "VEC4", 34962),
        "WEIGHTS_0": w.add(weights, 5126, "VEC4", 34962),
    }
    index_acc = w.add(np.concatenate(indices), 5125, "SCALAR", 34963)

    # juntas: mesmos nomes, hierarquia e pose de descanso (escalada)
    new_nodes = []
    joint_new = {}
    for j, old in enumerate(joints):
        node = nodes[old]
        t = np.array(node.get("translation", [0, 0, 0])) * SCALE
        entry = {"name": node["name"], "translation": t.tolist()}
        if "rotation" in node:
            entry["rotation"] = node["rotation"]
        if "scale" in node:
            entry["scale"] = node["scale"]
        new_nodes.append(entry)
        joint_new[old] = j
    for j, old in enumerate(joints):
        kids = [joint_new[c] for c in nodes[old].get("children", []) if c in joint_new]
        if kids:
            new_nodes[j]["children"] = kids
    ibm = w.add(inverse_bind, 5126, "MAT4")
    mesh_node = len(new_nodes)
    new_nodes.append({"name": "Adventurer", "mesh": 0, "skin": 0})

    # textura da paleta embutida, sem mudança
    image = gltf["images"][0]
    view = gltf["bufferViews"][image["bufferView"]]
    raw = blob[view.get("byteOffset", 0):view.get("byteOffset", 0) + view["byteLength"]]
    image_view = w.add_bytes(raw)

    # clipes: só as trilhas dos ossos; translação escalada
    animations = []
    by_name = {a["name"]: a for a in gltf["animations"]}
    for clip in clips:
        anim = by_name.get(clip)
        if not anim:
            print("clipe faltando", clip)
            continue
        samplers, channels = [], []
        for ch in anim["channels"]:
            target = ch["target"]["node"]
            if target not in joint_new:
                continue
            s = anim["samplers"][ch["sampler"]]
            times = read_accessor(gltf, blob, s["input"]).astype(np.float32)
            values = read_accessor(gltf, blob, s["output"]).astype(np.float32)
            if ch["target"]["path"] == "translation":
                values = values * SCALE
            kind = {"translation": "VEC3", "scale": "VEC3", "rotation": "VEC4"}[ch["target"]["path"]]
            samplers.append({"input": w.add(times, 5126, "SCALAR", minmax=True),
                             "output": w.add(values, 5126, kind), "interpolation": s.get("interpolation", "LINEAR")})
            channels.append({"sampler": len(samplers) - 1, "target": {"node": joint_new[target], "path": ch["target"]["path"]}})
        animations.append({"name": clip, "samplers": samplers, "channels": channels})

    material = dict(gltf["materials"][0])
    material["name"] = "Adventurer_Palette"
    out_gltf = {
        "asset": {"version": "2.0", "generator": "Vortex ConvertGodotAdventurer"},
        "scene": 0,
        "scenes": [{"nodes": [0, mesh_node]}],
        "nodes": new_nodes,
        "meshes": [{"name": "Adventurer", "primitives": [{"attributes": attributes, "indices": index_acc, "material": 0}]}],
        "skins": [{"joints": list(range(len(joints))), "inverseBindMatrices": ibm, "skeleton": 0}],
        "materials": [material],
        "textures": [{"source": 0, "sampler": 0}],
        "samplers": [{"magFilter": 9728, "minFilter": 9728}],
        "images": [{"name": "palette", "mimeType": image.get("mimeType", "image/png"), "bufferView": image_view}],
        "animations": animations,
        "accessors": w.accessors,
        "bufferViews": w.views,
        "buffers": [{"byteLength": len(w.blob)}],
    }
    js = json.dumps(out_gltf, separators=(",", ":")).encode()
    js += b" " * ((4 - len(js) % 4) % 4)
    while len(w.blob) % 4:
        w.blob.append(0)
    with open(out, "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(w.blob)))
        f.write(struct.pack("<II", len(js), 0x4E4F534A) + js)
        f.write(struct.pack("<II", len(w.blob), 0x004E4942) + bytes(w.blob))
    print(f"ADVENTURER_CONVERTED vertices={count} triangulos={len(np.concatenate(indices)) // 3} clipes={len(animations)} "
          f"altura={pos[:, 1].max() - pos[:, 1].min():.2f}m -> {out}")


if __name__ == "__main__":
    src, look_path, out = sys.argv[1], sys.argv[2], sys.argv[3]
    clips = sys.argv[4].split(",") if len(sys.argv) > 4 and sys.argv[4] else DEFAULT_CLIPS
    gltf, blob = load_glb(src)
    convert(gltf, blob, json.load(open(look_path, encoding="utf-8")), out, clips)
