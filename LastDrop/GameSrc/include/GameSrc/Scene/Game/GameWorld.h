#ifndef INCLUDED_GAME_WORLD_H
#define INCLUDED_GAME_WORLD_H

#include "GameSrc/Map/TownMap.h"
#include "GameSrc/Map/TownGround.h"
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Zombie/ZombieModel.h"
#include "GameSrc/Zombie/ZombieBrainComponent.h"
#include "GameSrc/Zombie/ZombieFactory.h"
#include "GameSrc/Zombie/SeveredLimbList.h"
#include "GameSrc/Zombie/ZombieSpawner.h"
#include "GameSrc/Zombie/ZombieCrowd.h"
#include "GameSrc/Zombie/ChaseTarget.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/World/GameObjectHandle.h"
#include "GameSrc/Map/Building.h"
#include "GameSrc/Component/BoxColliderComponent.h"
#include "GameSrc/Render/DrawCulling.h"
#include "GameSrc/Player/PlayerCommand.h"
#include "GameSrc/Player/PlayerFactory.h"
#include "GameSrc/Weapon/BulletList.h"
#include "GameSrc/Weapon/BulletHoleList.h"
#include "GameSrc/Effect/ImpactDustList.h"
#include "GameSrc/Effect/BloodSprayList.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Random.h"
#include <memory>
#include <utility>
#include <vector>

class MeshCache;
class StaticMeshData;
class SkinnedMeshData;
class IInteractable;
class Inventory;
class PlayerComponent;
class FuelTankComponent;
class ZombieStateComponent;
class SkinnedMeshInstance;
struct LightSource;

// 本編の世界。町（道路・歩道・建物）と、世界に置く物（GameObject。自キャラ・銃・ゾンビ・落ちているガソリンと弾・脱出用の車・
// ガソリンの置き場）の持ち主、飛んでいる弾・エフェクトの並び、それらの当たり判定の名簿（移動の CollisionWorld・命中の HitWorld）の持ち主。
// 「何をどこに置くか」は TownMap が決め、ここは物を部品から組み立てて置き（SummaryPlan/個別仕様/ComponentSystem2_Plan.txt。
// 自キャラと銃は PlayerFactory、ゾンビは ZombieFactory が組み立てる）、
// 1ティック進め、描く。名簿へ載せる・外すのは物の部品（IWorldMember）が GameObjectWorld の spawn・flush の中で行う。
// 建物・地面・飛ぶ弾・エフェクトは GameObject にしない（数が多く、まとめて描く・詰めて持つ方が速いため。★1）。
//
// 舞台は碁盤の目の町。町の周りは建物で囲んであり、外へは出られない。
// 建物とゾンビと車は移動を止め、自キャラは当たると沿って滑る。道路と歩道は移動を止めないが、弾・視線は止める（TownGround）。
// ゲームループ: 候補点から選んだ場所にガソリンが出る。拾って車へ運び、決まった数を入れると車が走れるようになる（isEscapeReady）。
// 倒したゾンビは、決まった割合で足元に予備の弾を落とす（拾うのはガソリンと同じく E。上を歩ける）。
// その後どうするか（クリア画面へ進む）は知らない。入力・カメラ・音・HUD も知らない（シーンが、この世界を読んで動かす）。
class GameWorld{
public:
	// ゾンビに弾が当たった出来事（ヒット音を鳴らすなど、世界の外の係が読む）。死体に当たったときも入る。
	struct ZombieHit{
		GameLib::Math::Vector3 point; // 当たった位置（ワールド座標）
		BodyPart part;                // 当たった部位
	};
	// 弾が物（建物・車・地面・拾える物）に当たった出来事（着弾音を鳴らす係が読む）。
	struct SurfaceHit{
		GameLib::Math::Vector3 point; // 当たった位置（ワールド座標）
		GroundMaterial material;      // 当たった物の、その位置の表面の材質
	};
	// ゾンビの腕が自キャラに当たった出来事（音・画面の赤み・揺れの係が読む）。自キャラの体力はもう減らしてある。
	struct ZombieAttackEvent{
		GameLib::Math::Vector3 point; // ゾンビの頭の位置（ワールド座標。音を鳴らす所）
	};

