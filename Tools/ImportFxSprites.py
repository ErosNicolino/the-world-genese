"""As imagens de efeito (Kenney Particle Pack, CC0; cópias em Data/Fx/Sprites) e o material delas, editor fechado:

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportFxSprites.py -unattended -nullrhi

Viram /Game/Vortex/Combat/Sprites/T_Fx_<nome>. M_FxSprite: translúcido e sem luz (não aditivo: segura a cor de dia);
Tex (a imagem), Color (a cor, já com o brilho) e Strength (a opacidade) por instância; o AVortexMonsterFx monta.
"""
import glob
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
FOLDER = "/Game/Vortex/Combat/Sprites"

tasks = []
for path in sorted(glob.glob(os.path.join(root, "Data", "Fx", "Sprites", "*.png"))):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", FOLDER)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tasks.append(task)
tools.import_asset_tasks(tasks)
first = None
for task in tasks:
    name = os.path.splitext(os.path.basename(task.get_editor_property("filename")))[0]
    tex = library.load_asset(FOLDER + "/" + name)
    if not tex:
        unreal.log_error("VORTEX_FX_SPRITES sem " + name)
        continue
    tex.set_editor_property("srgb", True)
    tex.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
    tex.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    library.save_loaded_asset(tex)
    first = first or tex

name = "M_FxSprite"
path = "/Game/Vortex/Combat/" + name
mat = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset(name, "/Game/Vortex/Combat", unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(mat)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)
tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -900, 0)
tex.set_editor_property("parameter_name", "Tex")
tex.set_editor_property("texture", first)
color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, 300)
color.set_editor_property("parameter_name", "Color")
color.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
strength = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 500)
strength.set_editor_property("parameter_name", "Strength")
strength.set_editor_property("default_value", 1.0)
# a forma é o alfa vezes o branco da imagem (as do Kenney são brancas no transparente)
shape = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -600, 100)
mel.connect_material_expressions(tex, "A", shape, "A")
mel.connect_material_expressions(tex, "R", shape, "B")
opacity = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -400, 300)
mel.connect_material_expressions(shape, "", opacity, "A")
mel.connect_material_expressions(strength, "", opacity, "B")
mel.connect_material_property(color, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(mat)
library.save_loaded_asset(mat)
unreal.log_warning("VORTEX_FX_SPRITES ok %d" % len(tasks))
