#include "GameSrc/Weapon/Magazine.h"

#include "GameLib/GameLib.h"

Magazine::Magazine( int capacity ) :
	mCapacity( capacity ),
	mRounds( capacity ) {
	ASSERT( capacity > 0 && "Magazine : capacity must be positive." );
}

int Magazine::rounds() const {
	return mRounds;
}

bool Magazine::isEmpty() const {
	return mRounds <= 0;
}

int Magazine::missingRounds() const {
	return mCapacity - mRounds;
}

void Magazine::takeRound() {
	ASSERT( !isEmpty() && "Magazine::takeRound : empty." );
	--mRounds;
}

void Magazine::setRounds( int rounds ) {
	mRounds = ( rounds < 0 ) ? 0 : ( ( rounds > mCapacity ) ? mCapacity : rounds );
}

void Magazine::load( int count ) {
	ASSERT( count >= 0 && count <= missingRounds() && "Magazine::load : bad count." );
	mRounds += count;
}
