"""Oficina das armas do Vortex no Blender (5.x): peças, materiais com textura e exportação.

Rodado pelo Tools/Blender/BuildVortexWeapons.py (dentro do Blender). Tudo é feito por código:
- peças: caixas e cilindros chanfrados, lâminas com perfil e fio (seção em losango), anéis,
  gemas facetadas, espinhos, cordas e correntes;
- materiais: cada um tem uma textura 256x256 gerada aqui (aço escovado com desgaste nas bordas,
  ouro, bronze, ferro escuro, madeiras com veio, couro com costura, tecido, osso, gemas e cristais
  que brilham), mapeada por projeção em caixa;
- o modelo é montado no referencial do item do Godot (pegada na origem, comprimento no +Z do
  Blender = +Y do glTF) e exportado já na mão do herói com o referencial de Data/VortexWeapons/frames.json.
"""
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

TEX = 256
WIDEN = 1.35
_images = {}
_materials = {}


# ---- texturas ---------------------------------------------------------------------------------------

def _noise(seed, size=TEX, octaves=4):
    rng = np.random.default_rng(seed)
    out = np.zeros((size, size), np.float32)
    amp, total = 1.0, 0.0
    for o in range(octaves):
        cells = 4 * 2 ** o
        grid = rng.random((cells + 1, cells + 1)).astype(np.float32)
        grid[-1, :] = grid[0, :]
        grid[:, -1] = grid[:, 0]
        x = np.linspace(0, cells, size, endpoint=False)
        i = x.astype(int)
        f = x - i
        f = f * f * (3 - 2 * f)
        a = grid[np.ix_(i, i)]
        b = grid[np.ix_(i, i + 1)]
        c = grid[np.ix_(i + 1, i)]
        d = grid[np.ix_(i + 1, i + 1)]
        fy = f[:, None]
        fx = f[None, :]
        out += amp * ((a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy)
        total += amp
        amp *= .5
    return out / total


def _pattern(kind, color, seed):
    """Imagem RGB (0-1) da textura de um tipo de material."""
    c = np.array(color, np.float32)
    n = _noise(seed)
    fine = _noise(seed + 7, octaves=6)
    yy, xx = np.mgrid[0:TEX, 0:TEX].astype(np.float32) / TEX
    if kind == "metal":
        # escovado na vertical, manchas suaves e riscos finos
        rng = np.random.default_rng(seed)
        streak = np.repeat(rng.random((1, TEX)).astype(np.float32), TEX, axis=0)
        shade = .82 + .1 * n + .08 * streak + .04 * (fine - .5)
        scratches = (rng.random((TEX, TEX)) > .9985).astype(np.float32)
        img = c[None, None, :] * shade[..., None] + scratches[..., None] * .25
    elif kind == "gold":
        shade = .78 + .22 * n + .06 * np.sin(yy * 60 + n * 6)
        img = c[None, None, :] * shade[..., None]
    elif kind == "wood":
        # veios: anéis deformados pelo ruído ao longo do comprimento
        grain = np.sin((xx * 34 + n * 5.0) * math.pi) * .5 + .5
        knots = np.clip((_noise(seed + 3, octaves=2) - .72) * 6, 0, 1)
        shade = .72 + .2 * grain + .1 * (fine - .5) - .25 * knots
        img = c[None, None, :] * shade[..., None]
    elif kind == "leather":
        # tiras enroladas na diagonal com costura clara
        band = np.mod(yy * 10 + xx * 3, 1.0)
        edge = np.clip(1 - np.abs(band - .5) * 2.4, 0, 1)
        stitch = ((np.mod(xx * 40, 1) < .5) & (np.abs(band - .08) < .02)).astype(np.float32)
        shade = .62 + .3 * edge + .12 * (fine - .5)
        img = c[None, None, :] * shade[..., None] + stitch[..., None] * .22
    elif kind == "cloth":
        weave = (np.sin(xx * TEX * .9) * np.sin(yy * TEX * .9)) * .5 + .5
        shade = .75 + .15 * weave + .1 * (n - .5)
        img = c[None, None, :] * shade[..., None]
    elif kind == "bone":
        shade = .82 + .14 * n + .05 * np.sin(yy * 50 + n * 8)
        img = c[None, None, :] * shade[..., None]
    elif kind in ("gem", "crystal"):
        # facetas: degraus de luz e um brilho no centro
        facet = np.floor((n * 5 + xx * 3)) / 6
        shade = .55 + .35 * np.mod(facet, 1) + .25 * np.exp(-((xx - .5) ** 2 + (yy - .5) ** 2) * 9)
        img = c[None, None, :] * shade[..., None]
    else:
        img = c[None, None, :] * (.85 + .15 * n)[..., None]
    return np.clip(img, 0, 1)


def _image(name, kind, color, seed):
    key = (name, kind, tuple(color))
    if key in _images:
        return _images[key]
    img = bpy.data.images.get("T_" + name)
    if img is None:
        img = bpy.data.images.new("T_" + name, TEX, TEX, alpha=False)
    rgb = _pattern(kind, color, seed)
    # sRGB: a textura guarda cor de exibição
    srgb = np.where(rgb <= .0031308, rgb * 12.92, 1.055 * np.power(rgb, 1 / 2.4) - .055)
    rgba = np.concatenate([srgb, np.ones((TEX, TEX, 1), np.float32)], axis=2)
    img.pixels.foreach_set(rgba.ravel())
    img.pack()
    _images[key] = img
    return img


# kind, cor base (linear), metálico, rugosidade, brilho
PRESETS = {
    "aco": ("metal", (.7, .72, .76), .7, .3, 0),
    "aco_escuro": ("metal", (.4, .41, .45), .7, .4, 0),
    "ferro": ("metal", (.52, .5, .48), .65, .5, 0),
    "aco_azul": ("metal", (.35, .45, .62), 1.0, .3, 0),
    "ouro": ("gold", (.95, .66, .22), 1.0, .28, 0),
    "bronze": ("gold", (.62, .38, .16), 1.0, .38, 0),
    "prata": ("metal", (.85, .87, .9), .75, .2, 0),
    "madeira": ("wood", (.36, .2, .09), 0.0, .7, 0),
    "madeira_clara": ("wood", (.62, .44, .25), 0.0, .7, 0),
    "betula": ("wood", (.86, .8, .68), 0.0, .7, 0),
    "madeira_escura": ("wood", (.15, .08, .04), 0.0, .65, 0),
    "couro": ("leather", (.32, .16, .07), 0.0, .75, 0),
    "couro_escuro": ("leather", (.11, .06, .04), 0.0, .7, 0),
    "couro_vermelho": ("leather", (.45, .07, .05), 0.0, .7, 0),
    "tecido_azul": ("cloth", (.08, .16, .45), 0.0, .9, 0),
    "tecido_vermelho": ("cloth", (.16, .015, .02), 0.0, .9, 0),
    "corda": ("cloth", (.62, .52, .33), 0.0, .9, 0),
    "osso": ("bone", (.85, .8, .66), 0.0, .6, 0),
    "papel": ("cloth", (.86, .8, .62), 0.0, .9, 0),
    "brasa": ("gem", (1.0, .35, .05), 0.0, .4, 6.0),
    "jade": ("gem", (.12, .9, .55), 0.0, .15, 3.0),
    "rubi": ("gem", (.9, .05, .12), 0.0, .15, 2.5),
    "safira": ("gem", (.1, .3, 1.0), 0.0, .15, 2.5),
    "ametista": ("gem", (.55, .2, 1.0), 0.0, .15, 2.5),
    "topazio": ("gem", (1.0, .75, .15), 0.0, .15, 3.0),
    "cristal": ("crystal", (.35, .6, .85), 0.0, .05, 1.5),
    "runa": ("gem", (.35, .85, 1.0), 0.0, .2, 5.0),
    "vidro": ("crystal", (.18, .42, .36), 0.0, .05, .6),
    "folha": ("cloth", (.16, .45, .12), 0.0, .8, 0),
}


def material(name):
    if name in _materials:
        return _materials[name]
    kind, color, metal, rough, glow = PRESETS[name]
    mat = bpy.data.materials.get("MW_" + name) or bpy.data.materials.new("MW_" + name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()
    out = nodes.new("ShaderNodeOutputMaterial")
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    tex = nodes.new("ShaderNodeTexImage")
    tex.image = _image(name, kind, color, abs(hash(name)) % 10000)
    tex.interpolation = "Linear"
    links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Metallic"].default_value = metal
    bsdf.inputs["Roughness"].default_value = rough
    if glow > 0:
        links.new(tex.outputs["Color"], bsdf.inputs["Emission Color"])
        bsdf.inputs["Emission Strength"].default_value = glow
    links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    _materials[name] = mat
    return mat


# ---- peças ------------------------------------------------------------------------------------------

class Weapon:
    """Uma arma em montagem: as peças vão para uma coleção e no fim viram um objeto só."""

    def __init__(self, name, scene=None):
        self.name = name
        self.parts = []
        self.scene = scene or bpy.context.scene
        self.col = bpy.data.collections.get("ARMA_" + name) or bpy.data.collections.new("ARMA_" + name)
        if self.col.name not in self.scene.collection.children:
            self.scene.collection.children.link(self.col)
        for o in list(self.col.objects):
            bpy.data.objects.remove(o, do_unlink=True)

    def _add(self, mesh, mat, loc=(0, 0, 0), rot=(0, 0, 0), bevel=0.0, smooth=False, segments=2):
        obj = bpy.data.objects.new(mesh.name, mesh)
        self.col.objects.link(obj)
        obj.location = loc
        obj.rotation_euler = rot
        obj.data.materials.append(material(mat))
        if bevel > 0:
            mod = obj.modifiers.new("chanfro", "BEVEL")
            mod.width = bevel
            mod.segments = segments
            mod.limit_method = "ANGLE"
            mod.angle_limit = math.radians(35)
        if smooth:
            for p in obj.data.polygons:
                p.use_smooth = True
        self.parts.append(obj)
        return obj

    # caixa com chanfro
    def box(self, mat, size, loc=(0, 0, 0), rot=(0, 0, 0), bevel=.012, taper=None):
        bm = bmesh.new()
        bmesh.ops.create_cube(bm, size=1.0)
        for v in bm.verts:
            v.co = Vector((v.co.x * size[0], v.co.y * size[1], v.co.z * size[2]))
            if taper is not None and v.co.z > 0:
                v.co.x *= taper[0]
                v.co.y *= taper[1]
        mesh = bpy.data.meshes.new(f"{self.name}_box")
        bm.to_mesh(mesh)
        bm.free()
        return self._add(mesh, mat, loc, rot, bevel)

    # cilindro (ou cone com r2) ao longo do eixo Z
    def cyl(self, mat, r, h, loc=(0, 0, 0), rot=(0, 0, 0), r2=None, verts=12, bevel=.006, smooth=True):
        bm = bmesh.new()
        bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=verts, radius1=r, radius2=r if r2 is None else r2, depth=h)
        mesh = bpy.data.meshes.new(f"{self.name}_cyl")
        bm.to_mesh(mesh)
        bm.free()
        return self._add(mesh, mat, loc, rot, bevel, smooth)

    def ring(self, mat, r, tube, loc=(0, 0, 0), rot=(0, 0, 0), segs=16):
        bm = bmesh.new()
        major, minor = segs, 6
        verts = []
        for i in range(major):
            a = 2 * math.pi * i / major
            row = []
            for j in range(minor):
                b = 2 * math.pi * j / minor
                x = (r + tube * math.cos(b)) * math.cos(a)
                y = (r + tube * math.cos(b)) * math.sin(a)
                z = tube * math.sin(b)
                row.append(bm.verts.new((x, y, z)))
            verts.append(row)
        for i in range(major):
            for j in range(minor):
                bm.faces.new((verts[i][j], verts[(i + 1) % major][j], verts[(i + 1) % major][(j + 1) % minor], verts[i][(j + 1) % minor]))
        mesh = bpy.data.meshes.new(f"{self.name}_ring")
        bm.to_mesh(mesh)
        bm.free()
        return self._add(mesh, mat, loc, rot, 0, True)

    def gem(self, mat, r, loc=(0, 0, 0), rot=(0, 0, 0), stretch=1.3):
        bm = bmesh.new()
        bmesh.ops.create_icosphere(bm, subdivisions=1, radius=r)
        for v in bm.verts:
            v.co.z *= stretch
        mesh = bpy.data.meshes.new(f"{self.name}_gem")
        bm.to_mesh(mesh)
        bm.free()
        return self._add(mesh, mat, loc, rot, 0)

    def sphere(self, mat, r, loc=(0, 0, 0), subdiv=2):
        bm = bmesh.new()
        bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=r)
        mesh = bpy.data.meshes.new(f"{self.name}_sphere")
        bm.to_mesh(mesh)
        bm.free()
        return self._add(mesh, mat, loc, (0, 0, 0), 0, True)

    def blade(self, mat, outline, thick, loc=(0, 0, 0), rot=(0, 0, 0), edge_mat=None, ridge=True, plane="XZ", edge=.45):
        """Lâmina a partir do contorno (lista de (x, z), sentido anti-horário): uma placa da grossura
        dada com a borda inteira chanfrada até quase um fio (o chanfro leva o material do fio)."""
        bm = bmesh.new()
        # mais largas e mais grossas que as do Godot: no herói voxel as lâminas finas sumiam
        cxw = sum(p[0] for p in outline) / len(outline)
        outline = [(cxw + (x - cxw) * WIDEN, z) for (x, z) in outline]
        thick *= 1.6
        verts = [bm.verts.new((x, -thick / 2, z)) for (x, z) in outline]
        face = bm.faces.new(verts)
        bmesh.ops.recalc_face_normals(bm, faces=[face])
        res = bmesh.ops.extrude_face_region(bm, geom=[face])
        moved = [e for e in res["geom"] if isinstance(e, bmesh.types.BMVert)]
        bmesh.ops.translate(bm, verts=moved, vec=(0, thick, 0))
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        mesh = bpy.data.meshes.new(f"{self.name}_blade")
        bm.to_mesh(mesh)
        bm.free()
        obj = self._add(mesh, mat, loc, rot, 0)
        mod = obj.modifiers.new("fio", "BEVEL")
        mod.width = thick * edge
        mod.segments = 1
        mod.limit_method = "ANGLE"
        mod.angle_limit = math.radians(60)
        if edge_mat:
            obj.data.materials.append(material(edge_mat))
            mod.material = 1
        if ridge:
            # espinha: uma placa fina mais grossa no meio, só um pouco menor que a lâmina
            cx = sum(p[0] for p in outline) / len(outline)
            inner = [(cx + (x - cx) * .25, z) for (x, z) in outline]
            zs = [z for (_, z) in outline]
            lo, hi = min(zs), max(zs)
            inner = [(x, lo + (z - lo) * .96 + (hi - lo) * .01) for (x, z) in inner]
            bm2 = bmesh.new()
            vs = [bm2.verts.new((x, -thick * .65, z)) for (x, z) in inner]
            f2 = bm2.faces.new(vs)
            bmesh.ops.recalc_face_normals(bm2, faces=[f2])
            r2 = bmesh.ops.extrude_face_region(bm2, geom=[f2])
            bmesh.ops.translate(bm2, verts=[e for e in r2["geom"] if isinstance(e, bmesh.types.BMVert)], vec=(0, thick * 1.3, 0))
            bmesh.ops.recalc_face_normals(bm2, faces=bm2.faces)
            m2 = bpy.data.meshes.new(f"{self.name}_ridge")
            bm2.to_mesh(m2)
            bm2.free()
            sp = self._add(m2, mat, loc, rot, 0)
            mod2 = sp.modifiers.new("chanfro", "BEVEL")
            mod2.width = thick * .5
            mod2.segments = 1
        return obj

    def sweep(self, mat, points, w, h, loc=(0, 0, 0), rot=(0, 0, 0), twist_up=(0, 0, 1)):
        """Tira contínua de seção retangular (w x h) ao longo dos pontos: braços de arco, chicote."""
        pts = [Vector(p) for p in points]
        bm = bmesh.new()
        rings = []
        side = None
        for i, p in enumerate(pts):
            t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
            if side is None:
                side = t.cross(Vector(twist_up))
                if side.length < 1e-4:
                    side = t.cross(Vector((1, 0, 0)))
            # transporte paralelo: a seção acompanha a curva sem virar de repente
            side = (side - t * side.dot(t)).normalized()
            up = side.cross(t).normalized()
            k = 1.0 - .35 * (abs(i / max(1, len(pts) - 1) - .5) * 2) if len(pts) > 2 else 1.0
            ww, hh = w * k, h * k
            ring = [bm.verts.new(p + side * sx * ww / 2 + up * sy * hh / 2) for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
            rings.append(ring)
        for a, b2 in zip(rings, rings[1:]):
            for j in range(4):
                bm.faces.new((a[j], a[(j + 1) % 4], b2[(j + 1) % 4], b2[j]))
        bm.faces.new(rings[0][::-1])
        bm.faces.new(rings[-1])
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        mesh = bpy.data.meshes.new(f"{self.name}_sweep")
        bm.to_mesh(mesh)
        bm.free()
        return self._add(mesh, mat, loc, rot, min(w, h) * .2)

    def wrap(self, mat, r, h, turns, loc=(0, 0, 0), rot=(0, 0, 0)):
        """Tiras de couro enroladas no cabo: anéis inclinados ao longo de h."""
        out = []
        for i in range(turns):
            z = -h / 2 + (i + .5) * h / turns
            o = self.ring(mat, r, r * .16, (0, 0, z), (math.radians(14), 0, 0), segs=14)
            out.append(o)
        if loc != (0, 0, 0) or rot != (0, 0, 0):
            pivot = Matrix.Translation(Vector(loc)) @ Matrix.Rotation(rot[0], 4, "X") @ Matrix.Rotation(rot[1], 4, "Y") @ Matrix.Rotation(rot[2], 4, "Z")
            for o in out:
                o.matrix_world = pivot @ o.matrix_world
        return out

    def spike(self, mat, r, h, loc, rot=(0, 0, 0), verts=6):
        return self.cyl(mat, r, h, loc, rot, r2=0.0, verts=verts, bevel=0, smooth=False)

    # ---- fechamento ---------------------------------------------------------------------------------

    def finish(self):
        """Aplica os modificadores, junta tudo num objeto e mapeia as texturas (projeção em caixa)."""
        deps = bpy.context.evaluated_depsgraph_get()
        meshes = []
        for o in self.parts:
            ev = o.evaluated_get(deps)
            m = bpy.data.meshes.new_from_object(ev)
            m.transform(o.matrix_world)
            meshes.append((m, [s.material for s in o.material_slots]))
        bm = bmesh.new()
        mats = []
        for m, slots in meshes:
            offset = len(bm.verts)
            tmp = bmesh.new()
            tmp.from_mesh(m)
            remap = {}
            for i, s in enumerate(slots):
                if s not in mats:
                    mats.append(s)
                remap[i] = mats.index(s)
            for f in tmp.faces:
                f.material_index = remap.get(f.material_index, 0)
            tmp.to_mesh(m)
            tmp.free()
            bm.from_mesh(m)
            bpy.data.meshes.remove(m)
        for o in self.parts:
            bpy.data.objects.remove(o, do_unlink=True)
        self.parts = []
        mesh = bpy.data.meshes.new("W_" + self.name)
        bm.to_mesh(mesh)
        bm.free()
        for m in mats:
            mesh.materials.append(m)
        obj = bpy.data.objects.new("W_" + self.name, mesh)
        self.col.objects.link(obj)
        # UV: projeção em caixa (cada face pela direção da normal), 1 textura a cada ~0,5 m do item
        uv = mesh.uv_layers.new(name="UV")
        for poly in mesh.polygons:
            nrm = poly.normal
            ax = max(range(3), key=lambda k: abs(nrm[k]))
            for li in poly.loop_indices:
                co = mesh.vertices[mesh.loops[li].vertex_index].co
                if ax == 0:
                    u, v = co.y, co.z
                elif ax == 1:
                    u, v = co.x, co.z
                else:
                    u, v = co.x, co.y
                uv.data[li].uv = (u * 2.0, v * 2.0)
        for p in mesh.polygons:
            p.use_smooth = len(p.vertices) > 4 or p.area < .0004
        self.obj = obj
        return obj

    def export(self, folder, frames):
        """W_<tipo>.glb (mão direita) e W_<tipo>_L.glb (armas de par), no referencial da mão."""
        os.makedirs(folder, exist_ok=True)
        gl_to_bl = Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))
        frame = frames[self.name]
        written = []
        for side, key in (("", "right"), ("_L", "left")):
            if key not in frame:
                continue
            m = Matrix(frame[key])
            mb = gl_to_bl @ m @ gl_to_bl.inverted()
            copy = self.obj.copy()
            copy.data = self.obj.data.copy()
            copy.name = f"W_{self.name}{side}"
            copy.data.name = copy.name
            self.col.objects.link(copy)
            copy.data.transform(mb)
            if mb.determinant() < 0:
                copy.data.flip_normals()
            copy.data.update()
            scene = self.scene
            layer = scene.view_layers[0]
            for o in scene.objects:
                o.select_set(False, view_layer=layer)
            copy.select_set(True, view_layer=layer)
            path = os.path.join(folder, f"W_{self.name}{side}.glb")
            with bpy.context.temp_override(scene=scene, view_layer=layer, active_object=copy, selected_objects=[copy]):
                bpy.ops.export_scene.gltf(filepath=path, use_selection=True, export_format="GLB", export_apply=True,
                                          export_image_format="AUTO", export_materials="EXPORT", export_yup=True)
            written.append(path)
            bpy.data.objects.remove(copy, do_unlink=True)
        return written

    def export_left(self, folder, frames):
        """Peça só da mão esquerda (o escudo): W_<tipo>_L.glb no referencial "shield" de frames.json."""
        gl_to_bl = Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))
        m = Matrix(frames[self.name]["shield"])
        mb = gl_to_bl @ m @ gl_to_bl.inverted()
        self.obj.data.transform(mb)
        if mb.determinant() < 0:
            self.obj.data.flip_normals()
        self.obj.name = f"W_{self.name}_L"
        layer = self.scene.view_layers[0]
        for o in self.scene.objects:
            o.select_set(False, view_layer=layer)
        self.obj.select_set(True, view_layer=layer)
        path = os.path.join(folder, f"W_{self.name}_L.glb")
        with bpy.context.temp_override(scene=self.scene, view_layer=layer, active_object=self.obj, selected_objects=[self.obj]):
            bpy.ops.export_scene.gltf(filepath=path, use_selection=True, export_format="GLB", export_apply=True,
                                      export_image_format="AUTO", export_materials="EXPORT", export_yup=True)
        return path


def export_textures(folder):
    """As texturas usadas (T_<material>.png) e os números de cada material (materials.json) para a Unreal."""
    import json
    os.makedirs(folder, exist_ok=True)
    info = {}
    for name in _materials:
        kind, color, metal, rough, glow = PRESETS[name]
        img = bpy.data.images.get("T_" + name)
        if img:
            img.filepath_raw = os.path.join(folder, f"T_{name}.png")
            img.file_format = "PNG"
            img.save()
        info[name] = {"metallic": metal, "roughness": rough, "glow": glow}
    path = os.path.join(folder, "materials.json")
    old = json.load(open(path, encoding="utf-8")) if os.path.exists(path) else {}
    old.update(info)
    json.dump(old, open(path, "w", encoding="utf-8"), indent=1)
    return len(info)
