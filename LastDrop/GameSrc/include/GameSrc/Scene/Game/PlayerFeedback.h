#ifndef INCLUDED_PLAYER_FEEDBACK_H
#define INCLUDED_PLAYER_FEEDBACK_H

#include "GameSrc/Player/PlayerBodySounds.h"
#include "GameSrc/Scene/Game/Sound/CombatSounds.h"
#include "GameSrc/Scene/Game/Hud/PlayerDamageView.h"

class AudioEngine;
class TownEnvironment;
class GameWorld;
class FirstPersonControl;
class HudController;

// このティックに世界で起きた事を、自キャラの視点・音・画面へ返す係（SummaryPlan/個別仕様/設計と分割/HighPrioritySplit_Plan.txt 7-1）
// 世界（Model）は音・カメラ・画面を知らないので、シーンの側でこれが読んで返す：
//   撃った手応え … 当たった音（CombatSounds）・部位が飛んだ/頭が弾けたら画面を少し揺らす
//   攻撃された   … つかんだゾンビの顔を見る・揺れ・怯み・攻撃の音・画面の赤み/痛み/死んだ後の暗転（PlayerDamageView）
//   自キャラの今 … 撃った反動と照準のぶれをカメラと体へ・右下の弾数を出すか・足音と装備の音（PlayerBodySounds）
// どれも 1 ティックに 1 回、GameWorld::update の後に GameScene が呼ぶ（呼ぶ順はシーンが決める）
class PlayerFeedback{
public:
	// audio: 音を鳴らす。town: 足元の材質（足音）を読む。どちらも所有権は受け取らない。nullptr 不可。これより長く生きていること
	PlayerFeedback( AudioEngine* audio, const TownEnvironment* town );
	PlayerFeedback( const PlayerFeedback& ) = delete;
	PlayerFeedback& operator=( const PlayerFeedback& ) = delete;

	void reactToCombat( const GameWorld& world, FirstPersonControl* control );					// 撃った手応えと、攻撃された反応を返す（GameWorld::update の直後）。視点を顔へ向けるのもここ（反動を渡す syncView より前）
	void syncView( const GameWorld& world, FirstPersonControl* control, HudController* hud );	// 自キャラの今の状態を、カメラ・体・HUD・音へ渡す（このティックの最後。反動・揺れ・ぶれは次のティックの絵から効く）

	const PlayerDamageView& damageView() const;	// 攻撃された・死んだ後の見せ方の量（描くときの赤み・暗さ・目の下がり、死んで暗くなり終わったか）

private:
	void shakeOnHeavyHits( const GameWorld& world, FirstPersonControl* control ) const;	// このティックの命中のうち、部位が飛んだ・頭が弾けた物だけ画面を少し揺らす（ふつうと倒しただけは揺らさない）
	void reactToAttacks( const GameWorld& world, FirstPersonControl* control );			// ゾンビがつかんだ・噛んだ出来事を読み、顔を向け、音を鳴らし、怯ませ、攻撃された・死んだ後の見せ方を進める

	PlayerBodySounds mBodySounds;
	CombatSounds mCombatSounds; // 戦闘の結果の効果音（ゾンビ命中・物命中・攻撃された）
	PlayerDamageView mDamageView;
};

#endif
