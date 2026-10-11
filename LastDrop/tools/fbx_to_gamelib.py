# -*- coding: utf-8 -*-
# =====================================================================
# fbx_to_gamelib.py
#   FBX を GameLib の Scene 形式(ニセxml)へ変換する。
#   mode="skinned": <Container> に <Tree>(骨格) / <Animation>(アニメ) / <SkinnedMesh>(頂点+重み)。
#                   ゲーム側は Container が Tree/Animation を、SkinnedMeshLoader が SkinnedMesh を読む。
#   mode="static" : <Container> に <StaticMesh>(頂点)。建物など動かないモデル用。StaticMeshLoader が読む。
#
# モデルごとの設定は tools/configs/*.json に書く（スクリプト本体はモデルごとに書き換えない）。
#
# 使い方A（コマンド。FBX の読み込みから書き出しまで自動）:
#   blender -b --factory-startup --python tools/fbx_to_gamelib.py -- tools/configs/building.json
# 使い方B（Blender の画面で実行）:
#   1. Blender で対象 FBX を開く（または File > Import > FBX）。
#   2. Scripting タブでこのファイルを開き、下の CONFIG_FILE に設定ファイルのパスを書く。
#   3. Run Script。設定の out_path に .txt が書き出される。
#
# ★較正が必要な2箇所（初回のみ）:
#   (A) AXIS_CONVERT … Blender(Z-up 右手) → エンジン(Y-up 右手) の座標変換。
#   (B) ROT_ORDER    … アニメ回転を分解するオイラー順。エンジンは rotateY→X→Z。
#   検証手順: まず1関節だけ Z 回転するモデルで出力し、skintest の曲げ(rotationZ)と
#   符号・向きが一致するか確認 → 合わなければ (A)(B) を調整、の順で詰める。
#   ※1軸だけの回転では (B) の順序の誤りは現れない。順序は、複数軸が同時に回る関節を持つモデルで
#     「出力したデータをエンジンと同じ式で組んだ頂点」と「Blender の変形後の頂点」を比べて確かめる。
# =====================================================================

import bpy
import mathutils
import numpy
import math
import json
import os
import sys

# ------------------------- CONFIG -------------------------
# 使い方B（画面で実行）のときに読む設定ファイル。コマンド引数で渡した場合はそちらが優先。
CONFIG_FILE = ""

# 既定値。設定ファイルに書いた項目だけが上書きされる。特定モデル向けの値はここに書かない。
# パス(fbx_path / out_path)は、設定ファイルからの相対パスで書ける。
CONFIG = {
    "mode": "skinned",          # "skinned"(骨+アニメ) / "static"(動かないモデル)
    "fbx_path": "",             # 使い方A で読み込む FBX
    "out_path": "",             # 書き出す .txt
    # True: (頂点, UV, 法線, 色) の組ごとに頂点を分ける。UVの継ぎ目・角が正しくなり、面に使われない頂点も出力しない。
    # False は旧来の近似（1頂点=1UV。継ぎ目でテクスチャがずれる）。比較・切り分け用に残している。
    "split_vertices": True,
    "mesh_names": [],           # 書き出すメッシュのオブジェクト名。空なら全メッシュ
    "tree_name": "skel",       # <Tree name>。ゲーム側 createTree(この名前)
    "mesh_texture": "",         # <SkinnedMesh texture>。空なら無地
    "mesh_normalmap": "",       # <SkinnedMesh normalmap>。空なら法線マップ無し
    # 材質値（任意）。書いたものだけ出力し、無ければゲーム側 MeshMaterial の既定値になる。
    # 例: {"specular": (0.01, 0.01, 0.01), "sharpness": 8.0, "normalstrength": 0.3, "diffuse": (1, 1, 1)}
    "material_params": {},
    "frame_step": 1,            # アニメを何フレームおきにサンプリングするか
    "translate_root_only": True,  # True: 根ボーンだけ移動カーブを出す(関節は回転のみ)
    "flip_winding": False,      # 三角形の巻き順を反転する（AXIS_CONVERT が鏡像変換のときだけ必要）
    "anim_name": "",            # 空でない場合、出力アニメ名をこれに固定する(Take 001対策)
    # マテリアル名 → 頂点カラー(RGB, 0..1)。テクスチャ無しでも部位ごとに色を出す。
    # 未登録のマテリアルは default_color。テクスチャを貼る場合は使われない（白になる）。
    "material_colors": {},
    "default_color": (0.6, 0.6, 0.6),
    # マテリアル名 → そのパーツの見た目。書くと <SkinnedMesh> の中をマテリアルごとの <Part> に分け、
    # パーツごとに別のテクスチャで描く（マテリアルごとに別のテクスチャ組を持つモデル用。Mixamo の体/髪など）。
    # 空なら従来どおり mesh_texture の1組で全体を描く。
    # 値に書けるもの: texture / normalmap（ゲームの作業フォルダ基準のパス）と material_params と同じ材質値。
    # 例: {"Ch15_body": {"texture": "Resource/a.tga", "normalmap": "Resource/a_n.tga", "sharpness": 16.0}}
    "parts": {},
    # >0 なら、parts のテクスチャを FBX 内の画像（Base Color / Normal Map に繋がった画像）から
    # この一辺の画素数へ縮小して TGA で書き出す。0 なら書き出さない（手作業で用意する）。
    "texture_export_size": 0,
    # parts のテクスチャパスの基準フォルダ（＝ゲームの作業フォルダ）。設定ファイルからの相対パス。
    "resource_root": "",
    # 同じ骨格のアニメだけの FBX（Mixamo の Without Skin）を追加で読み、<Animation> を足す。
    # 例: [{"fbx_path": "Walking.fbx", "name": "walk"}]
    # "in_place": true を書くと、根ボーンの水平移動（X/Z）をレスト姿勢の位置に固定する。
    #   前へ進む動き（ルートモーション）入りのアニメを、移動をゲーム側で行うキャラに使うため。上下の揺れは残す。
    "extra_animations": [],
    # False なら、fbx_path の FBX 自体に入っているアニメは出力しない（メッシュだけ使い、アニメは extra_animations で揃える）。
    "fbx_animations": True,
    # >0 なら、アニメのキー時刻を「ゲームのフレーム数」へ直す（FBX の fps とゲームの fps の比を掛ける）。
    # ゲーム側はアニメ時刻を1フレームに1進めるので、30fps のアニメを 60fps のゲームで等速にするのに使う。
    # 0 なら Blender のフレーム番号のまま（従来の出力）。
    "game_fps": 0,
    # static モードのみ。モデルの置き方を変換時に直す（実物大・原点の揃ったデータにして、ゲーム側で拡大率や
    # ずれを補わずに済ませる。当たり判定の箱は頂点の範囲から作るので、ゲーム側で縮めると見た目とずれる）。
    # 頂点に掛ける拡大率（1 で等倍）。[x, y, z] と書くと軸ごとに変えられる（Blender に読み込んだ向きの軸。Z が上）。
    # 軸ごとの拡大は、部品を決まった長さへ伸び縮みさせるためのもの（歩道の帯を交差点の幅に合わせる、など）。
    "scale": 1.0,
    "center_origin": False,     # True: 原点を「真上から見た中心・一番下」へ移す（地面に置いて、その場で回せるように）
    # static モードのみ。mesh_texture / mesh_normalmap の元にする画像ファイル（設定ファイルからの相対パスで書ける）。
    # 書いた項目は、マテリアルに繋がった画像ではなくこのファイルから書き出す。
    # FBX の中の画像の参照が切れているモデル用（作者の PC のパスが入っていて、読み込んでも画像が空になる）。
    # 例: {"mesh_texture": "textures/a.png", "mesh_normalmap": "textures/a_n.png"}
    "texture_sources": {},
}

