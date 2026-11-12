#include "GameSrc/Render/Lighting/LightList.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Math/Functions.h"

#include <algorithm>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// LightList.hlsli の cbuffer LightConstants の番号（b2）。b1 は全画面のパス・キャラの骨、b3 は G-Buffer の材質値が使う
	const int kLightSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT + 1;
	// 影のマップの最初の番号（t6）。t2〜t5 は G-Buffer と深度（DeferredLighting.cpp）。LightList.hlsli と揃える
	const int kFirstShadowSlot = Graphics::Manager::FIRST_USER_TEXTURE_SLOT + 4;
	const int kMatrixFloats = 16;
	// 影のずらしと濃さの初期値（setShadowSettings を呼ぶまで。ShadowParams の初期値と同じ）
	const float kDefaultNormalOffsetTexels = 1.5f;
	const float kDefaultLightOffsetTexels = 1.f;
	const float kDefaultShadowStrength = 1.f;
	const float kPerspectiveFlag = 1.f;
	const float kOrthographicFlag = 0.f;

	// スポットの角度の幅が 0 のとき（inner == outer）の 0 割りよけ
	const float kMinSpotCosineWidth = 0.0001f;

	const int kVectorFloats = 4;
	// LightList.hlsli の LightData と同じ並び（float4 を 4 つ）
	struct LightData{
		float positionRange[ kVectorFloats ];   // xyz 位置  w 届く距離
		float directionKind[ kVectorFloats ];   // xyz 光の進む向き  w 種類（LightKind の番号）
		float colorIntensity[ kVectorFloats ];  // xyz 色（sRGB）  w 強さ
		float spotScaleOffset[ kVectorFloats ]; // x 角度の倍率  y 角度のずらし（Frostbite の式。LightList.hlsli）  z 影のマップの番号（無ければ −1）  w 影のマップの枚数
		float nearDistance[ kVectorFloats ];    // x 近さの下限（m）  yzw 使わない
	};
	// LightList.hlsli の cbuffer LightConstants と同じ並び
	struct LightConstants{
		float count[ kVectorFloats ];           // x 照らすライトの数  yzw 使わない
		float skyZenith[ kVectorFloats ];       // xyz 天頂の色（sRGB）  w 描く空の明るさ
		float skyHorizon[ kVectorFloats ];      // xyz 地平線の色（sRGB）  w 環境光の強さ
		float ground[ kVectorFloats ];          // xyz 地面の色（sRGB。下からの環境光）  w 使わない
		float fog[ kVectorFloats ];             // x 距離の霧の係数  y 高さの霧の濃さ（1/m）  z 高さで薄まる係数  w 使わない（fogConstants）
		float moonDirection[ kVectorFloats ];   // xyz 月の方への向き  w 見かけの半径（ラジアン）
		float moonColor[ kVectorFloats ];       // xyz 月の色（sRGB）  w 円板の明るさ（0 なら描かない）
		float moonHalo[ kVectorFloats ];        // x にじみの明るさ  y にじみの広がり（半径の倍数）  z 1 画素の角度（ラジアン）  w 使わない
		float shadowSettings[ kVectorFloats ];  // x 法線のずらし（画素）  y 光の方へのずらし（画素）  z 影の濃さ  w 使わない
		float cascadeEnds[ kVectorFloats ];     // xyz 月の CSM の各枚の奥の端（m）  w 次の枚と混ぜる割合
		float cascadeView[ kVectorFloats ];     // xyz カメラの向き（奥行きを測る向き）  w 使わない
		float shadowMatrices[ LightList::kMaxShadowMaps ][ kMatrixFloats ]; // 光の側の射影×ビュー（行ごと）
		float shadowTexels[ LightList::kMaxShadowMaps ][ kVectorFloats ];   // x 1 画素の大きさ（m）  y 1 なら透視（x は光から 1m の所）  zw 使わない
		LightData lights[ LightList::kMaxLights ];
	};
	static_assert( sizeof( LightData ) == sizeof( float ) * kVectorFloats * 5, "LightData : keep the HLSL layout." );

	void setVector( float* out, const Vector3& v, float w ) {
		out[ 0 ] = v.x;
		out[ 1 ] = v.y;
		out[ 2 ] = v.z;
		out[ 3 ] = w;
	}

	// 霧の値を、シェーダ（Fog.hlsli）がそのまま掛けられる係数へ直す
	//   x：距離 d に掛けて 2 乗すると濃さの合計になる係数 k = √(ln 2) / 半分の距離（半分の距離で exp(−1 × ln 2) = 0.5）
	//   z：高さ y に掛けて exp( −z × y ) が高さの霧の薄まり方になる係数 b = ln 2 / 半分の高さ
	const float kLn2 = 0.693147f;
	// 月の半径・1 画素の角度は、シェーダが acos の角度と比べるのでラジアンで渡す（GameLib の三角関数は度）
	const float kRadiansPerDegree = 3.14159265f / 180.f;
	Vector3 fogConstants( const EnvironmentLight& environment ) {
		float distanceScale = ( environment.fogHalfDistance > 0.f ) ? sqrt( kLn2 ) / environment.fogHalfDistance : 0.f;
		float heightFalloff = ( environment.fogHeightHalf > 0.f ) ? kLn2 / environment.fogHeightHalf : 0.f;
		return Vector3( distanceScale, environment.fogGroundDensity, heightFalloff );
	}

	// 平行光は、ほかのライトより先に必ず選ぶ（町全体を照らす月なので、消えると全部が暗くなる）
	const float kDirectionalPriority = -1.f;

	// 選ぶ順番の値（小さいほど先）。目からライトの届く範囲のふちまでの距離（範囲の中にいれば 0）
	float reachDistance( const LightSource& light, const Vector3& eye ) {
		if ( light.kind == LightKind::Directional ) {
			return kDirectionalPriority;
		}
		Vector3 d;
		d.setSub( light.position, eye );
		return max( 0.f, d.length() - light.range );
	}

	LightData toData( const LightSource& light ) {
		LightData data = {};
		setVector( data.positionRange, light.position, light.range );
		setVector( data.directionKind, light.direction, static_cast< float >( light.kind ) );
		setVector( data.colorIntensity, light.color, light.intensity );
		if ( light.kind == LightKind::Spot ) {
			// 中心からの角度の cos を、外側で 0・内側で 1 になる直線へ写す（saturate( cos × 倍率 + ずらし )）
			float cosOuter = cos( light.outerAngleDegree );
			float cosInner = cos( light.innerAngleDegree );
			float scale = 1.f / max( cosInner - cosOuter, kMinSpotCosineWidth );
			data.spotScaleOffset[ 0 ] = scale;
			data.spotScaleOffset[ 1 ] = -cosOuter * scale;
		}
		data.spotScaleOffset[ 2 ] = static_cast< float >( light.shadowMap );
		data.spotScaleOffset[ 3 ] = static_cast< float >( light.shadowMapCount );
		data.nearDistance[ 0 ] = light.nearDistance;
		return data;
	}
}

