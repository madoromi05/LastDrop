#ifndef INCLUDED_FIRST_PERSON_CONTROL_H
#define INCLUDED_FIRST_PERSON_CONTROL_H

#include "GameSrc/Player/Camera/FirstPersonCamera.h"
#include "GameSrc/Player/PlayerCommand.h"
#include "GameSrc/Player/Camera/AimSway.h"
#include "GameSrc/Player/Camera/CommandReader.h"
#include "GameSrc/Player/Camera/ViewEffects.h"
#include "GameSrc/Player/Camera/LookPull.h"
#include "GameLib/Math/Vector3.h"

struct GameSettings;

// 一人称の操作の窓口。入力（Pad・マウス）を読んで、視点（FirstPersonCamera）を回し、自キャラへの操作（PlayerCommand）を作る
// キー割り当てと「キャラの動き・カメラの向き」をつなぐのはここだけ（PlayerCharacter も FirstPersonCamera も入力デバイスを知らない）
// 中身は係へ任せる（SummaryPlan/個別仕様/設計と分割/HighPrioritySplit_Plan.txt 6 章。外から呼ぶ口は前のまま）：
//   操作を作る … CommandReader　／　反動・揺れ・照準のぶれ … ViewEffects　／　相手の顔へ向ける … LookPull
//   ここに残るのは、視点を回す（感度）・カーソルの固定・カメラを置く、と係を呼ぶ順番
//
// マウスはカーソルロック（GameLib::Input::Manager）で読む。解除は一時停止（GameScene が unlockCursor）、解除中のクリックで再ロック
// 解除中は視点を回さない（カーソルが自由に動く間の移動量を視点に入れると、画面外の操作で回ってしまうため）
// 解除中は撃たない。ロックし直すためのクリックも、離すまでは撃つ操作にしない
//
// 呼ぶ順（IScene の beginFrame → update → draw に合わせる）:
//   readInput（1回の描画につき1回）→ command を使ってキャラを動かす → consumePressed・updateViewEffects（update のたび）→ placeCamera（描く前）
class FirstPersonControl{
public:
	// startYaw: 始めに見る水平の向き（度。FirstPersonCamera::yaw と同じ約束）。キャラの向いている方を渡す
	// settings: 視点の感度を毎回読む（設定画面で変えたらすぐ効く）。所有権は受け取らない。nullptr 不可。これより長く生きていること
	FirstPersonControl( float startYaw, const GameSettings* settings );
	// シーンを抜けたらカーソルを返す（隠れたまま・閉じ込められたままにしない）
	~FirstPersonControl();
	FirstPersonControl( const FirstPersonControl& ) = delete;
	FirstPersonControl& operator=( const FirstPersonControl& ) = delete;

	void lockCursor();				// カーソルをロックして、視点を回せるようにする（世界を並べ終えて操作を始めるときに呼ぶ）
	// カーソルのロックを外す（一時停止でメニューを出すとき）
	void unlockCursor();
	bool isCursorLocked() const;
	bool isFireSuppressed() const;	// 押しっぱなしの抑止の最中か（離すまで撃たない。1 発目が出ない件の記録 FireProbe が読むだけ）

