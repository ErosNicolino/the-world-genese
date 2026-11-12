"""Água estilizada do Vortex, em três materiais (um por nível de qualidade) — rode com o editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/CreateVortexWater.py -unattended

Menos realista que o M_Lake (que nos lagos rasos do vale ficava quase invisível), combinando com os
blocos: a espessura de água entre a superfície e o chão (pela profundidade da cena) decide tudo:
- cor: turquesa vivo no raso que fecha num azul-petróleo no fundo;
- opacidade: transparente na beirada e quase opaca no fundo (o fundo some com a profundidade);
- uma linha de espuma clara e firme onde a água encosta na margem, que respira devagar;
- superfície quase lisa (marolinhas finas e lentas), luz por pixel com um brilho de sol discreto.
Parâmetros (MI ou material): Shallow, Deep, Foam, DepthScale (cm até o fundo), OpacityShallow,
OpacityDeep, FoamWidth (cm), Ripple.

Níveis (Opções → Qualidade da água; Source/Vortex/*/Framework/VortexGraphics):
- M_VortexWaterSimple (baixa): opaca, só a cor e marolinhas, sem profundidade (a mais leve);
- M_VortexWater: média (o jogo põe Ripple=0, parada) e alta (como está);
- M_VortexWaterEpic (épica): refração do fundo, ondas maiores em três camadas e brilho do sol.
"""
import os
import unreal

MEL = unreal.MaterialEditingLibrary
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = "/Game/Vortex/Materials"

COMMON = """
// espessura de água ao longo do olhar (cm) e um "fundo" de 0 (raso) a 1 (fundo)
float thick = max(SceneD - PixD, 0.0);
float deep = 1.0 - exp(-thick / max(DepthScale, 1.0));
// espuma: faixa firme junto à margem, com o contorno mexendo devagar
float2 wp = Uv * 1000.0;
float wob = sin(wp.x * 0.045 + Tm * 0.9) * sin(wp.y * 0.05 - Tm * 0.7);
float foam = 1.0 - smoothstep(FoamWidth * (0.75 + 0.35 * wob), FoamWidth * (0.75 + 0.35 * wob) + 2.5, thick);
"""

RIPPLES = """
// marolinhas finas e lentas (sem textura: não tremem nem cintilam ao girar a câmera)
float2 p = Uv * 1000.0;
float2 g = float2(0, 0);
g += float2(cos(p.x * 0.021 + Tm * 0.55), 0.0) * 0.5;
g += float2(0.0, cos(p.y * 0.026 - Tm * 0.47)) * 0.5;
g += float2(cos(dot(p, float2(0.013, 0.017)) + Tm * 0.8), cos(dot(p, float2(-0.016, 0.011)) - Tm * 0.6)) * 0.35;
"""

EPIC_WAVES = """
// épica: uma camada de ondas mais largas e uma de ondinhas rápidas por cima
g += float2(cos(dot(p, float2(0.006, 0.004)) + Tm * 0.35), cos(dot(p, float2(-0.003, 0.007)) - Tm * 0.3)) * 0.9;
g += float2(cos(dot(p, float2(0.061, -0.047)) + Tm * 2.1), cos(dot(p, float2(0.052, 0.058)) - Tm * 1.8)) * 0.18;
"""


