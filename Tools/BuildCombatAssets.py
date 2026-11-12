"""Materiais e dados do combate novo (editor fechado):

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/BuildCombatAssets.py -unattended

- M_SpectralGlow: brilho translúcido aditivo (borda mais forte), parâmetros Color e Opacity. O punho
  do Soco Sério e o cone de vento usam.
- M_WaterRipple: anel claro num plano (onda na água), parâmetro Fade (1 -> 0).
- M_LavaFlow: lava do chão da arena do chefe (rachaduras que brilham e se mexem).
- DA_manoplas_de_ferro: o especial vira o Soco Sério (nome, texto, pose de carga e o golpe).
- re-salva os efeitos dos pacotes novos (Free_Spells, Vefects) com a compilação desta versão.
Relatório em Saved/Logs/BuildCombatAssets.txt.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
lines = []
FOLDER = "/Game/Vortex/Combat"


def fresh_material(name):
    path = FOLDER + "/" + name
    material = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(material)
    return material


def node(material, kind, x, y, **props):
    expression = mel.create_material_expression(material, kind, x, y)
    for key, value in props.items():
        expression.set_editor_property(key, value)
    return expression


def link(a, b, pin_b="", pin_a=""):
    mel.connect_material_expressions(a, pin_a, b, pin_b)


def finish(material):
    mel.recompile_material(material)
    library.save_loaded_asset(material)
    lines.append("material " + material.get_name())


# ---- brilho fantasma (punho, vento)
m = fresh_material("M_SpectralGlow")
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("two_sided", True)
color = node(m, unreal.MaterialExpressionVectorParameter, -900, 0, parameter_name="Color", default_value=unreal.LinearColor(1, .7, .25, 1))
opacity = node(m, unreal.MaterialExpressionScalarParameter, -900, 250, parameter_name="Opacity", default_value=1.0)
fresnel = node(m, unreal.MaterialExpressionFresnel, -900, 120, exponent=2.2, base_reflect_fraction=0.0)
edge = node(m, unreal.MaterialExpressionMultiply, -650, 120, const_b=3.0)
link(fresnel, edge, "A")
core = node(m, unreal.MaterialExpressionAdd, -450, 120, const_b=0.35)
link(edge, core, "A")
tint = node(m, unreal.MaterialExpressionMultiply, -250, 60)
link(color, tint, "A")
link(core, tint, "B")
fade = node(m, unreal.MaterialExpressionMultiply, -80, 120)
link(tint, fade, "A")
link(opacity, fade, "B")
boost = node(m, unreal.MaterialExpressionMultiply, 80, 120, const_b=3.0)
link(fade, boost, "A")
mel.connect_material_property(boost, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
finish(m)

# ---- anel na água
m = fresh_material("M_WaterRipple")
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("two_sided", True)
uv = node(m, unreal.MaterialExpressionTextureCoordinate, -1100, 0)
center = node(m, unreal.MaterialExpressionConstant2Vector, -1100, 120, r=.5, g=.5)
dist = node(m, unreal.MaterialExpressionDistance, -900, 40)
link(uv, dist, "A")
link(center, dist, "B")
# distância 0..0.5 -> anel fino perto da borda (0.44), largura 0.028
off = node(m, unreal.MaterialExpressionSubtract, -720, 40, const_b=.44)
link(dist, off, "A")
absolute = node(m, unreal.MaterialExpressionAbs, -560, 40)
link(off, absolute)
scaled = node(m, unreal.MaterialExpressionDivide, -420, 40, const_b=.028)
link(absolute, scaled, "A")
ring = node(m, unreal.MaterialExpressionOneMinus, -280, 40)
link(scaled, ring)
ring_clamped = node(m, unreal.MaterialExpressionSaturate, -150, 40)
link(ring, ring_clamped)
fade_param = node(m, unreal.MaterialExpressionScalarParameter, -280, 200, parameter_name="Fade", default_value=1.0)
alpha = node(m, unreal.MaterialExpressionMultiply, 0, 80)
link(ring_clamped, alpha, "A")
link(fade_param, alpha, "B")
alpha_final = node(m, unreal.MaterialExpressionMultiply, 150, 80, const_b=.42)
link(alpha, alpha_final, "A")
white = node(m, unreal.MaterialExpressionConstant3Vector, 0, -100, constant=unreal.LinearColor(.9, .97, 1, 1))
mel.connect_material_property(white, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(alpha_final, "", unreal.MaterialProperty.MP_OPACITY)
finish(m)

# ---- lava (chão da arena do chefe): duas camadas de ruído andando, rachaduras quentes
m = fresh_material("M_LavaFlow")
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
wpos = node(m, unreal.MaterialExpressionWorldPosition, -1400, 0)
mask = node(m, unreal.MaterialExpressionComponentMask, -1250, 0, r=True, g=True, b=False, a=False)
link(wpos, mask)
scale_a = node(m, unreal.MaterialExpressionMultiply, -1100, -60, const_b=.0016)
link(mask, scale_a, "A")
time = node(m, unreal.MaterialExpressionTime, -1250, 200)
drift = node(m, unreal.MaterialExpressionMultiply, -1100, 200, const_b=.05)
link(time, drift, "A")
moved_a = node(m, unreal.MaterialExpressionAdd, -950, -60)
link(scale_a, moved_a, "A")
link(drift, moved_a, "B")
noise_a = node(m, unreal.MaterialExpressionNoise, -800, -60, scale=1.0, levels=4, output_min=0.0, output_max=1.0)
link(moved_a, noise_a, "Position")
scale_b = node(m, unreal.MaterialExpressionMultiply, -1100, 80, const_b=.0041)
link(mask, scale_b, "A")
moved_b = node(m, unreal.MaterialExpressionSubtract, -950, 80)
link(scale_b, moved_b, "A")
link(drift, moved_b, "B")
noise_b = node(m, unreal.MaterialExpressionNoise, -800, 80, scale=1.0, levels=3, output_min=0.0, output_max=1.0)
link(moved_b, noise_b, "Position")
mix = node(m, unreal.MaterialExpressionMultiply, -600, 0)
link(noise_a, mix, "A")
link(noise_b, mix, "B")
heat = node(m, unreal.MaterialExpressionPower, -450, 0, const_exponent=2.2)
link(mix, heat, "Base")
hot = node(m, unreal.MaterialExpressionConstant3Vector, -450, -200, constant=unreal.LinearColor(6.0, 1.15, .12, 1))
crust = node(m, unreal.MaterialExpressionConstant3Vector, -450, 200, constant=unreal.LinearColor(.06, .02, .015, 1))
glow = node(m, unreal.MaterialExpressionMultiply, -250, -120)
link(hot, glow, "A")
link(heat, glow, "B")
pulse_time = node(m, unreal.MaterialExpressionMultiply, -600, 300, const_b=1.7)
link(time, pulse_time, "A")
pulse_sin = node(m, unreal.MaterialExpressionSine, -450, 300)
link(pulse_time, pulse_sin)
pulse = node(m, unreal.MaterialExpressionMultiply, -300, 300, const_b=.2)
link(pulse_sin, pulse, "A")
pulse_add = node(m, unreal.MaterialExpressionAdd, -150, 300, const_b=1.0)
link(pulse, pulse_add, "A")
glow_pulse = node(m, unreal.MaterialExpressionMultiply, -50, -120)
link(glow, glow_pulse, "A")
link(pulse_add, glow_pulse, "B")
inv = node(m, unreal.MaterialExpressionOneMinus, -250, 120)
link(heat, inv)
base_color = node(m, unreal.MaterialExpressionMultiply, -50, 120)
link(crust, base_color, "A")
link(inv, base_color, "B")
mel.connect_material_property(base_color, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(glow_pulse, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
rough = node(m, unreal.MaterialExpressionConstant, -50, 260, r=.85)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
finish(m)

# ---- manoplas: Soco Sério
da = library.load_asset("/Game/Vortex/Weapons/Types/DA_manoplas_de_ferro")
if da:
    da.set_editor_property("secondary_kind", "serious")
    da.set_editor_property("secondary_name", "Soco Sério")
    da.set_editor_property("secondary_text", "Junta força e solta um punho gigante que voa longe, atravessa e arremessa tudo no caminho (20 de mana).")
    clips = "/Game/Vortex/Weapons/Clips/Adventurer_WeaponClips/SkeletalMeshes/Adventurer_WeaponClips"
    hold = library.load_asset(clips + "Fist_Counter")
    hit = library.load_asset(clips + "Fist_Attack_01")
    if hold:
        da.set_editor_property("secondary_hold_clip", hold)
    if hit:
        da.set_editor_property("secondary_clip", hit)
    library.save_loaded_asset(da)
    lines.append("manoplas: Soco Sério (pose %s, golpe %s)" % (bool(hold), bool(hit)))
else:
    lines.append("FALHOU: DA_manoplas_de_ferro")

# ---- efeitos novos re-salvos (sem recompilar a cada uso)
total = 0
for folder in ["/Game/Free_Spells", "/Game/Vefects"]:
    for path in library.list_assets(folder, recursive=True):
        kind = str(library.find_asset_data(path).asset_class_path.asset_name)
        if kind not in ("NiagaraSystem", "NiagaraEmitter"):
            continue
        asset = library.load_asset(path)
        if asset:
            asset.modify()
            library.save_loaded_asset(asset, False)
            total += 1
        else:
            lines.append("FALHOU efeito " + path)
lines.append("%d efeitos re-salvos" % total)

with open(os.path.join(root, "Saved", "Logs", "BuildCombatAssets.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
