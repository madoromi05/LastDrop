#ifndef INCLUDED_PLAYER_FOLEY_H
#define INCLUDED_PLAYER_FOLEY_H

#include "GameSrc/Audio/Gait.h"
#include "GameLib/Math/Vector3.h"

class AudioEngine;

// キャラの装備・動作音（フォーリー）を鳴らす係。
//  ・武器を出す／しまう、覗く／やめる: 状態が切り替わったフレームに1回鳴らす
//  ・装備の揺れ: 一歩ごとに鳴らす（足音と同じ瞬間。FootstepEmitter::update の戻り値を受け取る）
//  ・銃声: 撃ったフレームに1回鳴らす（連射ならその数だけ重なる）
//  ・空撃ち・リロード（弾倉を抜く・挿す・槓桿を引く）: その合図が来たフレームに1回鳴らす
// 装備の揺れは銃を持っている間だけ鳴らす（銃と装備を抱えて動くと音が増えるため。
// タルコフでも銃を持って動く方が相手に聞こえやすい）。服の擦れは足音に被るので一歩ごとには鳴らさず、
// 銃を出す／しまう／覗く／やめるときにだけ銃の音に重ねる。
//
// キャラのクラスは知らない（状態を FoleyState で受け取る）ので、敵や他のプレイヤーにもそのまま使える。
class PlayerFoley{
public:
	// 1フレームぶんのキャラの状態（音に関わるものだけ）。
	struct FoleyState{
		GameLib::Math::Vector3 bodyPosition;	// 音を鳴らす位置（胸のあたり）
		Gait gait;
		bool weaponDrawn;	// 武器を持っているか
		bool aiming;		// 覗いているか
		bool fired;			// このフレームに1発撃ったか
		bool dryFired;		// このフレームに空撃ちしたか
		bool magazineOut;	// このフレームに弾倉を抜いたか（リロード）
		bool magazineIn;	// このフレームに弾倉を挿したか（リロード）
		bool boltPulled;	// このフレームに槓桿を引いたか（リロード）
	};

	// audio: 鳴らす先。所有権は受け取らない。この係より長く生きていること。
	explicit PlayerFoley( AudioEngine* audio );

	// 1フレーム進める。stepped: このフレームに足が着いたか（FootstepEmitter::update の戻り値）。
	void update( const FoleyState& state, bool stepped );

private:
	// 状態の切り替わりで鳴らす音（出し入れ・覗く）。
	void playTransitions( const FoleyState& state );
	// 空撃ち・リロードの音（合図の来たフレームに1回）。
	void playWeaponHandling( const FoleyState& state );
	// 一歩ごとの音（装備）。
	void playStep( const FoleyState& state );

	AudioEngine* mAudio;
	bool mHasPrevious;			// 前のフレームの状態を覚えているか（最初のフレームは切り替わりとみなさない）
	bool mWasWeaponDrawn;
	bool mWasAiming;
};

#endif
