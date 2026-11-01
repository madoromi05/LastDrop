#ifndef INCLUDED_ZOMBIE_LOCOMOTION_H
#define INCLUDED_ZOMBIE_LOCOMOTION_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Zombie/ZombieChase.h"
#include "GameSrc/Zombie/ZombiePath.h"

class GameObject;
class NavMesh;
class CharacterMover;
class HitWorld;
class ZombieCrowd;
struct ChaseTarget;

// ゾンビ1体の「どこへ行き、どちらを向いて、どう進むか」（SummaryPlan/個別仕様/ZombieChase_Plan.txt・ZombiePath_Plan.txt・
// ZombieCrowd_Plan.txt・ZombieCrawl_Plan.txt）。行き先を決めるのは ZombieChase、建物を回り込む通り道は ZombiePath（どちらもここが持つ）。
// 立っている体は、向く点に近くの仲間から離れる向きと、フラつきで倒れている側への流れを混ぜる。這っている体は混ぜない。
// 足元と向きは持ち主（Zombie）が持ち、関数ごとに渡す。攻撃を始めてよいか・どのアニメを流すかは知らない。
// 考える（thinkStanding / thinkCrawling）と動く（walkStep / standAside）を分けてあり、その間で持ち主が攻撃を始めるかを見る
// （SummaryPlan/個別仕様/ZombieSplit2_Plan.txt の 3 章）。
class ZombieLocomotion{
public:
	// 向きを変える速さが違う（這っている体はゆっくり）。
	enum class Posture{
		Standing,
		Crawling,
	};

	// 考えた結果。
	struct Intent{
		bool walking;   // 歩きたいか（ZombieChase::wantsToWalk）
		bool facing;    // 行き先の方を向き終わったか（向きの差が大きいうちは false。まだ進まない）
		bool wandering; // うろついているか（Wander。這っている体はその場でもがくだけ）
	};

	// self: ZombieCrowd に自分を名乗るための持ち主（ポインタを比べるだけで、中身は読まない）。
	// navMesh: 建物を回り込む通り道を聞く町の床の形。mover: 壁・ほかの体に沿って滑る移動。
	// どれも所有権は受け取らない。nullptr 不可。この部品より長く生きていること（mover は同じ Zombie のメンバで、この部品より先に宣言する）。
	// sightPhase: 見通し・まっすぐ通れるかを調べる時刻をずらす番号。randomSeed: うろつく先・立つ長さを選ぶ乱数の種（ZombieChase）。
	ZombieLocomotion( const GameObject* self, const NavMesh* navMesh, const CharacterMover* mover, int sightPhase, int randomSeed );
	ZombieLocomotion( const ZombieLocomotion& ) = delete;
	ZombieLocomotion& operator=( const ZombieLocomotion& ) = delete;

	// ── 考える ──
	// 立っている体：見て・行き先を決め・回り込む点を選び・向く点へ 1 ティックぶん向きを変える。
	// 歩いている間は、向く点に近くの仲間から離れる向きと、weaveDegree（フラつきで流す角度。0 なら流さない）を混ぜる。
	// center: 体の中心（足元）。eye: 目の位置。crowd: 近くの仲間（nullptr なら避けない）。angleY: 向き（度）。nullptr 不可。
	Intent thinkStanding( const GameLib::Math::Vector3& center, const GameLib::Math::Vector3& eye, const ChaseTarget* target,
		const HitWorld& hitWorld, const ZombieCrowd* crowd, float weaveDegree, float* angleY );
	// 這っている体：同じことを、避けず・揺れずに行う。うろついている間は通り道を捨て、向きも変えない（もがくだけ）。
	// center: 胸の位置。eye: 胸の上の目の位置。angleY: 進む向き（度）。nullptr 不可。
	Intent thinkCrawling( const GameLib::Math::Vector3& center, const GameLib::Math::Vector3& eye, const ChaseTarget* target,
		const HitWorld& hitWorld, float* angleY );
	// center から destination の方へ 1 ティックぶん向きを変える（攻撃中に相手の方を向くのにも使う）。
	// 向きの差が大きいうちは false（まだ進まない）。destination が center に近すぎれば、向きを変えずに true。
	bool turnToward( const GameLib::Math::Vector3& center, const GameLib::Math::Vector3& destination, Posture posture, float* angleY ) const;

	// ── 動く ──
	// angleY の向きへ distance だけ進もうとして、壁・車・ほかの体に沿って滑った後の移動量を返す（足すのは持ち主）。
	GameLib::Math::Vector3 walkStep( float distance, float angleY ) const;
	// 追いついて立ち止まっている間に、重なりそうな仲間から横へ半歩よける（向きは変えない）。footing を動かす。nullptr 不可。
	void standAside( const GameLib::Math::Vector3& center, const ZombieCrowd* crowd, GameLib::Math::Vector3* footing );

	// ── 持ち主が使う ──
	void beginTick();                                          // このティックの「仲間から離れる向き」を 0 に戻す（update の頭で呼ぶ）
	void alert();                                              // 撃たれた。見えていなくても気付く
	void hearNoise( const GameLib::Math::Vector3& position );  // position で銃声がした。少し遅れてから探しに来る
	void clearPath();                                          // 通り道を捨てる（攻撃を始めた・這い始めた）
	ZombieChase::Mode mode() const;                            // 今の考え（Wander / Chase / Search）
	const ZombiePath& path() const;                            // 建物を回り込む通り道（確認用の表示）
	// 最後に向く点へ混ぜた・横へよけた、仲間から離れる向き（水平。確認用の表示）。
	const GameLib::Math::Vector3& separation() const;

private:
	// 見て行き先を決め、探し始めたら「着けるか」を数え直す（立つ・這う共通の頭）。
	void look( const GameLib::Math::Vector3& center, const GameLib::Math::Vector3& eye, float angleY, const ChaseTarget* target,
		const HitWorld& hitWorld );
	// point（向かいたい点）へ、近くの仲間から離れる向きを混ぜた「向く点」を返し、混ぜた向きを mSeparation へ覚える。
	GameLib::Math::Vector3 avoidCrowd( const GameLib::Math::Vector3& center, const GameLib::Math::Vector3& point, const ZombieCrowd* crowd );
	// point を、center のまわりに degree だけ回した点（倒れている側へ流れて歩く）。degree が 0 なら point のまま。
	GameLib::Math::Vector3 weave( const GameLib::Math::Vector3& center, const GameLib::Math::Vector3& point, float degree ) const;

	const GameObject* mSelf;                // 名乗るだけ
	const NavMesh* mNavMesh;             // 借りているだけ
	const CharacterMover* mMover;        // 借りているだけ
	ZombieChase mChase;                  // どこへ行きたいか
	ZombiePath mPath;                    // 行き先へまっすぐ通れないときの通り道
	GameLib::Math::Vector3 mSeparation;  // 最後に混ぜた、仲間から離れる向き（確認用の表示）
};

#endif
