# -*- coding: utf-8 -*-
# =====================================================================
# pose_compare_render.py（確かめ用。ゲームには使わない）
#   一人称の腕を、ゲームと同じ当て方で Blender に再現して描き、ライフルと拳銃の構えを比べる（PistolAnimation_Plan.txt 14-7）。
#   体：ライフルのアニメ（rifle aiming idle / walking）。拳銃の腕：pistol_ready などを胸から写し、末尾が "hf" なら
#   rifle_idle の右手からの動きのとおりに動かす（FirstPersonBody::overrideArms と同じ）。銃は握り→銃口の棒で描く（ライフル赤・拳銃青）。
#   目（頭＋上 0.08・前 0.20・縦 60 度）・横・前の 3 枚を書く。頭は潰す。
#   WeaponCatalog の握り（RIFLE・PISTOL）を変えたら、ここも合わせる。
# 使い方（作業フォルダはプロジェクトの LastDrop。絵は pose_compare_out/ に出る）:
#   blender -b --factory-startup --python tools/pose_compare_render.py -- [腕のFBX] [名前の印（"cur"ならライフルも描く・末尾 hf で右手に付いていく）] [腕のフレーム]
# =====================================================================
import bpy, math, os, sys
from mathutils import Matrix, Vector

D = "C:/Users/mador/Downloads/ゾンビゲーム/ゲームモデル/"
OUT = os.path.abspath("pose_compare_out")
os.makedirs(OUT, exist_ok=True)
AXIS = Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, -1, 0, 0), (0, 0, 0, 1)))
P = "mixamorig:"
ARM_PREFIX = (P + "LeftShoulder", P + "LeftArm", P + "LeftForeArm", P + "LeftHand", P + "RightShoulder", P + "RightArm", P + "RightForeArm", P + "RightHand")

RIFLE = dict(in_hand=(0.354, 0.38, -9.102), rot=(-156.062, 2.115, 89.224), grip=(0, 1.438, -0.169), muzzle=(0, 1.541, 0.471))
PISTOL = dict(in_hand=(-0.706, 1.342, -10.76), rot=(-2.956, -157.73, -84.344), grip=(-0.008, -0.0268, -0.122), muzzle=(0, 0.1069, 0.169))

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
PISTOL_READY = args[0] if args else D + "Pistol/pistol_ready.fbx"
TAG = args[1] if len(args) > 1 else "cur"
HAND_FOLLOW = TAG.endswith("hf")
LAYER_FRAME = int(args[2]) if len(args) > 2 else 1


def grip_matrix(g):
    r = g["rot"]
    return (Matrix.Translation(Vector(g["in_hand"])) @ Matrix.Rotation(math.radians(r[0]), 4, "Y")
            @ Matrix.Rotation(math.radians(r[1]), 4, "X") @ Matrix.Rotation(math.radians(r[2]), 4, "Z")
            @ Matrix.Scale(100.0, 4) @ Matrix.Translation(-Vector(g["grip"])))


def is_arm(name):
    return any(name.startswith(p) for p in ARM_PREFIX)


def setup():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=D + "Idle.fbx")
    arm = [o for o in bpy.context.scene.objects if o.type == "ARMATURE"][0]
    return arm


def apply_action(arm, path, frame):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    imp = [o for o in bpy.data.objects if o not in before]
    src = next(o for o in imp if o.type == "ARMATURE")
    act = src.animation_data.action
    arm.animation_data_clear(); arm.animation_data_create()
    arm.animation_data.action = act
    if getattr(arm.animation_data, "action_slot", 1) is None and act.slots:
        arm.animation_data.action_slot = act.slots[0]
    bpy.data.batch_remove(imp)
    bpy.context.scene.frame_set(frame)
    bpy.context.view_layer.update()


def world(arm, name):
    return arm.matrix_world @ arm.pose.bones[name].matrix


def bake_current(arm):
    # 今のポーズを matrix_basis に固定し、アクションを外す。
    basis = {pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones}
    arm.animation_data_clear()
    for pb in arm.pose.bones:
        pb.matrix_basis = basis[pb.name]
    bpy.context.view_layer.update()


def order(arm):
    out = []
    def walk(b):
        out.append(b.name)
        for c in b.children:
            walk(c)
    for b in arm.data.bones:
        if b.parent is None:
            walk(b)
    return out


