"""As 5 armas novas (Tools/Blender/FitNewWeapons.py) na Unreal, no lugar das antigas — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportNewWeapons.py -unattended -nullrhi

- Data/VortexWeapons/Textures/Novas/T_N_<tipo>{,_normal,_rough}.png -> /Game/Vortex/Weapons/Materials/Novas;
- MI_WN_<tipo> (filha do M_WeaponTextured, o metálico médio do Meshy em metallic.json);
- W_espada_de_cinzas, W_martelo_de_bram, W_adagas_gemeas(_L), W_cajado_de_ilsa, W_arco_de_betula no mesmo caminho
  das antigas (as definições DA_<tipo> continuam apontando para elas), mais W_arco_ametista e W_flecha.
Relatório em Saved/Logs/ImportNewWeapons.txt.
"""
import json
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
MODELS = "/Game/Vortex/Weapons/Models"
MATS = "/Game/Vortex/Weapons/Materials/Novas"
textures = os.path.join(root, "Data", "VortexWeapons", "Textures", "Novas")
models = os.path.join(root, "Data", "VortexWeapons", "Models")
NAMES = ["W_espada_de_cinzas", "W_martelo_de_bram", "W_adagas_gemeas", "W_adagas_gemeas_L", "W_cajado_de_ilsa",
         "W_arco_de_betula", "W_arco_ametista", "W_flecha"]
metals = json.load(open(os.path.join(textures, "metallic.json"), encoding="utf-8"))
lines = []


def bring(path, folder, name=None):
    t = unreal.AssetImportTask()
    t.filename = path
    t.destination_path = folder
    if name:
        t.destination_name = name
    t.automated = True
    t.save = True
    t.replace_existing = True
    if path.endswith(".glb"):
        # as normais do arquivo e o UV em precisão total (ver abaixo, nas malhas)
        pipeline = unreal.InterchangeGenericAssetsPipeline()
        common = pipeline.get_editor_property("common_meshes_properties")
        common.set_editor_property("recompute_normals", False)
        common.set_editor_property("recompute_tangents", True)
        common.set_editor_property("use_full_precision_u_vs", True)
        common.set_editor_property("use_high_precision_tangent_basis", True)
        override = unreal.InterchangePipelineStackOverride()
        override.add_pipeline(pipeline)
        t.options = override
    tools.import_asset_tasks([t])


for f in sorted(os.listdir(textures)):
    if f.endswith(".png"):
        bring(os.path.join(textures, f), MATS, f[:-4])
        asset = library.load_asset(MATS + "/" + f[:-4])
        # 31/10: inteiras na memória (são poucas): por streaming, a arma recém-equipada aparecia só com a cor média
        # da textura (lisa e escura) e às vezes ficava assim quando o orçamento de texturas enchia
        asset.set_editor_property("never_stream", True)
        if f.endswith("_normal.png"):
            asset.set_editor_property("srgb", False)
            asset.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            # 31/10: o Meshy (glTF) faz o mapa de normais na convenção do OpenGL (verde para cima); a Unreal usa a do
            # DirectX. Sem inverter o verde, as saliências apareciam ao contrário e a arma parecia pedra facetada
            asset.set_editor_property("flip_green_channel", True)
        elif f.endswith("_rough.png"):
            asset.set_editor_property("srgb", False)
            asset.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        else:
            # 02/11: a cor repintada em pixel art (Tools/Blender/RestyleWeaponTexture.py): pixel nítido, sem borrar
            asset.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
            asset.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
        library.save_loaded_asset(asset)

