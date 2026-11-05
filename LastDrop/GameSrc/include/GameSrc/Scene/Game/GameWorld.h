#ifndef INCLUDED_GAME_WORLD_H
#define INCLUDED_GAME_WORLD_H

#include "GameSrc/Map/TownMap.h"
#include "GameSrc/Map/TownGround.h"
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Map/PathBudget.h"
#include "GameSrc/Map/TownVisibility.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Zombie/ZombieModel.h"
#include "GameSrc/Zombie/ZombieBrainComponent.h"
#include "GameSrc/Zombie/ZombieFactory.h"
#include "GameSrc/Zombie/ZombieSpawner.h"
#include "GameSrc/Zombie/ZombieCrowd.h"
#include "GameSrc/Zombie/ChaseTarget.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Map/Building.h"
#include "GameSrc/Player/PlayerCommand.h"
#include "GameSrc/Player/PlayerFactory.h"
#include "GameSrc/Interaction/PropFactory.h"
#include "GameSrc/Weapon/BulletList.h"
#include "GameSrc/Scene/Game/WorldBuilder.h"
#include "GameSrc/Scene/Game/EffectManager.h"
#include "GameSrc/Scene/Game/CombatResolver.h"
#include "GameSrc/Scene/Game/InteractionSystem.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Random.h"
#include <memory>
#include <optional>
#include <vector>

class MeshCache;
class TownCache;
class SkinnedMeshData;
class StaticMeshData;
class Inventory;
class PlayerComponent;

// 本編の世界（Model）。町（道路・歩道・建物）と、世界に置く物（GameObject。自キャラ・銃・ゾンビ・落ちているガソリンと弾・置物の車）の持ち主、
// 飛んでいる弾の並び、それらの当たり判定の名簿（移動の CollisionWorld・命中の HitWorld）の持ち主。
// ここが持つ仕事は「今ある状態」と「1 ティックの順番」だけ。中身は係へ任せる（SummaryPlan/個別仕様/GameWorldSplit_Plan.txt。2026-11-02 分けた）：
//   最初に町を組み立てる         … WorldBuilder（置物の部品の並びは PropFactory、自キャラは PlayerFactory、ゾンビは ZombieFactory）
//   弾痕・砂ぼこり・血・千切れた部位 … EffectManager
//   当たった後に何が起きるか     … CombatResolver（出来事の並びも持つ）
//   見て E で使う物を探す         … InteractionSystem
//   描く                         … WorldRenderer（シーンが持ち、この世界を読む。世界は描き方を知らない）
// 名簿へ載せる・外すのは物の部品（IWorldMember）が GameObjectWorld の spawn・flush の中で行う。
// 建物・地面・飛ぶ弾・エフェクトは GameObject にしない（数が多く、まとめて描く・詰めて持つ方が速いため。★1）。
//
// 舞台は碁盤の目の町。町の周りは建物で囲んであり、外へは出られない。
// 建物とゾンビと車は移動を止め、自キャラは当たると沿って滑る。道路と歩道は移動を止めないが、弾・視線は止める（TownGround）。
// ガソリンは漁れる場所の箱に入っていて、脱出の車へ入れると脱出できる（FuelEscapeLoading_Plan.txt。持ち帰ると点にもなる）。
// 1 日の時間・脱出点・終わり方は知らない（RaidSession。シーンが持つ）。入力・カメラ・音・HUD も知らない（シーンが、この世界を読んで動かす）。
class GameWorld{
public:
	// 部品の読み込みを頼むだけ（非同期）。meshes->update() が true を返してから、下のロードの段階を順に呼ぶ。
	// meshes: 設計図を借りる倉庫。town: 町の決まった形（歩ける床・隠れの表）を起動中 1 回だけ作って持つ所。
	// どちらも所有権は受け取らない。nullptr 不可。この世界より長く生きていること。
	GameWorld( MeshCache* meshes, TownCache* town );
	~GameWorld();
	GameWorld( const GameWorld& ) = delete;
	GameWorld& operator=( const GameWorld& ) = delete;

