"""As armas novas do Vortex (Tools/Blender/BuildVortexWeapons.py) no lugar das do Godot — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportVortexWeapons.py -unattended

- texturas Data/VortexWeapons/Textures/T_<material>.png -> /Game/Vortex/Weapons/Materials/T_<material>;
- M_WeaponTextured (cor da textura, metálico, rugosidade e brilho nas gemas/runas) e uma instância por
  material (MI_W_aco, MI_W_ouro, MI_W_couro...), com os números de materials.json;
- modelos Data/VortexWeapons/Models/W_<tipo>.glb (e _L) -> /Game/Vortex/Weapons/Models/W_<tipo>, no
  mesmo caminho dos antigos (as definições DA_<tipo> continuam apontando para eles). A reimportação
  não troca os materiais: cada espaço recebe a instância pelo nome (MW_<material> -> MI_W_<material>);
- a Espada e Escudo ganha o escudo na mão esquerda (left_mesh).
Relatório em Saved/Logs/ImportVortexWeapons.txt.
"""
import json
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
MODELS = "/Game/Vortex/Weapons/Models"
MATS = "/Game/Vortex/Weapons/Materials"
models = os.path.join(root, "Data", "VortexWeapons", "Models")
# As texturas PBR mais novas são preparadas para o Blender na pasta de trabalho
# SourceArt/Blender_Personagem_Armas. Ela passa a ser a fonte quando estiver presente.
textures = os.path.join(root, "SourceArt", "Blender_Personagem_Armas", "04_Texturas")
if not os.path.isdir(textures):
    textures = os.path.join(root, "Data", "VortexWeapons", "Textures")
numbers = json.load(open(os.path.join(textures, "materials.json"), encoding="utf-8"))
lines = []

# ---- texturas
tasks = []
for f in sorted(os.listdir(textures)):
    if not f.lower().endswith(".png") or not f.startswith("T_"):
        continue
    t = unreal.AssetImportTask()
    t.filename = os.path.join(textures, f)
    t.destination_path = MATS
    t.destination_name = f[:-4]
    t.automated = True
    t.save = True
    t.replace_existing = True
    tasks.append(t)
tools.import_asset_tasks(tasks)

# Os mapas PBR não usam correção de cor; normais usam a compressão própria da Unreal.
for f in sorted(os.listdir(textures)):
    if not f.lower().endswith(".png") or not f.startswith("T_"):
        continue
    asset = library.load_asset(MATS + "/" + f[:-4])
    if not asset:
        continue
    lower = f.lower()
    if lower.endswith("_normal.png"):
        asset.set_editor_property("srgb", False)
        asset.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    elif lower.endswith("_rough.png"):
        asset.set_editor_property("srgb", False)
        asset.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    else:
        # 02/11: as cores em pixel art (Tools/MakePixelWeaponMaterials.py): pixel nítido, sem borrar
        asset.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    library.save_loaded_asset(asset)

