# -*- coding: utf-8 -*-
# =====================================================================
# pistol_reload_gen.py
#   拳銃のリロードの腕のアニメを作る（Mixamo に拳銃のリロードが無いため）。仕様は SummaryPlan/個別仕様/PistolAnimation_Plan.txt。
#   Shooting.fbx（両手で拳銃を構えた Mixamo のアニメ）の 1 フレーム目を、骨ごとの向きで自キャラ（Ch15）の骨格に写した構えから始めて、構えで終わる。
#   構えそのもの（pistol_hold。覗いたとき）と、覗いていない間の構え（pistol_ready。ライフルと同じ所に持つ）も、止まったアニメとして
#   ここで書き出す（ゲームの腕の差し替えに使う）。pistol_ready は、ライフルの構え（rifle_idle）の体の上で、拳銃の握りをライフルの握りと
#   同じ所に・同じ向きで持たせ、両腕を IK で届かせた形（肘の曲がる側はライフルのまま。PistolAnimation_Plan.txt 14-7）。
#   リロードは pistol_ready から始めて pistol_ready で終わる（リロードを始めると覗くのをやめるため。14 章 Q3）。
#   自キャラの骨格の上で作り、自キャラの骨格で書き出す（別のキャラの骨格で作ると、骨の初めの向きの違いで手の向きがずれるため）。
#   1. 右手：構え（重み 0）とリロードの構え（重み 1。銃を胸へ寄せて傾ける）の間を、時刻ごとの重みで動かす。
#   2. 左手：設定の点（時刻と位置）をなめらかにつないだ所へ、手首を動かす。位置は「拳銃から見た位置」（構えの左手首からのずれ）か、
#      「腰から見た位置」（新しい弾倉を取る所）で書く。左手の向きは、構えのときの「拳銃から見た向き」のまま。
#   3. 肩・肘は、手首が届くように 2 本の骨の IK を解いて決める（肘の曲がる向きは構えのときのまま）。肩より根の骨と脚は動かさない。
#   毎フレームの回転をキーにして FBX（骨とアニメだけ）に書き出す。書き出した FBX は player.json の extra_animations に足す。
#
#   座標：Blender に読み込んだ世界（Z が上・キャラは -Y を向く・m）。「拳銃から見た位置」だけはゲームの銃のモデル座標
#   （x 横・y 上・z 前。WeaponCatalog の WeaponGrip と同じ）で書く。拳銃は右手に、設定の grip（WeaponCatalog の拳銃の値と同じにする）で持たせた所。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   blender -b --factory-startup --python tools/pistol_reload_gen.py -- tools/configs/pistol_reload.json
# =====================================================================

import bpy
import json
import math
import os
import sys

from mathutils import Matrix, Quaternion, Vector

# Blender の世界 → ゲームの座標（tools/fbx_to_gamelib.py の AXIS_CONVERT と同じ）。
AXIS_CONVERT = Matrix( ( ( 1.0, 0.0, 0.0, 0.0 ), ( 0.0, 0.0, 1.0, 0.0 ), ( 0.0, -1.0, 0.0, 0.0 ), ( 0.0, 0.0, 0.0, 1.0 ) ) )
BONE_PREFIX = "mixamorig:"
SIDES = { "right": "Right", "left": "Left" }
HIPS = "Hips"
REACH_MARGIN = 0.999  # 腕を伸ばしきらない（伸びきると肘の向きが決まらない）
BONE_SCALE = 100.0    # 骨格は cm・銃は m（WeaponModelComponent の kWeaponScaleInBone と同じ）
REST_FRAME = 0        # 骨格の初めの形のキーを打つフレーム（書き出す範囲 1〜 の外）
SOURCE_FRAME = 1      # 構えのアニメ（Shooting.fbx）の、構えて止まっているフレーム
BASE_HOLD = "hold"    # クリップの始めと終わりの構え：構え（覗いたとき）
BASE_READY = "ready"  # 覗いていない間の構え（ライフルと同じ所に持つ。設定の ready）


