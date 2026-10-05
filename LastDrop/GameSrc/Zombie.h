#ifndef INCLUDED_ZOMBIE_H
#define INCLUDED_ZOMBIE_H

#include "GameLib/Math/Vector3.h"
#include "SkinnedMeshInstance.h"
#include "ICollisionBody.h"
#include "IHittable.h"
#include "HitboxSet.h"
#include "BodyFootprint.h"

class SkinnedMeshData;

// ゾンビ1体。今は AI が無いので、置いた場所でアニメを再生するだけ。
// 見た目は SkinnedMeshInstance に任せ、体を真上から囲む長方形（BodyFootprint）を差し出して、他のキャラの移動を止める（ICollisionBody）。
// 部位ごとの当たり判定（命中判定用）も持ち、弾がどの部位に当たったかを答えられる（IHittable）。
// 歩かせるとき（AI を足すとき）は、PlayerCharacter と同じく CharacterMover を持たせて移動を任せる。
class Zombie : public ICollisionBody, public IHittable{
public:
	// data: 形・見た目・アニメ。所有権は受け取らない。nullptr 不可、isReady() 済みであること。
	//       このゾンビより長く生きていること（MeshCache が持つ）。
	// position / angleY: 足元の位置と向き（度。0 は +Z）。
	// animation / startTime: 再生するアニメ名と開始時刻（フレーム。体ごとにずらして動きを揃えない）。
	//                        animation が nullptr ならアニメを付けず、レスト姿勢（T ポーズ）のまま立たせる（当たり判定の確認用）。
	Zombie( const SkinnedMeshData* data, const GameLib::Math::Vector3& position, float angleY,
		const char* animation, float startTime );
	Zombie( const Zombie& ) = delete;
	Zombie& operator=( const Zombie& ) = delete;

	// アニメを1フレーム（1ティック）進め、部位の当たり判定をそのポーズへ置き直す。頂点の変形は draw で行う。
	void update();
	void draw() const;

	// 移動の当たり判定（体を真上から囲む長方形。体の向きに合わせて回る）。
	virtual BoxCollider collider() const;
	// 部位ごとの当たり判定。
	virtual const HitboxSet& hitboxes() const;

private:
	SkinnedMeshInstance mMesh;
	BodyFootprint mFootprint;
	HitboxSet mHitboxes;
	GameLib::Math::Vector3 mPosition; // 足元の位置
	float mAngleY;                    // 向き（度）
};

#endif
