#ifndef INCLUDED_TPS_TEST_SCENE_H
#define INCLUDED_TPS_TEST_SCENE_H

#include "IScene.h"
#include "MeshCache.h"
#include "SkinnedMeshInstance.h"
#include "StaticMeshInstance.h"
#include "FirstPersonCamera.h"
#include <memory>
#include <vector>

class PlayerCharacter;

// 自キャラを一人称(FPS)視点で動かす検証シーン。WASD で移動（Shift で走る）、マウスで視点を回す。
// Escape from Tarkov のように自分の体（腕・銃・脚）が見える方式。視点は頭の骨に付き、歩きの揺れも出る。
// （クラス名・シーン番号は TPS のまま。名前の変更は別作業）
// 背景は SkinTestScene と同じ建物とゾンビ（動作確認用の目印。当たり判定は無い）。
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
	float fieldOfView() const;   // 覗き具合に合わせた縦の視野角（度）
	void setupLights() const;
	void draw() const;

	// 設計図の倉庫。インスタンスより先に宣言する（＝インスタンスより後に破棄される）。
	MeshCache mCache;
	const SkinnedMeshData* mPlayerData;   // mCache から借りる
	const StaticMeshData* mWeaponData;    // mCache から借りる（自キャラが右手に持つ銃）
	const SkinnedMeshData* mZombieData;   // mCache から借りる
	const StaticMeshData* mBuildingData;  // mCache から借りる

	std::unique_ptr< PlayerCharacter > mPlayer;
	std::vector< std::unique_ptr< SkinnedMeshInstance > > mZombies;
	std::vector< StaticMeshInstance > mBuildings;
	FirstPersonCamera mCamera;

	bool mReady; // 配置が済んだか
};

#endif
