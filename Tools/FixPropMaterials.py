"""Liga as permissões de uso dos materiais do protótipo (malhas instanciadas, Nanite) e salva de
novo todas as instâncias: sem isso a Unreal usa o material padrão e recompila a cada execução."""

import unreal

MEL = unreal.MaterialEditingLibrary
for path, flags in (
    ("/Game/Vortex/Materials/M_VoxelProp", ("used_with_instanced_static_meshes", "used_with_nanite")),
    ("/Game/Vortex/Materials/M_TerrainVertexColor", ()),
    ("/Game/Vortex/Materials/M_Water", ()),
):
    material = unreal.EditorAssetLibrary.load_asset(path)
    for flag in flags:
        material.set_editor_property(flag, True)
    MEL.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, False)

saved = 0
for path in unreal.EditorAssetLibrary.list_assets("/Game/Vortex/Props", recursive=True):
    data = unreal.EditorAssetLibrary.find_asset_data(path)
    if data.asset_class_path.asset_name in ("MaterialInstanceConstant", "Material"):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, unreal.Material):
            asset.set_editor_property("used_with_instanced_static_meshes", True)
            asset.set_editor_property("used_with_nanite", True)
            MEL.recompile_material(asset)
        else:
            MEL.update_material_instance(asset)
        unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
        saved += 1
unreal.log(f"VORTEX_MATERIALS_FIXED salvos={saved}")
