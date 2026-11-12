"""O herói molhado (03/11) — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/AddWetLook.py -unattended -nullrhi

Acha os materiais-mestre do herói (os pais do MI_Hero_humano, das roupas MI_Roupa_*, dos cabelos M_BlockHair*) e põe
neles o "molhado":
- abaixo da linha da água (dado primitivo 30: a altura no mundo, cm) a cor escurece (até 55%), a rugosidade cai
  (brilha) e o especular sobe;
- dado primitivo 31: o quanto ainda está molhado (1 ao sair da água, seca em ~45 s; UVortexWetness);
- a borda da linha é um pouco irregular (um ruído pela posição), como pano que puxa a água.
Não refaz o material: só pega o que já está ligado na cor e na rugosidade e põe a mistura no meio. Rodar de novo não
duplica (o nó "VortexWet" marca os já feitos).
"""
import unreal

library = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
lines = []

WET = """
float edge = WetZ - Z + (frac(sin(dot(floor(P.xy / 6.0), float2(12.9898, 78.233))) * 43758.5453) - 0.5) * 10.0;
return saturate(edge / 12.0 + 0.5) * Wetness;
"""


def masters():
    found = {}
    paths = ["/Game/Vortex/Characters/Heroes/humano/MI_Hero_humano"]
    for folder in ("/Game/Vortex/Characters/Heroes",):
        if library.does_directory_exist(folder):
            paths += [p for p in library.list_assets(folder, recursive=True)]
    for p in paths:
        data = library.find_asset_data(p)
        kind = str(data.asset_class_path.asset_name) if data else ""
        if kind not in ("MaterialInstanceConstant", "Material"):
            continue
        asset = library.load_asset(p)
        base = asset
        while isinstance(base, unreal.MaterialInstance):
            base = base.get_editor_property("parent")
        if isinstance(base, unreal.Material):
            found[base.get_path_name()] = base
    return list(found.values())


def scalar_cpd(mat, name, index, x, y):
    e = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", 0.0)
    e.set_editor_property("use_custom_primitive_data", True)
    e.set_editor_property("primitive_data_index", index)
    return e


def add_wet(mat):
    base_in = mel.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
    if base_in is None:
        return "sem cor"
    if isinstance(base_in, unreal.MaterialExpressionLinearInterpolate) and base_in.get_editor_property("desc") == "VortexWet":
        return "já tinha"
    base_out = mel.get_material_property_input_node_output_name(mat, unreal.MaterialProperty.MP_BASE_COLOR)
    rough_in = mel.get_material_property_input_node(mat, unreal.MaterialProperty.MP_ROUGHNESS)
    rough_out = mel.get_material_property_input_node_output_name(mat, unreal.MaterialProperty.MP_ROUGHNESS) if rough_in else ""
    x, y = -400, -600
    world = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, x - 700, y)
    wz = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, x - 500, y)
    wz.set_editor_property("b", True)
    mel.connect_material_expressions(world, "", wz, "")
    line = scalar_cpd(mat, "WetLine", 30, x - 700, y + 100)
    amount = scalar_cpd(mat, "Wetness", 31, x - 700, y + 180)
    custom = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, x - 300, y + 60)
    custom.set_editor_property("code", WET)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    custom.set_editor_property("description", "VortexWetMask")
    ins = []
    for n in ("Z", "WetZ", "Wetness", "P"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        ins.append(ci)
    custom.set_editor_property("inputs", ins)
    mel.connect_material_expressions(wz, "", custom, "Z")
    mel.connect_material_expressions(line, "", custom, "WetZ")
    mel.connect_material_expressions(amount, "", custom, "Wetness")
    mel.connect_material_expressions(world, "", custom, "P")
    # cor: base x (1 - 0,45 x molhado)
    darker = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, x - 100, y - 60)
    mel.connect_material_expressions(base_in, base_out, darker, "A")
    k = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, x - 300, y - 120)
    k.set_editor_property("r", .55)
    mel.connect_material_expressions(k, "", darker, "B")
    color = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, x + 100, y - 40)
    color.set_editor_property("desc", "VortexWet")
    mel.connect_material_expressions(base_in, base_out, color, "A")
    mel.connect_material_expressions(darker, "", color, "B")
    mel.connect_material_expressions(custom, "", color, "Alpha")
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    # rugosidade: a de antes (ou 0,8) até 0,12 molhado
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, x + 100, y + 160)
    if rough_in:
        mel.connect_material_expressions(rough_in, rough_out, rough, "A")
    else:
        dry = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, x - 100, y + 140)
        dry.set_editor_property("r", .8)
        mel.connect_material_expressions(dry, "", rough, "A")
    wet = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, x - 100, y + 220)
    wet.set_editor_property("r", .12)
    mel.connect_material_expressions(wet, "", rough, "B")
    mel.connect_material_expressions(custom, "", rough, "Alpha")
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, x + 100, y + 300)
    s0 = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, x - 100, y + 300)
    s0.set_editor_property("r", .5)
    s1 = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, x - 100, y + 360)
    s1.set_editor_property("r", .85)
    mel.connect_material_expressions(s0, "", spec, "A")
    mel.connect_material_expressions(s1, "", spec, "B")
    mel.connect_material_expressions(custom, "", spec, "Alpha")
    if mel.get_material_property_input_node(mat, unreal.MaterialProperty.MP_SPECULAR) is None:
        mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(mat)
    library.save_loaded_asset(mat)
    return "ok"


for m in masters():
    try:
        lines.append(f"{m.get_path_name()}: {add_wet(m)}")
    except Exception as error:
        lines.append(f"{m.get_path_name()}: ERRO {error}")
unreal.log_warning("VORTEX_WET " + " | ".join(lines))
