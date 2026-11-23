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

// ゾンビの数と、ゾンビ全員の周りを受け持つ係
//   数     … 設計図（ZombieModel）・組み立て（ZombieFactory）・見えない所へ湧かせる（ZombieSpawner）・同時に居られる数（今日の条件）
//   周り   … Think の段で全員が読む物（追う先・群れ・攻撃に行っている体・アニメの間引きの視点・道探しの回数）
//   音     … 音をゾンビ全員へ知らせる ZombieHearing（共有の棚に置き、音を立てる部品が引く）
// 何体ものゾンビをまとめて扱うので部品にはしない（1 体の振る舞いはゾンビの部品。ZombieBrainComponent 以下）
// 1 ティックの順番は持たない（GameWorld::update が、prepareThink → Think の段 → finishThink の順に呼ぶ）
class ZombieDirector{
public:
	// ゾンビの形の読み込みを頼むだけ（非同期）。どれも所有権は受け取らない。nullptr 不可。この係より長く生きていること
	// objects はまだ作られていなくてよい（覚えるだけ。使うのは provideShared から）
	// day: 今日の条件（同時に居られる数・足音の届く距離）
	ZombieDirector( MeshCache* meshes, GameObjectWorld* objects, const HitWorld* hitWorld, const CollisionWorld* collisionWorld,
		const DayConditionEffect* day );
	~ZombieDirector();
	ZombieDirector( const ZombieDirector& ) = delete;
	ZombieDirector& operator=( const ZombieDirector& ) = delete;

	void provideShared();	// ゾンビの部品が共有の棚から引く物（TickContext・PathBudget・ZombieHearing）を置く。物の並びを作った後に 1 回
	// ナビメッシュの床の縁を壁から離す長さ（m）。ゾンビの移動の長方形が収まる円の半径（BodyFootprint::reachRadius）に、
	// 壁との余白（ZombieCrowdParams::navMeshClearance）を足す。GameWorld::buildNavMesh から（setup より前）
	// 長方形を求めるのでゾンビの設計図をここで作る（setup はそれを使う）。ゾンビの形が読めていなければ既定の長さ
	float navMeshRadius();
	// 上と同じ求め方で、体の余白（footprintPadding）と壁との余白（navMeshClearance）だけを、F6・プリセットの今の値ではなく
	// コードの既定値（ZombieCrowdParams の初期値）にした長さ。Release が出撃のロードで求める長さと同じになる
	// 事前に書き出す床（TownCache::bake）はこの長さで作る。体の頂点を全部調べ直すので、毎フレームは呼ばない（ボタンを押したときなど）
	// ゾンビの形が読めていなければ既定の長さ
	float defaultNavMeshRadius() const;
	// ゾンビの組み立て・湧かせる係を作る（GameWorld::placeActors から。ゾンビの形の構築に失敗していれば何もしない）
	// 設計図がまだ無ければ作る。map: 湧いてよい場所の候補を読む
	void setup( const TownMap& map );

	// 歩いて追ってくるゾンビを 1 体出す（倒れ方は体ごとにランダム）。update の外（ティックの間）で呼ぶ
	// setup の前・形の構築に失敗した・数が上限に達しているなら出さずに false
	bool spawn( const GameLib::Math::Vector3& position, float yawDegree );
	// 今日の条件（借りている day）の中身が替わった（F6）。湧かせる係の目標の数を合わせる（同時に居られる数と足音は毎回 day から読む）
	// setup の前なら何もしない（setup で day から読む）
	void onDayConditionChanged();

	void beginTick();						// ティックの始めに、このティックに道を探してよい回数を戻す
	// 今のフレームで絵を描いたカメラを覚える（次のティックのアニメの間引きに使う）
	// visibility: 隠れの表（F6 の PVS を切っている・まだ無いなら nullptr）
	void setAnimationView( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& eye, const TownVisibility* visibility );
	// Think の段の前に、全員が読む周りを作る（追う先＝このティックに動いた後の自キャラ・攻撃に行っている体・立っている仲間の位置）
	// player: 自キャラ（居なければ nullptr。追わない）
	void prepareThink( const GameObject* player );
	void finishThink( GameObject* player );	// Think の段の後に、自キャラがつかまれているかを伝える（つかんでいる体が倒れた・振りほどかれた・噛み終えたら、次の update から動ける）
	// ゾンビを止めている間（zombiesFrozen）、finishThink の代わりに呼ぶ。つかまれている自キャラを放す
	// （つかんだ体が固まったまま自キャラも動けない、とならないように。戻すと finishThink がまたつかまれた形にする）
	void releaseTarget( GameObject* player );
	// 立っているゾンビの数・カメラの向きを ZombieSpawner に渡し、出すと言われた所へ spawn する（ティックの最後。並びをたどり終えてから）
	// cameraYaw: カメラの水平の向き（度）。自キャラが居なければ何もしない（見えない所を決められない）
	void spawnFromSpawner( const GameObject* player, float cameraYaw );

	ZombieHearing& hearing();				// 音をゾンビへ知らせる（物音・確認用。ふだんの足音・銃声は自キャラの PlayerNoiseComponent が棚から引いて使う）
	const ZombieSpawner* spawner() const;	// ゾンビを湧かせる係（確認用の表示が読む）。setup の前・形が無ければ nullptr

private:
	bool prepareModel();	// ゾンビの設計図がまだ無ければ作る。ゾンビの形が読めていなければ作らずに false

	GameObjectWorld* mObjects;							// 借りているだけ
	const HitWorld* mHitWorld;							// 借りているだけ（湧いてよいかの見通し）
	const CollisionWorld* mCollisionWorld;
	const DayConditionEffect* mDay;						// 借りているだけ
	const SkinnedMeshData* mZombieData;					// MeshCache から借りる
	PathBudget mPathBudget;								// 1 ティックに道を探してよい回数を配る。ゾンビの通り道が棚から借りる（ゾンビより長く生きる）
	std::unique_ptr< ZombieModel > mZombieModel;		// ゾンビ全員で共有する決まりごと（設計図）。ゾンビが借りる
	ZombieBrainComponent::TickContext mContext;			// Think の段で全員が読む、このティックの周り。ゾンビが棚から借りる
	ChaseTarget mChaseTarget;							// mContext.target が指す（自キャラが居る間）
	std::optional< AnimationView > mAnimationView;		// mContext.view が指す（setAnimationView の後）
	ZombieCrowd mCrowd;									// 立って歩けるゾンビの位置（prepareThink で作り直す）
	ZombieHearing mHearing;
	std::unique_ptr< ZombieFactory > mFactory;			// mZombieModel を借りる
	GameLib::Math::Random mRandom;						// 倒れ方（死亡アニメ）・湧かせ方の種を選ぶ（setup で種を決める）
	int mZombieNumber;									// これまでに出したゾンビの数（歩き出しの時刻・見通しを調べる時刻をずらす番号）
	std::unique_ptr< ZombieSpawner > mSpawner;			// mZombieModel の体の大きさを借りる
	std::vector< ZombieSpawner::Order > mSpawnOrders;	// このティックに出す所（毎回確保し直さないために持つ）
};

// ゾンビを全部止めるか（F6 → Debug の Freeze zombies。確認用）
// true の間は GameWorld::update が Think の段・ゾンビの攻撃・湧かせる係を飛ばす（考える・動く・アニメ・攻撃・湧く、が止まる）
// 撃てば体力は減るが、ひるむ・倒れる動きは戻してから出る。起動中ずっと 1 つ（初期値 false。Release では触る所が無いので false のまま）
bool& zombiesFrozen();

#endif
