"""Peças de voxel novas (só do Vortex) para a NPC Julia: cabelo longo preto com franja e pontas azuis,
óculos redondos, gargantilha com estrela, vestido preto (corpete, mangas bufantes e saia rodada) e
meias 7/8. Cada peça é desenhada em caixas no espaço do modelo do aventureiro do Godot (unidades do
modelo, voxel de 1/32), presa a um osso, e passa pelo mesmo conversor das peças oficiais
(ConvertGodotAdventurer.convert): sai em Data/GodotCharacter/Wardrobe/<peça>.glb com a pele do herói.

python Tools/BuildJuliaPieces.py
Depois: VORTEX_WARDROBE_ONLY=<peças> UnrealEditor-Cmd ... -script=Tools/ImportWardrobe.py

As cores vêm de células fixas da paleta (sem cor escolhida pelo jogador). O catálogo das peças fica em
Data/GodotCharacter/look_catalog_extra.json (o jogo junta ao catálogo do Godot).
O Godot não é alterado (só leitura).
"""
import json
import struct
from pathlib import Path

import numpy as np

from ConvertGodotAdventurer import convert, load_glb, local_matrix

ROOT = Path(__file__).resolve().parents[1]
GODOT = ROOT.parents[1] / "the-world-geneses" / "Art" / "Adventurer"
OUT = ROOT / "Data" / "GodotCharacter" / "Wardrobe"
PALETTE_MAP = json.loads((ROOT / "Data" / "GodotCharacter" / "palette_map.json").read_text(encoding="utf-8"))
V = 1.0 / 32.0   # voxel


