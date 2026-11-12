"""A flecha do Arco de Bétula (08/11), que combina com ele: haste branca com faixas pretas e penas laranja como as pontas.

blender -b --factory-startup -P Tools/Blender/MakeBetulaArrow.py -- <raiz>
Sai Data/VortexWeapons/Models/W_flecha_betula_blocos.glb (Tools/ImportBlockSword.py, VORTEX_ONLY=flecha_betula, leva para a Unreal).
"""
import importlib.util
import os
import sys

import bpy
import numpy as np

ARGS = sys.argv[sys.argv.index("--") + 1:]
spec = importlib.util.spec_from_file_location("pixel_bows", os.path.join(ARGS[0], "Tools", "Blender", "MakePixelBows.py"))
pixel_bows = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pixel_bows)

bpy.ops.wm.read_factory_settings(use_empty=True)
# a "prancha" só dá as duas cores: a mais comum vira a haste, a mais saturada as penas
colors = np.zeros((110, 1, 4), np.float32)
colors[:, 0] = (226 / 255, 220 / 255, 204 / 255, 1)
colors[100:, 0] = (232 / 255, 118 / 255, 40 / 255, 1)
obj, img, img_r, cells = pixel_bows.build_arrow("betula", {"element": "", "rarity": "common"}, colors, pixel_bows.arrow_frame(), band=.16)
pixel_bows.save(obj, img, img_r)
print("BETULA_ARROW ok")
