# -*- coding: utf-8 -*-
# =====================================================================
# blood_stain_gen.py
#   血の跡の絵（BloodStainList が撃ったゾンビの後ろの壁・足元の地面にデカールとして貼る絵）を作る。
#   仕様は SummaryPlan/個別仕様/BloodStain_Plan.txt 5 章（グラフィック段階10）。
#   4 種類を 2×2 に並べた 1 枚にする（デカールごとに UV の範囲で選ぶ。Q1）。
#     左上・右上・左下 … 飛び散った血。真ん中より少し左の溜まりから、絵の右（+x）へ筋と点が伸びる。
#                         貼るときに絵の右を弾の進む向きに合わせる（跡の形で撃った向きが分かる）
#     右下             … 落ちた滴の跡。筋の無い丸い滴と小さな点だけ（向きはばらして貼る）
#   血の色は絵に焼く（デカールは G-Buffer の色をそのまま書き換えるので、頂点の色で掛ける血しぶき（blood_drop）とは違う）。
#   厚い所ほど暗い赤、薄い縁ほど明るい赤にする（溜まった血は暗く見える）。
#   凹凸の法線の絵も一緒に作る（bullet_hole_gen.py と同じく、高さの隣の画素との差から）。血の厚みで縁が少し盛り上がる。
#   乱数の種を固定しているので、何度作っても同じ絵になる。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\4.5\python\bin\python.exe" tools/blood_stain_gen.py [出力.tga] [法線の出力.tga]
#   出力を省くと Resource/Texture/Effect/blood_stain.tga と Resource/Texture/Effect/blood_stain_normal.tga に書く。その後 tools/texture_to_dds.ps1 blood_stain で DDS にする。
# =====================================================================

import math
import random
import sys

from tga_image import smooth_step, to_byte, write_tga

DEFAULT_OUTPUT = "Resource/Texture/Effect/blood_stain.tga"
DEFAULT_NORMAL_OUTPUT = "Resource/Texture/Effect/blood_stain_normal.tga"

# 絵の大きさ（ピクセル。2 の累乗）と、1 種類の絵の大きさ（2×2 に並べる）。
SIZE = 256
CELLS_PER_ROW = 2
CELL_SIZE = SIZE // CELLS_PER_ROW
SEED = 7624
SPLATTER_CELLS = 3  # 飛び散った血の絵の数。残りの 1 つ（右下）が滴の跡

# 位置・半径はどれも、1 種類の絵の半分を 1 とした割合。絵の端に届かせない上限（縮小の段で隣の絵がにじまないように）。
CELL_MARGIN = 0.92

# ── 飛び散った血 ──
POOL_CENTER_X = -0.3      # 溜まりの中心（弾が当たった所。絵の右へ飛び散るので左に寄せる）
POOL_RADIUS = 0.3
POOL_JAGGED_WAVES = (2, 3, 5, 7)
POOL_JAGGED_MAX = 0.03
STREAK_MIN_NUMBER = 5
STREAK_MAX_NUMBER = 9
STREAK_SPREAD_DEGREE = 35.0   # 筋が絵の右から上下へ開く角度の上限
STREAK_MIN_LENGTH = 0.55
STREAK_MAX_LENGTH = 1.15
STREAK_MIN_WIDTH = 0.025      # 根元の太さの半分
STREAK_MAX_WIDTH = 0.06
STREAK_TIP_RADIUS = 0.6       # 筋の先の滴の半径（根元の太さに対する割合。先に溜まって丸くなる）
SPECK_MIN_NUMBER = 10
SPECK_MAX_NUMBER = 18
SPECK_SPREAD_DEGREE = 60.0
SPECK_MIN_DISTANCE = 0.2
SPECK_MAX_DISTANCE = 1.1
SPECK_MIN_RADIUS = 0.015
SPECK_MAX_RADIUS = 0.05

# ── 落ちた滴の跡 ──
DRIP_RADIUS = 0.42
DRIP_JAGGED_WAVES = (3, 5, 8, 13)
DRIP_JAGGED_MAX = 0.025
DRIP_SPECK_NUMBER = 8
DRIP_SPECK_MIN_DISTANCE = 0.55
DRIP_SPECK_MAX_DISTANCE = 0.85
DRIP_SPECK_MIN_RADIUS = 0.02
DRIP_SPECK_MAX_RADIUS = 0.06

