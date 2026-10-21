#ifndef INCLUDED_GAME_HUD_H
#define INCLUDED_GAME_HUD_H

#include "GameSrc/UI/UiScreen.h"

class UiLabel;
class UiRenderer;
class EscapeCar;
class Inventory;
class PlayerHealth;

// 本編の HUD。照準、見ている物の案内（照準の下）、車のガソリンと持ち物（左下）、左上に自キャラの体の図（PlayerBodyFigure）を出す。
// 表示するだけで、ゲームの状態は変えない（車と持ち物を読むだけ）。文言と並べ方を変えるときは、このクラスだけを直す。
class GameHud{
public:
	// renderer: 描く道具（UiScreen へ渡す）。所有権は受け取らない。nullptr 不可。この HUD より長く生きていること。
	explicit GameHud( UiRenderer* renderer );
	GameHud( const GameHud& ) = delete;
	GameHud& operator=( const GameHud& ) = delete;

	// 文字を今の状態にして描く。1回の描画につき1回、3D の後に呼ぶ（上に重ねるため）。
	// prompt: 見ている物の案内（Interactor::prompt。見ている物が無ければ空文字列）。nullptr 不可。
	// car: 脱出用の車。nullptr なら車のガソリンの行は書き換えない（車を置けなかったとき）。所有権は受け取らない。
	// health: 自キャラの体力。nullptr なら体の図を出さない（自キャラを置けなかったとき）。所有権は受け取らない。
	void draw( const char* prompt, const EscapeCar* car, const Inventory& inventory, const PlayerHealth* health );

private:
	UiScreen mScreen;
	UiRenderer* mRenderer; // 借りているだけ（体の図の四角を積む。mScreen が描く）
	UiLabel* mPromptLabel; // mScreen が持つ部品を借りる（文字を毎回書き換える）
	UiLabel* mTankLabel;
	UiLabel* mCarryLabel;
};

#endif
