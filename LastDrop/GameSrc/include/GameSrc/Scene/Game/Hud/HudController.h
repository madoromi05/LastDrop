#ifndef INCLUDED_HUD_CONTROLLER_H
#define INCLUDED_HUD_CONTROLLER_H

#include "GameSrc/Scene/Game/Hud/RaidHud.h"
#include "GameSrc/Scene/Game/Hud/AmmoHud.h"
#include "GameSrc/Scene/Game/Hud/AmmoHudVisibility.h"
#include "GameSrc/Scene/Game/Hud/InteractMarkers.h"

class GameWorld;
class Inventory;
class UiRenderer;
class FirstPersonCamera;
class RaidSession;
class IInteractable;
struct DayConditionEffect;
namespace GameLib{ namespace Math{ class Vector3; } }

// 遊んでいる間の画面の読み取り表示（HUD）をまとめる。始めの知らせ・脱出と漁る棒（RaidHud）、右下の弾数（AmmoHud）、
// 使える物の上のマーク（InteractMarkers）。止めている間（一時停止・カバン・漁り）は出さない＝GameScene がこのクラスの draw を呼ばない。
// 攻撃された赤み・暗転（PlayerDamageView）や観察用の表示（GameDebugOverlay）は別系統。
class HudController{
public:
	// world/inventory: 借りるだけ。ui: 描画に使う。どれも nullptr 不可・この Controller より長く生きていること。
	HudController( GameWorld* world, Inventory* inventory, UiRenderer* ui );
	// 右下の弾数を出す時間と濃さを進める。wanted: 今出したいか（覗いている・リロード中）。update のビューの段で 1 回。
	void updateAmmoVisibility( bool wanted );
	// 使える物の上のマークを更新する（見ている物・持ち物で色が変わる）。eye: 目の位置。target: 今見ている使える物。
	void updateMarkers( const GameLib::Math::Vector3& eye, const IInteractable* target );
	// マークを消す（自キャラがいない・死んだとき）。
	void clearMarkers();
	// 遊んでいる間の HUD を積む（マーク → 弾数 → 始めの知らせと棒の順。1 回の描画で UiRenderer::draw がまとめて描く）。
	// searchProgress: 漁り/給油の棒（出さないなら負。RaidHud 既定）。waitingKind: その文字（漁る/給油）。
	void draw( const FirstPersonCamera& camera, const RaidSession& session, const DayConditionEffect& dayCondition,
		float searchProgress, RaidHud::ProgressKind waitingKind );

private:
	// 手に持っている銃の弾数を、右下の HUD へ積む（出す時間でなければ何もしない）。
	void addAmmo();

	GameWorld* mWorld;     // 借りているだけ
	Inventory* mInventory; // 借りているだけ
	UiRenderer* mUi;       // 借りているだけ（マークを積むのに使う）
	RaidHud mHud;
	AmmoHud mAmmoHud;                     // 右下の銃の絵と弾数（AmmoHud_Plan.txt）
	AmmoHudVisibility mAmmoHudVisibility; // 弾数を出す時間と濃さ（覗いている・リロード中と、その後 2 秒）
	InteractMarkers mInteractMarkers;     // 使える物の上のマーク（文字の案内の代わり）
};

#endif
