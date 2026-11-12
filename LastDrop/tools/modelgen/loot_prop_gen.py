# -*- coding: utf-8 -*-
# =====================================================================
# loot_prop_gen.py
#   段階R3（SummaryPlan/個別仕様/出撃と帰還/R3_Plan.txt Q2）の形を、Blender の箱と円柱を組み合わせて作る。
#   手元にゴミ箱・木箱・包帯の 3D モデルが無いため（ダウンロードのフォルダも調べた）。
#     ゴミ箱（dumpster）・木箱（crate）     … 町の漁れる場所（人 1.79m に合わせた実寸）
#     包帯（bandage） … 地面に落ちた形と、カバンの絵（item_icon_gen.py）の元
#   どの形も 1 枚の色の画像（4 × 4 のます）を分け合う。形の部品ごとに、どのますの色を貼るか（PATCHES の番号）を決め、
#   部品の面をそのますの中へ平らに写す（色は無地＋細かいむら・木目・板の線などの模様だけなので、写し方の向きは問わない）。
#   材質の画像（ORM：r AO・g 粗さ・b 金属）も、同じますの並びで書く。
#
#   書き出す物：
#     tools/generated/loot_props/<名前>.fbx   … fbx_to_gamelib.py（tools/configs/<名前>.json）と item_icon_gen.py が読む
#     Resource/Texture/Prop/loot_props_albedo.tga・Resource/Texture/Prop/loot_props_orm.tga … 色と材質（その後 tools/convert/texture_to_dds.ps1 loot_props で DDS へ）
#   向き：Blender の -Y が物の正面（ゲームの +Z。fbx_to_gamelib の座標変換 (x, y, z) → (x, z, -y)）。長い辺は X に沿わせる。
#   乱数の種を固定しているので、何度作っても同じ形と絵になる。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   "C:\Program Files\Blender Foundation\Blender 4.5\blender.exe" -b --factory-startup --python tools/modelgen/loot_prop_gen.py
# =====================================================================

import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

OUTPUT_FBX_DIR = "tools/generated/loot_props"
OUTPUT_ALBEDO = "Resource/Texture/Prop/loot_props_albedo.tga"
OUTPUT_ORM = "Resource/Texture/Prop/loot_props_orm.tga"

ATLAS_SIZE = 512       # 2 の累乗（ミップを全段作れるように）
ATLAS_CELLS = 4        # 1 辺のますの数（4 × 4 = 16 色）
PATCH_INSET = 0.08     # ますの縁を空ける割合（ミップで隣のますの色がにじまないように）
SEED = 3105
CYLINDER_SEGMENTS = 20

# ── ますの色（PATCHES の番号 = ますの番号。左下から右へ、下の段から上の段へ）──
# color: sRGB の数。noise: 明るさのむらの強さ。roughness・metallic: ORM。pattern: 模様（None・"planks"・"grain"・"stripes"）。
PATCHES = [
    { "name": "paint_green",   "color": (0.13, 0.30, 0.17), "noise": 0.06, "roughness": 0.60, "metallic": 0.0, "pattern": "grime" },
    { "name": "plastic_black", "color": (0.05, 0.05, 0.05), "noise": 0.03, "roughness": 0.70, "metallic": 0.0, "pattern": None },
    { "name": "metal_grey",    "color": (0.46, 0.47, 0.48), "noise": 0.05, "roughness": 0.45, "metallic": 1.0, "pattern": None },
    # 車輪はふたと同じ黒（plastic_black）。このますは包帯の赤い帯（手当ての物だと見て分かるように。2026-11-05 に絵で見て足した）。
    { "name": "medical_red",   "color": (0.70, 0.08, 0.07), "noise": 0.03, "roughness": 0.90, "metallic": 0.0, "pattern": "weave" },
    { "name": "label_white",   "color": (0.83, 0.83, 0.78), "noise": 0.04, "roughness": 0.60, "metallic": 0.0, "pattern": "stripes" },
    { "name": "wood_olive",    "color": (0.31, 0.33, 0.18), "noise": 0.05, "roughness": 0.85, "metallic": 0.0, "pattern": "planks" },
    { "name": "wood_dark",     "color": (0.22, 0.23, 0.13), "noise": 0.05, "roughness": 0.85, "metallic": 0.0, "pattern": "grain" },
    { "name": "stencil",       "color": (0.31, 0.33, 0.18), "noise": 0.04, "roughness": 0.80, "metallic": 0.0, "pattern": "stencil" },
    { "name": "cloth_cream",   "color": (0.88, 0.85, 0.76), "noise": 0.05, "roughness": 0.95, "metallic": 0.0, "pattern": "weave" },
    { "name": "cloth_shadow",  "color": (0.62, 0.58, 0.50), "noise": 0.04, "roughness": 0.95, "metallic": 0.0, "pattern": "weave" },
]
PATCH = { patch[ "name" ]: index for index, patch in enumerate( PATCHES ) }

