"""Golpes da espada refeitos a partir dos clipes do Godot (Data/GodotWeapons/Adventurer_SwordAttacks.glb).

No Blender aberto (MCP ou console):   exec(open(<este arquivo>).read()); preview()   -> ações VX_<clipe> no "Armature"
Em segundo plano:  blender --background --python Tools/Blender/BuildSwordAttacks.py -- export

Os clipes são lidos direto do glTF (rotação por slerp, como a Unreal) e editados no espaço de cada osso
do arquivo original. O GLB novo (Adventurer_SwordAttacksV2.glb) usa os mesmos nós, malha e esqueleto do
original; só as animações mudam. O exportador glTF do Blender não serve aqui: ele gira os eixos dos ossos
e a Unreal aplicaria as chaves num esqueleto diferente.

O que muda (pedido de 22/10, ver Docs/02_Combate/ADAGA_INICIAL.md):
- Attack_02: na preparação (0,08–0,19 s) a lâmina descia entre as pernas; agora fica para trás e para
  fora, acima do joelho. O corte, que girava ~150° num só quadro, passa a durar 3 quadros
  (0,167–0,25 s), e o resto do clipe é comprimido para manter os 0,62 s.
- Attack_03: o clipe tem 1,12 s e o golpe acaba aos 0,94 s. O corte (até 0,47 s) fica igual e a volta
  é comprimida para caber nos 0,94 s, sem o salto para a pose parada.
- Skill_Guard: os dois braços vão ~12° para a frente, afastando a lâmina do queixo.
- Attack_01: o corte (0,208–0,25 s, ~2,5 m de arco na ponta em 2 quadros a 60 FPS) passa a durar até
  0,29 s; o resto encolhe para manter os 0,62 s.
"""
import json
import math
import os
import struct
import sys

import numpy as np

try:
    import bpy
    from mathutils import Matrix, Quaternion, Vector
except ImportError:  # pragma: no cover
    raise SystemExit("Rode no Blender")

ROOT = os.environ.get("VORTEX_ROOT", r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex")
SRC = os.path.join(ROOT, "Data", "GodotWeapons", "Adventurer_SwordAttacks.glb")
OUT = os.path.join(ROOT, "Data", "GodotWeapons", "Adventurer_SwordAttacksV2.glb")
FPS = 60
# ponta da adaga no espaço do Socket_Weapon_R do glTF (metros), medida pela malha W_adaga_inicial presa ao
# osso (tip_from_blender); no glTF o Y é para cima e o herói olha para +Z
TIP_SOCKET = None  # definido abaixo, depois de Vector

raw = open(SRC, "rb").read()
_jl = struct.unpack("<I", raw[12:16])[0]
G = json.loads(raw[20:20 + _jl])
BIN = raw[20 + _jl + 8:]
nodes = G["nodes"]
parent = {c: i for i, n in enumerate(nodes) for c in n.get("children", [])}
joints = G["skins"][0]["joints"]
by_name = {nodes[i]["name"]: i for i in joints}
A = Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))   # glTF (Y para cima) -> Blender
TIP_SOCKET = Vector((-0.0048, -0.4422, 0.5249))
UP, BACK = Vector((0, 1, 0)), Vector((0, 0, -1))


def acc(i):
    a = G["accessors"][i]
    v = G["bufferViews"][a["bufferView"]]
    n = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}[a["type"]]
    dt = {5126: np.float32, 5123: np.uint16, 5125: np.uint32, 5121: np.uint8}[a["componentType"]]
    off = v.get("byteOffset", 0) + a.get("byteOffset", 0)
    arr = np.frombuffer(BIN, dtype=dt, count=a["count"] * n, offset=off)
    return arr.reshape(a["count"], n) if n > 1 else arr


def rest_trs(i):
    n = nodes[i]
    r = n.get("rotation", [0, 0, 0, 1])
    return [Vector(n.get("translation", [0, 0, 0])), Quaternion((r[3], r[0], r[1], r[2])), Vector(n.get("scale", [1, 1, 1]))]


def compose(t, q, s):
    return Matrix.LocRotScale(t, q, s)


