"""Importa o Azhûr refeito (Tools/Blender/BuildAzhur.py) no lugar do modelo do Godot (editor fechado).

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportAzhur.py -unattended

- apaga o modelo antigo em /Game/Vortex/Creatures/RuneColossus (malha, esqueleto, clipes, materiais);
- importa Data/VortexCreatures/RuneColossus/RuneColossus_Skinned.glb no mesmo caminho (o código do
  chefe acha a malha e os clipes RuneColossus_Skinned<Clipe> por ali);
- materiais pelos espaços: Rock (basalto com a cor dos blocos), Obsidian (vidro vulcânico, brilho
  frio), Lava (brasa que pulsa e corre) e Core (o núcleo, branco-dourado e forte).
Relatório em Saved/Logs/ImportAzhur.txt.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
DEST = "/Game/Vortex/Creatures/RuneColossus"
MATS = "/Game/Vortex/Creatures/RuneColossus/Materials"
# o Azhûr novo do Meshy, com as animações do de blocos (Tools/Blender/MeshyBiped.py); o de blocos
# (Tools/Blender/BuildAzhur.py) continua em Data/VortexCreatures/RuneColossus, é dele que saem as animações
SRC = os.path.join(root, "Data", "Modelos", "Azhur", "RuneColossus_Skinned.glb")
lines = []

# o modelo antigo do Godot (RuneColossus_Skinned, MI_*, T_*) sai antes, apagado do disco: pelo editor a
# ordem importa (sem o esqueleto os clipes não abrem para apagar)

# ---- importa o novo (já importado: só refaz os materiais; para reimportar, apague a pasta RuneColossus_Skinned)
if not library.does_asset_exist(DEST + "/RuneColossus_Skinned/SkeletalMeshes/RuneColossus_Skinned"):
    task = unreal.AssetImportTask()
    task.filename = SRC
    task.destination_path = DEST
    task.automated = True
    task.save = True
    task.replace_existing = True
    tools.import_asset_tasks([task])


def node(material, kind, x, y, **props):
    expression = mel.create_material_expression(material, kind, x, y)
    for key, value in props.items():
        expression.set_editor_property(key, value)
    return expression


def link(a, b, pin_b="", pin_a=""):
    mel.connect_material_expressions(a, pin_a, b, pin_b)


def fresh(name):
    path = MATS + "/" + name
    material = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset(name, MATS, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(material)
    material.set_editor_property("used_with_skeletal_mesh", True)
    # também nos paredões e agulhas da cratera (malhas instanciadas: sem isso aparece o xadrez padrão)
    material.set_editor_property("used_with_instanced_static_meshes", True)
    return material


def done(material):
    mel.recompile_material(material)
    library.save_loaded_asset(material)
    lines.append("material " + material.get_name())


# pedra: cor do bloco (cor do vértice: as lascas mais claras e mais escuras) x tom de basalto, fosca;
# a borda brilha de calor (laranja fraco), para o corpo não virar silhueta contra o céu vermelho
rock = fresh("M_AzhurRock")
vc = node(rock, unreal.MaterialExpressionVertexColor, -700, 0)
tone = node(rock, unreal.MaterialExpressionConstant3Vector, -700, 160, constant=unreal.LinearColor(.2, .155, .13, 1))
mul = node(rock, unreal.MaterialExpressionMultiply, -450, 60)
link(vc, mul, "A")
link(tone, mul, "B")
mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(node(rock, unreal.MaterialExpressionConstant, -450, 220, r=.92), "", unreal.MaterialProperty.MP_ROUGHNESS)
fres = node(rock, unreal.MaterialExpressionFresnel, -700, 320, exponent=3.0, base_reflect_fraction=0.0)
heat = node(rock, unreal.MaterialExpressionConstant3Vector, -700, 440, constant=unreal.LinearColor(.55, .16, .03, 1))
glow = node(rock, unreal.MaterialExpressionMultiply, -450, 360)
link(fres, glow, "A")
link(heat, glow, "B")
lit = node(rock, unreal.MaterialExpressionMultiply, -250, 380)
link(glow, lit, "A")
link(vc, lit, "B")
mel.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
done(rock)

# obsidiana: quase preta, lisa, um reflexo roxo na borda
obs = fresh("M_AzhurObsidian")
vc = node(obs, unreal.MaterialExpressionVertexColor, -700, 0)
tone = node(obs, unreal.MaterialExpressionConstant3Vector, -700, 160, constant=unreal.LinearColor(.035, .028, .05, 1))
mul = node(obs, unreal.MaterialExpressionMultiply, -450, 60)
link(vc, mul, "A")
link(tone, mul, "B")
mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(node(obs, unreal.MaterialExpressionConstant, -450, 200, r=.22), "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(node(obs, unreal.MaterialExpressionConstant, -450, 260, r=.35), "", unreal.MaterialProperty.MP_SPECULAR)
fres = node(obs, unreal.MaterialExpressionFresnel, -700, 320, exponent=4.0, base_reflect_fraction=0.0)
rim = node(obs, unreal.MaterialExpressionMultiply, -450, 340, const_b=1.0)
link(fres, rim, "A")
purple = node(obs, unreal.MaterialExpressionConstant3Vector, -700, 440, constant=unreal.LinearColor(.6, .15, 1.2, 1))
rim2 = node(obs, unreal.MaterialExpressionMultiply, -250, 360)
link(rim, rim2, "A")
link(purple, rim2, "B")
mel.connect_material_property(rim2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
done(obs)


def glowing(name, color, strength, speed, low):
    """Brasa: cor forte x pulso (seno no tempo) x um ruído que corre pela peça."""
    m = fresh(name)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    time = node(m, unreal.MaterialExpressionTime, -1100, 0)
    fast = node(m, unreal.MaterialExpressionMultiply, -950, 0, const_b=speed)
    link(time, fast, "A")
    wpos = node(m, unreal.MaterialExpressionWorldPosition, -1100, 160)
    mask = node(m, unreal.MaterialExpressionComponentMask, -950, 160, r=False, g=False, b=True, a=False)
    link(wpos, mask)
    height = node(m, unreal.MaterialExpressionMultiply, -800, 160, const_b=.012)
    link(mask, height, "A")
    wave = node(m, unreal.MaterialExpressionAdd, -650, 80)
    link(fast, wave, "A")
    link(height, wave, "B")
    sine = node(m, unreal.MaterialExpressionSine, -500, 80)
    link(wave, sine)
    half = node(m, unreal.MaterialExpressionMultiply, -350, 80, const_b=(1 - low) * .5)
    link(sine, half, "A")
    pulse = node(m, unreal.MaterialExpressionAdd, -200, 80, const_b=(1 + low) * .5)
    link(half, pulse, "A")
    tint = node(m, unreal.MaterialExpressionVectorParameter, -500, -160, parameter_name="Glow",
                default_value=unreal.LinearColor(color[0] * strength, color[1] * strength, color[2] * strength, 1))
    out = node(m, unreal.MaterialExpressionMultiply, -50, -40)
    link(tint, out, "A")
    link(pulse, out, "B")
    mel.connect_material_property(out, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    done(m)
    return m


lava = glowing("M_AzhurLava", (1.0, .24, .02), 4.0, 2.3, .55)
core = glowing("M_AzhurCore", (1.0, .62, .22), 9.0, 4.0, .7)

# o modelo do Meshy: a textura dele de cor, fosca; as rachaduras de lava e o núcleo (laranja) e as runas (ciano) brilham
# pela própria cor (onde o vermelho passa muito do azul, ou o azul do vermelho)
painted = None
texture = next((library.load_asset(p) for p in library.list_assets(DEST, recursive=True)
                if str(library.find_asset_data(p).asset_class_path.asset_name) == "Texture2D" and "Materials/" not in p
                and library.load_asset(p).get_editor_property("srgb")), None)
if texture:
    texture.set_editor_property("max_texture_size", 4096)
    library.save_loaded_asset(texture)
    painted = fresh("M_AzhurMeshy")
    base = node(painted, unreal.MaterialExpressionTextureSampleParameter2D, -1100, 0, parameter_name="Base", texture=texture)
    mel.connect_material_property(base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(node(painted, unreal.MaterialExpressionConstant, -450, 120, r=.85), "", unreal.MaterialProperty.MP_ROUGHNESS)
    r = node(painted, unreal.MaterialExpressionComponentMask, -900, 200, r=True, g=False, b=False, a=False)
    b = node(painted, unreal.MaterialExpressionComponentMask, -900, 300, r=False, g=False, b=True, a=False)
    link(base, r)
    link(base, b)
    hot = node(painted, unreal.MaterialExpressionSubtract, -750, 220)
    link(r, hot, "A")
    link(b, hot, "B")
    cold = node(painted, unreal.MaterialExpressionSubtract, -750, 320)
    link(b, cold, "A")
    link(r, cold, "B")
    def mask(src, y, offset, gain):
        shifted = node(painted, unreal.MaterialExpressionAdd, -600, y, const_b=-offset)
        link(src, shifted, "A")
        scaled_ = node(painted, unreal.MaterialExpressionMultiply, -480, y, const_b=gain)
        link(shifted, scaled_, "A")
        clamp = node(painted, unreal.MaterialExpressionSaturate, -360, y)
        link(scaled_, clamp)
        return clamp
    lava_mask = mask(hot, 220, .35, 4.0)
    rune_mask = mask(cold, 320, .08, 6.0)
    lava_glow = node(painted, unreal.MaterialExpressionMultiply, -220, 220, const_b=7.0)
    link(lava_mask, lava_glow, "A")
    rune_glow = node(painted, unreal.MaterialExpressionMultiply, -220, 320, const_b=4.0)
    link(rune_mask, rune_glow, "A")
    # 02/11: CoreHeat (padrão 1) aquece as rachaduras e o núcleo; o UVortexVolcanicBoss sobe na fase 2
    heat = node(painted, unreal.MaterialExpressionScalarParameter, -360, 160, parameter_name="CoreHeat", default_value=1.0)
    hot_glow = node(painted, unreal.MaterialExpressionMultiply, -160, 200)
    link(lava_glow, hot_glow, "A")
    link(heat, hot_glow, "B")
    both = node(painted, unreal.MaterialExpressionAdd, -100, 260)
    link(hot_glow, both, "A")
    link(rune_glow, both, "B")
    emit = node(painted, unreal.MaterialExpressionMultiply, 0, 120)
    link(base, emit, "A", "RGB")
    link(both, emit, "B")
    mel.connect_material_property(emit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    done(painted)

by_slot = {"Rock": rock, "Obsidian": obs, "Lava": lava, "Core": core}
meshes = [p for p in library.list_assets(DEST, recursive=True)
          if str(library.find_asset_data(p).asset_class_path.asset_name) == "SkeletalMesh"]
for path in meshes:
    mesh = library.load_asset(path)
    # a lista vem copiada: monta uma nova com o material de cada espaço (mexer nos itens da cópia não salva)
    fixed = []
    for slot in mesh.get_editor_property("materials"):
        name = str(slot.get_editor_property("material_slot_name"))
        chosen = next((m for key, m in by_slot.items() if name.startswith(key)), painted or slot.get_editor_property("material_interface"))
        slot.set_editor_property("material_interface", chosen)
        fixed.append(slot)
        lines.append("espaço %s -> %s" % (name, chosen.get_name() if chosen else "nenhum"))
    mesh.set_editor_property("materials", fixed)
    library.save_loaded_asset(mesh)
    lines.append("malha " + path)
clips = [p.split("/")[-1].split(".")[0] for p in library.list_assets(DEST, recursive=True)
         if str(library.find_asset_data(p).asset_class_path.asset_name) == "AnimSequence"]
lines.append("clipes: " + ", ".join(sorted(clips)))
# sobras do importador (materiais e texturas do glTF) fora da pasta Materials
for path in library.list_assets(DEST, recursive=True):
    kind = str(library.find_asset_data(path).asset_class_path.asset_name)
    if "/Materials/M_Azhur" in path or (texture and path.split(".")[0] == texture.get_path_name().split(".")[0]):
        continue
    if kind in ("Material", "MaterialInstanceConstant", "Texture2D") and not library.find_package_referencers_for_asset(path):
        library.delete_asset(path)
with open(os.path.join(root, "Saved", "Logs", "ImportAzhur.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
