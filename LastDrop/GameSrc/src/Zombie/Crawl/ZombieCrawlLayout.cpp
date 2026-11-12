#include "GameSrc/Zombie/Crawl/ZombieCrawlLayout.h"
#include "GameSrc/Render/SkinnedMesh/BoneNamePattern.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"

namespace{
	// 骨の名前の書き方（末尾の '*' でまとめて引く）は BoneNamePattern
	struct ArmDefinition{
		const char* shoulder;
		const char* elbow;
		const char* hand;
		const char* knuckle;
		const char* handAndFingers;
	};
	struct LegDefinition{
		const char* hip;
		const char* knee;
		const char* ankle;
		const char* toe;
	};
	// 並びは ZombieCrawlLayout::Side と同じ
	const ArmDefinition kArmDefinitions[] = {
		{ "LeftArm",  "LeftForeArm",  "LeftHand",  "LeftHandMiddle1",  "LeftHand*" },
		{ "RightArm", "RightForeArm", "RightHand", "RightHandMiddle1", "RightHand*" },
	};
	const LegDefinition kLegDefinitions[] = {
		{ "LeftUpLeg",  "LeftLeg",  "LeftFoot",  "LeftToeBase" },
		{ "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase" },
	};
	static_assert( sizeof( kArmDefinitions ) / sizeof( kArmDefinitions[ 0 ] ) == ZombieCrawlLayout::SIDE_NUMBER,
		"kArmDefinitions must have one entry per Side." );
	static_assert( sizeof( kLegDefinitions ) / sizeof( kLegDefinitions[ 0 ] ) == ZombieCrawlLayout::SIDE_NUMBER,
		"kLegDefinitions must have one entry per Side." );
	const char* kHipsBone = "Hips";
	const char* kSpineBone = "Spine1";
	const char* kChestBone = "Spine2";
	const char* kNeckBone = "Neck";
	const char* kHeadBone = "Head";
	const char* kChinBone = "low_jaw3";	// あごの骨は low_jaw1 → 2 → 3 と前へ並ぶ。一番前（あごの先）で顔の向きを測る
	// 胸を反らせるときに一緒に回す骨（Spine2 から先。ZombieFlinchLayout の胴から先と同じ並び）
	const char* const kChestFollowerNames[] = {
		"Spine2", "LeftShoulder", "RightShoulder", "LeftArm", "LeftForeArm", "LeftHand*",
		"RightArm", "RightForeArm", "RightHand*", "Neck", "Head", "low_jaw*",
	};
	const char* const kHeadFollowerNames[] = { "Neck", "Head", "low_jaw*" };
}

