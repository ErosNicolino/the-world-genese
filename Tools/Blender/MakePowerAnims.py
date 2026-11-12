"""As animações dos poderes das 35 espadas e dos 35 arcos (07/11, o usuário: "as animações próprias para cada uma"),
feitas no Blender, no esqueleto do herói de blocos (o do Mixamo), uma para cada arma.

Antes os poderes usavam a animação do secundário de outra arma, mas aquelas eram do esqueleto antigo (o Adventurer): no
herói de blocos não tocavam e todos caíam no mesmo giro da espada.

Cada animação é montada em trechos dos clipes que o herói já tem (o golpe de martelo, a estocada da adaga, a magia do
cajado, o pulo, o level up, o puxar e soltar do arco...), emendados com transição suave, e por cima as camadas da arma:
o giro do corpo inteiro, o pulo, o agachar, a inclinação para a frente ou para trás, a torção do tronco, os braços para
cima, o avanço e o recuo. O tipo do poder dá a forma (o golpe no chão pula e bate; o redemoinho gira N vezes; a chuva
de flechas mira para o céu); a arma dá o ritmo, a altura, o lado, a força e o arremate (Data/Rpg/swords.json e
bows.json: a chave, a raridade e o elemento sorteiam as variações, sempre iguais para a mesma arma).

blender -b --factory-startup -P Tools/Blender/MakePowerAnims.py -- <raiz> [chave ...]
Sai Data/Heroes/humano_Mixamo/Poderes_humano.glb (o esqueleto e uma faixa por animação: Poder_espada_<chave>_Full,
Poder_arco_<chave>_Full e, nas de carregar, Poder_<...>_Carga_Full) e poderes.json. Na Unreal: Tools/ImportPowerAnims.py.
"""
import json
import math
import os
import random
import sys
import zlib

import bpy
from mathutils import Quaternion, Vector

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
ROOT = ARGS[0] if ARGS else r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex"
CLASSES = "--classes" in ARGS
NPC = "--npc" in ARGS        # 08/11: os ofícios dos moradores da vila (Tools/Blender/npc_anims_hand.py), em laço   # 07/11: os 264 poderes de classe (Tools/Blender/class_anims_hand.py), num arquivo à parte
ONLY = [a for a in ARGS[1:] if not a.startswith("--")]
SRC = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo", "Hero_humano.glb")
OUT = os.path.join(ROOT, "Data", "Heroes", "humano_Mixamo")
P = "mixamorig:"
FPS = 30


def log(*a):
    print("PODER", *a)


bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.render.fps = FPS
bpy.ops.import_scene.gltf(filepath=SRC)
hero = next(o for o in bpy.data.objects if o.type == "ARMATURE")
body = next((o for o in bpy.data.objects if o.type == "MESH" and o.find_armature() == hero), None)
if not hero.animation_data:
    hero.animation_data_create()
for t in list(hero.animation_data.nla_tracks):
    hero.animation_data.nla_tracks.remove(t)
ACTIONS = {}
for a in bpy.data.actions:
    # o glTF importa "Idle" às vezes como "Idle_<armadura>": a chave é o nome do clipe
    ACTIONS[a.name.split("_Armature")[0].split("|")[-1]] = a
P_BONES = [pb.name for pb in hero.pose.bones]
HIPS = P + "Hips"
to_arm = hero.matrix_world.to_quaternion().inverted()
UP, SIDE, FRONT = to_arm @ Vector((0, 0, 1)), to_arm @ Vector((1, 0, 0)), to_arm @ Vector((0, -1, 0))
HIP_H = hero.data.bones[HIPS].head_local.length


def clip(name):
    a = ACTIONS.get(name)
    if not a:
        a = next((v for k, v in ACTIONS.items() if k.endswith(name)), None)
    assert a, "sem o clipe %s (tem: %s)" % (name, ", ".join(sorted(ACTIONS)[:20]))
    return a


def length(action):
    return max(1, int(round(action.frame_range[1] - action.frame_range[0])))


_cache = {}


