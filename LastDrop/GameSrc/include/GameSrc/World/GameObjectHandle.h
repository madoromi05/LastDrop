#ifndef INCLUDED_GAME_OBJECT_HANDLE_H
#define INCLUDED_GAME_OBJECT_HANDLE_H

class GameObject;
class GameObjectWorld;

// 物（GameObject）を、消えたかどうか分かる形で指す（SummaryPlan/個別仕様/設計と分割/ComponentSystem2_Plan.txt の決まりごと）
// 持ち主（GameObjectWorld）が物ごとに振った番号で引き直すので、物が flush で消えた後は get() が nullptr を返す
// （生のポインタのように、消えた物を指したままにならない）。番号は使い回さない
// 物をまたいで指す所（自キャラが持つ銃・死体とその死体が落とした弾）で使う。同じ物の部品どうしは生のポインタでよい
class GameObjectHandle{
public:
	GameObjectHandle();                            // 何も指さない
	explicit GameObjectHandle( const GameObject* object ); // object: nullptr なら何も指さない
	// 指している物。消えた・何も指していなければ nullptr
	GameObject* get() const;

private:
	const GameObjectWorld* mWorld;
	unsigned mId;
};

#endif
