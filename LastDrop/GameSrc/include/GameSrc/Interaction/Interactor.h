#ifndef INCLUDED_INTERACTOR_H
#define INCLUDED_INTERACTOR_H

#include "GameLib/Math/Vector3.h"
#include <functional>
#include <string>

class IInteractable;
class Inventory;

// 視線の先にある「使える物」（IInteractable）を探し、操作キーが押されたら使う。
// 視線の先のどれが「使える物」かは、世界の持ち主（シーン）が探す（finder）。壁の向こうは見えない・死体を見たらその死体が落とした弾、
// といった見え方の決まりと名簿に載る物の種類を Interactor が知らずに済み、使える物の種類を増やしても Interactor は変わらない。
// ここは「手が届く距離まで視線を伸ばす」「押されたら使う」「案内を覚える」だけを受け持つ。
class Interactor{
public:
	// origin から delta だけ進む視線の先の、一番手前の使える物を探す。無ければ nullptr を返すこと。
	typedef std::function< IInteractable*( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta ) > Finder;

	// reach: 手が届く距離（m）。これより遠い物は使えない。finder: 空は不可。
	Interactor( float reach, Finder finder );

	// 目 eye から向き direction（長さ1）へ視線を撃って、見ている使える物を探す。interactPressed なら使う。
	// 1回の描画につき1回（押した瞬間を1回だけ拾うため）。
	// inventory: 使うときに増やしたり減らしたりする持ち物。nullptr 不可。
	void update( const GameLib::Math::Vector3& eye, const GameLib::Math::Vector3& direction,
		Inventory* inventory, bool interactPressed );
	// 今見ている使える物の案内（無ければ空）。update の後で読む。
	const std::string& prompt() const;

private:
	float mReach;
	Finder mFinder;
	std::string mPrompt;
};

#endif