# (A) 座標変換: Blender(x,y,z: Z-up 右手) → エンジン(Y-up 右手)。
#     GameLib は右手系（Matrix34::setViewTransform は x = 上×z、カメラは -Z を見る）なので、
#     X軸まわりの -90度回転 (x, y, z) → (x, z, -y) にする（行列式+1＝鏡像にならない）。
#     Blender の正面(-Y)はエンジンの +Z（カメラ側）を向く。
#     ※以前は「YとZを入れ替える」(行列式-1)で、全モデルが左右反転していた（建物のドア位置で発覚）。
AXIS_CONVERT = mathutils.Matrix((
    (1.0, 0.0, 0.0, 0.0),
    (0.0, 0.0, 1.0, 0.0),
    (0.0, -1.0, 0.0, 0.0),
    (0.0, 0.0, 0.0, 1.0),
))

# (B) 回転を分解するオイラー順。
#     エンジン(Scene::Node / TreeTemplate)は M = T * Ry * Rx * Rz * S（rotateY→X→Z を右から掛ける）で、
#     頂点には Z→X→Y の順に回転が掛かる。Blender の順序名は「先に掛かる軸から」書くので、これは "ZXY"。
#     ※以前は "YXZ"（行列 Rz*Rx*Ry）で、複数軸が同時に回る関節（Mixamo の肩・腕など）が大きくずれていた。
#       1軸ずつしか回らない関節は順序に関係なく一致するので、ゾンビの歩きでは目立たなかった。
ROT_ORDER = "ZXY"

MAX_INFLUENCE = 4  # SkinnedMesh::MAX_INFLUENCE と一致させること
MAX_VERTEX_NUMBER = 65536  # MeshLoaderUtil::MAX_VERTEX_NUMBER と一致させること（16bitインデックス）
# ----------------------------------------------------------


def f(x):
    # 小数を短く整形する（-0 を 0 に、末尾ゼロを削る）。
    if abs(x) < 1e-6:
        x = 0.0
    return ("%.6f" % x).rstrip("0").rstrip(".")


def sanitize(name):
    # 名前を [A-Za-z0-9_] に丸める（"...|Take 001|..." のような名前対策）。
    return "".join(c if (c.isalnum() or c == "_") else "_" for c in name)


def to_engine(mat):
    # Blender 行列 → エンジン座標の行列（相似変換）。
    return AXIS_CONVERT @ mat @ AXIS_CONVERT.inverted()


NORMAL_CONV = AXIS_CONVERT.to_3x3()


def conv_point(co):
    # 点（位置）を Blender → エンジン座標へ。
    v = AXIS_CONVERT @ mathutils.Vector((co[0], co[1], co[2], 1.0))
    return v.to_3d()


def conv_normal(n):
    # 法線を Blender → エンジン座標へ。
    return (NORMAL_CONV @ mathutils.Vector((n[0], n[1], n[2]))).normalized()


def find_armature():
    for o in bpy.data.objects:
        if o.type == "ARMATURE":
            return o
    return None


