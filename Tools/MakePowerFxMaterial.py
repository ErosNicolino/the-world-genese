"""Os materiais dos efeitos dos poderes (AVortexPowerFx). Editor fechado:
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/MakePowerFxMaterial.py -unattended -nullrhi

- M_PowerFx: a energia (colunas de luz, feixes, anéis, orbes, runas, raios, penas, rastros). 07/11, o usuário: "muitas
  animações sobem um retângulo cinza, sem textura": era um material sólido de cor chapada (a cor clara com brilho baixo,
  no sol, virava um bloco cinza). Agora soma luz (aditivo, dos dois lados): mais forte no meio e apagando na borda
  (fresnel) e nas pontas (a altura da UV), como um facho de verdade. Parâmetros: Color, Glow.
- M_PowerSolid: as peças sólidas (pedras, raízes, lascas, água): com luz e sombra e blocos pixelados (8 por face, cada um
  num tom, junta mais escura) tingidos pela Color.
"""
import os

import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary


def fresh(name):
    path = "/Game/Vortex/Combat/" + name
    if library.does_asset_exist(path):
        library.delete_asset(path)
    m = tools.create_asset(name, "/Game/Vortex/Combat", unreal.Material, unreal.MaterialFactoryNew())
    for flag in ("used_with_instanced_static_meshes", "used_with_static_lighting"):
        try:
            m.set_editor_property(flag, True)
        except Exception:
            pass
    return m


def param_color(m, x, y, default):
    c = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
    c.set_editor_property("parameter_name", "Color")
    c.set_editor_property("default_value", default)
    return c


def custom(m, x, y, code, names, out=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
    node = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    node.set_editor_property("output_type", out)
    node.set_editor_property("code", code)
    ins = []
    for n in names:
        i = unreal.CustomInput()
        i.set_editor_property("input_name", n)
        ins.append(i)
    node.set_editor_property("inputs", ins)
    return node


# ---- a energia: aditiva, borda e pontas suaves
m = fresh("M_PowerFx")
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("two_sided", True)
color = param_color(m, -900, 0, unreal.LinearColor(1, .6, .2, 1))
glow = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, 200)
glow.set_editor_property("parameter_name", "Glow")
glow.set_editor_property("default_value", 1.0)
fresnel = MEL.create_material_expression(m, unreal.MaterialExpressionFresnel, -900, 350)
uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -900, 500)
node = custom(m, -450, 150,
              "float Edge = saturate(1.0 - F * 0.8);"
              "float Ends = pow(saturate(sin(saturate(UV.y) * 3.14159)), 0.6);"
              "return C.rgb * G * 1.5 * Edge * (0.3 + 0.7 * Ends);",
              ("C", "G", "F", "UV"))
for src, pin in ((color, "C"), (glow, "G"), (fresnel, "F"), (uv, "UV")):
    MEL.connect_material_expressions(src, "", node, pin)
MEL.connect_material_property(node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
library.save_loaded_asset(m)

# ---- as peças sólidas: blocos de 1/8 da face, cada um num tom, com a junta mais escura (calculado no material: a textura
# importada pela linha de comando chegava preta)
s = fresh("M_PowerSolid")
color = param_color(s, -900, 0, unreal.LinearColor(.5, .45, .4, 1))
glow = MEL.create_material_expression(s, unreal.MaterialExpressionScalarParameter, -900, 400)
glow.set_editor_property("parameter_name", "Glow")
glow.set_editor_property("default_value", 0.0)
uv = MEL.create_material_expression(s, unreal.MaterialExpressionTextureCoordinate, -900, 200)
blocks = custom(s, -500, 50,
                "float2 Cell = floor(UV * 8.0);"
                "float H = frac(sin(dot(Cell, float2(12.9898, 78.233))) * 43758.5453);"
                "float2 F = frac(UV * 8.0);"
                "float Joint = (F.x < 0.1 || F.y < 0.1) ? 0.82 : 1.0;"
                "return C.rgb * (0.74 + 0.26 * H) * Joint;",
                ("C", "UV"))
MEL.connect_material_expressions(color, "", blocks, "C")
MEL.connect_material_expressions(uv, "", blocks, "UV")
MEL.connect_material_property(blocks, "", unreal.MaterialProperty.MP_BASE_COLOR)
rough = MEL.create_material_expression(s, unreal.MaterialExpressionConstant, -500, 250)
rough.set_editor_property("r", .85)
MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(s)
library.save_loaded_asset(s)
unreal.log_warning("VORTEX_POWER_FX_MATERIAL ok")
