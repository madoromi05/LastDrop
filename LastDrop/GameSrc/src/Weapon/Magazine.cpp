#include "GameSrc/Weapon/Magazine.h"

#include "GameLib/GameLib.h"

namespace{
	// 弾倉に入る弾の数。AKM の標準の弾倉（30 発）に合わせる。
	const int kCapacity = 30;
}

Magazine::Magazine() :
	mRounds( kCapacity ) {
}

int Magazine::rounds() const {
	return mRounds;
}

bool Magazine::isEmpty() const {
	return mRounds <= 0;
}

int Magazine::missingRounds() const {
	return kCapacity - mRounds;
}

void Magazine::takeRound() {
	ASSERT( !isEmpty() && "Magazine::takeRound : empty." );
	--mRounds;
}

void Magazine::load( int count ) {
	ASSERT( count >= 0 && count <= missingRounds() && "Magazine::load : bad count." );
	mRounds += count;
}