def adjust_static_transform(mesh_objs):
    # scale / center_origin を、全メッシュの置き方（ワールド行列）に掛ける。頂点はワールド座標で書き出すので、
    # ここで動かせば出力に反映される。親子関係があるモデルで子まで二重に動かさないよう、根のオブジェクトだけ動かす。
    scale = CONFIG["scale"]
    factors = tuple(scale) if isinstance(scale, (list, tuple)) else (scale, scale, scale)
    if factors == (1.0, 1.0, 1.0) and not CONFIG["center_origin"]:
        return
    offset = mathutils.Vector((0.0, 0.0, 0.0))
    if CONFIG["center_origin"]:
        # Blender は Z が上なので、X/Y の中心と Z の一番下を原点へ持ってくる。
        points = [o.matrix_world @ v.co for o in mesh_objs for v in o.data.vertices]
        low = [min(p[i] for p in points) for i in range(3)]
        high = [max(p[i] for p in points) for i in range(3)]
        offset = mathutils.Vector(((low[0] + high[0]) * 0.5, (low[1] + high[1]) * 0.5, low[2]))
    adjust = mathutils.Matrix.Diagonal((factors[0], factors[1], factors[2], 1.0)) @ mathutils.Matrix.Translation(-offset)
    for obj in [o for o in bpy.data.objects if o.parent is None]:
        obj.matrix_world = adjust @ obj.matrix_world
    bpy.context.view_layer.update()  # 子の matrix_world を新しい親の位置で計算し直す
    print("ADJUST", "scale", " ".join(f(x) for x in factors), "offset", f(offset.x), f(offset.y), f(offset.z))


def find_meshes():
    # 書き出すメッシュを返す。mesh_names が空なら全メッシュ（ゾンビは7分割なので複数対応が必須）。
    # 不要なメッシュ（作業用の残骸など）が混ざっているモデルでは、mesh_names で必要なものに絞る。
    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    names = CONFIG["mesh_names"]
    if names:
        missing = [n for n in names if n not in [o.name for o in meshes]]
        if missing:
            raise RuntimeError("mesh_names のメッシュが見つかりません: %s（あるもの: %s）" % (missing, [o.name for o in meshes]))
        meshes = [o for o in meshes if o.name in names]
    return meshes


def obj_color(obj):
    # メッシュのマテリアル名から頂点カラー(RGB)を決める。
    name = ""
    if obj.material_slots and obj.material_slots[0].material:
        name = obj.material_slots[0].material.name
    return CONFIG["material_colors"].get(name, CONFIG["default_color"])


# 骨行列は全てワールド空間で統一する（メッシュのワールド座標と噛み合わせるため）。

def rest_world(bone, arm):
    return arm.matrix_world @ bone.matrix_local


def rest_local(bone, arm):
    if bone.parent:
        return rest_world(bone.parent, arm).inverted() @ rest_world(bone, arm)
    return rest_world(bone, arm)


def pose_world(pbone, arm):
    return arm.matrix_world @ pbone.matrix


def pose_local(pbone, arm):
    if pbone.parent:
        return pose_world(pbone.parent, arm).inverted() @ pose_world(pbone, arm)
    return pose_world(pbone, arm)


# ---------------------- <Tree> 出力 ----------------------

def emit_node(bone, arm, indent):
    # ボーン1つを <Node> として出力し、子を再帰する。
    local = to_engine(rest_local(bone, arm))
    t, q, s = local.decompose()
    e = q.to_euler(ROT_ORDER)
    pad = "\t" * indent
    attrs = 'name="%s"' % bone.name
    attrs += ' translation="%s, %s, %s"' % (f(t.x), f(t.y), f(t.z))
    attrs += ' rotation="%s, %s, %s"' % (f(math.degrees(e.x)), f(math.degrees(e.y)), f(math.degrees(e.z)))
    attrs += ' scale="%s, %s, %s"' % (f(s.x), f(s.y), f(s.z))

    children = [c for c in bone.children]
    if not children:
        return '%s<Node %s/>\n' % (pad, attrs)
    out = '%s<Node %s>\n' % (pad, attrs)
    for c in children:
        out += emit_node(c, arm, indent + 1)
    out += '%s</Node>\n' % pad
    return out


def emit_tree(armature):
    roots = [b for b in armature.data.bones if b.parent is None]
    # <Tree> はルートノードが1つだけ。複数根なら最初の1つを使う（要調整）。
    root = roots[0]
    out = '\t<Tree name="%s">\n' % CONFIG["tree_name"]
    out += emit_node(root, armature, 2)
    out += '\t</Tree>\n'
    return out


# ------------------- <Animation> 出力 -------------------

def emit_curve(indent, ctype, times, values):
    pad = "\t" * indent
    out = '%s<Curve type="%s" interporation="linear">\n' % (pad, ctype)
    for tm, v in zip(times, values):
        out += '%s\t<Key time="%s" value="%s"/>\n' % (pad, f(tm), f(v))
    out += '%s</Curve>\n' % pad
    return out


def key_time_scale():
    # Blender のフレーム番号 → 出力するキー時刻 の倍率。
    if CONFIG["game_fps"] > 0:
        return float(CONFIG["game_fps"]) / bpy.context.scene.render.fps
    return 1.0


