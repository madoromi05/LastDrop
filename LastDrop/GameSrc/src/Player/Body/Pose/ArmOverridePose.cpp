#include "GameSrc/Player/Body/Pose/ArmOverridePose.h"
#include "GameSrc/Player/Body/Pose/FirstPersonRig.h"
#include "GameSrc/Math/MatrixBlend.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

ArmOverridePose::ArmOverridePose( const FirstPersonRig* rig ) :
	mRig( rig ),
	mLayer( nullptr ),
	mWeight( 0.f ),
	mFollowHandWeight( 0.f ),
	mFollowReference( nullptr ) {
	ASSERT( mRig && "ArmOverridePose : rig is null." );
}

void ArmOverridePose::set( const std::vector< Matrix34 >* pose, float weight, float followHandWeight, const Matrix34* followReference ) {
	mLayer = pose;
	mWeight = weight;
	mFollowHandWeight = followHandWeight;
	mFollowReference = followReference;
}

void ArmOverridePose::apply( std::vector< Matrix34 >* pose ) {
	if ( !mLayer || mWeight <= 0.f || mRig->chestBone < 0 ) {
		return;
	}
	const std::vector< Matrix34 >& layer = *mLayer;
	const int chest = mRig->chestBone;
	// 差し替えるポーズの胸 → 今のポーズの胸 へ移す変換。腕はこれを掛けて置く（今の胸に付いた、アニメどおりの腕）
	// 根から見た位置で写さないのは、歩きで揺れる胸から腕が離れて見えないようにするため
	Matrix34 toCurrentChest;
	toCurrentChest.setInverse( layer[ chest ] );
	Matrix34 chestMove;
	chestMove.setMul( ( *pose )[ chest ], toCurrentChest );
	// 今の体のアニメの右手へ寄せる（set の followHandWeight）。差し替える腕を作ったときの体（followReference）の右手から、
	// 今の体のアニメの右手がどれだけ動いたか（位置と向き）を、腕にまとめて掛ける（腕の形は変えない）
	// ライフルの歩き・走りは、止まった構えより手が低く・胸の傾きも違い・揺れるので、拳銃もライフルの右手と同じに動かす
	// 胸に付けて写すだけだと、胸の傾きの違いで拳銃の向きが変わる（歩くと銃口が上を向いた。14-7）
	if ( mFollowHandWeight > 0.f && mFollowReference && mRig->hasWeapon() ) {
		Matrix34 toReference;
		toReference.setInverse( *mFollowReference );
		Matrix34 handMove;
		handMove.setMul( ( *pose )[ mRig->handBone ], toReference );
		Matrix34 followed;
		MatrixBlend::blend( chestMove, handMove, mFollowHandWeight, &followed );
		chestMove = followed;
	}
	for ( int b : mRig->armBones ) {
		Matrix34 target;
		target.setMul( chestMove, layer[ b ] );
		Matrix34 blended;
		MatrixBlend::blend( ( *pose )[ b ], target, mWeight, &blended );
		( *pose )[ b ] = blended;
	}
}
