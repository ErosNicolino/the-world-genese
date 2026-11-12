"""As construções novas da vila maior na Unreal (03/11) — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportTownBuildings.py -unattended -nullrhi

- Data/GodotTown/Models/<nome>.glb (Tools/Blender/BuildTownBuildings.py) -> /Game/Vortex/Props/<nome>/<nome>/StaticMeshes/<nome>,
  o mesmo caminho dos modelos do Godot (o AVortexTown acha pelo nome). A cor está na paleta de cada modelo (uma textura
  pequena, um pixel por cor; a Unreal não importa a cor dos vértices destes glb): MI_<nome>, filho do M_TownVoxel, com
  a paleta em "Atlas" (sem filtro, sem compressão e sem mipmap, para as cores não se misturarem).
- M_TownCloth: as bandeiras. O pano balança com o vento, mais embaixo (a malha pende do alto, na origem).
- M_LiquidPuddle: a poça do barril quebrado (água ou vinho), translúcida e brilhante; Color e Opacity por instância.
Relatório em Saved/Logs/ImportTownBuildings.txt.
"""
import glob
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
MAT_DIR = "/Game/Vortex/Materials"
lines = []


def material(name):
    path = f"{MAT_DIR}/{name}"
    mat = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    # a vila desenha as construções em instâncias (ISM): sem esta marca a Unreal troca o material por outro
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    return mat


def vertex_color(mat, rough=.9):
    """A cor da paleta (Atlas, por instância) e fosco."""
    color = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
    color.set_editor_property("parameter_name", "Atlas")
    color.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    color.set_editor_property("texture", library.load_asset("/Engine/EngineResources/DefaultTexture"))
    # sempre a paleta inteira (mip 0): de longe a placa de vídeo pegava uma paleta reduzida (as faces vizinhas têm UVs
    # longe um do outro) e misturava todas as cores num marrom
    color.set_editor_property("mip_value_mode", unreal.TextureMipValueMode.TMVM_MIP_LEVEL)
    color.set_editor_property("const_mip_value", 0)
    mel.connect_material_property(color, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -600, 200)
    r.set_editor_property("r", rough)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    return color


voxel = material("M_TownVoxel")
vertex_color(voxel)
mel.recompile_material(voxel)
library.save_loaded_asset(voxel)

