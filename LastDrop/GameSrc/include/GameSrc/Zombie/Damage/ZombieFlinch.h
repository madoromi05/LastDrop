#ifndef INCLUDED_ZOMBIE_FLINCH_H
#define INCLUDED_ZOMBIE_FLINCH_H

#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"
#include "GameSrc/Hit/BodyPart.h"
#include "GameLib/Math/Vector3.h"
#include <vector>

class ZombieFlinchLayout;

// ゾンビ1体ぶんの「のけぞり」。弾が当たるたびに、当たった部位を弾の進む向きへ押し、毎ティック少しずつ戻す。
// アニメ（歩き・よろけ）で決まった姿勢の上に、描く直前に曲げを足す（BonePoseModifier）。どのアニメの最中でも効く。
// 押す量は「まとまり（頭・胴・腕・脚）」ごとに、向きと角度を1本の矢印で持つ。続けて当たったら矢印を足し合わせる（上限あり）。
// 曲げるのは見た目の姿勢だけ。部位の当たり判定（HitboxSet）はアニメどおりの姿勢に付くので動かない。
// どの関節で・どの骨を曲げるかは ZombieFlinchLayout（種類ごとに共有）を借りる。
class ZombieFlinch : public BonePoseModifier{
public:
	// layout: 曲げ方の設計図。所有権は受け取らない。nullptr 不可。こののけぞりより長く生きていること（ZombieModel が持つ）。
	explicit ZombieFlinch( const ZombieFlinchLayout* layout );

	// part に当たった弾で押す。direction: 弾の進む向き（長さ1。骨格の根の空間＝置き場所と向きを掛ける前）。
	// 押す角度は、まとまりごとの角度に ZombieStaggerParams::flinchScale を掛けた物。
	void push( BodyPart part, const GameLib::Math::Vector3& direction );
	// 噛むときに、首と背骨を rootDirection（骨格の根の空間。長さ1）へ突き出す（噛むアニメが無いので、顔を相手へ近づける代わり）。
	// 首は 2 回・背骨は 1 回押す（首は 1 回 20°なので 40°、背骨は 12°。のけぞりの倍率は掛けない）。のけぞりと同じく、すぐ曲がって 0.25 秒で戻る。
	void lunge( const GameLib::Math::Vector3& rootDirection );
	// 押した分を1ティック戻す。
	void update();
	// 見て分かるほど押されているか（どこかのまとまりが kNegligibleDegree 以上曲がっている）。
	// 押されている間はアニメを間引かない（AnimationThrottle。のけぞりが毎ティック戻っていくのを飛ばして見せないため）。
	bool isActive() const;
	// 押されている分だけ、当たったまとまりを関節で曲げる（BonePoseModifier）。
	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;

private:
	// part のまとまりを direction へ degree だけ足して押す（足した角度は上限で切る）。
	void kick( BodyPart part, const GameLib::Math::Vector3& direction, float degree );

	const ZombieFlinchLayout* mLayout; // 借りているだけ
	// まとまりごとの押す量（骨格の根の空間。向き＝押す向き、長さ＝角度（度））。並びは ZombieFlinchLayout::groups と同じ。
	std::vector< GameLib::Math::Vector3 > mPushes;
};

#endif
