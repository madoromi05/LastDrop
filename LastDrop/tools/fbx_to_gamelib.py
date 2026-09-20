# -*- coding: utf-8 -*-
# =====================================================================
# fbx_to_gamelib.py
#   FBX(リグ+スキン+アニメ)を GameLib の Scene 形式(ニセxml)へ変換する。
#   出力は <Container> に <Tree>(骨格) / <Animation>(アニメ) / <SkinnedMesh>(頂点+重み)。
#   ゲーム側は Container が Tree/Animation を、SkinnedMeshLoader が SkinnedMesh を読む。
#
# 使い方（Blender 上で実行）:
#   1. Blender で対象 FBX を開く（または File > Import > FBX）。
#   2. Scripting タブでこのファイルを開き、下の CONFIG を設定。
#   3. Run Script。CONFIG["out_path"] に .txt が書き出される。
#
# ★較正が必要な2箇所（初回のみ）:
#   (A) AXIS_CONVERT … Blender(Z-up 右手) → エンジン(Y-up 左手) の座標変換。
#   (B) ROT_ORDER    … アニメ回転を分解するオイラー順。エンジンは rotateY→X→Z。
#   検証手順: まず1関節だけ Z 回転するモデルで出力し、skintest の曲げ(rotationZ)と
#   符号・向きが一致するか確認 → 合わなければ (A)(B) を調整、の順で詰める。
# =====================================================================

import bpy
import mathutils
import math

# ------------------------- CONFIG -------------------------
CONFIG = {
    "out_path": "C:/Users/mador/source/repos/LastDrop/LastDrop/Resource/character.txt",
    "tree_name": "skel",       # <Tree name>。ゲーム側 createTree(この名前)
    "mesh_texture": "",         # <SkinnedMesh texture>。空なら無地
    "frame_step": 1,            # アニメを何フレームおきにサンプリングするか
    "translate_root_only": True,  # True: 根ボーンだけ移動カーブを出す(関節は回転のみ)
    "flip_winding": True,       # 左右手変換で面が裏返る場合に三角形の巻き順を反転
    "anim_name": "",            # 空でない場合、出力アニメ名をこれに固定する(Take 001対策)
}

# (A) 座標変換: Blender(x,y,z: Z-up 右手) → エンジン(Y-up 左手)。
#     既定は「YとZを入れ替える」= Z-up→Y-up かつ 右手→左手(行列式-1)。
#     もし上下や前後が反転していたらここの符号を調整する。
AXIS_CONVERT = mathutils.Matrix((
    (1.0, 0.0, 0.0, 0.0),
    (0.0, 0.0, 1.0, 0.0),
    (0.0, 1.0, 0.0, 0.0),
    (0.0, 0.0, 0.0, 1.0),
))

# (B) アニメ回転を分解するオイラー順。エンジンは rotateY→rotateX→rotateZ 適用。
ROT_ORDER = "YXZ"

MAX_INFLUENCE = 4  # SkinnedMesh::MAX_INFLUENCE と一致させること
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


def find_meshes():
    # 全メッシュを返す（このモデルは7分割なので複数対応が必須）。
    return [o for o in bpy.data.objects if o.type == "MESH"]


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


