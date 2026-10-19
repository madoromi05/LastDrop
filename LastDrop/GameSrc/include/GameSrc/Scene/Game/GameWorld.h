#ifndef INCLUDED_GAME_WORLD_H
#define INCLUDED_GAME_WORLD_H

#include "GameSrc/Map/TownMap.h"
#include "GameSrc/Map/TownGround.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Zombie/Zombie.h"
#include "GameSrc/Zombie/ZombieModel.h"
#include "GameSrc/Zombie/SeveredLimbList.h"
#include "GameSrc/World/ActorList.h"
#include "GameSrc/Map/Building.h"
#include "GameSrc/Interaction/Pickup.h"
#include "GameSrc/Interaction/EscapeCar.h"
#include "GameSrc/Interaction/ItemSource.h"
#include "GameSrc/Player/PlayerCommand.h"
#include "GameSrc/Weapon/BulletList.h"
#include "GameSrc/Weapon/BulletHoleList.h"
#include "GameSrc/Effect/ImpactDustList.h"
#include "GameSrc/Effect/BloodSprayList.h"
#include "GameSrc/Effect/MuzzleFlash.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Random.h"
#include <memory>
#include <vector>

class MeshCache;
class StaticMeshData;
class SkinnedMeshData;
class PlayerCharacter;
class IInteractable;
class ViewFrustum;

// 本編の世界。町（道路・歩道・建物）、自キャラ、飛んでいる弾、ゾンビ、ゲームループの物（脱出用の車・落ちているガソリン）と、
// それらの当たり判定の名簿（移動の CollisionWorld・命中の HitWorld）の持ち主。
// 「何をどこに置くか」は TownMap が決め、ここは物を作って名簿へ載せ、1ティック進め、描く。
// 名簿へ載せる・外すのはここだけなので、載せ忘れ・外し忘れを探す場所が1つになる。
//
// 舞台は碁盤の目の町。町の周りは建物で囲んであり、外へは出られない。
// 建物とゾンビと車は移動を止め、自キャラは当たると沿って滑る。道路と歩道は移動を止めないが、弾・視線は止める（TownGround）。
// ゲームループ: 候補点から選んだ場所にガソリンが出る。拾って車へ運び、決まった数を入れると車が走れるようになる（isEscapeReady）。
// その後どうするか（クリア画面へ進む）は知らない。入力・カメラ・音・HUD も知らない（シーンが、この世界を読んで動かす）。
class GameWorld{
public:
	// ゾンビに弾が当たった出来事（ヒット音を鳴らすなど、世界の外の係が読む）。死体に当たったときも入る。
	struct ZombieHit{
		GameLib::Math::Vector3 point; // 当たった位置（ワールド座標）
		BodyPart part;                // 当たった部位
	};

	// 部品の読み込みを頼むだけ（非同期）。meshes->update() が true を返してから place を呼ぶ。
	// meshes: 設計図を借りる倉庫。所有権は受け取らない。nullptr 不可。この世界より長く生きていること。
	explicit GameWorld( MeshCache* meshes );
	~GameWorld();
	GameWorld( const GameWorld& ) = delete;
	GameWorld& operator=( const GameWorld& ) = delete;