class Clip:
    """Um clipe do glTF: amostra (t, r, s) de cada osso no tempo pedido, como a Unreal (slerp)."""

    def __init__(self, name):
        self.anim = next(a for a in G["animations"] if a["name"] == name)
        self.name = name
        self.tracks = {}
        for ch in self.anim["channels"]:
            s = self.anim["samplers"][ch["sampler"]]
            times = acc(s["input"]).astype(np.float64)
            vals = acc(s["output"]).astype(np.float64)
            path = ch["target"]["path"]
            if path == "rotation":   # mesmo hemisfério entre chaves vizinhas (slerp pelo caminho curto)
                vals = vals.copy()
                for k in range(1, len(vals)):
                    if np.dot(vals[k], vals[k - 1]) < 0:
                        vals[k] = -vals[k]
            self.tracks[(ch["target"]["node"], path)] = (times, vals, s.get("interpolation", "LINEAR"))
        self.length = max(t[0][-1] for t in self.tracks.values())

    def sample(self, i, t):
        out = rest_trs(i)
        for k, path in enumerate(("translation", "rotation", "scale")):
            tr = self.tracks.get((i, path))
            if not tr:
                continue
            times, vals, interp = tr
            t = min(max(t, times[0]), times[-1])
            j = int(np.searchsorted(times, t, side="right")) - 1
            j = min(max(j, 0), len(times) - 1)
            if interp == "STEP" or j == len(times) - 1:
                v = vals[j]
                f = 0.0
                w = vals[j]
            else:
                f = (t - times[j]) / (times[j + 1] - times[j])
                v, w = vals[j], vals[j + 1]
            if path == "rotation":
                qa = Quaternion((v[3], v[0], v[1], v[2]))
                qb = Quaternion((w[3], w[0], w[1], w[2]))
                out[k] = qa.slerp(qb, f)
            else:
                out[k] = Vector(v * (1 - f) + w * f)
        return out


def worlds(local):
    """local: {nó: Matrix} dos ossos; o resto vem do repouso. Devolve o mundo glTF de cada osso."""
    cache = {}

    def w(i):
        if i in cache:
            return cache[i]
        m = local[i] if i in local else compose(*rest_trs(i))
        cache[i] = (w(parent[i]) @ m) if i in parent else m
        return cache[i]
    return {i: w(i) for i in joints}


def pose(clip, t):
    return {i: compose(*clip.sample(i, t)) for i in joints}


def smooth(x):
    x = min(max(x, 0.0), 1.0)
    return x * x * (3 - 2 * x)


def bump(t, a, b, c, d):
    """0 antes de a, sobe até b, fica em 1 até c, desce até d."""
    if t <= a or t >= d:
        return 0.0
    if t < b:
        return smooth((t - a) / (b - a))
    if t <= c:
        return 1.0
    return 1.0 - smooth((t - c) / (d - c))


def rotate_world(local, bone, rot, pivot=None):
    """Gira o osso (e os filhos) no mundo glTF em volta do próprio início."""
    i = by_name[bone]
    W = worlds(local)
    wm = W[i]
    p = wm.to_translation() if pivot is None else pivot
    new = Matrix.Translation(p) @ rot.to_matrix().to_4x4() @ Matrix.Translation(-p) @ wm
    pm = W[parent[i]] if parent.get(i) in W else (compose(*rest_trs(parent[i])) if i in parent else Matrix.Identity(4))
    local[i] = pm.inverted() @ new


def blade(local):
    W = worlds(local)
    s = W[by_name["Socket_Weapon_R"]]
    return s.to_translation(), s @ TIP_SOCKET


# ---- edições ------------------------------------------------------------------------------------------

