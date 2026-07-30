#ifndef INCLUDED_GRAPHICS_DATABASE_H
#define INCLUDED_GRAPHICS_DATABASE_H

#include "GameLib/GameLib.h"

namespace GameLib{
	namespace PseudoXml{
		class Element;
	}
}
using GameLib::Array;
using GameLib::SharedPtr;
using GameLib::ConstSharedPtr;

class VertexBuffer;
class IndexBuffer;
class Texture;
class Batch;
class Model;

// 頂点バッファ・インデックスバッファ・テクスチャ・Batchをまとめて保持し、
// 名前を指定して取得したり、Modelを生成したりするための資源管理クラス
class GraphicsDatabase{
public:
	GraphicsDatabase( GameLib::PseudoXml::Element& );				// XMLのElementから各種リソースを生成して構築する
	GraphicsDatabase( const char* filename );					// XMLファイルを読み込んで構築する
	~GraphicsDatabase();

	ConstSharedPtr< VertexBuffer > getVertexBuffer( const char* name ) const;	// 名前を指定してVertexBufferを取得する
	ConstSharedPtr< IndexBuffer > getIndexBuffer( const char* name ) const;		// 名前を指定してIndexBufferを取得する
	ConstSharedPtr< ::Texture > getTexture( const char* name ) const;		// 名前を指定してTextureを取得する
	ConstSharedPtr< Batch > getBatch( const char* name ) const;			// 名前を指定してBatchを取得する

	Model* createModel( const char* batchName ) const;				// 指定したBatch名からModelを生成する(呼び出し側が所有権を持つ)
private:
	void createFromElement( GameLib::PseudoXml::Element& );			// XMLのElementを解析し、各種リソースを生成して登録する

	Array< SharedPtr< VertexBuffer > > mVertexBuffers;
	Array< SharedPtr< IndexBuffer > > mIndexBuffers;
	Array< SharedPtr< ::Texture > > mTextures;
	Array< SharedPtr< Batch > > mBatches;
};

#endif