base = library.load_asset("/Game/Vortex/Weapons/Materials/M_WeaponTextured")
instances = {}
for kind, metallic in metals.items():
    path = MATS + "/MI_WN_" + kind
    mi = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset("MI_WN_" + kind, MATS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, base)
    for param, suffix in (("Base", ""), ("Normal", "_normal"), ("RoughnessMap", "_rough")):
        tex = library.load_asset(MATS + "/T_N_" + kind + suffix) if library.does_asset_exist(MATS + "/T_N_" + kind + suffix) else None
        # 02/11: o relevo liso (a cor é pixel art; o mapa de normais do Meshy, fino e cheio de facetas, fazia cada
        # triângulo brilhar de um jeito: os "diamantes" e os pontos pretos). VORTEX_MESHY_NORMAL=1 volta ao do Meshy
        if param == "Normal" and not os.environ.get("VORTEX_MESHY_NORMAL"):
            tex = unreal.load_asset("/Engine/EngineMaterials/FlatNormal")
        if tex:
            mel.set_material_instance_texture_parameter_value(mi, param, tex)
    # como as antigas: pouco metálico (sem Lumen o reflexo escurece o aço) e um pouco mais claro
    mel.set_material_instance_scalar_parameter_value(mi, "Metallic", metallic * .3)
    # 02/11: a pixel art repintada é escura (média 0,13 linear, o aço das antigas 0,46 x 1,35): com 1,0 a lâmina saía
    # quase preta no sol da tarde. 1,8 deixa perto das armas antigas sem estourar o dourado
    mel.set_material_instance_scalar_parameter_value(mi, "Bright", 1.8)
    mel.set_material_instance_scalar_parameter_value(mi, "Glow", 0.0)
    library.save_loaded_asset(mi)
    instances["MWN_" + kind] = mi
    lines.append("MI_WN_%s metalico %.2f" % (kind, metallic))


def mesh(name):
    path = MODELS + "/" + name
    if library.does_asset_exist(path):
        asset = library.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            return asset
    for p in library.list_assets(MODELS, recursive=True):
        if p.split("/")[-1].split(".")[0] == name:
            asset = library.load_asset(p)
            if isinstance(asset, unreal.StaticMesh):
                return asset
    return None


for name in NAMES:
    bring(os.path.join(models, name + ".glb"), MODELS)
    m = mesh(name)
    if not m:
        lines.append("FALHOU " + name)
        continue
    settings = m.get_editor_property("nanite_settings")
    settings.set_editor_property("enabled", False)
    m.set_editor_property("nanite_settings", settings)
    # 31/10: as normais do arquivo (suaves, do Meshy) e o UV em precisão total. A Unreal recalculava uma normal por
    # triângulo (sem suavizar): nas milhares de faces pequenas a arma ficava facetada, escura e manchada, com cara de
    # pedra; e o UV em meia precisão deslocava as ilhas do atlas de 2048
    build = unreal.EditorStaticMeshLibrary.get_lod_build_settings(m, 0)
    build.set_editor_property("recompute_normals", False)
    build.set_editor_property("recompute_tangents", True)
    build.set_editor_property("use_full_precision_u_vs", True)
    build.set_editor_property("use_high_precision_tangent_basis", True)
    unreal.EditorStaticMeshLibrary.set_lod_build_settings(m, 0, build)
    slots = m.get_editor_property("static_materials")
    # pelo nome da arma (a reimportação mantinha o espaço antigo, MI_W_aco_escuro, e a textura nova não aparecia)
    kind = {"W_flecha": "arco_flecha", "W_arco_ametista": "arco_flecha", "W_adagas_gemeas_L": "adagas_gemeas"}.get(name, name[2:])
    mi = instances.get("MWN_" + kind)
    for i in range(len(slots)):
        if mi:
            m.set_material(i, mi)
    library.save_loaded_asset(m)
    box = m.get_bounding_box()
    lines.append("%s %s slots=%d caixa %s .. %s" % (name, m.get_path_name(), len(slots), box.min, box.max))

# os arcos: a corda (VortexWeaponParts) acha as pontas nos vértices
for name in ("W_arco_de_betula", "W_arco_ametista"):
    m = mesh(name)
    if m:
        m.set_editor_property("allow_cpu_access", True)
        library.save_loaded_asset(m)