def emit_animation(armature, action, anim_name, in_place=False):
    scene = bpy.context.scene
    if armature.animation_data is None:
        armature.animation_data_create()
    armature.animation_data.action = action
    # Blender 4.4+ のアクションは「スロット」単位で ID に結び付く。別の骨格（extra_animations の FBX）から
    # 持ってきたアクションはスロットが自動で選ばれず、何も動かない（全キーがレスト姿勢の値になる）ので、明示する。
    if getattr(armature.animation_data, "action_slot", 1) is None and action.slots:
        armature.animation_data.action_slot = action.slots[0]
    start = int(action.frame_range[0])
    end = int(action.frame_range[1])
    time_scale = key_time_scale()

    # レスト親相対（エンジン空間）を先に取っておく。
    rest = {}
    for b in armature.data.bones:
        rest[b.name] = to_engine(rest_local(b, armature))

    # 各ボーンのカーブ用サンプルを貯める。
    times = []
    samples = {b.name: {"t": [], "r": []} for b in armature.data.bones}
    roots = set(b.name for b in armature.data.bones if b.parent is None)

    frame = start
    while frame <= end:
        scene.frame_set(frame)
        times.append((frame - start) * time_scale)  # 0 起点
        for pb in armature.pose.bones:
            animated = to_engine(pose_local(pb, armature))
            if in_place and pb.name in roots:
                # 根の親相対＝ワールド（エンジン空間, Y-up）なので、水平は X/Z。
                animated.translation.x = rest[pb.name].translation.x
                animated.translation.z = rest[pb.name].translation.z
            # エンジンは world = parent * rest * delta。delta = rest^-1 * animated。
            delta = rest[pb.name].inverted() @ animated
            t, q, s = delta.decompose()
            e = q.to_euler(ROT_ORDER)
            samples[pb.name]["t"].append(t)
            samples[pb.name]["r"].append(e)
        frame += CONFIG["frame_step"]

    out ='\t<Animation name="%s">\n' % anim_name
    for pb in armature.pose.bones:
        s = samples[pb.name]
        node = '\t\t<Node name="%s">\n' % pb.name
        # 回転(全ボーン)
        node += emit_curve(3, "rotationX", times, [math.degrees(e.x) for e in s["r"]])
        node += emit_curve(3, "rotationY", times, [math.degrees(e.y) for e in s["r"]])
        node += emit_curve(3, "rotationZ", times, [math.degrees(e.z) for e in s["r"]])
        # 移動(根のみ or 全部)
        if (not CONFIG["translate_root_only"]) or (pb.name in roots):
            node += emit_curve(3, "translationX", times, [t.x for t in s["t"]])
            node += emit_curve(3, "translationY", times, [t.y for t in s["t"]])
            node += emit_curve(3, "translationZ", times, [t.z for t in s["t"]])
        node += '\t\t</Node>\n'
        out += node
    out += '\t</Animation>\n'
    return out


# ------------------- メッシュ共通出力 -------------------
# SkinnedMesh / StaticMesh の両方が使う部分。座標変換・巻き順・材質属性・頂点数チェックは
# ここにだけ書き、メッシュ形式ごとの違いは「頂点に足す属性」だけにする。

PART_PATH_KEYS = ("texture", "normalmap")  # parts の値のうち、材質値ではなく画像パスの項目


def material_attrs(texture, normalmap, params):
    # 見た目1つ分の属性文字列（ゲーム側 MeshLoaderUtil::loadMaterial が読む）。
    attrs = ''
    if texture:
        attrs += ' texture="%s"' % texture
    if normalmap:
        attrs += ' normalmap="%s"' % normalmap
    # 材質値は書いたものだけ出す（無ければゲーム側 MeshMaterial の既定値）。
    for name, value in params.items():
        if isinstance(value, (tuple, list)):
            value = ", ".join(f(x) for x in value)
        elif isinstance(value, float):
            value = f(value)
        attrs += ' %s="%s"' % (name, value)
    return attrs


def mesh_element_attrs(textured):
    # メッシュ要素に付ける材質属性。
    # textured=False のパーツ（UVの無い単色部品）にはテクスチャを付けない。
    return material_attrs(CONFIG["mesh_texture"] if textured else "",
                          CONFIG["mesh_normalmap"] if textured else "",
                          CONFIG["material_params"])


def part_element_attrs(part):
    # <Part> に付ける材質属性。材質値は material_params を土台に、パーツに書いたもので上書きする。
    params = dict(CONFIG["material_params"])
    params.update({k: v for k, v in part.items() if k not in PART_PATH_KEYS})
    return material_attrs(part.get("texture", ""), part.get("normalmap", ""), params)


SPLIT_KEY_DIGITS = 5  # 頂点分割で「同じUV・法線」とみなす小数桁（浮動小数の誤差で分かれすぎないように丸める）


WHITE = (1.0, 1.0, 1.0)  # テクスチャを貼る部分の頂点カラー（テクスチャの色をそのまま出す）


def linear_to_srgb(c):
    # Blender の色はリニア値。このエンジンのシェーダはガンマ補正をしない（テクスチャも sRGB のまま使う）ので、
    # 頂点カラーも sRGB へ直して見た目の明るさを揃える。
    return 12.92 * c if c <= 0.0031308 else 1.055 * (c ** (1.0 / 2.4)) - 0.055


