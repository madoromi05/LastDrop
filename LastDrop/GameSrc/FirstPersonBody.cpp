#include "FirstPersonBody.h"
#include "SkinnedMeshData.h"
#include "MatrixBlend.h"

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

	// ── 目の位置（m）──
	// 頭の骨の原点は頭蓋の付け根（首の上端）にあるので、そこから目の高さ・顔の前面まで出す。
	// 前へ出すのは、潰した頭の頂点（骨の原点に集まる）や首の内側が視界に入らないようにするため。
	const float kEyeAboveHead = 0.08f;
	const float kEyeForward   = 0.10f;
	const float kFallbackEyeHeight = 1.65f; // 頭の骨が無いときの、足元からの目の高さ（身長約1.8）

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

	// pivot を中心に、長さ1の軸 axis のまわりへ degree 度回す行列（ロドリゲスの回転公式）。
	// Matrix34 には X/Y/Z 軸まわりの回転しか無いので、任意の軸はここで組む。
	Matrix34 rotationAbout( const Vector3& pivot, const Vector3& axis, float degree ) {
		float c = Math::cos( degree );
		float s = Math::sin( degree );
		float t = 1.f - c;
		Matrix34 m;
		m.setIdentity();
		m.m00 = c + t * axis.x * axis.x;          m.m01 = t * axis.x * axis.y - s * axis.z; m.m02 = t * axis.x * axis.z + s * axis.y;
		m.m10 = t * axis.y * axis.x + s * axis.z; m.m11 = c + t * axis.y * axis.y;          m.m12 = t * axis.y * axis.z - s * axis.x;
		m.m20 = t * axis.z * axis.x - s * axis.y; m.m21 = t * axis.z * axis.y + s * axis.x; m.m22 = c + t * axis.z * axis.z;
		// 中心を原点へ動かして回し、戻す（= 移動成分は pivot - R*pivot）。
		Vector3 rotatedPivot;
		m.mul33( &rotatedPivot, pivot );
		m.m03 = pivot.x - rotatedPivot.x;
		m.m13 = pivot.y - rotatedPivot.y;
		m.m23 = pivot.z - rotatedPivot.z;
		return m;
	}

	// 骨の並びへまとめて、左から変換を掛ける（骨格の根の空間で動かす）。
	void transformBones( const Matrix34& transform, const std::vector< int >& bones, std::vector< Matrix34 >* pose ) {
		for ( int b : bones ) {
			Matrix34 moved;
			moved.setMul( transform, ( *pose )[ b ] );
			( *pose )[ b ] = moved;
		}
	}
}

FirstPersonBody::FirstPersonBody( const SkinnedMeshData* data ) :
	mHeadBone( data->findBone( kHeadBone ) ),
	mPitch( 0.f ),
	mHandBone( -1 ),
	mSightEye( 0.f, 0.f, 0.f ),
	mSupportHandBone( -1 ),
	mSupportHandOnWeapon( 0.f, 0.f, 0.f ),
	mHoldWeight( 0.f ),
	mAimWeight( 0.f ),
	mEyePosition( 0.f, kFallbackEyeHeight, 0.f ) {
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
int supportHandBone, const Vector3& supportHandOnWeapon ) {
	mHandBone = handBone;
	mGrip = grip;
	mSightEye = sightEye;
	mSupportHandBone = supportHandBone;
	mSupportHandOnWeapon = supportHandOnWeapon;
}

void FirstPersonBody::setHoldWeight( float weight ) {
	mHoldWeight = weight;
}

void FirstPersonBody::setAimWeight( float weight ) {
	mAimWeight = weight;
}

const Vector3& FirstPersonBody::eyePosition() const {
	return mEyePosition;
}

