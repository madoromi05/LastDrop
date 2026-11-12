#include "GameSrc/Zombie/Damage/ZombieSeverLayout.h"
#include "GameSrc/Render/SkinnedMesh/BoneNamePattern.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"

namespace{
	const int kMaxBoneNames = 4;
	const int kMaxParts = 2;
	const bool kLosesFooting = true;
	const bool kKeepsFooting = false;

	// 部位1つの定義。名前は骨の名前の頭の共通部分を除いたもの（末尾の '*' でまとめて引く。BoneNamePattern）
	struct CutDefinition{
		const char* jointBone;                 // nullptr なら切れない部位
		const char* bones[ kMaxBoneNames ];    // 余りは nullptr
		BodyPart parts[ kMaxParts ];           // 余りは BodyPart::Count
		bool losesFooting;
	};
	// BodyPart の並びと同じ順（下の static_assert で数を確かめる）
	// 足先（ToeBase）は Foot の子なので、脚と一緒に無くす（"LeftFoot*" では引けない名前のため別に書く）
	const BodyPart kNone = BodyPart::Count;
	const CutDefinition kCutDefinitions[] = {
		// Head：首の上で切る（頭が 0 で倒れるときに飛ぶ）
		{ "Head",         { "Head", "low_jaw*" },                                   { BodyPart::Head, kNone },                         kKeepsFooting },
		// Torso：切れない
		{ nullptr,        { nullptr },                                              { kNone, kNone },                                  kKeepsFooting },
		// 上腕：肩から腕ごと
		{ "LeftArm",      { "LeftArm", "LeftForeArm", "LeftHand*" },                { BodyPart::LeftUpperArm, BodyPart::LeftForeArm },   kKeepsFooting },
		{ "RightArm",     { "RightArm", "RightForeArm", "RightHand*" },             { BodyPart::RightUpperArm, BodyPart::RightForeArm }, kKeepsFooting },
		// 前腕：肘から先
		{ "LeftForeArm",  { "LeftForeArm", "LeftHand*" },                           { BodyPart::LeftForeArm, kNone },                  kKeepsFooting },
		{ "RightForeArm", { "RightForeArm", "RightHand*" },                         { BodyPart::RightForeArm, kNone },                 kKeepsFooting },
		// 太もも：脚の付け根から脚ごと
		{ "LeftUpLeg",    { "LeftUpLeg", "LeftLeg", "LeftFoot*", "LeftToeBase*" },  { BodyPart::LeftThigh, BodyPart::LeftLowerLeg },   kLosesFooting },
		{ "RightUpLeg",   { "RightUpLeg", "RightLeg", "RightFoot*", "RightToeBase*" }, { BodyPart::RightThigh, BodyPart::RightLowerLeg }, kLosesFooting },
		// すね：膝から先
		{ "LeftLeg",      { "LeftLeg", "LeftFoot*", "LeftToeBase*" },               { BodyPart::LeftLowerLeg, kNone },                 kLosesFooting },
		{ "RightLeg",     { "RightLeg", "RightFoot*", "RightToeBase*" },            { BodyPart::RightLowerLeg, kNone },                kLosesFooting },
	};
	static_assert( sizeof( kCutDefinitions ) / sizeof( kCutDefinitions[ 0 ] ) == static_cast< size_t >( BodyPart::Count ),
		"kCutDefinitions must have one entry per BodyPart." );

	const int kNoBone = -1;
}

ZombieSeverLayout::ZombieSeverLayout( const SkinnedMeshData* data, const char* boneNamePrefix ) {
	ASSERT( data && boneNamePrefix && "ZombieSeverLayout : argument is null." );
	for ( int i = 0; i < static_cast< int >( BodyPart::Count ); ++i ) {
		const CutDefinition& definition = kCutDefinitions[ i ];
		Cut& cut = mCuts[ i ];
		cut.jointBone = kNoBone;
		cut.losesFooting = definition.losesFooting;
		if ( !definition.jointBone ) {
			continue;
		}
		cut.jointBone = BoneNamePattern::find( data, boneNamePrefix, definition.jointBone );
		if ( cut.jointBone < 0 ) {
			GameLib::cout << "ZombieSeverLayout: bone not found: " << boneNamePrefix << definition.jointBone << GameLib::endl;
			continue;
		}
		for ( const char* name : definition.bones ) {
			if ( !name ) {
				break;
			}
			BoneNamePattern::append( data, boneNamePrefix, name, &cut.bones );
		}
		for ( BodyPart part : definition.parts ) {
			if ( part != kNone ) {
				cut.parts.push_back( part );
			}
		}
	}
}

bool ZombieSeverLayout::canSever( BodyPart part ) const {
	return cut( part ).jointBone >= 0;
}

const ZombieSeverLayout::Cut& ZombieSeverLayout::cut( BodyPart part ) const {
	int index = static_cast< int >( part );
	ASSERT( index >= 0 && index < static_cast< int >( BodyPart::Count ) && "ZombieSeverLayout : bad part." );
	return mCuts[ index ];
}
