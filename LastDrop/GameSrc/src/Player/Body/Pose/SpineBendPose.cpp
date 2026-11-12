#include "GameSrc/Player/Body/Pose/SpineBendPose.h"
#include "GameSrc/Player/Body/Pose/FirstPersonRig.h"
#include "GameSrc/Math/PlacementMatrix.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

SpineBendPose::SpineBendPose( const FirstPersonRig* rig ) :
	mRig( rig ) {
	ASSERT( mRig && "SpineBendPose : rig is null." );
}

void SpineBendPose::apply( std::vector< Matrix34 >* pose ) {
	// 骨格の根の空間ではキャラの前方が +Z、右が -X。X 軸まわりに +θ 回すと前方が下を向くので、
	// ピッチ（正で見下ろす）をそのまま X 軸の回転角に使える
	// 全部同じ軸まわりなので、根の側から順に「今の関節の位置」を中心に回せば、親子で曲げたのと同じになる
	float bend = mRig->pitch / static_cast< float >( mRig->spineDivisions );
	for ( const FirstPersonRig::SpineJoint& joint : mRig->spine ) {
		Vector3 center = PlacementMatrix::translationOf( ( *pose )[ joint.bone ] );
		Matrix34 rotation; // 関節を原点へ → 回す → 関節へ戻す
		rotation.setTranslation( center );
		rotation.rotateX( bend );
		rotation.translate( -center.x, -center.y, -center.z );
		for ( int b : joint.followers ) {
			Matrix34 bent;
			bent.setMul( rotation, ( *pose )[ b ] );
			( *pose )[ b ] = bent;
		}
	}
}
