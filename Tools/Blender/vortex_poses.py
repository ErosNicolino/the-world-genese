"""Posador das armas do Vortex no Blender: põe as mãos do aventureiro onde a pose pede e grava os clipes.

Usado por Tools/Blender/BuildWeaponClips.py. O esqueleto é o do aventureiro convertido do Godot
(Data/GodotWeapons/Adventurer_WeaponClips.glb). No Blender (Z para cima) o herói olha para -Y, a mão
direita fica em -X; os ossos em repouso têm X = +X do mundo, Y = para cima e Z = para a frente.

Cada pose é um dicionário (todos os campos opcionais):
  hips, torso, head: (giro, inclinação para a frente, rolagem) em graus; o tronco soma ao quadril
  root: (giro em graus, para a frente m, para cima m) — só nos clipes de corpo inteiro (*_Full)
  r, l: mão direita / esquerda:
      "at": (direita, frente, cima) em metros, no espaço do tronco (origem na base do tronco)
      "tip": para onde a ponta da arma aponta (direita, frente, cima), no espaço do corpo (não gira com o tronco)
      "side": para onde vai o lado da arma (gira em volta da ponta), idem
      "pole": para onde o cotovelo dobra (direita, frente, cima), no espaço do tronco
  l: {"on": d} = a mão esquerda segura a arma da direita a d metros da pegada (negativo = para o cabo)
  legs: {"spread": graus, "bend": graus, "step": graus (perna direita à frente)}
O tempo de cada chave vai em "t" (segundos). Entre as chaves a pose é interpolada suavemente e o
braço é resolvido quadro a quadro (IK de dois ossos, como o AdventurerActor.Arms.cs do Godot fazia
só para a lança).
"""
import math
import os

import bpy
from mathutils import Matrix, Quaternion, Vector

ROOT = os.environ.get("VORTEX_ROOT", r"C:\Users\Valorant Niko\Documents\ChatGPT\Mangas\Vortex-UE5-migracao\Vortex")
RIG = os.path.join(ROOT, "Data", "GodotWeapons", "Adventurer_WeaponClips.glb")
MODELS = os.path.join(ROOT, "Data", "VortexWeapons", "Models")
FPS = 30

# direções do personagem no espaço da armadura
R_ = Vector((-1, 0, 0))
F_ = Vector((0, -1, 0))
U_ = Vector((0, 0, 1))
# a arma na mão: o modelo glb (Y para cima) preso ao soquete do jeito da Unreal
ATTACH = Matrix.Rotation(math.radians(-90), 4, 'X')


def char(v):
    """(direita, frente, cima) -> vetor da armadura."""
    return R_ * v[0] + F_ * v[1] + U_ * v[2]


