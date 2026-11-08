#include "GameSrc/Zombie/Crawl/ZombieCrawler.h"
#include "GameSrc/Zombie/Crawl/ZombieCrawlLayout.h"
#include "GameSrc/Zombie/Damage/ZombieDismemberment.h"

#include "GameLib/GameLib.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 目の高さ（m。胸から）。伏せているので低い。ここから自キャラの目が見えているかを調べる。
	const float kEyeHeight = 0.4f;
	// 腕の往復の速さ（ZombieCrawl::update の pace）。追っている間は 1、気付いていない・追いついて待っている間はゆっくりもがく。
	const float kCrawlPace = 1.f;
	const float kStrugglePace = 0.3f;
	const float kStillPace = 0.f; // 攻撃している間・両腕を失った体は、往復を止める

	float paceOf( ZombieCrawler::Effort effort ) {
		switch ( effort ) {
			case ZombieCrawler::Effort::Crawl: return kCrawlPace;
			case ZombieCrawler::Effort::Struggle: return kStrugglePace;
			case ZombieCrawler::Effort::Still: break;
		}
		return kStillPace;
	}
}

ZombieCrawler::ZombieCrawler( const ZombieCrawlLayout* layout, const ZombieDismemberment* dismemberment ) :
	mLayout( layout ),
	mDismemberment( dismemberment ),
	mArms( layout ) {
	ASSERT( mLayout && mDismemberment && "ZombieCrawler : layout or dismemberment is null." );
}

ZombieCrawl* ZombieCrawler::arms() {
	return &mArms;
}

bool ZombieCrawler::isStarted() const {
	return mArms.isStarted();
}

bool ZombieCrawler::canPull() const {
	return hasArm( ZombieCrawlLayout::SIDE_LEFT ) || hasArm( ZombieCrawlLayout::SIDE_RIGHT );
}

bool ZombieCrawler::start( const SkinnedMeshInstance& mesh, const Vector3& footing, float* angleY ) {
	if ( !mBody.start( mesh, *mLayout, footing, angleY ) ) {
		return false;
	}
	mArms.start();
	return true;
}

const Vector3& ZombieCrawler::center() const {
	return mBody.center();
}

Vector3 ZombieCrawler::eye() const {
	Vector3 eye = mBody.center();
	eye.y += kEyeHeight;
	return eye;
}

float ZombieCrawler::moveArms( Effort effort, bool reaching, bool moving ) {
	mArms.update( paceOf( effort ), reaching );
	if ( !moving ) {
		return 0.f;
	}
	// 腕で引いた分だけ進む（引いていない腕・失った腕の分は進まない）。
	return mArms.pullDistance( hasArm( ZombieCrawlLayout::SIDE_LEFT ), hasArm( ZombieCrawlLayout::SIDE_RIGHT ) );
}

void ZombieCrawler::moveCenter( const Vector3& step ) {
	mBody.moveCenter( step );
}

void ZombieCrawler::place( float angleY, const CharacterMover& mover, SkinnedMeshInstance* mesh, Vector3* footing ) {
	mBody.place( angleY, mover, mesh, footing );
}

void ZombieCrawler::look( bool hasTarget, const Vector3& rootPoint ) {
	mArms.setLookTarget( hasTarget, rootPoint );
}

bool ZombieCrawler::hasArm( ZombieCrawlLayout::Side side ) const {
	// 前腕から先が無ければ引けない（上腕を失ったときは、その先の前腕も無い）。
	BodyPart foreArm = ( side == ZombieCrawlLayout::SIDE_LEFT ) ? BodyPart::LeftForeArm : BodyPart::RightForeArm;
	return !mDismemberment->isLost( foreArm );
}
