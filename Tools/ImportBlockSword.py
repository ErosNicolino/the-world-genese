"""As armas iniciais em blocos (Tools/Blender/MakeBlockSword.py e MakeBlockWeapons.py) na Unreal — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportBlockSword.py -unattended -nullrhi

Para cada arma com Data/VortexWeapons/Models/W_<arma>_blocos.glb:
- a malha -> /Game/Vortex/Weapons/Models/W_<arma>_blocos;
- T_<arma>_blocos(_rough).png -> /Game/Vortex/Weapons/Materials/Novas (pixel nítido, sem mipmaps);
- MI_WN_<arma>_blocos (filha do M_WeaponTextured, relevo liso, nada metálico) e MI_WN_<arma>_blocos_brasa (a mesma,
  acesa: gemas, runas, brasa e cristal, no espaço MW_<arma>_blocos_brasa);
- a definição da arma passa a usar a de blocos (DA_DefaultSword para a espada; DA_<arma> para as outras, e a adaga da
  mão esquerda no left_mesh). VORTEX_SWORD=meshy deixa as definições nas do Meshy.
(A 1ª versão, de 02/11, só fazia a espada: daí o nome.) Relatório em Saved/Logs/ImportBlockSword.txt.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
MODELS = "/Game/Vortex/Weapons/Models"
MATS = "/Game/Vortex/Weapons/Materials/Novas"
models = os.path.join(root, "Data", "VortexWeapons", "Models")
textures = os.path.join(root, "Data", "VortexWeapons", "Textures", "Novas")
use_blocks = os.environ.get("VORTEX_SWORD", "blocos") == "blocos"
lines = []

# a arma -> (a textura dela, a definição e onde a malha entra)
WEAPONS = {
    "espada_de_cinzas": ("espada", "/Game/Vortex/Weapons/Sword/DA_DefaultSword", "visual_mesh"),
    "adagas_gemeas": ("adagas_gemeas", "/Game/Vortex/Weapons/Types/DA_adagas_gemeas", "visual_mesh"),
    "adagas_gemeas_L": ("adagas_gemeas_L", "/Game/Vortex/Weapons/Types/DA_adagas_gemeas", "left_mesh"),
    "martelo_de_bram": ("martelo_de_bram", "/Game/Vortex/Weapons/Types/DA_martelo_de_bram", "visual_mesh"),
    "cajado_de_ilsa": ("cajado_de_ilsa", "/Game/Vortex/Weapons/Types/DA_cajado_de_ilsa", "visual_mesh"),
    "arco_de_betula": ("arco_de_betula", "/Game/Vortex/Weapons/Types/DA_arco_de_betula", "visual_mesh"),
}
# 07/11: as 35 espadas das pranchas do usuário (Tools/Blender/MakePixelSwords.py): só a malha e os materiais; o jogo
# escolhe pela raridade da espada equipada (VortexWeaponComponent). VORTEX_ONLY_SWORDS=1 importa só elas.
import glob as _glob
import re as _re
# 07/11: e os 35 arcos (sem corda) e as 35 flechas (Tools/Blender/MakePixelBows.py), do mesmo jeito
for _p in sorted(_glob.glob(os.path.join(models, "W_*_blocos.glb"))):
    _m = _re.match(r"W_((?:espada|arco|flecha)_[a-z]+_\d+|flecha_betula|item_[a-z0-9_]+)_blocos\.glb$", os.path.basename(_p))
    if _m:
        WEAPONS[_m.group(1)] = (_m.group(1), None, "visual_mesh")
if os.environ.get("VORTEX_ONLY_SWORDS"):
    WEAPONS = {k: v for k, v in WEAPONS.items() if v[1] is None}
# VORTEX_ONLY=arco,flecha: só as que começam assim
if os.environ.get("VORTEX_ONLY"):
    WEAPONS = {k: v for k, v in WEAPONS.items() if k.startswith(tuple(os.environ["VORTEX_ONLY"].split(",")))}


def bring(path, folder, name):
    t = unreal.AssetImportTask()
    t.filename = path
    t.destination_path = folder
    t.destination_name = name
    t.automated = True
    t.save = True
    t.replace_existing = True
    if path.endswith(".glb"):
        pipeline = unreal.InterchangeGenericAssetsPipeline()
        common = pipeline.get_editor_property("common_meshes_properties")
        common.set_editor_property("recompute_normals", False)
        common.set_editor_property("use_full_precision_u_vs", True)
        override = unreal.InterchangePipelineStackOverride()
        override.add_pipeline(pipeline)
        t.options = override
    tools.import_asset_tasks([t])


def instance(name, parent):
    path = MATS + "/" + name
    mi = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset(name, MATS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    return mi


for weapon, (tex_name, definition, slot) in WEAPONS.items():
    glb = os.path.join(models, "W_%s_blocos.glb" % weapon)
    if not os.path.exists(glb):
        continue
    for name in ("T_%s_blocos" % tex_name, "T_%s_blocos_rough" % tex_name):
        bring(os.path.join(textures, name + ".png"), MATS, name)
        tex = library.load_asset(MATS + "/" + name)
        tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tex.set_editor_property("never_stream", True)
        if name.endswith("_rough"):
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        else:
            # a compressão padrão, como as outras armas (a de ícone, sem compressão, saía pontilhada e borrada no jogo)
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        library.save_loaded_asset(tex)

    mi = instance("MI_WN_%s_blocos" % tex_name, library.load_asset("/Game/Vortex/Weapons/Materials/M_WeaponTextured"))
    mel.set_material_instance_texture_parameter_value(mi, "Base", library.load_asset(MATS + "/T_%s_blocos" % tex_name))
    mel.set_material_instance_texture_parameter_value(mi, "RoughnessMap", library.load_asset(MATS + "/T_%s_blocos_rough" % tex_name))
    mel.set_material_instance_texture_parameter_value(mi, "Normal", unreal.load_asset("/Engine/EngineMaterials/FlatNormal"))
    mel.set_material_instance_scalar_parameter_value(mi, "Metallic", 0.0)
    mel.set_material_instance_scalar_parameter_value(mi, "Bright", 1.0 if weapon.startswith("item_") else 1.6)   # o sol da tarde escurece o aço fosco
    mel.set_material_instance_scalar_parameter_value(mi, "Glow", 0.0)
    library.save_loaded_asset(mi)
    # a brasa, as gemas e o cristal (o espaço MW_<arma>_blocos_brasa): a mesma textura, acesa
    glow = instance("MI_WN_%s_blocos_brasa" % tex_name, mi)
    mel.set_material_instance_scalar_parameter_value(glow, "Glow", 2.5)
    library.save_loaded_asset(glow)

    bring(glb, MODELS, "W_%s_blocos" % weapon)
    mesh = library.load_asset(MODELS + "/W_%s_blocos" % weapon)
    settings = mesh.get_editor_property("nanite_settings")
    settings.set_editor_property("enabled", False)
    mesh.set_editor_property("nanite_settings", settings)
    build = unreal.EditorStaticMeshLibrary.get_lod_build_settings(mesh, 0)
    build.set_editor_property("recompute_normals", False)
    build.set_editor_property("use_full_precision_u_vs", True)
    unreal.EditorStaticMeshLibrary.set_lod_build_settings(mesh, 0, build)
    for i, s in enumerate(mesh.get_editor_property("static_materials")):
        mesh.set_material(i, glow if "brasa" in str(s.get_editor_property("material_slot_name")) else mi)
    if weapon.startswith("arco"):
        # a corda (VortexWeaponParts) acha as pontas nos vértices
        mesh.set_editor_property("allow_cpu_access", True)
    library.save_loaded_asset(mesh)
    box = mesh.get_bounding_box()
    lines.append("W_%s_blocos caixa %s .. %s" % (weapon, box.min, box.max))

    if definition is None:
        continue
    da = library.load_asset(definition)
    if da:
        chosen = mesh if use_blocks else library.load_asset(MODELS + "/W_%s" % weapon)
        da.set_editor_property(slot, chosen)
        library.save_loaded_asset(da)
        lines.append("%s.%s = %s" % (definition.split("/")[-1], slot, chosen.get_name()))
    else:
        lines.append("FALTA a definição " + definition)

out = os.path.join(root, "Saved", "Logs", "ImportBlockSword.txt")
open(out, "w", encoding="utf-8").write("\n".join(lines) + "\n")