EDGE_SOFTNESS = 0.025  # ふちをぼかす幅（ジャギーを抑える。液体なので小さく）
OPACITY = 0.95         # 一番濃い所の不透明度

# 色（リニア。0〜1）。厚い所ほど暗く、薄い縁ほど明るい。BloodSprayList.cpp の kBlood*（0.35, 0.02, 0.02）を薄い縁の色にする。
THIN_COLOR = (0.35, 0.02, 0.02)
THICK_COLOR = (0.12, 0.005, 0.005)
THICKNESS_RAMP = 0.06  # ふちからこの距離（割合）で一番厚くなる

# 法線：血の厚み（割合）と傾きの強さ。見た目を確かめる前の初期値。
BLOOD_HEIGHT = 0.03
NORMAL_STRENGTH = 2.0

# sRGB の変換（IEC 61966-2-1）。絵は BC7_SRGB で読む（texture_to_dds.ps1 の Color）ので、リニアの色を sRGB の数で書く。
SRGB_LINEAR_LIMIT = 0.0031308
SRGB_LINEAR_SCALE = 12.92
SRGB_GAMMA = 2.4
SRGB_SCALE = 1.055
SRGB_OFFSET = 0.055


def linear_to_srgb(value):
    if value <= SRGB_LINEAR_LIMIT:
        return value * SRGB_LINEAR_SCALE
    return SRGB_SCALE * math.pow(value, 1.0 / SRGB_GAMMA) - SRGB_OFFSET


def make_waves(rng, counts, depth_max):
    """ふちを波打たせる波（一周の波の数, 深さ, 位相）の一覧。1 つの波だと星形に見えるので、数の違う波を足す。"""
    return [(count, rng.uniform(0.0, depth_max), rng.uniform(0.0, 2.0 * math.pi)) for count in counts]


def blob_distance(x, y, center_x, center_y, radius, waves):
    """波打つ丸のふちまでの距離（中が正）。"""
    dx, dy = x - center_x, y - center_y
    angle = math.atan2(dy, dx)
    edge = radius + sum(depth * math.sin(angle * count + phase) for count, depth, phase in waves)
    return edge - math.hypot(dx, dy)


def disc_distance(x, y, disc):
    center_x, center_y, radius = disc
    return radius - math.hypot(x - center_x, y - center_y)


def streak_distance(x, y, streak):
    """筋（溜まりから伸びる、先ほど細い線）のふちまでの距離（中が正）。"""
    start_x, start_y, angle, length, width = streak
    dx, dy = math.cos(angle), math.sin(angle)
    along = (x - start_x) * dx + (y - start_y) * dy
    if along <= 0.0 or along >= length:
        return -1.0
    across = abs(-(x - start_x) * dy + (y - start_y) * dx)
    return width * (1.0 - along / length * (1.0 - STREAK_TIP_RADIUS)) - across


def clamp_disc(center_x, center_y, radius):
    """絵の端（CELL_MARGIN）からはみ出さない丸か。"""
    return math.hypot(center_x, center_y) + radius < CELL_MARGIN


def make_splatter(rng):
    """飛び散った血の形：（溜まりの波, 筋の一覧, 丸の一覧）。筋の先には滴の丸を付ける。"""
    waves = make_waves(rng, POOL_JAGGED_WAVES, POOL_JAGGED_MAX)
    streaks = []
    discs = []
    spread = math.radians(STREAK_SPREAD_DEGREE)
    for _ in range(rng.randint(STREAK_MIN_NUMBER, STREAK_MAX_NUMBER)):
        angle = rng.uniform(-spread, spread)
        width = rng.uniform(STREAK_MIN_WIDTH, STREAK_MAX_WIDTH)
        length = rng.uniform(STREAK_MIN_LENGTH, STREAK_MAX_LENGTH)
        tip_radius = width * STREAK_TIP_RADIUS * 1.5
        # 先の滴が絵の端に届くなら、届かない長さまで縮める。
        while length > 0.0:
            tip_x = POOL_CENTER_X + math.cos(angle) * length
            tip_y = math.sin(angle) * length
            if clamp_disc(tip_x, tip_y, tip_radius):
                break
            length -= 0.05
        streaks.append((POOL_CENTER_X, 0.0, angle, length, width))
        discs.append((tip_x, tip_y, tip_radius))
    speck_spread = math.radians(SPECK_SPREAD_DEGREE)
    target = rng.randint(SPECK_MIN_NUMBER, SPECK_MAX_NUMBER)
    while len(discs) < len(streaks) + target:
        angle = rng.uniform(-speck_spread, speck_spread)
        distance = rng.uniform(SPECK_MIN_DISTANCE, SPECK_MAX_DISTANCE)
        disc = (POOL_CENTER_X + math.cos(angle) * distance, math.sin(angle) * distance,
                rng.uniform(SPECK_MIN_RADIUS, SPECK_MAX_RADIUS))
        if clamp_disc(*disc):
            discs.append(disc)
    return waves, streaks, discs


