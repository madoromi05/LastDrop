#include "SkinnedMeshLoader.h"
#include "SkinnedMesh.h"

#include "GameLib/PseudoXml/Element.h"
#include "GameLib/PseudoXml/Attribute.h"
#include "GameLib/Base/RefString.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Vector2.h"

using namespace GameLib;
using namespace GameLib::PseudoXml;
using namespace GameLib::Math;

namespace{
	// Element には名前引きの属性APIが無いので、名前で線形探索する小ヘルパ。
	// 見つからなければ空の ConstAttribute（if( a ) で判定できる）を返す。
	ConstAttribute findAttribute( const ConstElement& e, const char* name ) {
		for ( int i = 0; i < e.attributeNumber(); ++i ) {
			ConstAttribute a = e.attribute( i );
			if ( RefString( name ) == a.name() ) {
				return a;
			}
		}
		return ConstAttribute();
	}
}

bool SkinnedMeshLoader::load( SkinnedMesh* mesh, ConstElement container ) {
	// <Container> の子から <SkinnedMesh> を探す。
	ConstElement skin;
	for ( int i = 0; i < container.childNumber(); ++i ) {
		ConstElement child = container.child( i );
		if ( RefString( "SkinnedMesh" ) == child.name() ) {
			skin = child;
			break;
		}
	}
	if ( !skin ) {
		return false; // <SkinnedMesh> が無い
	}

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
	mesh->create( vertexNumber, indexNumber, boneNumber );

	// 各要素を出現順に流し込む（Bone/Vertex/Index はそれぞれ出現順が添え字になる）。
	int boneIndex = 0;
	int vertexIndex = 0;
	int indexIndex = 0;
	for ( int i = 0; i < skin.childNumber(); ++i ) {
		ConstElement child = skin.child( i );
		RefString name( child.name() );
		if ( name == "Bone" ) {
			ConstAttribute nameAttr = findAttribute( child, "name" );
			if ( nameAttr ) {
				mesh->setBone( boneIndex, nameAttr.value() );
			}
			++boneIndex;
		} else if ( name == "Vertex" ) {
			float position[ 3 ] = { 0.f, 0.f, 0.f };
			float normal[ 3 ] = { 0.f, 0.f, 1.f };
			float uv[ 2 ] = { 0.f, 0.f };
			ConstAttribute a;
			if ( ( a = findAttribute( child, "position" ) ) ) { a.getFloatValues( position, 3 ); }
			if ( ( a = findAttribute( child, "normal" ) ) )   { a.getFloatValues( normal, 3 ); }
			if ( ( a = findAttribute( child, "uv" ) ) )       { a.getFloatValues( uv, 2 ); }
			mesh->setVertex( vertexIndex,
				Vector3( position[ 0 ], position[ 1 ], position[ 2 ] ),
				Vector3( normal[ 0 ], normal[ 1 ], normal[ 2 ] ),
				Vector2( uv[ 0 ], uv[ 1 ] ) );

			// 影響ボーンと重み（最大 MAX_INFLUENCE 本、少ない分は書かない＝重み0のまま）。
			int bones[ SkinnedMesh::MAX_INFLUENCE ] = { 0 };
			float weights[ SkinnedMesh::MAX_INFLUENCE ] = { 0.f };
			int boneCount = 0;
			int weightCount = 0;
			if ( ( a = findAttribute( child, "bones" ) ) )   { boneCount = a.getIntValues( bones, SkinnedMesh::MAX_INFLUENCE ); }
			if ( ( a = findAttribute( child, "weights" ) ) ) { weightCount = a.getFloatValues( weights, SkinnedMesh::MAX_INFLUENCE ); }
			int influenceCount = ( boneCount < weightCount ) ? boneCount : weightCount;
			for ( int slot = 0; slot < influenceCount; ++slot ) {
				mesh->setInfluence( vertexIndex, slot, bones[ slot ], weights[ slot ] );
			}
			++vertexIndex;
		} else if ( name == "Index" ) {
			ConstAttribute valueAttr = findAttribute( child, "value" );
			if ( valueAttr ) {
				mesh->setIndex( indexIndex, static_cast< unsigned short >( valueAttr.getIntValue() ) );
			}
			++indexIndex;
		}
	}
	return true;
}
