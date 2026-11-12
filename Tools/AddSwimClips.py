"""Acrescenta as animações de nado (e a de montar) ao aventureiro (Data/GodotCharacter/Adventurer_Skinned.glb).

python Tools/AddSwimClips.py      (depois: Tools/ImportAdventurer.py no editor fechado)

O Godot não tem clipe de nado (o herói "andava" na água). Aqui são escritos dois clipes no próprio
arquivo, osso por osso, a partir do descanso do esqueleto (todos os ossos com rotação zero, Y para
cima, frente em +Z):
  - Swim: crawl. Corpo deitado na água (quadril inclinado 78°), braços dando a volta inteira um
    depois do outro (puxam por baixo, voltam por cima), o tronco rola com a braçada, a cabeça olha
    para a frente e as pernas batem rápido.
  - Swim_Idle: boiar parado. Corpo em pé, braços abertos remando para os lados e pernas pedalando.
  - Ride: sentado no cavalo, pernas abertas em volta dele, mãos nas rédeas.
Rodar de novo substitui os dois clipes (não duplica). ConvertGodotAdventurer.py recria o arquivo do
zero: rode este depois dele.
"""
import json
import math
import os
import struct

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GLB = os.path.join(ROOT, "Data", "GodotCharacter", "Adventurer_Skinned.glb")


def quat_axis(axis, degrees):
    a = math.radians(degrees) * .5
    s = math.sin(a)
    return (axis[0] * s, axis[1] * s, axis[2] * s, math.cos(a))


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


X, Y, Z = (1, 0, 0), (0, 1, 0), (0, 0, 1)


def rot(*steps):
    q = (0.0, 0.0, 0.0, 1.0)
    for axis, deg in steps:
        q = qmul(q, quat_axis(axis, deg))
    return q


def swim(t):
    """Crawl, t em [0, 1). Ângulo do braço: -180 = esticado à frente (acima da cabeça, no corpo
    deitado), -90 = puxando por baixo, 0 = junto do quadril, +90 = voltando por cima da água."""
    tau = 2 * math.pi
    arm_r = -180 + 360 * t
    arm_l = -180 + 360 * ((t + .5) % 1.0)
    roll = 22 * math.sin(tau * t)
    kick = 18 * math.sin(tau * t * 3)
    pose = {
        "Hips": rot((X, 78)),
        "Torso": rot((Y, roll)),
        "Head": rot((X, -30), (Y, -roll * .6)),
        "UpperArm_R": rot((X, arm_r), (Z, -8)),
        "UpperArm_L": rot((X, arm_l), (Z, 8)),
        # cotovelo dobra na volta por cima (braço relaxado) e estica na puxada
        "Forearm_R": rot((X, -35 * max(0.0, math.sin(math.radians(arm_r))))),
        "Forearm_L": rot((X, -35 * max(0.0, math.sin(math.radians(arm_l))))),
        "Thigh_R": rot((X, kick)),
        "Thigh_L": rot((X, -kick)),
        "Shin_R": rot((X, 12 + 10 * math.sin(tau * t * 3 + 1.2))),
        "Shin_L": rot((X, 12 - 10 * math.sin(tau * t * 3 + 1.2))),
        "Foot_R": rot((X, 35)),
        "Foot_L": rot((X, 35)),
    }
    # o corpo deitado fica na linha da água (o herói boia com a sola ~1 m abaixo da superfície)
    return pose, (0.0, .34, 0.0)


def tread(t):
    """Boiar parado, t em [0, 1): braços abertos remando e pernas pedalando."""
    tau = 2 * math.pi
    scull = 22 * math.sin(tau * t * 2)
    pedal = 28 * math.sin(tau * t * 2)
    pose = {
        "Hips": rot((X, 10)),
        "Torso": rot((X, 4)),
        "Head": rot((X, -8)),
        "UpperArm_R": rot((Z, -62), (X, -20 + scull)),
        "UpperArm_L": rot((Z, 62), (X, -20 - scull)),
        "Forearm_R": rot((X, -45 + scull * .5)),
        "Forearm_L": rot((X, -45 - scull * .5)),
        "Thigh_R": rot((X, -25 + pedal)),
        "Thigh_L": rot((X, -25 - pedal)),
        "Shin_R": rot((X, 45 - pedal * .6)),
        "Shin_L": rot((X, 45 + pedal * .6)),
    }
    return pose, (0.0, .02 * math.sin(tau * t * 2), 0.0)


