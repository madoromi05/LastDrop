#include "GameSrc/Player/Body/Pose/FirstPersonRig.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"

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

	// 目の位置を決める骨。頭は潰さずに描く（影・観察カメラでも頭が見えるように）。
	// 腕・右手の骨の名前の頭のどれにも合わないので、両手持ち・胸元への寄せで頭は動かない（目を先に決めてよい理由。5-4）。
	const char* const kHeadBone = "mixamorig:Head";

	// 走るときに胸元へ寄せる基準の骨。抱え方・反動・目の位置の値は WeaponPoseParams（起動中に F6 で動かせる）。
	const char* const kChestBone = "mixamorig:Spine2";
	const float kFallbackEyeHeight = 1.65f; // 頭の骨が無いときの、足元からの目の高さ（身長約1.8）

	const int kNoBone = -1;

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
}

FirstPersonRig::FirstPersonRig( const SkinnedMeshData* data ) :
	spineDivisions( kSpineBoneNumber ),
	headBone( data->findBone( kHeadBone ) ),
	chestBone( data->findBone( kChestBone ) ),
	pitch( 0.f ),
	handBone( kNoBone ),
	sightEye( 0.f, 0.f, 0.f ),
	supportHandBone( kNoBone ),
	supportHandOnWeapon( 0.f, 0.f, 0.f ),
	weaponMin( 0.f, 0.f, 0.f ),
	weaponMax( 0.f, 0.f, 0.f ),
	eyePosition( 0.f, kFallbackEyeHeight, 0.f ) {
	grip.setIdentity();
	std::vector< int > lowerBody = findBones( data, kLowerBodyBones );
	std::vector< int > spineBones = findBones( data, kSpineBones );
	armBones = findBonesByPrefix( data, kArmBonePrefixes );
	rightHandBones = findBonesByPrefix( data, kRightHandBonePrefixes );

	// 背骨 i で回すのは「下半身でもなく、i より根の側の背骨でもない骨」。
	// 根の側の背骨は、その背骨自身の回転で既に位置が決まっているので回さない。
	for ( size_t i = 0; i < spineBones.size(); ++i ) {
		SpineJoint joint;
		joint.bone = spineBones[ i ];
		std::vector< int > rootSide( spineBones.begin(), spineBones.begin() + i );
		for ( int b = 0; b < data->boneNumber(); ++b ) {
			if ( !contains( lowerBody, b ) && !contains( rootSide, b ) ) {
				joint.followers.push_back( b );
			}
		}
		spine.push_back( joint );
	}
}

bool FirstPersonRig::hasWeapon() const {
	return handBone >= 0;
}

Matrix34 FirstPersonRig::weaponPlacement( const std::vector< Matrix34 >& pose ) const {
	Matrix34 gun;
	gun.setMul( pose[ handBone ], grip );
	return gun;
}

Matrix34 FirstPersonRig::viewRotation() const {
	Matrix34 view;
	view.setRotationX( pitch );
	return view;
}
