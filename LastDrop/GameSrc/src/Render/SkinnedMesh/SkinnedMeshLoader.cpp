#include "GameSrc/Render/SkinnedMesh/SkinnedMeshLoader.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Render/MeshLoaderUtil.h"

#include "GameLib/PseudoXml/Element.h"
#include "GameLib/PseudoXml/Attribute.h"
#include "GameLib/Base/RefString.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Vector2.h"

using namespace GameLib;
using namespace GameLib::PseudoXml;
using namespace GameLib::Math;

namespace{
	const char* kElementName = "SkinnedMesh";
	const char* kBoneElementName = "Bone";
	const char* kVertexElementName = "Vertex";
	const char* kPartElementName = "Part";
	const char* kIndexElementName = "Index";
	const int kSinglePart = 1;	// <Part> が無い形式は、全体を1パーツとして扱う
	const int kColorFloats = 3;	// 頂点カラーの要素の数（r・g・b）

	// <SkinnedMesh> 直下の要素の数
	struct ElementCounts{
		int bones;
		int vertices;
		int indices;	// <Part> の中の <Index> も数に入れる
		int parts;
	};

	// <Index> 要素を1つ読んで data の indexIndex 番目へ書き、indexIndex を進める
	void readIndex( SkinnedMeshData* mesh, ConstElement indexElement, int* indexIndex ) {
		ConstAttribute valueAttr = MeshLoaderUtil::findAttribute( indexElement, "value" );
		if ( valueAttr ) {
			mesh->setIndex( *indexIndex, static_cast< unsigned short >( valueAttr.getIntValue() ) );	// 頂点数は checkVertexNumber 済みなので 16bit に収まる
		}
		++( *indexIndex );
	}

	// element 直下の <Index> の数
	int countIndices( ConstElement element ) {
		int count = 0;
		for ( int i = 0; i < element.childNumber(); ++i ) {
			if ( RefString( kIndexElementName ) == element.child( i ).name() ) {
				++count;
			}
		}
		return count;
	}

	// 器（SkinnedMeshData::create）の大きさを決めるために、先に数だけ数える
	ElementCounts countElements( ConstElement skin ) {
		ElementCounts counts = { 0, 0, 0, 0 };
		for ( int i = 0; i < skin.childNumber(); ++i ) {
			ConstElement child = skin.child( i );
			RefString name( child.name() );
			if ( name == kBoneElementName ) {
				++counts.bones;
			} else if ( name == kVertexElementName ) {
				++counts.vertices;
			} else if ( name == kIndexElementName ) {
				++counts.indices;
			} else if ( name == kPartElementName ) {
				++counts.parts;
				counts.indices += countIndices( child );
			}
		}
		return counts;
	}

	// <Bone name="..."> を mesh の boneIndex 番目の骨にする
	void readBone( SkinnedMeshData* mesh, ConstElement element, int boneIndex ) {
		ConstAttribute nameAttr = MeshLoaderUtil::findAttribute( element, "name" );
		if ( nameAttr ) {
			mesh->setBone( boneIndex, nameAttr.value() );
		}
	}

	// <Vertex> を mesh の vertexIndex 番目の頂点にする（位置・法線・UV、頂点カラー、影響ボーンと重み）
	void readVertex( SkinnedMeshData* mesh, ConstElement element, int vertexIndex ) {
		Vector3 position;
		Vector3 normal;
		Vector2 uv;
		MeshLoaderUtil::readVertex( element, &position, &normal, &uv );
		mesh->setVertex( vertexIndex, position, normal, uv );

		// 頂点カラー（部位ごとの色。無ければ既定の白のまま）
		ConstAttribute a;
		if ( ( a = MeshLoaderUtil::findAttribute( element, "color" ) ) ) {
			float color[ kColorFloats ] = { 1.f, 1.f, 1.f };
			a.getFloatValues( color, kColorFloats );
			mesh->setColor( vertexIndex, color[ 0 ], color[ 1 ], color[ 2 ] );
		}

		// 影響ボーンと重み（最大 MAX_INFLUENCE 本、少ない分は書かない＝重み0のまま）
		int bones[ SkinnedMeshData::MAX_INFLUENCE ] = { 0 };
		float weights[ SkinnedMeshData::MAX_INFLUENCE ] = { 0.f };
		int boneCount = 0;
		int weightCount = 0;
		if ( ( a = MeshLoaderUtil::findAttribute( element, "bones" ) ) )   { boneCount = a.getIntValues( bones, SkinnedMeshData::MAX_INFLUENCE ); }
		if ( ( a = MeshLoaderUtil::findAttribute( element, "weights" ) ) ) { weightCount = a.getFloatValues( weights, SkinnedMeshData::MAX_INFLUENCE ); }
		int influenceCount = ( boneCount < weightCount ) ? boneCount : weightCount;
		for ( int slot = 0; slot < influenceCount; ++slot ) {
			mesh->setInfluence( vertexIndex, slot, bones[ slot ], weights[ slot ] );
		}
	}

