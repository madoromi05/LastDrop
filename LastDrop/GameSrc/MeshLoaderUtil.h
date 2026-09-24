#ifndef INCLUDED_MESH_LOADER_UTIL_H
#define INCLUDED_MESH_LOADER_UTIL_H

#include "GameLib/PseudoXml/Element.h"
#include "GameLib/PseudoXml/Attribute.h"
#include "GameLib/Math/Vector2.h"
#include "GameLib/Math/Vector3.h"

class MeshMaterial;

// メッシュ系ローダ（SkinnedMeshLoader / StaticMeshLoader …）が共通で使う読み取り処理。
// データ形式の「読み方の決まりごと」（UVのV反転など）はここにだけ書き、修正の入口を1つにする。
class MeshLoaderUtil{
public:
	// 頂点インデックスの上限。インデックスバッファが16bit(unsigned short)のため。
	static const int MAX_VERTEX_NUMBER = 65536;

	// 属性を名前で探す。無ければ空の ConstAttribute（if( a ) で判定できる）。
	static GameLib::PseudoXml::ConstAttribute findAttribute( const GameLib::PseudoXml::ConstElement&, const char* name );
	// 子要素を名前で探す（最初に見つかったもの）。無ければ空の ConstElement。
	static GameLib::PseudoXml::ConstElement findChild( const GameLib::PseudoXml::ConstElement&, const char* name );

	// メッシュ要素の属性から材質を読む。
	//   texture / normalmap : 画像ファイルのパス（空・無しなら設定しない）
	//   diffuse / specular  : RGB（無ければ MeshMaterial の既定値のまま）
	//   sharpness / normalstrength : 数値（同上）
	// material: 出力先。nullptr 不可。所有権は受け取らない。
	static void loadMaterial( MeshMaterial* material, const GameLib::PseudoXml::ConstElement& meshElement );

	// <Vertex> 要素から位置・法線・UVを読む。UVはエンジンの規則（V下向き）へ変換済みで返す。
	// 出力はすべて nullptr 不可。属性が無ければ 原点 / +Z / (0,0) になる。
	static void readVertex(
		const GameLib::PseudoXml::ConstElement& vertexElement,
		GameLib::Math::Vector3* positionOut,
		GameLib::Math::Vector3* normalOut,
		GameLib::Math::Vector2* uvOut );

	// 頂点数がインデックスバッファで扱える範囲か確かめる。超えていればデバッグ出力して false。
	// 超えたまま読むと番号が一周してポリゴンが崩れる（エラーにはならない）ので、読む前に必ず呼ぶ。
	static bool checkVertexNumber( int vertexNumber, const char* meshName );
};

#endif
