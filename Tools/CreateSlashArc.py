"""Corte à distância (arco de energia 3D): /Game/Vortex/Combat/M_SlashArc — rode com o editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/CreateSlashArc.py -unattended

Usado pelo AVortexSlashWave numa fita curva gerada em código (UV.x ao longo do arco, UV.y da borda
da frente, 0, até a de trás, 1). Aditivo e sem luz própria (o bloom faz o halo):
- borda da frente quase branca, corpo na cor do golpe (Color), pontas finas;
- fiapos de energia correndo ao longo do arco (ruído dos shockwaves do Vefects);
- Glow (força), Fade (0..1 liga/desliga), Seed (desenho de cada cópia), Sharp (finura da borda).
"""
import os
import unreal

MEL = unreal.MaterialEditingLibrary
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = "/Game/Vortex/Combat"
PATH = FOLDER + "/M_SlashArc"
NOISE = "/Game/Vefects/Easy_Impact_Frames/VFX/Extras/Textures/Shockwaves/T_VFX_Easy_Shockwaves_Noise_01"

if library.does_asset_exist(PATH):
    library.delete_asset(PATH)
m = tools.create_asset("M_SlashArc", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("two_sided", True)

uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1100, 0)
time = MEL.create_material_expression(m, unreal.MaterialExpressionTime, -1100, 100)
noise = MEL.create_material_expression(m, unreal.MaterialExpressionTextureObjectParameter, -1100, 200)
noise.set_editor_property("parameter_name", "Noise")
noise.set_editor_property("texture", library.load_asset(NOISE))
sources = {"UV": uv, "Time": time, "Noise": noise}
for i, (name, value) in enumerate((("Fade", 1.0), ("Glow", 6.0), ("Seed", 0.0), ("Sharp", 7.0))):
    p = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -1100, 350 + i * 100)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", value)
    sources[name] = p
color = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -1100, 800)
color.set_editor_property("parameter_name", "Color")
color.set_editor_property("default_value", unreal.LinearColor(0.08, 0.45, 1.0, 1.0))
sources["Color"] = color

node = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -500, 100)
node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
node.set_editor_property("description", "Arco de energia")
node.set_editor_property("code", """
float u = UV.x, v = UV.y;
// pontas finas e um pouco mais fracas que o meio
float tips = smoothstep(0.0, 0.16, u) * smoothstep(1.0, 0.84, u);
float middle = 0.7 + 0.3 * sin(u * 3.14159);
// borda da frente: fio branco brilhante
float edge = exp(-v * Sharp);
// fiapos de energia correndo ao longo do arco, mais soltos para trás
float2 a = float2(u * 2.4 - Time * 3.2 + Seed * 5.0, v * 0.9 + Seed);
float2 b = float2(u * 5.1 + Time * 1.9 - Seed * 3.0, v * 1.7 - Time * 0.6);
float n = Noise.Sample(NoiseSampler, a).r;
float n2 = Noise.Sample(NoiseSampler, b).r;
float wisps = saturate(n * n2 * 2.6);
float body = saturate(1.0 - v) * (0.25 + 1.1 * wisps) * (1.0 - smoothstep(0.55, 1.0, v));
float alpha = tips * middle * (edge * 1.6 + body);
float3 c = lerp(Color.rgb, float3(1.0, 1.0, 1.0), saturate(edge * edge * 1.2));
return c * alpha * Glow * Fade;
""")
inputs = []
for name in ("UV", "Time", "Noise", "Fade", "Glow", "Seed", "Sharp", "Color"):
    item = unreal.CustomInput()
    item.set_editor_property("input_name", name)
    inputs.append(item)
node.set_editor_property("inputs", inputs)
for name in ("UV", "Time", "Noise", "Fade", "Glow", "Seed", "Sharp", "Color"):
    if not MEL.connect_material_expressions(sources[name], "", node, name):
        raise RuntimeError("ligação falhou: " + name)
MEL.connect_material_property(node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
library.save_loaded_asset(m)
root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
with open(os.path.join(root, "Saved", "Logs", "CreateSlashArc.txt"), "w") as out:
    out.write("VORTEX_SLASH_ARC " + PATH + "\n")
