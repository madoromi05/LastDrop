#ifndef INCLUDED_LIT_QUAD_BATCH_H
#define INCLUDED_LIT_QUAD_BATCH_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Graphics/Texture.h"
#include "GameLib/Graphics/Vertex.h"
#include "GameLib/Graphics/VertexBuffer.h"
#include <vector>

// 絵を1枚まるごと貼った四角い板を決まった数までためて、壁や地面と同じライティング（ピクセル単位）で1回で描く。
// 弾痕（面に貼る板）と着弾の砂ぼこり（画面に平行な板）が使う。PrimitiveRenderer はライトを必ず切って描くので使わない。
// 半透明（絵のアルファ×頂点の色のアルファ）で描き、奥行きは読むが書かない。反射のツヤは付けない（粉や穴は光らないため）。
// ライト（環境光・太陽・補助光）の値は持たない。シーンが先に設定した値（GameScene::setupLights）でそのまま照らされる。
// 使い方: add を何回か → draw を、1フレームに1回。
class LitQuadBatch{
public:
	// quadCapacity: 1回に描ける板の数。1以上。
	explicit LitQuadBatch( int quadCapacity );
	LitQuadBatch( const LitQuadBatch& ) = delete;
	LitQuadBatch& operator=( const LitQuadBatch& ) = delete;

	// 板を1枚ためる。容量を超えた板はためない（持ち主が数の上限を決めているので、ふつうは起きない）。
	// center: 板の中心。right / up: 中心から右の辺・上の辺までの向きと長さ（ワールド）。
	// normal: 照らす向き（長さ1。この向きの面として光が当たる）。
	// color: 頂点の色（VertexColor::pack の並び。絵の色と不透明度に掛かる）。
	void add( const GameLib::Math::Vector3& center, const GameLib::Math::Vector3& right, const GameLib::Math::Vector3& up,
		const GameLib::Math::Vector3& normal, unsigned color );
	// ためた板を texture で描き、ためた板を捨てる。板が無ければ何もしない。
	// 物を描いた後、ライトを設定した後に、同じ描画先（PostProcess の beginScene〜endScene の間）で呼ぶ。
	// texture: 読み込み済み（isReady）であること。projectionView: カメラの射影×ビュー行列。
	// 描き終えたら、変えた設定（混ぜ方・奥行きの書き込み）を、物を描くときの設定に戻す（後に描く物に半透明が移らないように）。
	void draw( GameLib::Graphics::Texture texture, const GameLib::Math::Matrix44& projectionView );

private:
	GameLib::Graphics::VertexBuffer mVertexBuffer;       // 描くときに mVertices を写す先（毎フレーム中身を入れ替える）
	std::vector< GameLib::Graphics::Vertex > mVertices;  // ためた板の頂点（ワールド座標）。draw で空になる
	int mVertexCapacity;
};

#endif
