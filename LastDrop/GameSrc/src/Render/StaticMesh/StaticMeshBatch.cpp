#include "GameSrc/Render/StaticMesh/StaticMeshBatch.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"
#include "GameSrc/Render/StaticMesh/StaticMeshInstance.h"
#include "GameSrc/Render/StaticMesh/StaticMeshInstancing.h"

#include "GameLib/GameLib.h"

#include <algorithm>

StaticMeshBatch::StaticMeshBatch() {
}

void StaticMeshBatch::clear() {
	for ( Group& group : mGroups ) {
		group.placements.clear();
	}
	mDrawOrder.clear();
}

void StaticMeshBatch::add( const StaticMeshInstance& instance ) {
	add( instance, instance.data() );
}

void StaticMeshBatch::add( const StaticMeshInstance& instance, const StaticMeshData* shape ) {
	ASSERT( shape && "StaticMeshBatch : shape is null." );
	int index = groupIndexOf( shape );
	Group& group = mGroups[ index ];
	if ( group.placements.empty() ) {
		mDrawOrder.push_back( index ); // このフレームで初めて足された形
	}
	group.placements.push_back( instance.worldMatrix() );
}

int StaticMeshBatch::groupIndexOf( const StaticMeshData* data ) {
	// 形は数種類しかないので、順に探す。
	for ( int i = 0; i < static_cast< int >( mGroups.size() ); ++i ) {
		if ( mGroups[ i ].data == data ) {
			return i;
		}
	}
	mGroups.push_back( Group{ data, {} } );
	return static_cast< int >( mGroups.size() ) - 1;
}

void StaticMeshBatch::draw( StaticMeshInstancing* instancing ) const {
	ASSERT( instancing && "StaticMeshBatch : instancing is null." );
	if ( mDrawOrder.empty() ) {
		return;
	}
	instancing->begin();
	for ( int index : mDrawOrder ) {
		const Group& group = mGroups[ index ];
		int total = static_cast< int >( group.placements.size() );
		for ( int first = 0; first < total; first += StaticMeshInstancing::kMaxInstances ) {
			int number = std::min( total - first, StaticMeshInstancing::kMaxInstances );
			instancing->setPlacements( &group.placements[ first ], number );
			group.data->drawInstanced( number );
		}
	}
	instancing->end();
}
