#include "GameSrc/Player/FirstPersonBody.h"
#include "GameSrc/Player/WeaponPoseParams.h"
#include "GameSrc/Player/AimSwayParams.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Math/MatrixBlend.h"
#include "GameSrc/Math/AxisRotation.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Math/Functions.h"

#include <algorithm>
#include <cstring>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 視線の上下を背骨へ振り分ける骨（根に近い順）。1本あたり ピッチ÷本数 ずつ曲げる。
	const char* const kSpineBones[] = {
		"mixamorig:Spine",
		"mixamorig:Spine1",
		"mixamorig:Spine2",
	};
	const int kSpineBoneNumber = sizeof( kSpineBones ) / sizeof( kSpineBones[ 0 ] );

	// 背骨より根の側（曲げない下半身）。これと背骨以外は全部、背骨の先（上半身）として一緒に回す。
	// 骨格から親子を引く手段が無い（Tree が公開していない）ので、少ない側を名前で持つ。
	const char* const kLowerBodyBones[] = {
		"mixamorig:Hips",
		"mixamorig:LeftUpLeg",  "mixamorig:LeftLeg",  "mixamorig:LeftFoot",  "mixamorig:LeftToeBase",  "mixamorig:LeftToe_End",
		"mixamorig:RightUpLeg", "mixamorig:RightLeg", "mixamorig:RightFoot", "mixamorig:RightToeBase", "mixamorig:RightToe_End",
	};

	// 覗くときに一緒に動かす両腕。名前の頭で探す（"mixamorig:LeftHand" で指の骨 LeftHandIndex1 なども拾う）。
	// 肩（Shoulder）から含めるのは、腕の付け根だけ動かすと二の腕が不自然に伸びるため。
	const char* const kArmBonePrefixes[] = {
		"mixamorig:LeftShoulder",  "mixamorig:LeftArm",  "mixamorig:LeftForeArm",  "mixamorig:LeftHand",
		"mixamorig:RightShoulder", "mixamorig:RightArm", "mixamorig:RightForeArm", "mixamorig:RightHand",
	};

	// 両手持ちで手首を曲げるときに一緒に回す骨（右手と指。"mixamorig:RightHandIndex1" なども拾う）。
	const char* const kRightHandBonePrefixes[] = {
		"mixamorig:RightHand",
	};
	// 手首から見た2つの向きがこれより平行に近ければ、回転軸が決まらないので回さない（ほぼ回す必要も無い）。
	const float kMinAxisLength = 1e-6f;

	// 一人称で消す骨。カメラは頭の中にあるので、頭（と髪・顔）が視界を塞がないようにする。首は残す（見下ろすと襟元が見える）。
	const char* const kHeadBone = "mixamorig:Head";
	const char* const kHiddenBones[] = {
		kHeadBone,
		"mixamorig:HeadTop_End",
	};

	const float kCollapsedScale = 0.f; // 骨の原点へ頂点を潰す縮尺

	// 走るときに胸元へ寄せる基準の骨。抱え方・反動・目の位置の値は WeaponPoseParams（起動中に F6 で動かせる）。
	const char* const kChestBone = "mixamorig:Spine2";
	const float kFallbackEyeHeight = 1.65f; // 頭の骨が無いときの、足元からの目の高さ（身長約1.8）

	WeaponPoseParams gParams;

	bool contains( const std::vector< int >& bones, int bone ) {
		return std::find( bones.begin(), bones.end(), bone ) != bones.end();
	}

	bool startsWith( const char* text, const char* prefix ) {
		return std::strncmp( text, prefix, std::strlen( prefix ) ) == 0;
	}

	// 名前の並びを骨の番号にする。見つからない名前は飛ばし、デバッグ出力で知らせる。
	template< int N >
	std::vector< int > findBones( const SkinnedMeshData* data, const char* const ( &names )[ N ] ) {
		std::vector< int > bones;
		for ( const char* name : names ) {
			int bone = data->findBone( name );
			if ( bone < 0 ) {
				GameLib::cout << "FirstPersonBody: bone not found: " << name << GameLib::endl;
				continue;
			}
			bones.push_back( bone );
		}
		return bones;
	}

	// 名前の頭が prefixes のどれかに合う骨を全部集める。
	template< int N >
	std::vector< int > findBonesByPrefix( const SkinnedMeshData* data, const char* const ( &prefixes )[ N ] ) {
		std::vector< int > bones;
		for ( int b = 0; b < data->boneNumber(); ++b ) {
			for ( const char* prefix : prefixes ) {
				if ( startsWith( data->boneName( b ), prefix ) ) {
					bones.push_back( b );
					break;
				}
			}
		}
		return bones;
	}

	Vector3 translationOf( const Matrix34& m ) {
		return Vector3( m.m03, m.m13, m.m23 );
	}

	// 骨の並びへまとめて、左から変換を掛ける（骨格の根の空間で動かす）。
	void transformBones( const Matrix34& transform, const std::vector< int >& bones, std::vector< Matrix34 >* pose ) {
		for ( int b : bones ) {
			Matrix34 moved;
			moved.setMul( transform, ( *pose )[ b ] );
			( *pose )[ b ] = moved;
		}
	}

	// 銃の置き場所 gun（手の骨 * 持ち方）を、向きが axes（列が 銃の +X / +Y / +Z(銃口) の行き先）になり、
	// 銃に付いた点 anchor が goal へ来るよう動かす変換を求める。両腕へ掛ければ、手が銃から離れずに銃ごと動く。
	Matrix34 alignWeapon( const Matrix34& gun, const Matrix34& axes, const Vector3& anchor, const Vector3& goal ) {
		// 銃の向き（銃口 = モデルの +Z、上 = +Y）を、長さ1で直交する3軸にする（持ち方の縮尺を取り除く）。
		Vector3 barrel( gun.m02, gun.m12, gun.m22 );
		barrel.normalize();
		Vector3 gunSide;
		gunSide.setCross( Vector3( gun.m01, gun.m11, gun.m21 ), barrel );
		gunSide.normalize();
		Vector3 gunUp;
		gunUp.setCross( barrel, gunSide );

		// 回転: 銃の3軸を axes の3軸へ重ねる（= axes × 銃の軸の転置）。銃の傾き（カント）も axes に揃う。
		Matrix34 align;
		align.setIdentity();
		const int kAxisNumber = 3;
		const int kSideAxis = 0;
		const int kUpAxis = 1;
		const int kBarrelAxis = 2;
		for ( int row = 0; row < kAxisNumber; ++row ) {
			for ( int column = 0; column < kAxisNumber; ++column ) {
				align( row, column ) = axes( row, kSideAxis ) * gunSide[ column ]
					+ axes( row, kUpAxis ) * gunUp[ column ]
					+ axes( row, kBarrelAxis ) * barrel[ column ];
			}
		}
		// 移動: 回した後の anchor が goal へ来るようにする。
		Vector3 rotatedAnchor;
		align.mul33( &rotatedAnchor, anchor );
		align.m03 = goal.x - rotatedAnchor.x;
		align.m13 = goal.y - rotatedAnchor.y;
		align.m23 = goal.z - rotatedAnchor.z;
		return align;
	}

	// 変換を効き具合の分だけ骨へ掛ける（回転は球面補間なので、途中で腕が縮んだりしない）。
	void transformBonesPartially( const Matrix34& transform, float weight, const std::vector< int >& bones, std::vector< Matrix34 >* pose ) {
		Matrix34 none;
		none.setIdentity();
		Matrix34 partial;
		MatrixBlend::blend( none, transform, weight, &partial );
		transformBones( partial, bones, pose );
	}
}

