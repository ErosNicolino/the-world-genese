"""Os clipes novos das famílias de arma (REVISAO_ARMAS: G1, G2, G3, G4), feitos pose a pose no Blender.

No Blender (com Tools/Blender/vortex_poses.py junto):
    exec(open(r"<este arquivo>").read()); build(["Spear"])        # uma família, com prévia
    build()                                                        # todas
    export()                                                       # Data/VortexWeapons/Adventurer_VortexClips.glb
Depois, com o editor fechado: Tools/ImportWeaponClips.py (importa e liga os clipes às definições).

Nomes: Vx_<Família>_Hold (pose de segurar: tronco e braços por cima de parado, andar e correr),
Vx_<Família>_Attack_01.. (golpes), Vx_<Família>_Special (secundário). "_Full" no nome = o clipe mexe o
corpo inteiro (giros, investidas, pancadas no chão). O instante do acerto de cada golpe vai em
Data/VortexWeapons/clips.json (lido pelo importador para a janela de dano).
"""
import json
import math
import os
import sys

import bpy

ROOT = os.environ.get("VORTEX_ROOT", r"C:\Users\Valorant Niko\Documents\ChatGPT\Mangas\Vortex-UE5-migracao\Vortex")
sys.path.insert(0, os.path.join(ROOT, "Tools", "Blender"))
import importlib  # noqa: E402
import vortex_poses as vp  # noqa: E402
importlib.reload(vp)

SHOTS = os.environ.get("VORTEX_SHOTS", r"C:\Users\VALORA~1\AppData\Local\Temp\claude\shots")
FAMILIES = {}
HITS = {}   # clipe -> instante do acerto (s)


def family(name, weapon, left=None):
    def wrap(fn):
        FAMILIES[name] = (weapon, fn, left)
        return fn
    return wrap


def k(t, **pose):
    pose["t"] = t
    return pose


# ---- lança --------------------------------------------------------------------------------------------
# De lado (ombro esquerdo para o inimigo), a mão direita atrás na altura do quadril e a esquerda à
# frente na haste; a ponta para a frente na altura do peito. Golpe: estocada (a lança corre para a
# frente pela mão esquerda, o tronco gira junto), sem corte de espada.
SPEAR_HOLD = dict(torso=(-30, 4, 0), head=(26, -4, 0), hips=(-10, 0, 0),
                  r={"at": (.20, .02, -.02), "tip": (.05, 1, .16), "side": (1, 0, 0), "pole": (1, -.3, -.6)},
                  l={"on": .34, "pole": (-.4, .3, -1)})


@family("Spear", "lanca_do_patrulheiro")
def spear():
    hold = [k(0, **SPEAR_HOLD), k(1.2, **dict(SPEAR_HOLD, torso=(-30, 6, 0))), k(2.4, **SPEAR_HOLD)]

    def thrust(t_back, t_hit, t_end, reach=.30, height=.06, twist=-6):
        back = dict(SPEAR_HOLD, torso=(-38, 2, 0), r=dict(SPEAR_HOLD["r"], at=(.24, -.04, -.03)), l={"on": .40, "pole": (-.4, .3, -1)})
        out = dict(SPEAR_HOLD, torso=(twist, 10, 0), head=(4, -6, 0), hips=(-4, 0, 0),
                   r=dict(SPEAR_HOLD["r"], at=(.10, reach, height), tip=(0, 1, .08)), l={"on": .16, "pole": (-.4, .3, -1)})
        return [k(0, **SPEAR_HOLD), k(t_back, **back), k(t_hit, ease="out", **out), k(t_hit + .08, **out), k(t_end, **SPEAR_HOLD)]

    flurry = [k(0, **SPEAR_HOLD)]
    t = 0.0
    for i in range(6):
        h = (.04, .10, -.02, .08, .0, .12)[i]
        out = dict(SPEAR_HOLD, torso=(-14, 8, 0), head=(14, -5, 0),
                   r=dict(SPEAR_HOLD["r"], at=(.11, .29, h), tip=(0, 1, .06 + h * .4)), l={"on": .17, "pole": (-.4, .3, -1)})
        flurry += [k(t + .10, ease="out", **out), k(t + .2, **dict(SPEAR_HOLD, torso=(-24, 5, 0)))]
        t += .2
    HITS.update({"Vx_Spear_Attack_01": .2, "Vx_Spear_Attack_02": .2, "Vx_Spear_Attack_03_Full": .34})
    strong = thrust(.2, .34, .8, reach=.33, height=.1, twist=0)
    # o terceiro golpe é uma investida: o corpo avança um passo
    for key in strong[2:4]:
        key["root"] = (0, .25, 0)
        key["legs"] = {"step": -22, "bend": 12}
    strong[-1]["root"] = (0, 0, 0)
    return {
        "Vx_Spear_Hold": (hold, True),
        "Vx_Spear_Attack_01": (thrust(.12, .2, .5), False),
        "Vx_Spear_Attack_02": (thrust(.12, .2, .5, height=.12), False),
        "Vx_Spear_Attack_03_Full": (strong, False),
        "Vx_Spear_Special": (flurry + [k(t + .2, **SPEAR_HOLD)], False),
    }


