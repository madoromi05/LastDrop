#include "GameSrc/Render/LitQuadBatch.h"
#include "GameSrc/Render/ShaderConstants.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Vector2.h"
#include "GameLib/Math/Vector4.h"

#include <algorithm>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kLitShaderFile = "Resource/Shader/LitQuad.hlsl";
	const char* kGlowShaderFile = "Resource/Shader/GlowQuad.hlsl";
	const int kSoftConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT; // SoftQuad.hlsli の b1
	const float kNoSoftDistance = 0.f; // 薄くしない（SoftQuad.hlsli は深度を読まない）

	// SoftQuad.hlsli の cbuffer SoftQuadConstants と同じ並び（16 バイトずつの区切りに合わせる）
	struct SoftQuadConstants{
		float inverseProjectionView[ ShaderConstants::kMatrixFloats ]; // 行ごと（Matrix44 の m00..m33 の順）
		float viewDepthRow[ ShaderConstants::kRowFloats ];             // 射影×ビューの 4 行目（m30..m33）
		float softDistance;
		float unused[ 3 ];
	};
	static_assert( sizeof( SoftQuadConstants ) == sizeof( float ) * ( ShaderConstants::kMatrixFloats + ShaderConstants::kRowFloats * 2 ), "SoftQuadConstants : keep the HLSL layout." );

	// 板の4隅（左下・右下・右上・左上）。横・縦の向きに何倍進むか
	// UV は、横が負なら uv.left・正なら uv.right、縦が負（下）なら uv.bottom・正（上）なら uv.top（絵の左上が 0,0）
	const int kQuadCorners = 4;
	const float kCornerSign[ kQuadCorners ][ 2 ] = { { -1.f, -1.f }, { 1.f, -1.f }, { 1.f, 1.f }, { -1.f, 1.f } };
	// 4隅を三角形2つに分ける並び（隅の番号）
	const int kVerticesPerQuad = 6;
	const int kQuadTriangleOrder[ kVerticesPerQuad ] = { 0, 1, 2, 0, 2, 3 };
	const int kVerticesPerTriangle = 3;

	// 材質の設定（MeshMaterial::apply と同じ項目）。色と不透明度は絵と頂点の色で決めるので、ここは素通しにする
	const Vector3 kWhite( 1.f, 1.f, 1.f );      // 拡散色（絵の色をそのまま使う。明るさの倍率を掛けて渡す）
	const float kDefaultBrightness = 1.f;       // 明るさの倍率の初期値（絵と頂点の色のまま）
	const Vector3 kNoSpecular( 0.f, 0.f, 0.f ); // 反射のツヤを付けない
	const Vector3 kNoEmission( 0.f, 0.f, 0.f ); // 自分では光らない
	const float kOpaque = 1.f;                  // 材質としての透明度（MeshMaterial と同じ。不透明度はアルファで決める）

	// 隅 index の UV
	Vector2 cornerUv( int index, const QuadUv& uv ) {
		return Vector2( ( kCornerSign[ index ][ 0 ] < 0.f ) ? uv.left : uv.right, ( kCornerSign[ index ][ 1 ] < 0.f ) ? uv.bottom : uv.top );
	}

	float squaredDistance( const Vector3& a, const Vector3& b ) {
		Vector3 d;
		d.setSub( a, b );
		return d.dot( d );
	}
}

bool& softParticlesEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

LitQuadBatch::LitQuadBatch( int quadCapacity, Mode mode, float softDistance ) :
	mVertexBuffer( Graphics::VertexBuffer::create( quadCapacity * kVerticesPerQuad ) ),
	mVertexCapacity( quadCapacity * kVerticesPerQuad ),
	mMode( mode ),
	mBrightness( kDefaultBrightness ),
	mSoftDistance( softDistance ) {
	ASSERT( quadCapacity > 0 && "LitQuadBatch : quadCapacity must be positive." );
	ASSERT( softDistance >= 0.f && "LitQuadBatch : softDistance must not be negative." );
	mShader = Graphics::Shader::create( ( mode == MODE_LIT ) ? kLitShaderFile : kGlowShaderFile );
	mSoftConstants = Graphics::ConstantBuffer::create( sizeof( SoftQuadConstants ) );
	mVertices.reserve( mVertexCapacity );
	mCenters.reserve( quadCapacity );
	mOrder.reserve( quadCapacity );
}