LightSource directionalLight( const Vector3& direction, const Vector3& color, float intensity ) {
	LightSource light;
	light.kind = LightKind::Directional;
	light.direction = direction;
	light.color = color;
	light.intensity = intensity;
	return light;
}

LightSource pointLight( const Vector3& position, const Vector3& color, float intensity, float range ) {
	ASSERT( range > 0.f && "LightList : range must be positive." );
	LightSource light;
	light.kind = LightKind::Point;
	light.position = position;
	light.color = color;
	light.intensity = intensity;
	light.range = range;
	return light;
}

LightSource spotLight( const Vector3& position, const Vector3& direction, const Vector3& color, float intensity, float range,
	float innerAngleDegree, float outerAngleDegree ) {
	ASSERT( range > 0.f && "LightList : range must be positive." );
	ASSERT( innerAngleDegree <= outerAngleDegree && "LightList : inner angle must not exceed outer angle." );
	LightSource light;
	light.kind = LightKind::Spot;
	light.position = position;
	light.direction = direction;
	light.color = color;
	light.intensity = intensity;
	light.range = range;
	light.innerAngleDegree = innerAngleDegree;
	light.outerAngleDegree = outerAngleDegree;
	return light;
}

LightList::LightList() :
	mShadowNormalOffset( kDefaultNormalOffsetTexels ),
	mShadowLightOffset( kDefaultLightOffsetTexels ),
	mShadowStrength( kDefaultShadowStrength ),
	mCascadeViewDirection( 0.f, 0.f, 1.f ),
	mCascadeBlendRatio( 0.f ) {
	for ( float& end : mCascadeEnds ) {
		end = 0.f;
	}
	for ( int i = 0; i < kMaxShadowMaps; ++i ) {
		mShadowTexelSizes[ i ] = 0.f;
		mShadowPerspective[ i ] = false;
	}
	mConstants = Graphics::ConstantBuffer::create( sizeof( LightConstants ) );
	mSelected.reserve( kMaxLights );
}

void LightList::clear() {
	mLights.clear();
	for ( Graphics::DepthTarget& depth : mShadowDepths ) {
		depth = Graphics::DepthTarget();
	}
}

void LightList::add( const LightSource& light ) {
	mLights.push_back( light );
}

void LightList::setEnvironment( const EnvironmentLight& environment ) {
	mEnvironment = environment;
}

void LightList::setShadowMap( int index, Graphics::DepthTarget depth, const Matrix44& projectionView, float texelSize, bool perspective ) {
	ASSERT( index >= 0 && index < kMaxShadowMaps && "LightList : shadow map index out of range." );
	mShadowDepths[ index ] = depth;
	mShadowMatrices[ index ] = projectionView;
	mShadowTexelSizes[ index ] = texelSize;
	mShadowPerspective[ index ] = perspective;
}

