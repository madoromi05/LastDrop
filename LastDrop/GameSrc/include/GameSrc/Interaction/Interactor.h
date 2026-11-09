#ifndef INCLUDED_INTERACTOR_H
#define INCLUDED_INTERACTOR_H

#include "GameLib/Math/Vector3.h"
#include <functional>

class IInteractable;
class Inventory;

// 視線の先にある「使える物」（IInteractable）を探し、操作キーが押されたら使う。
// 視線の先のどれが「使える物」かは、世界の持ち主（シーン）が探す（finder）。壁の向こうは見えない、
// といった見え方の決まりと名簿に載る物の種類を Interactor が知らずに済み、使える物の種類を増やしても Interactor は変わらない。
// ここは「手が届く距離まで視線を伸ばす」「押されたら使う」「見ている物を覚える」だけを受け持つ（マークを出すのは InteractMarkers）。
class Interactor{
public:
	// origin から delta だけ進む視線の先の、一番手前の使える物を探す。無ければ nullptr を返すこと。
	typedef std::function< IInteractable*( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta ) > Finder;

	// reach: 手が届く距離（m）。これより遠い物は使えない。finder: 空は不可。
	Interactor( float reach, Finder finder );

	// 目 eye から向き direction（長さ1）へ視線を撃って、見ている使える物を探す。interactPressed なら使う
	// （拾う物は使わずに offer に残す。置く所を選ぶのは呼ぶ側。
	// 漁れる場所も使わずに opened に残す。開けるまで待つ・中を見せるのは呼ぶ側）。
	// 1回の描画につき1回（押した瞬間を1回だけ拾うため）。
	// inventory: 使うときに増やしたり減らしたりする持ち物。nullptr 不可。
	void update( const GameLib::Math::Vector3& eye, const GameLib::Math::Vector3& direction,
		Inventory* inventory, bool interactPressed );
	// 今見ている使える物（無ければ nullptr）。update の直後に読む（拾われた物は次の世界の update で消えるので、それより後には使わない）。
	const IInteractable* target() const;
	// 最後の update で E を押した拾う物（無ければ nullptr）。update の直後に読む。世界を進める前に使い終えること
	// （拾い終えた物は世界の update で消える）。
	IInteractable* offer() const;
	// 最後の update で E を押した漁れる場所・脱出の車（lootGrid がある物。無ければ nullptr）。読む決まりは offer と同じ。
	IInteractable* opened() const;

private:
	float mReach;
	Finder mFinder;
	const IInteractable* mTarget; // 借りているだけ
	IInteractable* mOffer;        // 借りているだけ
	IInteractable* mOpened;       // 借りているだけ
};

#endif
