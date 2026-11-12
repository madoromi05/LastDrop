#include "GameSrc/Effect/BloodSprayList.h"
#include "GameSrc/Render/VertexColor.h"
#include "GameSrc/Math/OrthoBasis.h"

#include "GameLib/Math/Functions.h"

#include <algorithm>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kTextureFile = "Resource/Texture/Effect/blood_drop.dds"; // tools/texgen/blood_gen.py で作る（TGA を作った後、tools/convert/texture_to_dds.ps1 で DDS にする）

	const int kCapacity = 16;			// 同時に出しておける回数。超えたら一番古い物から消す（フルオート毎秒 10 発・0.4 秒で消えるので、ふだんは 4 回分ほど）
	const unsigned kRandomSeed = 7623u;	// 見た目のばらつきに使うだけなので、毎回同じ種でよい

	// 血の色（暗い赤）。ライトが掛かる前の色。見た目を確かめる前の初期値
	const float kBloodRed = 0.35f;
	const float kBloodGreen = 0.02f;
	const float kBloodBlue = 0.02f;

	const float kSoftDistance = 0.15f;	// 壁・地面に刺さった所で滴を薄くし始める奥行きの差（m）。見た目を確かめる前の初期値
}

BloodSprayList::BloodSprayList() :
	mRandom( kRandomSeed ),
	mTexture( Graphics::Texture::create( kTextureFile, Graphics::TEXTURE_COLOR_SPACE_SRGB ) ),
	mQuads( kCapacity * BloodSpray::kMaxDropNumber, LitQuadBatch::MODE_LIT, kSoftDistance ) {
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
	// isReady を呼ぶと読み込みが進む（毎フレーム呼ぶ）。読めなかった絵では描かない
	if ( !mTexture.isReady() || mTexture.isError() || mSprays.empty() ) {
		return;
	}
	// 視線に直交する横と縦（長さ1）。滴の板をこの2本で張ると、画面に平行になる
	Vector3 right;
	Vector3 up;
	OrthoBasis::around( viewDirection, &right, &up );
	// 滴には面が無いので、カメラの方を向いた面として照らす（ImpactDustList と同じ）
	Vector3 towardCamera;
	towardCamera.setNeg( viewDirection );

	// 1回分の滴は、どれも同じ大きさ・濃さなので、板の大きさと色は1回分ごとに作る
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
