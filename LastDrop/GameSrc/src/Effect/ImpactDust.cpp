#include "GameSrc/Effect/ImpactDust.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib::Math;

namespace{
	// 出てから消えるまで（ティック）。見た目を確かめる前の初期値。
	const int kLifeTicks = GameTime::secondsToTicks( 0.5f );
	// 粒を出す所を面から浮かせる量（m）。出たばかりの板が壁に半分めり込んで、切れて見えないように。
	const float kStartLift = 0.05f;
	// 粒の出る速さの範囲　飛ぶ距離）。
	const float kMinSpeed = 1.5f / GameTime::kTicksPerSecond;
	const float kMaxSpeed = 3.5f / GameTime::kTicksPerSecond;
	// 法線からのばらつき。法線の各軸に ±kSpread を足す。kSpread×√3 < 1 なので、粒は必ず面の外へ飛ぶ。
	const float kSpread = 1.0f;
	// 毎ティック速さに掛ける割合、飛ぶ距離。
	const float kDragPerTick = 0.85f;

	// 落ちる加速度（m/ティック²。1m/秒²）。細かい粉は空気でゆっくり落ちるので、本当の重力より弱くする。
	const float kGravityPerTick = 1.f / ( GameTime::kTicksPerSecond * GameTime::kTicksPerSecond );
	// 粒の外向き度を速さの範囲に当てはめる前に掛ける乗数。0〜1 の乱数を累乗すると小さい側に寄るので、
	// 遅い粒（中心近くで止まる）が多く、外へ遠く飛ぶ粒が少なくなる。大きいほど中心に集まる（1 で均等）。
	const float kSpeedBias = 2.f;
	// 粒の板の幅の半分（m）。出た時 → 消える時（中心の粒で直径 15cm → 60cm）。
	const float kStartRadius = 0.175f;
	const float kEndRadius = 0.3f;
	// 出た時の不透明度。全体の密度。
	const float kStartOpacity = 0.4f;
	// 一番外へ飛ぶ粒（外向き度 1）の濃さ・大きさを、中心の粒の何倍にするか。間の粒は外向き度で割り振る。
	// 外へ散った粉は広い所に薄く広がるので、薄く大きくする。
	const float kOuterOpacityRate = 0.2f;
	const float kOuterRadiusRate = 1.3f;

	const float kFullTurn = 360.f; // 粒の板の向きの範囲（度）

	// 外向き度 reach（0〜1）に合わせて、1 倍（中心）から rate 倍（一番外）までの間の倍率を返す。
	float rateByReach( float rate, float reach ) {
		return 1.f + ( rate - 1.f ) * reach;
	}
}

ImpactDust::ImpactDust( const Vector3& point, const Vector3& normal, std::mt19937* random ) :
	mAge( 0 ) {
	ASSERT( random && "ImpactDust : random is null." );
	std::uniform_real_distribution< float > spread( -kSpread, kSpread );
	std::uniform_real_distribution< float > unit( 0.f, 1.f );
	Vector3 start;
	start.setMadd( point, normal, kStartLift );
	for ( int i = 0; i < kParticleNumber; ++i ) {
		Vector3 direction( normal.x + spread( *random ), normal.y + spread( *random ), normal.z + spread( *random ) );
		direction.normalize();
		// 速い粒ほど遠くで止まる（減速が速さに比例するため）ので、速さの割合をそのまま外向き度にする。
		mReach[ i ] = GameLib::Math::pow( unit( *random ), kSpeedBias );
		mAngle[ i ] = unit( *random ) * kFullTurn;
		mPosition[ i ] = start;
		mVelocity[ i ].setMul( direction, kMinSpeed + ( kMaxSpeed - kMinSpeed ) * mReach[ i ] );
	}
}

void ImpactDust::update() {
	if ( isFinished() ) {
		return;
	}
	for ( int i = 0; i < kParticleNumber; ++i ) {
		mVelocity[ i ] *= kDragPerTick;
		mVelocity[ i ].y -= kGravityPerTick;
		mPosition[ i ] += mVelocity[ i ];
	}
	++mAge;
}

bool ImpactDust::isFinished() const {
	return mAge >= kLifeTicks;
}

const Vector3& ImpactDust::particlePosition( int index ) const {
	ASSERT( index >= 0 && index < kParticleNumber && "ImpactDust : index is out of range." );
	return mPosition[ index ];
}

float ImpactDust::particleRadius( int index ) const {
	ASSERT( index >= 0 && index < kParticleNumber && "ImpactDust : index is out of range." );
	// 出た直後に早く広がり、だんだんゆっくりになる（煙が広がる感じ）。
	float remain = 1.f - progress();
	float radius = kStartRadius + ( kEndRadius - kStartRadius ) * ( 1.f - remain * remain );
	return radius * rateByReach( kOuterRadiusRate, mReach[ index ] );
}

float ImpactDust::particleOpacity( int index ) const {
	ASSERT( index >= 0 && index < kParticleNumber && "ImpactDust : index is out of range." );
	return kStartOpacity * ( 1.f - progress() ) * rateByReach( kOuterOpacityRate, mReach[ index ] );
}

float ImpactDust::particleAngle( int index ) const {
	ASSERT( index >= 0 && index < kParticleNumber && "ImpactDust : index is out of range." );
	return mAngle[ index ];
}

float ImpactDust::progress() const {
	return static_cast< float >( mAge ) / kLifeTicks;
}
