#ifndef INCLUDED_GAME_DEBUG_OVERLAY_H
#define INCLUDED_GAME_DEBUG_OVERLAY_H

#include "GameSrc/Hit/HitWorld.h"
#ifdef ZF_EDITOR
#include "GameSrc/DebugMode/CollisionDebugDrawer.h"
#include "GameSrc/DebugMode/ZombiePathDrawer.h"
#include "GameSrc/Scene/Game/ZombieHealthBars.h"
#endif

class GameWorld;
class FirstPersonCamera;
class UiRenderer;

// 本編の確認用の表示。画面の左下（状態表示の上）に文字（DebugScreen）で出すものと、Debug・Profile ビルドだけの当たり判定の表示をまとめる。
// ・どのビルドでも出す文字: 操作説明、シェーダのコンパイルエラー（あるときだけ）
// ・Debug・Profile ビルド（ZF_EDITOR）だけ:
//   F1（InputDebugCollision）で当たり判定の表示を切り替える。表示中は画面中央（視線）へ確認用の線分を撃ち、
//   当たった所と相手を出す。
//   F2（InputDebugPickupCollision）で、落ちている物（ガソリン）の当たり判定の箱を、壁や建物を透かして表示する切り替え。
//   F1 の表示とは別で、どちらか片方だけでも出せる（F1 の線は手前の物に隠れるので、遠くの缶は F2 でないと見つけられない）。
//   F3（InputDebugZombieHealth）で、ゾンビの体力（全体のバーと部位の数字）を頭の上に出す切り替え（ZombieHealthBars）。
//   表示中は左下の文字に、湧く仕組みの様子（立っている数 / 目標の数・次に湧くまで。ZombieSpawner）も出す。
//   表示中は、建物を回り込んでいるゾンビの通り道も水色の線で描く（ZombiePathDrawer）。
//   表示中は、左上の体の図の右に、自キャラの部位ごとの体力の数字も出す。
//   表示の切り替えのキーの一覧は画面に出さない（2026-10-21 ユーザー指定）。
//   Release にはキーも描画も残さない（関数ごと無い）。
// 世界は読むだけで、変えない。
class GameDebugOverlay{
public:
	GameDebugOverlay();

	// 画面に出す文字の材料。
	struct TextState{
		bool cursorLocked;            // カーソルをロックしているか（操作説明を切り替える）
		const char* shaderError;      // シェーダのコンパイルエラー。空文字列なら出さない。nullptr 不可
	};
	// 文字を出す。1回の描画につき1回。
	void drawText( const TextState& state ) const;

#ifdef ZF_EDITOR
	// 入力を読んで、表示を切り替える（F1・F2・F3）。1回の描画につき1回、Pad::update の後に呼ぶ。
	void handleInput();
	// 表示中のもの（F1 の当たり判定・F2 の落ちている物の箱・F3 のゾンビの通り道）を描く。他を全部描いた後、同じ描画先（PostProcess の beginScene〜endScene の間）で呼ぶ。
	void drawCollision( const GameWorld& world, const FirstPersonCamera& camera );
	// 表示中なら（F3）、ゾンビの体力を renderer に積む。PostProcess::endScene の後、HUD を描く前に呼ぶ
	// （積むだけ。HUD の描画の UiRenderer::draw でまとめて描かれ、HUD の文字の下になる）。renderer: nullptr 不可。
	void addZombieHealth( const GameWorld& world, const FirstPersonCamera& camera, UiRenderer* renderer ) const;
	// 表示中なら（F3）、画面の左上に、自キャラの体力の数字と状態（Fine など）を renderer に積む。
	// 呼ぶ場所の決まりは addZombieHealth と同じ。renderer: nullptr 不可。
	void addPlayerHealth( const GameWorld& world, UiRenderer* renderer ) const;

private:
	// 画面中央（視線）へ確認用の線分を撃つ。自キャラには当てない（目の位置は自分の頭の箱の中にあるため）。
	bool raycast( const GameWorld& world, const FirstPersonCamera& camera, HitWorld::Hit* hit ) const;
	// 線分を撃ち、当たった所を当たり判定の表示に足し、相手を文字で出す（弾の当たり判定の確認用）。
	void castRay( const GameWorld& world, const FirstPersonCamera& camera );

	CollisionDebugDrawer mCollisionDrawer;
	ZombiePathDrawer mZombiePathDrawer;
	ZombieHealthBars mZombieHealthBars;
	bool mShowCollision; // 当たり判定を表示しているか
	bool mShowPickups;   // 落ちている物の当たり判定を、透かして表示しているか
	bool mShowZombieHealth; // ゾンビの体力を表示しているか
	std::vector< BoxCollider > mPickupBoxes; // 描く箱を世界から受け取る作業領域（毎回の描画で確保し直さないために持つ）
#endif
};

#endif