def emit_animation(armature, action):
    scene = bpy.context.scene
    if armature.animation_data is None:
        armature.animation_data_create()
    armature.animation_data.action = action
    start = int(action.frame_range[0])
    end = int(action.frame_range[1])

    # レスト親相対（エンジン空間）を先に取っておく。
    rest = {}
    for b in armature.data.bones:
        rest[b.name] = to_engine(rest_local(b, armature))

    # 各ボーンのカーブ用サンプルを貯める。
    times = []
    samples = {b.name: {"t": [], "r": []} for b in armature.data.bones}

    frame = start
    while frame <= end:
        scene.frame_set(frame)
        times.append(frame - start)  # 0 起点
        for pb in armature.pose.bones:
            animated = to_engine(pose_local(pb, armature))
            # エンジンは world = parent * rest * delta。delta = rest^-1 * animated。
            delta = rest[pb.name].inverted() @ animated
            t, q, s = delta.decompose()
            e = q.to_euler(ROT_ORDER)
            samples[pb.name]["t"].append(t)
            samples[pb.name]["r"].append(e)
        frame += CONFIG["frame_step"]

    roots = set(b.name for b in armature.data.bones if b.parent is None)
    anim_name = CONFIG["anim_name"] if CONFIG["anim_name"] else sanitize(action.name)
    out = '\t<Animation name="%s">\n' % anim_name
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

    lines = []
    lines.append('\t<SkinnedMesh%s>' % (' texture="%s"' % CONFIG["mesh_texture"] if CONFIG["mesh_texture"] else ''))
    for name in bone_order:
        lines.append('\t\t<Bone name="%s"/>' % name)

    bmin = [1e30, 1e30, 1e30]
    bmax = [-1e30, -1e30, -1e30]
    vertex_base = 0  # メッシュを連結するための頂点番号オフセット
    for obj in mesh_objs:
        mesh = obj.data
        mesh.calc_loop_triangles()
        matw = obj.matrix_world
        nmat = matw.to_3x3()

        # このメッシュの vertex_group index -> グローバル bone index。
        local_bone = {}
        for vg in obj.vertex_groups:
            if vg.name in bone_index:
                local_bone[vg.index] = bone_index[vg.name]

        # 頂点ごとの UV（ループから拾う。共有頂点なので近似）。
        uv_of_vertex = {}
        if mesh.uv_layers.active:
            uvdata = mesh.uv_layers.active.data
            for loop in mesh.loops:
                if loop.vertex_index not in uv_of_vertex:
                    uv_of_vertex[loop.vertex_index] = uvdata[loop.index].uv

        for v in mesh.vertices:
            co = conv_point(matw @ v.co)                 # ワールド→エンジン
            no = conv_normal(nmat @ v.normal)
            uv = uv_of_vertex.get(v.index, mathutils.Vector((0.0, 0.0)))
            for i in range(3):
                bmin[i] = min(bmin[i], co[i])
                bmax[i] = max(bmax[i], co[i])

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

            lines.append('\t\t<Vertex position="%s, %s, %s" normal="%s, %s, %s" uv="%s, %s" bones="%s" weights="%s"/>' % (
                f(co.x), f(co.y), f(co.z), f(no.x), f(no.y), f(no.z), f(uv[0]), f(uv[1]), bones, weights))

        for lt in mesh.loop_triangles:
            a = lt.vertices[0] + vertex_base
            b = lt.vertices[1] + vertex_base
            c = lt.vertices[2] + vertex_base
            if CONFIG["flip_winding"]:
                b, c = c, b  # 左右手変換で裏返る分を戻す
            for idx in (a, b, c):
                lines.append('\t\t<Index value="%d"/>' % idx)

        vertex_base += len(mesh.vertices)

    lines.append('\t</SkinnedMesh>')
    print("BBOX_MIN", f(bmin[0]), f(bmin[1]), f(bmin[2]))
    print("BBOX_MAX", f(bmax[0]), f(bmax[1]), f(bmax[2]))
    return "\n".join(lines) + "\n"


# ------------------------- main -------------------------

def main():
    armature = find_armature()
    if armature is None:
        raise RuntimeError("Armature が見つかりません。")
    mesh_objs = find_meshes()
    if not mesh_objs:
        raise RuntimeError("Mesh が見つかりません。")

    out = "<Container>\n"
    out += emit_tree(armature)
    # pose.bones を実際に動かすアクションだけ出力する（Mayaのコントロール系静止アクションを除外）。
    for action in bpy.data.actions:
        if not any(fc.data_path.startswith("pose.bones") for fc in action.fcurves):
            continue
        out += emit_animation(armature, action)
        print("ANIM", CONFIG["anim_name"] if CONFIG["anim_name"] else sanitize(action.name))
    out += emit_skinned_mesh(mesh_objs, armature)
    out += "</Container>\n"

    with open(CONFIG["out_path"], "w", encoding="utf-8") as fp:
        fp.write(out)
    print("WROTE", CONFIG["out_path"])


if __name__ == "__main__":
    main()
