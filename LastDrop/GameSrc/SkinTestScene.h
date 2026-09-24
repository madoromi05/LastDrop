#ifndef INCLUDED_SKIN_TEST_SCENE_H
#define INCLUDED_SKIN_TEST_SCENE_H

#include "IScene.h"
#include "SkinnedMesh.h"
#include "StaticMeshData.h"
#include "StaticMeshInstance.h"
#include "GameLib/PseudoXml/Document.h"
#include "GameLib/Scene/Container.h"
#include "GameLib/Scene/Tree.h"

// メッシュ描画の動作検証シーン。
// ゾンビ(SkinnedMesh: CPUスキニング＋歩きアニメ)と、その背景の建物(StaticMesh)を一緒に描く。
// character.txt / building.txt をそれぞれ1回だけ読み、両方のロードが終わってから構築する。
// 既存ゲームには一切触れない。
class SkinTestScene : public IScene{
public:
	SkinTestScene();
	virtual ~SkinTestScene();
	virtual void update();
private:
	bool isAllLoaded();  // 全ファイルのロードが終わったか（isReady() がロードを進めるので非const）
	void buildOnce();    // ロード完了後に一度だけ骨格・メッシュを構築する
	void setupCameraAndLights();

	// ゾンビ
	GameLib::PseudoXml::Document mDocument;
	GameLib::Scene::Container mContainer;
	GameLib::Scene::Tree mSkeleton;
	SkinnedMesh mMesh;

	// 建物。mBuilding は mBuildingData を借りて描くので、宣言順（=構築順）はデータが先。
	GameLib::PseudoXml::Document mBuildingDocument;
	StaticMeshData mBuildingData;
	StaticMeshInstance mBuilding;

	float mTime;
	bool mReady; // 構築が済んだか
};

#endif
