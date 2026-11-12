#include "GameSrc/Effect/BloodSpray.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"

#include <cmath>

using namespace GameLib::Math;

namespace{
	// 出てから消えるまで（ティック）。見た目を確かめる前の初期値
	const int kLifeTicks = GameTime::secondsToTicks( 0.4f );

	// 体に当たったときの滴の数。弾の抜ける側（体の向こう）へ多く、撃った側（こちら）へ少し
	// 頭など scale が大きいときは、どちらも scale 倍にする（合計が kMaxDropNumber を超えないこと）
	const int kExitDropNumber = 8;
	const int kEntryDropNumber = 4;
	static_assert( kExitDropNumber + kEntryDropNumber == BloodSpray::kBaseDropNumber, "kBaseDropNumber must be the drops at scale 1" );

	// 抜ける側の滴の速さの範囲（m/ティック）。弾の勢いで飛ぶので、撃った側より速い
	const float kExitMinSpeed = 2.f / GameTime::kTicksPerSecond;
	const float kExitMaxSpeed = 4.f / GameTime::kTicksPerSecond;
	// 撃った側の滴の速さの範囲（m/ティック）
	const float kEntryMinSpeed = 1.f / GameTime::kTicksPerSecond;
	const float kEntryMaxSpeed = 2.f / GameTime::kTicksPerSecond;
	// 向きのばらつき。飛ぶ向きの各軸に ±この値を足す。砂ぼこり（1.0）より狭くして、弾の向きへまとまって飛ばす
	const float kExitSpread = 0.5f;
	const float kEntrySpread = 0.6f;

	// 毎ティック速さに掛ける割合。滴は重いので、砂ぼこり（0.85）より減速しない
	const float kDragPerTick = 0.95f;
	// 落ちる加速度（m/ティック²）。本当の重力（9.8m/秒²）。滴は弧を描いて落ちる
	const float kGravityPerTick = 9.8f / ( GameTime::kTicksPerSecond * GameTime::kTicksPerSecond );

	// 板の幅の半分（m）。出た時 → 消える時（直径 4cm → 15cm。scale 倍する）
	const float kStartRadius = 0.02f;
	const float kEndRadius = 0.075f;
	// 出た時の不透明度。液体なので濃く始め、終わりにかけて薄くする
	const float kStartOpacity = 0.9f;

	// direction へ、各軸に ±spread のばらつきを足した向き（長さ1）へ、minSpeed〜maxSpeed の速さで飛ぶ速度
	Vector3 randomVelocity( const Vector3& direction, float spread, float minSpeed, float maxSpeed, std::mt19937* random ) {
		std::uniform_real_distribution< float > jitter( -spread, spread );
		std::uniform_real_distribution< float > speed( minSpeed, maxSpeed );
		Vector3 velocity( direction.x + jitter( *random ), direction.y + jitter( *random ), direction.z + jitter( *random ) );
		velocity.normalize();
		velocity *= speed( *random );
		return velocity;
	}

	int scaledCount( int count, float scale ) {
		return static_cast< int >( std::lround( static_cast< float >( count ) * scale ) );
	}
}

BloodSpray::BloodSpray( const Vector3& point, const Vector3& direction, float scale, std::mt19937* random ) :
	mDropNumber( 0 ),
	mScale( scale ),
	mAge( 0 ) {
	ASSERT( random && "BloodSpray : random is null." );
	ASSERT( scale > 0.f && scale <= kMaxScale && "BloodSpray : scale is out of range." );
	int exitNumber = scaledCount( kExitDropNumber, scale );
	int entryNumber = scaledCount( kEntryDropNumber, scale );
	ASSERT( exitNumber + entryNumber <= kMaxDropNumber && "BloodSpray : too many drops." );
	Vector3 back;
	back.setNeg( direction );
	for ( int i = 0; i < exitNumber; ++i ) {
		mPosition[ mDropNumber ] = point;
		mVelocity[ mDropNumber ] = randomVelocity( direction, kExitSpread, kExitMinSpeed, kExitMaxSpeed, random );
		++mDropNumber;
	}
	for ( int i = 0; i < entryNumber; ++i ) {
		mPosition[ mDropNumber ] = point;
		mVelocity[ mDropNumber ] = randomVelocity( back, kEntrySpread, kEntryMinSpeed, kEntryMaxSpeed, random );
		++mDropNumber;
	}
}

void BloodSpray::update() {
	if ( isFinished() ) {
		return;
	}
	for ( int i = 0; i < mDropNumber; ++i ) {
		mVelocity[ i ] *= kDragPerTick;
		mVelocity[ i ].y -= kGravityPerTick;
		mPosition[ i ] += mVelocity[ i ];
	}
	++mAge;
}

bool BloodSpray::isFinished() const {
	return mAge >= kLifeTicks;
}

int BloodSpray::dropNumber() const {
	return mDropNumber;
}

const Vector3& BloodSpray::dropPosition( int index ) const {
	ASSERT( index >= 0 && index < mDropNumber && "BloodSpray : index is out of range." );
	return mPosition[ index ];
}

float BloodSpray::dropRadius() const {
	// 出た直後に早く広がり、だんだんゆっくりになる（ImpactDust と同じ曲線）
	float remain = 1.f - progress();
	return ( kStartRadius + ( kEndRadius - kStartRadius ) * ( 1.f - remain * remain ) ) * mScale;
}

float BloodSpray::dropOpacity() const {
	// 終わり近くで急に薄くする（液体なので、途中までは濃いまま見せる）
	float t = progress();
	return kStartOpacity * ( 1.f - t * t );
}

float BloodSpray::progress() const {
	return static_cast< float >( mAge ) / kLifeTicks;
}
