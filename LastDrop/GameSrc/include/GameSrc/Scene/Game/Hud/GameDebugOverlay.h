#ifndef INCLUDED_GAME_DEBUG_OVERLAY_H
#define INCLUDED_GAME_DEBUG_OVERLAY_H

#include "GameSrc/Hit/HitWorld.h"
#ifdef ZF_EDITOR
#include "GameSrc/DebugMode/CollisionDebugDrawer.h"
#include "GameSrc/DebugMode/ZombiePathDrawer.h"
#include "GameSrc/DebugMode/NavMeshDrawer.h"
#include "GameSrc/DebugMode/LightDebugDrawer.h"
#include "GameSrc/DebugMode/TownEditPreview.h"
#include "GameSrc/Scene/Game/Hud/ZombieHealthBars.h"
#endif

class GameWorld;
class LightList;
class FirstPersonCamera;
class UiRenderer;

// 本編の確認用の表示。画面の左下（状態表示の上）に文字（DebugScreen）で出すものと、Debug・Profile ビルドだけの当たり判定の表示をまとめる
// ・どのビルドでも出す文字: シェーダのコンパイルエラー（あるときだけ）
// ・Debug・Profile ビルド（ZF_EDITOR）だけ。どれを出すかは F6 の Debug タブで切り替える（DebugSwitches）:
//   showCollision: 当たり判定の表示。表示中は画面中央（視線）へ確認用の線分を撃ち、当たった所と相手を出す
//   showPickups: 落ちている物（ガソリン）の当たり判定の箱を、壁や建物を透かして表示する
//   showCollision とは別で、どちらか片方だけでも出せる（当たり判定の線は手前の物に隠れるので、遠くの缶はこちらでないと見つけられない）
//   showZombieInfo: ゾンビの体力（全体のバーと部位の数字）を頭の上に出す（ZombieHealthBars）
//   表示中は左下の文字に、湧く仕組みの様子（立っている数 / 目標の数・次に湧くまで。ZombieSpawner）も出す
//   表示中は、建物を回り込んでいるゾンビの通り道・引っかかりの印も描く（ZombiePathDrawer と番号の文字）
//   showNavMesh: ゾンビが歩ける床（ナビメッシュ）の多角形の辺を薄い緑の線で描く（NavMeshDrawer）
//   表示中は、左上の体の図の右に、自キャラの部位ごとの体力の数字も出す
//   showTownInfo: 街灯などの点光源の置き場所と届く範囲を描く（LightDebugDrawer。照らしている物は黄色、上限に入らず消えている物は灰色）
//   表示中は左下の文字に簡易 PVS の様子を、漁れる場所の上に中身の名前を出す
//   F6 の Town タブで選んでいる物は、切り替えに関係なく枠で示す（TownEditPreview。F6 を開いている間だけ）
//   Release には描画を残さない（関数ごと無い）
// 世界は読むだけで、変えない
class GameDebugOverlay{
public:
	GameDebugOverlay();

	// シェーダのコンパイルエラーを、左下に 1 行で出す。1回の描画につき1回
	// shaderError: エラーの文。空文字列なら何も出さない。nullptr 不可
	void drawShaderError( const char* shaderError ) const;

#ifdef ZF_EDITOR
	void drawCollision( const GameWorld& world, const FirstPersonCamera& camera );									// 表示中のもの（当たり判定・落ちている物の箱・ゾンビの通り道）と、Town タブで選んでいる物の枠を描く。他を全部描いた後、同じ描画先（PostProcess の beginScene〜endScene の間）で呼ぶ
	void drawLights( const LightList& lights, const FirstPersonCamera& camera );									// 町の表示中なら、点光源の置き場所と届く範囲を描く。lights：このフレームに bind した一覧。呼ぶ場所の決まりは drawCollision と同じ
	// 表示中なら（F3）、ゾンビの体力を renderer に積む。PostProcess::endScene の後、HUD を描く前に呼ぶ
	// （積むだけ。HUD の描画の UiRenderer::draw でまとめて描かれ、HUD の文字の下になる）。renderer: nullptr 不可
	void addZombieHealth( const GameWorld& world, const FirstPersonCamera& camera, UiRenderer* renderer ) const;
	// 表示中なら（F3）、画面の左上に、自キャラの体力の数字と状態（Fine など）を renderer に積む
	// 呼ぶ場所の決まりは addZombieHealth と同じ。renderer: nullptr 不可
	void addPlayerHealth( const GameWorld& world, UiRenderer* renderer ) const;
	void addLootContents( const GameWorld& world, const FirstPersonCamera& camera, UiRenderer* renderer ) const;	// 町の表示中なら、漁れる場所の上に中身の名前を壁越しに積む。呼ぶ場所の決まりは addZombieHealth と同じ
	void addTownStatus( const GameWorld& world, const FirstPersonCamera& camera ) const;							// 町の表示中なら、左下の文字に簡易 PVS の様子（目のいるます・隠した建物の数）を出す。1 回の描画につき 1 回
	// ゾンビの表示中なら、引っかかったゾンビ（ZombieMoveTrace の印が出ている間）の頭の上に、物の番号（GameObject::id）を壁越しに積む
	// 呼ぶ場所の決まりは addZombieHealth と同じ。renderer: nullptr 不可
	void addZombieStuckLabels( const GameWorld& world, const FirstPersonCamera& camera, UiRenderer* renderer ) const;

private:
	bool raycast( const GameWorld& world, const FirstPersonCamera& camera, HitWorld::Hit* hit ) const;	// 画面中央（視線）へ確認用の線分を撃つ。自キャラには当てない（目の位置は自分の頭の箱の中にあるため）
	void castRay( const GameWorld& world, const FirstPersonCamera& camera );							// 線分を撃ち、当たった所を当たり判定の表示に足し、相手を文字で出す（弾の当たり判定の確認用）

	CollisionDebugDrawer mCollisionDrawer;
	ZombiePathDrawer mZombiePathDrawer;
	NavMeshDrawer mNavMeshDrawer;
	LightDebugDrawer mLightDrawer;
	TownEditPreview mTownEditPreview;
	ZombieHealthBars mZombieHealthBars;
	std::vector< BoxCollider > mPickupBoxes; // 描く箱を世界から受け取る作業領域（毎回の描画で確保し直さないために持つ）
#endif
};

#endif