def splatter_distance(x, y, shape):
    waves, streaks, discs = shape
    best = blob_distance(x, y, POOL_CENTER_X, 0.0, POOL_RADIUS, waves)
    for streak in streaks:
        best = max(best, streak_distance(x, y, streak))
    for disc in discs:
        best = max(best, disc_distance(x, y, disc))
    return best


def make_drip(rng):
    """落ちた滴の跡：（丸の波, 周りの小さな点の一覧）。"""
    waves = make_waves(rng, DRIP_JAGGED_WAVES, DRIP_JAGGED_MAX)
    discs = []
    while len(discs) < DRIP_SPECK_NUMBER:
        angle = rng.uniform(0.0, 2.0 * math.pi)
        distance = rng.uniform(DRIP_SPECK_MIN_DISTANCE, DRIP_SPECK_MAX_DISTANCE)
        disc = (math.cos(angle) * distance, math.sin(angle) * distance, rng.uniform(DRIP_SPECK_MIN_RADIUS, DRIP_SPECK_MAX_RADIUS))
        if clamp_disc(*disc):
            discs.append(disc)
    return waves, discs


def drip_distance(x, y, shape):
    waves, discs = shape
    best = blob_distance(x, y, 0.0, 0.0, DRIP_RADIUS, waves)
    for disc in discs:
        best = max(best, disc_distance(x, y, disc))
    return best


def pixel(distance):
    """ふちまでの距離から (色 r,g,b のリニア, 不透明度, 厚み 0〜1)。"""
    opacity = smooth_step(-EDGE_SOFTNESS, 0.0, distance) * OPACITY
    thickness = smooth_step(0.0, THICKNESS_RAMP, distance)
    color = tuple(thin + (thick - thin) * thickness for thin, thick in zip(THIN_COLOR, THICK_COLOR))
    return color, opacity, thickness


def normal_body(heights):
    """高さの表（行は上から）から、法線の絵（B,G,R,A の並び）を作る（bullet_hole_gen.py と同じ）。"""
    body = bytearray()
    step = 2.0 / CELL_SIZE  # 1 ピクセルの幅（1 種類の絵の半分を 1 とした割合）
    for row in range(SIZE):
        for column in range(SIZE):
            left = heights[row][max(column - 1, 0)]
            right = heights[row][min(column + 1, SIZE - 1)]
            above = heights[max(row - 1, 0)][column]
            below = heights[min(row + 1, SIZE - 1)][column]
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
    # 絵の並び：左上・右上・左下が飛び散った血、右下が滴の跡（BloodStainList.cpp の kDripCell と合わせる）。
    shapes = [(splatter_distance, make_splatter(rng)) for _ in range(SPLATTER_CELLS)]
    shapes.append((drip_distance, make_drip(rng)))
    half = CELL_SIZE / 2.0
    body = bytearray()
    heights = []
    for row in range(SIZE):
        height_row = []
        for column in range(SIZE):
            cell = (row // CELL_SIZE) * CELLS_PER_ROW + column // CELL_SIZE
            distance_of, shape = shapes[cell]
            # ピクセルの中心を、1 種類の絵の中心が原点・半分が 1 の座標へ直す（y は絵の上が正）。
            x = (column % CELL_SIZE + 0.5 - half) / half
            y = (half - (row % CELL_SIZE + 0.5)) / half
            color, opacity, thickness = pixel(distance_of(x, y, shape))
            r, g, b = (to_byte(linear_to_srgb(channel)) for channel in color)
            body += bytes((b, g, r, to_byte(opacity)))
            height_row.append(BLOOD_HEIGHT * thickness)
        heights.append(height_row)
    write_tga(output, SIZE, body)
    print("wrote", output)
    write_tga(normal_output, SIZE, normal_body(heights))
    print("wrote", normal_output)


if __name__ == "__main__":
    main()
