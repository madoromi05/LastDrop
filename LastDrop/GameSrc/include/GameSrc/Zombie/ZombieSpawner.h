#ifndef INCLUDED_ZOMBIE_SPAWNER_H
#define INCLUDED_ZOMBIE_SPAWNER_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Random.h"
#include <vector>

class HitWorld;
class CollisionWorld;
class BodyFootprint;

// ゾンビを「いつ・どこに」湧かせるかを決める。自分では作らない（作るのは GameWorld::spawnZombie）
// ・始め：最初の plan で、目標の数まで湧いてよい場所を選ぶ
// ・補充：立っている数が目標より少ない間だけ決まった時間を数え、0 になったら湧いてよい場所を 1 か所選ぶ
// ・目標の数は決まった数（時間・日で増やすのは段階R4。R2_Plan.txt Q15）
// 湧いてよい場所＝候補のうち、自キャラから離れていて、自キャラから見えず（画面の外か物の陰）、何にも重ならない所
class ZombieSpawner{
public:
	// このティックの世界の様子（持ち主が毎ティック作って渡す。spawner が自キャラ・車の中身を知らずに済むように）
	struct View{
		GameLib::Math::Vector3 playerEye; // 自キャラの目の位置
		float viewYaw;                    // カメラの水平の向き（度。0 は +Z）
		int standingZombies;              // 立っているゾンビの数（Downed・死体は数えない）
		bool hasRoom;                     // 全体の上限まで余裕があるか（無ければ補充を待つ）
	};
	// 出す場所と向き
	struct Order{
		GameLib::Math::Vector3 position;
		float yawDegree;
	};

	// candidates: 湧く場所の候補（足元の位置）。写して持つ
	// hitWorld / collisionWorld: 見えているか・重なるかを聞く名簿。footprint: ゾンビの体の長方形の大きさ
	// どれも所有権は受け取らない。nullptr 不可。この spawner より長く生きていること
	// randomSeed: 候補・向きを選ぶ乱数の種
	// targetCount: 立っているゾンビの目標の数（日の条件で変わる。DayConditionEffect::zombieCount）。1 以上
	ZombieSpawner( const std::vector< GameLib::Math::Vector3 >& candidates, const HitWorld* hitWorld,
		const CollisionWorld* collisionWorld, const BodyFootprint* footprint, int randomSeed, int targetCount );
	ZombieSpawner( const ZombieSpawner& ) = delete;
	ZombieSpawner& operator=( const ZombieSpawner& ) = delete;

	// 1ティック考えて、このティックに出す所を out の後ろへ足す（出さなければ足さない）
	// 最初の 1 回は、目標の数まで（湧いてよい場所が足りる分だけ）まとめて足す。それ以降は多くても 1 か所
	// 持ち主は、全員の update と flush が済んだ後（名簿をたどっていない所）で呼び、足された所へ出す。out: nullptr 不可
	void update( const View& view, std::vector< Order >* out );
	// 目標の数を替える（F6 で今日の条件を切り替えたとき）。1 以上。多い分のゾンビは消さない（減るのを待つ）
	void setTargetCount( int targetCount );

	// ── 確認用の表示（F3）が読む ──
	int targetCount() const;          // 目標の数（作るときに受け取った値か、setTargetCount で替えた値）
	int standingCount() const;        // 最後の update で渡された、立っている数
	float secondsUntilNext() const;   // 次の補充までの残り（秒。補充を待っていない間は補充の間隔そのもの）

private:
	// 始めに置く分を out へ足す（候補を混ぜて、湧いてよい所から順に）
	void planInitial( const View& view, std::vector< Order >* out );
	// 補充の残りを数え、0 なら候補を何か所か調べ、湧いてよい所が見つかれば out へ足す
	void planRefill( const View& view, std::vector< Order >* out );
	// position に yawDegree で出してよいか（離れている・見えない・重ならない。軽い順に調べる）
	bool canSpawnAt( const View& view, const GameLib::Math::Vector3& position, float yawDegree ) const;
	// 自キャラから head（ゾンビの頭）が見えるか（画面の内で、間に何も無い）
	bool isVisible( const View& view, const GameLib::Math::Vector3& head ) const;

	std::vector< GameLib::Math::Vector3 > mCandidates;
	const HitWorld* mHitWorld;
	const CollisionWorld* mCollisionWorld;
	const BodyFootprint* mFootprint;
	GameLib::Math::Random mRandom;
	bool mPlacedInitial;  // 始めに置く分を済ませたか
	int mRefillTicks;     // 次の補充までの残り（ティック）
	int mTargetCount;     // 立っているゾンビの目標の数（作るときに受け取る。F6 で替わる）
	int mStandingCount;   // 確認用（同上）
};

#endif