def sample(name, frac):
    """A pose do clipe na fração (0 a 1) do comprimento dele: {osso: (giro, posição)}."""
    a = clip(name)
    f0, f1 = a.frame_range
    f = round(f0 + (f1 - f0) * max(0.0, min(1.0, frac)), 2)
    key = (a.name, f)
    if key in _cache:
        return _cache[key]
    hero.animation_data.action = a
    try:
        hero.animation_data.action_slot = a.slots[0]
    except Exception:
        pass
    scene.frame_set(int(f), subframe=f - int(f))
    bpy.context.view_layer.update()
    pose = {pb.name: (pb.rotation_quaternion.copy(), pb.location.copy()) for pb in hero.pose.bones}
    _cache[key] = pose
    return pose


def blend(a, b, w):
    return {n: (a[n][0].slerp(b[n][0], w), a[n][1].lerp(b[n][1], w)) for n in a}


def smooth(t):
    t = max(0.0, min(1.0, t))
    return t * t * t * (t * (6 * t - 15) + 10)


def turn(pose, bone, axis, deg):
    if bone not in pose or abs(deg) < 1e-4:
        return
    q, loc = pose[bone]
    rq = hero.data.bones[bone].matrix_local.to_quaternion()
    pose[bone] = (rq.inverted() @ Quaternion(axis, math.radians(deg)) @ rq @ q, loc)


def shift(pose, delta_arm):
    q, loc = pose[HIPS]
    rq = hero.data.bones[HIPS].matrix_local.to_quaternion()
    pose[HIPS] = (q, loc + rq.inverted() @ delta_arm)


def curve(t, keys):
    """Valor de uma curva por pontos [(t, v), ...] com transição suave entre eles."""
    if t <= keys[0][0]:
        return keys[0][1]
    for (t0, v0), (t1, v1) in zip(keys, keys[1:]):
        if t <= t1:
            return v0 + (v1 - v0) * smooth((t - t0) / max(1e-6, t1 - t0))
    return keys[-1][1]


def build(segments, layers, total=None):
    """segments: [(clipe, de, até, segundos)]: os trechos em sequência (cada um emenda no anterior em 0,08 s).
    layers: {nome: [(t, valor)]} em fração do tempo total: spin (graus em volta do eixo de cima), lift (fração da altura
    do quadril: o pulo; negativo agacha), lean (graus: + para a frente), twist (graus do tronco), arms (graus dos dois
    braços para cima/frente; armr e arml: um só), wings (os dois braços abertos para os lados), look (a cabeça: + olha
    para baixo), push (fração da altura do quadril: + para a frente, - recua), tilt (graus de lado)."""
    total = total or sum(s[3] for s in segments)
    frames = max(2, int(round(total * FPS)) + 1)
    poses = []
    starts = []
    acc = 0.0
    for s in segments:
        starts.append(acc)
        acc += s[3]
    for i in range(frames):
        t = i / FPS
        k = max(j for j in range(len(segments)) if starts[j] <= t + 1e-6)
        name, a, b, dur = segments[k]
        local = min(1.0, (t - starts[k]) / max(1e-6, dur))
        pose = dict(sample(name, a + (b - a) * local))
        # a emenda: os primeiros 0,08 s de cada trecho saem da pose em que o anterior terminou
        if k > 0 and t - starts[k] < .08:
            pn, pa, pb_, _ = segments[k - 1]
            pose = blend(sample(pn, pb_), pose, smooth((t - starts[k]) / .08))
        u = t / total
        val = {n: curve(u, keys) for n, keys in layers.items()}
        turn(pose, HIPS, UP, val.get("spin", 0))
        turn(pose, P + "Spine", SIDE, -val.get("lean", 0) * .5)
        turn(pose, P + "Spine1", SIDE, -val.get("lean", 0) * .5)
        turn(pose, P + "Spine1", UP, val.get("twist", 0))
        turn(pose, P + "Spine", FRONT, val.get("tilt", 0))
        for side in ("Left", "Right"):
            turn(pose, P + side + "Arm", SIDE, -val.get("arms", 0) - val.get("arm" + side[0].lower(), 0))
        # wings: os dois braços abertos para os lados (as asas da fênix, do grifo)
        # (60%: os clipes de base já levantam os braços; 85 inteiros passavam por cima da cabeça)
        turn(pose, P + "LeftArm", FRONT, val.get("wings", 0) * .6)
        turn(pose, P + "RightArm", FRONT, -val.get("wings", 0) * .6)
        # 08/11 (os ofícios dos moradores): os cotovelos (elbows nos dois, elbr e elbl: um só; + dobra para a frente)
        for side in ("Left", "Right"):
            turn(pose, P + side + "ForeArm", SIDE, -val.get("elbows", 0) - val.get("elb" + side[0].lower(), 0))
        turn(pose, P + "Head", SIDE, -val.get("look", 0))
        lift, push = val.get("lift", 0), val.get("push", 0)
        if lift or push:
            shift(pose, UP * (lift * HIP_H) + FRONT * (push * HIP_H))
        if lift < 0:
            # agachando: as coxas para a frente e os joelhos dobrados (os pés ficam no chão)
            for side in ("Left", "Right"):
                turn(pose, P + side + "UpLeg", SIDE, lift * 140)
                turn(pose, P + side + "Leg", SIDE, -lift * 240)
        poses.append(pose)
    return poses


