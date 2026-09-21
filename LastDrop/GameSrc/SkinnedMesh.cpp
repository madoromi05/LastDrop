#include "SkinnedMesh.h"

#include "GameLib/Scene/Tree.h"
#include "GameLib/Scene/Animation.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Graphics/Vertex.h"
#include "GameLib/Math/Vector4.h"

SkinnedMesh::Vertex::Vertex() :
	mColor( 0xff59996bu ) { // 色未設定時のフォールバック（灰緑, BGR並び）。data に color があれば上書き
	// 未使用スロットの重みは0にしておく(update() の合成で無視されるように)。
	for ( int i = 0; i < MAX_INFLUENCE; ++i ) {
		mBoneIndex[ i ] = 0;
		mWeight[ i ] = 0.f;
	}
}

SkinnedMesh::SkinnedMesh() :
	mIndicesUploaded( false ) {
}

SkinnedMesh::~SkinnedMesh() {
}

void SkinnedMesh::create( int vertexNumber, int indexNumber, int boneNumber ) {
	mVertices.setSize( vertexNumber );
	mBones.setSize( boneNumber );
	mSkinMatrices.setSize( boneNumber );
	mIndices.setSize( indexNumber );
	mVertexBuffer = Graphics::VertexBuffer::create( vertexNumber );
	mIndexBuffer = Graphics::IndexBuffer::create( indexNumber );
	mIndicesUploaded = false;
}

void SkinnedMesh::setBone( int boneIndex, const char* nodeName ) {
	mBones[ boneIndex ].mNodeName = nodeName;
}

void SkinnedMesh::setVertex( int i, const Vector3& position, const Vector3& normal, const Vector2& uv ) {
	mVertices[ i ].mPosition = position;
	mVertices[ i ].mNormal = normal;
	mVertices[ i ].mUv = uv;
}

void SkinnedMesh::setInfluence( int vertexIndex, int slot, int boneIndex, float weight ) {
	mVertices[ vertexIndex ].mBoneIndex[ slot ] = boneIndex;
	mVertices[ vertexIndex ].mWeight[ slot ] = weight;
}

void SkinnedMesh::setColor( int i, float r, float g, float b ) {
	// 0..1 を 0..255 にして 0xAARRGGBB へ詰める（シェーダ側の .zyxw で RGB に戻る）。
	unsigned rr = static_cast< unsigned >( ( r < 0.f ? 0.f : ( r > 1.f ? 1.f : r ) ) * 255.f );
	unsigned gg = static_cast< unsigned >( ( g < 0.f ? 0.f : ( g > 1.f ? 1.f : g ) ) * 255.f );
	unsigned bb = static_cast< unsigned >( ( b < 0.f ? 0.f : ( b > 1.f ? 1.f : b ) ) * 255.f );
	// このエンジンは R と B が入れ替わる並びなので、B<<16 | G<<8 | R で詰める。
	mVertices[ i ].mColor = 0xff000000u | ( bb << 16 ) | ( gg << 8 ) | rr;
}

void SkinnedMesh::setIndex( int i, unsigned short value ) {
	mIndices[ i ] = value;
}

void SkinnedMesh::setTexture( Graphics::Texture texture ) {
	mTexture = texture;
}

void SkinnedMesh::bindPose( Scene::Tree& skeleton ) {
	skeleton.setAnimation( Scene::Animation() ); //レスト姿勢へ戻す
	skeleton.updateWorldMatrix();
	for ( int i = 0; i < mBones.size(); ++i ) {
		const Matrix34* rest = skeleton.nodeMatrix( mBones[ i ].mNodeName.c_str() );
		if ( rest ) {
			mBones[ i ].mBindInverse.setInverse( *rest );
		} else {
			mBones[ i ].mBindInverse.setIdentity();
		}
	}
}

void SkinnedMesh::update( Scene::Tree& skeleton ) {
	uploadIndices();
	if ( mTexture ) {
		mTexture.isReady(); // 非同期テクスチャの読み込みを毎フレーム進める（完了時にデコード＆GPU転送）
	}
	skeleton.updateWorldMatrix();
	//各ボーンの skin[b] = 現在ワールド行列 * bindInverse を作る
	for ( int b = 0; b < mBones.size(); ++b ) {
		const Matrix34* now = skeleton.nodeMatrix( mBones[ b ].mNodeName.c_str() );
		if ( now ) {
			mSkinMatrices[ b ].setMul( *now, mBones[ b ].mBindInverse );
		} else {
			mSkinMatrices[ b ].setIdentity();
		}
	}
	//頂点を重み付きで合成して動的VBへ書く
	Graphics::Vertex* out = mVertexBuffer.lock();
	for ( int i = 0; i < mVertices.size(); ++i ) {
		const Vertex& src = mVertices[ i ];
		Vector3 position;
		Vector3 normal;
		position.set( 0.f, 0.f, 0.f );
		normal.set( 0.f, 0.f, 0.f );
		for ( int slot = 0; slot < MAX_INFLUENCE; ++slot ) {
			float weight = src.mWeight[ slot ];
			if ( weight == 0.f ) {
				continue;
			}
			const Matrix34& skin = mSkinMatrices[ src.mBoneIndex[ slot ] ];
			Vector3 p;
			Vector3 n;
			skin.mul( &p, src.mPosition );		//位置は移動込み
			skin.mul33( &n, src.mNormal );		//法線は移動なし
			position.madd( p, weight );			//position += p * weight
			normal.madd( n, weight );
		}
		out[ i ].mPosition.set( position.x, position.y, position.z, 1.f );
		out[ i ].mNormal = normal;
		out[ i ].mUv = src.mUv;
		out[ i ].mColor = src.mColor; //部位ごとの頂点カラー
	}
	mVertexBuffer.unlock( &out );
}

void SkinnedMesh::draw() const {
	Graphics::Manager m = Graphics::Manager();
	m.setTexture( mTexture );
	// ライティング有り。テクスチャ未設定でも陰影で立体が見えるようにする。
	// テクスチャを設定した場合はディフューズ色に乗算される（白なら素の色）。
	m.setLightingMode( Graphics::LIGHTING_PER_PIXEL );
	m.setDiffuseColor( Vector3( 1.0f, 1.0f, 1.0f ) ); // 暗めアルベドを少し持ち上げる（頂点色は白のまま）
	m.setSpecularColor( Vector3( 0.1f, 0.1f, 0.1f ) );
	m.setEmissionColor( Vector3( 0.f, 0.f, 0.f ) );
	m.setSpecularSharpness( 20.f );
	m.setTransparency( 1.f );
	Matrix34 world;
	world.setIdentity(); //頂点は既にワールド空間へ変換済み
	m.setWorldMatrix( world );
	m.setVertexBuffer( mVertexBuffer );
	m.setIndexBuffer( mIndexBuffer );
	m.drawIndexed( 0, mIndices.size() / 3 );
}

void SkinnedMesh::uploadIndices() {
	if ( mIndicesUploaded ) {
		return;
	}
	unsigned short* dst = mIndexBuffer.lock();
	for ( int i = 0; i < mIndices.size(); ++i ) {
		dst[ i ] = mIndices[ i ];
	}
	mIndexBuffer.unlock( &dst );
	mIndicesUploaded = true;
}
