#ifndef INCLUDED_CASCADED_SHADOW_MAP_H
#define INCLUDED_CASCADED_SHADOW_MAP_H

#include "GameSrc/Render/Shadow/ShadowMap.h"
#include "GameSrc/Render/Shadow/CascadeRefresh.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Vector3.h"
#include <memory>

// 月（平行光）の影の、カメラからの距離で分けた 3 枚の影のマップ（CSM。Shadow_Plan.txt 6 章）
// 近い枚ほど狭い範囲を同じ画素の数で描くので、手前の影は細かく、遠くの影は粗い
// 各枚は「その距離の範囲に映る所を包む球」を、光の向きから正射影で見る。球の大きさはカメラの画角だけで決まるので、
// カメラを回しても枚の大きさが変わらない。さらに枚の中心を影のマップの 1 画素の単位で丸め、カメラが動いても影の縁が泳がないようにする
// 遠い枚ほど間を空けて描き直す。描き直さないフレームは、前に描いた影と行列をそのまま使う
// （影と行列がずれない）。いつ描き直すかは枚ごとの CascadeRefresh が決める
// 使い方：update（カメラと光の向き）→ isDue( i ) の枚だけ map( i ).begin( projectionView( i ) ) → 影を落とす物を描く → end
// 影を描かないフレーム（影を切った・月が無い）には invalidate を呼ぶ（次に描くときに全部描き直す）
// 作るのは Graphics::Manager ができた後（シーンの中）
class CascadedShadowMap{
public:
	static const int kCascadeCount = 3;

	// 描き直しの間引き方
	struct Refresh{
		int intervals[ kCascadeCount ]; // 枚ごとに、何フレームに 1 回描き直すか（1 以上。手前の枚は 1）
		float redrawAngleDegree;        // 前に描いた時からカメラの向きがこれ以上（度）変わったら、間隔を待たずに描き直す
		float redrawDistance;           // 前に描いた時から目がこれ以上（m）動いたら、間隔を待たずに描き直す
	};

	// size：1 枚の影のマップの一辺の画素の数
	explicit CascadedShadowMap( int size );
	CascadedShadowMap( const CascadedShadowMap& ) = delete;
	CascadedShadowMap& operator=( const CascadedShadowMap& ) = delete;

	// このフレームに描き直す枚を決め、その枚の行列を作り直す（描き直さない枚の行列は前のまま）。1 フレームに 1 回呼ぶ
	// eye / viewDirection：カメラの目と向き（長さ 1）。fieldOfViewY：縦の画角（度）。aspect：横 / 縦
	// lightDirection：光の進む向き（長さ 1）。cascadeEnds：各枚の受け持ちの奥の端（カメラの向きに沿った距離 m。小さい順）
	// casterMargin：枚の球より光の側へ、影を落とす物を拾う長さ（m。球の外の高い建物の影も入るように）
	// 光の向き・端・casterMargin・画角・縦横比のどれかが前と違えば、全部の枚を描き直す（F6 で影の調整値を動かしたとき）
	void update( const GameLib::Math::Vector3& eye, const GameLib::Math::Vector3& viewDirection, float fieldOfViewY, float aspect,
		const GameLib::Math::Vector3& lightDirection, const float ( &cascadeEnds )[ kCascadeCount ], float casterMargin,
		const Refresh& refresh );
	void invalidate();					// 次の update で全部の枚を描き直す（影を描かなかったフレームに呼ぶ。止めていた間の古い影を使わないように）
	bool isDue( int cascade ) const;	// 最後の update で、この枚を描き直すと決めたか（描き直す枚だけ影を描く）

	ShadowMap& map( int cascade );
	const GameLib::Math::Matrix44& projectionView( int cascade ) const;
	// 影のマップ 1 画素の大きさ（m。正射影なので距離に関わらず同じ）
	float texelSize( int cascade ) const;
	const char* errorMessage() const;

private:
	// 行列の作り方を決める値（カメラの位置と向き以外）。前のフレームと比べて、変わったら全部の枚を描き直す
	struct Placement{
		GameLib::Math::Vector3 lightDirection;
		float cascadeEnds[ kCascadeCount ];
		float casterMargin;
		float fieldOfViewY;
		float aspect;
		bool operator==( const Placement& other ) const;
	};
	// cascade の枚の行列と 1 画素の大きさを作る。start / end：その枚が受け持つ奥の範囲（m）。right / upward：光から見た右・上の向き
	void placeCascade( int cascade, const GameLib::Math::Vector3& eye, const GameLib::Math::Vector3& viewDirection, float lateral,
		const GameLib::Math::Vector3& lightDirection, const GameLib::Math::Vector3& right, const GameLib::Math::Vector3& upward,
		float start, float end, float casterMargin );

	std::unique_ptr< ShadowMap > mMaps[ kCascadeCount ];
	GameLib::Math::Matrix44 mProjectionViews[ kCascadeCount ];
	float mTexelSizes[ kCascadeCount ];
	CascadeRefresh mRefresh[ kCascadeCount ];
	Placement mPlacement; // 最後の update の値
	unsigned mFrame;      // update を呼んだ回数（描き直す間隔を数える）
	bool mInvalid;        // 次の update で全部描き直すか
	int mSize;
};

#endif
