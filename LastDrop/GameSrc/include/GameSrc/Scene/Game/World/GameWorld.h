#ifndef INCLUDED_GAME_WORLD_H
#define INCLUDED_GAME_WORLD_H

#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Player/PlayerCommand.h"
#include "GameSrc/Player/PlayerFactory.h"
#include "GameSrc/Interaction/PropFactory.h"
#include "GameSrc/Weapon/Bullet/BulletList.h"
#include "GameSrc/Scene/Game/World/TownEnvironment.h"
#include "GameSrc/Scene/Game/World/ZombieDirector.h"
#include "GameSrc/Scene/Game/World/EffectManager.h"
#include "GameSrc/Scene/Game/World/CombatResolver.h"
#include "GameSrc/Scene/Game/World/InteractionSystem.h"
#include "GameSrc/Raid/DayCondition.h"
#include "GameLib/Math/Vector3.h"
#include <vector>

class MeshCache;
class TownCache;
class StaticMeshData;
class Inventory;
class PlayerComponent;

// 本編の世界（Model）。町（道路・歩道・建物）と、世界に置く物（GameObject。自キャラ・銃・ゾンビ・落ちているガソリンと弾・置物の車）の持ち主、
// 飛んでいる弾の並び、それらの当たり判定の名簿（移動の CollisionWorld・命中の HitWorld）の持ち主
// ここが持つ仕事は「今ある状態」と「1 ティックの順番」だけ。中身は係へ任せる
//   町（地図・地面・建物・歩ける床・隠れの表） … TownEnvironment（組み立ては WorldBuilder、置物の部品の並びは PropFactory）
//   ゾンビの数と周り・音を知らせる     … ZombieDirector（組み立ては ZombieFactory、音は ZombieHearing）
//   自キャラ                       … PlayerFactory（自キャラの足音・銃声は、自キャラの PlayerNoiseComponent が ZombieHearing へ）
//   弾痕・砂ぼこり・血・千切れた部位 … EffectManager
//   当たった後に何が起きるか     … CombatResolver（出来事の並びも持つ）
//   見て E で使う物を探す         … InteractionSystem
//   描く                         … WorldRenderer（シーンが持ち、この世界を読む。世界は描き方を知らない）
// 名簿へ載せる・外すのは物の部品（IWorldMember）が GameObjectWorld の spawn・flush の中で行う
// 建物・地面・飛ぶ弾・エフェクトは GameObject にしない（数が多く、まとめて描く・詰めて持つ方が速いため）
//
// 舞台は碁盤の目の町。町の周りは建物で囲んであり、外へは出られない
// 建物とゾンビと車は移動を止め、自キャラは当たると沿って滑る。道路と歩道は移動を止めないが、弾・視線は止める（TownGround）
// ガソリンは漁れる場所の箱に入っていて、脱出の車へ入れると脱出できる（持ち帰ると点にもなる）
// 1 日の時間・脱出点・終わり方は知らない（RaidSession。シーンが持つ）。入力・カメラ・音・HUD も知らない（シーンが、この世界を読んで動かす）
class GameWorld{
public:
	// 部品の読み込みを頼むだけ（非同期）。meshes->update() が true を返してから、下のロードの段階を順に呼ぶ
	// meshes: 設計図を借りる倉庫。town: 町の決まった形（歩ける床・隠れの表）を起動中 1 回だけ作って持つ所
	// どちらも所有権は受け取らない。nullptr 不可。この世界より長く生きていること
	// condition: 今日の条件（街灯・霧・足音の届く距離・ゾンビの数）。F6 の changeDayCondition で替わる
	GameWorld( MeshCache* meshes, TownCache* town, DayCondition condition );
	~GameWorld();
	GameWorld( const GameWorld& ) = delete;
	GameWorld& operator=( const GameWorld& ) = delete;

