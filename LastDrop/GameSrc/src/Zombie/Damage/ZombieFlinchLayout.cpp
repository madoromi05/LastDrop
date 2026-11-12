#include "GameSrc/Zombie/Damage/ZombieFlinchLayout.h"
#include "GameSrc/Render/SkinnedMesh/BoneNamePattern.h"
#include "GameSrc/Math/AxisRotation.h"
#include "GameSrc/Math/PlacementMatrix.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"

using namespace GameLib::Math;

namespace{
	const float kParallelLimit = 0.001f;	// 骨の向きと倒す向きがほぼ平行（外積の長さがこれ未満）なら、回す軸が決まらないので曲げない

	const int kMaxFollowerNames = 16;
	const int kMaxJointsPerGroup = 3;
	// 骨の名前の書き方（末尾の '*' でまとめて引く）は BoneNamePattern

	// 胴から先（背骨を曲げたときに一緒に動く骨）。背骨そのものは関節ごとに違うので含めない
	const char* const kUpperBodyBones[] = {
		"LeftShoulder", "RightShoulder", "LeftArm", "LeftForeArm", "LeftHand*",
		"RightArm", "RightForeArm", "RightHand*", "Neck", "Head", "low_jaw*",
	};

	// 関節1つの定義。名前は骨の名前の頭の共通部分を除いたもの
	struct JointDefinition{
		const char* bone;
		const char* tipBone;
		float share;
		bool withUpperBody;                         // followers に胴から先（kUpperBodyBones）も足すか
		const char* followers[ kMaxFollowerNames ]; // 余りは nullptr
	};
	// まとまり1つの定義。関節は根に近い順。値は SummaryPlan/個別仕様/ゾンビ/ZombieStagger_Plan.txt の 3 章（見た目を確かめる前の初期値）
	struct GroupDefinition{
		float kickDegree;
		JointDefinition joints[ kMaxJointsPerGroup ]; // 余りは bone が nullptr
	};

	// 胴は背骨 3 本に分けて曲げる（1 本で曲げると腰から折れたように見えるため。FirstPersonBody と同じ）
	const float kSpineShare = 1.f / 3.f;
	const float kWholeShare = 1.f;
	const bool kWithUpperBody = true;
	const bool kOwnBonesOnly = false;

	// まとまりの番号。並びは曲げる順（胴が先）で、kGroupDefinitions の並びと同じ
	enum GroupIndex{
		GROUP_TORSO,
		GROUP_HEAD,
		GROUP_LEFT_ARM,
		GROUP_RIGHT_ARM,
		GROUP_LEFT_LEG,
		GROUP_RIGHT_LEG,
		GROUP_NUMBER,
	};
	const GroupDefinition kGroupDefinitions[] = {
		// 胴：合わせて 12°
		{ 12.f, {
			{ "Spine",  "Spine1", kSpineShare, kWithUpperBody, { "Spine", "Spine1", "Spine2" } },
			{ "Spine1", "Spine2", kSpineShare, kWithUpperBody, { "Spine1", "Spine2" } },
			{ "Spine2", "Neck",   kSpineShare, kWithUpperBody, { "Spine2" } },
		} },
		{ 20.f, { { "Neck", "Head", kWholeShare, kOwnBonesOnly, { "Neck", "Head", "low_jaw*" } } } },	// 頭：首で 20°
		// 腕：肩で 25°（上腕・前腕のどちらに当たっても腕ごと押す）
		{ 25.f, { { "LeftArm",  "LeftForeArm",  kWholeShare, kOwnBonesOnly, { "LeftArm", "LeftForeArm", "LeftHand*" } } } },
		{ 25.f, { { "RightArm", "RightForeArm", kWholeShare, kOwnBonesOnly, { "RightArm", "RightForeArm", "RightHand*" } } } },
		// 脚：脚の付け根で 15°
		{ 15.f, { { "LeftUpLeg",  "LeftLeg",  kWholeShare, kOwnBonesOnly, { "LeftUpLeg", "LeftLeg", "LeftFoot*" } } } },
		{ 15.f, { { "RightUpLeg", "RightLeg", kWholeShare, kOwnBonesOnly, { "RightUpLeg", "RightLeg", "RightFoot*" } } } },
	};
	static_assert( sizeof( kGroupDefinitions ) / sizeof( kGroupDefinitions[ 0 ] ) == GROUP_NUMBER,
		"kGroupDefinitions must have one entry per GroupIndex." );

