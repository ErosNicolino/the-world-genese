"""Os materiais dos golpes especiais dos monstros (08/11), editor fechado:

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportMonsterFx.py -unattended -nullrhi

M_FxFunnel: o funil de vento (faixas em espiral subindo, somem nas pontas); M_FxPuff: as nuvens macias (poeira, veneno, lama),
em esferas instanciadas de borda suave. O círculo no chão reaproveita o M_DropRing. Color e Strength por instância; o
AVortexMonsterFx monta.
"""
import unreal

library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
FOLDER = "/Game/Vortex/Combat"

FUNNEL = """
float h = saturate(P.z / 100.0 + 0.5);
float a = atan2(P.y, P.x);
float ends = smoothstep(0.0, 0.12, h) * smoothstep(0.0, 0.25, 1.0 - h);
float soft = pow(saturate(abs(NdV)), 0.8);
float s1 = pow(saturate(0.5 + 0.5 * sin(a * 5.0 + h * 13.0 - T * 13.0)), 3.0);
float s2 = pow(saturate(0.5 + 0.5 * sin(a * 11.0 - h * 21.0 - T * 19.0)), 5.0);
float band = 0.1 + s1 * 0.6 + s2 * 0.3;
return C * S * ends * soft * band;
"""

PUFF = """
float rim = saturate(abs(NdV));
float n = 0.72 + 0.28 * sin(W.x * 0.07 + T * 1.3) * sin(W.y * 0.09 - T * 0.9) * sin(W.z * 0.08 + T * 0.7);
return pow(rim, 1.6) * n * S;
"""


def base(name, blend, instanced):
    path = FOLDER + "/" + name
    mat = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    mat.set_editor_property("blend_mode", blend)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", name == "M_FxFunnel")
    mat.set_editor_property("used_with_instanced_static_meshes", instanced)
    return mat


def param(mat, cls, name, value, y):
    node = mel.create_material_expression(mat, cls, -900, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def dot_view(mat):
    normal = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -1100, 600)
    view = mel.create_material_expression(mat, unreal.MaterialExpressionCameraVectorWS, -1100, 700)
    node = mel.create_material_expression(mat, unreal.MaterialExpressionDotProduct, -900, 650)
    mel.connect_material_expressions(normal, "", node, "A")
    mel.connect_material_expressions(view, "", node, "B")
    return node


def custom(mat, code, output, inputs):
    node = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -400, 0)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", output)
    entries = []
    for name in inputs:
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        entries.append(entry)
    node.set_editor_property("inputs", entries)
    for name, (source, pin) in inputs.items():
        mel.connect_material_expressions(source, pin, node, name)
    return node


funnel = base("M_FxFunnel", unreal.BlendMode.BLEND_ADDITIVE, False)
color = param(funnel, unreal.MaterialExpressionVectorParameter, "Color", unreal.LinearColor(1, 1, 1, 1), 0)
strength = param(funnel, unreal.MaterialExpressionScalarParameter, "Strength", 1.0, 200)
pos = mel.create_material_expression(funnel, unreal.MaterialExpressionLocalPosition, -900, 400)
time = mel.create_material_expression(funnel, unreal.MaterialExpressionTime, -900, 500)
node = custom(funnel, FUNNEL, unreal.CustomMaterialOutputType.CMOT_FLOAT3,
              {"C": (color, "RGB"), "S": (strength, ""), "P": (pos, ""), "T": (time, ""), "NdV": (dot_view(funnel), "")})
mel.connect_material_property(node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(funnel)
library.save_loaded_asset(funnel)

puff = base("M_FxPuff", unreal.BlendMode.BLEND_TRANSLUCENT, True)
color = param(puff, unreal.MaterialExpressionVectorParameter, "Color", unreal.LinearColor(.5, .5, .5, 1), 0)
strength = param(puff, unreal.MaterialExpressionScalarParameter, "Strength", .6, 200)
world = mel.create_material_expression(puff, unreal.MaterialExpressionWorldPosition, -900, 400)
time = mel.create_material_expression(puff, unreal.MaterialExpressionTime, -900, 500)
node = custom(puff, PUFF, unreal.CustomMaterialOutputType.CMOT_FLOAT1,
              {"S": (strength, ""), "W": (world, ""), "T": (time, ""), "NdV": (dot_view(puff), "")})
mel.connect_material_property(color, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(node, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(puff)
library.save_loaded_asset(puff)
unreal.log_warning("VORTEX_MONSTER_FX ok")
