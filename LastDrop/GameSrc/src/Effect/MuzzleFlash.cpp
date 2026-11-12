#include "GameSrc/Effect/MuzzleFlash.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 芯は 2×2 のます目に 4 つの形（tools/texgen/muzzle_flash_gen.py）。撃つたびに 1 つを選ぶ（コマ送りではない）
	// 2 枚とも、作った TGA を tools/convert/texture_to_dds.ps1 で DDS にした物を読む
	const char* kCoreTextureFile = "Resource/Texture/Effect/muzzle_flash_sheet.dds";
	const int kCoreShapeColumns = 2;
	const int kCoreShapeRows = 2;
	const int kCoreShapeNumber = kCoreShapeColumns * kCoreShapeRows;
	const char* kGlowTextureFile = "Resource/Texture/Effect/muzzle_glow.dds";
	const unsigned kRandomSeed = 5630u; // 見た目のばらつきに使うだけなので、毎回同じ種でよい

	// 光る長さ（ティック）。本物の発砲炎は 1/60 秒よりずっと短いので、撃ったティックだけ光らせる
	const int kLifeTicks = 5;

	// ── 光の芯 ── 見た目を確かめる前の初期値
	const float kFlashOffset = 0.02f;   // 銃口の先端から少し前に置く（銃身の先に半分埋まって見えないように）
	const float kCoreWidth = 0.07f;     // 幅（m）
	const float kSizeJitter = 0.2f;     // 大きさのばらつき（±割合）
	// 明るさの倍率。描画先（RGBA16F）に 1 を超える明るさで描いておき、ブルームが入ったときに光としてにじむようにする
	// 今は画面に出すときに 1 で切られるので、真ん中は白く飛んで見える
	const float kCoreBrightness = 4.f;
	const int kCoreQuadNumber = 1;
	const float kCoreSoftDistance = 0.1f; // 壁に刺さった所で薄くし始める奥行きの差（m）

	// ── にじみの板 ── 見た目を確かめる前の初期値
	const float kGlowWidth = 0.8f;      // 幅（m）
	const float kGlowOpacity = 0.45f;   // 濃さ
	const float kGlowBrightness = 1.2f;  // 明るさの倍率（芯と違い、1 のまま）
	const int kGlowQuadNumber = 1;
	const float kGlowSoftDistance = 0.4f; // 同上。板が大きいので、幅の半分にする

	// ── 覗いているとき ── 覗き具合 1 での倍率。照準のすぐ先が埋まらないように。にじみの板は大きいので強く絞る
	const float kAimCoreSize = 0.6f;
	const float kAimCoreOpacity = 0.7f;
	const float kAimGlowSize = 0.5f;
	const float kAimGlowOpacity = 0.4f;

	// ── 点光源 ── 光の主役。色はにじみの板と同じ（炎の赤みより白に寄せた暖色）。見た目を確かめる前の初期値
	// 強さは段階 3-a まで（強さ / 距離）と 1m 先で同じ明るさになる値のまま。今は距離の 2 乗で弱まり、届く距離で 0 になる
	// （1m 先で 1.5、2m 先で 0.37）
	const Vector3 kLightColor( 1.f, 0.75f, 0.45f );
	const float kLightOffset = 0.05f;   // 銃口の少し前に置く（銃身の中から照らして、銃が真っ暗にならないように）
	const float kLightIntensity = 1.5f;
	const float kLightRange = 8.f;      // 届く距離（m）。撃った瞬間に照らされて見えるのは、ふつう目の前の壁・ゾンビ・地面まで

	const float kHalf = 0.5f;           // 幅から、板の中心から辺までの長さにする

	// 板を画面に平行にするときの仮の「上」。視線と平行に近いときは、もう一方を使う（ImpactDustList::draw と同じ）
	const Vector3 kWorldUp( 0.f, 1.f, 0.f );
	const Vector3 kWorldRight( 1.f, 0.f, 0.f );
	const float kParallelLimit = 0.99f;

	// 覗き具合 aimWeight（0〜1）に合わせて、1 倍から aimRate 倍までの間の倍率を返す
	float rateByAim( float aimRate, float aimWeight ) {
		return 1.f + ( aimRate - 1.f ) * aimWeight;
	}

	// 画面に平行な、幅 width の丸い板を1枚ためる。right / up: 視線に直交する横と縦（長さ1）
	// uv: 貼る絵の範囲（省けば絵 1 枚まるごと）
	void addFacingQuad( LitQuadBatch* quads, const Vector3& center, const Vector3& right, const Vector3& up,
		const Vector3& towardCamera, float width, unsigned color, const QuadUv& uv = QuadUv() ) {
		Vector3 halfRight;
		halfRight.setMul( right, width * kHalf );
		Vector3 halfUp;
		halfUp.setMul( up, width * kHalf );
		quads->add( center, halfRight, halfUp, towardCamera, color, uv ); // 加算で描くので、照らす向きは使われない
	}
}