WeaponPoseParams& weaponPoseParams() {
	return gParams;
}

FirstPersonBody::FirstPersonBody( const SkinnedMeshData* data ) :
	mHeadBone( data->findBone( kHeadBone ) ),
	mChestBone( data->findBone( kChestBone ) ),
	mPitch( 0.f ),
	mHandBone( -1 ),
	mSightEye( 0.f, 0.f, 0.f ),
	mSupportHandBone( -1 ),
	mSupportHandOnWeapon( 0.f, 0.f, 0.f ),
	mWeaponMin( 0.f, 0.f, 0.f ),
	mWeaponMax( 0.f, 0.f, 0.f ),
	mHoldWeight( 0.f ),
	mAimWeight( 0.f ),
	mCarryWeight( 0.f ),
	mRecoil( 0.f ),
	mSwayYaw( 0.f ),
	mSwayPitch( 0.f ),
	mArmOverride( nullptr ),
	mArmOverrideWeight( 0.f ),
	mEyePosition( 0.f, kFallbackEyeHeight, 0.f ),
	mHasWeaponBounds( false ),
	mWeaponLow( 0.f, 0.f, 0.f ),
	mWeaponHigh( 0.f, 0.f, 0.f ) {
	mGrip.setIdentity();
	std::vector< int > lowerBody = findBones( data, kLowerBodyBones );
	std::vector< int > spine = findBones( data, kSpineBones );
	mHiddenBones = findBones( data, kHiddenBones );
	mArmBones = findBonesByPrefix( data, kArmBonePrefixes );
	mRightHandBones = findBonesByPrefix( data, kRightHandBonePrefixes );

	// 背骨 i で回すのは「下半身でもなく、i より根の側の背骨でもない骨」。
	// 根の側の背骨は、その背骨自身の回転で既に位置が決まっているので回さない。
	for ( size_t i = 0; i < spine.size(); ++i ) {
		SpineJoint joint;
		joint.bone = spine[ i ];
		std::vector< int > rootSide( spine.begin(), spine.begin() + i );
		for ( int b = 0; b < data->boneNumber(); ++b ) {
			if ( !contains( lowerBody, b ) && !contains( rootSide, b ) ) {
				joint.followers.push_back( b );
			}
		}
		mSpine.push_back( joint );
	}
}