def write(name, poses):
    out = bpy.data.actions.new(name)
    hero.animation_data.action = out
    for i, pose in enumerate(poses):
        for n, (q, loc) in pose.items():
            pb = hero.pose.bones[n]
            pb.rotation_quaternion = q
            pb.keyframe_insert("rotation_quaternion", frame=i)
            if n == HIPS:
                pb.location = loc
                pb.keyframe_insert("location", frame=i)
    track = hero.animation_data.nla_tracks.new()
    track.name = name
    strip = track.strips.new(name, 0, out)
    try:
        if out.slots and not strip.action_slot:
            strip.action_slot = out.slots[0]
    except Exception:
        pass
    hero.animation_data.action = None
    return (len(poses) - 1) / FPS


# ---- as formas (pelo tipo do poder). v: as variações da arma (o sorteio dela)

def sword_anim(kind, v, vals):
    d = v["tempo"]
    s = v["side"]
    if kind == "dash":
        return [("Atk_Espada_2", 0, .45, .22 * d), ("Atk_Adagas_3", .1, 1, .3 * d), ("Atk_Espada_1", .55, 1, .3 * d)], {
            "lean": [(0, 0), (.25, -8), (.45, 22 * v["force"]), (1, 0)], "push": [(0, 0), (.3, -.08), (.5, .32), (1, 0)],
            "lift": [(0, 0), (.25, -.12), (.5, -.05), (1, 0)], "twist": [(0, 0), (.3, -20 * s), (.55, 15 * s), (1, 0)]}
    if kind == "flurry":
        n = int(vals.get("count", 5))
        segs = [("Atk_Adagas_1" if i % 2 == 0 else "Atk_Adagas_2", .1, .9, .16 * d) for i in range(n)]
        segs.append(("Atk_Espada_1", .6, 1, .25 * d))
        return segs, {"lean": [(0, 0), (.1, 12), (.85, 14), (1, 0)], "twist": [(i / n, (12 if i % 2 else -12) * s) for i in range(n)] + [(1, 0)],
                      "push": [(0, 0), (.15, .1), (.85, .12), (1, 0)], "lift": [(0, -.06), (1, 0)]}
    if kind == "sweep":
        return [("Atk_Martelo_2", 0, .4, .3 * d), ("Atk_Espada_3_Full", .15, .75, .45 * d), ("Atk_Espada_1", .7, 1, .25 * d)], {
            "lift": [(0, 0), (.3, -.18), (.7, -.15), (1, 0)], "twist": [(0, 0), (.3, -45 * s), (.75, 50 * s), (1, 0)],
            "spin": [(0, 0), (.3, -25 * s), (.8, 40 * s), (1, 0)], "lean": [(0, 0), (.5, 10), (1, 0)]}
    if kind == "groundslam":
        h = .35 * v["height"]
        return [("Jump_Start", 0, 1, .16 * d), ("Atk_Martelo_1", 0, .45, .3 * d), ("Atk_Martelo_1", .45, 1, .28 * d), ("Jump_Land", 0, 1, .3 * d)], {
            "lift": [(0, 0), (.12, -.1), (.35, h), (.55, h * .6), (.68, -.25), (.85, -.2), (1, 0)],
            "lean": [(0, 0), (.4, -15), (.65, 32 * v["force"]), (.85, 20), (1, 0)], "arms": [(0, 0), (.35, 30), (.6, 0), (1, 0)]}
    if kind == "reap":
        return [("Atk_Espada_3_Full", 0, 1, .55 * d), ("Atk_Espada_2", .5, 1, .25 * d)], {
            "lift": [(0, -.05), (.2, -.32), (.75, -.3), (1, 0)], "spin": [(0, 0), (.7, 360 * s), (1, 360 * s)],
            "lean": [(0, 0), (.3, 22), (.8, 18), (1, 0)]}
    if kind == "shockwave":
        return [("LevelUp", .1, .45, .35 * d), ("Atk_Martelo_3_Full", .35, .8, .4 * d), ("Atk_Martelo_2", .7, 1, .25 * d)], {
            "arms": [(0, 0), (.3, 70), (.45, 60), (.6, 0), (1, 0)], "lean": [(0, 0), (.3, -12), (.6, 30 * v["force"]), (1, 0)],
            "lift": [(0, 0), (.3, .08), (.6, -.22), (.85, -.15), (1, 0)]}
    if kind == "whirl":
        turns = max(1, int(round(vals.get("hits", 4) * .5 * v["force"])))
        dur = max(1.2, vals.get("time", 2.0)) * d
        segs = [("Atk_Espada_3_Full", 0, 1, dur / turns) for _ in range(turns)]
        return segs, {"spin": [(0, 0), (1, 360 * turns * s)], "lift": [(0, -.08), (.5, -.12 + .1 * v["height"]), (1, -.04)],
                      "arms": [(0, 10), (1, 10)], "lean": [(0, 5), (1, 0)]}
    if kind == "runeseal":
        return [("Atk_Cajado_1", 0, .7, .35 * d), ("Habilidade_Corte", .55, .75, .35 * d), ("Atk_Martelo_2", .4, 1, .35 * d)], {
            "arms": [(0, 0), (.35, 45), (.55, 20), (1, 0)], "lift": [(0, 0), (.6, -.1), (.8, -.28), (1, 0)],
            "lean": [(0, 0), (.35, -10), (.8, 28), (1, 0)], "twist": [(0, 0), (.4, 25 * s), (1, 0)]}
    if kind == "overload":
        return [("Atk_Cajado_3", .35, 1, .5 * d), ("Atk_Adagas_3", .3, 1, .3 * d)], {
            "push": [(0, 0), (.15, -.12), (.6, .2), (1, 0)], "lean": [(0, -8), (.6, 18), (1, 0)], "twist": [(0, 25 * s), (.5, -10 * s), (1, 0)]}
    if kind == "orbit":
        return [("LevelUp", 0, .8, .6 * d), ("Emote_Vitoria", .05, .3, .5 * d)], {
            "arms": [(0, 0), (.35, 80), (.8, 60), (1, 0)], "spin": [(0, 0), (1, 180 * s)], "lift": [(0, 0), (.4, .08), (1, 0)]}
    if kind == "firewall":
        return [("Atk_Martelo_2", 0, .4, .25 * d), ("Atk_Espada_2", .2, 1, .4 * d), ("Grito", .2, .45, .35 * d)], {
            "lean": [(0, 15), (.4, -20), (.7, -10), (1, 0)], "arms": [(0, 0), (.45, 55), (.8, 40), (1, 0)],
            "lift": [(0, -.15), (.4, .05), (1, 0)], "twist": [(0, -30 * s), (.45, 30 * s), (1, 0)]}
    if kind == "holyslam":
        return [("LevelUp", .1, .5, .4 * d), ("Atk_Martelo_1", .4, 1, .35 * d), ("Atk_Espada_1", .7, 1, .2 * d)], {
            "arms": [(0, 0), (.35, 90), (.5, 80), (.65, 0), (1, 0)], "lift": [(0, 0), (.35, .15 * v["height"]), (.65, -.25), (1, 0)],
            "lean": [(0, 0), (.35, -18), (.65, 30), (1, 0)]}
    if kind == "heal":
        return [("Emote_Saudar", .1, .5, .5 * d), ("LevelUp", .2, .7, .6 * d)], {
            "arms": [(0, 0), (.6, 85), (1, 30)], "lean": [(0, 0), (.6, -12), (1, 0)], "lift": [(0, 0), (.6, .06), (1, 0)]}
    raise KeyError(kind)