def material_color(mat):
    # マテリアルの色。material_colors に登録があればそれを優先し、無ければ Blender のベースカラー。
    if mat is None:
        return CONFIG["default_color"]
    if mat.name in CONFIG["material_colors"]:
        return CONFIG["material_colors"][mat.name]
    base = mat.diffuse_color
    if mat.use_nodes and mat.node_tree:
        for node in mat.node_tree.nodes:
            if node.type == "BSDF_PRINCIPLED":
                base = node.inputs["Base Color"].default_value
                break
    return tuple(linear_to_srgb(base[i]) for i in range(3))


def face_material(obj, poly):
    # 面に割り当てられたマテリアル（無ければ None）。
    slots = obj.material_slots
    return slots[poly.material_index].material if poly.material_index < len(slots) else None


def face_material_name(obj, poly):
    mat = face_material(obj, poly)
    return mat.name if mat else ""


def face_color(obj, poly):
    # 面ごとのマテリアル色（1オブジェクトに複数マテリアルがあっても正しく塗り分ける）。
    return material_color(face_material(obj, poly))


def shared_corners(obj, textured):
    # 1メッシュ頂点=1出力頂点（旧来の近似）。UV は最初に見つかったループのもの、色はオブジェクト単位。
    # 返り値: (出力頂点リスト[(頂点, 法線, UV, 色)], 三角形リスト[(i0, i1, i2)])
    mesh = obj.data
    col = WHITE if textured else obj_color(obj)
    uv_of_vertex = {}
    if mesh.uv_layers.active:
        uvdata = mesh.uv_layers.active.data
        for loop in mesh.loops:
            if loop.vertex_index not in uv_of_vertex:
                uv_of_vertex[loop.vertex_index] = uvdata[loop.index].uv
    corners = [(v, v.normal, uv_of_vertex.get(v.index, mathutils.Vector((0.0, 0.0))), col) for v in mesh.vertices]
    triangles = [tuple(lt.vertices) for lt in mesh.loop_triangles]
    return corners, triangles


def split_corners(obj, textured):
    # (頂点, UV, 法線, 色) の組ごとに出力頂点を分ける。UVの継ぎ目・角ばった面・マテリアル境界でも正しくなる。
    # 法線は corner_normals（Blender 4.1+。シャープ辺・スムーズ設定を反映した面の角ごとの法線）。
    mesh = obj.data
    uvdata = mesh.uv_layers.active.data if mesh.uv_layers.active else None
    corner_normals = mesh.corner_normals
    zero_uv = mathutils.Vector((0.0, 0.0))
    index_of_key = {}
    corners = []
    triangles = []
    for lt in mesh.loop_triangles:
        col = WHITE if textured else face_color(obj, mesh.polygons[lt.polygon_index])
        tri = []
        for loop_index in lt.loops:
            v = mesh.vertices[mesh.loops[loop_index].vertex_index]
            no = corner_normals[loop_index].vector
            uv = uvdata[loop_index].uv if uvdata else zero_uv
            key = (v.index,
                   round(uv[0], SPLIT_KEY_DIGITS), round(uv[1], SPLIT_KEY_DIGITS),
                   round(no.x, SPLIT_KEY_DIGITS), round(no.y, SPLIT_KEY_DIGITS), round(no.z, SPLIT_KEY_DIGITS),
                   tuple(round(c, SPLIT_KEY_DIGITS) for c in col))
            index = index_of_key.get(key)
            if index is None:
                index = len(corners)
                index_of_key[key] = index
                corners.append((v, no.copy(), uv.copy(), col))
            tri.append(index)
        triangles.append(tuple(tri))
    return corners, triangles


def emit_geometry(mesh_objs, lines, make_extra_attrs, textured, part_indices=None):
    # 全メッシュの <Vertex>/<Index> を lines へ追記する（メッシュは頂点番号をずらして連結）。
    # make_extra_attrs(obj) は「メッシュ頂点 v → 追加属性文字列」の関数を返す（スキンなら bones/weights）。
    # 頂点を分割しても元のメッシュ頂点を渡すので、重みなど頂点単位の情報はそのまま使える。
    # textured: テクスチャを貼るパーツか。True なら頂点カラーは白、False ならマテリアル色。
    # part_indices: dict を渡すと、インデックスを lines へ書かずに「マテリアル名 → [頂点番号...]」へ振り分ける
    #   （呼び出し側がマテリアルごとの <Part> にまとめて書く）。頂点は全パーツで共有する。
    bmin = [1e30, 1e30, 1e30]
    bmax = [-1e30, -1e30, -1e30]
    vertex_base = 0  # メッシュを連結するための頂点番号オフセット
    for obj in mesh_objs:
        mesh = obj.data
        mesh.calc_loop_triangles()
        matw = obj.matrix_world
        nmat = matw.to_3x3()
        extra_attrs = make_extra_attrs(obj)
        if textured and not mesh.uv_layers.active:
            # UV が無いとテクスチャの一点(0,0)の色で塗られてしまう。
            print("WARNING: メッシュ '%s' に UV がありませんが、テクスチャ付きで出力します。" % obj.name)

        corners, triangles = split_corners(obj, textured) if CONFIG["split_vertices"] else shared_corners(obj, textured)

        for v, normal, uv, col in corners:
            co = conv_point(matw @ v.co)                 # ワールド→エンジン
            no = conv_normal(nmat @ normal)
            for i in range(3):
                bmin[i] = min(bmin[i], co[i])
                bmax[i] = max(bmax[i], co[i])

            lines.append('\t\t<Vertex position="%s, %s, %s" normal="%s, %s, %s" uv="%s, %s" color="%s, %s, %s"%s/>' % (
                f(co.x), f(co.y), f(co.z), f(no.x), f(no.y), f(no.z), f(uv[0]), f(uv[1]),
                f(col[0]), f(col[1]), f(col[2]), extra_attrs(v)))

        # shared_corners / split_corners はどちらも loop_triangles の順で三角形を返すので、同じ順でマテリアルを引ける。
        tri_materials = [face_material_name(obj, mesh.polygons[lt.polygon_index]) for lt in mesh.loop_triangles]
        for tri, mat_name in zip(triangles, tri_materials):
            a = tri[0] + vertex_base
            b = tri[1] + vertex_base
            c = tri[2] + vertex_base
            if CONFIG["flip_winding"]:
                b, c = c, b  # 鏡像変換で裏返る分を戻す
            if part_indices is not None:
                part_indices.setdefault(mat_name, []).extend((a, b, c))
                continue
            for idx in (a, b, c):
                lines.append('\t\t<Index value="%d"/>' % idx)

        vertex_base += len(corners)

    # 超えたまま出力するとゲーム側で番号が一周してポリゴンが崩れるので、ここで止める。
    if vertex_base > MAX_VERTEX_NUMBER:
        raise RuntimeError("頂点数 %d が上限 %d を超えています。メッシュを分割してください。" % (vertex_base, MAX_VERTEX_NUMBER))
    print("VERTICES", vertex_base)
    print("BBOX_MIN", f(bmin[0]), f(bmin[1]), f(bmin[2]))
    print("BBOX_MAX", f(bmax[0]), f(bmax[1]), f(bmax[2]))