	// ── ロード（FuelEscapeLoading_Plan.txt 6 章）。全データの構築後に、GameLoader が 1 回の描画に 1 つずつ、この順に 1 回だけ呼ぶ ──
	// 構築に失敗したデータ（理由は MeshCache がデバッグ出力済み）の物は置かない（その物が無いまま、他の確認はできる）。
	// 町（地面・建物・置物・漁れる場所・脱出の車・箱の中のガソリン）を組み立てる（WorldBuilder）。
	// escapeCarSpot: 今日の脱出の車の置き場所（TownMap::escapeCarSpots の番号。RaidSession が選ぶ）。
	void buildTown( int escapeCarSpot );
	// ゾンビの歩ける床を TownCache から借りる（その車の置き場所の床がまだ無ければ、町の動かない形から作る）。buildTown の後。
	void buildNavMesh();
	// 建物の隠れの表を TownCache から借りる（同じ）。buildTown の後。
	void buildVisibility();
	// 自キャラ（と手に持つ銃）と確認用のゾンビを置く。歩ける床の後（ゾンビが共有の棚から床を借りる）。
	void placeActors();
	// 物音（脱出の車のエンジンなど）がした。銃声と同じ距離の内のゾンビが、その場所へ寄ってくる（FuelEscapeLoading_Plan.txt Q5）。
	void alertZombiesToNoise( const GameLib::Math::Vector3& position );
	// 1ティック進める。段の順（UpdatePhase）：
	//   Input（自キャラが操作どおりに動く・撃つ）→ 撃ったなら弾を出し、近くのゾンビに銃声を聞かせる → 弾を飛ばし、当たった所を CombatResolver が解決
	//   → エフェクトを進める → Think（ゾンビが追う・攻撃する。腕が当たったら CombatResolver が自キャラの体力を減らす）→
	//   Late（銃口が光る）→ 沈み終わった死体と、消す印の付いた物を消す（flush）→ 足りなければゾンビを見えない所へ湧かせる（ZombieSpawner）。
	// command / cameraYaw / cameraPitch / inventory: PlayerComponent::setInput へそのまま渡す（cameraYaw は湧かせてよい「画面の外」を決めるのにも使う）。
	// viewDirection: 弾を出す向き（長さ1。FirstPersonCamera::weaponDirection）。弾は自キャラの目からこの向きへ飛ぶ
	//                （画面の中央から、照準のぶれの分だけずれた所＝銃が向いている所へ飛ぶ）。
	void update( const PlayerCommand& command, float cameraYaw, float cameraPitch, const GameLib::Math::Vector3& viewDirection,
		Inventory* inventory );

	// 歩いて追ってくるゾンビを1体出す（倒れ方は体ごとにランダム）。
	// update の外（ティックの間）で呼ぶ（物の並びをたどっている最中に増やさない）。
	// まだ place していない・ゾンビの形の構築に失敗した・数が上限に達しているなら出さずに false。
	// position / yawDegree: 足元の位置と向き（度。0 は +Z）。
	bool spawnZombie( const GameLib::Math::Vector3& position, float yawDegree );
	// カバンから捨てた物を、自キャラの少し前（向いている方）の地面へ置く（拾い直せる。GridInventory_Plan.txt Q6）。
	// update の外（カバンの画面を開いて世界を止めている間）で呼ぶ。自キャラが居ない・形の構築に失敗したなら置かずに false
	// （そのとき物は無くなる。捨てる前に呼ぶ側が確かめる手段は無いので、置けなかったことだけ返す）。
	// cameraYaw: カメラの水平の向き（度。0 は +Z）。
	bool dropItem( const ItemStack& stack, float cameraYaw );

	// 今のフレームで絵を描いたカメラを覚える。次の update でゾンビが、アニメを間引く段（映っているか・遠いか）を決めるのに読む
	// （OptimizationB_Plan.txt Q9）。描いた後に毎フレーム呼ぶ。呼ばれるまでは誰も間引かない。
	// projectionView：カメラの射影×ビュー（TAA でずらす前）。eye：目の位置。
	void setAnimationView( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& eye );

	// ── 係（シーンが直接頼む・読む）──
	// 見て E で使う物を探す・拾われた物を消す（Interactor の finder と、1 回の描画ごとの後片付け）。
	InteractionSystem& interaction();
	const InteractionSystem& interaction() const;
	// 当たった後の出来事（音・画面の赤みの係が、update の後に読む）。
	const CombatResolver& combat() const;
	// 弾痕・砂ぼこり・血・千切れた部位。描くときに並べ替えるので、WorldRenderer へ書いてよい形で貸す（描く以外には使わない）。
	EffectManager& effects();

