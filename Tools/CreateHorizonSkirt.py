"""Saia do horizonte: material da faixa abaixo do horizonte, depois do fim do terreno montado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/CreateHorizonSkirt.py -unattended

A névoa da Fase 12 só vale até 1 km (para o céu, as nuvens volumétricas e as estrelas ficarem
limpos). Sem névoa ali, a faixa logo abaixo do horizonte, depois do fim do terreno (~126 m),
mostraria o chão escuro do SkyAtmosphere. UVortexDayNightSubsystem põe sob o terreno um plano
grande que acompanha a câmera, com este material: sem luz, com a cor do próprio céu logo acima do
horizonte na direção do olhar (SkyAtmosphereViewLuminance), para não haver emenda entre o chão
distante e o céu em nenhuma hora do dia.
"""
import os
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
destination = "/Game/Vortex/Sky"
path = destination + "/M_HorizonSkirt"
material = library.load_asset(path) if library.does_asset_exist(path) else \
    tools.create_asset("M_HorizonSkirt", destination, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(material)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

# direção do olhar, achatada para logo acima do horizonte
camera = mel.create_material_expression(material, unreal.MaterialExpressionCameraVectorWS, -1000, 0)
custom = mel.create_material_expression(material, unreal.MaterialExpressionCustom, -750, 0)
custom.set_editor_property("code", "float2 h = -V.xy; return normalize(float3(h, length(h) * 0.035));")
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
item = unreal.CustomInput()
item.set_editor_property("input_name", "V")
custom.set_editor_property("inputs", [item])
mel.connect_material_expressions(camera, "", custom, "V")
sky = mel.create_material_expression(material, unreal.MaterialExpressionSkyAtmosphereViewLuminance, -500, 0)
mel.connect_material_expressions(custom, "", sky, "WorldDirection")
scale = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -500, 150)
scale.set_editor_property("parameter_name", "SkyScale")
scale.set_editor_property("default_value", 1.0)
multiply = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -250, 0)
mel.connect_material_expressions(sky, "", multiply, "A")
mel.connect_material_expressions(scale, "", multiply, "B")
mel.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(material)
library.save_loaded_asset(material)
root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
with open(os.path.join(root, "Saved", "Logs", "CreateHorizonSkirt.txt"), "w") as out:
    out.write("VORTEX_HORIZON_SKIRT " + path + "\n")
