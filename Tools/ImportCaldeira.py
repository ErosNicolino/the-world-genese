"""A borda da cratera do Azhûr e a lasca de obsidiana (14/11), editor fechado:

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportCaldeira.py -unattended -nullrhi

Data/Modelos/Caldeira/CrateraBorda.fbx e CrateraLasca.fbx (modelando ainda/cratera_azhur_2026-11-14.blend, feitos no Blender
pelo MCP) -> /Game/Vortex/World/Caldeira/SM_CrateraBorda e SM_CrateraLasca. A borda não tem colisão (as paredes invisíveis
do AVortexLavaArena::BuildCrater seguram); os materiais: Basalto = M_CalderaWall, Lava = M_AzhurLava.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
DEST = "/Game/Vortex/World/Caldeira"
MATS = {"Basalto": DEST + "/M_CalderaWall", "Lava": "/Game/Vortex/Creatures/RuneColossus/Materials/M_AzhurLava"}
lines = []
for name in ("CrateraBorda", "CrateraLasca"):
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("scene_name_sub_folder", False)
    pipeline.set_editor_property("asset_type_sub_folders", False)
    pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
    meshes = pipeline.get_editor_property("mesh_pipeline")
    meshes.set_editor_property("import_static_meshes", True)
    meshes.set_editor_property("import_skeletal_meshes", False)
    for prop, value in (("generate_collision", name == "CrateraLasca"), ("build_nanite", False)):
        try:
            meshes.set_editor_property(prop, value)
        except Exception:
            pass
    override = unreal.InterchangePipelineStackOverride()
    override.add_pipeline(pipeline)
    task = unreal.AssetImportTask()
    task.filename = os.path.join(root, "Data", "Modelos", "Caldeira", name + ".fbx")
    task.destination_path = DEST
    task.destination_name = "SM_" + name
    task.automated = True
    task.save = False
    task.replace_existing = True
    task.options = override
    tools.import_asset_tasks([task])
    mesh = library.load_asset(DEST + "/SM_" + name)
    if not isinstance(mesh, unreal.StaticMesh):
        lines.append("FALHOU " + name)
        continue
    named = []
    for index, slot in enumerate(mesh.get_editor_property("static_materials")):
        key = str(slot.get_editor_property("material_slot_name")).split(".")[0]
        mat = library.load_asset(MATS.get(key, MATS["Basalto"]))
        if mat:
            mesh.set_material(index, mat)
        named.append(key)
    library.save_loaded_asset(mesh)
    box = mesh.get_bounding_box()
    lines.append("%s: %s, %d triângulos, de %s a %s" % (name, named, mesh.get_num_triangles(0), box.min, box.max))
unreal.log_warning("VORTEX_CALDEIRA_IMPORT " + " | ".join(lines))
