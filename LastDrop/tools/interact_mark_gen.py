# -*- coding: utf-8 -*-
# =====================================================================
# interact_mark_gen.py
#   使える物の上に出すマーク（白いひし形 1 枚）を作る（SummaryPlan/個別仕様/InteractMarker_Plan.txt Q2）。
#   白く塗ったひし形の外側に、暗い縁を付ける（明るい壁や空の前でも形が見えるように）。縁はどちらも 1 画素ぶんなめらか。
#   ゲームは絵に色を掛けて使う（使えない物は灰色）。見ている物は大きく描き、真ん中に「E」の文字を重ねる（InteractMarkers）。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\4.5\python\bin\python.exe" tools/interact_mark_gen.py [出力.tga]
#   出力を省くと Resource/UI/ui_interact_mark.tga に書く。UI の絵なので DDS にはせず、ゲームは LINEAR で読む。
# =====================================================================

import sys

from tga_image import over, smooth_step, to_byte, write_tga

DEFAULT_OUTPUT = "Resource/UI/ui_interact_mark.tga"

SIZE = 64            # 絵の幅・高さ（画素）
FILL_RADIUS = 0.72   # 白いひし形の大きさ（中心から頂点まで。絵の幅の半分を 1 とする）
OUTLINE_RADIUS = 0.92  # 暗い縁の外側
EDGE = 1.5 / ( SIZE * 0.5 )  # なめらかにする幅（1.5 画素）
FILL_SHADE = 1.0
OUTLINE_SHADE = 0.08
OUTLINE_OPACITY = 0.85


def pixel( x, y ):
    # ひし形は「中心からの |x| + |y|」が一定の線。絵の中心を 0、端を 1 にして測る。
    u = ( x + 0.5 ) / ( SIZE * 0.5 ) - 1.0
    v = ( y + 0.5 ) / ( SIZE * 0.5 ) - 1.0
    distance = abs( u ) + abs( v )
    fill = 1.0 - smooth_step( FILL_RADIUS - EDGE, FILL_RADIUS + EDGE, distance )
    outline = ( 1.0 - smooth_step( OUTLINE_RADIUS - EDGE, OUTLINE_RADIUS + EDGE, distance ) ) * OUTLINE_OPACITY
    return over( ( FILL_SHADE, fill ), ( OUTLINE_SHADE, outline ) )


def main():
    output = sys.argv[ 1 ] if len( sys.argv ) > 1 else DEFAULT_OUTPUT
    body = bytearray()
    for y in range( SIZE ):
        for x in range( SIZE ):
            shade, opacity = pixel( x, y )
            gray = to_byte( shade )
            body += bytes( ( gray, gray, gray, to_byte( opacity ) ) )
    write_tga( output, SIZE, body )
    print( "interact_mark_gen: wrote " + output )


main()
