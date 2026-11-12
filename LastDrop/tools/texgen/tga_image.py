# -*- coding: utf-8 -*-
# =====================================================================
# tga_image.py
#   絵を作るツール（bullet_hole_gen.py・dust_gen.py）が共通で使う部品。
#   (灰色, 不透明度) のピクセルの重ね方と、32 ビット TGA の書き出し。
#   GameLib の TGA 読み込み（TextureImpl::readTGA）が読める形（圧縮なし・アルファ付き・上の行から）で書く。
# =====================================================================

import struct

# TGA の見出し。形式 2 = 色そのまま・圧縮なし。0x28 = アルファ 8 ビット＋上の行から並ぶ。
TGA_UNCOMPRESSED_TRUE_COLOR = 2
TGA_BITS_PER_PIXEL = 32
TGA_DESCRIPTOR = 0x28
BYTE_MAX = 255


def smooth_step(edge0, edge1, x):
    """edge0 で 0、edge1 で 1 になる、なめらかな切り替え。"""
    t = min(max((x - edge0) / (edge1 - edge0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def over(top, bottom):
    """(灰色, 不透明度) の top を bottom の上に重ねる。"""
    top_shade, top_opacity = top
    bottom_shade, bottom_opacity = bottom
    opacity = top_opacity + bottom_opacity * (1.0 - top_opacity)
    if opacity <= 0.0:
        return 0.0, 0.0
    shade = (top_shade * top_opacity + bottom_shade * bottom_opacity * (1.0 - top_opacity)) / opacity
    return shade, opacity


def to_byte(value):
    return int(round(min(max(value, 0.0), 1.0) * BYTE_MAX))


def write_tga(path, size, body):
    """size×size の TGA を書く。body: 上の行から B,G,R,A の並び。"""
    write_tga_size(path, size, size, body)


def write_tga_size(path, width, height, body):
    """width×height の TGA を書く（UI の背景のように横長の絵）。body: 上の行から B,G,R,A の並び。"""
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, TGA_UNCOMPRESSED_TRUE_COLOR, 0, 0, 0, 0, 0,
                         width, height, TGA_BITS_PER_PIXEL, TGA_DESCRIPTOR)
    with open(path, "wb") as f:
        f.write(header)
        f.write(body)
