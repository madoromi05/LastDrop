#ifndef INCLUDED_INTERACT_MARKERS_H
#define INCLUDED_INTERACT_MARKERS_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Graphics/Texture.h"
#include "GameSrc/Scene/Game/World/InteractionSystem.h"
#include <vector>

class IInteractable;
class Inventory;
class FirstPersonCamera;
class UiRenderer;

// 使える物（落ちている物・漁れる場所・脱出の車）の上に出すマーク（SummaryPlan/個別仕様/UI/InteractMarker_Plan.txt。文字の案内の代わり）
// ・近く（kMarkDistance 以内）で、壁などに隠れていない物の上に、小さな白いひし形
// ・見ている物（E で使う物。InteractorComponent::target）は、ひし形を大きくして真ん中に「E」
// ・今の持ち物で使えない物（カバンが一杯・ガソリンを持っていない・満タン）は灰色
// どの物が近くて見えるかは InteractionSystem、見ている物は InteractorComponent が決める。ここは集めた物を覚えて、画面に積むだけ
// 物の状態は変えない
class InteractMarkers{
public:
	InteractMarkers();

	// 近くの使える物を集め、マークの位置・色・大きさを決めて覚える。InteractorComponent::look と拾われた物の印付けの直後に、1回の描画につき1回呼ぶ
	// （拾われた物は次の世界の update で消えるので、物そのものは覚えず、位置と状態だけを写しておく）
	// eye: 目の位置。target: 見ている物（InteractorComponent::target。無ければ nullptr）。inventory: 使えるかを決める持ち物
	void update( const InteractionSystem& interaction, const GameLib::Math::Vector3& eye, const IInteractable* target, const Inventory& inventory );
	void clear();	// 覚えた物を消す（自キャラがいない・死んだときなど、マークを出さないとき）
	// 覚えたマークを renderer に積む（描くのは UiRenderer::draw。HUD の描画でまとめて描かれる）。HUD を描く前に呼ぶ
	// camera: 今の描画のカメラ（物の位置を画面へ直す）。renderer: nullptr 不可
	void add( const FirstPersonCamera& camera, UiRenderer* renderer ) const;

private:
	// マーク 1 つ
	struct Mark{
		GameLib::Math::Vector3 position; // 世界の位置
		bool usable;                     // 今の持ち物で使えるか（使えなければ灰色）
		bool targeted;                   // 見ている物か（大きくして「E」）
	};

	GameLib::Graphics::Texture mTexture;         // ひし形の絵（Resource/UI/ui_interact_mark.tga）。読めなければ四角で代わりに描く
	bool mHasTexture;                            // mTexture が読めたか（update で確かめる）
	std::vector< InteractionSystem::NearbyInteractable > mNearby; // collectNearby の答えを受ける（毎回作り直さないよう持っておく）
	std::vector< Mark > mMarks;
};

#endif
