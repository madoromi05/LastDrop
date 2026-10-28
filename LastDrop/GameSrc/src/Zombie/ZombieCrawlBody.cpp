#include "GameSrc/Zombie/ZombieCrawlBody.h"
#include "GameSrc/Zombie/ZombieCrawlLayout.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Collision/CharacterMover.h"
#include "GameSrc/Math/AngleMath.h"
#include "GameSrc/Math/PlacementMatrix.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib;
using namespace GameLib::Math;

ZombieCrawlBody::ZombieCrawlBody() :
	mCenter( 0.f, 0.f, 0.f ),
	mChestOffset( 0.f, 0.f, 0.f ),
	mYawOffset( 0.f ) {
}

bool ZombieCrawlBody::start( const SkinnedMeshInstance& mesh, const ZombieCrawlLayout& layout, const Vector3& footing, float* angleY ) {
	ASSERT( angleY && "ZombieCrawlBody : angleY is null." );
	Matrix34 hips;
	Matrix34 chest;
	Matrix34 neck;
	if ( !mesh.animatedBoneMatrix( layout.hips(), &hips ) || !mesh.animatedBoneMatrix( layout.chest(), &chest ) ||
		!mesh.animatedBoneMatrix( layout.neck(), &neck ) ) {
		return false; // まだ update していない（倒れ終わってから呼ぶので起きない）
	}
	// 骨の位置を、置き場所と向きを掛ける前（骨格の根の空間）へ戻して測る。
	Matrix34 worldToRoot;
	worldToRoot.setInverse( mesh.rootMatrix() );
	Vector3 hipsRoot;
	Vector3 chestRoot;
	Vector3 neckRoot;
	worldToRoot.mul( &hipsRoot, PlacementMatrix::translationOf( hips ) );
	worldToRoot.mul( &chestRoot, PlacementMatrix::translationOf( chest ) );
	worldToRoot.mul( &neckRoot, PlacementMatrix::translationOf( neck ) );
	Vector3 bodyDirection;
	bodyDirection.setSub( neckRoot, hipsRoot );
	mYawOffset = AngleMath::headingOf( bodyDirection );
	mChestOffset.set( chestRoot.x, 0.f, chestRoot.z );
	// 見た目の向き（今の angleY）は変えず、進む向きを体の向きへ。胸の位置も今の見た目から求める（どちらも見た目は飛ばない）。
	Vector3 chestOffset;
	mesh.rootMatrix().mul33( &chestOffset, mChestOffset );
	mCenter.setAdd( footing, chestOffset );
	*angleY = AngleMath::wrapSigned( *angleY + mYawOffset );
	return true;
}

void ZombieCrawlBody::moveCenter( const Vector3& step ) {
	mCenter += step;
}

void ZombieCrawlBody::place( float angleY, const CharacterMover& mover, SkinnedMeshInstance* mesh, Vector3* footing ) {
	ASSERT( mesh && footing && "ZombieCrawlBody : mesh or footing is null." );
	mesh->setAngleY( AngleMath::wrapSigned( angleY - mYawOffset ) );
	mCenter.y = mover.followGround( mCenter );
	// 足元は、胸から見た目の向きで戻した所（胸を中心に向きを変えるので、向きを変えると足元が回る）。
	Vector3 chestOffset;
	mesh->rootMatrix().mul33( &chestOffset, mChestOffset );
	footing->setSub( mCenter, chestOffset );
	footing->y = mCenter.y;
	mesh->setPosition( *footing );
}

const Vector3& ZombieCrawlBody::center() const {
	return mCenter;
}
