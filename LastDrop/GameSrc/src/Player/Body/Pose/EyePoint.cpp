#include "GameSrc/Player/Body/Pose/EyePoint.h"
#include "GameSrc/Player/Body/Pose/FirstPersonRig.h"
#include "GameSrc/Player/Body/WeaponPoseParams.h"
#include "GameSrc/Math/PlacementMatrix.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

EyePoint::EyePoint( FirstPersonRig* rig ) :
	mRig( rig ) {
	ASSERT( mRig && "EyePoint : rig is null." );
}

void EyePoint::apply( std::vector< Matrix34 >* pose ) {
	if ( mRig->headBone < 0 ) {
		return; // 足元から一定の高さのまま
	}
	// 目の向きは頭の骨の回転ではなくキャラの前方（+Z）で決める。頭の骨はアニメで揺れるので、
	// 回転まで使うと覗いた銃ごと画面が傾いて酔いやすい。位置の揺れ（歩きの上下）だけを視点に出す
	// 頭の骨から目までの出し方は、背骨と同じくピッチぶん傾ける。傾けないと、見上げたときに目が上半身に対して
	// 下（あご・胸の側）へずれ、腕を下から見上げる形になって、腕が前へ突っ張って見える
	const WeaponPoseParams& params = weaponPoseParams();
	Vector3 eyeFromHead;
	mRig->viewRotation().mul33( &eyeFromHead, Vector3( 0.f, params.eyeAboveHead, params.eyeForward ) );
	mRig->eyePosition.setAdd( PlacementMatrix::translationOf( ( *pose )[ mRig->headBone ] ), eyeFromHead );
}
