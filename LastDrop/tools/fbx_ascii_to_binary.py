# -*- coding: utf-8 -*-
# =====================================================================
# fbx_ascii_to_binary.py
#   ASCII 形式の FBX を、Blender で読めるバイナリ FBX に作り直す。
#   Blender の FBX インポータは ASCII 形式を読めない（"ASCII FBX files are not supported"）ので、
#   fbx_to_gamelib.py の前にこれを通す。
#
#   読むのはメッシュ（頂点・面・法線・UV・マテリアル番号）と、その置き場所（Model の親子と Lcl 変換）だけ。
#   ボーン・スキン・アニメは読まない（銃など「動かないモデル」として使う前提）。
#   ASCII FBX はテクスチャへの参照を持たないことが多いので、画像はコマンド引数で渡してマテリアルに繋ぐ。
#
# 使い方:
#   blender -b --factory-startup --python tools/fbx_ascii_to_binary.py -- <入力.fbx> <出力.fbx>
#       [--texture <色.png>] [--normalmap <法線.png>]
#   出力 FBX を tools/configs/*.json の fbx_path に書き、fbx_to_gamelib.py で変換する。
# =====================================================================

import bpy
import mathutils
import math
import re
import sys

# FBX の RotationOrder（enum）→ Blender のオイラー順。どちらも「先に掛かる軸から」書く。
FBX_ROTATION_ORDER = {0: "XYZ", 1: "XZY", 2: "YZX", 3: "YXZ", 4: "ZXY", 5: "ZYX"}
FBX_UP_AXIS_Y = 1       # GlobalSettings の UpAxis。Y-up 以外は想定していない
CENTIMETERS_PER_METER = 100.0  # UnitScaleFactor は「1単位が何cmか」

# FBX(Y-up 右手) → Blender(Z-up 右手)。Blender 標準の FBX インポータと同じ向き (x, y, z) → (x, -z, y)。
Y_UP_TO_Z_UP = mathutils.Matrix((
    (1.0, 0.0, 0.0, 0.0),
    (0.0, 0.0, -1.0, 0.0),
    (0.0, 1.0, 0.0, 0.0),
    (0.0, 0.0, 0.0, 1.0),
))


# ------------------------- ASCII FBX の読み取り -------------------------

class Node:
    # FBX の1ノード。name: 値… { 子ノード } の形。
    def __init__(self, name, values):
        self.name = name
        self.values = values
        self.children = []

    def child(self, name):
        return next((c for c in self.children if c.name == name), None)

    def children_named(self, name):
        return [c for c in self.children if c.name == name]

    def array(self):
        # "Vertices: *N { a: ... }" の a: の中身。
        a = self.child("a")
        return a.values if a else []


VALUE_TOKEN = re.compile(r'"[^"]*"|[^,\s]+')
KEY_LINE = re.compile(r'^\s*([A-Za-z_][\w|]*):\s?(.*)$')


def parse_value(token):
    if token.startswith('"'):
        return token[1:-1]
    try:
        return int(token)
    except ValueError:
        pass
    try:
        return float(token)
    except ValueError:
        return token  # "*16650" のような要素数表記や T/Y などの記号


def parse_values(text):
    return [parse_value(t) for t in VALUE_TOKEN.findall(text)]


def parse_fbx(path):
    # 行単位で読む。長い配列（a: ...）は次の行へ続くので、次のキー行か } までを同じ値として繋ぐ。
    root = Node("", [])
    stack = [root]
    pending = None  # 続きの行を待っている葉ノード: (Node, [テキスト...])

    def flush():
        nonlocal pending
        if pending:
            pending[0].values = parse_values(",".join(pending[1]))
            pending = None

    with open(path, encoding="utf-8", errors="replace") as fp:
        for raw in fp:
            line = raw.strip()
            if not line or line.startswith(";"):
                continue
            if line == "}":
                flush()
                stack.pop()
                continue
            match = KEY_LINE.match(line)
            if not match:
                if pending:
                    pending[1].append(line)  # 配列の続き
                continue
            flush()
            name, rest = match.group(1), match.group(2).strip()
            opens = rest.endswith("{")
            if opens:
                rest = rest[:-1].strip()
            node = Node(name, [])
            stack[-1].children.append(node)
            if opens:
                node.values = parse_values(rest)
                stack.append(node)
            else:
                pending = (node, [rest])
    flush()
    return root


