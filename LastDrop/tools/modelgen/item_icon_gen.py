# -*- coding: utf-8 -*-
# =====================================================================
# item_icon_gen.py
#   カバンの画面（InventoryScreen）に出す物の絵（アイコン）を、物の 3D モデルを Blender で撮って作る。
#   どの物も同じ向きのカメラ・同じライト・透明の背景で撮るので、並べたときに揃う（タルコフ・RE:2 の物の絵と同じ作り方）。
#   物は、絵の枠いっぱいに収まるように、モデルの外側の箱からカメラの写す範囲を決める（物の大きさを手で合わせない）。
#   仕様は SummaryPlan/個別仕様/UI/ItemIcon_Plan.txt。
#
#   絵は TGA（RGBA・透明の背景）で書く。UI は ToneMap の後に、sRGB ではない画面へそのまま描くので、
#   ゲームでは色の変換をせずに読む（TEXTURE_COLOR_SPACE_LINEAR）。そのため Blender の色の見え方は「Standard」にする
#   （AgX・Filmic のトーンカーブを掛けると、画像の色と違って見える）。
#
# 物ごとの設定は tools/configs/item_icons.json に書く（スクリプト本体は物ごとに書き換えない）。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   blender -b --factory-startup --python tools/modelgen/item_icon_gen.py -- tools/configs/item_icons.json
#   blender -b --factory-startup --python tools/modelgen/item_icon_gen.py -- tools/configs/item_icons.json icon_fuel   … 名前を指定した物だけ
# =====================================================================

import bpy
import json
import math
import os
import sys

from mathutils import Euler, Matrix, Vector

# ------------------------- CONFIG -------------------------
# 既定値。設定ファイルに書いた項目だけが上書きされる。特定の物の値はここに書かない。
# パスは設定ファイルからの相対パスで書ける。
CONFIG = {
    "output_dir": "",       # 絵を書き出すフォルダ（<name>.tga）
    "samples": 64,          # Cycles のサンプル数（小さい絵なので少なくてよい。ノイズはデノイズで消す）
    "margin": 0.08,         # 絵の端に空ける割合（物が枠に触れないように）
    # カメラの向き（度）。物を斜め上・少し左から見る。どの物も同じにする（並べたときに揃えるため）。
    "camera_elevation": 25.0,
    "camera_azimuth": -35.0,
    # 物ごとの設定。
    #   name: 書き出す名前（拡張子なし）。ItemCatalog.cpp の icon と同じにする
    #   fbx: 読み込む FBX
    #   size: [幅, 高さ]（画素。2 の累乗。1 マスの物は正方形、2 マスの物は横 2 : 縦 1）
    #   rotation: [x, y, z]（度。物の置き方。横長の絵に縦長の物を寝かせるときなど）
    #   mesh_names: 撮る部品の名前（任意。書かなければ全部。銃の FBX の、銃の外に置かれた弾などを外すのに使う）
    #   camera_elevation / camera_azimuth: カメラの向き（任意。書かなければ上の既定。銃は真横から撮る）
    #   albedo: 色の画像（必須）
    #   normal: 法線マップ（任意。OpenGL 式＝緑が上）
    #   orm: 材質の画像（任意。r AO・g 粗さ・b 金属。ゲームの *_orm.tga と同じ並び）
    #   metallic_smoothness: Unity 式の材質の画像（任意。r 金属・a 滑らかさ）
    #   roughness / metallic: 材質の画像が無いときの値
    "icons": [],
}

# ライト（どの物も同じ）。主な光は左上の前から、弱い光を右から、縁を出す光を後ろから当てる。
KEY_LIGHT = { "rotation": ( 50.0, 0.0, -40.0 ), "energy": 4.0 }
FILL_LIGHT = { "rotation": ( 60.0, 0.0, 70.0 ), "energy": 1.2 }
RIM_LIGHT = { "rotation": ( -60.0, 0.0, 160.0 ), "energy": 2.0 }
WORLD_STRENGTH = 0.5  # まわりの明るさ（陰の中が真っ黒にならないように）
WORLD_GRAY = 0.6
DEFAULT_ROUGHNESS = 0.6
DEFAULT_METALLIC = 0.0
CAMERA_DISTANCE = 10.0  # 正投影なので距離は写る大きさに効かない。物より十分遠ければよい
CLIP_END = 100.0
# ----------------------------------------------------------


