#ifndef INCLUDED_ESCAPE_CAR_H
#define INCLUDED_ESCAPE_CAR_H

#include "GameLib/Math/Vector3.h"
#include "StaticProp.h"
#include "IInteractable.h"

class StaticMeshData;
class CollisionWorld;
class HitWorld;

// 脱出用の車。ガソリンを決まった数だけ入れると走れる（isFueled）。見て操作キーで、持っているガソリンを1缶ずつ入れる。
// 見た目・移動の当たり判定（箱1つ）・視線の当たり判定は StaticProp に任せ、ここは給油だけを受け持つ。
// 走れるようになった後どうするか（クリア画面へ進む）は知らない（シーンが isFueled を見て決める）。
class EscapeCar : public IInteractable{
public:
	// data: 形と見た目（StaticProp と同じ約束）。position / angleY: 置き場所と向き（度）。
	// requiredFuel: 走れるようになるまでに入れるガソリンの数（1 以上）。
	EscapeCar( const StaticMeshData* data, const GameLib::Math::Vector3& position, float angleY, int requiredFuel );

	// 移動の名簿へ箱を、命中の名簿へ「弾（視線）を止める物」として自分の見た目を載せる。
	// この車はどちらの名簿よりも先に消えないこと（名簿は借りるだけなので、外す処理を持たない。シーンの最後まで置いておく物）。
	void addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld ) const;
	void draw() const;

	// 視線が当たった物がこの車か（Interactor が、当たった物から使える物を探すため）。
	bool isBody( const IRayBlocker* blocker ) const;
	int fuelDelivered() const;
	int fuelRequired() const;
	// ガソリンが決まった数だけ入ったか。
	bool isFueled() const;

	// IInteractable
	virtual bool canInteract( const Inventory& inventory ) const;
	virtual std::string prompt( const Inventory& inventory ) const;
	virtual void interact( Inventory* inventory );

private:
	StaticProp mBody;
	int mFuelRequired;
	int mFuelDelivered;
};

#endif
