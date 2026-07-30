#ifndef INCLUDED_TEXTURE_H
#define INCLUDED_TEXTURE_H

#include "GameLib/Graphics/Texture.h"

#include <string>
using namespace std;

namespace GameLib{
	namespace PseudoXml{
		class Element;
	}
}

// テクスチャ(GameLib::Graphics::Texture)をラップし、
// 名前の管理とデバイスへのセット処理を行うクラス
class Texture{
public:
	Texture( GameLib::PseudoXml::Element& );	// XMLのElementからテクスチャファイル情報を読み込んで構築する
	Texture( const char* filename );		// ファイル名を指定して直接構築する
	~Texture();
	void set() const;				// このテクスチャを描画デバイスにセットする
	const string* name() const;			// このTextureの名前を取得する
private:
	GameLib::Graphics::Texture mTexture;
	string mName;
};

#endif
