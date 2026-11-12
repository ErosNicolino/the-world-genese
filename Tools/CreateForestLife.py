"""Floresta viva: materiais dos bichinhos em /Game/Vortex/Life — rode com o editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/CreateForestLife.py -unattended

As malhas são montadas pelo AVortexForestLife; aqui fica todo o movimento (na placa de vídeo, por
World Position Offset, sem custo no processador):
- M_Butterfly: asas batendo (com planadas), voo errante em volta do ponto de cada uma, cores
  sorteadas por borboleta (monarca, limão, branca, azul) com borda escura e pintas.
- M_Seed: sementes de dente-de-leão levadas pelo vento (a 26,6 graus, o da grama), subindo e balançando; nascem e somem
  aos poucos (sem pulo quando voltam ao começo).
- M_Firefly: vaga-lumes sem luz própria na cena (só brilho + bloom), piscando cada um no seu ritmo.
Todas têm o parâmetro Fade (0..1, dia/noite): cada bicho aparece ou some num ponto sorteado
dessa faixa, então eles vêm e vão aos poucos. Nenhum faz sombra.
"""
import os
import unreal

MEL = unreal.MaterialEditingLibrary
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = "/Game/Vortex/Life"


def new_material(name):
    path = FOLDER + "/" + name
    if library.does_asset_exist(path):
        library.delete_asset(path)
    material = tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("used_with_instanced_static_meshes", True)
    return material