# ── 形（部品の並び）。box: (大きさ x, y, z), (中心 x, y, z)。cylinder: 半径, 長さ, (中心), 軸 "X"/"Y"/"Z"。単位は m ──
# ゴミ箱：本体 1.6 × 0.95 × 1.0 を車輪の上に載せ、黒いふた・正面の縁・横の取っ手・車輪・白い札。高さ約 1.2m。
DUMPSTER = [
    ( "box", ( 1.60, 0.95, 1.00 ), ( 0.0, 0.0, 0.66 ), "paint_green" ),
    ( "box", ( 1.66, 1.01, 0.06 ), ( 0.0, 0.02, 1.19 ), "plastic_black" ),
    ( "box", ( 1.64, 0.05, 0.08 ), ( 0.0, -0.50, 1.10 ), "paint_green" ),
    ( "box", ( 1.50, 0.85, 0.06 ), ( 0.0, 0.0, 0.17 ), "metal_grey" ),
    ( "box", ( 0.04, 0.50, 0.04 ), ( 0.84, 0.0, 0.90 ), "metal_grey" ),
    ( "box", ( 0.04, 0.50, 0.04 ), ( -0.84, 0.0, 0.90 ), "metal_grey" ),
    ( "box", ( 0.50, 0.01, 0.25 ), ( 0.0, -0.48, 0.80 ), "label_white" ),
    ( "cylinder", 0.08, 0.06, ( 0.65, -0.35, 0.08 ), "X", "plastic_black" ),
    ( "cylinder", 0.08, 0.06, ( -0.65, -0.35, 0.08 ), "X", "plastic_black" ),
    ( "cylinder", 0.08, 0.06, ( 0.65, 0.35, 0.08 ), "X", "plastic_black" ),
    ( "cylinder", 0.08, 0.06, ( -0.65, 0.35, 0.08 ), "X", "plastic_black" ),
]
# 木箱（軍の弾薬箱の形）：1.0 × 0.6 × 0.6。ふたの縁・角の金具・横の取っ手・正面の文字の板。
CRATE = [
    ( "box", ( 1.00, 0.60, 0.56 ), ( 0.0, 0.0, 0.28 ), "wood_olive" ),
    ( "box", ( 1.02, 0.62, 0.06 ), ( 0.0, 0.0, 0.58 ), "wood_dark" ),
    ( "box", ( 0.06, 0.06, 0.62 ), ( 0.49, -0.29, 0.31 ), "metal_grey" ),
    ( "box", ( 0.06, 0.06, 0.62 ), ( -0.49, -0.29, 0.31 ), "metal_grey" ),
    ( "box", ( 0.06, 0.06, 0.62 ), ( 0.49, 0.29, 0.31 ), "metal_grey" ),
    ( "box", ( 0.06, 0.06, 0.62 ), ( -0.49, 0.29, 0.31 ), "metal_grey" ),
    ( "box", ( 0.04, 0.22, 0.04 ), ( 0.52, 0.0, 0.40 ), "metal_grey" ),
    ( "box", ( 0.04, 0.22, 0.04 ), ( -0.52, 0.0, 0.40 ), "metal_grey" ),
    ( "box", ( 0.44, 0.01, 0.16 ), ( 0.0, -0.305, 0.30 ), "stencil" ),
]
# 包帯：寝かせた巻き（拾いやすいよう実物より少し大きい。落ちた弾と同じ考え）と、芯の暗い穴・ほどけた端。
BANDAGE = [
    ( "cylinder", 0.055, 0.11, ( 0.0, 0.0, 0.055 ), "X", "cloth_cream" ),
    ( "cylinder", 0.018, 0.112, ( 0.0, 0.0, 0.055 ), "X", "cloth_shadow" ),
    ( "cylinder", 0.0565, 0.03, ( 0.0, 0.0, 0.055 ), "X", "medical_red" ),
    ( "box", ( 0.10, 0.08, 0.004 ), ( 0.0, -0.09, 0.002 ), "cloth_cream" ),
]
MODELS = [
    ( "dumpster", DUMPSTER ),
    ( "crate", CRATE ),
    ( "bandage", BANDAGE ),
]


# ------------------------- 色と材質の画像 -------------------------
def value_noise( rng, size, cells ):
    """size × size の、なめらかなむら（cells × cells の格子の乱数をつないだ物。-1〜1）。"""
    grid = rng.uniform( -1.0, 1.0, ( cells + 1, cells + 1 ) )
    t = np.linspace( 0.0, cells, size, endpoint=False )
    i = t.astype( int )
    f = t - i
    f = f * f * ( 3.0 - 2.0 * f )
    a = grid[ np.ix_( i, i ) ]
    b = grid[ np.ix_( i, i + 1 ) ]
    c = grid[ np.ix_( i + 1, i ) ]
    d = grid[ np.ix_( i + 1, i + 1 ) ]
    fy = f[ :, None ]
    fx = f[ None, : ]
    return ( a * ( 1 - fx ) + b * fx ) * ( 1 - fy ) + ( c * ( 1 - fx ) + d * fx ) * fy