void FirstPersonBody::setPitch( float degree ) {
	mPitch = degree;
}

void FirstPersonBody::setWeapon( int handBone, const Matrix34& grip, const Vector3& sightEye,
int supportHandBone, const Vector3& supportHandOnWeapon, const Vector3& weaponMin, const Vector3& weaponMax ) {
	mHandBone = handBone;
	mGrip = grip;
	mSightEye = sightEye;
	mSupportHandBone = supportHandBone;
	mSupportHandOnWeapon = supportHandOnWeapon;
	mWeaponMin = weaponMin;
	mWeaponMax = weaponMax;
	mHasWeaponBounds = false; // 次の apply で、この銃の範囲を求め直す
}

void FirstPersonBody::setHoldWeight( float weight ) {
	mHoldWeight = weight;
}

void FirstPersonBody::setAimWeight( float weight ) {
	mAimWeight = weight;
}

void FirstPersonBody::setCarryWeight( float weight ) {
	mCarryWeight = weight;
}

void FirstPersonBody::setRecoil( float kick ) {
	mRecoil = kick;
}

void FirstPersonBody::setSway( float yawDegree, float pitchDegree ) {
	mSwayYaw = yawDegree;
	mSwayPitch = pitchDegree;
}

void FirstPersonBody::setArmOverride( const std::vector< Matrix34 >* pose, float weight ) {
	mArmOverride = pose;
	mArmOverrideWeight = weight;
}

const Vector3& FirstPersonBody::eyePosition() const {
	return mEyePosition;
}

bool FirstPersonBody::weaponBounds( Vector3* low, Vector3* high ) const {
	if ( !mHasWeaponBounds ) {
		return false;
	}
	*low = mWeaponLow;
	*high = mWeaponHigh;
	return true;
}

void FirstPersonBody::apply( std::vector< Matrix34 >* pose ) {
	// 順番に意味がある: リロードなら腕をアニメへ差し替える → 背骨を曲げる → 銃を両手の間に置く → 走るなら胸元へ寄せる → 曲げた後の頭から目を決める
	// → その目へ銃を寄せる → 照準のぶれで銃を回す → 反動で銃を跳ねさせる
	// 腕の差し替えを最初にするのは、差し替えた腕も背骨の曲げ（視線の上下）に付いてくるようにするため。
	// → 跳ねた銃を囲む → 頭を消す。
	// ぶれと反動は、置き終えた銃（覗いていればサイトが目に合った銃）から動かす。覗く補正より前に掛けると、合わせ直されて消える。
	// 銃を囲むのは最後に銃を動かした後（見えている銃と当たり判定がずれないように）。
	// 目を曲げる前の頭で決めると、見上げ・見下ろしで目と銃がずれる。
	// 胸元へ寄せる・覗く補正は両腕をまとめて動かすだけなので、先に両手持ちにしておけば左手は銃に付いたまま。
	// 覗く補正は最後に目へ合わせ直すので、走り⇔覗くの切り替え中に両方が効いていても銃は目へ向かう。
	overrideArms( pose );
	bendSpine( pose );
	holdWithBothHands( pose );
	carryAtChest( pose );
	updateEyePosition( *pose );
	alignSightToEye( pose );
	swayWeapon( pose );
	kickWeapon( pose );
	updateWeaponBounds( *pose );
	hideHead( pose );
}