ZombieCrawlLayout::ZombieCrawlLayout( const SkinnedMeshData* data, const char* boneNamePrefix ) :
	mHips( BoneNamePattern::find( data, boneNamePrefix, kHipsBone ) ),
	mSpine( BoneNamePattern::find( data, boneNamePrefix, kSpineBone ) ),
	mChest( BoneNamePattern::find( data, boneNamePrefix, kChestBone ) ),
	mNeck( BoneNamePattern::find( data, boneNamePrefix, kNeckBone ) ),
	mHead( BoneNamePattern::find( data, boneNamePrefix, kHeadBone ) ),
	mChin( BoneNamePattern::find( data, boneNamePrefix, kChinBone ) ),
	mValid( true ) {
	ASSERT( data && boneNamePrefix && "ZombieCrawlLayout : argument is null." );
	for ( int s = 0; s < SIDE_NUMBER; ++s ) {
		const ArmDefinition& definition = kArmDefinitions[ s ];
		Arm& arm = mArms[ s ];
		arm.shoulder = BoneNamePattern::find( data, boneNamePrefix, definition.shoulder );
		arm.elbow = BoneNamePattern::find( data, boneNamePrefix, definition.elbow );
		arm.hand = BoneNamePattern::find( data, boneNamePrefix, definition.hand );
		arm.knuckle = BoneNamePattern::find( data, boneNamePrefix, definition.knuckle );
		BoneNamePattern::append( data, boneNamePrefix, definition.shoulder, &arm.upperFollowers );
		BoneNamePattern::append( data, boneNamePrefix, definition.elbow, &arm.upperFollowers );
		BoneNamePattern::append( data, boneNamePrefix, definition.handAndFingers, &arm.upperFollowers );
		BoneNamePattern::append( data, boneNamePrefix, definition.elbow, &arm.lowerFollowers );
		BoneNamePattern::append( data, boneNamePrefix, definition.handAndFingers, &arm.lowerFollowers );
		BoneNamePattern::append( data, boneNamePrefix, definition.handAndFingers, &arm.handFollowers );
		if ( arm.shoulder < 0 || arm.elbow < 0 || arm.hand < 0 || arm.knuckle < 0 ) {
			GameLib::cout << "ZombieCrawlLayout: arm bone not found: " << boneNamePrefix << definition.shoulder << GameLib::endl;
			mValid = false;
		}

		const LegDefinition& legDefinition = kLegDefinitions[ s ];
		Leg& leg = mLegs[ s ];
		leg.hip = BoneNamePattern::find( data, boneNamePrefix, legDefinition.hip );
		leg.knee = BoneNamePattern::find( data, boneNamePrefix, legDefinition.knee );
		leg.ankle = BoneNamePattern::find( data, boneNamePrefix, legDefinition.ankle );
		BoneNamePattern::append( data, boneNamePrefix, legDefinition.hip, &leg.thighFollowers );
		BoneNamePattern::append( data, boneNamePrefix, legDefinition.knee, &leg.thighFollowers );
		BoneNamePattern::append( data, boneNamePrefix, legDefinition.ankle, &leg.thighFollowers );
		BoneNamePattern::append( data, boneNamePrefix, legDefinition.toe, &leg.thighFollowers );
		BoneNamePattern::append( data, boneNamePrefix, legDefinition.knee, &leg.shinFollowers );
		BoneNamePattern::append( data, boneNamePrefix, legDefinition.ankle, &leg.shinFollowers );
		BoneNamePattern::append( data, boneNamePrefix, legDefinition.toe, &leg.shinFollowers );
		if ( leg.hip < 0 || leg.knee < 0 || leg.ankle < 0 ) {
			GameLib::cout << "ZombieCrawlLayout: leg bone not found: " << boneNamePrefix << legDefinition.hip << GameLib::endl;
			mValid = false;
		}
	}
	for ( const char* name : kChestFollowerNames ) {
		BoneNamePattern::append( data, boneNamePrefix, name, &mChestFollowers );
	}
	for ( const char* name : kHeadFollowerNames ) {
		BoneNamePattern::append( data, boneNamePrefix, name, &mHeadFollowers );
	}
	if ( mHips < 0 || mSpine < 0 || mChest < 0 || mNeck < 0 || mHead < 0 ) {
		GameLib::cout << "ZombieCrawlLayout: body bone not found (" << boneNamePrefix << "Hips / Spine1 / Spine2 / Neck / Head)" <<
			GameLib::endl;
		mValid = false;
	}
	if ( mChin < 0 ) {
		GameLib::cout << "ZombieCrawlLayout: chin bone not found: " << boneNamePrefix << kChinBone << GameLib::endl;
	}
}

bool ZombieCrawlLayout::isValid() const {
	return mValid;
}

const ZombieCrawlLayout::Arm& ZombieCrawlLayout::arm( Side side ) const {
	return mArms[ side ];
}

const ZombieCrawlLayout::Leg& ZombieCrawlLayout::leg( Side side ) const {
	return mLegs[ side ];
}

int ZombieCrawlLayout::hips() const {
	return mHips;
}

int ZombieCrawlLayout::spine() const {
	return mSpine;
}

int ZombieCrawlLayout::chest() const {
	return mChest;
}

int ZombieCrawlLayout::neck() const {
	return mNeck;
}

int ZombieCrawlLayout::head() const {
	return mHead;
}

int ZombieCrawlLayout::chin() const {
	return mChin;
}

const std::vector< int >& ZombieCrawlLayout::chestFollowers() const {
	return mChestFollowers;
}

const std::vector< int >& ZombieCrawlLayout::headFollowers() const {
	return mHeadFollowers;
}
