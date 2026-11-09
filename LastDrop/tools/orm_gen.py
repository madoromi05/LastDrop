# -*- coding: utf-8 -*-
# =====================================================================
# orm_gen.py
#   材質の画像（ORM：r AO・g 粗さ・b 金属）を作る。仕様は SummaryPlan/個別仕様/グラフィック/MaterialTexture_Plan.txt 4・5 章（グラフィック段階11）。
#   メッシュは作り直さない（形・UV はそのまま。ORM は色の画像と同じ UV で読む）。作った後は、メッシュの txt に ormmap を足し、
#   roughness・metallic を 1 にする（ゲームは「txt の数値 × ORM」で読む。SsrDecalMaterial_Plan.txt 5-1）。
#   元の画像の種類で、作り方が 3 つある（設定にどれを書くかで決まる）。
#     1. 元の画像から     … roughness（光沢なら invert）・metallic・ao の画像を 1 枚にまとめる（ゾンビ・弾）
#     2. FBX の埋め込みから … FBX を読み、名前で探した画像を 1. と同じに使う（自キャラ。Mixamo は画像を FBX に埋め込む）。
#                            Blender の FBX の読み込みは Glossiness を Roughness へそのままつなぐ（逆さま）ので、材質のノードは見ない
#     3. 色の画像から     … 色相・彩度・明るさの表（rules）で粗さ・金属を決める（建物・銃。元に粗さ・金属の画像が無い）。
#                            色は sRGB の数のまま HSV にする（目で見た色と表の値を合わせやすい）。明るさの細かい揺れを粗さに少し混ぜる
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\blender.exe" --background --factory-startup --python tools/orm_gen.py -- tools/configs/orm/<名前>.json
#   その後 tools/texture_to_dds.ps1 <名前>_orm で DDS（Kind = Data。BC7_UNORM）にする。
#
# 設定（json。パスは作業フォルダ基準か絶対パス）:
#   "output": 書き出す TGA。 "size": 一辺の画素数。
#   1. "roughness" / "metallic" / "ao": 画像のパス（どれも灰色。r だけ使う。無い物は 粗さ 1・金属 0・AO 1）。
#      "roughness_invert": true なら 1 − 値（光沢・なめらかさの画像）。
#   2. "fbx": FBX のパス。"roughness" などに、パスの代わりに FBX の中の画像の名前（拡張子なし）を書く。
#   3. "albedo": 色の画像。"rules": [ { "name", "hue": [下, 上]（度。下 > 上なら 0° をまたぐ）, "saturation": [下, 上], "value": [下, 上],
#      "roughness", "metallic" }, … ]。上から順に見て、最初に合った物を使う。どれにも合わなければ "default"（{"roughness", "metallic"}）。
#      "detail": 明るさの細かい揺れを粗さに混ぜる量（±この値まで。暗いくぼみほど粗く）。
# =====================================================================

import json
import os
import sys

import bpy
import numpy

CHANNEL_AO = 0
CHANNEL_ROUGHNESS = 1
CHANNEL_METALLIC = 2
CHANNEL_NUMBER = 4

MISSING_VALUE = {"ao": 1.0, "roughness": 1.0, "metallic": 0.0}
SOURCE_CHANNEL = {"ao": CHANNEL_AO, "roughness": CHANNEL_ROUGHNESS, "metallic": CHANNEL_METALLIC}

FULL_TURN_DEGREE = 360.0
HUE_SECTOR_DEGREE = 60.0
GREEN_SECTOR_OFFSET = 120.0
BLUE_SECTOR_OFFSET = 240.0
EPSILON = 1e-6

# 明るさの揺れを取る「周りの平均」の幅（画素の半分。size に対する割合）と、揺れを ±1 にする明るさの差。
DETAIL_BLUR_RATIO = 1.0 / 256.0
DETAIL_FULL_DIFFERENCE = 0.1


def load_pixels(image, size):
    """image を size×size に縮め、色の変換をせずに (画素, RGBA) の float の配列で返す。"""
    image.colorspace_settings.name = "Non-Color"  # 色の画像も数のまま（sRGB の数）で読む
    image.scale(size, size)
    pixels = numpy.empty(size * size * CHANNEL_NUMBER, dtype=numpy.float32)
    image.pixels.foreach_get(pixels)
    return pixels.reshape(-1, CHANNEL_NUMBER)


def find_fbx_image(name):
    for image in bpy.data.images:
        base = os.path.splitext(os.path.basename(image.filepath.replace("\\", "/")))[0]
        if base == name and image.size[0] > 0:
            return image
    raise RuntimeError("FBX の中に画像 '%s' がありません。" % name)


def gray_sources(config, orm):
    """1. と 2.：元の灰色の画像を、決まったチャンネルへ写す。"""
    size = config["size"]
    if config.get("fbx"):
        bpy.ops.import_scene.fbx(filepath=config["fbx"])
    for key, channel in SOURCE_CHANNEL.items():
        source = config.get(key)
        if not source:
            orm[:, channel] = MISSING_VALUE[key]
            continue
        image = find_fbx_image(source) if config.get("fbx") else bpy.data.images.load(os.path.abspath(source))
        values = load_pixels(image, size)[:, 0]
        if key == "roughness" and config.get("roughness_invert", False):
            values = 1.0 - values
        orm[:, channel] = values