def build(name, kind):
    path = FOLDER + "/" + name
    if library.does_asset_exist(path):
        library.delete_asset(path)
    m = tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    opaque = kind == "simple"
    if not opaque:
        m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        m.set_editor_property("translucency_lighting_mode", getattr(unreal.TranslucencyLightingMode,
            [k for k in dir(unreal.TranslucencyLightingMode) if "PER_PIXEL" in k or "FORWARD" in k][0]))
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    m.set_editor_property("two_sided", False)
    if kind == "epic":
        # o fundo entorta pelo normal da superfície (refração simples, barata)
        try:
            mode = [k for k in dir(unreal.RefractionMode) if "PIXEL_NORMAL" in k][0]
            m.set_editor_property("refraction_method", getattr(unreal.RefractionMode, mode))
        except Exception as error:
            unreal.log_warning("refração: %s" % error)

    y = [0]

    def expr(cls, x=-1200):
        e = MEL.create_material_expression(m, cls, x, y[0])
        y[0] += 110
        return e

    sources = {"Uv": expr(unreal.MaterialExpressionTextureCoordinate), "Tm": expr(unreal.MaterialExpressionTime)}
    if not opaque:
        sources["SceneD"] = expr(unreal.MaterialExpressionSceneDepth)
        sources["PixD"] = expr(unreal.MaterialExpressionPixelDepth)
    ripple = 2.2 if kind == "epic" else 1.0
    scalars = [("DepthScale", 45.0), ("OpacityShallow", 0.5 if kind == "epic" else 0.62), ("OpacityDeep", 0.985), ("FoamWidth", 12.0), ("Ripple", ripple)]
    for pname, value in scalars:
        p = expr(unreal.MaterialExpressionScalarParameter)
        p.set_editor_property("parameter_name", pname)
        p.set_editor_property("default_value", value)
        sources[pname] = p
    for pname, value in (("Shallow", unreal.LinearColor(0.035, 0.40, 0.38, 1)), ("Deep", unreal.LinearColor(0.004, 0.06, 0.12, 1)),
                         ("Foam", unreal.LinearColor(0.9, 0.97, 0.95, 1))):
        p = expr(unreal.MaterialExpressionVectorParameter)
        p.set_editor_property("parameter_name", pname)
        p.set_editor_property("default_value", value)
        sources[pname] = p

    def custom(title, out_type, code, names, prop, x):
        node = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, x, 0 if prop != unreal.MaterialProperty.MP_NORMAL else 500)
        node.set_editor_property("output_type", out_type)
        node.set_editor_property("description", title)
        node.set_editor_property("code", code)
        ins = []
        for n in names:
            item = unreal.CustomInput()
            item.set_editor_property("input_name", n)
            ins.append(item)
        node.set_editor_property("inputs", ins)
        for n in names:
            if not MEL.connect_material_expressions(sources[n], "", node, n):
                raise RuntimeError("ligação falhou: %s -> %s" % (n, title))
        MEL.connect_material_property(node, "", prop)
        return node

    if opaque:
        # baixa: sem profundidade; um mosaico suave de raso e fundo que anda devagar
        custom("Cor", unreal.CustomMaterialOutputType.CMOT_FLOAT3, """
float2 p = Uv * 1000.0;
float t = 0.5 + 0.25 * sin(p.x * 0.011 + Tm * 0.3) * sin(p.y * 0.013 - Tm * 0.25);
return lerp(Shallow.rgb, Deep.rgb, 0.45 + 0.3 * t);
""", ["Uv", "Tm", "Shallow", "Deep"], unreal.MaterialProperty.MP_BASE_COLOR, -500)
    else:
        custom("Cor", unreal.CustomMaterialOutputType.CMOT_FLOAT3, COMMON + """
float3 c = lerp(Shallow.rgb, Deep.rgb, deep);
return lerp(c, Foam.rgb, foam * 0.9);
""", ["SceneD", "PixD", "Uv", "Tm", "DepthScale", "FoamWidth", "Shallow", "Deep", "Foam"], unreal.MaterialProperty.MP_BASE_COLOR, -500)
        custom("Opacidade", unreal.CustomMaterialOutputType.CMOT_FLOAT1, COMMON + """
float o = lerp(OpacityShallow, OpacityDeep, deep);
// some de vez nos primeiros centímetros (sem serrilhado na margem) e a espuma é sólida
o *= saturate(thick / 1.5);
return saturate(max(o, foam * 0.92));
""", ["SceneD", "PixD", "Uv", "Tm", "DepthScale", "FoamWidth", "OpacityShallow", "OpacityDeep"], unreal.MaterialProperty.MP_OPACITY, -500)

    custom("Marolinhas", unreal.CustomMaterialOutputType.CMOT_FLOAT3, RIPPLES + (EPIC_WAVES if kind == "epic" else "") + """
return normalize(float3(g * 0.05 * Ripple, 1.0));
""", ["Uv", "Tm", "Ripple"], unreal.MaterialProperty.MP_NORMAL, -500)

    look = {"simple": (0.3, 0.25), "normal": (0.22, 0.2), "epic": (0.07, 0.5)}[kind]
    for prop, value in ((unreal.MaterialProperty.MP_ROUGHNESS, look[0]), (unreal.MaterialProperty.MP_SPECULAR, look[1])):
        c = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -300, y[0])
        y[0] += 110
        c.set_editor_property("r", value)
        MEL.connect_material_property(c, "", prop)
    if kind == "epic":
        c = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -300, y[0])
        c.set_editor_property("r", 1.2)
        MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_REFRACTION)

    MEL.recompile_material(m)
    library.save_loaded_asset(m)
    return path


made = [build("M_VortexWater", "normal"), build("M_VortexWaterSimple", "simple"), build("M_VortexWaterEpic", "epic")]
root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
with open(os.path.join(root, "Saved", "Logs", "CreateVortexWater.txt"), "w") as out:
    for path in made:
        out.write("VORTEX_WATER " + path + "\n")
