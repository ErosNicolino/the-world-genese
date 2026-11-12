"""Material da miragem da Yggdrasil (M_YggMirage): uma cópia da árvore, translúcida e sem luz, que aparece
quando ela é vista de muito longe (de 800 m a 1,5 km fica inteira). Faixas de calor sobem pelo tronco, a
silhueta ondula de lado (mais no alto que na base), flutua devagar e brilha em tons de céu e ouro; a base
some no ar, como uma miragem sobre o horizonte. Não recebe névoa (senão sumiria junto com o relevo).
Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi
"""
import unreal

library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
FOLDER = "/Game/Vortex/Sky"
path = f"{FOLDER}/M_YggMirage"
m = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset("M_YggMirage", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("two_sided", True)
m.set_editor_property("use_translucency_vertex_fog", False)


def node(kind, x, y, **props):
    e = mel.create_material_expression(m, kind, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def custom(x, y, code, out, names):
    c = node(unreal.MaterialExpressionCustom, x, y, code=code, output_type=out)
    ins = []
    for n in names:
        i = unreal.CustomInput()
        i.set_editor_property("input_name", n)
        ins.append(i)
    c.set_editor_property("inputs", ins)
    return c


wp = node(unreal.MaterialExpressionWorldPosition, -1100, 0)
cam = node(unreal.MaterialExpressionCameraPositionWS, -1100, 120)
obj = node(unreal.MaterialExpressionObjectPositionWS, -1100, 240)
time = node(unreal.MaterialExpressionTime, -1100, 360)
bright = node(unreal.MaterialExpressionScalarParameter, -1100, 480, parameter_name="Brightness", default_value=.8)

COMMON = """
float D = distance(Wp, Cam);
float Fade = saturate((D - 80000.0) / 70000.0);
float H = saturate((Wp.z - Obj.z) / 24000.0 + 0.5);   // Obj = centro dos limites
"""
look = custom(-700, 0, COMMON + """
float S = 0.5 + 0.5 * sin(Wp.z * 0.0035 - T * 2.4 + sin(Wp.x * 0.001 + T) * 1.5);
float Flick = 0.85 + 0.15 * sin(T * 5.3 + Wp.z * 0.01);
float3 Low = float3(0.33, 0.56, 0.72);
float3 High = float3(0.95, 0.76, 0.42);
float3 C = lerp(Low, High, H) * (0.75 + 0.5 * S) * Bright;
float A = Fade * 0.8 * (0.5 + 0.5 * S) * Flick * saturate(H * 3.0 + 0.05);
return float4(C, A);
""", unreal.CustomMaterialOutputType.CMOT_FLOAT4, ["Wp", "Cam", "Obj", "T", "Bright"])
for i, src in enumerate([wp, cam, obj, time, bright]):
    mel.connect_material_expressions(src, "", look, ["Wp", "Cam", "Obj", "T", "Bright"][i])
rgb = node(unreal.MaterialExpressionComponentMask, -400, 0, r=True, g=True, b=True, a=False)
alpha = node(unreal.MaterialExpressionComponentMask, -400, 140, r=False, g=False, b=False, a=True)
mel.connect_material_expressions(look, "", rgb, "")
mel.connect_material_expressions(look, "", alpha, "")
mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)

wobble = custom(-700, 320, COMMON + """
float Wx = sin(T * 1.6 + Wp.z * 0.0022) * 320.0 + sin(T * 3.1 + Wp.z * 0.007) * 90.0;
float Wy = cos(T * 1.2 + Wp.z * 0.0018) * 220.0;
float Wz = sin(T * 0.7) * 400.0;
return float3(Wx, Wy, Wz) * Fade * (0.2 + H);
""", unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["Wp", "Cam", "Obj", "T"])
for i, src in enumerate([wp, cam, obj, time]):
    mel.connect_material_expressions(src, "", wobble, ["Wp", "Cam", "Obj", "T"][i])
mel.connect_material_property(wobble, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
mel.recompile_material(m)
library.save_loaded_asset(m)
unreal.log_warning("VORTEX_MIRAGE material ok")
