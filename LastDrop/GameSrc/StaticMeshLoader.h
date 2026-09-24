#ifndef INCLUDED_STATIC_MESH_LOADER_H
#define INCLUDED_STATIC_MESH_LOADER_H

#include "GameLib/PseudoXml/Element.h"

class StaticMeshData;

// ニセxmlの <StaticMesh> 要素を読み、StaticMeshData を構築して GPU へ送るまでを行うローダ。
// StaticMeshData をファイル形式に依存させないため、読み込み処理はここに分離する（SkinnedMeshLoader と同じ構成）。
// <Container> 直下に <StaticMesh> が複数あれば、それぞれを1パーツ（見た目1つ）として読む。
class StaticMeshLoader{
public:
	// container(<Container>要素)の子の <StaticMesh> を全て読んで data を構築し、upload まで済ませる。
	// data: 出力先。nullptr 不可。所有権は受け取らない。
	// 構築できたら true。<StaticMesh> が1つも無い・頂点数が上限超えなら false（理由はデバッグ出力）。
	static bool load( StaticMeshData* data, GameLib::PseudoXml::ConstElement container );
};

#endif
