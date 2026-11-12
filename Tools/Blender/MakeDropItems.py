"""Os itens que caem no chão em blocos (08/11), a partir dos ícones sem fundo (Tools/Weapons/CutDropItems.py).

blender -b --factory-startup -P Tools/Blender/MakeDropItems.py -- <raiz>
Cada pixel vira uma coluna de cubos, mais grossa no meio e fina na borda: o desenho do ícone com volume (o usuário preferiu assim
a um objeto modelado do zero): bem cheio nos itens redondos e quase plano nos mapas e peles.
Sai Data/VortexWeapons/Models/W_item_<id>_blocos.glb (Tools/ImportBlockSword.py leva para a Unreal).
"""
import colorsys
import glob
import importlib.util
import os
import sys

import bpy
import numpy as np
from mathutils import Vector

ARGS = sys.argv[sys.argv.index("--") + 1:]
spec = importlib.util.spec_from_file_location("pixel_bows", os.path.join(ARGS[0], "Tools", "Blender", "MakePixelBows.py"))
pixel_bows = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pixel_bows)

VOXEL = .0065      # 48 blocos = 31 cm
ROUND = {"potion_red": 11, "potion_blue": 11, "potion_green": 11, "antidoto": 8, "bread": 9, "gel_jade": 9, "mushroom_cap": 8, "moss_core": 9,
         "azhur_core": 9, "frutas_silvestres": 8, "meat": 8, "racao_de_viagem": 8, "colossus_shard": 7, "obsidian_fragment": 6}
FLAT = {"mapa_de_viajante": 1, "carta_de_terras_distantes": 4, "mapa_de_nicolly": 2, "wolf_pelt": 1, "rabbit_pelt": 1, "bandit_bandana": 1,
        "healing_herb": 1, "bat_wing": 1, "shield_wood": 2, "ring_jade": 2, "amulet_moon": 1, "caldera_amulet": 2}
GLOW = {"azhur_core", "moss_core", "colossus_shard", "obsidian_fragment", "caldera_amulet", "thorun_heartwood_charm"}


def distance(mask):
    """Distância de cada pixel até a borda da silhueta, em pixels."""
    out = np.zeros(mask.shape, np.int32)
    now = mask.copy()
    d = 0
    while now.any():
        d += 1
        out[now] = d
        p = np.pad(now, 1)
        now = p[1:-1, 1:-1] & p[2:, 1:-1] & p[:-2, 1:-1] & p[1:-1, 2:] & p[1:-1, :-2]
    return out


def build(key, a):
    mask = a[..., 3] > .5
    dist = distance(mask)
    height = ROUND.get(key, FLAT.get(key, 3))
    reach = max(1.0, min(float(dist.max()), height * 1.6))
    half = np.round(height * np.sqrt(np.clip(1 - (1 - np.minimum(dist, reach) / reach) ** 2, 0, 1))).astype(int)
    R, C = mask.shape
    A, Wd, T = Vector((0, 0, -1)), Vector((1, 0, 0)), Vector((0, 1, 0))
    filled, colors = {}, {}
    for r, c in zip(*np.nonzero(mask)):
        rgb = np.floor(a[r, c, :3] * 16) / 16 + 1 / 32     # poucas cores: a textura fica pequena
        _, ss, vv = colorsys.rgb_to_hsv(*a[r, c, :3])
        lit = key in GLOW and vv > .8 and ss > .45
        for d in range(-int(half[r, c]), int(half[r, c]) + 1):
            cell = (int(r), int(c), d)
            filled[cell] = (A * (r - (R - 1) / 2) * VOXEL + Wd * (c - (C - 1) / 2) * VOXEL + T * d * VOXEL, (A, Wd, T))
            colors[cell] = (rgb, lit)
    return pixel_bows.voxels_to_mesh("W_item_%s_blocos" % key, filled, colors, VOXEL)


bpy.ops.wm.read_factory_settings(use_empty=True)
os.makedirs(pixel_bows.TEXTURES, exist_ok=True)
for path in sorted(glob.glob(os.path.join(ARGS[0], "Data", "VortexWeapons", "DropItems", "*.png"))):
    key = os.path.basename(path)[:-4]
    if key.startswith("_"):
        continue
    obj, img, img_r = build(key, pixel_bows.load_grid(path))
    print("DROP_ITEM %-26s %6d faces" % (key, len(obj.data.polygons)))
    pixel_bows.save(obj, img, img_r)
print("DROP_ITEMS ok")
