#include "GameSrc/Effect/ImpactDustList.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/Math/Functions.h"

#include <algorithm>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// tools/texgen/smoke_flipbook_gen.py で作る（4×4 のます目に 16 コマ）。その TGA を tools/convert/texture_to_dds.ps1 で DDS にした物を読む
	const char* kTextureFile = "Resource/Texture/Effect/impact_smoke_sheet.dds";
	const int kFrameColumns = 4;
	const int kFrameRows = 4;
	const int kFrameNumber = kFrameColumns * kFrameRows;

	// 同時に出しておける回数。超えたら一番古い物から消す（フルオート毎秒 10 発・0.5 秒で消えるので、ふだんは 5 回分ほど）
	const int kCapacity = 16;
	const unsigned kRandomSeed = 7622u; // 見た目のばらつきに使うだけなので、毎回同じ種でよい

	// 粉の色（明るい砂の色）。ライトが掛かる前の色。材質では分けない。見た目を確かめる前の初期値
	const float kDustRed = 0.78f;
	const float kDustGreen = 0.74f;
	const float kDustBlue = 0.66f;

	// 板を画面に平行にするときの仮の「上」。真上・真下を見ているときは、もう一方を使う（Bullet::draw と同じ）
	const Vector3 kWorldUp( 0.f, 1.f, 0.f );
	const Vector3 kWorldRight( 1.f, 0.f, 0.f );
	const float kParallelLimit = 0.99f;

	// 壁・地面に刺さった所で粒を薄くし始める奥行きの差（m）。見た目を確かめる前の初期値
	const float kSoftDistance = 0.3f;
}

ImpactDustList::ImpactDustList() :
	mRandom( kRandomSeed ),
	mTexture( Graphics::Texture::create( kTextureFile, Graphics::TEXTURE_COLOR_SPACE_SRGB ) ),
	mFrames( kFrameColumns, kFrameRows, kFrameNumber ),
	mQuads( kCapacity * ImpactDust::kParticleNumber, LitQuadBatch::MODE_LIT, kSoftDistance ) {
	mDusts.reserve( kCapacity );
}

void ImpactDustList::spawn( const Vector3& point, const Vector3& normal ) {
	if ( static_cast< int >( mDusts.size() ) >= kCapacity ) {
		mDusts.erase( mDusts.begin() ); // 一番古い物
	}
	mDusts.emplace_back( point, normal, &mRandom );
}

void ImpactDustList::update() {
	for ( ImpactDust& dust : mDusts ) {
		dust.update();
	}
	mDusts.erase( std::remove_if( mDusts.begin(), mDusts.end(),
		[]( const ImpactDust& dust ) { return dust.isFinished(); } ), mDusts.end() );
}

void ImpactDustList::draw( const Matrix44& projectionView, const Vector3& viewDirection, const Vector3& eye ) {
	// isReady を呼ぶと読み込みが進む（毎フレーム呼ぶ）。読めなかった絵では描かない
	if ( !mTexture.isReady() || mTexture.isError() || mDusts.empty() ) {
		return;
	}
	// 視線に直交する横と縦（長さ1）。粒の板をこの2本で張ると、画面に平行になる
	const Vector3& helper = ( Math::abs( viewDirection.dot( kWorldUp ) ) > kParallelLimit ) ? kWorldRight : kWorldUp;
	Vector3 right;
	right.setCross( helper, viewDirection );
	right.normalize();
	Vector3 up;
	up.setCross( viewDirection, right );
	// 粒には面が無いので、カメラの方を向いた面として照らす（こちらから見える側に当たる光の明るさになる）
	Vector3 towardCamera;
	towardCamera.setNeg( viewDirection );

	// 粒ごとに濃さと大きさ・向きが違う（外へ飛ぶ粒ほど薄く大きい）ので、色と板の大きさは粒ごとに作る
	// コマは 1 回分の中で同じ（同じ時に出た粒は、同じだけ育っている）
	for ( const ImpactDust& dust : mDusts ) {
		QuadUv frame = mFrames.frameUv( mFrames.frameAt( dust.progress() ) );
		for ( int i = 0; i < ImpactDust::kParticleNumber; ++i ) {
			unsigned color = VertexColor::pack( kDustRed, kDustGreen, kDustBlue, dust.particleOpacity( i ) );
			float radius = dust.particleRadius( i );
			// 板の横と縦を、視線の周りに粒の向きだけ回す（どちらも視線に直交したまま）
			float cosine = Math::cos( dust.particleAngle( i ) );
			float sine = Math::sin( dust.particleAngle( i ) );
			Vector3 halfRight;
			halfRight.setMul( right, cosine * radius );
			halfRight.madd( up, sine * radius );
			Vector3 halfUp;
			halfUp.setMul( up, cosine * radius );
			halfUp.madd( right, -sine * radius );
			mQuads.add( dust.particlePosition( i ), halfRight, halfUp, towardCamera, color, frame );
		}
	}
	mQuads.draw( mTexture, projectionView, &eye );
}