MuzzleFlash::MuzzleFlash() :
	mAge( kLifeTicks ), // 消えている
	mScale( 1.f ),
	mShape( 0 ),
	mRandom( kRandomSeed ),
	mCoreTexture( Graphics::Texture::create( kCoreTextureFile, Graphics::TEXTURE_COLOR_SPACE_SRGB ) ),
	mCoreFrames( kCoreShapeColumns, kCoreShapeRows, kCoreShapeNumber ),
	mGlowTexture( Graphics::Texture::create( kGlowTextureFile, Graphics::TEXTURE_COLOR_SPACE_SRGB ) ),
	mCoreQuads( kCoreQuadNumber, LitQuadBatch::MODE_GLOW, kCoreSoftDistance ),
	mGlowQuads( kGlowQuadNumber, LitQuadBatch::MODE_GLOW, kGlowSoftDistance ) {
	mCoreQuads.setBrightness( kCoreBrightness );
	mGlowQuads.setBrightness( kGlowBrightness );
}

void MuzzleFlash::fire() {
	std::uniform_real_distribution< float > scale( 1.f - kSizeJitter, 1.f + kSizeJitter );
	std::uniform_int_distribution< int > shape( 0, kCoreShapeNumber - 1 );
	mAge = 0;
	mScale = scale( mRandom );
	mShape = shape( mRandom );
}

void MuzzleFlash::update() {
	if ( isLit() ) {
		++mAge;
	}
}

bool MuzzleFlash::isLit() const {
	return mAge < kLifeTicks;
}

bool MuzzleFlash::light( const Vector3& muzzlePosition, const Vector3& muzzleDirection, LightSource* out ) const {
	ASSERT( out && "MuzzleFlash : out is null." );
	if ( !isLit() ) {
		return false;
	}
	Vector3 position;
	position.setMadd( muzzlePosition, muzzleDirection, kLightOffset );
	*out = pointLight( position, kLightColor, kLightIntensity, kLightRange );
	return true;
}

void MuzzleFlash::draw( const Matrix44& projectionView, const Vector3& viewDirection,
	const Vector3& muzzlePosition, const Vector3& muzzleDirection, float aimWeight ) {
	if ( !isLit() ) {
		return;
	}
	Vector3 center;
	center.setMadd( muzzlePosition, muzzleDirection, kFlashOffset );
	// 視線に直交する横と縦（長さ1）。板をこの2本で張ると、画面に平行になる。どちらの板も丸いので回さない
	const Vector3& helper = ( Math::abs( viewDirection.dot( kWorldUp ) ) > kParallelLimit ) ? kWorldRight : kWorldUp;
	Vector3 right;
	right.setCross( helper, viewDirection );
	right.normalize();
	Vector3 up;
	up.setCross( viewDirection, right );
	Vector3 towardCamera;
	towardCamera.setNeg( viewDirection );

	// isReady を呼ぶと読み込みが進む。読めなかった絵では描かない（芯とにじみの板は別々に判断する）
	if ( mCoreTexture.isReady() && !mCoreTexture.isError() ) {
		float width = kCoreWidth * mScale * rateByAim( kAimCoreSize, aimWeight );
		unsigned color = VertexColor::pack( 1.f, 1.f, 1.f, rateByAim( kAimCoreOpacity, aimWeight ) ); // 色は絵に入っている
		addFacingQuad( &mCoreQuads, center, right, up, towardCamera, width, color, mCoreFrames.frameUv( mShape ) );
		mCoreQuads.draw( mCoreTexture, projectionView );
	}
	if ( mGlowTexture.isReady() && !mGlowTexture.isError() ) {
		float width = kGlowWidth * rateByAim( kAimGlowSize, aimWeight );
		float opacity = kGlowOpacity * rateByAim( kAimGlowOpacity, aimWeight );
		unsigned color = VertexColor::pack( kLightColor.x, kLightColor.y, kLightColor.z, opacity );
		addFacingQuad( &mGlowQuads, center, right, up, towardCamera, width, color );
		mGlowQuads.draw( mGlowTexture, projectionView );
	}
}
