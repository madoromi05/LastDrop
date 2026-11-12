#include "GameSrc/Zombie/Animation/ZombieAnimationSet.h"
#include "GameSrc/Zombie/Animation/ZombieModel.h"
#include "GameSrc/Zombie/Attack/ZombieAttack.h"
#include "GameSrc/Zombie/Animation/ZombieRootMotionSpan.h"
#include "GameSrc/Zombie/Move/ZombieRunParams.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kDeathFadeTicks = 6.f;	// 撃たれた瞬間の姿勢から死亡アニメへ繋ぐ長さ（ティック）。短いほど撃たれた手応えが強いが、0 だと姿勢が飛ぶ
	const float kDeathStartTime = 0.f;	// 死亡アニメは頭から再生する

	const float kStaggerFadeTicks = 6.f;	// 撃たれた瞬間の姿勢からよろけのアニメへ繋ぐ長さ（ティック）。死亡アニメと同じく短くして、手応えを早く出す
	// よろけのアニメ（gethit）を再生し始める時刻（フレーム）。gethit には撃たれた反応が 2 回続けて入っていて
	// （0〜30 フレームで 1 回目、30 フレームで立ち姿に戻り、30〜59 フレームで 2 回目）、頭から流すと
	// 「反応 → 立ち姿 → もう一度よろける」と 2 回に見える。左脚と両腕が大きく動く 2 回目だけを使う
	// 終わりはアニメの最後（59 フレーム）なので、PLAY_ONCE で止まったら歩きへ戻す
	const float kStaggerStartTime = 30.f;
	const float kRecoverFadeTicks = 10.f;	// よろけ終わりから歩きへ戻す長さ（ティック）。よろけの最後と歩きの始めの姿勢は大きく違う（最大 46°）ので、少し長めに混ぜる
	const float kMoveRestartTime = 0.f;		// 歩きは頭から再生し直す

	// 歩く速さ（m/ティック）。ふだんは歩きアニメの足の動きから求めた表（ZombieStride）で進み、これは表が作れなかったときだけ使う
	// 接地している足先が 1 周で左 0.45m・右 0.57m 後ろへ動くのを Blender で測った平均（1 周 1 秒で約 1.0m）
	const float kFallbackWalkSpeed = 1.0f / GameTime::kTicksPerSecond;
	const float kLocomotionFadeTicks = 10.f;	// 歩く ⇔ 立つ のアニメを繋ぐ長さ（ティック）
	const float kAttackFadeTicks = 6.f;			// 立つ・歩くから攻撃のアニメへ繋ぐ長さ（ティック）。始める時刻は ZombieAttack::startTime（段階の時刻と揃える）
	// 前へ倒れるアニメ（death1）が倒れ終わる時刻（フレーム）。ここから後は腰が動かない（Resource/Model/Character/zombie.txt の腰のキーで確かめた）
	// これより短いアニメなら、最後まで来たら倒れ終わったとみなす
	const float kDownedFallEndTime = 160.f;

	// name のアニメを引く。name が nullptr なら空（使わない）。名前が無ければ、静かに使えなくなるので知らせる
	Scene::Animation findAnimation( const SkinnedMeshData* data, const char* name ) {
		if ( !name ) {
			return Scene::Animation();
		}
		Scene::Animation animation = data->animation( name );
		if ( !animation ) {
			GameLib::cout << "Zombie: animation not found: " << name << GameLib::endl;
		}
		return animation;
	}
}

ZombieAnimationSet::ZombieAnimationSet( const ZombieModel* model, const ZombieAnimationNames& names,
SkinnedMeshInstance* mesh, ZombieRootMotionSpan* rootSpan ) :
	mModel( model ),
	mMesh( mesh ),
	mRootSpan( rootSpan ),
	mMove( findAnimation( model->data(), names.move ) ),
	mIdle( findAnimation( model->data(), names.idle ) ),
	mRun( findAnimation( model->data(), names.run ) ),
	mDeath( findAnimation( model->data(), names.death ) ),
	mDowned( findAnimation( model->data(), names.downed ) ),
	mStagger( findAnimation( model->data(), names.stagger ) ),
	mAttack( findAnimation( model->data(), names.attack ) ),
	mAttackRootMotion( model->rootMotion( names.attack ) ),
	mStaggerRootMotion( model->rootMotion( names.stagger ) ),
	mDeathRootMotion( model->rootMotion( names.death ) ),
	mDownedRootMotion( model->rootMotion( names.downed ? names.downed : names.death ) ), // playDowned と同じ選び方
	mMoveStartTime( names.moveStartTime ),
	mGait( Gait::Stand ) {
	ASSERT( mMesh && mRootSpan && "ZombieAnimationSet : mesh or rootSpan is null." );
}

void ZombieAnimationSet::startStanding() {
	mMesh->setAnimation( locomotionAnimation( mGait ), mMoveStartTime );
}

