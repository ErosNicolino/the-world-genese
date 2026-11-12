"""A vida da cidade e o clima na Unreal (11/11), editor fechado:
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportTownLife.py -unattended -nullrhi

Antes: python Tools/Sfx/MakeTownSounds.py e python Tools/Weather/MakeWeatherTextures.py.
- Data/Sfx/Town/*.wav -> /Game/Vortex/Audio/Sfx (bichos e bigorna em Efeitos; fogo, taverna, chuva e vento em Ambiente, em
  loop);
- Data/Weather/T_Rain, T_Snow -> /Game/Vortex/Weather, e o material M_VortexPrecip (translúcido, sem luz, dos dois lados):
  a textura rola para baixo (Speed), repete (Tiling), balança de lado (Sway, a neve) e some nas tampas e nas pontas do
  cilindro; Intensity e Color vêm do UVortexWeatherSubsystem;
- os bichos do Cube World Kit da Quaternius (CC0, `modelando ainda/quaternius_cubos/pacote`) -> /Game/Vortex/Town/Animals/
  <Bicho>: SK_<Bicho> e as animações A_<Bicho>_<Walk, Run, Idle, Idle_Eating, Idle_Peck, ...> (AVortexTownLife).
Relatório em Saved/Logs/ImportTownLife.txt.
"""
import glob
import os

import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
lines = []


def plain_import(file, dest, name):
    t = unreal.AssetImportTask()
    t.filename = file
    t.destination_path = dest
    t.destination_name = name
    t.automated = True
    t.replace_existing = True
    t.save = False
    tools.import_asset_tasks([t])
    return library.load_asset(dest + "/" + name)


# ---- sons
effects = library.load_asset("/Game/Vortex/Audio/SC_Effects")
ambience = library.load_asset("/Game/Vortex/Audio/SC_Ambience")
LOOPS = ("amb_", "town_fire", "town_tavern", "anim_cat_purr")
count = 0
for file in sorted(glob.glob(os.path.join(root, "Data", "Sfx", "Town", "*.wav"))):
    name = os.path.splitext(os.path.basename(file))[0]
    wave = plain_import(file, "/Game/Vortex/Audio/Sfx", name)
    if not wave:
        lines.append("FALHOU som %s" % name)
        continue
    loop = name.startswith(LOOPS)
    wave.set_editor_property("looping", loop)
    sound_class = ambience if loop and name != "anim_cat_purr" else effects
    if sound_class:
        wave.set_editor_property("sound_class_object", sound_class)
    library.save_loaded_asset(wave)
    count += 1
lines.append("%d sons" % count)

# ---- o clima: texturas e material
WEATHER = "/Game/Vortex/Weather"
textures = {}
for name in ("T_Rain", "T_Snow"):
    tex = plain_import(os.path.join(root, "Data", "Weather", name + ".png"), WEATHER, name)
    tex.set_editor_property("srgb", False)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    tex.set_editor_property("never_stream", True)
    library.save_loaded_asset(tex)
    textures[name] = tex
path = WEATHER + "/M_VortexPrecip"
mat = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset("M_VortexPrecip", WEATHER, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(mat)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)


def node(cls, x, y, **props):
    n = mel.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        n.set_editor_property(k, v)
    return n


def custom(x, y, code, inputs, output):
    n = node(unreal.MaterialExpressionCustom, x, y, code=code, output_type=output)
    pins = []
    for name in inputs:
        pin = unreal.CustomInput()
        pin.set_editor_property("input_name", name)
        pins.append(pin)
    n.set_editor_property("inputs", pins)
    return n


uv = node(unreal.MaterialExpressionTextureCoordinate, -1100, 0)
tiling = node(unreal.MaterialExpressionVectorParameter, -1100, 120, parameter_name="Tiling", default_value=unreal.LinearColor(8, 3, 0, 0))
speed = node(unreal.MaterialExpressionVectorParameter, -1100, 300, parameter_name="Speed", default_value=unreal.LinearColor(0, 1.8, 0, 0))
sway = node(unreal.MaterialExpressionScalarParameter, -1100, 480, parameter_name="Sway", default_value=0.0)
time = node(unreal.MaterialExpressionTime, -1100, 560)
roll = custom(-800, 200, "float2 uv = UV * Tiling.xy; uv.y -= Speed.y * T; uv.x += Speed.x * T + Sway * 6.0 * sin(T * 0.9 + uv.y * 2.0); return uv;",
              ["UV", "Tiling", "Speed", "Sway", "T"], unreal.CustomMaterialOutputType.CMOT_FLOAT2)
