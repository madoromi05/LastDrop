#include "GameSrc/Render/MeshLoaderUtil.h"
#include "GameSrc/Render/MeshMaterial.h"
#include "GameSrc/Render/TextureCache.h"

#include "GameLib/Base/RefString.h"
#include "GameLib/Base/DebugStream.h"

using namespace GameLib;
using namespace GameLib::PseudoXml;
using namespace GameLib::Math;

namespace{
	const int kRgb = 3; // 色属性の要素数

	// 値が空文字でない属性か。パス属性の「書いてあるが空」を未設定と同じに扱うため。
	bool hasValue( const ConstAttribute& a ) {
		return a && a.value() && a.value()[ 0 ] != '\0';
	}

	// RGB属性を読む。無ければ何もせず false。
	bool readColor( const ConstElement& e, const char* name, Vector3* out ) {
		ConstAttribute a = MeshLoaderUtil::findAttribute( e, name );
		if ( !a ) {
			return false;
		}
		float rgb[ kRgb ] = { 0.f, 0.f, 0.f };
		a.getFloatValues( rgb, kRgb );
		out->set( rgb[ 0 ], rgb[ 1 ], rgb[ 2 ] );
		return true;
	}
}

ConstAttribute MeshLoaderUtil::findAttribute( const ConstElement& e, const char* name ) {
	// Element には名前引きの属性APIが無いので、名前で線形探索する。
	for ( int i = 0; i < e.attributeNumber(); ++i ) {
		ConstAttribute a = e.attribute( i );
		if ( RefString( name ) == a.name() ) {
			return a;
		}
	}
	return ConstAttribute();
}

ConstElement MeshLoaderUtil::findChild( const ConstElement& e, const char* name ) {
	for ( int i = 0; i < e.childNumber(); ++i ) {
		ConstElement child = e.child( i );
		if ( RefString( name ) == child.name() ) {
			return child;
		}
	}
	return ConstElement();
}

ConstElement MeshLoaderUtil::containerRoot( const ConstElement& documentRoot ) {
	if ( documentRoot.childNumber() == 1 && RefString( "Container" ) != documentRoot.name() ) {
		return documentRoot.child( 0 );
	}
	return documentRoot;
}

void MeshLoaderUtil::loadMaterial( MeshMaterial* material, const ConstElement& meshElement, TextureCache* textures ) {
	ConstAttribute a;
	if ( hasValue( a = findAttribute( meshElement, "texture" ) ) ) {
		material->setTexture( textures->load( a.value(), Graphics::TEXTURE_COLOR_SPACE_SRGB ) ); // アルベドは色の画像
	}
	if ( hasValue( a = findAttribute( meshElement, "normalmap" ) ) ) {
		material->setNormalMap( textures->load( a.value(), Graphics::TEXTURE_COLOR_SPACE_LINEAR ) ); // 法線の向き（色ではない）
	}
	// 材質値は書いてあるものだけ上書きする（無ければ MeshMaterial の既定値）。
	Vector3 color;
	if ( readColor( meshElement, "diffuse", &color ) ) {
		material->setDiffuseColor( color );
	}
	if ( readColor( meshElement, "specular", &color ) ) {
		material->setSpecularColor( color );
	}
	if ( ( a = findAttribute( meshElement, "sharpness" ) ) ) {
		material->setSpecularSharpness( a.getFloatValue() );
	}
	if ( ( a = findAttribute( meshElement, "normalstrength" ) ) ) {
		material->setNormalMapStrength( a.getFloatValue() );
	}
	if ( ( a = findAttribute( meshElement, "doublesided" ) ) ) {
		material->setDoubleSided( a.getIntValue() != 0 );
	}
}

void MeshLoaderUtil::readVertex(
const ConstElement& vertexElement,
Vector3* positionOut,
Vector3* normalOut,
Vector2* uvOut ) {
	float position[ 3 ] = { 0.f, 0.f, 0.f };
	float normal[ 3 ] = { 0.f, 0.f, 1.f };
	float uv[ 2 ] = { 0.f, 0.f };
	ConstAttribute a;
	if ( ( a = findAttribute( vertexElement, "position" ) ) ) { a.getFloatValues( position, 3 ); }
	if ( ( a = findAttribute( vertexElement, "normal" ) ) )   { a.getFloatValues( normal, 3 ); }
	if ( ( a = findAttribute( vertexElement, "uv" ) ) )       { a.getFloatValues( uv, 2 ); }
	// V反転：TGAは上原点／BlenderのVは下原点なので合わせる。
	// どのメッシュ形式もこの1か所を通すので、規則を変えるときはここだけ直せばよい。
	uv[ 1 ] = 1.0f - uv[ 1 ];
	positionOut->set( position[ 0 ], position[ 1 ], position[ 2 ] );
	normalOut->set( normal[ 0 ], normal[ 1 ], normal[ 2 ] );
	uvOut->set( uv[ 0 ], uv[ 1 ] );
}

bool MeshLoaderUtil::checkVertexNumber( int vertexNumber, const char* meshName ) {
	if ( vertexNumber <= MAX_VERTEX_NUMBER ) {
		return true;
	}
	GameLib::cout << meshName << ": too many vertices (" << vertexNumber
		<< " > " << MAX_VERTEX_NUMBER << "). Split the mesh or use 32bit indices." << GameLib::endl;
	return false;
}
