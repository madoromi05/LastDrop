#include "GameSrc/Zombie/Animation/ZombieRootMotionSpan.h"
#include "GameSrc/Zombie/Animation/ZombieRootMotion.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Collision/CharacterMover.h"

#include "GameLib/GameLib.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 区間を始めた直後の「最後に動かした時刻」。最初の apply で必ず当たり直す（アニメ時刻は 0 以上）。
	const float kNotAppliedTime = -1.f;
}

ZombieRootMotionSpan::ZombieRootMotionSpan( SkinnedMeshInstance* mesh, const CharacterMover* mover ) :
	mMesh( mesh ),
	mMover( mover ),
	mTable( nullptr ),
	mPass( Pass::None ),
	mAnchor( 0.f, 0.f, 0.f ),
	mStartOffset( 0.f, 0.f, 0.f ),
	mLastTime( kNotAppliedTime ),
	mMeshOffset( 0.f, 0.f, 0.f ) {
	ASSERT( mesh && "ZombieRootMotionSpan : mesh is null." );
	ASSERT( mover && "ZombieRootMotionSpan : mover is null." );
}

void ZombieRootMotionSpan::start( const ZombieRootMotion* table, float startTime, Pass pass, Vector3* footing ) {
	// 前の区間は閉じ、足元を今見えている絵の根へ戻してから、そこをアンカーにする。
	end( footing );
	if ( !table ) {
		return;
	}
	mTable = table;
	mPass = pass;
	mAnchor = *footing;
	mStartOffset = table->offsetAt( startTime );
	mLastTime = kNotAppliedTime;
}

void ZombieRootMotionSpan::apply( const ICollisionBody* passThrough, Vector3* footing ) {
	if ( !mTable ) {
		return;
	}
	float time = mMesh->animationTime();
	if ( time != mLastTime ) {
		// 始めた時刻からの根の水平の動きを、今の向きでワールドへ（攻撃中は相手の方へ向きを変えるので、毎回回し直す）。
		Vector3 rootDelta = mTable->offsetAt( time );
		rootDelta -= mStartOffset;
		Vector3 wanted;
		mMesh->rootMatrix().mul33( &wanted, rootDelta );
		wanted.y = 0.f;
		// 移動の箱はアンカーにある（bodyCenter）ので、毎回アンカーから当たり直す。1 ティックごとの差を足していくと、
		// 踏み込みを壁で止められた後の戻りで、始めた所より後ろへ下がってしまう。
		Vector3 moved( 0.f, 0.f, 0.f );
		switch ( mPass ) {
			case Pass::Target:    moved = mMover->movePassingThrough( wanted, passThrough ); break;
			case Pass::AllBodies: moved = mMover->moveIgnoringBodies( wanted ); break;
			case Pass::None:      moved = mMover->move( wanted ); break;
		}
		footing->x = mAnchor.x + moved.x;
		footing->z = mAnchor.z + moved.z;
		// 絵の根は、足元から根の動きの分だけ戻した所。当たらなければアンカーのまま（今までと同じ見た目）で、
		// 当たって進めなかった分だけ手前へ戻る。
		mMeshOffset.set( -wanted.x, 0.f, -wanted.z );
		mLastTime = time;
	}
	// 足元の高さは、腰の下（足元）の地面に合わせる（歩道の端で倒れても浮かない）。
	footing->y = mMover->followGround( *footing );
	mAnchor.y = footing->y;
	mMesh->setPosition( meshRoot( *footing ) );
}

void ZombieRootMotionSpan::end( Vector3* footing ) {
	if ( !mTable ) {
		return;
	}
	Vector3 root = meshRoot( *footing );
	footing->x = root.x;
	footing->z = root.z;
	mMeshOffset.set( 0.f, 0.f, 0.f );
	mTable = nullptr;
	mMesh->setPosition( *footing );
}

void ZombieRootMotionSpan::freeze() {
	mTable = nullptr;
}

void ZombieRootMotionSpan::moveFooting( const Vector3& moved, Vector3* footing ) {
	if ( mTable ) {
		mAnchor += moved;
	}
	*footing += moved;
}

bool ZombieRootMotionSpan::isActive() const {
	return mTable != nullptr;
}

const Vector3& ZombieRootMotionSpan::anchor() const {
	return mAnchor;
}

Vector3 ZombieRootMotionSpan::meshRoot( const Vector3& footing ) const {
	Vector3 root;
	root.setAdd( footing, mMeshOffset );
	return root;
}