def bow_anim(kind, v, vals):
    d = v["tempo"]
    s = v["side"]
    draw, rel = ("Arco_Draw", 0, 1), ("Arco_Release", 0, 1)
    if kind == "volley":
        n = int(vals.get("count", 5))
        spread = vals.get("spread", 40)
        return [(*draw, .35 * d), (*rel, .3 * d), ("Arco_Aim", .3, .5, .2 * d)], {
            "twist": [(0, -spread * .5 * s), (.45, -spread * .5 * s), (.75, spread * .5 * s), (1, 0)], "lean": [(0, 0), (.5, -6), (1, 0)],
            "lift": [(0, 0), (.45, -.06 - .01 * n), (1, 0)]}
    if kind == "pierce":
        return [(*draw, .55 * d), ("Arco_Aim", .4, .6, .15 * d), (*rel, .3 * d)], {
            "lift": [(0, 0), (.6, -.18), (.8, -.1), (1, 0)], "push": [(0, 0), (.6, .05), (.75, -.22 * v["force"]), (1, 0)],
            "lean": [(0, 0), (.6, 10), (.75, -12), (1, 0)]}
    if kind == "explosive":
        return [(*draw, .4 * d), (*rel, .3 * d)], {"lift": [(0, 0), (.45, .14 * v["height"]), (.7, 0), (1, 0)],
                                                   "lean": [(0, 0), (.5, -14), (1, 0)], "twist": [(0, 10 * s), (1, 0)]}
    if kind == "arrowrain":
        return [(*draw, .45 * d), (*rel, .3 * d), ("Arco_Aim", .5, .7, .25 * d)], {
            "lean": [(0, 0), (.45, -24 * v["force"]), (.8, -18), (1, 0)], "lift": [(0, 0), (.45, -.08), (1, 0)]}
    if kind == "burst3":
        n = int(vals.get("count", 3))
        segs = []
        for _ in range(n):
            segs += [("Arco_Draw", .4, 1, .14 * d), ("Arco_Release", 0, 1, .1 * d)]
        return segs, {"twist": [(i / n, (8 if i % 2 else -8) * s) for i in range(n)] + [(1, 0)], "lean": [(0, 5), (1, 0)],
                      "push": [(i / n, -.03 * (i % 2)) for i in range(n)] + [(1, 0)]}
    if kind == "runeseal":
        return [(*draw, .4 * d), (*rel, .3 * d), ("Atk_Cajado_2", .5, 1, .3 * d)], {
            "lift": [(0, 0), (.5, -.2), (1, 0)], "lean": [(0, 0), (.5, 15), (1, 0)], "arms": [(0, 0), (.8, 30), (1, 0)]}
    if kind == "overload":
        return [(*draw, .6 * d), (*rel, .35 * d)], {"push": [(0, 0), (.6, .04), (.75, -.18), (1, 0)], "lean": [(0, 0), (.6, 10), (.8, -10), (1, 0)]}
    if kind == "orbit":
        return [("LevelUp", 0, .8, .6 * d), ("Arco_Aim", .3, .5, .3 * d)], {"arms": [(0, 0), (.4, 70), (1, 0)], "spin": [(0, 0), (1, 160 * s)]}
    if kind == "firewall":
        return [(*draw, .4 * d), (*rel, .3 * d), ("Grito", .2, .4, .3 * d)], {"lift": [(0, -.2), (.5, -.15), (1, 0)], "twist": [(0, -35 * s), (.6, 35 * s), (1, 0)]}
    if kind == "fireball":
        return [(*draw, .45 * d), (*rel, .3 * d)], {"push": [(0, -.06), (.55, .22), (1, 0)], "lean": [(0, -6), (.55, 18), (1, 0)], "lift": [(0, 0), (.5, -.08), (1, 0)]}
    if kind == "heal":
        return [("Emote_Saudar", .1, .5, .45 * d), ("LevelUp", .2, .7, .55 * d)], {"arms": [(0, 0), (.6, 80), (1, 25)], "lift": [(0, 0), (.6, .05), (1, 0)]}
    raise KeyError(kind)


