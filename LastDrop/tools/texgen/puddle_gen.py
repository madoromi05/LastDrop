# -*- coding: utf-8 -*-
# =====================================================================
# puddle_gen.py
#   雨上がりの水たまりの模様（白黒 1 枚）を作る。道路の G-Buffer のシェーダが、ワールドの位置（x・z）で繰り返し読む
#   （SummaryPlan/個別仕様/グラフィック/SsrDecalMaterial_Plan.txt 6 章）。道路のタイルの画像に焼かないのは、タイルごとに同じ形が並ぶため。
#   いくつかの大きさのノイズ（値ノイズ）を重ね、白いほど「水たまりになりやすい」値にする。
#   値は「その値より小さい画素の割合」に並べ直してある（ヒストグラムの平坦化）。なので、ゲームの puddleCoverage（0〜1）が
#   そのまま「水たまりの面積の割合」になる（しきい値 1 − coverage より白い所が水たまり）。
#   画像の端どうしがつながる（繰り返して貼っても継ぎ目が出ない）ように、ノイズの格子は画像の大きさで折り返す。
#   乱数の種を固定しているので、何度作っても同じ絵になる。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\4.5\python\bin\python.exe" tools/texgen/puddle_gen.py [出力.tga]
#   出力を省くと Resource/Texture/Map/puddle_mask.tga に書く。その後 tools/convert/texture_to_dds.ps1 puddle で DDS にする。
# =====================================================================

import sys

import numpy as np

from tga_image import write_tga

DEFAULT_OUTPUT = "Resource/Texture/Map/puddle_mask.tga"

SIZE = 1024   # 2 の累乗（ミップを全段作れるように）
SEED = 7701

# 重ねるノイズ（1 辺に並ぶ格子の数, 重み）。粗い格子が水たまりの大きな形、細かい格子が縁のでこぼこ。
# ゲームでは画像 1 枚を 16m に貼る（kPuddleTileSize）ので、格子 12 個 = 約 1.3m 間隔の形（水たまり 1 つが 0.5〜2m ほど）。
# 格子 4 個から始めると 1 つが 6m ほどの池になった（2026-10-30 に絵で確認）。
OCTAVES = ((12, 1.0), (24, 0.5), (48, 0.25), (96, 0.125), (192, 0.0625))


def periodic_value_noise(rng, cells):
    """cells×cells の格子に乱数を置き、SIZE×SIZE へなめらかに（3 次のつなぎで）引き伸ばす。端は反対側の端とつながる。"""
    lattice = rng.random((cells, cells), dtype=np.float32)
    coords = np.arange(SIZE, dtype=np.float32) * cells / SIZE
    i0 = np.floor(coords).astype(np.int32)
    t = coords - i0
    t = t * t * (3.0 - 2.0 * t)
    i1 = (i0 + 1) % cells
    i0 = i0 % cells
    # 縦（行）と横（列）の 2 回に分けて混ぜる。
    rows = lattice[i0, :] * (1.0 - t)[:, None] + lattice[i1, :] * t[:, None]
    return rows[:, i0] * (1.0 - t)[None, :] + rows[:, i1] * t[None, :]


def equalize(values):
    """値を「それより小さい画素の割合」（0〜1）に並べ直す。"""
    flat = values.ravel()
    ranks = np.empty(flat.size, dtype=np.float32)
    ranks[np.argsort(flat, kind="stable")] = np.arange(flat.size, dtype=np.float32) / (flat.size - 1)
    return ranks.reshape(values.shape)


def main():
    output = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUTPUT
    rng = np.random.default_rng(SEED)
    noise = np.zeros((SIZE, SIZE), dtype=np.float32)
    for cells, weight in OCTAVES:
        noise += periodic_value_noise(rng, cells) * weight
    mask = equalize(noise)
    gray = np.clip(np.round(mask * 255.0), 0, 255).astype(np.uint8)
    body = np.empty((SIZE, SIZE, 4), dtype=np.uint8)
    body[:, :, 0] = gray  # B
    body[:, :, 1] = gray  # G
    body[:, :, 2] = gray  # R
    body[:, :, 3] = 255   # A（使わない）
    write_tga(output, SIZE, body.tobytes())
    print("WROTE", output)


if __name__ == "__main__":
    main()
