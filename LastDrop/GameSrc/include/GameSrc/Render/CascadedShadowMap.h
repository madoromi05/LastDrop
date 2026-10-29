#ifndef INCLUDED_CASCADED_SHADOW_MAP_H
#define INCLUDED_CASCADED_SHADOW_MAP_H

#include "GameSrc/Render/ShadowMap.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Vector3.h"
#include <memory>

// 月（平行光）の影の、カメラからの距離で分けた 3 枚の影のマップ（CSM。Shadow_Plan.txt 6 章）。
// 近い枚ほど狭い範囲を同じ画素の数で描くので、手前の影は細かく、遠くの影は粗い。
// 各枚は「その距離の範囲に映る所を包む球」を、光の向きから正射影で見る。球の大きさはカメラの画角だけで決まるので、
// カメラを回しても枚の大きさが変わらない。さらに枚の中心を影のマップの 1 画素の単位で丸め、カメラが動いても影の縁が泳がないようにする。
// 使い方：update（カメラと光の向き）→ 枚ごとに map( i ).begin( projectionView( i ) ) → 影を落とす物を描く → end。
// 作るのは Graphics::Manager ができた後（シーンの中）。
class CascadedShadowMap{
public:
	static const int kCascadeCount = 3;

	// size：1 枚の影のマップの一辺の画素の数。
	explicit CascadedShadowMap( int size );
	CascadedShadowMap( const CascadedShadowMap& ) = delete;
	CascadedShadowMap& operator=( const CascadedShadowMap& ) = delete;

	// 3 枚の行列を決める。eye / viewDirection：カメラの目と向き（長さ 1）。fieldOfViewY：縦の画角（度）。aspect：横 / 縦。
	// lightDirection：光の進む向き（長さ 1）。cascadeEnds：各枚の受け持ちの奥の端（カメラの向きに沿った距離 m。小さい順）。
	// casterMargin：枚の球より光の側へ、影を落とす物を拾う長さ（m。球の外の高い建物の影も入るように）。
	void update( const GameLib::Math::Vector3& eye, const GameLib::Math::Vector3& viewDirection, float fieldOfViewY, float aspect,
		const GameLib::Math::Vector3& lightDirection, const float ( &cascadeEnds )[ kCascadeCount ], float casterMargin );

	ShadowMap& map( int cascade );
	const GameLib::Math::Matrix44& projectionView( int cascade ) const;
	// 影のマップ 1 画素の大きさ（m。正射影なので距離に関わらず同じ）。
	float texelSize( int cascade ) const;
	const char* errorMessage() const;

private:
	std::unique_ptr< ShadowMap > mMaps[ kCascadeCount ];
	GameLib::Math::Matrix44 mProjectionViews[ kCascadeCount ];
	float mTexelSizes[ kCascadeCount ];
	int mSize;
};

#endif
