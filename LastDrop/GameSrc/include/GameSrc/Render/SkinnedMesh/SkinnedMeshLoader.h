#ifndef INCLUDED_SKINNED_MESH_LOADER_H
#define INCLUDED_SKINNED_MESH_LOADER_H

#include "GameLib/PseudoXml/Element.h"

class SkinnedMeshData;
class TextureCache;

// ニセxmlの <Container> を読み、SkinnedMeshData を使える状態（finish 済み）まで構築するローダ。
// <SkinnedMesh>（頂点/重み/見た目）はここで読み、<Tree>/<Animation> は GameLib の Container に読ませて
// データに持たせる。SkinnedMeshData 本体をファイル形式に依存させないため、読み込み処理はここに分離する。
class SkinnedMeshLoader{
public:
	// container(<Container>要素)から data を構築する。
	// data: 出力先。nullptr 不可。所有権は受け取らない。
	// textures: パーツのテクスチャを借りる倉庫（同じ絵は1枚にまとまる）。nullptr 不可。所有権は受け取らない。
	// 構築できたら true。<SkinnedMesh>/<Tree> が無い・頂点数が上限超えなら false（理由はデバッグ出力）。
	static bool load( SkinnedMeshData* data, GameLib::PseudoXml::ConstElement container, TextureCache* textures );
};

#endif