class Rig:
    def __init__(self):
        self.arm = bpy.data.objects.get("Armature")
        if not self.arm:
            for o in list(bpy.data.objects):
                bpy.data.objects.remove(o)
            bpy.ops.import_scene.gltf(filepath=RIG)
            self.arm = bpy.data.objects["Armature"]
        self.pb = self.arm.pose.bones
        if not self.arm.animation_data:
            self.arm.animation_data_create()
        self.reset()
        self.rest = {b.name: b.matrix.copy() for b in self.pb}
        self.weapon_r = self.weapon_l = None
        self.tip_local = Vector((0, 0, 1))
        self.side_local = Vector((1, 0, 0))
        self.weapon_len = 1.0

    def update(self):
        bpy.context.view_layer.update()

    def reset(self):
        self.arm.animation_data.action = None
        for b in self.pb:
            b.rotation_mode = 'QUATERNION'
            b.matrix_basis = Matrix()
        self.update()

    # ---- arma ------------------------------------------------------------------------------------
    def clear_weapons(self):
        for o in list(bpy.data.objects):
            if o.name.startswith("Weapon_"):
                bpy.data.objects.remove(o)

    def _import(self, path, bone, name):
        before = set(bpy.data.objects)
        bpy.ops.import_scene.gltf(filepath=path)
        new = [o for o in bpy.data.objects if o not in before]
        mesh = next(o for o in new if o.type == 'MESH')
        for o in new:
            if o is not mesh:
                bpy.data.objects.remove(o)
        mesh.name = name
        mesh.parent = self.arm
        mesh.parent_type = 'BONE'
        mesh.parent_bone = bone
        self.update()
        mesh.matrix_world = self.arm.matrix_world @ self.pb[bone].matrix @ ATTACH
        return mesh

    def equip(self, weapon_id, left_model=None):
        """A arma do tipo na mão direita (e a da esquerda: a cópia das armas de par, o escudo)."""
        self.reset()
        self.clear_weapons()
        self.weapon_r = self.weapon_l = None
        right = os.path.join(MODELS, "W_%s.glb" % weapon_id)
        if os.path.exists(right):
            self.weapon_r = self._import(right, "Socket_Weapon_R", "Weapon_R")
        if left_model:
            left = os.path.join(MODELS, "W_%s.glb" % left_model)
            if os.path.exists(left):
                self.weapon_l = self._import(left, "Hand_L", "Weapon_L")
                if not left_model.endswith("_L"):
                    # peça feita para o soquete da mão direita (a garrafinha): no soquete espelhado da esquerda
                    offset = self.pb["Hand_L"].matrix.inverted() @ mirror(self.pb["Socket_Weapon_R"].matrix)
                    self.weapon_l.matrix_world = self.arm.matrix_world @ self.pb["Hand_L"].matrix @ offset @ ATTACH
        # eixo comprido da arma no espaço do soquete (do cabo para a ponta)
        if self.weapon_r:
            inv = (self.arm.matrix_world @ self.pb["Socket_Weapon_R"].matrix).inverted()
            pts = [inv @ (self.weapon_r.matrix_world @ v.co) for v in self.weapon_r.data.vertices]
            far = max(pts, key=lambda v: v.length)
            self.weapon_len = far.length
            # direção média dos pontos longe da pegada (evita a inclinação de uma gema na ponta)
            ahead = [p for p in pts if p.dot(far) > 0 and p.length > .5 * far.length]
            tip = sum(ahead, Vector()) / max(1, len(ahead))
            self.tip_local = tip.normalized() if tip.length > 1e-4 else Vector((0, 0, 1))
            side = Vector((1, 0, 0))
            self.side_local = (side - self.tip_local * side.dot(self.tip_local)).normalized()

    # ---- ossos -----------------------------------------------------------------------------------
    def head(self, name):
        return self.pb[name].matrix.translation.copy()

    def turn(self, name, q, pivot=None):
        """Gira o osso (e os filhos) por q no espaço da armadura, em volta da cabeça dele."""
        b = self.pb[name]
        m = b.matrix.copy()
        p = pivot if pivot is not None else m.translation
        r = q.to_matrix().to_4x4()
        b.matrix = Matrix.Translation(p) @ r @ Matrix.Translation(-p) @ m
        self.update()

    def aim(self, name, child_point, target):
        """Gira o osso para o ponto filho (cotovelo, punho) ir na direção do alvo."""
        h = self.head(name)
        a = (child_point - h)
        b = (target - h)
        if a.length < 1e-6 or b.length < 1e-6:
            return
        self.turn(name, a.normalized().rotation_difference(b.normalized()))

    def frame(self, quat):
        """Direções do personagem giradas por quat (o tronco virado)."""
        return quat @ R_, quat @ F_, quat @ U_

    # ---- pose ------------------------------------------------------------------------------------
    def euler(self, ypr, basis):
        r, f, u = basis
        y, p, ro = ypr
        return Quaternion(u, math.radians(y)) @ Quaternion(r, math.radians(-p)) @ Quaternion(f, math.radians(ro))

    def solve_arm(self, side, grip, tip, flat, pole):
        """Mão (soquete) em grip com a ponta da arma em tip e o lado em flat; cotovelo para pole."""
        s = "R" if side == "r" else "L"
        upper, fore, hand = "UpperArm_" + s, "Forearm_" + s, "Hand_" + s
        # o punho vai onde a mão precisa estar para o soquete cair na pegada
        tip = tip.normalized()
        flat = (flat - tip * flat.dot(tip)).normalized()
        local_tip, local_side = self.tip_local, self.side_local
        if s == "L":
            local_tip = Vector((-local_tip.x, local_tip.y, local_tip.z))
            local_side = Vector((-local_side.x, local_side.y, local_side.z))
        want = basis_from(tip, flat) @ basis_from(local_tip, local_side).transposed()   # rotação do soquete
        sock_rest = self.rest["Socket_Weapon_R"] if s == "R" else mirror(self.rest["Socket_Weapon_R"])
        hand_rest = self.rest[hand]
        offset = hand_rest.inverted() @ sock_rest                                    # soquete no espaço da mão
        hand_rot = want @ offset.to_3x3().inverted()
        wrist = grip - hand_rot @ offset.translation
        shoulder = self.head(upper)
        elbow_now, wrist_now = self.head(fore), self.head(hand)
        a, b = (elbow_now - shoulder).length, (wrist_now - elbow_now).length
        to = wrist - shoulder
        d = max(.02, min(to.length, a + b - .002))
        dirv = to.normalized()
        bend = pole - dirv * pole.dot(dirv)
        bend = bend.normalized() if bend.length > 1e-5 else -U_
        cos = max(-1, min(1, (a * a + d * d - b * b) / (2 * a * d)))
        elbow = shoulder + dirv * (a * cos) + bend * (a * math.sqrt(1 - cos * cos))
        self.aim(upper, elbow_now, elbow)
        self.aim(fore, self.head(hand), shoulder + dirv * d)
        # o antebraço gira no próprio eixo (como o de verdade) e leva a torção da mão: no pulso fica só a
        # dobra, senão a mão-bloco aparecia virada em relação ao antebraço ("punho quebrado")
        fb = self.pb[fore]
        axis = (self.head(hand) - self.head(fore)).normalized()
        follow = fb.matrix.to_quaternion() @ self.rest[fore].to_quaternion().inverted() @ hand_rest.to_quaternion()
        delta = hand_rot.to_quaternion() @ follow.inverted()
        # a mão em repouso segue o antebraço em repouso; delta = o quanto a mão quer girar além dele
        v = Vector((delta.x, delta.y, delta.z))
        p = axis * v.dot(axis)
        twist = Quaternion((delta.w, p.x, p.y, p.z))
        if twist.magnitude > 1e-6:
            twist.normalize()
            self.turn(fore, twist)
        # a mão: rotação inteira no espaço da armadura, na posição que o antebraço deixou
        hb = self.pb[hand]
        m = hand_rot.to_4x4()
        m.translation = hb.matrix.translation
        hb.matrix = m
        self.update()

    def apply(self, pose):
        self.reset()
        base = (R_, F_, U_)
        root = pose.get("root")
        if root:
            self.pb["Root"].matrix = Matrix.Translation(char((0, root[1], root[2]))) @ \
                Quaternion(U_, math.radians(root[0])).to_matrix().to_4x4() @ self.rest["Root"]
            self.update()
        rq = Quaternion(U_, math.radians(root[0])) if root else Quaternion()
        hq = rq @ self.euler(pose.get("hips", (0, 0, 0)), self.frame(rq))
        if pose.get("hips"):
            self.turn("Hips", hq @ rq.inverted())
        tq = hq @ self.euler(pose.get("torso", (0, 0, 0)), self.frame(hq))
        if pose.get("torso"):
            self.turn("Torso", tq @ hq.inverted())
        legs = pose.get("legs")
        if legs:
            self.legs(legs, hq)
        if pose.get("head"):
            nq = tq @ self.euler(pose["head"], self.frame(tq))
            self.turn("Head", nq @ tq.inverted())
        origin = self.head("Torso")
        to_world = lambda v: origin + tq @ char(v)
        # a ponta e o lado da arma vão no espaço do corpo inteiro (raiz): a lança aponta para o
        # inimigo mesmo com o tronco de lado; o cotovelo vai no espaço do tronco
        dir_world = lambda v: rq @ char(v)
        pole_world = lambda v: tq @ char(v)
        def side_of(h, default):
            # "edge": para onde vai o fio (o lado largo da arma: o fio da lâmina, a cabeça do machado)
            if "edge" in h:
                return dir_world(h["edge"]).cross(dir_world(h.get("tip", (0, 1, 0))))
            return dir_world(h.get("side", default))
        r = pose.get("r")
        if r:
            self.solve_arm("r", to_world(r["at"]), dir_world(r.get("tip", (0, 1, 0))), side_of(r, (1, 0, 0)),
                           pole_world(r.get("pole", (.6, -.2, -1))))
        l = pose.get("l")
        if l:
            if "on" in l and self.weapon_r:
                sock = self.pb["Socket_Weapon_R"].matrix
                tip = (sock.to_3x3() @ self.tip_local).normalized()
                side = (sock.to_3x3() @ self.side_local).normalized()
                grip = sock.translation + tip * l["on"]
                ltip = tip if l.get("same", True) else -tip
                self.solve_arm("l", grip, ltip, side if not l.get("flip") else -side, pole_world(l.get("pole", (-.6, -.2, -1))))
            else:
                self.solve_arm("l", to_world(l["at"]), dir_world(l.get("tip", (0, 1, 0))), side_of(l, (-1, 0, 0)),
                               pole_world(l.get("pole", (-.6, -.2, -1))))

    def legs(self, legs, hq):
        r, f, u = self.frame(hq)
        spread, bend, step = legs.get("spread", 0), legs.get("bend", 0), legs.get("step", 0)
        drop = legs.get("drop", 0)
        if drop:
            self.pb["Hips"].matrix = Matrix.Translation(-U_ * drop) @ self.pb["Hips"].matrix
            self.update()
        for s, sign in (("R", 1), ("L", -1)):
            q = Quaternion(f, math.radians(-spread * sign)) @ Quaternion(r, math.radians(-(step * sign) - bend * .6))
            self.turn("Thigh_" + s, q)
            self.turn("Shin_" + s, Quaternion(r, math.radians(bend * 1.2)))
            self.turn("Foot_" + s, Quaternion(r, math.radians(-bend * .6 + step * sign * .5)))

    # ---- clipes ----------------------------------------------------------------------------------
    def bake(self, name, keys, loop=False):
        """Grava o clipe: as chaves interpoladas e resolvidas quadro a quadro."""
        if name in bpy.data.actions:
            bpy.data.actions.remove(bpy.data.actions[name])
        act = bpy.data.actions.new(name)
        act.use_fake_user = True
        self.arm.animation_data.action = act
        end = keys[-1]["t"]
        frames = max(1, int(round(end * FPS)))
        names = [b.name for b in self.pb]
        for i in range(frames + 1):
            t = i / FPS
            pose = sample(keys, t)
            self.arm.animation_data.action = None
            self.apply(pose)
            mats = {n: self.pb[n].matrix_basis.copy() for n in names}
            self.arm.animation_data.action = act
            for n in names:
                b = self.pb[n]
                b.matrix_basis = mats[n]
                b.keyframe_insert("rotation_quaternion", frame=i, group=n)
                b.keyframe_insert("location", frame=i, group=n)
        if loop:
            act["loop"] = True
        self.arm.animation_data.action = act
        return act

    def pose_at(self, act, frame):
        self.arm.animation_data.action = act
        bpy.context.scene.frame_set(frame)
        self.update()


