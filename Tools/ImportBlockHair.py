"""Os cabelos do humano de blocos na Unreal — editor fechado, depois do ImportMixamoHeroes.py.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportBlockHair.py -unattended -nullrhi

- Data/Heroes/humano_Mixamo/Hair/Hair_<estilo>.glb (Tools/Blender/MakeBlockHair.py -- jogo: o cabelo no esqueleto do
  Hero_humano.glb) -> /Game/Vortex/Characters/Heroes/humano/Hair/Hair_<estilo> (malha com esqueleto, os mesmos nomes
  de osso do herói: o AVortexCharacter veste por cima do corpo e copia a pose dele);
- T_Cabelo_Tons.png -> T_Cabelo_Tons (linear, pixel nítido): o quanto cada tom é mais claro ou escuro que o tom-base;
- M_BlockHair: cor = Tint x tom x 2 (Tint é a cor "hair_color" do herói, escolhida no menu);
- Data/Heroes/Mixamo/humano_cor_f.png (Tools/MakeBlockFemale.py) -> /Game/Vortex/Characters/Heroes/humano/T_humano_Pixel_F
  (a cor da versão feminina, pixel nítido como a T_humano_Pixel).
Relatório em Saved/Logs/ImportBlockHair.txt.
"""
import glob
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
SOURCE = os.path.join(root, "Data", "Heroes", "humano_Mixamo", "Hair")
DEST = "/Game/Vortex/Characters/Heroes/humano/Hair"
STAGING = DEST + "/_import"
lines = []