def pattern_shade( pattern, size, rng ):
    """模様の明るさの掛け算（1 が元のまま）。行は下から（Blender の画像の並び）。"""
    shade = np.ones( ( size, size ) )
    v = np.linspace( 0.0, 1.0, size, endpoint=False )[ :, None ]
    u = np.linspace( 0.0, 1.0, size, endpoint=False )[ None, : ]
    if pattern == "planks":
        # 板の継ぎ目（横に 4 本）と木目（細かい縦長のむら）。
        seam = np.abs( ( v * 4.0 ) % 1.0 - 0.5 ) > 0.47
        shade = shade - 0.35 * seam + 0.08 * value_noise( rng, size, 24 )[ :, :: 1 ] * ( 1.0 - seam )
    elif pattern == "grain":
        shade = shade + 0.10 * value_noise( rng, size, 32 )
    elif pattern == "stripes":
        # 札の黒い横線 3 本（文字の代わり）。
        lines = ( ( v * 6.0 ) % 1.0 < 0.35 ) & ( v > 0.2 ) & ( v < 0.8 ) & ( u > 0.12 ) & ( u < 0.88 )
        shade = np.where( lines, 0.15, shade )
    elif pattern == "stencil":
        # 黄色っぽい文字の塊（弾薬箱の刷り込み）。色は下で混ぜるので、ここでは印だけ返す。
        blocks = ( ( u * 7.0 ) % 1.0 < 0.65 ) & ( v > 0.35 ) & ( v < 0.65 ) & ( u > 0.08 ) & ( u < 0.92 )
        return np.where( blocks, -1.0, shade )
    elif pattern == "weave":
        shade = shade + 0.05 * np.sign( np.sin( u * 120.0 ) * np.sin( v * 120.0 ) )
    elif pattern == "grime":
        # 下ほど汚れて暗い（地面の泥）・細かいむら。
        shade = shade - 0.15 * np.clip( 0.35 - v, 0.0, 0.35 ) / 0.35 + 0.05 * value_noise( rng, size, 16 )
    return shade


STENCIL_COLOR = np.array( ( 0.78, 0.70, 0.35 ) )


def make_atlas():
    """色（sRGB の数）と ORM の画像（ATLAS_SIZE 四方。行は下から）を作る。"""
    rng = np.random.default_rng( SEED )
    cell = ATLAS_SIZE // ATLAS_CELLS
    albedo = np.zeros( ( ATLAS_SIZE, ATLAS_SIZE, 4 ) )
    orm = np.zeros( ( ATLAS_SIZE, ATLAS_SIZE, 4 ) )
    albedo[ :, :, 3 ] = 1.0
    orm[ :, :, 3 ] = 1.0
    for index, patch in enumerate( PATCHES ):
        column = index % ATLAS_CELLS
        row = index // ATLAS_CELLS
        ys = slice( row * cell, ( row + 1 ) * cell )
        xs = slice( column * cell, ( column + 1 ) * cell )
        base = np.array( patch[ "color" ] )
        shade = pattern_shade( patch[ "pattern" ], cell, rng )
        stencil = shade < 0.0
        shade = np.where( stencil, 1.0, shade ) + patch[ "noise" ] * value_noise( rng, cell, 8 )
        color = base[ None, None, : ] * shade[ :, :, None ]
        color = np.where( stencil[ :, :, None ], STENCIL_COLOR[ None, None, : ], color )
        albedo[ ys, xs, 0:3 ] = np.clip( color, 0.0, 1.0 )
        orm[ ys, xs, 0 ] = 1.0                                # AO（形が小さいので焼かない）
        orm[ ys, xs, 1 ] = patch[ "roughness" ] + 0.05 * value_noise( rng, cell, 8 )
        orm[ ys, xs, 2 ] = patch[ "metallic" ]
    return albedo, np.clip( orm, 0.0, 1.0 )


def save_tga( pixels, path, colorspace ):
    image = bpy.data.images.new( os.path.basename( path ), ATLAS_SIZE, ATLAS_SIZE, alpha=False )
    image.colorspace_settings.name = colorspace
    image.pixels.foreach_set( pixels.astype( np.float32 ).ravel() )
    image.filepath_raw = os.path.abspath( path ) # 相対のままだと Blender の作業フォルダ基準にならない
    image.file_format = "TARGA_RAW"
    image.save()
    bpy.data.images.remove( image )
    print( "TEXTURE", path )


