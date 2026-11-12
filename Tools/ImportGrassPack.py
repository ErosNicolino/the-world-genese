"""Grama da Quaternius no jogo (10/11): /Game/Vortex/Nature/Grass com T_GrassPack, M_GrassPack e os 4 tufos
SM_GrassPack_* (de Tools/Blender/MakeGrassPack.py). Quando os 4 existem, o AVortexTerrainStreamer usa estes no lugar
das touceiras feitas por código (BuildGrassMeshes).

M_GrassPack: só a forma vem do pacote. A cor nasce da cor do chão naquele ponto (dados por cópia 0-2, como no
M_GrassWind), em 4 degraus de tom pela altura (v da UV = 1 na raiz e 0 na ponta), com palha nas pontas onde a região é
seca e as mesmas manchas grandes do chão (T_MacroNoise). O vento e a pisada são os do M_GrassWind.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/ImportGrassPack.py (editor fechado)
"""
import os
import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
PACK = os.path.join(ROOT, "modelando ainda", "quaternius_natureza")
DEST = "/Game/Vortex/Nature/Grass"
tools = unreal.AssetToolsHelpers.get_asset_tools()


def import_file(path, name, options=None):
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = DEST
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    if options:
        task.options = options
    tools.import_asset_tasks([task])
    asset = EAL.load_asset(DEST + "/" + name)
    if not asset:
        raise RuntimeError("não importou: " + path)
    return asset


texture = import_file(os.path.join(PACK, "pacote", "Textures", "Grass.png"), "T_GrassPack")
texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
EAL.save_loaded_asset(texture)

MAT = DEST + "/M_GrassPack"
if EAL.does_asset_exist(MAT):
    EAL.delete_asset(MAT)
material = tools.create_asset("M_GrassPack", DEST, unreal.Material, unreal.MaterialFactoryNew())
material.set_editor_property("two_sided", True)
material.set_editor_property("used_with_instanced_static_meshes", True)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)


def custom(x, y, name, code, inputs, output=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionCustom, x, y)
    node.set_editor_property("output_type", output)
    node.set_editor_property("description", name)
    node.set_editor_property("code", code)
    items = []
    for input_name, _ in inputs:
        item = unreal.CustomInput()
        item.set_editor_property("input_name", input_name)
        items.append(item)
    node.set_editor_property("inputs", items)
    for input_name, source in inputs:
        out = "RGBA" if isinstance(source, unreal.MaterialExpressionVectorParameter) else ""
        if not MEL.connect_material_expressions(source, out, node, input_name):
            raise RuntimeError("ligação falhou: " + input_name)
    return node


uv = MEL.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -1300, 0)
position = MEL.create_material_expression(material, unreal.MaterialExpressionWorldPosition, -1300, 100)
rand = MEL.create_material_expression(material, unreal.MaterialExpressionPerInstanceRandom, -1300, 200)
ground = []
for index in range(4):
    data = MEL.create_material_expression(material, unreal.MaterialExpressionPerInstanceCustomData, -1300, -300 + index * 60)
    data.set_editor_property("data_index", index)
    ground.append(data)

# as manchas grandes do chão (o mesmo T_MacroNoise a 40 m e 9 m do M_TerrainTiles, Tools/ImproveVegetationMaterials.py):
# chão e grama clareiam, escurecem e esquentam juntos
noise = EAL.load_asset("/Game/Vortex/Materials/T_MacroNoise")
macro_samples = []
for i, scale in enumerate((4000.0, 900.0)):
    divide = MEL.create_material_expression(material, unreal.MaterialExpressionDivide, -1100, 300 + i * 120)
    divide.set_editor_property("const_b", scale)
    mask = MEL.create_material_expression(material, unreal.MaterialExpressionComponentMask, -1200, 300 + i * 120)
    mask.set_editor_property("r", True)
    mask.set_editor_property("g", True)
    mask.set_editor_property("b", False)
    MEL.connect_material_expressions(position, "", mask, "")
    MEL.connect_material_expressions(mask, "", divide, "A")
    tex = MEL.create_material_expression(material, unreal.MaterialExpressionTextureSample, -950, 300 + i * 120)
    tex.set_editor_property("texture", noise)
    tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    MEL.connect_material_expressions(divide, "", tex, "UVs")
    macro_samples.append(tex)