# 07/11 (o usuário: "bem detalhado, uma por uma"): as animações feitas à mão, arma por arma (no lugar da forma do tipo
# com o sorteio). Os tempos casam com o poder (o "delay" do swords.json) e com o efeito (BuildPowerFx.py, HAND)
def mare_baixa():
    # agacha com a espada para trás, varre rente ao chão, empurra a onda para a frente (a onda sai em 0,5 s) e volta
    return [("Atk_Martelo_2", 0, .45, .3), ("Atk_Espada_3_Full", .1, .45, .25), ("Atk_Adagas_3", .25, .8, .3), ("Atk_Espada_1", .7, 1, .45)], {
        "lift": [(0, 0), (.15, -.28), (.4, -.3), (.5, -.15), (.75, -.05), (1, 0)],
        "twist": [(0, 0), (.2, -50), (.4, 40), (.55, 0), (1, 0)],
        "lean": [(0, 0), (.2, 10), (.4, 25), (.5, 30), (.65, 12), (1, 0)],
        "arms": [(0, 0), (.4, 0), (.5, 45), (.65, 50), (1, 0)],
        "push": [(0, 0), (.4, 0), (.5, .25), (.7, .15), (1, 0)]}


def marco_guardiao():
    # ergue a espada com as duas mãos, um pulinho, crava no chão agachado (em 0,62 s, a batida), segura firme e levanta
    return [("Atk_Martelo_1", 0, .4, .3), ("Pegar", .15, .5, .32), ("Pegar", .5, .5, .45), ("Pegar", .5, 1, .43)], {
        "arms": [(0, 0), (.12, 60), (.2, 85), (.3, 40), (.41, 0), (1, 0)],
        "lift": [(0, 0), (.15, .12), (.21, .16), (.3, 0), (.41, -.3), (.7, -.32), (.85, -.15), (1, 0)],
        "lean": [(0, 0), (.2, -15), (.41, 35), (.7, 32), (.85, 10), (1, 0)],
        "twist": [(0, 0), (.45, 4), (.5, -4), (.55, 3), (.6, -2), (.65, 0), (1, 0)]}


