# -*- coding: utf-8 -*-
# =====================================================================
# gameover_backdrop_gen.py
#   ゲームオーバーの画面の背景（1 枚）を作る（SummaryPlan/個別仕様/UI/ResultScreen_Plan.txt）
#   ほぼ黒の地に、画面の端へ向かって暗い赤がにじむ。細かいざらつきを重ねる
#   UiBackdrop が縦を画面に合わせて貼り、はみ出た横は画面の外になる。16:9 の画面で端が一番赤くなるよう、
#   にじみの距離は 16:9 に見える範囲を基準に測る（4:3 の窓では左右が少し切れる）
#   透けない絵（アルファは全部 255）。UI は ToneMap の後にそのまま描くので、色の数はモックの 16 進と同じ
#   乱数の種を固定しているので、何度作っても同じ絵になる
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\4.5\python\bin\python.exe" tools/texgen/gameover_backdrop_gen.py [出力.tga]
#   出力を省くと Resource/UI/gameover_backdrop.tga に書く
# =====================================================================

import sys

import numpy as np

from tga_image import write_tga_size

DEFAULT_OUTPUT = "Resource/UI/gameover_backdrop.tga"

# 幅と高さは 2 の累乗（GameLib は TGA を 2 の累乗へ切り上げて余白を作るため）。タイトルの背景と同じ大きさ
WIDTH = 2048
HEIGHT = 1024
SEED = 1112
VISIBLE_ASPECT = 16.0 / 9.0  # にじみの距離を測る基準の、画面の縦横の比

# 色（0..1。モックの 16 進から）
GROUND = np.array([0x04, 0x03, 0x03]) / 255.0  # 地の色 #040303
BLEED = np.array([0x3A, 0x0B, 0x0A]) / 255.0   # 端ににじむ赤 #3a0b0a

# にじみ。画面の真ん中からの距離（見える範囲の幅と高さを 1 とした楕円の距離）で、FADE_START から外は濃くなる
BLEED_RADIUS = 0.75      # 距離をこの値で割る（モックの放射グラデーションの半径）
BLEED_FADE_START = 0.45  # 割った距離がここまでは赤なし
BLEED_MAX_OPACITY = 0.75 # 割った距離が 1 の所の濃さ

# ざらつき。明るさに足す量（0..GRAIN_AMOUNT の一様な乱数）
GRAIN_AMOUNT = 0.03


def main():
    output = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUTPUT
    rng = np.random.default_rng(SEED)

    # 画面の真ん中からの距離。横は 16:9 に見える範囲の幅で割る
    visible_width = HEIGHT * VISIBLE_ASPECT
    xs = (np.arange(WIDTH) + 0.5 - WIDTH * 0.5) / visible_width
    ys = (np.arange(HEIGHT) + 0.5 - HEIGHT * 0.5) / HEIGHT
    u, v = np.meshgrid(xs, ys)
    distance = np.sqrt(u * u + v * v) / BLEED_RADIUS

    # にじみの濃さ（FADE_START で 0、1 で最大。その外は最大のまま）
    fade = np.clip((distance - BLEED_FADE_START) / (1.0 - BLEED_FADE_START), 0.0, 1.0)
    opacity = fade * BLEED_MAX_OPACITY

    color = GROUND[None, None, :] * (1.0 - opacity[:, :, None]) + BLEED[None, None, :] * opacity[:, :, None]
    color += rng.uniform(0.0, GRAIN_AMOUNT, size=(HEIGHT, WIDTH, 1))
    color = np.clip(color, 0.0, 1.0)

    rgb = np.round(color * 255.0).astype(np.uint8)
    body = np.empty((HEIGHT, WIDTH, 4), dtype=np.uint8)
    body[:, :, 0] = rgb[:, :, 2]  # B
    body[:, :, 1] = rgb[:, :, 1]  # G
    body[:, :, 2] = rgb[:, :, 0]  # R
    body[:, :, 3] = 255           # 透けない
    write_tga_size(output, WIDTH, HEIGHT, body.tobytes())
    print("WROTE", output)


if __name__ == "__main__":
    main()
