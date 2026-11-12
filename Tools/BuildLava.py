"""Lava em desenho (M_LavaToon) e a textura de ruído dela (T_LavaNoise, de Tools/MakeLavaNoise.py).

- Correntezas que correm numa direção (FlowX/FlowY por poça, Speed): as coordenadas giram para a
  corrente e esticam ao longo dela; uma corrente larga e lenta torce outra fina e rápida (os veios).
- Placas de crosta (células) derivam devagar; as frestas entre elas brilham.
- O calor vira faixas nítidas como num desenho: crosta escura, vermelho, laranja, amarelo; o miolo da
  poça (cor do vértice R = 1) é mais quente, a borda (R = 0) é crosta.
Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi
"""
import os
import unreal

library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
FOLDER = "/Game/Vortex/Combat"
ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())

# textura (linear, repete)
task = unreal.AssetImportTask()
task.filename = os.path.join(ROOT, "Data", "UI", "Lava", "T_LavaNoise.png")
task.destination_path = FOLDER
task.automated = True
task.save = True
task.replace_existing = True
tools.import_asset_tasks([task])
tex = library.load_asset(f"{FOLDER}/T_LavaNoise")
tex.set_editor_property("srgb", False)
tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
library.save_loaded_asset(tex)

path = f"{FOLDER}/M_LavaToon"
m = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset("M_LavaToon", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)


def node(kind, x, y, **props):
    e = mel.create_material_expression(m, kind, x - 3000, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def link(a, b, pin_b="", pin_a=""):
    mel.connect_material_expressions(a, pin_a, b, pin_b)


def op(kind, a, b=None, const=None, x=0, y=0):
    e = node(kind, x, y)
    if const is not None:
        e.set_editor_property("const_b", const)
    link(a, e, "A")
    if b is not None:
        link(b, e, "B")
    return e


def mul(a, b=None, const=None, x=0, y=0): return op(unreal.MaterialExpressionMultiply, a, b, const, x, y)
def add(a, b=None, const=None, x=0, y=0): return op(unreal.MaterialExpressionAdd, a, b, const, x, y)
def sub(a, b=None, const=None, x=0, y=0): return op(unreal.MaterialExpressionSubtract, a, b, const, x, y)
def scalar(name, value, x, y): return node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value)
def const(value, x, y): return node(unreal.MaterialExpressionConstant, x, y, r=value)
def color(r, g, b, x, y): return node(unreal.MaterialExpressionConstant3Vector, x, y, constant=unreal.LinearColor(r, g, b, 1))


def append(a, b, x, y):
    e = node(unreal.MaterialExpressionAppendVector, x, y)
    link(a, e, "A")
    link(b, e, "B")
    return e


def channel(e, c, x, y):
    k = node(unreal.MaterialExpressionComponentMask, x, y, r=c == "r", g=c == "g", b=c == "b", a=False)
    link(e, k)
    return k


def sample(uv, x, y):
    s = node(unreal.MaterialExpressionTextureSample, x, y, texture=tex, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    link(uv, s, "UVs")
    return s


def lerp(a, b, alpha, x, y):
    e = node(unreal.MaterialExpressionLinearInterpolate, x, y)
    link(a, e, "A")
    link(b, e, "B")
    link(alpha, e, "Alpha")
    return e


def band(heat, threshold, x, y):
    k = mul(sub(heat, const=threshold, x=x, y=y), const=24.0, x=x + 150, y=y)
    s = node(unreal.MaterialExpressionSaturate, x + 300, y)
    link(k, s)
    return s


wp = node(unreal.MaterialExpressionWorldPosition, 0, 0)
wx, wy = channel(wp, "r", 150, -40), channel(wp, "g", 150, 60)
fx, fy = scalar("FlowX", 1.0, 150, 200), scalar("FlowY", 0.0, 150, 300)
u = add(mul(wx, fx, x=350, y=-60), mul(wy, fy, x=350, y=40), x=520, y=-20)
v = sub(mul(wy, fx, x=350, y=140), mul(wx, fy, x=350, y=240), x=520, y=180)
t = mul(node(unreal.MaterialExpressionTime, 350, 400), scalar("Speed", 1.0, 350, 500), x=520, y=420)

# corrente larga e lenta
s1 = sample(append(sub(mul(u, const=.0018, x=700, y=-60), mul(t, const=.035, x=700, y=40), x=870, y=-40), mul(v, const=.0042, x=700, y=140), 1040, 0), 1200, 0)
big = channel(s1, "r", 1400, 0)
# veios finos e rápidos, torcidos pela larga
u2 = add(sub(mul(u, const=.0052, x=700, y=360), mul(t, const=.13, x=700, y=460), x=870, y=400), mul(big, const=.45, x=1400, y=300), x=1560, y=380)
v2 = add(mul(v, const=.0125, x=700, y=560), mul(big, const=.25, x=1400, y=560), x=1560, y=540)
veins = channel(sample(append(u2, v2, 1720, 460), 1880, 460), "g", 2080, 460)
# placas de crosta que derivam devagar
s3 = sample(append(sub(mul(u, const=.0034, x=700, y=760), mul(t, const=.05, x=700, y=860), x=870, y=800), mul(v, const=.0046, x=700, y=960), 1040, 860), 1200, 860)
plates = channel(s3, "b", 1400, 860)
cracks = sub(const(1.0, 1400, 960), plates, x=1560, y=900)

vr = channel(node(unreal.MaterialExpressionVertexColor, 1880, 1100), "r", 2080, 1100)
heat = add(add(add(mul(big, const=.45, x=2250, y=0), mul(veins, const=.38, x=2250, y=460), x=2420, y=200),
               mul(cracks, const=.42, x=2250, y=900), x=2580, y=500),
           mul(sub(vr, const=.55, x=2250, y=1100), const=.6, x=2420, y=1100), x=2760, y=700)

crust_mask = band(heat, .56, 2950, 200)
red_mask = band(heat, .68, 2950, 420)
hot_mask = band(heat, .82, 2950, 640)
c1 = lerp(color(.05, .022, .018, 3450, -200), color(.78, .1, .02, 3450, -60), crust_mask, 3650, 0)
c2 = lerp(c1, color(1.0, .36, .03, 3450, 80), red_mask, 3850, 100)
c3 = lerp(c2, color(1.0, .66, .12, 3450, 220), hot_mask, 4050, 200)
strength = add(add(mul(crust_mask, const=.7, x=3650, y=500), mul(red_mask, const=.6, x=3650, y=600), x=3850, y=540), mul(hot_mask, const=.7, x=3650, y=700), x=4050, y=600)
sine = node(unreal.MaterialExpressionSine, 3850, 900)
link(mul(t, const=1.6, x=3650, y=900), sine)
pulse = add(mul(sine, const=.08, x=4050, y=900), const=1.0, x=4250, y=900)
mel.connect_material_property(mul(mul(c3, strength, x=4250, y=300), pulse, x=4450, y=400), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(mul(c3, const=.4, x=4250, y=100), "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(lerp(const(.9, 4050, 1100), const(.4, 4050, 1200), crust_mask, 4250, 1150), "", unreal.MaterialProperty.MP_ROUGHNESS)

mel.recompile_material(m)
library.save_loaded_asset(m)
unreal.log_warning("VORTEX_LAVA material ok")
