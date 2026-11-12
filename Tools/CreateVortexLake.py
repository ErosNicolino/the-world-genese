"""Água cristalina do Vortex: /Game/Vortex/Materials/MI_VortexLake (filha de M_Lake do pacote
WaterMaterials, referência do jogador, em Content/WaterMaterials).

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/CreateVortexLake.py -unattended

O M_Lake original é quase incolor (feito para lagos fundos). Os lagos do vale são rasos (5-75 cm).
Estilo do Vortex (menos realista, combina com os blocos): turquesa vivo no raso que fecha rápido num
azul-petróleo quase opaco no fundo (o fundo some com a profundidade), superfície fosca e pouco
espelhada, brilho de sol discreto e superfície de laguinho: sem ondas nem refração (que tremiam e rasgavam ao girar
a câmera), só marolinhas finas e lentas. O reflexo do céu e das
margens vem do Lumen (r.Lumen.TranslucencyReflections.FrontLayer.Enable=1 em DefaultEngine.ini).
"""
import os
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
parent = library.load_asset("/Game/WaterMaterials/Materials/M_Lake")
if not parent:
    raise RuntimeError("M_Lake não encontrado (Content/WaterMaterials)")
path = "/Game/Vortex/Materials/MI_VortexLake"
mi = library.load_asset(path) if library.does_asset_exist(path) else \
    tools.create_asset("MI_VortexLake", "/Game/Vortex/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
mel.set_material_instance_parent(mi, parent)

VECTORS = {
    "Colour": unreal.LinearColor(0.07, 0.36, 0.38, 1.0),        # raso: turquesa vivo
    "ColourDeep": unreal.LinearColor(0.006, 0.05, 0.10, 1.0),   # fundo: azul-petróleo
}
SCALARS = {
    "Opacity": 0.66,
    "OpacityDeep": 0.985,          # o fundo não aparece mais lá no fundo
    "River_DepthScale": 55.0,      # cm até a cor/opacidade do fundo: fecha logo
    "FakeSpec_Intensity": 0.3,    # brilho do sol nas marolinhas (alto demais pisca com a câmera)
    "Roughness": 0.16,             # menos espelho
    # laguinho parado: sem ondas de verdade (o M_Lake mexe a superfície com 3 ondas grandes) e
    # sem refração de tela, que borrava e "rasgava" a água ao girar a câmera
    "Master_Intensity": 0.0,
    "Refraction": 0.0,
    "RefractionIndex": 1.0,
    # só marolinhas finas e lentas
    "NormalIntensity": 0.7,
    "WaterNormal_Intensity": 0.08,
    "WaterNormal_Speed": 0.15,
    "RoughNormal_Intensity": 0.05,
    "RoughNormal_Speed": 0.3,
    "SubtleNormal_Intensity": 0.03,
    "SubtleNormal_Speed": 0.6,
    "Lake1_Intensity": 0.08,
    "Lake2_Intensity": 0.08,
    "EdgeNormal_Intensity": 0.25,
    "EdgeNormal_Speed": 0.4,
    "FakeSpec_Speed": 0.15,
}
for name, value in VECTORS.items():
    mel.set_material_instance_vector_parameter_value(mi, name, value)
for name, value in SCALARS.items():
    mel.set_material_instance_scalar_parameter_value(mi, name, value)
mel.update_material_instance(mi)
library.save_loaded_asset(mi)
root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
with open(os.path.join(root, "Saved", "Logs", "CreateVortexLake.txt"), "w") as out:
    out.write("VORTEX_LAKE " + path + "\n")
