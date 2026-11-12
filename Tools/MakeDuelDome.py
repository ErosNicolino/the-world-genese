"""O material do domo do duelo (02/11) — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/MakeDuelDome.py -unattended -nullrhi

/Game/Vortex/World/Duel/M_DuelDome: translúcido, sem luz e dos dois lados. Brilha na borda (fresnel) e em faixas
hexagonais que sobem devagar, o meio fica quase transparente. Parâmetros Color e Intensity. AVortexDuelDome põe
numa esfera em volta dos dois heróis do duelo.
"""
import unreal

library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.MaterialEditingLibrary
FOLDER = "/Game/Vortex/World/Duel"
NAME = "M_DuelDome"
full = FOLDER + "/" + NAME
if library.does_asset_exist(full):
    library.delete_asset(full)
mat = tools.create_asset(NAME, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)


def node(kind, x, y, **props):
    e = lib.create_material_expression(mat, kind, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


color = node(unreal.MaterialExpressionVectorParameter, -900, -100, parameter_name="Color", default_value=unreal.LinearColor(.35, .7, 1.0, 1.0))
intensity = node(unreal.MaterialExpressionScalarParameter, -900, 100, parameter_name="Intensity", default_value=1.6)
fresnel = node(unreal.MaterialExpressionFresnel, -900, 250, exponent=3.0)
world = node(unreal.MaterialExpressionWorldPosition, -900, 400)
time = node(unreal.MaterialExpressionTime, -900, 550)
custom = node(unreal.MaterialExpressionCustom, -500, 200)
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT2)
custom.set_editor_property("code", """
// faixas de hexágonos (em cm do mundo) que sobem devagar, mais a borda
float2 p = float2(Pos.x + Pos.y * 0.5, Pos.z - Time * 25.0) / 60.0;
float2 q = float2(p.x * 1.1547, p.y + p.x * 0.5774);
float2 f = frac(q) - 0.5;
float hexEdge = smoothstep(0.38, 0.48, max(abs(f.x), abs(f.y)));
float pulse = 0.6 + 0.4 * sin(Time * 2.0 + Pos.z * 0.01);
float glow = 0.25 + Rim * 2.0 + hexEdge * 0.9 * pulse;
float alpha = saturate(0.12 + Rim * 0.7 + hexEdge * 0.3 * pulse);
return float2(glow, alpha);
""")
inputs = []
for name in ("Pos", "Time", "Rim"):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", name)
    inputs.append(ci)
custom.set_editor_property("inputs", inputs)
lib.connect_material_expressions(world, "", custom, "Pos")
lib.connect_material_expressions(time, "", custom, "Time")
lib.connect_material_expressions(fresnel, "", custom, "Rim")
glow = node(unreal.MaterialExpressionComponentMask, -250, 150, r=True, g=False, b=False, a=False)
alpha = node(unreal.MaterialExpressionComponentMask, -250, 300, r=False, g=True, b=False, a=False)
lib.connect_material_expressions(custom, "", glow, "")
lib.connect_material_expressions(custom, "", alpha, "")
tint = node(unreal.MaterialExpressionMultiply, -100, -50)
lib.connect_material_expressions(color, "", tint, "A")
lib.connect_material_expressions(intensity, "", tint, "B")
emit = node(unreal.MaterialExpressionMultiply, 50, 100)
lib.connect_material_expressions(tint, "", emit, "A")
lib.connect_material_expressions(glow, "", emit, "B")
lib.connect_material_property(emit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)
lib.recompile_material(mat)
library.save_asset(full)
unreal.log_warning("VORTEX_DUEL_DOME material ok")