	// 全データの構築後に一度だけ、物を作って並べ、名簿へ載せる。
	// 構築に失敗したデータ（理由は MeshCache がデバッグ出力済み）の物は置かない（その物が無いまま、他の確認はできる）。
	void place();
	// 1ティック進める: 自キャラを操作どおりに動かし、撃ったなら弾を出して銃口を光らせ、弾を飛ばし、物に当たった所に弾痕と砂ぼこりを出し、
	// ゾンビに当たったらダメージを渡して血しぶきを出し、部位が千切れたら飛ばし、ゾンビに自キャラを追わせてアニメを進め、
	// 沈み終わった死体・千切れた部位と、消す印の付いた物を消す。
	// command / cameraYaw / cameraPitch: PlayerCharacter::update へそのまま渡す。
	// viewDirection: 視線の向き（長さ1。FirstPersonCamera::viewDirection）。弾は自キャラの目からこの向きへ飛ぶ（画面の中央へ飛ぶ）。
	void update( const PlayerCommand& command, float cameraYaw, float cameraPitch, const GameLib::Math::Vector3& viewDirection );
	// 町・キャラ・物を描くだけ（何回呼んでも同じ絵になる）。
	// frustum: カメラに映る範囲。地面・建物・置物は、映らない物を描かない（キャラは数が少ないので、いつも描く）。
	void draw( const ViewFrustum& frustum ) const;
	// 弾痕を描く。draw の後、同じ描画先で呼ぶ（壁の奥行きで隠すため）。1フレームに1回まで。
	// projectionView: カメラの射影×ビュー行列（FirstPersonCamera::projectionViewMatrix）。
	void drawBulletHoles( const GameLib::Math::Matrix44& projectionView );
	// 着弾の砂ぼこりを描く。呼ぶ場所の決まりは drawBulletHoles と同じ（弾痕の後に呼ぶ）。
	// viewDirection: 視線の向き（長さ1。FirstPersonCamera::viewDirection）。粒の板を画面に平行にし、カメラ側から照らすため。
	void drawImpactDust( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection );
	// ゾンビに当たった血しぶきを描く。呼ぶ場所の決まり・引数は drawImpactDust と同じ（砂ぼこりの後に呼ぶ）。
	void drawBloodSpray( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection );
	// 撃った瞬間に銃口が光る板を描く。呼ぶ場所の決まりは drawBulletHoles と同じ（砂ぼこりの後に呼ぶ）。
	// 銃口の位置は、描くたびに自キャラの今の銃から取り直す。引数は drawImpactDust と同じ。
	void drawMuzzleFlash( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection );
	// 撃った瞬間に銃口の前に灯す点光源。光っていない・自キャラが銃を構えていなければ false（out は変えない）。
	// シーンがライトを設定するときに読む。out: nullptr 不可。
	bool muzzleLight( MuzzleFlash::Light* out ) const;

	// 歩いて追ってくるゾンビを1体出す（place で町に置くのと同じ決まり。倒れ方は体ごとにランダム）。
	// update の外（ティックの間）で呼ぶ（ゾンビの名簿をたどっている最中に増やさない）。
	// まだ place していない・ゾンビの形の構築に失敗した・数が上限に達しているなら出さずに false。
	// position / yawDegree: 足元の位置と向き（度。0 は +Z）。
	bool spawnZombie( const GameLib::Math::Vector3& position, float yawDegree );

	// 拾われた物に、消す印を付ける（実際に消えるのは次の update の最後）。物を使った後、1回の描画につき1回呼ぶ。
	void removeTakenPickups();
	// 視線が当たった物のうち、使える物（落ちている物・何度でも拾える置き場・車）。使える物でなければ nullptr（Interactor の resolver に使う）。
	IInteractable* resolveInteractable( const HitWorld::Hit& hit );

	const TownMap& map() const;							// 町のどこに何を置くか
	const TownGround& ground() const;					// 道路と歩道（足元の材質を聞ける）
	const CollisionWorld& collisionWorld() const;		// 移動の当たり判定の名簿
	const HitWorld& hitWorld() const;					// 命中の当たり判定の名簿
	// 自キャラ。place の前・構築に失敗して置けなかったときは nullptr。返すポインタはこの世界がある間使える。
	PlayerCharacter* player();
	const PlayerCharacter* player() const;
	// 脱出用の車。place の前・構築に失敗して置けなかったときは nullptr。
	const EscapeCar* escapeCar() const;
	// 車にガソリンが決まった数だけ入ったか（車が無ければ false）。
	bool isEscapeReady() const;
	// 落ちている物（まだ拾われていないガソリン）の移動の当たり判定の箱を、全部 out の後ろへ足す。
	// 確認用の表示が、物の場所を壁越しに見せるのに使う。out: 出力先。nullptr 不可。
	void appendPickupColliders( std::vector< BoxCollider >* out ) const;
	// ゾンビ全員（倒れている途中・死体も含む）。確認用の表示が体力を読むのに使う。
	const ActorList< Zombie >& zombies() const;
	// 最後の update でゾンビに当たった出来事（当たった順）。次の update で入れ替わる。
	const std::vector< ZombieHit >& zombieHitsThisTick() const;

private:
	// 弾がキャラ（impact.hit.target）に当たったとき、ゾンビならダメージを渡し、血しぶきを出し、出来事を覚える。
	// 部位が切れたら飛ばす。倒れ始めたら移動の名簿から外す（update から）。
	void hitZombie( const BulletList::Impact& impact );
	// zombie の part が今切れたので、切り口に血しぶきを出し、千切れた部位を飛ばす（hitZombie から）。
	// direction: 弾の進む向き（長さ1）。
	void severLimb( const Zombie& zombie, BodyPart part, const GameLib::Math::Vector3& direction );

