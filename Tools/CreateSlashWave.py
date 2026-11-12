"""Corte à distância da espada: /Game/Vortex/Combat/M_SlashWave — rode com o editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/CreateSlashWave.py -unattended

Lua crescente de energia, aditiva e sem luz própria (brilha sozinha e o bloom faz o halo):
- forma desenhada no material (dois círculos: dentro de um e fora do outro), com pontas finas;
- borda da frente quase branca, corpo azul-ciano, e fiapos de energia correndo pelo arco com o
  ruído dos shockwaves do Vefects (referência do jogador, copiado em Content/Vefects/...);
- Fade (0..1) liga/desliga; Seed muda o desenho de cada placa.
"""
import os
import unreal

MEL = unreal.MaterialEditingLibrary
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = "/Game/Vortex/Combat"
PATH = FOLDER + "/M_SlashWave"
NOISE = "/Game/Vefects/Easy_Impact_Frames/VFX/Extras/Textures/Shockwaves/T_VFX_Easy_Shockwaves_Noise_01"

if library.does_asset_exist(PATH):
    library.delete_asset(PATH)
m = tools.create_asset("M_SlashWave", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("two_sided", True)

uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1100, 0)
time = MEL.create_material_expression(m, unreal.MaterialExpressionTime, -1100, 100)
noise = MEL.create_material_expression(m, unreal.MaterialExpressionTextureObjectParameter, -1100, 200)
noise.set_editor_property("parameter_name", "Noise")
noise.set_editor_property("texture", library.load_asset(NOISE))
params = {}
for i, (name, value) in enumerate((("Fade", 1.0), ("Glow", 5.0), ("Seed", 0.0))):
    p = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -1100, 350 + i * 100)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", value)
    params[name] = p

node = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -500, 100)
node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
node.set_editor_property("description", "Lua de energia")
node.set_editor_property("code", """
// p: -1..1, x ao longo do arco, y para cima (a parte de cima da placa é a frente do arco)
float2 p = (UV - 0.5) * 2.0;
p.y = -p.y;
float d1 = length(p - float2(0.0, -0.62));   // círculo de fora
float d2 = length(p - float2(0.0, -0.98));   // círculo de dentro (tira o miolo)
float inner = d1 - 1.0;                        // < 0 dentro do de fora
float outer = 1.12 - d2;                       // < 0 fora do de dentro
float band = saturate(-inner / 0.035) * saturate(-outer / 0.05);
// pontas finas: some nas laterais
float tips = 1.0 - smoothstep(0.55, 0.98, abs(p.x));
// brilho maior na borda da frente
float edge = exp(inner * 14.0);
// fiapos de energia correndo pelo arco
float a = atan2(p.x, p.y + 0.62);
float2 nuv = float2(a * 1.4 - Time * 2.6 + Seed * 7.0, d1 * 1.8 + Seed);
float n = Noise.Sample(NoiseSampler, nuv).r;
float n2 = Noise.Sample(NoiseSampler, nuv * float2(2.3, 0.7) + float2(Time * 1.3, 0.0)).r;
float energy = saturate(0.35 + 1.1 * n * n2 + edge * 0.8);
float mask = band * tips * energy;
float3 body = float3(0.04, 0.30, 1.00);
float3 core = float3(0.85, 0.96, 1.00);
float3 c = lerp(body, core, saturate(edge * edge * 1.1));
return c * mask * (0.5 + 1.5 * edge) * Glow * Fade;
""")
inputs = []
for name in ("UV", "Time", "Noise", "Fade", "Glow", "Seed"):
    item = unreal.CustomInput()
    item.set_editor_property("input_name", name)
    inputs.append(item)
node.set_editor_property("inputs", inputs)
for name, source in (("UV", uv), ("Time", time), ("Noise", noise), ("Fade", params["Fade"]), ("Glow", params["Glow"]), ("Seed", params["Seed"])):
    if not MEL.connect_material_expressions(source, "", node, name):
        raise RuntimeError("ligação falhou: " + name)
MEL.connect_material_property(node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
library.save_loaded_asset(m)
root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
with open(os.path.join(root, "Saved", "Logs", "CreateSlashWave.txt"), "w") as out:
    out.write("VORTEX_SLASH_MATERIAL " + PATH + "\n")
