# -*- coding: utf-8 -*-
# =====================================================================
# blood_gen.py
#   血しぶきの粒の絵（BloodSprayList が画面に平行な板として描く絵）を作る。
#   ふちが少しギザギザの丸い滴を真ん中に 1 つ、その周りに小さな滴をいくつか描く。
#   砂ぼこり（dust_gen.py）と違い、血は粉ではなく液体なので、ぼかさずに濃く（ほぼ不透明に）する。
#   色は白（濃淡だけ）。血の色はゲーム側で頂点の色として掛ける（BloodSprayList.cpp の kBlood*）ので、
#   色を直すのに絵を作り直さなくてよい。
#   乱数の種を固定しているので、何度作っても同じ絵になる。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\4.5\python\bin\python.exe" tools/blood_gen.py [出力.tga]
#   出力を省くと Resource/Texture/Effect/blood_drop.tga に書く。
# =====================================================================

import math
import random
import sys

from tga_image import over, smooth_step, to_byte, write_tga

DEFAULT_OUTPUT = "Resource/Texture/Effect/blood_drop.tga"

SIZE = 64   # 2 の累乗（bullet_hole_gen.py と同じ理由）
SEED = 7623

# 半径・ずれはどれも、絵の半分を 1 とした割合。ずれ＋半径 が 1 を超えないので、絵のふちは透明になる。
# ── 真ん中の滴 ──
DROP_RADIUS = 0.42
# ふちのギザギザ。一周する間の波の数が違う波を、ばらばらの深さと位相で足す（1 つの波だと星形に見えるため）。
DROP_JAGGED_WAVES = (2, 3, 5, 8)
DROP_JAGGED_MAX = 0.05   # 波 1 つの深さの上限（半径に足し引きする量）
DROP_SOFTNESS = 0.12     # 半径のうち、ふちからぼかす割合（液体なので小さく）
DROP_OPACITY = 0.95
# 真ん中ほど明るくして、丸みのある滴に見せる（ふちの明るさ → 真ん中の明るさ）。
DROP_EDGE_SHADE = 0.75
DROP_CENTER_SHADE = 1.0
# ── 周りの小さな滴 ──
SPECK_NUMBER = 6
SPECK_MIN_OFFSET = 0.55
SPECK_MAX_OFFSET = 0.82
SPECK_MIN_RADIUS = 0.05
SPECK_MAX_RADIUS = 0.13
SPECK_SOFTNESS = 0.3
SPECK_OPACITY = 0.85
SPECK_SHADE = 0.8


def make_specks(rng):
    """周りの小さな滴（中心 x, 中心 y, 半径）の一覧。ずれ＋半径 が 1 を超えないものだけ。"""
    specks = []
    while len(specks) < SPECK_NUMBER:
        angle = rng.uniform(0.0, 2.0 * math.pi)
        offset = rng.uniform(SPECK_MIN_OFFSET, SPECK_MAX_OFFSET)
        radius = rng.uniform(SPECK_MIN_RADIUS, SPECK_MAX_RADIUS)
        if offset + radius < 1.0:
            specks.append((offset * math.cos(angle), offset * math.sin(angle), radius))
    return specks


def make_waves(rng):
    """ふちを波打たせる波（一周の波の数, 深さ, 位相）の一覧。"""
    return [(count, rng.uniform(0.0, DROP_JAGGED_MAX), rng.uniform(0.0, 2.0 * math.pi)) for count in DROP_JAGGED_WAVES]


def drop_pixel(x, y, waves):
    """真ん中の滴の (明るさ, 不透明度)。ふちの半径を角度で波打たせてギザギザにする。"""
    distance = math.hypot(x, y)
    angle = math.atan2(y, x)
    radius = DROP_RADIUS + sum(depth * math.sin(angle * count + phase) for count, depth, phase in waves)
    opacity = (1.0 - smooth_step(radius * (1.0 - DROP_SOFTNESS), radius, distance)) * DROP_OPACITY
    inner = min(distance / radius, 1.0)
    shade = DROP_CENTER_SHADE + (DROP_EDGE_SHADE - DROP_CENTER_SHADE) * inner * inner
    return (shade, opacity)


def shade_pixel(x, y, waves, specks):
    """点 (x, y)（絵の中心が原点、半分が 1）の (明るさ, 不透明度)。小さな滴の上に真ん中の滴を重ねる。"""
    pixel = (0.0, 0.0)
    for center_x, center_y, radius in specks:
        distance = math.hypot(x - center_x, y - center_y)
        opacity = (1.0 - smooth_step(radius * (1.0 - SPECK_SOFTNESS), radius, distance)) * SPECK_OPACITY
        pixel = over((SPECK_SHADE, opacity), pixel)
    return over(drop_pixel(x, y, waves), pixel)


def main():
    output = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUTPUT
    rng = random.Random(SEED)
    waves = make_waves(rng)
    specks = make_specks(rng)
    half = SIZE / 2.0
    body = bytearray()
    for row in range(SIZE):
        for column in range(SIZE):
            # ピクセルの中心を、絵の中心が原点・半分が 1 の座標へ直す。
            x = (column + 0.5 - half) / half
            y = (row + 0.5 - half) / half
            shade, opacity = shade_pixel(x, y, waves, specks)
            gray = to_byte(shade)
            body += bytes((gray, gray, gray, to_byte(opacity)))
    write_tga(output, SIZE, body)
    print("wrote", output)


if __name__ == "__main__":
    main()
