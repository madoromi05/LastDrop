#ifndef INCLUDED_COMPONENT_H
#define INCLUDED_COMPONENT_H

class GameObject;

// 物（GameObject）に付ける部品の元（Unity の Component と同じ考え方。SummaryPlan/個別仕様/ComponentSystem_Plan.txt）。
// 今のクラス（Zombie・Pickup …）がこれも継承して、そのまま部品になる。振る舞いとデータは部品の側が持つ（委譲）。
// 「全部の部品へ毎ティック update を呼ぶ」仕組みは持たない。いつ何を動かすかは持ち主（GameWorld）が
// GameObjectWorld::forEach で種類ごとに順に呼ぶ（順番がコードに見えるように。仕様書の ★2）。
class Component{
public:
	virtual ~Component(){}
	// 付いている物（GameObject::addComponent の後から使える）。消す印を付けるときなどに使う。
	GameObject* gameObject() const { return mGameObject; }

private:
	friend class GameObject; // 付けたときに、付いた物を覚えさせる
	GameObject* mGameObject = nullptr;
};

#endif
