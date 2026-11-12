"""O portal (estilo Solo Leveling): materiais e sons. Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi

- /Game/Vortex/World/Portal/M_PortalSwirl: o redemoinho (sem luz, aditivo, dos dois lados). Em pixels grandes
  (combina com o mundo de blocos): braços girando para dentro, chuvisco, a borda acesa e o centro claro. Parâmetros:
  Color, Intensity, Speed (negativo gira ao contrário), Twist, Arms, Pixels.
- /Game/Vortex/World/Portal/M_PortalGlow: brilho chapado (os blocos da borda e as faíscas). Color, Intensity.
- Sons de Data/Sfx/Portal (Tools/MakePortalAssets.py) -> /Game/Vortex/Audio/Sfx, na categoria Efeitos; o zumbido em laço.
"""
import os
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
lib = unreal.MaterialEditingLibrary
FOLDER = "/Game/Vortex/World/Portal"


def fresh_material(name):
    full = f"{FOLDER}/{name}"
    if library.does_asset_exist(full):
        library.delete_asset(full)
    mat = tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    return mat, full


def scalar(mat, name, value, x, y):
    node = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def vector(mat, name, value, x, y):
    node = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


# ---- redemoinho -------------------------------------------------------------------------------------
mat, full = fresh_material("M_PortalSwirl")
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("two_sided", True)
uv = lib.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1200, 0)
time = lib.create_material_expression(mat, unreal.MaterialExpressionTime, -1200, 120)
color = vector(mat, "Color", unreal.LinearColor(.25, .55, 1.0, 1.0), -1200, 240)
intensity = scalar(mat, "Intensity", 6.0, -1200, 400)
speed = scalar(mat, "Speed", 1.4, -1200, 480)
twist = scalar(mat, "Twist", 7.0, -1200, 560)
arms = scalar(mat, "Arms", 5.0, -1200, 640)
pixels = scalar(mat, "Pixels", 46.0, -1200, 720)
custom = lib.create_material_expression(mat, unreal.MaterialExpressionCustom, -700, 200)
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property("code", """
float2 q = floor(UV * Pixels) / Pixels + 0.5 / Pixels;
float2 p = q - 0.5;
float r = length(p) * 2.0;
float a = atan2(p.y, p.x);
float swirl = a * Arms + r * Twist - Time * Speed * 3.0;
float bands = 0.5 + 0.5 * sin(swirl);
float2 cell = floor(float2(a * 5.0 + Time * Speed, r * 9.0 - Time * 2.5));
float n = frac(sin(dot(cell, float2(12.9898, 78.233))) * 43758.5453);
float mask = 1.0 - smoothstep(0.9, 1.0, r);
float edge = smoothstep(0.7, 0.93, r) * mask;
float core = pow(saturate(1.0 - r * 1.4), 2.0);
float flicker = 0.85 + 0.15 * sin(Time * 9.0 + n * 6.28);
float v = (bands * 0.55 + n * n * 0.35) * mask * (0.3 + 0.7 * r) + edge * 1.8 + core * 1.2;
return Color * v * Intensity * flicker;
""")
inputs = []
for input_name in ("UV", "Time", "Color", "Intensity", "Speed", "Twist", "Arms", "Pixels"):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", input_name)
    inputs.append(ci)
custom.set_editor_property("inputs", inputs)
lib.connect_material_expressions(uv, "", custom, "UV")
lib.connect_material_expressions(time, "", custom, "Time")
lib.connect_material_expressions(color, "", custom, "Color")
lib.connect_material_expressions(intensity, "", custom, "Intensity")
lib.connect_material_expressions(speed, "", custom, "Speed")
lib.connect_material_expressions(twist, "", custom, "Twist")
lib.connect_material_expressions(arms, "", custom, "Arms")
lib.connect_material_expressions(pixels, "", custom, "Pixels")
lib.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.recompile_material(mat)
library.save_asset(full)

# ---- brilho chapado ---------------------------------------------------------------------------------
mat, full = fresh_material("M_PortalGlow")
color = vector(mat, "Color", unreal.LinearColor(.3, .6, 1.0, 1.0), -600, 0)
intensity = scalar(mat, "Intensity", 8.0, -600, 200)
mul = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 50)
lib.connect_material_expressions(color, "", mul, "A")
lib.connect_material_expressions(intensity, "", mul, "B")
lib.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.recompile_material(mat)
library.save_asset(full)

# ---- sons -------------------------------------------------------------------------------------------
effects = unreal.load_asset("/Game/Vortex/Audio/SC_Effects")
src = os.path.join(ROOT, "Data", "Sfx", "Portal")
count = 0
for file in sorted(os.listdir(src)):
    if not file.endswith(".wav"):
        continue
    name = file[:-4]
    task = unreal.AssetImportTask()
    task.filename = os.path.join(src, file)
    task.destination_path = "/Game/Vortex/Audio/Sfx"
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    wave = library.load_asset(f"/Game/Vortex/Audio/Sfx/{name}")
    wave.set_editor_property("looping", name == "portal_hum")
    if effects:
        wave.set_editor_property("sound_class_object", effects)
    library.save_loaded_asset(wave)
    count += 1
unreal.log_warning(f"VORTEX_PORTAL_ASSETS materiais ok, {count} sons")