void LitQuadBatch::add( const Vector3& center, const Vector3& right, const Vector3& up, const Vector3& normal, unsigned color,
	const QuadUv& uv ) {
	if ( static_cast< int >( mVertices.size() ) + kVerticesPerQuad > mVertexCapacity ) {
		ASSERT( false && "LitQuadBatch : capacity over." );
		return;
	}
	Vector3 corner[ kQuadCorners ];
	for ( int i = 0; i < kQuadCorners; ++i ) {
		corner[ i ] = center;
		corner[ i ].madd( right, kCornerSign[ i ][ 0 ] );
		corner[ i ].madd( up, kCornerSign[ i ][ 1 ] );
	}
	for ( int index : kQuadTriangleOrder ) {
		Graphics::Vertex vertex;
		vertex.mPosition = Vector4( corner[ index ].x, corner[ index ].y, corner[ index ].z );
		vertex.mNormal = normal;
		vertex.mColor = color;
		vertex.mUv = cornerUv( index, uv );
		mVertices.push_back( vertex );
	}
	mCenters.push_back( center );
}

void LitQuadBatch::setBrightness( float brightness ) {
	ASSERT( brightness >= 0.f && "LitQuadBatch : brightness must not be negative." );
	mBrightness = brightness;
}

void LitQuadBatch::copySortedFromBack( const Vector3& eye, Graphics::Vertex* locked ) {
	mOrder.clear();
	for ( int i = 0; i < static_cast< int >( mCenters.size() ); ++i ) {
		mOrder.push_back( i );
	}
	// 遠い板から描く（手前の板が後から重なる）。同じ距離なら、ためた順のまま（毎フレーム並びが入れ替わってちらつかないように）
	std::stable_sort( mOrder.begin(), mOrder.end(), [ this, &eye ]( int a, int b ) {
		return squaredDistance( mCenters[ a ], eye ) > squaredDistance( mCenters[ b ], eye );
	} );
	for ( int quad : mOrder ) {
		auto first = mVertices.begin() + quad * kVerticesPerQuad;
		locked = std::copy( first, first + kVerticesPerQuad, locked );
	}
}

void LitQuadBatch::draw( Graphics::Texture texture, const Matrix44& projectionView, const Vector3* eye ) {
	if ( mVertices.empty() ) {
		return;
	}
	Graphics::Vertex* locked = mVertexBuffer.lock();
	if ( eye ) {
		copySortedFromBack( *eye, locked );
	} else {
		std::copy( mVertices.begin(), mVertices.end(), locked );
	}
	mVertexBuffer.unlock( &locked );

	Graphics::Manager m = Graphics::Manager::instance();
	Matrix34 world;
	world.setIdentity();					// 頂点はワールド座標で書いてある
	m.setProjectionViewMatrix( projectionView );
	m.setWorldMatrix( world );
	m.setTexture( texture );				// 法線マップも外れる（setTexture の決まり）
	m.setShader( mShader );					// MODE_LIT は壁や地面と同じライトの一覧で照らす（LitQuad.hlsl）。MODE_GLOW は自分で光るのでライト無し（GlowQuad.hlsl）
	// MODE_GLOW は後ろの色に絵の色×アルファを足し、MODE_LIT は絵のアルファで後ろと混ぜる
	m.setBlendMode( ( mMode == MODE_GLOW ) ? Graphics::BLEND_ADDITIVE : Graphics::BLEND_LINEAR );
	setSoftConstants( projectionView );
	Vector3 diffuse;
	diffuse.setMul( kWhite, mBrightness );	// 頂点の色に掛かる（1 を超えると、1 より明るく描ける）
	m.setDiffuseColor( diffuse );
	m.setSpecularColor( kNoSpecular );
	m.setEmissionColor( kNoEmission );
	m.setTransparency( kOpaque );
	m.enableDepthTest( true );				// 壁の向こうの板は隠す
	m.enableDepthWrite( false );			// 板どうし・後から描く物を隠さない
	m.setCullMode( Graphics::CULL_NONE );
	m.setVertexBuffer( mVertexBuffer );
	m.draw( 0, static_cast< int >( mVertices.size() ) / kVerticesPerTriangle );

	// 物を描くときの設定へ戻す（GameScene::setupLights と同じ）
	m.setShader( Graphics::Shader() ); // 後から描く物（固定パイプライン）に残さない
	m.setConstantBuffer( kSoftConstantSlot, Graphics::ConstantBuffer() );
	m.setBlendMode( Graphics::BLEND_OPAQUE );
	m.enableDepthWrite( true );
	mVertices.clear();
	mCenters.clear();
}

void LitQuadBatch::setSoftConstants( const Matrix44& projectionView ) {
	SoftQuadConstants constants;
	Matrix44 inverse;
	inverse.setInverse( projectionView );
	ShaderConstants::copyMatrix44( inverse, constants.inverseProjectionView );
	ShaderConstants::copyDepthRow( projectionView, constants.viewDepthRow );
	constants.softDistance = softParticlesEnabled() ? mSoftDistance : kNoSoftDistance;
	constants.unused[ 0 ] = 0.f;
	constants.unused[ 1 ] = 0.f;
	constants.unused[ 2 ] = 0.f;
	mSoftConstants.set( &constants, sizeof( constants ) );
	Graphics::Manager::instance().setConstantBuffer( kSoftConstantSlot, mSoftConstants );
}
