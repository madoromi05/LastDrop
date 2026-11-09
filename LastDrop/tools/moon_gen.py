# -*- coding: utf-8 -*-
# =====================================================================
# moon_gen.py
#   夜空に描く満月の模様（1 枚）を作る。空のシェーダ（LightList.hlsli の skyColor）が、月の向きの円板の中で読む
#   （SummaryPlan/個別仕様/グラフィック/MoonStarsLut_Plan.txt 4 章・Q2）。
#   色（灰色）は月の面の明るさ、アルファは月の形（丸。縁は 1 画素ぶんなめらか）。
#   暗い大きな斑（海）は粗いノイズのしきい値、小さなクレーターは「暗い底＋明るい縁」の丸を散らして作る。
#   丸の外の色は月の平均の明るさにしておく（ミップで縮めたとき、縁に外の黒がにじまないように）。
#   乱数の種を固定しているので、何度作っても同じ絵になる。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\4.5\python\bin\python.exe" tools/moon_gen.py [出力.tga]
#   出力を省くと Resource/Texture/Map/moon.tga に書く。その後 tools/texture_to_dds.ps1 moon で DDS（BC7_SRGB）にする。
# =====================================================================

import sys

import numpy as np

from tga_image import write_tga

DEFAULT_OUTPUT = "Resource/Texture/Map/moon.tga"

SIZE = 512    # 2 の累乗（ミップを全段作れるように）
SEED = 2610
EDGE_PIXELS = 1.0  # 丸の縁をなめらかにする幅（画素）

# 月の面の明るさ（画像の色。sRGB の数）。高地は明るく、海は暗い。ゲームでは moonColor × moonDiscIntensity を掛ける
HIGHLAND_SHADE = 0.86
MARE_SHADE = 0.52
# 海の形：粗いノイズ（1 辺に並ぶ格子の数, 重み）を重ね、MARE_COVERAGE の割合の所を海にする（境目は MARE_SOFTNESS でぼかす）
MARE_OCTAVES = ((3, 1.0), (6, 0.6), (12, 0.35), (24, 0.2), (48, 0.1))
MARE_COVERAGE = 0.32
MARE_SOFTNESS = 0.04
# 面のざらつき（細かいノイズ）の強さ
GRAIN_OCTAVES = ((16, 1.0), (32, 0.7), (64, 0.5), (128, 0.3))
GRAIN_STRENGTH = 0.07
# クレーター：数・半径（月の半径に対する割合の範囲）・底の暗さ・縁の明るさ。
# 満月は光が正面から当たり凹凸の影がほとんど無いので、どちらも弱く（強くすると輪が並んだ絵になった。2026-10-31 に絵で確認）
CRATER_COUNT = 70
CRATER_RADIUS_MIN = 0.012
CRATER_RADIUS_MAX = 0.05
CRATER_FLOOR_DARKEN = 0.04
CRATER_RIM_BRIGHTEN = 0.05
CRATER_RIM_WIDTH = 0.25  # 縁の幅（クレーターの半径に対する割合）
# 縁に近いほど少し暗くする量（周縁減光。満月は弱い）
LIMB_DARKEN = 0.12


def value_noise(rng, cells):
    """cells×cells の格子に乱数を置き、SIZE×SIZE へなめらかに（3 次のつなぎで）引き伸ばす。"""
    lattice = rng.random((cells + 1, cells + 1), dtype=np.float32)
    coords = np.arange(SIZE, dtype=np.float32) * cells / SIZE
    i0 = np.floor(coords).astype(np.int32)
    t = coords - i0
    t = t * t * (3.0 - 2.0 * t)
    i1 = i0 + 1
    rows = lattice[i0, :] * (1.0 - t)[:, None] + lattice[i1, :] * t[:, None]
    return rows[:, i0] * (1.0 - t)[None, :] + rows[:, i1] * t[None, :]


def layered_noise(rng, octaves):
    noise = np.zeros((SIZE, SIZE), dtype=np.float32)
    total = 0.0
    for cells, weight in octaves:
        noise += value_noise(rng, cells) * weight
        total += weight
    return noise / total


def smooth_step(edge0, edge1, x):
    t = np.clip((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def main():
    output = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUTPUT
    rng = np.random.default_rng(SEED)

    # 画素の中心の位置（月の中心 0・月の縁 1）
    radius_pixels = SIZE * 0.5 - EDGE_PIXELS * 2.0
    axis = (np.arange(SIZE, dtype=np.float32) + 0.5 - SIZE * 0.5) / radius_pixels
    x, y = np.meshgrid(axis, axis)
    distance = np.sqrt(x * x + y * y)

    # 海：粗いノイズの低い所（MARE_COVERAGE の割合）
    mare_noise = layered_noise(rng, MARE_OCTAVES)
    threshold = np.quantile(mare_noise, MARE_COVERAGE)
    mare = 1.0 - smooth_step(threshold - MARE_SOFTNESS, threshold + MARE_SOFTNESS, mare_noise)
    shade = HIGHLAND_SHADE + (MARE_SHADE - HIGHLAND_SHADE) * mare

    # ざらつき（−0.5〜0.5 に寄せてから掛ける）
    shade += (layered_noise(rng, GRAIN_OCTAVES) - 0.5) * GRAIN_STRENGTH * 2.0

    # クレーター。小さい物ほど多く（半径を乱数の 2 乗で小さい側へ寄せる）
    for _ in range(CRATER_COUNT):
        r = CRATER_RADIUS_MIN + (CRATER_RADIUS_MAX - CRATER_RADIUS_MIN) * rng.random() ** 2
        angle = rng.random() * 2.0 * np.pi
        reach = np.sqrt(rng.random()) * (1.0 - r)
        cx, cy = reach * np.cos(angle), reach * np.sin(angle)
        d = np.sqrt((x - cx) ** 2 + (y - cy) ** 2) / r
        floor = 1.0 - smooth_step(0.6, 1.0 - CRATER_RIM_WIDTH * 0.5, d)
        rim = smooth_step(1.0 - CRATER_RIM_WIDTH, 1.0, d) * (1.0 - smooth_step(1.0, 1.0 + CRATER_RIM_WIDTH, d))
        shade += -CRATER_FLOOR_DARKEN * floor + CRATER_RIM_BRIGHTEN * rim

    # 周縁減光（縁ほど少し暗い）
    shade *= 1.0 - LIMB_DARKEN * np.clip(distance, 0.0, 1.0) ** 4
    shade = np.clip(shade, 0.0, 1.0)

    # 形（アルファ）。丸の外の色は月の中の平均にする
    alpha = 1.0 - smooth_step(1.0 - EDGE_PIXELS / radius_pixels, 1.0 + EDGE_PIXELS / radius_pixels, distance)
    inside = distance < 1.0
    shade = np.where(inside, shade, shade[inside].mean())

    gray = np.clip(np.round(shade * 255.0), 0, 255).astype(np.uint8)
    body = np.empty((SIZE, SIZE, 4), dtype=np.uint8)
    body[:, :, 0] = gray  # B
    body[:, :, 1] = gray  # G
    body[:, :, 2] = gray  # R
    body[:, :, 3] = np.clip(np.round(alpha * 255.0), 0, 255).astype(np.uint8)
    write_tga(output, SIZE, body.tobytes())
    print("WROTE", output)


if __name__ == "__main__":
    main()