# ---- material-base
base_path = MATS + "/M_WeaponTextured"
base = library.load_asset(base_path) if library.does_asset_exist(base_path) else \
    tools.create_asset("M_WeaponTextured", MATS, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(base)
tex = mel.create_material_expression(base, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0)
tex.set_editor_property("parameter_name", "Base")
metal = mel.create_material_expression(base, unreal.MaterialExpressionScalarParameter, -400, 200)
metal.set_editor_property("parameter_name", "Metallic")
normal_map = mel.create_material_expression(base, unreal.MaterialExpressionTextureSampleParameter2D, -700, -260)
normal_map.set_editor_property("parameter_name", "Normal")
rough_map = mel.create_material_expression(base, unreal.MaterialExpressionTextureSampleParameter2D, -700, -520)
rough_map.set_editor_property("parameter_name", "RoughnessMap")
glow = mel.create_material_expression(base, unreal.MaterialExpressionScalarParameter, -400, 420)
glow.set_editor_property("parameter_name", "Glow")
emit = mel.create_material_expression(base, unreal.MaterialExpressionMultiply, -200, 380)
mel.connect_material_expressions(tex, "RGB", emit, "A")
mel.connect_material_expressions(glow, "", emit, "B")
bright = mel.create_material_expression(base, unreal.MaterialExpressionScalarParameter, -400, 80)
bright.set_editor_property("parameter_name", "Bright")
bright.set_editor_property("default_value", 1.0)
color = mel.create_material_expression(base, unreal.MaterialExpressionMultiply, -200, 0)
mel.connect_material_expressions(tex, "RGB", color, "A")
mel.connect_material_expressions(bright, "", color, "B")
mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
mel.connect_material_property(rough_map, "R", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(normal_map, "RGB", unreal.MaterialProperty.MP_NORMAL)
mel.connect_material_property(emit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
base.set_editor_property("used_with_nanite", True)
# dois lados (01/11): os modelos do Meshy têm faces do avesso (triângulos pretos e buracos com um lado só)
base.set_editor_property("two_sided", True)
base.set_editor_property("used_with_skeletal_mesh", True)
mel.recompile_material(base)
library.save_loaded_asset(base)

instances = {}
for name, values in numbers.items():
    path = MATS + "/MI_W_" + name
    mi = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset("MI_W_" + name, MATS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, base)
    texture = library.load_asset(MATS + "/T_" + name)
    if texture:
        mel.set_material_instance_texture_parameter_value(mi, "Base", texture)
    normal = library.load_asset(MATS + "/T_" + name + "_normal")
    if normal:
        mel.set_material_instance_texture_parameter_value(mi, "Normal", normal)
    rough_map_asset = library.load_asset(MATS + "/T_" + name + "_rough")
    if rough_map_asset:
        mel.set_material_instance_texture_parameter_value(mi, "RoughnessMap", rough_map_asset)
    # metais: um pouco menos de metálico (sem Lumen o reflexo do chão de areia amarelava o aço)
    # metais: pouco metálico e um pouco mais claros (sem Lumen o reflexo do chão escurecia e amarelava o aço)
    mel.set_material_instance_scalar_parameter_value(mi, "Metallic", values["metallic"] * .3)
    mel.set_material_instance_scalar_parameter_value(mi, "Bright", 1.35 if values["metallic"] > .5 else 1.1)
    mel.set_material_instance_scalar_parameter_value(mi, "Glow", values["glow"] * 2.0)
    library.save_loaded_asset(mi)
    instances["MW_" + name] = mi

# ---- modelos
tasks = []
for f in sorted(os.listdir(models)):
    if not f.endswith(".glb"):
        continue
    t = unreal.AssetImportTask()
    t.filename = os.path.join(models, f)
    t.destination_path = MODELS
    t.automated = True
    t.save = True
    t.replace_existing = True
    tasks.append(t)
tools.import_asset_tasks(tasks)


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


for f in sorted(os.listdir(models)):
    if not f.endswith(".glb"):
        continue
    name = f[:-4]
    m = mesh(name)
    if not m:
        lines.append("FALHOU " + name)
        continue
    settings = m.get_editor_property("nanite_settings")
    settings.set_editor_property("enabled", False)
    m.set_editor_property("nanite_settings", settings)
    missing = []
    slots = m.get_editor_property("static_materials")
    for i, slot in enumerate(slots):
        slot_name = str(slot.get_editor_property("material_slot_name"))
        mi = instances.get(slot_name) or instances.get(slot_name.split(".")[0])
        if mi:
            m.set_material(i, mi)
        else:
            missing.append(slot_name)
    library.save_loaded_asset(m)
    lines.append("%s espaços=%d%s" % (name, len(slots), (" SEM MATERIAL: " + ",".join(missing)) if missing else ""))

shield = mesh("W_espada_escudo_L")
da = library.load_asset("/Game/Vortex/Weapons/Types/DA_espada_escudo")
if da and shield:
    da.set_editor_property("left_mesh", shield)
    library.save_loaded_asset(da)
    lines.append("escudo na Espada e Escudo: " + shield.get_path_name())

# a espada padrão (DA_DefaultSword, a Espada de Cinzas) usa a espada nova, no soquete da mão como as outras
# (desde 22/10 é a adaga inicial de ImportStarterDagger.py, quando ela já foi importada)
sword = library.load_asset("/Game/Vortex/Weapons/Sword/DA_DefaultSword")
dagger = "/Game/Vortex/Weapons/StarterDagger/W_adaga_inicial/StaticMeshes/W_adaga_inicial"
new_sword = library.load_asset(dagger) if library.does_asset_exist(dagger) else mesh("W_espada_de_cinzas")
if sword and new_sword:
    sword.set_editor_property("visual_mesh", new_sword)
    sword.set_editor_property("hand_bone", "Socket_Weapon_R")
    library.save_loaded_asset(sword)
    lines.append("espada padrão = %s no Socket_Weapon_R" % new_sword.get_name())

# sobras da importação do glTF (materiais e texturas criados pelo importador): não usadas
for p in library.list_assets(MODELS, recursive=True):
    data = library.find_asset_data(p)
    kind = str(data.asset_class_path.asset_name)
    if kind in ("MaterialInstanceConstant", "Material", "Texture2D") and not library.find_package_referencers_for_asset(p):
        library.delete_asset(p)

with open(os.path.join(root, "Saved", "Logs", "ImportVortexWeapons.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
