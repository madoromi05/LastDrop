#include "GameSrc/UI/ScreenProjection.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Vector4.h"
#include "Framework.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kHalf = 0.5f;
}

bool ScreenProjection::worldToScreen( const Matrix44& projectionView, const Vector3& world, Vector2* out ) {
	ASSERT( out && "ScreenProjection::worldToScreen : out is null." );
	Vector4 clip;
	projectionView.mul( &clip, world );
	if ( clip.w <= 0.f ) {
		return false;
	}
	// -1〜1（上が +1）の正規化した位置を、画素（下が +）へ。
	float ndcX = clip.x / clip.w;
	float ndcY = clip.y / clip.w;
	out->x = ( ndcX * kHalf + kHalf ) * static_cast< float >( Framework::instance().width() );
	out->y = ( kHalf - ndcY * kHalf ) * static_cast< float >( Framework::instance().height() );
	return true;
}