	// BodyPart の並びと同じ順（下の static_assert で数を確かめる）
	const GroupIndex kPartGroups[] = {
		GROUP_HEAD,      // Head
		GROUP_TORSO,     // Torso
		GROUP_LEFT_ARM,  // LeftUpperArm
		GROUP_RIGHT_ARM, // RightUpperArm
		GROUP_LEFT_ARM,  // LeftForeArm
		GROUP_RIGHT_ARM, // RightForeArm
		GROUP_LEFT_LEG,  // LeftThigh
		GROUP_RIGHT_LEG, // RightThigh
		GROUP_LEFT_LEG,  // LeftLowerLeg
		GROUP_RIGHT_LEG, // RightLowerLeg
	};
	static_assert( sizeof( kPartGroups ) / sizeof( kPartGroups[ 0 ] ) == static_cast< size_t >( BodyPart::Count ),
		"kPartGroups must have one entry per BodyPart." );
}

ZombieFlinchLayout::ZombieFlinchLayout( const SkinnedMeshData* data, const char* boneNamePrefix ) {
	ASSERT( data && boneNamePrefix && "ZombieFlinchLayout : argument is null." );
	for ( const GroupDefinition& groupDefinition : kGroupDefinitions ) {
		Group group;
		group.kickDegree = groupDefinition.kickDegree;
		for ( const JointDefinition& jointDefinition : groupDefinition.joints ) {
			if ( !jointDefinition.bone ) {
				break;
			}
			Joint joint;
			joint.bone = BoneNamePattern::find( data, boneNamePrefix, jointDefinition.bone );
			joint.tipBone = BoneNamePattern::find( data, boneNamePrefix, jointDefinition.tipBone );
			if ( joint.bone < 0 || joint.tipBone < 0 ) {
				GameLib::cout << "ZombieFlinchLayout: bone not found: " << boneNamePrefix << jointDefinition.bone << GameLib::endl;
				continue;
			}
			joint.share = jointDefinition.share;
			for ( const char* name : jointDefinition.followers ) {
				if ( !name ) {
					break;
				}
				BoneNamePattern::append( data, boneNamePrefix, name, &joint.followers );
			}
			if ( jointDefinition.withUpperBody ) {
				for ( const char* name : kUpperBodyBones ) {
					BoneNamePattern::append( data, boneNamePrefix, name, &joint.followers );
				}
			}
			group.joints.push_back( joint );
		}
		mGroups.push_back( group );
	}
	for ( int i = 0; i < static_cast< int >( BodyPart::Count ); ++i ) {
		mPartGroups[ i ] = kPartGroups[ i ];
	}
}

const std::vector< ZombieFlinchLayout::Group >& ZombieFlinchLayout::groups() const {
	return mGroups;
}

int ZombieFlinchLayout::groupOf( BodyPart part ) const {
	int index = static_cast< int >( part );
	ASSERT( index >= 0 && index < static_cast< int >( BodyPart::Count ) && "ZombieFlinchLayout : bad part." );
	return mPartGroups[ index ];
}

void ZombieFlinchLayout::bend( int group, const Vector3& direction, float degree, std::vector< Matrix34 >* pose ) const {
	ASSERT( group >= 0 && group < static_cast< int >( mGroups.size() ) && "ZombieFlinchLayout : bad group." );
	// 根の側の関節から順に、「今の」関節の位置で曲げる（前の関節で動いた後の位置。FirstPersonBody::bendSpine と同じ）
	for ( const Joint& joint : mGroups[ group ].joints ) {
		Vector3 center = PlacementMatrix::translationOf( ( *pose )[ joint.bone ] );
		Vector3 boneDirection;
		boneDirection.setSub( PlacementMatrix::translationOf( ( *pose )[ joint.tipBone ] ), center );
		boneDirection.normalize();
		// 骨の向き × 倒す向き の軸で正に回すと、骨の先が倒す向きへ倒れる
		// 外積の長さ（sin）を角度に掛けて、骨が倒す向きに近いほど少なく倒す（平行なら倒れない）
		Vector3 axis;
		axis.setCross( boneDirection, direction );
		float sine = axis.length();
		if ( sine < kParallelLimit ) {
			continue;
		}
		axis *= 1.f / sine;
		Matrix34 rotation = AxisRotation::about( center, axis, degree * joint.share * sine );
		for ( int b : joint.followers ) {
			Matrix34 bent;
			bent.setMul( rotation, ( *pose )[ b ] );
			( *pose )[ b ] = bent;
		}
	}
}