# ------------------- <SkinnedMesh> 出力 -------------------

def emit_skinned_mesh(mesh_objs, armature):
    bone_names = set(b.name for b in armature.data.bones)

    # 全メッシュ横断で使うボーンの並び（出現順にグローバル index を確定）。
    bone_order = []
    bone_index = {}
    for obj in mesh_objs:
        for vg in obj.vertex_groups:
            if vg.name in bone_names and vg.name not in bone_index:
                bone_index[vg.name] = len(bone_order)
                bone_order.append(vg.name)

    def make_weight_attrs(obj):
        # このメッシュの vertex_group index -> グローバル bone index。
        local_bone = {}
        for vg in obj.vertex_groups:
            if vg.name in bone_index:
                local_bone[vg.index] = bone_index[vg.name]

        def weight_attrs(v):
            # 重み上位 MAX_INFLUENCE 本を取り、合計1へ正規化。
            infl = []
            for g in v.groups:
                if g.group in local_bone and g.weight > 0.0:
                    infl.append((local_bone[g.group], g.weight))
            infl.sort(key=lambda x: x[1], reverse=True)
            infl = infl[:MAX_INFLUENCE]
            total = sum(w for _, w in infl)
            if total <= 0.0:
                infl = [(0, 1.0)]
                total = 1.0
            bones = ", ".join(str(bi) for bi, _ in infl)
            weights = ", ".join(f(w / total) for _, w in infl)
            return ' bones="%s" weights="%s"' % (bones, weights)
        return weight_attrs

    parts = CONFIG["parts"]
    lines = []
    if not parts:
        # 1組の見た目で全体を描く（従来の形式。<Index> は <SkinnedMesh> の直下）。
        textured = bool(CONFIG["mesh_texture"])
        lines.append('\t<SkinnedMesh%s>' % mesh_element_attrs(textured))
        for name in bone_order:
            lines.append('\t\t<Bone name="%s"/>' % name)
        emit_geometry(mesh_objs, lines, make_weight_attrs, textured)
        lines.append('\t</SkinnedMesh>')
        return "\n".join(lines) + "\n"

    # マテリアルごとの <Part>（見た目＋インデックス）。頂点とボーンは全パーツ共通なので <SkinnedMesh> 直下に置く。
    lines.append('\t<SkinnedMesh>')
    for name in bone_order:
        lines.append('\t\t<Bone name="%s"/>' % name)
    part_indices = {}
    emit_geometry(mesh_objs, lines, make_weight_attrs, True, part_indices)
    # parts に書き忘れたマテリアルの面は描かれなくなるので、黙って捨てずに止める。
    missing = [m for m in part_indices if m not in parts]
    if missing:
        raise RuntimeError("parts に無いマテリアルがあります: %s（parts: %s）" % (missing, list(parts.keys())))
    for mat_name, part in parts.items():
        indices = part_indices.get(mat_name, [])
        if not indices:
            print("WARNING: parts のマテリアル '%s' を使う面がありません。" % mat_name)
            continue
        lines.append('\t\t<Part%s>' % part_element_attrs(part))
        for idx in indices:
            lines.append('\t\t\t<Index value="%d"/>' % idx)
        lines.append('\t\t</Part>')
        print("PART", mat_name, "triangles", len(indices) // 3)
    lines.append('\t</SkinnedMesh>')
    return "\n".join(lines) + "\n"


# ------------------- <StaticMesh> 出力 -------------------

def emit_static_part(mesh_objs, textured):
    # パーツ1つ = <StaticMesh> 要素1つ = ゲーム側で1回の描画（1つの見た目）。
    lines = []
    lines.append('\t<StaticMesh%s>' % mesh_element_attrs(textured))
    emit_geometry(mesh_objs, lines, lambda obj: (lambda v: ''), textured)  # 頂点に足す属性は無い
    lines.append('\t</StaticMesh>')
    print("PART", "textured" if textured else "plain", [o.name for o in mesh_objs])
    return "\n".join(lines) + "\n"


def emit_static_mesh(mesh_objs):
    # テクスチャを貼るメッシュ（UVあり）と、マテリアル色だけで塗るメッシュ（UVなし）を別パーツに分ける。
    # 焼き込み済みモデルは「壁はテクスチャ、窓枠やガラスは単色」のように両方が混ざっていることが多い。
    if CONFIG["mesh_texture"]:
        textured = [o for o in mesh_objs if o.data.uv_layers.active]
    else:
        textured = []
    plain = [o for o in mesh_objs if o not in textured]
    out = ""
    for group in split_by_vertex_limit(textured, True):
        out += emit_static_part(group, True)
    for group in split_by_vertex_limit(plain, False):
        out += emit_static_part(group, False)
    return out


def split_by_vertex_limit(mesh_objs, textured):
    # 1パーツの頂点数が MAX_VERTEX_NUMBER（ゲーム側の16bitインデックスの上限）以内になるよう、メッシュを順にまとめる。
    # ゲーム側は <StaticMesh> を何個でも読んで全部描くので、上限を超えるモデルはパーツを分ければ出せる。
    # 上限以内のモデルは今までどおり1パーツになる。1つのメッシュだけで上限を超える場合は分けられないので止める。
    groups = []
    group = []
    group_vertices = 0
    for obj in mesh_objs:
        obj.data.calc_loop_triangles()
        corners, _ = split_corners(obj, textured) if CONFIG["split_vertices"] else shared_corners(obj, textured)
        count = len(corners)
        if count > MAX_VERTEX_NUMBER:
            raise RuntimeError("メッシュ '%s' だけで頂点数 %d が上限 %d を超えています。メッシュを分割してください。" % (
                obj.name, count, MAX_VERTEX_NUMBER))
        if group and group_vertices + count > MAX_VERTEX_NUMBER:
            groups.append(group)
            group = []
            group_vertices = 0
        group.append(obj)
        group_vertices += count
    if group:
        groups.append(group)
    return groups


# ------------------------- main -------------------------

def config_path_from_args():
    # blender ... --python fbx_to_gamelib.py -- <設定ファイル>  の <設定ファイル> を返す。
    if "--" in sys.argv:
        args = sys.argv[sys.argv.index("--") + 1:]
        if args:
            return args[0]
    return CONFIG_FILE


def load_config(path):
    # 設定ファイルの項目で CONFIG を上書きし、相対パスは設定ファイルの場所を基準に解決する。
    with open(path, encoding="utf-8") as fp:
        CONFIG.update(json.load(fp))
    base = os.path.dirname(os.path.abspath(path))

    def resolve(p):
        return os.path.normpath(os.path.join(base, p)) if p and not os.path.isabs(p) else p

    for key in ("fbx_path", "out_path", "resource_root"):
        CONFIG[key] = resolve(CONFIG[key])
    for extra in CONFIG["extra_animations"]:
        extra["fbx_path"] = resolve(extra["fbx_path"])
    CONFIG["texture_sources"] = {key: resolve(p) for key, p in CONFIG["texture_sources"].items()}


def import_fbx():
    # 使い方A（画面なし実行）のときだけ、空のシーンに FBX を読み込む。
    # 画面で実行したときは、開いているシーンを壊さないよう読み込まない。
    if not (bpy.app.background and CONFIG["fbx_path"]):
        return
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=CONFIG["fbx_path"])


