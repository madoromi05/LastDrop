# -*- coding: utf-8 -*-
# =====================================================================
# dust_gen.py
#   着弾の砂ぼこりの粒の絵（ImpactDustList が画面に平行な板として描く絵）を作る。
#   ふちがぼけた丸をいくつか少しずらして重ね、もこもこした粉のかたまりにする。
#   色は白（濃淡だけ）。粉の色はゲーム側で頂点の色として掛ける（ImpactDustList.cpp の kDust*）ので、
#   色を直すのに絵を作り直さなくてよい。
#   遠くで見えるのは、絵ではなく板の大きさ（中心の粒で直径 60cm、外の粒で約 80cm まで広がる。ImpactDust.cpp）と粒の数（8 個が重なる）による。
#   絵の不透明度の平均は弾痕の絵と同じくらい（約 0.15）なので、遠くで縮めた絵（ミップマップ）では同じように薄まる。
#   乱数の種を固定しているので、何度作っても同じ絵になる。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\4.5\python\bin\python.exe" tools/dust_gen.py [出力.tga]
#   出力を省くと Resource/impact_dust.tga に書く。
# =====================================================================

import math
import random
import sys

from tga_image import over, smooth_step, to_byte, write_tga

DEFAULT_OUTPUT = "Resource/impact_dust.tga"

SIZE = 64   # 2 の累乗（bullet_hole_gen.py と同じ理由）
SEED = 7622

# 重ねる丸。半径・ずれはどれも、絵の半分を 1 とした割合。ずれ＋半径 が 1 を超えないので、絵のふちは透明になる。
PUFF_NUMBER = 5
PUFF_MAX_OFFSET = 0.35
PUFF_MIN_RADIUS = 0.35
PUFF_MAX_RADIUS = 0.55
PUFF_SOFTNESS = 0.6   # 半径のうち、ふちからぼかす割合（大きいほどふんわり）
PUFF_MIN_SHADE = 0.8  # 丸ごとの明るさの範囲（少しずつ変えて、かたまりに濃淡を出す）
PUFF_MAX_SHADE = 1.0
PUFF_OPACITY = 0.6    # 丸 1 つの一番濃い所の不透明度（重なった所が濃くなる）


def make_puffs(rng):
    """重ねる丸（中心 x, 中心 y, 半径, 明るさ）の一覧。"""
    puffs = []
    for _ in range(PUFF_NUMBER):
        angle = rng.uniform(0.0, 2.0 * math.pi)
        offset = rng.uniform(0.0, PUFF_MAX_OFFSET)
        puffs.append((offset * math.cos(angle), offset * math.sin(angle),
                      rng.uniform(PUFF_MIN_RADIUS, PUFF_MAX_RADIUS), rng.uniform(PUFF_MIN_SHADE, PUFF_MAX_SHADE)))
    return puffs


def shade_pixel(x, y, puffs):
    """点 (x, y)（絵の中心が原点、半分が 1）の (灰色, 不透明度)。丸を順に重ねる。"""
    pixel = (0.0, 0.0)
    for center_x, center_y, radius, shade in puffs:
        distance = math.hypot(x - center_x, y - center_y)
        opacity = (1.0 - smooth_step(radius * (1.0 - PUFF_SOFTNESS), radius, distance)) * PUFF_OPACITY
        pixel = over((shade, opacity), pixel)
    return pixel


def main():
    output = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUTPUT
    puffs = make_puffs(random.Random(SEED))
    half = SIZE / 2.0
    body = bytearray()
    for row in range(SIZE):
        for column in range(SIZE):
            # ピクセルの中心を、絵の中心が原点・半分が 1 の座標へ直す。
            x = (column + 0.5 - half) / half
            y = (row + 0.5 - half) / half
            shade, opacity = shade_pixel(x, y, puffs)
            gray = to_byte(shade)
            body += bytes((gray, gray, gray, to_byte(opacity)))
    write_tga(output, SIZE, body)
    print("wrote", output)


if __name__ == "__main__":
    main()
