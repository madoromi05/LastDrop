#include "GameSrc/Effect/BloodSprayList.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/Math/Functions.h"

#include <algorithm>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kTextureFile = "Resource/blood_drop.tga"; // tools/blood_gen.py で作る

	// 同時に出しておける回数。超えたら一番古い物から消す（フルオート毎秒 10 発・0.4 秒で消えるので、ふだんは 4 回分ほど）。
	const int kCapacity = 16;
	const unsigned kRandomSeed = 7623u; // 見た目のばらつきに使うだけなので、毎回同じ種でよい

	// 血の色（暗い赤。2026-10-18 ユーザー選択）。ライトが掛かる前の色。見た目を確かめる前の初期値。
	const float kBloodRed = 0.35f;
	const float kBloodGreen = 0.02f;
	const float kBloodBlue = 0.02f;

	// 板を画面に平行にするときの仮の「上」。真上・真下を見ているときは、もう一方を使う（ImpactDustList と同じ）。
	const Vector3 kWorldUp( 0.f, 1.f, 0.f );
	const Vector3 kWorldRight( 1.f, 0.f, 0.f );
	const float kParallelLimit = 0.99f;
}

BloodSprayList::BloodSprayList() :
	mRandom( kRandomSeed ),
	mTexture( Graphics::Texture::create( kTextureFile, Graphics::TEXTURE_COLOR_SPACE_SRGB ) ),
	mQuads( kCapacity * BloodSpray::kMaxDropNumber ) {
	mSprays.reserve( kCapacity );
}

void BloodSprayList::spawn( const Vector3& point, const Vector3& direction, float scale ) {
	if ( static_cast< int >( mSprays.size() ) >= kCapacity ) {
		mSprays.erase( mSprays.begin() ); // 一番古い物
	}
	mSprays.emplace_back( point, direction, scale, &mRandom );
}

void BloodSprayList::update() {
	for ( BloodSpray& spray : mSprays ) {
		spray.update();
	}
	mSprays.erase( std::remove_if( mSprays.begin(), mSprays.end(),
		[]( const BloodSpray& spray ) { return spray.isFinished(); } ), mSprays.end() );
}

void BloodSprayList::draw( const Matrix44& projectionView, const Vector3& viewDirection, const Vector3& eye ) {
	// isReady を呼ぶと読み込みが進む（毎フレーム呼ぶ）。読めなかった絵では描かない。
	if ( !mTexture.isReady() || mTexture.isError() || mSprays.empty() ) {
		return;
	}
	// 視線に直交する横と縦（長さ1）。滴の板をこの2本で張ると、画面に平行になる。
	const Vector3& helper = ( Math::abs( viewDirection.dot( kWorldUp ) ) > kParallelLimit ) ? kWorldRight : kWorldUp;
	Vector3 right;
	right.setCross( helper, viewDirection );
	right.normalize();
	Vector3 up;
	up.setCross( viewDirection, right );
	// 滴には面が無いので、カメラの方を向いた面として照らす（ImpactDustList と同じ）。
	Vector3 towardCamera;
	towardCamera.setNeg( viewDirection );

	// 1回分の滴は、どれも同じ大きさ・濃さなので、板の大きさと色は1回分ごとに作る。
	for ( const BloodSpray& spray : mSprays ) {
		unsigned color = VertexColor::pack( kBloodRed, kBloodGreen, kBloodBlue, spray.dropOpacity() );
		Vector3 halfRight;
		halfRight.setMul( right, spray.dropRadius() );
		Vector3 halfUp;
		halfUp.setMul( up, spray.dropRadius() );
		for ( int i = 0; i < spray.dropNumber(); ++i ) {
			mQuads.add( spray.dropPosition( i ), halfRight, halfUp, towardCamera, color );
		}
	}
	mQuads.draw( mTexture, projectionView, &eye );
}
