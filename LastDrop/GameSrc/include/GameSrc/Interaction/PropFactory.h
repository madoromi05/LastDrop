#ifndef INCLUDED_PROP_FACTORY_H
#define INCLUDED_PROP_FACTORY_H

#include "GameSrc/World/GameObjectTag.h"
#include "GameSrc/Interaction/Item/ItemGrid.h"
#include "GameSrc/Interaction/Loot/LootTable.h"
#include "GameSrc/Render/Culling/DrawCulling.h"
#include "GameSrc/Component/BoxColliderComponent.h"
#include "GameSrc/Render/StaticMesh/StaticMeshLods.h"
#include "GameSrc/Weapon/WeaponCatalog.h"
#include "GameSrc/Audio/GroundMaterial.h"
#include "GameSrc/Map/TownEdits.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Random.h"
#include <map>
#include <vector>

class MeshCache;
class StaticMeshData;
class GameObject;
class GameObjectWorld;

// 置物（置物の車・漁れるゴミ箱と木箱・落ちているガソリン・落ちた弾・落ちている包帯）を部品から組み立てる
// （Unity のプレハブの代わり。PlayerFactory・ZombieFactory と同じ役）
// 部品の並び・形のファイル・名前・材質・中身の量を 1 か所で決める（「何で出来ているか」）
// 銃だけは新しい物を作らない。捨てた銃は、銃の物そのものへ地面に置くための部品を付け、拾われたら外す
// どこに置くか・何個置くかは使う側（WorldBuilder：最初に置く物、GameWorld：カバンから捨てた物）が決める
// 形の構築に失敗していたら、どの spawn も置かずに nullptr を返す（理由は MeshCache がデバッグ出力済み
// 置けなくても他の確認はできる）
class PropFactory{
public:
	// 形の読み込みを頼むだけ（非同期）。meshes: 設計図を借りる倉庫。所有権は受け取らない。nullptr 不可
	// 作った物より長く生きていること（形を借りるため）
	explicit PropFactory( MeshCache* meshes );

