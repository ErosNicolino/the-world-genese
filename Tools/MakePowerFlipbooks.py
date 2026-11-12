"""As texturas animadas dos poderes na Unreal (07/11) — editor fechado, depois do Tools/VFX/MakeFlipbooks.py.
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/MakePowerFlipbooks.py -unattended -nullrhi

- Data/VFX/Flipbooks/T_FB_<nome>.png -> /Game/Vortex/VFX/Flipbooks/T_FB_<nome> (a folha de 4 x 4 quadros);
- M_PowerFlipbook: translúcido, sem luz, dos dois lados, para malhas em lote: o dado do lote (PerInstanceCustomData 0 = o
  andamento de 0 a 1) escolhe o quadro e apaga no fim; "Atlas" = a folha, "Glow" = o brilho (AVortexPowerFx::TickSprites).
"""
import glob
import os

import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
DEST = "/Game/Vortex/VFX/Flipbooks"
first = None
for png in sorted(glob.glob(os.path.join(root, "Data", "VFX", "Flipbooks", "T_FB_*.png"))):
    name = os.path.splitext(os.path.basename(png))[0]
    t = unreal.AssetImportTask()
    t.filename = png
    t.destination_path = DEST
    t.destination_name = name
    t.automated = True
    t.save = False
    t.replace_existing = True
    tools.import_asset_tasks([t])
    tex = library.load_asset(DEST + "/" + name)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
    tex.set_editor_property("srgb", True)
    library.save_loaded_asset(tex)
    first = first or tex

path = "/Game/Vortex/VFX/M_PowerFlipbook"
if library.does_asset_exist(path):
    library.delete_asset(path)
m = tools.create_asset("M_PowerFlipbook", "/Game/Vortex/VFX", unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("two_sided", True)
for flag in ("used_with_instanced_static_meshes",):
    try:
        m.set_editor_property(flag, True)
    except Exception:
        pass
progress = MEL.create_material_expression(m, unreal.MaterialExpressionPerInstanceCustomData, -1100, 0)
progress.set_editor_property("data_index", 0)
uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1100, 200)
cell = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -800, 100)
cell.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT2)
cell.set_editor_property("code", "float f = min(floor(P * 16.0), 15.0); float2 c = float2(fmod(f, 4.0), floor(f / 4.0)); return (UV + c) / 4.0;")
ins = []
for n in ("P", "UV"):
    i = unreal.CustomInput()
    i.set_editor_property("input_name", n)
    ins.append(i)
cell.set_editor_property("inputs", ins)
MEL.connect_material_expressions(progress, "", cell, "P")
MEL.connect_material_expressions(uv, "", cell, "UV")
atlas = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -500, 100)
atlas.set_editor_property("parameter_name", "Atlas")
if first:
    atlas.set_editor_property("texture", first)
MEL.connect_material_expressions(cell, "", atlas, "UVs")
glow = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -500, 350)
glow.set_editor_property("parameter_name", "Glow")
glow.set_editor_property("default_value", 1.6)
color = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, 100)
MEL.connect_material_expressions(atlas, "RGB", color, "A")
MEL.connect_material_expressions(glow, "", color, "B")
MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
fade = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -500, 500)
fade.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
fade.set_editor_property("code", "return A * (1.0 - smoothstep(0.8, 1.0, P));")
ins = []
for n in ("A", "P"):
    i = unreal.CustomInput()
    i.set_editor_property("input_name", n)
    ins.append(i)
fade.set_editor_property("inputs", ins)
MEL.connect_material_expressions(atlas, "A", fade, "A")
MEL.connect_material_expressions(progress, "", fade, "P")
MEL.connect_material_property(fade, "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(m)
library.save_loaded_asset(m)
unreal.log_warning("VORTEX_POWER_FLIPBOOKS ok")
