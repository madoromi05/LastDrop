#include "StaticMeshPart.h"
#include "VertexColor.h"

#include "GameLib/Graphics/Manager.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const int kVerticesPerTriangle = 3;
}

StaticMeshPart::StaticMeshPart() :
	mTriangleNumber( 0 ),
	mKeepsSource( false ) {
}

StaticMeshPart::~StaticMeshPart() {
}

void StaticMeshPart::create( int vertexNumber, int indexNumber ) {
	mStagingVertices.setSize( vertexNumber );
	mStagingIndices.setSize( indexNumber );
	for ( int i = 0; i < vertexNumber; ++i ) {
		mStagingVertices[ i ].mColor = VertexColor::WHITE;
	}
	mVertexBuffer = Graphics::VertexBuffer::create( vertexNumber );
	mIndexBuffer = Graphics::IndexBuffer::create( indexNumber );
	mTriangleNumber = 0;
}

void StaticMeshPart::setVertex( int i, const Vector3& position, const Vector3& normal, const Vector2& uv ) {
	Graphics::Vertex& v = mStagingVertices[ i ];
	v.mPosition.set( position.x, position.y, position.z, 1.f );
	v.mNormal = normal;
	v.mUv = uv;
}

void StaticMeshPart::setColor( int i, float r, float g, float b ) {
	mStagingVertices[ i ].mColor = VertexColor::pack( r, g, b );
}

void StaticMeshPart::setIndex( int i, unsigned short value ) {
	mStagingIndices[ i ] = value;
}

void StaticMeshPart::setColor( int i, unsigned packedColor ) {
	mStagingVertices[ i ].mColor = packedColor;
}

MeshMaterial& StaticMeshPart::material() {
	return mMaterial;
}

const MeshMaterial& StaticMeshPart::material() const {
	return mMaterial;
}

void StaticMeshPart::keepSourceVertices() {
	ASSERT( !isReady() && "StaticMeshPart : keepSourceVertices must be called before upload." );
	mKeepsSource = true;
}

void StaticMeshPart::upload() {
	Graphics::Vertex* vertices = mVertexBuffer.lock();
	for ( int i = 0; i < mStagingVertices.size(); ++i ) {
		vertices[ i ] = mStagingVertices[ i ];
	}
	mVertexBuffer.unlock( &vertices );

	unsigned short* indices = mIndexBuffer.lock();
	for ( int i = 0; i < mStagingIndices.size(); ++i ) {
		indices[ i ] = mStagingIndices[ i ];
	}
	mIndexBuffer.unlock( &indices );

	// 弾の当たり判定用に、位置とつながりだけ写して残す（UV・法線・色は要らないので捨てる）。
	std::vector< Vector3 > positions( mStagingVertices.size() );
	for ( int i = 0; i < mStagingVertices.size(); ++i ) {
		const Vector4& p = mStagingVertices[ i ].mPosition;
		positions[ i ].set( p.x, p.y, p.z );
	}
	std::vector< unsigned short > triangleIndices( mStagingIndices.size() );
	for ( int i = 0; i < mStagingIndices.size(); ++i ) {
		triangleIndices[ i ] = mStagingIndices[ i ];
	}
	mShape.set( positions, triangleIndices );

	mTriangleNumber = mStagingIndices.size() / kVerticesPerTriangle;
	// GPUへ送ったので作業用配列は不要。建物1つで数百KBあるので持ち続けない（当たり判定の分は mShape に写した）。
	// まとめる元にする部品だけは残す。
	if ( !mKeepsSource ) {
		mStagingVertices.clear();
		mStagingIndices.clear();
	}
}

bool StaticMeshPart::isReady() const {
	return mTriangleNumber > 0;
}

void StaticMeshPart::updateLoading() {
	mMaterial.updateLoading();
}

void StaticMeshPart::draw( const Matrix34& world ) const {
	if ( !isReady() ) {
		return;
	}
	mMaterial.apply(); // テクスチャ・法線マップ・材質値・ライティングモード
	Graphics::Manager m = Graphics::Manager::instance();
	m.setWorldMatrix( world );
	m.setVertexBuffer( mVertexBuffer );
	m.setIndexBuffer( mIndexBuffer );
	m.drawIndexed( 0, mTriangleNumber );
}

bool StaticMeshPart::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return mShape.raycast( origin, delta, time, normal );
}

void StaticMeshPart::appendTriangles( std::vector< Vector3 >* corners ) const {
	mShape.appendTriangles( corners );
}

int StaticMeshPart::sourceVertexNumber() const {
	return mStagingVertices.size();
}

const Graphics::Vertex& StaticMeshPart::sourceVertex( int i ) const {
	return mStagingVertices[ i ];
}

int StaticMeshPart::sourceIndexNumber() const {
	return mStagingIndices.size();
}

unsigned short StaticMeshPart::sourceIndex( int i ) const {
	return mStagingIndices[ i ];
}
