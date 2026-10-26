#include "GameSrc/Scene/Game/ZombieHealthBars.h"
#include "GameSrc/Scene/Game/GameWorld.h"
#include "GameSrc/Player/FirstPersonCamera.h"
#include "GameSrc/Player/PlayerCharacter.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include "GameLib/Math/Vector2.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Vector4.h"
#include "GameLib/Math/Matrix34.h"

#include <cmath>
#include <cstdio>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── 出す条件 ──
	const float kMaxBarDistance = 30.f;  // これより遠いゾンビは出さない（m）
	const float kMaxPartDistance = 10.f; // 部位の数字はこれより近いときだけ（遠いと 10 個の数字が重なって読めない）

	// ── 全体のバー ──
	const float kBarAboveHead = 0.25f; // 頭の箱の上端からバーまでの高さ（m）
	// 画面の大きさ（画素）で固定する。遠くても読めるよう、距離で縮めない。
	const float kBarWidth = 120.f;
	const float kBarHeight = 8.f;
	const float kTextGap = 2.f;        // バーと文字の間（画素）
	const float kLineHeight = 20.f;    // 文字1行の高さ（画素。UiRenderer の UI_TEXT_NORMAL）

	// ── 色（当たり判定の表示は白い線だけ、の方針に合わせて最小限）──
	const unsigned kBarBackColor = VertexColor::pack( 0.f, 0.f, 0.f, 0.5f ); // 枠・背景
	const unsigned kBarFillColor = VertexColor::WHITE;
	const unsigned kDeadBarFillColor = VertexColor::pack( 0.5f, 0.5f, 0.5f ); // 倒れたゾンビの中身
	const unsigned kTextColor = VertexColor::WHITE;
	const unsigned kBrokenColor = VertexColor::pack( 1.f, 0.25f, 0.25f );    // 0 になった部位
	const float kBarBorder = 1.f; // 背景を中身より外へはみ出させる幅（枠に見せる。画素）
	const float kBorderSides = 2.f; // 枠は両側（左右・上下）に付く

	const int kTextSize = 32;
	const float kHalf = 0.5f;

	// 世界の位置を画面の位置（画素。左上が原点）へ直す。カメラの後ろなら false。
	bool toScreen( const Matrix44& projectionView, const Vector3& world, Vector2* out ) {
		Vector4 clip;
		projectionView.mul( &clip, world );
		if ( clip.w <= 0.f ) {
			return false;
		}
		// -1〜1（上が +1）の正規化した位置を、画素（下が +）へ。
		float ndcX = clip.x / clip.w;
		float ndcY = clip.y / clip.w;
		out->x = ( ndcX * kHalf + kHalf ) * static_cast< float >( Framework::instance().width() );
		out->y = ( kHalf - ndcY * kHalf ) * static_cast< float >( Framework::instance().height() );
		return true;
	}

	// -1〜1 の立方体を置く行列（HitboxSet::boxMatrix）から、箱の中心と、一番高い所の高さ。
	Vector3 boxCenter( const Matrix34& box ) {
		return Vector3( box.m03, box.m13, box.m23 );
	}
	float boxTop( const Matrix34& box ) {
		return box.m13 + std::fabs( box.m10 ) + std::fabs( box.m11 ) + std::fabs( box.m12 );
	}

	// part の箱の真上（バーを置く所）。その箱が置かれていなければ false。
	bool anchorAbove( const HitboxSet& boxes, BodyPart part, Vector3* out ) {
		for ( int i = 0; i < boxes.boxNumber(); ++i ) {
			Matrix34 box;
			if ( boxes.boxPart( i ) == part && boxes.boxMatrix( i, &box ) ) {
				*out = boxCenter( box );
				out->y = boxTop( box ) + kBarAboveHead;
				return true;
			}
		}
		return false;
	}

	// 頭の箱の真上（バーを置く所）。頭が千切れて無ければ胴の真上。どちらも置かれていなければ false。
	bool barAnchor( const HitboxSet& boxes, Vector3* out ) {
		return anchorAbove( boxes, BodyPart::Head, out ) || anchorAbove( boxes, BodyPart::Torso, out );
	}

	// 目から point までの間に、弾・視線を止める物（壁・建物・車・地面）があるか。キャラ（他のゾンビ）は隠す物に数えない。
	bool isBlocked( const GameWorld& world, const Vector3& eye, const Vector3& point ) {
		Vector3 delta;
		delta.setSub( point, eye );
		HitWorld::Hit hit;
		return world.hitWorld().raycast( eye, delta, world.player(), &hit ) && hit.blocker;
	}

	int rounded( float value ) {
		return static_cast< int >( std::lround( value ) );
	}

	const char* stateName( Zombie::State state ) {
		switch ( state ) {
			case Zombie::State::Alive:   return "Alive";
			case Zombie::State::Downed:  return "Downed";
			case Zombie::State::Falling: return "Falling";
			case Zombie::State::Corpse:  return "Corpse";
			case Zombie::State::Sinking: return "Sinking";
		}
		return "-";
	}

	const char* attackPhaseName( ZombieAttack::Phase phase ) {
		switch ( phase ) {
			case ZombieAttack::Phase::Ready:   return "Ready";
			case ZombieAttack::Phase::Reach:   return "Reach";
			case ZombieAttack::Phase::Hold:    return "Hold";
			case ZombieAttack::Phase::Recover: return "Recover";
		}
		return "-";
	}

	const char* chaseModeName( ZombieChase::Mode mode ) {
		switch ( mode ) {
			case ZombieChase::Mode::Wander: return "Wander";
			case ZombieChase::Mode::Chase:  return "Chase";
			case ZombieChase::Mode::Search: return "Search";
		}
		return "-";
	}
}

