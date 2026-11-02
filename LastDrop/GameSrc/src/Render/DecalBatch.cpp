#include "GameSrc/Render/DecalBatch.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Graphics/Vertex.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Vector2.h"
#include "GameLib/Math/Vector4.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kShaderFile = "Resource/Shader/Decal.hlsl";
	const int kConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT; // Decal.hlsl の b1

	// 箱（1 辺 1・中心が原点）の 6 面。面ごとに外向き n と、面に沿う 2 本 u・v（u × v = n）。
	// 隅を c−u−v → c+u−v → c+u+v → c−u+v の順に並べると、三角形の表（GameLib の CULL_BACK で残る側）が外を向く
	// （メッシュの三角形と同じ向き。MeshMaterial::apply のコメント）。描くときは裏（箱の内側の面）だけ描く。
	const int kFaceNumber = 6;
	const Vector3 kFaceNormal[ kFaceNumber ] = {
		Vector3( 1.f, 0.f, 0.f ), Vector3( -1.f, 0.f, 0.f ), Vector3( 0.f, 1.f, 0.f ),
		Vector3( 0.f, -1.f, 0.f ), Vector3( 0.f, 0.f, 1.f ), Vector3( 0.f, 0.f, -1.f ) };
	const Vector3 kFaceU[ kFaceNumber ] = {
		Vector3( 0.f, 1.f, 0.f ), Vector3( 0.f, 0.f, 1.f ), Vector3( 0.f, 0.f, 1.f ),
		Vector3( 1.f, 0.f, 0.f ), Vector3( 1.f, 0.f, 0.f ), Vector3( 0.f, 1.f, 0.f ) };
	const Vector3 kFaceV[ kFaceNumber ] = {
		Vector3( 0.f, 0.f, 1.f ), Vector3( 0.f, 1.f, 0.f ), Vector3( 1.f, 0.f, 0.f ),
		Vector3( 0.f, 0.f, 1.f ), Vector3( 0.f, 1.f, 0.f ), Vector3( 1.f, 0.f, 0.f ) };
	const int kCornersPerFace = 4;
	const float kCornerSign[ kCornersPerFace ][ 2 ] = { { -1.f, -1.f }, { 1.f, -1.f }, { 1.f, 1.f }, { -1.f, 1.f } };
	const int kVerticesPerFace = 6;
	const int kFaceTriangleOrder[ kVerticesPerFace ] = { 0, 1, 2, 0, 2, 3 };
	const int kBoxVertexNumber = kFaceNumber * kVerticesPerFace;
	const int kVerticesPerTriangle = 3;
	const float kBoxHalf = 0.5f;
	const float kFull = 2.f; // 半分の長さ → 箱の 1 辺（行列の軸は「箱の中の 1 が何 m か」）

	const int kMatrixFloats = 16;
	const int kRowFloats = 4;
	const int kLocalRows = 3;
	// Decal.hlsl の cbuffer DecalConstants と同じ並び（16 バイトずつの区切りに合わせる）。
	struct DecalConstants{
		float inverseProjectionView[ kMatrixFloats ]; // 行ごと（Matrix44 の m00..m33 の順）
		float toLocal[ kLocalRows ][ kRowFloats ];    // ワールド → 箱の中の座標（行ごと）
		float right[ kRowFloats ];
		float up[ kRowFloats ];
		float facing[ kRowFloats ];                   // w が不透明度
		float uvRect[ kRowFloats ];                   // 絵のうち使う所（左・上・幅・高さ）
	};
	static_assert( sizeof( DecalConstants ) == sizeof( float ) * ( kMatrixFloats + kRowFloats * ( kLocalRows + 4 ) ), "DecalConstants : keep the HLSL layout." );

	// axis（箱の軸。長さは箱の 1 辺が何 m か）に沿った「箱の中の座標」を出す行：(axis / |axis|², −center・axis / |axis|²)。
	// 軸どうしが直交しているので、逆行列を作らずに内積だけで戻せる。
	void writeLocalRow( const Vector3& axis, const Vector3& center, float* row ) {
		const float inverseSquared = 1.f / axis.dot( axis );
		row[ 0 ] = axis.x * inverseSquared;
		row[ 1 ] = axis.y * inverseSquared;
		row[ 2 ] = axis.z * inverseSquared;
		row[ 3 ] = -center.dot( axis ) * inverseSquared;
	}

	void writeUnit( const Vector3& v, float w, float* row ) {
		Vector3 unit = v;
		unit.normalize();
		row[ 0 ] = unit.x;
		row[ 1 ] = unit.y;
		row[ 2 ] = unit.z;
		row[ 3 ] = w;
	}
}

bool& decalsEnabled() {
	static bool sEnabled = true; // 起動中ずっと 1 つ（F6 で書き換える）
	return sEnabled;
}

