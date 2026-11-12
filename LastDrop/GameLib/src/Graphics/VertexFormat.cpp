#include "GameLib/GameLib.h"
#include "GameLib/Graphics/VertexFormat.h"

namespace GameLib{
namespace Graphics{

namespace{
	// 要素1つのバイト数。並びは VertexElement と同じ
	int elementSize( VertexElement e ){
		switch ( e ){
			case VERTEX_POSITION4:     return 16; // float4
			case VERTEX_NORMAL3:       return 12; // float3
			case VERTEX_COLOR:         return 4;  // 8bit×4
			case VERTEX_TEXCOORD2:     return 8;  // float2
			case VERTEX_TANGENT4:      return 16; // float4
			case VERTEX_BONE_INDICES4: return 4;  // 8bit×4
			case VERTEX_BONE_WEIGHTS4: return 16; // float4
		}
		ASSERT( false && "VertexFormat : unknown element." );
		return 0;
	}

	// Graphics::Vertex（Vertex.h）と同じ並び。変えると固定パイプラインの頂点が読めなくなる
	const VertexElement kStandardElements[] = {
		VERTEX_POSITION4,
		VERTEX_NORMAL3,
		VERTEX_COLOR,
		VERTEX_TEXCOORD2,
	};
	const int kStandardElementNumber = sizeof( kStandardElements ) / sizeof( kStandardElements[ 0 ] );
}

VertexFormat::VertexFormat() : mElementNumber( kStandardElementNumber ){
	// create は中で VertexFormat を作る（＝この関数を呼ぶ）ので、ここでは使わず直接詰める
	for ( int i = 0; i < kStandardElementNumber; ++i ){
		mElements[ i ] = kStandardElements[ i ];
	}
}

VertexFormat VertexFormat::standard(){
	return VertexFormat();
}

VertexFormat VertexFormat::create( const VertexElement* elements, int count ){
	ASSERT( count > 0 && count <= MAX_ELEMENT && "VertexFormat : element count is out of range." );
	VertexFormat r;
	r.mElementNumber = count;
	for ( int i = 0; i < count; ++i ){
		for ( int j = 0; j < i; ++j ){
			ASSERT( elements[ i ] != elements[ j ] && "VertexFormat : same element twice." );
		}
		r.mElements[ i ] = elements[ i ];
	}
	return r;
}

int VertexFormat::elementNumber() const {
	return mElementNumber;
}

VertexElement VertexFormat::element( int index ) const {
	ASSERT( index >= 0 && index < mElementNumber );
	return mElements[ index ];
}

int VertexFormat::offset( int index ) const {
	ASSERT( index >= 0 && index < mElementNumber );
	int r = 0;
	for ( int i = 0; i < index; ++i ){
		r += elementSize( mElements[ i ] );
	}
	return r;
}

int VertexFormat::stride() const {
	int r = 0;
	for ( int i = 0; i < mElementNumber; ++i ){
		r += elementSize( mElements[ i ] );
	}
	return r;
}

bool VertexFormat::operator==( const VertexFormat& o ) const {
	if ( mElementNumber != o.mElementNumber ){
		return false;
	}
	for ( int i = 0; i < mElementNumber; ++i ){
		if ( mElements[ i ] != o.mElements[ i ] ){
			return false;
		}
	}
	return true;
}

bool VertexFormat::operator!=( const VertexFormat& o ) const {
	return !( *this == o );
}

} //namespace Graphics
} //namespace GameLib