HAND = {"espada_incomum_3": mare_baixa, "espada_incomum_4": marco_guardiao}
# 07/11: as outras 66, uma por uma (Tools/Blender/power_anims_hand.py: {chave: (trechos, camadas)})
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "power_anims_hand.py"), encoding="utf-8").read())
HAND.update({k: (lambda v=v: v) for k, v in HAND_DATA.items()})


def hold_anim(bow):
    """A pose de carregar (a sobrecarga): a mão para trás tremendo de leve, em laço."""
    if bow:
        return [("Arco_Draw", .7, 1, .6), ("Arco_Draw", 1, .85, .6)], {"lean": [(0, 4), (.5, 6), (1, 4)]}
    return [("Atk_Cajado_3", .05, .35, .6), ("Atk_Cajado_3", .35, .05, .6)], {"lift": [(0, -.08), (.5, -.12), (1, -.08)], "twist": [(0, 20), (.5, 28), (1, 20)]}


def variations(key, rarity, element):
    rnd = random.Random(zlib.crc32(key.encode()))
    rank = ["common", "uncommon", "rare", "epic", "legendary", "unique", "divine"].index(rarity)
    return {"tempo": round(rnd.uniform(.85, 1.15) * (1.05 - .03 * rank), 3), "side": rnd.choice((1, -1)),
            "force": round(.85 + .07 * rank + rnd.uniform(-.08, .08), 3), "height": round(.8 + .08 * rank + rnd.uniform(-.1, .1), 3),
            "flourish": rank >= 3 and rnd.random() < .7}


def finish(segs, layers, v):
    """O arremate das raras para cima: a arma gira na mão (um meio giro do braço) e uma pose de vitória curta."""
    if not v["flourish"]:
        return segs, layers
    segs = list(segs) + [("Emote_Provocar", .1, .35, .35)]
    return segs, layers


