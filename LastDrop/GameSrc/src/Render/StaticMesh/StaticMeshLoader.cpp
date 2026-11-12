#include "GameSrc/Render/StaticMesh/StaticMeshLoader.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"
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
	const char* kElementName = "StaticMesh";

	// <StaticMesh> 要素1つ（=パーツ1つ）を読み、GPUへ送るまでを行う
	// 読んだ頂点で data の囲む箱を広げる（パーツは GPU へ送った後に頂点を捨てるので、読むときに測る）
	bool loadPart( StaticMeshPart* part, StaticMeshData* data, const ConstElement& mesh, TextureCache* textures ) {
		// テクスチャ・法線マップ・材質値（<StaticMesh texture="..." normalmap="..." ...>）
		MeshLoaderUtil::loadMaterial( &part->material(), mesh, textures );

		// まず Vertex/Index の数を数える
		int vertexNumber = 0;
		int indexNumber = 0;
		for ( int i = 0; i < mesh.childNumber(); ++i ) {
			RefString name( mesh.child( i ).name() );
			if ( name == "Vertex" ) {
				++vertexNumber;
			} else if ( name == "Index" ) {
				++indexNumber;
			}
		}
		if ( !MeshLoaderUtil::checkVertexNumber( vertexNumber, kElementName ) ) {
			return false;
		}
		part->create( vertexNumber, indexNumber );

		// 各要素を出現順に流し込む（Vertex/Index はそれぞれ出現順が添え字になる）
		int vertexIndex = 0;
		int indexIndex = 0;
		for ( int i = 0; i < mesh.childNumber(); ++i ) {
			ConstElement child = mesh.child( i );
			RefString name( child.name() );
			if ( name == "Vertex" ) {
				Vector3 position;
				Vector3 normal;
				Vector2 uv;
				MeshLoaderUtil::readVertex( child, &position, &normal, &uv );
				part->setVertex( vertexIndex, position, normal, uv );
				data->extendBounds( position );

				// 頂点カラー（テクスチャ無しのパーツはマテリアル色で塗る。無ければ白）
				ConstAttribute a = MeshLoaderUtil::findAttribute( child, "color" );
				if ( a ) {
					float color[ 3 ] = { 1.f, 1.f, 1.f };
					a.getFloatValues( color, 3 );
					part->setColor( vertexIndex, color[ 0 ], color[ 1 ], color[ 2 ] );
				}
				++vertexIndex;
			} else if ( name == "Index" ) {
				ConstAttribute valueAttr = MeshLoaderUtil::findAttribute( child, "value" );
				if ( valueAttr ) {
					// 頂点数は checkVertexNumber 済みなので 16bit に収まる
					part->setIndex( indexIndex, static_cast< unsigned short >( valueAttr.getIntValue() ) );
				}
				++indexIndex;
			}
		}
		part->upload();
		return true;
	}
}

bool StaticMeshLoader::load( StaticMeshData* data, ConstElement container, TextureCache* textures ) {
	// <Container> 直下の <StaticMesh> がそれぞれパーツになる。まず数える
	int partNumber = 0;
	for ( int i = 0; i < container.childNumber(); ++i ) {
		if ( RefString( kElementName ) == container.child( i ).name() ) {
			++partNumber;
		}
	}
	if ( partNumber == 0 ) {
		GameLib::cout << "StaticMeshLoader: <" << kElementName << "> not found." << GameLib::endl;
		return false;
	}
	data->create( partNumber );

	int partIndex = 0;
	for ( int i = 0; i < container.childNumber(); ++i ) {
		ConstElement child = container.child( i );
		if ( RefString( kElementName ) != child.name() ) {
			continue;
		}
		if ( !loadPart( &data->part( partIndex ), data, child, textures ) ) {
			return false;
		}
		++partIndex;
	}
	return true;
}
