#include "HeldItem.h"
#include "StaticMeshData.h"
#include "SkinnedMeshInstance.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

HeldItem::HeldItem( const StaticMeshData* data, int boneIndex, const Matrix34& grip ) :
	mData( data ),
	mBoneIndex( boneIndex ),
	mGrip( grip ) {
	ASSERT( mData && "HeldItem : data is null." );
}

void HeldItem::draw( const SkinnedMeshInstance& holder ) const {
	Matrix34 world;
	if ( !holder.boneMatrix( mBoneIndex, &world ) ) {
		return;
	}
	world *= mGrip; // world = 骨 * 持ち方（持ち方で骨の空間へ置いてから、骨と一緒にワールドへ）
	mData->draw( world );
}
