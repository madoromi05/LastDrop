#ifndef INCLUDED_BLOOD_SPRAY_LIST_H
#define INCLUDED_BLOOD_SPRAY_LIST_H

#include "GameSrc/Effect/BloodSpray.h"
#include "GameSrc/Render/LitQuadBatch.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Graphics/Texture.h"
#include <random>
#include <vector>

// 血しぶきの持ち主。出す（spawn）と1回分増え、進めて（update）終わった物を消し、全部の滴を1回で描く（draw）。
// 同時に出しておけるのは決まった回数まで。超えたら一番古い物から消す。
// どこで・どれだけ出すか（ゾンビに当たったら、頭なら多く）は持ち主（GameWorld）が決める。
// 滴は画面に平行な板で描き、壁や地面と同じライトで照らす（LitQuadBatch。ImpactDustList と同じく、カメラの方を向いた面として）。
// 滴は奥行きで並べ替えない（全部同じ色なので、重なる順番が違っても見た目がほとんど変わらないため）。
class BloodSprayList{
public:
	BloodSprayList();
	BloodSprayList( const BloodSprayList& ) = delete;
	BloodSprayList& operator=( const BloodSprayList& ) = delete;

	// 引数は BloodSpray のコンストラクタと同じ（random はこのリストが持つ物を使う）。
	void spawn( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& direction, float scale );
	// 全部を1ティック進め、終わった物を消す。
	void update();
	// 全部の滴を1回で描く。1フレームに1回まで。呼ぶ場所の決まりは LitQuadBatch::draw と同じ。絵の読み込みが終わるまでは描かない。
	// projectionView: カメラの射影×ビュー行列。
	// viewDirection: 視線の向き（長さ1）。板を画面に平行にするのと、照らす向き（その逆向き）に使う。
	void draw( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection );

private:
	std::vector< BloodSpray > mSprays;   // 古い順
	std::mt19937 mRandom;                // 滴の向き・速さのばらつき
	GameLib::Graphics::Texture mTexture; // 滴の絵（Resource/blood_drop.tga）
	LitQuadBatch mQuads;                 // 滴の板をためて、ライトを当てて描く
};

#endif
