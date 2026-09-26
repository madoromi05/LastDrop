#include "SkinnedMeshData.h"

#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Vertex.h"
#include "GameLib/Math/Vector4.h"
#include "GameLib/Base/DebugStream.h"
#include "VertexColor.h"

namespace{
	const int kVerticesPerTriangle = 3;
	const int kSkeletonTreeIndex = 0; // 変換ツールは <Tree> を1つだけ出力する
}

SkinnedMeshData::Vertex::Vertex() :
	mColor( VertexColor::WHITE ) { // data に color があれば上書き
	// 未使用スロットの重みは0にしておく(deform() の合成で無視されるように)。
	for ( int i = 0; i < MAX_INFLUENCE; ++i ) {
		mBoneIndex[ i ] = 0;
		mWeight[ i ] = 0.f;
	}
}

SkinnedMeshData::Part::Part() :
	mFirstTriangle( 0 ),
	mTriangleNumber( 0 ) {
}

SkinnedMeshData::SkinnedMeshData() :
	mReady( false ) {
}

SkinnedMeshData::~SkinnedMeshData() {
}

void SkinnedMeshData::create( int vertexNumber, int indexNumber, int boneNumber, int partNumber ) {
	ASSERT( partNumber >= 1 && "SkinnedMeshData::create : partNumber must be 1 or more." );
	mVertices.setSize( vertexNumber );
	mBones.setSize( boneNumber );
	mStagingIndices.setSize( indexNumber );
	mIndexBuffer = Graphics::IndexBuffer::create( indexNumber );
	mParts.setSize( partNumber );
	mReady = false;
}

void SkinnedMeshData::setBone( int boneIndex, const char* nodeName ) {
	mBones[ boneIndex ].mNodeName = nodeName;
}

void SkinnedMeshData::setVertex( int i, const Vector3& position, const Vector3& normal, const Vector2& uv ) {
	mVertices[ i ].mPosition = position;
	mVertices[ i ].mNormal = normal;
	mVertices[ i ].mUv = uv;
}

void SkinnedMeshData::setInfluence( int vertexIndex, int slot, int boneIndex, float weight ) {
	mVertices[ vertexIndex ].mBoneIndex[ slot ] = boneIndex;
	mVertices[ vertexIndex ].mWeight[ slot ] = weight;
}

void SkinnedMeshData::setColor( int i, float r, float g, float b ) {
	mVertices[ i ].mColor = VertexColor::pack( r, g, b );
}

void SkinnedMeshData::setIndex( int i, unsigned short value ) {
	mStagingIndices[ i ] = value; // finish でまとめて IB へ送る（1個ずつ lock すると遅く、書き方次第で前の内容が消える）
}

void SkinnedMeshData::setPart( int partIndex, int firstIndex, int indexNumber ) {
	// 描画 API は三角形単位で範囲を受け取るので、ここで換算して持つ。
	mParts[ partIndex ].mFirstTriangle = firstIndex / kVerticesPerTriangle;
	mParts[ partIndex ].mTriangleNumber = indexNumber / kVerticesPerTriangle;
}

MeshMaterial& SkinnedMeshData::material( int partIndex ) {
	return mParts[ partIndex ].mMaterial;
}

void SkinnedMeshData::setContainer( Scene::Container container ) {
	mContainer = container;
}

bool SkinnedMeshData::finish() {
	if ( !mContainer || mContainer.treeNumber() <= kSkeletonTreeIndex ) {
		GameLib::cout << "SkinnedMeshData: <Tree> not found." << GameLib::endl;
		return false;
	}
	// インデックスは変形しないので、GPUへ一度だけ送って作業用配列は捨てる。
	unsigned short* indices = mIndexBuffer.lock();
	for ( int i = 0; i < mStagingIndices.size(); ++i ) {
		indices[ i ] = mStagingIndices[ i ];
	}
	mIndexBuffer.unlock( &indices );
	mStagingIndices.clear();

	// レスト姿勢(アニメ無し)の骨格から、各ボーンの bindInverse を確定する。
	// 一時的な骨格で計算するので、インスタンスの骨格には影響しない。
	Scene::Tree rest = createSkeleton();
	rest.updateWorldMatrix();
	for ( int i = 0; i < mBones.size(); ++i ) {
		const Matrix34* restMatrix = rest.nodeMatrix( mBones[ i ].mNodeName.c_str() );
		if ( restMatrix ) {
			mBones[ i ].mBindInverse.setInverse( *restMatrix );
		} else {
			// 骨名の不一致はエラーにならず「その骨だけ動かない」見た目になるので、ここで知らせる。
			GameLib::cout << "SkinnedMeshData: bone node not found in Tree: " << mBones[ i ].mNodeName.c_str() << GameLib::endl;
			mBones[ i ].mBindInverse.setIdentity();
		}
	}
	mReady = true;
	return true;
}

