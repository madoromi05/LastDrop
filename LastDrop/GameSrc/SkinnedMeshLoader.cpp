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
}

bool SkinnedMeshLoader::load( SkinnedMeshData* mesh, ConstElement container ) {
	ConstElement skin = MeshLoaderUtil::findChild( container, kElementName );
	if ( !skin ) {
		GameLib::cout << "SkinnedMeshLoader: <" << kElementName << "> not found." << GameLib::endl;
		return false;
	}

	// テクスチャ・法線マップ・材質値（<SkinnedMesh texture="..." normalmap="..." ...>）。
	MeshLoaderUtil::loadMaterial( &mesh->material(), skin );

	// まず Bone/Vertex/Index の数を数える。
	int boneNumber = 0;
	int vertexNumber = 0;
	int indexNumber = 0;
	for ( int i = 0; i < skin.childNumber(); ++i ) {
		RefString name( skin.child( i ).name() );
		if ( name == "Bone" ) {
			++boneNumber;
		} else if ( name == "Vertex" ) {
			++vertexNumber;
		} else if ( name == "Index" ) {
			++indexNumber;
		}
	}
	if ( !MeshLoaderUtil::checkVertexNumber( vertexNumber, kElementName ) ) {
		return false;
	}
	mesh->create( vertexNumber, indexNumber, boneNumber );

	// 各要素を出現順に流し込む（Bone/Vertex/Index はそれぞれ出現順が添え字になる）。
	int boneIndex = 0;
	int vertexIndex = 0;
	int indexIndex = 0;
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
		} else if ( name == "Index" ) {
			ConstAttribute valueAttr = MeshLoaderUtil::findAttribute( child, "value" );
			if ( valueAttr ) {
				// 頂点数は checkVertexNumber 済みなので 16bit に収まる。
				mesh->setIndex( indexIndex, static_cast< unsigned short >( valueAttr.getIntValue() ) );
			}
			++indexIndex;
		}
	}
	// 骨格とアニメは同じ <Container> 要素から GameLib に読ませる。
	mesh->setContainer( Scene::Container::create( container ) );
	return mesh->finish();
}
