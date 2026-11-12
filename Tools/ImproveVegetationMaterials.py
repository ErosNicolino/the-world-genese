"""Vegetação e terreno mais vivos, no estilo voxel — rode com o editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImproveVegetationMaterials.py -unattended -nullrhi

Antes: blender --background --factory-startup --python Tools/Blender/MakeMacroNoise.py (Data/Textures/T_MacroNoise.png).

- Terreno (M_TerrainTiles) e objetos (M_VoxelProp): manchas de tom em escala grande (40 m e 9 m, pelo ruído
  T_MacroNoise na posição do mundo), mais claras/quentes e mais escuras/frias. Antes cada bloco de grama era
  igual ao vizinho em todo o vale e a repetição aparecia de longe.
- Objetos instanciados (árvores, arbustos, flores, pedras): cada cópia com um tom e um brilho um pouco
  diferentes (PerInstanceRandom). Uma floresta de carvalhos deixa de ser o mesmo carvalho repetido.
- Árvores e arbustos (MI_tree_* e MI_bush_*): passam para M_VoxelPropFade, igual ao M_VoxelProp mas que
  some com pontilhado (dither) quando a câmera chega a menos de ~2,5 m. A câmera entrava nas copas e a tela
  ficava toda de blocos de folha. A sombra não some.
Rodar de novo não acumula: o nó final de cada material é marcado ("VortexVariation") e refeito.
Relatório em Saved/Logs/ImproveVegetationMaterials.txt.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MATS = "/Game/Vortex/Materials"
TAG = "VortexVariation"
lines = []

# ---- textura de ruído
task = unreal.AssetImportTask()
task.filename = os.path.join(root, "Data", "Textures", "T_MacroNoise.png")
task.destination_path = MATS
task.destination_name = "T_MacroNoise"
task.automated = True
task.save = True
task.replace_existing = True
tools.import_asset_tasks([task])
noise = lib.load_asset(MATS + "/T_MacroNoise")
noise.set_editor_property("srgb", False)
noise.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
lib.save_loaded_asset(noise)


def node(material, cls, x, y, **props):
    expression = mel.create_material_expression(material, cls, x, y)
    for key, value in props.items():
        expression.set_editor_property(key, value)
    return expression


def color(material, x, y, r, g, b):
    return node(material, unreal.MaterialExpressionConstant3Vector, x, y, constant=unreal.LinearColor(r, g, b, 1))


def lerp(material, x, y, a, b, alpha, const_a=None, const_b=None):
    expression = node(material, unreal.MaterialExpressionLinearInterpolate, x, y)
    if const_a is not None:
        expression.set_editor_property("const_a", const_a)
    else:
        mel.connect_material_expressions(a, "", expression, "A")
    if const_b is not None:
        expression.set_editor_property("const_b", const_b)
    else:
        mel.connect_material_expressions(b, "", expression, "B")
    mel.connect_material_expressions(alpha, "", expression, "Alpha")
    return expression


def multiply(material, x, y, a, b):
    expression = node(material, unreal.MaterialExpressionMultiply, x, y)
    mel.connect_material_expressions(a, "", expression, "A")
    mel.connect_material_expressions(b, "", expression, "B")
    return expression


def original_base(material):
    """O que entrava na cor antes desta melhoria (desfaz o nó marcado de uma rodada anterior)."""
    current = mel.get_material_property_input_node(material, unreal.MaterialProperty.MP_BASE_COLOR)
    if current and current.get_editor_property("desc") == TAG:
        return mel.get_inputs_for_material_expression(material, current)[0]
    return current


def add_variation(material, per_instance):
    base = original_base(material)
    world = node(material, unreal.MaterialExpressionWorldPosition, -1400, 600)
    xy = node(material, unreal.MaterialExpressionComponentMask, -1200, 600, r=True, g=True, b=False, a=False)
    mel.connect_material_expressions(world, "", xy, "")
    samples = []
    for i, (scale, channel) in enumerate(((4000.0, "R"), (900.0, "G"))):
        divide = node(material, unreal.MaterialExpressionDivide, -1000, 520 + i * 180, const_b=scale)
        mel.connect_material_expressions(xy, "", divide, "A")
        sample = node(material, unreal.MaterialExpressionTextureSample, -800, 520 + i * 180, texture=noise,
                      sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        mel.connect_material_expressions(divide, "", sample, "UVs")
        samples.append((sample, channel))
    # 70% as manchas grandes, 30% as médias
    mixed = node(material, unreal.MaterialExpressionLinearInterpolate, -560, 700)
    mel.connect_material_expressions(samples[0][0], "R", mixed, "A")
    mel.connect_material_expressions(samples[1][0], "G", mixed, "B")
    mel.connect_material_expressions(node(material, unreal.MaterialExpressionConstant, -700, 820, r=.3), "", mixed, "Alpha")
    # brilho 0,88..1,08 e tom: manchas quentes (mais douradas) e frias (mais azuladas)
    bright = lerp(material, -300, 700, None, None, mixed, 0.88, 1.08)
    tone = lerp(material, -300, 840, color(material, -500, 900, 1.04, 1.0, .93), color(material, -500, 980, .95, 1.0, 1.04), mixed)
    result = multiply(material, -120, 760, bright, tone)
    if per_instance:
        rnd = node(material, unreal.MaterialExpressionPerInstanceRandom, -500, 1100)
        shade = lerp(material, -300, 1100, None, None, rnd, 0.9, 1.07)
        hue = lerp(material, -300, 1220, color(material, -500, 1200, 1.05, 1.0, .9), color(material, -500, 1280, .92, 1.02, 1.04), rnd)
        result = multiply(material, -60, 1100, result, multiply(material, -160, 1150, shade, hue))
    final = multiply(material, 100, 200, base, result)
    final.set_editor_property("desc", TAG)
    mel.connect_material_property(final, "", unreal.MaterialProperty.MP_BASE_COLOR)
    # (nós soltos de uma rodada anterior não pesam: o compilador ignora o que não chega na saída)


terrain = lib.load_asset(MATS + "/M_TerrainTiles")
add_variation(terrain, per_instance=False)
mel.recompile_material(terrain)
lib.save_loaded_asset(terrain)
lines.append("M_TerrainTiles: manchas de tom em escala grande")

prop = lib.load_asset(MATS + "/M_VoxelProp")
add_variation(prop, per_instance=True)
prop.set_editor_property("used_with_instanced_static_meshes", True)
mel.recompile_material(prop)
lib.save_loaded_asset(prop)
lines.append("M_VoxelProp: manchas de tom + tom por cópia")

# ---- árvores e arbustos: somem com pontilhado perto da câmera
fade_path = MATS + "/M_VoxelPropFade"
if lib.does_asset_exist(fade_path):
    lib.delete_asset(fade_path)
fade = lib.duplicate_asset(MATS + "/M_VoxelProp", fade_path)
fade.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
fade.set_editor_property("opacity_mask_clip_value", .5)
world = node(fade, unreal.MaterialExpressionWorldPosition, -900, 1500)
camera = node(fade, unreal.MaterialExpressionCameraPositionWS, -900, 1600)
distance = node(fade, unreal.MaterialExpressionDistance, -700, 1540)
mel.connect_material_expressions(world, "", distance, "A")
mel.connect_material_expressions(camera, "", distance, "B")
# 0 até 1 m da câmera, 1 a partir de 2,5 m
near = node(fade, unreal.MaterialExpressionSubtract, -550, 1540, const_b=100.0)
mel.connect_material_expressions(distance, "", near, "A")
ramp = node(fade, unreal.MaterialExpressionDivide, -420, 1540, const_b=150.0)
mel.connect_material_expressions(near, "", ramp, "A")
clamp = node(fade, unreal.MaterialExpressionSaturate, -300, 1540)
mel.connect_material_expressions(ramp, "", clamp, "")
# pontilhado por pixel (ruído de gradiente intercalado; o DitherTemporalAA não aparece no Python da Unreal)
dither = node(fade, unreal.MaterialExpressionCustom, -160, 1540, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT1,
              code="\n".join(["#if SHADOW_DEPTH_SHADER",
                              "return 1;",
                              "#else",
                              "float2 p = floor(Parameters.SvPosition.xy);",
                              "float n = frac(52.9829189 * frac(dot(p, float2(0.06711056, 0.00583715))));",
                              "return A - n + 0.5;",
                              "#endif"]))
dither_input = unreal.CustomInput()
dither_input.set_editor_property("input_name", "A")
dither.set_editor_property("inputs", [dither_input])
mel.connect_material_expressions(clamp, "", dither, "A")
# (a sombra não some: no passe de sombra o código devolve 1; o ShadowPassSwitch também não aparece no Python)
mel.connect_material_property(dither, "", unreal.MaterialProperty.MP_OPACITY_MASK)
fade.set_editor_property("used_with_instanced_static_meshes", True)
fade.set_editor_property("used_with_nanite", True)
mel.recompile_material(fade)
lib.save_loaded_asset(fade)

moved = 0
for path in lib.list_assets("/Game/Vortex/Props", recursive=True):
    name = path.split("/")[-1].split(".")[0]
    if not (name.startswith("MI_tree_") or name.startswith("MI_bush_")):
        continue
    mi = lib.load_asset(path)
    if isinstance(mi, unreal.MaterialInstanceConstant):
        mel.set_material_instance_parent(mi, fade)
        lib.save_loaded_asset(mi)
        moved += 1
lines.append("M_VoxelPropFade: %d árvores e arbustos somem perto da câmera" % moved)

with open(os.path.join(root, "Saved", "Logs", "ImproveVegetationMaterials.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
unreal.log("VORTEX_VEGETATION " + " | ".join(lines))
