"""Cria /Game/Vortex/Materials/M_WindTrail: os rastros de vento no ar (10/11; AVortexWindTrails).

Translúcido e sem luz, dos dois lados. A fita (malha feita no código, UV0.x = 0 no começo e 1 no fim, UV0.y = de um
lado ao outro da largura) não aparece inteira: um pedaço claro corre por ela do começo ao fim, com o rabo sumindo
devagar e a frente cortada, e a fita toda acende e apaga com a vida (seno). A largura some nas beiradas (fio macio).
Dados por cópia (o ator manda): 0 = quando nasceu (segundos do mundo, o mesmo relógio do nó Time), 1 = quanto vive,
2 e 3 = o empurrão do vento em cm/s (x, y): a fita anda com o vento enquanto vive.
Parâmetros: Opacidade (o ator muda com a claridade do dia) e Cor.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<raiz>/Tools/CreateWindTrailMaterial.py (editor fechado)
"""
import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
PATH = "/Game/Vortex/Materials/M_WindTrail"
if EAL.does_asset_exist(PATH):
    EAL.delete_asset(PATH)
material = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_WindTrail", "/Game/Vortex/Materials", unreal.Material, unreal.MaterialFactoryNew())
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property("two_sided", True)
material.set_editor_property("used_with_instanced_static_meshes", True)


def custom(x, y, name, code, inputs, output=unreal.CustomMaterialOutputType.CMOT_FLOAT1):
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
time = MEL.create_material_expression(material, unreal.MaterialExpressionTime, -900, 100)
data = []
for index in range(4):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionPerInstanceCustomData, -900, 200 + index * 70)
    node.set_editor_property("data_index", index)
    data.append(node)
opacity = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -900, 520)
opacity.set_editor_property("parameter_name", "Opacidade")
opacity.set_editor_property("default_value", 0.4)
tint = MEL.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -900, 600)
tint.set_editor_property("parameter_name", "Cor")
tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

alpha = custom(-450, 0, "Rastro", """
float life = max(L, 0.1);
float t = saturate((T - B) / life);
if (T < B) return 0.0;
float head = t * 1.4;
float tail = head - 0.42;
float u = U.x;
float seg = smoothstep(tail, tail + 0.3, u) * (1.0 - smoothstep(head - 0.04, head, u));
float across = saturate((1.0 - abs(U.y * 2.0 - 1.0)) * 1.8);
float ends = smoothstep(0.0, 0.06, u) * (1.0 - smoothstep(0.94, 1.0, u));
return seg * across * ends * sin(3.14159 * t) * O;
""", [("U", uv), ("T", time), ("B", data[0]), ("L", data[1]), ("O", opacity)])
MEL.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)
MEL.connect_material_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

drift = custom(-450, 300, "Vento", """
float age = max(T - B, 0.0);
return float3(DX, DY, 0.0) * age;
""", [("T", time), ("B", data[0]), ("DX", data[2]), ("DY", data[3])], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
MEL.connect_material_property(drift, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
MEL.recompile_material(material)
EAL.save_loaded_asset(material)
unreal.log("WIND_TRAIL_MATERIAL_OK " + PATH)
