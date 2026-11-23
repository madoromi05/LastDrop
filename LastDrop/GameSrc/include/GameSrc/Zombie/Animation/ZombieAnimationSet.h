#ifndef INCLUDED_ZOMBIE_ANIMATION_SET_H
#define INCLUDED_ZOMBIE_ANIMATION_SET_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Scene/Animation.h"

class ZombieModel;
class ZombieRootMotion;
class SkinnedMeshInstance;
class ZombieRootMotionSpan;

// ゾンビが使うアニメの名前（文字列は持ち主の定数。ZombieAnimationSet を作るときに引くだけで、覚えない）
// Zombie::Animations はこれの別名
struct ZombieAnimationNames{
	// 歩くアニメと開始時刻（フレーム。体ごとにずらして動きを揃えない）
	// nullptr ならアニメを付けず、レスト姿勢（T ポーズ）のまま立たせ、追ってもこない（当たり判定の確認用）
	const char* move;
	float moveStartTime;
	const char* idle;		// 立ち止まっているときのアニメ。nullptr なら立ち止まっている間も歩くアニメのまま
	const char* run;		// その場で走るアニメ（zombie@run）。nullptr ならどの体も走らない（走る体に選ばれても歩く）
	// 倒れるときのアニメ（脚を失って倒れるときにも使う）。nullptr なら体力が 0 になっても・脚を失っても倒れない
	// （確認用。体力は減って 0 で止まる）
	const char* death;
	// 脚を失って倒れるときのアニメ（前へ倒れてうつ伏せになる物。倒れ終わったら体を引きずる）。nullptr なら death で倒し、
	// 倒れたまま動かない（仰向けに倒れるアニメでは引きずれないため）
	const char* downed;
	const char* stagger;	// よろけるときのアニメ。nullptr ならよろけない（のけぞりだけ）
	const char* attack;		// 立っている体の攻撃のアニメ（腕振りは atack1・つかみは atack3。ZombieAttackStyle）。nullptr なら攻撃しない（追いついても立って待つだけ）
};

// ゾンビ1体のアニメの切り替え。どのアニメを・どの時刻から・何ティックかけて繋ぎ・どの根の動きの表で区間を始めるか、の組を持つ
// （アニメの繋ぎ目がおかしい・腰が飛ぶ、の修正の入口）
// 「いつ」流すか（撃たれた・追いついた・倒れた）は持ち主（Zombie）が決める。ここは状態を知らない
// メッシュと根の動きの区間は持ち主のメンバを借りる。足元は持ち主が持ったまま、関数ごとにポインタで渡す（ZombieRootMotionSpan と同じ）
class ZombieAnimationSet{
public:
	// 足の運び（立つ・歩く・走る）。どれにするかは持ち主が決める（ここは流すアニメと速さを合わせるだけ）
	enum class Gait{
		Stand,
		Walk,
		Run,
	};

	// model: 形・アニメ・歩きの表・根の動きの表。mesh: アニメを流すメッシュ。rootSpan: 根の動きの区間
	// どれも所有権は受け取らない。nullptr 不可。この部品より長く生きていること
	// （mesh・rootSpan は同じ Zombie のメンバで、この部品より先に宣言する）
	// names: 使うアニメの名前（ここで引くだけで、覚えない）。見つからない名前は知らせて、そのアニメを使わない
	ZombieAnimationSet( const ZombieModel* model, const ZombieAnimationNames& names,
		SkinnedMeshInstance* mesh, ZombieRootMotionSpan* rootSpan );
	ZombieAnimationSet( const ZombieAnimationSet& ) = delete;
	ZombieAnimationSet& operator=( const ZombieAnimationSet& ) = delete;

	void startStanding();										// 作った直後の姿勢にする（立つアニメ。無ければ歩くアニメ。どちらも無ければレスト姿勢＝T ポーズ）
	// 立つ・歩く・走るのアニメを gait に合わせる（変わったときだけ繋ぐ）。走る間は毎回、走りのアニメの倍率
	// （ZombieRunParams::runPlaybackRate。F6 で動かすとすぐ効く）を掛け直す。走るアニメが無ければ歩く
	void setGait( Gait gait );
	// 攻撃・よろけ・倒れるアニメを PLAY_ONCE で流し始め、根の動きの区間を始める。footing: 足元。nullptr 不可
	void playAttack( GameLib::Math::Vector3* footing );			// 相手だけを通り抜ける。終わったら、まず立つ（歩くかは持ち主が考え直す）
	void playStagger( GameLib::Math::Vector3* footing );		// どれにも当たる
	void playDeath( GameLib::Math::Vector3* footing );			// 全員を通り抜ける
	void playDowned( GameLib::Math::Vector3* footing );			// 前へ倒れるアニメ（無ければ死亡アニメ）で。全員を通り抜ける
	void returnToLocomotion( GameLib::Math::Vector3* footing );	// 攻撃・よろけの終わり（と振りほどかれたとき）に、歩く・立つのアニメへ戻し、根の動きの区間を閉じる。footing: 足元。nullptr 不可

	// このティックに歩いて（走って）進む長さ（m）。歩き・走りアニメの足の動きの表（ZombieModel::stride / runStride）から引く
	// 歩き・走りのアニメは倍率の速さで流すので、進む長さにも同じ倍率を掛ける（足が地面を滑らない）
	float walkDistanceThisTick() const;
	bool hasFinishedDownedFall() const;	// 前へ倒れるアニメ（playDowned）が倒れ終わったか（ここから後は腰が動かない）

	// そのアニメがあるか（Names の nullptr・見つからなかった名前なら false）
	bool canWalk() const;       // 歩くアニメがある（追ってくる体。T ポーズの確認用ではない）
	bool canRun() const;        // 走るアニメと、その足の動きの表がある
	bool hasDeath() const;
	bool hasDowned() const;     // 前へ倒れるアニメそのものがある（無ければ playDowned は死亡アニメで倒す）
	bool hasStagger() const;
	bool hasAttack() const;

private:
	GameLib::Scene::Animation locomotionAnimation( Gait gait ) const;	// 足の運び gait に合ったアニメ（立つアニメが無ければ歩くアニメ、走るアニメが無ければ歩くアニメ）
	void applyGaitRate();												// 歩いている・走っているなら、そのアニメの倍率を掛ける（アニメを繋ぐと速さは 1 に戻るので、繋いだ後に呼ぶ）

	const ZombieModel* mModel;           // 借りているだけ
	SkinnedMeshInstance* mMesh;          // 借りているだけ
	ZombieRootMotionSpan* mRootSpan;     // 借りているだけ
	// 使うアニメ（作るときに名前から引いておく）。空なら、そのアニメは使わない（Names の nullptr と同じ意味）
	GameLib::Scene::Animation mMove;
	GameLib::Scene::Animation mIdle;
	GameLib::Scene::Animation mRun;
	GameLib::Scene::Animation mDeath;
	GameLib::Scene::Animation mDowned;
	GameLib::Scene::Animation mStagger;
	GameLib::Scene::Animation mAttack;
	// アニメごとの根の動きの表（ZombieModel が持つ。借りているだけ）。nullptr なら、そのアニメは根の動きを移さない
	const ZombieRootMotion* mAttackRootMotion;
	const ZombieRootMotion* mStaggerRootMotion;
	const ZombieRootMotion* mDeathRootMotion;
	const ZombieRootMotion* mDownedRootMotion;
	float mMoveStartTime; // 歩き・立つのアニメを最初に始める時刻（体ごとにずらした値）
	Gait mGait;           // 今流している足の運び
};

#endif
