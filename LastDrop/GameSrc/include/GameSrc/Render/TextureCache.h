#ifndef INCLUDED_TEXTURE_CACHE_H
#define INCLUDED_TEXTURE_CACHE_H

#include "GameLib/Graphics/Texture.h"
#include <map>
#include <string>

// テクスチャをファイル名で管理する倉庫番。同じファイルを何度頼まれても読み込みは1回だけで、同じテクスチャを返す。
// 道路の部品（9ファイルが同じ road_albedo.tga を使う）や、建物の2つの形、車の3パーツのように、
// 別々のメッシュが同じ絵を使うとき、絵のデコードと GPU のメモリを1枚ぶんで済ませるため。
//
// Graphics::Texture は参照を数えるハンドルなので、返した写しを誰かが持っている限り絵は消えない。
// この倉庫も写しを1つ持ち続けるので、倉庫がある間は、使う人がいなくなっても読み直しにならない。
class TextureCache{
public:
	TextureCache();
	TextureCache( const TextureCache& ) = delete;
	TextureCache& operator=( const TextureCache& ) = delete;

	// filename の絵を返す（初めてなら読み込みを始める。読み込みは非同期で、進めるのは使う側の MeshMaterial::updateLoading）。
	// filename: nullptr・空文字列は不可。
	// colorSpace: アルベドは SRGB、法線マップは LINEAR（LinearToneMap_Plan.txt 3 章）。
	//   同じファイルを違う指定で頼むと ASSERT で止める（1 枚の絵は 1 つの読み方しかできないため）。
	GameLib::Graphics::Texture load( const char* filename, GameLib::Graphics::TextureColorSpace colorSpace );

private:
	struct Entry{
		GameLib::Graphics::Texture texture;
		GameLib::Graphics::TextureColorSpace colorSpace; // 最初に頼まれたときの読み方
	};
	std::map< std::string, Entry > mTextures;
};

#endif