	// <Part texture="..." normalmap="..." ...> を mesh の partIndex 番目のパーツにする（見た目と、中の <Index> が占める範囲）
	// indexIndex: 次に書くインデックスの番号。中の <Index> を読んだ分だけ進める
	void readPart( SkinnedMeshData* mesh, ConstElement element, int partIndex, int* indexIndex, TextureCache* textures ) {
		MeshLoaderUtil::loadMaterial( &mesh->material( partIndex ), element, textures );
		int firstIndex = *indexIndex;
		for ( int j = 0; j < element.childNumber(); ++j ) {
			ConstElement indexElement = element.child( j );
			if ( RefString( kIndexElementName ) == indexElement.name() ) {
				readIndex( mesh, indexElement, indexIndex );
			}
		}
		mesh->setPart( partIndex, firstIndex, *indexIndex - firstIndex );
	}

	// <SkinnedMesh> 直下の要素を出現順に mesh へ流し込む（Bone/Vertex/Index/Part はそれぞれ出現順が添え字になる）
	void readElements( SkinnedMeshData* mesh, ConstElement skin, TextureCache* textures ) {
		int boneIndex = 0;
		int vertexIndex = 0;
		int indexIndex = 0;
		int partIndex = 0;
		for ( int i = 0; i < skin.childNumber(); ++i ) {
			ConstElement child = skin.child( i );
			RefString name( child.name() );
			if ( name == kBoneElementName ) {
				readBone( mesh, child, boneIndex++ );
			} else if ( name == kVertexElementName ) {
				readVertex( mesh, child, vertexIndex++ );
			} else if ( name == kIndexElementName ) {
				readIndex( mesh, child, &indexIndex );
			} else if ( name == kPartElementName ) {
				readPart( mesh, child, partIndex++, &indexIndex, textures );
			}
		}
	}
}

bool SkinnedMeshLoader::load( SkinnedMeshData* mesh, ConstElement container, TextureCache* textures ) {
	ConstElement skin = MeshLoaderUtil::findChild( container, kElementName );
	if ( !skin ) {
		GameLib::cout << "SkinnedMeshLoader: <" << kElementName << "> not found." << GameLib::endl;
		return false;
	}

	const ElementCounts counts = countElements( skin );
	if ( !MeshLoaderUtil::checkVertexNumber( counts.vertices, kElementName ) ) {
		return false;
	}
	// 見た目の持ち方は2通り
	//   <Part> あり: パーツごとに <Part texture=... > の見た目と、その中の <Index> の範囲を持つ
	//   <Part> なし: <SkinnedMesh texture=... > の見た目1つで、直下の <Index> 全部を描く
	bool hasParts = ( counts.parts > 0 );
	mesh->create( counts.vertices, counts.indices, counts.bones, hasParts ? counts.parts : kSinglePart );
	if ( !hasParts ) {
		MeshLoaderUtil::loadMaterial( &mesh->material( 0 ), skin, textures );
		mesh->setPart( 0, 0, counts.indices );
	}
	readElements( mesh, skin, textures );
	// 骨格とアニメは同じ <Container> 要素から GameLib に読ませる
	mesh->setContainer( Scene::Container::create( container ) );
	return mesh->finish();
}
