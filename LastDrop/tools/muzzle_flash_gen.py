# -*- coding: utf-8 -*-
# =====================================================================
# muzzle_flash_gen.py
#   マズルフラッシュの絵（MuzzleFlash が加算で描く板の絵）を 2 枚作る。
#   炎の形は描かない。銃口に一瞬灯る「光」に見せる（2026-10-17 ユーザー指定）。
#
#   muzzle_flash.tga … 光の芯。中心が白く、ふちへ向かって急に暗くなる丸。
#       色は絵に入れる。中心は白、外へ行くほど淡い黄色 → オレンジ → 黒。アルファはどこも不透明。
#       加算で描くので黒は透明と同じになる（アルファでも薄くすると、頂点のアルファと二重に掛かって外が暗くなりすぎる）。
#   muzzle_glow.tga  … 光のにじみの板の絵。中心から外へなめらかに暗くなる丸。
#       色は白（濃淡はアルファだけ）。色はゲーム側で頂点の色として掛ける（MuzzleFlash.cpp の kGlowColor）。
#
#   どちらも端（丸の外）は 0 まで落とす（板の四角い輪郭が見えないように）。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\4.5\python\bin\python.exe" tools/muzzle_flash_gen.py [芯.tga] [にじみ.tga]
#   出力を省くと Resource/muzzle_flash.tga と Resource/muzzle_glow.tga に書く。
# =====================================================================

import math
import sys

from tga_image import smooth_step, to_byte, write_tga

DEFAULT_CORE_OUTPUT = "Resource/muzzle_flash.tga"
DEFAULT_GLOW_OUTPUT = "Resource/muzzle_glow.tga"

SIZE = 64  # 2 の累乗（bullet_hole_gen.py と同じ理由）

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

# にじみ。
GLOW_FALLOFF = 4.0    # 大きいほど中心に集まる


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


def write_image(path, pixel):
    """pixel(距離) → (赤, 緑, 青, 不透明度) で、丸い絵を書く。"""
    half = SIZE / 2.0
    body = bytearray()
    for row in range(SIZE):
        for column in range(SIZE):
            x = (column + 0.5 - half) / half
            y = (row + 0.5 - half) / half
            red, green, blue, opacity = pixel(math.hypot(x, y))
            body += bytes((to_byte(blue), to_byte(green), to_byte(red), to_byte(opacity)))
    write_tga(path, SIZE, body)
    print("wrote", path)


def core_pixel(distance):
    red, green, blue = ramp_color(core_intensity(distance))
    return red, green, blue, 1.0


def glow_pixel(distance):
    return 1.0, 1.0, 1.0, falloff(distance, GLOW_FALLOFF)


def main():
    core_output = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_CORE_OUTPUT
    glow_output = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_GLOW_OUTPUT
    write_image(core_output, core_pixel)
    write_image(glow_output, glow_pixel)


if __name__ == "__main__":
    main()