def basis_from(a, b):
    """Matriz com colunas (b, a×b... ) para levar (tip, side) a uma base ortonormal."""
    a = a.normalized()
    b = (b - a * b.dot(a)).normalized()
    c = b.cross(a)
    m = Matrix((b, a, c)).transposed()
    return m


def mirror(m):
    s = Matrix.Diagonal((-1, 1, 1, 1))
    return s @ m @ s


# ---- interpolação ----------------------------------------------------------------------------------
NEUTRAL = {"root": (0, 0, 0), "hips": (0, 0, 0), "torso": (0, 0, 0), "head": (0, 0, 0),
           "legs": {"spread": 0, "bend": 0, "step": 0, "drop": 0}}

def ease(x):
    return x * x * (3 - 2 * x)


def lerp(a, b, t):
    if isinstance(a, (int, float)):
        return a + (b - a) * t
    if isinstance(a, dict):
        out = {}
        for k in set(a) | set(b):
            if k in a and k in b:
                out[k] = lerp(a[k], b[k], t)
            elif k in NEUTRAL:
                # campo que só uma das chaves tem: sai do neutro (sem salto)
                out[k] = lerp(a.get(k, NEUTRAL[k]), b.get(k, NEUTRAL[k]), t)
            else:
                out[k] = a.get(k, b.get(k))
        return out
    if isinstance(a, (tuple, list)):
        return tuple(lerp(x, y, t) for x, y in zip(a, b))
    return a if t < .5 else b


def sample(keys, t):
    if t <= keys[0]["t"]:
        return keys[0]
    for k0, k1 in zip(keys, keys[1:]):
        if t <= k1["t"]:
            x = (t - k0["t"]) / max(1e-6, k1["t"] - k0["t"])
            x = ease(x) if k1.get("ease", "smooth") == "smooth" else (x if k1.get("ease") == "linear" else 1 - (1 - x) ** 3)
            a = {k: v for k, v in k0.items() if k not in ("t", "ease")}
            b = {k: v for k, v in k1.items() if k not in ("t", "ease")}
            return lerp(a, b, x)
    return keys[-1]
