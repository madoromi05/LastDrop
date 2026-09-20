#ifndef INCLUDED_SKIN_TEST_SCENE_H
#define INCLUDED_SKIN_TEST_SCENE_H

#include "IScene.h"
#include "SkinnedMesh.h"
#include "GameLib/PseudoXml/Document.h"
#include "GameLib/Scene/Container.h"
#include "GameLib/Scene/Tree.h"

// SkinnedMesh(CPUスキニング)の動作検証シーン。
// 2ボーンの帯を曲げアニメで動かし、関節から滑らかに曲がるかを目視確認する。
// skintest.txt を1回だけ読み、その要素から Container(骨格/アニメ) と
// SkinnedMesh(頂点/重み) の両方を構築する。既存ゲームには一切触れない。
class SkinTestScene : public IScene{
public:
	SkinTestScene();
	virtual ~SkinTestScene();
	virtual void update();
private:
	void buildOnce(); // ロード完了後に一度だけ骨格・メッシュを構築する

	GameLib::PseudoXml::Document mDocument;
	GameLib::Scene::Container mContainer;
	GameLib::Scene::Tree mSkeleton;
	SkinnedMesh mMesh;
	float mTime;
	bool mReady; // 構築が済んだか
};

#endif