void ZombieAnimationSet::setGait( Gait gait ) {
	if ( gait != mGait ) {
		Scene::Animation before = locomotionAnimation( mGait );
		mGait = gait;
		// 立つアニメが無い（立つ＝歩く）・走るアニメが無い（走る＝歩く）ときは、同じアニメなので繋ぎ直さない
		Scene::Animation after = locomotionAnimation( mGait );
		if ( after != before ) {
			mMesh->crossFadeTo( after, kMoveRestartTime, kLocomotionFadeTicks );
		}
	}
	applyRunRate(); // F6 で倍率を動かしたら、走っている体にもすぐ効く
}

void ZombieAnimationSet::playAttack( Vector3* footing ) {
	mMesh->crossFadeTo( mAttack, ZombieAttack::startTime(), kAttackFadeTicks, SkinnedMeshInstance::PLAY_ONCE );
	mRootSpan->start( mAttackRootMotion, ZombieAttack::startTime(), ZombieRootMotionSpan::Pass::Target, footing );
	mGait = Gait::Stand;	// 攻撃が終わったら、まず立つ（次の update で追う先を考え直し、離れていれば歩き出す）
}

void ZombieAnimationSet::playStagger( Vector3* footing ) {
	mMesh->crossFadeTo( mStagger, kStaggerStartTime, kStaggerFadeTicks, SkinnedMeshInstance::PLAY_ONCE );
	mRootSpan->start( mStaggerRootMotion, kStaggerStartTime, ZombieRootMotionSpan::Pass::None, footing );
}

void ZombieAnimationSet::playDeath( Vector3* footing ) {
	// 撃たれた瞬間に見えていた姿勢から繋ぐ（歩き・よろけのどの瞬間に倒れても、姿勢が飛ばない）
	mMesh->crossFadeTo( mDeath, kDeathStartTime, kDeathFadeTicks, SkinnedMeshInstance::PLAY_ONCE );
	mRootSpan->start( mDeathRootMotion, kDeathStartTime, ZombieRootMotionSpan::Pass::AllBodies, footing );
}

void ZombieAnimationSet::playDowned( Vector3* footing ) {
	// 這うアニメが無いので、前へ倒れるアニメで倒し、最後の姿勢で止めて代わりにする
	Scene::Animation fall = mDowned ? mDowned : mDeath;
	mMesh->crossFadeTo( fall, kDeathStartTime, kDeathFadeTicks, SkinnedMeshInstance::PLAY_ONCE );
	mRootSpan->start( mDownedRootMotion, kDeathStartTime, ZombieRootMotionSpan::Pass::AllBodies, footing );
}

void ZombieAnimationSet::returnToLocomotion( Vector3* footing ) {
	// 攻撃・よろけの最後（元の立ち姿）から戻す。腰は始めた所へ戻っているので、置き場所も絵の根へ戻す
	mMesh->crossFadeTo( locomotionAnimation( mGait ), kMoveRestartTime, kRecoverFadeTicks );
	applyRunRate(); // よろけの前に走っていたら、また走る速さで流す
	mRootSpan->end( footing );
}

float ZombieAnimationSet::walkDistanceThisTick() const {
	if ( mGait == Gait::Run && canRun() ) {
		return mModel->runStride().distanceAt( mMesh->animationPhase() * mRun.period() ) * zombieRunParams().runPlaybackRate;	// 走りのアニメを倍率の速さで流すので、1 ティックに進むフレームも倍率の数だけ。表の 1 フレームぶんに倍率を掛ける
	}
	const ZombieStride& stride = mModel->stride();
	if ( !stride.isValid() ) {
		return kFallbackWalkSpeed;
	}
	// 今見えている歩きのフレームから、次の update で見せるフレームまでに、地面についた足が後ろへ動く長さだけ進む
	// （足が地面を滑らない。歩き方がびっこを引いているので、速さは1歩ごとに波打つ）
	return stride.distanceAt( mMesh->animationPhase() * mMove.period() );
}

bool ZombieAnimationSet::hasFinishedDownedFall() const {
	return mMesh->isAnimationFinished() || mMesh->animationTime() >= kDownedFallEndTime;
}

bool ZombieAnimationSet::canWalk() const {
	return static_cast< bool >( mMove );
}

bool ZombieAnimationSet::canRun() const {
	return static_cast< bool >( mRun ) && mModel->runStride().isValid();
}

bool ZombieAnimationSet::hasDeath() const {
	return static_cast< bool >( mDeath );
}

bool ZombieAnimationSet::hasDowned() const {
	return static_cast< bool >( mDowned );
}

bool ZombieAnimationSet::hasStagger() const {
	return static_cast< bool >( mStagger );
}

bool ZombieAnimationSet::hasAttack() const {
	return static_cast< bool >( mAttack );
}

Scene::Animation ZombieAnimationSet::locomotionAnimation( Gait gait ) const {
	switch ( gait ) {
		case Gait::Run:   return canRun() ? mRun : mMove;
		case Gait::Walk:  return mMove;
		case Gait::Stand: break;
	}
	return mIdle ? mIdle : mMove;
}

void ZombieAnimationSet::applyRunRate() {
	if ( mGait == Gait::Run && canRun() ) {
		mMesh->setPlaybackRate( zombieRunParams().runPlaybackRate );
	}
}