def emit_extra_animations(armature):
    # アニメだけの FBX を読み、そのアクションを本体の骨格に当てて <Animation> を出す。
    # 骨名が同じなら、アクション（ボーンごとのカーブ）は本体の骨格にそのまま使える。
    # 読み込んだ骨格は出力に混ざらないよう、書き出し後に消す。
    out = ""
    for extra in CONFIG["extra_animations"]:
        before = set(bpy.data.objects)
        bpy.ops.import_scene.fbx(filepath=extra["fbx_path"])
        imported = [o for o in bpy.data.objects if o not in before]
        source = next((o for o in imported if o.type == "ARMATURE"), None)
        if source is None or source.animation_data is None or source.animation_data.action is None:
            raise RuntimeError("アニメが見つかりません: %s" % extra["fbx_path"])
        missing = [b.name for b in armature.data.bones if b.name not in source.data.bones]
        if missing:
            raise RuntimeError("骨格が違います（本体にある骨が無い）: %s %s" % (extra["fbx_path"], missing[:5]))
        in_place = extra.get("in_place", False)
        out += emit_animation(armature, source.animation_data.action, extra["name"], in_place)
        print("ANIM", extra["name"], "from", extra["fbx_path"], "(in place)" if in_place else "")
        bpy.data.batch_remove(imported)
    return out