	// 部品の読み込みを頼むだけ（非同期）。meshes->update() が true を返してから place を呼ぶ。
	// meshes: 設計図を借りる倉庫。所有権は受け取らない。nullptr 不可。この世界より長く生きていること。
	explicit GameWorld( MeshCache* meshes );
	~GameWorld();
	GameWorld( const GameWorld& ) = delete;
	GameWorld& operator=( const GameWorld& ) = delete;

	// 全データの構築後に一度だけ、物を組み立てて並べ、名簿へ載せる。
	// 構築に失敗したデータ（理由は MeshCache がデバッグ出力済み）の物は置かない（その物が無いまま、他の確認はできる）。
	void place();
	// 1ティック進める。段の順（UpdatePhase）：
	//   Input（自キャラが操作どおりに動く・撃つ）→ 撃ったなら弾を出し、近くのゾンビに銃声を聞かせる → 弾を飛ばし、物に当たった所に弾痕と砂ぼこり、
	//   ゾンビに当たったらダメージを渡して血しぶき・千切れた部位 → Think（ゾンビが追う・攻撃する。腕が当たったら自キャラの体力を減らす）→
	//   Late（銃口が光る）→ 沈み終わった死体と、消す印の付いた物を消す（flush）→ 足りなければゾンビを見えない所へ湧かせる（ZombieSpawner）。
	// command / cameraYaw / cameraPitch / inventory: PlayerComponent::setInput へそのまま渡す（cameraYaw は湧かせてよい「画面の外」を決めるのにも使う）。
	// viewDirection: 弾を出す向き（長さ1。FirstPersonCamera::weaponDirection）。弾は自キャラの目からこの向きへ飛ぶ
	//                （画面の中央から、照準のぶれの分だけずれた所＝銃が向いている所へ飛ぶ）。
	void update( const PlayerCommand& command, float cameraYaw, float cameraPitch, const GameLib::Math::Vector3& viewDirection,
		Inventory* inventory );
	// 町・キャラ・物を描くだけ（何回呼んでも同じ絵になる）。物ごとの GPU の時間を印（Profiling::gpuMark）で分ける。
	// 動かない物は手前に来やすい物から描く：建物（目に近い順）→ 車・置物 → 地面。先に描いた物に隠れる画素は、GPU が色の計算の前に捨てる
	// （Early-Z。Optimization_Plan.txt 2-3）。自キャラ（腕と銃）を一番先に描いていたのは同じ理由だが、デカールのために後へ回した
	// （G-Buffer のパスは 1 画素が軽いので、重さは段階 7 の計測で確かめる）。
	// 動かない物（建物・車・置物・地面）と動く物（自キャラ・弾・ゾンビ・千切れた部位）に分けて描く。間にデカール（弾痕）を描き、
	// 後から描く動く物がデカールを上書きして、キャラに弾痕が乗らないようにする（SsrDecalMaterial_Plan.txt Q7）。
	// culling: 描くかどうか（DrawCulling::forCamera）。地面は映る範囲だけ、建物・置物は描く距離・小ささでも間引く（OptimizationA_Plan.txt 6 章）。
	// eye: 目の位置（FirstPersonCamera::eyePosition）。建物を近い順に並べるのに使う。
	void drawStatic( const DrawCulling& culling, const GameLib::Math::Vector3& eye ) const;
	// 動く物を描く。drawStatic とデカールの後に、この順に呼ぶ。TAA のために動きの種類を分けて描く（Taa_Plan.txt 6 章。
	// 呼ぶ側が間で GBuffer::setMotion を切り替える）。drawFirstPerson：手元（自キャラ・持っている銃・弾。いつも映るので間引かない）、
	// drawCharacters：ゾンビ・千切れた部位（culling は drawStatic と同じ物）。
	void drawFirstPerson() const;
	void drawCharacters( const DrawCulling& culling ) const;
	// 影を落とす物を描く（ShadowMap::begin〜end の間。Shadow_Plan.txt 8 章）。ゾンビ・千切れた部位・建物・車・置物。
	// 地面（受けるだけ）と弾頭（小さく速い）は描かない。culling：光の側から見た映る範囲と、月の影なら小ささ（全部の物を間引く）。
	// includesPlayer：自キャラ（体・腕・銃）も描くか（懐中電灯の影には描かない。銃の影が画面の真ん中を埋めるため。Q4）。
	void drawShadowCasters( const DrawCulling& culling, bool includesPlayer ) const;
	// 弾痕を G-Buffer へデカールとして描く。drawStatic の後・drawFirstPerson の前に、GBuffer::beginDecals〜endDecals の間で呼ぶ。1フレームに1回まで。
	// projectionView: カメラの射影×ビュー行列（FirstPersonCamera::projectionViewMatrix）。
	void drawBulletHoles( const GameLib::Math::Matrix44& projectionView );
	// 着弾の砂ぼこりを描く。呼ぶ場所の決まりは drawBulletHoles と同じ（弾痕の後に呼ぶ）。
	// viewDirection: 視線の向き（長さ1。FirstPersonCamera::viewDirection）。粒の板を画面に平行にし、カメラ側から照らすため。
	// eye: 目の位置（FirstPersonCamera::eyePosition）。粒を目から遠い順に並べ替えて描くため。
	void drawImpactDust( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection,
		const GameLib::Math::Vector3& eye );
	// ゾンビに当たった血しぶきを描く。呼ぶ場所の決まり・引数は drawImpactDust と同じ（砂ぼこりの後に呼ぶ）。
	void drawBloodSpray( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection,
		const GameLib::Math::Vector3& eye );
	// 撃った瞬間に銃口が光る板を描く。呼ぶ場所の決まりは drawBulletHoles と同じ（砂ぼこりの後に呼ぶ）。
	// 銃口の位置は、描くたびに自キャラの今の銃から取り直す。引数は drawImpactDust と同じ。
	void drawMuzzleFlash( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection );
	// 撃った瞬間に銃口の前に灯す点光源。光っていない・自キャラが銃を構えていなければ false（out は変えない）。
	// シーンがライトの一覧（LightList）を作るときに読む。out: nullptr 不可。
	bool muzzleLight( LightSource* out ) const;