# o pano: o deslocamento é no espaço da peça (Y: para fora do pano), cresce para baixo (Z local negativo)
SWAY = """
float w = saturate(-P.z / 250.0);
float wave = sin(T * 2.3 + P.z * 0.025 + O.x * 0.002) * 0.7 + sin(T * 3.7 + P.x * 0.04 + O.y * 0.003) * 0.3;
float gust = 0.6 + 0.4 * sin(T * 0.5 + O.x * 0.001);
return float3(sin(T * 1.7 + P.z * 0.02) * 4.0 * w, wave * 22.0 * w * w * gust, abs(wave) * 3.0 * w);
"""
cloth = material("M_TownCloth")
cloth.set_editor_property("two_sided", True)
vertex_color(cloth, .95)
pos = mel.create_material_expression(cloth, unreal.MaterialExpressionLocalPosition, -900, 400)
obj = mel.create_material_expression(cloth, unreal.MaterialExpressionObjectPositionWS, -900, 500)
time = mel.create_material_expression(cloth, unreal.MaterialExpressionTime, -900, 600)
sway = mel.create_material_expression(cloth, unreal.MaterialExpressionCustom, -600, 450)
sway.set_editor_property("code", SWAY)
sway.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in ("P", "O", "T"):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    ins.append(ci)
sway.set_editor_property("inputs", ins)
mel.connect_material_expressions(pos, "", sway, "P")
mel.connect_material_expressions(obj, "", sway, "O")
mel.connect_material_expressions(time, "", sway, "T")
to_world = mel.create_material_expression(cloth, unreal.MaterialExpressionTransform, -350, 450)
to_world.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
to_world.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
mel.connect_material_expressions(sway, "", to_world, "")
mel.connect_material_property(to_world, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
mel.recompile_material(cloth)
library.save_loaded_asset(cloth)

# a poça: translúcida, bem lisa (brilha), a cor e a opacidade por instância, a borda mais fraca (UV do cilindro)
puddle = material("M_LiquidPuddle")
puddle.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
puddle.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
col = mel.create_material_expression(puddle, unreal.MaterialExpressionVectorParameter, -600, 0)
col.set_editor_property("parameter_name", "Color")
col.set_editor_property("default_value", unreal.LinearColor(.05, .2, .3, 1))
mel.connect_material_property(col, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
op = mel.create_material_expression(puddle, unreal.MaterialExpressionScalarParameter, -600, 200)
op.set_editor_property("parameter_name", "Opacity")
op.set_editor_property("default_value", .7)
mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
r = mel.create_material_expression(puddle, unreal.MaterialExpressionConstant, -600, 300)
r.set_editor_property("r", .12)
mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
spec = mel.create_material_expression(puddle, unreal.MaterialExpressionConstant, -600, 380)
spec.set_editor_property("r", 1.0)
mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
mel.recompile_material(puddle)
library.save_loaded_asset(puddle)

# ---- os modelos
for path in sorted(glob.glob(os.path.join(root, "Data", "GodotTown", "Models", "*.glb"))):
    name = os.path.splitext(os.path.basename(path))[0]
    dest = f"/Game/Vortex/Props/{name}"
    if library.does_directory_exist(dest):
        library.delete_directory(dest)
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = dest
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    mesh_path = f"{dest}/{name}/StaticMeshes/{name}"
    mesh = library.load_asset(mesh_path)
    if not mesh:
        lines.append(f"FALHOU {name}")
        continue
    target = cloth if name.startswith("town_banner_") and name != "town_banner_pole" else voxel
    texture = None
    for p in library.list_assets(dest, recursive=True):
        asset = library.load_asset(p)
        if isinstance(asset, unreal.Texture2D):
            texture = asset
    if texture:
        texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("srgb", True)
        library.save_loaded_asset(texture)
    mi_path = f"{dest}/MI_{name}"
    mi = tools.create_asset(f"MI_{name}", dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, target)
    if texture:
        mel.set_material_instance_texture_parameter_value(mi, "Atlas", texture)
    library.save_loaded_asset(mi)
    for i in range(len(mesh.get_editor_property("static_materials"))):
        mesh.set_material(i, mi)
    settings = mesh.get_editor_property("nanite_settings")
    settings.set_editor_property("enabled", False)   # o Nanite também aperta os UVs da paleta
    mesh.set_editor_property("nanite_settings", settings)
    # só o LOD 0: os LODs simplificados juntam vértices e misturam os UVs da paleta (as cores saíam trocadas de longe)
    # (o subsistema de malhas não existe no commandlet: a biblioteca antiga faz o mesmo)
    lods = mesh.get_num_lods()
    if lods > 1:
        unreal.EditorStaticMeshLibrary.remove_lods(mesh)
    library.save_loaded_asset(mesh)
    # as sobras do glTF (o material que ele cria)
    for p in library.list_assets(dest, recursive=True):
        kind = str(library.find_asset_data(p).asset_class_path.asset_name)
        if kind in ("MaterialInstanceConstant", "Material") and not library.find_package_referencers_for_asset(p) and not p.split("/")[-1].startswith("MI_town"):
            library.delete_asset(p)
    box = mesh.get_bounding_box()
    lines.append(f"{name}: lods={lods}->{mesh.get_num_lods()} paleta={texture.get_name() if texture else 'NENHUMA'} {box.min} .. {box.max}")

with open(os.path.join(root, "Saved", "Logs", "ImportTownBuildings.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
unreal.log_warning("VORTEX_TOWN_IMPORT " + " | ".join(lines))