void FirstPersonBody::bendSpine( std::vector< Matrix34 >* pose ) const {
	// 骨格の根の空間ではキャラの前方が +Z、右が -X。X 軸まわりに +θ 回すと前方が下を向くので、
	// ピッチ（正で見下ろす）をそのまま X 軸の回転角に使える。
	// 全部同じ軸まわりなので、根の側から順に「今の関節の位置」を中心に回せば、親子で曲げたのと同じになる。
	float bend = mPitch / static_cast< float >( kSpineBoneNumber );
	for ( const SpineJoint& joint : mSpine ) {
		Vector3 center = translationOf( ( *pose )[ joint.bone ] );
		Matrix34 rotation; // 関節を原点へ → 回す → 関節へ戻す
		rotation.setTranslation( center );
		rotation.rotateX( bend );
		rotation.translate( -center.x, -center.y, -center.z );
		for ( int b : joint.followers ) {
			Matrix34 bent;
			bent.setMul( rotation, ( *pose )[ b ] );
			( *pose )[ b ] = bent;
		}
	}
}

void FirstPersonBody::holdWithBothHands( std::vector< Matrix34 >* pose ) const {
	if ( mHandBone < 0 || mSupportHandBone < 0 || mHoldWeight <= 0.f ) {
		return;
	}
	// 手首（右手の骨の原点）から見た「銃の上で左手が来るべき点」と「今の左手」の向きを重ねる、最小の回転を求める。
	// 手首を中心に回すのは、握りの中心を中心にすると手首が前腕から外れて見えるため。
	Matrix34 gun;
	gun.setMul( ( *pose )[ mHandBone ], mGrip );
	Vector3 supportOnGun;
	gun.mul( &supportOnGun, mSupportHandOnWeapon );
	Vector3 wrist = translationOf( ( *pose )[ mHandBone ] );
	Vector3 toGun;
	toGun.setSub( supportOnGun, wrist );
	Vector3 toHand;
	toHand.setSub( translationOf( ( *pose )[ mSupportHandBone ] ), wrist );

	Vector3 axis;
	axis.setCross( toGun, toHand );
	float sine = axis.length();
	if ( sine < kMinAxisLength ) {
		return;
	}
	axis *= 1.f / sine;
	float angle = Math::atan2( sine, toGun.dot( toHand ) ); // 度。外積の長さと内積の比から求めると、0°・180°付近でも精度が落ちない
	// 手首から左手までの距離は合わせない（向きだけ）。向きを合わせた後の残りは 歩き 1〜1.7cm・走り 0.6cm 以下と測れたので、
	// 腕を伸び縮みさせる処理（IK）は持たない。
	transformBones( AxisRotation::about( wrist, axis, angle * mHoldWeight ), mRightHandBones, pose );
}

void FirstPersonBody::updateEyePosition( const std::vector< Matrix34 >& pose ) {
	if ( mHeadBone < 0 ) {
		return; // 足元から一定の高さのまま
	}
	// 目の向きは頭の骨の回転ではなくキャラの前方（+Z）で決める。頭の骨はアニメで揺れるので、
	// 回転まで使うと覗いた銃ごと画面が傾いて酔いやすい。位置の揺れ（歩きの上下）だけを視点に出す。
	// 頭の骨から目までの出し方は、背骨と同じくピッチぶん傾ける。傾けないと、見上げたときに目が上半身に対して
	// 下（あご・胸の側）へずれ、腕を下から見上げる形になって、腕が前へ突っ張って見える。
	Matrix34 view;
	view.setRotationX( mPitch );
	Vector3 eyeFromHead;
	view.mul33( &eyeFromHead, Vector3( 0.f, gParams.eyeAboveHead, gParams.eyeForward ) );
	mEyePosition.setAdd( translationOf( pose[ mHeadBone ] ), eyeFromHead );
}

