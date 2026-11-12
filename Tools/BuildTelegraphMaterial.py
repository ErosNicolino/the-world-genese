"""O aviso dos golpes de área (AVortexTelegraph) mais legível e bonito — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/BuildTelegraphMaterial.py -unattended -nullrhi

M_TelegraphRunes (/Game/Vortex/Combat), desenhado só pela coordenada do plano (sem textura), com os mesmos parâmetros
do M_Telegraph (Color, Ring, Progress):
- a borda forte e um anel de runas (traços) girando por dentro dela;
- o enchimento vai do meio até a borda no tempo do golpe, com listras correndo para dentro, e uma frente mais clara
  marca até onde chegou;
- no anel (Ring = 1) o miolo seguro fica escuro, com um tracejado na divisa;
- no último instante pisca rápido.
"""
import math
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
PATH = "/Game/Vortex/Combat/M_TelegraphRunes"

m = library.load_asset(PATH) if library.does_asset_exist(PATH) else \
    tools.create_asset("M_TelegraphRunes", "/Game/Vortex/Combat", unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("two_sided", True)
X = [-1800]


def node(kind, y, **props):
    e = mel.create_material_expression(m, kind, X[0], y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def link(a, b, pin_b="", pin_a=""):
    mel.connect_material_expressions(a, pin_a, b, pin_b)


def op(kind, a, b=None, y=0, **props):
    e = node(kind, y, **props)
    link(a, e, "A")
    if b is not None:
        link(b, e, "B")
    return e


def smooth(lo, hi, value, y):
    e = node(unreal.MaterialExpressionSmoothStep, y, const_min=lo, const_max=hi)
    link(value, e, "Value")
    return e


def band(lo, hi, soft, value, y):
    """1 entre lo e hi (bordas suaves de largura soft)."""
    a = smooth(lo - soft, lo, value, y)
    b = smooth(hi, hi + soft, value, y + 40)
    inv = op(unreal.MaterialExpressionOneMinus, b, y=y + 80)
    return op(unreal.MaterialExpressionMultiply, a, inv, y=y + 120)


color = node(unreal.MaterialExpressionVectorParameter, -600, parameter_name="Color", default_value=unreal.LinearColor(1, .4, .2, 1))
ring = node(unreal.MaterialExpressionScalarParameter, -450, parameter_name="Ring", default_value=0.0)
progress = node(unreal.MaterialExpressionScalarParameter, -300, parameter_name="Progress", default_value=0.0)
time = node(unreal.MaterialExpressionTime, -150)
uv = node(unreal.MaterialExpressionTextureCoordinate, 0)
X[0] += 200
centered = op(unreal.MaterialExpressionSubtract, uv, node(unreal.MaterialExpressionConstant2Vector, 120, r=.5, g=.5), y=0)
X[0] += 200
length = node(unreal.MaterialExpressionLength, 200)
link(centered, length)
dist = op(unreal.MaterialExpressionMultiply, length, y=260, const_b=2.0)
cx = node(unreal.MaterialExpressionComponentMask, 400, r=True, g=False, b=False, a=False)
cy = node(unreal.MaterialExpressionComponentMask, 480, r=False, g=True, b=False, a=False)
link(centered, cx)
link(centered, cy)
atan = node(unreal.MaterialExpressionArctangent2, 440)
link(cy, atan, "Y")
link(cx, atan, "X")
X[0] += 200
turn = op(unreal.MaterialExpressionMultiply, atan, y=440, const_b=1.0 / (2.0 * math.pi))
spin = op(unreal.MaterialExpressionMultiply, time, y=560, const_b=.12)
angle = op(unreal.MaterialExpressionAdd, turn, spin, y=500)
X[0] += 200
# --- a borda forte e o anel de runas (24 traços girando)
rim = band(.93, .995, .02, dist, -200)
X[0] += 200
runes = op(unreal.MaterialExpressionFrac, op(unreal.MaterialExpressionMultiply, angle, y=500, const_b=24.0), y=560)
X[0] += 200
dash = op(unreal.MaterialExpressionOneMinus, smooth(.5, .58, runes, 560), y=640)
rune_band = op(unreal.MaterialExpressionMultiply, band(.8, .87, .012, dist, 300), dash, y=420)
X[0] += 200
# --- o enchimento até o progresso, com listras correndo para dentro, e a frente
inside = op(unreal.MaterialExpressionOneMinus, smooth(0.0, .015, op(unreal.MaterialExpressionSubtract, dist, progress, y=760), 820), y=860)
stripes_in = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, dist, y=900, const_b=6.0), op(unreal.MaterialExpressionMultiply, time, y=960, const_b=1.4), y=930)
X[0] += 200
stripes = smooth(.35, .65, op(unreal.MaterialExpressionFrac, stripes_in, y=930), 930)
fill = op(unreal.MaterialExpressionMultiply, inside, op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, stripes, y=990, const_b=.25), y=1020, const_b=.2), y=880)
front = band(0.0, .025, .03, op(unreal.MaterialExpressionAbs, op(unreal.MaterialExpressionSubtract, dist, progress, y=1100), y=1140), 1100)
X[0] += 200
# --- anel: o miolo (até 45%) é seguro: escuro, com tracejado na divisa
safe_out = smooth(.43, .46, dist, 1300)
keep = op(unreal.MaterialExpressionLinearInterpolate, node(unreal.MaterialExpressionConstant, 1300, r=1.0), safe_out, y=1340)
link(ring, keep, "Alpha")
inner_dash = op(unreal.MaterialExpressionMultiply, band(.44, .46, .006, dist, 1450), dash, y=1450)
inner = op(unreal.MaterialExpressionMultiply, inner_dash, ring, y=1500)
X[0] += 200
# --- o pisca dos últimos 15%
late = smooth(.85, 1.0, progress, 1650)
blink = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionSine, op(unreal.MaterialExpressionMultiply, time, y=1700, const_b=28.0), y=1700), y=1700, const_b=.5), y=1700, const_b=.5)
flash = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, late, blink, y=1750), y=1750, const_b=1.0)
X[0] += 200
# --- soma: brilho e opacidade
body = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionAdd, fill, op(unreal.MaterialExpressionMultiply, front, y=1150, const_b=2.5), y=1000), keep, y=1000)
lines_ = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, rim, y=-150, const_b=3.0), op(unreal.MaterialExpressionMultiply, rune_band, y=420, const_b=2.0), y=-100)
X[0] += 200
total = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionAdd, lines_, body, y=300), inner, y=400)
X[0] += 200
lit = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, total, flash, y=500), color, y=520)
glow = op(unreal.MaterialExpressionMultiply, lit, y=560, const_b=3.0)
mel.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
sat_in = op(unreal.MaterialExpressionMultiply, total, y=650, const_b=.9)
saturate = node(unreal.MaterialExpressionSaturate, 900)
link(sat_in, saturate)
mask = op(unreal.MaterialExpressionMultiply, saturate, op(unreal.MaterialExpressionOneMinus, smooth(.995, 1.0, dist, 960), y=1000), y=940)
mel.connect_material_property(mask, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(m)
library.save_loaded_asset(m)
with open(os.path.join(root, "Saved", "Logs", "BuildTelegraphMaterial.txt"), "w", encoding="utf-8") as out:
    out.write("M_TelegraphRunes ok\n")
