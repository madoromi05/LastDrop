#ifndef INCLUDED_VERTEX_BUFFER_H
#define INCLUDED_VERTEX_BUFFER_H

#include "GameLib/Graphics/VertexBuffer.h"
#include <string>
using namespace std;

namespace GameLib{
	namespace PseudoXml{
		class Element;
	}
	namespace Math{
		class Vector3;
		class Vector2;
	}
}
using namespace GameLib::Math;

// 頂点バッファ(GameLib::Graphics::VertexBuffer)をラップし、
// 名前や頂点数の管理と、デバイスへのセット処理を行うクラス
class VertexBuffer{
public:
	VertexBuffer( GameLib::PseudoXml::Element& );	// XMLのElementから頂点データを読み込んで構築する
	~VertexBuffer();
	int size() const;				// 頂点数を取得する
	const string* name() const;			// このVertexBufferの名前を取得する
	void set() const;				// この頂点バッファを描画デバイスにセットする
private:
	GameLib::Graphics::VertexBuffer mVertexBuffer;
	int mSize;
	string mName;
};

#endif

