#ifndef INCLUDED_SKIN_TEST_SCENE_H
#define INCLUDED_SKIN_TEST_SCENE_H

#include "GameSrc/Scene/IScene.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Render/StaticMesh/StaticMeshInstance.h"
#include <memory>
#include <vector>

class GameResources;
class MeshCache;
class SkinnedMeshData;
class StaticMeshData;

// メッシュ描画の動作検証シーン。
// 同じゾンビ(SkinnedMesh)を複数体、同じ建物(StaticMesh)を複数軒並べ、
// ファイルの読み込みとテクスチャは1回だけで使い回せていることを確かめる。
// 本編（GameScene）とは別の、描画だけの確認用。起動シーン（Main.cpp の kStartScene）を書き換えたときだけ開く。
class SkinTestScene : public IScene{
public:
	// resources: メッシュの倉庫を借りる。所有権は受け取らない。nullptr 不可。このシーンより長く生きていること。
	explicit SkinTestScene( GameResources* resources );
	virtual ~SkinTestScene();
	virtual void beginFrame();
	virtual void update();
	virtual void draw();
private:
	void placeObjects();         // 全データの構築後に一度だけ、インスタンスを作って並べる
	void setupCameraAndLights();

	MeshCache* mMeshes;                   // 設計図の倉庫。借りているだけ（GameResources が持つ）
	const SkinnedMeshData* mZombieData;   // mMeshes から借りる
	const StaticMeshData* mBuildingData;  // mMeshes から借りる

	// 置いた1体・1軒。ゾンビは頂点バッファと骨格を持つのでコピー禁止＝unique_ptr で持つ。
	std::vector< std::unique_ptr< SkinnedMeshInstance > > mZombies;
	std::vector< StaticMeshInstance > mBuildings;

	bool mReady; // 配置が済んだか
};

#endif
