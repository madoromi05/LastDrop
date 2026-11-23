#ifndef INCLUDED_COLLISION_WORLD_H
#define INCLUDED_COLLISION_WORLD_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Collision/BoxCollider.h"
#include "GameSrc/Collision/SweepMath.h"
#include "GameSrc/Collision/WallMesh.h"
#include "GameSrc/Hit/GroundGrid.h"
#include <vector>

class ICollisionBody;
class IGroundHeight;

// 移動の当たり判定の相手の名簿と、それへの問い合わせ（「この長方形をこう動かしたら最初に何に当たるか」）
// 位置を動かしたり押し戻したりはしない（それは CharacterMover の仕事）。問い合わせるだけ
// 相手は上から見た 4m のます（GroundGrid）で、届く範囲のますにいる物だけを調べる
// ますは問い合わせの時に、変わっていれば作り直す（動かない物は登録が変わった時、動く体は refreshBodies の後）
// 建物の面のかけら（WallMesh）は数が多い（1軒で数百）ので、ますで絞った軒の中でも、届かない範囲のかけらを先に捨ててから調べる
// 移動は真上から見た 2D で解くが、キャラが乗る地面の高さ（歩道の段差）もここで答える
class CollisionWorld{
public:
	CollisionWorld();

	void addBox( const BoxCollider& box );			// 動かない形を登録する。値をコピーして持つ（動かないので、登録した後に元の持ち主が消えても困らない）
	// 動かない、メッシュの形に沿った当たり判定（建物）を登録する。受け取って持つ（addBox と同じ理由）
	// かけらの数が多いので、作った物をそのまま渡せば写しを作らずに済む（一時オブジェクトか std::move で渡す）
	void addWallMesh( WallMesh walls );
	// 動くもの（キャラ）や、途中で消える物（拾える物）を登録する。形は問い合わせのたびに collider() で読む。所有権は受け取らない。nullptr 不可
	// この世界に問い合わせる間は生きていること（シーンでは、この世界をキャラより先に宣言する＝キャラより後に破棄される）
	void addBody( const ICollisionBody* body );
	void removeBody( const ICollisionBody* body );	// 登録した動くものを外す（消す前に呼ぶ）。登録されていなければ何もしない
	// キャラが乗る地面を登録する（1つだけ。登録し直すと置き換わる）。所有権は受け取らない。nullptr なら地面なし（高さは変えない）
	// この世界に問い合わせる間は生きていること（シーンでは、地面をこの世界より先に宣言する）
	void setGround( const IGroundHeight* ground );
	// 動く体のますを、次の問い合わせで入れ直させる。毎ティックの始め（体が動く前）に 1 回呼ぶ
	// 入れる範囲には 1 ティックに動ける分の余裕を足すので、同じティックの中で体が動いても外れない
	void refreshBodies();

	// box が一番深くめり込んでいる相手から、外へ出す移動量を push に書く。どれにもめり込んでいなければ false
	// ignore: 調べない相手（動かしている本人）。所有権は受け取らない。nullptr なら全員を調べる
	// push: 出力先。nullptr 不可
	bool findPenetration( const BoxCollider& box, const ICollisionBody* ignore, GameLib::Math::Vector3* push ) const;
	// 問い合わせで調べない動くもの（addBody で載せた物）。動かない形・建物は、いつも調べる
	struct BodyFilter{
		const ICollisionBody* self;        // 動かしている本人。nullptr なら除かない
		const ICollisionBody* passThrough; // 本人のほかに通り抜けてよい相手。nullptr なら無し
		bool skipAllBodies;                // true なら動くもの（キャラ・拾える物）を全部調べない
		bool skips( const ICollisionBody* body ) const;
	};
	// box を delta だけ動かしたとき、一番早く当たる相手での結果を hit に書く。何にも当たらなければ false
	// filter: 調べない相手。hit: 出力先。nullptr 不可
	bool sweep( const BoxCollider& box, const GameLib::Math::Vector3& delta, const BodyFilter& filter, SweepHit* hit ) const;
	bool findPenetration( const BoxCollider& box, const BodyFilter& filter, GameLib::Math::Vector3* push ) const;	// 上の findPenetration と同じだが、調べない相手を filter で決める
	// position（キャラの足元）の真下の地面の高さを height に書く。地面が登録されていない・足元に面が無ければ false
	// height: 出力先。nullptr 不可
	bool groundHeight( const GameLib::Math::Vector3& position, float* height ) const;

	// 登録された形を読む（当たり判定の表示など、問い合わせ以外で中身を見る係のため）。読むだけで、変えられない
	const std::vector< BoxCollider >& boxes() const;
	const std::vector< WallMesh >& wallMeshes() const;
	const std::vector< const ICollisionBody* >& bodies() const;

private:
	void updateGrids() const;	// 変わっていれば、ますを作り直す（問い合わせの始めに呼ぶ）。動かない物の登録が変わったら全部、動く体だけなら体のますだけ
	void fillBodyGrid() const;	// 動く体を、今の置き場所でますへ入れ直す

	std::vector< BoxCollider > mBoxes;
	std::vector< WallMesh > mWallMeshes;
	std::vector< const ICollisionBody* > mBodies; // 借りているだけ
	const IGroundHeight* mGround;                 // 借りているだけ。nullptr なら地面なし
	// 届く範囲で引くます。問い合わせ（const）の中で作り直すので mutable。並びの番号は mBoxes・mWallMeshes・mBodies の添え字
	mutable GroundGrid mBoxGrid;
	mutable GroundGrid mWallGrid;
	mutable GroundGrid mBodyGrid;
	mutable bool mStaticGridDirty; // 箱・壁メッシュの登録が変わった（ますの大きさから作り直す）
	mutable bool mBodyGridDirty;   // 動く体の登録か置き場所が変わった（体のますだけ入れ直す）
	mutable std::vector< int > mCandidates; // ますから引いた番号（問い合わせのたびに確保し直さないための作業領域）
};

#endif