def cell_uv(rgb):
    """Centro da célula fixa (sem slot) da paleta com a cor mais próxima."""
    cols, px, (w, h) = PALETTE_MAP["cols"], PALETTE_MAP["px"], PALETTE_MAP["size"]
    best = min((e for e in PALETTE_MAP["entries"] if not e.get("slot")),
               key=lambda e: sum((a - b) ** 2 for a, b in zip(e["rgb"], rgb)))
    i = best["i"]
    return ((i % cols) * px + px / 2) / w, ((i // cols) * px + px / 2) / h, best["rgb"]


BLACK = cell_uv((28, 26, 32))
BLACK_SOFT = cell_uv((44, 42, 50))       # brilho do cabelo / barra da saia
BLUE = cell_uv((46, 82, 190))
BLUE_DEEP = cell_uv((30, 52, 140))
SILVER = cell_uv((200, 204, 214))
LENS = cell_uv((60, 64, 74))


class Builder:
    def __init__(self):
        self.parts = {}   # osso -> (pos, normais, uv, cor)

    def box(self, bone, a, b, color, shade=1.0):
        """Caixa de a a b (cantos, unidades do modelo) presa ao osso."""
        (x0, y0, z0), (x1, y1, z1) = a, b
        p = self.parts.setdefault(bone, ([], [], [], []))
        u, v, _ = color
        faces = [
            ((1, 0, 0), [(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)], .9),
            ((-1, 0, 0), [(x0, y0, z1), (x0, y1, z1), (x0, y1, z0), (x0, y0, z0)], .9),
            ((0, 1, 0), [(x0, y1, z0), (x0, y1, z1), (x1, y1, z1), (x1, y1, z0)], 1.0),
            ((0, -1, 0), [(x0, y0, z1), (x0, y0, z0), (x1, y0, z0), (x1, y0, z1)], .72),
            ((0, 0, 1), [(x1, y0, z1), (x1, y1, z1), (x0, y1, z1), (x0, y0, z1)], .95),
            ((0, 0, -1), [(x0, y0, z0), (x0, y1, z0), (x1, y1, z0), (x1, y0, z0)], .85),
        ]
        for normal, corners, light in faces:
            for c in corners:
                p[0].append(c)
                p[1].append(normal)
                p[2].append((u, v))
                k = light * shade
                p[3].append((k, k, k, 1.0))


def q(x):
    return round(x / V) * V


def hair(b):
    """Cabelo liso preto até a cintura, franja reta, mechas na frente dos ombros, pontas azuis."""
    H = "Head"
    # topo e coroa (a cabeça vai de x ±0,5, y 2,375..3,375, z −0,41..0,47)
    b.box(H, (-0.5625, 3.375, -0.5), (0.5625, 3.53, 0.53), BLACK)
    b.box(H, (-0.5, 3.53, -0.44), (0.5, 3.59, 0.47), BLACK_SOFT)
    # franja reta com degraus, deixando os olhos (y 2,56..3,06)
    for i, x in enumerate(np.arange(-0.5, 0.5, 0.125)):
        low = 3.0625 + (0.03125 if i % 3 == 1 else 0.0)
        b.box(H, (x, low, 0.469), (x + 0.125, 3.40, 0.56), BLACK if i % 2 else BLACK_SOFT)
    # laterais (cobrindo as orelhas) e a cortina de trás até a cintura
    for s in (-1, 1):
        x0, x1 = (0.5, 0.625) if s > 0 else (-0.625, -0.5)
        b.box(H, (x0, 2.25, -0.5), (x1, 3.4375, 0.47), BLACK)
        # mechas da frente, caindo na frente dos ombros até o peito
        fx0, fx1 = (0.34, 0.5) if s > 0 else (-0.5, -0.34)
        b.box(H, (fx0, 1.86, 0.20), (fx1, 2.40, 0.36), BLACK)
        b.box(H, (fx0, 1.74, 0.21), (fx1, 1.86, 0.35), BLUE)
        b.box(H, (fx0 + 0.03, 1.66, 0.23), (fx1 - 0.03, 1.74, 0.33), BLUE_DEEP)
    b.box(H, (-0.625, 2.0, -0.625), (0.625, 3.4375, -0.47), BLACK)
    b.box(H, (-0.5625, 1.55, -0.6), (0.5625, 2.0, -0.47), BLACK)
    # pontas azuis atrás (em degraus, como mechas)
    b.box(H, (-0.5625, 1.30, -0.59), (0.5625, 1.55, -0.47), BLUE)
    for i, x in enumerate(np.arange(-0.5625, 0.5625, 0.1875)):
        b.box(H, (x + 0.02, 1.16 - (0.06 if i % 2 else 0.0), -0.58), (x + 0.1875 - 0.02, 1.30, -0.48), BLUE_DEEP)


def glasses(b):
    """Óculos redondos de aro fino preto, ponte e hastes até as orelhas."""
    H = "Head"
    z0, z1 = 0.47, 0.53
    for cx in (-0.25, 0.25):
        r = 0.19
        y0, y1 = 2.62, 3.0
        x0, x1 = cx - r, cx + r
        t = V * 1.2
        # aro "redondo" em voxels: lados retos com cantos recuados
        b.box(H, (x0 + 2 * V, y1 - t, z0), (x1 - 2 * V, y1, z1), BLACK)
        b.box(H, (x0 + 2 * V, y0, z0), (x1 - 2 * V, y0 + t, z1), BLACK)
        b.box(H, (x0, y0 + 2 * V, z0), (x0 + t, y1 - 2 * V, z1), BLACK)
        b.box(H, (x1 - t, y0 + 2 * V, z0), (x1, y1 - 2 * V, z1), BLACK)
        for sx, sy in ((x0 + t, y1 - 2 * V - t), (x1 - 2 * t, y1 - 2 * V - t), (x0 + t, y0 + 2 * V), (x1 - 2 * t, y0 + 2 * V)):
            b.box(H, (sx, sy, z0), (sx + t, sy + t, z1), BLACK)
    b.box(H, (-0.06, 2.88, z0), (0.06, 2.92, z1), BLACK)                  # ponte
    for s in (-1, 1):                                                     # hastes
        x0, x1 = (0.5, 0.54) if s > 0 else (-0.54, -0.5)
        b.box(H, (x0, 2.88, -0.05), (x1, 2.92, 0.5), BLACK)


def choker(b):
    """Gargantilha preta no pescoço com pingente de estrela prateado."""
    T = "Torso"
    b.box(T, (-0.22, 2.19, -0.19), (0.22, 2.29, -0.15), BLACK)
    b.box(T, (-0.22, 2.19, 0.15), (0.22, 2.29, 0.2), BLACK)
    for s in (-1, 1):
        b.box(T, ((0.18 if s > 0 else -0.22), 2.19, -0.19), ((0.22 if s > 0 else -0.18), 2.29, 0.2), BLACK)
    # corrente curta e a estrela (quatro pontas e o miolo)
    b.box(T, (-0.012, 2.08, 0.18), (0.012, 2.19, 0.2), SILVER)
    c = (0.0, 2.02)
    b.box(T, (c[0] - 0.03, c[1] - 0.03, 0.18), (c[0] + 0.03, c[1] + 0.03, 0.215), SILVER, 1.1)
    for dx, dy in ((0, 0.065), (0, -0.065), (0.065, 0), (-0.065, 0)):
        b.box(T, (c[0] + dx - 0.015, c[1] + dy - 0.015, 0.185), (c[0] + dx + 0.015, c[1] + dy + 0.015, 0.21), SILVER, 1.1)


def dress(b):
    """Vestido preto: corpete com decote, mangas bufantes curtas e saia rodada em camadas."""
    T, Hp = "Torso", "Hips"
    b.box(T, (-0.39, 1.55, -0.175), (0.39, 2.02, 0.175), BLACK)
    # decote em coração: duas bordas na frente, deixando o colo de fora
    for s in (-1, 1):
        x0, x1 = (0.04, 0.33) if s > 0 else (-0.33, -0.04)
        b.box(T, (x0, 2.02, 0.1), (x1, 2.08, 0.178), BLACK)
    b.box(T, (-0.39, 2.02, -0.175), (0.39, 2.19, -0.1), BLACK)            # costas sobem até o pescoço
    for s in (-1, 1):                                                     # alças largas
        x0, x1 = (0.22, 0.39) if s > 0 else (-0.39, -0.22)
        b.box(T, (x0, 2.02, -0.175), (x1, 2.19, 0.178), BLACK)
    b.box(T, (-0.4, 1.55, -0.18), (0.4, 1.6, 0.18), BLACK_SOFT)            # cinto fino
    for bone, s in (("UpperArm_L", 1), ("UpperArm_R", -1)):                # mangas bufantes
        x0, x1 = (0.54, 0.78) if s > 0 else (-0.78, -0.54)
        b.box(bone, (x0, 1.86, -0.19), (x1, 2.16, 0.19), BLACK)
        b.box(bone, (x0 + 0.02 * s if s > 0 else x0, 1.83, -0.17), (x1 if s > 0 else x1 - 0.02, 1.86, 0.17), BLACK_SOFT)
    # saia rodada, em três camadas abrindo até o meio da coxa
    for y0, y1, w, d, col in ((1.40, 1.60, 0.37, 0.20, BLACK), (1.18, 1.40, 0.43, 0.25, BLACK), (0.98, 1.18, 0.49, 0.30, BLACK)):
        b.box(Hp, (-w, y0, -d), (w, y1, d), col)
    b.box(Hp, (-0.5, 0.95, -0.31), (0.5, 0.99, 0.31), BLACK_SOFT)           # barra


def stockings(b):
    """Meias 7/8 pretas: canela e coxa até um palmo abaixo da saia, com barra."""
    for side, s in (("L", 1), ("R", -1)):
        x0, x1 = (0.11, 0.39) if s > 0 else (-0.39, -0.11)
        b.box("Shin_" + side, (x0, 0.12, -0.105), (x1, 0.76, 0.105), BLACK)
        b.box("Thigh_" + side, (x0, 0.74, -0.105), (x1, 1.02, 0.105), BLACK)
        b.box("Thigh_" + side, (x0 - 0.01 * s if s < 0 else x0 - 0.01, 0.98, -0.115), (x1 + 0.01 if s > 0 else x1 + 0.01, 1.04, 0.115), BLACK_SOFT)


PIECES = {
    "julia_hair": ("hair", "Longo com pontas azuis", hair),
    "julia_glasses": ("face_gear", "Óculos redondos", glasses),
    "julia_choker": ("back", "Gargantilha de estrela", choker),
    "julia_dress": ("shirt", "Vestido preto", dress),
    "julia_stockings": ("pants", "Meias 7/8", stockings),
}


def inject(gltf, blob, piece, builder):
    """Põe as caixas no glTF do Godot como grupos "<peça>__<osso>" (nó com a matriz inversa do osso)."""
    nodes = gltf["nodes"]
    parent = {}
    for i, n in enumerate(nodes):
        for c in n.get("children", []):
            parent[c] = i
    name_index = {n.get("name"): i for i, n in enumerate(nodes)}

    def global_matrix(i):
        m = local_matrix(nodes[i])
        while i in parent:
            i = parent[i]
            m = local_matrix(nodes[i]) @ m
        return m

    blob = bytearray(blob)
    groups = []
    for bone, (pos, nor, uv, col) in builder.parts.items():
        joint = name_index[bone]
        inv = np.linalg.inv(global_matrix(joint))
        arrays = [np.array(pos, np.float32), np.array(nor, np.float32), np.array(uv, np.float32), np.array(col, np.float32)]
        quads = len(pos) // 4
        idx = np.array([[4 * k, 4 * k + 1, 4 * k + 2, 4 * k, 4 * k + 2, 4 * k + 3] for k in range(quads)], np.uint32).reshape(-1)
        acc = []
        for arr, kind in zip(arrays + [idx], ["VEC3", "VEC3", "VEC2", "VEC4", "SCALAR"]):
            while len(blob) % 4:
                blob.append(0)
            gltf["bufferViews"].append({"buffer": 0, "byteOffset": len(blob), "byteLength": arr.nbytes})
            blob += arr.tobytes()
            gltf["accessors"].append({"bufferView": len(gltf["bufferViews"]) - 1, "componentType": 5125 if kind == "SCALAR" else 5126,
                                      "count": len(arr), "type": kind})
            acc.append(len(gltf["accessors"]) - 1)
        gltf["meshes"].append({"name": f"{piece}_{bone}", "primitives": [{"attributes": {"POSITION": acc[0], "NORMAL": acc[1], "TEXCOORD_0": acc[2],
                                                                                      "COLOR_0": acc[3]}, "indices": acc[4]}]})
        mesh_node = {"name": f"{piece}__{bone}_mesh", "mesh": len(gltf["meshes"]) - 1, "matrix": inv.T.reshape(16).tolist()}
        nodes.append(mesh_node)
        group = {"name": f"{piece}__{bone}", "children": [len(nodes) - 1]}
        nodes.append(group)
        nodes[joint].setdefault("children", []).append(len(nodes) - 1)
        groups.append(group["name"])
    return groups, bytes(blob)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    manifest_path = OUT / "manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8")) if manifest_path.exists() else {}
    catalog = {"_doc": "Peças só do Vortex (Tools/BuildJuliaPieces.py); juntadas ao catálogo do Godot pelo jogo.", "pieces": {}}
    for piece, (category, name, draw) in PIECES.items():
        gltf, blob = load_glb(GODOT / "Adventurer.glb")
        builder = Builder()
        draw(builder)
        groups, blob = inject(gltf, blob, piece, builder)
        convert(gltf, blob, groups, OUT / f"{piece}.glb", [])
        manifest[piece] = {"category": category, "groups": groups}
        catalog["pieces"][piece] = {"name": name, "category": category, "defaults": {}, "races": []}
    # mãos sem luva (peça vazia: o jogo esconde tudo que termina em _none)
    catalog["pieces"]["gloves_none"] = {"name": "Nenhuma", "category": "gloves", "defaults": {}, "races": []}
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (ROOT / "Data" / "GodotCharacter" / "look_catalog_extra.json").write_text(json.dumps(catalog, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("JULIA_PIECES", ", ".join(PIECES), "cores:", BLACK[2], BLUE[2], SILVER[2])


if __name__ == "__main__":
    main()
