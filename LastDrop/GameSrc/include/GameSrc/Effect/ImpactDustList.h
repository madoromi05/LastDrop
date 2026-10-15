#ifndef INCLUDED_IMPACT_DUST_LIST_H
#define INCLUDED_IMPACT_DUST_LIST_H

#include "GameSrc/Effect/ImpactDust.h"
#include "GameSrc/Render/LitQuadBatch.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Graphics/Texture.h"
#include <random>
#include <vector>

// 着弾の砂ぼこりの持ち主。出す（spawn）と1回分増え、進めて（update）終わった物を消し、全部の粒を1回で描く（draw）。
// 同時に出しておけるのは決まった回数まで。超えたら一番古い物から消す。
// どこで出すか（ゾンビには出さない、など）は持ち主（GameWorld）が決める。
// 粒は画面に平行な板で描き、壁や地面と同じライトで照らす（LitQuadBatch）。粒には面が無いので、カメラの方を向いた面として照らす
// （太陽を背にして見ると明るく、太陽の方を見ると逆光で暗い。2026-10-15 ユーザー指定）。
// 粒は奥行きで並べ替えない（全部同じ色なので、重なる順番が違っても見た目がほとんど変わらないため）。
class ImpactDustList{
public:
	ImpactDustList();
	ImpactDustList( const ImpactDustList& ) = delete;
	ImpactDustList& operator=( const ImpactDustList& ) = delete;

	// point / normal: 当たった位置と、面の外向きの法線（長さ1）。HitWorld::Hit の値をそのまま渡す。
	void spawn( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& normal );
	// 全部を1ティック進め、終わった物を消す。
	void update();
	// 全部の粒を1回で描く。1フレームに1回まで。呼ぶ場所の決まりは LitQuadBatch::draw と同じ。絵の読み込みが終わるまでは描かない。
	// projectionView: カメラの射影×ビュー行列。
	// viewDirection: 視線の向き（長さ1）。板を画面に平行にするのと、照らす向き（その逆向き）に使う。
	void draw( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection );

private:
	std::vector< ImpactDust > mDusts;    // 古い順
	std::mt19937 mRandom;                // 粒の向き・速さのばらつき
	GameLib::Graphics::Texture mTexture; // 粒の絵（Resource/impact_dust.tga）
	LitQuadBatch mQuads;                 // 粒の板をためて、ライトを当てて描く
};

#endif
