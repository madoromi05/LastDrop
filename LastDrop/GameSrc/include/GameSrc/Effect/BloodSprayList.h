#ifndef INCLUDED_BLOOD_SPRAY_LIST_H
#define INCLUDED_BLOOD_SPRAY_LIST_H

#include "GameSrc/Effect/BloodSpray.h"
#include "GameSrc/Render/LitQuadBatch.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Graphics/Texture.h"
#include <random>
#include <vector>

// 血しぶきの持ち主。出す（spawn）と1回分増え、進めて（update）終わった物を消し、全部の滴を1回で描く（draw）
// 同時に出しておけるのは決まった回数まで。超えたら一番古い物から消す
// どこで・どれだけ出すか（ゾンビに当たったら、頭なら多く）は頼む側（CombatResolver。EffectManager を通す）が決める
// 滴は画面に平行な板で描き、壁や地面と同じライトで照らす（LitQuadBatch。ImpactDustList と同じく、カメラの方を向いた面として）
// 滴は目から遠い順に並べ替えて描く（Z ソート。EffectFlipbook_Plan.txt の 3 章）。砂ぼこりとの間は並べ替えない（別の絵なので別に描く）
class BloodSprayList{
public:
	BloodSprayList();
	BloodSprayList( const BloodSprayList& ) = delete;
	BloodSprayList& operator=( const BloodSprayList& ) = delete;

	void spawn( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& direction, float scale );	// 引数は BloodSpray のコンストラクタと同じ（random はこのリストが持つ物を使う）
	void update();																								// 全部を1ティック進め、終わった物を消す
	// 全部の滴を1回で描く。1フレームに1回まで。呼ぶ場所の決まりは LitQuadBatch::draw と同じ。絵の読み込みが終わるまでは描かない
	// projectionView: カメラの射影×ビュー行列
	// viewDirection: 視線の向き（長さ1）。板を画面に平行にするのと、照らす向き（その逆向き）に使う
	// eye: 目の位置（ワールド）。滴を目から遠い順に並べ替えるのに使う
	void draw( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection,
		const GameLib::Math::Vector3& eye );

private:
	std::vector< BloodSpray > mSprays;   // 古い順
	std::mt19937 mRandom;                // 滴の向き・速さのばらつき
	GameLib::Graphics::Texture mTexture; // 滴の絵（Resource/Texture/Effect/blood_drop.tga）
	LitQuadBatch mQuads;                 // 滴の板をためて、ライトを当てて描く
};

#endif
