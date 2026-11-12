#ifndef INCLUDED_ZOMBIE_STAGGER_H
#define INCLUDED_ZOMBIE_STAGGER_H

#include "GameSrc/Hit/BodyPart.h"

// ゾンビ1体の「よろけ」の時間を数える。よろけの最中か、と、終わってから次のよろけを出せるまでの間あけ
// よろけは、よろけていない間に当たったときに出し、終わってから少しの間は出さない（頭に当たったときは待たない）
// フルオートで当て続けても、よろけ続けて動けなくならないようにするため（SummaryPlan/個別仕様/ゾンビ/ZombieStagger_Plan.txt）
// アニメを流す・止める、根の動きの区間を始める・閉じる、よろけのアニメがあるかを見るのは持ち主（Zombie）
// ここは時間と「出してよいか」だけ（ZombieAttack と同じく、自分では動かず、アニメも持たない）
class ZombieStagger{
public:
	ZombieStagger();

	bool canStart( BodyPart part ) const;	// part に当たったとき、よろけを始めてよいか（よろけていない、かつ間あけが済んだか頭）
	void start();							// よろけ始めた
	void finish();							// よろけのアニメが終わった。ここから間あけを数える
	void cancel();							// 倒れたので、よろけを打ち切る（間あけは数え直さない）
	void tick();							// よろけていない間、間あけを 1 ティック進める（毎ティック呼ぶ）
	bool isActive() const;					// よろけの最中か

private:
	bool mActive;       // よろけの最中か
	int mCooldownTicks; // 次のよろけを出せるまでの残り（ティック。0 なら出せる）
};

#endif
