#ifndef INCLUDED_BATCH_H
#define INCLUDED_BATCH_H

#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Vector3.h"
#include <string>
using namespace std;

namespace GameLib{
	namespace PseudoXml{
		class Element;
	}
	namespace Math{
		class Matrix44;
	}
}
using namespace GameLib::Math;

class VertexBuffer;
class IndexBuffer;
class Texture;
class GraphicsDatabase;

// 頂点バッファ・インデックスバッファ・テクスチャと、マテリアル情報(色や質感)を
// まとめて保持し、描画を行う単位となるクラス
class Batch{
public:
	Batch( GameLib::PseudoXml::Element&, const GraphicsDatabase& );	// XMLのElementとGraphicsDatabaseから、参照するバッファ・テクスチャを解決して構築する
	~Batch();
	void draw() const;						// このBatchが持つ頂点・インデックス・テクスチャを使って描画する
	const string* name() const;					// このBatchの名前を取得する
private:
	ConstSharedPtr< VertexBuffer > mVertexBuffer;
	ConstSharedPtr< IndexBuffer > mIndexBuffer;
	ConstSharedPtr< ::Texture > mTexture;

	GameLib::Graphics::BlendMode mBlendMode;
	string mName;
	Vector3 mDiffuseColor;
	Vector3 mSpecularColor;
	float mTransparency;
	float mSpecularSharpness;
};

#endif