bool SkinnedMeshData::isReady() const {
	return mReady;
}

int SkinnedMeshData::vertexNumber() const {
	return mVertices.size();
}

int SkinnedMeshData::boneNumber() const {
	return mBones.size();
}

int SkinnedMeshData::findBone( const char* nodeName ) const {
	for ( int b = 0; b < mBones.size(); ++b ) {
		if ( mBones[ b ].mNodeName == nodeName ) {
			return b;
		}
	}
	return -1;
}

const char* SkinnedMeshData::boneName( int boneIndex ) const {
	if ( boneIndex < 0 || boneIndex >= mBones.size() ) {
		return "";
	}
	return mBones[ boneIndex ].mNodeName.c_str();
}

Scene::Tree SkinnedMeshData::createSkeleton() const {
	return mContainer.createTree( kSkeletonTreeIndex );
}

Scene::Animation SkinnedMeshData::animation( const char* name ) const {
	return mContainer.animation( name );
}

void SkinnedMeshData::updateLoading() {
	for ( int i = 0; i < mParts.size(); ++i ) {
		mParts[ i ].mMaterial.updateLoading();
	}
}

void SkinnedMeshData::computeBonePose( Scene::Tree& pose, std::vector< Matrix34 >* bonePose ) const {
	pose.updateWorldMatrix();
	for ( int b = 0; b < mBones.size(); ++b ) {
		const Matrix34* now = pose.nodeMatrix( mBones[ b ].mNodeName.c_str() );
		if ( now ) {
			( *bonePose )[ b ] = *now;
		} else {
			( *bonePose )[ b ].setIdentity();
		}
	}
}

void SkinnedMeshData::deform( const Matrix34& root, const std::vector< Matrix34 >& bonePose,
std::vector< Matrix34 >* skinWork, Graphics::VertexBuffer out ) const {
	std::vector< Matrix34 >& skinMatrices = *skinWork;
	//各ボーンの skin[b] = 置き場所 * ボーンの姿勢 * bindInverse を作る
	for ( int b = 0; b < mBones.size(); ++b ) {
		skinMatrices[ b ].setMul( root, bonePose[ b ] );
		skinMatrices[ b ] *= mBones[ b ].mBindInverse;
	}
	//頂点を重み付きで合成して動的VBへ書く
	Graphics::Vertex* dst = out.lock();
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
			const Matrix34& skin = skinMatrices[ src.mBoneIndex[ slot ] ];
			Vector3 p;
			Vector3 n;
			skin.mul( &p, src.mPosition );		//位置は移動込み
			skin.mul33( &n, src.mNormal );		//法線は移動なし
			position.madd( p, weight );			//position += p * weight
			normal.madd( n, weight );
		}
		dst[ i ].mPosition.set( position.x, position.y, position.z, 1.f );
		dst[ i ].mNormal = normal;
		dst[ i ].mUv = src.mUv;
		dst[ i ].mColor = src.mColor; //部位ごとの頂点カラー
	}
	out.unlock( &dst );
}

void SkinnedMeshData::draw( Graphics::VertexBuffer deformed ) const {
	if ( !mReady ) {
		return;
	}
	Graphics::Manager m = Graphics::Manager::instance();
	Matrix34 world;
	world.setIdentity(); //頂点は deform で既にワールド空間へ変換済み
	m.setWorldMatrix( world );
	m.setVertexBuffer( deformed );
	m.setIndexBuffer( mIndexBuffer );
	// 頂点・インデックスは全パーツ共通。見た目だけ差し替えて、パーツの範囲を描く。
	for ( int i = 0; i < mParts.size(); ++i ) {
		const Part& part = mParts[ i ];
		part.mMaterial.apply(); // テクスチャ・法線マップ・材質値・ライティングモード
		m.drawIndexed( part.mFirstTriangle, part.mTriangleNumber );
	}
}
