"""Materiais da sensação de acerto (Combat/VortexHitFx) — rode com o editor fechado:

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/CreateHitMaterials.py -unattended

- M_HitFlash: brilho por cima do corpo acertado (material "overlay"), aditivo e sem luz, Color x Glow.
- M_SolidColor: cor sólida com luz (parâmetro Color), para os respingos de sangue/palha em lote.
"""
import os
import unreal

MEL = unreal.MaterialEditingLibrary
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = "/Game/Vortex/Combat"


def fresh(name):
    path = FOLDER + "/" + name
    if library.does_asset_exist(path):
        library.delete_asset(path)
    return tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())


flash = fresh("M_HitFlash")
flash.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
flash.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
for flag in ("used_with_skeletal_mesh", "used_with_instanced_static_meshes", "used_with_nanite"):
    try:
        flash.set_editor_property(flag, True)
    except Exception:
        pass
c = MEL.create_material_expression(flash, unreal.MaterialExpressionVectorParameter, -500, 0)
c.set_editor_property("parameter_name", "Color")
c.set_editor_property("default_value", unreal.LinearColor(1.0, .55, .45, 1))
g = MEL.create_material_expression(flash, unreal.MaterialExpressionScalarParameter, -500, 200)
g.set_editor_property("parameter_name", "Glow")
g.set_editor_property("default_value", 1.5)
m = MEL.create_material_expression(flash, unreal.MaterialExpressionMultiply, -250, 100)
MEL.connect_material_expressions(c, "", m, "A")
MEL.connect_material_expressions(g, "", m, "B")
MEL.connect_material_property(m, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(flash)
library.save_loaded_asset(flash)

solid = fresh("M_SolidColor")
for flag in ("used_with_instanced_static_meshes", "used_with_nanite"):
    try:
        solid.set_editor_property(flag, True)
    except Exception:
        pass
c = MEL.create_material_expression(solid, unreal.MaterialExpressionVectorParameter, -500, 0)
c.set_editor_property("parameter_name", "Color")
c.set_editor_property("default_value", unreal.LinearColor(.6, .05, .05, 1))
MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
r = MEL.create_material_expression(solid, unreal.MaterialExpressionConstant, -500, 200)
r.set_editor_property("r", .45)
MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(solid)
library.save_loaded_asset(solid)

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
with open(os.path.join(root, "Saved", "Logs", "CreateHitMaterials.txt"), "w") as out:
    out.write("VORTEX_HIT_MATERIALS ok\n")
