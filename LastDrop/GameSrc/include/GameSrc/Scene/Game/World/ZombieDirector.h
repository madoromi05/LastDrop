#ifndef INCLUDED_ZOMBIE_DIRECTOR_H
#define INCLUDED_ZOMBIE_DIRECTOR_H

#include "GameSrc/Map/PathBudget.h"
#include "GameSrc/Zombie/Animation/ZombieModel.h"
#include "GameSrc/Zombie/Component/ZombieBrainComponent.h"
#include "GameSrc/Zombie/ZombieFactory.h"
#include "GameSrc/Zombie/ZombieSpawner.h"
#include "GameSrc/Zombie/Move/ZombieCrowd.h"
#include "GameSrc/Zombie/Move/ChaseTarget.h"
#include "GameSrc/Zombie/Move/ZombieHearing.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Random.h"
#include <memory>
#include <optional>
#include <vector>

class MeshCache;
class SkinnedMeshData;
class TownMap;
class TownVisibility;
class HitWorld;
class CollisionWorld;
class GameObject;
class GameObjectWorld;
struct DayConditionEffect;

// ゾンビの数と、ゾンビ全員の周りを受け持つ係（SummaryPlan/個別仕様/設計と分割/HighPrioritySplit_Plan.txt 3-2。2026-11-09 GameWorld から分けた）。
//   数     … 設計図（ZombieModel）・組み立て（ZombieFactory）・見えない所へ湧かせる（ZombieSpawner）・同時に居られる数（今日の条件）
//   周り   … Think の段で全員が読む物（追う先・群れ・攻撃に行っている体・アニメの間引きの視点・道探しの回数）
//   音     … 音をゾンビ全員へ知らせる ZombieHearing（共有の棚に置き、音を立てる部品が引く）
// 何体ものゾンビをまとめて扱うので部品にはしない（1 体の振る舞いはゾンビの部品。ZombieBrainComponent 以下）。
// 1 ティックの順番は持たない（GameWorld::update が、prepareThink → Think の段 → finishThink の順に呼ぶ）。
class ZombieDirector{
public:
	// ゾンビの形の読み込みを頼むだけ（非同期）。どれも所有権は受け取らない。nullptr 不可。この係より長く生きていること。
	// objects はまだ作られていなくてよい（覚えるだけ。使うのは provideShared から）。
	// day: 今日の条件（同時に居られる数・足音の届く距離）。
	ZombieDirector( MeshCache* meshes, GameObjectWorld* objects, const HitWorld* hitWorld, const CollisionWorld* collisionWorld,
		const DayConditionEffect* day );
	~ZombieDirector();
	ZombieDirector( const ZombieDirector& ) = delete;
	ZombieDirector& operator=( const ZombieDirector& ) = delete;

	// ゾンビの部品が共有の棚から引く物（TickContext・PathBudget・ZombieHearing）を置く。物の並びを作った後に 1 回。
	void provideShared();
	// ゾンビの設計図・組み立て・湧かせる係を作る（GameWorld::placeActors から。ゾンビの形の構築に失敗していれば何もしない）。
	// map: 湧いてよい場所の候補を読む。
	void setup( const TownMap& map );

	// 歩いて追ってくるゾンビを 1 体出す（倒れ方は体ごとにランダム）。update の外（ティックの間）で呼ぶ。
	// setup の前・形の構築に失敗した・数が上限に達しているなら出さずに false。
	bool spawn( const GameLib::Math::Vector3& position, float yawDegree );

	// ティックの始めに、このティックに道を探してよい回数を戻す（OptimizationC_Plan.txt 6 章）。
	void beginTick();
	// 今のフレームで絵を描いたカメラを覚える（次のティックのアニメの間引きに使う。OptimizationB_Plan.txt Q9）。
	// visibility: 隠れの表（F6 の PVS を切っている・まだ無いなら nullptr）。
	void setAnimationView( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& eye, const TownVisibility* visibility );
	// Think の段の前に、全員が読む周りを作る（追う先＝このティックに動いた後の自キャラ・攻撃に行っている体・立っている仲間の位置）。
	// player: 自キャラ（居なければ nullptr。追わない）。
	void prepareThink( const GameObject* player );
	// Think の段の後に、自キャラがつかまれているかを伝える（つかんでいる体が倒れた・振りほどかれた・噛み終えたら、次の update から動ける）。
	void finishThink( GameObject* player );
	// 立っているゾンビの数・カメラの向きを ZombieSpawner に渡し、出すと言われた所へ spawn する（ティックの最後。並びをたどり終えてから）。
	// cameraYaw: カメラの水平の向き（度）。自キャラが居なければ何もしない（見えない所を決められない）。
	void spawnFromSpawner( const GameObject* player, float cameraYaw );

	// 音をゾンビへ知らせる（物音・確認用。ふだんの足音・銃声は自キャラの PlayerNoiseComponent が棚から引いて使う）。
	ZombieHearing& hearing();
	// ゾンビを湧かせる係（確認用の表示が読む）。setup の前・形が無ければ nullptr。
	const ZombieSpawner* spawner() const;

private:
	GameObjectWorld* mObjects;        // 借りているだけ
	const HitWorld* mHitWorld;        // 借りているだけ（湧いてよいかの見通し）
	const CollisionWorld* mCollisionWorld;
	const DayConditionEffect* mDay;   // 借りているだけ
	const SkinnedMeshData* mZombieData; // MeshCache から借りる
	// 1 ティックに道を探してよい回数を配る。ゾンビの通り道が棚から借りる（ゾンビより長く生きる）。
	PathBudget mPathBudget;
	// ゾンビ全員で共有する決まりごと（設計図）。ゾンビが借りる。
	std::unique_ptr< ZombieModel > mZombieModel;
	// Think の段で全員が読む、このティックの周り。ゾンビが棚から借りる。
	ZombieBrainComponent::TickContext mContext;
	ChaseTarget mChaseTarget;                      // mContext.target が指す（自キャラが居る間）
	std::optional< AnimationView > mAnimationView; // mContext.view が指す（setAnimationView の後）
	ZombieCrowd mCrowd;                            // 立って歩けるゾンビの位置（prepareThink で作り直す）
	ZombieHearing mHearing;
	std::unique_ptr< ZombieFactory > mFactory;     // mZombieModel を借りる
	GameLib::Math::Random mRandom; // 倒れ方（死亡アニメ）・湧かせ方の種を選ぶ（setup で種を決める）
	int mZombieNumber;             // これまでに出したゾンビの数（歩き出しの時刻・見通しを調べる時刻をずらす番号）
	std::unique_ptr< ZombieSpawner > mSpawner;          // mZombieModel の体の大きさを借りる
	std::vector< ZombieSpawner::Order > mSpawnOrders;   // このティックに出す所（毎回確保し直さないために持つ）
};

#endif