void ZombieHealthBars::add( const GameWorld& world, const FirstPersonCamera& camera, UiRenderer* renderer ) const {
	ASSERT( renderer && "ZombieHealthBars : renderer is null." );
	Matrix44 projectionView = camera.projectionViewMatrix();
	world.objects().forEach< Zombie >( [ & ]( const Zombie& zombie ) {
		addZombie( zombie, world, camera, projectionView, renderer );
	} );
}

void ZombieHealthBars::addZombie( const Zombie& zombie, const GameWorld& world, const FirstPersonCamera& camera,
const Matrix44& projectionView, UiRenderer* renderer ) const {
	Vector3 anchor;
	if ( !barAnchor( zombie.hitboxes(), &anchor ) ) {
		return;
	}
	Vector3 toAnchor;
	toAnchor.setSub( anchor, camera.eyePosition() );
	float distance = toAnchor.length();
	Vector2 screen;
	if ( distance > kMaxBarDistance || !toScreen( projectionView, anchor, &screen ) ||
		isBlocked( world, camera.eyePosition(), anchor ) ) {
		return;
	}

	// 全体のバー（背景を少し大きく描いて枠に見せ、その上に残りの割合だけ中身を描く）。
	const ZombieHealth& health = zombie.health();
	bool alive = zombie.isAlive(); // 倒れたまま生きている（Downed）も、生きているので白い中身
	UiRect back;
	back.left = screen.x - kBarWidth * kHalf - kBarBorder;
	back.top = screen.y - kBarHeight * kHalf - kBarBorder;
	back.width = kBarWidth + kBarBorder * kBorderSides;
	back.height = kBarHeight + kBarBorder * kBorderSides;
	renderer->addRect( back, kBarBackColor );
	UiRect fill;
	fill.left = screen.x - kBarWidth * kHalf;
	fill.top = screen.y - kBarHeight * kHalf;
	fill.width = kBarWidth * ( health.total() / health.maxTotal() );
	fill.height = kBarHeight;
	renderer->addRect( fill, alive ? kBarFillColor : kDeadBarFillColor );

	// 数字はバーの上、状態はバーの下。
	char text[ kTextSize ];
	std::snprintf( text, sizeof( text ), "HP %d/%d", rounded( health.total() ), rounded( health.maxTotal() ) );
	Vector2 above( screen.x, back.top - kTextGap - kLineHeight * kHalf );
	renderer->addText( text, above, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor );
	// 立っている間は、追ってくる AI の考え（Wander / Chase / Search）。よろけている・攻撃している間はそちらを出す。
	Zombie::State state = zombie.state();
	if ( alive && zombie.isStaggering() ) {
		std::snprintf( text, sizeof( text ), "Staggered" );
	} else if ( alive && zombie.attackPhase() != ZombieAttack::Phase::Ready ) {
		std::snprintf( text, sizeof( text ), "%sAttack %s", zombie.isCrawling() ? "Downed " : "",
			attackPhaseName( zombie.attackPhase() ) );
	} else if ( zombie.isCrawling() ) {
		// 体を引きずっている間は、Downed の後ろに追ってくる AI の考え（SummaryPlan/個別仕様/ZombieCrawl_Plan.txt）。
		std::snprintf( text, sizeof( text ), "%s %s", stateName( state ), chaseModeName( zombie.chaseMode() ) );
	} else if ( state == Zombie::State::Alive && zombie.chaseMode() == ZombieChase::Mode::Chase && world.player() ) {
		// 追っている間は自キャラまでの距離（足元どうし・水平）も出す。攻撃を始める間合いに入れているかを見るため。
		Vector3 toPlayer;
		toPlayer.setSub( world.player()->position(), zombie.position() );
		toPlayer.y = 0.f;
		std::snprintf( text, sizeof( text ), "%s %.2fm", chaseModeName( zombie.chaseMode() ), toPlayer.length() );
	} else if ( state == Zombie::State::Alive ) {
		std::snprintf( text, sizeof( text ), "%s", chaseModeName( zombie.chaseMode() ) );
	} else if ( state == Zombie::State::Downed || state == Zombie::State::Falling ) {
		std::snprintf( text, sizeof( text ), "%s", stateName( state ) );
	} else {
		std::snprintf( text, sizeof( text ), "%s %.1fs", stateName( state ), zombie.secondsUntilGone() );
	}
	Vector2 below( screen.x, back.bottom() + kTextGap + kLineHeight * kHalf );
	renderer->addText( text, below, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor );

	if ( distance <= kMaxPartDistance ) {
		addPartLabels( zombie, projectionView, renderer );
	}
}

void ZombieHealthBars::addPartLabels( const Zombie& zombie, const Matrix44& projectionView, UiRenderer* renderer ) const {
	const HitboxSet& boxes = zombie.hitboxes();
	const ZombieHealth& health = zombie.health();
	char text[ kTextSize ];
	for ( int i = 0; i < boxes.boxNumber(); ++i ) {
		Matrix34 box;
		Vector2 screen;
		if ( !boxes.boxMatrix( i, &box ) || !toScreen( projectionView, boxCenter( box ), &screen ) ) {
			continue;
		}
		BodyPart part = boxes.boxPart( i );
		bool broken = health.isBroken( part );
		if ( broken ) {
			std::snprintf( text, sizeof( text ), "0 BROKEN" );
		} else {
			std::snprintf( text, sizeof( text ), "%d", rounded( health.part( part ) ) );
		}
		renderer->addText( text, screen, UI_TEXT_NORMAL, UI_ALIGN_CENTER, broken ? kBrokenColor : kTextColor );
	}
}
