#ifndef INCLUDED_GAMELIB_SCENE_STRINGRENDERER_H
#define INCLUDED_GAMELIB_SCENE_STRINGRENDERER_H

namespace GameLib{

namespace Graphics{
	class Texture;
}
namespace Math{
	class Vector2;
}
using namespace Math;

namespace Scene{

class Font;

///文字列描画クラス。優先度指定あり。自動でソートします
class StringRenderer{
public:
	///最大合計文字数、最大要求発行数を指定。超えるとassert
	static StringRenderer create( int charCapacity, int requestCapacity );
	///ピクセル座標で指定。zだけfloatで[0,1]であることに注意
	void add( 
		int x, 
		int y, 
		const char* string, 
		unsigned color = 0xffffffff, 
		bool wrapAround = false,
		float z = 0.f );
	///ピクセルだがfloatで場所を指定(x=[0,w], y=[0,h], z=[0,1] )
	void add( 
		const Vector2& position, 
		const char* string, 
		unsigned color = 0xffffffff, 
		bool wrapAroung = false, 
		float z = 0.f );
	///溜め込んだ要求を一挙に描画
	void draw();
	///フォントをセット。最低一回はこれをやらないと字は書けない
	void setFont( Font& );
	///文字の絵の画素を何で割って並べるか（初めは 1）。実際の画面の細かさで作ったフォントを、ゲームの座標の大きさで並べるときに使う
	///例：画面が座標の 2 倍の細かさなら、2 倍の高さで作ったフォントと 2 を渡す。次の draw から効く。scale: 0 より大きいこと
	void setGlyphScale( float scale );

	//以下意識しなくていい
	StringRenderer();
	~StringRenderer();
	StringRenderer( const StringRenderer& );
	operator void*() const;
	StringRenderer& operator=( const StringRenderer& );
	void release();
	bool operator==( const StringRenderer& ) const;
	bool operator!=( const StringRenderer& ) const;
private:
	class Impl;
	Impl* mImpl;
};

} //namespace Scene
} //namespace GameLib

#endif