def export(glb_name, json_name, report):
    for t in hero.animation_data.nla_tracks:
        t.mute = False
    bpy.ops.object.select_all(action="DESELECT")
    hero.select_set(True)
    if body:
        body.select_set(True)
    bpy.context.view_layer.objects.active = hero
    glb = os.path.join(OUT, glb_name)
    bpy.ops.export_scene.gltf(filepath=glb, use_selection=True, export_format="GLB", export_animations=True,
                              export_animation_mode="NLA_TRACKS", export_skins=True, export_apply=False, export_yup=True,
                              export_force_sampling=True, export_frame_step=1, export_def_bones=False)
    json.dump(report, open(os.path.join(OUT, json_name), "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    log("exportado", glb, len(report), "animações")


if __name__ == "__main__" and NPC:
    # Npc_humano.glb + npc.json: os clipes em laço do que cada morador faz parado (AVortexNpc, npc_schedule.json)
    exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "npc_anims_hand.py"), encoding="utf-8").read())
    report = {}
    for name, (period, layers) in NPC_ANIMS.items():
        if ONLY and name not in ONLY:
            continue
        secs = write(name, build([("Idle", 0, 0, period)], layers))
        report[name] = {"item": name, "kind": "npc", "seconds": round(secs, 3)}
        log("%-20s %.2f s" % (name, secs))
    export("Npc_humano.glb", "npc.json", report)
    sys.exit(0)

if __name__ == "__main__" and CLASSES:
    # Classes_humano.glb + classes.json (o instante do golpe de cada poder, em segundos: o servidor espera por ele)
    exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "class_anims_hand.py"), encoding="utf-8").read())
    skills = json.load(open(os.path.join(ROOT, "Data", "Rpg", "rpg.json"), encoding="utf-8"))["classSkills"]["classes"]
    ids = [s["id"] for v in skills.values() for s in v]
    missing = [i for i in ids if i not in CLASS]
    assert not missing, "sem animação: %s" % missing
    report = {}
    for sid in ids:
        if ONLY and sid not in ONLY:
            continue
        segs, layers, hit = class_anim(CLASS[sid])
        name = "Poder_classe_%s_Full" % sid
        secs = write(name, build(segs, layers))
        report[name] = {"item": sid, "seconds": round(secs, 3), "hit": round(hit * secs, 3)}
        log("%-40s %.2f s golpe %.2f" % (name, secs, hit * secs))
    export("Classes_humano.glb", "classes.json", report)
    sys.exit(0)

if __name__ == "__main__":
    swords = json.load(open(os.path.join(ROOT, "Data", "Rpg", "swords.json"), encoding="utf-8"))["swords"]
    bows = json.load(open(os.path.join(ROOT, "Data", "Rpg", "bows.json"), encoding="utf-8"))["bows"]
    report = {}
    for table, kindname, fn in ((swords, "espada", sword_anim), (bows, "arco", bow_anim)):
        for item, w in table.items():
            kind = w["power"].get("kind")
            if not kind or (ONLY and w["key"] not in ONLY):
                continue
            v = variations("%s_%s" % (kindname, w["key"]), w["rarity"], w["element"])
            hand = HAND.get("%s_%s" % (kindname, w["key"]))
            if hand:
                segs, layers = hand()
            else:
                segs, layers = fn(kind, v, w["power"].get("values", {}))
                segs, layers = finish(segs, layers, v)
            name = "Poder_%s_%s_Full" % (kindname, w["key"])
            secs = write(name, build(segs, layers))
            report[name] = {"item": item, "kind": kind, "seconds": round(secs, 3), "variation": v}
            log("%-30s %-10s %.2f s" % (name, kind, secs))
            if kind == "overload":
                held = HAND_DATA.get("%s_%s_carga" % (kindname, w["key"]))
                hs, hl = held if held else hold_anim(kindname == "arco")
                hname = "Poder_%s_%s_Carga_Full" % (kindname, w["key"])
                report[hname] = {"item": item, "kind": "hold", "seconds": round(write(hname, build(hs, hl)), 3)}
    for t in hero.animation_data.nla_tracks:
        t.mute = False
    bpy.ops.object.select_all(action="DESELECT")
    hero.select_set(True)
    if body:
        body.select_set(True)
    bpy.context.view_layer.objects.active = hero
    glb = os.path.join(OUT, "Poderes_humano.glb")
    bpy.ops.export_scene.gltf(filepath=glb, use_selection=True, export_format="GLB", export_animations=True,
                              export_animation_mode="NLA_TRACKS", export_skins=True, export_apply=False, export_yup=True,
                              export_force_sampling=True, export_frame_step=1, export_def_bones=False)
    json.dump(report, open(os.path.join(OUT, "poderes.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    log("exportado", glb, len(report), "animações")