	// 歩いて追ってくるゾンビを1体出す（倒れ方は体ごとにランダム）。
	// update の外（ティックの間）で呼ぶ（物の並びをたどっている最中に増やさない）。
	// まだ place していない・ゾンビの形の構築に失敗した・数が上限に達しているなら出さずに false。
	// position / yawDegree: 足元の位置と向き（度。0 は +Z）。
	bool spawnZombie( const GameLib::Math::Vector3& position, float yawDegree );

	// 拾われた物に、消す印を付ける（実際に消えるのは次の update の最後）。物を使った後、1回の描画につき1回呼ぶ。
	void removeTakenPickups();
	// origin から delta だけ進む視線の先の、一番手前の使える物（落ちている物・死体が落とした弾・何度でも拾える置き場・車）。
	// 視線はキャラ（自キャラを除く）にも当たる。死体に当たれば、その死体が落としてまだ残っている弾を返す。
	// 落ちている物は一回り大きい見る箱（PickupComponent::lookRaycast）でも当たる。壁などの向こうの物は返さない。
	// 使える物が無ければ nullptr（Interactor の finder に使う）。
	IInteractable* findInteractable( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta );

	const TownMap& map() const;							// 町のどこに何を置くか
	const NavMesh& navMesh() const;					// 町の歩ける床の形（確認用の表示が、床の縁を描くのに使う）
	const TownGround& ground() const;					// 道路と歩道（足元の材質を聞ける）
	const CollisionWorld& collisionWorld() const;		// 移動の当たり判定の名簿
	const HitWorld& hitWorld() const;					// 命中の当たり判定の名簿
	// 自キャラの物（部品は PlayerComponent の説明）。place の前・構築に失敗して置けなかったときは nullptr。
	// 返すポインタはこの世界がある間使える（自キャラは消えない）。
	GameObject* player();
	const GameObject* player() const;
	// 自キャラの、命中の名簿に載っている部位の箱・移動の名簿に載っている長方形（自分の撃った弾・視線から除く、確認用の表示で隠すのに使う）。
	// 自キャラが居なければ nullptr。
	const IHittable* playerTarget() const;
	const ICollisionBody* playerBody() const;
	// 脱出用の車の燃料タンク。place の前・構築に失敗して置けなかったときは nullptr。
	const FuelTankComponent* fuelTank() const;
	// 車にガソリンが決まった数だけ入ったか（車が無ければ false）。
	bool isEscapeReady() const;
	// 落ちている物（まだ拾われていないガソリン）の移動の当たり判定の箱を、全部 out の後ろへ足す。
	// 確認用の表示が、物の場所を壁越しに見せるのに使う。out: 出力先。nullptr 不可。
	void appendPickupColliders( std::vector< BoxCollider >* out ) const;
	// 世界に置いた物（自キャラ・銃・ゾンビ・落ちている物・車…）。確認用の表示が forEach< ZombieStateComponent > で
	// ゾンビ全員（倒れている途中・死体も含む）の体力・通り道を読む、F6 のエディタが物の一覧を出すのに使う。
	const GameObjectWorld& objects() const;
	// 最後の update でゾンビに当たった出来事（当たった順）。次の update で入れ替わる。
	const std::vector< ZombieHit >& zombieHitsThisTick() const;
	// 最後の update で弾が物に当たった出来事（当たった順）。次の update で入れ替わる。
	const std::vector< SurfaceHit >& surfaceHitsThisTick() const;
	// 最後の update でゾンビの腕が自キャラに当たった出来事（起きた順）。次の update で入れ替わる。
	const std::vector< ZombieAttackEvent >& zombieAttacksThisTick() const;
	// 自キャラが死んだか（自キャラが居なければ false）。
	bool isPlayerDead() const;
	// ゾンビを湧かせる係（確認用の表示が、立っている数・目標の数・次に湧くまでを読む）。ゾンビの形が無ければ nullptr。
	const ZombieSpawner* zombieSpawner() const;

private:
	// 物を組み立てて置く関数は、1 つの関数で 1 つの物だけを作る（中の変数は画像と同じ obj。SummaryPlan/個別仕様/ComponentSystem3_Plan.txt の ★6）。
	// 自キャラ（と手に持つ銃）を町の出発点へ置く（place から。組み立ては mPlayerFactory）。
	void placePlayer();
	// 弾がキャラ（impact.hit.target）に当たったとき、ゾンビならダメージを渡し、血しぶきを出し、出来事を覚える。
	// 部位が切れたら飛ばす（update から）。
	void hitZombie( const BulletList::Impact& impact );
	// zombieMesh（ゾンビの見た目）の part が今切れたので、切り口に血しぶきを出し、千切れた部位を飛ばす（hitZombie から）。
	// direction: 弾の進む向き（長さ1）。
	void severLimb( const SkinnedMeshInstance& zombieMesh, BodyPart part, const GameLib::Math::Vector3& direction );
	// zombie が今倒れたので、決まった割合で足元（自キャラ側へ少し寄せた所）に予備の弾を落とす（hitZombie から）。
	// 落ちている弾（タグ AmmoDrop）が多すぎたら、一番古い物を消す。数の上限があるのは弾だけ（ガソリンは数が決まっている）。
	void dropAmmo( const ZombieStateComponent& zombie );
	// 視線が当たった物（findInteractable が一番手前を選ぶ前の 1 つ）が使える物ならそれを返す。死体なら、その死体が落とした弾。
	IInteractable* resolveInteractable( const HitWorld::Hit& hit );
	// culling で描くと決まったゾンビだけ描く（drawCharacters・drawShadowCasters から）。
	void drawZombies( const DrawCulling& culling ) const;
	// 自キャラの体と、手に持って出している銃を描く（drawFirstPerson・drawShadowCasters から）。
	void drawPlayer() const;

