"""Colisão pela própria malha (complexa como simples) nas peças em que se anda por cima: o tronco e as
raízes da Yggdrasil. Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi
"""
import unreal

for name in ["yggdrasil_trunk"]:
    path = f"/Game/Vortex/Props/{name}/{name}/StaticMeshes/{name}"
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    if not mesh:
        unreal.log_warning(f"VORTEX_COLLISION sem malha {path}")
        continue
    body = mesh.get_editor_property("body_setup")
    body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    mesh.set_editor_property("body_setup", body)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.log(f"VORTEX_COLLISION complexa {name}")
