"""As peças de armadura de blocos na Unreal (07/11) — editor fechado, depois do Tools/Armor/MakeAllArmor.sh.
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportBlockArmor.py -unattended -nullrhi

Data/Heroes/humano_Mixamo/Armaduras/A_<conjunto>_<peça>.glb, T_A_<...>.png e T_A_<...>_mask.png
(Tools/Blender/MakeBlockOutfit.py + armor_designs.py) -> /Game/Vortex/Characters/Heroes/humano/Armaduras:
- A_<...> (a malha com os nomes de osso do herói: o AVortexCharacter veste com a pose do corpo, SetLeaderPoseComponent);
- T_A_<...> e T_A_<...>_mask (pixel nítido; a máscara sem sRGB);
- MI_A_<...>, filha do M_ArmorBlock (criado aqui): a cor da textura, fosca; onde a máscara é vermelha a cor brilha
  (runas, lava, gemas: "Glow"); onde é verde vira a franja na cor do cabelo do herói ("HairTint", o jogo põe a cor).
VORTEX_ONLY=guarda_real,magma importa só esses conjuntos. Relatório em Saved/Logs/ImportBlockArmor.txt.
"""
import glob
import os

import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
ARMOR_SOURCE = os.path.join(root, "Data", "Heroes", "humano_Mixamo", "Armaduras")
ARMOR_DEST = "/Game/Vortex/Characters/Heroes/humano/Armaduras"
# VORTEX_PECAS=1: as roupas separadas (Tools/Blender/piece_designs.py), de Pecas/P_*, com o mesmo M_ArmorBlock
PIECES = bool(os.environ.get("VORTEX_PECAS"))
SOURCE = os.path.join(root, "Data", "Heroes", "humano_Mixamo", "Pecas") if PIECES else ARMOR_SOURCE
DEST = "/Game/Vortex/Characters/Heroes/humano/Pecas" if PIECES else ARMOR_DEST
only =[k for k in os.environ.get("VORTEX_ONLY", "").split(",") if k]
lines = []

# ---- o material das armaduras
path = ARMOR_DEST + "/M_ArmorBlock"
mat = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset("M_ArmorBlock", ARMOR_DEST, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(mat)


def node(cls, x, y, **props):
    n = mel.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        n.set_editor_property(k, v)
    return n


# 09/11: todo parâmetro de textura precisa de um padrão: sem ele o material não compila e a Unreal desenha o material padrão (branco e cinza)
base = node(unreal.MaterialExpressionTextureSampleParameter2D, -900, 0, parameter_name="Base",
            texture=unreal.load_object(None, "/Engine/EngineResources/DefaultTexture.DefaultTexture"))
mask = node(unreal.MaterialExpressionTextureSampleParameter2D, -900, 300, parameter_name="Mask",
            sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)   # a textura da máscara é TC_MASKS (o tipo tem de bater, senão o material quebra)
# o padrão da máscara tem de ser uma máscara (TC_MASKS, sem sRGB) como as nossas: a preta do motor é de cor e quebrava o
# material ("Sampler type ... should be Color")
for _p in sorted(glob.glob(os.path.join(ARMOR_SOURCE, "T_A_*_mask.png"))):
    _a = ARMOR_DEST + "/" +os.path.splitext(os.path.basename(_p))[0]
    if library.does_asset_exist(_a):
        mask.set_editor_property("texture", library.load_asset(_a))
        break
# 09/11: o relevo por pixel (sem o mapa da peça, fica o plano do motor)
normal = node(unreal.MaterialExpressionTextureSampleParameter2D, -500, 760, parameter_name="Normal", sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
              texture=unreal.load_object(None, "/Engine/EngineMaterials/DefaultNormal.DefaultNormal"))
if not normal.get_editor_property("texture") or not base.get_editor_property("texture"):
    raise RuntimeError("textura padrão do material não encontrada")
mel.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)
tint = node(unreal.MaterialExpressionVectorParameter, -900, -250, parameter_name="HairTint",
            default_value=unreal.LinearColor(0.36, 0.22, 0.12, 1))
