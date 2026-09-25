#include "PlayerCharacter.h"
#include "SkinnedMeshData.h"
#include "SkinnedMeshInstance.h"
#include "Pad.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 速さは「毎秒」で決め、1フレームぶんに直して使う（update は1フレームに1回）。
	const float kFramesPerSecond = 60.f;
	const float kWalkSpeed = 1.4f / kFramesPerSecond;  // 歩き（m/フレーム）。人の歩く速さ約1.4m/s
	const float kRunSpeed  = 3.2f / kFramesPerSecond;  // 走り（m/フレーム）。Slow Run の足運びに合わせた値
	const float kTurnSpeed = 720.f / kFramesPerSecond; // 振り向きの速さ（度/フレーム）。真後ろへ約0.25秒

	const float kHalfTurn = 180.f;
	const float kFullTurn = 360.f;
	const float kMinInputLength = 0.001f; // これより短い入力（逆方向キーの同時押しなど）は静止とみなす

	// 状態ごとのアニメ名（tools/configs/player.json の anim_name / extra_animations の name）。
	struct StateAnimation{
		Animator::State state;
		const char* name;
	};
	const StateAnimation kStateAnimations[] = {
		{ Animator::STATE_IDLE, "idle" },
		{ Animator::STATE_WALK, "walk" },
		{ Animator::STATE_RUN,  "run"  },
	};

	// 角度を [-180, 180) へ折り返す。振り向きを近い回り方向で行うため。
	float wrapSignedAngle( float degree ) {
		while ( degree >= kHalfTurn ) {
			degree -= kFullTurn;
		}
		while ( degree < -kHalfTurn ) {
			degree += kFullTurn;
		}
		return degree;
	}

	// 押されていれば 1、いなければ 0（逆方向同士を引き算して軸にする）。
	float axisValue( bool on ) {
		return on ? 1.f : 0.f;
	}
}

PlayerCharacter::PlayerCharacter( const SkinnedMeshData* data, const Vector3& position, float angleY ) :
	mMesh( std::make_unique< SkinnedMeshInstance >( data ) ),
	mPosition( position ),
	mAngleY( wrapSignedAngle( angleY ) ) {
	// 状態にアニメを登録する。見つからないアニメは静止のアニメで代用する
	// （レスト姿勢＝Tポーズのまま滑るより目立たない。変換し忘れはデバッグ出力で知らせる）。
	Scene::Animation idle = data->animation( kStateAnimations[ 0 ].name );
	for ( const StateAnimation& entry : kStateAnimations ) {
		Scene::Animation animation = data->animation( entry.name );
		if ( !animation ) {
			GameLib::cout << "PlayerCharacter: animation not found: " << entry.name << GameLib::endl;
			animation = idle;
		}
		mAnimator.setAnimation( entry.state, animation );
	}
	// Animator は静止状態から始まる（request は同じ状態への遷移を無視する）ので、最初のアニメはここで付ける。
	mMesh->setAnimation( mAnimator.animation(), 0.f );
}

PlayerCharacter::~PlayerCharacter() {
}

void PlayerCharacter::update( float cameraYaw ) {
	Vector3 direction;
	bool moving = readMoveDirection( cameraYaw, &direction );
	bool running = moving && InputRun();
	if ( moving ) {
		turnToward( direction );
		// 向きが追いつく前から入力方向へ進める（振り向き待ちで操作が重く感じないように）。
		mPosition.madd( direction, running ? kRunSpeed : kWalkSpeed );
	}
	if ( !moving ) {
		changeState( Animator::STATE_IDLE );
	} else if ( running ) {
		changeState( Animator::STATE_RUN );
	} else {
		changeState( Animator::STATE_WALK );
	}

	mMesh->setPosition( mPosition );
	mMesh->setAngleY( mAngleY );
	mMesh->update();
}

void PlayerCharacter::draw() const {
	mMesh->draw();
}

const Vector3& PlayerCharacter::position() const {
	return mPosition;
}

bool PlayerCharacter::readMoveDirection( float cameraYaw, Vector3* direction ) const {
	float strafe  = axisValue( InputRight() ) - axisValue( InputLeft() ); // 右が正
	float forward = axisValue( InputUp() ) - axisValue( InputDown() );    // 前が正
	// カメラの水平の前方と右（ThirdPersonCamera と同じ約束: 前方はヨー0で +Z、右は 前方×上）。
	Vector3 cameraForward( Math::sin( cameraYaw ), 0.f, Math::cos( cameraYaw ) );
	Vector3 cameraRight( -Math::cos( cameraYaw ), 0.f, Math::sin( cameraYaw ) );
	direction->set( 0.f, 0.f, 0.f );
	direction->madd( cameraForward, forward );
	direction->madd( cameraRight, strafe );
	if ( direction->length() < kMinInputLength ) {
		return false;
	}
	direction->normalize(); // 斜め入力で速くならないように
	return true;
}

void PlayerCharacter::turnToward( const Vector3& direction ) {
	// rotateY(θ) は +Z を (sinθ, 0, cosθ) へ回すので、向けたい角度は atan2(x, z)。
	float targetAngle = Math::atan2( direction.x, direction.z );
	float difference = wrapSignedAngle( targetAngle - mAngleY );
	float step = Math::max( -kTurnSpeed, Math::min( kTurnSpeed, difference ) );
	mAngleY = wrapSignedAngle( mAngleY + step );
}

void PlayerCharacter::changeState( Animator::State next ) {
	if ( mAnimator.request( next ) ) {
		// 切り替えたアニメは頭から再生する（歩き→走りで足の位相を合わせる処理はまだ無い）。
		mMesh->setAnimation( mAnimator.animation(), 0.f );
	}
}