def properties(node):
    # Properties70 の P: "名前", 型, …, 値… を {名前: [値…]} にする。
    result = {}
    p70 = node.child("Properties70")
    if p70:
        for p in p70.children_named("P"):
            result[p.values[0]] = p.values[4:]
    return result


# ------------------------- 置き場所（Model の親子） -------------------------

def local_matrix(model):
    props = properties(model)
    t = props.get("Lcl Translation", [0.0, 0.0, 0.0])
    r = props.get("Lcl Rotation", [0.0, 0.0, 0.0])
    s = props.get("Lcl Scaling", [1.0, 1.0, 1.0])
    for unsupported in ("PreRotation", "PostRotation", "RotationPivot", "ScalingPivot", "RotationOffset"):
        if unsupported in props and any(abs(v) > 1e-6 for v in props[unsupported]):
            # 読まない項目に値があると置き場所がずれるので、黙って進めない。
            raise RuntimeError("%s の %s には対応していません。" % (model.values[1], unsupported))
    order = FBX_ROTATION_ORDER[int(props.get("RotationOrder", [0])[0])]
    rotation = mathutils.Euler([math.radians(v) for v in r], order).to_matrix().to_4x4()
    scale = mathutils.Matrix.Diagonal((s[0], s[1], s[2], 1.0))
    return mathutils.Matrix.Translation(t) @ rotation @ scale


def global_matrix(model_id, models, parent_of):
    matrix = mathutils.Matrix.Identity(4)
    while model_id in models:
        matrix = local_matrix(models[model_id]) @ matrix
        model_id = parent_of.get(model_id)
    return matrix


# ------------------------- メッシュ -------------------------

def polygons_of(indices):
    # PolygonVertexIndex は、面の最後の頂点だけ ~i（負数）で入っている。
    polygons = []
    current = []
    for i in indices:
        if i < 0:
            current.append(~i)
            polygons.append(current)
            current = []
        else:
            current.append(i)
    return polygons


