"""O kit da vila nova na Unreal (05/11) — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportVilaKit.py -unattended -nullrhi

- Data/GodotTown/VilaTex/T_Vila_<nome>.png (Tools/Town/MakeVilaTextures.py) -> /Game/Vortex/Town/Vila/Textures;
- M_VilaKit: a textura (Tex) vezes a cor de cada peça (a cor do vértice), fosco; Glow faz a textura brilhar (a brasa);
  marcado para instâncias (a vila desenha em ISM);
- MI_Vila_<nome>: um por textura;
- Data/GodotTown/VilaKit/<modelo>.fbx (Tools/Blender/BuildVilaKit.py) -> /Game/Vortex/Props/<modelo>/<modelo>/StaticMeshes/<modelo>
  (o caminho que o AVortexTown procura), com os materiais MV_<nome> trocados pelos MI_Vila_<nome>.
Relatório em Saved/Logs/ImportVilaKit.txt.
"""
import glob
import os

import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
BASE = "/Game/Vortex/Town/Vila"
lines = []


def bring_texture(path):
    name = os.path.splitext(os.path.basename(path))[0]
    t = unreal.AssetImportTask()
    t.filename = path
    t.destination_path = BASE + "/Textures"
    t.destination_name = name
    t.automated = True
    t.save = True
    t.replace_existing = True
    tools.import_asset_tasks([t])
    asset = library.load_asset(BASE + "/Textures/" + name)
    # 11/11: as do kit da Quaternius (T_Vila_Q_*, Tools/Town/MakeQuaterniusTextures.py): a original do kit, filtro suave
    # (a reimportação guardava o pixel nítido da versão em pixel art)
    if asset and name.startswith("T_Vila_Q_"):
        asset.set_editor_property("filter", unreal.TextureFilter.TF_DEFAULT)
        library.save_loaded_asset(asset)
    return asset


textures, normals = {}, {}
for p in sorted(glob.glob(os.path.join(root, "Data", "GodotTown", "VilaTex", "T_Vila_*.png"))):
    tex = bring_texture(p)
    if not tex:
        continue
    key = os.path.splitext(os.path.basename(p))[0][len("T_Vila_"):]
    if key.endswith("_N"):
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        library.save_loaded_asset(tex)
        normals[key[:-2]] = tex
    else:
        textures[key] = tex
lines.append("texturas: %d" % len(textures))

