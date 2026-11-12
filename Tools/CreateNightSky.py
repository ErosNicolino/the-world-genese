"""Fase 15: material do céu noturno (estrelas, lua com fases e halo) — rode com o editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/CreateNightSky.py -unattended

Cria /Game/Vortex/Sky/T_NightStars (de Data/Sky/NightStars.png, Tools/MakeNightStars.py) e
/Game/Vortex/Sky/M_NightSky: aditivo, sem luz, dos dois lados, numa esfera grande em volta da
câmera (UVortexDayNightSubsystem). Tudo é calculado pela direção do olhar:
- estrelas: mapa equirretangular girando devagar, piscando pela fase de cada uma, somem perto do
  horizonte e de dia (parâmetro Night);
- lua: disco de ~2° na direção MoonDir, com fase (Phase 0 = nova, 0,5 = cheia) iluminada como uma
  esfera, manchas em blocos (estilo voxel) e um resto muito fraco de luz cinérea no lado escuro;
- halo azulado em volta, proporcional à parte iluminada.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
destination = "/Game/Vortex/Sky"
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

task = unreal.AssetImportTask()
task.filename = os.path.join(root, "Data", "Sky", "NightStars.png")
task.destination_path = destination
task.destination_name = "T_NightStars"
task.automated = True
task.replace_existing = True
task.save = False
tools.import_asset_tasks([task])
stars = library.load_asset(destination + "/T_NightStars")
if not stars:
    raise RuntimeError("star texture import failed")
stars.set_editor_property("srgb", False)
stars.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
stars.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
stars.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
stars.set_editor_property("never_stream", True)
library.save_loaded_asset(stars)

CODE = r"""
float3 d = normalize(Dir);
float3 col = 0;
// estrelas
float s = sin(StarSpin), c = cos(StarSpin);
float3 r = float3(d.x * c - d.y * s, d.x * s + d.y * c, d.z);
float2 uv = float2(atan2(r.y, r.x) / 6.2831853 + 0.5, acos(clamp(r.z, -1.0, 1.0)) / 3.14159265);
float4 st = Texture2DSample(Tex, TexSampler, uv);
float twinkle = 0.65 + 0.35 * sin(Time * 2.1 + st.g * 6.2831853);
float horizon = smoothstep(0.02, 0.25, d.z);
float3 tint = lerp(float3(0.75, 0.85, 1.0), float3(1.0, 0.92, 0.78), st.b);
col += st.r * st.r * twinkle * tint * Night * horizon * StarBright;
// lua
float3 m = normalize(MoonDir);
float cosA = dot(d, m);
float ang = acos(clamp(cosA, -1.0, 1.0));
float radius = 0.036;
float3 side = normalize(cross(float3(0, 0, 1), m) + float3(0.0001, 0, 0));
float3 top = cross(m, side);
float2 p = float2(dot(d, side), dot(d, top)) / sin(radius);
float rr = dot(p, p);
float phaseAngle = Phase * 6.2831853;
float lit = 0.5 - 0.5 * cos(phaseAngle);
float moonUp = smoothstep(-0.03, 0.02, d.z) * MoonVisible;
if (cosA > 0.0 && rr < 1.0)
{
    float3 n = float3(p, sqrt(1.0 - rr));
    float3 l = float3(-sin(phaseAngle), 0.0, -cos(phaseAngle));
    float shade = smoothstep(-0.07, 0.07, dot(n, l));
    float2 cell = floor(p * 7.0);
    float maria = 0.78 + 0.22 * frac(sin(dot(cell, float2(12.9898, 78.233))) * 43758.5453);
    float edge = smoothstep(1.0, 0.86, rr);
    col += float3(0.93, 0.95, 1.0) * (shade * maria + 0.0012) * edge * MoonBright * moonUp;
}
// halo
float beyond = max(ang - radius, 0.0);
float halo = (0.22 * exp(-beyond * 14.0) + 0.5 * exp(-beyond * 70.0)) * lit;
col += float3(0.55, 0.66, 1.0) * halo * MoonBright * 0.12 * moonUp;
return col;
"""

path = destination + "/M_NightSky"
material = library.load_asset(path) if library.does_asset_exist(path) else \
    tools.create_asset("M_NightSky", destination, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(material)
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property("two_sided", True)
applied = []
for name, value in (("use_translucency_vertex_fog", False), ("apply_fogging", False), ("is_sky", False),
                    ("allow_negative_emissive_color", False), ("used_with_static_lighting", False)):
    try:
        material.set_editor_property(name, value)
        applied.append(name)
    except Exception:
        pass

custom = mel.create_material_expression(material, unreal.MaterialExpressionCustom, -400, 0)
custom.set_editor_property("code", CODE)
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property("description", "NightSky")
names = ["Dir", "MoonDir", "Phase", "Night", "StarSpin", "StarBright", "MoonBright", "MoonVisible", "Time", "Tex"]
inputs = []
for name in names:
    item = unreal.CustomInput()
    item.set_editor_property("input_name", name)
    inputs.append(item)
custom.set_editor_property("inputs", inputs)

camera = mel.create_material_expression(material, unreal.MaterialExpressionCameraVectorWS, -900, -300)
flip = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -700, -300)
flip.set_editor_property("const_b", -1.0)
mel.connect_material_expressions(camera, "", flip, "A")
mel.connect_material_expressions(flip, "", custom, "Dir")

moon = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -900, -150)
moon.set_editor_property("parameter_name", "MoonDir")
moon.set_editor_property("default_value", unreal.LinearColor(0.3, 0.2, 0.9, 0.0))
mel.connect_material_expressions(moon, "RGB", custom, "MoonDir")
defaults = {"Phase": 0.5, "Night": 1.0, "StarSpin": 0.0, "StarBright": 2.5, "MoonBright": 6.0, "MoonVisible": 1.0}
y = 0
for name, value in defaults.items():
    param = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -900, y)
    param.set_editor_property("parameter_name", name)
    param.set_editor_property("default_value", value)
    mel.connect_material_expressions(param, "", custom, name)
    y += 80
time = mel.create_material_expression(material, unreal.MaterialExpressionTime, -900, y)
mel.connect_material_expressions(time, "", custom, "Time")
texture = mel.create_material_expression(material, unreal.MaterialExpressionTextureObject, -900, y + 80)
texture.set_editor_property("texture", stars)
mel.connect_material_expressions(texture, "", custom, "Tex")
mel.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(material)
library.save_loaded_asset(material)
with open(os.path.join(root, "Saved", "Logs", "CreateNightSky.txt"), "w") as out:
    out.write("VORTEX_NIGHT_SKY material=%s applied=%s\n" % (path, applied))
