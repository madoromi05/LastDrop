#include "GameSrc/Render/Shadow/CascadeRefresh.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

CascadeRefresh::CascadeRefresh() :
	mDue( true ),
	mDrawn( false ),
	mEye( 0.f, 0.f, 0.f ),
	mView( 0.f, 0.f, 1.f ) {
}

void CascadeRefresh::decide( unsigned frame, int interval, int phase, const Vector3& eye, const Vector3& viewDirection,
	float redrawCos, float redrawDistance, bool force ) {
	ASSERT( interval >= 1 && phase >= 0 && "CascadeRefresh : interval must be positive." );
	bool scheduled = ( ( frame + static_cast< unsigned >( phase ) ) % static_cast< unsigned >( interval ) ) == 0;
	bool turned = mView.dot( viewDirection ) < redrawCos;
	Vector3 moved;
	moved.setSub( eye, mEye );
	bool walked = moved.squareLength() >= redrawDistance * redrawDistance;
	mDue = force || !mDrawn || scheduled || turned || walked;
	if ( mDue ) {
		mDrawn = true;
		mEye = eye;
		mView = viewDirection;
	}
}

bool CascadeRefresh::isDue() const {
	return mDue;
}
