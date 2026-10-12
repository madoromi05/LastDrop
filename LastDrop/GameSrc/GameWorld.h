#ifndef INCLUDED_GAME_WORLD_H
#define INCLUDED_GAME_WORLD_H

#include "TownMap.h"
#include "TownGround.h"
#include "CollisionWorld.h"
#include "HitWorld.h"
#include "Zombie.h"
#include "ZombieModel.h"
#include "ActorList.h"
#include "Building.h"
#include "Pickup.h"
#include "EscapeCar.h"
#include "ItemSource.h"
#include "PlayerCommand.h"
#include "BulletList.h"
#include "GameLib/Math/Vector3.h"
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
	// 部品の読み込みを頼むだけ（非同期）。meshes->update() が true を返してから place を呼ぶ。
	// meshes: 設計図を借りる倉庫。所有権は受け取らない。nullptr 不可。この世界より長く生きていること。
	explicit GameWorld( MeshCache* meshes );
	~GameWorld();
	GameWorld( const GameWorld& ) = delete;
	GameWorld& operator=( const GameWorld& ) = delete;

	// 全データの構築後に一度だけ、物を作って並べ、名簿へ載せる。
	// 構築に失敗したデータ（理由は MeshCache がデバッグ出力済み）の物は置かない（その物が無いまま、他の確認はできる）。
	void place();
	// 1ティック進める: 自キャラを操作どおりに動かし、撃ったなら弾を出し、弾を飛ばし、ゾンビのアニメを進め、消す印の付いた物を消す。
	// command / cameraYaw / cameraPitch: PlayerCharacter::update へそのまま渡す。
	// viewDirection: 視線の向き（長さ1。FirstPersonCamera::viewDirection）。弾は自キャラの目からこの向きへ飛ぶ（画面の中央へ飛ぶ）。
	void update( const PlayerCommand& command, float cameraYaw, float cameraPitch, const GameLib::Math::Vector3& viewDirection );
	// 町・キャラ・物を描くだけ（何回呼んでも同じ絵になる）。
	// frustum: カメラに映る範囲。地面・建物・置物は、映らない物を描かない（キャラは数が少ないので、いつも描く）。
	void draw( const ViewFrustum& frustum ) const;

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

private:
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
	// ゾンビ全員で共有する決まりごと（当たり判定の設計図）。ゾンビが借りるので、ゾンビより先に宣言する。置けなかったときは空。
	std::unique_ptr< ZombieModel > mZombieModel;
	// ゾンビ。途中で消せるよう ActorList で持つ（名簿への登録・解除もここが行う）。
	ActorList< Zombie > mZombies;
	// 名簿が要素を指すので、並べ終えた後は増やさない（増やすと並べ直しで要素の場所が変わる）。
	std::vector< Building > mBuildings;
	ActorList< Pickup > mPickups;            // 落ちているガソリン（拾うと消える）
	std::unique_ptr< EscapeCar > mEscapeCar; // 名簿が自分の箱を指すので、置いた後は動かさない（unique_ptr で持つ）
	std::unique_ptr< ItemSource > mEndlessFuel; // 車の近くの、何度でも拾えるガソリン（同上）
};

#endif
