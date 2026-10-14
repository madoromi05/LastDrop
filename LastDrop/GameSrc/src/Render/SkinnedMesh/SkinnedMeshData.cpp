#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"

#include "GameLib/Graphics/Manager.h"
#include "GameLib/Base/DebugStream.h"
#include "GameSrc/Render/VertexColor.h"
#include "GameSrc/Render/SkinnedMesh/SkinningPipeline.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const int kVerticesPerTriangle = 3;
	const int kSkeletonTreeIndex = 0; // 変換ツールは <Tree> を1つだけ出力する

	// GPU へ送る頂点1つ。並びは SkinningPipeline::vertexFormat() と同じ（位置・法線・色・UV・骨の番号・重み）。
	struct SkinnedGpuVertex{
		float position[ 4 ];
		float normal[ 3 ];
		unsigned color;
		float uv[ 2 ];
		unsigned char bones[ SkinnedMeshData::MAX_INFLUENCE ]; // 骨の番号は kMaxBones（128）未満なので 8bit に収まる
		float weights[ SkinnedMeshData::MAX_INFLUENCE ];
	};
	const float kRestW = 1.f; // 位置の w
	const float kNoNormalMap = 0.f; // 法線マップの強さ（0 で効かない）
}

SkinnedMeshData::Vertex::Vertex() :
	mColor( VertexColor::WHITE ) { // data に color があれば上書き
	// 未使用スロットの重みは0にしておく(GPU での変形の合成で無視されるように)。
	for ( int i = 0; i < MAX_INFLUENCE; ++i ) {
		mBoneIndex[ i ] = 0;
		mWeight[ i ] = 0.f;
	}
}

SkinnedMeshData::Bone::Bone() :
	mNodeIndex( -1 ) {
}

SkinnedMeshData::Part::Part() :
	mFirstTriangle( 0 ),
	mTriangleNumber( 0 ) {
}

SkinnedMeshData::SkinnedMeshData() :
	mSkinning( nullptr ),
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
	if ( mBones.size() > SkinningPipeline::kMaxBones ) {
		// 骨の行列はシェーダの定数バッファに入る数までしか渡せない。描けないデータは読み込みの時点で止める。
		GameLib::cout << "SkinnedMeshData: too many bones ( " << mBones.size() << " > " << SkinningPipeline::kMaxBones << " )." << GameLib::endl;
		return false;
	}
	uploadGpuVertices();
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
		// ノードは名前で探すと骨の数ぶんの文字列比較になるので、ここで番号にしておく（毎フレームの computeBonePose が番号で読む）。
		mBones[ i ].mNodeIndex = rest.nodeIndex( mBones[ i ].mNodeName.c_str() );
		if ( mBones[ i ].mNodeIndex >= 0 ) {
			mBones[ i ].mBindInverse.setInverse( *rest.nodeMatrix( mBones[ i ].mNodeIndex ) );
		} else {
			// 骨名の不一致はエラーにならず「その骨だけ動かない」見た目になるので、ここで知らせる。
			GameLib::cout << "SkinnedMeshData: bone node not found in Tree: " << mBones[ i ].mNodeName.c_str() << GameLib::endl;
			mBones[ i ].mBindInverse.setIdentity();
		}
	}
	mReady = true;
	return true;
}

void SkinnedMeshData::uploadGpuVertices() {
	ASSERT( sizeof( SkinnedGpuVertex ) == static_cast< size_t >( SkinningPipeline::vertexFormat().stride() ) &&
		"SkinnedMeshData : SkinnedGpuVertex does not match SkinningPipeline::vertexFormat()." );
	// レスト姿勢のまま送る。体ごとのポーズは描くときに骨の行列で渡し、GPU が変形する（頂点は全インスタンスで共有）。
	mGpuVertices = Graphics::VertexBuffer::create( mVertices.size(), SkinningPipeline::vertexFormat() );
	void* locked = mGpuVertices.lockData();
	SkinnedGpuVertex* dst = static_cast< SkinnedGpuVertex* >( locked );
	for ( int i = 0; i < mVertices.size(); ++i ) {
		const Vertex& src = mVertices[ i ];
		SkinnedGpuVertex& v = dst[ i ];
		v.position[ 0 ] = src.mPosition.x;
		v.position[ 1 ] = src.mPosition.y;
		v.position[ 2 ] = src.mPosition.z;
		v.position[ 3 ] = kRestW;
		v.normal[ 0 ] = src.mNormal.x;
		v.normal[ 1 ] = src.mNormal.y;
		v.normal[ 2 ] = src.mNormal.z;
		v.color = src.mColor; // 部位ごとの頂点カラー（固定パイプラインと同じ詰め方）
		v.uv[ 0 ] = src.mUv.x;
		v.uv[ 1 ] = src.mUv.y;
		for ( int slot = 0; slot < MAX_INFLUENCE; ++slot ) {
			v.bones[ slot ] = static_cast< unsigned char >( src.mBoneIndex[ slot ] );
			v.weights[ slot ] = src.mWeight[ slot ]; // 未使用スロットは重み0（どの骨を指していても効かない）
		}
	}
	mGpuVertices.unlockData( &locked );
}

