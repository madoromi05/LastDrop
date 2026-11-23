#ifndef INCLUDED_IMPACT_DUST_LIST_H
#define INCLUDED_IMPACT_DUST_LIST_H

#include "GameSrc/Effect/ImpactDust.h"
#include "GameSrc/Render/LitQuadBatch.h"
#include "GameSrc/Render/Flipbook.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Graphics/Texture.h"
#include <random>
#include <vector>

// 着弾の砂ぼこりの持ち主。出す（spawn）と1回分増え、進めて（update）終わった物を消し、全部の粒を1回で描く（draw）
// 同時に出しておけるのは決まった回数まで。超えたら一番古い物から消す
// どこで出すか（ゾンビには出さない、など）は頼む側（CombatResolver。EffectManager を通す）が決める
// 粒は画面に平行な板で描き、壁や地面と同じライトで照らす（LitQuadBatch）。粒には面が無いので、カメラの方を向いた面として照らす
// （太陽を背にして見ると明るく、太陽の方を見ると逆光で暗い）
// 粒の絵は、もこもこ広がって薄れていく煙のコマ送り（フリップブック。4×4 の 16 コマ）
// 粒が出てから消えるまでで 1 回り進める。粒ごとに板を視線の周りに回して、同じ絵が同じ向きに並ばないようにする
// 粒は目から遠い順に並べ替えて描く（Z ソート。手前の粒が奥の粒に消されて、四角い切れ目が見えないように）
class ImpactDustList{
public:
	ImpactDustList();
	ImpactDustList( const ImpactDustList& ) = delete;
	ImpactDustList& operator=( const ImpactDustList& ) = delete;

	void spawn( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& normal );	// point / normal: 当たった位置と、面の外向きの法線（長さ1）。HitWorld::Hit の値をそのまま渡す
	void update();																				// 全部を1ティック進め、終わった物を消す
	// 全部の粒を1回で描く。1フレームに1回まで。呼ぶ場所の決まりは LitQuadBatch::draw と同じ。絵の読み込みが終わるまでは描かない
	// projectionView: カメラの射影×ビュー行列
	// viewDirection: 視線の向き（長さ1）。板を画面に平行にするのと、照らす向き（その逆向き）に使う
	// eye: 目の位置（ワールド）。粒を目から遠い順に並べ替えるのに使う
	void draw( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection,
		const GameLib::Math::Vector3& eye );

private:
	std::vector< ImpactDust > mDusts;    // 古い順
	std::mt19937 mRandom;                // 粒の向き・速さのばらつき
	GameLib::Graphics::Texture mTexture; // 粒の煙のコマ送りの絵（Resource/Texture/Effect/impact_smoke_sheet.tga）
	Flipbook mFrames;                    // 上の絵のコマの並び
	LitQuadBatch mQuads;                 // 粒の板をためて、ライトを当てて描く
};

#endif
