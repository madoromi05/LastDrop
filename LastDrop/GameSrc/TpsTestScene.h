#ifndef INCLUDED_TPS_TEST_SCENE_H
#define INCLUDED_TPS_TEST_SCENE_H

#include "IScene.h"
#include "MeshCache.h"
#include "CollisionWorld.h"
#include "HitWorld.h"
#include "Zombie.h"
#include "Building.h"
#include "FirstPersonCamera.h"
#ifdef _DEBUG
#include "CollisionDebugDrawer.h"
#endif
#include "AudioEngine.h"
#include "GroundMaterial.h"
#include "FootstepEmitter.h"
#include "PlayerFoley.h"
#include <memory>
#include <vector>

class PlayerCharacter;

// 自キャラを一人称(FPS)視点で動かす検証シーン。WASD で移動（Shift で走る）、マウスで視点を回す。
// Escape from Tarkov のように自分の体（腕・銃・脚）が見える方式。視点は頭の骨に付き、歩きの揺れも出る。
// （クラス名・シーン番号は TPS のまま。名前の変更は別作業）
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
	virtual void update();
private:
	void placeObjects();         // 全データの構築後に一度だけ、キャラと背景を作って並べる
	void updateCursorLock();     // Esc / クリックでカーソルロックを切り替える
	void rotateCamera();         // ロック中ならマウスの移動量でカメラを回す（キャラの更新より前）
	void placeCameraAtEye();     // 視点を自キャラの目へ置いて描画ステートへ反映する（キャラの更新より後）
	void updateAudio();          // 聞き手をカメラに合わせ、足音・装備の音を鳴らす（視点を置いた後）
	float fieldOfView() const;   // 覗き具合に合わせた縦の視野角（度）
	void setupLights() const;
	void draw() const;
#ifdef _DEBUG
	void updateCollisionDebug(); // 切り替えの入力を読み、表示中なら当たり判定を描く（他を全部描いた後）
#endif

	// 設計図の倉庫。インスタンスより先に宣言する（＝インスタンスより後に破棄される）。
	MeshCache mCache;
	const SkinnedMeshData* mPlayerData;   // mCache から借りる
	const StaticMeshData* mWeaponData;    // mCache から借りる（自キャラが右手に持つ銃）
	const SkinnedMeshData* mZombieData;   // mCache から借りる
	const StaticMeshData* mBuildingData;  // mCache から借りる

	// 当たり判定の名簿。キャラを借りて持つので、キャラより先に宣言する（＝キャラより後に破棄される）。
	CollisionWorld mWorld;
	HitWorld mHitWorld; // 部位の当たり判定（命中判定用）の名簿。mWorld と同じく、キャラより先に宣言する
	std::unique_ptr< PlayerCharacter > mPlayer;
	std::vector< std::unique_ptr< Zombie > > mZombies; // mWorld が指すので、並べ替えても動かないよう unique_ptr で持つ
	std::vector< Building > mBuildings;
	FirstPersonCamera mCamera;

	// 音。鳴らす係はどれも mAudio を借りるので、mAudio を先に宣言する（＝係より後に破棄される）。
	AudioEngine mAudio;
	GroundMaterialMap mGround;	// 足元の床の材質（検証用に地面へ長方形を並べたもの）
	FootstepEmitter mFootsteps;
	PlayerFoley mFoley;

	bool mReady; // 配置が済んだか
#ifdef _DEBUG
	CollisionDebugDrawer mCollisionDrawer;
	bool mShowCollision; // 当たり判定を表示しているか
#endif
};

#endif
