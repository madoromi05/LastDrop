#include "GameSrc/Zombie/Damage/ZombieDismemberment.h"
#include "GameSrc/Zombie/Damage/ZombieSeverLayout.h"
#include "GameSrc/Math/PlacementMatrix.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const float kCollapsedScale = 0.f; // 骨の原点へ頂点を潰す縮尺（FirstPersonBody の頭を消すのと同じ）
}

ZombieDismemberment::ZombieDismemberment( const ZombieSeverLayout* layout ) :
	mLayout( layout ) {
	ASSERT( mLayout && "ZombieDismemberment : layout is null." );
	for ( bool& lost : mLost ) {
		lost = false;
	}
}

bool ZombieDismemberment::sever( BodyPart part ) {
	if ( !mLayout->canSever( part ) || isLost( part ) ) {
		return false;
	}
	for ( BodyPart lostPart : mLayout->cut( part ).parts ) {
		mLost[ static_cast< int >( lostPart ) ] = true;
	}
	mCutParts.push_back( part );
	mSeveredThisHit.push_back( part );
	return true;
}

void ZombieDismemberment::clearSeveredThisHit() {
	mSeveredThisHit.clear();
}

const std::vector< BodyPart >& ZombieDismemberment::severedThisHit() const {
	return mSeveredThisHit;
}

bool ZombieDismemberment::isLost( BodyPart part ) const {
	int index = static_cast< int >( part );
	ASSERT( index >= 0 && index < static_cast< int >( BodyPart::Count ) && "ZombieDismemberment : bad part." );
	return mLost[ index ];
}

bool ZombieDismemberment::hasLostFooting() const {
	for ( BodyPart part : mCutParts ) {
		if ( mLayout->cut( part ).losesFooting ) {
			return true;
		}
	}
	return false;
}

void ZombieDismemberment::apply( std::vector< Matrix34 >* pose ) {
	for ( BodyPart part : mCutParts ) {
		const ZombieSeverLayout::Cut& cut = mLayout->cut( part );
		// 関節の位置は、潰す前に読む（関節の骨も潰す骨に入っている）
		Matrix34 collapsed = collapsedAt( PlacementMatrix::translationOf( ( *pose )[ cut.jointBone ] ) );
		for ( int bone : cut.bones ) {
			( *pose )[ bone ] = collapsed;
		}
	}
}

Matrix34 ZombieDismemberment::collapsedAt( const Vector3& point ) {
	Matrix34 collapsed;
	collapsed.setScaling( kCollapsedScale );
	collapsed.m03 = point.x;
	collapsed.m13 = point.y;
	collapsed.m23 = point.z;
	return collapsed;
}
