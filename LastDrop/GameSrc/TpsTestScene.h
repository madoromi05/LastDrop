#ifndef INCLUDED_TPS_TEST_SCENE_H
#define INCLUDED_TPS_TEST_SCENE_H

#include "IScene.h"
#include "MeshCache.h"
#include "CollisionWorld.h"
#include "HitWorld.h"
#include "Zombie.h"
#include "ActorList.h"
#include "Building.h"
#include "Pickup.h"
#include "EscapeCar.h"
#include "Inventory.h"
#include "Interactor.h"
#include "UiScreen.h"
#include "FirstPersonCamera.h"
#ifdef _DEBUG
#include "CollisionDebugDrawer.h"
#endif
#include "AudioEngine.h"
#include "GroundMaterial.h"
#include "FootstepEmitter.h"
#include "PlayerFoley.h"
#include "PlayerCommand.h"
#include "PostProcess.h"
#include <memory>
#include <vector>

class PlayerCharacter;
class UiLabel;
class IInteractable;

// 自キャラを一人称(FPS)視点で動かす検証シーン。WASD で移動（Shift で走る）、マウスで視点を回す。
// Escape from Tarkov のように自分の体（腕・銃・脚）が見える方式。視点は頭の骨に付き、歩きの揺れも出る。
// （クラス名・シーン番号は TPS のまま。名前の変更は別作業）
// ゲームループ（とりあえずここで遊ぶ）: 候補点から選んだ場所にガソリンが出る。E で拾い（1缶ずつ）、車まで運んで E で給油する。
// 決まった数を入れるとクリア画面へ進む。左下に車のガソリンと持ち物、見ている物の案内を照準の下に出す。
// 背景は SkinTestScene と同じ建物とゾンビ（動作確認用の目印）。どちらも当たり判定があり、自キャラは当たると沿って滑る。
// 音も確かめられる: 足音（床の材質で変わる）・装備の音。
// 環境音（AmbienceSystem）は今は鳴らしていない。クラスは残してあるので、メンバに戻せば再び鳴る。
//
// Debug ビルドだけ、F1（InputDebugCollision）で当たり判定の表示を切り替えられる（Release にはキーも描画も残さない）。
//
// マウスはカーソルロック（GameLib::Input::Manager）で読む。Esc で解除、クリックで再ロック。
// 解除中は視点を回さない（カーソルが自由に動く間の移動量を視点に入れると、画面外の操作で回ってしまうため）。
class TpsTestScene : public IScene{
public:
	TpsTestScene();
	virtual ~TpsTestScene();
	virtual void beginFrame();
	virtual void update();
	virtual void draw();
private:
	void placeObjects();         // 全データの構築後に一度だけ、キャラと背景を作って並べる
	void updateCursorLock();     // Esc / クリックでカーソルロックを切り替える
	void rotateCamera();         // ロック中ならマウスの移動量でカメラを回す（キャラの更新より前）
	void placeCameraAtEye();     // 視点を自キャラの目へ置いて描画ステートへ反映する（キャラの更新より後）
	void updateBodySounds();     // 足音・装備の音を鳴らす（1ティックに1回。キャラの更新の後）
	void updateListener();       // 聞き手をカメラに合わせて音を更新する（1回の描画につき1回。視点を置いた後）
	float fieldOfView() const;   // 覗き具合に合わせた縦の視野角（度）
	void setupLights() const;
	void drawWorld() const;      // キャラと背景を描くだけ（何回呼んでも同じ絵になる）
	void placeMissionObjects();  // 脱出用の車と、候補点から選んだ場所にガソリンを置く（placeObjects から）
	void buildHud();             // HUD の部品を並べる（コンストラクタから一度だけ）
	void updateInteraction();    // 見ている物の案内を出し、E で使う。車が走れるようになったらクリア画面へ（1回の描画につき1回）
	IInteractable* resolveInteractable( const HitWorld::Hit& hit ); // 視線が当たった物のうち使える物（Interactor から呼ばれる）
	void drawHud();              // HUD の文字を今の状態にして描く（3D の後）
#ifdef _DEBUG
	void toggleCollisionDebug(); // 表示の切り替えの入力を読む（1回の描画につき1回）
	void drawCollisionDebug();   // 表示中なら当たり判定を描く（他を全部描いた後）
	bool debugRaycast( HitWorld::Hit* hit ) const; // 画面中央（視線）へ確認用の線分を撃つ。自分には当てない
	void castDebugRay();         // 画面中央から線分を撃ち、当たった所を当たり判定の表示に足す（弾の当たり判定の確認用）
	void despawnLookedAtZombie(); // 視線の先のゾンビを消す（F2。ActorList で消す仕組みの確認用）
#endif

	// 設計図の倉庫。インスタンスより先に宣言する（＝インスタンスより後に破棄される）。
	MeshCache mCache;
	const SkinnedMeshData* mPlayerData;   // mCache から借りる
	const StaticMeshData* mWeaponData;    // mCache から借りる（自キャラが右手に持つ銃）
	const SkinnedMeshData* mZombieData;   // mCache から借りる
	const StaticMeshData* mBuildingData;  // mCache から借りる
	const StaticMeshData* mVehicleData;   // mCache から借りる（脱出用の車）
	const StaticMeshData* mFuelCanData;   // mCache から借りる（ガソリンの携行缶）

	// 当たり判定の名簿。キャラを借りて持つので、キャラより先に宣言する（＝キャラより後に破棄される）。
	CollisionWorld mWorld;
	HitWorld mHitWorld; // 部位の当たり判定（命中判定用）の名簿。mWorld と同じく、キャラより先に宣言する
	std::unique_ptr< PlayerCharacter > mPlayer;
	// ゾンビ。途中で消せるよう ActorList で持つ（名簿への登録・解除もここが行う）。名簿より後に宣言する。
	ActorList< Zombie > mZombies;
	std::vector< Building > mBuildings;
	// ゲームループ。どれも名簿（mWorld・mHitWorld）を借りるので、名簿より後に宣言する。
	ActorList< Pickup > mPickups;          // 落ちているガソリン（拾うと消える）
	std::unique_ptr< EscapeCar > mEscapeCar; // 名簿が自分の箱を指すので、置いた後は動かさない（unique_ptr で持つ）
	Inventory mInventory;
	Interactor mInteractor;
	UiScreen mHud;
	UiLabel* mPromptLabel; // mHud が持つ部品を借りる（文字を毎回書き換える）
	UiLabel* mTankLabel;
	UiLabel* mCarryLabel;
	FirstPersonCamera mCamera;
	PostProcess mPostProcess; // シーンを中間の描画先へ描き、シェーダを通して画面へ写す

	// 音。鳴らす係はどれも mAudio を借りるので、mAudio を先に宣言する（＝係より後に破棄される）。
	AudioEngine mAudio;
	GroundMaterialMap mGround;	// 足元の床の材質（検証用に地面へ長方形を並べたもの）
	FootstepEmitter mFootsteps;
	PlayerFoley mFoley;

	// 自キャラへの操作。押した瞬間の入力（holsterPressed）は、次の update で使うまで貯めておく。
	// 1回の描画の間に update が0回でも入力が消えず、2回あっても2回反応しないようにするため。
	PlayerCommand mCommand;
	bool mReady; // 配置が済んだか
#ifdef _DEBUG
	CollisionDebugDrawer mCollisionDrawer;
	bool mShowCollision; // 当たり判定を表示しているか
#endif
};

#endif
