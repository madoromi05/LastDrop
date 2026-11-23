#include "GameLib/GameLib.h"
#include "GameLib/Scene/StringRenderer.h"
#include "GameLib/Scene/Font.h"
#include "GameLib/Graphics/Texture.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Vector2.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Scene/PrimitiveRenderer.h"
#include "GameLib/Base/Stack.h"
#include "GameLib/Base/Array.h"
#include "GameLib/Base/Impl/ReferenceType.h"
#include "GameLib/Base/Utf8.h"
#include <algorithm>
#include <string>

namespace GameLib{
namespace Scene{

using namespace GameLib::Math;
using namespace GameLib::Graphics;

//ソート用便利クラス
template< class T > struct PointerLess{
	bool operator()( const T* a, const T* b ){
		return ( *a < *b );
	}
};

class StringRenderer::Impl : public ReferenceType{
public:
	Impl( int charCapacity, int requestCapacity ) :
	mCharCapacity( charCapacity ),
	mRequests( requestCapacity ), //リクエスト確保
	mGlyphScale( 1.f ){
		mPrimitiveRenderer = PrimitiveRenderer::create( charCapacity * 2, charCapacity + 4 );	//TODO:ステート変更命令の最大数はとりあえず文字数分。過剰すぎる
		//draw二回呼ぶのに対処するための変数
		mPreviousFrameId = Manager().frameId() - 1;												//過ぎ去った値に
	}
	~Impl(){
	}
	void add( const Vector2& p, const char* s, unsigned c, bool wrap, float depth ){
		STRONG_ASSERT( mCurrentFont && "Font hasn't been set." );
		mRequests.push(); //空プッシュ
		Request& r = *mRequests.get();
		r.mPosition = p;
		r.mDepth = depth;
		r.mString = s;
		r.mColor = c;
		r.mFont = mCurrentFont;
		r.mWrap = wrap;
	}
	void draw(){
		//フレームIDチェック
		unsigned fid = Manager().frameId();
		STRONG_ASSERT( mPreviousFrameId != fid && "StringRenderer::draw() : you can't draw() twice in a frame!" );
		mPreviousFrameId = fid;

		if ( mRequests.size() == 0 ){ //やることない
			return;
		}
		//TriangleRendererに標準ステートを発行
		mPrimitiveRenderer.setBlendMode( Graphics::BLEND_LINEAR );
		mPrimitiveRenderer.setCullMode( Graphics::CULL_NONE );
		mPrimitiveRenderer.enableDepthTest( false );
		mPrimitiveRenderer.enableDepthWrite( false );
		//現在のビューポートを取得してfloat化
		Vector2 screenSize;
		int tw, th;
		Graphics::Manager().getViewport( 0, 0, &tw, &th );
		screenSize.set( 
			static_cast< float >( tw ),
			static_cast< float >( th ) );

		//文字バッファを用意
		Array< Char > chars( mCharCapacity );
		int charPosition = 0;
		//リクエストを取り出して処理
		while ( mRequests.size() > 0 ){
			Request& r = *mRequests.get();
			Vector2 pos = r.mPosition;
			float charHeight = static_cast< float >( r.mFont.charHeight() ) / mGlyphScale;
			//一文字づつ取り出す。文字列は UTF-8（/utf-8 でビルドするため）。code は Unicode のコードポイント
			const char* s = r.mString.c_str();
			int byteCount = 0;
			for ( ; *s != '\0'; s += byteCount ){
				int code = Utf8::decode( s, &byteCount );	//Char準備
				if ( code == '\n' ){						//改行文字ですな
					pos.x = r.mPosition.x;
					pos.y += charHeight;
					continue;
				}
				//文字ゲット
				Char& c = chars[ charPosition ];
				bool found = r.mFont.getChar( 
					&c.mTexture,
					&c.mUv0,
					&c.mUv1,
					code );
				if ( found ){ //文字あった
					c.mSize.setSub( c.mUv1, c.mUv0 );	//現在位置にピクセルを加えていく
					//テクスチャから幅と高さをもらって、ピクセルに直す比を求める
					c.mSize.x *= static_cast< float >( c.mTexture.width() ) / mGlyphScale;
					c.mSize.y *= static_cast< float >( c.mTexture.height() ) / mGlyphScale;
				}else{ //文字なかった。豆腐を出す
					c.mSize.set( charHeight, charHeight ); //正方形
				}
				//カリングと折り返し計算
				//折り返しアリの時はちょっとでもはみ出すなら折り返す
				if ( r.mWrap ){
					if ( pos.x + c.mSize.x > screenSize.x ){
						pos.x = r.mPosition.x;
						pos.y += charHeight;
					}
				}else{ //折り返しなしなら完全にあふれた時に切り捨てる
					if ( pos.x > screenSize.x ){
						break;
					}
				}
				//縦が完全にあふれた場合のみ終了
				if ( pos.y > screenSize.y ){
					break;
				}
				//エントリ追加
				c.mColor = r.mColor;
				c.mPosition.set( pos.x, pos.y, r.mDepth );
				++charPosition;
				pos.x += c.mSize.x;	//文字送り
			}
			mRequests.pop();
		}
		if ( charPosition == 0 ){ //文字がない
			return;
		}
		//処理後文字バッファをZとテクスチャでソート
		Array< Char* > charPointers( charPosition );
		for ( int i = 0; i < charPosition; ++i ){
			charPointers[ i ] = &chars[ i ];
		}
		std::sort( &charPointers[ 0 ], &charPointers[ 0 ] + charPosition, PointerLess< Char >() );
		//ソートされた順に文字バッファをTriangleBufferにadd
		Texture lastTexture;
		for ( int i = 0; i < charPosition; ++i ){
			Char& c = *charPointers[ i ];
			//テクスチャが前と違えばセット
			if ( lastTexture != c.mTexture ){
				mPrimitiveRenderer.setTexture( c.mTexture );
				lastTexture = c.mTexture;
			}
			Vector2 p[ 2 ];														//頂点を作って四角形を描画
			p[ 0 ].set( c.mPosition.x, c.mPosition.y );							//左上点
			p[ 1 ].set( c.mPosition.x + c.mSize.x, c.mPosition.y + c.mSize.y );	//右下
			//描画
			if ( c.mTexture ){													//テクスチャあれば
				mPrimitiveRenderer.addRectangle( 
					p[ 0 ], p[ 1 ],
					c.mUv0, c.mUv1,
					c.mColor );
			}else{ //ねえよ。豆腐出したれ
				mPrimitiveRenderer.addRectangle( 
					p[ 0 ], p[ 1 ],
					c.mColor );
			}
		}
		mPrimitiveRenderer.draw();	//全部終わったのでTriangleRendererを描画
	}
	void setFont( Font& font ){
		mCurrentFont = font;
	}
	void setGlyphScale( float scale ){
		mGlyphScale = scale;
	}
private:
	struct Request{
		Vector2 mPosition;
		float mDepth;
		std::string mString;
		unsigned mColor;
		Font mFont;
		bool mWrap;
	};
	//文字構造体
	struct Char{
		//Zとテクスチャで比較。Zは奥が先
		bool operator<( const Char& a ) const {
			if ( mPosition.z > a.mPosition.z ){
				return true;
			}else if ( mPosition.z < a.mPosition.z ){
				return false;
			}else if ( mTexture < a.mTexture ){
				return true;
			}else{
				return false;
			}
		}
		Vector3 mPosition; //左上点
		Vector2 mUv0; //左上UV
		Vector2 mUv1; //右下UV
		Vector2 mSize;
		Texture mTexture; //テクスチャ
		unsigned mColor;
	};
	int mCharCapacity;
	Stack< Request > mRequests;
	PrimitiveRenderer mPrimitiveRenderer;
	Font mCurrentFont;
	unsigned mPreviousFrameId;
	float mGlyphScale; //文字の絵の画素をこれで割って並べる
};

StringRenderer StringRenderer::create( int charCapacity, int requestCapacity ){
	StringRenderer r;
	r.mImpl = NEW Impl( charCapacity, requestCapacity );
	return r;
}

void StringRenderer::add( int x, int y, const char* s, unsigned c, bool wrap, float z ){
	Vector2 p( 
		static_cast< float >( x ),
		static_cast< float >( y ) );
	mImpl->add( p, s, c, wrap, z );
}

void StringRenderer::add( const Vector2& p, const char* s, unsigned c, bool wrap, float z ){
	mImpl->add( p, s, c, wrap, z );
}

void StringRenderer::draw(){
	mImpl->draw();
}

void StringRenderer::setFont( Font& font ){
	mImpl->setFont( font );
}

void StringRenderer::setGlyphScale( float scale ){
	ASSERT( scale > 0.f && "StringRenderer : glyph scale must be positive." );
	mImpl->setGlyphScale( scale );
}

#define TYPE StringRenderer
#include "GameLib/Base/Impl/ReferenceTypeTemplate.h"

} //namespace Scene
} //namespace GameLib
