"""Fauna e flora mais vivas — rode com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportWildlifeAssets.py -unattended -nullrhi

Antes: python Tools/MakeWildlifeSounds.py (Data/Sfx/Wildlife).

- Sons de Data/Sfx/Wildlife -> /Game/Vortex/Audio/Sfx (pios, corvo, gaivota, asas, peixe), na categoria Efeitos.
- /Game/Vortex/Life/M_LifeBlocks: pássaros, peixes, vitórias-régias e algas (blocos montados no C++, AVortexWildlife):
  a cor do vértice é a cor de base; vértice com alfa 0 brilha (cauda do pássaro-lume, barbatana do peixe-lume, flor).
- M_VoxelProp (plantas e pedras instanciadas) ganha 3 chaves (desligadas por padrão, ligadas só nas espécies certas):
  - Shy ("dormideira"): a planta se fecha (encolhe para a base) quando alguém passa a menos de ~1,7 m.
    Usa os pés dos personagens que a grama já recebe (MPC_GrassStep, Player0..7).
  - GlowNear: cogumelos e flores mágicas acendem quando alguém chega perto (até ~4,5 m).
  - Variety: as pétalas de cada cópia ganham uma de 4 cores (a original e 3 giros de tom); o caule verde não muda.
    A cor é o parâmetro PetalHue (0-3), sorteado pelo AVortexTerrainStreamer pelo lugar da flor.
Rodar de novo não acumula: os nós finais são marcados ("VortexFlora") e refeitos a partir do que havia antes.
Relatório em Saved/Logs/ImportWildlifeAssets.txt.
"""
import os
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
TAG = "VortexFlora"
lines = []

# ---- sons
effects = unreal.load_asset("/Game/Vortex/Audio/SC_Effects")
src = os.path.join(ROOT, "Data", "Sfx", "Wildlife")
count = 0
for file in sorted(os.listdir(src)):
    if not file.endswith(".wav"):
        continue
    name = file[:-4]
    task = unreal.AssetImportTask()
    task.filename = os.path.join(src, file)
    task.destination_path = "/Game/Vortex/Audio/Sfx"
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    wave = lib.load_asset(f"/Game/Vortex/Audio/Sfx/{name}")
    if effects:
        wave.set_editor_property("sound_class_object", effects)
    lib.save_loaded_asset(wave)
    count += 1
lines.append(f"{count} sons")


def node(material, cls, x, y, **props):
    expression = mel.create_material_expression(material, cls, x, y)
    for key, value in props.items():
        expression.set_editor_property(key, value)
    return expression


def custom(material, x, y, code, inputs, output=unreal.CustomMaterialOutputType.CMOT_FLOAT1):
    expression = node(material, unreal.MaterialExpressionCustom, x, y, output_type=output, code=code)
    pins = []
    for name in inputs:
        pin = unreal.CustomInput()
        pin.set_editor_property("input_name", name)
        pins.append(pin)
    expression.set_editor_property("inputs", pins)
    return expression


# ---- material dos bichos
full = "/Game/Vortex/Life/M_LifeBlocks"
if lib.does_asset_exist(full):
    lib.delete_asset(full)