# ---- o material-mestre
path = BASE + "/M_VilaKit"
m = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset("M_VilaKit", BASE, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
m.set_editor_property("used_with_instanced_static_meshes", True)
tex = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, 0)
tex.set_editor_property("parameter_name", "Tex")
tex.set_editor_property("texture", next(iter(textures.values())))
vc = mel.create_material_expression(m, unreal.MaterialExpressionVertexColor, -800, 300)
mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -450, 100)
mel.connect_material_expressions(tex, "RGB", mul, "A")
mel.connect_material_expressions(vc, "", mul, "B")
bright = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -450, -150)
bright.set_editor_property("parameter_name", "Bright")
bright.set_editor_property("default_value", 1.0)
lit = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 0)
mel.connect_material_expressions(mul, "", lit, "A")
mel.connect_material_expressions(bright, "", lit, "B")
mel.connect_material_property(lit, "", unreal.MaterialProperty.MP_BASE_COLOR)
# 06/11: o relevo (o normal map do Poly Haven, quando a textura tem)
nrm = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, 650)
nrm.set_editor_property("parameter_name", "Nrm")
nrm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
nrm.set_editor_property("texture", normals.get("Plano"))      # T_Vila_Plano_N: o relevo plano (as sem normal map)
mel.connect_material_property(nrm, "RGB", unreal.MaterialProperty.MP_NORMAL)
rough = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -450, 300)
rough.set_editor_property("parameter_name", "Rough")
rough.set_editor_property("default_value", .85)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
glow = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -450, 450)
glow.set_editor_property("parameter_name", "Glow")
glow.set_editor_property("default_value", 0.0)
emit = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, 450)
mel.connect_material_expressions(mul, "", emit, "A")
mel.connect_material_expressions(glow, "", emit, "B")
mel.connect_material_property(emit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(m)
library.save_loaded_asset(m)

# ---- um material por textura
SETTINGS = {"Vidro": {"Rough": .2}, "Ferro": {"Rough": .5}, "Brasa": {"Glow": 6.0}, "Reboco": {"Bright": 1.75},
            "PedraBruta": {"Bright": 1.6}, "PedraRua": {"Bright": 1.15}, "PedraClara": {"Bright": 1.1}, "Terra": {"Bright": 1.1}}
instances = {}
for name, t in textures.items():
    mi_path = BASE + "/MI_Vila_" + name
    mi = library.load_asset(mi_path) if library.does_asset_exist(mi_path) else tools.create_asset("MI_Vila_" + name, BASE, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", m)
    mel.set_material_instance_texture_parameter_value(mi, "Tex", t)
    if name in normals:
        mel.set_material_instance_texture_parameter_value(mi, "Nrm", normals[name])
    for k, v in SETTINGS.get(name, {}).items():
        mel.set_material_instance_scalar_parameter_value(mi, k, v)
    library.save_loaded_asset(mi)
    instances[name] = mi

# ---- 07/11: o vidro de verdade (o usuário: "os vidros das casas serem vidros mesmo"): translúcido, liso, com reflexo;
# o interior das casas aparece por ele
gpath = BASE + "/M_VilaVidro"
g = library.load_asset(gpath) if library.does_asset_exist(gpath) else tools.create_asset("M_VilaVidro", BASE, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(g)
g.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
g.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
g.set_editor_property("used_with_instanced_static_meshes", True)
g.set_editor_property("two_sided", True)
gc = mel.create_material_expression(g, unreal.MaterialExpressionConstant3Vector, -500, 0)
gc.set_editor_property("constant", unreal.LinearColor(.32, .42, .46, 1))
mel.connect_material_property(gc, "", unreal.MaterialProperty.MP_BASE_COLOR)
for prop, value, y in ((unreal.MaterialProperty.MP_OPACITY, .28, 150), (unreal.MaterialProperty.MP_ROUGHNESS, .04, 250),
                       (unreal.MaterialProperty.MP_SPECULAR, 1.0, 350), (unreal.MaterialProperty.MP_METALLIC, .0, 450)):
    c = mel.create_material_expression(g, unreal.MaterialExpressionConstant, -500, y)
    c.set_editor_property("r", value)
    mel.connect_material_property(c, "", prop)
mel.recompile_material(g)
library.save_loaded_asset(g)
instances["Vidro"] = g
# a água do poço: a mesma do lago e da fonte
water = library.load_asset("/Game/Vortex/Materials/M_VortexWater")
if water:
    # o poço é desenhado em instâncias (ISM): o material-mestre da água precisa da marca
    base = water.get_base_material() if hasattr(water, "get_base_material") else water
    if base and not base.get_editor_property("used_with_instanced_static_meshes"):
        base.set_editor_property("used_with_instanced_static_meshes", True)
        mel.recompile_material(base)
        library.save_loaded_asset(base)
    instances["Agua"] = water
lines.append("vidro e agua: %s %s" % (bool(g), bool(water)))

# ---- as malhas
ONLY = [n for n in os.environ.get("VORTEX_VILA_ONLY", "").split(",") if n]     # 07/11: só estas malhas (as texturas sempre)
for fbx in sorted(glob.glob(os.path.join(root, "Data", "GodotTown", "VilaKit", "*.fbx"))):
    name = os.path.splitext(os.path.basename(fbx))[0]
    if ONLY and name not in ONLY:
        continue
    dest = "/Game/Vortex/Props/%s/%s/StaticMeshes" % (name, name)
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("scene_name_sub_folder", False)
    pipeline.set_editor_property("asset_type_sub_folders", False)
    pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
    meshes = pipeline.get_editor_property("mesh_pipeline")
    meshes.set_editor_property("import_static_meshes", True)
    meshes.set_editor_property("import_skeletal_meshes", False)
    for prop, value in (("generate_collision", False), ("build_nanite", False)):
        try:
            meshes.set_editor_property(prop, value)
        except Exception:
            pass
    common = pipeline.get_editor_property("common_meshes_properties")
    try:
        common.set_editor_property("vertex_color_import_option", unreal.InterchangeVertexColorImportOption.IVCIO_REPLACE)
    except Exception as error:
        lines.append("aviso: cor do vértice (%s)" % str(error)[:80])
    try:
        common.set_editor_property("recompute_normals", False)
    except Exception:
        pass
    override = unreal.InterchangePipelineStackOverride()
    override.add_pipeline(pipeline)
    task = unreal.AssetImportTask()
    task.filename = fbx
    task.destination_path = dest
    task.destination_name = name
    task.automated = True
    task.save = False
    task.replace_existing = True
    task.options = override
    tools.import_asset_tasks([task])
    mesh = library.load_asset(dest + "/" + name)
    if not isinstance(mesh, unreal.StaticMesh):
        found = [p for p in library.list_assets(dest, recursive=True) if isinstance(library.load_asset(p), unreal.StaticMesh)]
        lines.append("FALHOU %s: %s" % (name, found))
        continue
    # 06/11: set_material (trocar a lista "static_materials" e devolver não gravava: ficava o WorldGridMaterial)
    slots = mesh.get_editor_property("static_materials")
    named = []
    for index, slot in enumerate(slots):
        key = str(slot.get_editor_property("material_slot_name")).split(".")[0]
        key = key[3:] if key.startswith("MV_") else key
        if key in instances:
            mesh.set_material(index, instances[key])
        named.append(key)
    library.save_loaded_asset(mesh)
    lines.append("%s: %d materiais (%s)" % (name, len(slots), ",".join(named)))

with open(os.path.join(root, "Saved", "Logs", "ImportVilaKit.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
unreal.log_warning("VORTEX_VILA_KIT " + " | ".join(lines[:4]) + " ... %d linhas" % len(lines))

# ---- 07/11: as placas (Tools/Town/MakeSigns.py): Data/GodotTown/Placas/T_Placa_<SLUG>.png -> BASE/Placas, e o M_VilaPlaca
# (a imagem da placa, fosca; o AVortexTown põe uma por placa num cubo achatado)
placas = 0
for p in sorted(glob.glob(os.path.join(root, "Data", "GodotTown", "Placas", "T_Placa_*.png"))):
    name = os.path.splitext(os.path.basename(p))[0]
    t = unreal.AssetImportTask()
    t.filename = p
    t.destination_path = BASE + "/Placas"
    t.destination_name = name
    t.automated = True
    t.save = True
    t.replace_existing = True
    tools.import_asset_tasks([t])
    tex = library.load_asset(BASE + "/Placas/" + name)
    if tex:
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SHARPEN4)
        library.save_loaded_asset(tex)
        placas += 1
ppath = BASE + "/M_VilaPlaca"
pm = library.load_asset(ppath) if library.does_asset_exist(ppath) else tools.create_asset("M_VilaPlaca", BASE, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(pm)
ptex = mel.create_material_expression(pm, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
ptex.set_editor_property("parameter_name", "Tex")
first = library.load_asset(BASE + "/Placas/T_Placa_PADARIA")
if first:
    ptex.set_editor_property("texture", first)
mel.connect_material_property(ptex, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
pr = mel.create_material_expression(pm, unreal.MaterialExpressionConstant, -600, 300)
pr.set_editor_property("r", .75)
mel.connect_material_property(pr, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(pm)
library.save_loaded_asset(pm)
unreal.log_warning("VORTEX_VILA_PLACAS %d placas" % placas)
