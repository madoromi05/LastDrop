#ifndef INCLUDED_SKIN_TEST_SCENE_H
#define INCLUDED_SKIN_TEST_SCENE_H

#include "IScene.h"
#include "MeshCache.h"
#include "SkinnedMeshInstance.h"
#include "StaticMeshInstance.h"
#include <memory>
#include <vector>

// メッシュ描画の動作検証シーン。
// 同じゾンビ(SkinnedMesh)を複数体、同じ建物(StaticMesh)を複数軒並べ、
// ファイルの読み込みとテクスチャは1回だけで使い回せていることを確かめる。
// 既存ゲームには一切触れない。
class SkinTestScene : public IScene{
public:
	SkinTestScene();
	virtual ~SkinTestScene();
	virtual void beginFrame();
	virtual void update();
	virtual void draw();
private:
	void placeObjects();         // 全データの構築後に一度だけ、インスタンスを作って並べる
	void setupCameraAndLights();

	// 設計図の倉庫。インスタンスより先に宣言する（＝インスタンスより後に破棄される）。
	MeshCache mCache;
	const SkinnedMeshData* mZombieData;   // mCache から借りる
	const StaticMeshData* mBuildingData;  // mCache から借りる

	// 置いた1体・1軒。ゾンビは頂点バッファと骨格を持つのでコピー禁止＝unique_ptr で持つ。
	std::vector< std::unique_ptr< SkinnedMeshInstance > > mZombies;
	std::vector< StaticMeshInstance > mBuildings;

	bool mReady; // 配置が済んだか
};

#endif