def to_hsv(rgb):
    """(画素, RGB) → 色相（度）・彩度・明るさ。"""
    maximum = rgb.max(axis=1)
    minimum = rgb.min(axis=1)
    delta = maximum - minimum
    saturation = numpy.where(maximum > EPSILON, delta / numpy.maximum(maximum, EPSILON), 0.0)
    red, green, blue = rgb[:, 0], rgb[:, 1], rgb[:, 2]
    colored = delta > EPSILON
    safe_delta = numpy.maximum(delta, EPSILON)
    hue = numpy.zeros_like(maximum)
    red_max = colored & (maximum == red)
    green_max = colored & ~red_max & (maximum == green)
    blue_max = colored & ~red_max & ~green_max
    hue[red_max] = numpy.mod(HUE_SECTOR_DEGREE * (green - blue)[red_max] / safe_delta[red_max], FULL_TURN_DEGREE)
    hue[green_max] = HUE_SECTOR_DEGREE * (blue - red)[green_max] / safe_delta[green_max] + GREEN_SECTOR_OFFSET
    hue[blue_max] = HUE_SECTOR_DEGREE * (red - green)[blue_max] / safe_delta[blue_max] + BLUE_SECTOR_OFFSET
    return hue, saturation, maximum


def in_range(values, bounds):
    low, high = bounds
    return (values >= low) & (values <= high)


def in_hue_range(hue, bounds):
    low, high = bounds
    if low <= high:
        return (hue >= low) & (hue <= high)
    return (hue >= low) | (hue <= high)  # 0° をまたぐ範囲（赤）


def box_blur(values, size, radius):
    """size×size の値を、一辺 2×radius+1 の箱で平均する（端は端の値を伸ばす）。"""
    grid = values.reshape(size, size)
    padded = numpy.pad(grid, radius + 1, mode="edge")
    total = padded.cumsum(axis=0).cumsum(axis=1)
    width = 2 * radius + 1
    sums = (total[width:, width:] - total[:-width, width:] - total[width:, :-width] + total[:-width, :-width])
    return (sums[:size, :size] / (width * width)).ravel()


def albedo_rules(config, orm):
    """3.：色の画像を表で引き、粗さ・金属を決める。"""
    size = config["size"]
    rgb = load_pixels(bpy.data.images.load(os.path.abspath(config["albedo"])), size)[:, :3]
    hue, saturation, value = to_hsv(rgb)
    default = config["default"]
    roughness = numpy.full(size * size, default["roughness"], dtype=numpy.float32)
    metallic = numpy.full(size * size, default["metallic"], dtype=numpy.float32)
    unassigned = numpy.ones(size * size, dtype=bool)
    for rule in config["rules"]:
        match = unassigned & in_hue_range(hue, rule.get("hue", [0.0, FULL_TURN_DEGREE]))
        match &= in_range(saturation, rule.get("saturation", [0.0, 1.0]))
        match &= in_range(value, rule.get("value", [0.0, 1.0]))
        roughness[match] = rule["roughness"]
        metallic[match] = rule["metallic"]
        unassigned &= ~match
        print("RULE %-12s %5.1f%%" % (rule["name"], match.mean() * 100.0))
    detail = config.get("detail", 0.0)
    if detail > 0.0:
        # 周りより暗い所（くぼみ・汚れ）ほど粗く、明るい所ほど少しなめらかにする。
        radius = max(1, int(size * DETAIL_BLUR_RATIO))
        difference = value - box_blur(value, size, radius)
        roughness -= detail * numpy.clip(difference / DETAIL_FULL_DIFFERENCE, -1.0, 1.0)
    orm[:, CHANNEL_AO] = 1.0
    orm[:, CHANNEL_ROUGHNESS] = roughness
    orm[:, CHANNEL_METALLIC] = metallic


def save_tga(orm, size, path):
    out = bpy.data.images.new(os.path.basename(path), size, size, alpha=False)
    out.colorspace_settings.name = "Non-Color"
    out.pixels.foreach_set(numpy.clip(orm, 0.0, 1.0).ravel())
    out.filepath_raw = os.path.abspath(path)
    out.file_format = "TARGA_RAW"
    out.save()
    print("TEXTURE orm ->", path)


def main():
    config_path = sys.argv[sys.argv.index("--") + 1]
    with open(config_path, encoding="utf-8") as f:
        config = json.load(f)
    size = config["size"]
    orm = numpy.ones((size * size, CHANNEL_NUMBER), dtype=numpy.float32)
    if config.get("albedo"):
        albedo_rules(config, orm)
    else:
        gray_sources(config, orm)
    print("MEAN ao %.2f roughness %.2f metallic %.2f" % tuple(orm[:, c].mean() for c in range(3)))
    save_tga(orm, size, config["output"])


main()
