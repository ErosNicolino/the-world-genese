"""Importa o kit "Cartoon" (Data/UI/KitCartoon, de Tools/MakeUiKitCartoon.py) para /Game/Vortex/UI/KitCartoon e os
sons cartoon da interface (Data/UI/SonsCartoon, de Tools/MakeUiSoundsCartoon.py) para /Game/Vortex/Audio/UI, na
categoria Interface. Texturas sem mipmaps e sem compressão com perda, com filtro suave (as bordas são lisas).
Também cria M_UiDoll (a vitrine do herói, com o fundo transparente). Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi
"""
import os
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary


def import_file(path, dest, name):
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = dest
    task.destination_name = name
    task.automated = True
    task.save = True
    task.replace_existing = True
    tools.import_asset_tasks([task])
    return library.load_asset(f"{dest}/{name}")


textures = 0
src = os.path.join(ROOT, "Data", "UI", "KitCartoon")
for file in sorted(os.listdir(src)):
    if not file.endswith(".png"):
        continue
    tex = import_file(os.path.join(src, file), "/Game/Vortex/UI/KitCartoon", file[:-4])
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    # algumas peças vinham marcadas como não-sRGB (o importador adivinha) e apareciam desbotadas no jogo
    tex.set_editor_property("srgb", True)
    library.save_loaded_asset(tex)
    textures += 1

sounds = 0
interface = unreal.load_asset("/Game/Vortex/Audio/SC_Interface")
src = os.path.join(ROOT, "Data", "UI", "SonsCartoon")
for file in sorted(os.listdir(src)):
    if not file.endswith(".wav"):
        continue
    wave = import_file(os.path.join(src, file), "/Game/Vortex/Audio/UI", file[:-4])
    if interface:
        wave.set_editor_property("sound_class_object", interface)
    library.save_loaded_asset(wave)
    sounds += 1

# a vitrine do herói: a câmera grava a cor sem luz (RGB) e a profundidade (A); onde não há nada a profundidade é
# enorme, e ali fica transparente (o herói aparece sobre a placa, sem o fundo preto)
lib = unreal.MaterialEditingLibrary
full = "/Game/Vortex/UI/KitCartoon/M_UiDoll"
if library.does_asset_exist(full):
    library.delete_asset(full)
mat = tools.create_asset("M_UiDoll", "/Game/Vortex/UI/KitCartoon", unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
tex = lib.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
tex.set_editor_property("parameter_name", "Doll")
tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
black = unreal.load_asset("/Engine/EngineResources/Black.Black")
if black:
    tex.set_editor_property("texture", black)
vertex = lib.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -600, 300)
mul = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 0)
lib.connect_material_expressions(tex, "RGB", mul, "A")
lib.connect_material_expressions(vertex, "RGB", mul, "B")
# 07/11: o modelo sem luz saía escuro na vitrine: um ganho de brilho (parâmetro "Bright")
bright = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -450, 150)
bright.set_editor_property("parameter_name", "Bright")
bright.set_editor_property("default_value", 1.55)
lit = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 0)
lib.connect_material_expressions(mul, "", lit, "A")
lib.connect_material_expressions(bright, "", lit, "B")
lib.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
# perto (menos de 50 m): opaco; longe/vazio: transparente
near = lib.create_material_expression(mat, unreal.MaterialExpressionCustom, -300, 250)
near.set_editor_property("code", "return D < 5000 ? 1 : 0;")
near.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
ci = unreal.CustomInput()
ci.set_editor_property("input_name", "D")
near.set_editor_property("inputs", [ci])
lib.connect_material_expressions(tex, "A", near, "D")
alpha = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -100, 250)
lib.connect_material_expressions(near, "", alpha, "A")
lib.connect_material_expressions(vertex, "A", alpha, "B")
lib.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)
lib.recompile_material(mat)
library.save_asset(full)
unreal.log_warning(f"VORTEX_UIKIT_CARTOON {textures} peças, {sounds} sons, M_UiDoll ok")
