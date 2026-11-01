#ifndef INCLUDED_SHADOW_MAP_H
#define INCLUDED_SHADOW_MAP_H

#include "GameLib/Graphics/DepthTarget.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Vector3.h"

// 影のマップ 1 枚（光の側から見た、一番手前の物までの深度）。仕様は SummaryPlan/個別仕様/Shadow_Plan.txt 4 章。
// 使い方：begin( 光の側の射影×ビュー ) → 影を落とす物を描く（WorldRenderer::drawShadowCasters）→ end。
// 間は RenderPass が Shadow になり、静的メッシュとキャラは深度だけを描くシェーダへ切り替わる（StaticMeshPart::draw・SkinningPipeline::begin）。
// 描いた深度は、ライトの一覧（LightList::setShadowMap）に渡して、ライティングのパスで読む。
// 作るのは Graphics::Manager ができた後（シーンの中）。
class ShadowMap{
public:
	// size：影のマップの一辺の画素の数（正方形）。
	explicit ShadowMap( int size );
	ShadowMap( const ShadowMap& ) = delete;
	ShadowMap& operator=( const ShadowMap& ) = delete;

	// 描画先を影のマップ（深度だけ）にして消し、描く行列を lightProjectionView にする。
	// end の後は、描く行列が光の側のまま残るので、呼び出し側がカメラの行列へ戻すこと。
	void begin( const GameLib::Math::Matrix44& lightProjectionView );
	// RenderPass を Forward へ戻し、深度だけのシェーダを外す。描画先はそのまま（次に描く所で付け直す）。
	void end();

	GameLib::Graphics::DepthTarget depth() const;
	int size() const;
	// 静的メッシュの深度だけのシェーダのコンパイルエラー。問題が無ければ空文字列。
	const char* errorMessage() const;

private:
	GameLib::Graphics::DepthTarget mDepth;
	GameLib::Graphics::Shader mStaticMeshShader; // Resource/Shader/StaticMeshShadow.hlsl
	int mSize;
};

// スポットライト（懐中電灯）の影のマップの、光の側の射影×ビュー（透視投影）。
// position / direction：光の位置と照らす向き（長さ 1）。outerAngleDegree：光の外側の角度（半角）。nearClip / farClip：深度の範囲（m）。
GameLib::Math::Matrix44 spotShadowProjectionView( const GameLib::Math::Vector3& position, const GameLib::Math::Vector3& direction,
	float outerAngleDegree, float nearClip, float farClip );
// 上の行列で、光から 1m の所の影のマップ 1 画素の大きさ（m）。影を読む位置をずらす量の単位（距離を掛けて使う）。
float spotShadowTexelSizeAtOneMeter( float outerAngleDegree, int mapSize );

#endif