	// ロード
	// 構築に失敗したデータ（理由は MeshCache がデバッグ出力済み）の物は置かない（その物が無いまま、他の確認はできる）
	// 町（地面・建物・置物・漁れる場所・脱出の車・箱の中のガソリン）を組み立てる（TownEnvironment）
	// escapeCarSpot: 今日の脱出の車の置き場所（TownMap::escapeCarSpots の番号。RaidSession が選ぶ）
	void buildTown( int escapeCarSpot );
	// ゾンビの歩ける床を TownCache から借りて、共有の棚へ置く（その車の置き場所の床がまだ無ければ、町の動かない形から作る）。buildTown の後
	void buildNavMesh();
	// 建物の隠れの表を TownCache から借りる（同じ）。buildTown の後
	void buildVisibility();
	// 自キャラ（と手に持つ銃）を置き、ゾンビを湧かせる係（ZombieSpawner）を作る。歩ける床の後（ゾンビが共有の棚から床を借りる）
	// escapeCarSpot: buildTown と同じ番号（自キャラはその車のそばから始める。TownEscapeCarSpot::playerStart）
	void placeActors( int escapeCarSpot );
	// 物音（脱出の車のエンジンなど）がした。銃声と同じ距離の内のゾンビが、その場所へ寄ってくる
	void alertZombiesToNoise( const GameLib::Math::Vector3& position );
	// 1ティック進める
	//   Input（自キャラが操作どおりに動く・撃つ。足音・銃声をゾンビへ知らせる）→ 撃ったなら弾を出す → 弾を飛ばし、当たった所を CombatResolver が解決
	//   → エフェクトを進める → Think（ゾンビが追う・攻撃する。噛んだら CombatResolver が自キャラの体力を減らす。沈み終えた死体は自分で消える印を付ける）→
	//   Late（銃口が光る）→ 消す印の付いた物を消す（flush）→ 足りなければゾンビを見えない所へ湧かせる（ZombieSpawner）
	// command / cameraYaw / cameraPitch / inventory: PlayerComponent::setInput へそのまま渡す（cameraYaw は湧かせてよい「画面の外」を決めるのにも使う）
	// viewDirection: 弾を出す向き（長さ1。FirstPersonCamera::weaponDirection）。弾は自キャラの目からこの向きへ飛ぶ
	//                （画面の中央から、照準のぶれの分だけずれた所＝銃が向いている所へ飛ぶ）
	void update( const PlayerCommand& command, float cameraYaw, float cameraPitch, const GameLib::Math::Vector3& viewDirection,
		Inventory* inventory );

	// 歩いて追ってくるゾンビを1体出す（倒れ方は体ごとにランダム）
	// update の外（ティックの間）で呼ぶ（物の並びをたどっている最中に増やさない）
	// まだ place していない・ゾンビの形の構築に失敗した・数が上限に達しているなら出さずに false
	// position / yawDegree: 足元の位置と向き（度。0 は +Z）
	bool spawnZombie( const GameLib::Math::Vector3& position, float yawDegree );
	// カバンから捨てた物を、自キャラの少し前（向いている方）の地面へ置く（拾い直せる）
	// update の外（カバンの画面を開いて世界を止めている間）で呼ぶ。自キャラが居ない・形の構築に失敗したなら置かずに false
	// （そのとき物は無くなる。捨てる前に呼ぶ側が確かめる手段は無いので、置けなかったことだけ返す）
	// cameraYaw: カメラの水平の向き（度。0 は +Z）
	bool dropItem( const ItemStack& stack, float cameraYaw );
	// 今日の条件を今の出撃のまま切り替える（F6 の Raid の欄。遊び直さずに見比べるため）。update の外で呼ぶ
	// 街灯の光・霧・足音は次の読み取りから、ゾンビの目標の数は次の補充から、街灯のガラスはすぐ替わる
	// 多い分のゾンビは消さない（群れ → ふつうでも、居るゾンビはそのまま）。出撃の始めの知らせは出し直さない
	void changeDayCondition( DayCondition condition );

	// 今のフレームで絵を描いたカメラを覚える。次の update でゾンビが、アニメを間引く段（映っているか・遠いか）を決めるのに読む
	void setAnimationView( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& eye );

	// 見て E で使う物を探す・拾われた物を消す（Interactor の finder と、1 回の描画ごとの後片付け）
	InteractionSystem& interaction();
	const InteractionSystem& interaction() const;
	// 当たった後の出来事（音・画面の赤みの係が、update の後に読む）
	const CombatResolver& combat() const;
	// 弾痕・砂ぼこり・血・千切れた部位。描くときに並べ替えるので、WorldRenderer へ書いてよい形で貸す（描く以外には使わない）
	EffectManager& effects();