void FirstPersonBody::alignSightToEye( std::vector< Matrix34 >* pose ) const {
	if ( mHandBone < 0 || mAimWeight <= 0.f ) {
		return;
	}
	// 今のポーズでの銃の置き場所（HeldItem が描くのと同じ「手の骨 * 持ち方」）。
	Matrix34 gun;
	gun.setMul( ( *pose )[ mHandBone ], mGrip );
	// 視線の3軸。前方 +Z をピッチぶん下へ傾けたもの（FirstPersonCamera と同じ向き）。
	// 銃をこの向きに揃えると、銃の傾き（カント）も取れてサイトがまっすぐ立つ。
	Matrix34 view;
	view.setRotationX( mPitch );
	// サイトの目の点を目の位置へ置く。
	Vector3 sight;
	gun.mul( &sight, mSightEye );
	transformBonesPartially( alignWeapon( gun, view, sight, mEyePosition ), mAimWeight, mArmBones, pose );
}

void FirstPersonBody::swayWeapon( std::vector< Matrix34 >* pose ) const {
	if ( mHandBone < 0 || ( mSwayYaw == 0.f && mSwayPitch == 0.f ) ) {
		return;
	}
	// 回す中心は目から視線の逆へ下げた所（視線は前方 +Z をピッチぶん下へ傾けたもの。alignSightToEye と同じ向き）。
	Matrix34 view;
	view.setRotationX( mPitch );
	Vector3 behind;
	view.mul33( &behind, Vector3( 0.f, 0.f, -aimSwayParams().pivotBehindEye ) );
	Vector3 pivot;
	pivot.setAdd( mEyePosition, behind );
	// 体はカメラのヨーを向き、背骨はピッチぶん曲げてあるので、銃の向きは「ヨー 0・ピッチ mPitch」の視線に揃っている。
	// そこへ上の軸（Y）まわりにヨー、横の軸（X）まわりにピッチを足すと、向きは ヨー mSwayYaw・ピッチ mPitch + mSwayPitch になり、
	// FirstPersonCamera::weaponDirection と同じ向きになる（X まわりの回転どうしは順番を入れ替えても同じなので、ピッチを先に戻さなくてよい）。
	Matrix34 sway; // 中心を原点へ → 回す → 中心へ戻す
	sway.setTranslation( pivot );
	sway.rotateY( mSwayYaw );
	sway.rotateX( mSwayPitch );
	sway.translate( -pivot.x, -pivot.y, -pivot.z );
	transformBones( sway, mArmBones, pose );
}

void FirstPersonBody::carryAtChest( std::vector< Matrix34 >* pose ) const {
	if ( mHandBone < 0 || mChestBone < 0 || mCarryWeight <= 0.f ) {
		return;
	}
	Matrix34 gun;
	gun.setMul( ( *pose )[ mHandBone ], mGrip );
	// 胸の向きは、胸の骨の回転ではなく体の前方をピッチぶん傾けたものにする（背骨はピッチぶん曲げてある）。
	// 胸の骨の回転は走りのアニメで大きく揺れるので、そのまま使うと銃が画面の中で暴れる。位置の揺れ（上下）だけを銃に出す。
	Matrix34 chest;
	chest.setRotationX( mPitch );
	Vector3 wristGoal;
	chest.mul33( &wristGoal, gParams.carryWristFromChest );
	wristGoal += translationOf( ( *pose )[ mChestBone ] );
	Matrix34 carry = chest;
	carry.rotateY( gParams.carryMuzzleInward );
	carry.rotateX( gParams.carryMuzzleDown );
	carry.rotateZ( gParams.carryCant );
	// 置き場所を手首で決めるのは、手首の位置と銃の向きを別々の定数で合わせられるようにするため（銃の先は向きだけで決まる）。
	Vector3 wrist = translationOf( ( *pose )[ mHandBone ] );
	transformBonesPartially( alignWeapon( gun, carry, wrist, wristGoal ), mCarryWeight, mArmBones, pose );
}