	// position（撃った瞬間の自キャラの足元）で銃声がした。届く距離の内のゾンビへ知らせる（update で撃ったティックに）。
	void alertZombiesToGunshot( const GameLib::Math::Vector3& position );
	// ゾンビの Think の段を動かし、腕が当たったら自キャラの体力を減らし、沈み終わった死体に消す印を付ける（update から）。
	// 攻撃に行けるのは一度に 1 体だけ（ZombieBrainComponent::TickContext::attacker）。
	void updateZombies();
	// 立っているゾンビの数・給油の数・カメラの向きを ZombieSpawner に渡し、出すと言われた所へ spawnZombie する（update の最後から）。
	// cameraYaw: カメラの水平の向き（度）。
	void spawnZombiesFromSpawner( float cameraYaw );
	// ゾンビを組み立てて置く（spawnZombie と、place の確認用の T ポーズの1体から）。mZombieFactory があること。
	void addZombie( const GameLib::Math::Vector3& position, float yawDegree, const ZombieAnimationNames& animations );

	// data の建物を placements の場所へ建てる（place から。名簿へ載せるのは、全部建て終えた後）。data: nullptr 不可。
	void placeBuildings( const StaticMeshData* data, const std::vector< TownPlacement >& placements );
	void placeMissionObjects(); // 脱出用の車と、何度でも拾えるガソリン、候補点から選んだ場所のガソリンを置く（place から）
	void placeEscapeCar();      // 脱出用の車（燃料タンク付き）を置く（placeMissionObjects から。車の形があること）
	void placeFuelStation();    // 何度でも拾えるガソリンの置き場を置く（同上。缶の形があること）
	void placeFuel( const TownPlacement& placement ); // 拾うと消えるガソリンを 1 つ置く（同上）
	// 動かない置物の物を作り、見た目と箱の部品まで付けて返す（まだ spawn しない。呼ぶ側が残りの部品を付けて spawn する）。
	GameObject* createProp( GameObjectTag tag, const char* name, const StaticMeshData* data, const GameLib::Math::Vector3& position,
		float yawDegree, DrawCulling::Target cullTarget, BoxColliderComponent::Mode colliderMode );