	// どれも world へ spawn して、その物を返す。position / yawDegree: 足元の位置と向き（度。0 は +Z）。world: nullptr 不可
	// 漁れるゴミ箱・木箱（table は LootTableId::Dumpster か Crate）。動かない箱として名簿へ載る（ゾンビの道の図にも、回り込む角として入る）
	// random: 中身を決める乱数（nullptr 不可。覚えない）
	// guaranteed: 必ず入れる物（町のガソリン・ライフル。空なら無し）
	GameObject* spawnLootContainer( GameObjectWorld* world, LootTableId table, const GameLib::Math::Vector3& position, float yawDegree,
		GameLib::Math::Random* random, const std::vector< ItemStack >& guaranteed ) const;
	GameObject* spawnEscapeCar( GameObjectWorld* world, const GameLib::Math::Vector3& position, float yawDegree, int requiredFuel ) const;	// 今日の脱出の車（置物の車と同じ形・同じ動かない箱。ガソリンを入れられ、漁れない）。requiredFuel: 走るのに要るガソリンの数
	GameObject* spawnFireHydrant( GameObjectWorld* world, const GameLib::Math::Vector3& position, float yawDegree ) const;					// 消火栓（見た目の置物。動かない箱として名簿へ載る。使えない・漁れない）
	// 街灯の柱（動かない箱）。lit: ガラスが光る形か、消えた形か（停電の日は消えた形）
	// 照らす光も柱に付ける（LightComponent。柱の頭から照らす）
	GameObject* spawnStreetLamp( GameObjectWorld* world, const GameLib::Math::Vector3& position, float yawDegree, bool lit ) const;
	// world に置いた街灯の柱を全部、ガラスが光る形（lit）か消えた形に替える（F6 で今日の条件を切り替えたとき）。world: nullptr 不可
	// update の外で呼ぶ。形の構築に失敗していれば替えない
	void relightStreetLamps( GameObjectWorld* world, bool lit ) const;
	GameObject* spawnFuel( GameObjectWorld* world, const GameLib::Math::Vector3& position, float yawDegree ) const;							// 拾うと消えるガソリン（カバンから捨てたとき）。外せる箱で移動を止める
	// world の中の id の銃の物を地面へ置く（拾うとカバンへ入る。上を歩ける）。置けたら、その銃の物を返す
	// 銃の物へ、地面に置くための部品 5 つ（置き場所・寝かせた見た目・箱・アイテム・拾う）を付ける
	// 弾倉の中身・懐中電灯の付け消しは、同じ物の GunComponent・LightComponent が持ち続ける
	// 置けないときは nullptr：形の構築に失敗した・銃の物が無い・その銃がもう地面にある
	// （拾われて外す印が付いた部品が、まだ外れていない間も置けない。同じ種類の部品は 1 つまでのため）
	// update の外で呼ぶ（部品を付けるので、物の並びをたどっている最中は呼べない）
	GameObject* layWeaponOnGround( GameObjectWorld* world, WeaponId id, const GameLib::Math::Vector3& position, float yawDegree ) const;
	// 地面に置いた銃 weapon から、地面に置くための部品 5 つを外す印を付ける（外れるのは次の flush。物は残る）。weapon: nullptr 不可
	static void liftFromGround( GameObject* weapon );
	// カバンから捨てた物。落ちているガソリン・落ちた弾・落ちている銃・落ちている包帯になり、拾い直せる
	// stack: 捨てた物と数（弾・包帯は数をそのまま持つ。ガソリン・銃は 1 つ）。銃は layWeaponOnGround と同じ（新しい物は作らない）
	GameObject* spawnDroppedItem( GameObjectWorld* world, const ItemStack& stack, const GameLib::Math::Vector3& position, float yawDegree ) const;
	// F6 の Town タブで足した町の置物を、出撃を読み直す前に見せる形（ゴミ箱・木箱・消火栓・街灯。ほかの種類は nullptr）
	// 町に置く物と同じ形と光。移動の名簿には載らない（上を通れる）。ゴミ箱・木箱は中身を持たず漁れない（中身は出撃を読み直すときに決める）
	// lampLit: 街灯のガラスが光る形か。update の外で呼ぶ
	GameObject* spawnEditPreview( GameObjectWorld* world, TownEditKind kind, const GameLib::Math::Vector3& position, float yawDegree, bool lampLit ) const;
	// F6 の Town タブで足した建物を、出撃を読み直す前に見せる物（GameObjectTag::BuildingPreview）
	// 建物の並び（TownEnvironment）には入れず、置物と同じ部品で置く（並びを増やすと、弾の名簿と隠れの表の番号が古い場所を指すため）
	// shapes / material: 町の建物と同じ形と壁の材質（WorldBuilder が持つ）。移動の名簿には載らない。update の外で呼ぶ。形の構築に失敗していたら nullptr
	GameObject* spawnBuildingPreview( GameObjectWorld* world, const StaticMeshLods& shapes, GroundMaterial material,
		const GameLib::Math::Vector3& position, float yawDegree ) const;

private:
	GameObject* spawnGroundItem( GameObjectWorld* world, const ItemStack& stack, const GameLib::Math::Vector3& position, float yawDegree ) const;	// 落ちている物（ItemSpec::groundFile の形で、上を歩ける）。弾・包帯。stack の数をそのまま持つ
	const StaticMeshLods& groundShapes( ItemId item ) const;																						// 物の種類 item の、落ちた形（ItemSpec::groundFile を持つ物だけ）
	const StaticMeshLods& streetLampShapes( bool lit ) const;																						// 街灯の形（lit: ガラスが光る形か、消えた形か）
	// 動かない置物の物を作り、見た目と箱の部品まで付けて返す（まだ spawn しない。呼ぶ側が残りの部品を付けて spawn する）
	// data: 構築済みであること。material: 表面の材質（着弾音）
	GameObject* createProp( GameObjectWorld* world, GameObjectTag tag, const char* name, const StaticMeshLods& shapes,
		const GameLib::Math::Vector3& position, float yawDegree, DrawCulling::Target cullTarget,
		BoxColliderComponent::Mode colliderMode, GroundMaterial material ) const;
	// obj へ、置き場所・見た目・箱の部品を、この順に付ける（createProp と、もう在る銃の物へ付ける layWeaponOnGround から）
	// spawn した後の物へ付けると、付けたときにすぐ awake・start するので、借りる相手を先に付ける順にしてある
	void addPropParts( GameObject* obj, const StaticMeshLods& shapes, const GameLib::Math::Vector3& position, float yawDegree,
		DrawCulling::Target cullTarget, BoxColliderComponent::Mode colliderMode, GroundMaterial material ) const;

	StaticMeshLods mVehicleShapes;						// どれも MeshCache から借りる。置物の車（細かさの段ごと）
	StaticMeshLods mFuelCanShapes;						// ガソリンの携行缶（細かさの段ごと）
	StaticMeshLods mFireHydrantShapes;					// 消火栓（細かさの段ごと）
	StaticMeshLods mStreetLampShapes;					// 街灯（ガラスが光る形。細かさの段ごと）
	StaticMeshLods mStreetLampOffShapes;				// 街灯（ガラスが消えた形。同上）
	StaticMeshLods mDumpsterShapes;						// ゴミ箱（1 段だけ。tools/modelgen/loot_prop_gen.py で作る）
	StaticMeshLods mCrateShapes;						// 木箱（同上）
	std::map< ItemId, StaticMeshLods > mGroundShapes;	// 落ちた形（物の種類ごと。形のファイルは ItemCatalog の groundFile。1 段だけ）。map なのは、groundFile の無い種類の分を持たないため
	std::vector< StaticMeshLods > mWeaponGroundShapes;	// 地面に置いた銃（WeaponId の順。横に寝かせた形。1 段だけ）
};

#endif