	// ── 読み取り口（描く・確認用の表示・音が読む）──
	const TownEnvironment& town() const;                // 町（地図・道路と歩道・建物・歩ける床・隠れの表）
	const ZombieDirector& zombies() const;              // ゾンビの数と周り（確認用の表示が、湧かせる係を読む）
	const DayConditionEffect& dayCondition() const;		// 今日の条件の効き目（街灯・霧・HUD はシーンがここから読む）
	DayCondition dayConditionKind() const;				// 今日の条件がどれか（F6 の Raid の欄が出す・結果の画面へ渡す）
	const CollisionWorld& collisionWorld() const;		// 移動の当たり判定の名簿
	const HitWorld& hitWorld() const;					// 命中の当たり判定の名簿
	const BulletList& bullets() const;					// 飛んでいる弾
	// 自キャラの物（部品は PlayerComponent の説明）。place の前・構築に失敗して置けなかったときは nullptr
	// 返すポインタはこの世界がある間使える（自キャラは消えない）
	GameObject* player();
	const GameObject* player() const;
	// 自キャラの、命中の名簿に載っている部位の箱・移動の名簿に載っている長方形（自分の撃った弾・視線から除く、確認用の表示で隠すのに使う）
	// 自キャラが居ない・その部品を外していれば nullptr
	const IHittable* playerTarget() const;
	const ICollisionBody* playerBody() const;
	// 世界に置いた物（自キャラ・銃・ゾンビ・落ちている物・車…）。確認用の表示が forEach< ZombieStateComponent > で
	// ゾンビ全員（倒れている途中・死体も含む）の体力・通り道を読む、F6 のエディタが物の一覧を出す、WorldRenderer が描くのに使う
	// 書ける方は、update の外で部品の値を入れる係（弾倉の数を入れる MagazineSync）だけが使う（物を増やす・消すのは世界の中の係）
	const GameObjectWorld& objects() const;
	GameObjectWorld& objects();
	// 自キャラが死んだか（自キャラが居なければ false）
	bool isPlayerDead() const;

private:
	// 物を組み立てて置く関数は、1 つの関数で 1 つの物だけを作る（中の変数は画像と同じ obj）
	// 自キャラ（と手に持つ銃）を、その日の脱出の車のそばへ置く（place から。組み立ては mPlayerFactory）
	void placePlayer( int escapeCarSpot );

	// 今日の条件と、その効き目（DayCondition.cpp の表の 1 行の写し。F6 で書き替わるので写して持つ）
	// ゾンビの係が mDay を指して読むので、mZombies より先に宣言する
	DayCondition mCondition;
	DayConditionEffect mDay;
	// 自キャラと銃・置物を部品から組み立てる（形は MeshCache から借りる）。作った物より長く生きるよう mObjects より先に宣言する
	PlayerFactory mPlayerFactory;
	PropFactory mPropFactory;
	// 町。名簿が地面・建物を指すので、名簿より先に宣言する（mPropFactory を借りるので、その後に宣言する）
	TownEnvironment mTown;
	const StaticMeshData* mBulletData;    // MeshCache から借りる。飛ぶ弾
	CollisionWorld mCollisionWorld;			// 当たり判定の名簿。載る物を借りて持つので、載る物より先に宣言する
	HitWorld mHitWorld;
	BulletList mBullets;			// 飛んでいる弾。名簿には載らない（名簿へ当たりを聞くだけ）
	EffectManager mEffects;	// 弾痕・砂ぼこり・血しぶき・千切れた部位
	// 弾が当たった所（update の作業領域。毎回確保し直さないために持つ）
	std::vector< BulletList::Impact > mImpacts;
	// ゾンビの数と周り。ゾンビが設計図・TickContext・道探しの回数を借りるので、mObjects より先に宣言する（＝後に破棄される）
	ZombieDirector mZombies;
	// 世界に置く物（自キャラ・銃・ゾンビ・落ちている物・車）。物（GameObject）に部品を付けて持つ
	// （名簿への登録・解除もここが行う）
	// 物が名簿・町の歩ける床・mZombies の物を借りるので、それらより後に宣言する（＝先に破棄される）
	GameObjectWorld mObjects;
	// 当たった後を解決する・使う物を探す。mObjects・mEffects・名簿を借りるので、それらより後に宣言する
	CombatResolver mCombat;
	InteractionSystem mInteraction;
	GameObject* mPlayer;             // 自キャラの物（mObjects が持つ。置けなければ nullptr）
	PlayerComponent* mPlayerControl; // 自キャラの操作を読む部品（同上）
};

#endif
