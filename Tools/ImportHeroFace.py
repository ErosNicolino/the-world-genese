"""O rosto animado (estilo Hytale) dos heróis do Mixamo na Unreal — editor fechado, depois do ImportMixamoHeroes.py.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportHeroFace.py -unattended -nullrhi

- Data/Heroes/<raça>_Mixamo/Face/eyes.png e mouth.png (Tools/MakeHeroFaces.py) -> /Game/Vortex/Characters/Heroes/<raça>/
  T_<raça>_Eyes e T_<raça>_Mouth (pixel nítido, sem mipmap, sempre na memória);
- M_HeroFace: a cor da textura do herói e, na frente da cabeça, os quadros dos olhos e da boca projetados pela posição
  de cada vértice antes da animação (PreSkinnedPosition: o rosto acompanha a cabeça em qualquer clipe);
- MI_HeroFace_<raça>: o retângulo do rosto passado do Blender (face.json, em m) para o espaço da malha na Unreal (cm),
  pelos ossos do quadril, da cabeça, das mãos e do pé. AVortexCharacter troca os quadros (EyeFrame, MouthFrame).
Relatório em Saved/Logs/ImportHeroFace.txt.
"""
import json
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
DEST = "/Game/Vortex/Characters/Heroes"
lines = []

CODE = r"""
float3 c = BaseColor;
float3 d = Pos - Origin;
float2 uv = float2(dot(d, Right), dot(d, Up)) / (2.0 * Shape.x) + 0.5;
if (dot(d, Fwd) < -Shape.y || dot(Nrm, Fwd) < 0.3) return c;
float2 e = (uv - EyeBox.xy) / EyeBox.zw;
if (all(e >= 0) && all(e <= 1))
{
    float f = floor(Frames.x + 0.5);
    float2 a = (float2(fmod(f, 4.0), floor(f / 4.0)) + float2(e.x, 1.0 - e.y)) / float2(4.0, 2.0);
    float4 s = Texture2DSampleLevel(EyeTex, EyeTexSampler, a, 0);
    c = lerp(c, s.rgb, s.a);
}
float2 m = (uv - MouthBox.xy) / MouthBox.zw;
if (all(m >= 0) && all(m <= 1))
{
    float f = floor(Frames.y + 0.5);
    float2 a = (float2(fmod(f, 4.0), floor(f / 4.0)) + float2(m.x, 1.0 - m.y)) / float2(4.0, 2.0);
    float4 s = Texture2DSampleLevel(MouthTex, MouthTexSampler, a, 0);
    c = lerp(c, s.rgb, s.a);
}
return c;
"""


