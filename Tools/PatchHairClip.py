"""Cabelo cortado pelo chapéu (09/11): M_BlockHair ganha o parâmetro ClipZ. Onde a altura em repouso passa dele, o cabelo some.
O corte usa a posição antes da animação, então acompanha a cabeça. Depois do Tools/ImportBlockHair.py, com o editor fechado:
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/PatchHairClip.py -unattended -nullrhi
"""
import unreal

library = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
mat = library.load_asset("/Game/Vortex/Characters/Heroes/humano/Hair/M_BlockHair")


def node(cls, x, y, **props):
    n = mel.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        n.set_editor_property(k, v)
    return n


if True:   # roda uma vez depois do ImportBlockHair.py (recria os nós do corte)
    pos = node(unreal.MaterialExpressionPreSkinnedPosition, -900, 500)
    z = node(unreal.MaterialExpressionComponentMask, -700, 500, r=False, g=False, b=True, a=False)
    # a posição antes do esqueleto só existe no vértice: o interpolador a leva até o pixel
    carry = node(unreal.MaterialExpressionVertexInterpolator, -800, 500)
    mel.connect_material_expressions(pos, "", carry, "VS")
    mel.connect_material_expressions(carry, "", z, "")
    clip = node(unreal.MaterialExpressionScalarParameter, -700, 650, parameter_name="ClipZ", default_value=100000.0)
    one = node(unreal.MaterialExpressionConstant, -500, 700, r=1.0)
    zero = node(unreal.MaterialExpressionConstant, -500, 800, r=0.0)
    choose = node(unreal.MaterialExpressionIf, -300, 550)
    mel.connect_material_expressions(z, "", choose, "A")
    mel.connect_material_expressions(clip, "", choose, "B")
    for value, pin in ((zero, "A > B"), (one, "A == B"), (one, "A < B")):
        if not mel.connect_material_expressions(value, "", choose, pin):
            raise RuntimeError("pino nao encontrado: " + pin)
    mel.connect_material_property(choose, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mel.recompile_material(mat)
    library.save_loaded_asset(mat)
unreal.log_warning("VORTEX_HAIR_CLIP ClipZ=%s" % mel.get_material_default_scalar_parameter_value(mat, "ClipZ"))
