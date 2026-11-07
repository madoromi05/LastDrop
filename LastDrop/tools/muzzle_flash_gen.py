# -*- coding: utf-8 -*-
# =====================================================================
# muzzle_flash_gen.py
#   マズルフラッシュの絵（MuzzleFlash が加算で描く板の絵）を 2 枚作る。
#   炎の形は描かない。銃口に一瞬灯る「光」に見せる（2026-10-17 ユーザー指定）。
#
#   muzzle_flash_sheet.tga … 光の芯を 4 つ、2×2 のます目に並べた絵（2026-10-25。EffectFlipbook_Plan.txt の 4-1）。
#       撃つたびに 1 つを選ぶ（コマ送りではない）。並びは左上から右へ、行ごとに下へ 0〜3（Render/Flipbook と同じ）。
#       4 つは「光の丸のふちを、角度で少しだけ出し入れした」違い（光のにじみ方が毎発少し違って見える程度。とげの形にはしない）。
#       色は絵に入れる。中心は白、外へ行くほど淡い黄色 → オレンジ → 黒。アルファはどこも不透明。
#       加算で描くので黒は透明と同じになる（アルファでも薄くすると、頂点のアルファと二重に掛かって外が暗くなりすぎる）。
#   muzzle_glow.tga  … 光のにじみの板の絵。中心から外へなめらかに暗くなる丸。
#       色は白（濃淡はアルファだけ）。色はゲーム側で頂点の色として掛ける（MuzzleFlash.cpp の kGlowColor）。
#
#   どちらも端（丸の外）は 0 まで落とす（板の四角い輪郭が見えないように）。芯のます目のふちにも黒のすき間を残す。
#   前の 1 枚だけの芯（muzzle_flash.tga）は、ゲームではもう使っていない。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\4.5\python\bin\python.exe" tools/muzzle_flash_gen.py [芯.tga] [にじみ.tga]
#   出力を省くと Resource/Texture/Effect/muzzle_flash_sheet.tga と Resource/Texture/Effect/muzzle_glow.tga に書く。
# =====================================================================

import math
import sys

from tga_image import smooth_step, to_byte, write_tga

DEFAULT_CORE_OUTPUT = "Resource/Texture/Effect/muzzle_flash_sheet.tga"
DEFAULT_GLOW_OUTPUT = "Resource/Texture/Effect/muzzle_glow.tga"

SIZE = 64  # 1 枚の絵（にじみ）と、芯の 1 コマの大きさ。2 の累乗（bullet_hole_gen.py と同じ理由）

# 芯のます目。
CORE_COLUMNS = 2
CORE_ROWS = 2
CORE_MARGIN = 2  # ます目のふちのすき間（画素。遠くで縮めた絵で、隣のコマがにじまないように）

# 芯の 4 つの違い（ふちを出し入れする山の数, 出し入れの大きさ, 向きのずれ（ラジアン））。
# 出し入れの大きさは丸の半径に対する割合。0.12 でも 64 画素では星の形に見えた（2026-10-25 に絵で確認）ので、その半分ほどにする。
CORE_SHAPES = (
    (4, 0.06, 0.0),
    (5, 0.07, 0.6),
    (6, 0.05, 1.3),
    (3, 0.08, 2.1),
)

# 明るさ（0〜1）→ 色。間は線形に混ぜる。加算で描くので、黒（0）は透明と同じ。
COLOR_STOPS = (
    (0.0, (0.0, 0.0, 0.0)),
    (0.3, (0.9, 0.45, 0.12)),  # オレンジ（光のふち）
    (0.6, (1.0, 0.82, 0.55)),  # 淡い黄色
    (1.0, (1.0, 1.0, 1.0)),    # 白（中心）
)

# 光の芯。半径は絵の半分を 1 とした割合。
CORE_RADIUS = 0.25    # ここまでは真っ白
CORE_FALLOFF = 6.0    # 芯の外の暗くなり方（exp(-CORE_FALLOFF × 距離²)。大きいほど急に暗くなる）
# 出し入れした丸が絵のふちを越えないよう、距離をこれだけ大きく数える（一番出た所でも 1 に収まる）。
CORE_FIT = 1.0 + max(amount for _, amount, _ in CORE_SHAPES)

# にじみ。
GLOW_FALLOFF = 4.0    # 大きいほど中心に集まる
BYTE_WHITE = 255      # にじみの色（白。濃淡はアルファだけ）


def falloff(distance, strength):
    """exp(-strength × 距離²) を、端（距離 1）でちょうど 0 になるよう引き伸ばした値（中心が 1）。"""
    floor = math.exp(-strength)
    distance = min(distance, 1.0)
    return (math.exp(-strength * distance * distance) - floor) / (1.0 - floor)


def lerp(a, b, t):
    return a + (b - a) * t


def ramp_color(intensity):
    """明るさ（0〜1）を COLOR_STOPS の色にする。"""
    intensity = min(max(intensity, 0.0), 1.0)
    for (low, low_color), (high, high_color) in zip(COLOR_STOPS, COLOR_STOPS[1:]):
        if intensity <= high:
            t = (intensity - low) / (high - low)
            return tuple(lerp(a, b, t) for a, b in zip(low_color, high_color))
    return COLOR_STOPS[-1][1]


def core_intensity(distance):
    """光の芯の、中心からの距離 distance での明るさ（0〜1）。"""
    white = 1.0 - smooth_step(0.0, CORE_RADIUS, distance)
    return max(white, falloff(distance, CORE_FALLOFF))


def core_pixel(x, y, shape):
    """芯の 1 コマの点 (x, y)（コマの中心が原点、半分が 1）の (赤, 緑, 青, 不透明度)。"""
    lobes, amount, phase = shape
    # 角度でふちを出し入れする（出た向きは距離を小さく数える＝明るい所が外まで届く）。
    reach = 1.0 + amount * math.cos(lobes * math.atan2(y, x) + phase)
    distance = math.hypot(x, y) * CORE_FIT / reach
    red, green, blue = ramp_color(core_intensity(distance))
    return red, green, blue, 1.0


def write_core_sheet(path):
    width = SIZE * CORE_COLUMNS
    half = SIZE / 2.0
    inner_half = half - CORE_MARGIN
    body = bytearray()
    for row in range(width):
        for column in range(width):
            shape = CORE_SHAPES[(row // SIZE) * CORE_COLUMNS + (column // SIZE)]
            x = (column % SIZE + 0.5 - half) / inner_half
            y = (row % SIZE + 0.5 - half) / inner_half
            if abs(x) > 1.0 or abs(y) > 1.0:
                body += bytes((0, 0, 0, 255))  # すき間（加算なので黒は透明と同じ）
                continue
            red, green, blue, opacity = core_pixel(x, y, shape)
            body += bytes((to_byte(blue), to_byte(green), to_byte(red), to_byte(opacity)))
    write_tga(path, width, body)
    print("wrote", path)


def write_glow(path):
    half = SIZE / 2.0
    body = bytearray()
    for row in range(SIZE):
        for column in range(SIZE):
            x = (column + 0.5 - half) / half
            y = (row + 0.5 - half) / half
            opacity = falloff(math.hypot(x, y), GLOW_FALLOFF)
            body += bytes((BYTE_WHITE, BYTE_WHITE, BYTE_WHITE, to_byte(opacity)))
    write_tga(path, SIZE, body)
    print("wrote", path)


def main():
    core_output = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_CORE_OUTPUT
    glow_output = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_GLOW_OUTPUT
    write_core_sheet(core_output)
    write_glow(glow_output)


if __name__ == "__main__":
    main()
