#ifndef INCLUDED_COMPONENT_H
#define INCLUDED_COMPONENT_H

#include "GameSrc/World/UpdatePhase.h"

class GameObject;

// 物（GameObject）に付ける部品の元（Unity の Component と同じ考え方。SummaryPlan/個別仕様/ComponentSystem2_Plan.txt）。
// 物は部品を組み合わせて作る（自キャラ＝Transform・SkinnedModel・CharacterCollider・Hitbox・Hold・Player …）。
// 振る舞いとデータは部品の側が持ち、隣の部品が要るときは start で getComponent して覚える（毎ティック探さない）。
// 呼ばれる順：
// ・start：物が GameObjectWorld::spawn された時（途中で付けたなら付けた時）に 1 回。全部の部品が付いた後なので、隣の部品を探してよい
// ・update：updatePhase が None でなければ、その段（UpdatePhase）で毎ティック。None の部品は、隣の部品から頼まれて動く
// ・onDestroy：外される・物が消える時（GameObjectWorld::flush の中）に 1 回。借りている物を手放す
class Component{
public:
	virtual ~Component(){}
	// 付いている物（GameObject::addComponent の後から使える）。
	GameObject* gameObject() const { return mGameObject; }
	// 同じ物に付いている T の部品（無ければ nullptr）。定義は GameObject.h（GameObject の中身が要るため）。
	template< class T >
	T* getComponent() const;
	// 同じ物に付いているはずの T の部品。無ければ ASSERT で止める（Unity の RequireComponent の代わり）。start の中で使う。
	template< class T >
	T* requireComponent() const;

	// どの段で update を呼ぶか。物を spawn した後は変えない（段ごとの並びに入れるのは spawn の時だけ）。
	virtual UpdatePhase updatePhase() const { return UpdatePhase::None; }
	virtual void start(){}
	virtual void update(){}
	virtual void onDestroy(){}

private:
	friend class GameObject; // 付けたときに、付いた物を覚えさせる
	GameObject* mGameObject = nullptr;
};

#endif
