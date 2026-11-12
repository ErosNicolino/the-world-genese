"""Todos os sons de efeito do Godot e as categorias de volume do Vortex — rode com o editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportSoundsAndClasses.py -unattended

- Assets/Sfx/<nome>.wav do Godot -> /Game/Vortex/Audio/Sfx/<nome> (os que ainda não estavam: criaturas,
  chefes, magias, missões, diálogo, moedas...). Os ambientes (amb_*) vão para /Game/Vortex/Audio.
- Categorias (Sound Class) filhas do Master da engine: SC_Music, SC_Effects, SC_Ambience e
  SC_Interface, e o mix SM_VortexVolumes. Cada som do Vortex recebe a sua categoria:
  Music/ -> Música, UI/ -> Interface, amb_* -> Ambiente, o resto -> Efeitos.
  As opções de áudio mudam o volume de cada categoria em tempo real (UVortexAudioSettings).
- M_Telegraph: o aviso no chão dos golpes de área (círculo ou anel que enche até o impacto).
Relatório em Saved/Logs/ImportSounds.txt.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
godot = os.path.normpath(os.path.join(root, "..", "..", "the-world-geneses", "Assets", "Sfx"))
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
lines = []

# ---- sons -----------------------------------------------------------------------------------------
existing_ui = {"ui_click", "ui_close", "ui_error", "ui_open", "equip", "level_up"}
imported = 0
for file in sorted(os.listdir(godot)):
    if not file.endswith(".wav"):
        continue
    name = file[:-4]
    folder = "/Game/Vortex/Audio" if name.startswith("amb_") else "/Game/Vortex/Audio/UI" if name in existing_ui else "/Game/Vortex/Audio/Sfx"
    if library.does_asset_exist(folder + "/" + name):
        continue
    task = unreal.AssetImportTask()
    task.filename = os.path.join(godot, file)
    task.destination_path = folder
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    wave = library.load_asset(folder + "/" + name)
    if not wave:
        lines.append("FALHOU " + name)
        continue
    wave.set_editor_property("looping", name.startswith("amb_"))
    library.save_loaded_asset(wave)
    imported += 1
lines.append("%d sons novos" % imported)

# ---- categorias -----------------------------------------------------------------------------------
master = unreal.load_asset("/Engine/EngineSounds/Master")
classes = {}
for key in ("Music", "Effects", "Ambience", "Interface"):
    path = "/Game/Vortex/Audio/SC_" + key
    sound_class = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset("SC_" + key, "/Game/Vortex/Audio", unreal.SoundClass, unreal.SoundClassFactory())
    if master:
        sound_class.set_editor_property("parent_class", master)
    library.save_loaded_asset(sound_class)
    classes[key] = sound_class
mix_path = "/Game/Vortex/Audio/SM_VortexVolumes"
mix = library.load_asset(mix_path) if library.does_asset_exist(mix_path) else \
    tools.create_asset("SM_VortexVolumes", "/Game/Vortex/Audio", unreal.SoundMix, unreal.SoundMixFactory())
adjusters = []
for sound_class in classes.values():
    adjuster = unreal.SoundClassAdjuster()
    adjuster.set_editor_property("sound_class_object", sound_class)
    adjuster.set_editor_property("volume_adjuster", 1.0)
    adjusters.append(adjuster)
mix.set_editor_property("sound_class_effects", adjusters)
library.save_loaded_asset(mix)

counts = {k: 0 for k in classes}
for path in library.list_assets("/Game/Vortex/Audio", recursive=True):
    data = library.find_asset_data(path)
    if str(data.asset_class_path.asset_name) != "SoundWave":
        continue
    name = str(data.asset_name)
    key = "Music" if "/Music/" in path else "Interface" if "/UI/" in path else "Ambience" if name.startswith("amb_") else "Effects"
    wave = library.load_asset(path)
    wave.set_editor_property("sound_class_object", classes[key])
    library.save_loaded_asset(wave)
    counts[key] += 1
lines.append("categorias: " + ", ".join("%s=%d" % kv for kv in counts.items()))

# ---- M_Telegraph ----------------------------------------------------------------------------------
mel = unreal.MaterialEditingLibrary
path = "/Game/Vortex/Combat/M_Telegraph"
material = library.load_asset(path) if library.does_asset_exist(path) else \
    tools.create_asset("M_Telegraph", "/Game/Vortex/Combat", unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(material)
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property("two_sided", True)
uv = mel.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
center = mel.create_material_expression(material, unreal.MaterialExpressionConstant2Vector, -1400, 150)
center.set_editor_property("r", 0.5)
center.set_editor_property("g", 0.5)
distance = mel.create_material_expression(material, unreal.MaterialExpressionDistance, -1200, 50)
mel.connect_material_expressions(uv, "", distance, "A")
mel.connect_material_expressions(center, "", distance, "B")
radius = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -1050, 50)   # 0 no centro, 1 na borda
radius.set_editor_property("const_b", 2.0)
mel.connect_material_expressions(distance, "", radius, "A")
progress = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -1400, 300)
progress.set_editor_property("parameter_name", "Progress")
ring = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -1400, 420)
ring.set_editor_property("parameter_name", "Ring")
color = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -600, -250)
color.set_editor_property("parameter_name", "Color")
color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.4, 0.15, 1.0))
# dentro do disco (r < 1) e, no anel, fora do miolo (r > 0,45)
inside = mel.create_material_expression(material, unreal.MaterialExpressionCustom, -800, 100)
inside.set_editor_property("code",
    "float edge = 1 - smoothstep(0.96, 1.0, R);\n"
    "float hole = Ring > 0.5 ? smoothstep(0.43, 0.47, R) : 1;\n"
    "float area = edge * hole;\n"
    "float border = (1 - smoothstep(0.90, 0.97, R)) * 0 + smoothstep(0.90, 0.97, R) * edge;\n"
    "float inner = Ring > 0.5 ? smoothstep(0.45, 0.49, R) * (1 - smoothstep(0.49, 0.53, R)) : 0;\n"
    "float fill = (Ring > 0.5 ? step((R - 0.45) / 0.55, P) : step(R, P)) * area;\n"
    "return saturate(area * 0.18 + fill * 0.32 + border * 0.85 + inner * 0.6);")
inside.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
inputs = []
for input_name in ("R", "P", "Ring"):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", input_name)
    inputs.append(ci)
inside.set_editor_property("inputs", inputs)
mel.connect_material_expressions(radius, "", inside, "R")
mel.connect_material_expressions(progress, "", inside, "P")
mel.connect_material_expressions(ring, "", inside, "Ring")
glow = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -400, -150)
glow.set_editor_property("const_b", 4.0)
mel.connect_material_expressions(color, "", glow, "A")
mel.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(inside, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(material)
library.save_loaded_asset(material)
lines.append("M_Telegraph pronto")

with open(os.path.join(root, "Saved", "Logs", "ImportSounds.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
