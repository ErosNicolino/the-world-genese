"""07/11: a vitrine do herói (M_UiDoll, janela I e mochila) saía escura: a câmera grava só a cor do modelo, sem luz.
Põe um ganho de brilho (o parâmetro "Bright", 1,55) entre a imagem e a cor, sem refazer o kit todo (o
ImportUiKitCartoon.py já cria o material assim). Editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/MakeUiDollBright.py -unattended -nullrhi
"""
import unreal

lib = unreal.MaterialEditingLibrary
full = "/Game/Vortex/UI/KitCartoon/M_UiDoll"
mat = unreal.load_asset(full)
src = lib.get_material_property_input_node(mat, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
if isinstance(src, unreal.MaterialExpressionMultiply) and any(
        isinstance(e, unreal.MaterialExpressionScalarParameter) and e.get_editor_property("parameter_name") == "Bright"
        for e in lib.get_inputs_for_material_expression(mat, src)):
    unreal.log_warning("VORTEX_UIDOLL já tem o Bright")
else:
    bright = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -450, 150)
    bright.set_editor_property("parameter_name", "Bright")
    bright.set_editor_property("default_value", 1.55)
    lit = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 0)
    lib.connect_material_expressions(src, "", lit, "A")
    lib.connect_material_expressions(bright, "", lit, "B")
    lib.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(full)
    unreal.log_warning("VORTEX_UIDOLL Bright 1.55 ok")