def attack_02(t_new):
    src = Clip("Attack_02")
    # tempo: o corte (0,1667 -> 0,2083 no original) passa a ir de 0,1667 a 0,25; o resto encolhe
    t = stretch(t_new, .1667, .2083, .25, src.length)
    local = pose(src, t)
    # preparação: a ponta sobe e vai para fora em vez de descer entre as pernas
    w = bump(t_new, .04, .09, .15, .19)
    if w > 0:
        grip, tip = blade(local)
        d = (tip - grip).normalized()
        W = worlds(local)
        hips = W[by_name["Hips"]].to_translation()
        side = Vector((grip.x - hips.x, 0.0, grip.z - hips.z))
        side = side.normalized() if side.length > 1e-6 else Vector((1, 0, 0))
        # alvo: a mesma direção no chão, mais para fora da perna e só um pouco abaixo da horizontal
        flat = Vector((d.x, 0.0, d.z))
        flat = flat.normalized() if flat.length > 1e-6 else BACK
        target = flat * .75 + side * .55
        target = (target.normalized() * .96 - UP * .28).normalized()
        rot = d.rotation_difference(target)
        rotate_world(local, "Hand_R", Quaternion().slerp(rot, w))
    return local


def stretch(t_new, a, b, b_new, length):
    """Tempo do original: [a, b] passa a durar [a, b_new]; o que vem depois encolhe até o fim do clipe."""
    if t_new <= a:
        return t_new
    if t_new <= b_new:
        return a + (t_new - a) * (b - a) / (b_new - a)
    return b + (t_new - b_new) * (length - b) / (length - b_new)


def attack_01(t_new):
    src = Clip("Attack_01")
    # o corte (0,208 -> 0,25 no original, ~2,5 m de arco na ponta em 2 quadros) passa a durar 0,208 -> 0,29
    return pose(src, stretch(t_new, .208, .25, .29, src.length))


def attack_03(t_new):
    src = Clip("Attack_03")
    t = t_new if t_new <= .47 else .47 + (t_new - .47) * ((src.length - .47) / (.94 - .47))
    return pose(src, t)


def skill_guard(t_new):
    src = Clip("Skill_Guard")
    local = pose(src, t_new)
    w = bump(t_new, 0.0, .15, .7, .9)
    if w > 0:
        W = worlds(local)
        torso = W[by_name["Torso"]]
        right = (torso.to_quaternion() @ Vector((1, 0, 0))).normalized()
        # os braços para a frente (em volta do eixo dos ombros); os dois juntos, a mão esquerda segue a lâmina
        for arm_bone in ("UpperArm_R", "UpperArm_L"):
            rotate_world(local, arm_bone, Quaternion(right, math.radians(12) * w))
    return local


CLIPS = {"Attack_01": (attack_01, None),
         "Attack_02": (attack_02, None),
         "Attack_03": (attack_03, .94),
         "Skill_Guard": (skill_guard, None)}


def bake(name):
    fn, length = CLIPS[name]
    length = length or Clip(name).length
    n = int(round(length * FPS))
    times = [length * k / n for k in range(n + 1)]
    return times, [fn(t) for t in times]


def tip_from_blender():
    """A ponta da adaga no Socket do glTF, pela adaga presa ao osso no Blender (BuildStarterDagger.py)."""
    arm = bpy.data.objects["Armature"]
    dag = bpy.data.objects["W_adaga_inicial"]
    pb = arm.pose.bones["Socket_Weapon_R"]
    rel = (arm.matrix_world @ pb.matrix).inverted() @ dag.matrix_world
    me = dag.data
    far = max(me.vertices, key=lambda v: v.co.length).co
    i = by_name["Socket_Weapon_R"]
    C = (arm.matrix_world @ arm.data.bones["Socket_Weapon_R"].matrix_local).inverted() @ A @ worlds({})[i]
    return C.inverted() @ rel @ far


# ---- prévia no Blender e exportação ------------------------------------------------------------------

