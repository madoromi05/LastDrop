#ifndef INCLUDED_GAMELIB_GRAPHICS_VERTEXFORMAT_H
#define INCLUDED_GAMELIB_GRAPHICS_VERTEXFORMAT_H

#include "GameLib/Graphics/Enum.h"

namespace GameLib{
namespace Graphics{

///頂点の形（どの要素がどの順に並ぶか）。Shader を作るときに渡し、頂点バッファの読み方（InputLayout）を決める
///要素はすべてストリーム0に、並べた順に詰めて置く
///ただの値なのでコピーしてよい（GPU の資源は持たない。InputLayout は Shader が作って持つ）
class VertexFormat{
public:
	enum{ MAX_ELEMENT = 8 };

	static VertexFormat standard();											///標準の頂点（Graphics::Vertex。位置・法線・色・UV）。固定パイプラインと同じ形
	static VertexFormat create( const VertexElement* elements, int count );	///elements を count 個並べた形。count は MAX_ELEMENT 以下、同じ要素は1回まで

	int elementNumber() const;
	VertexElement element( int index ) const;
	int offset( int index ) const;	///要素 index が頂点の先頭から何バイト目にあるか
	int stride() const;				///1頂点のバイト数

	VertexFormat(); ///標準の形
	bool operator==( const VertexFormat& ) const;
	bool operator!=( const VertexFormat& ) const;
private:
	VertexElement mElements[ MAX_ELEMENT ];
	int mElementNumber;
};

} //namespace Graphics
} //namespace GameLib

#endif