def config_path():
    if "--" not in sys.argv:
        raise SystemExit( "usage: blender -b --factory-startup --python tools/pistol_reload_gen.py -- <config.json>" )
    return sys.argv[ sys.argv.index( "--" ) + 1 ]


def bone_name( side, part ):
    return BONE_PREFIX + SIDES[ side ] + part


def smooth( t ):
    return t * t * ( 3.0 - 2.0 * t )


def sample_keys( keys, seconds ):
    # keys: [ [時刻, 値], ... ]（時刻の順）。間は始めと終わりがゆっくりの曲線でつなぐ。
    if seconds <= keys[ 0 ][ 0 ]:
        return keys[ 0 ][ 1 ]
    for ( t0, v0 ), ( t1, v1 ) in zip( keys, keys[ 1: ] ):
        if seconds <= t1:
            return v0 + ( v1 - v0 ) * smooth( ( seconds - t0 ) / ( t1 - t0 ) )
    return keys[ -1 ][ 1 ]


def world_matrix( arm, name ):
    return arm.matrix_world @ arm.pose.bones[ name ].matrix


def set_world_matrix( arm, name, world ):
    arm.pose.bones[ name ].matrix = arm.matrix_world.inverted() @ world
    bpy.context.view_layer.update()


def bone_head( arm, name ):
    return arm.matrix_world @ arm.pose.bones[ name ].head


def point_bone_at( arm, name, target ):
    # 骨の根を中心に回して、骨の向き（根 → 先）を target へ向ける（ねじれは今のまま）。
    head = bone_head( arm, name )
    tail = arm.matrix_world @ arm.pose.bones[ name ].tail
    rotation = ( tail - head ).rotation_difference( target - head ).to_matrix().to_4x4()
    world = Matrix.Translation( head ) @ rotation @ Matrix.Translation( -head ) @ world_matrix( arm, name )
    set_world_matrix( arm, name, world )


def solve_arm( arm, side, wrist, hand_world, pole ):
    # 2 本の骨の IK。肩（Arm の根）は動かさず、手首（Hand の根）を wrist へ。肘は pole の側へ曲げる。手の向きは hand_world。
    upper = bone_name( side, "Arm" )
    lower = bone_name( side, "ForeArm" )
    hand = bone_name( side, "Hand" )
    shoulder = bone_head( arm, upper )
    upper_length = ( bone_head( arm, lower ) - shoulder ).length
    lower_length = ( bone_head( arm, hand ) - bone_head( arm, lower ) ).length
    to_wrist = wrist - shoulder
    distance = min( to_wrist.length, ( upper_length + lower_length ) * REACH_MARGIN )
    axis = to_wrist.normalized()
    side_dir = pole - axis * pole.dot( axis )
    side_dir.normalize()
    along = ( upper_length * upper_length - lower_length * lower_length + distance * distance ) / ( 2.0 * distance )
    height = math.sqrt( max( 0.0, upper_length * upper_length - along * along ) )
    elbow = shoulder + axis * along + side_dir * height
    point_bone_at( arm, upper, elbow )
    point_bone_at( arm, lower, shoulder + axis * distance )
    # 手：位置は前腕の先に付いてくる。向きだけ hand_world にする（大きさは今のまま）。
    current = world_matrix( arm, hand )
    head = bone_head( arm, hand )
    scale = Matrix.Diagonal( current.to_scale() ).to_4x4()
    set_world_matrix( arm, hand, Matrix.Translation( head ) @ hand_world.to_3x3().normalized().to_4x4() @ scale )


def blend_matrices( a, b, weight ):
    # 位置は直線、向きは球面でつなぐ（大きさは a のまま）。
    location = a.to_translation().lerp( b.to_translation(), weight )
    rotation = a.to_3x3().normalized().to_quaternion().slerp( b.to_3x3().normalized().to_quaternion(), weight )
    scale = Matrix.Diagonal( a.to_scale() ).to_4x4()
    return Matrix.Translation( location ) @ rotation.to_matrix().to_4x4() @ scale


