#include "StaticMeshPart.h"
#include "VertexColor.h"

#include "GameLib/Graphics/Manager.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const int kVerticesPerTriangle = 3;
}

StaticMeshPart::StaticMeshPart() :
	mTriangleNumber( 0 ) {
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

MeshMaterial& StaticMeshPart::material() {
	return mMaterial;
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

	mTriangleNumber = mStagingIndices.size() / kVerticesPerTriangle;
	// GPUへ送ったので作業用配列は不要。建物1つで数百KBあるので持ち続けない。
	mStagingVertices.clear();
	mStagingIndices.clear();
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
