#include "GameSrc/Player/Camera/CameraShake.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	// トラウマが 1 秒に減る量。1.6/秒なので、攻撃された 0.8 は 0.5 秒、射撃の 0.45 は約 0.3 秒で消える。
	const float kTraumaDecayPerSecond = 1.6f;
	const float kTraumaDecayPerTick = kTraumaDecayPerSecond / GameTime::kTicksPerSecond;
	// トラウマ 1 のときの一番大きな角度（度）。ロールは真ん中（照準）を動かさないので、上下・左右より大きくしてよい。
	const float kMaxPitch = 1.2f;
	const float kMaxYaw = 1.2f;
	const float kMaxRoll = 2.5f;
	// 1 秒にノイズの格子をいくつ進むか（≒ 1 秒に向きが変わる回数）。大きいほど細かく震える。
	const float kNoisePerSecond = 18.f;
	const float kNoisePerTick = kNoisePerSecond / GameTime::kTicksPerSecond;
	// mPhase がこれを超えたら折り返す（float の精度が落ちる前に）。
	const int kPhaseWrap = 1024;

	// 上下・左右・ロールのノイズを別の列から読む（同じ列だと 3 つが同じ形で動いてしまう）。
	enum Channel{
		CHANNEL_PITCH,
		CHANNEL_YAW,
		CHANNEL_ROLL,
	};

	// 整数から −1〜1 の値を作る（同じ入力なら同じ値。決まった間隔に置く乱数の代わり）。
	// 掛ける数は、ビットをよく混ぜることが知られている整数ハッシュの定数（lowbias32 と黄金比）。意味のある量ではない。
	float latticeValue( unsigned seed, int channel, int index ) {
		unsigned h = seed ^ ( static_cast< unsigned >( index ) * 0x9E3779B1u ) ^ ( static_cast< unsigned >( channel ) * 0x85EBCA77u );
		h ^= h >> 16;
		h *= 0x7FEB352Du;
		h ^= h >> 15;
		h *= 0x846CA68Bu;
		h ^= h >> 16;
		const float kToUnit = 1.f / 4294967295.f;
		return static_cast< float >( h ) * kToUnit * 2.f - 1.f;
	}
}

CameraShake::CameraShake( int seed ) :
	mSeed( static_cast< unsigned >( seed ) ),
	mTrauma( 0.f ),
	mPhase( 0.f ),
	mLatticeBase( 0 ) {
}

void CameraShake::addTrauma( float amount, float limit ) {
	if ( mTrauma >= limit ) {
		return;
	}
	mTrauma = Math::min( limit, mTrauma + amount );
}

void CameraShake::update() {
	mTrauma = Math::max( 0.f, mTrauma - kTraumaDecayPerTick );
	mPhase += kNoisePerTick;
	if ( mPhase >= static_cast< float >( kPhaseWrap ) ) {
		mPhase -= static_cast< float >( kPhaseWrap );
		mLatticeBase += kPhaseWrap;
	}
}

float CameraShake::noise( int channel ) const {
	// 前後の格子の値を smoothstep でつなぐ（バリューノイズ）。乱数をそのまま使うとガタガタに跳ねるため。
	int index = static_cast< int >( mPhase );
	float t = ScalarMath::smoothStep( mPhase - static_cast< float >( index ) );
	float a = latticeValue( mSeed, channel, mLatticeBase + index );
	float b = latticeValue( mSeed, channel, mLatticeBase + index + 1 );
	return a + ( b - a ) * t;
}

float CameraShake::pitch() const {
	return mTrauma * mTrauma * kMaxPitch * noise( CHANNEL_PITCH );
}

float CameraShake::yaw() const {
	return mTrauma * mTrauma * kMaxYaw * noise( CHANNEL_YAW );
}

float CameraShake::roll() const {
	return mTrauma * mTrauma * kMaxRoll * noise( CHANNEL_ROLL );
}

float CameraShake::trauma() const {
	return mTrauma;
}
