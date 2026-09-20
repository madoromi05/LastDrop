#ifndef INCLUDED_SKINNED_MESH_LOADER_H
#define INCLUDED_SKINNED_MESH_LOADER_H

#include "GameLib/PseudoXml/Element.h"

class SkinnedMesh;

// ニセxmlの <SkinnedMesh> 要素を読み、SkinnedMesh を構築するローダ。
// SkinnedMesh 本体をファイル形式に依存させないため、読み込み処理はここに分離する。
// 骨格・アニメは Container が別途 <Tree>/<Animation> から読む（この責務の外）。
class SkinnedMeshLoader{
public:
	// container(<Container>要素)の子から <SkinnedMesh> を探して mesh を構築する。
	// 見つかって構築できたら true、無ければ false。
	static bool load( SkinnedMesh* mesh, GameLib::PseudoXml::ConstElement container );
};

#endif