def per_loop(layer, key, width, loop_number, polygon_vertex_index):
    # レイヤ要素（法線・UV）を「面の角(loop)ごとの値」の並びにする。
    data = layer.child(key).array()
    mapping = layer.child("MappingInformationType").values[0]
    reference = layer.child("ReferenceInformationType").values[0]
    index_node = layer.child(key + "Index") or layer.child(key.rstrip("s") + "Index")
    values = [tuple(data[i * width:(i + 1) * width]) for i in range(len(data) // width)]
    if reference == "IndexToDirect":
        values = [values[i] for i in index_node.array()]
    if mapping == "ByPolygonVertex":
        return values
    if mapping in ("ByVertex", "ByVertice"):
        return [values[v] for v in polygon_vertex_index]
    raise RuntimeError("%s の MappingInformationType %s には対応していません。" % (layer.name, mapping))


def build_mesh(geometry, name):
    vertices = geometry.child("Vertices").array()
    positions = [tuple(vertices[i:i + 3]) for i in range(0, len(vertices), 3)]
    raw_index = geometry.child("PolygonVertexIndex").array()
    polygons = polygons_of(raw_index)
    polygon_vertex_index = [v for poly in polygons for v in poly]

    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(positions, [], polygons)

    uv_layer = geometry.child("LayerElementUV")
    if uv_layer:
        uvs = per_loop(uv_layer, "UV", 2, len(polygon_vertex_index), polygon_vertex_index)
        layer = mesh.uv_layers.new(name="UVMap")
        for loop, uv in zip(layer.data, uvs):
            loop.uv = uv

    material_layer = geometry.child("LayerElementMaterial")
    if material_layer and material_layer.child("MappingInformationType").values[0] == "ByPolygon":
        for poly, index in zip(mesh.polygons, material_layer.child("Materials").array()):
            poly.material_index = index

    mesh.validate()
    # 法線は FBX に入っている面の角ごとの値をそのまま使う（角ばった面・なめらかな面の区別を保つ）。
    normal_layer = geometry.child("LayerElementNormal")
    if normal_layer:
        normals = per_loop(normal_layer, "Normals", 3, len(polygon_vertex_index), polygon_vertex_index)
        mesh.normals_split_custom_set(normals)
    return mesh


# ------------------------- マテリアル -------------------------

def make_material(name, texture, normalmap):
    # fbx_to_gamelib.py の linked_image が探す形（Base Color ← 画像、Normal ← Normal Map ← 画像）で繋ぐ。
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    if texture:
        image_node = nodes.new("ShaderNodeTexImage")
        image_node.image = bpy.data.images.load(texture)
        links.new(image_node.outputs["Color"], bsdf.inputs["Base Color"])
    if normalmap:
        image_node = nodes.new("ShaderNodeTexImage")
        image_node.image = bpy.data.images.load(normalmap)
        image_node.image.colorspace_settings.name = "Non-Color"  # 法線は色ではないので色変換させない
        normal_node = nodes.new("ShaderNodeNormalMap")
        links.new(image_node.outputs["Color"], normal_node.inputs["Color"])
        links.new(normal_node.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


# ------------------------- main -------------------------

def parse_args():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    options = {"texture": "", "normalmap": ""}
    paths = []
    i = 0
    while i < len(args):
        if args[i] in ("--texture", "--normalmap"):
            options[args[i][2:]] = args[i + 1]
            i += 2
        else:
            paths.append(args[i])
            i += 1
    if len(paths) != 2:
        raise RuntimeError("使い方: -- <入力.fbx> <出力.fbx> [--texture 色.png] [--normalmap 法線.png]")
    return paths[0], paths[1], options


def main():
    in_path, out_path, options = parse_args()
    root = parse_fbx(in_path)

    settings = properties(root.child("GlobalSettings"))
    if int(settings.get("UpAxis", [FBX_UP_AXIS_Y])[0]) != FBX_UP_AXIS_Y:
        raise RuntimeError("Y-up 以外の FBX には対応していません。")
    meters_per_unit = float(settings.get("UnitScaleFactor", [1.0])[0]) / CENTIMETERS_PER_METER

    objects = root.child("Objects")
    models = {m.values[0]: m for m in objects.children_named("Model")}
    geometries = {g.values[0]: g for g in objects.children_named("Geometry") if g.values[2] == "Mesh"}
    materials = {m.values[0]: m for m in objects.children_named("Material")}

    # Connections の "OO", 子, 親。子→親 を引けるようにする。
    parent_of = {}
    materials_of = {}
    geometry_of = {}
    for c in root.child("Connections").children_named("C"):
        if c.values[0] != "OO":
            continue
        child, parent = c.values[1], c.values[2]
        if child in models and parent in models:
            parent_of[child] = parent
        elif child in geometries and parent in models:
            geometry_of[parent] = child
        elif child in materials and parent in models:
            materials_of.setdefault(parent, []).append(child)  # 接続の順番がマテリアル番号

    bpy.ops.wm.read_factory_settings(use_empty=True)
    to_blender = Y_UP_TO_Z_UP @ mathutils.Matrix.Scale(meters_per_unit, 4)
    for model_id, geometry_id in geometry_of.items():
        model_name = models[model_id].values[1].split("::")[-1]
        mesh = build_mesh(geometries[geometry_id], model_name)
        for material_id in materials_of.get(model_id, []):
            material_name = materials[material_id].values[1].split("::")[-1]
            mesh.materials.append(make_material(material_name, options["texture"], options["normalmap"]))
        obj = bpy.data.objects.new(model_name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        # 置き場所は頂点へ焼き込む（出力の Model は原点・向き0・等倍になる）。
        mesh.transform(to_blender @ global_matrix(model_id, models, parent_of))
        print("MESH", model_name, "vertices", len(mesh.vertices), "polygons", len(mesh.polygons),
              "materials", [m.name for m in mesh.materials])

    bpy.ops.export_scene.fbx(filepath=out_path, path_mode="ABSOLUTE", object_types={"MESH"})
    print("WROTE", out_path)


if __name__ == "__main__":
    main()