void FirstPersonBody::apply( std::vector< Matrix34 >* pose ) {
	// 順番に意味がある: 背骨を曲げる → 銃を両手の間に置く → 曲げた後の頭から目を決める → その目へ銃を寄せる → 頭を消す。
	// 目を曲げる前の頭で決めると、見上げ・見下ろしで目と銃がずれる。
	// 覗く補正は両腕をまとめて動かすだけなので、先に両手持ちにしておけば覗いても左手は銃に付いたまま。
	bendSpine( pose );
	holdWithBothHands( pose );
	updateEyePosition( *pose );
	alignSightToEye( pose );
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
	transformBones( rotationAbout( wrist, axis, angle * mHoldWeight ), mRightHandBones, pose );
}

void FirstPersonBody::updateEyePosition( const std::vector< Matrix34 >& pose ) {
	if ( mHeadBone < 0 ) {
		return; // 足元から一定の高さのまま
	}
	// 目の向きは頭の骨の回転ではなくキャラの前方（+Z）で決める。頭の骨はアニメで揺れるので、
	// 回転まで使うと覗いた銃ごと画面が傾いて酔いやすい。位置の揺れ（歩きの上下）だけを視点に出す。
	mEyePosition = translationOf( pose[ mHeadBone ] );
	mEyePosition.y += kEyeAboveHead;
	mEyePosition.z += kEyeForward;
}

void FirstPersonBody::alignSightToEye( std::vector< Matrix34 >* pose ) const {
	if ( mHandBone < 0 || mAimWeight <= 0.f ) {
		return;
	}
	// 今のポーズでの銃の置き場所（HeldItem が描くのと同じ「手の骨 * 持ち方」）。
	Matrix34 gun;
	gun.setMul( ( *pose )[ mHandBone ], mGrip );

	// 銃の向き（銃口 = モデルの +Z、上 = +Y）を、長さ1で直交する3軸にする（持ち方の縮尺を取り除く）。
	Vector3 barrel( gun.m02, gun.m12, gun.m22 );
	barrel.normalize();
	Vector3 gunRight;
	gunRight.setCross( Vector3( gun.m01, gun.m11, gun.m21 ), barrel );
	gunRight.normalize();
	Vector3 gunUp;
	gunUp.setCross( barrel, gunRight );

	// 視線の3軸。前方 +Z をピッチぶん下へ傾けたもの（FirstPersonCamera と同じ向き）。
	Vector3 viewForward( 0.f, -Math::sin( mPitch ), Math::cos( mPitch ) );
	Vector3 viewUp( 0.f, Math::cos( mPitch ), Math::sin( mPitch ) );
	Vector3 viewRight;
	viewRight.setCross( viewUp, viewForward );

	// 回転: 銃の3軸を視線の3軸へ重ねる（= 視線の軸 × 銃の軸の転置）。銃の傾き（カント）も取れて、サイトがまっすぐ立つ。
	Matrix34 align;
	align.setIdentity();
	const int kAxisNumber = 3;
	for ( int row = 0; row < kAxisNumber; ++row ) {
		for ( int column = 0; column < kAxisNumber; ++column ) {
			align( row, column ) = viewRight[ row ] * gunRight[ column ]
				+ viewUp[ row ] * gunUp[ column ]
				+ viewForward[ row ] * barrel[ column ];
		}
	}
	// 移動: 回した後のサイトの目の点が、目の位置へ来るようにする。
	Vector3 sight;
	gun.mul( &sight, mSightEye );
	Vector3 rotatedSight;
	align.mul33( &rotatedSight, sight );
	align.m03 = mEyePosition.x - rotatedSight.x;
	align.m13 = mEyePosition.y - rotatedSight.y;
	align.m23 = mEyePosition.z - rotatedSight.z;

	// 覗き具合の分だけ補正する（回転は球面補間なので、途中で腕が縮んだりしない）。
	Matrix34 none;
	none.setIdentity();
	Matrix34 partial;
	MatrixBlend::blend( none, align, mAimWeight, &partial );
	transformBones( partial, mArmBones, pose );
}

void FirstPersonBody::hideHead( std::vector< Matrix34 >* pose ) const {
	for ( int b : mHiddenBones ) {
		( *pose )[ b ].scale( kCollapsedScale );
	}
}
