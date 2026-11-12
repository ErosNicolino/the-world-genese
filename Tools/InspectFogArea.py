"""Mostra como o material do FogArea (pacote copiado do VortexReferencia) desenha: domínio, mistura e os parâmetros."""
import unreal

for path in ("/Game/FogArea/Materials/M_FogArea", "/Game/FogArea/Materials/Base/MI_FogArea_Box", "/Game/FogArea/Materials/Base/MI_FogArea_Box_DF"):
    m = unreal.EditorAssetLibrary.load_asset(path)
    base = m if isinstance(m, unreal.Material) else m.get_base_material()
    unreal.log_warning(f"VORTEX_FOG {path}: domain={base.get_editor_property('material_domain')} blend={base.get_editor_property('blend_mode')} shading={base.get_editor_property('shading_model')}")
    if isinstance(m, unreal.MaterialInstance):
        for p in m.get_editor_property("scalar_parameter_values"):
            unreal.log_warning(f"VORTEX_FOG   escalar {p.parameter_info.name} = {p.parameter_value}")
        for p in m.get_editor_property("vector_parameter_values"):
            unreal.log_warning(f"VORTEX_FOG   cor {p.parameter_info.name} = {p.parameter_value}")
cls = unreal.EditorAssetLibrary.load_blueprint_class("/Game/FogArea/Blueprints/BP_FogArea")
cdo = unreal.get_default_object(cls)
for comp in cdo.get_components_by_class(unreal.ActorComponent):
    unreal.log_warning(f"VORTEX_FOG comp {comp.get_name()} {comp.get_class().get_name()}")