	// 町のどこに何を置くか。
	TownMap mMap;
	// 町の歩ける床の形（ゾンビが建物を回り込む通り道を聞く）。place で建物・車を置き終えてから作る。
	// ゾンビが借りるのでゾンビより先に宣言する（作った後は当たり判定の名簿を借りないので、名簿との順は問わない）。
	NavMesh mNavMesh;
	// 自キャラと銃を部品から組み立てる（形は MeshCache から借りる）。作った自キャラより長く生きるよう mObjects より先に宣言する。
	PlayerFactory mPlayerFactory;
	const StaticMeshData* mBulletData;    // どれも MeshCache から借りる。飛ぶ弾（7.62 NATO の弾頭）
	const SkinnedMeshData* mZombieData;
	const StaticMeshData* mBuildingData;
	const StaticMeshData* mPerimeterBuildingData; // 町を囲む、足元の板の無い建物
	const StaticMeshData* mVehicleData;   // 脱出用の車
	const StaticMeshData* mFuelCanData;   // ガソリンの携行缶
	const StaticMeshData* mAmmoDropData;  // ゾンビが落とす予備の弾（7.62 NATO の弾薬を大きくして寝かせた物）
	TownGround mGround;                   // 道路と歩道（見た目・弾の当たり判定・足元の材質）