void FirstPersonBody::kickWeapon( std::vector< Matrix34 >* pose ) const {
	if ( mHandBone < 0 || mRecoil <= 0.f ) {
		return;
	}
	// 今の銃の向き（銃口 = モデルの +Z、横 = +X）。持ち方の縮尺が入っているので長さ1にする。
	Matrix34 gun;
	gun.setMul( ( *pose )[ mHandBone ], mGrip );
	Vector3 barrel( gun.m02, gun.m12, gun.m22 );
	barrel.normalize();
	Vector3 gunSide;
	gunSide.setCross( Vector3( gun.m01, gun.m11, gun.m21 ), barrel );
	gunSide.normalize();

	// 覗き具合で、腰だめの動きと覗いているときの動きの間を取る。
	float back = ( gParams.recoilBackHip + ( gParams.recoilBackAim - gParams.recoilBackHip ) * mAimWeight ) * mRecoil;
	float rise = ( gParams.recoilMuzzleRiseHip + ( gParams.recoilMuzzleRiseAim - gParams.recoilMuzzleRiseHip ) * mAimWeight ) * mRecoil;

	// 手首を中心に銃口を跳ね上げてから、銃口と逆の向きへ引く。銃の横の軸まわりは、正の角度で銃口が下を向くので負で回す。
	// 手首を中心にするのは、握っている所を支点に銃の先が跳ねるように見せるため。
	Matrix34 kick = AxisRotation::about( translationOf( ( *pose )[ mHandBone ] ), gunSide, -rise );
	kick.m03 -= barrel.x * back;
	kick.m13 -= barrel.y * back;
	kick.m23 -= barrel.z * back;
	transformBones( kick, mArmBones, pose );
}

void FirstPersonBody::overrideArms( std::vector< Matrix34 >* pose ) const {
	if ( !mArmOverride || mArmOverrideWeight <= 0.f || mChestBone < 0 ) {
		return;
	}
	const std::vector< Matrix34 >& layer = *mArmOverride;
	// 差し替えるポーズの胸 → 今のポーズの胸 へ移す変換。腕はこれを掛けて置く（今の胸に付いた、アニメどおりの腕）。
	// 根から見た位置で写さないのは、歩きで揺れる胸から腕が離れて見えないようにするため。
	Matrix34 toCurrentChest;
	toCurrentChest.setInverse( layer[ mChestBone ] );
	Matrix34 chestMove;
	chestMove.setMul( ( *pose )[ mChestBone ], toCurrentChest );
	for ( int b : mArmBones ) {
		Matrix34 target;
		target.setMul( chestMove, layer[ b ] );
		Matrix34 blended;
		MatrixBlend::blend( ( *pose )[ b ], target, mArmOverrideWeight, &blended );
		( *pose )[ b ] = blended;
	}
}

void FirstPersonBody::updateWeaponBounds( const std::vector< Matrix34 >& pose ) {
	if ( mHandBone < 0 ) {
		return;
	}
	// 今のポーズでの銃の置き場所（HeldItem が描くのと同じ「手の骨 * 持ち方」）で、頂点の範囲の8隅を移して囲む。
	Matrix34 gun;
	gun.setMul( pose[ mHandBone ], mGrip );
	const int kCornerNumber = 8;
	for ( int i = 0; i < kCornerNumber; ++i ) {
		// i の各ビットで、その軸を最小・最大のどちらにするかを選ぶ（8通りで8隅）。
		Vector3 corner(
			( i & 1 ) ? mWeaponMax.x : mWeaponMin.x,
			( i & 2 ) ? mWeaponMax.y : mWeaponMin.y,
			( i & 4 ) ? mWeaponMax.z : mWeaponMin.z );
		Vector3 placed;
		gun.mul( &placed, corner );
		if ( i == 0 ) {
			mWeaponLow = placed;
			mWeaponHigh = placed;
			continue;
		}
		mWeaponLow.set( Math::min( mWeaponLow.x, placed.x ), Math::min( mWeaponLow.y, placed.y ), Math::min( mWeaponLow.z, placed.z ) );
		mWeaponHigh.set( Math::max( mWeaponHigh.x, placed.x ), Math::max( mWeaponHigh.y, placed.y ), Math::max( mWeaponHigh.z, placed.z ) );
	}
	mHasWeaponBounds = true;
}

void FirstPersonBody::hideHead( std::vector< Matrix34 >* pose ) const {
	for ( int b : mHiddenBones ) {
		( *pose )[ b ].scale( kCollapsedScale );
	}
}
