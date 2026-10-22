#include "GameSrc/Player/HeldMagazine.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Math/MatrixBlend.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

HeldMagazine::HeldMagazine( const StaticMeshData* data, int handBone ) :
	mData( data ),
	mHandBone( handBone ) {
	ASSERT( mData && "HeldMagazine : data is null." );
	mInHand.setIdentity();
}

void HeldMagazine::takeOut( const SkinnedMeshInstance& holder, const Matrix34& gun ) {
	Matrix34 hand;
	if ( !holder.boneMatrix( mHandBone, &hand ) ) {
		return;
	}
	// 手から見た弾倉 = 手の逆 × 銃（弾倉は銃に付いた場所にある）。
	Matrix34 toHand;
	toHand.setInverse( hand );
	mInHand.setMul( toHand, gun );
}

void HeldMagazine::draw( const SkinnedMeshInstance& holder, const Matrix34& gun, bool inHand, float seatWeight ) const {
	Matrix34 hand;
	if ( !inHand || !holder.boneMatrix( mHandBone, &hand ) ) {
		mData->draw( gun );
		return;
	}
	Matrix34 held;
	held.setMul( hand, mInHand );
	Matrix34 world;
	MatrixBlend::blend( held, gun, seatWeight, &world );
	mData->draw( world );
}