# a espada do começo (DA_DefaultSword) volta a ser a espada (era a adaga inicial desde 22/10): a espada longa nova.
# O alcance do golpe sai do tamanho do modelo, e a adaga curta encurtava o golpe da espada.
sword = library.load_asset("/Game/Vortex/Weapons/Sword/DA_DefaultSword")
# 02/11: a de blocos (Tools/ImportBlockSword.py), se já foi importada; VORTEX_SWORD=meshy volta à do Meshy
long_sword = None if os.environ.get("VORTEX_SWORD") == "meshy" else mesh("W_espada_de_cinzas_blocos")
long_sword = long_sword or mesh("W_espada_de_cinzas")
if sword and long_sword:
    sword.set_editor_property("visual_mesh", long_sword)
    sword.set_editor_property("hand_bone", "Socket_Weapon_R")
    library.save_loaded_asset(sword)
    lines.append("DA_DefaultSword = " + long_sword.get_name())

# M_BowString: fio trançado (listras em diagonal ao longo do cilindro), creme e castanho, fosco
cord_path = MATS + "/M_BowString"
cord = library.load_asset(cord_path) if library.does_asset_exist(cord_path) else \
    tools.create_asset("M_BowString", MATS, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(cord)
uv = mel.create_material_expression(cord, unreal.MaterialExpressionTextureCoordinate, -900, 0)
mask_u = mel.create_material_expression(cord, unreal.MaterialExpressionComponentMask, -750, -60)
mask_u.set_editor_property("r", True)
mask_v = mel.create_material_expression(cord, unreal.MaterialExpressionComponentMask, -750, 60)
mask_v.set_editor_property("g", True)
mel.connect_material_expressions(uv, "", mask_u, "")
mel.connect_material_expressions(uv, "", mask_v, "")
ku = mel.create_material_expression(cord, unreal.MaterialExpressionMultiply, -600, -60)
ku.set_editor_property("const_b", 6.2832 * 3)
mel.connect_material_expressions(mask_u, "", ku, "A")
kv = mel.create_material_expression(cord, unreal.MaterialExpressionMultiply, -600, 60)
kv.set_editor_property("const_b", 6.2832 * 60)
mel.connect_material_expressions(mask_v, "", kv, "A")
twist = mel.create_material_expression(cord, unreal.MaterialExpressionAdd, -450, 0)
mel.connect_material_expressions(ku, "", twist, "A")
mel.connect_material_expressions(kv, "", twist, "B")
wave = mel.create_material_expression(cord, unreal.MaterialExpressionSine, -320, 0)
mel.connect_material_expressions(twist, "", wave, "")
alpha = mel.create_material_expression(cord, unreal.MaterialExpressionMultiply, -200, 0)
alpha.set_editor_property("const_b", .5)
mel.connect_material_expressions(wave, "", alpha, "A")
alpha2 = mel.create_material_expression(cord, unreal.MaterialExpressionAdd, -100, 0)
alpha2.set_editor_property("const_b", .5)
mel.connect_material_expressions(alpha, "", alpha2, "A")
light = mel.create_material_expression(cord, unreal.MaterialExpressionConstant3Vector, -200, -200)
light.set_editor_property("constant", unreal.LinearColor(.62, .55, .42, 1))
dark = mel.create_material_expression(cord, unreal.MaterialExpressionConstant3Vector, -200, -320)
dark.set_editor_property("constant", unreal.LinearColor(.30, .23, .15, 1))
mix = mel.create_material_expression(cord, unreal.MaterialExpressionLinearInterpolate, 50, -100)
mel.connect_material_expressions(dark, "", mix, "A")
mel.connect_material_expressions(light, "", mix, "B")
mel.connect_material_expressions(alpha2, "", mix, "Alpha")
rough = mel.create_material_expression(cord, unreal.MaterialExpressionConstant, 50, 100)
rough.set_editor_property("r", .8)
mel.connect_material_property(mix, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
cord.set_editor_property("used_with_instanced_static_meshes", True)
mel.recompile_material(cord)
library.save_loaded_asset(cord)
lines.append("M_BowString pronto")

for p in library.list_assets(MODELS, recursive=True):
    data = library.find_asset_data(p)
    kind = str(data.asset_class_path.asset_name)
    if kind in ("MaterialInstanceConstant", "Material", "Texture2D") and not library.find_package_referencers_for_asset(p):
        library.delete_asset(p)

with open(os.path.join(root, "Saved", "Logs", "ImportNewWeapons.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
unreal.log_warning("VORTEX_NEW_WEAPONS " + " | ".join(lines))
