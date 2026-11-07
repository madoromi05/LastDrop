#ifndef INCLUDED_PROP_FACTORY_H
#define INCLUDED_PROP_FACTORY_H

#include "GameSrc/World/GameObjectTag.h"
#include "GameSrc/Interaction/ItemGrid.h"
#include "GameSrc/Interaction/LootTable.h"
#include "GameSrc/Render/DrawCulling.h"
#include "GameSrc/Component/BoxColliderComponent.h"
#include "GameSrc/Render/StaticMesh/StaticMeshLods.h"
#include "GameSrc/Weapon/WeaponCatalog.h"
#include "GameSrc/Audio/GroundMaterial.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Random.h"
#include <map>
#include <vector>

class MeshCache;
class StaticMeshData;
class GameObject;
class GameObjectWorld;

// 置物（置物の車・漁れるゴミ箱と木箱・落ちているガソリン・落ちた弾・落ちている銃・落ちている包帯と売れる物）を部品から組み立てる
// （Unity のプレハブの代わり。PlayerFactory・ZombieFactory と同じ役。SummaryPlan/個別仕様/GameWorldSplit_Plan.txt の ★5）。
// 部品の並び・形のファイル・名前・材質・中身の量を 1 か所で決める（「何で出来ているか」）。
// どこに置くか・何個置くかは使う側（WorldBuilder：最初に置く物、GameWorld：カバンから捨てた物）が決める。
// 形の構築に失敗していたら、どの spawn も置かずに nullptr を返す（理由は MeshCache がデバッグ出力済み。
// 置けなくても他の確認はできる）。
class PropFactory{
public:
	// 形の読み込みを頼むだけ（非同期）。meshes: 設計図を借りる倉庫。所有権は受け取らない。nullptr 不可。
	// 作った物より長く生きていること（形を借りるため）。
	explicit PropFactory( MeshCache* meshes );

	// どれも world へ spawn して、その物を返す。position / yawDegree: 足元の位置と向き（度。0 は +Z）。world: nullptr 不可。
	// 漁れるゴミ箱・木箱（table は LootTableId::Dumpster か Crate）。動かない箱として名簿へ載る（ゾンビの道の図にも、回り込む角として入る）。
	// random: 中身を決める乱数（nullptr 不可。覚えない）。
	// withFuel: ガソリンを必ず 1 個入れる（町のガソリン。FuelEscapeLoading_Plan.txt Q15・改訂 2）。
	GameObject* spawnLootContainer( GameObjectWorld* world, LootTableId table, const GameLib::Math::Vector3& position, float yawDegree,
		GameLib::Math::Random* random, bool withFuel ) const;
	// 今日の脱出の車（置物の車と同じ形・同じ動かない箱。ガソリンを入れられ、漁れない）。requiredFuel: 走るのに要るガソリンの数。
	GameObject* spawnEscapeCar( GameObjectWorld* world, const GameLib::Math::Vector3& position, float yawDegree, int requiredFuel ) const;
	// 消火栓（見た目の置物。動かない箱として名簿へ載る。使えない・漁れない）。
	GameObject* spawnFireHydrant( GameObjectWorld* world, const GameLib::Math::Vector3& position, float yawDegree ) const;
	// 拾うと消えるガソリン（カバンから捨てたとき）。外せる箱で移動を止める（ユーザー指定）。
	GameObject* spawnFuel( GameObjectWorld* world, const GameLib::Math::Vector3& position, float yawDegree ) const;
	// 地面に落ちている銃（拾うとカバンへ入る）。id: どの銃か。上を歩ける。
	// 弾倉の中身は持たない（銃の物〈自キャラが持てる銃〉が持ち続ける。銃は 1 種類 1 丁だけなので、拾い直すと前の弾数のまま）。
	GameObject* spawnWeaponDrop( GameObjectWorld* world, WeaponId id, const GameLib::Math::Vector3& position, float yawDegree ) const;
	// カバンから捨てた物。落ちているガソリン・落ちた弾・落ちている銃・落ちている包帯と売れる物になり、拾い直せる（GridInventory_Plan.txt Q6）。
	// stack: 捨てた物と数（弾・包帯は数をそのまま持つ。ガソリン・銃・売れる物は 1 つ）。
	GameObject* spawnDroppedItem( GameObjectWorld* world, const ItemStack& stack, const GameLib::Math::Vector3& position, float yawDegree ) const;

private:
	// 落ちている物（ItemSpec::groundFile の形で、上を歩ける）。弾・包帯・売れる物。stack の数をそのまま持つ。
	GameObject* spawnGroundItem( GameObjectWorld* world, const ItemStack& stack, const GameLib::Math::Vector3& position, float yawDegree ) const;
	// 物の種類 item の、落ちた形（ItemSpec::groundFile を持つ物だけ）。
	const StaticMeshLods& groundShapes( ItemId item ) const;
	// 動かない置物の物を作り、見た目と箱の部品まで付けて返す（まだ spawn しない。呼ぶ側が残りの部品を付けて spawn する）。
	// data: 構築済みであること。material: 表面の材質（着弾音）。
	GameObject* createProp( GameObjectWorld* world, GameObjectTag tag, const char* name, const StaticMeshLods& shapes,
		const GameLib::Math::Vector3& position, float yawDegree, DrawCulling::Target cullTarget,
		BoxColliderComponent::Mode colliderMode, GroundMaterial material ) const;

	StaticMeshLods mVehicleShapes;  // どれも MeshCache から借りる。置物の車（細かさの段ごと）
	StaticMeshLods mFuelCanShapes;  // ガソリンの携行缶（細かさの段ごと）
	StaticMeshLods mFireHydrantShapes; // 消火栓（細かさの段ごと）
	StaticMeshLods mDumpsterShapes; // ゴミ箱（1 段だけ。tools/loot_prop_gen.py で作る）
	StaticMeshLods mCrateShapes;    // 木箱（同上）
	// 落ちた形（物の種類ごと。形のファイルは ItemCatalog の groundFile。1 段だけ）。map なのは、groundFile の無い種類の分を持たないため。
	std::map< ItemId, StaticMeshLods > mGroundShapes;
	std::vector< StaticMeshLods > mWeaponDropShapes; // 落ちている銃（WeaponId の順。横に寝かせた形。1 段だけ）
};

#endif