def grip_in_hand_game( grip ):
    # WeaponModelComponent::gripInHand と同じ行列（ゲームの銃の座標 → 手の骨の空間）。T・Ry・Rx・Rz・100 倍・T(-握り)。
    rotation = grip[ "rotation" ]
    return ( Matrix.Translation( Vector( grip[ "in_hand" ] ) )
        @ Matrix.Rotation( math.radians( rotation[ 0 ] ), 4, "Y" )
        @ Matrix.Rotation( math.radians( rotation[ 1 ] ), 4, "X" )
        @ Matrix.Rotation( math.radians( rotation[ 2 ] ), 4, "Z" )
        @ Matrix.Scale( BONE_SCALE, 4 )
        @ Matrix.Translation( -Vector( grip[ "on_weapon" ] ) ) )


def bones_root_first( armature ):
    # 親から子の順（親の向きを決めてから子を決めるため）。
    order = []
    def walk( bone ):
        order.append( bone.name )
        for child in bone.children:
            walk( child )
    for bone in armature.data.bones:
        if bone.parent is None:
            walk( bone )
    return order


def load_body_with_source( config ):
    # 自キャラ（body_fbx の骨格）を読み、構えのアニメ（source_fbx）の SOURCE_FRAME のポーズを、骨ごとの「世界から見た向き」で写す。
    # tools/fbx_to_gamelib.py の当て方（骨の初めの形からの回転を写す）だと、Shooting.fbx と Ch15 は骨（とくに指）の初めの向きが違うので、
    # 左手の指が伸びて開き、手の向きもずれる（2026-11-04 の画面写真）。向きそのものを写せば、元のアニメと同じ手の形になる。
    # 骨の根の位置は自キャラの骨の長さのまま（親の向きに付いてくる）。根（Hips）だけは元のアニメの位置に置く。
    # 写したポーズはアクションを持たない（呼んだ側がキーにする）。このツールが書き出す構え・リロードは、どれもこのポーズから作る。
    bpy.ops.import_scene.fbx( filepath=config[ "body_fbx" ] )
    body = [ o for o in bpy.context.scene.objects if o.type == "ARMATURE" ][ 0 ]
    body.animation_data_clear()
    before = set( bpy.data.objects )
    bpy.ops.import_scene.fbx( filepath=config[ "source_fbx" ] )
    imported = [ o for o in bpy.data.objects if o not in before ]
    source = next( o for o in imported if o.type == "ARMATURE" )
    bpy.context.scene.frame_set( SOURCE_FRAME )
    bpy.context.view_layer.update()
    for name in bones_root_first( body ):
        if name not in source.pose.bones:
            continue
        pb = body.pose.bones[ name ]
        pb.rotation_mode = "QUATERNION"
        bpy.context.view_layer.update()
        rotation = world_matrix( source, name ).to_3x3().normalized().to_4x4()
        head = bone_head( body, name ) if pb.parent else bone_head( source, name )
        scale = Matrix.Diagonal( world_matrix( body, name ).to_scale() ).to_4x4()
        set_world_matrix( body, name, Matrix.Translation( head ) @ rotation @ scale )
    bpy.data.batch_remove( imported )
    bpy.context.view_layer.update()
    return body


def is_finger( name ):
    # 手の骨より先の骨（指）。Mixamo では "LeftHandIndex1" のように手の骨の名前で始まる。
    return any( name.startswith( bone_name( side, "Hand" ) ) and name != bone_name( side, "Hand" ) for side in SIDES )


