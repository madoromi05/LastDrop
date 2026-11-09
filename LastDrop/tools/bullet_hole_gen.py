# -*- coding: utf-8 -*-
# =====================================================================
# bullet_hole_gen.py
#   弾痕の絵（BulletHoleList が壁・地面に貼る板の絵）を作る。
#   真ん中の黒い穴、その周りの欠け（ふちがギザギザ）、外へ伸びるヒビ を、アルファ付きで描く。
#   素材を探さずに済ませ、大きさ・色を数字で直せるように、絵はこのスクリプトで作る（音の SoundGen と同じ考え方）。
#   材質（コンクリート・金属・地面）で絵は分けない（2026-10-15 ユーザー指定）。
#   乱数の種を固定しているので、何度作っても同じ絵になる。
#   Python の標準ライブラリだけで動く。この PC には Python 単体が入っていないので、Blender に入っている Python で動かす。
#   凹みの法線の絵も一緒に作る（デカールが G-Buffer の法線に書く。SummaryPlan/個別仕様/グラフィック/SsrDecalMaterial_Plan.txt 8-4）。
#   穴・欠け・ヒビの深さで高さを作り、隣のピクセルとの差から法線を出す。並びは r 横（右が正）・g 縦（絵の上が正）・b 面の外向き。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\4.5\python\bin\python.exe" tools/bullet_hole_gen.py [出力.tga] [法線の出力.tga]
#   出力を省くと Resource/Texture/Effect/bullet_hole.tga と Resource/Texture/Effect/bullet_hole_normal.tga に書く。その後 tools/texture_to_dds.ps1 bullet_hole で DDS にする。
# =====================================================================

import math
import random
import sys

from tga_image import over, smooth_step, to_byte, write_tga

DEFAULT_OUTPUT = "Resource/Texture/Effect/bullet_hole.tga"
DEFAULT_NORMAL_OUTPUT = "Resource/Texture/Effect/bullet_hole_normal.tga"

# 絵の大きさ（ピクセル）。GameLib は 2 の累乗でない絵を 2 の累乗へ広げて読み、UV がずれるので 2 の累乗にする。
SIZE = 64
SEED = 7621  # 乱数の種（7.62mm から）

# 半径はどれも、絵の半分を 1 とした割合。
HOLE_RADIUS = 0.16       # 真ん中の穴
CHIP_RADIUS = 0.50       # 欠けの平均の半径
CHIP_JAGGED = 0.25       # 欠けのふちの揺れ（半径に対する割合）
CHIP_WAVES = 3           # ふちの揺れを作る波の数（重ねる）
CHIP_MIN_FREQUENCY = 2   # 波の山の数（1 周あたり）の範囲
CHIP_MAX_FREQUENCY = 9
CRACK_NUMBER = 6         # ヒビの本数
CRACK_MIN_LENGTH = 0.65  # ヒビの長さの範囲
CRACK_MAX_LENGTH = 0.95
CRACK_WIDTH = 0.035      # ヒビの根元の太さの半分
EDGE_SOFTNESS = 0.06     # ふちをぼかす幅（ギザギザのジャギーを抑える）

# 色（0〜1 の灰色）と不透明度。壁と同じライトが当たる（日なたで明るく、太陽と反対を向いた面で暗くなる）。
HOLE_SHADE = 0.04
CHIP_SHADE = 0.30
CHIP_OPACITY = 0.85
CRACK_SHADE = 0.15
CRACK_OPACITY = 0.7

# 凹みの深さ（面を 0 とした下向きの深さ。絵の半分を 1 とした割合）。法線の絵だけに使う。
HOLE_DEPTH = 0.5
CHIP_DEPTH = 0.12
CRACK_DEPTH = 0.04
# 法線の傾きの強さ（高さの差に掛ける）。大きいほど縁が立って見える。見た目を確かめる前の初期値。
NORMAL_STRENGTH = 2.0


def make_chip_edge(rng):
    """角度（ラジアン）→ 欠けのふちの半径 を返す関数を作る。波を重ねてギザギザにする。"""
    waves = [(rng.randint(CHIP_MIN_FREQUENCY, CHIP_MAX_FREQUENCY), rng.uniform(0.0, 2.0 * math.pi))
             for _ in range(CHIP_WAVES)]

    def edge(angle):
        wobble = sum(math.sin(frequency * angle + phase) for frequency, phase in waves) / CHIP_WAVES
        return CHIP_RADIUS * (1.0 + CHIP_JAGGED * wobble)
    return edge


