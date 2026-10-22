#include "GameSrc/Map/StreetGraph.h"
#include "GameSrc/Map/TownMap.h"
#include "GameSrc/Hit/HitWorld.h"

#include "GameLib/GameLib.h"

#include <algorithm>
#include <limits>

using namespace GameLib::Math;

namespace{
	// まっすぐ通れるかを調べる線の高さ（m。足元から）。歩道の段差（0.15m）には当たらず、車・壁には当たる高さ
	// （ZombieChase のうろつく先を選ぶ線と同じ）。
	const float kProbeHeight = 0.5f;
	// 図に乗る・降りる交差点を、近い順に何個まで調べるか。通りの途中なら両端の 2 個が入る数。
	// 線は重いので全部は調べない。
	const int kPortalCandidates = 4;
	const int kNoNode = -1;
	const float kUnreached = std::numeric_limits< float >::max();

	// a と b の水平の距離（m）。
	float flatDistance( const Vector3& a, const Vector3& b ) {
		Vector3 d;
		d.setSub( b, a );
		d.y = 0.f;
		return d.length();
	}
}

StreetGraph::StreetGraph( const TownMap& map ) {
	for ( const TownPlacement& junction : map.junctions() ) {
		Node node;
		node.position = Vector3( junction.x, junction.y, junction.z );
		mNodes.push_back( node );
	}
	const int nodeNumber = static_cast< int >( mNodes.size() );
	for ( const StreetLink& link : map.streetLinks() ) {
		ASSERT( link.from >= 0 && link.from < nodeNumber && link.to >= 0 && link.to < nodeNumber && "StreetGraph : bad link." );
		float length = flatDistance( mNodes[ link.from ].position, mNodes[ link.to ].position );
		mNodes[ link.from ].links.push_back( { link.to, length } );
		mNodes[ link.to ].links.push_back( { link.from, length } );
	}
}

bool StreetGraph::isWalkable( const Vector3& from, const Vector3& to, const HitWorld& hitWorld ) const {
	Vector3 origin( from.x, from.y + kProbeHeight, from.z );
	Vector3 delta( to.x - from.x, to.y - from.y, to.z - from.z );
	return !hitWorld.isBlocked( origin, delta );
}

void StreetGraph::findPortals( const Vector3& point, const HitWorld& hitWorld, std::vector< Portal >* out ) const {
	out->clear();
	std::vector< Portal > candidates;
	for ( int i = 0; i < static_cast< int >( mNodes.size() ); ++i ) {
		candidates.push_back( { i, flatDistance( point, mNodes[ i ].position ) } );
	}
	int number = std::min( kPortalCandidates, static_cast< int >( candidates.size() ) );
	std::partial_sort( candidates.begin(), candidates.begin() + number, candidates.end(),
		[]( const Portal& a, const Portal& b ) { return a.distance < b.distance; } );
	for ( int i = 0; i < number; ++i ) {
		if ( isWalkable( point, mNodes[ candidates[ i ].node ].position, hitWorld ) ) {
			out->push_back( candidates[ i ] );
		}
	}
}

bool StreetGraph::findPath( const Vector3& start, const Vector3& goal, const HitWorld& hitWorld, std::vector< Vector3 >* out ) const {
	ASSERT( out && "StreetGraph : out is null." );
	out->clear();
	std::vector< Portal > entries;
	std::vector< Portal > exits;
	findPortals( start, hitWorld, &entries );
	if ( entries.empty() ) {
		return false;
	}
	findPortals( goal, hitWorld, &exits );
	if ( exits.empty() ) {
		return false;
	}

	// 交差点の番号のあとに、行き先（goal）を 1 つの点として足して A* で探す。行き先へは降り口の交差点からだけ行ける。
	const int nodeNumber = static_cast< int >( mNodes.size() );
	const int goalIndex = nodeNumber;
	std::vector< float > cost( nodeNumber + 1, kUnreached ); // 出発からの道のり（m）
	std::vector< int > previous( nodeNumber + 1, kNoNode );
	std::vector< bool > closed( nodeNumber + 1, false );
	std::vector< float > exitCost( nodeNumber, kUnreached ); // 降り口から行き先までの距離（降り口でなければ kUnreached）
	for ( const Portal& entry : entries ) {
		cost[ entry.node ] = entry.distance;
	}
	for ( const Portal& exit : exits ) {
		exitCost[ exit.node ] = exit.distance;
	}

	while ( true ) {
		// 開いている点のうち「道のり＋行き先までのまっすぐの距離」が一番小さいもの（点が少ないので順に見る）。
		int current = kNoNode;
		float best = kUnreached;
		for ( int i = 0; i <= nodeNumber; ++i ) {
			if ( closed[ i ] || cost[ i ] == kUnreached ) {
				continue;
			}
			float estimate = cost[ i ] + ( i == goalIndex ? 0.f : flatDistance( mNodes[ i ].position, goal ) );
			if ( estimate < best ) {
				best = estimate;
				current = i;
			}
		}
		if ( current == kNoNode ) {
			return false; // つながっていない
		}
		if ( current == goalIndex ) {
			break;
		}
		closed[ current ] = true;
		for ( const Link& link : mNodes[ current ].links ) {
			float next = cost[ current ] + link.length;
			if ( !closed[ link.to ] && next < cost[ link.to ] ) {
				cost[ link.to ] = next;
				previous[ link.to ] = current;
			}
		}
		if ( exitCost[ current ] != kUnreached ) {
			float next = cost[ current ] + exitCost[ current ];
			if ( next < cost[ goalIndex ] ) {
				cost[ goalIndex ] = next;
				previous[ goalIndex ] = current;
			}
		}
	}

	// 行き先から出発へたどった交差点を、逆に並べる。
	for ( int node = previous[ goalIndex ]; node != kNoNode; node = previous[ node ] ) {
		out->push_back( mNodes[ node ].position );
	}
	std::reverse( out->begin(), out->end() );
	out->push_back( goal );
	return true;
}
