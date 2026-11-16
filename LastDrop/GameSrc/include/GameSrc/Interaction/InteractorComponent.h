#ifndef INCLUDED_INTERACTOR_COMPONENT_H
#define INCLUDED_INTERACTOR_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameLib/Math/Vector3.h"

class IInteractable;
class IInteractFinder;
class InventoryComponent;

// 自キャラの「見ている物を使う」部品（SummaryPlan/個別仕様/設計と分割/InteractorComponent_Plan.txt）
// 視線の先にある「使える物」（IInteractable）を探し、操作キーが押されたら使う
// 視線の先のどれが「使える物」かは、共有の棚の IInteractFinder が探す。壁の向こうは見えない、といった見え方の決まりと
// 名簿に載る物の種類をここが知らずに済み、使える物の種類を増やしてもここは変わらない
// ここは「手が届く距離まで視線を伸ばす」「押されたら使う」「見ている物を覚える」だけを受け持つ（マークを出すのは InteractMarkers）
// ティックの段では動かない。シーンが 1 回の描画につき 1 回 look を呼ぶ
// （押した瞬間を 1 回だけ拾うため。目と向きも、描画で決まるカメラの値を使う）
// 使うときに増減する持ち物と、視線を当てない自分の体は、同じ物の部品から自分で引く
// 外せる部品（外すと、物を見ても使えない）
class InteractorComponent : public Component{
public:
	explicit InteractorComponent( float reach );	// reach: 手が届く距離（m）。これより遠い物は使えない

	// 目 eye から向き direction（長さ1）へ視線を撃って、見ている使える物を探す。interactPressed なら使う
	// 拾う物は使わずに offer に残す（置く所を選ぶのは呼ぶ側）
	// 漁れる場所も使わずに opened に残す（開けるまで待つ・中を見せるのは呼ぶ側）
	void look( const GameLib::Math::Vector3& eye, const GameLib::Math::Vector3& direction, bool interactPressed );
	const IInteractable* target() const;	// 今見ている使える物（無ければ nullptr）。look の直後に読む（拾われた物は次の世界の update で消えるので、それより後には使わない）
	// 最後の look で E を押した拾う物（無ければ nullptr）。look の直後に読む。世界を進める前に使い終えること
	// （拾い終えた物は世界の update で消える）
	IInteractable* offer() const;
	IInteractable* opened() const;			// 最後の look で E を押した漁れる場所・脱出の車（lootGrid がある物。無ければ nullptr）。読む決まりは offer と同じ

	void start() override;					// Component（InventoryComponent が要る。使える物を探す相手は共有の棚から引く）

private:
	float mReach;
	IInteractFinder* mFinder;		// 借りているだけ（共有の棚の物）
	InventoryComponent* mInventory;	// 同じ物の必須の部品
	const IInteractable* mTarget;	// 借りているだけ
	IInteractable* mOffer;			// 借りているだけ
	IInteractable* mOpened;			// 借りているだけ
};

#endif