def make_cracks(rng):
    """ヒビ（向きのラジアン, 長さ）の一覧。"""
    return [(rng.uniform(0.0, 2.0 * math.pi), rng.uniform(CRACK_MIN_LENGTH, CRACK_MAX_LENGTH))
            for _ in range(CRACK_NUMBER)]


def crack_opacity(x, y, cracks):
    """点 (x, y) に重なるヒビのうち一番濃い不透明度（0〜1）。先へ行くほど細く薄くなる。"""
    best = 0.0
    for angle, length in cracks:
        dx, dy = math.cos(angle), math.sin(angle)
        along = x * dx + y * dy  # ヒビの向きへ進んだ距離
        if along <= 0.0 or along >= length:
            continue
        across = abs(-x * dy + y * dx)  # ヒビの線からの離れ
        taper = 1.0 - along / length    # 先ほど 0 に近づく
        width = CRACK_WIDTH * taper
        best = max(best, (1.0 - smooth_step(width * 0.5, width, across)) * taper)
    return best


def shade_pixel(x, y, chip_edge, cracks):
    """点 (x, y)（絵の中心が原点、半分が 1）の (灰色, 不透明度)。下から ヒビ → 欠け → 穴 の順に重ねる。"""
    radius = math.hypot(x, y)
    edge = chip_edge(math.atan2(y, x))
    crack = (CRACK_SHADE, crack_opacity(x, y, cracks) * CRACK_OPACITY)
    chip = (CHIP_SHADE, (1.0 - smooth_step(edge - EDGE_SOFTNESS, edge, radius)) * CHIP_OPACITY)
    hole = (HOLE_SHADE, 1.0 - smooth_step(HOLE_RADIUS - EDGE_SOFTNESS, HOLE_RADIUS, radius))
    return over(hole, over(chip, crack))


def height_pixel(x, y, chip_edge, cracks):
    """点 (x, y) の高さ（面が 0、凹むほど負）。shade_pixel と同じ形の穴・欠け・ヒビを、深さとして足す。"""
    radius = math.hypot(x, y)
    edge = chip_edge(math.atan2(y, x))
    hole = 1.0 - smooth_step(HOLE_RADIUS - EDGE_SOFTNESS, HOLE_RADIUS, radius)
    chip = 1.0 - smooth_step(edge - EDGE_SOFTNESS, edge, radius)
    crack = crack_opacity(x, y, cracks)
    return -(HOLE_DEPTH * hole + CHIP_DEPTH * chip + CRACK_DEPTH * crack)


def normal_body(heights):
    """高さの表（行は上から）から、法線の絵（B,G,R,A の並び）を作る。端は隣が無いので、端の値をそのまま使う。"""
    body = bytearray()
    step = 2.0 / SIZE  # 1 ピクセルの幅（絵の半分を 1 とした割合）
    for row in range(SIZE):
        for column in range(SIZE):
            left = heights[row][max(column - 1, 0)]
            right = heights[row][min(column + 1, SIZE - 1)]
            above = heights[max(row - 1, 0)][column]
            below = heights[min(row + 1, SIZE - 1)][column]
            # 高さが右へ上がるほど法線は左へ、絵の上へ上がるほど法線は下へ傾く（行は下へ数えるので、上向きの差は above − below）。
            nx = -(right - left) / (2.0 * step) * NORMAL_STRENGTH
            ny = -(above - below) / (2.0 * step) * NORMAL_STRENGTH
            length = math.sqrt(nx * nx + ny * ny + 1.0)
            r, g, b = (nx / length * 0.5 + 0.5, ny / length * 0.5 + 0.5, 1.0 / length * 0.5 + 0.5)
            body += bytes((to_byte(b), to_byte(g), to_byte(r), 255))
    return body


def main():
    output = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUTPUT
    normal_output = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_NORMAL_OUTPUT
    rng = random.Random(SEED)
    chip_edge = make_chip_edge(rng)
    cracks = make_cracks(rng)
    half = SIZE / 2.0
    body = bytearray()
    heights = []
    for row in range(SIZE):
        height_row = []
        for column in range(SIZE):
            # ピクセルの中心を、絵の中心が原点・半分が 1 の座標へ直す。
            x = (column + 0.5 - half) / half
            y = (row + 0.5 - half) / half
            shade, opacity = shade_pixel(x, y, chip_edge, cracks)
            gray = to_byte(shade)
            body += bytes((gray, gray, gray, to_byte(opacity)))
            height_row.append(height_pixel(x, y, chip_edge, cracks))
        heights.append(height_row)
    write_tga(output, SIZE, body)
    print("wrote", output)
    write_tga(normal_output, SIZE, normal_body(heights))
    print("wrote", normal_output)


if __name__ == "__main__":
    main()