def preview(names=None):
    arm = bpy.data.objects["Armature"]
    rest = worlds({})
    C = {i: (arm.matrix_world @ arm.data.bones[nodes[i]["name"]].matrix_local).inverted() @ A @ rest[i] for i in joints}
    report = {}
    for name in names or CLIPS:
        times, locals_ = bake(name)
        act = bpy.data.actions.get("VX_" + name) or bpy.data.actions.new("VX_" + name)
        if arm.animation_data is None:
            arm.animation_data_create()
        arm.animation_data.action = act
        for pb in arm.pose.bones:
            pb.rotation_mode = "QUATERNION"
        clear = []
        for k, (t, local) in enumerate(zip(times, locals_)):
            W = worlds(local)
            frame = t * bpy.context.scene.render.fps
            for i in joints:
                pb = arm.pose.bones[nodes[i]["name"]]
                P = arm.matrix_world.inverted() @ (A @ W[i]) @ C[i].inverted()
                if pb.parent:
                    pp = arm.matrix_world.inverted() @ (A @ W[by_name[pb.parent.name]]) @ C[by_name[pb.parent.name]].inverted()
                    basis = (pp @ pb.parent.bone.matrix_local.inverted() @ pb.bone.matrix_local).inverted() @ P
                else:
                    basis = pb.bone.matrix_local.inverted() @ P
                loc, rot, sca = basis.decompose()
                pb.location, pb.rotation_quaternion, pb.scale = loc, rot, sca
                for path in ("location", "rotation_quaternion", "scale"):
                    pb.keyframe_insert(path, frame=frame)
        # menor distância da lâmina à cabeça, ao tronco e ao quadril, e a altura da ponta
        worst = {"cabeça": 9, "tronco": 9, "quadril": 9, "ponta_min_z": 9}
        for local in locals_:
            W = worlds(local)
            g, tp = blade(local)
            for key, bone in (("cabeça", "Head"), ("tronco", "Torso"), ("quadril", "Hips")):
                p = W[by_name[bone]].to_translation()
                ab = tp - g
                f = max(0.0, min(1.0, (p - g).dot(ab) / ab.dot(ab)))
                worst[key] = min(worst[key], (p - (g + ab * f)).length)
            worst["ponta_min_z"] = min(worst["ponta_min_z"], tp.y)   # glTF: Y para cima
        report[name] = {k: round(v, 3) for k, v in worst.items()}
    return report


def export():
    """Adventurer_SwordAttacksV2.glb: o GLB original com as animações trocadas pelas novas."""
    gl = json.loads(json.dumps(G))
    blob = bytearray(BIN)
    while len(blob) % 4:
        blob.append(0)

    def add(arr, comp, kind, minmax=False):
        data = np.ascontiguousarray(arr, dtype=np.float32).tobytes()
        off = len(blob)
        blob.extend(data)
        while len(blob) % 4:
            blob.append(0)
        gl["bufferViews"].append({"buffer": 0, "byteOffset": off, "byteLength": len(data)})
        a = {"bufferView": len(gl["bufferViews"]) - 1, "componentType": 5126, "count": len(arr), "type": kind}
        if minmax:
            a["min"] = [float(np.min(arr))]
            a["max"] = [float(np.max(arr))]
        gl["accessors"].append(a)
        return len(gl["accessors"]) - 1

    gl["animations"] = []
    for name in CLIPS:
        times, locals_ = bake(name)
        t_acc = add(np.array(times), 5126, "SCALAR", True)
        channels, samplers = [], []
        for i in joints:
            T, R, S = [], [], []
            prev = None
            for local in locals_:
                t, q, s = local[i].decompose()
                if prev is not None and prev.dot(q) < 0:
                    q = -q
                prev = q
                T.append(t[:])
                R.append((q.x, q.y, q.z, q.w))
                S.append(s[:])
            for path, vals, kind in (("translation", T, "VEC3"), ("rotation", R, "VEC4"), ("scale", S, "VEC3")):
                samplers.append({"input": t_acc, "output": add(np.array(vals), 5126, kind), "interpolation": "LINEAR"})
                channels.append({"sampler": len(samplers) - 1, "target": {"node": i, "path": path}})
        gl["animations"].append({"name": name, "channels": channels, "samplers": samplers})
    gl["buffers"] = [{"byteLength": len(blob)}]
    gl["asset"]["generator"] = "Vortex BuildSwordAttacks"
    enc = json.dumps(gl, separators=(",", ":")).encode()
    enc += b" " * ((-len(enc)) % 4)
    with open(OUT, "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(enc) + 8 + len(blob)))
        f.write(struct.pack("<II", len(enc), 0x4E4F534A) + enc)
        f.write(struct.pack("<II", len(blob), 0x004E4942) + bytes(blob))
    return OUT


if __name__ == "__main__" and bpy.app.background and "export" in sys.argv:
    print("VORTEX_SWORD_ATTACKS", export())
