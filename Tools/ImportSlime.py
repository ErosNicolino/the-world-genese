"""O slime novo na Unreal (Tools/Blender/BuildSlime.py) — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportSlime.py -unattended -nullrhi

- Data/Creatures/Slime/SM_Slime.glb -> /Game/Vortex/Creatures/SlimeNew/SM_Slime (malha parada, com a cor do vértice);
- M_SlimeJelly (gelatina translúcida, borda mais opaca, brilho por dentro) e M_SlimeSolid (opaco: miolo e o slime de
  ferro). Os dois deformam a malha no próprio material (VortexSlimeLook manda os números pelos dados
  primitivos do componente, um envio só para todos os espaços):
    0 amassado (0 normal, + achata e alarga, - estica para cima), 1 esticada do ataque (cm), 2 balanço (cm),
    3-5 direção da esticada (mundo), 7 altura do corpo (cm), 8 piscar (0 aberto, 0,9 fechado), 9-10 o quanto o alto
    do corpo fica para trás (inércia, cm no mundo; 03/11), 11 humor (-1 bravo, 1 contente), 12 boca aberta, 13 língua,
    14 sobrancelha brava;
  08/11: o rosto é desenhado na própria gelatina (FACE, pelo UV da malha), para amassar e esticar junto com ela;
  03/11 (o usuário: "bem amoeba, estilo Slime Rancher"): a superfície ondula sempre devagar (cada gota num tempo) e o
  braço do bote ondula;
  a cor do vértice (R) diz o quanto cada parte acompanha a esticada;
- uma instância por tipo (normal, fogo, veneno, ferro) para o corpo e o miolo, e as dos olhos e do brilho.
Relatório em Saved/Logs/ImportSlime.txt.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
DEST = "/Game/Vortex/Creatures/SlimeNew"
lines = []

WPO = """
float3 c = float3(0, 0, H * 0.5);
float3 p = Rel - c;
float low = 1.0 - saturate(Rel.z / max(H, 1.0));
float3 o = float3(Rel.x * Squash * (0.3 + 0.5 * low), Rel.y * Squash * (0.3 + 0.5 * low), -Rel.z * Squash);
float dl = length(Dir);
float3 d = dl > 0.001 ? Dir / dl : float3(1, 0, 0);
float r = max(length(p), 1.0);
float t = saturate(dot(p / r, d));
float w = t * t * t * Follow;
float3 lat = p - d * dot(p, d);
o += d * Len * w - lat * w * saturate(Len / 120.0) * 0.6;
o.z += sin(t * 9.0 - T * 16.0) * w * saturate(Len / 60.0) * 4.0;
float hz = saturate(Rel.z / max(H, 1.0));
o.xy += Lean * hz * hz;
float ph = dot(Obj.xy, float2(0.013, 0.021));
float3 n = p / r;
float jig = 2.0 + Wob * 0.8;
o += n * jig * (sin(T * 3.1 + n.x * 3.0 + n.y * 2.0 + ph) * 0.6 + sin(T * 4.7 - n.y * 3.5 + n.z * 2.5 + ph * 1.7) * 0.4) * (0.35 + 0.65 * hz);
float pl = max(length(p.xy), 1.0);
o.xy += (p.xy / pl) * sin(T * 6.0 + Rel.z * 0.09) * Wob * saturate(Rel.z / max(H, 1.0));
return o;
"""


def scalar(mat, name, value, x, y, cpd=None):
    e = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    if cpd is not None:
        e.set_editor_property("use_custom_primitive_data", True)
        e.set_editor_property("primitive_data_index", cpd)
    return e


def vector(mat, name, value, x, y, cpd=None):
    e = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", unreal.LinearColor(*value))
    if cpd is not None:
        e.set_editor_property("use_custom_primitive_data", True)
        e.set_editor_property("primitive_data_index", cpd)
    return e


def deform(mat):
    """O nó de deformação (HLSL) ligado na posição do vértice (World Position Offset)."""
    world = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1400, 600)
    obj = mel.create_material_expression(mat, unreal.MaterialExpressionObjectPositionWS, -1400, 700)
    rel = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -1200, 640)
    mel.connect_material_expressions(world, "", rel, "A")
    mel.connect_material_expressions(obj, "", rel, "B")
    squash = scalar(mat, "Squash", 0.0, -1200, 760, 0)
    length = scalar(mat, "StretchLen", 0.0, -1200, 840, 1)
    wob = scalar(mat, "Wobble", 0.0, -1200, 920, 2)
    direction = vector(mat, "StretchDir", (1, 0, 0, 0), -1200, 1000, 3)
    height = scalar(mat, "BodyHeight", 70.0, -1200, 1160, 7)
    follow = mel.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -1200, 1240)
    time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -1200, 1320)
    lean_x = scalar(mat, "LeanX", 0.0, -1400, 1480, 9)
    lean_y = scalar(mat, "LeanY", 0.0, -1400, 1560, 10)
    lean = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -1200, 1500)
    mel.connect_material_expressions(lean_x, "", lean, "A")
    mel.connect_material_expressions(lean_y, "", lean, "B")
    custom = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -900, 800)
    custom.set_editor_property("code", WPO)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property("description", "SlimeDeform")
    inputs = []
    for name in ("Rel", "Squash", "Len", "Wob", "Dir", "H", "Follow", "T", "Lean", "Obj"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", name)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)
    mel.connect_material_expressions(rel, "", custom, "Rel")
    mel.connect_material_expressions(squash, "", custom, "Squash")
    mel.connect_material_expressions(length, "", custom, "Len")
    mel.connect_material_expressions(wob, "", custom, "Wob")
    mel.connect_material_expressions(direction, "RGB", custom, "Dir")
    mel.connect_material_expressions(height, "", custom, "H")
    mel.connect_material_expressions(follow, "R", custom, "Follow")
    mel.connect_material_expressions(time, "", custom, "T")
    mel.connect_material_expressions(lean, "", custom, "Lean")
    mel.connect_material_expressions(obj, "", custom, "Obj")
    mel.connect_material_property(custom, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)


FACE = """
if (Show < 0.5 || F < 0.5) return float4(0.0, 0.0, 0.0, 0.0);
float x = (uv.x - 0.5) * 1.25;
float y = 1.0 - uv.y;   // a Unreal lê o V do glTF de cima para baixo
float cx = (x < 0.0 ? -1.0 : 1.0) * 0.2;
float2 e = float2(abs(x) - 0.2, y - 0.61);
float ry = max(0.016, 0.155 * (1.0 - Blink));
float d = length(e / float2(0.1, ry));
float lid = 0.16 - Angry * (0.08 - e.x * 0.85);
float eye = (1.0 - smoothstep(0.9, 1.0, d)) * step(e.y, lid);
float bright = step(0.3, 1.0 - Blink) * eye;
float shine = (1.0 - smoothstep(0.024, 0.032, length(float2(x - cx + 0.035, y - 0.665)))) * bright;
shine = max(shine, (1.0 - smoothstep(0.012, 0.02, length(float2(x - cx - 0.03, y - 0.565)))) * bright);
float cheek = (1.0 - smoothstep(0.65, 1.0, length(float2(abs(x) - 0.35, y - 0.46) / float2(0.08, 0.042)))) * saturate(Mood) * 0.5;
float gloss = (1.0 - smoothstep(0.7, 1.0, length(float2(x + 0.24, y - 0.86) / float2(0.13, 0.05)))) * 0.5;
float2 q = float2(x, y - 0.39) / 0.2;
float ax = abs(q.x);
float w = 0.62 + 0.08 * Open;
float edge = 1.0 - smoothstep(w - 0.06, w, ax);
float mx = q.x / w;
float mtop = Mood * 0.38 * (mx * mx) - 0.08 * Mood + 0.05 * (1.0 - Open);
float mlow = mtop - Open * 0.75 * saturate(1.0 - mx * mx);
float stroke = (1.0 - smoothstep(0.05, 0.09, abs(q.y - mtop))) * edge;
float inside = step(mlow, q.y) * step(q.y, mtop) * edge * step(0.02, Open);
float tongueY = mlow + max(Open, Tongue * 0.5) * 0.28;
float tongue = inside * step(q.y, tongueY) * (1.0 - smoothstep(0.2, 0.32, ax));
float lick = Tongue * (1.0 - smoothstep(0.1, 0.16, length((q - float2(0.08, mtop - 0.12 * Tongue)) * float2(1.0, 0.8))));
float3 dark = float3(0.015, 0.015, 0.03);
float3 c = float3(1.0, 0.42, 0.52);
float a = cheek;
if (gloss > a) { c = float3(1.0, 1.0, 1.0); a = gloss; }
c = lerp(c, dark, eye);
a = max(a, eye);
c = lerp(c, float3(1.0, 1.0, 1.0), shine);
float m = saturate(max(max(stroke, inside), lick));
c = lerp(c, lerp(dark, float3(0.85, 0.18, 0.25), saturate(tongue + lick)), m);
return float4(c, saturate(max(a, m)));
"""


def add_face(mat, color, emit, opacity=None):
    """O rosto entra na cor, no brilho e (na gelatina) na opacidade."""
    node = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -700, -500)
    node.set_editor_property("code", FACE)
    node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    node.set_editor_property("description", "SlimeFace")
    uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1000, -700)
    vertex = mel.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -1000, -620)
    show = scalar(mat, "ShowFace", 1.0, -1000, -540)
    sources = {"uv": (uv, ""), "F": (vertex, "B"), "Show": (show, ""), "Blink": (scalar(mat, "Blink", 0.0, -1000, -460, 8), ""),
               "Mood": (scalar(mat, "Mood", 1.0, -1000, -380, 11), ""), "Open": (scalar(mat, "Open", 0.0, -1000, -300, 12), ""),
               "Tongue": (scalar(mat, "Tongue", 0.0, -1000, -220, 13), ""), "Angry": (scalar(mat, "Angry", 0.0, -1000, -140, 14), "")}
    entries = []
    for name in sources:
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        entries.append(entry)
    node.set_editor_property("inputs", entries)
    for name, (source, pin) in sources.items():
        mel.connect_material_expressions(source, pin, node, name)
    rgb = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -450, -560)
    for channel in "rgb":
        rgb.set_editor_property(channel, True)
    alpha = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -450, -460)
    for channel, on in (("r", False), ("g", False), ("b", False), ("a", True)):
        alpha.set_editor_property(channel, on)
    mel.connect_material_expressions(node, "", rgb, "")
    mel.connect_material_expressions(node, "", alpha, "")
    def mix(target, a_node, y, pin=""):
        lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -250, y)
        mel.connect_material_expressions(a_node, pin, lerp, "A")
        mel.connect_material_expressions(rgb, "", lerp, "B")
        mel.connect_material_expressions(alpha, "", lerp, "Alpha")
        mel.connect_material_property(lerp, "", target)
    mix(unreal.MaterialProperty.MP_BASE_COLOR, color, -400, "RGB")
    mix(unreal.MaterialProperty.MP_EMISSIVE_COLOR, emit, -300)
    if opacity is not None:
        top = mel.create_material_expression(mat, unreal.MaterialExpressionMax, -250, -200)
        mel.connect_material_expressions(opacity, "", top, "A")
        mel.connect_material_expressions(alpha, "", top, "B")
        mel.connect_material_property(top, "", unreal.MaterialProperty.MP_OPACITY)


def material(name):
    path = DEST + "/" + name
    mat = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    return mat


# ---- gelatina: translúcida, a borda mais opaca (fresnel), brilho por dentro, bem lisa e brilhante
jelly = material("M_SlimeJelly")
jelly.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
jelly.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
color = vector(jelly, "Color", (.15, .75, .45, 1), -700, -200)
glow = scalar(jelly, "Glow", .25, -700, -60)
fres = mel.create_material_expression(jelly, unreal.MaterialExpressionFresnel, -700, 60)
fres.set_editor_property("exponent", 2.6)
edge = scalar(jelly, "EdgeOpacity", .95, -700, 180)
center = scalar(jelly, "Opacity", .55, -700, 260)
opacity = mel.create_material_expression(jelly, unreal.MaterialExpressionLinearInterpolate, -450, 200)
mel.connect_material_expressions(center, "", opacity, "A")
mel.connect_material_expressions(edge, "", opacity, "B")
mel.connect_material_expressions(fres, "", opacity, "Alpha")
emit = mel.create_material_expression(jelly, unreal.MaterialExpressionMultiply, -450, -120)
mel.connect_material_expressions(color, "RGB", emit, "A")
mel.connect_material_expressions(glow, "", emit, "B")
rim = mel.create_material_expression(jelly, unreal.MaterialExpressionMultiply, -300, -20)
mel.connect_material_expressions(fres, "", rim, "A")
mel.connect_material_expressions(color, "RGB", rim, "B")
emit_all = mel.create_material_expression(jelly, unreal.MaterialExpressionAdd, -150, -80)
mel.connect_material_expressions(emit, "", emit_all, "A")
mel.connect_material_expressions(rim, "", emit_all, "B")
rough = scalar(jelly, "Roughness", .08, -450, 340)
metal = scalar(jelly, "Metallic", 0.0, -450, 420)
add_face(jelly, color, emit_all, opacity)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
deform(jelly)
mel.recompile_material(jelly)
library.save_loaded_asset(jelly)

# ---- sólido: olhos, brilho, miolo e o slime de ferro
solid = material("M_SlimeSolid")
color = vector(solid, "Color", (.05, .05, .06, 1), -700, -200)
glow = scalar(solid, "Glow", 0.0, -700, -60)
emit = mel.create_material_expression(solid, unreal.MaterialExpressionMultiply, -450, -120)
mel.connect_material_expressions(color, "RGB", emit, "A")
mel.connect_material_expressions(glow, "", emit, "B")
rough = scalar(solid, "Roughness", .15, -450, 340)
metal = scalar(solid, "Metallic", 0.0, -450, 420)
add_face(solid, color, emit)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
deform(solid)
mel.recompile_material(solid)
library.save_loaded_asset(solid)


def instance(name, parent, values):
    path = DEST + "/" + name
    mi = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset(name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    for key, value in values.items():
        if isinstance(value, tuple):
            mel.set_material_instance_vector_parameter_value(mi, key, unreal.LinearColor(*value))
        else:
            mel.set_material_instance_scalar_parameter_value(mi, key, value)
    library.save_loaded_asset(mi)
    return mi


# tipos: o corpo e o miolo de cada um
TYPES = {
    "Normal": (jelly, {"Color": (.12, .78, .42, 1), "Glow": .25, "Opacity": .5},
               {"Color": (.12, .78, .42, 1), "Glow": .3}),
    "Fogo": (jelly, {"Color": (1.0, .33, .04, 1), "Glow": .6, "Opacity": .7, "EdgeOpacity": 1.0},
             {"Color": (1.0, .75, .2, 1), "Glow": 3.0}),
    "Veneno": (jelly, {"Color": (.42, .08, .7, 1), "Glow": .5, "Opacity": .6},
               {"Color": (.85, .3, 1.0, 1), "Glow": 3.5}),
    "Ferro": (solid, {"Color": (.52, .54, .58, 1), "Metallic": .85, "Roughness": .32, "Glow": 0.0},
              {"Color": (1.0, .45, .1, 1), "Glow": 2.0}),
}
for kind, (parent, body, core) in TYPES.items():
    instance("MI_Slime_%s_Body" % kind, parent, body)
    instance("MI_Slime_%s_Core" % kind, solid, core)
# as bolhas de veneno da gota peçonhenta (VortexSlimeLook): roxas, brilhando
instance("MI_Slime_Veneno_Bubble", jelly, {"Color": (.7, .2, 1.0, 1), "Glow": 1.8, "Opacity": .7, "ShowFace": 0.0})

# ---- a malha
task = unreal.AssetImportTask()
task.filename = os.path.join(root, "Data", "Creatures", "Slime", "SM_Slime.glb")
task.destination_path = DEST
task.automated = True
task.save = True
task.replace_existing = True
tools.import_asset_tasks([task])
mesh = None
for p in library.list_assets(DEST, recursive=True):
    asset = library.load_asset(p)
    if isinstance(asset, unreal.StaticMesh):
        mesh = asset
if mesh:
    settings = mesh.get_editor_property("nanite_settings")
    settings.set_editor_property("enabled", False)
    mesh.set_editor_property("nanite_settings", settings)
    slots = mesh.get_editor_property("static_materials")
    names = []
    for i, slot in enumerate(slots):
        n = str(slot.get_editor_property("material_slot_name"))
        names.append(n)
        mi = library.load_asset(DEST + "/MI_Slime_Normal_%s" % ("Body" if "Body" in n else "Core"))
        mesh.set_material(i, mi)
    library.save_loaded_asset(mesh)
    box = mesh.get_bounding_box()
    lines.append("SM_Slime %s espaços %s caixa %s .. %s" % (mesh.get_path_name(), names, box.min, box.max))
else:
    lines.append("FALHOU a malha")

# as peças do rosto antigo (08/11: o rosto agora é desenhado na gelatina)
for old in ("M_SlimeFace", "MI_Slime_Eye", "MI_Slime_Shine"):
    if library.does_asset_exist(DEST + "/" + old):
        library.delete_asset(DEST + "/" + old)

# sobras do glTF (materiais e texturas que o importador cria)
for p in library.list_assets(DEST, recursive=True):
    data = library.find_asset_data(p)
    kind = str(data.asset_class_path.asset_name)
    name = p.split("/")[-1]
    if kind in ("MaterialInstanceConstant", "Material", "Texture2D") and not name.startswith(("M_Slime", "MI_Slime")) \
            and not library.find_package_referencers_for_asset(p):
        library.delete_asset(p)

with open(os.path.join(root, "Saved", "Logs", "ImportSlime.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
unreal.log_warning("VORTEX_SLIME_IMPORT " + " | ".join(lines))
