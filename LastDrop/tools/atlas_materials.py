# -*- coding: utf-8 -*-
# =====================================================================
# atlas_materials.py
#   マテリアルごとに別のテクスチャを使う FBX を、テクスチャ1組（albedo / normal）で描ける FBX に作り直す。
#   1. マテリアルごとの画像を、正方形の格子（アトラス）の1区画ずつへ縮小して並べ、TGA で書き出す。
#   2. 各面の UV を、そのマテリアルの区画へ縮めて移す。
#   3. マテリアルを1つにまとめた FBX を書き出す。
#   書き出した FBX は fbx_to_gamelib.py の static モード（テクスチャ1組）でそのまま変換できる。
#
#   orm_path を書くと、材質の画像（r：AO・g：粗さ・b：金属。SummaryPlan/個別仕様/SsrDecalMaterial_Plan.txt 5 章）も
#   同じ区画の並びで書き出す。そのときの AO は ORM の r へ入れる（albedo に焼かない。SSAO と二重に暗くならないように）。
#   orm_path が無ければ今までどおり、AO（凹みの陰）がある区画は albedo に掛け合わせて焼き込む。Height は扱わない。
#
# モデルごとの設定は tools/configs/*_atlas.json に書く（スクリプト本体はモデルごとに書き換えない）。
#
# 使い方:
#   blender -b --factory-startup --python tools/atlas_materials.py -- tools/configs/uaz_atlas.json
#   その後、出力した FBX を fbx_to_gamelib.py で変換する（mesh_texture / mesh_normalmap に書き出した TGA を指定し、
#   texture_export_size は 0 のままにする）。
#   画像だけ作り直すとき（区画の並びは同じなので、FBX とメッシュの txt は今のまま使える）:
#   blender -b --factory-startup --python tools/atlas_materials.py -- tools/configs/uaz_atlas.json --textures-only
#
# 前提: 各マテリアルの UV が 0〜1 に収まっていること（繰り返し貼るテクスチャは区画からはみ出すので使えない）。
# =====================================================================

import bpy
import json
import os
import sys

import numpy as np

# ------------------------- CONFIG -------------------------
# 既定値。設定ファイルに書いた項目だけが上書きされる。特定モデル向けの値はここに書かない。
# パスは設定ファイルからの相対パスで書ける。
CONFIG = {
    "fbx_path": "",        # 読み込む FBX
    "out_fbx": "",         # 書き出す FBX（マテリアル1つ・UV をアトラスへ移したもの）
    "texture_dir": "",     # cells の画像ファイル名の基準フォルダ
    "albedo_path": "",     # 書き出す albedo の TGA
    "normal_path": "",     # 書き出す normal の TGA
    "orm_path": "",        # 書き出す材質の画像（r AO・g 粗さ・b 金属）の TGA。空なら書き出さない
    "atlas_size": 2048,    # アトラス全体の一辺の画素数
    "grid": 2,             # 1辺に並べる区画の数（grid x grid 区画）
    # 区画の内側に空ける画素数。境目で縮小版（ミップマップ）が隣の区画の色を拾ってにじむのを防ぐ。
    # 空けた所は区画の端の色を引き伸ばして埋める。
    "padding": 4,
    # マテリアル名 → 区画と画像。
    #   cell: [列, 行]（左上が [0, 0]）
    #   albedo / normal: 画像ファイル名（必須）
    #   ao: 画像ファイル名（任意。orm_path があれば ORM の r へ、無ければ albedo に掛け合わせる）
    #   roughness / metallic: 画像ファイル名（任意。orm_path があるときだけ使う。無い区画は粗さ 1・金属 0）
    #   flip_normal_green: true なら normal の緑（Y）を反転する（DirectX 式の法線マップを OpenGL 式へ揃える）
    "cells": {},
}
# ----------------------------------------------------------


def config_path_from_args():
    # blender ... --python atlas_materials.py -- <設定ファイル>  の <設定ファイル> を返す。
    if "--" in sys.argv:
        args = sys.argv[sys.argv.index("--") + 1:]
        if args:
            return args[0]
    return ""