def ride(t):
    """Montado, t em [0, 1): sentado na sela com as pernas abertas em volta do cavalo, joelhos
    dobrados, tronco um pouco à frente e as mãos nas rédeas; balança com o passo do cavalo."""
    tau = 2 * math.pi
    bob = math.sin(tau * t * 2)
    pose = {
        "Hips": rot((X, -4 + 3 * bob)),
        "Torso": rot((X, 10 + 2 * bob)),
        "Head": rot((X, -8 - 2 * bob)),
        "UpperArm_R": rot((Z, -14), (X, -38)),
        "UpperArm_L": rot((Z, 14), (X, -38)),
        "Forearm_R": rot((X, -62), (Y, 12)),
        "Forearm_L": rot((X, -62), (Y, -12)),
        "Thigh_R": rot((X, -78), (Z, -24)),
        "Thigh_L": rot((X, -78), (Z, 24)),
        "Shin_R": rot((X, 84)),
        "Shin_L": rot((X, 84)),
        "Foot_R": rot((X, -12)),
        "Foot_L": rot((X, -12)),
    }
    return pose, (0.0, .025 * bob, 0.0)


CLIPS = [("Swim", swim, 1.25, 30), ("Swim_Idle", tread, 1.6, 24), ("Ride", ride, 1.0, 24)]


def main():
    data = open(GLB, "rb").read()
    magic, version, length = struct.unpack("<III", data[:12])
    json_len, json_type = struct.unpack("<II", data[12:20])
    gltf = json.loads(data[20:20 + json_len])
    rest = data[20 + json_len:]
    bin_len, bin_type = struct.unpack("<II", rest[:8])
    blob = bytearray(rest[8:8 + bin_len])

    names = {n.get("name"): i for i, n in enumerate(gltf["nodes"])}
    gltf["animations"] = [a for a in gltf["animations"] if a["name"] not in {c[0] for c in CLIPS}]
    # o buffer é reescrito com os dados novos no fim; acessores/views antigos dos clipes removidos ficam órfãos (inofensivos)

    def add(values, component_count, kind, minmax=False):
        while len(blob) % 4:
            blob.append(0)
        offset = len(blob)
        flat = [v for item in values for v in (item if isinstance(item, (tuple, list)) else (item,))]
        blob.extend(struct.pack("<%df" % len(flat), *flat))
        gltf["bufferViews"].append({"buffer": 0, "byteOffset": offset, "byteLength": len(flat) * 4})
        accessor = {"bufferView": len(gltf["bufferViews"]) - 1, "componentType": 5126, "count": len(values), "type": kind}
        if minmax:
            accessor["min"] = [min(values)]
            accessor["max"] = [max(values)]
        gltf["accessors"].append(accessor)
        return len(gltf["accessors"]) - 1

    for name, fn, seconds, steps in CLIPS:
        times = [seconds * i / steps for i in range(steps + 1)]
        samples = [fn((i % steps) / steps) for i in range(steps + 1)]
        time_acc = add(times, 1, "SCALAR", minmax=True)
        animation = {"name": name, "channels": [], "samplers": []}
        for bone in samples[0][0]:
            quats = [s[0][bone] for s in samples]
            # mesmo hemisfério entre chaves vizinhas (sem giro pelo caminho longo)
            for i in range(1, len(quats)):
                if sum(a * b for a, b in zip(quats[i - 1], quats[i])) < 0:
                    quats[i] = tuple(-v for v in quats[i])
            out = add(quats, 4, "VEC4")
            animation["samplers"].append({"input": time_acc, "output": out, "interpolation": "LINEAR"})
            animation["channels"].append({"sampler": len(animation["samplers"]) - 1, "target": {"node": names[bone], "path": "rotation"}})
        # o quadril sobe/desce um pouco (posição de descanso + deslocamento)
        hips_rest = gltf["nodes"][names["Hips"]].get("translation", [0, 0, 0])
        moves = [tuple(hips_rest[k] + s[1][k] for k in range(3)) for s in samples]
        out = add(moves, 3, "VEC3")
        animation["samplers"].append({"input": time_acc, "output": out, "interpolation": "LINEAR"})
        animation["channels"].append({"sampler": len(animation["samplers"]) - 1, "target": {"node": names["Hips"], "path": "translation"}})
        gltf["animations"].append(animation)

    while len(blob) % 4:
        blob.append(0)
    gltf["buffers"][0]["byteLength"] = len(blob)
    text = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
    while len(text) % 4:
        text += b" "
    total = 12 + 8 + len(text) + 8 + len(blob)
    with open(GLB, "wb") as out:
        out.write(struct.pack("<III", magic, version, total))
        out.write(struct.pack("<II", len(text), json_type))
        out.write(text)
        out.write(struct.pack("<II", len(blob), bin_type))
        out.write(blob)
    print("clipes:", [a["name"] for a in gltf["animations"]])


if __name__ == "__main__":
    main()
