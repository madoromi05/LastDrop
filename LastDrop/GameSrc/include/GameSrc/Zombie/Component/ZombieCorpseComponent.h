#ifndef INCLUDED_ZOMBIE_CORPSE_COMPONENT_H
#define INCLUDED_ZOMBIE_CORPSE_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Zombie/Damage/ZombieCorpse.h"

class SkinnedMeshInstance;
class ZombieStateComponent;
class ZombieRootMotionSpan;

// 倒れ終わってから、死体として残り、地面へ沈んで消えるまで（時間は ZombieCorpse）
// 状態を Falling → Corpse → Sinking と進め、沈む分だけ見た目の位置を下げる（部位の箱も見た目に付いて沈む）
// 沈み終わったら isGone が true になり、自分の物に消す印を付ける（GameObjectWorld::destroyLater）
class ZombieCorpseComponent : public Component{
public:
	ZombieCorpseComponent();

	void update();					// 倒れ始めた後、毎ティック（死亡アニメを進めた後に呼ぶ）
	bool isGone() const;			// 沈み終わって、消してよいか（そうなった update で、自分の物に destroyLater する）
	float secondsUntilGone() const;	// 消えるまでの残り秒（死体・沈んでいる間だけ。それ以外は 0）。確認用の表示に使う

	void start() override;	// Component（Transform・SkinnedModel・ZombieState・ZombieRootMotion が要る）

private:
	ZombieCorpse mCorpse;
	// ── 同じ物の部品（start で借りる）──
	const GameLib::Math::Vector3* mPosition; // 足元の位置（TransformComponent。沈んでも変えない）
	SkinnedMeshInstance* mMesh;
	ZombieStateComponent* mState;
	ZombieRootMotionSpan* mRootSpan;
};

#endif
