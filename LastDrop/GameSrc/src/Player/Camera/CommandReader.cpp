#include "GameSrc/Player/Camera/CommandReader.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/Math/ScalarMath.h"

namespace{
	// 押されていれば 1、いなければ 0（逆方向同士を引き算して軸にする）
	float axisValue( bool on ) {
		return on ? 1.f : 0.f;
	}

	const float kMoveAxisLimit = 1.f;	// 移動の軸の上限（キーとスティックを同時に入れても PlayerCommand の -1〜1 に収める）
}

CommandReader::CommandReader() :
	mCommand(),
	mFireSuppressed( true ) {
}

void CommandReader::read( bool cursorLocked ) {
	// キーとパッドのスティックを足す。スティックの縦は下が正なので、前（上）を正にするため引く
	AxisStatus move = InputMove();
	mCommand.forward = ScalarMath::clampSigned( axisValue( InputUp() ) - axisValue( InputDown() ) - move.m_height, kMoveAxisLimit );
	mCommand.strafe = ScalarMath::clampSigned( axisValue( InputRight() ) - axisValue( InputLeft() ) + move.m_width, kMoveAxisLimit );
	mCommand.run = InputRun();
	mCommand.holsterPressed = mCommand.holsterPressed || InputHolster();
	mCommand.reloadPressed = mCommand.reloadPressed || InputReload();
	mCommand.flashlightPressed = mCommand.flashlightPressed || InputFlashlight();
	mCommand.aimHeld = InputAim();
	bool fireHeld = InputFireHeld();
	if ( !fireHeld ) {
		mFireSuppressed = false;
	}
	mCommand.fireHeld = fireHeld && cursorLocked && !mFireSuppressed;
}

void CommandReader::clear() {
	mCommand.forward = 0.f;
	mCommand.strafe = 0.f;
	mCommand.run = false;
	mCommand.aimHeld = false;
	mCommand.fireHeld = false;
	mFireSuppressed = true;
}

void CommandReader::suppressFire() {
	mFireSuppressed = true;
}

void CommandReader::consumePressed() {
	mCommand.holsterPressed = false;
	mCommand.reloadPressed = false;
	mCommand.flashlightPressed = false;
}

const PlayerCommand& CommandReader::command() const {
	return mCommand;
}

bool CommandReader::isFireSuppressed() const {
	return mFireSuppressed;
}