def load_config(path):
    # 設定ファイルの項目で CONFIG を上書きし、相対パスは設定ファイルの場所を基準に解決する。
    with open(path, encoding="utf-8") as fp:
        CONFIG.update(json.load(fp))
    base = os.path.dirname(os.path.abspath(path))

    def resolve(p):
        return os.path.normpath(os.path.join(base, p)) if p and not os.path.isabs(p) else p

    for key in ("fbx_path", "out_fbx", "texture_dir", "albedo_path", "normal_path", "orm_path"):
        CONFIG[key] = resolve(CONFIG[key])


def cell_inner_size():
    # 1区画のうち、画像を置く部分（padding を除いた部分）の一辺の画素数。
    cell = CONFIG["atlas_size"] // CONFIG["grid"]
    inner = cell - 2 * CONFIG["padding"]
    if inner <= 0:
        raise RuntimeError("padding が区画より大きいです（区画 %d px, padding %d px）。" % (cell, CONFIG["padding"]))
    return cell, inner


def cell_origin(cell_pos):
    # 区画の左下の画素位置（x, y）。Blender の画像も UV も原点は左下なので、行（上から数える）を反転する。
    cell, _ = cell_inner_size()
    col, row = cell_pos
    grid = CONFIG["grid"]
    if not (0 <= col < grid and 0 <= row < grid):
        raise RuntimeError("cell %s が格子 %d x %d の外です。" % (cell_pos, grid, grid))
    return col * cell, (grid - 1 - row) * cell


def load_scaled_pixels(file_name, size):
    # 画像を読み、size x size に縮小した画素（高さ, 幅, RGBA の float 配列）を返す。元画像は残さない。
    path = os.path.join(CONFIG["texture_dir"], file_name)
    if not os.path.exists(path):
        raise RuntimeError("画像が見つかりません: %s" % path)
    image = bpy.data.images.load(path)
    # 画素をそのまま（色変換なしで）扱う。sRGB のまま読んで sRGB のまま書くので、色は変わらない。
    image.colorspace_settings.name = "Non-Color"
    image.scale(size, size)
    pixels = np.empty(size * size * 4, dtype=np.float32)
    image.pixels.foreach_get(pixels)
    bpy.data.images.remove(image)
    return pixels.reshape(size, size, 4)


def build_cell_orm(cell, inner):
    # 1 区画ぶんの材質の画像（r AO・g 粗さ・b 金属。どれも灰色画像なので R だけ使う）。無い画像は AO 1・粗さ 1・金属 0。
    orm = np.ones((inner, inner, 4), dtype=np.float32)
    orm[:, :, 2] = 0.0
    for key, channel in (("ao", 0), ("roughness", 1), ("metallic", 2)):
        if cell.get(key):
            orm[:, :, channel] = load_scaled_pixels(cell[key], inner)[:, :, 0]
    return orm


def build_atlas_images():
    # マテリアルごとの画像を区画へ並べた albedo / normal / orm の画素を返す（orm_path が無ければ orm は None）。
    size = CONFIG["atlas_size"]
    _, inner = cell_inner_size()
    pad = CONFIG["padding"]
    writes_orm = bool(CONFIG["orm_path"])
    albedo = np.ones((size, size, 4), dtype=np.float32)
    normal = np.ones((size, size, 4), dtype=np.float32)
    normal[:, :, 0:3] = (0.5, 0.5, 1.0)  # どの区画にも使われない所は「平ら」な法線にしておく
    orm = None
    if writes_orm:
        orm = np.ones((size, size, 4), dtype=np.float32)
        orm[:, :, 2] = 0.0  # 使われない所は AO 1・粗さ 1・金属 0

    for mat_name, cell in CONFIG["cells"].items():
        x, y = cell_origin(cell["cell"])
        color = load_scaled_pixels(cell["albedo"], inner)
        if cell.get("ao") and not writes_orm:
            ao = load_scaled_pixels(cell["ao"], inner)
            color[:, :, 0:3] *= ao[:, :, 0:1]  # AO は灰色画像なので R だけ使う
        bump = load_scaled_pixels(cell["normal"], inner)
        if cell.get("flip_normal_green"):
            bump[:, :, 1] = 1.0 - bump[:, :, 1]
        # padding の所は区画の端の色を引き伸ばして埋める（にじんでも同じ色なので目立たない）。
        edge = ((pad, pad), (pad, pad), (0, 0))
        albedo[y:y + inner + 2 * pad, x:x + inner + 2 * pad] = np.pad(color, edge, mode="edge")
        normal[y:y + inner + 2 * pad, x:x + inner + 2 * pad] = np.pad(bump, edge, mode="edge")
        if writes_orm:
            orm[y:y + inner + 2 * pad, x:x + inner + 2 * pad] = np.pad(build_cell_orm(cell, inner), edge, mode="edge")
        print("CELL", mat_name, "->", cell["cell"])
    return albedo, normal, orm


