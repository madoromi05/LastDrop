#include "StaticMeshData.h"

using namespace GameLib::Math;

StaticMeshData::StaticMeshData() {
}

StaticMeshData::~StaticMeshData() {
}

void StaticMeshData::create( int partNumber ) {
	mParts.setSize( partNumber );
}

int StaticMeshData::partNumber() const {
	return mParts.size();
}

StaticMeshPart& StaticMeshData::part( int index ) {
	return mParts[ index ];
}

bool StaticMeshData::isReady() const {
	if ( mParts.size() == 0 ) {
		return false;
	}
	for ( int i = 0; i < mParts.size(); ++i ) {
		if ( !mParts[ i ].isReady() ) {
			return false;
		}
	}
	return true;
}

void StaticMeshData::updateLoading() {
	for ( int i = 0; i < mParts.size(); ++i ) {
		mParts[ i ].updateLoading();
	}
}

void StaticMeshData::draw( const Matrix34& world ) const {
	for ( int i = 0; i < mParts.size(); ++i ) {
		mParts[ i ].draw( world );
	}
}
