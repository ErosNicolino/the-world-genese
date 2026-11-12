"""As peças dos lugares marcantes e das raízes na Unreal (07/11) — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportMarvels.py -unattended -nullrhi

- Data/World/Marvels/SM_<peça>.glb (Tools/Blender/MakeMarvels.py) -> /Game/Vortex/World/Marvels/SM_<peça>, com a cor do
  vértice e a própria malha como colisão (anda-se em cima da ponte de raiz, do anel, do mirante);
- os três espaços de material pelo nome: MB_block -> M_MarvelBlock (a cor do vértice, fosco), MB_glow -> M_MarvelGlow
  (a cor acesa: cristais e seiva), MB_water -> M_MarvelWater (translúcida, faixas claras descendo: a cachoeira).
Relatório em Saved/Logs/ImportMarvels.txt.
"""
import glob
import os

import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
BASE = "/Game/Vortex/World/Marvels"
lines = []


def material(name):
    path = BASE + "/" + name
    m = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset(name, BASE, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    return m


def const(m, value, prop, y):
    c = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -400, y)
    c.set_editor_property("r", value)
    mel.connect_material_property(c, "", prop)


# a pedra, a madeira, a casca: a cor do vértice
block = material("M_MarvelBlock")
vc = mel.create_material_expression(block, unreal.MaterialExpressionVertexColor, -600, 0)
mel.connect_material_property(vc, "", unreal.MaterialProperty.MP_BASE_COLOR)
const(block, .88, unreal.MaterialProperty.MP_ROUGHNESS, 200)
mel.recompile_material(block)
library.save_loaded_asset(block)

# os cristais e a seiva: a cor acesa
glow = material("M_MarvelGlow")
vc = mel.create_material_expression(glow, unreal.MaterialExpressionVertexColor, -800, 0)
mel.connect_material_property(vc, "", unreal.MaterialProperty.MP_BASE_COLOR)
gp = mel.create_material_expression(glow, unreal.MaterialExpressionScalarParameter, -800, 200)
gp.set_editor_property("parameter_name", "Glow")
gp.set_editor_property("default_value", 2.0)
mul = mel.create_material_expression(glow, unreal.MaterialExpressionMultiply, -500, 100)
mel.connect_material_expressions(vc, "", mul, "A")
mel.connect_material_expressions(gp, "", mul, "B")
mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
const(glow, .3, unreal.MaterialProperty.MP_ROUGHNESS, 300)
mel.recompile_material(glow)
library.save_loaded_asset(glow)

# a cachoeira: translúcida, faixas claras que descem (a altura no mundo + o tempo)
water = material("M_MarvelWater")
water.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
water.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
water.set_editor_property("two_sided", True)
vc = mel.create_material_expression(water, unreal.MaterialExpressionVertexColor, -900, 0)
wp = mel.create_material_expression(water, unreal.MaterialExpressionWorldPosition, -900, 200)
tm = mel.create_material_expression(water, unreal.MaterialExpressionTime, -900, 350)
flow = mel.create_material_expression(water, unreal.MaterialExpressionCustom, -550, 150)
flow.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
flow.set_editor_property("code",
                         "float b = frac((P.z + T * 900.0) / 260.0 + sin(P.x * 0.01 + P.y * 0.013) * 0.4);"
                         "float s = smoothstep(0.75, 0.95, b);"
                         "return C.rgb * (0.7 + 0.7 * s);")
ins = []
for n in ("C", "P", "T"):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    ins.append(ci)
flow.set_editor_property("inputs", ins)
mel.connect_material_expressions(vc, "", flow, "C")
mel.connect_material_expressions(wp, "", flow, "P")
mel.connect_material_expressions(tm, "", flow, "T")
mel.connect_material_property(flow, "", unreal.MaterialProperty.MP_BASE_COLOR)
em = mel.create_material_expression(water, unreal.MaterialExpressionMultiply, -300, 300)
mel.connect_material_expressions(flow, "", em, "A")
ec = mel.create_material_expression(water, unreal.MaterialExpressionConstant, -500, 400)
ec.set_editor_property("r", 1.1)
mel.connect_material_expressions(ec, "", em, "B")
mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
const(water, .93, unreal.MaterialProperty.MP_OPACITY, 500)   # 07/11: com .78 de longe virava névoa
const(water, .1, unreal.MaterialProperty.MP_ROUGHNESS, 600)
mel.recompile_material(water)
library.save_loaded_asset(water)

MATS = {"MB_block": block, "MB_glow": glow, "MB_water": water}

for glb in sorted(glob.glob(os.path.join(root, "Data", "World", "Marvels", "SM_*.glb"))):
    name = os.path.splitext(os.path.basename(glb))[0]
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("scene_name_sub_folder", False)
    pipeline.set_editor_property("asset_type_sub_folders", False)
    pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
    meshes = pipeline.get_editor_property("mesh_pipeline")
    meshes.set_editor_property("import_static_meshes", True)
    meshes.set_editor_property("import_skeletal_meshes", False)
    for prop, value in (("generate_collision", False), ("build_nanite", False), ("combine_static_meshes", True)):
        try:
            meshes.set_editor_property(prop, value)
        except Exception:
            pass
    common = pipeline.get_editor_property("common_meshes_properties")
    for prop, value in (("vertex_color_import_option", unreal.InterchangeVertexColorImportOption.IVCIO_REPLACE), ("recompute_normals", False)):
        try:
            common.set_editor_property(prop, value)
        except Exception as error:
            lines.append("aviso %s: %s" % (prop, str(error)[:80]))
    override = unreal.InterchangePipelineStackOverride()
    override.add_pipeline(pipeline)
    task = unreal.AssetImportTask()
    task.filename = glb
    task.destination_path = BASE
    task.destination_name = name
    task.automated = True
    task.save = False
    task.replace_existing = True
    task.options = override
    tools.import_asset_tasks([task])
    mesh = library.load_asset(BASE + "/" + name)
    if not isinstance(mesh, unreal.StaticMesh):
        lines.append("FALHOU " + name)
        continue
    slots = mesh.get_editor_property("static_materials")
    named = []
    for index, slot in enumerate(slots):
        key = str(slot.get_editor_property("material_slot_name"))
        target = next((m for k, m in MATS.items() if key.startswith(k)), block)
        mesh.set_material(index, target)
        named.append(key)
    # a própria malha é a colisão (a ponte, o anel e o mirante são de andar em cima)
    body = mesh.get_editor_property("body_setup")
    if body:
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    library.save_loaded_asset(mesh)
    box = mesh.get_bounding_box()
    lines.append("%s: %s  %s .. %s" % (name, ",".join(named), box.min, box.max))

# as sobras do glTF (materiais que ele cria)
for p in library.list_assets(BASE, recursive=False):
    kind = str(library.find_asset_data(p).asset_class_path.asset_name)
    short = p.split("/")[-1].split(".")[0]
    if kind in ("Material", "MaterialInstanceConstant") and short not in ("M_MarvelBlock", "M_MarvelGlow", "M_MarvelWater"):
        library.delete_asset(p)

with open(os.path.join(root, "Saved", "Logs", "ImportMarvels.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
unreal.log_warning("VORTEX_MARVELS %d linhas" % len(lines))