for src, pin in ((uv, "UV"), (tiling, "Tiling"), (speed, "Speed"), (sway, "Sway"), (time, "T")):
    mel.connect_material_expressions(src, "", roll, pin)
tex = node(unreal.MaterialExpressionTextureSampleParameter2D, -500, 200, parameter_name="Tex", texture=textures["T_Rain"],
           sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
mel.connect_material_expressions(roll, "", tex, "UVs")
normal = node(unreal.MaterialExpressionVertexNormalWS, -800, 500)
ends = custom(-500, 500, "return (1.0 - abs(N.z)) * sin(saturate(V.y) * 3.14159);", ["N", "V"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
mel.connect_material_expressions(normal, "", ends, "N")
mel.connect_material_expressions(uv, "", ends, "V")
intensity = node(unreal.MaterialExpressionScalarParameter, -500, 680, parameter_name="Intensity", default_value=0.0)
a = node(unreal.MaterialExpressionMultiply, -250, 300)
mel.connect_material_expressions(tex, "R", a, "A")
mel.connect_material_expressions(ends, "", a, "B")
b = node(unreal.MaterialExpressionMultiply, -100, 400)
mel.connect_material_expressions(a, "", b, "A")
mel.connect_material_expressions(intensity, "", b, "B")
soft = node(unreal.MaterialExpressionDepthFade, 80, 400, fade_distance_default=80.0)
mel.connect_material_expressions(b, "", soft, "Opacity")
mel.connect_material_property(soft, "", unreal.MaterialProperty.MP_OPACITY)
color = node(unreal.MaterialExpressionVectorParameter, -100, 100, parameter_name="Color", default_value=unreal.LinearColor(.75, .8, .88, 1))
mel.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(mat)
library.save_loaded_asset(mat)
lines.append("M_VortexPrecip ok")

# ---- os bichos
SOURCE = os.path.join(root, "Data", "Town", "Animals")   # já limpos (Tools/Blender/PrepTownAnimals.py)
for file in sorted(glob.glob(os.path.join(SOURCE, "*.glb"))):
    animal = os.path.splitext(os.path.basename(file))[0]
    if animal == "Horse":
        continue   # o jogo já tem cavalos
    dest = "/Game/Vortex/Town/Animals/" + animal
    if library.does_directory_exist(dest):
        library.delete_directory(dest)
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("scene_name_sub_folder", False)
    pipeline.set_editor_property("asset_type_sub_folders", False)
    meshes = pipeline.get_editor_property("mesh_pipeline")
    meshes.set_editor_property("import_static_meshes", False)
    meshes.set_editor_property("import_skeletal_meshes", True)
    meshes.set_editor_property("create_physics_asset", False)
    pipeline.get_editor_property("animation_pipeline").set_editor_property("import_animations", True)
    common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    common.set_editor_property("try_auto_select_skeleton", False)
    override = unreal.InterchangePipelineStackOverride()
    override.add_pipeline(pipeline)
    task = unreal.AssetImportTask()
    task.filename = file
    task.destination_path = dest
    task.automated = True
    task.replace_existing = True
    task.save = False
    task.options = override
    tools.import_asset_tasks([task])
    names = []
    for asset_path in library.list_assets(dest, recursive=True):
        asset = library.load_asset(asset_path)
        old = asset_path.split("/")[-1].split(".")[0]
        if isinstance(asset, unreal.SkeletalMesh):
            new = "SK_" + animal
        elif isinstance(asset, unreal.Skeleton):
            new = "SKEL_" + animal
        elif isinstance(asset, unreal.AnimSequence):
            act = old.split("Anim_")[-1]
            act = act[len(animal):] if act.startswith(animal) else act   # a Unreal põe o nome do bicho na frente
            new = "A_%s_%s" % (animal, act.lstrip("_"))
        elif isinstance(asset, unreal.Material):
            # sem isto a Unreal troca pelo material padrão (cinza) nas malhas animadas
            asset.set_editor_property("used_with_skeletal_mesh", True)
            mel.recompile_material(asset)
            library.save_loaded_asset(asset)
            continue
        elif isinstance(asset, unreal.Texture2D):
            asset.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)   # o atlas de cores dos cubos, nítido
            library.save_loaded_asset(asset)
            continue
        else:
            continue
        if old != new:
            library.rename_asset(asset_path.split(".")[0], dest + "/" + new)
        names.append(new)
    lines.append("%s: %s" % (animal, ", ".join(sorted(names))))

library.save_directory("/Game/Vortex/Town/Animals", only_if_is_dirty=False, recursive=True)
with open(os.path.join(root, "Saved", "Logs", "ImportTownLife.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
unreal.log_warning("VORTEX_TOWN_LIFE " + " | ".join(lines))
