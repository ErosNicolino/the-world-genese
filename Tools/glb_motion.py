"""Quais clipes de um .glb não se mexem (16/11: Montar e Desmontar chegaram ao jogo parados, exportados errados do Blender).

still(path) devolve os nomes das animações em que nenhum canal varia. Os importadores (ImportPowerAnims.py,
ImportHorseAnims.py) recusam esses clipes. Só a biblioteca padrão: roda dentro da Unreal e fora dela.
python Tools/glb_motion.py <arquivo.glb> lista o quanto cada clipe varia.
"""
import json
import struct
import sys

_SIZE = {"SCALAR": 1, "VEC3": 3, "VEC4": 4}


def motion(path):
    """{clipe: maior variação de um canal} (rotações em quatérnio, posições em metros)."""
    data = open(path, "rb").read()
    length = struct.unpack_from("<I", data, 12)[0]
    doc = json.loads(data[20:20 + length].decode("utf-8"))
    binary = data[20 + length + 8:]
    out = {}
    for animation in doc.get("animations", []):
        most = 0.0
        for sampler in animation["samplers"]:
            accessor = doc["accessors"][sampler["output"]]
            width = _SIZE.get(accessor["type"])
            if not width or accessor["componentType"] != 5126 or "bufferView" not in accessor:
                continue
            view = doc["bufferViews"][accessor["bufferView"]]
            start = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
            stride = view.get("byteStride", width * 4)
            keys = [struct.unpack_from("<%df" % width, binary, start + i * stride) for i in range(accessor["count"])]
            for axis in range(width):
                column = [key[axis] for key in keys]
                most = max(most, max(column) - min(column))
        out[animation.get("name", "?")] = most
    return out


def still(path, limit=1e-3):
    return sorted(name for name, most in motion(path).items() if most < limit)


if __name__ == "__main__":
    for name, most in sorted(motion(sys.argv[1]).items()):
        print("%-40s %.4f%s" % (name, most, "  PARADO" if most < 1e-3 else ""))
