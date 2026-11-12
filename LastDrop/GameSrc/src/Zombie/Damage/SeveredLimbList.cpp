#include "GameSrc/Zombie/Damage/SeveredLimbList.h"

#include <algorithm>

using namespace GameLib::Math;

namespace{
	// 同時に出しておける数。ゾンビ 1 体で腕 2・脚 2・頭 1 の 5 つまでなので、4〜5 体ぶん
	const int kCapacity = 24;
	const unsigned kRandomSeed = 4517u; // 見た目のばらつきに使うだけなので、毎回同じ種でよい
}

SeveredLimbList::SeveredLimbList() :
	mRandom( kRandomSeed ) {
	mLimbs.reserve( kCapacity );
}

void SeveredLimbList::spawn( const SkinnedMeshData* data, const ZombieSeverLayout::Cut& cut, const std::vector< Matrix34 >& worldPose,
const Vector3& direction ) {
	if ( static_cast< int >( mLimbs.size() ) >= kCapacity ) {
		mLimbs.erase( mLimbs.begin() ); // 一番古い物
	}
	mLimbs.push_back( std::make_unique< SeveredLimb >( data, cut, worldPose, direction, &mRandom ) );
}

void SeveredLimbList::update( const IRayBlocker* ground ) {
	for ( const auto& limb : mLimbs ) {
		limb->update( ground );
	}
	mLimbs.erase( std::remove_if( mLimbs.begin(), mLimbs.end(),
		[]( const std::unique_ptr< SeveredLimb >& limb ) { return limb->isGone(); } ), mLimbs.end() );
}

void SeveredLimbList::draw( const DrawCulling& culling ) const {
	for ( const auto& limb : mLimbs ) {
		limb->draw( culling );
	}
}
