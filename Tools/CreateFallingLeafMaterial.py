"""Cria /Game/Vortex/Materials/M_FallingLeaf: a folha que cai das árvores (10/11; AVortexFallingLeaves).

Uma plaquinha quadrada (malha feita no código) recortada em folha de pixel art numa grade de 8 x 8: oval com ponta,
cabinho embaixo, nervura do meio mais escura e o lado de cima à esquerda mais claro (a luz do jogo vem de cima). A cor
vem por cópia (dados 0-2, a cor da árvore de onde caiu, Data/World/folhas.json). Duas faces com luz atravessando,
como a grama, e 35% de luz própria (contra o céu ela virava uma mancha preta).

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<raiz>/Tools/CreateFallingLeafMaterial.py (editor fechado)
"""
import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
PATH = "/Game/Vortex/Materials/M_FallingLeaf"
if EAL.does_asset_exist(PATH):
    EAL.delete_asset(PATH)
material = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_FallingLeaf", "/Game/Vortex/Materials", unreal.Material, unreal.MaterialFactoryNew())
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
material.set_editor_property("two_sided", True)
material.set_editor_property("used_with_instanced_static_meshes", True)


def custom(x, y, name, code, inputs, output):
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
        if not MEL.connect_material_expressions(source, "", node, input_name):
            raise RuntimeError("ligação falhou: " + input_name)
    return node


uv = MEL.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -900, 0)
color = []
for index in range(3):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionPerInstanceCustomData, -900, 120 + index * 70)
    node.set_editor_property("data_index", index)
    color.append(node)

# o desenho: 0 fora, 1 folha, 2 nervura/cabo; luz pelo canto
SHAPE = """
float2 cell = floor(saturate(U) * 7.999);
float2 q = (cell + 0.5) / 8.0 * 2.0 - 1.0;   // -1..1, y para baixo
float y = -q.y;                                // y para cima
float half_w = 0.78 * sqrt(saturate(1.0 - pow(abs(y - 0.08) / 0.86, 2.0)));
if (y > 0.55) half_w *= saturate((1.0 - y) / 0.45) * 1.15;   // a ponta
bool leaf = abs(q.x) <= half_w + 0.01 && y > -0.8;
bool stem = abs(q.x) < 0.13 && y <= -0.6;
bool vein = abs(q.x) < 0.13 && y > -0.6 && y < 0.7;
"""
mask = custom(-450, 300, "Recorte", SHAPE + "return (leaf || stem) ? 1.0 : 0.0;", [("U", uv)], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
MEL.connect_material_property(mask, "", unreal.MaterialProperty.MP_OPACITY_MASK)
base = custom(-450, 0, "Cor", SHAPE + """
float3 c = max(float3(R, G, B), 0.005);
if (R + G + B <= 0.0) c = float3(0.18, 0.35, 0.08);
float light = (q.y < 0.0 && q.x < 0.0) ? 1.18 : (q.y > 0.2 && q.x > 0.1 ? 0.8 : 1.0);
if (vein || stem) light = 0.62;
return c * light;
""", [("U", uv), ("R", color[0]), ("G", color[1]), ("B", color[2])], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
sss = MEL.create_material_expression(material, unreal.MaterialExpressionMultiply, -200, 150)
MEL.connect_material_expressions(base, "", sss, "A")
sss.set_editor_property("const_b", 0.6)
MEL.connect_material_property(sss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
# um pouco de luz própria: contra o céu, na sombra da copa, a folha virava uma mancha preta
glow = MEL.create_material_expression(material, unreal.MaterialExpressionMultiply, -200, 350)
MEL.connect_material_expressions(base, "", glow, "A")
glow.set_editor_property("const_b", 0.35)
MEL.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
rough = MEL.create_material_expression(material, unreal.MaterialExpressionConstant, -200, 250)
rough.set_editor_property("r", 0.75)
MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(material)
EAL.save_loaded_asset(material)
unreal.log("FALLING_LEAF_MATERIAL_OK " + PATH)