# 10/11: a cor nasce do chão (dados por cópia 0-2 = a cor da célula, que é a média do quadradinho do chão vezes o tom
# da região) em 4 degraus pela altura, os mesmos tons do chão em pixel art (Tools/MakeStylizedTerrainTiles.py): raiz um
# pouco mais escura e fria, meio, luz e a ponta clara e quente. Degrau seco, sem degradê: cara de pixel art.
color = custom(-400, 0, "Cor", """
float h = saturate(1.0 - U.y);
float3 g = max(float3(GR, GG, GB), 0.004);
if (GR + GG + GB <= 0.0) g = float3(0.13, 0.33, 0.06);
float3 c = h < 0.3 ? g * 0.89 + float3(-0.01, 0.0, 0.015)
         : h < 0.62 ? g
         : h < 0.86 ? g * 1.09 + float3(0.01, 0.006, -0.01)
         : g * 1.30 + float3(0.03, 0.02, -0.02);
// secura da região: só os dois degraus de cima puxam para palha (no começo a folha toda virava palha nos prados secos)
float dry = saturate((Dry - 0.45) * 1.6 + (M1.r - 0.5) * 0.8);
c = h < 0.62 ? c : lerp(c, float3(0.42, 0.34, 0.13), dry * (h < 0.86 ? 0.3 : 0.55));
// manchas do chão (as mesmas do M_TerrainTiles) e um pouco por touceira
// as amostras chegam com os 4 canais: o grande no R, o médio no G (como no M_TerrainTiles)
float m = lerp(M1.r, M2.g, 0.3);
c *= lerp(0.88, 1.08, m) * lerp(float3(1.04, 1.0, 0.93), float3(0.95, 1.0, 1.04), m);
return max(c, 0.0) * (0.95 + 0.1 * frac(R * 53.7));
""", [("U", uv), ("GR", ground[0]), ("GG", ground[1]), ("GB", ground[2]), ("Dry", ground[3]), ("R", rand),
      ("M1", macro_samples[0]), ("M2", macro_samples[1])])
MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
rough = MEL.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 150)
rough.set_editor_property("r", 0.7)
MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
sss_tint = MEL.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -650, 250)
sss_tint.set_editor_property("constant", unreal.LinearColor(0.75, 0.9, 0.35, 1.0))
sss = MEL.create_material_expression(material, unreal.MaterialExpressionMultiply, -400, 250)
MEL.connect_material_expressions(color, "", sss, "A")
MEL.connect_material_expressions(sss_tint, "", sss, "B")
MEL.connect_material_property(sss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)

# vento e pisada: os mesmos do M_GrassWind (Tools/CreateGrassMaterial.py); a altura é 1 - v
height = custom(-900, 450, "Altura", "return saturate(1.0 - U.y);", [("U", uv)], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
time = MEL.create_material_expression(material, unreal.MaterialExpressionTime, -900, 550)
wind = custom(-400, 450, "Vento", """
float w = H * H;
float phase = R * 6.2831;
float gust = 0.55 + 0.45 * sin(T * 0.55 + P.x * 0.0021 + P.y * 0.0016);
float s = sin(T * 2.2 + phase + P.x * 0.021 + P.y * 0.017) + 0.45 * sin(T * 3.9 + P.y * 0.033 - P.x * 0.012);
float c = cos(T * 1.7 + phase * 0.7 + P.x * 0.019 - P.y * 0.008);
return float3((s * 5.0 + 4.0) * gust * w, (c * 3.0 + 2.0) * gust * w, -abs(s) * 1.2 * w);
""", [("P", position), ("T", time), ("H", height), ("R", rand)])
collection = EAL.load_asset("/Game/Vortex/Materials/MPC_GrassStep")
players = []
for i in range(8):
    param = MEL.create_material_expression(material, unreal.MaterialExpressionCollectionParameter, -900, 700 + i * 110)
    param.set_editor_property("collection", collection)
    param.set_editor_property("parameter_name", f"Player{i}")
    players.append((f"A{i}", param))
step = custom(-400, 800, "Pisada", """
float3 o = 0;
float4 pl[8] = { A0, A1, A2, A3, A4, A5, A6, A7 };
for (int i = 0; i < 8; ++i)
{
    if (pl[i].w <= 0.0) continue;
    float2 d = P.xy - pl[i].xy;
    float dist = length(d);
    float near = saturate(1.0 - dist / 75.0);
    near *= near * saturate(1.0 - abs(P.z - pl[i].z) / 120.0);
    float2 dir = d / max(dist, 1.0);
    o.xy += dir * near * 28.0 * H;
    o.z -= near * 30.0 * H;
}
return o;
""", [("P", position), ("H", height)] + players)
total = MEL.create_material_expression(material, unreal.MaterialExpressionAdd, -150, 600)
MEL.connect_material_expressions(wind, "", total, "A")
MEL.connect_material_expressions(step, "", total, "B")
MEL.connect_material_property(total, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
MEL.recompile_material(material)
EAL.save_loaded_asset(material)

for name in ("Rasteira", "PradoFina", "PradoAlta", "Alta"):
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_as_skeletal", False)
    options.static_mesh_import_data.set_editor_property("combine_meshes", True)
    options.static_mesh_import_data.set_editor_property("auto_generate_collision", False)
    mesh = import_file(os.path.join(PACK, "grama_jogo", f"SM_GrassPack_{name}.fbx"), f"SM_GrassPack_{name}", options)
    # (trocar dentro da lista static_materials não grava: a lista volta como cópia)
    for index in range(len(mesh.get_editor_property("static_materials"))):
        mesh.set_material(index, material)
    nanite = mesh.get_editor_property("nanite_settings")
    nanite.set_editor_property("enabled", False)
    mesh.set_editor_property("nanite_settings", nanite)
    EAL.save_loaded_asset(mesh)
    box = mesh.get_bounding_box()
    unreal.log(f"GRASS_PACK_IMPORTED {name} height={box.max.z:.0f} material={mesh.get_material(0).get_name()}")
unreal.log("GRASS_PACK_OK")