void LightList::setShadowSettings( float normalOffsetTexels, float lightOffsetTexels, float strength ) {
	mShadowNormalOffset = normalOffsetTexels;
	mShadowLightOffset = lightOffsetTexels;
	mShadowStrength = strength;
}

void LightList::setCascades( const Vector3& viewDirection, const float ( &cascadeEnds )[ kCascadeCount ], float blendRatio ) {
	mCascadeViewDirection = viewDirection;
	for ( int i = 0; i < kCascadeCount; ++i ) {
		mCascadeEnds[ i ] = cascadeEnds[ i ];
	}
	mCascadeBlendRatio = blendRatio;
}

void LightList::select( const Vector3& eye ) {
	mSelected.clear();
	for ( int i = 0; i < static_cast< int >( mLights.size() ); ++i ) {
		mSelected.push_back( i );
	}
	// 平行光 → 届く範囲のふちが目に近い順。同じなら足した順のまま（毎フレーム入れ替わってちらつかないように）
	std::stable_sort( mSelected.begin(), mSelected.end(), [ this, &eye ]( int a, int b ) {
		return reachDistance( mLights[ a ], eye ) < reachDistance( mLights[ b ], eye );
	} );
	if ( static_cast< int >( mSelected.size() ) > kMaxLights ) {
		mSelected.resize( kMaxLights );
	}
}

void LightList::bind( const Vector3& eye ) {
	select( eye );
	LightConstants constants = {};
	constants.count[ 0 ] = static_cast< float >( mSelected.size() );
	setVector( constants.skyZenith, mEnvironment.skyZenithColor, mEnvironment.skyIntensity );
	setVector( constants.skyHorizon, mEnvironment.skyHorizonColor, mEnvironment.ambientIntensity );
	setVector( constants.ground, mEnvironment.groundColor, 0.f );
	setVector( constants.fog, fogConstants( mEnvironment ), 0.f );
	setVector( constants.moonDirection, mEnvironment.moonDirection, mEnvironment.moonRadius * kRadiansPerDegree );
	setVector( constants.moonColor, mEnvironment.moonColor, mEnvironment.moonDiscIntensity );
	setVector( constants.moonHalo,
		Vector3( mEnvironment.moonHaloIntensity, mEnvironment.moonHaloFalloff, mEnvironment.pixelAngle * kRadiansPerDegree ), 0.f );
	setVector( constants.shadowSettings, Vector3( mShadowNormalOffset, mShadowLightOffset, mShadowStrength ), 0.f );
	static_assert( kCascadeCount == 3, "LightList : cascadeEnds holds three ends in xyz." );
	setVector( constants.cascadeEnds, Vector3( mCascadeEnds[ 0 ], mCascadeEnds[ 1 ], mCascadeEnds[ 2 ] ), mCascadeBlendRatio );
	setVector( constants.cascadeView, mCascadeViewDirection, 0.f );
	for ( int i = 0; i < kMaxShadowMaps; ++i ) {
		for ( int f = 0; f < kMatrixFloats; ++f ) {
			constants.shadowMatrices[ i ][ f ] = ( &mShadowMatrices[ i ].m00 )[ f ];
		}
		constants.shadowTexels[ i ][ 0 ] = mShadowTexelSizes[ i ];
		constants.shadowTexels[ i ][ 1 ] = mShadowPerspective[ i ] ? kPerspectiveFlag : kOrthographicFlag;
	}
	for ( int i = 0; i < static_cast< int >( mSelected.size() ); ++i ) {
		constants.lights[ i ] = toData( mLights[ mSelected[ i ] ] );
	}
	mConstants.set( &constants, sizeof( constants ) );
	Graphics::Manager::instance().setConstantBuffer( kLightSlot, mConstants );
}

void LightList::bindShadowMaps() const {
	Graphics::Manager gm = Graphics::Manager::instance();
	for ( int i = 0; i < kMaxShadowMaps; ++i ) {
		if ( mShadowDepths[ i ] ) {
			gm.setShaderResource( kFirstShadowSlot + i, mShadowDepths[ i ] );
		}
	}
}

void LightList::rebindConstants() const {
	Graphics::Manager::instance().setConstantBuffer( kLightSlot, mConstants );
}

void LightList::unbind() {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setConstantBuffer( kLightSlot, Graphics::ConstantBuffer() );
	for ( int i = 0; i < kMaxShadowMaps; ++i ) {
		gm.setShaderResource( kFirstShadowSlot + i, Graphics::DepthTarget() );
	}
}

const std::vector< LightSource >& LightList::lights() const {
	return mLights;
}

bool LightList::isSelected( int index ) const {
	return std::find( mSelected.begin(), mSelected.end(), index ) != mSelected.end();
}