	// ── 読み取り口（描く・確認用の表示・音が読む）──
	const TownMap& map() const;							// 町のどこに何を置くか
	const NavMesh& navMesh() const;					// 町の歩ける床の形（確認用の表示が、床の縁を描くのに使う）
	const TownGround& ground() const;					// 道路と歩道（足元の材質を聞ける）
	const CollisionWorld& collisionWorld() const;		// 移動の当たり判定の名簿
	const HitWorld& hitWorld() const;					// 命中の当たり判定の名簿
	const std::vector< Building >& buildings() const;	// 建てた建物（place の後は増えない）
	const TownVisibility& visibility() const;			// 建物・ゾンビの隠れの表（簡易 PVS。place で作る）
	const BulletList& bullets() const;					// 飛んでいる弾
	// 自キャラの物（部品は PlayerComponent の説明）。place の前・構築に失敗して置けなかったときは nullptr。
	// 返すポインタはこの世界がある間使える（自キャラは消えない）。
	GameObject* player();
	const GameObject* player() const;
	// 自キャラの、命中の名簿に載っている部位の箱・移動の名簿に載っている長方形（自分の撃った弾・視線から除く、確認用の表示で隠すのに使う）。
	// 自キャラが居ない・その部品を外していれば nullptr。
	const IHittable* playerTarget() const;
	const ICollisionBody* playerBody() const;
	// 銃の物 item（ITEM_KIND_WEAPON）の弾倉に入っている弾の数（銃が無ければ 0）・入れる（弾倉の大きさまでに収める）。
	// 拠点で覚えた数を出撃の始めに入れ、脱出したときに読み出す（R2_Plan.txt Q13）。銃は 1 種類 1 丁なので、種類で引ける。
	int magazineRounds( ItemId item ) const;
	void setMagazineRounds( ItemId item, int rounds );
	// 世界に置いた物（自キャラ・銃・ゾンビ・落ちている物・車…）。確認用の表示が forEach< ZombieStateComponent > で
	// ゾンビ全員（倒れている途中・死体も含む）の体力・通り道を読む、F6 のエディタが物の一覧を出す、WorldRenderer が描くのに使う。
	const GameObjectWorld& objects() const;
	// 自キャラが死んだか（自キャラが居なければ false）。
	bool isPlayerDead() const;
	// ゾンビを湧かせる係（確認用の表示が、立っている数・目標の数・次に湧くまでを読む）。ゾンビの形が無ければ nullptr。
	const ZombieSpawner* zombieSpawner() const;

private:
	// 物を組み立てて置く関数は、1 つの関数で 1 つの物だけを作る（中の変数は画像と同じ obj。SummaryPlan/個別仕様/ComponentSystem3_Plan.txt の ★6）。
	// 自キャラ（と手に持つ銃）を町の出発点へ置く（place から。組み立ては mPlayerFactory）。
	void placePlayer();
	// position（撃った瞬間の自キャラの足元）で銃声がした。届く距離の内のゾンビへ知らせる（update で撃ったティックに）。
	void alertZombiesToGunshot( const GameLib::Math::Vector3& position );
	// ゾンビの Think の段を動かし、腕の当たりを CombatResolver へ渡し、沈み終わった死体に消す印を付ける（update から）。
	// 攻撃に行けるのは一度に 1 体だけ（ZombieBrainComponent::TickContext::attacker）。
	void updateZombies();
	// 立っているゾンビの数・カメラの向きを ZombieSpawner に渡し、出すと言われた所へ spawnZombie する（update の最後から）。
	// cameraYaw: カメラの水平の向き（度）。
	void spawnZombiesFromSpawner( float cameraYaw );
	// ゾンビを組み立てて置く（spawnZombie と、place の確認用の T ポーズの1体から）。mZombieFactory があること。
	void addZombie( const GameLib::Math::Vector3& position, float yawDegree, const ZombieAnimationNames& animations );