	// 入力を読む。1回の描画につき1回、Pad::update の後に呼ぶ（update の回数に合わせると、2回進めた描画で視点が2倍回るため）
	// カーソルロックの切り替え → ロック中ならマウスの移動量で視点を回す → 操作を command へまとめる
	// aimWeight: 覗き具合（0〜1。PlayerCharacter::aimWeight）。ズーム中は視野角の比で感度を下げるのに使う
	void readInput( float aimWeight );
	// 自キャラを操作しない間（確認用の FreeCamera を使っている間）に、readInput の代わりに呼ぶ
	// カーソルロックの切り替えだけ行い、視点は回さず、command は「何もしない」（その場で立ち止まる・撃たない・覗かない）にする
	// 撃つ入力は一度離すまで止める（観察用カメラから戻った瞬間に、押していたクリックで撃ち出さないように）
	void readInputIdle();
	// command を「何もしない」にし、撃つ入力を一度離すまで止める（readInputIdle の後半。カーソルロックには触らない）
	// 一時停止した瞬間に呼ぶ（メニューを押したクリックで、戻った瞬間に撃ち出さないように）
	void clearInput();
	// 自キャラへの操作。押した瞬間の入力（holsterPressed）は、consumePressed で消すまで貯めてある
	// 1回の描画の間に update が0回でも入力が消えず、2回あっても2回反応しないようにするため
	const PlayerCommand& command() const;
	void consumePressed();				// 押した瞬間の入力を消す。command をキャラへ渡した後（update のたび）に呼ぶ
	// 狙いのずれ（1 発ごとに跳ねて戻る反動・連射でたまる弾道のずれ・噛まれた怯み）と、見える絵の揺れ、
	// 照準のぶれ（銃と弾の向きだけ。AimSway）を進めてカメラへ渡す
	// 1ティックに1回、キャラを動かした後（flinch・shakeOnGrab の後）に呼ぶ
	// shotRecoil: このティックに1発撃ったなら、その銃の反動の大きさ（WeaponSpec::recoilScale。ライフルが 1）。撃っていなければ 0
	// body: 自キャラの今の状態（PlayerAimSwaySource::read。自キャラがいなければ既定のまま＝ぶれない）
	void updateViewEffects( float shotRecoil, const AimSwayBody& body );
	void flinch();						// 噛まれた（這うゾンビの腕が当たった）瞬間に呼ぶ。狙いを跳ねさせ（弾の向きも動く）、絵を大きく揺らす。次の updateViewEffects から効く
	void shakeOnGrab();					// つかまれた瞬間に呼ぶ。絵だけを揺らす（狙いは lookToward で相手の顔へ向けるので、跳ねさせない）
	// 撃ったゾンビの部位が飛んだ・頭が弾けた瞬間に呼ぶ。絵だけを少し揺らす（弾道は動かさない）
	// headBurst: 頭が弾けた（部位が飛んだより少し大きく揺らす）
	void shakeOnHeavyHit( bool headBurst );
	// 視点を target（ワールド座標）の方へ、決まった時間をかけて向け始める（ゾンビにつかまれた瞬間に、その顔を見る）
	// 向けている間もマウスで回せる（回した分はそのまま足される）。続けて呼ぶと、その時点の向きから向け直す
	void lookToward( const GameLib::Math::Vector3& target );
	void updateLookPull();				// lookToward の向きへ1ティックぶん回す。1ティックに1回、キャラを動かした後に呼ぶ
	void setEyeDrop( float meters );	// 目の位置から下げる長さ（m。0 以上）。死んだ後に膝から崩れて見せるのに使う。placeCamera で渡された目の位置から引く

	// 視点を目へ置き、覗き具合に合わせた視野角にして、描画ステートへ反映する。描く前、キャラを動かした後に呼ぶ
	// eye: 目の位置（ワールド座標。PlayerCharacter::eyePosition）。nullptr なら前に置いた位置のまま（キャラがいないとき）
	// aimWeight: 覗き具合（0〜1）。覗いている間は視野を狭めて少しズームする
	void placeCamera( const GameLib::Math::Vector3* eye, float aimWeight );
	const FirstPersonCamera& camera() const;

private:
	void updateCursorLock();	// 解除中のクリックでロックし直す（readInput と readInputIdle で共通）。解除は一時停止が行う（ESC は一時停止のキー）

	const GameSettings* mSettings; // 借りているだけ（視点の感度）
	FirstPersonCamera mCamera;
	CommandReader mCommands; // 操作（移動・押した瞬間・撃つの抑止）
	ViewEffects mEffects;    // 反動・揺れ・照準のぶれ
	LookPull mLookPull;      // つかまれたときに相手の顔へ向ける
	float mEyeDrop; // setEyeDrop の値（m）
};

#endif
