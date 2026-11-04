#ifndef INCLUDED_GAME_HUD_H
#define INCLUDED_GAME_HUD_H

#include "GameSrc/UI/UiScreen.h"

class UiLabel;
class UiRenderer;
class FuelTankComponent;

// 本編の HUD。車のガソリン（左下）を出す。見ている物の文字の案内は出さない（物の上のマーク InteractMarkers に替えた。InteractMarker_Plan.txt）。
// 自キャラの体力は出さない（バイオハザード式で隠す。心電図の表示は 2026-10-26 にユーザーが保留にした）。
// 持ち物も出さない（カバンの画面で見る。弾数を出さない 10-22 の決定と揃えた。GridInventory_Plan.txt Q11。2026-11-04 に「Carrying」を消した）。
// 表示するだけで、ゲームの状態は変えない（車を読むだけ）。文言と並べ方を変えるときは、このクラスだけを直す。
class GameHud{
public:
	// renderer: 描く道具（UiScreen へ渡す）。所有権は受け取らない。nullptr 不可。この HUD より長く生きていること。
	explicit GameHud( UiRenderer* renderer );
	GameHud( const GameHud& ) = delete;
	GameHud& operator=( const GameHud& ) = delete;

	// 文字を今の状態にして描く。1回の描画につき1回、3D の後に呼ぶ（上に重ねるため）。
	// car: 脱出用の車。nullptr なら車のガソリンの行は書き換えない（車を置けなかったとき）。所有権は受け取らない。
	void draw( const FuelTankComponent* car );

private:
	UiScreen mScreen;
	UiLabel* mTankLabel; // mScreen が持つ部品を借りる（文字を毎回書き換える）
};

#endif
