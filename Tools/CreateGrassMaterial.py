"""Cria /Game/Vortex/Materials/M_GrassWind: grama realista do chão, folha de duas faces com luz
atravessando (Two Sided Foliage) e balanço de vento na placa de vídeo.

A cor nasce do chão: cada cópia de touceira traz (dados por cópia 0-2) a cor do chão naquele ponto
e (3) a secura da região, montadas pelo streamer. A folha vai da raiz escura e sombreada ao corpo
na cor do chão e à ponta mais clara e amarelada; manchas grandes no mundo e a secura da região
puxam para palha; cada touceira e cada folha variam o brilho. Dados por vértice:
UV0 = (sorteio da folha, altura 0 na raiz .. 1 na ponta), UV1 = (espiga 0/1, brilho). A altura
também diz quanto aquele ponto balança. O vento soma uma onda por folha (com fase por touceira) e
uma rajada lenta que atravessa o campo.

Rodar: UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/CreateGrassMaterial.py
"""

import unreal

MEL = unreal.MaterialEditingLibrary
PATH = "/Game/Vortex/Materials/M_GrassWind"

if unreal.EditorAssetLibrary.does_asset_exist(PATH):
    unreal.EditorAssetLibrary.delete_asset(PATH)
tools = unreal.AssetToolsHelpers.get_asset_tools()
material = tools.create_asset("M_GrassWind", "/Game/Vortex/Materials", unreal.Material, unreal.MaterialFactoryNew())
material.set_editor_property("two_sided", True)
# a grama é desenhada em cópias (InstancedStaticMeshComponent); sem esta marca a Unreal usa o
# material padrão cinza no jogo
material.set_editor_property("used_with_instanced_static_meshes", True)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)

uv0 = MEL.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -1100, 0)
uv1 = MEL.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -1100, 150)
uv1.set_editor_property("coordinate_index", 1)


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
        # parâmetro de vetor: a saída padrão tem só RGB; a pisada precisa do W (ativo)
        output = "RGBA" if isinstance(source, unreal.MaterialExpressionVectorParameter) else ""
        if not MEL.connect_material_expressions(source, output, node, input_name):
            raise RuntimeError(f"ligação falhou: {input_name}")
    return node


position_for_color = MEL.create_material_expression(material, unreal.MaterialExpressionWorldPosition, -1100, -250)
ground = []
for index in range(4):
    data = MEL.create_material_expression(material, unreal.MaterialExpressionPerInstanceCustomData, -1100, -500 + index * 60)
    data.set_editor_property("data_index", index)
    ground.append(data)
instance_random = MEL.create_material_expression(material, unreal.MaterialExpressionPerInstanceRandom, -1100, -560)

# cor a partir do chão do ponto, com manchas de secura e variação por touceira e por folha
color = custom(-700, 0, "Cor", """
float pick = U0.x, h = U0.y, seed = U1.x, bright = U1.y;
float3 g = max(float3(GR, GG, GB), 0.004);
// sem dados (cópia antiga): um verde de campo
if (GR + GG + GB <= 0.0) g = float3(0.13, 0.33, 0.06);
float lum = dot(g, float3(0.3, 0.59, 0.11));
float3 base = max(lerp(lum.xxx, g, 1.15), 0.0);
float3 root = base * float3(0.30, 0.34, 0.26);
float3 body = base * 0.85;
float3 tip = base * float3(1.12, 1.10, 0.80);
// manchas grandes no mundo (dezenas de metros) + secura da região
float patch = 0.5 + 0.28 * sin(P.x * 0.00061 + sin(P.y * 0.00047) * 2.3) + 0.22 * sin(P.y * 0.00083 + sin(P.x * 0.00037) * 1.9);
float dry = saturate((Dry - 0.45) * 1.6 + (patch - 0.5) * 0.8);
float3 straw = float3(0.40, 0.33, 0.14);
tip = lerp(tip, straw, dry * 0.8);
body = lerp(body, straw * 0.7, dry * 0.4);
// cada touceira e cada folha com seu tom
float r = frac(R * 17.31 + pick * 7.13);
float3 c = h < 0.55 ? lerp(root, body, h / 0.55) : lerp(body, tip, (h - 0.55) / 0.45);
c *= (0.84 + 0.32 * r) * bright;
c = lerp(c, lerp(base * 1.1, float3(0.58, 0.48, 0.24), 0.5 + 0.5 * dry), seed);
return c;
""", [("U0", uv0), ("U1", uv1), ("GR", ground[0]), ("GG", ground[1]), ("GB", ground[2]), ("Dry", ground[3]),
      ("R", instance_random), ("P", position_for_color)])
MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)

rough = MEL.create_material_expression(material, unreal.MaterialExpressionConstant, -500, 150)
rough.set_editor_property("r", 0.62)
MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

# luz atravessando a folha: a própria cor, mais amarelada
sss_tint = MEL.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -700, 250)
sss_tint.set_editor_property("constant", unreal.LinearColor(0.75, 0.9, 0.35, 1.0))
sss = MEL.create_material_expression(material, unreal.MaterialExpressionMultiply, -500, 250)
MEL.connect_material_expressions(color, "", sss, "A")
MEL.connect_material_expressions(sss_tint, "", sss, "B")
MEL.connect_material_property(sss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)

position = MEL.create_material_expression(material, unreal.MaterialExpressionWorldPosition, -900, 450)
time = MEL.create_material_expression(material, unreal.MaterialExpressionTime, -900, 550)
height = MEL.create_material_expression(material, unreal.MaterialExpressionComponentMask, -900, 650)
height.set_editor_property("r", False)
height.set_editor_property("g", True)
MEL.connect_material_expressions(uv0, "", height, "")

wind_random = MEL.create_material_expression(material, unreal.MaterialExpressionPerInstanceRandom, -900, 380)
wind = custom(-400, 450, "Vento", """
float w = H * H;
float phase = R * 6.2831;
float gust = 0.55 + 0.45 * sin(T * 0.55 + P.x * 0.0021 + P.y * 0.0016);
float s = sin(T * 2.2 + phase + P.x * 0.021 + P.y * 0.017) + 0.45 * sin(T * 3.9 + P.y * 0.033 - P.x * 0.012);
float c = cos(T * 1.7 + phase * 0.7 + P.x * 0.019 - P.y * 0.008);
return float3((s * 5.0 + 4.0) * gust * w, (c * 3.0 + 2.0) * gust * w, -abs(s) * 1.2 * w);
""", [("P", position), ("T", time), ("H", height), ("R", wind_random)])
# Pisada: até 8 jogadores (Player0..7 = pés em cm, w=1 quando ativo, mandados pelo streamer).
# Perto do pé a folha se abre para fora e deita; o efeito cai até ~75 cm e só vale para grama na
# altura do pé (não mexe na grama de um degrau muito acima ou abaixo).
# Os pés vêm de uma Material Parameter Collection (MPC_GrassStep): mudar um parâmetro de material
# instanciado obrigava a Unreal a refazer o desenho de toda a grama a cada quadro (~2 ms).
MPC_PATH = "/Game/Vortex/Materials/MPC_GrassStep"
if not unreal.EditorAssetLibrary.does_asset_exist(MPC_PATH):
    tools.create_asset("MPC_GrassStep", "/Game/Vortex/Materials", unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
collection = unreal.EditorAssetLibrary.load_asset(MPC_PATH)
vectors = []
for i in range(8):
    v = unreal.CollectionVectorParameter()
    v.set_editor_property("parameter_name", f"Player{i}")
    v.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, -100000.0, 0.0))
    vectors.append(v)
collection.set_editor_property("vector_parameters", vectors)
unreal.EditorAssetLibrary.save_loaded_asset(collection)
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
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log("GRASS_MATERIAL_OK " + PATH)
