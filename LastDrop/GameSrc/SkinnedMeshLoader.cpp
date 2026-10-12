#include "SkinnedMeshLoader.h"
#include "SkinnedMeshData.h"
#include "MeshLoaderUtil.h"

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
	const char* kPartElementName = "Part";
	const char* kIndexElementName = "Index";
	const int kSinglePart = 1; // <Part> が無い形式は、全体を1パーツとして扱う

	// <Index> 要素を1つ読んで data の indexIndex 番目へ書き、indexIndex を進める。
	void readIndex( SkinnedMeshData* mesh, ConstElement indexElement, int* indexIndex ) {
		ConstAttribute valueAttr = MeshLoaderUtil::findAttribute( indexElement, "value" );
		if ( valueAttr ) {
			// 頂点数は checkVertexNumber 済みなので 16bit に収まる。
			mesh->setIndex( *indexIndex, static_cast< unsigned short >( valueAttr.getIntValue() ) );
		}
		++( *indexIndex );
	}

	// element 直下の <Index> の数。
	int countIndices( ConstElement element ) {
		int count = 0;
		for ( int i = 0; i < element.childNumber(); ++i ) {
			if ( RefString( kIndexElementName ) == element.child( i ).name() ) {
				++count;
			}
		}
		return count;
	}
}

bool SkinnedMeshLoader::load( SkinnedMeshData* mesh, ConstElement container, TextureCache* textures ) {
	ConstElement skin = MeshLoaderUtil::findChild( container, kElementName );
	if ( !skin ) {
		GameLib::cout << "SkinnedMeshLoader: <" << kElementName << "> not found." << GameLib::endl;
		return false;
	}

	// まず Bone/Vertex/Index/Part の数を数える（<Part> の中の <Index> も数に入れる）。
	int boneNumber = 0;
	int vertexNumber = 0;
	int indexNumber = 0;
	int partNumber = 0;
	for ( int i = 0; i < skin.childNumber(); ++i ) {
		ConstElement child = skin.child( i );
		RefString name( child.name() );
		if ( name == "Bone" ) {
			++boneNumber;
		} else if ( name == "Vertex" ) {
			++vertexNumber;
		} else if ( name == kIndexElementName ) {
			++indexNumber;
		} else if ( name == kPartElementName ) {
			++partNumber;
			indexNumber += countIndices( child );
		}
	}
	if ( !MeshLoaderUtil::checkVertexNumber( vertexNumber, kElementName ) ) {
		return false;
	}
	// 見た目の持ち方は2通り。
	//   <Part> あり: パーツごとに <Part texture=... > の見た目と、その中の <Index> の範囲を持つ。
	//   <Part> なし: <SkinnedMesh texture=... > の見た目1つで、直下の <Index> 全部を描く（従来の形式）。
	bool hasParts = ( partNumber > 0 );
	mesh->create( vertexNumber, indexNumber, boneNumber, hasParts ? partNumber : kSinglePart );
	if ( !hasParts ) {
		MeshLoaderUtil::loadMaterial( &mesh->material( 0 ), skin, textures );
		mesh->setPart( 0, 0, indexNumber );
	}

	// 各要素を出現順に流し込む（Bone/Vertex/Index/Part はそれぞれ出現順が添え字になる）。
	int boneIndex = 0;
	int vertexIndex = 0;
	int indexIndex = 0;
	int partIndex = 0;
	for ( int i = 0; i < skin.childNumber(); ++i ) {
		ConstElement child = skin.child( i );
		RefString name( child.name() );
		if ( name == "Bone" ) {
			ConstAttribute nameAttr = MeshLoaderUtil::findAttribute( child, "name" );
			if ( nameAttr ) {
				mesh->setBone( boneIndex, nameAttr.value() );
			}
			++boneIndex;
		} else if ( name == "Vertex" ) {
			Vector3 position;
			Vector3 normal;
			Vector2 uv;
			MeshLoaderUtil::readVertex( child, &position, &normal, &uv );
			mesh->setVertex( vertexIndex, position, normal, uv );

			// 頂点カラー（部位ごとの色。無ければ既定の白のまま）。
			ConstAttribute a;
			if ( ( a = MeshLoaderUtil::findAttribute( child, "color" ) ) ) {
				float color[ 3 ] = { 1.f, 1.f, 1.f };
				a.getFloatValues( color, 3 );
				mesh->setColor( vertexIndex, color[ 0 ], color[ 1 ], color[ 2 ] );
			}

			// 影響ボーンと重み（最大 MAX_INFLUENCE 本、少ない分は書かない＝重み0のまま）。
			int bones[ SkinnedMeshData::MAX_INFLUENCE ] = { 0 };
			float weights[ SkinnedMeshData::MAX_INFLUENCE ] = { 0.f };
			int boneCount = 0;
			int weightCount = 0;
			if ( ( a = MeshLoaderUtil::findAttribute( child, "bones" ) ) )   { boneCount = a.getIntValues( bones, SkinnedMeshData::MAX_INFLUENCE ); }
			if ( ( a = MeshLoaderUtil::findAttribute( child, "weights" ) ) ) { weightCount = a.getFloatValues( weights, SkinnedMeshData::MAX_INFLUENCE ); }
			int influenceCount = ( boneCount < weightCount ) ? boneCount : weightCount;
			for ( int slot = 0; slot < influenceCount; ++slot ) {
				mesh->setInfluence( vertexIndex, slot, bones[ slot ], weights[ slot ] );
			}
			++vertexIndex;
		} else if ( name == kIndexElementName ) {
			readIndex( mesh, child, &indexIndex );
		} else if ( name == kPartElementName ) {
			// パーツの見た目（<Part texture="..." normalmap="..." ...>）と、中の <Index> が占める範囲。
			MeshLoaderUtil::loadMaterial( &mesh->material( partIndex ), child, textures );
			int firstIndex = indexIndex;
			for ( int j = 0; j < child.childNumber(); ++j ) {
				ConstElement indexElement = child.child( j );
				if ( RefString( kIndexElementName ) == indexElement.name() ) {
					readIndex( mesh, indexElement, &indexIndex );
				}
			}
			mesh->setPart( partIndex, firstIndex, indexIndex - firstIndex );
			++partIndex;
		}
	}
	// 骨格とアニメは同じ <Container> 要素から GameLib に読ませる。
	mesh->setContainer( Scene::Container::create( container ) );
	return mesh->finish();
}
