# -*- coding: utf-8 -*-
# =====================================================================
# ads_view_render.py（確かめ用。ゲームには使わない）
#   拳銃を覗いたときの一人称の絵を Blender で描く（PistolAnimation_Plan.txt 15 章）。腕は pistol_hold、拳銃は本物のモデル（G2-Combat.fbx）。
#   ゲームの覗く補正（FirstPersonBody::alignSightToEye）と同じく、カメラを拳銃のサイトの目の点に置き、拳銃の前（+Z）・上（+Y）へ向ける（縦 45 度）。
#   大きさと握りの点のずらしを引数で変えて、描き比べられる。下の値は今の WeaponCatalog の拳銃と同じ（変えたら合わせる）。
# 使い方（作業フォルダはプロジェクトの LastDrop。絵は pose_compare_out/ に出る）:
#   blender -b --factory-startup --python tools/render_check/ads_view_render.py -- <大きさ（今は 1.8）> <握りの x のずらし m> [y のずらし m] [名前]
# =====================================================================
import bpy, math, os, sys
from mathutils import Matrix, Vector

D = "C:/Users/mador/Downloads/ゾンビゲーム/ゲームモデル/"
PISTOL_FBX = "C:/Users/mador/Downloads/はてなぶろぐ記事/拳銃/G2-Combat.fbx"
OUT = os.path.abspath("pose_compare_out")
os.makedirs(OUT, exist_ok=True)
AXIS = Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, -1, 0, 0), (0, 0, 0, 1)))
P = "mixamorig:"
BASE_SCALE = 1.8   # tools/configs/pistol.json の scale
GUN_MESHES = ["gun", "barrel", "slide", "loader", "safety-R", "safety-L", "slide-lock", "hammer", "trigger", "magazine-release", "magazine", "magazine-bullet"]
IN_HAND = (-0.706, 1.342, -10.76)
ROT = (-2.956, -157.73, -84.344)
GRIP = Vector((-0.008, -0.0268, -0.122))   # WeaponGrip::gripOnWeapon（BASE_SCALE のとき）
SIGHT = Vector((0.0, 0.1339, -0.51))      # WeaponGrip::sightEye

args = sys.argv[sys.argv.index("--") + 1:]
scale = float(args[0]); dx = float(args[1]); dy = float(args[2]) if len(args) > 2 else 0.0; name = args[3] if len(args) > 3 else "ads"
ratio = scale / BASE_SCALE
# 大きさを変えると、モデル上の点は原点から ratio 倍の所へ動く。ずらしは握りの点だけに足す。
grip = Vector((GRIP.x, GRIP.y * ratio, GRIP.z * ratio)) + Vector((dx, dy, 0.0))
sight = Vector((SIGHT.x, SIGHT.y * ratio, SIGHT.z))  # 目の z は変えない（目を離さない）

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=D + "Idle.fbx")
arm = [o for o in bpy.context.scene.objects if o.type == "ARMATURE"][0]
before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=D + "Pistol/pistol_hold.fbx")
imp = [o for o in bpy.data.objects if o not in before]
src = next(o for o in imp if o.type == "ARMATURE")
act = src.animation_data.action
arm.animation_data_clear(); arm.animation_data_create(); arm.animation_data.action = act
if getattr(arm.animation_data, "action_slot", 1) is None and act.slots:
    arm.animation_data.action_slot = act.slots[0]
bpy.data.batch_remove(imp)
bpy.context.scene.frame_set(1)
basis = {pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones}
arm.animation_data_clear()
for pb in arm.pose.bones:
    pb.matrix_basis = basis[pb.name]
arm.pose.bones[P + "Head"].scale = (0.001, 0.001, 0.001)
bpy.context.view_layer.update()

hand = arm.matrix_world @ arm.pose.bones[P + "RightHand"].matrix
G = (Matrix.Translation(Vector(IN_HAND)) @ Matrix.Rotation(math.radians(ROT[0]), 4, "Y") @ Matrix.Rotation(math.radians(ROT[1]), 4, "X")
     @ Matrix.Rotation(math.radians(ROT[2]), 4, "Z") @ Matrix.Scale(100.0, 4) @ Matrix.Translation(-grip))
M = hand @ AXIS.inverted() @ G   # 銃のモデル座標（ゲーム）→ 世界

before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=PISTOL_FBX)
imp = [o for o in bpy.data.objects if o not in before]
keep = [o for o in imp if o.type == "MESH" and o.name in GUN_MESHES]
bpy.data.batch_remove([o for o in imp if o not in keep and o.type != "EMPTY"])
mat = bpy.data.materials.new("gun"); mat.diffuse_color = (0.08, 0.1, 0.15, 1)
roots = [o for o in bpy.data.objects if o in imp and o.parent is None and o.name in bpy.data.objects]
adjust = M @ AXIS @ Matrix.Diagonal((scale, scale, scale, 1.0))
for o in [o for o in bpy.data.objects if o in imp and o.parent is None]:
    o.matrix_world = adjust @ o.matrix_world
for o in keep:
    o.data.materials.clear(); o.data.materials.append(mat)
bpy.context.view_layer.update()

eye = M @ sight
fwd = (M.to_3x3() @ Vector((0, 0, 1))).normalized()
up = (M.to_3x3() @ Vector((0, 1, 0))).normalized()
right = fwd.cross(up).normalized()
rot = Matrix((right, up, -fwd)).transposed()
sc = bpy.context.scene
cam_data = bpy.data.cameras.new("c"); cam_data.sensor_fit = "VERTICAL"; cam_data.angle_y = math.radians(45); cam_data.clip_start = 0.05
cam = bpy.data.objects.new("c", cam_data); sc.collection.objects.link(cam)
cam.matrix_world = Matrix.Translation(eye) @ rot.to_4x4()
sc.camera = cam
sc.render.engine = "BLENDER_WORKBENCH"; sc.display.shading.color_type = "MATERIAL"
sc.render.resolution_x = 1024; sc.render.resolution_y = 854
w = bpy.data.worlds.new("w"); w.color = (0.55, 0.6, 0.65); sc.world = w
sc.render.filepath = os.path.join(OUT, name + ".png")
bpy.ops.render.render(write_still=True)
print("RENDERED", name, "grip", tuple(round(v, 4) for v in grip), "sight", tuple(round(v, 4) for v in sight))
