#include "GameSrc/Map/SpawnPicker.h"

#include "GameLib/GameLib.h"
#include <random>
#include <utility>

std::vector< int > SpawnPicker::pick( int candidateCount, int pickCount, unsigned seed ) {
	ASSERT( pickCount >= 0 && pickCount <= candidateCount && "SpawnPicker : bad pickCount." );
	std::vector< int > order( candidateCount );
	for ( int i = 0; i < candidateCount; ++i ) {
		order[ i ] = i;
	}
	std::mt19937 random( seed );
	for ( int i = 0; i < pickCount; ++i ) {
		// まだ選んでいない残り（i 番目以降）から1つ選び、i 番目と入れ替える
		std::uniform_int_distribution< int > rest( i, candidateCount - 1 );
		std::swap( order[ i ], order[ rest( random ) ] );
	}
	order.resize( pickCount );
	return order;
}
