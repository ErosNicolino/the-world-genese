"""Os materiais animados do item no chão (08/11), editor fechado:

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportDropFx.py -unattended -nullrhi

M_DropBeam (coluna de luz: suave, com faixas subindo, some no alto), M_DropRing (o círculo no chão, em pixels, girando, com a onda)
e M_DropMote (os cubinhos que sobem). Color e Strength por instância; o AVortexItemDrop monta.
"""
import unreal

library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
FOLDER = "/Game/Vortex/Combat"

BEAM = """
float h = saturate(P.z / 100.0 + 0.5);
float fade = smoothstep(0.0, 0.05, h) * pow(1.0 - h, 1.7);
float soft = pow(saturate(abs(NdV)), 2.2);
float a = atan2(P.y, P.x);
float rise = 0.6 + 0.4 * sin(a * 5.0 + h * 26.0 - T * 4.0);
float pulse = 0.85 + 0.15 * sin(T * 2.4);
return C * S * fade * soft * rise * pulse;
"""

RING = """
float2 q = floor(uv * 72.0) / 72.0 + 0.5 / 72.0 - 0.5;
float d = length(q) * 2.0;
if (d > 1.0) return float3(0.0, 0.0, 0.0);
float a = atan2(q.y, q.x) / 6.28318;
float core = pow(saturate(1.0 - d), 2.5) * 0.45;
float band1 = saturate(1.0 - abs(d - 0.78) / 0.035);
float dash = step(frac(a * 12.0 + T * 0.3), 0.55) * saturate(1.0 - abs(d - 0.91) / 0.04);
float tick = step(frac(a * 20.0 - T * 0.2), 0.3) * saturate(1.0 - abs(d - 0.62) / 0.03);
float w = frac(T * 0.5 + Seed);
float wave = saturate(1.0 - abs(d - w) / 0.06) * (1.0 - w);
return C * S * (core + band1 * 0.9 + dash + tick * 0.7 + wave * 1.2);
"""


def base(name, two_sided=False, instanced=False):
    path = FOLDER + "/" + name
    mat = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", two_sided)
    mat.set_editor_property("used_with_instanced_static_meshes", instanced)
    return mat


def params(mat):
    color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    strength = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 200)
    strength.set_editor_property("parameter_name", "Strength")
    strength.set_editor_property("default_value", 1.0)
    return color, strength


def custom(mat, code, inputs):
    node = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -400, 0)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    entries = []
    for name in inputs:
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        entries.append(entry)
    node.set_editor_property("inputs", entries)
    for name, (source, pin) in inputs.items():
        mel.connect_material_expressions(source, pin, node, name)
    mel.connect_material_property(node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def finish(mat):
    mel.recompile_material(mat)
    library.save_loaded_asset(mat)


beam = base("M_DropBeam", two_sided=True)
color, strength = params(beam)
pos = mel.create_material_expression(beam, unreal.MaterialExpressionLocalPosition, -900, 400)
time = mel.create_material_expression(beam, unreal.MaterialExpressionTime, -900, 500)
normal = mel.create_material_expression(beam, unreal.MaterialExpressionVertexNormalWS, -1100, 600)
view = mel.create_material_expression(beam, unreal.MaterialExpressionCameraVectorWS, -1100, 700)
dot = mel.create_material_expression(beam, unreal.MaterialExpressionDotProduct, -900, 650)
mel.connect_material_expressions(normal, "", dot, "A")
mel.connect_material_expressions(view, "", dot, "B")
custom(beam, BEAM, {"C": (color, "RGB"), "S": (strength, ""), "P": (pos, ""), "T": (time, ""), "NdV": (dot, "")})
finish(beam)

ring = base("M_DropRing")
color, strength = params(ring)
uv = mel.create_material_expression(ring, unreal.MaterialExpressionTextureCoordinate, -900, 400)
time = mel.create_material_expression(ring, unreal.MaterialExpressionTime, -900, 500)
seed = mel.create_material_expression(ring, unreal.MaterialExpressionScalarParameter, -900, 600)
seed.set_editor_property("parameter_name", "Seed")
custom(ring, RING, {"C": (color, "RGB"), "S": (strength, ""), "uv": (uv, ""), "T": (time, ""), "Seed": (seed, "")})
finish(ring)

mote = base("M_DropMote", instanced=True)
color, strength = params(mote)
multiply = mel.create_material_expression(mote, unreal.MaterialExpressionMultiply, -400, 0)
mel.connect_material_expressions(color, "RGB", multiply, "A")
mel.connect_material_expressions(strength, "", multiply, "B")
mel.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
finish(mote)
unreal.log_warning("VORTEX_DROP_FX ok")