	// 当たり判定の名簿。載る物を借りて持つので、載る物より先に宣言する（＝載る物より後に破棄される）。
	CollisionWorld mCollisionWorld;
	HitWorld mHitWorld;
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
	// このティックに弾が物へ当たった出来事（同上）。
	std::vector< SurfaceHit > mSurfaceHits;
	// このティックにゾンビの腕が自キャラに当たった出来事（同上）。
	std::vector< ZombieAttackEvent > mZombieAttacks;
	// 弾が当たった所（update の作業領域。毎回確保し直さないために持つ）。
	std::vector< BulletList::Impact > mImpacts;
	// ゾンビ全員で共有する決まりごと（当たり判定の設計図）。ゾンビが借りるので、ゾンビより先に宣言する。置けなかったときは空。
	std::unique_ptr< ZombieModel > mZombieModel;
	// ゾンビの Think の段で全員が読む、このティックの周り（追う先・仲間の位置・攻撃に行っている体）。ゾンビが借りるので、ゾンビより先に宣言する。
	ZombieBrainComponent::TickContext mZombieContext;
	ChaseTarget mChaseTarget; // mZombieContext.target が指す（自キャラが居る間）
	// 立って歩けるゾンビの位置の並び（updateZombies の頭で作り直し、mZombieContext から全員が読む。毎回確保し直さないために持つ）。
	ZombieCrowd mCrowd;
	// ゾンビを部品から組み立てる。mZombieModel を借りる（mNavMesh・名簿・mZombieContext は部品が mObjects の共有の棚から引く）。
	std::unique_ptr< ZombieFactory > mZombieFactory;
	// 世界に置く物（自キャラ・銃・ゾンビ・落ちている物・車・置き場）。物（GameObject）に部品を付けて持つ
	// （名簿への登録・解除もここが行う。SummaryPlan/個別仕様/ComponentSystem2_Plan.txt）。
	// 物が名簿・mZombieModel・mNavMesh・mZombieContext を借りるので、それらより後に宣言する（＝先に破棄される）。
	GameObjectWorld mObjects;
	GameObject* mPlayer;             // 自キャラの物（mObjects が持つ。置けなければ nullptr）
	PlayerComponent* mPlayerControl; // 自キャラの操作を読む部品（同上）
	const FuelTankComponent* mFuelTank; // 脱出用の車の燃料タンク（同上）
	// 死体と、その死体が落とした弾の組（2026-10-31 ユーザー指定：死体を見ても弾を拾える）。
	// ゾンビは落とした物を知らずに済むよう、組はここで持つ。どちらかが消えたら、その組は使わない（GameObjectHandle が nullptr を返す）。
	struct CorpseDrop{
		GameObjectHandle corpse; // 弾を落としたゾンビの物
		GameObjectHandle drop;   // 落とした弾の物
	};
	std::vector< CorpseDrop > mCorpseDrops;
	GameLib::Math::Random mZombieRandom; // 倒れ方（死亡アニメ）・弾を落とすかを選ぶ（place で種を決める）
	int mZombieNumber;                   // これまでに出したゾンビの数（歩き出しの時刻・見通しを調べる時刻をずらす番号）
	// いつ・どこにゾンビを湧かせるか。名簿（mHitWorld・mCollisionWorld）と mZombieModel の体の大きさを借りるので、それらより後に宣言する。
	std::unique_ptr< ZombieSpawner > mZombieSpawner;
	std::vector< ZombieSpawner::Order > mSpawnOrders; // このティックに出す所（毎回確保し直さないために持つ）
	// ゾンビから千切れた部位（どの名簿にも載らない）。ゾンビと同じ形を借りるので、mZombieData が生きている間だけ使う。
	SeveredLimbList mSeveredLimbs;
	// 切れた瞬間の骨の姿勢（severLimb の作業領域。毎回確保し直さないために持つ）。
	std::vector< GameLib::Math::Matrix34 > mSeverPose;
	// 名簿が要素を指すので、並べ終えた後は増やさない（増やすと並べ直しで要素の場所が変わる）。
	std::vector< Building > mBuildings;
	// 建物を目に近い順に描くための並び（目からの距離の 2 乗と建物）。draw の中でだけ書く作業領域で、
	// 絵の中身ではないので const の draw からも書けるようにしてある（毎フレーム確保し直さないために持つ）。
	mutable std::vector< std::pair< float, const Building* > > mBuildingDrawOrder;
};

#endif