def args_after_dashes():
    # blender ... --python item_icon_gen.py -- <設定ファイル> [名前 ...] の、-- より後ろを返す。
    if "--" not in sys.argv:
        return []
    return sys.argv[ sys.argv.index( "--" ) + 1: ]


def load_config( path ):
    config = dict( CONFIG )
    with open( path, encoding="utf-8" ) as f:
        config.update( json.load( f ) )
    base = os.path.dirname( os.path.abspath( path ) )
    config[ "output_dir" ] = os.path.normpath( os.path.join( base, config[ "output_dir" ] ) )
    for icon in config[ "icons" ]:
        for key in ( "fbx", "albedo", "normal", "orm", "metallic_smoothness" ):
            if icon.get( key ):
                icon[ key ] = os.path.normpath( os.path.join( base, icon[ key ] ) )
    return config


def reset_scene( config ):
    bpy.ops.wm.read_factory_settings( use_empty=True )
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = config[ "samples" ]
    scene.cycles.use_denoising = True
    scene.render.film_transparent = True
    scene.view_settings.view_transform = "Standard"
    scene.render.image_settings.file_format = "TARGA_RAW"
    scene.render.image_settings.color_mode = "RGBA"
    world = bpy.data.worlds.new( "IconWorld" )
    world.use_nodes = True
    background = world.node_tree.nodes[ "Background" ]
    background.inputs[ "Color" ].default_value = ( WORLD_GRAY, WORLD_GRAY, WORLD_GRAY, 1.0 )
    background.inputs[ "Strength" ].default_value = WORLD_STRENGTH
    scene.world = world
    return scene


def add_sun( name, light ):
    data = bpy.data.lights.new( name, type="SUN" )
    data.energy = light[ "energy" ]
    obj = bpy.data.objects.new( name, data )
    obj.rotation_euler = Euler( [ math.radians( a ) for a in light[ "rotation" ] ] )
    bpy.context.scene.collection.objects.link( obj )


def load_image( path, non_color ):
    image = bpy.data.images.load( path, check_existing=True )
    if non_color:
        image.colorspace_settings.name = "Non-Color"
    return image


def make_material( icon ):
    # FBX のマテリアルは画像を持っていない（Unity・ゲーム用に画像を別に置いている）ので、設定の画像で作り直す。
    material = bpy.data.materials.new( icon[ "name" ] )
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    shader = nodes[ "Principled BSDF" ]
    albedo = nodes.new( "ShaderNodeTexImage" )
    albedo.image = load_image( icon[ "albedo" ], False )
    links.new( albedo.outputs[ "Color" ], shader.inputs[ "Base Color" ] )
    shader.inputs[ "Roughness" ].default_value = icon.get( "roughness", DEFAULT_ROUGHNESS )
    shader.inputs[ "Metallic" ].default_value = icon.get( "metallic", DEFAULT_METALLIC )
    if icon.get( "orm" ):
        orm = nodes.new( "ShaderNodeTexImage" )
        orm.image = load_image( icon[ "orm" ], True )
        split = nodes.new( "ShaderNodeSeparateColor" )
        links.new( orm.outputs[ "Color" ], split.inputs[ "Color" ] )
        links.new( split.outputs[ "Green" ], shader.inputs[ "Roughness" ] )
        links.new( split.outputs[ "Blue" ], shader.inputs[ "Metallic" ] )
    if icon.get( "metallic_smoothness" ):
        texture = nodes.new( "ShaderNodeTexImage" )
        texture.image = load_image( icon[ "metallic_smoothness" ], True )
        split = nodes.new( "ShaderNodeSeparateColor" )
        links.new( texture.outputs[ "Color" ], split.inputs[ "Color" ] )
        links.new( split.outputs[ "Red" ], shader.inputs[ "Metallic" ] )
        invert = nodes.new( "ShaderNodeMath" )  # 粗さ = 1 - 滑らかさ
        invert.operation = "SUBTRACT"
        invert.inputs[ 0 ].default_value = 1.0
        links.new( texture.outputs[ "Alpha" ], invert.inputs[ 1 ] )
        links.new( invert.outputs[ "Value" ], shader.inputs[ "Roughness" ] )
    if icon.get( "normal" ):
        texture = nodes.new( "ShaderNodeTexImage" )
        texture.image = load_image( icon[ "normal" ], True )
        normal_map = nodes.new( "ShaderNodeNormalMap" )
        links.new( texture.outputs[ "Color" ], normal_map.inputs[ "Color" ] )
        links.new( normal_map.outputs[ "Normal" ], shader.inputs[ "Normal" ] )
    return material