def save_tga(pixels, path):
    # 画素をそのまま（色変換なしで）TGA に書き出す。
    size = pixels.shape[0]
    image = bpy.data.images.new(os.path.basename(path), size, size, alpha=False)
    image.colorspace_settings.name = "Non-Color"
    image.pixels.foreach_set(pixels.ravel())
    image.filepath_raw = path
    image.file_format = "TARGA_RAW"  # ゲーム側のローダは RLE 圧縮も読めるが、無圧縮の方が読み込みが速い
    image.save()
    bpy.data.images.remove(image)
    print("TEXTURE", path)


def remap_uvs(mesh_objs):
    # 各面の UV を、その面のマテリアルの区画へ縮めて移す。
    # 同じメッシュデータを複数のオブジェクトが共有していることがある（左右のタイヤなど）ので、
    # メッシュデータごとに1回だけ移す（2回移すと区画の外へずれる）。
    size = float(CONFIG["atlas_size"])
    _, inner = cell_inner_size()
    pad = CONFIG["padding"]
    done = set()
    for obj in mesh_objs:
        mesh = obj.data
        if mesh in done:
            continue
        done.add(mesh)
        uv_layer = mesh.uv_layers.active
        if uv_layer is None:
            raise RuntimeError("UV がありません: %s" % obj.name)
        for poly in mesh.polygons:
            mat = obj.material_slots[poly.material_index].material if obj.material_slots else None
            if mat is None or mat.name not in CONFIG["cells"]:
                raise RuntimeError("cells に無いマテリアルの面があります: %s（%s）" % (mat.name if mat else None, obj.name))
            x, y = cell_origin(CONFIG["cells"][mat.name]["cell"])
            for loop_index in poly.loop_indices:
                u, v = uv_layer.data[loop_index].uv
                uv_layer.data[loop_index].uv = ((x + pad + u * inner) / size, (y + pad + v * inner) / size)


def merge_materials(mesh_objs):
    # 全メッシュのマテリアルを1つにまとめる（区画の割り当ては UV に移したので、もう要らない）。
    atlas = bpy.data.materials.new("atlas")
    done = set()
    for obj in mesh_objs:
        mesh = obj.data
        if mesh in done:
            continue
        done.add(mesh)
        mesh.materials.clear()
        mesh.materials.append(atlas)
        for poly in mesh.polygons:
            poly.material_index = 0


def main():
    config_path = config_path_from_args()
    if not config_path:
        raise RuntimeError("設定ファイルを指定してください（-- の後ろ）。")
    load_config(config_path)
    if not CONFIG["cells"]:
        raise RuntimeError("cells が空です。")

    bpy.ops.wm.read_factory_settings(use_empty=True)
    albedo, normal, orm = build_atlas_images()
    save_tga(albedo, CONFIG["albedo_path"])
    save_tga(normal, CONFIG["normal_path"])
    if orm is not None:
        save_tga(orm, CONFIG["orm_path"])
    if "--textures-only" in sys.argv:
        return  # 区画の並びは設定で決まるので、前に書き出した FBX の UV はそのまま合う

    bpy.ops.import_scene.fbx(filepath=CONFIG["fbx_path"])
    mesh_objs = [o for o in bpy.data.objects if o.type == "MESH"]
    if not mesh_objs:
        raise RuntimeError("Mesh が見つかりません。")

    remap_uvs(mesh_objs)
    merge_materials(mesh_objs)
    bpy.ops.export_scene.fbx(filepath=CONFIG["out_fbx"], object_types={"MESH"}, path_mode="STRIP")
    print("WROTE", CONFIG["out_fbx"])


if __name__ == "__main__":
    main()