	// 町のどこに何を置くか。
	TownMap mMap;
	// 町の決まった形の持ち主（GameResources が持つ。借りているだけ）。
	TownCache* mTown;
	// 町の歩ける床の形（ゾンビが建物を回り込む通り道を聞く）。buildNavMesh で mTown から借りる（それまでは nullptr）。
	NavMesh* mNavMesh;
	int mEscapeCarSpot; // 今日の脱出の車の置き場所（buildTown で受け取る。歩ける床を選ぶのに使う）
	// 1 ティックに道を探してよい回数を配る（OptimizationC_Plan.txt 6 章）。ゾンビの通り道が借りるので、ゾンビより先に宣言する。
	PathBudget mPathBudget;
	// 自キャラと銃・置物を部品から組み立てる（形は MeshCache から借りる）。作った物より長く生きるよう mObjects より先に宣言する。
	PlayerFactory mPlayerFactory;
	PropFactory mPropFactory;
	// 町を組み立てる（mMap・mPropFactory を借りるので、それらより後に宣言する）。
	WorldBuilder mBuilder;
	const StaticMeshData* mBulletData;    // どれも MeshCache から借りる。飛ぶ弾（7.62 NATO の弾頭）
	const SkinnedMeshData* mZombieData;
	TownGround mGround;                   // 道路と歩道（見た目・弾の当たり判定・足元の材質）

	// 当たり判定の名簿。載る物を借りて持つので、載る物より先に宣言する（＝載る物より後に破棄される）。
	CollisionWorld mCollisionWorld;
	HitWorld mHitWorld;
	// 飛んでいる弾。名簿には載らない（名簿へ当たりを聞くだけ）。
	BulletList mBullets;
	// 弾痕・砂ぼこり・血しぶき・千切れた部位（どの名簿にも載らない。千切れた部位はゾンビの形を借りるので mZombieData が生きている間だけ使う）。
	EffectManager mEffects;
	// 弾が当たった所（update の作業領域。毎回確保し直さないために持つ）。
	std::vector< BulletList::Impact > mImpacts;
	// ゾンビ全員で共有する決まりごと（当たり判定の設計図）。ゾンビが借りるので、ゾンビより先に宣言する。置けなかったときは空。
	std::unique_ptr< ZombieModel > mZombieModel;
	// ゾンビの Think の段で全員が読む、このティックの周り（追う先・仲間の位置・攻撃に行っている体）。ゾンビが借りるので、ゾンビより先に宣言する。
	ZombieBrainComponent::TickContext mZombieContext;
	ChaseTarget mChaseTarget; // mZombieContext.target が指す（自キャラが居る間）
	std::optional< AnimationView > mAnimationView; // mZombieContext.view が指す（setAnimationView の後）
	// 立って歩けるゾンビの位置の並び（updateZombies の頭で作り直し、mZombieContext から全員が読む。毎回確保し直さないために持つ）。
	ZombieCrowd mCrowd;
	// ゾンビを部品から組み立てる。mZombieModel を借りる（mNavMesh・名簿・mZombieContext は部品が mObjects の共有の棚から引く）。
	std::unique_ptr< ZombieFactory > mZombieFactory;
	// 世界に置く物（自キャラ・銃・ゾンビ・落ちている物・車）。物（GameObject）に部品を付けて持つ
	// （名簿への登録・解除もここが行う。SummaryPlan/個別仕様/ComponentSystem2_Plan.txt）。
	// 物が名簿・mZombieModel・mNavMesh・mZombieContext を借りるので、それらより後に宣言する（＝先に破棄される）。
	GameObjectWorld mObjects;
	// 当たった後を解決する・使う物を探す。mObjects・mEffects・名簿・mPropFactory を借りるので、それらより後に宣言する。
	CombatResolver mCombat;
	InteractionSystem mInteraction;
	GameObject* mPlayer;             // 自キャラの物（mObjects が持つ。置けなければ nullptr）
	PlayerComponent* mPlayerControl; // 自キャラの操作を読む部品（同上）
	GameLib::Math::Random mZombieRandom; // 倒れ方（死亡アニメ）・湧かせ方の種を選ぶ（place で種を決める）
	int mZombieNumber;                   // これまでに出したゾンビの数（歩き出しの時刻・見通しを調べる時刻をずらす番号）
	// いつ・どこにゾンビを湧かせるか。名簿（mHitWorld・mCollisionWorld）と mZombieModel の体の大きさを借りるので、それらより後に宣言する。
	std::unique_ptr< ZombieSpawner > mZombieSpawner;
	std::vector< ZombieSpawner::Order > mSpawnOrders; // このティックに出す所（毎回確保し直さないために持つ）
	// 名簿が要素を指すので、並べ終えた後は増やさない（増やすと並べ直しで要素の場所が変わる）。
	std::vector< Building > mBuildings;
	// 建物の隠れの表（OptimizationC_Plan.txt 7 章）。buildVisibility で mTown から借りる（それまでは nullptr）。
	const TownVisibility* mVisibility;
};

#endif