def node(m, kind, x, y, **props):
    e = mel.create_material_expression(m, kind, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def material():
    path = DEST + "/M_HeroFace"
    m = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset("M_HeroFace", DEST, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    base = node(m, unreal.MaterialExpressionTextureSampleParameter2D, -1100, -200, parameter_name="Base")
    custom = node(m, unreal.MaterialExpressionCustom, -300, 0, code=CODE, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                  description="Rosto")
    names = ["Pos", "Nrm", "BaseColor", "Origin", "Right", "Up", "Fwd", "Shape", "EyeBox", "MouthBox", "Frames", "EyeTex", "MouthTex"]
    inputs = []
    for n in names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)
    sources = {
        "Pos": node(m, unreal.MaterialExpressionPreSkinnedPosition, -1100, 100),
        "Nrm": node(m, unreal.MaterialExpressionPreSkinnedNormal, -1100, 180),
        "Origin": node(m, unreal.MaterialExpressionVectorParameter, -1100, 260, parameter_name="FaceOrigin"),
        "Right": node(m, unreal.MaterialExpressionVectorParameter, -1100, 420, parameter_name="FaceRight"),
        "Up": node(m, unreal.MaterialExpressionVectorParameter, -1100, 580, parameter_name="FaceUp"),
        "Fwd": node(m, unreal.MaterialExpressionVectorParameter, -1100, 740, parameter_name="FaceForward"),
        "Shape": node(m, unreal.MaterialExpressionVectorParameter, -1100, 900, parameter_name="FaceShape"),
        "EyeBox": node(m, unreal.MaterialExpressionVectorParameter, -1100, 1060, parameter_name="EyeBox"),
        "MouthBox": node(m, unreal.MaterialExpressionVectorParameter, -1100, 1220, parameter_name="MouthBox"),
        "EyeTex": node(m, unreal.MaterialExpressionTextureObjectParameter, -1100, 1380, parameter_name="Eyes"),
        "MouthTex": node(m, unreal.MaterialExpressionTextureObjectParameter, -1100, 1540, parameter_name="Mouth"),
    }
    eye = node(m, unreal.MaterialExpressionScalarParameter, -900, 1700, parameter_name="EyeFrame", default_value=0.0)
    mouth = node(m, unreal.MaterialExpressionScalarParameter, -900, 1780, parameter_name="MouthFrame", default_value=0.0)
    frames = node(m, unreal.MaterialExpressionAppendVector, -700, 1720)
    mel.connect_material_expressions(eye, "", frames, "A")
    mel.connect_material_expressions(mouth, "", frames, "B")
    sources["Frames"] = frames
    # a posição e a normal antes da animação só existem no cálculo dos vértices: passam para os pixels por interpoladores
    for key, y in (("Pos", 100), ("Nrm", 180)):
        through = node(m, unreal.MaterialExpressionVertexInterpolator, -800, y)
        mel.connect_material_expressions(sources[key], "", through, "")
        sources[key] = through
    for n, src in sources.items():
        # os retângulos (u, v, largura, altura) precisam dos 4 valores
        pin = ("RGBA" if n in ("EyeBox", "MouthBox", "Shape") else "RGB") if isinstance(src, unreal.MaterialExpressionVectorParameter) else ""
        mel.connect_material_expressions(src, pin, custom, n)
    mel.connect_material_expressions(base, "RGB", custom, "BaseColor")
    mel.connect_material_property(custom, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = node(m, unreal.MaterialExpressionScalarParameter, -300, 300, parameter_name="Roughness", default_value=.85)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    m.set_editor_property("used_with_skeletal_mesh", True)
    # dois lados (01/11): o modelo do Mixamo tem faces do avesso; com um lado só ficavam buracos (a roupa do anão)
    m.set_editor_property("two_sided", True)
    mel.recompile_material(m)
    library.save_loaded_asset(m)
    return m


def texture(path, folder, name):
    t = unreal.AssetImportTask()
    t.filename = path
    t.destination_path = folder
    t.destination_name = name
    t.automated = True
    t.save = True
    t.replace_existing = True
    tools.import_asset_tasks([t])
    tex = library.load_asset(folder + "/" + name)
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("never_stream", True)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    library.save_loaded_asset(tex)
    return tex


def sub(a, b):
    return [a[i] - b[i] for i in range(3)]


def dot(a, b):
    return sum(a[i] * b[i] for i in range(3))


def unit(a):
    n = dot(a, a) ** .5
    return [v / n for v in a]


def basis(left, up, fwd):
    """Uma base ortonormal: esquerda, cima (sem a parte da esquerda) e frente (sem as duas)."""
    e1 = unit(left)
    e2 = unit([up[i] - dot(up, e1) * e1[i] for i in range(3)])
    e3 = unit([fwd[i] - dot(fwd, e1) * e1[i] - dot(fwd, e2) * e2[i] for i in range(3)])
    return e1, e2, e3


base_material = material()
# as raças com o herói do Mixamo e o rosto feito (Data/Heroes/<raça>_Mixamo/Face/face.json)
import glob as _glob
RACES = sorted(os.path.basename(os.path.dirname(os.path.dirname(f)))[:-len("_Mixamo")]
               for f in _glob.glob(os.path.join(root, "Data", "Heroes", "*_Mixamo", "Face", "face.json")))
for race in RACES:
    folder = DEST + "/" + race
    data_dir = os.path.join(root, "Data", "Heroes", race + "_Mixamo", "Face")
    mesh = library.load_asset(folder + "/Hero_" + race)
    if not mesh or not os.path.exists(os.path.join(data_dir, "face.json")):
        lines.append("%s: sem herói do Mixamo ou sem rosto" % race)
        continue
    with open(os.path.join(data_dir, "face.json"), encoding="utf-8") as f:
        face = json.load(f)
    eyes = texture(os.path.join(data_dir, "eyes.png"), folder, "T_%s_Eyes" % race)
    mouth = texture(os.path.join(data_dir, "mouth.png"), folder, "T_%s_Mouth" % race)
    # a versão feminina dos olhos (Tools/MakeBlockFace.py --feminino): o AVortexCharacter troca o "Eyes" quando é feminino
    if os.path.exists(os.path.join(data_dir, "eyes_f.png")):
        texture(os.path.join(data_dir, "eyes_f.png"), folder, "T_%s_Eyes_F" % race)
    # os ossos de pé (pose de referência) na Unreal, em cm, no espaço da malha
    pose = unreal.AnimPoseExtensions.get_reference_pose(mesh.get_editor_property("skeleton"))
    names = [str(n) for n in unreal.AnimPoseExtensions.get_bone_names(pose)]

    def ue(bone):
        name = next(n for n in names if n.endswith(bone) and n.startswith("mixamorig"))
        t = unreal.AnimPoseExtensions.get_bone_pose(pose, name, unreal.AnimPoseSpaces.WORLD).translation
        return [t.x, t.y, t.z]

    bl = face["bones"]
    B = {k: bl[k] for k in bl}
    U = {k: ue(k) for k in bl}
    # Blender (m) -> malha na Unreal (cm): as mesmas direções (esquerda, cima, frente) nas duas, e a escala pelas mãos
    bb = basis(sub(B["LeftHand"], B["RightHand"]), sub(B["Head"], B["Hips"]), sub(B["LeftToeBase"], B["LeftFoot"]))
    ub = basis(sub(U["LeftHand"], U["RightHand"]), sub(U["Head"], U["Hips"]), sub(U["LeftToeBase"], U["LeftFoot"]))
    scale = dot(sub(U["LeftHand"], U["RightHand"]), sub(U["LeftHand"], U["RightHand"])) ** .5 / \
        dot(sub(B["LeftHand"], B["RightHand"]), sub(B["LeftHand"], B["RightHand"])) ** .5

    def vec(v):
        k = [dot(v, e) for e in bb]
        return [sum(k[j] * ub[j][i] for j in range(3)) for i in range(3)]

    def point(p):
        v = vec(sub(p, B["Hips"]))
        return [U["Hips"][i] + v[i] * scale for i in range(3)]

    rect = face["rect"]
    origin = point(rect["center"])
    right, up, fwd = vec([1, 0, 0]), vec([0, 0, 1]), vec([0, -1, 0])
    path = folder + "/MI_HeroFace_" + race
    mi = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset("MI_HeroFace_" + race, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, base_material)
    old = library.load_asset(folder + "/MI_Hero_" + race)
    skin = mel.get_material_instance_texture_parameter_value(old, "Base") if old else None
    if skin:
        mel.set_material_instance_texture_parameter_value(mi, "Base", skin)
    mel.set_material_instance_texture_parameter_value(mi, "Eyes", eyes)
    mel.set_material_instance_texture_parameter_value(mi, "Mouth", mouth)
    color = lambda v: unreal.LinearColor(v[0], v[1], v[2], 0)
    mel.set_material_instance_vector_parameter_value(mi, "FaceOrigin", color(origin))
    mel.set_material_instance_vector_parameter_value(mi, "FaceRight", color(right))
    mel.set_material_instance_vector_parameter_value(mi, "FaceUp", color(up))
    mel.set_material_instance_vector_parameter_value(mi, "FaceForward", color(fwd))
    mel.set_material_instance_vector_parameter_value(mi, "FaceShape", unreal.LinearColor(rect["half"] * scale, rect["depth"] * scale, 0, 0))
    mel.set_material_instance_vector_parameter_value(mi, "EyeBox", unreal.LinearColor(*face["eyes"]))
    mel.set_material_instance_vector_parameter_value(mi, "MouthBox", unreal.LinearColor(*face["mouth"]))
    library.save_loaded_asset(mi)
    lines.append("%s: origem %s direita %s cima %s frente %s meia largura %.1f cm (escala %.1f)" % (
        race, [round(v, 1) for v in origin], [round(v, 2) for v in right], [round(v, 2) for v in up], [round(v, 2) for v in fwd],
        rect["half"] * scale, scale))

with open(os.path.join(root, "Saved", "Logs", "ImportHeroFace.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