glow = node(unreal.MaterialExpressionScalarParameter, -600, 450, parameter_name="Glow", default_value=2.5)
rough = node(unreal.MaterialExpressionScalarParameter, -300, 300, parameter_name="Roughness", default_value=.85)
two = node(unreal.MaterialExpressionConstant, -700, -150, r=2.0)
# a franja: o tom da textura x a cor do cabelo x 2 (como o M_BlockHair)
hair = node(unreal.MaterialExpressionMultiply, -600, -200)
mel.connect_material_expressions(base, "RGB", hair, "A")
mel.connect_material_expressions(tint, "", hair, "B")
hair2 = node(unreal.MaterialExpressionMultiply, -450, -200)
mel.connect_material_expressions(hair, "", hair2, "A")
mel.connect_material_expressions(two, "", hair2, "B")
pick = node(unreal.MaterialExpressionLinearInterpolate, -300, 0)
mel.connect_material_expressions(base, "RGB", pick, "A")
mel.connect_material_expressions(hair2, "", pick, "B")
mel.connect_material_expressions(mask, "G", pick, "Alpha")
mel.connect_material_property(pick, "", unreal.MaterialProperty.MP_BASE_COLOR)
# o brilho: a cor onde a máscara é vermelha
lit = node(unreal.MaterialExpressionMultiply, -300, 250)
mel.connect_material_expressions(base, "RGB", lit, "A")
mel.connect_material_expressions(mask, "R", lit, "B")
lit2 = node(unreal.MaterialExpressionMultiply, -150, 300)
mel.connect_material_expressions(lit, "", lit2, "A")
mel.connect_material_expressions(glow, "", lit2, "B")
# 09/11: a luz nas bordas: um contorno sutil que separa o herói do fundo
rim_fresnel = node(unreal.MaterialExpressionFresnel, -420, 900, exponent=4.0, base_reflect_fraction=0.0)
rim_color = node(unreal.MaterialExpressionVectorParameter, -420, 1040, parameter_name="RimColor", default_value=unreal.LinearColor(.22, .22, .26, 1))
rim = node(unreal.MaterialExpressionMultiply, -220, 950)
mel.connect_material_expressions(rim_fresnel, "", rim, "A")
mel.connect_material_expressions(rim_color, "", rim, "B")
glow_rim = node(unreal.MaterialExpressionAdd, 0, 500)
mel.connect_material_expressions(lit2, "", glow_rim, "A")
mel.connect_material_expressions(rim, "", glow_rim, "B")
mel.connect_material_property(glow_rim, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
# 09/11: o canal azul da máscara é metal: reflete, e menos que 100% (sem o Lumen, metal puro fica escuro)
metal_amount = node(unreal.MaterialExpressionConstant, -300, 520, r=.05)   # 10/11: metal demais sem reflexo escurecia
metal = node(unreal.MaterialExpressionMultiply, -150, 520)
mel.connect_material_expressions(mask, "B", metal, "A")
mel.connect_material_expressions(metal_amount, "", metal, "B")
mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
shiny = node(unreal.MaterialExpressionConstant, -300, 620, r=.9)   # 11/11: 0,36 e 0,65 brilhavam até na sombra (o reflexo do céu)
rough_mix = node(unreal.MaterialExpressionLinearInterpolate, -150, 620)
mel.connect_material_expressions(rough, "", rough_mix, "A")
mel.connect_material_expressions(shiny, "", rough_mix, "B")
mel.connect_material_expressions(mask, "B", rough_mix, "Alpha")
mel.connect_material_property(rough_mix, "", unreal.MaterialProperty.MP_ROUGHNESS)
mat.set_editor_property("used_with_skeletal_mesh", True)
mel.recompile_material(mat)
library.save_loaded_asset(mat)


def bring_texture(file, name, srgb, is_normal=False):
    t = unreal.AssetImportTask()
    t.filename = file
    t.destination_path = DEST
    t.destination_name = name
    t.automated = True
    t.save = False
    t.replace_existing = True
    tools.import_asset_tasks([t])
    tex = library.load_asset(DEST + "/" + name)
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("never_stream", True)
    tex.set_editor_property("srgb", srgb)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP if is_normal else unreal.TextureCompressionSettings.TC_EDITOR_ICON if srgb else unreal.TextureCompressionSettings.TC_MASKS)
    library.save_loaded_asset(tex)
    return tex