DecalBatch::DecalBatch( int capacity ) :
	mBox( Graphics::VertexBuffer::create( kBoxVertexNumber ) ),
	mCapacity( capacity ) {
	ASSERT( capacity > 0 && "DecalBatch : capacity must be positive." );
	mShader = Graphics::Shader::create( kShaderFile );
	mConstants = Graphics::ConstantBuffer::create( sizeof( DecalConstants ) );
	mDecals.reserve( capacity );
	Graphics::Vertex* v = mBox.lock();
	for ( int face = 0; face < kFaceNumber; ++face ) {
		for ( int index : kFaceTriangleOrder ) {
			Vector3 corner;
			corner.setMul( kFaceNormal[ face ], kBoxHalf );
			corner.madd( kFaceU[ face ], kCornerSign[ index ][ 0 ] * kBoxHalf );
			corner.madd( kFaceV[ face ], kCornerSign[ index ][ 1 ] * kBoxHalf );
			v->mPosition = Vector4( corner.x, corner.y, corner.z );
			v->mNormal = kFaceNormal[ face ];
			v->mColor = 0xffffffff;
			v->mUv = Vector2( 0.f, 0.f ); // 絵の位置はシェーダが深度から決める
			++v;
		}
	}
	mBox.unlock( &v );
}

void DecalBatch::add( const Vector3& center, const Vector3& right, const Vector3& up, const Vector3& normal, float halfDepth, float opacity,
const UvRect& uvRect ) {
	if ( static_cast< int >( mDecals.size() ) >= mCapacity ) {
		ASSERT( false && "DecalBatch : capacity over." );
		return;
	}
	mDecals.push_back( Decal{ center, right, up, normal, halfDepth, opacity, uvRect } );
}

void DecalBatch::draw( Graphics::Texture albedo, Graphics::Texture normalMap, const Matrix44& projectionView ) {
	if ( mDecals.empty() ) {
		return;
	}
	Graphics::Manager m = Graphics::Manager::instance();
	Matrix44 inverse;
	inverse.setInverse( projectionView );
	m.setProjectionViewMatrix( projectionView );
	m.setTexture( albedo );       // 法線マップも外れる（setTexture の決まり）
	m.setNormalMap( normalMap );
	m.setShader( mShader );
	// 絵のアルファで、G-Buffer の色と法線に混ぜる。G-Buffer のアルファはデータ（RT1 の a は動きの種類。Taa_Plan.txt 6 章）なので書き換えない
	// （BLEND_LINEAR は描画先のアルファを絵のアルファで上書きし、弾痕の下の壁が「手元」と読まれる）。
	m.setBlendMode( Graphics::BLEND_LINEAR_KEEP_ALPHA );
	m.enableDepthTest( false );               // 当たっているかはシェーダが深度から決める（目が箱の中でも描けるように）
	m.enableDepthWrite( false );
	m.setCullMode( Graphics::CULL_FRONT );    // 箱の裏（内側の面）だけ描く。表も描くと 1 画素に 2 回混ざる
	m.setVertexBuffer( mBox );
	for ( const Decal& decal : mDecals ) {
		setDecalConstants( decal, inverse );
		m.draw( 0, kBoxVertexNumber / kVerticesPerTriangle );
	}
	mDecals.clear();
	// 物を描くときの設定へ戻す（裏表は物を描く側が毎回決める。MeshMaterial::apply）。
	m.setShader( Graphics::Shader() );
	m.setConstantBuffer( kConstantSlot, Graphics::ConstantBuffer() );
	m.setBlendMode( Graphics::BLEND_OPAQUE );
	m.enableDepthTest( true );
	m.enableDepthWrite( true );
}

void DecalBatch::setDecalConstants( const Decal& decal, const Matrix44& inverseProjectionView ) {
	// 箱の 3 本の軸（箱の中の 1 が何 m か）。横・縦は半分の長さの 2 倍、厚さは面の前後に halfDepth ずつ。
	Vector3 axisX;
	axisX.setMul( decal.right, kFull );
	Vector3 axisY;
	axisY.setMul( decal.up, kFull );
	Vector3 axisZ;
	axisZ.setMul( decal.normal, decal.halfDepth * kFull );
	Matrix34 world;
	world.m00 = axisX.x; world.m01 = axisY.x; world.m02 = axisZ.x; world.m03 = decal.center.x;
	world.m10 = axisX.y; world.m11 = axisY.y; world.m12 = axisZ.y; world.m13 = decal.center.y;
	world.m20 = axisX.z; world.m21 = axisY.z; world.m22 = axisZ.z; world.m23 = decal.center.z;
	Graphics::Manager::instance().setWorldMatrix( world );

	DecalConstants constants;
	for ( int i = 0; i < kMatrixFloats; ++i ) {
		constants.inverseProjectionView[ i ] = ( &inverseProjectionView.m00 )[ i ];
	}
	writeLocalRow( axisX, decal.center, constants.toLocal[ 0 ] );
	writeLocalRow( axisY, decal.center, constants.toLocal[ 1 ] );
	writeLocalRow( axisZ, decal.center, constants.toLocal[ 2 ] );
	writeUnit( decal.right, 0.f, constants.right );
	writeUnit( decal.up, 0.f, constants.up );
	writeUnit( decal.normal, decal.opacity, constants.facing );
	constants.uvRect[ 0 ] = decal.uvRect.left;
	constants.uvRect[ 1 ] = decal.uvRect.top;
	constants.uvRect[ 2 ] = decal.uvRect.width;
	constants.uvRect[ 3 ] = decal.uvRect.height;
	mConstants.set( &constants, sizeof( constants ) );
	Graphics::Manager::instance().setConstantBuffer( kConstantSlot, mConstants );
}

const char* DecalBatch::errorMessage() const {
	return mShader.errorMessage();
}
