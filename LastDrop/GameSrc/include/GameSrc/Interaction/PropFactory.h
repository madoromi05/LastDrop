#ifndef INCLUDED_PROP_FACTORY_H
#define INCLUDED_PROP_FACTORY_H

#include "GameSrc/World/GameObjectTag.h"
#include "GameSrc/Render/DrawCulling.h"
#include "GameSrc/Component/BoxColliderComponent.h"
#include "GameSrc/Render/StaticMesh/StaticMeshLods.h"
#include "GameLib/Math/Vector3.h"

class MeshCache;
class StaticMeshData;
class GameObject;
class GameObjectWorld;

// 置物（脱出用の車・ガソリンの置き場・落ちているガソリン・ゾンビが落とした弾）を部品から組み立てる
// （Unity のプレハブの代わり。PlayerFactory・ZombieFactory と同じ役。SummaryPlan/個別仕様/GameWorldSplit_Plan.txt の ★5）。
// 部品の並び・形のファイル・名前・材質・中身の量を 1 か所で決める（「何で出来ているか」）。
// どこに置くか・何個置くかは使う側（WorldBuilder：最初に置く物、CombatResolver：倒したゾンビが落とす弾）が決める。
// 形の構築に失敗していたら、どの spawn も置かずに nullptr を返す（理由は MeshCache がデバッグ出力済み。
// 置けなくても他の確認はできる。車が無いとクリアできない）。
class PropFactory{
public:
	// 形の読み込みを頼むだけ（非同期）。meshes: 設計図を借りる倉庫。所有権は受け取らない。nullptr 不可。
	// 作った物より長く生きていること（形を借りるため）。
	explicit PropFactory( MeshCache* meshes );

	// どれも world へ spawn して、その物を返す。position / yawDegree: 足元の位置と向き（度。0 は +Z）。world: nullptr 不可。
	// 脱出用の車（燃料タンク付き。requiredFuel 個入れると走れる）。最後まで置いておくので、動かない箱として名簿へ載る
	// （ゾンビの道の図にも、回り込む角として入る）。
	GameObject* spawnEscapeCar( GameObjectWorld* world, const GameLib::Math::Vector3& position, float yawDegree, int requiredFuel ) const;
	// 何度でも拾えるガソリンの置き場。
	GameObject* spawnFuelStation( GameObjectWorld* world, const GameLib::Math::Vector3& position, float yawDegree ) const;
	// 拾うと消えるガソリン。外せる箱で移動を止める（ユーザー指定）。
	GameObject* spawnFuel( GameObjectWorld* world, const GameLib::Math::Vector3& position, float yawDegree ) const;
	// ゾンビが落とす予備の弾。足元の小さな物でつまずかないよう、移動の名簿には載せない（上を歩ける）。
	GameObject* spawnAmmoDrop( GameObjectWorld* world, const GameLib::Math::Vector3& position, float yawDegree ) const;

private:
	// 動かない置物の物を作り、見た目と箱の部品まで付けて返す（まだ spawn しない。呼ぶ側が残りの部品を付けて spawn する）。
	// data: 構築済みであること。
	GameObject* createProp( GameObjectWorld* world, GameObjectTag tag, const char* name, const StaticMeshLods& shapes,
		const GameLib::Math::Vector3& position, float yawDegree, DrawCulling::Target cullTarget,
		BoxColliderComponent::Mode colliderMode ) const;

	StaticMeshLods mVehicleShapes;  // どれも MeshCache から借りる。脱出用の車（細かさの段ごと）
	StaticMeshLods mFuelCanShapes;  // ガソリンの携行缶（落ちているガソリンと置き場で共通。細かさの段ごと）
	StaticMeshLods mAmmoDropShapes; // ゾンビが落とす予備の弾（7.62 NATO の弾薬を大きくして寝かせた物。1 段だけ）
};

#endif