def export_part_textures():
    # parts の texture / normalmap を、FBX 内の画像から縮小して TGA で書き出す。
    # マテリアルのノードから「Base Color に繋がった画像」「Normal Map ノードに繋がった画像」を探す。
    size = CONFIG["texture_export_size"]
    if size <= 0:
        return
    for mat_name, part in CONFIG["parts"].items():
        mat = bpy.data.materials.get(mat_name)
        if mat is None or not mat.node_tree:
            raise RuntimeError("マテリアルが見つかりません: %s" % mat_name)
        images = {"texture": linked_image(mat, "Base Color", via_normal_map=False),
                  "normalmap": linked_image(mat, "Normal", via_normal_map=True)}
        for key, image in images.items():
            if not part.get(key):
                continue
            if image is None:
                raise RuntimeError("マテリアル '%s' に %s の画像がありません。" % (mat_name, key))
            save_scaled_tga(image, size, os.path.join(CONFIG["resource_root"], part[key]))


def export_mesh_textures(mesh_objs):
    # static モード用。mesh_texture / mesh_normalmap を、テクスチャを貼るメッシュの最初のマテリアルの画像から書き出す。
    # static は1組の見た目で描く（parts を使わない）ので、画像も1組だけ。
    size = CONFIG["texture_export_size"]
    if size <= 0:
        return
    for key, bsdf_input, via_normal_map in (("mesh_texture", "Base Color", False), ("mesh_normalmap", "Normal", True)):
        if not CONFIG[key]:
            continue
        source = CONFIG["texture_sources"].get(key)
        image = bpy.data.images.load(source) if source else material_image(mesh_objs, key, bsdf_input, via_normal_map)
        save_scaled_tga(image, size, os.path.join(CONFIG["resource_root"], CONFIG[key]))


def material_image(mesh_objs, key, bsdf_input, via_normal_map):
    # テクスチャを貼るメッシュの最初のマテリアルから、bsdf_input に繋がった画像を返す。無ければ止める（key はエラー文用）。
    mat = next((o.material_slots[0].material for o in mesh_objs
                if o.data.uv_layers.active and o.material_slots and o.material_slots[0].material), None)
    if mat is None or not mat.node_tree:
        raise RuntimeError("テクスチャを書き出すマテリアルが見つかりません。")
    image = linked_image(mat, bsdf_input, via_normal_map)
    if image is None:
        raise RuntimeError("マテリアル '%s' に %s の画像がありません。" % (mat.name, key))
    return image


def linked_image(mat, bsdf_input, via_normal_map):
    # Principled BSDF の入力 bsdf_input に（Normal Map ノード経由で）繋がった画像を返す。無ければ None。
    bsdf = next((n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
    if bsdf is None or not bsdf.inputs[bsdf_input].is_linked:
        return None
    node = bsdf.inputs[bsdf_input].links[0].from_node
    if via_normal_map:
        if node.type != "NORMAL_MAP" or not node.inputs["Color"].is_linked:
            return None
        node = node.inputs["Color"].links[0].from_node
    return node.image if node.type == "TEX_IMAGE" else None


def save_scaled_tga(image, size, path):
    # 元画像を壊さないよう複製を縮小し、画素をそのまま（色変換なしで）新しい画像へ移して保存する。
    # save_render は表示用の色管理(AgX 等)が掛かり色が変わるので使わない。
    scaled = image.copy()
    scaled.scale(size, size)
    # Python のリストだと 4096px で 2GB を超えるので、numpy の配列で受け渡す（値は同じ）。
    pixels = numpy.empty(size * size * 4, dtype=numpy.float32)
    scaled.pixels.foreach_get(pixels)
    out = bpy.data.images.new(os.path.basename(path), size, size, alpha=False)
    out.colorspace_settings.name = image.colorspace_settings.name
    out.pixels.foreach_set(pixels)
    out.filepath_raw = path
    out.file_format = "TARGA_RAW"  # ゲーム側のローダは RLE 圧縮も読めるが、無圧縮の方が読み込みが速い
    out.save()
    bpy.data.images.remove(scaled)
    bpy.data.images.remove(out)
    print("TEXTURE", image.name, "->", path)


def build_skinned(mesh_objs):
    armature = find_armature()
    if armature is None:
        raise RuntimeError("Armature が見つかりません。動かないモデルなら mode を \"static\" にしてください。")
    out = emit_tree(armature)
    # pose.bones を実際に動かすアクションだけ出力する（Mayaのコントロール系静止アクションを除外）。
    for action in (list(bpy.data.actions) if CONFIG["fbx_animations"] else []):
        if not any(fc.data_path.startswith("pose.bones") for fc in action.fcurves):
            continue
        anim_name = CONFIG["anim_name"] if CONFIG["anim_name"] else sanitize(action.name)
        out += emit_animation(armature, action, anim_name)
        print("ANIM", anim_name)
    out += emit_extra_animations(armature)
    out += emit_skinned_mesh(mesh_objs, armature)
    export_part_textures()
    return out


def main():
    config_path = config_path_from_args()
    if config_path:
        load_config(config_path)
    if not CONFIG["out_path"]:
        raise RuntimeError("out_path が未設定です。設定ファイルを指定してください。")
    import_fbx()

    mesh_objs = find_meshes()
    if not mesh_objs:
        raise RuntimeError("Mesh が見つかりません。")

    out = "<Container>\n"
    if CONFIG["mode"] == "static":
        adjust_static_transform(mesh_objs)
        out += emit_static_mesh(mesh_objs)
        export_mesh_textures(mesh_objs)
    else:
        out += build_skinned(mesh_objs)
    out += "</Container>\n"

    with open(CONFIG["out_path"], "w", encoding="utf-8") as fp:
        fp.write(out)
    print("WROTE", CONFIG["out_path"])


if __name__ == "__main__":
    main()
