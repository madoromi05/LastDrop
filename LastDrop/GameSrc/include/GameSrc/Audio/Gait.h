#ifndef INCLUDED_GAIT_H
#define INCLUDED_GAIT_H

// 歩き方。キャラ（PlayerCharacter）が今どう動いているかを、音など見た目以外の係に伝えるための言葉。
// アニメの状態（Animator::State）は「素手か銃か」も含むが、足音には関係ないので、ここでは移動の速さの段階だけを持つ。
// しゃがみ歩き・ゆっくり歩きを足すときは、ここに段階を足し、足音の音量の表（FootstepEmitter.cpp）にも足す。
enum Gait{
	GAIT_STILL,	// 止まっている（足音は鳴らない）
	GAIT_WALK,	// 歩き
	GAIT_RUN,	// 走り
	GAIT_NUM,
};

#endif
