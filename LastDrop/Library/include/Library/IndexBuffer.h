#ifndef INCLUDED_INDEX_BUFFER_H
#define INCLUDED_INDEX_BUFFER_H

#include <string>
#include "GameLib/Graphics/IndexBuffer.h"
using namespace std;

namespace GameLib{
	namespace PseudoXml{
		class Element;
	}
}

// インデックスバッファ(GameLib::Graphics::IndexBuffer)をラップし、
// 名前や要素数の管理と、デバイスへのセット処理を行うクラス
class IndexBuffer{
public:
	IndexBuffer( GameLib::PseudoXml::Element& );	// XMLのElementからインデックスデータを読み込んで構築する
	~IndexBuffer();
	int size() const;				// インデックスの要素数を取得する
	const string* name() const;			// このIndexBufferの名前を取得する
	void set() const;				// このインデックスバッファを描画デバイスにセットする
private:
	GameLib::Graphics::IndexBuffer mIndexBuffer;
	int mSize;
	string mName;
};

#endif