class Graph:
    def __init__(self, material):
        self.m = material
        self.y = 0

    def node(self, cls, **props):
        self.y += 120
        n = MEL.create_material_expression(self.m, cls, -1200, self.y)
        for key, value in props.items():
            n.set_editor_property(key, value)
        return n

    def uv(self, index):
        return self.node(unreal.MaterialExpressionTextureCoordinate, coordinate_index=index)

    def scalar(self, name, value):
        return self.node(unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=value)

    def custom(self, name, code, inputs, output=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
        n = MEL.create_material_expression(self.m, unreal.MaterialExpressionCustom, -500, self.y)
        self.y += 120
        n.set_editor_property("output_type", output)
        n.set_editor_property("description", name)
        n.set_editor_property("code", code)
        items = []
        for input_name, _ in inputs:
            item = unreal.CustomInput()
            item.set_editor_property("input_name", input_name)
            items.append(item)
        n.set_editor_property("inputs", items)
        for input_name, source in inputs:
            if not MEL.connect_material_expressions(source, "", n, input_name):
                raise RuntimeError("ligação falhou: %s.%s" % (name, input_name))
        return n

    def local_to_world(self, source):
        t = self.node(unreal.MaterialExpressionTransform,
                      transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL,
                      transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
        MEL.connect_material_expressions(source, "", t, "")
        return t

    def out(self, source, prop):
        if not MEL.connect_material_property(source, "", prop):
            raise RuntimeError("saída falhou: %s" % prop)

    def finish(self):
        MEL.recompile_material(self.m)
        library.save_loaded_asset(self.m)


lines = []

# ---------------------------------------------------------------- borboleta
m = new_material("M_Butterfly")
m.set_editor_property("two_sided", True)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
g = Graph(m)
uv0, uv1 = g.uv(0), g.uv(1)
rnd = g.node(unreal.MaterialExpressionPerInstanceRandom)
time = g.node(unreal.MaterialExpressionTime)
fade = g.scalar("Fade", 1.0)
flight = g.custom("Voo", """
float y = U1.x, d = abs(y);
// planadas: de vez em quando as asas param abertas
float glide = saturate(sin(T * 0.9 + R * 9.0) * 3.0 + 1.2);
float a = 0.35 + 0.95 * sin(T * (13.0 + R * 5.0) + R * 40.0) * glide;
float3 flap = float3(0, y * (cos(a) - 1.0), d * sin(a));
float3 path = float3(sin(T * 0.31 + R * 41.0) * 160.0 + sin(T * 0.83 + R * 7.0) * 55.0,
                     cos(T * 0.27 + R * 73.0) * 160.0 + cos(T * 0.71 + R * 3.0) * 55.0,
                     sin(T * 0.6 + R * 17.0) * 35.0 - sin(a) * 2.5);
return flap + path;
""", [("U1", uv1), ("R", rnd), ("T", time)])
g.out(g.local_to_world(flight), unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
color = g.custom("Cor", """
if (U1.y > 0.5) return float3(0.02, 0.018, 0.015);
float k = frac(R * 7.77);
float3 c = k < 0.4 ? float3(0.85, 0.30, 0.02) : k < 0.65 ? float3(0.92, 0.80, 0.16)
         : k < 0.85 ? float3(0.86, 0.86, 0.80) : float3(0.08, 0.30, 0.85);
float2 p = U0.xy;
float fore = length((p - float2(0.55, 0.68)) / float2(0.50, 0.36));
float hind = length((p - float2(0.42, 0.28)) / float2(0.40, 0.30));
float edge = smoothstep(0.72, 0.95, min(fore, hind));
float spots = step(length(frac(p * float2(5.0, 4.0)) - 0.5), 0.18) * step(0.8, p.x);
float3 dark = float3(0.015, 0.012, 0.01);
c = lerp(c, dark, max(edge, spots * 0.9));
c = lerp(c * 0.55, c, smoothstep(0.0, 0.25, p.x));   // perto do corpo mais escuro
return c;
""", [("U0", uv0), ("U1", uv1), ("R", rnd)])
g.out(color, unreal.MaterialProperty.MP_BASE_COLOR)
mask = g.custom("Recorte", """
if (U1.y > 0.5) return F > frac(D * 13.7) ? 1.0 : 0.0;
float2 p = U0.xy;
float fore = length((p - float2(0.55, 0.68)) / float2(0.50, 0.36));
float hind = length((p - float2(0.42, 0.28)) / float2(0.40, 0.30));
return (min(fore, hind) < 1.0 && F > frac(D * 13.7)) ? 1.0 : 0.0;
""", [("U0", uv0), ("U1", uv1), ("F", fade), ("D", rnd)], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
g.out(mask, unreal.MaterialProperty.MP_OPACITY_MASK)
rough = g.node(unreal.MaterialExpressionConstant, r=0.55)
g.out(rough, unreal.MaterialProperty.MP_ROUGHNESS)
g.finish()
lines.append("M_Butterfly")

# ---------------------------------------------------------------- semente
m = new_material("M_Seed")
m.set_editor_property("two_sided", True)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
g = Graph(m)
uv0 = g.uv(0)
rnd = g.node(unreal.MaterialExpressionPerInstanceRandom)
time = g.node(unreal.MaterialExpressionTime)
fade = g.scalar("Fade", 1.0)
drift = g.custom("Vento", """
float s = frac(T * 0.035 * (0.7 + R * 0.6) + R);
// 10/11: na direção do vento da grama e dos rastros (26,6 graus; Data/World/vento.json), antes era +X do mundo
float along = (s - 0.5) * 1600.0 + sin(T * 0.9 + R * 30.0) * 20.0;
float side = sin(T * 0.5 + R * 20.0) * 60.0;
return float3(along * 0.894 - side * 0.447, along * 0.447 + side * 0.894,
              sin(T * 0.7 + R * 9.0) * 40.0 + s * 140.0);
""", [("R", rnd), ("T", time)])
g.out(drift, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
white = g.node(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(0.85, 0.85, 0.8, 1.0))
g.out(white, unreal.MaterialProperty.MP_BASE_COLOR)
g.out(white, unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
seed_mask = g.custom("Recorte", """
float s = frac(T * 0.035 * (0.7 + R * 0.6) + R);
float life = sin(s * 3.14159);
float r = length(U0.xy - 0.5) * 2.0;
// penugem: miolo cheio e fiapos em volta
float rays = 0.5 + 0.5 * sin(atan2(U0.y - 0.5, U0.x - 0.5) * 12.0);
float fluff = r < 0.25 ? 1.0 : (r < 1.0 ? rays * (1.0 - r) * 1.6 : 0.0);
return (fluff > 0.35 && F * saturate(life * 3.0) > frac(D * 13.7) * 0.9 + 0.05) ? 1.0 : 0.0;
""", [("U0", uv0), ("R", rnd), ("T", time), ("F", fade), ("D", rnd)], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
g.out(seed_mask, unreal.MaterialProperty.MP_OPACITY_MASK)
g.out(g.node(unreal.MaterialExpressionConstant, r=0.8), unreal.MaterialProperty.MP_ROUGHNESS)
g.finish()
lines.append("M_Seed")

# ---------------------------------------------------------------- vaga-lume
m = new_material("M_Firefly")
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
g = Graph(m)
rnd = g.node(unreal.MaterialExpressionPerInstanceRandom)
time = g.node(unreal.MaterialExpressionTime)
fade = g.scalar("Fade", 1.0)
glow = g.scalar("Glow", 110.0)
wander = g.custom("Voo", """
return float3(sin(T * 0.21 + R * 31.0) * 120.0 + sin(T * 0.77 + R * 5.0) * 25.0,
              cos(T * 0.17 + R * 57.0) * 120.0 + cos(T * 0.69 + R * 8.0) * 25.0,
              sin(T * 0.4 + R * 11.0) * 45.0);
""", [("R", rnd), ("T", time)])
g.out(g.local_to_world(wander), unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
light = g.custom("Brilho", """
// cada um no seu ritmo: acende devagar, apaga, fica um tempo escuro
float b = pow(saturate(sin(T * (0.9 + R * 0.8) + R * 60.0)), 6.0);
return float3(0.65, 1.0, 0.18) * (0.04 + b) * F * G;
""", [("R", rnd), ("T", time), ("F", fade), ("G", glow)])
g.out(light, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
g.finish()
lines.append("M_Firefly")

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
with open(os.path.join(root, "Saved", "Logs", "CreateForestLife.txt"), "w") as out:
    out.write("VORTEX_FOREST_LIFE " + " ".join(lines) + "\n")