	// ゾンビを作って名簿へ載せる（spawnZombie と、place の確認用の T ポーズの1体から）。mZombieModel があること。
	void addZombie( const GameLib::Math::Vector3& position, float yawDegree, const Zombie::Animations& animations );

	// data の建物を placements の場所へ建てる（place から。名簿へ載せるのは、全部建て終えた後）。data: nullptr 不可。
	void placeBuildings( const StaticMeshData* data, const std::vector< TownPlacement >& placements );
	void placeMissionObjects(); // 脱出用の車と、何度でも拾えるガソリン、候補点から選んだ場所のガソリンを置く（place から）

	// 町のどこに何を置くか。
	TownMap mMap;
	const SkinnedMeshData* mPlayerData;   // どれも MeshCache から借りる
	const StaticMeshData* mWeaponData;    // 自キャラが右手に持つ銃
	const StaticMeshData* mBulletData;    // 飛ぶ弾（7.62 NATO の弾頭）
	const SkinnedMeshData* mZombieData;
	const StaticMeshData* mBuildingData;
	const StaticMeshData* mPerimeterBuildingData; // 町を囲む、足元の板の無い建物
	const StaticMeshData* mVehicleData;   // 脱出用の車
	const StaticMeshData* mFuelCanData;   // ガソリンの携行缶
	TownGround mGround;                   // 道路と歩道（見た目・弾の当たり判定・足元の材質）

	// 当たり判定の名簿。載る物を借りて持つので、載る物より先に宣言する（＝載る物より後に破棄される）。
	CollisionWorld mCollisionWorld;
	HitWorld mHitWorld;
	std::unique_ptr< PlayerCharacter > mPlayer;
	// 飛んでいる弾。名簿には載らない（名簿へ当たりを聞くだけ）。
	BulletList mBullets;
	// 弾が物に当たった跡。付いた物は、消すときの目印として覚えるだけ（中身は読まない）。
	BulletHoleList mBulletHoles;
	// 弾が物に当たった瞬間の砂ぼこり（遠くの着弾を見えるようにする）。
	ImpactDustList mImpactDust;
	// 弾がゾンビに当たった瞬間の血しぶき。
	BloodSprayList mBloodSpray;
	// このティックにゾンビへ当たった出来事（update のたびに作り直す。毎回確保し直さないために持つ）。
	std::vector< ZombieHit > mZombieHits;
	// 弾が当たった所（update の作業領域。毎回確保し直さないために持つ）。
	std::vector< BulletList::Impact > mImpacts;
	// 撃った瞬間に銃口が光る演出（自キャラの銃の）。
	MuzzleFlash mMuzzleFlash;
	// ゾンビ全員で共有する決まりごと（当たり判定の設計図）。ゾンビが借りるので、ゾンビより先に宣言する。置けなかったときは空。
	std::unique_ptr< ZombieModel > mZombieModel;
	// ゾンビ。途中で消せるよう ActorList で持つ（名簿への登録・解除もここが行う）。
	ActorList< Zombie > mZombies;
	GameLib::Math::Random mZombieRandom; // 倒れ方（死亡アニメ）を選ぶ（place で種を決める）
	int mZombieNumber;                   // これまでに出したゾンビの数（歩き出しの時刻・見通しを調べる時刻をずらす番号）
	// ゾンビから千切れた部位（どの名簿にも載らない）。ゾンビと同じ形を借りるので、mZombieData が生きている間だけ使う。
	SeveredLimbList mSeveredLimbs;
	// 切れた瞬間の骨の姿勢（severLimb の作業領域。毎回確保し直さないために持つ）。
	std::vector< GameLib::Math::Matrix34 > mSeverPose;
	// 名簿が要素を指すので、並べ終えた後は増やさない（増やすと並べ直しで要素の場所が変わる）。
	std::vector< Building > mBuildings;
	ActorList< Pickup > mPickups;            // 落ちているガソリン（拾うと消える）
	std::unique_ptr< EscapeCar > mEscapeCar; // 名簿が自分の箱を指すので、置いた後は動かさない（unique_ptr で持つ）
	std::unique_ptr< ItemSource > mEndlessFuel; // 車の近くの、何度でも拾えるガソリン（同上）
};

#endif