for glb in sorted(glob.glob(os.path.join(SOURCE, ("P_*.glb" if PIECES else "A_*.glb")))):
    name = os.path.splitext(os.path.basename(glb))[0]
    key = name[2:].rsplit("_", 1)[0]
    if only and key not in only:
        continue
    tex = bring_texture(os.path.join(SOURCE, "T_%s.png" % name), "T_" + name, True)
    mask_file = os.path.join(SOURCE, "T_%s_mask.png" % name)
    mtex = bring_texture(mask_file, "T_%s_mask" % name, False) if os.path.exists(mask_file) else None
    mi_path = DEST + "/MI_" + name
    mi = library.load_asset(mi_path) if library.does_asset_exist(mi_path) else tools.create_asset(
        "MI_" + name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, mat)
    mel.set_material_instance_texture_parameter_value(mi, "Base", tex)
    if mtex:
        mel.set_material_instance_texture_parameter_value(mi, "Mask", mtex)
    normal_file = os.path.join(SOURCE, "T_%s_normal.png" % name)
    if os.path.exists(normal_file):
        mel.set_material_instance_texture_parameter_value(mi, "Normal", bring_texture(normal_file, "T_%s_normal" % name, False, True))
    library.save_loaded_asset(mi)
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("scene_name_sub_folder", False)
    pipeline.set_editor_property("asset_type_sub_folders", False)
    common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    common.set_editor_property("try_auto_select_skeleton", False)
    pipeline.get_editor_property("mesh_pipeline").set_editor_property("create_physics_asset", False)
    pipeline.get_editor_property("animation_pipeline").set_editor_property("import_animations", False)
    pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
    override = unreal.InterchangePipelineStackOverride()
    override.add_pipeline(pipeline)
    task = unreal.AssetImportTask()
    task.filename = glb
    task.destination_path = DEST
    task.automated = True
    task.save = False
    task.replace_existing = True
    task.options = override
    tools.import_asset_tasks([task])
    mesh = library.load_asset(DEST + "/" + name) if library.does_asset_exist(DEST + "/" + name) else None
    if not mesh or not isinstance(mesh, unreal.SkeletalMesh):
        lines.append("FALHOU %s" % name)
        continue
    slots = [unreal.SkeletalMaterial(material_interface=mi, material_slot_name=s.get_editor_property("material_slot_name"),
                                     uv_channel_data=s.get_editor_property("uv_channel_data"))
             for s in mesh.get_editor_property("materials")]
    mesh.set_editor_property("materials", slots)
    library.save_loaded_asset(mesh)
    skeleton = mesh.get_editor_property("skeleton")
    if skeleton:
        library.save_loaded_asset(skeleton)
    for a in library.list_assets(DEST, recursive=False):
        n = a.split("/")[-1].split(".")[0]
        if n.startswith(("Image", "Material_")) or (n.startswith("T_" + name + "_") and not n.endswith(("_mask", "_normal"))):
            library.delete_asset(a)
    lines.append("%s ok" % name)
with open(os.path.join(root, "Saved", "Logs", "ImportBlockArmor.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
unreal.log_warning("VORTEX_BLOCK_ARMOR %d peças | %s" % (sum(1 for l in lines if l.endswith(" ok")), " | ".join(l for l in lines if "FALHOU" in l)))
