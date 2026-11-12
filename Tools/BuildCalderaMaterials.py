"""Os materiais da cratera do Azhûr (a Caldeira) — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/BuildCalderaMaterials.py -unattended -nullrhi

- texturas de basalto da Poly Haven (CC0, Data/Textures/Caldeira: dark_rock no chão, dark_rock_02 nos paredões);
- M_CalderaGround: o chão (malha única da cratera, AVortexLavaArena::BuildCrater) com o basalto projetado de cima pela
  posição no mundo (sem esticar) e as rachaduras de lava pela cor do vértice (R = calor: brilha e pulsa);
- M_CalderaWall: os paredões, os pilares e as agulhas com o basalto projetado dos três lados;
- os materiais do Azhûr e o das runas passam a valer em malhas instanciadas (sem isso a Unreal punha o xadrez padrão).
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
DEST = "/Game/Vortex/World/Caldeira"
SRC = os.path.join(root, "Data", "Textures", "Caldeira")
lines = []


def texture(file, normal=False, linear=False):
    name = "T_" + os.path.splitext(file)[0]
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SRC, file)
    task.destination_path = DEST
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    tex = library.load_asset(DEST + "/" + name)
    if normal:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("flip_green_channel", False)
    elif linear:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
    library.save_loaded_asset(tex)
    return tex


def fresh(name):
    path = DEST + "/" + name
    m = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    return m


def node(m, kind, x, y, **props):
    e = mel.create_material_expression(m, kind, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def link(a, b, pin_b="", pin_a=""):
    mel.connect_material_expressions(a, pin_a, b, pin_b)


def done(m):
    mel.recompile_material(m)
    library.save_loaded_asset(m)
    lines.append("material " + m.get_name())


ground_diff = texture("dark_rock_diff_2k.jpg")
ground_nor = texture("dark_rock_nor_gl_2k.jpg", normal=True)
ground_rough = texture("dark_rock_rough_2k.jpg", linear=True)
wall_diff = texture("dark_rock_02_diff_2k.jpg")
wall_rough = texture("dark_rock_02_rough_2k.jpg", linear=True)

# ---- chão: UV = posição no mundo (XY) / 5 m; a lava pela cor do vértice
g = fresh("M_CalderaGround")
wpos = node(g, unreal.MaterialExpressionWorldPosition, -1400, 0)
xy = node(g, unreal.MaterialExpressionComponentMask, -1250, 0, r=True, g=True, b=False, a=False)
link(wpos, xy)
uv = node(g, unreal.MaterialExpressionDivide, -1100, 0, const_b=500.0)
link(xy, uv, "A")
# um segundo tamanho, mais largo, quebra a repetição
uv2 = node(g, unreal.MaterialExpressionDivide, -1100, 150, const_b=1700.0)
link(xy, uv2, "A")
diff = node(g, unreal.MaterialExpressionTextureSample, -900, -100, texture=ground_diff)
link(uv, diff, "UVs")
diff2 = node(g, unreal.MaterialExpressionTextureSample, -900, 120, texture=ground_diff)
link(uv2, diff2, "UVs")
mix = node(g, unreal.MaterialExpressionMultiply, -700, 0)
link(diff, mix, "A", "RGB")
link(diff2, mix, "B", "RGB")
# basalto queimado: escuro e um pouco quente
warm = node(g, unreal.MaterialExpressionMultiply, -550, 0, const_b=1.6)
link(mix, warm, "A")
tint = node(g, unreal.MaterialExpressionConstant3Vector, -700, 260, constant=unreal.LinearColor(.62, .5, .46, 1))
stone = node(g, unreal.MaterialExpressionMultiply, -400, 40)
link(warm, stone, "A")
link(tint, stone, "B")
vc = node(g, unreal.MaterialExpressionVertexColor, -900, 420)
heat = node(g, unreal.MaterialExpressionComponentMask, -750, 420, r=True, g=False, b=False, a=False)
link(vc, heat)
# a crosta perto da lava fica preta e a lava em si laranja
crust = node(g, unreal.MaterialExpressionConstant3Vector, -550, 360, constant=unreal.LinearColor(.03, .02, .015, 1))
dark = node(g, unreal.MaterialExpressionLinearInterpolate, -250, 120)
link(stone, dark, "A")
link(crust, dark, "B")
link(heat, dark, "Alpha")
mel.connect_material_property(dark, "", unreal.MaterialProperty.MP_BASE_COLOR)
nor = node(g, unreal.MaterialExpressionTextureSample, -900, 560, texture=ground_nor, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
link(uv, nor, "UVs")
mel.connect_material_property(nor, "RGB", unreal.MaterialProperty.MP_NORMAL)
rough = node(g, unreal.MaterialExpressionTextureSample, -900, 760, texture=ground_rough, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
link(uv, rough, "UVs")
mel.connect_material_property(rough, "R", unreal.MaterialProperty.MP_ROUGHNESS)
# brilho da lava: calor^2 x pulso que corre pelo chão
time = node(g, unreal.MaterialExpressionTime, -1100, 900)
speed = node(g, unreal.MaterialExpressionMultiply, -950, 900, const_b=1.7)
link(time, speed, "A")
wx = node(g, unreal.MaterialExpressionComponentMask, -1100, 1000, r=True, g=False, b=False, a=False)
link(wpos, wx)
wave_x = node(g, unreal.MaterialExpressionMultiply, -950, 1000, const_b=.004)
link(wx, wave_x, "A")
wave = node(g, unreal.MaterialExpressionAdd, -800, 950)
link(speed, wave, "A")
link(wave_x, wave, "B")
sine = node(g, unreal.MaterialExpressionSine, -650, 950)
link(wave, sine)
pulse = node(g, unreal.MaterialExpressionMultiply, -500, 950, const_b=.3)
link(sine, pulse, "A")
pulse1 = node(g, unreal.MaterialExpressionAdd, -350, 950, const_b=1.0)
link(pulse, pulse1, "A")
heat2 = node(g, unreal.MaterialExpressionMultiply, -550, 480)
link(heat, heat2, "A")
link(heat, heat2, "B")
lava = node(g, unreal.MaterialExpressionConstant3Vector, -550, 620, constant=unreal.LinearColor(1.0, .28, .03, 1))
glow = node(g, unreal.MaterialExpressionMultiply, -350, 520)
link(heat2, glow, "A")
link(lava, glow, "B")
glow2 = node(g, unreal.MaterialExpressionMultiply, -200, 600, const_b=3.2)   # 02/11: era 9 (o chão estourava)
link(glow, glow2, "A")
glow3 = node(g, unreal.MaterialExpressionMultiply, -50, 700)
link(glow2, glow3, "A")
link(pulse1, glow3, "B")
mel.connect_material_property(glow3, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
done(g)

# ---- paredões: o basalto dos três lados (projeção feita à mão: X, Y e Z pela posição no mundo, misturadas pela
# direção da face). A função WorldAlignedTexture da Unreal não recebia a textura pelo Python e caía no xadrez padrão
w = fresh("M_CalderaWall")
wp = node(w, unreal.MaterialExpressionWorldPosition, -1500, 0)
scale = node(w, unreal.MaterialExpressionDivide, -1350, 0, const_b=700.0)
link(wp, scale, "A")
samples = []
for i, (r_, g_, b_) in enumerate(((False, True, True), (True, False, True), (True, True, False))):
    m2 = node(w, unreal.MaterialExpressionComponentMask, -1200, i * 160, r=r_, g=g_, b=b_, a=False)
    link(scale, m2)
    t = node(w, unreal.MaterialExpressionTextureSample, -1000, i * 160, texture=wall_diff)
    link(m2, t, "UVs")
    samples.append(t)
nrm = node(w, unreal.MaterialExpressionVertexNormalWS, -1200, 520)
absn = node(w, unreal.MaterialExpressionAbs, -1050, 520)
link(nrm, absn)
weights = []
for i, (r_, g_, b_) in enumerate(((True, False, False), (False, True, False), (False, False, True))):
    m3 = node(w, unreal.MaterialExpressionComponentMask, -900, 480 + i * 80, r=r_, g=g_, b=b_, a=False)
    link(absn, m3)
    p4 = node(w, unreal.MaterialExpressionPower, -760, 480 + i * 80, const_exponent=4.0)
    link(m3, p4, "Base")
    weights.append(p4)
total = node(w, unreal.MaterialExpressionAdd, -620, 560)
link(weights[0], total, "A")
link(weights[1], total, "B")
total2 = node(w, unreal.MaterialExpressionAdd, -500, 600)
link(total, total2, "A")
link(weights[2], total2, "B")
acc = None
for i in range(3):
    part = node(w, unreal.MaterialExpressionMultiply, -760, i * 160)
    link(samples[i], part, "A", "RGB")
    link(weights[i], part, "B")
    if acc is None:
        acc = part
    else:
        add = node(w, unreal.MaterialExpressionAdd, -620, i * 160)
        link(acc, add, "A")
        link(part, add, "B")
        acc = add
tri = node(w, unreal.MaterialExpressionDivide, -480, 200)
link(acc, tri, "A")
link(total2, tri, "B")
tone = node(w, unreal.MaterialExpressionConstant3Vector, -480, 340, constant=unreal.LinearColor(.75, .58, .52, 1))
col = node(w, unreal.MaterialExpressionMultiply, -350, 260)
link(tri, col, "A")
link(tone, col, "B")
mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(node(w, unreal.MaterialExpressionConstant, -350, 420, r=.9), "", unreal.MaterialProperty.MP_ROUGHNESS)
# a borda de cima do paredão esquenta (o brilho da lava de baixo)
fres = node(w, unreal.MaterialExpressionFresnel, -600, 360, exponent=4.0, base_reflect_fraction=0.0)
heat_rim = node(w, unreal.MaterialExpressionMultiply, -350, 380)
link(fres, heat_rim, "A")
link(node(w, unreal.MaterialExpressionConstant3Vector, -600, 480, constant=unreal.LinearColor(.35, .08, .01, 1)), heat_rim, "B")
mel.connect_material_property(heat_rim, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
done(w)

# ---- os do Azhûr e o das runas valendo em malhas instanciadas
for path in ("/Game/Vortex/Creatures/RuneColossus/Materials/M_AzhurRock", "/Game/Vortex/Creatures/RuneColossus/Materials/M_AzhurObsidian",
             "/Game/Vortex/Creatures/RuneColossus/Materials/M_AzhurLava", "/Game/Vortex/Creatures/RuneColossus/Materials/M_AzhurCore",
             "/Game/Vortex/Combat/M_SpectralGlow"):
    m = library.load_asset(path)
    if not m:
        lines.append("FALTA " + path)
        continue
    m.set_editor_property("used_with_instanced_static_meshes", True)
    mel.recompile_material(m)
    library.save_loaded_asset(m)
    lines.append("instanciado " + path)

with open(os.path.join(root, "Saved", "Logs", "BuildCalderaMaterials.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