# ------------------------- 形 -------------------------
def patch_rect( index ):
    """ますの番号 → UV の範囲（左下 u, v と一辺）。縁を PATCH_INSET だけ空ける。"""
    step = 1.0 / ATLAS_CELLS
    column = index % ATLAS_CELLS
    row = index // ATLAS_CELLS
    inset = step * PATCH_INSET
    return column * step + inset, row * step + inset, step - 2.0 * inset


def add_part( bm, uv_layer, part ):
    """部品 1 つを bm へ足し、その面を部品のますへ平らに写す。"""
    if part[ 0 ] == "box":
        _, size, center, patch = part
        geom = bmesh.ops.create_cube( bm, size=1.0 )
        verts = geom[ "verts" ]
        bmesh.ops.scale( bm, vec=Vector( size ), verts=verts )
        bmesh.ops.translate( bm, vec=Vector( center ), verts=verts )
    else:
        _, radius, length, center, axis, patch = part
        geom = bmesh.ops.create_cone( bm, cap_ends=True, segments=CYLINDER_SEGMENTS, radius1=radius, radius2=radius, depth=length )
        verts = geom[ "verts" ]
        rotation = { "X": Matrix.Rotation( math.pi / 2, 3, "Y" ), "Y": Matrix.Rotation( math.pi / 2, 3, "X" ), "Z": Matrix.Identity( 3 ) }[ axis ]
        bmesh.ops.rotate( bm, cent=Vector( ( 0, 0, 0 ) ), matrix=rotation, verts=verts )
        bmesh.ops.translate( bm, vec=Vector( center ), verts=verts )
    low = Vector( ( min( v.co[ i ] for v in verts ) for i in range( 3 ) ) )
    high = Vector( ( max( v.co[ i ] for v in verts ) for i in range( 3 ) ) )
    extent = high - low
    u0, v0, side = patch_rect( PATCH[ patch ] )
    faces = { f for v in verts for f in v.link_faces }
    for face in faces:
        normal = face.normal
        axis_index = max( range( 3 ), key=lambda i: abs( normal[ i ] ) )
        a, b = [ i for i in range( 3 ) if i != axis_index ]
        for loop in face.loops:
            co = loop.vert.co
            u = ( co[ a ] - low[ a ] ) / extent[ a ] if extent[ a ] > 0 else 0.5
            v = ( co[ b ] - low[ b ] ) / extent[ b ] if extent[ b ] > 0 else 0.5
            loop[ uv_layer ].uv = ( u0 + u * side, v0 + v * side )


def build_model( name, parts, material ):
    mesh = bpy.data.meshes.new( name )
    bm = bmesh.new()
    uv_layer = bm.loops.layers.uv.new( "UVMap" )
    for part in parts:
        add_part( bm, uv_layer, part )
    bm.normal_update()
    bm.to_mesh( mesh )
    bm.free()
    obj = bpy.data.objects.new( name, mesh )
    obj.data.materials.append( material )
    bpy.context.scene.collection.objects.link( obj )
    return obj


def make_material( albedo_path ):
    # FBX に色の画像をつないでおく（item_icon_gen.py・fbx_to_gamelib.py は設定の画像を使うので、無くても動く。見て確かめる用）。
    material = bpy.data.materials.new( "loot_props" )
    material.use_nodes = True
    bsdf = material.node_tree.nodes.get( "Principled BSDF" )
    texture = material.node_tree.nodes.new( "ShaderNodeTexImage" )
    texture.image = bpy.data.images.load( os.path.abspath( albedo_path ) )
    material.node_tree.links.new( texture.outputs[ "Color" ], bsdf.inputs[ "Base Color" ] )
    return material


def export_fbx( obj, path ):
    bpy.ops.object.select_all( action="DESELECT" )
    obj.select_set( True )
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx( filepath=path, use_selection=True, object_types={ "MESH" }, path_mode="RELATIVE" )
    print( "FBX", path )


def main():
    bpy.ops.wm.read_factory_settings( use_empty=True )
    albedo, orm = make_atlas()
    os.makedirs( os.path.dirname( OUTPUT_ALBEDO ), exist_ok=True )
    save_tga( albedo, OUTPUT_ALBEDO, "sRGB" )
    save_tga( orm, OUTPUT_ORM, "Non-Color" )
    material = make_material( OUTPUT_ALBEDO )
    os.makedirs( OUTPUT_FBX_DIR, exist_ok=True )
    for name, parts in MODELS:
        obj = build_model( name, parts, material )
        export_fbx( obj, os.path.abspath( os.path.join( OUTPUT_FBX_DIR, name + ".fbx" ) ) )
        bpy.data.objects.remove( obj )


main()
