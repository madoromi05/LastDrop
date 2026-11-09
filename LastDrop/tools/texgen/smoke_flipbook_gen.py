# -*- coding: utf-8 -*-
# =====================================================================
# smoke_flipbook_gen.py
#   着弾の砂ぼこりのコマ送りの絵（ImpactDustList が粒ごとに 1 コマを貼る）を作る。
#   仕様は SummaryPlan/個別仕様/射撃と手応え/EffectFlipbook_Plan.txt の 4-1。
#
#   4×4 のます目に 16 コマ。左上から右へ、行ごとに下へ 0〜15 コマ目（Render/Flipbook と同じ並び）。
#   はじめは小さく濃いかたまり → ぼけた丸が外へ広がって大きくなる → ふちがふんわりして少し薄れる。
#   消えていく濃さはゲーム側（ImpactDust::particleOpacity）でも下げるので、絵ではあまり薄くしない（二重に薄れすぎないように）。
#   ます目のふちには透明のすき間（MARGIN 画素）を残す。遠くで縮めた絵（ミップマップ）で、隣のコマがにじまないように。
#   色は白（濃淡だけ）。粉の色はゲーム側で頂点の色として掛ける（ImpactDustList.cpp の kDust*）。
#   作り（ぼけた丸を重ねる）は dust_gen.py と同じ。乱数の種を固定しているので、何度作っても同じ絵になる。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\4.5\python\bin\python.exe" tools/texgen/smoke_flipbook_gen.py [出力.tga]
#   出力を省くと Resource/Texture/Effect/impact_smoke_sheet.tga に書く。
# =====================================================================

import math
import random
import sys

from tga_image import over, smooth_step, to_byte, write_tga

DEFAULT_OUTPUT = "Resource/Texture/Effect/impact_smoke_sheet.tga"

COLUMNS = 4
ROWS = 4
FRAME_NUMBER = COLUMNS * ROWS
CELL = 64                  # 1 コマの大きさ（画素）。今までの 1 枚の絵（dust_gen.py）と同じ細かさ
SIZE = CELL * COLUMNS      # 絵全体（256。2 の累乗）
MARGIN = 2                 # ます目のふちの透明のすき間（画素）
SEED = 7624

# 重ねる丸。半径・ずれはどれも、コマの半分（すき間を除く）を 1 とした割合。
# 最後のコマで ずれ＋半径 が 1 を超えないので、コマのふちは透明になる。
PUFF_NUMBER = 7
PUFF_MAX_OFFSET = 0.4
PUFF_MIN_RADIUS = 0.3
PUFF_MAX_RADIUS = 0.55
PUFF_MIN_SHADE = 0.8       # 丸ごとの明るさの範囲（かたまりに濃淡を出す）
PUFF_MAX_SHADE = 1.0
PUFF_OPACITY = 0.6         # 丸 1 つの一番濃い所の不透明度（重なった所が濃くなる）

# コマの進み具合（0〜1）での変わり方。始め → 終わり。
START_SPREAD = 0.3         # 丸のずれの倍率（中心に集まったかたまりから、外へ広がる）
START_SIZE = 0.5           # 丸の半径の倍率
START_SOFTNESS = 0.45      # 半径のうち、ふちからぼかす割合（後ほどふんわり）
END_SOFTNESS = 0.85
END_OPACITY = 0.7          # 最後のコマの濃さの倍率（薄れはゲーム側が主）


def ease_out(t):
    """始めに速く進み、だんだんゆっくりになる（煙が広がる感じ。ImpactDust の広がり方と同じ形）。"""
    remain = 1.0 - t
    return 1.0 - remain * remain


def make_puffs(rng):
    """重ねる丸（向き, ずれ, 半径, 明るさ）の一覧。全部のコマで同じ丸を使い、広げ方だけ変える。"""
    puffs = []
    for _ in range(PUFF_NUMBER):
        puffs.append((rng.uniform(0.0, 2.0 * math.pi), rng.uniform(0.0, PUFF_MAX_OFFSET),
                      rng.uniform(PUFF_MIN_RADIUS, PUFF_MAX_RADIUS), rng.uniform(PUFF_MIN_SHADE, PUFF_MAX_SHADE)))
    return puffs


def frame_puffs(puffs, t):
    """進み具合 t（0〜1）のコマで使う丸（中心 x, 中心 y, 半径, 明るさ, ぼかし, 濃さ）。"""
    grow = ease_out(t)
    spread = START_SPREAD + (1.0 - START_SPREAD) * grow
    size = START_SIZE + (1.0 - START_SIZE) * grow
    softness = START_SOFTNESS + (END_SOFTNESS - START_SOFTNESS) * t
    opacity = PUFF_OPACITY * (1.0 + (END_OPACITY - 1.0) * t)
    placed = []
    for angle, offset, radius, shade in puffs:
        distance = offset * spread
        placed.append((distance * math.cos(angle), distance * math.sin(angle), radius * size, shade, softness, opacity))
    return placed


def shade_pixel(x, y, puffs):
    """点 (x, y)（コマの中心が原点、半分が 1）の (灰色, 不透明度)。丸を順に重ねる。"""
    pixel = (0.0, 0.0)
    for center_x, center_y, radius, shade, softness, opacity in puffs:
        distance = math.hypot(x - center_x, y - center_y)
        alpha = (1.0 - smooth_step(radius * (1.0 - softness), radius, distance)) * opacity
        pixel = over((shade, alpha), pixel)
    return pixel


def main():
    output = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUTPUT
    puffs = make_puffs(random.Random(SEED))
    frames = [frame_puffs(puffs, frame / (FRAME_NUMBER - 1.0)) for frame in range(FRAME_NUMBER)]
    half = CELL / 2.0
    inner_half = half - MARGIN
    body = bytearray()
    for row in range(SIZE):
        for column in range(SIZE):
            frame = (row // CELL) * COLUMNS + (column // CELL)
            # ピクセルの中心を、コマの中心が原点・すき間を除いた半分が 1 の座標へ直す。
            x = (column % CELL + 0.5 - half) / inner_half
            y = (row % CELL + 0.5 - half) / inner_half
            if abs(x) > 1.0 or abs(y) > 1.0:
                body += bytes((0, 0, 0, 0))  # すき間
                continue
            shade, opacity = shade_pixel(x, y, frames[frame])
            gray = to_byte(shade)
            body += bytes((gray, gray, gray, to_byte(opacity)))
    write_tga(output, SIZE, body)
    print("wrote", output)


if __name__ == "__main__":
    main()