def node(m, kind, x, y, **props):
    e = mel.create_material_expression(m, kind, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


# ---- os tons
t = unreal.AssetImportTask()
t.filename = os.path.join(SOURCE, "T_Cabelo_Tons.png")
t.destination_path = DEST
t.destination_name = "T_Cabelo_Tons"
t.automated = True
t.save = False
t.replace_existing = True
tools.import_asset_tasks([t])
tones = library.load_asset(DEST + "/T_Cabelo_Tons")
tones.set_editor_property("srgb", False)
tones.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
tones.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
tones.set_editor_property("never_stream", True)
tones.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
library.save_loaded_asset(tones)

# ---- a cor da versão feminina
t = unreal.AssetImportTask()
t.filename = os.path.join(root, "Data", "Heroes", "Mixamo", "humano_cor_f.png")
t.destination_path = "/Game/Vortex/Characters/Heroes/humano"
t.destination_name = "T_humano_Pixel_F"
t.automated = True
t.save = False
t.replace_existing = True
tools.import_asset_tasks([t])
female = library.load_asset("/Game/Vortex/Characters/Heroes/humano/T_humano_Pixel_F")
female.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
female.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
female.set_editor_property("never_stream", True)
female.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
library.save_loaded_asset(female)

# ---- o material
path = DEST + "/M_BlockHair"
m = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset("M_BlockHair", DEST, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
tex = node(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, 0, parameter_name="Tones", texture=tones,
           sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
tint = node(m, unreal.MaterialExpressionVectorParameter, -800, -250, parameter_name="Tint",
            default_value=unreal.LinearColor(.188, .052, .019, 1.0))      # o #784026 em linear
times = node(m, unreal.MaterialExpressionMultiply, -500, -100)
mel.connect_material_expressions(tint, "", times, "A")
mel.connect_material_expressions(tex, "R", times, "B")
double = node(m, unreal.MaterialExpressionMultiply, -300, -100, const_b=2.0)
mel.connect_material_expressions(times, "", double, "A")
mel.connect_material_property(double, "", unreal.MaterialProperty.MP_BASE_COLOR)
rough = node(m, unreal.MaterialExpressionConstant, -300, 150, r=.9)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
m.set_editor_property("used_with_skeletal_mesh", True)
mel.recompile_material(m)
library.save_loaded_asset(m)

# ---- o das mechas (02/11, a Julia: Hair_<estilo>_mechas.glb): a cor de vértice R = 0 marca as mechas, pintadas com
# StreakTint; o resto com o Tint, como no M_BlockHair. Só os cabelos _mechas usam (os outros não têm cor de vértice)
path = DEST + "/M_BlockHairMechas"
m = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset("M_BlockHairMechas", DEST, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
tex = node(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, 0, parameter_name="Tones", texture=tones,
           sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
tint = node(m, unreal.MaterialExpressionVectorParameter, -900, -300, parameter_name="Tint",
            default_value=unreal.LinearColor(.012, .010, .011, 1.0))
streak = node(m, unreal.MaterialExpressionVectorParameter, -900, -450, parameter_name="StreakTint",
              default_value=unreal.LinearColor(.03, .08, .45, 1.0))
vcol = node(m, unreal.MaterialExpressionVertexColor, -900, -600)
mix = node(m, unreal.MaterialExpressionLinearInterpolate, -650, -350)
mel.connect_material_expressions(streak, "", mix, "A")
mel.connect_material_expressions(tint, "", mix, "B")
mel.connect_material_expressions(vcol, "R", mix, "Alpha")
times = node(m, unreal.MaterialExpressionMultiply, -450, -100)
mel.connect_material_expressions(mix, "", times, "A")
mel.connect_material_expressions(tex, "R", times, "B")
double = node(m, unreal.MaterialExpressionMultiply, -250, -100, const_b=2.0)
mel.connect_material_expressions(times, "", double, "A")
mel.connect_material_property(double, "", unreal.MaterialProperty.MP_BASE_COLOR)
rough = node(m, unreal.MaterialExpressionConstant, -250, 150, r=.9)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
m.set_editor_property("used_with_skeletal_mesh", True)
mel.recompile_material(m)
library.save_loaded_asset(m)

# ---- as malhas: direto na pasta final com o nome do arquivo (Hair_<estilo>), substituindo no lugar (importar à parte e
# renomear não funcionava: o apagado continuava ocupando o nome)
for glb in sorted(glob.glob(os.path.join(SOURCE, "Hair_*.glb"))):
    name = os.path.splitext(os.path.basename(glb))[0]
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("scene_name_sub_folder", False)
    pipeline.set_editor_property("asset_type_sub_folders", False)
    common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    common.set_editor_property("try_auto_select_skeleton", False)
    pipeline.get_editor_property("mesh_pipeline").set_editor_property("create_physics_asset", False)
    # a cor de vértice do arquivo (as mechas): a do glb (já é o padrão; fica escrito)
    pipeline.get_editor_property("common_meshes_properties").set_editor_property(
        "vertex_color_import_option", unreal.InterchangeVertexColorImportOption.IVCIO_REPLACE)
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
    # slots novos: mudar o material_interface das cópias que o get_editor_property devolve não chegava na malha (os 26
    # cabelos ficaram com o WorldGridMaterial cinza e sem cor; corrigido em 01/11)
    material = library.load_asset(DEST + ("/M_BlockHairMechas" if name.endswith("_mechas") else "/M_BlockHair"))
    slots = [unreal.SkeletalMaterial(material_interface=material, material_slot_name=s.get_editor_property("material_slot_name"),
                                     uv_channel_data=s.get_editor_property("uv_channel_data"))
             for s in mesh.get_editor_property("materials")]
    mesh.set_editor_property("materials", slots)
    library.save_loaded_asset(mesh)
    skeleton = mesh.get_editor_property("skeleton")
    if skeleton:
        library.save_loaded_asset(skeleton)
    lines.append("%s: %s (%d materiais)" % (name, mesh.get_path_name(), len(slots)))
# o que sobrou da importação antiga (a pasta _import)
if library.does_directory_exist(STAGING):
    library.delete_directory(STAGING)

with open(os.path.join(root, "Saved", "Logs", "ImportBlockHair.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
unreal.log_warning("VORTEX_BLOCK_HAIR " + " | ".join(lines))