# as outras famílias (instrumento, orbe, grimório, varinha, cajado, garrafa, maça, adagas, alabarda,
# machados, espadão, espada e escudo, corrente, chicote, foice, chakrams, pistola, besta, arco)
exec(open(os.path.join(ROOT, "Tools", "Blender", "weapon_families.py"), encoding="utf-8").read())


# ---- construir e conferir ---------------------------------------------------------------------------
def preview(rig, act, name, count=5, yaws=(35, -70)):
    sc = bpy.context.scene
    cam = sc.camera
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = 1.9
    sc.render.resolution_x = sc.render.resolution_y = 300
    end = int(act.frame_range[1])
    files = []
    for yaw in yaws:
        for i in range(count):
            f = round(i * end / max(1, count - 1))
            rig.pose_at(act, f)
            a = math.radians(-90 + yaw)
            cam.location = (4 * math.cos(a), 4 * math.sin(a), .72)
            cam.rotation_euler = (math.radians(90), 0, a + math.radians(90))
            path = os.path.join(SHOTS, "clip_%s_%d_%d.png" % (name, yaw, i))
            sc.render.filepath = path
            bpy.ops.render.render(write_still=True)
            files.append(path)
    return files


def show(names):
    """Prévia das ações já gravadas (sem refazer)."""
    rig = vp.Rig()
    for name, (weapon, fn, left) in FAMILIES.items():
        if name not in names:
            continue
        rig.equip(weapon, left)
        for a in bpy.data.actions:
            if a.name.startswith("Vx_%s_" % name):
                preview(rig, a, a.name)


def build(names=None, show=True):
    rig = vp.Rig()
    made = {}
    for name, (weapon, fn, left) in FAMILIES.items():
        if names and name not in names:
            continue
        rig.equip(weapon, left)
        clips = fn()
        for clip, (keys, loop) in clips.items():
            act = rig.bake(clip, keys, loop)
            made[clip] = act
            if show:
                preview(rig, act, clip)
    # junta com os instantes de acerto das famílias que não foram refeitas agora
    path = os.path.join(ROOT, "Data", "VortexWeapons", "clips.json")
    hits = json.load(open(path, encoding="utf-8")).get("hits", {}) if os.path.exists(path) else {}
    hits.update(HITS)
    with open(path, "w", encoding="utf-8") as f:
        json.dump({"hits": hits, "families": {n: w for n, (w, _, _l) in FAMILIES.items()}}, f, indent=1)
    return made


def export():
    """Todas as ações Vx_* num glb só (com o corpo, para a Unreal reconhecer o esqueleto)."""
    rig = vp.Rig()
    rig.clear_weapons()
    for a in list(bpy.data.actions):
        a.use_fake_user = a.name.startswith("Vx_")
    keep = [a for a in bpy.data.actions if a.name.startswith("Vx_")]
    for a in list(bpy.data.actions):
        if a not in keep:
            bpy.data.actions.remove(a)
    for o in bpy.data.objects:
        o.select_set(o.type in ('ARMATURE', 'MESH'))
    bpy.context.view_layer.objects.active = rig.arm
    out = os.path.join(ROOT, "Data", "VortexWeapons", "Adventurer_VortexClips.glb")
    bpy.ops.export_scene.gltf(filepath=out, use_selection=True, export_animation_mode='ACTIONS', export_anim_single_armature=True,
                              export_force_sampling=True, export_def_bones=False, export_yup=True, export_apply=False)
    return out, len(keep)
