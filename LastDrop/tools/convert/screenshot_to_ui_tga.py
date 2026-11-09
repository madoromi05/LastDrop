# =====================================================================
# screenshot_to_ui_tga.py
#   ゲームの画面写真（PNG・JPG・WebP）を、UI の背景に貼る TGA にする（タイトル画面の背景。TitleScreen_Plan.txt）。
#   GameLib の TGA の読み込みは幅と高さを 2 の累乗へ切り上げて余白を作るので、余白が出ないよう 2 の累乗の大きさ（既定 2048×1024）にする。
#   縦横の比が違うときは、真ん中を残して上下（か左右）を切ってから縮める（絵をつぶさない）。
#   画面には UiBackdrop が縦を合わせて貼り、はみ出た横は画面の外になる。
#
# 使い方（Blender の中で走らせる。Blender は WebP を読めて、TGA も書ける。PIL が要らない）:
#   blender -b --factory-startup --python tools/convert/screenshot_to_ui_tga.py -- <入力の画像> <出力の TGA> [幅] [高さ]
# =====================================================================
import sys

import bpy
import numpy as np

DEFAULT_WIDTH = 2048
DEFAULT_HEIGHT = 1024
CHANNELS = 4  # Blender の画素は RGBA の float


def parse_args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if len(argv) < 2:
        raise SystemExit("usage: -- <input image> <output tga> [width] [height]")
    width = int(argv[2]) if len(argv) > 2 else DEFAULT_WIDTH
    height = int(argv[3]) if len(argv) > 3 else DEFAULT_HEIGHT
    return argv[0], argv[1], width, height


def crop_to_aspect(pixels, width, height, aspect):
    """真ん中を残して aspect（幅/高さ）に切る。pixels は (高さ, 幅, 4)。"""
    if width / height > aspect:
        keep = int(round(height * aspect))
        left = (width - keep) // 2
        return pixels[:, left:left + keep, :]
    keep = int(round(width / aspect))
    top = (height - keep) // 2
    return pixels[top:top + keep, :, :]


def main():
    source_path, output_path, out_width, out_height = parse_args()
    source = bpy.data.images.load(source_path)
    width, height = source.size
    pixels = np.array(source.pixels[:], dtype=np.float32).reshape(height, width, CHANNELS)
    cropped = crop_to_aspect(pixels, width, height, out_width / out_height)
    crop_height, crop_width = cropped.shape[0], cropped.shape[1]

    result = bpy.data.images.new("ui_backdrop", crop_width, crop_height, alpha=True)
    result.colorspace_settings.name = source.colorspace_settings.name  # 色をそのまま（変換しない）
    result.pixels = cropped.ravel()
    result.scale(out_width, out_height)
    result.filepath_raw = output_path
    result.file_format = "TARGA_RAW"
    result.save()
    print(f"wrote {output_path} ({out_width}x{out_height}) from {source_path} ({width}x{height})")


main()