def add_gun(arm, g, color):
    hand = world(arm, P + "RightHand")
    m = hand @ AXIS.inverted() @ grip_matrix(g)
    a = m @ Vector(g["grip"]); b = m @ Vector(g["muzzle"])
    d = b - a
    bpy.ops.mesh.primitive_cylinder_add(radius=0.018, depth=d.length, location=(a + b) / 2)
    cyl = bpy.context.active_object
    cyl.rotation_mode = "QUATERNION"
    cyl.rotation_quaternion = Vector((0, 0, 1)).rotation_difference(d)
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.03, location=a)
    mat = bpy.data.materials.new("gun"); mat.diffuse_color = color
    for o in (cyl, bpy.context.active_object):
        o.data.materials.append(mat)


def render(arm, name):
    head = world(arm, P + "Head").to_translation()
    sc = bpy.context.scene
    # 頭を潰す（ゲームと同じく一人称で頭の中が見えないように）
    arm.pose.bones[P + "Head"].scale = (0.001, 0.001, 0.001)
    bpy.context.view_layer.update()
    sc.render.engine = "BLENDER_WORKBENCH"
    sc.display.shading.color_type = "MATERIAL"
    sc.render.resolution_x = 640; sc.render.resolution_y = 480
    world_bg = bpy.data.worlds.new("w"); world_bg.color = (0.35, 0.4, 0.45); sc.world = world_bg
    chest = world(arm, P + "Spine2").to_translation()
    fwd = Vector((0, -1, 0))
    eye = head + Vector((0, -0.20, 0.08))
    for label, loc, look, fov in (
        ("fp", eye, eye + fwd, 60),
        ("side", chest + Vector((-2.4, -0.5, 0.0)), chest + Vector((0, -0.4, 0)), 40),
        ("front", chest + Vector((-1.2, -2.6, 0.3)), chest + Vector((0, -0.4, 0)), 40)):
        cam_data = bpy.data.cameras.new("c"); cam_data.sensor_fit = "VERTICAL"; cam_data.angle_y = math.radians(fov)
        cam_data.clip_start = 0.05
        cam = bpy.data.objects.new("c", cam_data); sc.collection.objects.link(cam)
        cam.location = loc
        cam.rotation_euler = (look - loc).to_track_quat("-Z", "Y").to_euler()
        sc.camera = cam
        sc.render.filepath = os.path.join(OUT, name + "_" + label + ".png")
        bpy.ops.render.render(write_still=True)
    print("RENDERED", name)


def rifle(anim, frame, name):
    arm = setup()
    apply_action(arm, D + "Basic Shooter Pack/" + anim, frame)
    add_gun(arm, RIFLE, (0.8, 0.3, 0.2, 1))
    render(arm, name)


def pistol(anim, frame, name, follow=1.0):
    arm = setup()
    apply_action(arm, D + "Basic Shooter Pack/rifle aiming idle.fbx", 1)
    ref_hand = world(arm, P + "RightHand")
    apply_action(arm, PISTOL_READY, LAYER_FRAME)
    chest_p = world(arm, P + "Spine2")
    arms = {n: world(arm, n) for n in order(arm) if is_arm(n)}
    apply_action(arm, D + "Basic Shooter Pack/" + anim, frame)
    bake_current(arm)
    chest_b = world(arm, P + "Spine2")
    move = chest_b @ chest_p.inverted()
    if HAND_FOLLOW:
        move = world(arm, P + "RightHand") @ ref_hand.inverted()
    else:
        hand_anim = world(arm, P + "RightHand").to_translation()
        hand_tgt = (move @ arms[P + "RightHand"]).to_translation()
        move = Matrix.Translation((hand_anim - hand_tgt) * follow) @ move
    for n in order(arm):
        if n in arms:
            pb = arm.pose.bones[n]
            pb.matrix = arm.matrix_world.inverted() @ (move @ arms[n])
            bpy.context.view_layer.update()
    add_gun(arm, PISTOL, (0.2, 0.5, 0.9, 1))
    render(arm, name)


if TAG.startswith("rl"):
    pistol("rifle aiming idle.fbx", 1, "pistol_" + TAG)
    sys.exit(0)
if TAG == "cur":
    rifle("rifle aiming idle.fbx", 1, "rifle_idle")
    rifle("walking.fbx", 10, "rifle_walk")
pistol("rifle aiming idle.fbx", 1, "pistol_idle_" + TAG)
pistol("walking.fbx", 10, "pistol_walk_" + TAG)