def arm_poles( world ):
    # 肘を曲げる向き（肩 → 肘）。IK で肘をこちら側へ曲げる（そのポーズと同じ曲がり方にする）。
    poles = {}
    for side in SIDES:
        shoulder = world[ bone_name( side, "Arm" ) ].to_translation()
        elbow = world[ bone_name( side, "ForeArm" ) ].to_translation()
        poles[ side ] = elbow - shoulder
    return poles


def apply_rifle_pose( arm, ready ):
    # ライフルの構えのアニメ（ready の source_fbx）の source_frame のポーズにする（全部の骨。アクションは外して形だけ残す）。
    # ゲームの変換ツール（fbx_to_gamelib.py）と同じく、アニメのアクションを自キャラの骨格へそのまま付けて読む（ゲームで見える形と同じ）。
    before = set( bpy.data.objects )
    bpy.ops.import_scene.fbx( filepath=ready[ "source_fbx" ] )
    imported = [ o for o in bpy.data.objects if o not in before ]
    source = next( o for o in imported if o.type == "ARMATURE" )
    action = source.animation_data.action
    if arm.animation_data is None:
        arm.animation_data_create()
    arm.animation_data.action = action
    # Blender 4.4+：別の骨格のアクションはスロットを付けないと動かない（fbx_to_gamelib.py の emit_animation と同じ）。
    if getattr( arm.animation_data, "action_slot", 1 ) is None and action.slots:
        arm.animation_data.action_slot = action.slots[ 0 ]
    bpy.context.scene.frame_set( ready[ "source_frame" ] )
    bpy.context.view_layer.update()
    basis = { pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones }
    arm.animation_data_clear()
    for pb in arm.pose.bones:
        pb.rotation_mode = "QUATERNION"
        pb.matrix_basis = basis[ pb.name ]
    bpy.data.batch_remove( imported )
    bpy.context.view_layer.update()


def ready_basis( arm, ready, gun_in_hand, grip_blender, left_in_gun, finger_basis ):
    # 覗いていない間の構え（PistolAnimation_Plan.txt 14-7）。ライフルの構えの体・腕から作る（ゲームでは体のアニメがライフルの物なので、
    # 同じ体の上で作れば、胸から腕を写したときに形が変わらない）。
    #   拳銃：握りの中心を、ライフルの握りの中心（＋設定のずらし）に置き、向き（銃口・上）をライフルと同じにする
    #   右腕：その拳銃を握る右手首へ IK。肘はライフルの構えと同じ側へ曲げる
    #   左腕：拳銃を構え（Shooting.fbx）と同じ形で下から包む所へ IK。肘はライフルの構えと同じ側へ曲げる
    #   指：構え（Shooting.fbx）の、拳銃を握る形（ライフルの指はハンドガードを持つ形なので使わない）
    apply_rifle_pose( arm, ready )
    rifle_world = { pb.name: world_matrix( arm, pb.name ) for pb in arm.pose.bones }
    poles = arm_poles( rifle_world )
    right_hand = bone_name( "right", "Hand" )
    rifle_grip = ready[ "rifle_grip" ]
    rifle_gun = rifle_world[ right_hand ] @ AXIS_CONVERT.inverted() @ grip_in_hand_game( rifle_grip ) @ AXIS_CONVERT
    rifle_grip_world = rifle_gun @ ( AXIS_CONVERT.inverted() @ Vector( rifle_grip[ "on_weapon" ] ).to_4d() ).to_3d()
    rotation = rifle_gun.to_3x3().normalized().to_4x4()
    pistol_gun = Matrix.Translation( rifle_grip_world + Vector( ready[ "offset" ] ) ) @ rotation @ Matrix.Translation( -grip_blender )
    hand_ready = pistol_gun @ gun_in_hand.inverted()
    for name, basis in finger_basis.items():
        arm.pose.bones[ name ].matrix_basis = basis
    bpy.context.view_layer.update()
    solve_arm( arm, "right", hand_ready.to_translation(), hand_ready, poles[ "right" ] )
    left_target = world_matrix( arm, right_hand ) @ gun_in_hand @ left_in_gun
    solve_arm( arm, "left", left_target.to_translation(), left_target, poles[ "left" ] )
    # 確かめ：手首が行き先へ届いたか（腕が届かないと離れる）。
    right_miss = ( world_matrix( arm, right_hand ).to_translation() - hand_ready.to_translation() ).length
    left_miss = ( world_matrix( arm, bone_name( "left", "Hand" ) ).to_translation() - left_target.to_translation() ).length
    print( "pistol_reload_gen: ready wrist miss right " + str( round( right_miss, 4 ) ) + " m / left " + str( round( left_miss, 4 ) ) + " m" )
    return { pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones }


def main():
    path = config_path()
    with open( path, encoding="utf-8" ) as f:
        config = json.load( f )
    fps = config[ "fps" ]
    os.makedirs( config[ "output_dir" ], exist_ok=True )

    for clip in config[ "clips" ]:
        bpy.ops.wm.read_factory_settings( use_empty=True )
        scene = bpy.context.scene
        scene.render.fps = fps
        arm = load_body_with_source( config )

        # 構え（1 フレーム目）を全部の骨の基準にし、元のアニメは外す。
        hold_basis = { pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones }
        hold_world = { pb.name: world_matrix( arm, pb.name ) for pb in arm.pose.bones }
        # 構えのアニメ（とそのスロット）を外す。スロットが残ると、これから打つキーが書き出しのときに使われない（骨格の初めの形になる）。
        arm.animation_data_clear()
        for pb in arm.pose.bones:
            pb.rotation_mode = "QUATERNION"
            pb.matrix_basis = hold_basis[ pb.name ]
        bpy.context.view_layer.update()

        # 右手から見た拳銃の置き方（拳銃の Blender 座標 → 世界）。ゲームの WeaponGrip と同じ値から作る（ゲームと同じ所に拳銃がある）。
        right_hand = bone_name( "right", "Hand" )
        left_hand = bone_name( "left", "Hand" )
        hand_hold = hold_world[ right_hand ]
        gun_in_hand = AXIS_CONVERT.inverted() @ grip_in_hand_game( config[ "grip" ] ) @ AXIS_CONVERT
        gun_hold = hand_hold @ gun_in_hand
        grip_blender = ( AXIS_CONVERT.inverted() @ Vector( config[ "grip" ][ "on_weapon" ] ).to_4d() ).to_3d()
        # 左手の構えを、拳銃から見た位置・向きで覚える（どの構えでも、左手は拳銃をこの形で下から包む）。
        left_in_gun = gun_hold.inverted() @ hold_world[ left_hand ]
        # 指の形（手の骨より先の骨の、親から見た回転）。構え（Shooting.fbx）の、拳銃を握る右手・包む左手の形。
        finger_basis = { name: m.copy() for name, m in hold_basis.items() if is_finger( name ) }
        to_game = AXIS_CONVERT.to_3x3()

        # このクリップの始めと終わりの構え（base）。"hold"：構え（覗いたとき）。"ready"：覗いていない間の構え。
        base_basis = hold_basis
        if clip.get( "base", BASE_HOLD ) == BASE_READY:
            base_basis = ready_basis( arm, config[ "ready" ], gun_in_hand, grip_blender, left_in_gun, finger_basis )
        for pb in arm.pose.bones:
            pb.matrix_basis = base_basis[ pb.name ]
        bpy.context.view_layer.update()
        base_world = { pb.name: world_matrix( arm, pb.name ) for pb in arm.pose.bones }
        hand_base = base_world[ right_hand ]
        palm = ( hand_base @ gun_in_hand ) @ grip_blender  # 拳銃の握りの中心（世界）。リロードで銃を傾ける中心
        hips_base = base_world[ BONE_PREFIX + HIPS ].to_translation()
        poles = arm_poles( base_world )

        # 右手のリロードの構え：握りを中心に、銃を傾け（横に寝かせる）・上へ向け、胸へ寄せる。
        reload = config[ "right_hand_reload" ]
        tilt = Matrix.Rotation( math.radians( reload[ "roll_degree" ] ), 4, "Y" ) @ Matrix.Rotation( math.radians( reload[ "pitch_degree" ] ), 4, "X" )
        hand_reload = Matrix.Translation( palm + Vector( reload[ "offset" ] ) ) @ tilt @ Matrix.Translation( -palm ) @ hand_base

        frames = int( round( clip[ "seconds" ] * fps ) )
        keyed = [ bone_name( side, part ) for side in SIDES for part in ( "Arm", "ForeArm", "Hand" ) ]
        for frame in range( frames + 1 ):
            seconds = frame / fps
            weight = sample_keys( clip[ "right_hand" ], seconds )
            hand_now = blend_matrices( hand_base, hand_reload, weight )
            for pb in arm.pose.bones:
                pb.matrix_basis = base_basis[ pb.name ]
            bpy.context.view_layer.update()
            solve_arm( arm, "right", hand_now.to_translation(), hand_now, poles[ "right" ] )
            gun_now = world_matrix( arm, right_hand ) @ gun_in_hand
            left_rest = gun_now @ left_in_gun
            # 左手首の行き先：点ごとに世界の位置を出してから、時刻でつなぐ（拳銃が動いても点が付いてくるように）。
            targets = []
            for key in clip[ "left_hand" ]:
                if "gun" in key:
                    offset = to_game.inverted() @ Vector( key[ "gun" ] )  # ゲームの銃の座標 → 拳銃の Blender 座標
                    targets.append( ( key[ "t" ], left_rest.to_translation() + gun_now.to_3x3() @ offset ) )
                else:
                    targets.append( ( key[ "t" ], hips_base + Vector( key[ "hip" ] ) ) )
            wrist = targets[ -1 ][ 1 ]
            for ( t0, p0 ), ( t1, p1 ) in zip( targets, targets[ 1: ] ):
                if seconds <= t1:
                    wrist = p0.lerp( p1, smooth( max( 0.0, ( seconds - t0 ) / ( t1 - t0 ) ) ) )
                    break
            solve_arm( arm, "left", wrist, left_rest, poles[ "left" ] )
            for pb in arm.pose.bones:
                if pb.name in keyed or frame == 0 or frame == frames:
                    pb.keyframe_insert( "rotation_quaternion", frame=frame + 1 )
                    pb.keyframe_insert( "location", frame=frame + 1 )
        scene.frame_start = 1
        scene.frame_end = frames + 1
        # FBX の書き出しは、書き出す瞬間のポーズを骨格の初めの形として書く。構えのまま書き出すと、変換ツールが自キャラへ写すとき
        # （初めの形からの回転を写す）に腕が T ポーズになる。なので、書き出す範囲の外（フレーム 0）に初めの形のキーを打ち、そこで書き出す。
        for pb in arm.pose.bones:
            pb.matrix_basis = Matrix.Identity( 4 )
            pb.keyframe_insert( "rotation_quaternion", frame=REST_FRAME )
            pb.keyframe_insert( "location", frame=REST_FRAME )
        scene.frame_set( REST_FRAME )
        if arm.animation_data and arm.animation_data.action:
            arm.animation_data.action.name = clip[ "name" ]

        bpy.ops.object.select_all( action="DESELECT" )
        arm.select_set( True )
        bpy.context.view_layer.objects.active = arm
        output = os.path.join( config[ "output_dir" ], clip[ "name" ] + ".fbx" )
        bpy.ops.export_scene.fbx( filepath=output, use_selection=True, object_types={ "ARMATURE" }, add_leaf_bones=False,
            bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False, bake_anim_simplify_factor=0.0 )
        print( "pistol_reload_gen: wrote " + output + " (" + str( frames + 1 ) + " frames)" )


main()
