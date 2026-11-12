#ifndef INCLUDED_FOOTSTEP_EMITTER_H
#define INCLUDED_FOOTSTEP_EMITTER_H

#include "GameSrc/Audio/Gait.h"
#include "GameSrc/Audio/GroundMaterial.h"
#include "GameLib/Math/Vector3.h"

class AudioEngine;

// 足音を鳴らす係。「いつ」「どの音を」「どの大きさで」鳴らすかだけを決める（鳴らすのは AudioEngine）
//
// いつ: 歩き・走りのアニメの位相（1周の何割か）が、足が地面に着く位相をまたいだフレーム
//       アニメに合わせるので、歩き⇔走りを切り替えても足の見た目と音がずれない（PlayerCharacter が位相を引き継ぐため）
// どの音: 足元の床の材質で選ぶ。材質は足が着いたフレームにだけ調べる（地面へ線分を撃つ重い問い合わせなので、毎フレームは引かない）
// どの大きさで: 歩き方で決める（走りは大きく、歩きは小さく）
//
// キャラのクラスは知らない（歩き方・位相・位置を引数で受け取る）ので、敵や他のプレイヤーの足音にもそのまま使える
class FootstepEmitter{
public:
	explicit FootstepEmitter( AudioEngine* audio );	// audio: 鳴らす先。所有権は受け取らない。この係より長く生きていること

	// 1フレーム進める。足が着いた瞬間なら足音を鳴らして true を返す（服や装備の音を一歩に合わせて鳴らすのに使う）
	// gait: 今の歩き方。phase: 歩き・走りのアニメが1周の何割まで進んだか（0〜1）
	// feet: 足元の位置（ワールド座標）。ground: 足元の床の材質を引く表（足が着いたフレームにだけ引く）
	bool update( Gait gait, float phase, const GameLib::Math::Vector3& feet, const GroundMaterialMap& ground );

private:
	AudioEngine* mAudio;
	Gait mPreviousGait;
	float mPreviousPhase;
};

#endif