def import_item( icon ):
    bpy.ops.import_scene.fbx( filepath=icon[ "fbx" ] )
    names = icon.get( "mesh_names" )
    if names:
        for obj in [ o for o in bpy.context.scene.objects if o.type == "MESH" and o.name not in names ]:
            bpy.data.objects.remove( obj, do_unlink=True )
    meshes = [ o for o in bpy.context.scene.objects if o.type == "MESH" ]
    if not meshes:
        raise RuntimeError( "no mesh in " + icon[ "fbx" ] )
    material = make_material( icon )
    # 物の置き方（rotation）は、読み込んだ物すべてを、世界の原点を中心に回す。
    rotation = Euler( [ math.radians( a ) for a in icon.get( "rotation", [ 0.0, 0.0, 0.0 ] ) ] ).to_matrix().to_4x4()
    for obj in meshes:
        obj.data.materials.clear()
        obj.data.materials.append( material )
        obj.matrix_world = rotation @ obj.matrix_world
    bpy.context.view_layer.update()
    return meshes


def world_corners( meshes ):
    corners = []
    for obj in meshes:
        corners.extend( obj.matrix_world @ Vector( c ) for c in obj.bound_box )
    return corners


def add_camera( config, icon, meshes ):
    width, height = icon[ "size" ]
    scene = bpy.context.scene
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.resolution_percentage = 100

    data = bpy.data.cameras.new( "IconCamera" )
    data.type = "ORTHO"
    data.clip_end = CLIP_END
    camera = bpy.data.objects.new( "IconCamera", data )
    scene.collection.objects.link( camera )
    scene.camera = camera
    # カメラの向き：Blender のカメラは -Z を見る。下を向く角度（elevation）と、物のまわりを回る角度（azimuth）で決める。
    elevation = icon.get( "camera_elevation", config[ "camera_elevation" ] )
    azimuth = icon.get( "camera_azimuth", config[ "camera_azimuth" ] )
    orientation = Euler( ( math.radians( 90.0 - elevation ), 0.0, math.radians( azimuth ) ) )
    rotation = orientation.to_matrix()

    # 物の外側の箱の角をカメラの向きの空間へ移し、写す範囲（中心と大きさ）を決める。
    corners = world_corners( meshes )
    to_camera = rotation.transposed()
    local = [ to_camera @ c for c in corners ]
    min_x = min( p.x for p in local )
    max_x = max( p.x for p in local )
    min_y = min( p.y for p in local )
    max_y = max( p.y for p in local )
    center_local = Vector( ( ( min_x + max_x ) * 0.5, ( min_y + max_y ) * 0.5, max( p.z for p in local ) + CAMERA_DISTANCE ) )
    # 正投影の ortho_scale は、絵の長い辺（横長なら幅）に写る長さ。縦も収まるように、縦の長さを横の比に直して大きい方を取る。
    aspect = width / height
    extent = max( max_x - min_x, ( max_y - min_y ) * aspect )
    data.ortho_scale = extent * ( 1.0 + config[ "margin" ] * 2.0 )
    camera.matrix_world = Matrix.Translation( rotation @ center_local ) @ rotation.to_4x4()


def render_icon( config, icon ):
    reset_scene( config )
    add_sun( "Key", KEY_LIGHT )
    add_sun( "Fill", FILL_LIGHT )
    add_sun( "Rim", RIM_LIGHT )
    meshes = import_item( icon )
    add_camera( config, icon, meshes )
    output = os.path.join( config[ "output_dir" ], icon[ "name" ] + ".tga" )
    bpy.context.scene.render.filepath = output
    bpy.ops.render.render( write_still=True )
    print( "item_icon_gen: wrote " + output )


def main():
    args = args_after_dashes()
    if not args:
        raise SystemExit( "usage: blender -b --factory-startup --python tools/modelgen/item_icon_gen.py -- <config.json> [name ...]" )
    config = load_config( args[ 0 ] )
    names = set( args[ 1: ] )
    for icon in config[ "icons" ]:
        if names and icon[ "name" ] not in names:
            continue
        render_icon( config, icon )


main()
