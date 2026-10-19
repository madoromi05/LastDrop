#ifndef INCLUDED_COLLISION_WORLD_H
#define INCLUDED_COLLISION_WORLD_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Collision/BoxCollider.h"
#include "GameSrc/Collision/SweepMath.h"
#include "GameSrc/Collision/WallMesh.h"
#include <vector>

class ICollisionBody;
class IGroundHeight;

// 移動の当たり判定の相手の名簿と、それへの問い合わせ（「この長方形をこう動かしたら最初に何に当たるか」）。
// 位置を動かしたり押し戻したりはしない（それは CharacterMover の仕事）。問い合わせるだけ。
// 相手は総当たりで調べる。ゾンビが数百体を超えて重くなったら、この中だけを空間分割に変えればよい（外の呼び方は変わらない）。
// 建物の面のかけら（WallMesh）だけは数が多い（1軒で数百）ので、届かない範囲のものを先に捨ててから調べる。
// 移動は真上から見た 2D で解くが、キャラが乗る地面の高さ（歩道の段差）もここで答える。
class CollisionWorld{
public:
	CollisionWorld();

	// 動かない形を登録する。値をコピーして持つ（動かないので、登録した後に元の持ち主が消えても困らない）。
	void addBox( const BoxCollider& box );
	// 動かない、メッシュの形に沿った当たり判定（建物）を登録する。受け取って持つ（addBox と同じ理由）。
	// かけらの数が多いので、作った物をそのまま渡せば写しを作らずに済む（一時オブジェクトか std::move で渡す）。
	void addWallMesh( WallMesh walls );
	// 動くもの（キャラ）や、途中で消える物（拾える物）を登録する。形は問い合わせのたびに collider() で読む。所有権は受け取らない。nullptr 不可。
	// この世界に問い合わせる間は生きていること（シーンでは、この世界をキャラより先に宣言する＝キャラより後に破棄される）。
	void addBody( const ICollisionBody* body );
	// 登録した動くものを外す（消す前に呼ぶ）。登録されていなければ何もしない。
	void removeBody( const ICollisionBody* body );
	// キャラが乗る地面を登録する（1つだけ。登録し直すと置き換わる）。所有権は受け取らない。nullptr なら地面なし（高さは変えない）。
	// この世界に問い合わせる間は生きていること（シーンでは、地面をこの世界より先に宣言する）。
	void setGround( const IGroundHeight* ground );

	// box を delta だけ動かしたとき、一番早く当たる相手での結果を hit に書く。何にも当たらなければ false。
	// ignore: 調べない相手（動かしている本人）。所有権は受け取らない。nullptr なら全員を調べる。
	// hit: 出力先。nullptr 不可。
	bool sweep( const BoxCollider& box, const GameLib::Math::Vector3& delta, const ICollisionBody* ignore, SweepHit* hit ) const;
	// box が一番深くめり込んでいる相手から、外へ出す移動量を push に書く。どれにもめり込んでいなければ false。
	// ignore: sweep と同じ。push: 出力先。nullptr 不可。
	bool findPenetration( const BoxCollider& box, const ICollisionBody* ignore, GameLib::Math::Vector3* push ) const;
	// position（キャラの足元）の真下の地面の高さを height に書く。地面が登録されていない・足元に面が無ければ false。
	// height: 出力先。nullptr 不可。
	bool groundHeight( const GameLib::Math::Vector3& position, float* height ) const;

	// 登録された形を読む（当たり判定の表示など、問い合わせ以外で中身を見る係のため）。読むだけで、変えられない。
	const std::vector< BoxCollider >& boxes() const;
	const std::vector< WallMesh >& wallMeshes() const;
	const std::vector< const ICollisionBody* >& bodies() const;

private:
	std::vector< BoxCollider > mBoxes;
	std::vector< WallMesh > mWallMeshes;
	std::vector< const ICollisionBody* > mBodies; // 借りているだけ
	const IGroundHeight* mGround;                 // 借りているだけ。nullptr なら地面なし
};

#endif
