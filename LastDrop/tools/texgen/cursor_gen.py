# -*- coding: utf-8 -*-
# =====================================================================
# cursor_gen.py
#   ゲームの窓の中のマウスカーソル（黒い細い矢印に白い縁）を .cur に書く（SummaryPlan/個別仕様/UI/MenuCursor_Plan.txt）
#   32・48・64 画素の 3 枚を 1 つのファイルに入れ、Windows が画面の拡大率に合う物を選ぶ
#   縁は 1 画素を 4×4 に分けて数え、なめらかにする
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\4.5\python\bin\python.exe" tools/texgen/cursor_gen.py [出力.cur]
#   出力を省くと Resource/UI/cursor.cur に書く。ゲームは WindowCreator::Configuration::setCursorFile で読む
# =====================================================================

import math
import struct
import sys

DEFAULT_OUTPUT = "Resource/UI/cursor.cur"

DESIGN_SIZE = 32.0                  # 形の座標を決めた絵の幅（この上で左上が 0,0）
SIZES = ( 32, 48, 64 )              # 書く絵の幅・高さ（画素）
ARROW = (                           # 矢印の輪郭。左上の先から反時計回り
    ( 2.0, 2.0 ), ( 2.0, 23.0 ), ( 7.5, 18.0 ), ( 11.0, 26.0 ),
    ( 13.6, 24.8 ), ( 10.2, 17.0 ), ( 17.0, 16.8 ),
)
HOTSPOT = ( 2.0, 2.0 )              # 指す点。矢印の先
OUTLINE_WIDTH = 1.4                 # 縁の太さ（DESIGN_SIZE の上で）
FILL_COLOR = ( 0x05, 0x05, 0x05 )   # 中の黒
OUTLINE_COLOR = ( 0xe8, 0xe4, 0xdc )  # 縁の骨色。メニューの文字と同じ色
SAMPLES_PER_SIDE = 4                # 1 画素を何×何に分けて数えるか

CUR_TYPE = 2                        # ICONDIR の種類。1 がアイコン、2 がカーソル
DIR_HEADER_SIZE = 6
DIR_ENTRY_SIZE = 16
BITMAP_HEADER_SIZE = 40
BITS_PER_PIXEL = 32
MASK_ROW_ALIGN = 4                  # AND マスクの 1 行はこのバイト数の倍数にそろえる
BYTE_MAX = 255


def inside( px, py ):
    """点が矢印の中か（横へ伸ばした線が輪郭を何回横切るかで調べる）"""
    crossings = 0
    count = len( ARROW )
    for i in range( count ):
        ax, ay = ARROW[ i ]
        bx, by = ARROW[ ( i + 1 ) % count ]
        if ( ay > py ) != ( by > py ):
            x = ax + ( py - ay ) * ( bx - ax ) / ( by - ay )
            if px < x:
                crossings += 1
    return crossings % 2 == 1


def distance_to_outline( px, py ):
    """点から輪郭の線までのいちばん近い距離"""
    nearest = math.inf
    count = len( ARROW )
    for i in range( count ):
        ax, ay = ARROW[ i ]
        bx, by = ARROW[ ( i + 1 ) % count ]
        dx, dy = bx - ax, by - ay
        t = ( ( px - ax ) * dx + ( py - ay ) * dy ) / ( dx * dx + dy * dy )
        t = min( max( t, 0.0 ), 1.0 )
        nearest = min( nearest, math.hypot( px - ( ax + dx * t ), py - ( ay + dy * t ) ) )
    return nearest


def sample_color( px, py ):
    """DESIGN_SIZE の上の 1 点の色。外なら None"""
    if distance_to_outline( px, py ) <= OUTLINE_WIDTH * 0.5:
        return OUTLINE_COLOR
    if inside( px, py ):
        return FILL_COLOR
    return None


def pixel( x, y, scale ):
    """size の絵の 1 画素の (R, G, B, A)。色は不透明度で重みを付けた平均"""
    total = SAMPLES_PER_SIDE * SAMPLES_PER_SIDE
    sums = [ 0, 0, 0 ]
    hits = 0
    for sy in range( SAMPLES_PER_SIDE ):
        for sx in range( SAMPLES_PER_SIDE ):
            px = ( x + ( sx + 0.5 ) / SAMPLES_PER_SIDE ) / scale
            py = ( y + ( sy + 0.5 ) / SAMPLES_PER_SIDE ) / scale
            color = sample_color( px, py )
            if color is None:
                continue
            hits += 1
            for c in range( 3 ):
                sums[ c ] += color[ c ]
    if hits == 0:
        return 0, 0, 0, 0
    r, g, b = ( int( round( s / hits ) ) for s in sums )
    return r, g, b, int( round( hits * BYTE_MAX / total ) )


def image_bytes( size ):
    """1 枚ぶんの BITMAPINFOHEADER＋色（BGRA・下の行から）＋AND マスク"""
    scale = size / DESIGN_SIZE
    rows = [ [ pixel( x, y, scale ) for x in range( size ) ] for y in range( size ) ]

    header = struct.pack( "<IiiHHIIiiII", BITMAP_HEADER_SIZE, size, size * 2, 1, BITS_PER_PIXEL,
                          0, 0, 0, 0, 0, 0 )   # 高さは色と AND マスクの 2 枚ぶん
    color = bytearray()
    for row in reversed( rows ):
        for r, g, b, a in row:
            color += bytes( ( b, g, r, a ) )

    mask_row_bytes = ( ( size + 7 ) // 8 + MASK_ROW_ALIGN - 1 ) // MASK_ROW_ALIGN * MASK_ROW_ALIGN
    mask = bytearray()
    for row in reversed( rows ):
        line = bytearray( mask_row_bytes )
        for x, ( _, _, _, a ) in enumerate( row ):
            if a == 0:
                line[ x // 8 ] |= 0x80 >> ( x % 8 )   # 1 は透明
        mask += line
    return header + color + mask


def main():
    output = sys.argv[ 1 ] if len( sys.argv ) > 1 else DEFAULT_OUTPUT
    images = [ image_bytes( size ) for size in SIZES ]

    data = bytearray( struct.pack( "<HHH", 0, CUR_TYPE, len( SIZES ) ) )
    offset = DIR_HEADER_SIZE + DIR_ENTRY_SIZE * len( SIZES )
    for size, image in zip( SIZES, images ):
        scale = size / DESIGN_SIZE
        hot_x = int( round( HOTSPOT[ 0 ] * scale ) )
        hot_y = int( round( HOTSPOT[ 1 ] * scale ) )
        data += struct.pack( "<BBBBHHII", size, size, 0, 0, hot_x, hot_y, len( image ), offset )
        offset += len( image )
    for image in images:
        data += image

    with open( output, "wb" ) as f:
        f.write( data )
    print( "cursor_gen: wrote " + output )


main()
