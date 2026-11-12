"""Material das baforadas de fumaça (M_SmokePuff): esfera translúcida, sem luz própria, mais densa no
meio e transparente na borda (fresnel invertido), cor e opacidade por parâmetro (Tint, Fade).
Usada pela coluna do vulcão (AVortexSmokeColumn) e pela fumaça da lava. Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi
"""
import unreal

library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
FOLDER = "/Game/Vortex/Sky"
path = f"{FOLDER}/M_SmokePuff"
m = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset("M_SmokePuff", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("two_sided", False)


def node(kind, x, y, **props):
    e = mel.create_material_expression(m, kind, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


tint = node(unreal.MaterialExpressionVectorParameter, -600, 0, parameter_name="Tint", default_value=unreal.LinearColor(.32, .3, .3, 1))
mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
fres = node(unreal.MaterialExpressionFresnel, -800, 200, exponent=1.6, base_reflect_fraction=0.0)
inv = node(unreal.MaterialExpressionOneMinus, -600, 200)
mel.connect_material_expressions(fres, "", inv, "")
fade = node(unreal.MaterialExpressionScalarParameter, -600, 320, parameter_name="Fade", default_value=.5)
op = node(unreal.MaterialExpressionMultiply, -400, 240)
mel.connect_material_expressions(inv, "", op, "A")
mel.connect_material_expressions(fade, "", op, "B")
mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(m)
library.save_loaded_asset(m)
unreal.log_warning("VORTEX_SMOKE material ok")
