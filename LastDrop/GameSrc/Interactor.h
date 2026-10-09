#ifndef INCLUDED_INTERACTOR_H
#define INCLUDED_INTERACTOR_H

#include "GameLib/Math/Vector3.h"
#include "HitWorld.h"
#include <functional>
#include <string>

class IInteractable;
class IHittable;
class Inventory;

// 視線の先にある「使える物」（IInteractable）を探し、操作キーが押されたら使う。
// 視線は命中の名簿（HitWorld）へ撃つので、壁や車の向こうにある物は見つからない（手前の物に遮られる）。
// 当たった物が「使える物」かどうかは、名簿の持ち主（シーン）が教える（resolver）。名簿に載る物の種類を
// Interactor が知らずに済み、使える物の種類を増やしても Interactor は変わらない。
class Interactor{
public:
	// 視線が当たった物から、使える物を探す。使える物でなければ nullptr を返すこと。
	typedef std::function< IInteractable*( const HitWorld::Hit& ) > Resolver;

	// world: 視線を撃つ名簿。所有権は受け取らない。nullptr 不可。この Interactor より長く生きていること。
	// reach: 手が届く距離（m）。これより遠い物は使えない。resolver: 空は不可。
	Interactor( const HitWorld* world, float reach, Resolver resolver );

	// 目 eye から向き direction（長さ1）へ視線を撃って、見ている使える物を探す。interactPressed なら使う。
	// 1回の描画につき1回（押した瞬間を1回だけ拾うため）。self: 視線を当てない自分の体。nullptr なら全員に当てる。
	// inventory: 使うときに増やしたり減らしたりする持ち物。nullptr 不可。
	void update( const GameLib::Math::Vector3& eye, const GameLib::Math::Vector3& direction, const IHittable* self,
		Inventory* inventory, bool interactPressed );
	// 今見ている使える物の案内（無ければ空）。update の後で読む。
	const std::string& prompt() const;

private:
	const HitWorld* mWorld;
	float mReach;
	Resolver mResolver;
	std::string mPrompt;
};

#endif