life = tools.create_asset("M_LifeBlocks", "/Game/Vortex/Life", unreal.Material, unreal.MaterialFactoryNew())
vertex = node(life, unreal.MaterialExpressionVertexColor, -700, 0)
mel.connect_material_property(vertex, "", unreal.MaterialProperty.MP_BASE_COLOR)
# (a saída sem nome do VertexColor é só RGB: o alfa vai pela saída "A")
glow = custom(life, -400, 200, "return C * (1.0 - A) * 6.0;", ["C", "A"], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
mel.connect_material_expressions(vertex, "", glow, "C")
mel.connect_material_expressions(vertex, "A", glow, "A")
mel.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
rough = node(life, unreal.MaterialExpressionConstant, -400, 380, r=.8)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
life.set_editor_property("used_with_instanced_static_meshes", True)
mel.recompile_material(life)
lib.save_asset(full)
lines.append("M_LifeBlocks")

# ---- plantas que reagem
prop = lib.load_asset("/Game/Vortex/Materials/M_VoxelProp")
collection = lib.load_asset("/Game/Vortex/Materials/MPC_GrassStep")


def original(material, prop_id):
    """O que entrava na saída antes desta melhoria (a entrada "False" da chave marcada de uma rodada anterior)."""
    current = mel.get_material_property_input_node(material, prop_id)
    if current and current.get_editor_property("desc") == TAG:
        inputs = mel.get_inputs_for_material_expression(material, current)
        return inputs[1] if len(inputs) > 1 else None
    return current


def switch(material, x, y, name, on, off):
    expression = node(material, unreal.MaterialExpressionStaticSwitchParameter, x, y, parameter_name=name, default_value=False)
    mel.connect_material_expressions(on, "", expression, "True")
    mel.connect_material_expressions(off, "", expression, "False")
    expression.set_editor_property("desc", TAG)
    return expression


base_before = original(prop, unreal.MaterialProperty.MP_BASE_COLOR)
wpo_before = original(prop, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
emissive_before = original(prop, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
zero3 = node(prop, unreal.MaterialExpressionConstant3Vector, 600, 2400, constant=unreal.LinearColor(0, 0, 0, 1))

# quão perto está o pé mais próximo (0 longe .. 1 em cima), com o raio e a borda macia
pivot = node(prop, unreal.MaterialExpressionObjectPositionWS, -1400, 2000)
feet = []
for i in range(8):
    p = node(prop, unreal.MaterialExpressionCollectionParameter, -1400, 2100 + i * 60, collection=collection, parameter_name=f"Player{i}")
    feet.append(p)
near_code = "float b = 1e9;\n" + "".join(f"b = min(b, distance(Pivot, P{i}.xyz));\n" for i in range(8)) + "return saturate((R - b) / S);"


def nearness(x, y, radius, soft):
    expression = custom(prop, x, y, near_code, ["Pivot"] + [f"P{i}" for i in range(8)] + ["R", "S"])
    mel.connect_material_expressions(pivot, "", expression, "Pivot")
    for i, p in enumerate(feet):
        mel.connect_material_expressions(p, "", expression, f"P{i}")
    mel.connect_material_expressions(node(prop, unreal.MaterialExpressionConstant, x - 200, y + 560, r=radius), "", expression, "R")
    mel.connect_material_expressions(node(prop, unreal.MaterialExpressionConstant, x - 200, y + 620, r=soft), "", expression, "S")
    return expression


# Shy: cada ponto anda até a base (a planta se fecha) — 60% no máximo, sem sumir
world = node(prop, unreal.MaterialExpressionWorldPosition, -900, 1900,
             world_position_shader_offset=unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
toward = node(prop, unreal.MaterialExpressionSubtract, -700, 1900)
mel.connect_material_expressions(pivot, "", toward, "A")
mel.connect_material_expressions(world, "", toward, "B")
shy_near = nearness(-900, 2000, 170.0, 80.0)
shy_amount = node(prop, unreal.MaterialExpressionMultiply, -500, 2000, const_b=.6)
mel.connect_material_expressions(shy_near, "", shy_amount, "A")
shrink = node(prop, unreal.MaterialExpressionMultiply, -300, 1950)
mel.connect_material_expressions(toward, "", shrink, "A")
mel.connect_material_expressions(shy_amount, "", shrink, "B")
if wpo_before:
    wpo_on = node(prop, unreal.MaterialExpressionAdd, -100, 1950)
    mel.connect_material_expressions(wpo_before, "", wpo_on, "A")
    mel.connect_material_expressions(shrink, "", wpo_on, "B")
else:
    wpo_on = shrink
wpo = switch(prop, 200, 1950, "Shy", wpo_on, wpo_before or zero3)
mel.connect_material_property(wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

# Variety: gira o tom das pétalas (o que não é verde) em 4 opções sorteadas por cópia
# A cor é o parâmetro PetalHue (0 a 3): o AVortexTerrainStreamer sorteia pelo lugar e põe cada cor num lote à parte,
# com um material dinâmico. Os dados por cópia (PerInstanceRandom, posição, CustomData, VertexInterpolator) chegavam
# iguais para todas na cor das plantas do terreno (Nanite): as flores saíam todas da mesma cor.
pick = node(prop, unreal.MaterialExpressionScalarParameter, -500, 2900, parameter_name="PetalHue", default_value=0.0)
hue = custom(prop, -300, 2800, "\n".join([
    "float pick = floor(R + 0.5);",
    "float ang = pick < 1.0 ? 0.0 : pick < 2.0 ? 0.75 : pick < 3.0 ? -1.1 : 3.6;",
    "float3 k = float3(0.57735, 0.57735, 0.57735);",
    "float cs = cos(ang), sn = sin(ang);",
    "float3 r = C * cs + cross(k, C) * sn + k * dot(k, C) * (1.0 - cs);",
    "float m = saturate((max(C.r, C.b) - C.g) * 8.0);",
    "return max(lerp(C, r, m), 0.0);"]), ["C", "R"], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
mel.connect_material_expressions(base_before, "", hue, "C")
mel.connect_material_expressions(pick, "", hue, "R")
base = switch(prop, 200, 2800, "Variety", hue, base_before)
mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

# GlowNear: a cor da própria planta acende (até 3x) quando alguém chega perto, pulsando devagar
glow_near = nearness(-900, 3300, 450.0, 280.0)
clock = node(prop, unreal.MaterialExpressionTime, -700, 3500)
pulse = custom(prop, -500, 3450, "return N * (2.4 + 0.6 * sin(T * 3.0));", ["N", "T"])
mel.connect_material_expressions(glow_near, "", pulse, "N")
mel.connect_material_expressions(clock, "", pulse, "T")
lit = node(prop, unreal.MaterialExpressionMultiply, -300, 3400)
mel.connect_material_expressions(base_before, "", lit, "A")
mel.connect_material_expressions(pulse, "", lit, "B")
if emissive_before:
    glow_on = node(prop, unreal.MaterialExpressionAdd, -100, 3400)
    mel.connect_material_expressions(emissive_before, "", glow_on, "A")
    mel.connect_material_expressions(lit, "", glow_on, "B")
else:
    glow_on = lit
emissive = switch(prop, 200, 3400, "GlowNear", glow_on, emissive_before or zero3)
mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
# tira os nós soltos de rodadas anteriores (um VertexInterpolator solto continua sendo compilado pela Unreal)
reachable = set()
stack = [mel.get_material_property_input_node(prop, p) for p in (
    unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialProperty.MP_EMISSIVE_COLOR, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET,
    unreal.MaterialProperty.MP_ROUGHNESS, unreal.MaterialProperty.MP_METALLIC, unreal.MaterialProperty.MP_SPECULAR,
    unreal.MaterialProperty.MP_NORMAL, unreal.MaterialProperty.MP_OPACITY, unreal.MaterialProperty.MP_OPACITY_MASK,
    unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)]
while stack:
    item = stack.pop()
    if item is None or item.get_name() in reachable:
        continue
    reachable.add(item.get_name())
    stack.extend(mel.get_inputs_for_material_expression(prop, item))
removed = 0
for expression in list(mel.get_material_expressions(prop)):
    if expression and expression.get_name() not in reachable and not isinstance(expression, unreal.MaterialExpressionComment):
        mel.delete_material_expression(prop, expression)
        removed += 1
mel.recompile_material(prop)
lib.save_loaded_asset(prop)
lines.append(f"M_VoxelProp: chaves Shy, Variety e GlowNear ({removed} nós soltos removidos)")

SHY = ["flowers_bell", "flowers_daisy", "flowers_tulip", "flowers_lavender", "poppy", "dandelion", "clover", "fern",
       "moon_fern", "alpine_bloom", "elf_bloom", "frost_flower"]
GLOW = ["glowcap", "bog_lantern", "soul_flower", "crystal_sprout", "ember_bloom", "moon_fern", "elf_bloom", "frost_flower",
        "storm_crystal"]
VARIETY = ["flowers_bell", "flowers_daisy", "flowers_tulip", "flowers_lavender", "poppy", "alpine_bloom", "elf_bloom",
           "mushroom_red"]
for kind in sorted(set(SHY + GLOW + VARIETY)):
    path = f"/Game/Vortex/Props/{kind}/MI_{kind}"
    if not lib.does_asset_exist(path):
        lines.append(f"{kind}: sem MI")
        continue
    mi = lib.load_asset(path)
    parent = mi.get_editor_property("parent")
    if not parent or parent.get_path_name().split(".")[0] != "/Game/Vortex/Materials/M_VoxelProp":
        lines.append(f"{kind}: outro material ({parent.get_path_name() if parent else None})")
        continue
    flags = []
    for name, group in (("Shy", SHY), ("GlowNear", GLOW), ("Variety", VARIETY)):
        on = kind in group
        mel.set_material_instance_static_switch_parameter_value(mi, name, on)
        if on:
            flags.append(name)
    mel.update_material_instance(mi)
    lib.save_loaded_asset(mi)
    lines.append(f"{kind}: {' '.join(flags)}")

with open(os.path.join(ROOT, "Saved", "Logs", "ImportWildlifeAssets.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
unreal.log_warning("VORTEX_WILDLIFE_ASSETS " + " | ".join(lines))
