"""Arte do menu e da criação de herói — rode com o editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/CreateFrontEndArt.py -unattended

Do Godot (somente leitura):
- Assets/UI/Races/<raça>.png -> /Game/Vortex/UI/Menu/Races/T_Race_<raça> (cartas da seleção);
- Assets/Brand/game_icon.png -> T_Emblem (brasão do logo);
- Assets/UI/character-select-background-v3.png -> T_StageBackdrop (o vale com o palco de pedra);
- Assets/Sfx/ui_click, ui_open, ui_close, ui_error, level_up, equip -> /Game/Vortex/Audio/UI;
- a música tema (Assets/Music/Echo_des_Montagnes.mp3), convertida antes para WAV pelo ffmpeg em
  %TEMP%/claude/menuaudio/MUS_Theme.wav (a Unreal não importa mp3) -> /Game/Vortex/Audio/Music/MUS_Theme.
Materiais do palco 3D do menu (AVortexMenuStage):
- M_StageBackdrop: o cenário pintado, sem luz, na frente da câmera;
- M_StageRing: círculo de runas que brilha no chão sob o herói (cor da raça).
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
godot = os.path.normpath(os.path.join(root, "..", "..", "the-world-geneses", "Assets"))
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
lines = []


def import_file(source, folder, name):
    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = folder
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    asset = library.load_asset(folder + "/" + name)
    if not asset:
        raise RuntimeError("falhou: " + source)
    return asset


def ui_texture(texture, max_size):
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("max_texture_size", max_size)
    texture.set_editor_property("srgb", True)
    library.save_loaded_asset(texture)


for race, file in (("humano", "humano"), ("elfo", "elfo"), ("diabo", "diabo"), ("anao", "anao"), ("barbaro", "barbaro"), ("morto_vivo", "morto-vivo")):
    ui_texture(import_file(os.path.join(godot, "UI", "Races", file + ".png"), "/Game/Vortex/UI/Menu/Races", "T_Race_" + race), 768)
    lines.append("T_Race_" + race)
ui_texture(import_file(os.path.join(godot, "Brand", "game_icon.png"), "/Game/Vortex/UI/Menu", "T_Emblem"), 512)
backdrop = import_file(os.path.join(godot, "UI", "character-select-background-v3.png"), "/Game/Vortex/UI/Menu", "T_StageBackdrop")
backdrop.set_editor_property("srgb", True)
# carrega inteira de uma vez: com streaming, o menu abria mostrando só a média da imagem (cinza)
backdrop.set_editor_property("never_stream", True)
backdrop.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
backdrop.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
backdrop.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
library.save_loaded_asset(backdrop)
lines.append("T_Emblem T_StageBackdrop")

for name in ("ui_click", "ui_open", "ui_close", "ui_error", "level_up", "equip"):
    wave = import_file(os.path.join(godot, "Sfx", name + ".wav"), "/Game/Vortex/Audio/UI", name)
    library.save_loaded_asset(wave)
music_source = os.path.join(os.environ["TEMP"], "claude", "menuaudio", "MUS_Theme.wav")
if os.path.isfile(music_source):
    music = import_file(music_source, "/Game/Vortex/Audio/Music", "MUS_Theme")
    music.set_editor_property("looping", True)
    library.save_loaded_asset(music)
    lines.append("MUS_Theme %.0fs" % music.get_editor_property("duration"))


def new_material(name):
    path = "/Game/Vortex/UI/Menu/" + name
    if library.does_asset_exist(path):
        library.delete_asset(path)
    return tools.create_asset(name, "/Game/Vortex/UI/Menu", unreal.Material, unreal.MaterialFactoryNew())


def custom(material, name, code, inputs, output, y=0):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionCustom, -400, y)
    node.set_editor_property("output_type", output)
    node.set_editor_property("description", name)
    node.set_editor_property("code", code)
    items = []
    for input_name, _ in inputs:
        item = unreal.CustomInput()
        item.set_editor_property("input_name", input_name)
        items.append(item)
    node.set_editor_property("inputs", items)
    for input_name, (source, pin) in inputs:
        if not MEL.connect_material_expressions(source, pin, node, input_name):
            raise RuntimeError("ligação falhou: %s.%s" % (name, input_name))
    return node


def scalar(material, name, value, y):
    p = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -1000, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", value)
    return p


# ---- cenário pintado
m = new_material("M_StageBackdrop")
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("two_sided", True)
uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1000, 0)
tex = MEL.create_material_expression(m, unreal.MaterialExpressionTextureObjectParameter, -1000, 100)
tex.set_editor_property("parameter_name", "Backdrop")
tex.set_editor_property("texture", backdrop)
bright = scalar(m, "Brightness", 1.0, 250)
flip_u = scalar(m, "FlipU", 0.0, 350)
flip_v = scalar(m, "FlipV", 0.0, 450)
blur = scalar(m, "Dim", 0.0, 550)
color = custom(m, "Cenario", """
// a placa do motor fica em pé com o eixo U na vertical: troca os eixos
float2 p = UV.yx;
if (FlipU > 0.5) p.x = 1.0 - p.x;
if (FlipV > 0.5) p.y = 1.0 - p.y;
float3 c = Tex.SampleLevel(TexSampler, p, 0).rgb;
// leve névoa dourada no horizonte e escurecido opcional (telas com cartas por cima)
c = lerp(c, c * 0.35, Dim);
return c * Brightness;
""", [("UV", (uv, "")), ("Tex", (tex, "")), ("Brightness", (bright, "")), ("FlipU", (flip_u, "")), ("FlipV", (flip_v, "")), ("Dim", (blur, ""))],
    unreal.CustomMaterialOutputType.CMOT_FLOAT3)
MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
library.save_loaded_asset(m)

# ---- círculo de runas
m = new_material("M_StageRing")
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("two_sided", True)
uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1000, 0)
time = MEL.create_material_expression(m, unreal.MaterialExpressionTime, -1000, 100)
tint = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -1000, 200)
tint.set_editor_property("parameter_name", "Color")
tint.set_editor_property("default_value", unreal.LinearColor(1.0, 0.75, 0.3, 1.0))
power = scalar(m, "Intensity", 3.0, 350)
flash = scalar(m, "Flash", 0.0, 450)
ring = custom(m, "Runas", """
float2 p = (UV - 0.5) * 2.0;
float r = length(p);
float a = atan2(p.y, p.x);
float outer = exp(-pow((r - 0.86) / 0.025, 2.0));
float inner = exp(-pow((r - 0.70) / 0.015, 2.0)) * 0.7;
// marcas de runas girando entre os dois aros
float spin = a + T * 0.25;
float runes = step(0.55, frac(spin * 24.0 / 6.2832)) * step(0.73, r) * step(r, 0.83);
runes *= 0.35 + 0.65 * step(0.5, frac(spin * 6.0 / 6.2832 + 0.25));
// brilho que respira por dentro
float glow = saturate(1.0 - r) * (0.18 + 0.08 * sin(T * 1.7)) * step(r, 1.0);
float wave = exp(-pow((r - frac(T * 0.35) * 0.95) / 0.04, 2.0)) * 0.35;
float m = (outer + inner + runes * 0.6 + glow + wave) * step(r, 1.0);
return C.rgb * m * (I + F * 12.0) + float3(1, 1, 1) * F * outer * 4.0;
""", [("UV", (uv, "")), ("T", (time, "")), ("C", (tint, "")), ("I", (power, "")), ("F", (flash, ""))],
    unreal.CustomMaterialOutputType.CMOT_FLOAT3)
MEL.connect_material_property(ring, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
library.save_loaded_asset(m)
lines.append("M_StageBackdrop M_StageRing")

with open(os.path.join(root, "Saved", "Logs", "CreateFrontEndArt.txt"), "w") as out:
    out.write("VORTEX_FRONTEND_ART " + " | ".join(lines) + "\n")