void SkinnedMeshData::setSkinning( SkinningPipeline* skinning ) {
	mSkinning = skinning;
}

bool SkinnedMeshData::isReady() const {
	return mReady;
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

void SkinnedMeshData::collectBoneSpaceVertices( const std::vector< bool >& boneMask, int anchorBone, std::vector< Vector3 >* out ) const {
	collectVertices( boneMask, mBones[ anchorBone ].mBindInverse, out );
}

Vector3 SkinnedMeshData::boneSpaceJointPosition( int anchorBone, int bone ) const {
	// bindInverse の逆＝レスト姿勢での骨の姿勢。その移動成分が関節の位置（モデルの空間）。
	Matrix34 restPose;
	restPose.setInverse( mBones[ bone ].mBindInverse );
	Vector3 joint( restPose.m03, restPose.m13, restPose.m23 );
	Vector3 local;
	mBones[ anchorBone ].mBindInverse.mul( &local, joint );
	return local;
}

bool SkinnedMeshData::computeRestBounds( const std::vector< bool >& boneMask, Vector3* boundsMin, Vector3* boundsMax ) const {
	Matrix34 identity;
	identity.setIdentity();
	std::vector< Vector3 > points;
	collectVertices( boneMask, identity, &points );
	if ( points.empty() ) {
		return false;
	}
	*boundsMin = points[ 0 ];
	*boundsMax = points[ 0 ];
	for ( const Vector3& p : points ) {
		boundsMin->min( p );
		boundsMax->max( p );
	}
	return true;
}

void SkinnedMeshData::collectVertices( const std::vector< bool >& boneMask, const Matrix34& transform, std::vector< Vector3 >* out ) const {
	out->clear();
	for ( int i = 0; i < mVertices.size(); ++i ) {
		const Vertex& v = mVertices[ i ];
		// 一番重みの大きい骨（重みは大きい順とは限らないので全部見る）。
		int strongestSlot = 0;
		for ( int slot = 1; slot < MAX_INFLUENCE; ++slot ) {
			if ( v.mWeight[ slot ] > v.mWeight[ strongestSlot ] ) {
				strongestSlot = slot;
			}
		}
		if ( !boneMask[ v.mBoneIndex[ strongestSlot ] ] ) {
			continue;
		}
		Vector3 local;
		transform.mul( &local, v.mPosition );
		out->push_back( local );
	}
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
		int node = mBones[ b ].mNodeIndex;
		if ( node >= 0 ) {
			( *bonePose )[ b ] = *pose.nodeMatrix( node );
		} else {
			( *bonePose )[ b ].setIdentity();
		}
	}
}

void SkinnedMeshData::computeSkinMatrices( const Matrix34& root, const std::vector< Matrix34 >& bonePose,
std::vector< Matrix34 >* out ) const {
	std::vector< Matrix34 >& skinMatrices = *out;
	skinMatrices.resize( mBones.size() );
	// 各ボーンの skin[b] = 置き場所 * ボーンの姿勢 * bindInverse。頂点（レスト姿勢）にこれを掛けるとワールドの位置になる。
	for ( int b = 0; b < mBones.size(); ++b ) {
		skinMatrices[ b ].setMul( root, bonePose[ b ] );
		skinMatrices[ b ] *= mBones[ b ].mBindInverse;
	}
}

void SkinnedMeshData::draw( const std::vector< Matrix34 >& skinMatrices ) const {
	if ( !mReady || !mSkinning || !mSkinning->isReady() ) {
		return;
	}
	Graphics::Manager m = Graphics::Manager::instance();
	Matrix34 world;
	world.setIdentity(); // 骨の行列に置き場所まで入っているので、ワールド行列は単位行列
	m.setWorldMatrix( world );
	m.setVertexBuffer( mGpuVertices );
	m.setIndexBuffer( mIndexBuffer );
	mSkinning->begin( skinMatrices );
	// 頂点・インデックス・骨の行列は全パーツ共通。見た目だけ差し替えて、パーツの範囲を描く。
	for ( int i = 0; i < mParts.size(); ++i ) {
		const Part& part = mParts[ i ];
		part.mMaterial.apply(); // テクスチャ・法線マップ・材質値（ライトの値は b0 経由でシェーダへ渡る）
		if ( !part.mMaterial.hasNormalMap() ) {
			// シェーダは法線マップを常に読むので、無いパーツは強さ0で効かなくする（元の法線のまま＝固定パイプラインと同じ見た目）。
			m.setNormalMapStrength( kNoNormalMap );
		}
		m.drawIndexed( part.mFirstTriangle, part.mTriangleNumber );
	}
	mSkinning->end();
}
