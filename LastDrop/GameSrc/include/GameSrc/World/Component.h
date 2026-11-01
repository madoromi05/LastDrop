#ifndef INCLUDED_COMPONENT_H
#define INCLUDED_COMPONENT_H

#include "GameSrc/World/UpdatePhase.h"

class GameObject;

// 物（GameObject）に付ける部品の元（Unity の Component と同じ考え方。SummaryPlan/個別仕様/ComponentSystem2_Plan.txt・ComponentSystem3_Plan.txt）。
// 物は部品を組み合わせて作る（自キャラ＝Transform・SkinnedModel・CharacterCollider・Hitbox・Hold・Player …）。
// 振る舞いとデータは部品の側が持つ。付ける順には意味を持たせない（awake と start の 2 段で、どの順に付けても同じに動く）。
// 隣の部品の借り方：
// ・必須の部品（無いとこの部品が成り立たない）：start で requireComponent して覚える。印が付き、外せなくなる
// ・任意の部品（外すとその振る舞いだけ無くなる）：使う所で getComponent し、nullptr なら飛ばす（覚えない。途中で外されても落ちない）
// 世界に 1 つの物（当たり判定の名簿・町の床の形など）は、付けるときの引数ではなく shared で引く。
// 呼ばれる順：
// ・awake：物が GameObjectWorld::spawn された時（途中で付けたなら付けた時）に 1 回、どの部品の start よりも前。自分の持ち物を作る。
//   隣の部品から借りてよいのは、付けたときから在る物（部品そのもの・そのコンストラクタで作った物）だけ（隣の awake で作る物はまだ無いことがある）
// ・start：全部の部品の awake の後に 1 回。隣の部品が awake で作った物も借りてよい
// ・update：updatePhase が None でなければ、その段（UpdatePhase）で毎ティック。None の部品は、隣の部品から頼まれて動く
// ・onDestroy：外される・物が消える時（GameObjectWorld::flush の中）に 1 回。借りている物を手放す
class Component{
public:
	virtual ~Component(){}
	// 付いている物（GameObject::addComponent の後から使える）。
	GameObject* gameObject() const { return mGameObject; }
	// 同じ物に付いている T の部品（無ければ nullptr）。定義は GameObject.h（GameObject の中身が要るため）。任意の部品はこれで引く。
	template< class T >
	T* getComponent() const;
	// 同じ物に付いているはずの T の部品。無ければ ASSERT で止める（Unity の RequireComponent の代わり）。awake・start の中で使う。
	// 引かれた部品には「ほかの部品が要る」印が付き、removeComponentLater で外せなくなる。
	template< class T >
	T* requireComponent() const;
	// 世界に 1 つの T（GameObjectWorld::provide で置いた物）。置いていなければ ASSERT で止める。awake・start の中で使う。
	// 定義は GameObjectWorld.h（GameObjectWorld の中身が要るため）。
	template< class T >
	T* shared() const;

	// どの段で update を呼ぶか。物を spawn した後は変えない（段ごとの並びに入れるのは spawn の時だけ）。
	virtual UpdatePhase updatePhase() const { return UpdatePhase::None; }
	virtual void awake(){}
	virtual void start(){}
	virtual void update(){}
	virtual void onDestroy(){}

private:
	friend class GameObject; // 付けたときに、付いた物を覚えさせる
	GameObject* mGameObject = nullptr;
};

#endif
